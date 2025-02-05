#include "ImGuiManager.h"

void DSM::ImGuiManager::UpdateImGui(const CpuTimer& timer)
{
	bool show = true;
	ImGui::ShowDemoWindow(&show);
}
