#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(const CpuTimer& timer)
	{
		static float lightDir[] = {0.57735f, -0.57735f, 0.57735f};
		static float lightColor[] = {1,1,1};
		float dt = timer.DeltaTime();
		auto io = ImGui::GetIO();
		static int currModel = 0;
		static const char* modes[] = {
			"Triangle", "Cylinder"
		};

		if (ImGui::Begin("ImGui"))
		{
			
			ImGui::Text("Light Direction: (%.1f, %.1f, %.1f)",m_LightDir.x, m_LightDir.y, m_LightDir.z);
			ImGui::SliderFloat3("##4", lightDir, -1, 1 ,"");
			ImGui::Text("Light Color: (%.1f, %.1f, %.1f)",m_LightColor.x, m_LightColor.y, m_LightColor.z);
			ImGui::SliderFloat3("##5", lightColor, 0, 1,"");
			ImGui::SliderFloat("CylineHeight: (%.1f)", &m_CylineHeight, 0, 10);
			if (ImGui::Combo("Modes", &currModel, modes, ARRAYSIZE(modes))) {
				if (currModel == 0) {
					m_RenderModel = RenderModel::Triangles;
				}
				else if (currModel == 1) {
					m_RenderModel = RenderModel::Cylinder;
				}
			}
		}
		ImGui::End();

		
		m_LightColor = {lightColor[0], lightColor[1], lightColor[2]};
		m_LightDir = {lightDir[0], lightDir[1], lightDir[2]};
	}

	void ImguiManager::RenderImGui(ID3D12GraphicsCommandList* cmdList)
	{
		BaseImGuiManager<ImguiManager>::RenderImGui(cmdList);
	}
}
