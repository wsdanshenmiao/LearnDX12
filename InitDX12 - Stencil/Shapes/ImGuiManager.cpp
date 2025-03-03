#include "ImGuiManager.h"

using namespace DirectX;

void DSM::ImGuiManager::UpdateImGui(const CpuTimer& timer)
{
	float dt = timer.DeltaTime();
	auto io = ImGui::GetIO();

	if (m_Animate) {
		m_Theta += .3f * dt;
		m_Phi += .3f * dt;
		m_Phi = XMScalarModAngle(m_Phi);
		m_Theta = XMScalarModAngle(m_Theta);
	}

	if (ImGui::Begin("Use ImGui"))
	{
		ImGui::Checkbox("Animate Cube", &m_Animate);
		ImGui::Checkbox("Enable Wire Frame", &m_EnableWireFrame);
		if (ImGui::Button("Reset Params"))
		{
			m_Dx = m_Dy = m_Phi = m_Theta = 0.0f;
			m_Scale = 1.0f;
			m_Fov = XM_PIDIV2;
		}
		ImGui::SliderInt("Subdivision", &m_Subdivision, 0, 6);
		ImGui::SliderFloat("Scale", &m_Scale, 0.2f, 2.0f);

		ImGui::Text("Phi: %.2f degrees", XMConvertToDegrees(m_Phi));
		ImGui::SliderFloat("##1", &m_Phi, -XM_PI, XM_PI, "");     // 不显示文字，但避免重复的标签
		ImGui::Text("Theta: %.2f degrees", XMConvertToDegrees(m_Theta));
		ImGui::SliderFloat("##2", &m_Theta, -XM_PI, XM_PI, "");

		ImGui::Text("Position: (%.1f, %.1f, 0.0)", m_Dx, m_Dy);

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
			m_Dx += io.MouseDelta.x * 0.01f;
			m_Dy -= io.MouseDelta.y * 0.01f;
		}
		// 鼠标右键拖动旋转
		else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
		{
			m_Phi -= io.MouseDelta.y * 0.01f;
			m_Theta -= io.MouseDelta.x * 0.01f;
			m_Phi = XMScalarModAngle(m_Phi);
			m_Theta = XMScalarModAngle(m_Theta);
		}
		// 鼠标滚轮缩放
		else if (io.MouseWheel != 0.0f)
		{
			m_Scale += 0.02f * io.MouseWheel;
			if (m_Scale > 2.0f)
				m_Scale = 2.0f;
			else if (m_Scale < 0.2f)
				m_Scale = 0.2f;
		}
	}
}
