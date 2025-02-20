#include "LightApp.h"
#include "ImguiManager.h"
#include "Vertex.h"
#include "Model.h"
#include "ModelManager.h"
#include "LightManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	LightApp::LightApp(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
	}

	bool LightApp::OnInit()
	{
		if (!D3D12App::OnInit()) {
			return false;
		}

		LightManager::Create();
		ModelManager::Create();
		ImguiManager::Create();
		if (!ImguiManager::GetInstance().InitImGui(
			m_D3D12Device.Get(),
			m_hMainWnd,
			FrameCount,
			m_BackBufferFormat)) {
			return false;
		}

		// 初始化相关资源
		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));
		if (!InitResource()) {
			return false;
		}
		// 提交列表
		ThrowIfFailed(m_CommandList->Close());
		ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(1, cmdLists);
		FlushCommandQueue();

		return true;
	}

	void LightApp::OnUpdate(const CpuTimer& timer)
	{
		ImguiManager::GetInstance().Update(timer);

		m_CurrFRIndex = (m_CurrFRIndex + 1) % FrameCount;
		m_CurrFrameResource = m_FrameResources[m_CurrFRIndex].get();

		// 若CPU过快，可能会超前一个循环，此时需要CPU等待,
		// GetCompletedValue()获取当前的栅栏值
		if (m_CurrFrameResource->m_Fence != 0 &&
			m_D3D12Fence->GetCompletedValue() < m_CurrFrameResource->m_Fence) {
			WaitForGPU();
		}

		UpdateFrameResource(timer);
		UpdateObjResource(timer);
		LightManager::GetInstance().UpdateLight(m_CurrFrameResource);
	}

	void LightApp::UpdateFrameResource(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();

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

	void LightApp::UpdateObjResource(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();

		for (const auto& [name, obj] : m_Objects) {
			auto& transform = obj.GetTransform();
			XMMATRIX scale = imgui.m_Transform.GetScaleMatrix() * transform.GetScaleMatrix();
			XMMATRIX rotate = imgui.m_Transform.GetRotateMatrix() * transform.GetRotateMatrix();
			XMVECTOR position = XMVectorAdd(imgui.m_Transform.GetTranslation(), transform.GetTranslation());
			for (const auto& item : obj.GetAllRenderItems()) {
				scale *= item->m_Transform.GetScaleMatrix();
				rotate *= item->m_Transform.GetRotateMatrix();
				XMVectorAdd(position, item->m_Transform.GetTranslation());
				auto world = scale * rotate * XMMatrixTranslationFromVector(position);
				auto detWorld = XMMatrixDeterminant(world);
				auto W = world;
				W.r[3] = g_XMIdentityR3;
				XMMATRIX invWorld = XMMatrixInverse(&detWorld, W);
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

	void LightApp::OnRender(const CpuTimer& timer)
	{
		auto& cmdListAlloc = m_CurrFrameResource->m_CmdListAlloc;
		ThrowIfFailed(cmdListAlloc->Reset());

		if (ImguiManager::GetInstance().m_EnableWireFrame) {
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

			OnRenderScene();

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

			OnRenderScene();

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

	void LightApp::OnRenderScene()
	{
		// 设置根签名
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

		// 设置当前帧资源的根描述符表
		ID3D12DescriptorHeap* passDescriptorHeaps[] = { m_PassCbv.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(passDescriptorHeaps), passDescriptorHeaps);
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_PassCbv->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(m_CurrFRIndex, m_CbvSrvUavDescriptorSize);
		m_CommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

		auto lightCbv = LightManager::GetInstance().GetLightCbv();
		ID3D12DescriptorHeap* lightDescriptorHeaps[] = { lightCbv };
		m_CommandList->SetDescriptorHeaps(_countof(lightDescriptorHeaps), lightDescriptorHeaps);
		auto lightCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(lightCbv->GetGPUDescriptorHandleForHeapStart());
		lightCbvHandle.Offset(m_CurrFRIndex, m_CbvSrvUavDescriptorSize);
		m_CommandList->SetGraphicsRootDescriptorTable(2, lightCbvHandle);

		ID3D12DescriptorHeap* matDescriptorHeaps[] = { m_MaterialCbv.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(matDescriptorHeaps), matDescriptorHeaps);
		auto matCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_MaterialCbv->GetGPUDescriptorHandleForHeapStart());
		matCbvHandle.Offset(m_CurrFRIndex, m_CbvSrvUavDescriptorSize);
		m_CommandList->SetGraphicsRootDescriptorTable(3, matCbvHandle);

		UINT currMeshIndex = 0;
		for (const auto& [name, obj] : m_Objects) {
			for (const auto& item : obj.GetAllRenderItems()) {
				auto verticesBV = item->m_Mesh->GetVertexBufferView();
				auto indicesBV = item->m_Mesh->GetIndexBufferView();
				m_CommandList->IASetVertexBuffers(0, 1, &verticesBV);
				m_CommandList->IASetIndexBuffer(&indicesBV);

				ID3D12DescriptorHeap* objDescriptorHeaps[] = { m_ObjCbv.Get() };
				m_CommandList->SetDescriptorHeaps(_countof(objDescriptorHeaps), objDescriptorHeaps);
				auto objCbvIndex = m_CurrFRIndex * m_RenderObjCount + item->m_RenderCBIndex;
				auto objCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_ObjCbv->GetGPUDescriptorHandleForHeapStart());
				objCbvHandle.Offset(objCbvIndex, m_CbvSrvUavDescriptorSize);
				m_CommandList->SetGraphicsRootDescriptorTable(0, objCbvHandle);

				auto& submesh = item->m_Mesh->m_DrawArgs.find(item->m_Name)->second;
				m_CommandList->DrawIndexedInstanced(
					submesh.m_IndexCount,
					1,
					submesh.m_StarIndexLocation,
					submesh.m_BaseVertexLocation,
					0);
			}
		}
		ImguiManager::GetInstance().RenderImGui(m_CommandList.Get());
	}

	void LightApp::WaitForGPU()
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
		ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrFrameResource->m_Fence, eventHandle));
		if (eventHandle != 0) {
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	bool LightApp::ImportModel()
	{
		// 加载模型
		Model* elenaModel = new Model{ "Elena", "Models\\Elena.obj" };
		auto& meshManager = ModelManager::GetInstance();
		auto& mesh = elenaModel->GetAllMesh();
		auto vertFunc = [](const Vertex& vert) {
			VertexPosLNormal ret{};
			ret.m_Pos = vert.m_Position;
			ret.m_Normal = vert.m_Normal;
			//ret.m_Color = { 1,1,1,1 };
			return ret;
			};
		// 加入网格
		for (auto& m : mesh) {
			auto newMesh = m.m_Mesh;
			meshManager.AddMesh(m.m_Name, std::move(newMesh));
		}
		m_MeshData[elenaModel->GetName()] = meshManager.GetAllMeshData<VertexPosLNormal>(
			m_D3D12Device.Get(),
			m_CommandList.Get(),
			elenaModel->GetName(),
			vertFunc);

		// 添加绘制对象
		Object itemObj{ elenaModel->GetName() };
		for (UINT i = 0; i < mesh.size(); ++i) {
			RenderItem item;
			item.m_Name = mesh[i].m_Name;
			item.m_Mesh = m_MeshData[elenaModel->GetName()].get();
			item.m_NumFramesDirty = FrameCount;
			item.m_RenderCBIndex = i;
			item.m_Transform.SetScale(1, 1, 1);
			item.m_Material = &elenaModel->GetMaterial(mesh[i].m_MaterialIndex);
			itemObj.AddRenderItem(std::make_shared<RenderItem>(std::move(item)));
		}
		auto& transform = itemObj.GetTransform();
		transform.SetScale(4, 4, 4);
		auto prePos = transform.GetPosition();
		transform.SetPosition(prePos.x, -40, prePos.z);
		itemObj.SetModel(elenaModel);
		m_Objects.insert(std::make_pair(itemObj.m_Name, std::move(itemObj)));

		return true;
	}

	bool LightApp::InitResource()
	{
		CreateLight();
		ImportModel();
		CreateShaderBlob();
		CreateFrameResource();
		CreateCBV();
		InitMaterials();
		CreateRootSignature();
		CreatePSO();

		return true;
	}

	void LightApp::CreateLight()
	{
		auto& lightManager = LightManager::GetInstance();
		DirectionalLight dirLight0{};
		dirLight0.m_Color = { 1,1,1 };
		dirLight0.m_Dir = { 0,-0.7,0.2 };
		PointLight pointLight0{};
		pointLight0.m_Color = { 1,1,1 };
		pointLight0.m_Pos = { -4,10,-8 };
		pointLight0.m_StartAtten = 5;
		pointLight0.m_EndAtten = 10;
		SpotLight spotLight0{};
		spotLight0.m_Pos = { 0,50,0 };
		spotLight0.m_Dir = { 0,-1,0 };
		spotLight0.m_Color = { 1,1,1 };
		spotLight0.m_StartAtten = 20;
		spotLight0.m_EndAtten = 30;
		spotLight0.m_SpotPower = 0.5;
		lightManager.SetDirLight(0, std::move(dirLight0));
		lightManager.SetPointLight(0, std::move(pointLight0));
		//lightManager.SetSpotLight(0, std::move(spotLight0));
	}

	void LightApp::CreateShaderBlob()
	{
		auto& lightManager = LightManager::GetInstance();
		auto dirLightCount = std::to_string(lightManager.GetDirLightCount());
		auto pointLightCount = std::to_string(lightManager.GetPointLightCount());
		auto spotLightCount = std::to_string(lightManager.GetSpotLightCount());
		D3D_SHADER_MACRO shaderMacro[] = {
			{"MAXDIRLIGHTCOUNT", dirLightCount.c_str()},
			{"MAXPOINTLIGHTCOUNT", pointLightCount.c_str()},
			{"MAXSPOTLIGHTCOUNT", spotLightCount.c_str()},
			{nullptr, nullptr}
		};
		auto colorVS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_1");
		auto colorPS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_1");
		m_VSByteCodes.insert(std::make_pair("Color", colorVS));
		m_PSByteCodes.insert(std::make_pair("Color", colorPS));

		auto lightVS = D3DUtil::CompileShader(L"Shaders\\Light.hlsl", shaderMacro, "VS", "vs_5_1");
		auto lightPS = D3DUtil::CompileShader(L"Shaders\\Light.hlsl", shaderMacro, "PS", "ps_5_1");
		m_VSByteCodes.insert(std::make_pair("Light", lightVS));
		m_PSByteCodes.insert(std::make_pair("Light", lightPS));
	}

	/// <summary>
	/// 创建帧资源
	/// </summary>
	void LightApp::CreateFrameResource()
	{
		UINT renderItemSize = 0;
		for (const auto& [name, obj] : m_Objects) {
			renderItemSize += (UINT)obj.GetRenderItemsCount();
		}
		m_RenderObjCount = std::max<UINT>(renderItemSize, 1);

		for (auto& frameResource : m_FrameResources) {
			frameResource = std::make_unique<FrameResource>(m_D3D12Device.Get());
			// 添加常量缓冲区
			frameResource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(ObjectConstants),
				m_RenderObjCount,
				"ObjectConstants");
			frameResource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(PassConstants),
				1,
				"PassConstants");
			frameResource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(MaterialConstants),
				m_RenderObjCount,
				"MaterialConstants");
			frameResource->AddConstantBuffer(
				LightManager::GetInstance().CreateLightBuffer(m_D3D12Device.Get()));
		}
	}

	void LightApp::CreateCBV()
	{
		// 创建常量描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NumDescriptors = m_RenderObjCount * FrameCount;
		heapDesc.NodeMask = 0;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(
			&heapDesc, IID_PPV_ARGS(m_ObjCbv.GetAddressOf())));
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(
			&heapDesc, IID_PPV_ARGS(m_MaterialCbv.GetAddressOf())));
		heapDesc.NumDescriptors = FrameCount;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(
			&heapDesc, IID_PPV_ARGS(m_PassCbv.GetAddressOf())));

		// 计算常量缓冲区的大小
		auto objCbvByteSize = D3DUtil::CalcCBByteSize(sizeof(ObjectConstants));
		auto passCbvByteSize = D3DUtil::CalcCBByteSize(sizeof(PassConstants));
		auto matCbvByteSize = D3DUtil::CalcCBByteSize(sizeof(MaterialConstants));

		// 创建对应视图
		for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			auto& constBuffer = m_FrameResources[frameIndex]->m_ConstantBuffers;
			for (UINT i = 0; i < m_RenderObjCount; ++i) {
				auto objCB = constBuffer.find("ObjectConstants")->second->GetResource();
				// 常量缓冲区虚拟地址
				auto objCbvAdress = objCB->GetGPUVirtualAddress();
				objCbvAdress += i * objCbvByteSize;

				D3D12_CONSTANT_BUFFER_VIEW_DESC objCbvDesc{};
				objCbvDesc.BufferLocation = objCbvAdress;
				objCbvDesc.SizeInBytes = objCbvByteSize;

				auto handleOffset = frameIndex * m_RenderObjCount + i;
				auto objHandle = m_ObjCbv->GetCPUDescriptorHandleForHeapStart();
				objHandle.ptr += handleOffset * m_CbvSrvUavDescriptorSize;

				m_D3D12Device->CreateConstantBufferView(&objCbvDesc, objHandle);


				// 创建材质的资源视图
				auto matCB = constBuffer.find("MaterialConstants")->second->GetResource();
				auto matCbvAdress = matCB->GetGPUVirtualAddress();
				matCbvAdress += i * matCbvByteSize;

				D3D12_CONSTANT_BUFFER_VIEW_DESC matCbvDesc{};
				matCbvDesc.BufferLocation = matCbvAdress;
				matCbvDesc.SizeInBytes = matCbvByteSize;

				auto matHandle = m_MaterialCbv->GetCPUDescriptorHandleForHeapStart();
				matHandle.ptr += handleOffset * m_CbvSrvUavDescriptorSize;

				m_D3D12Device->CreateConstantBufferView(&matCbvDesc, matHandle);
			}

			// 创建帧常量缓冲区
			auto passCB = constBuffer.find("PassConstants")->second->GetResource();
			auto passCbvAdress = passCB->GetGPUVirtualAddress();

			D3D12_CONSTANT_BUFFER_VIEW_DESC passCbvDesc{};
			passCbvDesc.BufferLocation = passCbvAdress;
			passCbvDesc.SizeInBytes = passCbvByteSize;

			auto passHandle = m_PassCbv->GetCPUDescriptorHandleForHeapStart();
			passHandle.ptr += frameIndex * m_CbvSrvUavDescriptorSize;

			m_D3D12Device->CreateConstantBufferView(&passCbvDesc, passHandle);
		}
		LightManager::GetInstance().CreateLightCbv(
			m_D3D12Device.Get(),
			m_FrameResources.data(),
			m_CbvSrvUavDescriptorSize);
	}

	void LightApp::InitMaterials()
	{
		for (int i = 0; i < FrameCount; ++i) {
			auto& matCB = m_FrameResources[i]->m_ConstantBuffers.find("MaterialConstants")->second;
			std::size_t j = 0;
			for (const auto& [name, obj] : m_Objects) {
				for (const auto& item : obj.GetAllRenderItems()) {
					auto& material = item->m_Material;
					matCB->m_IsDirty = true;
					matCB->Map();
					MaterialConstants mat{};
					if (auto diffuse = material->Get<XMFLOAT3>("DiffuseColor"); diffuse != nullptr) {
						mat.m_Diffuse = *diffuse;
					}
					if (auto specular = material->Get<XMFLOAT3>("SpecularColor"); specular != nullptr) {
						mat.m_Specular = *specular;
					}
					if (auto ambient = material->Get<XMFLOAT3>("AmbientColor"); ambient != nullptr) {
						mat.m_Ambient = *ambient;
						float ambientScale = 0.08f;
						mat.m_Ambient = XMFLOAT3{
							mat.m_Ambient.x * ambientScale,
							mat.m_Ambient.y * ambientScale,
							mat.m_Ambient.z * ambientScale };
					}
					if (auto gloss = material->Get<float>("SpecularFactor"); gloss != nullptr) {
						mat.m_Gloss = *gloss;
					}
					if (auto alpha = material->Get<float>("Opacity"); alpha != nullptr) {
						mat.m_Alpha = *alpha;
					}
					mat.m_Gloss = 0.5;
					matCB->CopyData(i * m_RenderObjCount + j, &mat, sizeof(MaterialConstants));
					matCB->Unmap();
					++j;
				}
			}
		}
	}

	void LightApp::CreateRootSignature()
	{
		auto constBufferSize = m_FrameResources[0]->m_ConstantBuffers.size();
		std::vector<CD3DX12_DESCRIPTOR_RANGE> cbvTables(constBufferSize);
		std::vector<CD3DX12_ROOT_PARAMETER> slotRootParameter(constBufferSize);
		for (UINT i = 0; i < constBufferSize; ++i) {
			cbvTables[i].Init(
				D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
				1, i);
			slotRootParameter[i].InitAsDescriptorTable(
				1, &cbvTables[i]);
		}

		D3D12_ROOT_SIGNATURE_DESC rootSignatrueDesc{};
		rootSignatrueDesc.NumParameters = (UINT)constBufferSize;
		rootSignatrueDesc.pParameters = slotRootParameter.data();
		rootSignatrueDesc.NumStaticSamplers = 0;
		rootSignatrueDesc.pStaticSamplers = nullptr;
		rootSignatrueDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> errorBlob;
		ComPtr<ID3DBlob> serilizedRootsig;
		auto hr = D3D12SerializeRootSignature(
			&rootSignatrueDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			serilizedRootsig.GetAddressOf(),
			errorBlob.GetAddressOf());

		if (errorBlob) {
			::OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(m_D3D12Device->CreateRootSignature(
			0,
			serilizedRootsig->GetBufferPointer(),
			serilizedRootsig->GetBufferSize(),
			IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
	}

	void LightApp::CreatePSO()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.VS = {
			m_VSByteCodes["Light"]->GetBufferPointer(),
			m_VSByteCodes["Light"]->GetBufferSize()
		};
		psoDesc.PS = {
			m_PSByteCodes["Light"]->GetBufferPointer(),
			m_PSByteCodes["Light"]->GetBufferSize()
		};
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = {
			VertexPosLNormal::GetInputLayout().data(),
			(UINT)VertexPosLNormal::GetInputLayout().size()
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
	}
}
