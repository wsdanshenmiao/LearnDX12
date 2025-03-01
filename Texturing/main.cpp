#include "TexturingApp.h"

using namespace DSM;
using namespace DirectX;

int WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try {
		ResourceAllocatorAPP app(hInstance, L"Texturing", 1024, 768);
		app.OnInit();
		return app.Run();
	}
	catch (DxException& ex) {
		MessageBox(nullptr, ex.ToString().c_str(), L"HR Faild", MB_OK);
		return -1;
	}
}
