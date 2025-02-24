#ifndef __TEXTURINGAPP__H__
#define __TEXTURINGAPP__H__

#include "D3D12App.h"
#include "FrameResource.h"
#include "MeshData.h"
#include "Material.h"
#include "ObjectManager.h"
#include "Waves.h"

namespace DSM {
	class ShaderReflectAPP : public D3D12App
	{
	public:
		ShaderReflectAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

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
		void CreateRootSignature();
		void CreatePSOs();

		void UpdatePassCB(const CpuTimer& timer);
		void UpdateObjCB(const CpuTimer& timer);
		void UpdateWaves(const CpuTimer& timer);

		const std::array<const D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() const noexcept;


	public:
		inline static constexpr UINT FrameCount = 5;


	protected:
		ComPtr<ID3D12RootSignature> m_RootSignature;
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

		std::unordered_map<std::string, Material> m_Materials;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
		FrameResource* m_CurrFrameResource = nullptr;

		std::unique_ptr<Waves> m_Waves;

		UINT m_CurrFrameIndex = 0;
	};


} // namespace DSM





#endif