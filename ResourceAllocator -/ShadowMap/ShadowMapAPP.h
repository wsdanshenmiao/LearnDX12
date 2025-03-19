#ifndef __TEXTURINGAPP__H__
#define __TEXTURINGAPP__H__

#include "D3D12App.h"
#include "FrameResource.h"
#include "ObjectManager.h"
#include "ShaderHelper.h"
#include <memory>
#include "Camera.h"
#include "CameraController.h"
#include "IShader.h"
#include "LitShader.h"

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

		bool InitResource();

		void CreateObject();
		void CreateTexture();
		void CreateFrameResource();

		void UpdatePassCB(const CpuTimer& timer);
		void UpdateLightCB(const CpuTimer& timer);


	public:
		inline static constexpr UINT FrameCount = 3;


	protected:
		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
		FrameResource* m_CurrFrameResource = nullptr;

		UINT m_CurrFrameIndex = 0;

		std::unique_ptr<LitShader> m_LitShader;
		std::unique_ptr<Camera> m_Camera;
		std::unique_ptr<CameraController> m_CameraController;
	};

	
} // namespace DSM





#endif