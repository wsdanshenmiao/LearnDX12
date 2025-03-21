#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"

#include "D3D12Allocatioin.h"
#include "D3DUtil.h"
#include "DDSTextureLoader12.h"
#include "stb_image.h"

using namespace DirectX;

namespace DSM {
	Texture::Texture(const std::string& name) noexcept
		:m_Name(name) {
	}

	Texture::Texture(
		const std::string& name,
		const std::string& fileName,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		D3D12TextureAllocator* texAllocator,
		D3D12UploadBufferAllocator* uploadAllocator)
		:m_Name(name) {
		assert(device != nullptr);
		assert(cmdList != nullptr);
		assert(texAllocator != nullptr);
		assert(uploadAllocator != nullptr);
		LoadTextureFromFile(*this, fileName, device, cmdList, texAllocator, uploadAllocator);
	}

	const std::string& Texture::GetName() const noexcept
	{
		return m_Name;
	}

	UINT Texture::GetDescriptorIndex() const noexcept
	{
		return m_DescriptorIndex;
	}

	D3D12ResourceLocation& Texture::GetTexture()
	{
		return m_Texture.m_ResourceLocation;
	}

	const D3D12ResourceLocation& Texture::GetTexture() const
	{
		return m_Texture.m_ResourceLocation;
	}

	D3D12DescriptorHandle Texture::GetSRV() const
	{
		return m_Texture.m_SrvHandle;
	}

	D3D12DescriptorHandle Texture::GetRTV() const
	{
		return m_Texture.m_RtvHandle;
	}

	void Texture::SetName(const std::string& name)
	{
		m_Name = name;
	}

	void Texture::SetTexture(const D3D12ResourceLocation& texture)
	{
		m_Texture.m_ResourceLocation = texture;
	}

	void Texture::SetUploadHeap(const D3D12ResourceLocation& uploadHeap)
	{
		m_UploadHeap = uploadHeap;
	}

	void Texture::SetDescriptorIndex(UINT index) noexcept
	{
		m_DescriptorIndex = index;
	}

	void Texture::SetSRVHandle(D3D12DescriptorHandle descriptor) noexcept
	{
		m_Texture.m_SrvHandle = descriptor;
	}

	void Texture::SetRTVHandle(D3D12DescriptorHandle descriptor) noexcept
	{
		m_Texture.m_RtvHandle = descriptor;
	}

	void Texture::DisposeUploader() noexcept
	{
		m_UploadHeap.m_Allocator->ClearUpAllocations();
	}

	bool Texture::LoadTextureFromFile(
		Texture& texture,
		const std::string& fileName,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		D3D12TextureAllocator* texAllocator,
		D3D12UploadBufferAllocator* uploadAllocator)
	{
		assert(device != nullptr);
		assert(cmdList != nullptr);
		assert(texAllocator != nullptr);
		assert(uploadAllocator != nullptr);
		
		texture.SetName(fileName);

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		stbi_uc* imgData = nullptr;
		auto wstr = AnsiToWString(fileName);
		int height, width, comp;

		D3D12_RESOURCE_DESC textureDesc{};
		// 若使用dds加载失败则使用stbimage
		if (FAILED(LoadDDSTextureFromFile(
				device,
				wstr.c_str(),
				textureDesc,
				ddsData, subresources))) {
			imgData = stbi_load(fileName.c_str(), &width, &height, &comp, STBI_rgb_alpha);
			if (imgData == nullptr)return false;

			subresources.clear();
			subresources.emplace_back(imgData,width * sizeof(uint32_t),0);
			
			textureDesc.Alignment = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = width;
			textureDesc.Height = height;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc = { 1,0 };
			textureDesc.MipLevels = 1;
		}

		texAllocator->AllocateTexture(textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, texture.m_Texture.m_ResourceLocation);
		
		LoadTexture(texture, subresources, device, cmdList, uploadAllocator);

		if (imgData != nullptr) {
			stbi_image_free(imgData);
		}

		return true;
	}

	bool Texture::LoadTextureFromFile(
		Texture& texture,
		const std::string& name,
		const std::string& fileName,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		D3D12TextureAllocator* texAllocator,
		D3D12UploadBufferAllocator* uploadAllocator)
	{
		bool success = LoadTextureFromFile(texture, fileName, device, cmdList, texAllocator, uploadAllocator);
		texture.SetName(name);
		return success;
	}

	bool Texture::LoadTextureFromMemory(
		Texture& texture,
		const std::string& name,
		void* data,
		size_t dataSize,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		D3D12TextureAllocator* texAllocator,
		D3D12UploadBufferAllocator* uploadAllocator)
	{
		assert(device != nullptr);
		assert(cmdList != nullptr);
		assert(texAllocator != nullptr);
		assert(uploadAllocator != nullptr);
		
		texture.SetName(name);

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		stbi_uc* imgData = nullptr;

		D3D12_RESOURCE_DESC textureDesc{};
		// 若使用dds加载失败则使用stbimage
		if (FAILED(LoadDDSTextureFromMemory(
				device,
				reinterpret_cast<std::uint8_t*>(data),
				dataSize,
				textureDesc,
				subresources))) {
			int height, width, comp;
			imgData = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(data), (int)dataSize, &width, &height, &comp, STBI_rgb_alpha);
			if (imgData == nullptr)return false;

			subresources.clear();
			subresources.emplace_back(imgData, width * sizeof(std::uint32_t), 0);
			
			textureDesc.Alignment = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = width;
			textureDesc.Height = height;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc = { 1,0 };
			textureDesc.MipLevels = 1;
		}

		texAllocator->AllocateTexture(textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, texture.m_Texture.m_ResourceLocation);
		
		LoadTexture(texture, subresources, device, cmdList, uploadAllocator);

		if (imgData != nullptr) {
			stbi_image_free(imgData);
		}

		return true;
	}

	void Texture::LoadTexture(
		Texture& texture,
		const std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		D3D12UploadBufferAllocator* uploadAllocator)
	{
		// 获取拷贝信息
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprint(subresources.size());	// 子资源的宽高偏移等信息
		std::vector<UINT> numRows(subresources.size());	// 子资源的行数
		std::vector<UINT64> rowByteSize(subresources.size());	// 子资源每一行的字节大小
		UINT64 uploadBufferSize;	// 整个纹理数据的大小
		auto texDesc = texture.m_Texture.m_ResourceLocation.m_UnderlyingResource->m_Resource->GetDesc();
		device->GetCopyableFootprints(
			&texDesc, 0,
			subresources.size(), 0,
			footprint.data(), numRows.data(),
			rowByteSize.data(), &uploadBufferSize);

		// 创建GPU上传堆
		uploadAllocator->AllocateUploadBuffer(uploadBufferSize, 0, texture.m_UploadHeap);

		BYTE* mappedData = static_cast<BYTE*>(texture.m_UploadHeap.m_MappedBaseAddress);
		// 每一个子资源
		for (std::size_t i = 0; i < subresources.size(); i++) {
			auto destData = mappedData + footprint[i].Offset;	// 每个子资源的偏移
			// 每一个深度
			for (std::size_t z = 0; z < footprint[i].Footprint.Depth; z++) {
				// 每一行
				auto sliceSize = footprint[i].Footprint.RowPitch * numRows[i] * z;
				for (std::size_t y = 0; y < numRows[i]; y++) {
					auto rowSize = y * subresources[i].RowPitch;
					memcpy(destData + sliceSize + rowSize,
						static_cast<const BYTE*>(subresources[i].pData) + sliceSize + rowSize,
						rowByteSize[i]);
				}
			}
		}

		// 拷贝所有子资源
		for (std::size_t i = 0; i < subresources.size(); i++) {
			D3D12_TEXTURE_COPY_LOCATION dest{};
			dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dest.SubresourceIndex = i;
			dest.pResource = texture.m_Texture.m_ResourceLocation.m_BlockData.m_PlacedResource->m_Resource.Get();

			auto baseOffset = texture.m_UploadHeap.m_OffsetFromBaseOfResource;
			D3D12_TEXTURE_COPY_LOCATION src{};
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = footprint[i];
			src.PlacedFootprint.Offset += baseOffset;
			src.pResource = texture.m_UploadHeap.m_UnderlyingResource->m_Resource.Get();
			cmdList->CopyTextureRegion(&dest,0,0,0,&src,nullptr);
		}
		
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition = {
			texture.m_Texture.m_ResourceLocation.m_BlockData.m_PlacedResource->m_Resource.Get(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		};
		cmdList->ResourceBarrier(1, &barrier);
	}
}
