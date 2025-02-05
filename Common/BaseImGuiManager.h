#pragma once
#ifndef __BASEIMGUIMANAGER__H__
#define __BASEIMGUIMANAGER__H__

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "Singleton.h"
#include "Pubh.h"
#include "CpuTimer.h"


namespace DSM {

	template<typename Driver>
	class BaseImGuiManager : public Singleton<Driver>
	{
	public:
		using BaseType = Singleton<Driver>;
		virtual bool InitImGui(ID3D12Device* device, HWND hMainWnd, int bufferCount, DXGI_FORMAT bufferFormat);
		void ImGuiNewFrame();
		void Update(const CpuTimer& timer);
		virtual void RenderImGui(ID3D12GraphicsCommandList* cmdList);

	protected:
		BaseImGuiManager() = default;
		virtual ~BaseImGuiManager();

		virtual void UpdateImGui(const CpuTimer& timer);

	protected:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_ImGuiSrvHeap;		// 提供给ImGui的着色器资源描述符堆
	};

	template<typename Driver>
	bool BaseImGuiManager<Driver>::InitImGui(
		ID3D12Device* device,
		HWND hMainWnd,
		int bufferCount,
		DXGI_FORMAT bufferFormat)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// 允许键盘控制

		ImGui::StyleColorsDark();

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NodeMask = 0;
		desc.NumDescriptors = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_ImGuiSrvHeap.GetAddressOf())) != S_OK)
			return false;

		ImGui_ImplWin32_Init(hMainWnd);
		ImGui_ImplDX12_Init(
			device,
			bufferCount,
			bufferFormat,
			m_ImGuiSrvHeap.Get(),
			m_ImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_ImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart());

		return true;
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::ImGuiNewFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::Update(const CpuTimer& timer)
	{
		ImGuiNewFrame();
		UpdateImGui(timer);
		ImGui::Render();
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::UpdateImGui(const CpuTimer& timer)
	{
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::RenderImGui(ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->SetDescriptorHeaps(1, m_ImGuiSrvHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
	}

	template<typename Driver>
	BaseImGuiManager<Driver> ::~BaseImGuiManager()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

}

#endif // !__BASEIMGUIMANAGER__H__
