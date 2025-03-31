#include "TreeBillboardsAPP.h"

#include <vector>

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
		if (imgui.m_RenderModel == ImguiManager::RenderModel::Triangles) {
			RenderTriangle();
		}
		else if (imgui.m_RenderModel == ImguiManager::RenderModel::Cylinder) {
			RenderCylinder();
		}

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
		{
			const auto plane = ObjectManager::GetInstance().GetObjectByName("Plane");
			const auto model = plane->GetModel();
			const auto& material = model->GetMaterial(0);
			const auto mesh = ModelManager::GetInstance().GetMeshData<VertexPosNormalTex>(model->GetName());
			const auto vertBV = mesh->GetVertexBufferView();
			m_CommandList->IASetVertexBuffers(0, 1, &vertBV);
			const auto indicesBV = mesh->GetIndexBufferView();
			m_CommandList->IASetIndexBuffer(&indicesBV);
		
			auto& CBuffers = m_CurrFrameResource->m_Resources;
			m_LitShader->SetObjectCB(CBuffers[plane->GetName()]);
			m_LitShader->SetPassCB(CBuffers[typeid(PassConstants).name()]);
			m_LitShader->SetLightCB(CBuffers[LightManager::GetInstance().GetLightBufferName()]);
			m_LitShader->SetMaterialCB(CBuffers[plane->GetName() + "Mat" + '0']);
		
			m_LitShader->SetObjectConstants(GetObjectConstants(*plane));
			m_LitShader->SetMaterialConstants(GetMaterialConstants(material));

			auto diffuseTex = material.Get<std::string>("Diffuse");
			std::string texName = diffuseTex == nullptr ? "" : *diffuseTex;
			m_LitShader->SetTexture(TextureManager::GetInstance().GetTextureResourceView(texName));
			m_LitShader->SetShadowMap(TextureManager::GetInstance().GetDefaultTextureResourceView());
			m_LitShader->Apply(m_CommandList.Get(), m_CurrFrameResource);

			auto drawItem = mesh->m_DrawArgs[plane->GetName()];
			m_CommandList->DrawIndexedInstanced(drawItem.m_IndexCount, 1, drawItem.m_StarIndexLocation, drawItem.m_BaseVertexLocation, 0);
		}
		
		{
			const auto tree = ObjectManager::GetInstance().GetObjectByName("Tree");
			const auto model = tree->GetModel();
			const auto& material = model->GetMaterial(0);
			const auto mesh = ModelManager::GetInstance().GetMeshData<BillboardVertex>(model->GetName());
			const auto vertBV = mesh->GetVertexBufferView();
			m_CommandList->IASetVertexBuffers(0, 1, &vertBV);
		
			auto& CBuffers = m_CurrFrameResource->m_Resources;
			m_TreeBillboardsShader->SetObjectCB(CBuffers[tree->GetName()]);
			m_TreeBillboardsShader->SetPassCB(CBuffers[typeid(PassConstants).name()]);
			m_TreeBillboardsShader->SetLightCB(CBuffers[LightManager::GetInstance().GetLightBufferName()]);
			m_TreeBillboardsShader->SetMaterialCB(CBuffers[tree->GetName() + "Mat" + '0']);
		
			m_TreeBillboardsShader->SetObjectConstants(GetObjectConstants(*tree));
			m_TreeBillboardsShader->SetMaterialConstants(GetMaterialConstants(material));

			auto diffuseTex = material.Get<std::string>("Diffuse");
			std::string texName = diffuseTex == nullptr ? "" : *diffuseTex;
			m_TreeBillboardsShader->SetTexture(TextureManager::GetInstance().GetTextureResourceView(texName));
			m_TreeBillboardsShader->Apply(m_CommandList.Get(), m_CurrFrameResource);

			auto drawItem = mesh->m_DrawArgs[tree->GetName()];
			m_CommandList->DrawInstanced(m_TreeCount, 1, 0, 0);
		}
	}

	void TreeBillboardsAPP::RenderTriangle()
	{
		const auto triangle = ObjectManager::GetInstance().GetObjectByName("Triangle");
		const auto mesh = ModelManager::GetInstance().GetMeshData<VertexPosColor>(triangle->GetName());
		const auto vertBV = mesh->GetVertexBufferView();
		auto& CBuffers = m_CurrFrameResource->m_Resources;
		m_CommandList->IASetVertexBuffers(0, 1, &vertBV);
		m_TriangleShader->SetObjectCB(CBuffers[triangle->GetName()]);
		m_TriangleShader->SetPassCB(CBuffers[typeid(PassConstants).name()]);
		m_TriangleShader->SetObjectConstants(GetObjectConstants(*triangle));
		m_TriangleShader->Apply(m_CommandList.Get(), m_CurrFrameResource);
		m_CommandList->DrawInstanced(3, 1, 0, 0);
	}

	void TreeBillboardsAPP::RenderCylinder()
	{
		const auto cylinder = ObjectManager::GetInstance().GetObjectByName("Cylinder");
		const auto model = cylinder->GetModel();
		assert(model != nullptr);
		const auto& material = model->GetMaterial(0);
		auto& CBuffers = m_CurrFrameResource->m_Resources;
		auto diffuseTex = material.Get<std::string>("Diffuse");
		std::string texName = diffuseTex == nullptr ? "" : *diffuseTex;

		const auto mesh = ModelManager::GetInstance().GetMeshData<VertexPosNormalColor>(cylinder->GetName());
		const auto vertBV = mesh->GetVertexBufferView();
		m_CommandList->IASetVertexBuffers(0, 1, &vertBV);
		
		m_CylinderShader->SetObjectCB(CBuffers[cylinder->GetName()]);
		m_CylinderShader->SetPassCB(CBuffers[typeid(PassConstants).name()]);
		m_CylinderShader->SetLightCB(CBuffers[LightManager::GetInstance().GetLightBufferName()]);
		m_CylinderShader->SetMaterialCB(CBuffers[cylinder->GetName() + "Mat" + '0']);
		m_CylinderShader->SetCylinderCB(m_CurrFrameResource->m_Resources["CylinderHeight"]);
		
		m_CylinderShader->SetCylinderConstants(ImguiManager::GetInstance().m_CylineHeight);
		m_CylinderShader->SetObjectConstants(GetObjectConstants(*cylinder));
		m_CylinderShader->SetMaterialConstants(GetMaterialConstants(material));
		
		m_CylinderShader->SetTexture(TextureManager::GetInstance().GetTextureResourceView(texName));
		
		m_CylinderShader->Apply(m_CommandList.Get(), m_CurrFrameResource);
		
		m_CommandList->DrawInstanced(41, 1, 0, 0);
	}

	bool TreeBillboardsAPP::InitResource()
	{
		auto& light = LightManager::GetInstance();

		m_Camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_Camera->SetFrustum(XM_PI / 3, GetAspectRatio(), 0.5f, 300.0f);
		m_Camera->LookAt({4, 10, -4}, {0,0,0}, {0,1,0});
		m_CameraController->InitCamera(m_Camera.get());
		
		m_ShaderDescriptorHeap = std::make_unique<D3D12DescriptorCache>(m_D3D12Device.Get());

		m_SceneSphere.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_SceneSphere.Radius = std::sqrt(50 * 50 + 40 * 40);

		m_TriangleShader = std::make_unique<TriangleShader>(m_D3D12Device.Get());
		m_CylinderShader = std::make_unique<CylinderShader>(m_D3D12Device.Get());
		m_LitShader = std::make_unique<LitShader>(m_D3D12Device.Get());
		m_TreeBillboardsShader = std::make_unique<TreeBillboardsShader>(m_D3D12Device.Get());

		CreateObject();
		CreateTexture();
		CreateFrameResource();

		return true;
	}

	void TreeBillboardsAPP::CreateObject()
	{
		auto& modelManager = ModelManager::GetInstance();
		auto& objManager = ObjectManager::GetInstance();

		GeometryMesh triangleMesh{};
		triangleMesh.m_Vertices = {
			{ XMFLOAT3(-1.0f * 3, -0.866f * 3, 1.0f * 3)},
			{ XMFLOAT3(0.0f, 0.866f * 3, 1.0f * 3)},
			{ XMFLOAT3(1.0f * 3, -0.866f * 3, 1.0f * 3)}};
		const Model* triangleModel = modelManager.LoadModelFromeGeometry("Triangle", triangleMesh);
		auto triangle = std::make_shared<Object>(triangleModel->GetName(), triangleModel);
		triangle->GetTransform().SetPosition(0, 3, 0);
		objManager.AddObject(triangle, RenderLayer::Opaque);

		GeometryMesh roundMesh{};
		roundMesh.m_Vertices.resize(41);
		auto& roundVertices = roundMesh.m_Vertices;
		for (int i = 0; i < 40; ++i)
		{
			roundVertices[i].m_Position = XMFLOAT3(cosf(XM_PI / 20 * i), -1.0f, -sinf(XM_PI / 20 * i));
			roundVertices[i].m_Normal = XMFLOAT3(cosf(XM_PI / 20 * i), 0.0f, -sinf(XM_PI / 20 * i));
		}
		roundVertices[40] = roundVertices[0];
		const Model* roundModel = modelManager.LoadModelFromeGeometry("Cylinder", roundMesh);
		auto round = std::make_shared<Object>(roundModel->GetName(), roundModel);
		round->GetTransform().SetPosition(0, 3, 0);
		objManager.AddObject(round, RenderLayer::Opaque);

		auto planeModel = modelManager.LoadModelFromeGeometry("Plane", GeometryGenerator::CreateGrid(80, 80, 2, 2));
		auto plane = std::make_shared<Object>(planeModel->GetName(), planeModel);
		objManager.AddObject(plane, RenderLayer::Opaque);
		
		GeometryMesh treeMesh{};
		treeMesh.m_Vertices.resize(m_TreeCount);
		auto& treeVertices = treeMesh.m_Vertices;
		for (int i = 0; i < m_TreeCount; ++i)
		{
			treeVertices[i].m_Position = XMFLOAT3{
				MathHelper::RandomFloat(-40, 40), 0, MathHelper::RandomFloat(-40, 40)};
		}
		const Model* treeModel = modelManager.LoadModelFromeGeometry("Tree", treeMesh);
		auto tree = std::make_shared<Object>(treeModel->GetName(), treeModel);
		objManager.AddObject(tree, RenderLayer::Opaque);

		auto vertFunc0 = [](const Vertex& vert) {
			VertexPosNormalTex ret{};
			ret.m_Normal = vert.m_Normal;
			ret.m_Pos = vert.m_Position;
			ret.m_TexCoord = vert.m_TexCoord;
			return ret;};
		auto vertFunc1 = [](const Vertex& vert) {
			VertexPosColor ret{};
			auto vector = MathHelper::RandomVector();
			ret.m_Pos = vert.m_Position;
			ret.m_Color = XMFLOAT4{vector.x, vector.y, vector.z, 1.0f};
			return ret;};
		auto vertFunc2 = [](const Vertex& vert) {
			VertexPosNormalColor ret{};
			ret.m_Pos = vert.m_Position;
			ret.m_Color = XMFLOAT4{1,1,1,1};
			ret.m_Normal = vert.m_Normal;
			return ret;};

		auto vertFunc3 = [=](const Vertex& vert) {
			BillboardVertex ret{};
			ret.Position = vert.m_Position;
			ret.Size = m_BoardSize;
			return ret;};

		modelManager.CreateMeshDataForAllModel<VertexPosNormalTex>(m_CommandList.Get(), vertFunc0);
		modelManager.CreateMeshDataForAllModel<VertexPosColor>(m_CommandList.Get(), vertFunc1);
		modelManager.CreateMeshDataForAllModel<VertexPosNormalColor>(m_CommandList.Get(), vertFunc2);
		modelManager.CreateMeshDataForAllModel<BillboardVertex>(m_CommandList.Get(), vertFunc3);
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
		
		setTexture("Plane", "Textures\\grass.dds", m_CommandList.Get());
		setTexture("Tree", "Textures\\treeArray2.dds", m_CommandList.Get());
	}

	void TreeBillboardsAPP::CreateFrameResource()
	{
		auto& objManager = ObjectManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		for (auto& resource : m_FrameResources) {
			resource = std::make_unique<FrameResource>(m_D3D12Device.Get());
			
			resource->AddConstantBuffer(sizeof(PassConstants), 1, typeid(PassConstants).name());
			resource->AddConstantBuffer(lightManager.GetLightByteSize(), 1, lightManager.GetLightBufferName());
			objManager.CreateObjectsResource(resource.get(), sizeof(ObjectConstants), sizeof(MaterialConstants));
			resource->AddConstantBuffer(sizeof(float), 1, "CylinderHeight");
		}
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

		m_TriangleShader->SetPassConstants(passConstants);
		m_CylinderShader->SetPassConstants(passConstants);
		m_LitShader->SetPassConstants(passConstants);
		m_TreeBillboardsShader->SetPassConstants(passConstants);
	}
	
	void TreeBillboardsAPP::UpdateLightCB(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		DirectionalLight dirLight{};
		dirLight.m_Dir = imgui.m_LightDir;
		dirLight.m_Color = imgui.m_LightColor;
		lightManager.SetDirLight(0, dirLight);

		auto lightByteSize = lightManager.GetDirLightCount() * sizeof(DirectionalLight);
		m_CylinderShader->SetDirectionalLights(lightByteSize, lightManager.GetDirLight());
		m_LitShader->SetDirectionalLights(lightByteSize, lightManager.GetDirLight());
		m_TreeBillboardsShader->SetDirectionalLights(lightByteSize, lightManager.GetDirLight());
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
		else {
			ret.m_Ambient = XMFLOAT3{0.1f, 0.1f, 0.1f};
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