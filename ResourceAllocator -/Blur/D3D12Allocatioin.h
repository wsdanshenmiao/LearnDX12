#pragma once
#ifndef __D3D12__ALLOCATOR__H__
#define __D3D12__ALLOCATOR__H__

#include <queue>
#include <set>
#include "D3D12Resource.h"

namespace DSM {
	// 使用 Buddy System 的显存管理
	class D3D12BuddyAllocator
	{
	public:
		// 两种分配策略
		// PlacedResource 先创建堆，后从堆中创建资源。
		// ManualSubAllocation，创建一个 ID3D12Resource 并从中划分需要的资源
		enum class AllocationStrategy
		{
			PlacedResource,
			ManualSubAllocation
		};

		struct AllocatorInitData
		{
			AllocationStrategy m_Strategy;
			D3D12_HEAP_TYPE m_HeapType;
			D3D12_HEAP_FLAGS m_HeapFlags;           // 分配策略为 PlacedResource 时使用
			D3D12_RESOURCE_FLAGS m_ResourceFlags;   // 分配策略为 ManualSubAllocation 时使用
		};

	public:
		D3D12BuddyAllocator(ID3D12Device* device,
			const AllocatorInitData& initData,
			std::size_t minBlockSize = 256,
			std::size_t maxBlockSize = DefaultPoolSize);
		~D3D12BuddyAllocator();

		// 分配内存
		bool Allocate(std::uint32_t size, std::uint32_t alignment, D3D12ResourceLocation& resourceLocation);
		// 释放内存
		void Deallocate(D3D12ResourceLocation& resourceLocation);
		void ClearUpAllocations();
		ID3D12Heap* GetHeap() const;
		AllocationStrategy GetAllocationStrategy() const;

	private:
		// 实际大小获取单位大小
		std::size_t SizeToUnitSize(std::size_t size);
		// 单位大小获取层级
		std::uint32_t UnitSizeToOrder(std::size_t unitSize);
		// 层级获取单位大小
		std::size_t OrderToUnitSize(std::uint32_t order);
		// 单位大小获取实际大小
		std::size_t UnitSizeToSize(std::size_t unitSize);
		std::size_t GetBuddyOffset(const std::size_t& offset, const std::size_t& size);
		// 获取偏移量并重新分割块的大小
		std::size_t AllocBlock(std::uint32_t order);
		void DeallocateInternal(D3D12BuddyBlockData& blockData);
		void DeallocateBlock(std::size_t offset, std::uint32_t order);

	private:
		static constexpr std::size_t DefaultPoolSize = 1024 * 1024 * 512;

		const AllocatorInitData m_InitData;     // 初始化数据

		const std::size_t m_MinBlockSize;     // 内存块的最小大小
		const std::size_t m_MaxBlockSize;     // 内存块的最大大小

		std::uint32_t m_MaxOrder;             // 最大层级

		std::vector<std::set<std::uint32_t>> m_FreeBlock;
		std::queue<D3D12BuddyBlockData> m_DeferredDeletionQueue;    // 延迟删除队列

		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;

		std::unique_ptr<D3D12Resource> m_Resource = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Heap> m_Heap = nullptr;
	};

	class D3D12MultiBuddyAllocator
	{
	public:
		D3D12MultiBuddyAllocator(ID3D12Device* device, D3D12BuddyAllocator::AllocatorInitData initData);
		void Allocate(std::uint32_t size, std::uint32_t alignment, D3D12ResourceLocation& resourceLocation);
		void Deallocate(D3D12ResourceLocation& resourceLocation);
		void ClearUpAllocations();

	private:
		std::vector<std::shared_ptr<D3D12BuddyAllocator>> m_BuddyAllocators;
		D3D12BuddyAllocator::AllocatorInitData m_InitData;
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;
	};

	class D3D12DefaultBufferAllocator
	{
	public:
		D3D12DefaultBufferAllocator(ID3D12Device* device);
		void AllocateDefaultBuffer(
			std::size_t byteSize,
			std::uint32_t alignment,
			D3D12ResourceLocation& resourceLocation);
		void Deallocate(D3D12ResourceLocation& resourceLocation);
		void ClearUpAllocations();

	private:
		std::unique_ptr<D3D12MultiBuddyAllocator> m_DefaultBufferAllocator;
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;
	};

	class D3D12UploadBufferAllocator
	{
	public:
		D3D12UploadBufferAllocator(ID3D12Device* device);
		void AllocateUploadBuffer(
			std::size_t byteSize,
			std::uint32_t alignment,
			D3D12ResourceLocation& resourceLocation);
		void Deallocate(D3D12ResourceLocation& resourceLocation);
		void ClearUpAllocations();

	private:
		std::unique_ptr<D3D12MultiBuddyAllocator> m_UploadBufferAllocator;
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;
	};

	class D3D12TextureAllocator
	{
	public:
		D3D12TextureAllocator(ID3D12Device* device);
		void AllocateTexture(
			const D3D12_RESOURCE_DESC& textureDesc,
			const D3D12_RESOURCE_STATES& textureState,
			D3D12ResourceLocation& resourceLocation,
			const D3D12_CLEAR_VALUE* clearValue = nullptr);
		void Deallocate(D3D12ResourceLocation& resourceLocation);
		void ClearUpAllocations();

	private:
		std::unique_ptr<D3D12MultiBuddyAllocator> m_TextureAllocator;
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;
	};

	class D3D12RenderTargetAllocator
	{
	public:
		D3D12RenderTargetAllocator(ID3D12Device* device);
		void Allocate(
			const D3D12_RESOURCE_DESC& textureDesc,
			const D3D12_RESOURCE_STATES& textureState,
			D3D12ResourceLocation& resourceLocation,
			const D3D12_CLEAR_VALUE* clearValue = nullptr);
		void Deallocate(D3D12ResourceLocation& resourceLocation);
		void ClearUpAllocations();

	private:
		std::unique_ptr<D3D12MultiBuddyAllocator> m_Allocator;
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;
	};
}


#endif