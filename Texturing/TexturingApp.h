#ifndef __TEXTURINGAPP__H__
#define __TEXTURINGAPP__H__

#include "D3D12App.h"
#include "FrameResource.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"

namespace DSM {
	class ResourceAllocatorAPP : public D3D12App
	{
	public:
		ResourceAllocatorAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);

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

		void UpdatePassCB(const CpuTimer& timer);
		void UpdateGeometry(const CpuTimer& timer);

		const std::array<const D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() const noexcept;


	public:
		inline static constexpr UINT FrameCount = 5;


	protected:
		ComPtr<ID3D12RootSignature> m_RootSignature;
		ComPtr<ID3D12DescriptorHeap> m_TexDescriptorHeap;
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;
		std::unordered_map<std::string, std::unique_ptr<Geometry::MeshData>> m_MeshData;
		std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
		Material m_BoxMaterial;

		FrameResource* m_CurrFrameResource = nullptr;
		UINT m_CurrFrameIndex = 0;
		UINT m_RenderObjCount = 0;
	};

} // namespace DSM





#endif