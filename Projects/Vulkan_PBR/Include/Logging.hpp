#pragma once

#include "Platform/Windows.hpp"

#include "Utilities/String.hpp"

namespace Logging
{
    enum class LogTypes
    {
        Info,
        Error,
        TypeCount,
    };

    extern bool const Initialise();

    extern void Destroy();

    extern void Log(Logging::LogTypes LogType, String Message);

    extern void Flush();

    extern void DebugLog(Platform::Windows::TCHAR const * Message);

    extern void FatalError(Platform::Windows::TCHAR const * Message);
}