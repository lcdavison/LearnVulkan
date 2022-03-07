#include "VulkanPBR.hpp"

#include "ErrorHandling.hpp"

#include <memory>

LRESULT CALLBACK PBR::VulkanApplication::WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	switch (Message)
	{
	case WM_DESTROY:
		::PostQuitMessage(EXIT_SUCCESS);
		return EXIT_SUCCESS;
	}

	return ::DefWindowProc(Window, Message, WParam, LParam);
}

bool PBR::VulkanApplication::Initialise()
{
	bool bResult = false;

	HINSTANCE CurrentInstance = ::GetModuleHandle(nullptr);

	WNDCLASSEX WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.hInstance = CurrentInstance;
	WindowClass.lpfnWndProc = &PBR::VulkanApplication::WindowProcedure;
	WindowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);

	if (::RegisterClassEx(&WindowClass))
	{
		WindowHandle_ = ::CreateWindow(kWindowClassName, kWindowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, kDefaultWindowWidth, kDefaultWindowHeight, nullptr, nullptr, CurrentInstance, nullptr);

		bResult = WindowHandle_ != INVALID_HANDLE_VALUE;
	}

	return bResult;
}

bool PBR::VulkanApplication::Run()
{
	bool bResult = false;

	::MessageBox(nullptr, TEXT("Running"), TEXT("MSG"), MB_OK);

	return bResult;
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	PBR::VulkanApplication& Application = PBR::VulkanApplication::Get();
	bool bInitialised = Application.Initialise();

	if (!bInitialised)
	{
		return EXIT_FAILURE;
	}

	RunFunctionAndCatchException(&PBR::VulkanApplication::Run, &Application);

	return EXIT_SUCCESS;
}
