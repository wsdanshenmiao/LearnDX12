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
class Material;

class TreeBillboardsAPP : public D3D12App {
   public:
    TreeBillboardsAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

    bool OnInit() override;

   protected:
    // �̳���D3D12App
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
	};


} // namespace DSM





#endif