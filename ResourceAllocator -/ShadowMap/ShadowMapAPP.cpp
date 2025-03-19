#include "ShadowMapAPP.h"
#include "ImguiManager.h"
#include "Vertex.h"
#include "ModelManager.h"
#include "ConstantData.h"
#include "LightManager.h"
#include "TextureManager.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	ShadowMapAPP::ShadowMapAPP(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
		m_Camera = std::make_unique<Camera>();
		m_CameraController = std::make_unique<CameraController>();
	}

	bool ShadowMapAPP::OnInit()
	{
		if (!D3D12App::OnInit()) {
			return false;
		}

		// 为创建帧资源，因此使用基类的命令列表堆
		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

		// 注意初始化顺序
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

	void ShadowMapAPP::OnUpdate(const CpuTimer& timer)
	{
		auto& lightManager = LightManager::GetInstance();
		auto& imgui = ImguiManager::GetInstance();

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
		UpdateLightCB(timer);
		m_CameraController->Update(timer.DeltaTime());
	}

	void ShadowMapAPP::OnRender(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();
		auto& texManager = TextureManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		auto& cmdListAlloc = m_CurrFrameResource->m_CmdListAlloc;
		ThrowIfFailed(cmdListAlloc->Reset());
		ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), nullptr));

		auto viewPort = m_Camera->GetViewPort();
		m_CommandList->RSSetViewports(1, &viewPort);
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


		// 绘制不透明物体
		RenderScene(RenderLayer::Opaque);


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
		m_CurrFrameResource->m_Fence = ++m_CurrentFence;
		m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;
		ThrowIfFailed(m_CommandQueue->Signal(m_D3D12Fence.Get(), m_CurrentFence));

		for (auto& frameResource : m_FrameResources) {
			frameResource->ClearUp(m_CurrentFence);
		}
	}

	void ShadowMapAPP::OnResize()
	{
		D3D12App::OnResize();

		m_Camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_Camera->SetFrustum(XM_PI / 3, GetAspectRatio(), 0.3, 300);
		
	}

	void ShadowMapAPP::WaitForGPU()
	{
		// 创建并设置事件
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
		// 设置当GPU触发什么栅栏值时发送事件
		ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));
		// 等待触发围栏
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	void ShadowMapAPP::RenderScene(RenderLayer layer)
	{
		auto& objManager = ObjectManager::GetInstance();
		auto& imgui = ImguiManager::GetInstance();
		auto& modelManager = ModelManager::GetInstance();
		auto& texManager = TextureManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		auto& constBuffers = m_CurrFrameResource->m_Resources;
		m_LitShader->SetPassCB(constBuffers[typeid(PassConstants).name()]);
		m_LitShader->SetLightCB(constBuffers[lightManager.GetLightBufferName()]);

		for (const auto& [name, obj] : objManager.GetAllObject()[(int)layer]) {
			if (auto model = obj->GetModel(); model != nullptr) {
				const auto& meshData = modelManager.GetMeshData<VertexPosLNormalTex>(model->GetName());
				auto vertexBV = meshData->GetVertexBufferView();
				auto indexBV = meshData->GetIndexBufferView();
				m_CommandList->IASetVertexBuffers(0, 1, &vertexBV);
				m_CommandList->IASetIndexBuffer(&indexBV);

				m_LitShader->SetObjectCB(constBuffers[name]);

				auto getObjCB = [&imgui](const Object& obj, RenderLayer layer) {
					ObjectConstants ret{};

					auto& trans = obj.GetTransform();
					auto scale = imgui.m_Transform.GetScaleMatrix() * trans.GetScaleMatrix();
					auto rotate = imgui.m_Transform.GetRotateMatrix() * trans.GetRotateMatrix();
					auto pos = imgui.m_Transform.GetTranslationMatrix() * trans.GetTranslationMatrix();
					auto world = scale * rotate * pos;

					XMStoreFloat4x4(&ret.m_World, XMMatrixTranspose(world));
					XMStoreFloat4x4(&ret.m_WorldInvTranspose, MathHelper::InverseTransposeWithOutTranslate(world));

					return ret;
					};
				m_LitShader->SetObjectConstants(getObjCB(*obj, layer));


				for (const auto& [itemName, drawItem] : meshData->m_DrawArgs) {
					auto matIndex = model->GetMesh(itemName)->m_MaterialIndex;
					const auto& mat = model->GetMaterial(matIndex);
					auto getMatCB = [](const Material& material) {
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
						};
					auto matResource = constBuffers[name + "Mat" + std::to_string(matIndex)];
					m_LitShader->SetMaterialCB(matResource);
					m_LitShader->SetMaterialConstants(getMatCB(mat));
					
					auto diffuseTex = mat.Get<std::string>("Diffuse");
					std::string texName = diffuseTex == nullptr ? "" : *diffuseTex;
					m_LitShader->SetTexture(texManager.GetTextureResourceView(texName));

					m_LitShader->Apply(m_CommandList.Get(), m_CurrFrameResource);

					m_CommandList->DrawIndexedInstanced(drawItem.m_IndexCount, 1, drawItem.m_StarIndexLocation, drawItem.m_BaseVertexLocation, 0);
				}
			}
		}
	}

	bool ShadowMapAPP::InitResource()
	{
		auto& light = LightManager::GetInstance();
		auto& texManager = TextureManager::GetInstance();
		
		m_Camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_Camera->SetFrustum(XM_PI / 3, GetAspectRatio(), 0.5f, 300.0f);
		m_CameraController->InitCamera(m_Camera.get());
		
		m_LitShader = std::make_unique<LitShader>(m_D3D12Device.Get(),
			light.GetDirLightCount(), light.GetPointLightCount(), light.GetSpotLightCount());
		
		CreateObject();
		CreateTexture();
		CreateFrameResource();

		for (auto& resource : m_FrameResources) {
			texManager.CreateTexDescriptor(resource.get());
		};

		return true;
	}
	
	/// <summary>
	/// 创建几何体
	/// </summary>
	void ShadowMapAPP::CreateObject()
	{
		auto& modelManager = ModelManager::GetInstance();
		auto& objManager = ObjectManager::GetInstance();

		const Model* sponzaModel = modelManager.LoadModelFromeFile(
			"Sponza",
			"Models\\Sponza\\Sponza.gltf",
			m_CommandList.Get());
		auto sponza = std::make_shared<Object>(sponzaModel->GetName(), sponzaModel);
		sponza->GetTransform().SetScale({ 0.2,0.2,0.2 });
		sponza->GetTransform().SetRotation(0, MathHelper::PI / 2, 0);
		objManager.AddObject(sponza, RenderLayer::Opaque);

		// 创建模型及物体
		const Model* elenaModel = modelManager.LoadModelFromeFile(
			"Elena",
			"Models\\Elena.obj",
			m_CommandList.Get());
		auto elena = std::make_shared<Object>(elenaModel->GetName(), elenaModel);
		elena->GetTransform().SetScale({ 2,2,2 });
		objManager.AddObject(elena, RenderLayer::Opaque);


		// 提前为所有模型生成网格数据
		auto vertFunc = [](const Vertex& vert) {
			VertexPosLNormalTex ret{};
			ret.m_Normal = vert.m_Normal;
			ret.m_Pos = vert.m_Position;
			ret.m_TexCoord = vert.m_TexCoord;
			return ret;
			};

		modelManager.CreateMeshDataForAllModel<VertexPosLNormalTex>(
			m_CommandList.Get(),
			vertFunc);
	}

	void ShadowMapAPP::CreateTexture()
	{
		auto& texManager = TextureManager::GetInstance();
		auto& modelManager = ModelManager::GetInstance();

		// 读取并创建纹理
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
	
	void ShadowMapAPP::CreateFrameResource()
	{
		auto& objManager = ObjectManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();

		for (auto& resource : m_FrameResources) {
			resource = std::make_unique<FrameResource>(m_D3D12Device.Get());
			resource->AddConstantBuffer(sizeof(PassConstants), 1, typeid(PassConstants).name());
			resource->AddConstantBuffer(lightManager.GetLightByteSize(), 1, lightManager.GetLightBufferName());
			objManager.CreateObjectsResource(resource.get(), sizeof(ObjectConstants), sizeof(MaterialConstants));
		}
	}

	void ShadowMapAPP::UpdatePassCB(const CpuTimer& timer)
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

		m_LitShader->SetPassConstants(passConstants);
	}

	void ShadowMapAPP::UpdateLightCB(const CpuTimer& timer)
	{
		auto& imgui = ImguiManager::GetInstance();
		auto& lightManager = LightManager::GetInstance();
		
		DirectionalLight dirLight{};
		dirLight.m_Dir = imgui.m_LightDir;
		dirLight.m_Color = imgui.m_LightColor;
		lightManager.SetDirLight(0, dirLight);

		m_LitShader->SetDirectionalLights(lightManager.GetDirLightCount() * sizeof(DirectionalLight), lightManager.GetDirLight());
	}



}