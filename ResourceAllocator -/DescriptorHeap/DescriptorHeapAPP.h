#ifndef __TEXTURINGAPP__H__
#define __TEXTURINGAPP__H__

#include "D3D12App.h"
#include "FrameResource.h"
#include "MeshData.h"
#include "Material.h"
#include "ObjectManager.h"
#include "ShaderHelper.h"
#include "Waves.h"

namespace DSM {
	class DescriptorHeapAPP : public D3D12App
	{
	public:
		DescriptorHeapAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

		bool OnInit() override;

	protected:
		// 继承自D3D12App
		void OnUpdate(const CpuTimer& timer) override;
		void OnRender(const CpuTimer& timer) override;

		void WaitForGPU();
		void RenderScene(RenderLayer layer);

		bool InitResource();

		void CreateShader();
		void CreateObject();
		void CreateTexture();
		void CreateLights();
		void CreateFrameResource();
		void CreateDescriptorHeaps();

		void UpdatePassCB(const CpuTimer& timer);
		void UpdateLightCB(const CpuTimer& timer);


	public:
		inline static constexpr UINT FrameCount = 3;


	protected:
		ComPtr<ID3D12RootSignature> m_RootSignature;
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

		std::unordered_map<std::string, Material> m_Materials;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
		FrameResource* m_CurrFrameResource = nullptr;

		std::unique_ptr<Waves> m_Waves;

		UINT m_CurrFrameIndex = 0;

		DirectX::XMFLOAT4 m_ReflectPlane;

		ShaderHelper m_ShaderHelper;
	};

	
} // namespace DSM





#endif