#include "D3D12App.h"
#include "ImGuiManager.h"

using namespace DSM;
using namespace DirectX;

class GameApp : public D3D12App
{
public:
	GameApp(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth = 512, int clientHeight = 512);
	virtual ~GameApp();

	bool OnInit() override;

private:
	void OnUpdate(const CpuTimer& timer) override;
	void OnRender(const CpuTimer& timer) override;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance,
	_Inout_ HINSTANCE prevInstance,
	_In_ LPSTR cmdLine, _In_ int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		GameApp app(hInstance, L"InitD3D12", 1024, 720);
		app.OnInit();
		return app.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}

GameApp::GameApp(HINSTANCE hAppInst, const std::wstring& mainWndCaption, int clientWidth, int clientHeight)
	:D3D12App(hAppInst, mainWndCaption, clientWidth, clientHeight) {
}

GameApp::~GameApp()
{
	if (m_D3D12Device)
		FlushCommandQueue();
	ImGuiManager::GetInstance().ShutDown();
}

bool GameApp::OnInit()
{
	if (!D3D12App::OnInit())
		return false;

	ImGuiManager::Create();
	if (!ImGuiManager::GetInstance().InitImGui(
		m_D3D12Device.Get(),
		m_hMainWnd,
		SwapChainBufferCount,
		m_BackBufferFormat))
		return false;

	return true;
}

void GameApp::OnUpdate(const CpuTimer& timer)
{
	ImGuiManager::GetInstance().Update(timer);
}

void GameApp::OnRender(const CpuTimer& timer)
{
	// 重复使用记录命令的相关内存
	// 当GPU关联的命令列表执行完成后才重置
	ThrowIfFailed(m_DirectCmdListAlloc->Reset());

	// 将命令列表加入命令队列后，可重置列表以复用其内存
	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	// 设置视口和裁剪矩形，当命令列表重置其也需重置
	m_CommandList->RSSetViewports(1, &m_ScreenViewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	if (m_Enable4xMsaa) {
		D3D12_RESOURCE_BARRIER resolveToRt =
			CD3DX12_RESOURCE_BARRIER::Transition(
				m_MsaaRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_CommandList->ResourceBarrier(1, &resolveToRt);

		auto msaaRtv = m_MsaaRtvHeap->GetCPUDescriptorHandleForHeapStart();
		auto msaaDsv = m_MsaaDsvHeap->GetCPUDescriptorHandleForHeapStart();
		m_CommandList->ClearRenderTargetView(msaaRtv, Colors::Pink, 0, nullptr);
		m_CommandList->ClearDepthStencilView(
			msaaDsv,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			1.f, 0, 0, nullptr);

		m_CommandList->OMSetRenderTargets(1, &msaaRtv, true, &msaaDsv);

		///////////
		// 绘制
		///////////

		// 绘制完成后将Rt解析到后台缓冲区
		D3D12_RESOURCE_BARRIER barriers[2] = {		// 转换资源状态
			CD3DX12_RESOURCE_BARRIER::Transition(	// 将msaaRt设置为源
				m_MsaaRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RESOLVE_DEST) };
		m_CommandList->ResourceBarrier(2, barriers);

		// 将Rt解析到后台缓冲区
		m_CommandList->ResolveSubresource(GetCurrentBackBuffer(), 0, m_MsaaRenderTarget.Get(), 0, m_BackBufferFormat);

		// 将后台缓冲区从RESOLVE_DEST转换到PRESENT
		D3D12_RESOURCE_BARRIER destToPresent =
			CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_RESOLVE_DEST,
				D3D12_RESOURCE_STATE_PRESENT);
		m_CommandList->ResourceBarrier(1, &destToPresent);
	}
	else {

		// 转化资源状态，从呈现状态转换到渲染目标状态
		D3D12_RESOURCE_BARRIER presentToRt =
			CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_CommandList->ResourceBarrier(1, &presentToRt);

		auto currentBbv = GetCurrentBackBufferView();
		auto dsv = GetDepthStencilView();
		// 清除后台和深度模板缓冲区
		m_CommandList->ClearRenderTargetView(currentBbv, Colors::Pink, 0, nullptr);
		m_CommandList->ClearDepthStencilView(
			dsv,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			1.f, 0, 0, nullptr);

		// 指定要渲染的缓冲区
		m_CommandList->OMSetRenderTargets(1, &currentBbv, true, &dsv);

		// 对资源进行转换，从渲染目标转换到呈现状态
		D3D12_RESOURCE_BARRIER rtToPresent =
			CD3DX12_RESOURCE_BARRIER::Transition(
				GetCurrentBackBuffer(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);
		m_CommandList->ResourceBarrier(1, &rtToPresent);
	}

	// 绘制ImGui
	ImGuiManager::GetInstance().RenderImGui(m_CommandList.Get());

	// 完成命令的记录
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台与前台缓冲区
	ThrowIfFailed(m_DxgiSwapChain->Present(0, 0));
	m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;

	// 等待命令执行完成，后续重新组织防止等待
	FlushCommandQueue();
}
