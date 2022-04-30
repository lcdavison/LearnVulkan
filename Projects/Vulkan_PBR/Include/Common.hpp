#pragma once

#if _DEBUG
    #include <intrin.h>
#endif

#ifdef _UNICODE
    #define PBR_TEXT(String) L##String
#else
    #define PBR_TEXT(String) String
#endif

#if _DEBUG
#define PBR_ASSERT(Condition) \
    if (!(Condition)) { \
        __debugbreak(); \
    }
#else
    /* TODO: Should probably throw an exception or something */
    #define PBR_ASSERT(Condition)
#endif