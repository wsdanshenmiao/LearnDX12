#pragma once
#ifndef __FRAMERESOURCE__H__
#define __FRAMERESOURCE__H__

#include "Pubh.h"
#include "D3D12Allocatioin.h"

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

		void Clear();


		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		ComPtr<ID3D12Device> m_Device;
		ComPtr<ID3D12CommandAllocator> m_CmdListAlloc;						// 每个帧资源都有一个命令列表分配器

		std::unique_ptr<D3D12DefaultBufferAllocator> m_DefaultBufferAllocator;
		std::unique_ptr<D3D12UploadBufferAllocator> m_UploadBufferAllocator;
		std::unordered_map<std::string, D3D12ResourceLocation> m_Resources;

		UINT64 m_Fence = 0;													// 当前帧资源的围栏值

	private:
		void AddUploadBuffer(
			UINT elementByteSize,
			UINT elementSize,
			const std::string& bufferName,
			bool isConstant);
	};
}


#endif // !__FRAMERESOURCE__H__
