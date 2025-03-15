#include "LightManager.h"
#include "FrameResource.h"

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

	const void* LightManager::GetDirLight() const
	{
		return m_DirLights.data();
	}

	const void* LightManager::GetPointLight() const
	{
		return m_PointLights.data();
	}

	const void* LightManager::GetSpotLight() const
	{
		return m_SpotLights.data();
	}

	const std::string& LightManager::GetLightBufferName() const
	{
		return m_LightBufferName;
	}

	/// <summary>
	/// 获取对齐后的所有光源大小
	/// </summary>
	/// <returns></returns>
	UINT LightManager::GetLightByteSize() const
	{
		auto dirLightSize = m_DirLights.size() * sizeof(DirectionalLight);
		auto pointLightSize = m_PointLights.size() * sizeof(PointLight);
		auto spotLightSize = m_PointLights.size() * sizeof(SpotLight);
		auto byteSize = dirLightSize + pointLightSize + spotLightSize;
		return D3DUtil::CalcCBByteSize(byteSize);
	}

	std::vector<D3D_SHADER_MACRO> LightManager::GetLightsShaderMacros(
		const char* dirName,
		const char* pointName,
		const char* spotName) const
	{
		static std::string lightCount[3];
		lightCount[0] = std::to_string(m_DirLights.size());
		lightCount[1] = std::to_string(m_PointLights.size());
		lightCount[2] = std::to_string(m_SpotLights.size());

		std::vector<D3D_SHADER_MACRO> shaderMacros(3);
		shaderMacros[0] = { dirName, lightCount[0].c_str() };
		shaderMacros[1] = { pointName, lightCount[1].c_str() };
		shaderMacros[2] = { spotName, lightCount[2].c_str() };

		return shaderMacros;
	}

	void LightManager::SetDirLight(std::size_t index, const DirectionalLight& light)
	{
		if (index < m_DirLights.size()) {
			m_DirLights[index] = light;
		}
	}

	void LightManager::SetPointLight(std::size_t index, const PointLight& light)
	{
		if (index < m_PointLights.size()) {
			m_PointLights[index] = light;
		}
	}

	void LightManager::SetSpotLight(std::size_t index, const SpotLight& light)
	{
		if (index < m_SpotLights.size()) {
			m_SpotLights[index] = light;
		}
	}

	LightManager::LightManager(
		int maxDirLight,
		int maxPointLight,
		int maxSpotLight)
	{
		m_DirLights.resize(maxDirLight);
		m_PointLights.resize(maxPointLight);
		m_SpotLights.resize(maxSpotLight);
	}

}