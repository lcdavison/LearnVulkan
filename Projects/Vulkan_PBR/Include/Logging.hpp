#pragma once

#include "Platform/Windows.hpp"

namespace Logging
{
    extern void DebugLog(Platform::Windows::TCHAR const * Message);

    extern void FatalError(Platform::Windows::TCHAR const * Message);
}