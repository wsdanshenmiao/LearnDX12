#ifndef __TREEBILLBOARDSAPP__H__
#define __TREEBILLBOARDSAPP__H__

#include "D3D12App.h"
#include "ObjectManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "Buffer.h"
#include "ConstantData.h"
#include "Shader.h"

namespace DSM {
struct Material;


    struct BillboardVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 Size;

        static const auto&  GetInputLayout()
        {
            static const std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputLayout = {
                D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                D3D12_INPUT_ELEMENT_DESC{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            return inputLayout;
        };
    };
 
class TreeBillboardsAPP : public D3D12App {
   public:
    TreeBillboardsAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

    bool OnInit() override;

   protected:
    void OnUpdate(const CpuTimer& timer) override;
    void OnRender(const CpuTimer& timer) override;
    void OnResize() override;

    void WaitForGPU();
    void RenderScene(RenderLayer layer);
    void RenderTriangle();
    void RenderCylinder();

    bool InitResource();

    void CreateObject();
    void CreateTexture();
    void CreateFrameResource();

    void UpdatePassCB(const CpuTimer& timer);
    void UpdateLightCB(const CpuTimer& timer);

    MaterialConstants GetMaterialConstants(const Material& material);
    ObjectConstants GetObjectConstants(const Object& obj);

   public:
    inline static constexpr UINT FrameCount = 3;

   protected:
    std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

    std::unique_ptr<D3D12DescriptorCache> m_ShaderDescriptorHeap;

    DirectX::BoundingSphere m_SceneSphere{};

    std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
    FrameResource* m_CurrFrameResource = nullptr;

    UINT m_CurrFrameIndex = 0;
    DirectX::XMMATRIX m_ShadowTrans;

    std::unique_ptr<Camera> m_Camera;
    std::unique_ptr<CameraController> m_CameraController;

    std::unique_ptr<TriangleShader> m_TriangleShader;
    std::unique_ptr<CylinderShader> m_CylinderShader;
    std::unique_ptr<LitShader> m_LitShader;
    std::unique_ptr<TreeBillboardsShader> m_TreeBillboardsShader;

    const std::uint8_t m_TreeCount = 16;
    const DirectX::XMFLOAT2 m_BoardSize = {10, 10};
};


} // namespace DSM





#endif