#pragma once
#ifndef __FRAMERESOURCE__H__
#define __FRAMERESOURCE__H__

#include "Pubh.h"
#include "UploadBuffer.h"

namespace DSM {

	// 帧资源
	struct FrameResource
	{
		FrameResource(ID3D12Device* device);
		FrameResource(const FrameResource& other) = delete;
		FrameResource& operator=(const FrameResource& other) = delete;
		~FrameResource() = default;

		// 添加常量缓冲区
		void AddConstantBuffer(
			UINT byteSize,
			UINT elementSize,
			const std::string& bufferName);
		void AddConstantBuffer(const std::string& name, ID3D12Resource* buffer);
		void AddDynamicBuffer(
			UINT byteSize,
			UINT elementSize,
			const std::string& bufferName);

			
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		ComPtr<ID3D12Device> m_Device;
		ComPtr<ID3D12CommandAllocator> m_CmdListAlloc;						// 每个帧资源都有一个命令列表分配器
		
		std::unordered_map<std::string, ComPtr<ID3D12Resource>> m_Buffers;	// 缓冲区资源
		UINT64 m_Fence = 0;													// 当前帧资源的围栏值

	private:
		void AddBuffer(
			UINT byteSize,
			UINT elementSize,
			const std::string& bufferName,
			bool isConstant);
	};
}


#endif // !__FRAMERESOURCE__H__
