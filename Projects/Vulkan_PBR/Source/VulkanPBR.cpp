#include "VulkanPBR.hpp"

#include "Graphics/VulkanModule.hpp"
#include "Graphics/ShaderLibrary.hpp"
#include "ForwardRenderer.hpp"

#include "Scene.hpp"

#include "Components/StaticMeshComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MaterialComponent.hpp"

#include "Assets/StaticMesh.hpp"
#include "Assets/Texture.hpp"
#include "Assets/Material.hpp"

#include <Windows.h>
#include <Windowsx.h>
#include <dwmapi.h>
#include <thread>

#include "Input/InputManager.hpp"

extern Application::ApplicationState Application::State = {};

static Scene::SceneData PBRScene = {};

static UINT DPI = {};

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

        Components::StaticMesh::CreateComponent(PBRScene, BoatActorHandle, BoatHandle);
        Components::Transform::CreateComponent(BoatActorHandle, PBRScene);
        Components::Material::CreateComponent(PBRScene, BoatActorHandle, BoatMaterial);

        Math::Vector3 const Position = Math::Vector3 { 0.0f, 100.0f, -50.0f };
        float const Scale = 1.0f;
        Components::Transform::SetTransform(BoatActorHandle, &Position, nullptr, &Scale);
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

    HINSTANCE CurrentInstance = ::GetModuleHandle(nullptr);

    WNDCLASSEX WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.lpszClassName = Application::kWindowClassName;
    WindowClass.hInstance = CurrentInstance;
    WindowClass.lpfnWndProc = &::WindowProcedure;
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    WindowClass.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
    WindowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));

    Application::State.CurrentWindowWidth = static_cast<uint32>(::GetSystemMetrics(SM_CXSCREEN));
    Application::State.CurrentWindowHeight = static_cast<uint32>(::GetSystemMetrics(SM_CYSCREEN));

    if (::RegisterClassEx(&WindowClass))
    {
        Application::State.WindowHandle = ::CreateWindow(Application::kWindowClassName,
                                                         Application::kWindowName,
                                                         WS_OVERLAPPED | WS_POPUP,
                                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                                         Application::State.CurrentWindowWidth, Application::State.CurrentWindowHeight,
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

        bool bSetupScene = ::SetupScene();

        if (bSetupScene)
        {
            DPI = ::GetDpiForWindow(reinterpret_cast<HWND>(Application::State.WindowHandle));

            bResult = ForwardRenderer::Initialise(ApplicationInfo);
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

    uint64 AccumulatedFrameTimeInMicroSeconds = {};
    constexpr uint32 kFixedUpdateTimeInMicroSeconds = { 1000000u / 60u };

    for (;;)
    {
        std::chrono::high_resolution_clock::time_point CurrentFrameBeginTime = { std::chrono::high_resolution_clock::now() };
        uint64 const FrameDuration = std::chrono::duration_cast<std::chrono::duration<uint64, std::micro>>(CurrentFrameBeginTime - PreviousFrameBeginTime).count();
        AccumulatedFrameTimeInMicroSeconds += FrameDuration;

        PreviousFrameBeginTime = CurrentFrameBeginTime;

        Input::UpdateInputState(DPI);

        if (::ProcessWindowMessages())
        {
            if (!Application::State.bMinimised)
            {
                /* Could do some actor culling and stuff here */

                if (AccumulatedFrameTimeInMicroSeconds >= kFixedUpdateTimeInMicroSeconds)
                {
                    constexpr float kFixedUpdateTimeInSeconds = static_cast<float>(kFixedUpdateTimeInMicroSeconds) / 1000000.0f;
                    float const kVariableUpdateTimeInSeconds = static_cast<float>(FrameDuration) / 1000000.0f;

                    Camera::UpdateOrientation(PBRScene.MainCamera,
                                              -1.0f * (Input::State.CurrentMouseY - Input::State.PreviousMouseY) * Input::State.MouseSensitivity * kVariableUpdateTimeInSeconds,
                                              -1.0f * (Input::State.CurrentMouseX - Input::State.PreviousMouseX) * Input::State.MouseSensitivity * kVariableUpdateTimeInSeconds);

                    /* Branchless WASD */
                    constexpr float kMovementSpeed = 32.0f * kFixedUpdateTimeInSeconds;
                    float const kForwardSpeed = { kMovementSpeed * (Input::IsKeyDown(0x53) * -1.0f + Input::IsKeyDown(0x57)) };
                    float const kStrafeSpeed = { kMovementSpeed * (Input::IsKeyDown(0x41) * -1.0f + Input::IsKeyDown(0x44)) };

                    Camera::UpdatePosition(PBRScene.MainCamera, kForwardSpeed, kStrafeSpeed);

                    AccumulatedFrameTimeInMicroSeconds -= kFixedUpdateTimeInMicroSeconds;
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
