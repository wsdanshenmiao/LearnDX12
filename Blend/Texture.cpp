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
			D3D12_SUBRESOURCE_DATA subResourceData;
			subResourceData.pData = imgData;
			subResourceData.RowPitch = width * sizeof(uint32_t);
			subResourceData.SlicePitch = 0;
			subresources.emplace_back(std::move(subResourceData));

			D3D12_HEAP_PROPERTIES texHeapDesc{};
			texHeapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
			D3D12_RESOURCE_DESC texResDesc{};
			texResDesc.Alignment = 0;
			texResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			texResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
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

		// 获取上传堆的大小
		UINT64 uploadBufferSize;
		auto texDesc = texture.m_Texture->GetDesc();
		texture.m_Texture->GetDevice(IID_PPV_ARGS(&device));
		device->GetCopyableFootprints(&texDesc, 0, subresources.size(), 0, nullptr, nullptr, nullptr, &uploadBufferSize);

		// 创建GPU上传堆
		D3D12_HEAP_PROPERTIES uploadHeapProps{};
		uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC uploadResDesc{};
		uploadResDesc.Alignment = 0;
		uploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		uploadResDesc.Format = DXGI_FORMAT_UNKNOWN;
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

		UpdateSubresources(
			cmdList, texture.m_Texture.Get(), texture.m_UploadHeap.Get(),
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition = {
			texture.m_Texture.Get(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		};
		cmdList->ResourceBarrier(1, &barrier);

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
			texResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			texResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			texResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
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

		// 获取上传堆的大小
		UINT64 uploadBufferSize;
		auto texDesc = texture.m_Texture->GetDesc();
		texture.m_Texture->GetDevice(IID_PPV_ARGS(&device));
		device->GetCopyableFootprints(&texDesc, 0, subresources.size(), 0, nullptr, nullptr, nullptr, &uploadBufferSize);


		// 创建GPU上传堆
		D3D12_HEAP_PROPERTIES uploadHeapProps{};
		uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC uploadResDesc{};
		uploadResDesc.Alignment = 0;
		uploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		uploadResDesc.Format = DXGI_FORMAT_UNKNOWN;
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

		UpdateSubresources(
			cmdList, texture.m_Texture.Get(), texture.m_UploadHeap.Get(),
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition = {
			texture.m_Texture.Get(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		};
		cmdList->ResourceBarrier(1, &barrier);

		if (imgData != nullptr) {
			stbi_image_free(imgData);
		}

		return true;
	}
}
