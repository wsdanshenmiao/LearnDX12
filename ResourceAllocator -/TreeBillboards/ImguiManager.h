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
		enum class RenderModel : int
		{
			Triangles, 
		};
		
		void RenderImGui(ID3D12GraphicsCommandList* cmdList) override;
		
	protected:
		friend BaseImGuiManager::BaseType;
		ImguiManager() = default;
		virtual ~ImguiManager() = default;

		void UpdateImGui(const CpuTimer& timer) override;

	public:
		DirectX::XMFLOAT3 m_LightDir;
		DirectX::XMFLOAT3 m_LightColor;
		float m_CylineHeight = 1;
	};
}

#endif // !__IMGUIMANAGER__H__
