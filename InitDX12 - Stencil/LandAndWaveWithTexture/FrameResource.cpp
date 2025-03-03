#include "FrameResource.h"

namespace DSM {
	FrameResource::FrameResource(ID3D12Device* device)
		: m_Device(device){
		// 创建命令队列分配器
		ThrowIfFailed(m_Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_CmdListAlloc.GetAddressOf())));
	}

	void FrameResource::AddConstantBuffer(
		UINT byteSize,
		UINT elementSize,
		const std::string& bufferName) {
		AddUploadBuffer(byteSize, elementSize, bufferName, true);
	}

	void FrameResource::AddConstantBuffer(const std::string& name, ID3D12Resource* buffer)
	{
		m_Buffers[name] = buffer;	
	}

	void FrameResource::AddDynamicBuffer(
		UINT byteSize,
		UINT elementSize,
		const std::string& bufferName) {
		AddUploadBuffer(byteSize, elementSize, bufferName, false);
	}

	void FrameResource::AddUploadBuffer(
		UINT byteSize,
		UINT elementSize,
		const std::string& bufferName,
		bool isConstant)
	{
		ComPtr<ID3D12Resource> pResource;
		
		UINT elementByteSize = isConstant ? D3DUtil::CalcCBByteSize(byteSize) : byteSize;
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC bufferDesc{};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = elementByteSize * elementSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.SampleDesc = {1,0};
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		
		ThrowIfFailed(m_Device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pResource.GetAddressOf())));

		m_Buffers[bufferName] = pResource;
	}
}
