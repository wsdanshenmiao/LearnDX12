#pragma once
#ifndef __D3D12APP__H__
#define __D3D12APP__H__

#include "D3DUtil.h"
#include "CpuTimer.h"

namespace DSM {


	class D3D12App
	{
	public:
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		D3D12App(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);
		D3D12App(const D3D12App& other) = delete;
		D3D12App& operator=(const D3D12App& other) = delete;
		virtual ~D3D12App();

		virtual bool OnInit();
		int Run();

		float GetAspectRatio() const noexcept;
		bool Get4xMsaaState() const noexcept;

		void Set4xMsaaState(bool value);

		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	protected:
		virtual void OnResize();
		virtual void OnUpdate(const CpuTimer& timer) = 0;
		virtual void OnRender(const CpuTimer& timer) = 0;
		virtual void CreateDescriptorHeap();

		bool InitMainWindow();
		bool InitDirectX3D();
		bool InitImGui();

		void CreateSwapChain();
		void CreateMsaaResources();
		void CreateCommandObjects();
		void FlushCommandQueue();		// 刷新命令队列

		ID3D12Resource* GetCurrentBackBuffer()const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

		// 检测是否支持Msaa
		bool CheckMassSupport(int count);
		//  计算帧数与每帧时间并显示
		void CalculateFrameStats();
		// 枚举适配器
		void LogAdapter();
		void LogOutputDisplayModes(IDXGIOutput* output);

	protected:
		static const int SwapChainBufferCount = 2;	// 交换链数量

	protected:
		ComPtr<IDXGIFactory4>	m_DxgiFactory;							// 工厂实例
		ComPtr<ID3D12Device>	m_D3D12Device;							// D3D设备

		ComPtr<ID3D12CommandQueue> m_CommandQueue;						// 命令队列
		ComPtr<ID3D12CommandAllocator> m_DirectCmdListAlloc;			// 命令分配器
		ComPtr<ID3D12GraphicsCommandList> m_CommandList;				// 命令列表

		ComPtr<ID3D12DescriptorHeap> m_RtvHeap;							// 渲染目标描述符堆
		ComPtr<ID3D12DescriptorHeap> m_DsvHeap;							// 深度模板描述符堆

		ComPtr<IDXGISwapChain1>	m_DxgiSwapChain;						// 交换链
		ComPtr<ID3D12Resource> m_SwapChainBuffer[SwapChainBufferCount];	// 后台缓冲区
		ComPtr<ID3D12Resource> m_DepthStencilBuffer;					// 深度模板缓冲区

		ComPtr<ID3D12DescriptorHeap> m_MsaaRtvHeap;						// 开启Msaa的渲染目标描述符堆
		ComPtr<ID3D12DescriptorHeap> m_MsaaDsvHeap;						// 开启Msaa的深度模板描述符堆
		ComPtr<ID3D12Resource> m_MsaaRenderTarget;						// 开启Msaa的渲染目标缓冲区
		ComPtr<ID3D12Resource> m_MsaaDepthStencil;						// 开启Msaa的深度模板缓冲区

		ComPtr<ID3D12Fence> m_D3D12Fence;								// CPU/GPU同步围栏
		UINT64 m_CurrentFence = 0;										// 当前围栏值

		D3D12_VIEWPORT m_ScreenViewport{};		// 屏幕视口
		D3D12_RECT m_ScissorRect{};				// 裁剪矩形

		DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;		// 后台缓冲区格式
		DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	// 深度模板缓冲区格式


		bool m_Enable4xMsaa = false;			// 是否使用多重采样
		UINT m_4xMsaaQuality = 0;				// 多重采样质量
		int m_CurrBackBuffer = 0;				// 当前后台缓冲区的索引

		UINT m_RtvDescriptorSize = 0;			// 渲染目标视图资源大小
		UINT m_DsvDescriptorSize = 0;			// 深度/模板视图资源大小
		UINT m_CbvSrvUavDescriptorSize = 0;		// 常量缓冲/着色器资源/无序访问视图大小

		HINSTANCE m_hAppInst = nullptr;		// application instance handle
		HWND      m_hMainWnd = nullptr;		// main window handle
		bool      m_AppPaused = false;		// is the application paused?
		bool      m_Minimized = false;		// is the application minimized?
		bool      m_Maximized = false;		// is the application maximized?
		bool      m_Resizing = false;		// are the resize bars being dragged?
		bool      m_FullscreenState = false;// fullscreen enabled

		int m_ClientWidth = 512;
		int m_ClientHeight = 512;

		std::wstring m_MainWndCaption = L"D3D12App";

		CpuTimer m_Timer;
	};

}

#endif // !__D3D12APP__H__
