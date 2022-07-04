#include "VulkanPBR.hpp"

#include "Graphics/VulkanModule.hpp"
#include "Graphics/ShaderLibrary.hpp"
#include "ForwardRenderer.hpp"

#include "AssetManager.hpp"
#include "Scene.hpp"
#include "Components/StaticMeshComponent.hpp"

#include <Windows.h>
#include <Windowsx.h>
#include <thread>

#include "InputManager.hpp"

extern Application::ApplicationState Application::State = {};

static Scene::SceneData PBRScene = {};

static LRESULT CALLBACK WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
        case WM_MOUSEMOVE:
        {
            Input::State.CurrentMouseX = GET_X_LPARAM(LParam);
            Input::State.CurrentMouseY = GET_Y_LPARAM(LParam);
        }
        break;
        case WM_KEYDOWN:
        {
            PBR_ASSERT(WParam < 256);

            Input::State.CurrentKeyboardState [WParam] = Input::KeyStates::Down;
        }
        break;
        case WM_KEYUP:
        {
            PBR_ASSERT(WParam < 256);

            Input::State.CurrentKeyboardState [WParam] = Input::KeyStates::Up;
        }
        break;
        case WM_SIZE:
        {
            if (WParam == SIZE_MINIMIZED)
            {
                Application::State.bMinimised = true;
            }
            else
            {
                Application::State.bMinimised = false;

                Application::State.CurrentWindowWidth = LOWORD(LParam);
                Application::State.CurrentWindowHeight = HIWORD(LParam);
            }
        }
        break;
        case WM_DESTROY:
        {
            ::PostQuitMessage(EXIT_SUCCESS);
        }
        return 0;
        default:
            return ::DefWindowProc(Window, Message, WParam, LParam);
    }

    return ::DefWindowProc(Window, Message, WParam, LParam);
}

static bool const Initialise()
{
    bool bResult = false;

    if (!Logging::Initialise())
    {
        return false;
    }

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

    ::ShowWindow(reinterpret_cast<HWND>(Application::State.WindowHandle), TRUE);
    ::UpdateWindow(reinterpret_cast<HWND>(Application::State.WindowHandle));

    Application::State.ProcessHandle = CurrentInstance;

    if (VulkanModule::Start())
    {
        VkApplicationInfo ApplicationInfo = {};
        ApplicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
        ApplicationInfo.pApplicationName = "Vulkan PBR";
        ApplicationInfo.applicationVersion = Application::kApplicationVersionNo;
        ApplicationInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

        bResult = ShaderLibrary::Initialise();
        AssetManager::Initialise();

        AssetManager::AssetHandle<AssetManager::MeshAsset> CubeHandle = {};
        AssetManager::LoadMeshAsset("Cube", CubeHandle);

        AssetManager::AssetHandle<AssetManager::MeshAsset> BoatHandle = {};
        bool bFoundBoat = AssetManager::LoadMeshAsset("Boat", BoatHandle);

        if (!bFoundBoat)
        {
            Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to load boat model."));
        }
        else
        {
            Scene::ActorData BoatActorData = { "Boat" };

            uint32 BoatActorHandle = {};
            Scene::CreateActor(PBRScene, BoatActorData, BoatActorHandle);

            Components::StaticMesh::CreateComponent(BoatActorHandle, BoatHandle);

            /* Temporary */
            PBRScene.ComponentMasks [BoatActorHandle - 1u] |= static_cast<uint32>(Scene::ComponentMasks::StaticMesh);
        }

        Scene::ActorData CubeActorData = { "Cube" };

        uint32 CubeActorHandle = {};
        Scene::CreateActor(PBRScene, CubeActorData, CubeActorHandle);
        Components::StaticMesh::CreateComponent(CubeActorHandle, CubeHandle);

        //PBRScene.ComponentMasks [CubeActorHandle - 1u] |= static_cast<uint32>(Scene::ComponentMasks::StaticMesh);

        bResult = ForwardRenderer::Initialise(ApplicationInfo);
    }

    return bResult;
}

static bool const Destroy()
{
    AssetManager::Destroy();

    ShaderLibrary::Destroy();

    bool bResult = ForwardRenderer::Shutdown() && VulkanModule::Stop();

    Logging::Destroy();

    return bResult;
}

static bool const ProcessWindowMessages()
{
    bool bResult = true;

    MSG CurrentMessage = {};

    while (::PeekMessage(&CurrentMessage, nullptr, 0u, 0u, PM_REMOVE))
    {
        if (CurrentMessage.message == WM_QUIT)
        {
            bResult = false;
            break;
        }

        ::TranslateMessage(&CurrentMessage);
        ::DispatchMessage(&CurrentMessage);
    }

    return bResult;
}

static bool const Run()
{
    bool bResult = true;

    std::chrono::high_resolution_clock::time_point PreviousFrameBeginTime = { std::chrono::high_resolution_clock::now() };

    uint64 AccumulatedFrameTimeInMicroSeconds = {};
    uint32 const FrameDeltaTimeInMicroSeconds = { 1000000u / 60u };

    for (;;)
    {
        std::chrono::high_resolution_clock::time_point CurrentFrameBeginTime = { std::chrono::high_resolution_clock::now() };
        uint64 const FrameDuration = std::chrono::duration_cast<std::chrono::duration<uint64, std::micro>>(CurrentFrameBeginTime - PreviousFrameBeginTime).count();
        AccumulatedFrameTimeInMicroSeconds += FrameDuration;

        PreviousFrameBeginTime = CurrentFrameBeginTime;

        Input::UpdateInputState();

        if (::ProcessWindowMessages())
        {
            if (!Application::State.bMinimised)
            {
                /* Could do some actor culling and stuff here */

                if (AccumulatedFrameTimeInMicroSeconds >= FrameDeltaTimeInMicroSeconds)
                {
                    constexpr float DeltaTimeInSeconds = static_cast<float>(FrameDeltaTimeInMicroSeconds) / 1000000.0f;

                    Camera::UpdateOrientation(PBRScene.MainCamera,
                                              -1.0f * (Input::State.CurrentMouseY - Input::State.PreviousMouseY) * Input::State.MouseSensitivity * DeltaTimeInSeconds,
                                              -1.0f * (Input::State.CurrentMouseX - Input::State.PreviousMouseX) * Input::State.MouseSensitivity * DeltaTimeInSeconds);

                    /* Branchless WASD */
                    constexpr float kMovementSpeed = 8.0f * DeltaTimeInSeconds;
                    float const ForwardSpeed = { kMovementSpeed * (Input::IsKeyDown(0x53) * -1.0f + Input::IsKeyDown(0x57)) };
                    float const StrafeSpeed = { kMovementSpeed * (Input::IsKeyDown(0x41) * -1.0f + Input::IsKeyDown(0x44)) };

                    Camera::UpdatePosition(PBRScene.MainCamera, ForwardSpeed, StrafeSpeed);

                    AccumulatedFrameTimeInMicroSeconds -= FrameDeltaTimeInMicroSeconds;
                }

                ForwardRenderer::CreateNewActorResources(PBRScene);

                ForwardRenderer::Render(PBRScene);

                if (PBRScene.NewActorHandles.size() > 0u)
                {
                    PBRScene.NewActorHandles.clear();
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::duration<uint32, std::milli>(1u));
            }
        }
        else
        {
            break;
        }
    }

    bResult = ::Destroy();

    return bResult;
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
    INT ExitCode = EXIT_SUCCESS;

    if (Initialise())
    {
        if (!Run())
        {
            ExitCode = EXIT_FAILURE;
        }
    }
    else
    {
        ::MessageBox(nullptr, TEXT("Failed to initialise application"), TEXT("Fatal Error"), MB_OK);
        ExitCode = EXIT_FAILURE;
    }

    return ExitCode;
}
