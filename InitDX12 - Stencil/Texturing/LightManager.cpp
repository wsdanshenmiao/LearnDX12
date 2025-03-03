#include "LightManager.h"
#include "Light.h"
#include "D3DUtil.h"

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

	UINT LightManager::GetLightByteSize() const
	{
		auto dirLightSize = m_DirLights.size() * sizeof(DirectionalLight);
		auto pointLightSize = m_PointLights.size() * sizeof(PointLight);
		auto spotLightSize = m_PointLights.size() * sizeof(SpotLight);
		return dirLightSize + pointLightSize + spotLightSize;
	}

	ID3D12Resource* LightManager::GetLightsResource() const
	{
		return m_LightsUploaders[m_Counter].Get();
	}

	void LightManager::SetDirLight(std::size_t index, const DirectionalLight& light)
	{
		if (index < m_DirLights.size()) {
			m_NumFrameDirty = m_FrameCount;
			m_DirLights[index] = light;
		}
	}

	void LightManager::SetPointLight(std::size_t index, const PointLight& light)
	{
		if (index < m_PointLights.size()) {
			m_NumFrameDirty = m_FrameCount;
			m_PointLights[index] = light;
		}
	}

	void LightManager::SetSpotLight(std::size_t index, const SpotLight& light)
	{
		if (index < m_SpotLights.size()) {
			m_NumFrameDirty = m_FrameCount;
			m_SpotLights[index] = light;
		}
	}

	void LightManager::UpdateLight()
	{
		m_Counter = (m_Counter + 1) % m_FrameCount;

		// 后续FrameNum个缓冲区都需要更新，前面的不用.
		if (m_NumFrameDirty <= 0)return;

		BYTE* mappedData;
		auto dirLightSize = sizeof(DirectionalLight) * m_DirLights.size();
		auto pointLightSize = sizeof(PointLight) * m_PointLights.size();
		auto spotLightSize = sizeof(SpotLight) * m_SpotLights.size();

		m_LightsUploaders[m_Counter]->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		memcpy(&mappedData[0], m_DirLights.data(), dirLightSize);
		memcpy(&mappedData[dirLightSize], m_PointLights.data(), pointLightSize);
		memcpy(&mappedData[dirLightSize + pointLightSize], m_SpotLights.data(), spotLightSize);
		m_LightsUploaders[m_Counter]->Unmap(0, nullptr);

		--m_NumFrameDirty;
	}
	
	void LightManager::CreateLightBuffer(ID3D12Device* device)
	{
		m_LightsUploaders.reserve(m_FrameCount);
		
		auto byteSize = D3DUtil::CalcCBByteSize(GetLightByteSize());

		// 创建堆属性
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;	// 根据适配器是NUMA或UMA来选择
		heapProps.CreationNodeMask = 0;
		heapProps.VisibleNodeMask = 0;

		// 创建资源描述
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = byteSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc = {1,0};
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		for (int i = 0; i < m_FrameCount; ++i){
			// 创建光源上传堆
			ThrowIfFailed(device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_LightsUploaders[i].GetAddressOf())));
		}
	}

	LightManager::LightManager(
		ID3D12Device* device,
		UINT frameCount,
		int maxDirLight,
		int maxPointLight,
		int maxSpotLight)
		:m_FrameCount(frameCount), m_Counter(0), m_NumFrameDirty(0){
		m_DirLights.resize(maxDirLight);
		m_PointLights.resize(maxPointLight);
		m_SpotLights.resize(maxSpotLight);
		
		CreateLightBuffer(device);
	}

}