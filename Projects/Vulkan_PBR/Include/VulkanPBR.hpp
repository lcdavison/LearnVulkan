#pragma once

#include <Windows.h>

#include "CommonTypes.hpp"

namespace Application
{
    struct ApplicationState
    {
        HINSTANCE ProcessHandle;
        HWND WindowHandle;

        uint32 CurrentWindowWidth;
        uint32 CurrentWindowHeight;

        bool bRunning;
    };

    static TCHAR const * kWindowClassName = TEXT("Vulkan-PBR");
    static TCHAR const * kWindowName = TEXT("PBR Window");

    static uint32 const kApplicationVersionNo = 0u;

    static uint32 const kDefaultWindowWidth = 1280u;
    static uint32 const kDefaultWindowHeight = 720u;

    extern ApplicationState State;
}
