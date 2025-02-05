#pragma once
#ifndef __LIGHTMANAGER__H__
#define __LIGHTMANAGER__H__

#include "FrameResource.h"
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
		std::size_t GetLightByteSize() const;
		ID3D12DescriptorHeap* GetLightCbv() const;

		void SetDirLight(std::size_t index, const DirectionalLight& light);
		void SetPointLight(std::size_t index, const PointLight& light);
		void SetSpotLight(std::size_t index, const SpotLight& light);

		void UpdateLight(FrameResource* currFrameResource);

		std::pair<std::string, std::unique_ptr<UploadBuffer<void>>>
			CreateLightBuffer(ID3D12Device* device);
		void CreateLightCbv(
			ID3D12Device* device,
			std::unique_ptr<FrameResource>* frameResourceArray,
			std::size_t cbvDescriptorSize);

	protected:
		friend Singleton<LightManager>;
		LightManager(int maxDirLight = 5, int maxPointLight = 5, int maxSpotLight = 5);
		virtual ~LightManager() = default;

	private:
		ComPtr<ID3D12DescriptorHeap> m_LightCbv = nullptr;

		int m_NumFrameDirty;
		std::vector<DirectionalLight> m_DirLights;
		std::vector<PointLight> m_PointLights;
		std::vector<SpotLight> m_SpotLights;
	};
}

#endif // !__LIGHTMANAGER__H__
