#include "D3D12DescriptorHeap.h"

#include <D3DUtil.h>

namespace DSM {
	//
	// D3D12DescriptorHandle Implementation
	//
	D3D12DescriptorHandle::D3D12DescriptorHandle()
		:m_CPUHandle(D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN),
		m_GPUHandle(D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
	}

	D3D12DescriptorHandle::D3D12DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
		:m_CPUHandle(cpuHandle), m_GPUHandle(gpuHandle) {
	}

	std::size_t D3D12DescriptorHandle::GetCpuPtr() const noexcept
	{
		return m_CPUHandle.ptr;
	}

	std::uint64_t D3D12DescriptorHandle::GetGpuPtr() const noexcept
	{
		return m_GPUHandle.ptr;
	}

	bool D3D12DescriptorHandle::IsValid() const noexcept
	{
		return m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	bool D3D12DescriptorHandle::IsShaderVisible() const noexcept
	{
		return m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	D3D12DescriptorHandle D3D12DescriptorHandle::operator+(int offset) const noexcept
	{
		auto ret = *this;
		ret += offset;
		return ret;
	}

	void D3D12DescriptorHandle::operator+=(int offset) noexcept
	{
		if (m_CPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_CPUHandle.ptr += offset;
		}
		if (m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_GPUHandle.ptr += offset;
		}
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE* D3D12DescriptorHandle::operator&() const noexcept
	{
		return &m_CPUHandle;
	}

	D3D12DescriptorHandle::operator D3D12_CPU_DESCRIPTOR_HANDLE() const noexcept
	{
		return m_CPUHandle;
	}

	D3D12DescriptorHandle::operator D3D12_GPU_DESCRIPTOR_HANDLE() const noexcept
	{
		return m_GPUHandle;
	}


	D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device* device, std::uint32_t descriptorSize)
		:m_Device(device), m_DescriptorSize(descriptorSize) {
		assert(device != nullptr);
	}

	//
	// D3D12DescriptorHeap Implementation
	//
	D3D12DescriptorHeap::~D3D12DescriptorHeap()
	{
		Destroy();
	}

	void D3D12DescriptorHeap::Create(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t maxCount)
	{
		if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
			m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		}
		else {
			m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}
		m_HeapDesc.NumDescriptors = maxCount;
		m_HeapDesc.Type = heapType;

		ThrowIfFailed(m_Device->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
#ifdef _DEBUG
		m_DescriptorHeap->SetName(name.c_str());
#endif

		m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
		m_DescriptorSize = m_Device->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
		m_FirstHandle = { m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart() };
		m_NextFreeHandle = m_FirstHandle;
	}

	void D3D12DescriptorHeap::Destroy()
	{
		m_DescriptorHeap = nullptr;
	}

	void D3D12DescriptorHeap::Clear()
	{
		m_NextFreeHandle = m_FirstHandle;
		m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
	}

	bool D3D12DescriptorHeap::HasValidSpace(std::uint32_t numDexciptors) const noexcept
	{
		return m_NumFreeDescriptors >= numDexciptors;
	}

	bool D3D12DescriptorHeap::IsValidHandle(const D3D12DescriptorHandle& handle) const noexcept
	{
		auto cpuPtr = handle.GetCpuPtr();
		auto gpuPtr = handle.GetGpuPtr();
		if (cpuPtr < m_FirstHandle.GetCpuPtr() ||
			cpuPtr >= m_NextFreeHandle.GetCpuPtr() + m_HeapDesc.NumDescriptors * m_DescriptorSize) {
			return false;
		}
		if (gpuPtr - m_FirstHandle.GetGpuPtr() != cpuPtr - m_FirstHandle.GetCpuPtr()) {
			return false;
		}

		return true;
	}

	D3D12DescriptorHandle D3D12DescriptorHeap::AllocateAndCopy(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcHandle)
	{
		auto size = static_cast<UINT>(srcHandle.size());
		assert(HasValidSpace(size));

		auto dstHandle = m_NextFreeHandle;
		m_Device->CopyDescriptors(1, &dstHandle, &size, size, srcHandle.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_NextFreeHandle += size * m_DescriptorSize;
		m_NumFreeDescriptors -= size;

		return dstHandle;
	}

	D3D12DescriptorHandle D3D12DescriptorHeap::Allocate()
	{
		assert(HasValidSpace(1));

		auto handle = m_NextFreeHandle;
		m_NextFreeHandle += m_DescriptorSize;
		m_NumFreeDescriptors -= 1;

		return handle;
	}

	ID3D12DescriptorHeap* D3D12DescriptorHeap::GetHeap() const noexcept
	{
		return m_DescriptorHeap.Get();
	}

	std::uint32_t D3D12DescriptorHeap::GetOffsetOfHandle(const D3D12DescriptorHandle& handle) const noexcept
	{
		return (handle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize;
	}

	std::uint32_t D3D12DescriptorHeap::GetDescriptorSize() const noexcept
	{
		return m_DescriptorSize;
	}

	D3D12DescriptorHandle D3D12DescriptorHeap::operator[](std::uint32_t index) const noexcept
	{
		assert(index < m_HeapDesc.NumDescriptors);

		return m_FirstHandle + index * m_DescriptorSize;
	}



	//
	// D3D12DescriptorCache Implementation
	//
	D3D12DescriptorCache::D3D12DescriptorCache(ID3D12Device* device, std::uint32_t maxCount)
		:m_Device(device) {
		assert(device != nullptr);

		for (int i = 0; i < m_DescriptorHeaps.size(); ++i) {
			auto& heap = m_DescriptorHeaps[i];
			auto size = device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
			heap = std::make_unique<D3D12DescriptorHeap>(device, size);
			auto heapType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
			heap->Create(L"D3D12DescriptorCache" + AnsiToWString(typeid(heapType).name()), heapType, maxCount);
		}
	}

	ID3D12DescriptorHeap* D3D12DescriptorCache::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const noexcept
	{
		auto index = static_cast<int>(heapType);
		assert(index < m_DescriptorHeaps.size());
		return m_DescriptorHeaps[index]->GetHeap();
	}

	D3D12DescriptorHandle D3D12DescriptorCache::AllocateAndCopy(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcHandle)
	{
		auto index = static_cast<int>(heapType);
		assert(index < m_DescriptorHeaps.size());
		return m_DescriptorHeaps[index]->AllocateAndCopy(srcHandle);
	}

	D3D12DescriptorHandle D3D12DescriptorCache::AllocateAndCreateSRV(
		ID3D12Resource* resource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
	{
		auto handle = m_DescriptorHeaps[static_cast<int>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)]->Allocate();
		m_Device->CreateShaderResourceView(resource, &desc, handle);
		return handle;
	}

	void D3D12DescriptorCache::Clear()
	{
		for (auto& heap : m_DescriptorHeaps) {
			heap->Clear();
		}
	}
}
