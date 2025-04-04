#pragma once
#ifndef __BLURFILTER__H__
#define __BLURFILTER__H__

#include "Buffer.h"
#include "D3D12Allocatioin.h"
#include "FrameResource.h"

namespace DSM{
    class BlurFilter
    {
    public:
        BlurFilter(ID3D12Device* device,
            std::uint32_t width,
            std::uint32_t height,
            DXGI_FORMAT format,
            D3D12TextureAllocator* allocator);
        BlurFilter(const BlurFilter&) = delete;
        BlurFilter& operator=(const BlurFilter&) = delete;
        BlurFilter(BlurFilter&&) = default;
        BlurFilter& operator=(BlurFilter&&) = default;
        ~BlurFilter() = default;

        // 创建描述符
        void CreateDescriptors(D3D12DescriptorHandle hDescriptor, std::uint32_t descriptorSize);
        void OnResize(std::uint32_t width, std::uint32_t height, D3D12TextureAllocator* allocator);
        void Execute(ID3D12GraphicsCommandList* commandList,
            FrameResource* frameResource,
            ID3D12Resource* input,
            std::uint32_t blurCount);

    private:
        void CreateResource(D3D12TextureAllocator* allocator);
        void CreateDescriptors();

        // 计算高斯核的权重
        std::vector<float> CalculGaussWeights(float sigma) const;
        // 高斯函数exp(-x ^ 2 / (2 * sigma ^ 2))
        float GaussFunction(float x, float sigma) const;

    private:
        constexpr int m_MaxBlurRadius = 5;
        
        ID3D12Device* m_pDevice = nullptr;
        std::unique_ptr<GpuBuffer> m_BlurMap;
        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
        DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    };

    
}


#endif