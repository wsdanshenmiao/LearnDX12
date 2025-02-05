#pragma once
#ifndef __DRAWBOX__H__
#define __DRAWBOX__H__

#include "D3D12APP.h"
#include "Mesh.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

namespace DSM {

	struct VertexPosLColor
	{
		DirectX::XMFLOAT3 m_Pos;
		DirectX::XMFLOAT4 m_Color;
	};

	struct ObjConstants
	{
		DirectX::XMFLOAT4X4 gWorldViewProj = MathHelper::Identity();
	};

	class DrawBox :public D3D12App
	{
	public:
		DrawBox(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);
		DrawBox(const DrawBox& other) = delete;
		DrawBox& operator=(const DrawBox& other) = delete;
		virtual ~DrawBox();

		bool OnInit() override;

	private:
		void OnResize() override;
		void OnUpdate(const CpuTimer& timer) override;
		void OnRender(const CpuTimer& timer) override;
		void OnRenderScene(const CpuTimer& timer);

		bool InitResource();
		bool CreateBox();
		bool CreateRootSignature();
		bool CreatedPSO();

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;			// 根签名
		ComPtr<ID3D12PipelineState> m_PSO = nullptr;
		ComPtr<ID3DBlob> m_VSByteCode = nullptr;
		ComPtr<ID3DBlob> m_PSByteCode = nullptr;
		ComPtr<ID3D12DescriptorHeap> m_CbvHeap = nullptr;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

		std::unique_ptr<Geometry::MeshData> m_Box;
		std::unique_ptr<UploadBuffer<ObjConstants>> m_ObjCB;
	};
}

#endif // !__DRAWBOX__H__
