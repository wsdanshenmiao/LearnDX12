#include "Shapes.h"
#include "Object.h"


using namespace DSM;
using namespace DirectX;

int App(HINSTANCE hInstance)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		LandAndWave app(hInstance, L"Shapes", 1024, 720);
		app.OnInit();
		return app.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}

void testObject() {
	std::unique_ptr obj0 = std::make_unique<Object>("obj0");
	std::unique_ptr obj1 = std::make_unique<Object>("obj1");
	obj0->AddChild(obj1.get());
}

int WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	return App(hInstance);
}

