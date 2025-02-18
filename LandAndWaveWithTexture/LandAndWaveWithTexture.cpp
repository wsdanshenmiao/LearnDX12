#include "LandAndWaveWithTexture.h"
#include "ImguiManager.h"
#include "Vertex.h"
#include "ModelManager.h"
#include "ConstantData.h"
#include "LightManager.h"
#include "ObjectManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	LandAndWaveWithTexture::LandAndWaveWithTexture(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
	}

	bool LandAndWaveWithTexture::OnInit()
	{
		ModelManager::Create();

		if (!D3D12App::OnInit()) {
			return false;
		}

		// 注意初始化顺序
		LightManager::Create(m_D3D12Device.Get(), FrameCount);
		ObjectManager::Create(FrameCount);
		ModelManager::Create();

		if (!InitResource()) {
			return false;
		}

		ImguiManager::Create();
		if (!ImguiManager::GetInstance().InitImGui(
			m_D3D12Device.Get(),
			m_hMainWnd,
			FrameCount,
			m_BackBufferFormat)) {
			return false;
		}

		return true;
	}

	void LandAndWaveWithTexture::OnUpdate(const CpuTimer& timer)
	{
		m_CurrFrameIndex = (m_CurrFrameIndex + 1) % FrameCount;
		m_CurrFrameResource = m_FrameResources[m_CurrFrameIndex].get();

		// 若CPU过快，可能会超前一个循环，此时需要CPU等待,
		// GetCompletedValue()获取当前的栅栏值
		auto fence = m_CurrFrameResource->m_Fence;
		if (fence != 0 && m_D3D12Fence->GetCompletedValue() < fence) {
			WaitForGPU();
		}

		// Update
		ImguiManager::GetInstance().Update(timer);
		UpdateFrameResource(timer);
		LightManager::GetInstance().UpdateLight();
		UpdateObjCB(timer);
	}

	void LandAndWaveWithTexture::OnRender(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();

		auto& cmdListAlloc = m_CurrFrameResource->m_CmdListAlloc;
		ThrowIfFailed(cmdListAlloc->Reset());
		auto& pso = imgui.m_EnableWireFrame ? m_PSOs["WireFrame"] : m_PSOs["Opaque"];
		ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), pso.Get()));

		m_CommandList->RSSetViewports(1, &m_ScreenViewport);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 将资源转换为渲染对象
		D3D12_RESOURCE_BARRIER presentToRt{};
		presentToRt.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentToRt.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentToRt.Transition = {
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		};
		m_CommandList->ResourceBarrier(1, &presentToRt);

		auto currBackBV = GetCurrentBackBufferView();
		auto dsv = GetDepthStencilView();
		// 若视锥为nullptr则全部清空
		m_CommandList->ClearRenderTargetView(currBackBV, Colors::Pink, 0, nullptr);
		m_CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);

		m_CommandList->OMSetRenderTargets(1, &currBackBV, true, &dsv);

		RenderScene();
		ImguiManager::GetInstance().RenderImGui(m_CommandList.Get());

		// 将资源转换为呈现状态
		D3D12_RESOURCE_BARRIER RtToPresent{};
		RtToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		RtToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		RtToPresent.Transition = {
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		};
		m_CommandList->ResourceBarrier(1, &RtToPresent);

		// 提交命令列表
		ThrowIfFailed(m_CommandList->Close());
		ID3D12CommandList* pCmdLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);

		ThrowIfFailed(m_DxgiSwapChain->Present(0, 0));

		// 更新围栏
		m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;
		ThrowIfFailed(m_CommandQueue->Signal(m_D3D12Fence.Get(), m_CurrentFence));
	}

	void LandAndWaveWithTexture::WaitForGPU()
	{
		// 创建并设置事件
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
		// 设置当GPU触发什么栅栏值时发送事件
		ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));
		// 等待触发围栏
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	void LandAndWaveWithTexture::RenderScene()
	{
		ID3D12DescriptorHeap* texHeap[] = { m_TexDescriptorHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(texHeap), texHeap);

		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

		auto& constBuffers = m_CurrFrameResource->m_ConstantBuffers;
		auto& [passName, passConstant] = *constBuffers.find("PassConstants");
		m_CommandList->SetGraphicsRootConstantBufferView(1, passConstant->GetResource()->GetGPUVirtualAddress());

		auto lightAddress = LightManager::GetInstance().GetGPUVirtualAddress();
		m_CommandList->SetGraphicsRootConstantBufferView(2, lightAddress);

		auto& [materialName, materialConstant] = *constBuffers.find("MaterialConstants");
		m_CommandList->SetGraphicsRootConstantBufferView(3, materialConstant->GetResource()->GetGPUVirtualAddress());

		m_CommandList->SetGraphicsRootDescriptorTable(4, m_TexDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		ObjectManager::GetInstance().RenderObjects<VertexPosLNormalTex>(m_CommandList.Get(), 0);
	}

	bool LandAndWaveWithTexture::InitResource()
	{
		// 为创建帧资源，因此使用基类的命令列表堆
		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

		CreateShader();
		CreateObject();
		CreateTexture();
		CreateLights();
		CreateFrameResource();
		CreateMaterial();
		CreateDescriptorHeaps();
		CreateRootSignature();
		CreatePSOs();

		ThrowIfFailed(m_CommandList->Close());
		ID3D12CommandList* pCmdLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
		FlushCommandQueue();

		return true;
	}

	void LandAndWaveWithTexture::CreateShader()
	{
		auto shaderMacor = LightManager::GetInstance().GetLightsShaderMacros(
			"MAXDIRLIGHTCOUNT", "MAXPOINTLIGHTCOUNT", "MAXSPOTLIGHTCOUNT");

		auto colorVS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", shaderMacor.data(), "VS", "vs_5_1");
		auto colorPS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", shaderMacor.data(), "PS", "ps_5_1");
		auto lightVS = D3DUtil::CompileShader(L"Shaders\\Light.hlsl", shaderMacor.data(), "VS", "vs_5_1");
		auto lightPS = D3DUtil::CompileShader(L"Shaders\\Light.hlsl", shaderMacor.data(), "PS", "ps_5_1");

		m_ShaderByteCode.insert(std::make_pair("ColorVS", colorVS));
		m_ShaderByteCode.insert(std::make_pair("ColorPS", colorPS));
		m_ShaderByteCode.insert(std::make_pair("LightVS", lightVS));
		m_ShaderByteCode.insert(std::make_pair("LightPS", lightPS));
	}

	/// <summary>
	/// 创建几何体
	/// </summary>
	void LandAndWaveWithTexture::CreateObject()
	{
		auto& modelManager = ModelManager::GetInstance();
		auto& objManager = ObjectManager::GetInstance();

		// 创建模型及物体
		const Model* elenaModel = modelManager.LoadModelFromeFile("Elena", "Models\\Elena.obj");
		auto elena = std::make_shared<Object>(elenaModel->GetName(), elenaModel);
		elena->GetTransform().SetScale({ 2,2,2 });
		elena->GetTransform().SetPosition({ 0,0,0 });
		objManager.AddObject(elena);

		auto grid = GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50);
		for (size_t i = 0; i < grid.m_Vertices.size(); ++i) {
			auto& vert = grid.m_Vertices[i];
			auto x = vert.m_Position.x;
			auto z = vert.m_Position.z;
			XMFLOAT3 n(-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f,
				-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));
			XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
			XMStoreFloat3(&n, unitNormal);

			vert.m_Position.y = 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
			vert.m_Normal = n;
		}
		auto landModel = modelManager.LoadModelFromeGeometry("Land", grid);
		auto land = std::make_shared<Object>(landModel->GetName(), landModel);
		objManager.AddObject(land);

		objManager.CreateObjectsResource<ObjectConstants>(m_D3D12Device.Get());

		// 提前为所有模型生成网格数据
		auto vertFunc = [](const Vertex& vert) {
			VertexPosLNormalTex ret{};
			ret.m_Normal = vert.m_Normal;
			ret.m_Pos = vert.m_Position;
			ret.m_TexCoord = vert.m_TexCoord;
			return ret;
			};

		modelManager.CreateMeshDataForAllModel<VertexPosLNormalTex>(
			m_D3D12Device.Get(),
			m_CommandList.Get(),
			vertFunc);
	}

	void LandAndWaveWithTexture::CreateTexture()
	{
		// 读取并创建纹理
		auto woodTex = std::make_unique<Texture>("Wood");
		Texture::LoadTextureFromFile(*woodTex,
			L"Textures/WoodCrate01.dds",
			m_D3D12Device.Get(),
			m_CommandList.Get());

		m_Textures[woodTex->GetName()] = std::move(woodTex);
	}

	void LandAndWaveWithTexture::CreateLights()
	{
		auto& lightManager = LightManager::GetInstance();

		DirectionalLight dirLight0{};
		dirLight0.m_Color = { 1,1,1 };
		dirLight0.m_Dir = { 0.57735f, -0.57735f, 0.57735f };
		lightManager.SetDirLight(0, std::move(dirLight0));
	}

	void LandAndWaveWithTexture::CreateFrameResource()
	{
		for (auto& resource : m_FrameResources) {
			resource = std::make_unique<FrameResource>(m_D3D12Device.Get());
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(PassConstants),
				1,
				"PassConstants");
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(MaterialConstants),
				ObjectManager::GetInstance().GetObjectCount(),
				"MaterialConstants");
		}
	}

	void LandAndWaveWithTexture::CreateMaterial()
	{
		m_Materials["Box"].Set("DiffuseColor", XMFLOAT3{ 1,1,1 });
		m_Materials["Box"].Set("AmbientColor", XMFLOAT3{ 0.1 ,0.1 ,0.1 });
		m_Materials["Box"].Set("SpecularColor", XMFLOAT3{ 1 ,1 ,1 });
		m_Materials["Box"].Set("SpecularFactor", 0.2f);

		MaterialConstants boxMat{};
		boxMat.m_Ambient = *m_Materials["Box"].Get<XMFLOAT3>("AmbientColor");
		boxMat.m_Diffuse = *m_Materials["Box"].Get<XMFLOAT3>("DiffuseColor");
		boxMat.m_Specular = *m_Materials["Box"].Get<XMFLOAT3>("SpecularColor");
		boxMat.m_Gloss = *m_Materials["Box"].Get<float>("SpecularFactor");

		for (int i = 0; i < FrameCount; ++i) {
			auto& matCB = m_FrameResources[i]->m_ConstantBuffers["MaterialConstants"];
			matCB->Map();
			matCB->CopyData(0, &boxMat, sizeof(MaterialConstants));
			matCB->Unmap();
		}
	}

	void LandAndWaveWithTexture::CreateDescriptorHeaps()
	{
		// 创建纹理的描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC texHeapDesc{};
		texHeapDesc.NumDescriptors = 1;
		texHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		texHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&texHeapDesc, IID_PPV_ARGS(m_TexDescriptorHeap.GetAddressOf())));

		auto tex = m_Textures["Wood"]->GetTexture();

		// 创建纹理的描述符
		D3D12_SHADER_RESOURCE_VIEW_DESC texViewDesc{};
		texViewDesc.Format = tex->GetDesc().Format;
		texViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		texViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		texViewDesc.Texture2D.MostDetailedMip = 0;
		texViewDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
		texViewDesc.Texture2D.ResourceMinLODClamp = 0;

		m_D3D12Device->CreateShaderResourceView(
			tex, &texViewDesc,
			m_TexDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}


	void LandAndWaveWithTexture::CreateRootSignature()
	{
		// 初始化根参数，使用根描述符和根描述符表
		auto count = 4;
		std::vector<D3D12_ROOT_PARAMETER> rootParamer(count + 1);
		for (int i = 0; i < count; ++i) {
			rootParamer[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParamer[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			// 对应的寄存器槽
			rootParamer[i].Constants.ShaderRegister = i;
			rootParamer[i].Constants.RegisterSpace = 0;
		}

		D3D12_DESCRIPTOR_RANGE texTable{};
		texTable.NumDescriptors = 1;
		texTable.BaseShaderRegister = 0;
		texTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		texTable.RegisterSpace = 0;
		texTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rootParamer[count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamer[count].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParamer[count].DescriptorTable.NumDescriptorRanges = 1;
		rootParamer[count].DescriptorTable.pDescriptorRanges = &texTable;

		auto staticSamplers = GetStaticSamplers();
		D3D12_ROOT_SIGNATURE_DESC signatureDesc{};
		signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		signatureDesc.NumParameters = rootParamer.size();
		signatureDesc.pParameters = rootParamer.data();
		signatureDesc.NumStaticSamplers = (UINT)staticSamplers.size();
		signatureDesc.pStaticSamplers = staticSamplers.data();

		ComPtr<ID3DBlob> errorBlob;
		ComPtr<ID3DBlob> serializedBlob;

		// 序列化根签名
		auto hr = D3D12SerializeRootSignature(&signatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&errorBlob,
			&serializedBlob);
		ThrowIfFailed(hr);

		if (errorBlob != nullptr)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}

		// 创建根签名
		ThrowIfFailed(m_D3D12Device->CreateRootSignature(0,
			errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
	}

	void LandAndWaveWithTexture::CreatePSOs()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.VS = {
			m_ShaderByteCode["LightVS"]->GetBufferPointer(),
			m_ShaderByteCode["LightVS"]->GetBufferSize()
		};
		psoDesc.PS = {
			m_ShaderByteCode["LightPS"]->GetBufferPointer(),
			m_ShaderByteCode["LightPS"]->GetBufferSize()
		};
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = {
			VertexPosLNormalTex::GetInputLayout().data(),
			(UINT)VertexPosLNormalTex::GetInputLayout().size()
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
	}

	void LandAndWaveWithTexture::UpdateFrameResource(const CpuTimer& timer)
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
		currPassCB->Map();
		currPassCB->CopyData(0, &passConstants, sizeof(PassConstants));
		currPassCB->Unmap();
	}

	void LandAndWaveWithTexture::UpdateObjCB(const CpuTimer& timer)
	{
		auto& objManager = ObjectManager::GetInstance();
		auto& imgui = ImguiManager::GetInstance();

		auto getObjCB = [&imgui](const Object& obj) {
			ObjectConstants ret{};

			auto& trans = obj.GetTransform();
			auto scale = imgui.m_Transform.GetScaleMatrix() * trans.GetScaleMatrix();
			auto rotate = imgui.m_Transform.GetRotateMatrix() * trans.GetRotateMatrix();
			auto pos = XMVectorAdd(imgui.m_Transform.GetTranslation(), trans.GetTranslation());
			auto world = scale * rotate * XMMatrixTranslationFromVector(pos);

			auto detWorld = XMMatrixDeterminant(world);
			auto W = world;
			W.r[3] = g_XMIdentityR3;
			XMMATRIX invWorld = XMMatrixInverse(&detWorld, W);
			XMStoreFloat4x4(&ret.m_World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&ret.m_WorldInvTranspos, XMMatrixTranspose(invWorld));

			return ret;
			};

		objManager.UpdateObjectsCB(timer, getObjCB);
	}

	const std::array<const D3D12_STATIC_SAMPLER_DESC, 6> LandAndWaveWithTexture::GetStaticSamplers() const noexcept
	{
		// 创建六种静态采样器
		D3D12_STATIC_SAMPLER_DESC staticSampler{};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias = 0;
		staticSampler.MaxAnisotropy = 16;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.MinLOD = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		staticSampler.ShaderRegister = 0;

		const auto pointWarp = staticSampler;

		staticSampler.ShaderRegister = 1;
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		const auto linearWarp = staticSampler;

		staticSampler.ShaderRegister = 2;
		staticSampler.Filter = D3D12_FILTER_ANISOTROPIC;
		const auto anisotropicWarp = staticSampler;

		staticSampler.ShaderRegister = 5;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		const auto anisotropicClamp = staticSampler;

		staticSampler.ShaderRegister = 4;
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		const auto linearClamp = staticSampler;

		staticSampler.ShaderRegister = 3;
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		const auto pointClamp = staticSampler;

		return { pointWarp, linearWarp, anisotropicWarp, pointClamp, linearClamp, anisotropicClamp };
	}





}