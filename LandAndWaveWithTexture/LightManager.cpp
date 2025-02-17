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

	/// <summary>
	/// 获取对齐后的所有光源大小
	/// </summary>
	/// <returns></returns>
	UINT LightManager::GetLightByteSize() const
	{
		return m_LightByteSize;
	}

	/// <summary>
	/// 获取当前光源常量缓冲区的GPU虚拟地址
	/// </summary>
	/// <returns></returns>
	D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetGPUVirtualAddress() const
	{
		return m_LightsUploaders->GetGPUVirtualAddress() + m_LightByteSize * m_Counter;
	}

	std::array<D3D_SHADER_MACRO, 4> LightManager::GetLightsShaderMacros(
		const char* dirName,
		const char* pointName,
		const char* spotName) const
	{
		static std::string lightCount[3];
		lightCount[0] = std::to_string(m_DirLights.size());
		lightCount[1] = std::to_string(m_PointLights.size());
		lightCount[2] = std::to_string(m_SpotLights.size());

		std::array<D3D_SHADER_MACRO, 4> shaderMacros{};
		shaderMacros[0] = { dirName, lightCount[0].c_str() };
		shaderMacros[1] = { pointName, lightCount[1].c_str() };
		shaderMacros[2] = { spotName, lightCount[2].c_str() };
		shaderMacros[3] = { nullptr, nullptr };	// 充当结束标志

		return shaderMacros;
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

		m_LightsUploaders->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		// 将映射数据偏移到当前帧的位置
		mappedData += m_LightByteSize * m_Counter;
		memcpy(&mappedData[0], m_DirLights.data(), dirLightSize);
		memcpy(&mappedData[dirLightSize], m_PointLights.data(), pointLightSize);
		memcpy(&mappedData[dirLightSize + pointLightSize], m_SpotLights.data(), spotLightSize);
		m_LightsUploaders->Unmap(0, nullptr);

		--m_NumFrameDirty;
	}

	void LightManager::CreateLightBuffer(ID3D12Device* device)
	{
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
		desc.Width = m_LightByteSize * m_FrameCount;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc = { 1,0 };
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_LightsUploaders.GetAddressOf())));
	}

	LightManager::LightManager(
		ID3D12Device* device,
		UINT frameCount,
		int maxDirLight,
		int maxPointLight,
		int maxSpotLight)
		:m_FrameCount(frameCount), m_Counter(0), m_NumFrameDirty(0) {
		m_DirLights.resize(maxDirLight);
		m_PointLights.resize(maxPointLight);
		m_SpotLights.resize(maxSpotLight);

		auto dirLightSize = m_DirLights.size() * sizeof(DirectionalLight);
		auto pointLightSize = m_PointLights.size() * sizeof(PointLight);
		auto spotLightSize = m_PointLights.size() * sizeof(SpotLight);
		auto byteSize = dirLightSize + pointLightSize + spotLightSize;
		m_LightByteSize = D3DUtil::CalcConstantBufferByteSize(byteSize);

		CreateLightBuffer(device);
	}

}