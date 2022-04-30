#pragma once

/* The reason the Windows API is abstracted here, is to prevent collisions with the names of macros. */
/* These were causing me a number of headaches. It also creates a wall between the application and the Win32 API */

namespace Platform::Windows
{
    template<bool bUseUnicode> struct CharType;

    template<>
    struct CharType<false>
    {
        using Type = char;
    };

    template<>
    struct CharType<true>
    {
        using Type = wchar_t;
    };

    using TCHAR = CharType<USE_UNICODE>::Type;

    using Handle = void *;

    enum class MessageBoxTypes
    {
        Ok,
        YesNo,
    };

    enum class MessageBoxResults
    {
        Ok,
        Yes,
        No,
    };

    extern void SetThreadDescription(Handle Thread, wchar_t const * Description);

    extern MessageBoxResults MessageBox(MessageBoxTypes Type, TCHAR const * Caption, TCHAR const * Title);

    extern void PostQuitMessage(int const ExitCode);

    extern void OutputDebugString(TCHAR const * Message);
}