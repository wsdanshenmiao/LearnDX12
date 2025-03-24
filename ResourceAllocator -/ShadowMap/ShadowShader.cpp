#include "ShadowShader.h"

#include "Vertex.h"

using namespace DirectX;

namespace DSM {
    ShadowShader::ShadowShader(ID3D12Device* device)
        :IShader(device){
        CreateShader();
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
        m_ShaderHelper->GetConstantBufferVariable("World")->SetMatrix(objConstants.m_World);
        m_ShaderHelper->GetConstantBufferVariable("WorldInvTranspose")->SetMatrix(objConstants.m_WorldInvTranspose);
    }

    void ShadowShader::SetPassConstants(const PassConstants& passConstants)
    {
        m_ShaderHelper->GetConstantBufferVariable("View")->SetMatrix(passConstants.m_View);
        m_ShaderHelper->GetConstantBufferVariable("InvView")->SetMatrix(passConstants.m_InvView);
        m_ShaderHelper->GetConstantBufferVariable("Proj")->SetMatrix(passConstants.m_Proj);
        m_ShaderHelper->GetConstantBufferVariable("InvProj")->SetMatrix(passConstants.m_InvProj);
        m_ShaderHelper->GetConstantBufferVariable("ShadowTrans")->SetMatrix(passConstants.m_ShadowTrans);
        m_ShaderHelper->GetConstantBufferVariable("EyePosW")->SetFloat3(passConstants.m_EyePosW);
        m_ShaderHelper->GetConstantBufferVariable("FogStart")->SetFloat(passConstants.m_FogStart);
        m_ShaderHelper->GetConstantBufferVariable("FogColor")->SetFloat3(passConstants.m_FogColor);
        m_ShaderHelper->GetConstantBufferVariable("FogRange")->SetFloat(passConstants.m_FogRange);
        m_ShaderHelper->GetConstantBufferVariable("RenderTargetSize")->SetFloat2(passConstants.m_RenderTargetSize);
        m_ShaderHelper->GetConstantBufferVariable("InvRenderTargetSize")->SetFloat2(passConstants.m_InvRenderTargetSize);
        m_ShaderHelper->GetConstantBufferVariable("NearZ")->SetFloat(passConstants.m_NearZ);
        m_ShaderHelper->GetConstantBufferVariable("FarZ")->SetFloat(passConstants.m_FarZ);
        m_ShaderHelper->GetConstantBufferVariable("TotalTime")->SetFloat(passConstants.m_TotalTime);
        m_ShaderHelper->GetConstantBufferVariable("DeltaTime")->SetFloat(passConstants.m_DeltaTime);
    }

    void ShadowShader::SetMaterialConstants(const MaterialConstants& materialConstants)
    {
        m_ShaderHelper->GetConstantBufferVariable("Diffuse")->SetFloat3(materialConstants.m_Diffuse);
        m_ShaderHelper->GetConstantBufferVariable("Specular")->SetFloat3(materialConstants.m_Specular);
        m_ShaderHelper->GetConstantBufferVariable("Ambient")->SetFloat3(materialConstants.m_Ambient);
        m_ShaderHelper->GetConstantBufferVariable("Alpha")->SetFloat(materialConstants.m_Alpha);
        m_ShaderHelper->GetConstantBufferVariable("Gloss")->SetFloat(materialConstants.m_Gloss);
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

    void ShadowShader::CreateShader()
    {
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
            m_ShaderPasses[i]->SetInputLayout({inputLayout.data(), (UINT)inputLayout.size()});

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
            m_ShaderPasses[i]->SetRTVFormat({DXGI_FORMAT_UNKNOWN});       // 不需要渲染对象
            m_ShaderPasses[i]->SetRTVFormat({});       // 不需要渲染对象
        }
    }
}
