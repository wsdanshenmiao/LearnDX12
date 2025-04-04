#include "Shader.h"
#include "BlurAPP.h"
#include "Vertex.h"

namespace DSM {

    void SetPassConstants(ShaderHelper* shaderHelper, const PassConstants& passConstants)
    {
        shaderHelper->GetConstantBufferVariable("View")->SetMatrix(passConstants.m_View);
        shaderHelper->GetConstantBufferVariable("InvView")->SetMatrix(passConstants.m_InvView);
        shaderHelper->GetConstantBufferVariable("Proj")->SetMatrix(passConstants.m_Proj);
        shaderHelper->GetConstantBufferVariable("InvProj")->SetMatrix(passConstants.m_InvProj);
        shaderHelper->GetConstantBufferVariable("ShadowTrans")->SetMatrix(passConstants.m_ShadowTrans);
        shaderHelper->GetConstantBufferVariable("EyePosW")->SetFloat3(passConstants.m_EyePosW);
        shaderHelper->GetConstantBufferVariable("FogStart")->SetFloat(passConstants.m_FogStart);
        shaderHelper->GetConstantBufferVariable("FogColor")->SetFloat3(passConstants.m_FogColor);
        shaderHelper->GetConstantBufferVariable("FogRange")->SetFloat(passConstants.m_FogRange);
        shaderHelper->GetConstantBufferVariable("RenderTargetSize")->SetFloat2(passConstants.m_RenderTargetSize);
        shaderHelper->GetConstantBufferVariable("InvRenderTargetSize")->SetFloat2(passConstants.m_InvRenderTargetSize);
        shaderHelper->GetConstantBufferVariable("NearZ")->SetFloat(passConstants.m_NearZ);
        shaderHelper->GetConstantBufferVariable("FarZ")->SetFloat(passConstants.m_FarZ);
        shaderHelper->GetConstantBufferVariable("TotalTime")->SetFloat(passConstants.m_TotalTime);
        shaderHelper->GetConstantBufferVariable("DeltaTime")->SetFloat(passConstants.m_DeltaTime);
    }
    
    void SetObjectConstants(ShaderHelper* shaderHelper, const ObjectConstants& objConstants)
    {
        shaderHelper->GetConstantBufferVariable("World")->SetMatrix(objConstants.m_World);
        shaderHelper->GetConstantBufferVariable("WorldInvTranspose")->SetMatrix(objConstants.m_WorldInvTranspose);
    }

    void SetMaterialConstants(ShaderHelper* shaderHelper, const MaterialConstants& materialConstants)
    {
        shaderHelper->GetConstantBufferVariable("Diffuse")->SetFloat3(materialConstants.m_Diffuse);
        shaderHelper->GetConstantBufferVariable("Specular")->SetFloat3(materialConstants.m_Specular);
        shaderHelper->GetConstantBufferVariable("Ambient")->SetFloat3(materialConstants.m_Ambient);
        shaderHelper->GetConstantBufferVariable("Alpha")->SetFloat(materialConstants.m_Alpha);
        shaderHelper->GetConstantBufferVariable("Gloss")->SetFloat(materialConstants.m_Gloss);
    }
    



    LitShader::LitShader(ID3D12Device* device,
        std::uint32_t numDirLight,
        std::uint32_t numPointLight,
        std::uint32_t numSpotLight)
        :IShader(device){
        ShaderDefines shaderDefines;
        shaderDefines.AddDefine("MAXDIRLIGHTCOUNT", std::to_string(max(1, numDirLight)));
        shaderDefines.AddDefine("MAXPOINTLIGHTCOUNT", std::to_string(max(1, numPointLight)));
        shaderDefines.AddDefine("MAXSPOTLIGHTCOUNT", std::to_string(max(1, numSpotLight)));
        
        ShaderDesc shaderDesc{};
        shaderDesc.m_Defines = shaderDefines;
        shaderDesc.m_Target = "ps_6_1";
        shaderDesc.m_EnterPoint = "PS";
        shaderDesc.m_Type = ShaderType::PIXEL_SHADER;
        shaderDesc.m_FileName = "Shaders\\Light.hlsl";
        shaderDesc.m_ShaderName = "LightsPS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_EnterPoint = "VS";
        shaderDesc.m_Type = ShaderType::VERTEX_SHADER;
        shaderDesc.m_ShaderName = "LightsVS";
        shaderDesc.m_Target = "vs_6_1";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        ShaderPassDesc passDesc{};
        passDesc.m_VSName = "LightsVS";
        passDesc.m_PSName = "LightsPS";
        m_ShaderHelper->AddShaderPass("Light", passDesc, m_Device.Get());

        auto& inputLayout = VertexPosNormalTex::GetInputLayout();
        auto pass = m_ShaderHelper->GetShaderPass("Light");
        pass->SetInputLayout({inputLayout.data(), (UINT)inputLayout.size()});
        pass->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    }

    void LitShader::SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gObjCB", cb);
    }

    void LitShader::SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gPassCB", cb);;
    }

    void LitShader::SetMaterialCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gMatCB", cb);
    }

    void LitShader::SetLightCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gLightCB", cb);
    }

    void LitShader::SetObjectConstants(const ObjectConstants& objConstants)
    {
        DSM::SetObjectConstants(m_ShaderHelper.get(), objConstants);
    }

    void LitShader::SetPassConstants(const PassConstants& passConstants)
    {
        DSM::SetPassConstants(m_ShaderHelper.get(), passConstants);
    }

    void LitShader::SetMaterialConstants(const MaterialConstants& materialConstants)
    {
        DSM::SetMaterialConstants(m_ShaderHelper.get(), materialConstants);
    }

    void LitShader::SetDirectionalLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("DirectionalLights")->SetRow(byteSize, lightData);
    }

    void LitShader::SetPointLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("PointLights")->SetRow(byteSize, lightData);
    }

    void LitShader::SetSpotLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("SpotLights")->SetRow(byteSize, lightData);
    }

    void LitShader::SetTexture(const D3D12DescriptorHandle& texture)
    {
        m_ShaderHelper->SetShaderResourceByName("gDiffuse", { texture });
    }

    void LitShader::SetShadowMap(const D3D12DescriptorHandle& shadowMap)
    {
        m_ShaderHelper->SetShaderResourceByName("gShadowMap", { shadowMap });
    }

    void LitShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_ShaderHelper->GetShaderPass("Light")->Apply(cmdList, m_Device.Get(), frameResource);
    }




    ShadowShader::ShadowShader(ID3D12Device* device)
        :IShader(device) {
        ShaderDefines shaderDefines;
        shaderDefines.AddDefine("ALPHATEST", "1");

        ShaderDesc shaderDesc{};
        shaderDesc.m_Target = "ps_6_1";
        shaderDesc.m_EnterPoint = "ShadowPS";
        shaderDesc.m_Type = ShaderType::PIXEL_SHADER;
        shaderDesc.m_FileName = "Shaders\\Shadow.hlsl";
        shaderDesc.m_ShaderName = "ShadowPS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);

        shaderDesc.m_ShaderName = "ShadowPSWithAlphaTest";
        shaderDesc.m_Defines = shaderDefines;
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);

        shaderDesc.m_EnterPoint = "ShadowVS";
        shaderDesc.m_Type = ShaderType::VERTEX_SHADER;
        shaderDesc.m_ShaderName = "ShadowVS";
        shaderDesc.m_Target = "vs_6_1";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);

        ShaderPassDesc shaderPassDesc{};
        shaderPassDesc.m_VSName = "ShadowVS";
        shaderPassDesc.m_PSName = "ShadowPS";
        m_ShaderHelper->AddShaderPass("Shadow", shaderPassDesc, m_Device.Get());
        shaderPassDesc.m_PSName = "ShadowWithAlphaTest";
        m_ShaderHelper->AddShaderPass("ShadowWithAlphaTest", shaderPassDesc, m_Device.Get());

        m_ShaderPasses[0] = m_ShaderHelper->GetShaderPass("Shadow");
        m_ShaderPasses[1] = m_ShaderHelper->GetShaderPass("ShadowWithAlphaTest");

        for (int i = 0; i < m_ShaderPasses.size(); i++) {
            auto& inputLayout = VertexPosNormalTex::GetInputLayout();
            m_ShaderPasses[i]->SetInputLayout({ inputLayout.data(), (UINT)inputLayout.size() });

            D3D12_RASTERIZER_DESC rasterizerDesc{};
            ZeroMemory(&rasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
            rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
            rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
            rasterizerDesc.DepthClipEnable = TRUE;
            rasterizerDesc.DepthBias = 100000;
            rasterizerDesc.SlopeScaledDepthBias = 1;
            rasterizerDesc.DepthBiasClamp = 0;
            rasterizerDesc.DepthClipEnable = TRUE;
            m_ShaderPasses[i]->SetRasterizerState(rasterizerDesc);
            m_ShaderPasses[i]->SetRTVFormat({ DXGI_FORMAT_UNKNOWN });       // ����Ҫ��Ⱦ����
            m_ShaderPasses[i]->SetRTVFormat({});       // ����Ҫ��Ⱦ����
        }
    }

    void ShadowShader::SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gObjCB", cb);
    }

    void ShadowShader::SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gPassCB", cb);;
    }

    void ShadowShader::SetMaterialCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gMatCB", cb);
    }

    void ShadowShader::SetObjectConstants(const ObjectConstants& objConstants)
    {
        DSM::SetObjectConstants(m_ShaderHelper.get(), objConstants);
    }

    void ShadowShader::SetPassConstants(const PassConstants& passConstants)
    {
        DSM::SetPassConstants(m_ShaderHelper.get(), passConstants);
    }

    void ShadowShader::SetMaterialConstants(const MaterialConstants& materialConstants)
    {
        DSM::SetMaterialConstants(m_ShaderHelper.get(), materialConstants);
    }
    void ShadowShader::SetTexture(const D3D12DescriptorHandle& texture)
    {
        m_ShaderHelper->SetShaderResourceByName("gDiffuse", { texture });
    }

    void ShadowShader::SetAlphaTest(bool enable)
    {
        if (enable != m_EnableAlphaTest) {
            m_CurrentPass = static_cast<std::uint8_t>(enable);
        }
    }

    void ShadowShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        m_ShaderPasses[m_CurrentPass]->Apply(cmdList, m_Device.Get(), frameResource);
    }
}
