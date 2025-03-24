#include "TreeBillboardsAPP.h"
#include "ImguiManager.h"
#include "Vertex.h"
#include "ModelManager.h"
#include "ConstantData.h"
#include "LightManager.h"
#include "TextureManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	TreeBillboardsAPP::TreeBillboardsAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
		m_Camera = std::make_unique<Camera>();
		m_CameraController = std::make_unique<CameraController>();
	}

	bool TreeBillboardsAPP::OnInit()
	{
		if (!D3D12App::OnInit()) {
			return false;
		}

		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

		LightManager::Create();
		ObjectManager::Create();
		ModelManager::Create(m_D3D12Device.Get());
		TextureManager::Create(m_D3D12Device.Get(), m_CommandList.Get());
		ImguiManager::Create();
		if (!ImguiManager::GetInstance().InitImGui(
			m_D3D12Device.Get(),
			m_hMainWnd,
			FrameCount,
			m_BackBufferFormat)) {
			return false;
		}

		if (!InitResource()) {
			return false;
		}

		ThrowIfFailed(m_CommandList->Close());
		ID3D12CommandList* pCmdLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
		FlushCommandQueue();

		return true;
	}

	void TreeBillboardsAPP::OnUpdate(const CpuTimer& timer)
	{
		m_CurrFrameIndex = (m_CurrFrameIndex + 1) % FrameCount;
		m_CurrFrameResource = m_FrameResources[m_CurrFrameIndex].get();

		auto fence = m_CurrFrameResource->m_Fence;
		if (fence != 0 && m_D3D12Fence->GetCompletedValue() < fence) {
			WaitForGPU();
		}

		// Update
		ImguiManager::GetInstance().Update(timer);
		UpdatePassCB(timer);
		UpdateShadowCB(timer);
		UpdateLightCB(timer);
		m_CameraController->Update(timer.DeltaTime());
	}

	void TreeBillboardsAPP::OnRender(const CpuTimer& timer)
	{
		auto& lightManager = LightManager::GetInstance();
		auto& imgui = ImguiManager::GetInstance();


		auto& cmdListAlloc = m_CurrFrameResource->m_CmdListAlloc;
		ThrowIfFailed(cmdListAlloc->Reset());
		ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), nullptr));

		auto viewPort = m_Camera->GetViewPort();
		m_CommandList->RSSetViewports(1, &viewPort);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

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
		
		m_CommandList->ClearRenderTargetView(currBackBV, Colors::Pink, 0, nullptr);
		m_CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
		m_CommandList->OMSetRenderTargets(1, &currBackBV, true, &dsv);


		RenderScene(RenderLayer::Opaque);
		RenderTriangle();


		ImguiManager::GetInstance().RenderImGui(m_CommandList.Get());

		D3D12_RESOURCE_BARRIER rtToPresent{};
		rtToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rtToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rtToPresent.Transition = {
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		};
		m_CommandList->ResourceBarrier(1, &rtToPresent);

		ThrowIfFailed(m_CommandList->Close());
		ID3D12CommandList* pCmdLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);

		ThrowIfFailed(m_DxgiSwapChain->Present(0, 0));

		m_CurrFrameResource->m_Fence = ++m_CurrentFence;
		m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;
		ThrowIfFailed(m_CommandQueue->Signal(m_D3D12Fence.Get(), m_CurrentFence));

		for (auto& frameResource : m_FrameResources) {
			frameResource->ClearUp(m_CurrentFence);
		}
	}

	void TreeBillboardsAPP::OnResize()
	{
		D3D12App::OnResize();

		m_Camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_Camera->SetFrustum(XM_PI / 3, GetAspectRatio(), 0.3, 300);

	}

	void TreeBillboardsAPP::WaitForGPU()
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
		ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	void TreeBillboardsAPP::RenderScene(RenderLayer layer)
	{
	}

	void TreeBillboardsAPP::RenderTriangle()
	{
		const auto triangle = ObjectManager::GetInstance().GetObjectByName("Triangle");
		const auto mesh = ModelManager::GetInstance().GetMeshData<VertexPosLColor>(triangle->GetName());
		const auto vertBV = mesh->GetVertexBufferView();
		auto& CBuffers = m_CurrFrameResource->m_Resources;
		m_CommandList->IASetVertexBuffers(0, 1, &vertBV);
		m_TriangleShader->SetObjectCB(CBuffers[triangle->GetName()]);
		m_TriangleShader->SetPassCB(CBuffers[typeid(PassConstants).name()]);
		m_TriangleShader->SetObjectConstants(GetObjectConstants(*triangle));
		m_TriangleShader->Apply(m_CommandList.Get(), m_CurrFrameResource);
		m_CommandList->DrawInstanced(3, 1, 0, 0);
	}

	bool TreeBillboardsAPP::InitResource()
	{
		auto& light = LightManager::GetInstance();

		m_Camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_Camera->SetFrustum(XM_PI / 3, GetAspectRatio(), 0.5f, 300.0f);
		m_CameraController->InitCamera(m_Camera.get());
		
		m_ShaderDescriptorHeap = std::make_unique<D3D12DescriptorCache>(m_D3D12Device.Get());

		m_SceneSphere.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_SceneSphere.Radius = std::sqrt(50 * 50 + 40 * 40);

		m_TriangleShader = std::make_unique<TriangleShader>(m_D3D12Device.Get());

		CreateObject();
		CreateTexture();
		CreateFrameResource();
		CreateDescriptor();

		return true;
	}

	void TreeBillboardsAPP::CreateObject()
	{
		auto& modelManager = ModelManager::GetInstance();
		auto& objManager = ObjectManager::GetInstance();

		/*const Model* sponzaModel = modelManager.LoadModelFromeFile(
			"Sponza",
			"Models\\Sponza\\Sponza.gltf",
			m_CommandList.Get());
		auto sponza = std::make_shared<Object>(sponzaModel->GetName(), sponzaModel);
		sponza->GetTransform().SetScale({ 0.1,0.1,0.1 });
		sponza->GetTransform().SetRotation(0, MathHelper::PI / 2, 0);*/
		//objManager.AddObject(sponza, RenderLayer::Opaque);

		const Model* elenaModel = modelManager.LoadModelFromeFile(
			"Elena",
			"Models\\Elena.obj",
			m_CommandList.Get());
		auto elena = std::make_shared<Object>(elenaModel->GetName(), elenaModel);
		//objManager.AddObject(elena, RenderLayer::Opaque);

		const Model* planeModel = modelManager.LoadModelFromeGeometry(
			"Plane", GeometryGenerator::CreateGrid(80, 80, 2, 2));
		auto plane = std::make_shared<Object>(planeModel->GetName(), planeModel);
		//objManager.AddObject(plane, RenderLayer::Opaque);

		GeometryMesh triangleMesh{};
		triangleMesh.m_Vertices = {
			{ XMFLOAT3(-1.0f, -0.866f, 1.0f)},
			{ XMFLOAT3(0.0f, 0.866f, 1.0f)},
			{ XMFLOAT3(1.0f, -0.866f, 1.0f)}};
		const Model* triangleModel = modelManager.LoadModelFromeGeometry("Triangle", triangleMesh);
		auto triangle = std::make_shared<Object>(triangleModel->GetName(), triangleModel);
		objManager.AddObject(triangle, RenderLayer::Opaque);


		auto vertFunc0 = [](const Vertex& vert) {
			VertexPosLNormalTex ret{};
			ret.m_Normal = vert.m_Normal;
			ret.m_Pos = vert.m_Position;
			ret.m_TexCoord = vert.m_TexCoord;
			return ret;};
		auto vertFunc1 = [](const Vertex& vert) {
			VertexPosLColor ret{};
			auto vector = MathHelper::RandomVector();
			ret.m_Pos = vert.m_Position;
			ret.m_Color = XMFLOAT4{vector.x, vector.y, vector.z, 1.0f};
			return ret;};

		modelManager.CreateMeshDataForAllModel<VertexPosLNormalTex>(m_CommandList.Get(), vertFunc0);
		modelManager.CreateMeshDataForAllModel<VertexPosLColor>(m_CommandList.Get(), vertFunc1);
	}

	void TreeBillboardsAPP::CreateTexture()
	{
		auto& texManager = TextureManager::GetInstance();
		auto& modelManager = ModelManager::GetInstance();

		auto setTexture = [&texManager, &modelManager](
			const std::string& modelname,
			const std::string& filename,
			ID3D12GraphicsCommandList* cmdList) {
				if (auto tex = texManager.LoadTextureFromFile(filename, cmdList); tex != nullptr) {
					if (auto model = modelManager.GetModel(modelname); model != nullptr) {
						auto& landMat = model->GetMaterial(model->GetMesh(modelname)->m_MaterialIndex);
						landMat.Set("Diffuse", tex->GetName());
					}
				}
			};

		setTexture("Water", "Textures\\water1.dds", m_CommandList.Get());
		setTexture("Mirror", "Textures\\ice.dds", m_CommandList.Get());
	}

	void TreeBillboardsAPP::CreateFrameResource()
	{
		auto& objManager = ObjectManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		for (auto& resource : m_FrameResources) {
			resource = std::make_unique<FrameResource>(m_D3D12Device.Get());
			resource->AddConstantBuffer(sizeof(PassConstants), 1, typeid(PassConstants).name());
			resource->AddConstantBuffer(sizeof(PassConstants), 1, "ShadowMap");
			resource->AddConstantBuffer(lightManager.GetLightByteSize(), 1, lightManager.GetLightBufferName());
			objManager.CreateObjectsResource(resource.get(), sizeof(ObjectConstants), sizeof(MaterialConstants));
		}
	}

	void TreeBillboardsAPP::CreateDescriptor()
	{

	}

	void TreeBillboardsAPP::UpdatePassCB(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();

		XMMATRIX view = m_Camera->GetViewMatrixXM();
		auto proj = m_Camera->GetProjMatrixXM();
		auto detView = XMMatrixDeterminant(view);
		auto detProj = XMMatrixDeterminant(proj);
		XMMATRIX invView = XMMatrixInverse(&detView, view);
		XMMATRIX invProj = XMMatrixInverse(&detProj, proj);

		PassConstants passConstants;
		XMStoreFloat4x4(&passConstants.m_View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&passConstants.m_InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&passConstants.m_Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&passConstants.m_InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&passConstants.m_ShadowTrans, XMMatrixTranspose(m_ShadowTrans));
		passConstants.m_EyePosW = m_Camera->GetTransform().GetPosition();
		passConstants.m_RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
		passConstants.m_InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
		passConstants.m_NearZ = m_Camera->GetNearZ();
		passConstants.m_FarZ = m_Camera->GetFarZ();
		passConstants.m_TotalTime = timer.TotalTime();
		passConstants.m_DeltaTime = timer.DeltaTime();
		passConstants.m_FogColor = imgui.m_FogColor;
		passConstants.m_FogStart = imgui.m_FogStart;
		passConstants.m_FogRange = imgui.m_FogRange;

		m_TriangleShader->SetPassConstants(passConstants);
	}

	void TreeBillboardsAPP::UpdateShadowCB(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();

		XMVECTOR lightDir = XMLoadFloat3(&imgui.m_LightDir);
		XMVECTOR lightPos = -2.0f * m_SceneSphere.Radius * lightDir;
		XMVECTOR targetPos = XMLoadFloat3(&m_SceneSphere.Center);
		XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		auto detView = XMMatrixDeterminant(lightView);
		XMMATRIX invView = XMMatrixInverse(&detView, lightView);
		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(XMLoadFloat3(&m_SceneSphere.Center), lightView));

		// Ortho frustum in light space encloses scene.
		float l = sphereCenterLS.x - m_SceneSphere.Radius;
		float b = sphereCenterLS.y - m_SceneSphere.Radius;
		float n = sphereCenterLS.z - m_SceneSphere.Radius;
		float r = sphereCenterLS.x + m_SceneSphere.Radius;
		float t = sphereCenterLS.y + m_SceneSphere.Radius;
		float f = sphereCenterLS.z + m_SceneSphere.Radius;

		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
		auto detProj = XMMatrixDeterminant(lightProj);
		XMMATRIX invProj = XMMatrixInverse(&detProj, lightProj);

		XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		XMMATRIX S = lightView * lightProj * T;
		m_ShadowTrans = S;

		PassConstants passConstants;
		XMStoreFloat4x4(&passConstants.m_View, XMMatrixTranspose(lightView));
		XMStoreFloat4x4(&passConstants.m_InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&passConstants.m_Proj, XMMatrixTranspose(lightProj));
		XMStoreFloat4x4(&passConstants.m_InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&passConstants.m_ShadowTrans, XMMatrixTranspose(S));
		XMStoreFloat3(&passConstants.m_EyePosW, lightPos);
		passConstants.m_RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
		passConstants.m_InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
		passConstants.m_NearZ = n;
		passConstants.m_FarZ = f;
		passConstants.m_TotalTime = timer.TotalTime();
		passConstants.m_DeltaTime = timer.DeltaTime();
		passConstants.m_FogColor = imgui.m_FogColor;
		passConstants.m_FogStart = imgui.m_FogStart;
		passConstants.m_FogRange = imgui.m_FogRange;

	}

	void TreeBillboardsAPP::UpdateLightCB(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		DirectionalLight dirLight{};
		dirLight.m_Dir = imgui.m_LightDir;
		dirLight.m_Color = imgui.m_LightColor;
		lightManager.SetDirLight(0, dirLight);

	}

	MaterialConstants TreeBillboardsAPP::GetMaterialConstants(const Material& material)
	{
		MaterialConstants ret{};
		if (auto diffuse = material.Get<XMFLOAT3>("DiffuseColor"); diffuse != nullptr) {
			ret.m_Diffuse = *diffuse;
		}
		if (auto specular = material.Get<XMFLOAT3>("SpecularColor"); specular != nullptr) {
			ret.m_Specular = *specular;
		}
		if (auto ambient = material.Get<XMFLOAT3>("AmbientColor"); ambient != nullptr) {
			ret.m_Ambient = *ambient;
			float ambientScale = 0.08f;
			ret.m_Ambient = XMFLOAT3{
				ret.m_Ambient.x * ambientScale,
				ret.m_Ambient.y * ambientScale,
				ret.m_Ambient.z * ambientScale };
		}
		if (auto gloss = material.Get<float>("SpecularFactor"); gloss != nullptr) {
			ret.m_Gloss = *gloss;
		}
		if (auto alpha = material.Get<float>("Opacity"); alpha != nullptr) {
			ret.m_Alpha = *alpha;
		}
		return ret;
	}

	ObjectConstants TreeBillboardsAPP::GetObjectConstants(const Object& obj)
	{
		ObjectConstants ret{};
		auto world = obj.GetTransform().GetLocalToWorldMatrix();
		XMStoreFloat4x4(&ret.m_World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&ret.m_WorldInvTranspose, MathHelper::InverseTransposeWithOutTranslate(world));
		return ret;
	}


	
}