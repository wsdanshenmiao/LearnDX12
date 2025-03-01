#include "TexturingApp.h"
#include "ImguiManager.h"
#include "Vertex.h"
#include "MeshManager.h"
#include "ConstantData.h"
#include "DDSTextureLoader12.h"
#include "LightManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	ResourceAllocatorAPP::ResourceAllocatorAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
	}

	bool ResourceAllocatorAPP::OnInit()
	{
		ModelManager::Create();

		if (!D3D12App::OnInit()) {
			return false;
		}

		// 注意初始化顺序
		LightManager::Create(m_D3D12Device.Get(), FrameCount);

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

	void ResourceAllocatorAPP::OnUpdate(const CpuTimer& timer)
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
		UpdatePassCB(timer);
		UpdateGeometry(timer);
		LightManager::GetInstance().UpdateLight();
	}

	void ResourceAllocatorAPP::OnRender(const CpuTimer& timer)
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

	void ResourceAllocatorAPP::WaitForGPU()
	{
		// 创建并设置事件
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
		// 设置当GPU触发什么栅栏值时发送事件
		ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));
		// 等待触发围栏
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	void ResourceAllocatorAPP::RenderScene()
	{
		ID3D12DescriptorHeap* texHeap[] = { m_TexDescriptorHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(texHeap), texHeap);

		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

		auto& constBuffers = m_CurrFrameResource->m_ConstantBuffers;
		auto& [passName, passConstant] = *constBuffers.find("PassConstants");
		m_CommandList->SetGraphicsRootConstantBufferView(1, passConstant->GetResource()->GetGPUVirtualAddress());

		auto lights = LightManager::GetInstance().GetLightsResource();
		m_CommandList->SetGraphicsRootConstantBufferView(2, lights->GetGPUVirtualAddress());

		for (const auto& [meshName, meshData] : m_MeshData) {
			auto vertexBV = meshData->GetVertexBufferView();
			auto indexBV = meshData->GetIndexBufferView();
			m_CommandList->IASetVertexBuffers(0, 1, &vertexBV);
			m_CommandList->IASetIndexBuffer(&indexBV);

			auto& [objName, objConstant] = *constBuffers.find("ObjectConstants");
			m_CommandList->SetGraphicsRootConstantBufferView(0, objConstant->GetResource()->GetGPUVirtualAddress());

			auto& [materialName, materialConstant] = *constBuffers.find("MaterialConstants");
			m_CommandList->SetGraphicsRootConstantBufferView(3, materialConstant->GetResource()->GetGPUVirtualAddress());

			m_CommandList->SetGraphicsRootDescriptorTable(4, m_TexDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

			for (const auto& [name, drawItem] : meshData->m_DrawArgs) {
				m_CommandList->DrawIndexedInstanced(drawItem.m_IndexCount, 1, drawItem.m_StarIndexLocation, drawItem.m_BaseVertexLocation, 0);
			}
		}
	}

	bool ResourceAllocatorAPP::InitResource()
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

	void ResourceAllocatorAPP::CreateShader()
	{
		auto& lightManager = LightManager::GetInstance();
		auto dirCount = std::to_string(lightManager.GetDirLightCount());
		auto pointCount = std::to_string(lightManager.GetPointLightCount());
		auto spotCount = std::to_string(lightManager.GetSpotLightCount());
		D3D_SHADER_MACRO shaderMacor[] = {
			{"MAXDIRLIGHTCOUNT", dirCount.c_str()},
			{"MAXPOINTLIGHTCOUNT", pointCount.c_str()},
			{"MAXSPOTLIGHTCOUNT", spotCount.c_str()},
			{nullptr, nullptr}	// 充当结束标志
		};

		auto colorVS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_1");
		auto colorPS = D3DUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_1");
		auto lightVS = D3DUtil::CompileShader(L"Shaders\\Light.hlsl", nullptr, "VS", "vs_5_1");
		auto lightPS = D3DUtil::CompileShader(L"Shaders\\Light.hlsl", nullptr, "PS", "ps_5_1");

		m_ShaderByteCode.insert(std::make_pair("ColorVS", colorVS));
		m_ShaderByteCode.insert(std::make_pair("ColorPS", colorPS));
		m_ShaderByteCode.insert(std::make_pair("LightVS", lightVS));
		m_ShaderByteCode.insert(std::make_pair("LightPS", lightPS));
	}

	/// <summary>
	/// 创建几何体
	/// </summary>
	void ResourceAllocatorAPP::CreateObject()
	{
		auto boxMesh = GeometryGenerator::CreateBox(10, 10, 10, 1);
		SubmeshData submeshData{};
		submeshData.m_IndexCount = boxMesh.m_Indices32.size();
		submeshData.m_BaseVertexLocation = 0;
		submeshData.m_StarIndexLocation = 0;

		auto vertFunc = [](const Vertex& vert) {
			VertexPosLNormalTex ret{};
			ret.m_Normal = vert.m_Normal;
			ret.m_Pos = vert.m_Position;
			ret.m_TexCoord = vert.m_TexCoord;
			return ret;
			};

		auto& meshManager = ModelManager::GetInstance();
		meshManager.AddMesh("Box", std::move(boxMesh));
		m_MeshData["Box"] = meshManager.GetAllMeshData<VertexPosLNormalTex>(
			m_D3D12Device.Get(), m_CommandList.Get(), "Box", vertFunc);
		meshManager.ClearMesh();
		meshManager.AddMesh("Box0", GeometryGenerator::CreateBox(100, 100, 100, 1));
		m_MeshData["Box0"] = meshManager.GetAllMeshData<VertexPosLNormalTex>(
			m_D3D12Device.Get(), m_CommandList.Get(), "Box", vertFunc);

		m_RenderObjCount++;
	}

	void ResourceAllocatorAPP::CreateTexture()
	{
		// 读取并创建纹理
		auto woodTex = std::make_unique<Texture>("Wood", L"Textures/WoodCrate01.dds");

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		ThrowIfFailed(LoadDDSTextureFromFile(
			m_D3D12Device.Get(),
			woodTex->m_Filename.c_str(),
			woodTex->m_Texture.GetAddressOf(),
			ddsData, subresources));

		// 获取上传堆的大小
		UINT64 uploadBufferSize;
		ID3D12Device* device = nullptr;
		auto texDesc = woodTex->m_Texture->GetDesc();
		woodTex->m_Texture->GetDevice(IID_PPV_ARGS(&device));
		device->GetCopyableFootprints(&texDesc, 0, subresources.size(), 0, nullptr, nullptr, nullptr, &uploadBufferSize);


		// 创建GPU上传堆
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		auto uploadResDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&uploadResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(woodTex->m_UploadHeap.GetAddressOf())));

		UpdateSubresources(m_CommandList.Get(), woodTex->m_Texture.Get(), woodTex->m_UploadHeap.Get(),
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition = {
			woodTex->m_Texture.Get(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		};
		m_CommandList->ResourceBarrier(1, &barrier);

		m_Textures[woodTex->m_Name] = std::move(woodTex);
	}

	void ResourceAllocatorAPP::CreateLights()
	{
		auto& lightManager = LightManager::GetInstance();

		DirectionalLight dirLight0{};
		dirLight0.m_Color = { 1,1,1 };
		dirLight0.m_Dir = { 0,-0.7,0.2 };
		lightManager.SetDirLight(0, std::move(dirLight0));
	}

	void ResourceAllocatorAPP::CreateFrameResource()
	{
		for (auto& resource : m_FrameResources) {
			resource = std::make_unique<FrameResource>(m_D3D12Device.Get());
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(ObjectConstants),
				m_RenderObjCount,
				"ObjectConstants");
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(PassConstants),
				1,
				"PassConstants");
			resource->AddConstantBuffer(
				m_D3D12Device.Get(),
				sizeof(MaterialConstants),
				m_RenderObjCount,
				"MaterialConstants");
		}
	}

	void ResourceAllocatorAPP::CreateMaterial()
	{
		m_BoxMaterial.Set("DiffuseColor", XMFLOAT3{ 1,1,1 });
		m_BoxMaterial.Set("AmbientColor", XMFLOAT3{ 0.1 ,0.1 ,0.1 });
		m_BoxMaterial.Set("SpecularColor", XMFLOAT3{ 1 ,1 ,1 });
		m_BoxMaterial.Set("SpecularFactor", 0.2f);

		MaterialConstants boxMat{};
		boxMat.m_Ambient = *m_BoxMaterial.Get<XMFLOAT3>("AmbientColor");
		boxMat.m_Diffuse = *m_BoxMaterial.Get<XMFLOAT3>("DiffuseColor");
		boxMat.m_Specular = *m_BoxMaterial.Get<XMFLOAT3>("SpecularColor");
		boxMat.m_Gloss = *m_BoxMaterial.Get<float>("SpecularFactor");

		for (int i = 0; i < FrameCount; ++i) {
			auto& matCB = m_FrameResources[i]->m_ConstantBuffers["MaterialConstants"];
			matCB->m_IsDirty = true;
			matCB->Map();
			matCB->CopyData(0, &boxMat, sizeof(MaterialConstants));
			matCB->Unmap();
		}
	}

	void ResourceAllocatorAPP::CreateDescriptorHeaps()
	{
		// 创建纹理的描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC texHeapDesc{};
		texHeapDesc.NumDescriptors = 1;
		texHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		texHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&texHeapDesc, IID_PPV_ARGS(m_TexDescriptorHeap.GetAddressOf())));

		auto& tex = m_Textures["Wood"]->m_Texture;

		// 创建纹理的描述符
		D3D12_SHADER_RESOURCE_VIEW_DESC texViewDesc{};
		texViewDesc.Format = tex->GetDesc().Format;
		texViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		texViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		texViewDesc.Texture2D.MostDetailedMip = 0;
		texViewDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
		texViewDesc.Texture2D.ResourceMinLODClamp = 0;

		m_D3D12Device->CreateShaderResourceView(
			tex.Get(), &texViewDesc,
			m_TexDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}


	void ResourceAllocatorAPP::CreateRootSignature()
	{
		auto constBufferSize = m_FrameResources[0]->m_ConstantBuffers.size();

		// 初始化根参数，使用根描述符和根描述符表
		auto count = constBufferSize;
		std::vector<D3D12_ROOT_PARAMETER> rootParamer(count + 2);
		for (int i = 0; i < count; ++i) {
			rootParamer[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParamer[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			// 对应的寄存器槽
			rootParamer[i].Constants.ShaderRegister = i;
			rootParamer[i].Constants.RegisterSpace = 0;
		}
		rootParamer[count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParamer[count].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParamer[count].Constants.ShaderRegister = count;
		rootParamer[count].Constants.RegisterSpace = 0;
		count++;

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

	void ResourceAllocatorAPP::CreatePSOs()
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

	void ResourceAllocatorAPP::UpdatePassCB(const CpuTimer& timer)
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

	void ResourceAllocatorAPP::UpdateGeometry(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();

		auto world = imgui.m_Transform.GetLocalToWorldMatrix();
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
		currObjCB->CopyData(0, &objectConstants, sizeof(ObjectConstants));
		currObjCB->Unmap();
	}

	const std::array<const D3D12_STATIC_SAMPLER_DESC, 6> ResourceAllocatorAPP::GetStaticSamplers() const noexcept
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