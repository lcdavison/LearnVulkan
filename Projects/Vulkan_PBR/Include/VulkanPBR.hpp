#pragma once

#include "Common.hpp"
#include "CommonTypes.hpp"

#include "Platform/Windows.hpp"

namespace Application
{
    struct ApplicationState
    {
        Platform::Windows::Types::Handle ProcessHandle;
        Platform::Windows::Types::Handle WindowHandle;

        uint32 CurrentWindowWidth;
        uint32 CurrentWindowHeight;

        bool bMinimised = {};
        bool bRunning = {};
    };

    static Platform::Windows::Types::Char const * kWindowClassName = PBR_TEXT("Vulkan-PBR");
    static Platform::Windows::Types::Char const * kWindowName = PBR_TEXT("PBR Window");

    static uint32 const kApplicationVersionNo = 0u;

    extern ApplicationState State;
}
