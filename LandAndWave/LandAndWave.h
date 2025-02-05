#pragma once
#ifndef __LANDANDWAVE__H__
#define __LANDANDWAVE__H__

#include "D3D12APP.h"
#include "Mesh.h"
#include "FrameResource.h"
#include "Object.h"
#include "Waves.h"
#include <random>

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
		void UpdateObjResource(const CpuTimer& timer);
		void UpdateWaves(const CpuTimer& gt);
		std::size_t GetAllRenderItemsCount()const noexcept;

	private:
		inline static constexpr UINT FrameCount = 3;

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;			// 根签名
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;
		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_VSByteCodes;
		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_PSByteCodes;
		std::unordered_map<std::string, std::unique_ptr<Geometry::MeshData>> m_MeshData;
		ComPtr<ID3D12DescriptorHeap> m_CbvHeap = nullptr;

		std::array<std::unique_ptr<FrameResource>, FrameCount> m_FrameResources;	// 帧资源循环数组
		FrameResource* m_CurrFrameResource = nullptr;
		UINT m_CurrFRIndex = 0;

		std::unique_ptr<Waves> m_Waves;
		std::unordered_map<std::string, Object> m_Objects;
		UINT m_PassCbvOffset = 0;
	};

}

#endif // !__LANDANDWAVE__H__
