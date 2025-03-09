#pragma once
#ifndef __D3D12DESCRIPTORHANDLE__H__
#define __D3D12DESCRIPTORHANDLE__H__

#include "Pubh.h"

namespace DSM {
    // 描述符的句柄
    class D3D12DescriptorHandle
    {
    public:
        D3D12DescriptorHandle();
        D3D12DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

        std::size_t GetCpuPtr() const noexcept;
        std::uint64_t GetGpuPtr() const noexcept;
        // 检测句柄是否有效
        bool IsValid() const noexcept;
        bool IsShaderVisible() const noexcept;

        // 便于偏移内部虚拟地址
        D3D12DescriptorHandle operator+(int offset) const noexcept;
        void operator+=(int offset) noexcept;
        const D3D12_CPU_DESCRIPTOR_HANDLE* operator&() const noexcept;
        // 隐式转换
        operator D3D12_CPU_DESCRIPTOR_HANDLE() const noexcept;
        operator D3D12_GPU_DESCRIPTOR_HANDLE() const noexcept;

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle{};
    };

    // 存储 Shader Visible 的静态描述符堆 
    class D3D12DescriptorHeap
    {
    public:
        D3D12DescriptorHeap(ID3D12Device* device, std::uint32_t descriptorSize);
        ~D3D12DescriptorHeap();

        // 创建描述符堆
        void Create(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t maxCount);
        // 销毁描述符堆
        void Destroy();
        void Clear();

        bool HasValidSpace(std::uint32_t numDescriptors) const noexcept;
        bool IsValidHandle(const D3D12DescriptorHandle& handle) const noexcept;
        D3D12DescriptorHandle AllocateAndCopy(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcHandle);
        D3D12DescriptorHandle Allocate();
        
        ID3D12DescriptorHeap* GetHeap() const noexcept;
        std::uint32_t GetOffsetOfHandle(const D3D12DescriptorHandle& handle) const noexcept; 
        std::uint32_t GetDescriptorSize() const noexcept;

        D3D12DescriptorHandle operator[](std::uint32_t index) const noexcept;
        
    private:
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
        D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc{};
        std::uint32_t m_DescriptorSize{};
        std::uint32_t m_NumFreeDescriptors{};
        
        D3D12DescriptorHandle m_FirstHandle{};
        D3D12DescriptorHandle m_NextFreeHandle{};
    };

    // Shader Visible 的描述符缓冲堆
    class D3D12DescriptorCache
    {
    public:
        D3D12DescriptorCache(ID3D12Device* device, std::uint32_t maxCount = 1024);

        ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const noexcept;
        D3D12DescriptorHandle AllocateAndCopy(
            D3D12_DESCRIPTOR_HEAP_TYPE heapType,
            const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcHandle);
        D3D12DescriptorHandle AllocateAndCreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        void Clear();
        
    private:
        // 存储不同描述符的数组
        std::array<std::unique_ptr<D3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeaps;
        ID3D12Device* m_Device;
    };
    
}

#endif

