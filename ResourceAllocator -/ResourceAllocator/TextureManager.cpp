#include "TextureManager.h"
#include "D3DUtil.h"


namespace DSM {
	const Texture* TextureManager::LoadTextureFromFile(
		const std::string& fileName,
		ID3D12GraphicsCommandList* cmdList)
	{
		return LoadTextureFromFile(fileName, fileName, cmdList);
	}

	const Texture* TextureManager::LoadTextureFromFile(
		const std::string& name,
		const std::string& fileName,
		ID3D12GraphicsCommandList* cmdList)
	{
		if (m_Textures.contains(fileName)) return nullptr;

		Texture tex{};
		bool sucess = Texture::LoadTextureFromFile(
			tex, name, fileName,
			m_Device.Get(), cmdList,
			m_TextureAllocator.get(),
			m_UploadBufferAllocator.get());
		if (sucess) {
			tex.SetDescriptorIndex(m_Textures.size());
			m_Textures[name] = std::move(tex);
		}
		return sucess ? &m_Textures[name] : nullptr;
	}

	const Texture* TextureManager::LoadTextureFromMemory(
		const std::string& name,
		void* data, size_t dataSize,
		ID3D12GraphicsCommandList* cmdList)
	{
		if (m_Textures.contains(name)) return nullptr;

		Texture tex{};
		bool sucess = Texture::LoadTextureFromMemory(
			tex, name, data, dataSize,
			m_Device.Get(), cmdList,
			m_TextureAllocator.get(),
			m_UploadBufferAllocator.get());
		if (sucess) {
			tex.SetDescriptorIndex(m_Textures.size());
			m_Textures[name] = std::move(tex);
		}
		return sucess ? &m_Textures[name] : nullptr;
	}

	bool TextureManager::AddTexture(const std::string& name, Texture&& texture)
	{
		if (m_Textures.contains(name)) {
			return false;
		}
		else {
			texture.SetDescriptorIndex(m_Textures.size());
			m_Textures[name] = std::move(texture);
			return true;
		}
	}

	size_t TextureManager::GetTextureSize() const noexcept
	{
		return m_Textures.size();
	}

	const std::unordered_map<std::string, Texture>& TextureManager::GetAllTextures() noexcept
	{
		return m_Textures;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureResourceView(
		const std::string& texName,
		UINT descriptorSize) const
	{
		auto handle = m_TexDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		// 若是不包含该纹理则返回第一个纹理
		if (m_Textures.contains(texName)) {
			handle.ptr += m_Textures.find(texName)->second.GetDescriptorIndex() * descriptorSize;
		}
		else {
			handle.ptr += m_Textures.find("DefaultTexture")->second.GetDescriptorIndex() * descriptorSize;
		}
		return handle;
	}

	ID3D12DescriptorHeap* TextureManager::GetDescriptorHeap() const
	{
		return  m_TexDescriptorHeap.Get();
	}

	void TextureManager::CreateTexDescriptor(UINT descriptorSize)
	{
		// 创建描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NumDescriptors = m_Textures.size();
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_TexDescriptorHeap.ReleaseAndGetAddressOf()));

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0;
		for (auto& [name, tex] : m_Textures) {
			auto& texResource = tex.GetTexture().m_UnderlyingResource->m_Resource;
			SRVDesc.Format = texResource->GetDesc().Format;
			SRVDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;

			auto handle=m_TexDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			handle.ptr += tex.GetDescriptorIndex() * descriptorSize;
			
			m_Device->CreateShaderResourceView(texResource.Get() ,&SRVDesc,handle);
		}
	}

	TextureManager::TextureManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
		:m_Device(device) {
		m_TextureAllocator = std::make_unique<D3D12TextureAllocator>(m_Device.Get());
		m_UploadBufferAllocator = std::make_unique<D3D12UploadBufferAllocator>(m_Device.Get());
		
		// 创建一个空白纹理，用来处理模型没有纹理的情况
		std::uint32_t white = (std::uint32_t) - 1;

		D3D12ResourceLocation texResource{};
		D3D12ResourceLocation uploadHeap{};

		D3D12_RESOURCE_DESC texDesc{};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Width = 1;
		texDesc.Height = 1;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.SampleDesc = {1,0};
		
		m_TextureAllocator->AllocateTexture(texDesc, D3D12_RESOURCE_STATE_COPY_DEST, texResource);
		
		auto desc = texResource.m_UnderlyingResource->m_Resource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT   textureRowNum = 0u;
		UINT64 textureRowSizes = 0u;
		UINT64 totalSize = 0u;
		m_Device->GetCopyableFootprints(
			&desc,
			0,1,0,
			&footprint,
			&textureRowNum,
			&textureRowSizes,
			& totalSize);

		m_UploadBufferAllocator->AllocateUploadBuffer(totalSize, 0, uploadHeap);

		BYTE* mappedData = static_cast<BYTE*>(uploadHeap.m_MappedBaseAddress);
		memcpy(mappedData, &white, sizeof(white));
		mappedData = nullptr;

		
		//向命令队列发出从上传堆复制纹理数据到默认堆的命令
		D3D12_TEXTURE_COPY_LOCATION stDst = {};
		stDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		stDst.pResource = texResource.m_UnderlyingResource->m_Resource.Get();
		stDst.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION stSrc = {};
		stSrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		stSrc.pResource = uploadHeap.m_UnderlyingResource->m_Resource.Get();
		stSrc.PlacedFootprint = footprint;
		stDst.PlacedFootprint.Offset += uploadHeap.m_OffsetFromBaseOfResource;
		cmdList->CopyTextureRegion(&stDst, 0, 0, 0, &stSrc, nullptr);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition = {
			texResource.m_UnderlyingResource->m_Resource.Get(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		};
		cmdList->ResourceBarrier(1, &barrier);

		Texture whiteTexture{};
		whiteTexture.SetTexture(texResource);
		whiteTexture.SetUploadHeap(uploadHeap);
		whiteTexture.SetName("DefaultTexture");
		whiteTexture.SetDescriptorIndex(m_Textures.size());
		m_Textures[whiteTexture.GetName()] = whiteTexture;
	}

}
