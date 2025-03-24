#include "TriangleShader.h"
#include "Vertex.h"

namespace DSM {
    TriangleShader::TriangleShader(ID3D12Device* device)
        :IShader(device){
        CreateShader();
    }

    void TriangleShader::SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gObjCB", cb);
    }

    void TriangleShader::SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gPassCB", cb);;
    }
    
    void TriangleShader::SetObjectConstants(const ObjectConstants& objConstants)
    {
        m_ShaderHelper->GetConstantBufferVariable("World")->SetMatrix(objConstants.m_World);
        m_ShaderHelper->GetConstantBufferVariable("WorldInvTranspose")->SetMatrix(objConstants.m_WorldInvTranspose);
    }

    void TriangleShader::SetPassConstants(const PassConstants& passConstants)
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

    void TriangleShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        assert(cmdList != nullptr && frameResource != nullptr);
        m_ShaderHelper->GetShaderPass("Triangle")->Apply(cmdList, m_Device.Get(), frameResource);
    }

    void TriangleShader::CreateShader()
    {
        ShaderDesc shaderDesc;
        shaderDesc.m_Target = "vs_6_1";
        shaderDesc.m_Type = ShaderType::VERTEX_SHADER;
        shaderDesc.m_EnterPoint = "TriangleVS";
        shaderDesc.m_FileName = "Shaders\\Triangle.hlsl";
        shaderDesc.m_ShaderName = "TriangleVS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_Target = "ps_6_1";
        shaderDesc.m_Type = ShaderType::PIXEL_SHADER;
        shaderDesc.m_EnterPoint = "TrianglePS";
        shaderDesc.m_ShaderName = "TrianglePS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_Target = "gs_6_1";
        shaderDesc.m_Type = ShaderType::GEOMETRY_SHADER;
        shaderDesc.m_EnterPoint = "TriangleGS";
        shaderDesc.m_ShaderName = "TriangleGS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);

        ShaderPassDesc shaderPassDesc;
        shaderPassDesc.m_VSName = "TriangleVS";
        shaderPassDesc.m_GSName = "TriangleGS";
        shaderPassDesc.m_PSName = "TrianglePS";
        m_ShaderHelper->AddShaderPass("Triangle", shaderPassDesc, m_Device.Get());

        auto& inputlayout = VertexPosLColor::GetInputLayout();
        m_ShaderHelper->GetShaderPass("Triangle")->SetInputLayout({inputlayout.data(), (UINT)inputlayout.size()});
    }
}
