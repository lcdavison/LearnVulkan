#include "VulkanPBR.hpp"

#include "Assets/Material.hpp"
#include "Assets/StaticMesh.hpp"
#include "Assets/Texture.hpp"
#include "Components/StaticMeshComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "ForwardRenderer.hpp"
#include "Graphics/VulkanModule.hpp"
#include "Graphics/ShaderLibrary.hpp"
#include "Input/InputManager.hpp"
#include "Scene.hpp"

#include <dwmapi.h>
#include <Math/Transform.hpp>
#include <Math/Utilities.hpp>
#include <Windows.h>
#include <Windowsx.h>
#include <dwmapi.h>
#include <thread>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC static_cast<USHORT>(0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE static_cast<USHORT>(0x02)
#endif

extern Application::ApplicationState Application::State = {};

static Scene::SceneData PBRScene = {};

static UINT DPI = {};

static LRESULT CALLBACK WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
        case WM_INPUT:
        {
            RAWINPUT RawInput = {};
            uint32 SizeInBytes = { sizeof(RawInput) };

            ::GetRawInputData(reinterpret_cast<HRAWINPUT>(LParam), RID_INPUT, &RawInput, &SizeInBytes, sizeof(RAWINPUTHEADER));

            if (RawInput.header.dwType == RIM_TYPEMOUSE)
            {
                Input::State.CurrentMouseX += RawInput.data.mouse.lLastX;
                Input::State.CurrentMouseY += RawInput.data.mouse.lLastY;
            }
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

static bool const SetupScene()
{
    static std::filesystem::path const kAssetDirectoryPath = std::filesystem::current_path() / "Assets";

    uint32 BoatHandle = {};
    bool bFoundBoat = Assets::StaticMesh::ImportStaticMesh(kAssetDirectoryPath / "Fishing Boat/Boat.obj", "Boat", BoatHandle);

    if (bFoundBoat)
    {
        std::array<uint32, 5u> TextureHandles = {};

        Assets::Texture::ImportTexture(kAssetDirectoryPath / "Fishing Boat/textures/boat_diffuse.bmp", "Boat Diffuse", TextureHandles[0u]);
        Assets::Texture::ImportTexture(kAssetDirectoryPath / "Fishing Boat/textures/boat_ao.bmp", "Boat AO", TextureHandles [1u]);
        Assets::Texture::ImportTexture(kAssetDirectoryPath / "Fishing Boat/textures/boat_normal.bmp", "Boat Normal", TextureHandles [2u]);
        Assets::Texture::ImportTexture(kAssetDirectoryPath / "Fishing Boat/textures/boat_gloss.bmp", "Boat Gloss", TextureHandles [3u]);
        Assets::Texture::ImportTexture(kAssetDirectoryPath / "Fishing Boat/textures/boat_specular.bmp", "Boat Specular", TextureHandles [4u]);

        Assets::Material::MaterialData const MaterialDesc =
        {
            TextureHandles [0u], TextureHandles [2u],
            TextureHandles [4u], TextureHandles [3u],
            TextureHandles [1u],
        };

        uint32 BoatMaterial = {};
        Assets::Material::CreateMaterial(MaterialDesc, "Boat Material", BoatMaterial);

        Scene::ActorData const BoatActorData = { "Boat" };

        uint32 BoatActorHandle = {};
        Scene::CreateActor(PBRScene, BoatActorData, BoatActorHandle);

        Components::StaticMesh::CreateComponent(PBRScene, BoatActorHandle, BoatHandle, BoatMaterial);
        Components::Transform::CreateComponent(BoatActorHandle, PBRScene);

        Math::Vector3 const kPosition = Math::Vector3 { 0.0f, 100.0f, -50.0f };
        float const kScale = { 1.0f };
        Components::Transform::SetTransform(BoatActorHandle, &kPosition, nullptr, &kScale);

        float const kAspectRatio = static_cast<float>(Application::State.CurrentWindowWidth) / static_cast<float>(Application::State.CurrentWindowHeight);
        float const kNearPlaneDistance = { 10.0f };
        float const kFarPlaneDistance = { 2000.0f };

        PBRScene.MainCamera.ProjectionMatrix = Math::PerspectiveMatrix(Math::ConvertDegreesToRadians(90.0f), kAspectRatio, kNearPlaneDistance, kFarPlaneDistance);
    }

    return bFoundBoat;
}

static bool const Initialise()
{
    bool bResult = false;

    if (!Logging::Initialise())
    {
        return false;
    }

    HINSTANCE const kCurrentInstance = ::GetModuleHandle(nullptr);

    WNDCLASSEX const kWindowClass = 
    {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        &::WindowProcedure,
        0l, 0l,
        kCurrentInstance,
        ::LoadIcon(nullptr, IDI_APPLICATION),
        ::LoadCursor(nullptr, IDC_ARROW),
        static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)),
        nullptr,
        Application::kWindowClassName,
        ::LoadIcon(nullptr, IDI_APPLICATION),
    };

    Application::State.CurrentWindowWidth = static_cast<uint32>(::GetSystemMetrics(SM_CXSCREEN));
    Application::State.CurrentWindowHeight = static_cast<uint32>(::GetSystemMetrics(SM_CYSCREEN));

    if (::RegisterClassEx(&kWindowClass))
    {
        Application::State.WindowHandle = ::CreateWindow(Application::kWindowClassName,
                                                         Application::kWindowName,
                                                         WS_OVERLAPPED | WS_POPUP,
                                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                                         Application::State.CurrentWindowWidth, Application::State.CurrentWindowHeight,
                                                         nullptr, nullptr,
                                                         kCurrentInstance, nullptr);

        bResult = Application::State.WindowHandle != INVALID_HANDLE_VALUE;
    }

    ::ShowWindow(static_cast<HWND>(Application::State.WindowHandle), TRUE);
    ::UpdateWindow(static_cast<HWND>(Application::State.WindowHandle));

    /* this should be done by the input manager */
    RAWINPUTDEVICE const kRawMouseInput =
    {
        HID_USAGE_PAGE_GENERIC,
        HID_USAGE_GENERIC_MOUSE,
        0u,
        static_cast<HWND>(Application::State.WindowHandle),
    };

    if (bResult &= (::RegisterRawInputDevices(&kRawMouseInput, 1u, sizeof(kRawMouseInput)) == TRUE);
        !bResult)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to register raw mouse input"));
        return bResult;
    }

    ::ShowCursor(FALSE);

    Application::State.ProcessHandle = kCurrentInstance;

    if (bResult &= VulkanModule::Start();
        bResult)
    {
        bResult &= ShaderLibrary::Initialise();

        bool bSetupScene = ::SetupScene();

        if (bSetupScene)
        {
            VkApplicationInfo const kApplicationInfo = Vulkan::ApplicationInfo("Vulkan PBR", Application::kApplicationVersionNo, VK_MAKE_API_VERSION(0, 1, 3, 0));

            DPI = ::GetDpiForWindow(static_cast<HWND>(Application::State.WindowHandle));

            bResult &= ForwardRenderer::Initialise(kApplicationInfo);
        }
        else
        {
            Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to setup scene. Please make sure all assets are available."));
        }
    }

    return bResult;
}

static bool const Destroy()
{
    ShaderLibrary::Destroy();

    bool bResult = ForwardRenderer::Shutdown();
    
    VulkanModule::Stop();

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
        else
        {
            ::TranslateMessage(&CurrentMessage);
            ::DispatchMessage(&CurrentMessage);
        }
    }

    return bResult;
}

static bool const Run()
{
    bool bResult = true;

    std::chrono::high_resolution_clock::time_point PreviousFrameBeginTime = { std::chrono::high_resolution_clock::now() };

    uint64 AccumulatedFrameTimeInNanoSeconds = {};
    constexpr uint32 kFixedUpdateTimeInNanoSeconds = { 1000000000u / 60u };

    for (;;)
    {
        std::chrono::high_resolution_clock::time_point CurrentFrameBeginTime = { std::chrono::high_resolution_clock::now() };
        uint64 const FrameDurationInNanoSeconds = std::chrono::duration_cast<std::chrono::duration<uint64, std::nano>>(CurrentFrameBeginTime - PreviousFrameBeginTime).count();
        AccumulatedFrameTimeInNanoSeconds += FrameDurationInNanoSeconds;

        PreviousFrameBeginTime = CurrentFrameBeginTime;

        Input::UpdateInputState(DPI);

        if (::ProcessWindowMessages())
        {
            if (!Application::State.bMinimised)
            {
                /* Could do some actor culling and stuff here */

                if (AccumulatedFrameTimeInNanoSeconds >= kFixedUpdateTimeInNanoSeconds)
                {
                    constexpr float kFixedUpdateTimeInSeconds = static_cast<float>(kFixedUpdateTimeInNanoSeconds) / 1000000000.0f;

                    /* Branchless WASD */
                    constexpr float kMovementSpeed = 64.0f * kFixedUpdateTimeInSeconds;
                    float const kForwardSpeed = { kMovementSpeed * (Input::IsKeyDown(0x53) * -1.0f + Input::IsKeyDown(0x57)) };
                    float const kStrafeSpeed = { kMovementSpeed * (Input::IsKeyDown(0x41) * -1.0f + Input::IsKeyDown(0x44)) };

                    Camera::UpdatePosition(PBRScene.MainCamera, kForwardSpeed, kStrafeSpeed);

                    Camera::UpdateOrientation(PBRScene.MainCamera,
                                              -1.0f * static_cast<float>(Input::State.CurrentMouseY - Input::State.PreviousMouseY) * Input::State.MouseSensitivity * kFixedUpdateTimeInSeconds,
                                              -1.0f * static_cast<float>(Input::State.CurrentMouseX - Input::State.PreviousMouseX) * Input::State.MouseSensitivity * kFixedUpdateTimeInSeconds);

                    AccumulatedFrameTimeInNanoSeconds -= kFixedUpdateTimeInNanoSeconds;
                }

                ForwardRenderer::Render(PBRScene);


                /* Not really necessary for fullscreen, but this helps little bit with microstutter in windowed/borderless */
                ::DwmFlush();
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

    ::Destroy();

    return ExitCode;
}
