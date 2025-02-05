#include "D3D12App.h"
#include "BaseImGuiManager.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace DSM {

	namespace {
		D3D12App* g_pD3D12App = nullptr;
	}


	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return g_pD3D12App->MsgProc(hwnd, msg, wParam, lParam);
	}


	D3D12App::D3D12App(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
		:m_hAppInst(hAppInst),
		m_MainWndCaption(mainWndCaption),
		m_ClientWidth(clientWidth),
		m_ClientHeight(clientHeight)
	{
		assert(g_pD3D12App == nullptr);
		g_pD3D12App = this;
	}

	D3D12App::~D3D12App()
	{
		if (m_D3D12Device != nullptr && m_D3D12Fence != nullptr)
			FlushCommandQueue();
	}

	bool D3D12App::OnInit()
	{
		if (!InitMainWindow())
			return false;
		if (!InitDirectX3D())
			return false;

		OnResize();

		return true;
	}

	void D3D12App::OnResize()
	{
		assert(m_D3D12Device);
		assert(m_DxgiSwapChain);
		assert(m_DirectCmdListAlloc);

		FlushCommandQueue();

		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

		CreateMsaaResources();

		// 释放先前创建的资源
		for (int i = 0; i < SwapChainBufferCount; ++i) {
			m_SwapChainBuffer[i].Reset();
		}
		m_DepthStencilBuffer.Reset();

		// 更改交换链大小
		ThrowIfFailed(m_DxgiSwapChain->ResizeBuffers(
			SwapChainBufferCount,
			m_ClientWidth,
			m_ClientHeight,
			m_BackBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
		m_CurrBackBuffer = 0;

		// 创建渲染目标视图
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; ++i) {
			// 获取交换链缓冲区
			ThrowIfFailed(m_DxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(m_SwapChainBuffer[i].GetAddressOf())));
			m_D3D12Device->CreateRenderTargetView(m_SwapChainBuffer[i].Get(), nullptr, rtvHandle);
			// 偏移到描述符堆的下一个缓冲区
			rtvHandle.Offset(1, m_RtvDescriptorSize);
		}

		// 创建深度/模板缓冲区即视图
		D3D12_RESOURCE_DESC depthStencilDesc{};
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.Width = m_ClientWidth;
		depthStencilDesc.Height = m_ClientHeight;
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;

		// 清除资源
		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = m_DepthStencilFormat;
		clearValue.DepthStencil.Depth = 1.f;
		clearValue.DepthStencil.Stencil = 0;
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		// 创建缓冲区
		ThrowIfFailed(m_D3D12Device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&clearValue,
			IID_PPV_ARGS(m_DepthStencilBuffer.GetAddressOf())));

		// 创建第0mip层描述符
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = m_DepthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		m_D3D12Device->CreateDepthStencilView(
			m_DepthStencilBuffer.Get(),
			&dsvDesc,
			GetDepthStencilView());

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_DepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		// 将资源从初始状态转换为深度缓冲区
		m_CommandList->ResourceBarrier(1, &barrier);

		ThrowIfFailed(m_CommandList->Close());

		// 提交调整大小命令
		ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// 在完成前等待
		FlushCommandQueue();

		m_ScreenViewport.TopLeftX = 0;
		m_ScreenViewport.TopLeftY = 0;
		m_ScreenViewport.Width = static_cast<float>(m_ClientWidth);
		m_ScreenViewport.Height = static_cast<float>(m_ClientHeight);
		m_ScreenViewport.MinDepth = 0.0f;
		m_ScreenViewport.MaxDepth = 1.0f;

		m_ScissorRect = { 0,0,m_ClientWidth,m_ClientHeight };
	}

	void D3D12App::OnUpdate(const CpuTimer& timer) {}

	int D3D12App::Run()
	{
		MSG msg{ 0 };

		m_Timer.Reset();

		while (msg.message != WM_QUIT) {
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				m_Timer.Tick();

				if (!m_AppPaused) {
					CalculateFrameStats();
					OnUpdate(m_Timer);
					OnRender(m_Timer);
				}
				else {
					Sleep(100);
				}
			}
		}
		return (int)msg.wParam;
	}

	float D3D12App::GetAspectRatio() const noexcept
	{
		return static_cast<float>(m_ClientWidth) / m_ClientHeight;
	}

	bool D3D12App::Get4xMsaaState() const noexcept
	{
		return m_Enable4xMsaa;
	}

	bool D3D12App::InitDirectX3D()
	{
		// 检索接口指针，根据使用的接口指针的类型自动提供所请求接口的 IID 值

#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif

	// 创建工厂
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_DxgiFactory.GetAddressOf())));

	// 创建D3D硬件设备
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,	// 使用默认适配器
		D3D_FEATURE_LEVEL_11_0,	// 最小特征值
		IID_PPV_ARGS(m_D3D12Device.GetAddressOf()
		));

	// 创建失败使用软适配器Warp设备
	if (FAILED(hardwareResult)) {
		ComPtr<IDXGIAdapter> pWarpAdapter;
		// 枚举软适配器
		ThrowIfFailed(m_DxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(pWarpAdapter.GetAddressOf())));
		// 创建设备
		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(m_D3D12Device.GetAddressOf())));
	}

	// 创建围栏
	ThrowIfFailed(m_D3D12Device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(m_D3D12Fence.GetAddressOf())));

	// 获取描述符大小
	m_RtvDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DsvDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CbvSrvUavDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	assert(CheckMassSupport(4));

#if defined(DEBUG) || defined(_DEBUG)
	// 枚举适配器
	LogAdapter();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeap();

	return true;
	}

	bool D3D12App::InitMainWindow()
	{
		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_hAppInst;
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = 0;
		wc.lpszClassName = L"MainWnd";

		if (!RegisterClass(&wc))
		{
			MessageBox(0, L"RegisterClass Failed.", 0, 0);
			return false;
		}

		// Compute window rectangle dimensions based on requested client area dimensions.
		RECT R = { 0, 0, m_ClientWidth, m_ClientHeight };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;

		m_hMainWnd = CreateWindow(L"MainWnd", m_MainWndCaption.c_str(),
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hAppInst, 0);
		if (!m_hMainWnd)
		{
			MessageBox(0, L"CreateWindow Failed.", 0, 0);
			return false;
		}

		ShowWindow(m_hMainWnd, SW_SHOW);
		UpdateWindow(m_hMainWnd);

		return true;
	}

	LRESULT D3D12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		switch (msg) {
			// WM_ACTIVATE is sent when the window is activated or deactivated.  
			// We pause the game when the window is deactivated and unpause it 
			// when it becomes active.  
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE) {
				m_AppPaused = true;
				m_Timer.Stop();
			}
			else {
				m_AppPaused = false;
				m_Timer.Start();
			}
			return 0;

			// WM_SIZE is sent when the user resizes the window.  
		case WM_SIZE:
			// Save the new client area dimensions.
			m_ClientWidth = LOWORD(lParam);
			m_ClientHeight = HIWORD(lParam);
			if (m_D3D12Device) {
				if (wParam == SIZE_MINIMIZED) {
					m_AppPaused = true;
					m_Minimized = true;
					m_Maximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED) {
					m_AppPaused = false;
					m_Minimized = false;
					m_Maximized = true;
					OnResize();
				}
				else if (wParam == SIZE_RESTORED) {

					// Restoring from minimized state?
					if (m_Minimized) {
						m_AppPaused = false;
						m_Minimized = false;
						OnResize();
					}

					// Restoring from maximized state?
					else if (m_Maximized) {
						m_AppPaused = false;
						m_Maximized = false;
						OnResize();
					}
					else if (m_Resizing) {
						// If user is dragging the resize bars, we do not resize 
						// the buffers here because as the user continuously 
						// drags the resize bars, a stream of WM_SIZE messages are
						// sent to the window, and it would be pointless (and slow)
						// to resize for each WM_SIZE message received from dragging
						// the resize bars.  So instead, we reset after the user is 
						// done resizing the window and releases the resize bars, which 
						// sends a WM_EXITSIZEMOVE message.
					}
					else { // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
						OnResize();
					}
				}
			}
			return 0;

			// 用户抓取调整栏
		case WM_ENTERSIZEMOVE:
			m_AppPaused = true;
			m_Resizing = true;
			m_Timer.Stop();
			return 0;

			// 用户释放调整栏
			// 根据窗口大小重新设置相关对象
		case WM_EXITSIZEMOVE:
			m_AppPaused = false;
			m_Resizing = false;
			m_Timer.Start();
			OnResize();
			return 0;

			// 当窗口被销毁
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

			// The WM_MENUCHAR message is sent when a menu is active and the user presses 
			// a key that does not correspond to any mnemonic or accelerator key. 
		case WM_MENUCHAR:
			// Don't beep when we alt-enter.
			return MAKELRESULT(0, MNC_CLOSE);

			// 捕获此消息以防窗口变的过小
		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			return 0;

		case WM_KEYUP:
			if (wParam == VK_ESCAPE) {
				PostQuitMessage(0);
			}
			else if ((int)wParam == VK_F2)
				Set4xMsaaState(!m_Enable4xMsaa);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	void D3D12App::CreateCommandObjects()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		ThrowIfFailed(m_D3D12Device->CreateCommandQueue(
			&queueDesc,
			IID_PPV_ARGS(m_CommandQueue.GetAddressOf())));

		ThrowIfFailed(m_D3D12Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_DirectCmdListAlloc.GetAddressOf())));

		ThrowIfFailed(m_D3D12Device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_DirectCmdListAlloc.Get(),
			nullptr,										// 不使用流水线状态对象
			IID_PPV_ARGS(m_CommandList.GetAddressOf())));

		// 第一次引用需要重置命令列表，重置前需要关闭列表
		ThrowIfFailed(m_CommandList->Close());
	}

	void D3D12App::FlushCommandQueue()
	{
		// 推进栅栏值，将命令标记到此栅栏点
		++m_CurrentFence;

		// 添加一条指令到命令队列来设置一个新的栅栏点。
		// 因为我们在GPU时间轴上，直到GPU完成处理这个Signal（）之前的所有命令,新的栅栏点将不会被设置.
		ThrowIfFailed(m_CommandQueue->Signal(m_D3D12Fence.Get(), m_CurrentFence));

		// 等待GPU完成这个栅栏点的命令。
		if (m_D3D12Fence->GetCompletedValue() < m_CurrentFence) {
			HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

			// 当GPU碰到当前栅栏时触发事件
			ThrowIfFailed(m_D3D12Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));

			// 等待直到GPU触发当前栅栏事件
			if (eventHandle != 0) {
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}
	}

	// DX12不支持直接在交换链上开启Msaa，
	void D3D12App::CreateSwapChain()
	{
		m_DxgiSwapChain.Reset();

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = m_ClientWidth;
		swapChainDesc.Height = m_ClientHeight;
		swapChainDesc.Format = m_BackBufferFormat;
		swapChainDesc.BufferCount = SwapChainBufferCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
		fullscreenDesc.RefreshRate.Numerator = 60;
		fullscreenDesc.RefreshRate.Denominator = 1;
		fullscreenDesc.Windowed = true;
		fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

		// [从 Direct3D 11.1 开始，建议不要再使用 CreateSwapChain 来创建交换链。 请改用 CreateSwapChainForHwnd、 CreateSwapChainForCoreWindow 或 CreateSwapChainForComposition 
		ThrowIfFailed(m_DxgiFactory->CreateSwapChainForHwnd(
			m_CommandQueue.Get(),
			m_hMainWnd,
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			m_DxgiSwapChain.GetAddressOf()));
	}

	void D3D12App::CreateMsaaResources()
	{
		// 创建描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC msaaRtvHeapDesc{};
		msaaRtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		msaaRtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		msaaRtvHeapDesc.NumDescriptors = 1;
		msaaRtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&msaaRtvHeapDesc, IID_PPV_ARGS(m_MsaaRtvHeap.GetAddressOf())));

		// 创建RT资源
		D3D12_RESOURCE_DESC msaaRtDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				m_BackBufferFormat,
				m_ClientWidth,
				m_ClientHeight,
				1, 1, 4);
		msaaRtDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE rtClearValue{};
		rtClearValue.Format = m_BackBufferFormat;
		memcpy(rtClearValue.Color, Colors::Pink, sizeof(float) * 4);

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_D3D12Device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&msaaRtDesc,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,	// 这里资源状态是RESOLVE_SOURCE，而不是RENDER_TARGET
			&rtClearValue,
			IID_PPV_ARGS(m_MsaaRenderTarget.GetAddressOf())));
		m_MsaaRenderTarget->SetName(L"Msaa Render Target");

		// 创建RT视图
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		rtvDesc.Format = m_BackBufferFormat;
		m_D3D12Device->CreateRenderTargetView(
			m_MsaaRenderTarget.Get(),
			&rtvDesc,
			m_MsaaRtvHeap->GetCPUDescriptorHandleForHeapStart());



		D3D12_DESCRIPTOR_HEAP_DESC msaaDsvHeapDesc{};
		msaaDsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		msaaDsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		msaaDsvHeapDesc.NumDescriptors = 1;
		msaaDsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&msaaDsvHeapDesc, IID_PPV_ARGS(m_MsaaDsvHeap.GetAddressOf())));

		// 创建RT资源
		D3D12_RESOURCE_DESC msaaDsDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				m_DepthStencilFormat,
				m_ClientWidth,
				m_ClientHeight,
				1, 1, 4);
		msaaDsDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE dsClearValue{};
		dsClearValue.Format = m_DepthStencilFormat;
		dsClearValue.DepthStencil.Depth = 1.f;
		dsClearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(m_D3D12Device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&msaaDsDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&dsClearValue,
			IID_PPV_ARGS(m_MsaaDepthStencil.GetAddressOf())));
		m_MsaaDepthStencil->SetName(L"Msaa Depth Stencil");

		// 创建Ds视图
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		dsvDesc.Format = m_DepthStencilFormat;
		m_D3D12Device->CreateDepthStencilView(
			m_MsaaDepthStencil.Get(),
			&dsvDesc,
			m_MsaaDsvHeap->GetCPUDescriptorHandleForHeapStart());

	}

	void D3D12App::CreateDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvDesc.NumDescriptors = SwapChainBufferCount;
		rtvDesc.NodeMask = 0;	// 对于单适配器操作，请将此项设置为零
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(m_RtvHeap.GetAddressOf())));

		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvDesc.NumDescriptors = 1;
		rtvDesc.NodeMask = 0;
		ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())));
	}

	ID3D12Resource* D3D12App::GetCurrentBackBuffer() const
	{
		return m_SwapChainBuffer[m_CurrBackBuffer].Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::GetCurrentBackBufferView() const
	{
		// 通过CD3DX12构造函数根据偏移量找到RTV
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_CurrBackBuffer,
			m_RtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::GetDepthStencilView() const
	{
		return m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	void D3D12App::Set4xMsaaState(bool value)
	{
		if (m_Enable4xMsaa != value)
		{
			m_Enable4xMsaa = value;
		}
	}

	bool D3D12App::CheckMassSupport(int count)
	{
		// 检测对Mass的支持
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLeavels{};
		msQualityLeavels.Format = m_BackBufferFormat;
		msQualityLeavels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msQualityLeavels.NumQualityLevels = 0;
		msQualityLeavels.SampleCount = count;
		ThrowIfFailed(m_D3D12Device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLeavels, sizeof(msQualityLeavels)));
		m_4xMsaaQuality = msQualityLeavels.NumQualityLevels;
		return m_4xMsaaQuality > 0;
	}

	void D3D12App::CalculateFrameStats()
	{
		// Code computes the average frames per second, and also the 
		// average time it takes to render one frame.  These stats 
		// are appended to the window caption bar.

		static int frameCnt = 0;
		static float timeElapsed = 0.0f;

		frameCnt++;

		// Compute averages over one second period.
		if ((m_Timer.TotalTime() - timeElapsed) >= 1.0f)
		{
			float fps = (float)frameCnt; // fps = frameCnt / 1
			float mspf = 1000.0f / fps;

			std::wstring fpsStr = std::to_wstring(fps);
			std::wstring mspfStr = std::to_wstring(mspf);

			std::wstring windowText = m_MainWndCaption +
				L"    fps: " + fpsStr +
				L"   mspf: " + mspfStr;

			SetWindowText(m_hMainWnd, windowText.c_str());

			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}
	}

	void D3D12App::LogAdapter()
	{
		UINT i = 0;
		ComPtr<IDXGIAdapter> pAdapter;
		std::vector<ComPtr<IDXGIAdapter>> pAdapterList;
		for (; m_DxgiFactory->EnumAdapters(i, pAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC desc;
			pAdapter->GetDesc(&desc);
			std::wstring text = L"Adapter: ";
			text += desc.Description;
			text += '\n';
			OutputDebugString(text.c_str());
			pAdapterList.push_back(pAdapter);
		}

		for (const auto& adapter : pAdapterList) {
			UINT i = 0;
			ComPtr<IDXGIOutput> output = nullptr;
			while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
			{
				DXGI_OUTPUT_DESC desc;
				output->GetDesc(&desc);

				std::wstring text = L"***Output: ";
				text += desc.DeviceName;
				text += L"\n";
				OutputDebugString(text.c_str());

				LogOutputDisplayModes(output.Get());

				++i;
			}
		}
	}

	void D3D12App::LogOutputDisplayModes(IDXGIOutput* output)
	{
		UINT count = 0;
		UINT flags = 0;

		// 通过nullptr获取列表数量
		output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, flags, &count, nullptr);

		std::vector<DXGI_MODE_DESC> modeList(count);
		output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, flags, &count, &modeList[0]);

		for (auto& x : modeList)
		{
			UINT n = x.RefreshRate.Numerator;
			UINT d = x.RefreshRate.Denominator;
			std::wstring text =
				L"Width = " + std::to_wstring(x.Width) + L" " +
				L"Height = " + std::to_wstring(x.Height) + L" " +
				L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
				L"\n";

			::OutputDebugString(text.c_str());
		}
	}

}

