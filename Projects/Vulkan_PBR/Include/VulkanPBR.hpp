#pragma once

#include "Common.hpp"
#include "CommonTypes.hpp"

#include "Platform/Windows.hpp"

namespace Application
{
    struct ApplicationState
    {
        Platform::Windows::Handle ProcessHandle;
        Platform::Windows::Handle WindowHandle;

        uint32 CurrentWindowWidth;
        uint32 CurrentWindowHeight;

        bool bRunning;
    };

    static Platform::Windows::TCHAR const * kWindowClassName = PBR_TEXT("Vulkan-PBR");
    static Platform::Windows::TCHAR const * kWindowName = PBR_TEXT("PBR Window");

    static uint32 const kApplicationVersionNo = 0u;

    static uint32 const kDefaultWindowWidth = 1280u;
    static uint32 const kDefaultWindowHeight = 720u;

    extern ApplicationState State;
}
