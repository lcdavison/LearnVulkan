#include "Platform/Windows.hpp"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#undef MessageBox
#undef OutputDebugString

using namespace Platform;
using namespace Platform::Windows::Types;

/* This might be overkill, but the name collisions from the Win32 macros were becoming a pain */

template<typename TFunction1, typename TFunction2, typename ... TArguments>
auto CallWindowsFunction(TFunction1 Function1, TFunction2, TArguments && ... Arguments) -> decltype(Function1(Arguments...))
{
    return Function1(Arguments...);
}

template<typename TFunction1, typename TFunction2, typename ... TArguments>
auto CallWindowsFunction(TFunction1, TFunction2 Function2, TArguments && ... Arguments) -> decltype(Function2(Arguments...))
{
    return Function2(Arguments...);
}

void Windows::SetThreadDescription(Handle Thread, wchar_t const * Description)
{
    ::SetThreadDescription(Thread, Description);
}

Windows::MessageBoxResults Windows::MessageBox(MessageBoxTypes Type, Char const * Caption, Char const * Title)
{
    switch (Type)
    {
        default:
        case MessageBoxTypes::Ok:
        {
            ::CallWindowsFunction(::MessageBoxA, ::MessageBoxW, nullptr, Caption, Title, MB_OK | MB_SYSTEMMODAL);
            return Windows::MessageBoxResults::Ok;
        }
        case MessageBoxTypes::YesNo:
        {
            HRESULT Result = ::CallWindowsFunction(::MessageBoxA, ::MessageBoxW, nullptr, Caption, Title, MB_YESNO | MB_SYSTEMMODAL);
            return Result == IDYES ? MessageBoxResults::Yes : MessageBoxResults::No;
        }
    }
}

Handle Platform::Windows::GetCurrentProcess()
{
    return ::GetCurrentProcess();
}

bool Platform::Windows::TerminateProcess(Handle Process, UInt ExitCode)
{
    return ::TerminateProcess(Process, ExitCode) == TRUE;
}

void Platform::Windows::PostQuitMessage(int const ExitCode)
{
    ::PostQuitMessage(ExitCode);
}

void Platform::Windows::OutputDebugString(TCHAR const * Message)
{
    ::CallWindowsFunction(::OutputDebugStringA, ::OutputDebugStringW, Message);
}