#ifndef __IMGUIMANAGER__H__
#define __IMGUIMANAGER__H__

#include "../Common/BaseImGuiManager.h"

namespace DSM {
	class ImGuiManager : public BaseImGuiManager<ImGuiManager>
	{
	protected:
		friend BaseImGuiManager::BaseType;
		ImGuiManager() = default;
		virtual ~ImGuiManager() = default;

		void UpdateImGui(const CpuTimer& timer) override;

	public:
		float m_Theta = 0.f,
			m_Phi = 0.f,
			m_Radius = 6.f,
			m_Scale = 1.f,
			m_Fov = DirectX::XM_PIDIV2,
			m_Dx = 0,
			m_Dy = 0;
		bool m_Animate = true;

	};
}

#endif // !__IMGUIMANAGER__H__
