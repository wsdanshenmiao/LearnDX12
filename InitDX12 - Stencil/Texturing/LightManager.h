#pragma once
#ifndef __LIGHTMANAGER__H__
#define __LIGHTMANAGER__H__

#include "Singleton.h"
#include "Light.h"

namespace DSM {

	class LightManager : public Singleton<LightManager>
	{

	public:
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		std::size_t GetDirLightCount()const;
		std::size_t GetPointLightCount() const;
		std::size_t GetSpotLightCount() const;
		UINT GetLightByteSize() const;
		ID3D12Resource* GetLightsResource() const;

		void SetDirLight(std::size_t index, const DirectionalLight& light);
		void SetPointLight(std::size_t index, const PointLight& light);
		void SetSpotLight(std::size_t index, const SpotLight& light);

		void UpdateLight();

	protected:
		friend Singleton<LightManager>;
		LightManager(ID3D12Device* device,
			UINT frameCount,
			int maxDirLight = 5,
			int maxPointLight = 5,
		int maxSpotLight = 5);
		virtual ~LightManager() = default;

		void CreateLightBuffer(ID3D12Device* device);

	private:
		
		std::vector<ComPtr<ID3D12Resource>> m_LightsUploaders;

		UINT m_NumFrameDirty;
		const UINT m_FrameCount;
		UINT m_Counter;
		
		std::vector<DirectionalLight> m_DirLights;
		std::vector<PointLight> m_PointLights;
		std::vector<SpotLight> m_SpotLights;
	};
}

#endif // !__LIGHTMANAGER__H__
