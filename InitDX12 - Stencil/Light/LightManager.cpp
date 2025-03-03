#include "LightManager.h"
#include "LightApp.h"

namespace DSM {
	std::size_t LightManager::GetDirLightCount() const
	{
		return m_DirLights.size();
	}

	std::size_t LightManager::GetPointLightCount() const
	{
		return m_PointLights.size();
	}

	std::size_t LightManager::GetSpotLightCount() const
	{
		return m_SpotLights.size();
	}

	std::size_t LightManager::GetLightByteSize() const
	{
		auto dirLightSize = m_DirLights.size() * sizeof(DirectionalLight);
		auto pointLightSize = m_PointLights.size() * sizeof(PointLight);
		auto spotLightSize = m_PointLights.size() * sizeof(SpotLight);
		return dirLightSize + pointLightSize + spotLightSize;
	}

	ID3D12DescriptorHeap* LightManager::GetLightCbv() const
	{
		return m_LightCbv.Get();
	}

	void LightManager::SetDirLight(std::size_t index, const DirectionalLight& light)
	{
		if (index < m_DirLights.size()) {
			m_NumFrameDirty = LightApp::FrameCount;
			m_DirLights[index] = light;
		}
	}

	void LightManager::SetPointLight(std::size_t index, const PointLight& light)
	{
		if (index < m_PointLights.size()) {
			m_NumFrameDirty = LightApp::FrameCount;
			m_PointLights[index] = light;
		}
	}

	void LightManager::SetSpotLight(std::size_t index, const SpotLight& light)
	{
		if (index < m_SpotLights.size()) {
			m_NumFrameDirty = LightApp::FrameCount;
			m_SpotLights[index] = light;
		}
	}

	void LightManager::UpdateLight(FrameResource* currFrameResource)
	{
		if (m_NumFrameDirty <= 0)return;

		BYTE* mappedData;
		auto& buffer = currFrameResource->m_ConstantBuffers.find("Lights")->second;
		auto dirLightSize = sizeof(DirectionalLight) * m_DirLights.size();
		auto pointLightSize = sizeof(PointLight) * m_PointLights.size();
		auto spotLightSize = sizeof(SpotLight) * m_SpotLights.size();

		ID3D12Resource* lightResource = buffer->GetResource();
		lightResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		memcpy(&mappedData[0], m_DirLights.data(), dirLightSize);
		memcpy(&mappedData[dirLightSize], m_PointLights.data(), pointLightSize);
		memcpy(&mappedData[dirLightSize + pointLightSize], m_SpotLights.data(), spotLightSize);
		lightResource->Unmap(0, nullptr);

		--m_NumFrameDirty;
	}

	std::pair<std::string, std::unique_ptr<UploadBuffer<void>>>
		LightManager::CreateLightBuffer(ID3D12Device* device)
	{
		auto pBuffer = std::make_unique<UploadBuffer<void>>(device, GetLightByteSize(), 1, true);
		return { "Lights", std::move(pBuffer) };
	}

	void LightManager::CreateLightCbv(
		ID3D12Device* device,
		std::unique_ptr<FrameResource>* frameResourceArray,
		std::size_t cbvDescriptorSize)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NumDescriptors = (UINT)LightApp::FrameCount;
		heapDesc.NodeMask = 0;
		ThrowIfFailed(device->CreateDescriptorHeap(
			&heapDesc, IID_PPV_ARGS(m_LightCbv.GetAddressOf())));

		// 创建材质的资源视图
		auto lightCbvSize = D3DUtil::CalcCBByteSize((UINT)GetLightByteSize());
		for (std::size_t i = 0; i < LightApp::FrameCount; ++i) {
			auto lightCB = frameResourceArray[i]->m_ConstantBuffers.find("Lights")->second->GetResource();
			auto lightCbvAdress = lightCB->GetGPUVirtualAddress();

			D3D12_CONSTANT_BUFFER_VIEW_DESC lightCbvDesc{};
			lightCbvDesc.BufferLocation = lightCbvAdress;
			lightCbvDesc.SizeInBytes = lightCbvSize;

			auto lightHandle = m_LightCbv->GetCPUDescriptorHandleForHeapStart();
			lightHandle.ptr += i * cbvDescriptorSize;

			device->CreateConstantBufferView(&lightCbvDesc, lightHandle);
		}
	}


	LightManager::LightManager(int maxDirLight, int maxPointLight, int maxSpotLight)
	{
		m_DirLights.resize(maxDirLight);
		m_PointLights.resize(maxPointLight);
		m_SpotLights.resize(maxSpotLight);
	}

}