#include "VulkanPBR.hpp"

#include "Graphics/VulkanModule.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"
#include "ForwardRenderer.hpp"

extern Application::ApplicationState Application::State = Application::ApplicationState();

static LRESULT CALLBACK WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
        case WM_SIZE:
        {
            Application::State.CurrentWindowWidth = LOWORD(LParam);
            Application::State.CurrentWindowHeight = HIWORD(LParam);
        }
        break;
        case WM_DESTROY:
        {
            Application::State.bRunning = false;

            ::PostQuitMessage(EXIT_SUCCESS);
        }
        return 0;
        default:
            return ::DefWindowProc(Window, Message, WParam, LParam);
    }
}

static bool const Initialise()
{
    bool bResult = false;

    HINSTANCE CurrentInstance = ::GetModuleHandle(nullptr);

    WNDCLASSEX WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.lpszClassName = Application::kWindowClassName;
    WindowClass.hInstance = CurrentInstance;
    WindowClass.lpfnWndProc = &::WindowProcedure;
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    WindowClass.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
    WindowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));

    if (::RegisterClassEx(&WindowClass))
    {
        Application::State.WindowHandle = ::CreateWindow(Application::kWindowClassName,
                                                         Application::kWindowName,
                                                         WS_OVERLAPPEDWINDOW,
                                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                                         Application::kDefaultWindowWidth, Application::kDefaultWindowHeight,
                                                         nullptr, nullptr,
                                                         CurrentInstance, nullptr);

        bResult = Application::State.WindowHandle != INVALID_HANDLE_VALUE;
    }

    ::ShowWindow(Application::State.WindowHandle, TRUE);
    ::UpdateWindow(Application::State.WindowHandle);

    Application::State.ProcessHandle = CurrentInstance;

    if (VulkanModule::Start())
    {
        VkApplicationInfo ApplicationInfo = {};
        ApplicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
        ApplicationInfo.pApplicationName = "Vulkan PBR";
        ApplicationInfo.applicationVersion = Application::kApplicationVersionNo;
        ApplicationInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

        bResult = ShaderLibrary::Initialise();

        bResult = ForwardRenderer::Initialise(ApplicationInfo);
    }

    return bResult;
}

static bool const Run()
{
    bool bResult = true;

    Application::State.bRunning = true;

    while (Application::State.bRunning)
    {
        MSG CurrentMessage = {};
        while (::PeekMessage(&CurrentMessage, nullptr, 0u, 0u, PM_REMOVE))
        {
            ::TranslateMessage(&CurrentMessage);
            ::DispatchMessage(&CurrentMessage);
        }

        ForwardRenderer::Render();
    }

    ShaderLibrary::Destroy();

    ForwardRenderer::Shutdown();

    bResult = VulkanModule::Stop();

    return bResult;
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
    INT Result = EXIT_SUCCESS;

    if (Initialise())
    {
        if (!Run())
        {
            Result = EXIT_FAILURE;
        }
    }
    else
    {
        ::MessageBox(nullptr, TEXT("Failed to initialise application"), TEXT("Fatal Error"), MB_OK);
        Result = EXIT_FAILURE;
    }

    return Result;
}
