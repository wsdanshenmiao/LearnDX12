#include "BlurShader.h"
#include <cassert>
#include <complex>

namespace DSM{
    BlurShader::BlurShader(
        ID3D12Device* device,
        std::uint32_t width,
        std::uint32_t height,
        DXGI_FORMAT format,
        D3D12TextureAllocator* allocator,
        std::shared_ptr<D3D12ResourceLocation> blurCB)
            :m_pDevice(device), m_Width(width), m_Height(height), m_Format(format){
        assert(allocator != nullptr && m_pDevice != nullptr);

        for (auto& tex : m_BlurMaps) {
            tex = std::make_unique<GpuBuffer>();
        }
        
        ShaderDesc shaderDesc{};
        shaderDesc.m_Target = "cs_6_1";
        shaderDesc.m_EnterPoint = "HorizBlurCS";
        shaderDesc.m_Type = ShaderType::COMPUTE_SHADER;
        shaderDesc.m_FileName = "Shaders\\Blur.hlsl";
        shaderDesc.m_ShaderName = "HorizBlurCS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_EnterPoint = "VerticBlurCS";
        shaderDesc.m_ShaderName = "VerticBlurCS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        ShaderPassDesc passDesc{};
        passDesc.m_CSName = "HorizBlurCS";
        m_ShaderHelper->AddShaderPass("HorizBlurCS", passDesc, device);
        passDesc.m_CSName = "VerticBlurCS";
        m_ShaderHelper->AddShaderPass("VerticBlurCS", passDesc, device);

        auto pass = m_ShaderHelper->GetShaderPass("HorizBlurCS");
        pass->CreatePipelineState(device);
        pass = m_ShaderHelper->GetShaderPass("VerticBlurCS");
        pass->CreatePipelineState(device);
        
        CreateResource(allocator);
        m_ShaderHelper->SetConstantBufferByName("BlurConstants", blurCB);
    }

    void BlurShader::CreateDescriptors(D3D12DescriptorHandle hDescriptor, std::uint32_t descriptorSize)
    {
        for (int i = 0; i < m_BlurMaps.size(); ++i) {
            m_BlurMaps[i]->m_SrvHandle = hDescriptor;
            hDescriptor += descriptorSize;
            m_BlurMaps[i]->m_UavHandle = hDescriptor;
            hDescriptor += descriptorSize;
        }
        CreateDescriptors();
    }

    void BlurShader::OnResize(std::uint32_t width, std::uint32_t height, D3D12TextureAllocator* allocator)
    {
        if (m_Width == width && m_Height == height) return;
        
        m_Width = width;
        m_Height = height;
        for (int i = 0; i < m_BlurMaps.size(); ++i) {
            allocator->Deallocate(m_BlurMaps[i]->m_ResourceLocation);
        }
        CreateResource(allocator);
        CreateDescriptors();
    }

    void BlurShader::SetInputTexture(ID3D12Resource* inputTexture)
    {
        m_pInputTexture = inputTexture;
    }

    void BlurShader::SetBlurCount(std::uint32_t count)
    {
        m_BlurCount = count;
    }

    ID3D12Resource* BlurShader::GetOutputTexture()
    {
        return m_BlurMaps[0]->GetResource()->m_Resource.Get();
    }

    void BlurShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        assert(m_pInputTexture != nullptr);
        
        constexpr float sigma = 2.5f;
        
        auto weights = CalculGaussWeights(sigma);
        int blurRadius = static_cast<int>(weights.size()) / 2;

        m_ShaderHelper->GetConstantBufferVariable("gBlurRadius")->SetInt(blurRadius);
        m_ShaderHelper->GetConstantBufferVariable("gBlurWeights")->SetRow(weights.size() * sizeof(float), weights.data());

        auto blurMap0 = m_BlurMaps[0]->GetResource()->m_Resource.Get();
        auto blurMap1 = m_BlurMaps[1]->GetResource()->m_Resource.Get();
        D3D12_RESOURCE_BARRIER rtToCopySource{};
        rtToCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        rtToCopySource.Transition.pResource = m_pInputTexture;
        rtToCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        rtToCopySource.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        rtToCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        auto comToDest = rtToCopySource;
        comToDest.Transition.pResource = blurMap0;
        comToDest.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        comToDest.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

        // 设置屏障
        cmdList->ResourceBarrier(1, &rtToCopySource);
        cmdList->ResourceBarrier(1, &comToDest);
        cmdList->CopyResource(blurMap0, m_pInputTexture);

        auto destToRead = comToDest;
        destToRead.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        destToRead.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        auto comToUA = comToDest;
        comToUA.Transition.pResource = blurMap1;
        comToUA.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        comToUA.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        cmdList->ResourceBarrier(1, &destToRead);
        cmdList->ResourceBarrier(1, &comToUA);

        auto readToUA0 = comToDest;
        readToUA0.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
        readToUA0.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        auto readToUA1 = comToUA;
        readToUA1.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;

        auto UAToRead0 = readToUA0;
        UAToRead0.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        UAToRead0.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        auto UAToRead1 = readToUA1;
        UAToRead1.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        UAToRead1.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

        for (int i = 0; i < m_BlurCount; ++i) {
            // 设置纹理
            m_ShaderHelper->SetShaderResourceByName("gBlurTexture", {m_BlurMaps[0]->m_SrvHandle});
            m_ShaderHelper->SetRWResourceByName("gOutput", {m_BlurMaps[1]->m_UavHandle});

            auto pass =m_ShaderHelper->GetShaderPass("HorizBlurCS");
            pass->Apply(cmdList, frameResource);
            
            pass->Dispatch(cmdList, m_Width, m_Height, 1);

            cmdList->ResourceBarrier(1, &readToUA0);
            cmdList->ResourceBarrier(1, &UAToRead1);
            
            m_ShaderHelper->SetShaderResourceByName("gBlurTexture", {m_BlurMaps[1]->m_SrvHandle});
            m_ShaderHelper->SetRWResourceByName("gOutput", {m_BlurMaps[0]->m_UavHandle});

            pass = m_ShaderHelper->GetShaderPass("VerticBlurCS");
            pass->Apply(cmdList, frameResource);
            pass->Dispatch(cmdList, m_Width, m_Height, 1);
            
            cmdList->ResourceBarrier(1, &UAToRead0);
            cmdList->ResourceBarrier(1, &readToUA1);
        }
    }

    void BlurShader::CreateResource(D3D12TextureAllocator* allocator)
    {
        // 创建无序访问视图
        for (int i = 0; i < m_BlurMaps.size(); ++i) {
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
            allocator->AllocateTexture(blurMapDesc, D3D12_RESOURCE_STATE_COMMON, m_BlurMaps[i]->m_ResourceLocation);
        }
    }
    
    void BlurShader::CreateDescriptors()
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC blurSrvDesc{};
        blurSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        blurSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        blurSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        blurSrvDesc.Texture2D.MipLevels = 1;
        
        D3D12_UNORDERED_ACCESS_VIEW_DESC bmUAVDesc = {};
        bmUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bmUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        bmUAVDesc.Texture2D.MipSlice = 0;
        
        for (int i =0; i < m_BlurMaps.size(); ++i) {
            auto pBlurMapResource = m_BlurMaps[i]->GetResource()->m_Resource.Get();
            m_pDevice->CreateShaderResourceView(pBlurMapResource, &blurSrvDesc, m_BlurMaps[i]->m_SrvHandle);
            m_pDevice->CreateUnorderedAccessView(pBlurMapResource, nullptr, &bmUAVDesc, m_BlurMaps[i]->m_UavHandle);
        }
    }

    std::vector<float> BlurShader::CalculGaussWeights(float sigma) const
    {
        // 取 2 sigma 范围，计算高斯核范围
        int blurRadius = std::ceil(2 * sigma);
        assert(blurRadius <= sm_MaxBlurRadius);

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

    float BlurShader::GaussFunction(float x, float sigma) const
    {
        return std::exp(- x * x / (2 * sigma * sigma));
    }
}
