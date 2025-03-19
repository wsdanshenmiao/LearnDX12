#include "LitShader.h"
#include "Vertex.h"

using namespace DirectX;

namespace DSM {
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

        auto& inputLayout = VertexPosLNormalTex::GetInputLayout();
        m_ShaderHelper->GetShaderPass("Light")->SetInputLayout({inputLayout.data(), (UINT)inputLayout.size()});
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
        m_ShaderHelper->GetConstantBufferVariable("World")->SetMatrix(objConstants.m_World);
        m_ShaderHelper->GetConstantBufferVariable("WorldInvTranspose")->SetMatrix(objConstants.m_WorldInvTranspose);
    }

    void LitShader::SetPassConstants(const PassConstants& passConstants)
    {
        m_ShaderHelper->GetConstantBufferVariable("View")->SetMatrix(passConstants.m_View);
        m_ShaderHelper->GetConstantBufferVariable("InvView")->SetMatrix(passConstants.m_InvView);
        m_ShaderHelper->GetConstantBufferVariable("Proj")->SetMatrix(passConstants.m_Proj);
        m_ShaderHelper->GetConstantBufferVariable("InvProj")->SetMatrix(passConstants.m_InvProj);
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

    void LitShader::SetMaterialConstants(const MaterialConstants& materialConstants)
    {
        m_ShaderHelper->GetConstantBufferVariable("Diffuse")->SetFloat3(materialConstants.m_Diffuse);
        m_ShaderHelper->GetConstantBufferVariable("Specular")->SetFloat3(materialConstants.m_Specular);
        m_ShaderHelper->GetConstantBufferVariable("Ambient")->SetFloat3(materialConstants.m_Ambient);
        m_ShaderHelper->GetConstantBufferVariable("Alpha")->SetFloat(materialConstants.m_Alpha);
        m_ShaderHelper->GetConstantBufferVariable("Gloss")->SetFloat(materialConstants.m_Gloss);
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

    void LitShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        m_ShaderHelper->GetShaderPass("Light")->Apply(cmdList, m_Device.Get(), frameResource);
    }

}
