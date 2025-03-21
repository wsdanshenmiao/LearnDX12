#ifndef __TEXTURINGAPP__H__
#define __TEXTURINGAPP__H__

#include "D3D12App.h"
#include "ObjectManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "LitShader.h"
#include "Buffer.h"
#include "ShadowDebugShader.h"
#include "ShadowShader.h"

namespace DSM {
	class ShadowMapAPP : public D3D12App
	{
	public:
		ShadowMapAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

		bool OnInit() override;

	protected:
		// 继承自D3D12App
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
		void UpdateShadowCB(const CpuTimer& timer);
		void UpdateLightCB(const CpuTimer& timer);

		MaterialConstants GetMaterialConstants(const Material& material);


	public:
		inline static constexpr UINT FrameCount = 1;


	protected:
		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

		std::unique_ptr<D3D12DescriptorCache> m_ShaderDescriptorHeap;

		std::unique_ptr<D3D12RTOrDSAllocator> m_RTOrDSAllocator;
		std::unique_ptr<DepthBuffer> m_ShadowMap;

		DirectX::BoundingSphere m_SceneSphere{};

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
		FrameResource* m_CurrFrameResource = nullptr;

		UINT m_CurrFrameIndex = 0;

		std::unique_ptr<LitShader> m_LitShader;
		std::unique_ptr<ShadowShader> m_ShadowShader;
		std::unique_ptr<ShadowDebugShader> m_ShadowDebugShader;
		D3D12DescriptorHandle m_ShadowMapDescriptor;
		DirectX::XMMATRIX m_ShadowTrans;
		
		
		std::unique_ptr<Camera> m_Camera;
		std::unique_ptr<CameraController> m_CameraController;
	};

} // namespace DSM





#endif