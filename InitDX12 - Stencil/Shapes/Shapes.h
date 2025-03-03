#pragma once
#ifndef __SHAPES__H__
#define __SHAPES__H__

#include "D3D12App.h"
#include "Mesh.h"
#include "FrameResource.h"
#include "Object.h"

namespace DSM {

	class LandAndWave : public D3D12App
	{
	public:
		LandAndWave(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);
		LandAndWave(const LandAndWave& other) = delete;
		LandAndWave& operator=(const LandAndWave& other) = delete;
		virtual ~LandAndWave();

		bool OnInit();

	private:
		void OnResize();
		void OnUpdate(const CpuTimer& timer);
		void OnRender(const CpuTimer& timer);
		void OnRenderScene(const CpuTimer& timer);

		bool InitResource();
		bool InitFrameResourceCB();
		bool CreateCBV();
		bool CreateGeometry();
		bool CreateRootSignature();
		bool CreatePSO();
		void WaitForGpu();
		void UpdateFrameResource(const CpuTimer& timer);

		std::size_t GetMeshSize() const noexcept;

	private:
		inline static constexpr UINT FrameCount = 3;

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;			// 根签名
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;
		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_VSByteCodes;
		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_PSByteCodes;
		ComPtr<ID3D12DescriptorHeap> m_CbvHeap = nullptr;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;	// 帧资源循环数组
		FrameResource* m_CurrFrameResource = nullptr;
		UINT m_CurrFRIndex = 0;

		std::unique_ptr<Geometry::MeshData> m_MeshData;
		UINT m_PassCbvOffset = 0;

		DirectX::XMFLOAT3 m_EyePos{};
	};

}

#endif // !__SHAPES__H__
