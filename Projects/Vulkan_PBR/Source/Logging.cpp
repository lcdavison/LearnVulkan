#include "Logging.hpp"

#include "Common.hpp"

using namespace Platform;

void Logging::DebugLog(Windows::TCHAR const * Message)
{
    Windows::OutputDebugString(Message);
}

void Logging::FatalError(Windows::TCHAR const * Message)
{
    Windows::MessageBox(Windows::MessageBoxTypes::Ok, Message, PBR_TEXT("Fatal Error"));
}