#pragma once
#ifndef __D3D12RESOURCE__H__
#define __D3D12RESOURCE__H__
#include <wrl/client.h>

#include "D3DUtil.h"

namespace DSM {
	class D3D12BuddyAllocator;

	struct D3D12Resource
	{
		explicit D3D12Resource(
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			D3D12_RESOURCE_STATES inintState = D3D12_RESOURCE_STATE_COMMON);

		void* Map();
		void Unmap();

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
		D3D12_RESOURCE_STATES m_ResourceState{};
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress{};
		// 上传堆使用
		void* m_MappedBaseAddress = nullptr;
	};

	struct D3D12BuddyBlockData
	{
		std::uint32_t m_Offset;         // 偏移量
		std::uint32_t m_Order;          // 层级
		std::uint32_t m_ActualUseSize;  // 实际使用的大小
		std::shared_ptr<D3D12Resource> m_PlacedResource;      // 资源为 PlacedResource 时使用
	};

	// 用于访问资源并定位资源的位置
	struct D3D12ResourceLocation
	{
		enum class ResourceLocationType
		{
			Undefined = 0,
			StandAlone,
			SubAllocation
		};

		ResourceLocationType m_ResourceLocationType = ResourceLocationType::Undefined;

		// SubAllocation Resource 子分配资源
		D3D12BuddyBlockData m_BlockData;
		D3D12BuddyAllocator* m_Allocator = nullptr;
		// StandAlone 独立的资源,或是 SubAllocation 的父资源, 若是在自定义堆上分配则为空
		D3D12Resource* m_UnderlyingResource = nullptr;

		// 提供给 SubAllocation 的偏移索引
		union
		{
			std::uint64_t m_OffsetFromBaseOfResource;
			std::uint64_t m_OffsetFromBaseOfHeap;
		};

		D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress{};
		void* m_MappedBaseAddress = nullptr;
	};
}

#endif