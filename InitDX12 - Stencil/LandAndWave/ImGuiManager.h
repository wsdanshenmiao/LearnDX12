#ifndef __IMGUIMANAGER__H__
#define __IMGUIMANAGER__H__

#include "BaseImGuiManager.h"
#include "Transform.h"
#include <DirectXMath.h>

namespace DSM {
	class ImGuiManager : public BaseImGuiManager<ImGuiManager>
	{
	protected:
		friend BaseImGuiManager::BaseType;
		ImGuiManager() = default;
		virtual ~ImGuiManager() = default;

		void UpdateImGui(const CpuTimer& timer) override;

	public:
		Transform m_Transform;
		float m_Radius = 50.0f, m_Fov = DirectX::XM_PIDIV2;
		bool m_Animate = false;
		bool m_EnableWireFrame = false;
		int m_Subdivision = 0;
		DirectX::XMFLOAT3 m_EyePos{};
	};
}

#endif // !__IMGUIMANAGER__H__
