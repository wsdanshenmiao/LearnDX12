#pragma once
#ifndef __IMGUIMANAGER__H__
#define __IMGUIMANAGER__H__

#include "BaseImGuiManager.h"
#include "Transform.h"

namespace DSM {
	class ImguiManager : public BaseImGuiManager<ImguiManager>
	{
	protected:
		friend BaseImGuiManager::BaseType;
		ImguiManager() = default;
		virtual ~ImguiManager() = default;

		void UpdateImGui(const CpuTimer& timer) override;

	public:
		Transform m_Transform;
		float m_Radius = 50.0f, m_Fov = DirectX::XM_PIDIV2;
		bool m_Animate = false;
		bool m_EnableWireFrame = false;
		float m_FogStart = 10;
		float m_FogRange = 100;
		DirectX::XMFLOAT3 m_FogColor = DirectX::XMFLOAT3(1, 1, 1);
		DirectX::XMFLOAT3 m_LightDir;
		DirectX::XMFLOAT3 m_LightColor;
		DirectX::XMFLOAT3 m_EyePos{};
	};
}

#endif // !__IMGUIMANAGER__H__
