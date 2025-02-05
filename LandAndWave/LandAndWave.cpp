#include "LandAndWave.h"
#include "ImGuiManager.h"
#include "ConstantData.h"
#include "Geometry.h"
#include "Vertex.h"
#include "Mesh.h"
#include "MeshManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	LandAndWave::LandAndWave(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
	}

	LandAndWave::~LandAndWave()
	{
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

		StaticMeshManager::Create();

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
		UpdateObjResource(timer);
		UpdateWaves(timer);
	}

	void LandAndWave::UpdateFrameResource(const CpuTimer& timer)
	{
		auto& imgui = ImGuiManager::GetInstance();

		XMMATRIX view = XMMatrixLookAtLH(
			XMVectorSet(imgui.m_EyePos.x, imgui.m_EyePos.y, imgui.m_EyePos.z, 1.0f),
			XMVectorZero(),
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		auto proj = XMMatrixPerspectiveFovLH(imgui.m_Fov, GetAspectRatio(), 1.0f, 1000.0f);

		auto detView = XMMatrixDeterminant(view);
		auto detProj = XMMatrixDeterminant(proj);
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invView = XMMatrixInverse(&detView, view);
		XMMATRIX invProj = XMMatrixInverse(&detProj, proj);

		PassConstants passConstants;
		XMStoreFloat4x4(&passConstants.m_View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&passConstants.m_InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&passConstants.m_Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&passConstants.m_InvProj, XMMatrixTranspose(invProj));
		passConstants.m_EyePosW = imgui.m_EyePos;
		passConstants.m_RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
		passConstants.m_InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
		passConstants.m_NearZ = 1.0f;
		passConstants.m_FarZ = 1000.0f;
		passConstants.m_TotalTime = timer.TotalTime();
		passConstants.m_DeltaTime = timer.DeltaTime();

		auto& currPassCB = m_CurrFrameResource->m_ConstantBuffers.find("PassConstants")->second;
		currPassCB->m_IsDirty = true;
		currPassCB->Map();
		currPassCB->CopyData(0, &passConstants, sizeof(PassConstants));
		currPassCB->Unmap();
	}

	void LandAndWave::UpdateObjResource(const CpuTimer& timer)
	{
		auto& imgui = ImGuiManager::GetInstance();

		for (const auto& objPair : m_Objects) {
			auto& obj = objPair.second;
			XMMATRIX scale = imgui.m_Transform.GetScaleMatrix() * obj.GetTransform().GetScaleMatrix();
			XMMATRIX rotate = imgui.m_Transform.GetRotateMatrix() * obj.GetTransform().GetRotateMatrix();
			XMVECTOR position = XMVectorAdd(imgui.m_Transform.GetTranslation(),
				obj.GetTransform().GetTranslation());
			for (const auto& item : obj.GetAllRenderItems()) {
				scale *= item->m_Transform.GetScaleMatrix();
				rotate *= item->m_Transform.GetRotateMatrix();
				XMVectorAdd(position, item->m_Transform.GetTranslation());
				auto world = scale * rotate * XMMatrixTranslationFromVector(position);
				auto detWorld = XMMatrixDeterminant(world);
				XMMATRIX invWorld = XMMatrixInverse(&detWorld, world);
				ObjectConstants objectConstants;
				XMStoreFloat4x4(&objectConstants.m_World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&objectConstants.m_WorldInvTranspos, XMMatrixTranspose(invWorld));
				auto& currObjCB = m_CurrFrameResource->m_ConstantBuffers.find("ObjectConstants")->second;
				currObjCB->Map();
				currObjCB->m_IsDirty = true;
				currObjCB->CopyData(item->m_RenderCBIndex, &objectConstants, sizeof(ObjectConstants));
				currObjCB->Unmap();
			}
		}
	}

	void LandAndWave::UpdateWaves(const CpuTimer& gt)
	{
		// Every quarter second, generate a random wave.
		static float t_base = 0.0f;
		if ((gt.TotalTime() - t_base) >= 0.25f)
		{
			t_base += 0.25f;

			std::mt19937 gen(std::random_device{}());
			std::uniform_int_distribution<int> range(4, m_Waves->RowCount() - 5);
			int i = range(gen);
			decltype(range)::param_type colRnage{ 4, m_Waves->ColumnCount() - 5 };
			int j = range(gen);

			std::uniform_real_distribution<float> rangeF{ .2f, .5f };
			float r = rangeF(gen);

			m_Waves->Disturb(i, j, r);
		}

		// Update the wave simulation.
		m_Waves->Update(gt.DeltaTime());

		// Update the wave vertex buffer with the new solution.
		auto& currWavesVB = m_CurrFrameResource->m_ConstantBuffers.find("WavesVertex")->second;
		currWavesVB->Map();
		for (int i = 0; i < m_Waves->VertexCount(); ++i)
		{
			VertexPosLColor v;

			v.m_Pos = m_Waves->Position(i);
			v.m_Color = XMFLOAT4(DirectX::Colors::Blue);
			currWavesVB->m_IsDirty = true;
			currWavesVB->CopyData(i, &v, sizeof(VertexPosLColor));
		}
		currWavesVB->Unmap();

		// Set the dynamic VB of the wave renderitem to the current frame VB.
		m_Objects["Wave"].GetRenderItem("Wave")->m_Mesh->m_VertexBufferGPU = currWavesVB->GetResource();
	}

	std::size_t LandAndWave::GetAllRenderItemsCount() const noexcept
	{
		std::size_t count = 0;
		for (const auto& obj : m_Objects) {
			count += obj.second.GetRenderItemsCount();
		}
		return count;
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
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		// 设置当前帧资源的根描述符表
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		int passCbvIndex = m_PassCbvOffset + m_CurrFRIndex;
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(passCbvIndex, m_CbvSrvUavDescriptorSize);
		m_CommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

		UINT currMeshIndex = 0;
		for (const auto& obj : m_Objects) {
			for (const auto& item : obj.second.GetAllRenderItems()) {
				auto verticesBV = item->m_Mesh->GetVertexBufferView();
				auto indicesBV = item->m_Mesh->GetIndexBufferView();
				m_CommandList->IASetVertexBuffers(0, 1, &verticesBV);
				m_CommandList->IASetIndexBuffer(&indicesBV);

				auto objCbvIndex = m_CurrFRIndex * GetAllRenderItemsCount() + item->m_RenderCBIndex;
				auto objCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
				objCbvHandle.Offset(objCbvIndex, m_CbvSrvUavDescriptorSize);
				m_CommandList->SetGraphicsRootDescriptorTable(0, objCbvHandle);

				auto& submesh = item->m_Mesh->m_DrawArgs[item->m_Name];
				m_CommandList->DrawIndexedInstanced(
					submesh.m_IndexCount,
					1,
					submesh.m_StarIndexLocation,
					submesh.m_BaseVertexLocation,
					0);
			}
		}
		ImGuiManager::GetInstance().RenderImGui(m_CommandList.Get());
	}

	bool LandAndWave::InitResource()
	{
		ImGuiManager::GetInstance().m_EyePos = { 0.0f, 0.0f, -5.0f };

		auto colorVS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_0");
		auto colorPS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_0");
		m_VSByteCodes.insert(std::make_pair("Color", colorVS));
		m_PSByteCodes.insert(std::make_pair("Color", colorPS));
		m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

		CreateGeometry();
		InitFrameResourceCB();
		CreateCBV();
		CreateRootSignature();
		CreatePSO();

		return true;
	}

	bool LandAndWave::InitFrameResourceCB()
	{
		auto meshCount = (UINT)GetAllRenderItemsCount();
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
			// 创建常量缓冲区
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(ObjectConstants),
				(UINT)meshCount,
				"ObjectConstants");
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(PassConstants),
				1,
				"PassConstants");
			// 创建动态缓冲区
			resource->AddDynamicBuffer(
				m_D3D12Device.Get(),
				sizeof(VertexPosLColor),
				m_Waves->VertexCount(),
				"WavesVertex");
		}

		return true;
	}

	bool LandAndWave::CreateCBV()
	{
		auto objByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		auto passByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

		// 创建每个帧资源的常量缓冲区视图
		for (auto frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			auto& currFrameResource = m_FrameResources[frameIndex];

			// 创建物体的常量缓冲区视图
			auto itemCount = (UINT)GetAllRenderItemsCount();
			auto objCB = currFrameResource->m_ConstantBuffers.find("ObjectConstants")->second->GetResource();
			for (std::size_t i = 0; i < itemCount; ++i) {
				auto objCbvAdress = objCB->GetGPUVirtualAddress();	// 常量缓冲区的GPU虚拟首地址
				objCbvAdress += i * objByteSize;	// 对首地址进行偏移

				UINT objHeapIndex = (UINT)(frameIndex * itemCount + i);
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
		// 生成网格数据
		auto& meshManager = StaticMeshManager::GetInstance();
		meshManager.AddMesh("Box", GeometryGenerator::CreateBox(3, 2, 2, 0));
		meshManager.AddMesh("Geosphere", GeometryGenerator::CreateGeosphere(4, 0));
		meshManager.AddMesh("Grid", GeometryGenerator::CreateGrid(100, 100, 50, 50));

		auto getHeight = [](float x, float z) {
			return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
			};

		auto vertFunc = [&getHeight](const Vertex& vert) {
			VertexPosLColor ret{};
			ret.m_Pos = vert.m_Position;
			ret.m_Pos.y = getHeight(ret.m_Pos.x, ret.m_Pos.z);
			float y = ret.m_Pos.y;
			if (y < -10.0f) {
				ret.m_Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
			}
			else if (y < 5.0f) {
				ret.m_Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
			}
			else if (y < 12.0f) {
				ret.m_Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
			}
			else if (y < 20.0f) {
				ret.m_Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
			}
			else {
				ret.m_Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			}
			return ret;
			};
		m_MeshData["Grid"] = meshManager.GetAllMeshData<VertexPosLColor>(
			m_D3D12Device.Get(),
			m_CommandList.Get(),
			"AllObject",
			vertFunc);

		RenderItem grid;
		grid.m_Name = "Grid";
		grid.m_Mesh = m_MeshData["Grid"].get();
		grid.m_NumFramesDirty = FrameCount;
		grid.m_RenderCBIndex = 0;
		Object gridObj{ grid.m_Name };
		gridObj.AddRenderItem(std::make_shared<RenderItem>(std::move(grid)));
		m_Objects.insert(std::make_pair(gridObj.m_Name, std::move(gridObj)));

		//RenderItem box;
		//box.m_Name = "Box";
		//box.m_NumFramesDirty = FrameCount;
		//box.m_RenderCBIndex = 1;
		//Object boxObj{ box.m_Name };
		//boxObj.AddRenderItem(std::make_shared<RenderItem>(std::move(box)));
		//m_Objects.push_back(std::move(boxObj));
		//
		//RenderItem geosphere;
		//geosphere.m_Name = "Geosphere";
		//geosphere.m_NumFramesDirty = FrameCount;
		//geosphere.m_RenderCBIndex = 2;
		//Object geosphereObj{ geosphere.m_Name };
		//geosphereObj.AddRenderItem(std::make_shared<RenderItem>(std::move(geosphere)));
		//m_Objects.push_back(std::move(geosphereObj));


		// 初始化海平面
		std::vector<std::uint16_t> indices(3 * m_Waves->TriangleCount()); // 3 indices per face
		assert(m_Waves->VertexCount() < 0x0000ffff);

		// Iterate over each quad.
		int m = m_Waves->RowCount();
		int n = m_Waves->ColumnCount();
		int k = 0;
		for (int i = 0; i < m - 1; ++i)
		{
			for (int j = 0; j < n - 1; ++j)
			{
				indices[k] = i * n + j;
				indices[k + 1] = i * n + j + 1;
				indices[k + 2] = (i + 1) * n + j;

				indices[k + 3] = (i + 1) * n + j;
				indices[k + 4] = i * n + j + 1;
				indices[k + 5] = (i + 1) * n + j + 1;

				k += 6; // next quad
			}
		}

		UINT vbByteSize = m_Waves->VertexCount() * sizeof(Vertex);
		UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto mesh = std::make_unique<MeshData>();
		mesh->m_Name = "Wave";
		mesh->m_VertexBufferCPU = nullptr;
		mesh->m_VertexBufferGPU = nullptr;
		ThrowIfFailed(D3DCreateBlob(ibByteSize, mesh->m_IndexBufferCPU.GetAddressOf()));
		mesh->m_IndexBufferGPU = D3DUtil::CreateDefaultBuffer(
			m_D3D12Device.Get(),
			m_CommandList.Get(),
			indices.data(),
			ibByteSize,
			mesh->m_IndexBufferUploader);
		mesh->m_VertexBufferByteSize = vbByteSize;
		mesh->m_VertexByteStride = sizeof(VertexPosLColor);
		mesh->m_IndexBufferByteSize = ibByteSize;
		mesh->m_IndexSize = indices.size();
		mesh->m_IndexFormat = DXGI_FORMAT_R16_UINT;

		SubmeshData submesh{};
		submesh.m_BaseVertexLocation = 0;
		submesh.m_StarIndexLocation = 0;
		submesh.m_IndexCount = (UINT)indices.size();

		mesh->m_DrawArgs["Wave"] = submesh;

		m_MeshData["Wave"] = std::move(mesh);

		RenderItem wave;
		wave.m_Name = "Wave";
		wave.m_Mesh = m_MeshData["Wave"].get();
		wave.m_NumFramesDirty = FrameCount;
		wave.m_RenderCBIndex = 1;
		Object waves("Wave");
		waves.AddRenderItem(std::make_shared<RenderItem>(std::move(wave)));
		m_Objects.insert(std::make_pair(waves.m_Name, std::move(waves)));

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
			VertexPosLColor::GetInputLayout().data(),
			(UINT)VertexPosLColor::GetInputLayout().size()
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
