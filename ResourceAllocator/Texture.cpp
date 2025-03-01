#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
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
		ID3D12GraphicsCommandList* cmdList)
		:m_Name(name) {
		LoadTextureFromFile(*this, fileName, device, cmdList);
	}

	const std::string& Texture::GetName() const noexcept
	{
		return m_Name;
	}

	UINT Texture::GetDescriptorIndex() const noexcept
	{
		return m_DescriptorIndex;
	}

	ID3D12Resource* Texture::GetTexture()
	{
		return m_Texture.Get();
	}

	const ID3D12Resource* Texture::GetTexture() const
	{
		return m_Texture.Get();
	}

	void Texture::SetName(const std::string& name)
	{
		m_Name = name;
	}

	void Texture::SetTexture(ID3D12Resource* texture)
	{
		m_Texture = texture;
	}

	void Texture::SetDescriptorIndex(UINT index) noexcept
	{
		m_DescriptorIndex = index;
	}

	void Texture::DisposeUploader() noexcept
	{
		m_UploadHeap = nullptr;
	}

	bool Texture::LoadTextureFromFile(
		Texture& texture,
		const std::string& fileName,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList)
	{
		texture.m_Texture = nullptr;
		texture.m_UploadHeap = nullptr;
		texture.SetName(fileName);

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		stbi_uc* imgData = nullptr;
		auto wstr = AnsiToWString(fileName);
		// 若使用dds加载失败则使用stbimage
		if (FAILED(LoadDDSTextureFromFile(
			device,
			wstr.c_str(),
			texture.m_Texture.GetAddressOf(),
			ddsData, subresources))) {
			int height, width, comp;
			imgData = stbi_load(fileName.c_str(), &width, &height, &comp, STBI_rgb_alpha);
			if (imgData == nullptr)return false;

			subresources.clear();
			subresources.emplace_back(imgData,width * sizeof(uint32_t),0);

			D3D12_HEAP_PROPERTIES texHeapDesc{};
			texHeapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
			D3D12_RESOURCE_DESC texResDesc{};
			texResDesc.Alignment = 0;
			texResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texResDesc.Width = width;
			texResDesc.Height = height;
			texResDesc.DepthOrArraySize = 1;
			texResDesc.SampleDesc = { 1,0 };
			texResDesc.MipLevels = 1;
			ThrowIfFailed(device->CreateCommittedResource(
				&texHeapDesc,
				D3D12_HEAP_FLAG_NONE,
				&texResDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(texture.m_Texture.GetAddressOf())));
		}

		LoadTexture(texture, subresources, device, cmdList);

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
		ID3D12GraphicsCommandList* cmdList)
	{
		bool success = LoadTextureFromFile(texture, fileName, device, cmdList);
		texture.SetName(name);
		return success;
	}

	bool Texture::LoadTextureFromMemory(
		Texture& texture,
		const std::string& name,
		void* data,
		size_t dataSize,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList)
	{
		texture.m_Texture = nullptr;
		texture.m_UploadHeap = nullptr;
		texture.SetName(name);

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		stbi_uc* imgData = nullptr;
		// 若使用dds加载失败则使用stbimage
		if (FAILED(LoadDDSTextureFromMemory(
			device,
			reinterpret_cast<std::uint8_t*>(data),
			dataSize,
			texture.m_Texture.GetAddressOf(),
			subresources))) {
			int height, width, comp;
			imgData = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(data), (int)dataSize, &width, &height, &comp, STBI_rgb_alpha);
			if (imgData == nullptr)return false;

			subresources.clear();
			subresources.emplace_back(imgData, width * sizeof(std::uint32_t), 0);
			D3D12_HEAP_PROPERTIES texHeapDesc{};
			texHeapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
			D3D12_RESOURCE_DESC texResDesc{};
			texResDesc.Alignment = 0;
			texResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texResDesc.Width = width;
			texResDesc.Height = height;
			texResDesc.DepthOrArraySize = 1;
			texResDesc.SampleDesc = { 1,0 };
			texResDesc.MipLevels = 1;
			ThrowIfFailed(device->CreateCommittedResource(
				&texHeapDesc,
				D3D12_HEAP_FLAG_NONE,
				&texResDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(texture.m_Texture.GetAddressOf())));
		}

		LoadTexture(texture, subresources, device, cmdList);

		if (imgData != nullptr) {
			stbi_image_free(imgData);
		}

		return true;
	}

	void Texture::LoadTexture(
		Texture& texture,
		const std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList)
	{
		// 获取拷贝信息
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprint(subresources.size());	// 子资源的宽高偏移等信息
		std::vector<UINT> numRows(subresources.size());	// 子资源的行数
		std::vector<UINT64> rowByteSize(subresources.size());	// 子资源每一行的字节大小
		UINT64 uploadBufferSize;	// 整个纹理数据的大小
		auto texDesc = texture.m_Texture->GetDesc();
		device->GetCopyableFootprints(
			&texDesc, 0,
			subresources.size(), 0,
			footprint.data(), numRows.data(),
			rowByteSize.data(), &uploadBufferSize);

		// 创建GPU上传堆
		D3D12_HEAP_PROPERTIES uploadHeapProps{};
		uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC uploadResDesc{};
		uploadResDesc.Alignment = 0;
		uploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		uploadResDesc.Width = uploadBufferSize;
		uploadResDesc.Height = 1;
		uploadResDesc.DepthOrArraySize = 1;
		uploadResDesc.SampleDesc = { 1,0 };
		uploadResDesc.MipLevels = 1;

		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&uploadResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(texture.m_UploadHeap.GetAddressOf())));


		BYTE* mappedData = nullptr;
		ThrowIfFailed(texture.m_UploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
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
		texture.m_UploadHeap->Unmap(0, nullptr);

		// 拷贝所有子资源
		for (std::size_t i = 0; i < subresources.size(); i++) {
			D3D12_TEXTURE_COPY_LOCATION dest{};
			dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dest.SubresourceIndex = i;
			dest.pResource = texture.m_Texture.Get();
			D3D12_TEXTURE_COPY_LOCATION src{};
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = footprint[i];
			src.pResource = texture.m_UploadHeap.Get();
			cmdList->CopyTextureRegion(&dest,0,0,0,&src,nullptr);
		}
		
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition = {
			texture.m_Texture.Get(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		};
		cmdList->ResourceBarrier(1, &barrier);
	}
}
