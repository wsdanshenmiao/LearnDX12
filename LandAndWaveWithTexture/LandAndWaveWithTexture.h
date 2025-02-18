#ifndef __TEXTURINGAPP__H__
#define __TEXTURINGAPP__H__

#include "D3D12App.h"
#include "FrameResource.h"
#include "MeshData.h"
#include "Material.h"
#include "Texture.h"
#include "Waves.h"

namespace DSM {
	class LandAndWaveWithTexture : public D3D12App
	{
	public:
		LandAndWaveWithTexture(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

		bool OnInit() override;

	protected:
		// 继承自D3D12App
		void OnUpdate(const CpuTimer& timer) override;
		void OnRender(const CpuTimer& timer) override;

		void WaitForGPU();
		void RenderScene();

		bool InitResource();

		void CreateShader();
		void CreateObject();
		void CreateTexture();
		void CreateLights();
		void CreateFrameResource();
		void CreateMaterial();
		void CreateDescriptorHeaps();
		void CreateRootSignature();
		void CreatePSOs();

		void UpdateFrameResource(const CpuTimer& timer);
		void UpdateObjCB(const CpuTimer& timer);
		void UpdateWaves(const CpuTimer& timer);

		const std::array<const D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() const noexcept;


	public:
		inline static constexpr UINT FrameCount = 5;


	protected:
		ComPtr<ID3D12RootSignature> m_RootSignature;
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

		ComPtr<ID3D12DescriptorHeap> m_TexDescriptorHeap;
		std::unordered_map<std::string, Material> m_Materials;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
		FrameResource* m_CurrFrameResource = nullptr;

		std::unique_ptr<Waves> m_Waves;

		UINT m_CurrFrameIndex = 0;
	};


} // namespace DSM





#endif