#pragma once

#include "CommonTypes.hpp"
#include "Logging.hpp"

#if _DEBUG
    #include <intrin.h>
#endif

#ifdef _UNICODE
    #define PBR_TEXT(Text) L##Text
#else
    #define PBR_TEXT(Text) Text
#endif

#if _DEBUG
#define PBR_ASSERT(Condition) \
    do { \
        if (!(Condition)) { \
            Logging::Log(Logging::LogTypes::Error, String::Format(PBR_TEXT("Assertion Failed: %s"), #Condition)); \
            __debugbreak(); \
        } \
    } while(false)
#else
    /* TODO: Should probably throw an exception or something */
    #define PBR_ASSERT(Condition)
#endif