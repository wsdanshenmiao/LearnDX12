#pragma once
#ifndef __IMGUIMANAGER__H__
#define __IMGUIMANAGER__H__

#include "BaseImGuiManager.h"
#include "Transform.h"
#include "D3D12DescriptorHeap.h"

namespace DSM {
	class ImguiManager : public BaseImGuiManager<ImguiManager>
	{
	public:
		void RenderImGui(ID3D12GraphicsCommandList* cmdList) override;
		
	protected:
		friend BaseImGuiManager::BaseType;
		ImguiManager() = default;
		virtual ~ImguiManager() = default;

		void UpdateImGui(const CpuTimer& timer) override;

	public:
		bool m_EnableWireFrame = false;
		bool m_EnableDebug = true;
		float m_FogStart = 10;
		float m_FogRange = 100;
		DirectX::XMFLOAT3 m_FogColor = DirectX::XMFLOAT3(1, 1, 1);
		DirectX::XMFLOAT3 m_LightDir;
		DirectX::XMFLOAT3 m_LightColor;

		D3D12DescriptorHandle m_DebugShadowMapHandle;
	};
}

#endif // !__IMGUIMANAGER__H__
