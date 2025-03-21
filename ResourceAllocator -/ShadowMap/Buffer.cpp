#include "Buffer.h"

namespace DSM{
    D3D12Resource* Buffer::GetResource()
    {
        return m_ResourceLocation.m_UnderlyingResource;
    }
}
