#pragma once
#ifndef __BUFFER__H__
#define __BUFFER__H__
#include "D3D12DescriptorHeap.h"
#include "D3D12Resource.h"

namespace DSM {
    struct Buffer
    {
        D3D12Resource* GetResource();

        D3D12ResourceLocation m_ResourceLocation;
        D3D12DescriptorHandle m_SrvHandle;
    };

    struct TextureBuffer : Buffer
    {
        D3D12DescriptorHandle m_RtvHandle;
    };

    struct DepthBuffer : Buffer
    {
        D3D12DescriptorHandle m_DsvHandle;
    };
}


#endif