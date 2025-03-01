#include "D3D12Resource.h"

namespace DSM{
    D3D12Resource::D3D12Resource(
        Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES inintState)
            : m_Resource(resource), m_ResourceState(inintState),m_MappedBaseAddress(nullptr){
        if (m_Resource != nullptr) {
            m_GpuVirtualAddress = m_Resource->GetGPUVirtualAddress();
        }
    }

    void* D3D12Resource::Map()
    {
        ThrowIfFailed(m_Resource->Map(0, nullptr, &m_MappedBaseAddress));
        return m_MappedBaseAddress;
    }

    void D3D12Resource::Unmap()
    {
        m_Resource->Unmap(0, nullptr);
    }
    
}
