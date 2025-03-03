#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(const CpuTimer& timer)
	{
		static float theta = 1.5f * XM_PI, phi = XM_PIDIV2 - 0.1f, scale = 1;
		float dt = timer.DeltaTime();
		auto io = ImGui::GetIO();
		auto& position = m_Transform.GetPosition();

		if (m_Animate) {
			theta += .3f * dt;
			phi += .3f * dt;
			phi = XMScalarModAngle(phi);
			theta = XMScalarModAngle(theta);
		}

		if (ImGui::Begin("Use ImGui"))
		{
			ImGui::Checkbox("Animate Cube", &m_Animate);
			ImGui::Checkbox("Enable Wire Frame", &m_EnableWireFrame);
			if (ImGui::Button("Reset Params"))
			{
				m_Transform.SetPosition(0, 0, 0);
				phi = theta = 0.0f;
				m_Transform.SetScale(1, 1, 1);
				m_Fov = XM_PIDIV2;
			}
			ImGui::SliderInt("Subdivision", &m_Subdivision, 0, 6);
			ImGui::SliderFloat("Scale", &scale, 0.2f, 2.0f);

			ImGui::Text("Phi: %.2f degrees", XMConvertToDegrees(phi));
			ImGui::SliderFloat("##1", &phi, -XM_PI, XM_PI, "");     // 不显示文字，但避免重复的标签
			ImGui::Text("Theta: %.2f degrees", XMConvertToDegrees(theta));
			ImGui::SliderFloat("##2", &theta, -XM_PI, XM_PI, "");

			ImGui::Text("Position: (%.1f, %.1f, 0.0)", position.x, position.y);

			ImGui::Text("FOV: %.2f degrees", XMConvertToDegrees(m_Fov));
			ImGui::SliderFloat("##3", &m_Fov, XM_PIDIV4, XM_PI / 3 * 2, "");
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
	}

}