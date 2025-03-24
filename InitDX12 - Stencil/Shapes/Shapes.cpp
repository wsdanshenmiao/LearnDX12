#include "Shapes.h"
#include "ImGuiManager.h"
#include "ConstantData.h"
#include "Geometry.h"
#include "Vertex.h"
#include "Mesh.h"
#include "ObjectManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	LandAndWave::LandAndWave(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
	}

	LandAndWave::~LandAndWave()
	{
		if (m_D3D12Fence) {
			FlushCommandQueue();
		}
		ImGuiManager::ShutDown();
	}

	bool LandAndWave::OnInit()
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

		ModelManager::Create();

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

	void LandAndWave::OnResize()
	{
		D3D12App::OnResize();
	}

	void LandAndWave::OnUpdate(const CpuTimer& timer)
	{
		auto& imgui = ImGuiManager::GetInstance();
		imgui.Update(timer);

		m_CurrFRIndex = (m_CurrFRIndex + 1) % FrameCount;
		m_CurrFrameResource = m_FrameResources[m_CurrFRIndex].get();

		// 若CPU过快，可能会超前一个循环，此时需要CPU等待
		if (m_CurrFrameResource->m_Fence != 0 &&
			m_D3D12Fence->GetCompletedValue() < m_CurrFrameResource->m_Fence) {
			WaitForGpu();
		}

		UpdateFrameResource(timer);
	}

	void LandAndWave::UpdateFrameResource(const CpuTimer& timer)
	{
		auto& imgui = ImGuiManager::GetInstance();

		XMMATRIX view = XMMatrixLookAtLH(
			XMVectorSet(m_EyePos.x, m_EyePos.y, m_EyePos.z, 1.0f),
			XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		auto world = XMMatrixScalingFromVector(XMVectorReplicate(imgui.m_Scale)) *
			XMMatrixRotationX(imgui.m_Phi) * XMMatrixRotationY(imgui.m_Theta) *
			XMMatrixTranslation(imgui.m_Dx, imgui.m_Dy, 0.0f);
		auto proj = XMMatrixPerspectiveFovLH(imgui.m_Fov, GetAspectRatio(), 1.0f, 1000.0f);

		auto detView = XMMatrixDeterminant(view);
		auto detProj = XMMatrixDeterminant(proj);
		auto detWorld = XMMatrixDeterminant(world);
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invView = XMMatrixInverse(&detView, view);
		XMMATRIX invProj = XMMatrixInverse(&detProj, proj);
		XMMATRIX invWorld = XMMatrixInverse(&detWorld, world);

		PassConstants passConstants;
		XMStoreFloat4x4(&passConstants.m_View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&passConstants.m_InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&passConstants.m_Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&passConstants.m_InvProj, XMMatrixTranspose(invProj));
		passConstants.m_EyePosW = m_EyePos;
		passConstants.m_RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
		passConstants.m_InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
		passConstants.m_NearZ = 1.0f;
		passConstants.m_FarZ = 1000.0f;
		passConstants.m_TotalTime = timer.TotalTime();
		passConstants.m_DeltaTime = timer.DeltaTime();

		auto& currPassCB = m_CurrFrameResource->m_ConstantBuffers.find("PassConstants")->second;
		currPassCB->Map();
		currPassCB->m_IsDirty = true;
		currPassCB->CopyData(0, &passConstants, sizeof(PassConstants));
		currPassCB->Unmap();

		ObjectConstants objectConstants;
		XMStoreFloat4x4(&objectConstants.m_World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objectConstants.m_WorldInvTranspose, invWorld);
		auto& currObjCB = m_CurrFrameResource->m_ConstantBuffers.find("ObjectConstants")->second;
		currObjCB->Map();
		for (auto i = 0; i < GetMeshSize(); ++i) {
			currObjCB->m_IsDirty = true;
			currObjCB->CopyData(i, &objectConstants, sizeof(ObjectConstants));
		}
		currObjCB->Unmap();
	}

	std::size_t LandAndWave::GetMeshSize() const noexcept
	{
		return ModelManager::GetInstance().GetMeshSize();
	}

	void LandAndWave::OnRender(const CpuTimer& timer)
	{
		auto cmdListAlloc = m_CurrFrameResource->m_CmdListAlloc;
		ThrowIfFailed(cmdListAlloc->Reset());

		if (ImGuiManager::GetInstance().m_EnableWireFrame) {
			auto& pso = m_Enable4xMsaa ? m_PSOs["WireFrameMSAA"] : m_PSOs["WireFrame"];
			ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), pso.Get()));
		}
		else {
			auto& pso = m_Enable4xMsaa ? m_PSOs["OpaqueMSAA"] : m_PSOs["Opaque"];
			ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), pso.Get()));
		}

		m_CommandList->RSSetViewports(1, &m_ScreenViewport);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (m_Enable4xMsaa) {
			auto resolveToRt = CD3DX12_RESOURCE_BARRIER::Transition(
				m_MsaaRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_CommandList->ResourceBarrier(1, &resolveToRt);

			auto msaaRtv = m_MsaaRtvHeap->GetCPUDescriptorHandleForHeapStart();
			auto msaaDsv = m_MsaaDsvHeap->GetCPUDescriptorHandleForHeapStart();
			m_CommandList->ClearRenderTargetView(msaaRtv, Colors::Pink, 0, nullptr);
			m_CommandList->ClearDepthStencilView(
				msaaDsv,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				1.f, 0, 0, nullptr);

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
			m_CommandList->ResolveSubresource(
				GetCurrentBackBuffer(), 0,
				m_MsaaRenderTarget.Get(), 0,
				m_BackBufferFormat);

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
			m_CommandList->ClearDepthStencilView(
				dsv,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				1.f, 0, 0, nullptr);

			m_CommandList->OMSetRenderTargets(1, &currentBbv, true, &dsv);

			OnRenderScene(timer);

			auto RtToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);
			m_CommandList->ResourceBarrier(1, &RtToPresent);
		}

		ThrowIfFailed(m_CommandList->Close());

		ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		ThrowIfFailed(m_DxgiSwapChain->Present(0, 0));
		m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;

		// 推进当前帧资源的围栏值
		m_CurrFrameResource->m_Fence = ++m_CurrentFence;
		m_CommandQueue->Signal(m_D3D12Fence.Get(), m_CurrentFence);
	}

	void LandAndWave::OnRenderScene(const CpuTimer& timer)
	{
		// 设置根签名
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		// 设置当前帧资源的根描述符表
		int passCbvIndex = m_PassCbvOffset + m_CurrFRIndex;
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(passCbvIndex, m_CbvSrvUavDescriptorSize);
		m_CommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

		UINT currMeshIndex = 0;
		auto& objManager = ModelManager::GetInstance();
		for (const auto& drawArg : m_MeshData->m_DrawArgs) {
			auto& submesh = drawArg.second;
			auto vertexBV = m_MeshData->GetVertexBufferView();
			auto indexBV = m_MeshData->GetIndexBufferView();
			m_CommandList->IASetVertexBuffers(0, 1, &vertexBV);
			m_CommandList->IASetIndexBuffer(&indexBV);

			auto objCbvIndex = m_CurrFRIndex * GetMeshSize() + currMeshIndex++;
			auto objCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
			objCbvHandle.Offset(objCbvIndex, m_CbvSrvUavDescriptorSize);
			m_CommandList->SetGraphicsRootDescriptorTable(0, objCbvHandle);

			m_CommandList->DrawIndexedInstanced(
				submesh.m_IndexCount,
				1,
				submesh.m_StarIndexLocation,
				submesh.m_BaseVertexLocation,
				0);

		}
		ImGuiManager::GetInstance().RenderImGui(m_CommandList.Get());
	}

	bool LandAndWave::InitResource()
	{
		m_EyePos = { 0.0f, 0.0f, -5.0f };

		auto colorVS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_0");
		auto colorPS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_0");
		m_VSByteCodes.insert(std::make_pair("Color", colorVS));
		m_PSByteCodes.insert(std::make_pair("Color", colorPS));

		CreateGeometry();
		InitFrameResourceCB();
		CreateCBV();
		CreateRootSignature();
		CreatePSO();

		return true;
	}

	bool LandAndWave::InitFrameResourceCB()
	{
		auto meshCount = (UINT)GetMeshSize();
		m_PassCbvOffset = meshCount * FrameCount;	// 帧常量的偏移量

		// 创建常量缓冲区描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NumDescriptors = m_PassCbvOffset + FrameCount;		// 前面放置物体常量的描述符，后面放置帧常量
		cbvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_CbvHeap.GetAddressOf())));

		for (auto i = 0; i < FrameCount; ++i) {
			m_FrameResources[i] = std::make_unique<FrameResource>(m_D3D12Device.Get());
		}

		for (auto& resource : m_FrameResources) {
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(ObjectConstants),
				(UINT)GetMeshSize(),
				"ObjectConstants");
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(PassConstants),
				1,
				"PassConstants");
		}

		return true;
	}

	bool LandAndWave::CreateCBV()
	{
		auto objByteSize = D3DUtil::CalcCBByteSize(sizeof(ObjectConstants));
		auto passByteSize = D3DUtil::CalcCBByteSize(sizeof(PassConstants));

		// 创建每个帧资源的常量缓冲区视图
		for (auto frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			auto& currFrameResource = m_FrameResources[frameIndex];

			// 创建物体的常量缓冲区视图
			auto objCount = (UINT)GetMeshSize();
			auto objCB = currFrameResource->m_ConstantBuffers.find("ObjectConstants")->second->GetResource();
			for (std::size_t i = 0; i < objCount; ++i) {
				auto objCbvAdress = objCB->GetGPUVirtualAddress();	// 常量缓冲区的GPU虚拟首地址
				objCbvAdress += i * objByteSize;	// 对首地址进行偏移

				UINT objHeapIndex = (UINT)(frameIndex * objCount + i);
				// 获取描述符堆的首地址
				auto objHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetCPUDescriptorHandleForHeapStart());
				objHandle.Offset(objHeapIndex, m_CbvSrvUavDescriptorSize);

				// 创建常量缓冲区视图
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
				cbvDesc.SizeInBytes = objByteSize;
				cbvDesc.BufferLocation = objCbvAdress;
				m_D3D12Device->CreateConstantBufferView(&cbvDesc, objHandle);
			}


			// 创建每一帧的常量缓冲区视图
			auto passCB = currFrameResource->m_ConstantBuffers.find("PassConstants")->second->GetResource();
			auto passCbvAdress = passCB->GetGPUVirtualAddress();

			int passHeapIndex = m_PassCbvOffset + frameIndex;	// 偏移到帧常量区
			auto passHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetCPUDescriptorHandleForHeapStart());
			passHandle.Offset(passHeapIndex, m_CbvSrvUavDescriptorSize);

			// 创建常量缓冲区视图
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.SizeInBytes = passByteSize;
			cbvDesc.BufferLocation = passCbvAdress;
			m_D3D12Device->CreateConstantBufferView(&cbvDesc, passHandle);
		}

		return true;
	}

	bool LandAndWave::CreateGeometry()
	{
		auto boxMesh = GeometryGenerator::CreateBox(3, 2, 2, 0);
		auto geosphereMesh = GeometryGenerator::CreateGeosphere(4, 0);
		auto box = std::make_shared<Object>("Box");
		box->SetGeometryMesh(std::make_shared<GeometryMesh>(boxMesh));
		auto geosphere = std::make_shared<Object>("Geosphere");
		geosphere->SetGeometryMesh(std::make_shared<GeometryMesh>(geosphereMesh));


		auto& objManager = ModelManager::GetInstance();
		auto vertFunc = [](const Vertex& vert) {
			VertexPosColor ret{};
			ret.m_Pos = vert.m_Position;
			ret.m_Color = XMFLOAT4(Colors::Blue);
			return ret;
			};
		objManager.AddMesh(box);
		objManager.AddMesh(geosphere);
		m_MeshData = objManager.GetAllMeshData<VertexPosColor>(
			m_D3D12Device.Get(),
			m_CommandList.Get(),
			"AllObject",
			vertFunc);

		return true;
	}

	bool LandAndWave::CreateRootSignature()
	{
		auto CBSize = (UINT)m_FrameResources[0]->m_ConstantBuffers.size();
		// 创建根签名
		std::vector<CD3DX12_DESCRIPTOR_RANGE> cbvTables(CBSize);
		std::vector<CD3DX12_ROOT_PARAMETER> slotRootParameters(CBSize);
		for (UINT i = 0; i < CBSize; ++i) {
			// 创建描述符表
			cbvTables[i].Init(
				D3D12_DESCRIPTOR_RANGE_TYPE_CBV,	// 描述符表的类型
				1,		// 表中的描述符数量
				i);		// 将描述符区域绑定到基准着色器寄存器
			slotRootParameters[i].InitAsDescriptorTable(
				1,				// 描述符区域的数量
				&cbvTables[i]);		// 指向描述符数组的指针
		}

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			2, slotRootParameters.data(), 0, nullptr,
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

	bool LandAndWave::CreatePSO()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.VS = {
			m_VSByteCodes["Color"]->GetBufferPointer(),
			m_VSByteCodes["Color"]->GetBufferSize()
		};
		psoDesc.PS = {
			m_PSByteCodes["Color"]->GetBufferPointer(),
			m_PSByteCodes["Color"]->GetBufferSize()
		};
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = {
			VertexPosColor::GetInputLayout().data(),
			(UINT)VertexPosColor::GetInputLayout().size()
		};
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = m_BackBufferFormat;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		psoDesc.DSVFormat = m_DepthStencilFormat;
		ThrowIfFailed(m_D3D12Device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(m_PSOs["Opaque"].GetAddressOf())));

		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ThrowIfFailed(m_D3D12Device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(m_PSOs["WireFrame"].GetAddressOf())));

		psoDesc.SampleDesc.Count = 4;
		psoDesc.SampleDesc.Quality = (m_4xMsaaQuality - 1);
		ThrowIfFailed(m_D3D12Device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(m_PSOs["WireFrameMSAA"].GetAddressOf())));

		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		ThrowIfFailed(m_D3D12Device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(m_PSOs["OpaqueMSAA"].GetAddressOf())));

		return true;
	}

	void LandAndWave::WaitForGpu()
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
		ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrFrameResource->m_Fence, eventHandle));
		if (eventHandle != 0) {
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


}
