#pragma once
#ifndef __FRAMERESOURCE__H__
#define __FRAMERESOURCE__H__

#include "Pubh.h"
#include "UploadBuffer.h"

namespace DSM {

	// 帧资源
	struct FrameResource
	{
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using PConstantBuffer = std::unique_ptr<UploadBuffer<void>>;

		FrameResource(ID3D12Device* device);
		FrameResource(const FrameResource& other) = delete;
		FrameResource& operator=(const FrameResource& other) = delete;
		~FrameResource() = default;

		// 添加常量缓冲区
		void AddConstantBuffer(
			ID3D12Device* device,
			UINT byteSize,
			UINT elementSize,
			const std::string& bufferName);
		void AddConstantBuffer(std::pair<std::string, PConstantBuffer>&& buffer);
		void AddDynamicBuffer(
			ID3D12Device* device,
			UINT byteSize,
			UINT elementSize,
			const std::string& bufferName);

		ComPtr<ID3D12CommandAllocator> m_CmdListAlloc;						// 每个帧资源都有一个命令列表分配器
		std::unordered_map<std::string, PConstantBuffer> m_ConstantBuffers;	// 常量缓冲区
		UINT64 m_Fence = 0;													// 当前帧资源的围栏值
	};
}


#endif // !__FRAMERESOURCE__H__
