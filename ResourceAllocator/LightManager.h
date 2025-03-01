#pragma once
#ifndef __LIGHTMANAGER__H__
#define __LIGHTMANAGER__H__

#include "Singleton.h"
#include "Light.h"

namespace DSM {
	class FrameResource;
	
	class LightManager : public Singleton<LightManager>
	{

	public:
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		std::size_t GetDirLightCount()const;
		std::size_t GetPointLightCount() const;
		std::size_t GetSpotLightCount() const;
		const std::string& GetLightBufferName() const;
		UINT GetLightByteSize() const;
		std::vector<D3D_SHADER_MACRO> GetLightsShaderMacros(
			const char* dirName,
			const char* pointName,
			const char* spotName) const;

		void SetDirLight(std::size_t index, const DirectionalLight& light);
		void SetPointLight(std::size_t index, const PointLight& light);
		void SetSpotLight(std::size_t index, const SpotLight& light);

		void UpdateLight(FrameResource* frameResource);

	protected:
		friend Singleton<LightManager>;
		LightManager(
			UINT frameCount,
			int maxDirLight = 3,
			int maxPointLight = 1,
			int maxSpotLight = 1);
		virtual ~LightManager() = default;

	private:
		const std::string m_LightBufferName = "LightBuffer";
		const UINT m_FrameCount;
		UINT m_NumFrameDirty;

		std::vector<DirectionalLight> m_DirLights;
		std::vector<PointLight> m_PointLights;
		std::vector<SpotLight> m_SpotLights;
	};
}

#endif // !__LIGHTMANAGER__H__
