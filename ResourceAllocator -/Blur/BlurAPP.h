#ifndef __TREEBILLBOARDSAPP__H__
#define __TREEBILLBOARDSAPP__H__

#include "BlurShader.h"
#include "D3D12App.h"
#include "ObjectManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "Buffer.h"
#include "ConstantData.h"
#include "Shader.h"

namespace DSM {
struct Material;


class BlurAPP : public D3D12App {
   public:
    BlurAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

    bool OnInit() override;

   protected:
    void OnUpdate(const CpuTimer& timer) override;
    void OnRender(const CpuTimer& timer) override;
    void OnResize() override;

    void WaitForGPU();
    void RenderScene(RenderLayer layer);
    void RenderShadow();

    bool InitResource();

    void CreateObject();
    void CreateTexture();
    void CreateFrameResource();
    void CreateDescriptor();

    void UpdatePassCB(const CpuTimer& timer);
    void UpdateLightCB(const CpuTimer& timer);
    void UpdateShadowCB(const CpuTimer& timer);

    MaterialConstants GetMaterialConstants(const Material& material);
    ObjectConstants GetObjectConstants(const Object& obj);

   public:
    inline static constexpr UINT FrameCount = 3;

   protected:
    std::unique_ptr<D3D12DescriptorCache> m_ShaderDescriptorHeap;

    DirectX::BoundingSphere m_SceneSphere{};

    std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
    FrameResource* m_CurrFrameResource = nullptr;

    UINT m_CurrFrameIndex = 0;

    std::unique_ptr<D3D12RenderTargetAllocator> m_RenderTargetAllocator;
    std::unique_ptr<D3D12TextureAllocator> m_TextureAllocator;

    std::unique_ptr<DepthBuffer> m_ShadowMap;

    std::unique_ptr<Camera> m_Camera;
    std::unique_ptr<CameraController> m_CameraController;

    std::unique_ptr<LitShader> m_LitShader;
    std::unique_ptr<ShadowShader> m_ShadowShader;
    std::unique_ptr<BlurShader> m_BlurShader;
 
    DirectX::XMMATRIX m_ShadowTrans;
};


} // namespace DSM





#endif