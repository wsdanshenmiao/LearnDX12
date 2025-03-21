#include "ShadowDebugShader.h"

namespace DSM{
    ShadowDebugShader::ShadowDebugShader(ID3D12Device* device)
        :IShader(device){
        CreateShader();
    }

    void ShadowDebugShader::SetTexture(const D3D12DescriptorHandle& handle)
    {
        m_ShaderHelper->SetShaderResourceByName("gDiffuse", {handle});
    }

    void ShadowDebugShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        m_ShaderHelper->GetShaderPass("ShadowDebug")->Apply(cmdList, m_Device.Get(), frameResource);
    }

    void ShadowDebugShader::CreateShader()
    {
        ShaderDesc shaderDescDebug{};
        shaderDescDebug.m_EnterPoint = "FullScreenVS";
        shaderDescDebug.m_Type = ShaderType::VERTEX_SHADER;
        shaderDescDebug.m_ShaderName = "ShadowDebugVS";
        shaderDescDebug.m_Target = "vs_6_1";
        shaderDescDebug.m_FileName = "Shaders\\FullScreen.hlsl";
        m_ShaderHelper->CreateShaderFormFile(shaderDescDebug);

        shaderDescDebug.m_EnterPoint = "ShadowPS";
        shaderDescDebug.m_Type = ShaderType::PIXEL_SHADER;
        shaderDescDebug.m_ShaderName = "ShadowDebugPS";
        shaderDescDebug.m_FileName = "Shaders\\Shadow.hlsl";
        shaderDescDebug.m_Target = "ps_6_1";
        m_ShaderHelper->CreateShaderFormFile(shaderDescDebug);
        
        ShaderPassDesc passDesc{};
        passDesc.m_VSName = "ShadowDebugVS";
        passDesc.m_PSName = "ShadowDebugPS";
        m_ShaderHelper->AddShaderPass("ShadowDebug", passDesc, m_Device.Get());

        m_ShaderHelper->GetShaderPass("ShadowDebug")->SetInputLayout({nullptr, 0 });
    }
}
