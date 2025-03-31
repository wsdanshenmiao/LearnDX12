#include "Shader.h"
#include "TreeBillboardsAPP.h"
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

    void CylinderShader::SetMaterialCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gMaterialCB", cb);
    }

    void CylinderShader::SetLightCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gLightCB", cb);
    }

    void CylinderShader::SetCylinderCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gMatCB", cb);
    }

    void CylinderShader::SetMaterialConstants(const MaterialConstants& materialConstants)
    {
        DSM::SetMaterialConstants(m_ShaderHelper.get(), materialConstants);
    }

    void CylinderShader::SetDirectionalLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("DirectionalLights")->SetRow(byteSize, lightData);
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

    void CylinderShader::SetTexture(const D3D12DescriptorHandle& tex)
    {
        m_ShaderHelper->SetShaderResourceByName("gDiffuse", {tex});
    }

    void CylinderShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        assert(cmdList != nullptr && frameResource != nullptr);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
        m_ShaderHelper->GetShaderPass("Cylinder")->Apply(cmdList, m_Device.Get(), frameResource);
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




    
    TreeBillboardsShader::TreeBillboardsShader(ID3D12Device* device,
        std::uint32_t numDirLight,
        std::uint32_t numPointLight,
        std::uint32_t numSpotLight)
        : IShader(device){
        
        ShaderDefines shaderDefines;
        shaderDefines.AddDefine("MAXDIRLIGHTCOUNT", std::to_string(max(1, numDirLight)));
        shaderDefines.AddDefine("MAXPOINTLIGHTCOUNT", std::to_string(max(1, numPointLight)));
        shaderDefines.AddDefine("MAXSPOTLIGHTCOUNT", std::to_string(max(1, numSpotLight)));
        shaderDefines.AddDefine("ALPHATEST", "1");
        
        ShaderDesc shaderDesc{};
        shaderDesc.m_Defines = shaderDefines;
        shaderDesc.m_Target = "ps_6_1";
        shaderDesc.m_EnterPoint = "PS";
        shaderDesc.m_Type = ShaderType::PIXEL_SHADER;
        shaderDesc.m_FileName = "Shaders\\TreeSprite.hlsl";
        shaderDesc.m_ShaderName = "TreeSpritePS";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_EnterPoint = "VS";
        shaderDesc.m_Type = ShaderType::VERTEX_SHADER;
        shaderDesc.m_ShaderName = "TreeSpriteVS";
        shaderDesc.m_Target = "vs_6_1";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        shaderDesc.m_EnterPoint = "GS";
        shaderDesc.m_Type = ShaderType::GEOMETRY_SHADER;
        shaderDesc.m_ShaderName = "TreeSpriteGS";
        shaderDesc.m_Target = "gs_6_1";
        m_ShaderHelper->CreateShaderFormFile(shaderDesc);
        ShaderPassDesc passDesc{};
        passDesc.m_VSName = "TreeSpriteVS";
        passDesc.m_GSName = "TreeSpriteGS";
        passDesc.m_PSName = "TreeSpritePS";
        m_ShaderHelper->AddShaderPass("TreeSprite", passDesc, m_Device.Get());

        auto& inputLayout = BillboardVertex::GetInputLayout();
        auto pass = m_ShaderHelper->GetShaderPass("TreeSprite");
        pass->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
        pass->SetInputLayout({inputLayout.data(), (UINT)inputLayout.size()});

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.DepthClipEnable = TRUE;
        pass->SetRasterizerState(rasterizerDesc);
    }

    void TreeBillboardsShader::SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gObjCB", cb);
    }

    void TreeBillboardsShader::SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gPassCB", cb);;
    }

    void TreeBillboardsShader::SetMaterialCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gMatCB", cb);
    }

    void TreeBillboardsShader::SetLightCB(std::shared_ptr<D3D12ResourceLocation> cb)
    {
        m_ShaderHelper->SetConstantBufferByName("gLightCB", cb);
    }

    void TreeBillboardsShader::SetObjectConstants(const ObjectConstants& objConstants)
    {
        DSM::SetObjectConstants(m_ShaderHelper.get(), objConstants);
    }

    void TreeBillboardsShader::SetPassConstants(const PassConstants& passConstants)
    {
        DSM::SetPassConstants(m_ShaderHelper.get(), passConstants);
    }

    void TreeBillboardsShader::SetMaterialConstants(const MaterialConstants& materialConstants)
    {
        auto mat = materialConstants;
        mat.m_Ambient = {mat.m_Ambient.x + 0.1f, mat.m_Ambient.y + 0.1f, mat.m_Ambient.z + 0.1f};
        DSM::SetMaterialConstants(m_ShaderHelper.get(), materialConstants);
    }

    void TreeBillboardsShader::SetDirectionalLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("DirectionalLights")->SetRow(byteSize, lightData);
    }

    void TreeBillboardsShader::SetPointLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("PointLights")->SetRow(byteSize, lightData);
    }

    void TreeBillboardsShader::SetSpotLights(std::size_t byteSize, const void* lightData)
    {
        m_ShaderHelper->GetConstantBufferVariable("SpotLights")->SetRow(byteSize, lightData);
    }

    void TreeBillboardsShader::SetTexture(const D3D12DescriptorHandle& texture)
    {
        m_ShaderHelper->SetShaderResourceByName("gTreeTexArray", { texture });
    }

    void TreeBillboardsShader::Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
    {
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        m_ShaderHelper->GetShaderPass("TreeSprite")->Apply(cmdList, m_Device.Get(), frameResource);
    }

    
}
