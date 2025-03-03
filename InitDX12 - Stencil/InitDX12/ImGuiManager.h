#pragma once
#ifndef __IMGUIMANAGER__H__
#define __IMGUIMANAGER__H__

#include "BaseImGuiManager.h"

namespace DSM {
	class ImGuiManager : public BaseImGuiManager<ImGuiManager>
	{
	protected:
		friend BaseImGuiManager::BaseType;
		ImGuiManager() = default;
		virtual ~ImGuiManager() = default;

		void UpdateImGui(const CpuTimer& timer) override;
	};
}

#endif // !__IMGUIMANAGER__H__
