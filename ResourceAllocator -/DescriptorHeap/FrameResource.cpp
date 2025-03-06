#include "FrameResource.h"
#include "D3D12DescriptorHeap.h"

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

	void FrameResource::ClearUp(std::uint64_t fenceValue)
	{
		// 若当前的围栏值大于该帧缓冲区的围栏值，则可释放帧资源内部数据
		if (fenceValue >= m_Fence) {
			m_DefaultBufferAllocator->ClearUpAllocations();
			m_UploadBufferAllocator->ClearUpAllocations();
		}
	}

	void FrameResource::AddUploadBuffer(
		UINT elementByteSize,
		UINT elementSize,
		const std::string& bufferName,
		bool isConstant)
	{
		auto byteSize = elementByteSize * elementSize;
		auto alignment = isConstant ? 256 : 0;

		auto resourceLocation = std::make_shared<D3D12ResourceLocation>();
		m_UploadBufferAllocator->AllocateUploadBuffer(byteSize, alignment, *resourceLocation);

		m_Resources[bufferName] = resourceLocation;
	}
}
