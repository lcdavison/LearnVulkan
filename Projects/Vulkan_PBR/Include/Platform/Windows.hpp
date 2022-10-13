#pragma once

/* The reason the Windows API is abstracted here, is to prevent collisions with the names of macros. */
/* These were causing me a number of headaches. It also creates a wall between the application and the Win32 API */

namespace Platform::Windows::Types
{
    using Int = int;
    using UInt = unsigned int;

    using Byte = unsigned char;
    using Word = unsigned short;
    using DWord = unsigned long;

    using Int16 = short;
    using Int32 = long;
    using Int64 = long long;

    using UInt16 = unsigned short;
    using UInt32 = unsigned long;
    using UInt64 = unsigned long long;

    using Handle = void *;

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

    using Char = CharType<USE_UNICODE>::Type;
}

namespace Platform::Windows
{
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

    extern void SetThreadDescription(Types::Handle Thread, wchar_t const * Description);

    extern MessageBoxResults MessageBox(MessageBoxTypes Type, Types::Char const * Caption, Types::Char const * Title);

    extern void PostQuitMessage(int const ExitCode);

    extern void OutputDebugString(Types::Char const * Message);
}