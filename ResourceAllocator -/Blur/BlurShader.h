#pragma once
#ifndef __BLURFILTER__H__
#define __BLURFILTER__H__

#include "Buffer.h"
#include "D3D12Allocatioin.h"
#include "FrameResource.h"
#include "IShader.h"

namespace DSM{
    class BlurShader : public IShader
    {
    public:
        BlurShader(ID3D12Device* device,
            std::uint32_t width,
            std::uint32_t height,
            DXGI_FORMAT format,
            D3D12TextureAllocator* allocator,
            std::shared_ptr<D3D12ResourceLocation> blurCB);
        BlurShader(const BlurShader&) = delete;
        BlurShader& operator=(const BlurShader&) = delete;
        BlurShader(BlurShader&&) = default;
        BlurShader& operator=(BlurShader&&) = default;
        ~BlurShader() = default;

        // 创建描述符
        void CreateDescriptors(D3D12DescriptorHandle hDescriptor, std::uint32_t descriptorSize);
        void OnResize(std::uint32_t width, std::uint32_t height, D3D12TextureAllocator* allocator);
        void SetInputTexture(ID3D12Resource* inputTexture);
        void SetBlurCount(std::uint32_t count);
        ID3D12Resource* GetOutputTexture();
        virtual void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) override;

    private:
        void CreateResource(D3D12TextureAllocator* allocator);
        void CreateDescriptors();

        // 计算高斯核的权重
        std::vector<float> CalculGaussWeights(float sigma) const;
        // 高斯函数exp(-x ^ 2 / (2 * sigma ^ 2))
        float GaussFunction(float x, float sigma) const;

    public:
        static const int sm_MaxBlurRadius = 5;

    private:
        ID3D12Device* m_pDevice = nullptr;
        ID3D12Resource* m_pInputTexture = nullptr;
        DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        
        std::array<std::unique_ptr<GpuBuffer>, 2> m_BlurMaps;
        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
        std::uint32_t m_BlurCount = 1;
    };

    
}


#endif