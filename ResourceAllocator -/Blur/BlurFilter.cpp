#include "BlurFilter.h"
#include <cassert>
#include <complex>

namespace DSM{
    BlurFilter::BlurFilter(
        ID3D12Device* device,
        std::uint32_t width,
        std::uint32_t height,
        DXGI_FORMAT format,
        D3D12TextureAllocator* allocator)
            :m_pDevice(device), m_Width(width), m_Height(height), m_Format(format){
        assert(allocator);

        CreateResource(allocator);
    }

    void BlurFilter::CreateDescriptors(D3D12DescriptorHandle hDescriptor, std::uint32_t descriptorSize)
    {
        m_BlurMap->m_SrvHandle = hDescriptor;
        hDescriptor += descriptorSize;
        m_BlurMap->m_UavHandle = hDescriptor;
        CreateDescriptors();
    }

    void BlurFilter::OnResize(std::uint32_t width, std::uint32_t height, D3D12TextureAllocator* allocator)
    {
        if (m_Width == width && m_Height == height) return;
        
        m_Width = width;
        m_Height = height;
        allocator->Deallocate(m_BlurMap->m_ResourceLocation);
        CreateResource(allocator);
        CreateDescriptors();
    }

    void BlurFilter::Execute(
        ID3D12GraphicsCommandList* commandList,
        FrameResource* frameResource,
        ID3D12Resource* input,
        std::uint32_t blurCount)
    {
        constexpr float sigma = 2.5f;
        auto weights = CalculGaussWeights(sigma);
        int blurRadius = static_cast<int>(weights.size()) / 2;

        
    }

    void BlurFilter::CreateResource(D3D12TextureAllocator* allocator)
    {
        // 创建无序访问视图
        D3D12_RESOURCE_DESC blurMapDesc{};
        blurMapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        blurMapDesc.Width = m_Width;
        blurMapDesc.Height = m_Height;
        blurMapDesc.DepthOrArraySize = 1;
        blurMapDesc.MipLevels = 1;
        blurMapDesc.SampleDesc = {1, 0};
        blurMapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        blurMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        blurMapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        allocator->AllocateTexture(blurMapDesc, D3D12_RESOURCE_STATE_COMMON, m_BlurMap->m_ResourceLocation);
    }
    
    void BlurFilter::CreateDescriptors()
    {
        auto pBlurMapResource = m_BlurMap->GetResource()->m_Resource.Get();
        D3D12_SHADER_RESOURCE_VIEW_DESC blurSrvDesc{};
        blurSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        blurSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        blurSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        blurSrvDesc.Texture2D.MipLevels = 1;
        m_pDevice->CreateShaderResourceView(pBlurMapResource, &blurSrvDesc, m_BlurMap->m_SrvHandle);
        
        D3D12_UNORDERED_ACCESS_VIEW_DESC bmUAVDesc = {};
        bmUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bmUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        bmUAVDesc.Texture2D.MipSlice = 0;
        m_pDevice->CreateUnorderedAccessView(pBlurMapResource, nullptr, &bmUAVDesc, m_BlurMap->m_UavHandle);
    }

    std::vector<float> BlurFilter::CalculGaussWeights(float sigma) const
    {
        // 取 2 sigma 范围，计算高斯核范围
        int blurRadius = std::ceil(2 * sigma);
        static_assert(blurRadius < m_MaxBlurRadius);

        // 计算权重
        float weightSum = 0;
        std::vector<float> ret(2 * blurRadius + 1);
        for (int i = -blurRadius; i <= blurRadius; ++i) {
            ret[i + blurRadius] = GaussFunction(i, sigma);
            weightSum += ret[i + blurRadius];
        }
        
        float invWeightSum = 1 / weightSum;
        for (auto& weight : ret) {
            weight *= invWeightSum;
        }

        return ret;
    }

    float BlurFilter::GaussFunction(float x, float sigma) const
    {
        return std::exp(- x * x / (2 * sigma * sigma));
    }
}
