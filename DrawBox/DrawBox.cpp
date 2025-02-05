#include "DrawBox.h"
#include "ImGuiManager.h"
#include "Transform.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {

	DrawBox::DrawBox(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
	}

	DrawBox::~DrawBox()
	{
	}

	bool DrawBox::OnInit()
	{
		if (!D3D12App::OnInit())
			return false;
		ImGuiManager::Create();
		if (!ImGuiManager::GetInstance().InitImGui(
			m_D3D12Device.Get(),
			m_hMainWnd,
			SwapChainBufferCount,
			m_BackBufferFormat))
			return false;

		// 为初始化资源重置命令列表
		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));
		if (!InitResource())
			return false;
		// 提交命令列表
		ThrowIfFailed(m_CommandList->Close());
		ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		FlushCommandQueue();

		return true;
	}

	void DrawBox::OnResize()
	{
		D3D12App::OnResize();
	}

	void DrawBox::OnUpdate(const CpuTimer& timer)
	{
		auto& imgui = ImGuiManager::GetInstance();
		imgui.Update(timer);

		XMMATRIX view = XMMatrixLookAtLH(
			XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
			XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		ObjConstants objConstants;
		auto world = XMMatrixScalingFromVector(XMVectorReplicate(imgui.m_Scale)) *
			XMMatrixRotationX(imgui.m_Phi) * XMMatrixRotationY(imgui.m_Theta) *
			XMMatrixTranslation(imgui.m_Dx, imgui.m_Dy, 0.0f);
		auto proj = XMMatrixPerspectiveFovLH(imgui.m_Fov, GetAspectRatio(), 1.0f, 1000.0f);
		XMStoreFloat4x4(&objConstants.gWorldViewProj, XMMatrixTranspose(world * view * proj));
		m_ObjCB->Map();
		m_ObjCB->m_IsDirty = true;
		m_ObjCB->CopyData(0, &objConstants, sizeof(ObjConstants));
		m_ObjCB->Unmap();
	}

	void DrawBox::OnRender(const CpuTimer& timer)
	{
		FlushCommandQueue();

		ThrowIfFailed(m_DirectCmdListAlloc->Reset());
		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), m_PSO.Get()));

		m_CommandList->RSSetViewports(1, &m_ScreenViewport);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

		if (m_Enable4xMsaa) {
			auto resolveToRt = CD3DX12_RESOURCE_BARRIER::Transition(
				m_MsaaRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_CommandList->ResourceBarrier(1, &resolveToRt);

			auto msaaRtv = m_MsaaRtvHeap->GetCPUDescriptorHandleForHeapStart();
			auto msaaDsv = m_MsaaDsvHeap->GetCPUDescriptorHandleForHeapStart();
			m_CommandList->ClearRenderTargetView(msaaRtv, Colors::Pink, 0, nullptr);
			m_CommandList->ClearDepthStencilView(msaaDsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			m_CommandList->OMSetRenderTargets(1, &msaaRtv, true, &msaaDsv);

			OnRenderScene(timer);

			// 转换资源状态
			D3D12_RESOURCE_BARRIER barriers[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(
					m_MsaaRenderTarget.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(
					GetCurrentBackBuffer(),
					D3D12_RESOURCE_STATE_PRESENT,
					D3D12_RESOURCE_STATE_RESOLVE_DEST)
			};
			m_CommandList->ResourceBarrier(2, barriers);

			// 将Rt解析到后台缓冲区
			m_CommandList->ResolveSubresource(GetCurrentBackBuffer(), 0, m_MsaaRenderTarget.Get(), 0, m_BackBufferFormat);

			// 转换后台缓冲区
			auto destToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_RESOLVE_DEST,
				D3D12_RESOURCE_STATE_PRESENT);
			m_CommandList->ResourceBarrier(1, &destToPresent);
		}
		else {
			auto presentToRt = CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_CommandList->ResourceBarrier(1, &presentToRt);

			auto currentBbv = GetCurrentBackBufferView();
			auto dsv = GetDepthStencilView();
			m_CommandList->ClearRenderTargetView(currentBbv, Colors::Pink, 0, nullptr);
			m_CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			m_CommandList->OMSetRenderTargets(1, &currentBbv, true, &dsv);

			OnRenderScene(timer);

			auto RtToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);
			m_CommandList->ResourceBarrier(1, &RtToPresent);
		}

		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ThrowIfFailed(m_CommandList->Close());

		ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(1, cmdsLists);

		ThrowIfFailed(m_DxgiSwapChain->Present(0, 0));
		m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;

		FlushCommandQueue();
	}

	void DrawBox::OnRenderScene(const CpuTimer& timer)
	{
		// 设置根签名
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		m_CommandList->SetGraphicsRootDescriptorTable(0, m_CbvHeap->GetGPUDescriptorHandleForHeapStart());

		auto vertexBV = m_Box->GetVertexBufferView();
		auto indexBV = m_Box->GetIndexBufferView();
		m_CommandList->IASetVertexBuffers(0, 1, &vertexBV);
		m_CommandList->IASetIndexBuffer(&indexBV);
		auto& box = m_Box->m_DrawArgs["Box"];
		m_CommandList->DrawIndexedInstanced(
			box.m_IndexCount,
			1,
			box.m_BaseVertexLocation,
			box.m_StarIndexLocation,
			0);

		ImGuiManager::GetInstance().RenderImGui(m_CommandList.Get());
	}

	bool DrawBox::InitResource()
	{
		CreateBox();

		// 创建常量缓冲区描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NumDescriptors = 1;
		cbvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_CbvHeap.GetAddressOf())));

		// 创建常量缓冲区及其视图
		m_ObjCB = std::make_unique<UploadBuffer<ObjConstants>>(m_D3D12Device.Get(), sizeof(ObjConstants), 1, true);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = m_ObjCB->GetResource()->GetGPUVirtualAddress();;
		cbvDesc.SizeInBytes = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjConstants));
		m_D3D12Device->CreateConstantBufferView(&cbvDesc, m_CbvHeap->GetCPUDescriptorHandleForHeapStart());

		m_VSByteCode = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_0");
		m_PSByteCode = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_0");
		m_InputLayout = {
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
		};

		CreateRootSignature();

		CreatedPSO();

		return true;
	}

	bool DrawBox::CreateBox()
	{
		// 设置顶点
		std::array<VertexPosLColor, 8> vertexs = {
			VertexPosLColor({ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT4(Colors::White) }),
			VertexPosLColor({ XMFLOAT3(-1.0f, +1.0f, -1.0f),XMFLOAT4(Colors::Black) }),
			VertexPosLColor({ XMFLOAT3(+1.0f, +1.0f, -1.0f),XMFLOAT4(Colors::Red) }),
			VertexPosLColor({ XMFLOAT3(+1.0f, -1.0f, -1.0f),XMFLOAT4(Colors::Green) }),
			VertexPosLColor({ XMFLOAT3(-1.0f, -1.0f, +1.0f),XMFLOAT4(Colors::Blue) }),
			VertexPosLColor({ XMFLOAT3(-1.0f, +1.0f, +1.0f),XMFLOAT4(Colors::Yellow) }),
			VertexPosLColor({ XMFLOAT3(+1.0f, +1.0f, +1.0f),XMFLOAT4(Colors::Cyan) }),
			VertexPosLColor({ XMFLOAT3(+1.0f, -1.0f, +1.0f),XMFLOAT4(Colors::Magenta) })
		};

		// 设置索引
		std::array<std::uint16_t, 36> indices =
		{
			// front face
			0, 1, 2,
			0, 2, 3,
			// back face
			4, 6, 5,
			4, 7, 6,
			// left face
			4, 5, 1,
			4, 1, 0,
			// right face
			3, 2, 6,
			3, 6, 7,
			// top face
			1, 5, 6,
			1, 6, 2,
			// bottom face
			4, 0, 3,
			4, 3, 7
		};


		const UINT64 vbByteSize = vertexs.size() * sizeof(VertexPosLColor);
		const UINT64 ibByteSize = indices.size() * sizeof(std::uint16_t);

		m_Box = std::make_unique<MeshData>();
		m_Box->m_Name = "Box";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, m_Box->m_VertexBufferCPU.GetAddressOf()));
		memcpy(m_Box->m_VertexBufferCPU->GetBufferPointer(), vertexs.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, m_Box->m_IndexBufferCPU.GetAddressOf()));
		memcpy(m_Box->m_IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		// 创建顶点和索引缓冲区
		m_Box->m_VertexBufferGPU = D3DUtil::CreateDefaultBuffer(
			m_D3D12Device.Get(), m_CommandList.Get(), vertexs.data(), vbByteSize, m_Box->m_VertexBufferUploader);
		m_Box->m_IndexBufferGPU = D3DUtil::CreateDefaultBuffer(
			m_D3D12Device.Get(), m_CommandList.Get(), indices.data(), ibByteSize, m_Box->m_IndexBufferUploader);

		m_Box->m_VertexBufferByteSize = vbByteSize;
		m_Box->m_IndexBufferByteSize = ibByteSize;
		m_Box->m_VertexByteStride = sizeof(VertexPosLColor);
		m_Box->m_IndexFormat = DXGI_FORMAT_R16_UINT;

		SubmeshData subMesh{};
		subMesh.m_IndexCount = (UINT)indices.size();
		subMesh.m_StarIndexLocation = 0;
		subMesh.m_BaseVertexLocation = 0;

		m_Box->m_DrawArgs.emplace(std::make_pair("Box", subMesh));

		return true;
	}

	bool DrawBox::CreateRootSignature()
	{
		// 创建根签名
		// 创建一个根参数来将含有一个CBV的描述符表绑定到 HLSL 中的 register(0)
		// 根参数可以是描述符表，根描述符，根常量
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];
		// 创建只有一个CBV的描述符表
		CD3DX12_DESCRIPTOR_RANGE cbvTable;
		cbvTable.Init(
			D3D12_DESCRIPTOR_RANGE_TYPE_CBV,	// 描述符表的类型
			1,		// 表中的描述符数量
			0);		// 将描述符区域绑定到基准着色器寄存器

		slotRootParameter[0].InitAsDescriptorTable(
			1,				// 描述符区域的数量
			&cbvTable);		// 指向描述符数组的指针

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			1, slotRootParameter, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// 创建含一个槽位的根签名
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		// 创建根签名之前需要对根签名的描述布局进行序列化，转换为以ID3DBlob接口表示的序列化数据格式
		auto hr = D3D12SerializeRootSignature(
			&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(),
			errorBlob.GetAddressOf());

		if (errorBlob) {
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		// 创建根签名
		ThrowIfFailed(m_D3D12Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(m_RootSignature.GetAddressOf())));

		return true;
	}

	bool DrawBox::CreatedPSO()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.VS = {
			m_VSByteCode->GetBufferPointer(),
			m_VSByteCode->GetBufferSize()
		};
		psoDesc.PS = {
			m_PSByteCode->GetBufferPointer(),
			m_PSByteCode->GetBufferSize()
		};
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = {
			m_InputLayout.data(),
			(UINT)m_InputLayout.size()
		};
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = m_BackBufferFormat;
		psoDesc.SampleDesc.Count = m_Enable4xMsaa ? 4 : 1;
		psoDesc.SampleDesc.Quality = m_Enable4xMsaa ? (m_4xMsaaQuality - 1) : 0;
		psoDesc.DSVFormat = m_DepthStencilFormat;
		m_D3D12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_PSO.GetAddressOf()));

		return true;
	}



}