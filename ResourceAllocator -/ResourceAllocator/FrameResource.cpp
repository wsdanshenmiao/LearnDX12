#include "FrameResource.h"

namespace DSM {
	FrameResource::FrameResource(ID3D12Device* device)
		: m_Device(device) {
		// 创建命令队列分配器
		ThrowIfFailed(m_Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_CmdListAlloc.GetAddressOf())));

		m_DefaultBufferAllocator = std::make_unique<D3D12DefaultBufferAllocator>(m_Device.Get());
		m_UploadBufferAllocator = std::make_unique<D3D12UploadBufferAllocator>(m_Device.Get());
	}

	void FrameResource::AddConstantBuffer(
		UINT byteSize,
		UINT elementSize,
		const std::string& bufferName) {
		AddUploadBuffer(byteSize, elementSize, bufferName, true);
	}

	void FrameResource::AddDynamicBuffer(
		UINT byteSize,
		UINT elementSize,
		const std::string& bufferName) {
		AddUploadBuffer(byteSize, elementSize, bufferName, false);
	}

	void FrameResource::Clear()
	{
		
	}

	void FrameResource::AddUploadBuffer(
		UINT elementByteSize,
		UINT elementSize,
		const std::string& bufferName,
		bool isConstant)
	{
		auto byteSize = elementByteSize * elementSize;
		auto alignment = isConstant ? 256 : 0;

		D3D12ResourceLocation resourceLocation;
		m_UploadBufferAllocator->AllocateUploadBuffer(byteSize, alignment, resourceLocation);

		m_Resources[bufferName] = std::move(resourceLocation);
	}
}
