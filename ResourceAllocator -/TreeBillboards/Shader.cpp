#include "Shader.h"
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
    
    TriangleShader::TriangleShader(ID3D12Device* device)
        :IShader(device){
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

        auto pass = m_ShaderHelper->GetShaderPass("Triangle");
        auto& inputlayout = VertexPosColor::GetInputLayout();
        pass->SetInputLayout({inputlayout.data(), (UINT)inputlayout.size()});
        pass->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
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
        DSM::SetObjectConstants(m_ShaderHelper.get(), objConstants);
    }

    void TriangleShader::SetPassConstants(const PassConstants& passConstants)
    {
        DSM::SetPassConstants(m_ShaderHelper.get(), passConstants);
    }

    void TriangleShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        assert(cmdList != nullptr && frameResource != nullptr);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_ShaderHelper->GetShaderPass("Triangle")->Apply(cmdList, m_Device.Get(), frameResource);
    }


    
    CylinderShader::CylinderShader(ID3D12Device* device)
        :IShader(device){
        ShaderDesc shaderDesc;
        shaderDesc.m_Target = "vs_6_1";
        shaderDesc.m_Type = ShaderType::VERTEX_SHADER;
        shaderDesc.m_EnterPoint = "CylinderVS";
        shaderDesc.m_FileName = "Shaders\\Cylinder.hlsl";
        shaderDesc.m_ShaderName = "CylinderVS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_Target = "ps_6_1";
        shaderDesc.m_Type = ShaderType::PIXEL_SHADER;
        shaderDesc.m_EnterPoint = "CylinderPS";
        shaderDesc.m_ShaderName = "CylinderPS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_Target = "gs_6_1";
        shaderDesc.m_Type = ShaderType::GEOMETRY_SHADER;
        shaderDesc.m_EnterPoint = "CylinderGS";
        shaderDesc.m_ShaderName = "CylinderGS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);

        ShaderPassDesc shaderPassDesc;
        shaderPassDesc.m_VSName = "CylinderVS";
        shaderPassDesc.m_GSName = "CylinderGS";
        shaderPassDesc.m_PSName = "CylinderPS";
        m_ShaderHelper->AddShaderPass("Cylinder", shaderPassDesc, m_Device.Get());

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        auto pass = m_ShaderHelper->GetShaderPass("Cylinder");
        auto& inputlayout = VertexPosNormalColor::GetInputLayout();
        pass->SetInputLayout({inputlayout.data(), (UINT)inputlayout.size()});
        pass->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
        pass->SetRasterizerState(rasterizerDesc);
    }

    void CylinderShader::SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gObjCB", cb);
    }

    void CylinderShader::SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gPassCB", cb);;
    }

    void CylinderShader::SetCylinderCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gCylinderCB", cb);
    }

    void CylinderShader::SetObjectConstants(const ObjectConstants& objConstants)
    {
        DSM::SetObjectConstants(m_ShaderHelper.get(), objConstants);
    }

    void CylinderShader::SetPassConstants(const PassConstants& passConstants)
    {
        DSM::SetPassConstants(m_ShaderHelper.get(), passConstants);
    }

    void CylinderShader::SetCylinderConstants(float height)
    {
        m_ShaderHelper->GetConstantBufferVariable("CylinderHeight")->SetFloat(height);
    }

    void CylinderShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        assert(cmdList != nullptr && frameResource != nullptr);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
        m_ShaderHelper->GetShaderPass("Cylinder")->Apply(cmdList, m_Device.Get(), frameResource);
    }
}
