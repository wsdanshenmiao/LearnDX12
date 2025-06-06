#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(const CpuTimer& timer)
	{
		static float theta = 1.5f * XM_PI, phi = XM_PIDIV2 - 0.1f, scale = 1;
		static float lightDir[] = {0.57735f, -0.57735f, 0.57735f};
		static float lightColor[] = {1,1,1};
		static float fogColor[] = {1,1,1};
		static float fogStart = 20;
		static float fogRange = 100;
		float dt = timer.DeltaTime();
		auto io = ImGui::GetIO();
		auto& position = m_Transform.GetPosition();
		
		if (m_Animate) {
			phi += .3f * dt;
			theta += .3f * dt;
			phi = XMScalarModAngle(phi);
			theta = XMScalarModAngle(theta);
		}

		if (ImGui::Begin("ImGui"))
		{
			ImGui::Checkbox("Animate Object", &m_Animate);
			ImGui::Checkbox("Enable Wire Frame", &m_EnableWireFrame);
			if (ImGui::Button("Reset Params"))
			{
				m_Transform.SetPosition(0, 0, 0);
				phi = theta = 0.0f;
				m_Transform.SetScale(1, 1, 1);
				m_Fov = XM_PIDIV2;
			}
			ImGui::SliderFloat("Scale", &scale, 0.2f, 2.0f);

			ImGui::Text("Phi: %.2f degrees", XMConvertToDegrees(phi));	// 弧度变角度
			ImGui::SliderFloat("##1", &phi, -XM_PI, XM_PI, "");     // 不显示文字，但避免重复的标签
			ImGui::Text("Theta: %.2f degrees", XMConvertToDegrees(theta));
			ImGui::SliderFloat("##2", &theta, -XM_PI, XM_PI, "");

			ImGui::Text("Position: (%.1f, %.1f, %.1f)", position.x, position.y, position.z);

			ImGui::Text("FOV: %.2f degrees", XMConvertToDegrees(m_Fov));
			ImGui::SliderFloat("##3", &m_Fov, XM_PIDIV4, XM_PI / 3 * 2, "");
			
			ImGui::Text("Light Direction: (%.1f, %.1f, %.1f)",m_LightDir.x, m_LightDir.y, m_LightDir.z);
			ImGui::SliderFloat3("##4", lightDir, -1, 1 ,"");
			ImGui::Text("Light Color: (%.1f, %.1f, %.1f)",m_LightColor.x, m_LightColor.y, m_LightColor.z);
			ImGui::SliderFloat3("##5", lightColor, 0, 1,"");

			ImGui::Text("Fog Color: (%.1f, %.1f, %.1f)" ,m_FogColor.x, m_FogColor.y, m_FogColor.z);
			ImGui::SliderFloat3("##6", fogColor, 0, 1 ,"");
			ImGui::Text("Fog Start: %.2f", fogStart);
			ImGui::SliderFloat("##7", &fogStart, 0, 100, "");
			ImGui::Text("Fog Range: %.2f", fogRange);
			ImGui::SliderFloat("##8", &fogRange, 0, 500, "");
		}
		ImGui::End();

		// 不允许在操作UI时操作物体
		if (!ImGui::IsAnyItemActive())
		{
			// 鼠标左键拖动平移
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				position.x += io.MouseDelta.x * 0.1f;
				position.y -= io.MouseDelta.y * 0.1f;
			}
			// 鼠标右键拖动旋转
			else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
			{
				phi -= io.MouseDelta.y * 0.01f;
				theta -= io.MouseDelta.x * 0.01f;
				phi = XMScalarModAngle(phi);
				theta = XMScalarModAngle(theta);
			}
			// 鼠标滚轮缩放
			else if (io.MouseWheel != 0.0f)
			{
				scale += 0.02f * io.MouseWheel;
				if (scale > 2.0f)
					scale = 2.0f;
				else if (scale < 0.2f)
					scale = 0.2f;
			}
		}
		m_Transform.SetScale(scale, scale, scale);
		m_EyePos.x = m_Radius * sinf(phi) * cosf(theta);
		m_EyePos.z = m_Radius * sinf(phi) * sinf(theta);
		m_EyePos.y = m_Radius * cosf(phi);
		m_LightColor = {lightColor[0], lightColor[1], lightColor[2]};
		m_LightDir = {lightDir[0], lightDir[1], lightDir[2]};
		m_FogColor  = {fogColor[0], fogColor[1], fogColor[2]};
		m_FogStart = fogStart;
		m_FogRange = fogRange;
	}

}