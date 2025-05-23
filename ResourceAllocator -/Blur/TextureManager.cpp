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
			CreateSRV(tex);
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
			CreateSRV(tex);
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

	size_t TextureManager::GetTextureCount() const noexcept
	{
		return m_Textures.size();
	}

	const std::unordered_map<std::string, Texture>& TextureManager::GetAllTextures() noexcept
	{
		return m_Textures;
	}

	D3D12DescriptorHandle TextureManager::GetTextureResourceView(const std::string& texName) const
	{
		auto defaultTex = "DefaultTexture";
		// 若是不包含该纹理则返回第一个纹理
		if (m_Textures.contains(texName)) {
			return m_Textures.find(texName)->second.GetSRV();
		}
		else {
			return m_Textures.find(defaultTex)->second.GetSRV();
		}
	}

	D3D12DescriptorHandle TextureManager::GetDefaultTextureResourceView() const
	{
		return m_Textures.find("DefaultTexture")->second.GetSRV();
	}

	TextureManager::TextureManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
		:m_Device(device), m_DescriptorHeap(std::make_unique<D3D12DescriptorHeap>(device)) {
		m_DescriptorHeap->Create(L"TextureShaderResourceView", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512);
		
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
		CreateSRV(whiteTexture);
		m_Textures[whiteTexture.GetName()] = std::move(whiteTexture);
	}

	void TextureManager::CreateSRV(Texture& texture)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		auto& texResource = texture.GetTexture().m_UnderlyingResource->m_Resource;
		SRVDesc.Format = texResource->GetDesc().Format;
		SRVDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = texResource->GetDesc().DepthOrArraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;
		if (SRVDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY) {
			SRVDesc.Texture2DArray.MostDetailedMip = 0;
			SRVDesc.Texture2DArray.MipLevels = -1;
			SRVDesc.Texture2DArray.FirstArraySlice = 0;
			SRVDesc.Texture2DArray.ArraySize = texResource->GetDesc().DepthOrArraySize;
		}
		else {
			SRVDesc.Texture2D.MostDetailedMip = 0;
			SRVDesc.Texture2D.ResourceMinLODClamp = 0;
		}
		auto handle = m_DescriptorHeap->Allocate();
		texture.SetSRVHandle(handle);
		m_Device->CreateShaderResourceView(
			texture.GetTexture().m_UnderlyingResource->m_Resource.Get(), &SRVDesc, handle);
	}
}
