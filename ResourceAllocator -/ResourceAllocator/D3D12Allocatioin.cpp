#include "D3D12Allocatioin.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace DSM {
	D3D12BuddyAllocator::D3D12BuddyAllocator(
		ID3D12Device* device,
		const AllocatorInitData& initData,
		std::size_t minBlockSize,
		std::size_t maxBlockSize)
		:m_Device(device), m_InitData(initData), m_MinBlockSize(minBlockSize), m_MaxBlockSize(maxBlockSize) {
		assert(m_Device != nullptr);

		D3D12_HEAP_PROPERTIES heapProper{};
		heapProper.Type = m_InitData.m_HeapType;
		if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
			// 创建堆
			D3D12_HEAP_DESC heapDesc{};
			heapDesc.Flags = m_InitData.m_HeapFlags;
			heapDesc.Properties = heapProper;
			heapDesc.SizeInBytes = m_MaxBlockSize;

			ThrowIfFailed(m_Device->CreateHeap(&heapDesc, IID_PPV_ARGS(m_Heap.GetAddressOf())));
			m_Heap->SetName(L"D3D12BuddyAllocator BuddyHeap");
		}
		else {  // ManualSubAllocation
			// 创建一大块资源
			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Flags = m_InitData.m_ResourceFlags;
			resourceDesc.Width = maxBlockSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc = { 1,0 };
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			D3D12_RESOURCE_STATES resourceState{};
			if (m_InitData.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
			else if (m_InitData.m_HeapType == D3D12_HEAP_TYPE_DEFAULT) {
				resourceState = D3D12_RESOURCE_STATE_COMMON;
			}

			ComPtr<ID3D12Resource> resource;
			m_Device->CreateCommittedResource(
				&heapProper,
				m_InitData.m_HeapFlags,
				&resourceDesc,
				resourceState,
				nullptr,
				IID_PPV_ARGS(resource.GetAddressOf()));
			resource->SetName(L"D3D12BuddyAllocator BuddyResource");
			m_Resource = std::make_unique<D3D12Resource>(resource.Get(), resourceState);

			// 映射资源
			if (m_InitData.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
				m_Resource->Map();
			}
		}
		// 计算最大层级
		m_MaxOrder = UnitSizeToOrder(SizeToUnitSize(maxBlockSize));

		// 创建空闲的块
		m_FreeBlock.resize(m_MaxOrder + 1);
		m_FreeBlock[m_MaxOrder].insert(std::uint32_t(0));   // 初始偏移量为0
	}

	D3D12BuddyAllocator::~D3D12BuddyAllocator()
	{
		if (m_Resource != nullptr) {
			m_Resource->Unmap();
		}
		if (m_Heap != nullptr) {
			m_Heap->Release();
		}
	}

	bool D3D12BuddyAllocator::Allocate(std::uint32_t size,
		std::uint32_t alignment,
		D3D12ResourceLocation& resourceLocation)
	{
		auto unitSize = SizeToUnitSize(size);
		auto order = UnitSizeToOrder(unitSize);
		auto blockSize = UnitSizeToSize(unitSize);

		try {
			// 获取偏移量
			auto offset = AllocBlock(order);
			const auto offsetSize = UnitSizeToSize(offset);
			auto aligOffsetSize = offsetSize;
			// 将偏移量进行对齐
			if (alignment != 0 && offsetSize % alignment != 0) {
				aligOffsetSize = D3DUtil::AlignArbitrary(aligOffsetSize, alignment);
				assert((size + aligOffsetSize - offsetSize) <= blockSize);
			}

			resourceLocation.m_Allocator = this;
			resourceLocation.m_BlockData.m_Offset = offset;
			resourceLocation.m_BlockData.m_Order = order;
			resourceLocation.m_BlockData.m_ActualUseSize = size;
			resourceLocation.m_ResourceLocationType = D3D12ResourceLocation::ResourceLocationType::SubAllocation;

			if (m_InitData.m_Strategy == AllocationStrategy::ManualSubAllocation) {
				resourceLocation.m_UnderlyingResource = m_Resource.get();
				resourceLocation.m_GPUVirtualAddress = m_Resource->m_GpuVirtualAddress + aligOffsetSize;
				resourceLocation.m_OffsetFromBaseOfResource = aligOffsetSize;
				if (m_InitData.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
					resourceLocation.m_MappedBaseAddress = static_cast<char*>(m_Resource->m_MappedBaseAddress) + aligOffsetSize;
				}
			}
			else {
				// Placed Resource 由创建者初始化
				resourceLocation.m_OffsetFromBaseOfHeap = aligOffsetSize;
			}
		}
		catch (std::bad_alloc&) {
			return false;
		}

		return true;
	}

	void D3D12BuddyAllocator::Deallocate(D3D12ResourceLocation& resourceLocation)
	{
		m_DeferredDeletionQueue.push(resourceLocation.m_BlockData);
	}

	void D3D12BuddyAllocator::ClearUpAllocations()
	{
		for (std::size_t i = 0; i < m_DeferredDeletionQueue.size(); ++i) {
			auto& blockData = m_DeferredDeletionQueue.front();
			DeallocateInternal(blockData);
			m_DeferredDeletionQueue.pop();
		}
	}

	ID3D12Heap* D3D12BuddyAllocator::GetHeap() const
	{
		return m_Heap.Get();
	}

	D3D12BuddyAllocator::AllocationStrategy D3D12BuddyAllocator::GetAllocationStrategy() const
	{
		return m_InitData.m_Strategy;
	}


	// 计算对其后的字节数
	std::size_t D3D12BuddyAllocator::SizeToUnitSize(std::size_t size)
	{
		return ((size + m_MinBlockSize - 1) / m_MinBlockSize);
	}

	std::uint32_t D3D12BuddyAllocator::UnitSizeToOrder(std::size_t unitSize)
	{
		// 计算对数log2(size)
		unsigned long index;
		_BitScanReverse(&index, unitSize + unitSize - 1);
		return index;
	}

	std::size_t D3D12BuddyAllocator::OrderToUnitSize(std::uint32_t order)
	{
		return std::size_t(1) << order;
	}

	std::size_t D3D12BuddyAllocator::UnitSizeToSize(std::size_t unitSize)
	{
		return unitSize * m_MinBlockSize;
	}

	std::size_t D3D12BuddyAllocator::GetBuddyOffset(const std::size_t& offset, const std::size_t& size)
	{
		return offset ^ size;
	}

	std::size_t D3D12BuddyAllocator::AllocBlock(std::uint32_t order)
	{
		if (order > m_MaxOrder) {
			throw std::bad_alloc();
		}

		std::size_t offset;

		if (auto it = m_FreeBlock[order].begin(); it == m_FreeBlock[order].end()) {
			// 若当前层级内存不够，向上查询并拆分
			auto left = AllocBlock(order + 1);
			auto size = OrderToUnitSize(order);
			auto right = left + size;
			// 将右块插入空闲块中，相当于记录资源的偏移量
			m_FreeBlock[order].insert(std::uint32_t(right));
			// 将左块提供给调用者
			offset = left;
		}
		else {  // 若够则将此内存块移除
			offset = *it;
			m_FreeBlock[order].erase(it);
		}

		return offset;
	}

	void D3D12BuddyAllocator::DeallocateInternal(D3D12BuddyBlockData& blockData)
	{
		DeallocateBlock(blockData.m_Offset, blockData.m_Order);

		if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
			blockData.m_PlacedResource = nullptr;
		}
	}

	void D3D12BuddyAllocator::DeallocateBlock(std::size_t offset, std::uint32_t order)
	{
		std::size_t unitSize = OrderToUnitSize(order);
		auto buddyOffset = GetBuddyOffset(offset, unitSize);

		if (auto it = m_FreeBlock[order].begin(); it == m_FreeBlock[order].end()) {
			// 将该块加入空闲块中
			m_FreeBlock[order].insert(offset);
		}
		else {
			// 该块空闲，向上合并
			DeallocateBlock(min(offset, buddyOffset), order + 1);
			m_FreeBlock[order].erase(it);
		}
	}

	D3D12MultiBuddyAllocator::D3D12MultiBuddyAllocator(ID3D12Device* device,
		D3D12BuddyAllocator::AllocatorInitData initData)
		:m_Device(device), m_InitData(initData) {
	}

	void D3D12MultiBuddyAllocator::Allocate(
		std::uint32_t size,
		std::uint32_t alignment,
		D3D12ResourceLocation& resourceLocation)
	{
		// 查找所有分配器分配内存
		for (auto& allocator : m_BuddyAllocators) {
			bool success = allocator->Allocate(size, alignment, resourceLocation);
			if (success) return;
		}

		// 新分配一个分配器
		auto newBuddyAllocator = std::make_shared<D3D12BuddyAllocator>(m_Device.Get(), m_InitData);
		bool sucess = newBuddyAllocator->Allocate(size, alignment, resourceLocation);
		assert(sucess);
		m_BuddyAllocators.push_back(newBuddyAllocator);
	}

	void D3D12MultiBuddyAllocator::ClearUpAllocations()
	{
		for (auto& allocator : m_BuddyAllocators) {
			allocator->ClearUpAllocations();
		}
	}

	D3D12DefaultBufferAllocator::D3D12DefaultBufferAllocator(
		ID3D12Device* device)
		:m_Device(device) {
		D3D12BuddyAllocator::AllocatorInitData initData{};
		initData.m_HeapFlags = D3D12_HEAP_FLAG_NONE;
		initData.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
		initData.m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
		initData.m_Strategy = D3D12BuddyAllocator::AllocationStrategy::ManualSubAllocation;

		m_DefaultBufferAllocator = std::make_unique<D3D12MultiBuddyAllocator>(m_Device.Get(), initData);
	}

	void D3D12DefaultBufferAllocator::AllocateDefaultBuffer(
		std::size_t byteSize,
		std::uint32_t alignment,
		D3D12ResourceLocation& resourceLocation)
	{
		m_DefaultBufferAllocator->Allocate(byteSize, alignment, resourceLocation);
	}

	void D3D12DefaultBufferAllocator::ClearUpAllocations()
	{
		m_DefaultBufferAllocator->ClearUpAllocations();
	}

	D3D12UploadBufferAllocator::D3D12UploadBufferAllocator(ID3D12Device* device)
		:m_Device(device) {
		D3D12BuddyAllocator::AllocatorInitData initData{};
		initData.m_HeapFlags = D3D12_HEAP_FLAG_NONE;
		initData.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
		initData.m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
		initData.m_Strategy = D3D12BuddyAllocator::AllocationStrategy::ManualSubAllocation;

		m_UploadBufferAllocator = std::make_unique<D3D12MultiBuddyAllocator>(m_Device.Get(), initData);
	}

	void D3D12UploadBufferAllocator::AllocateUploadBuffer(
		std::size_t byteSize,
		std::uint32_t alignment,
		D3D12ResourceLocation& resourceLocation)
	{
		m_UploadBufferAllocator->Allocate(byteSize, alignment, resourceLocation);
	}

	void D3D12UploadBufferAllocator::ClearUpAllocations()
	{
		m_UploadBufferAllocator->ClearUpAllocations();
	}

	D3D12TextureAllocator::D3D12TextureAllocator(ID3D12Device* device)
		:m_Device(device){
		D3D12BuddyAllocator::AllocatorInitData initData{};
		initData.m_HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		initData.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
		initData.m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
		initData.m_Strategy = D3D12BuddyAllocator::AllocationStrategy::PlacedResource;
		
		m_TextureAllocator = std::make_unique<D3D12MultiBuddyAllocator>(m_Device.Get(), initData);
	}

	void D3D12TextureAllocator::AllocateTexture(const D3D12_RESOURCE_DESC& textureDesc,
		const D3D12_RESOURCE_STATES& textureState, D3D12ResourceLocation& resourceLocation)
	{
		// 获取在堆中的偏移
		auto texInfo = m_Device->GetResourceAllocationInfo(0, 1, &textureDesc);
		m_TextureAllocator->Allocate(texInfo.SizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, resourceLocation);

		// 在堆中分配资源
		ComPtr<ID3D12Resource> resource;
		m_Device->CreatePlacedResource(resourceLocation.m_Allocator->GetHeap(),
			resourceLocation.m_OffsetFromBaseOfHeap,
			&textureDesc,
			textureState,
			nullptr,
			IID_PPV_ARGS(resource.GetAddressOf()));
		auto texResource = std::make_shared<D3D12Resource>(resource.Get());
		resourceLocation.m_UnderlyingResource = texResource.get();
		resourceLocation.m_BlockData.m_PlacedResource = texResource;
		resourceLocation.m_GPUVirtualAddress = resourceLocation.m_UnderlyingResource->m_Resource->GetGPUVirtualAddress();
	}

	void D3D12TextureAllocator::ClearUpAllocations()
	{
		m_TextureAllocator->ClearUpAllocations();
	}
}
