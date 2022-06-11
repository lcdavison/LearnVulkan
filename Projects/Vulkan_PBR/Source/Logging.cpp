#include "Logging.hpp"

#include "Common.hpp"

#include <thread>
#include <atomic>
#include <string>
#include <future>
#include <array>
#include <queue>
#include <fstream>
#include <cstdarg>

using namespace Platform;

namespace LoggingThread
{
    struct LogMessageData
    {
        String Content;
        Logging::LogTypes Type;
    };

    static std::wstring const kThreadName = L"Logging Thread";

    static std::array<String, static_cast<uint32>(Logging::LogTypes::TypeCount)> const LogHeaderTable =
    {
        PBR_TEXT("Info"),
        PBR_TEXT("Error"),
    };

    static std::queue<LogMessageData> MessageQueue = {};

    static std::mutex MessageQueueMutex = {};
    static std::condition_variable WorkCondition = {};

    static std::atomic_bool bStopThread = { false };
    static std::atomic_bool bEmptyMessageQueue = { true };

    static std::thread Thread = {};

    static void Log(LogMessageData const & Parameters)
    {
        String const & TypeHeader = LogHeaderTable [static_cast<uint32>(Parameters.Type)];

        /* TODO: Add date and time to the log entry */
        /* TODO: Also output to a file */

        String const OutputString = String::Format(PBR_TEXT("[%s]: %s\n"), TypeHeader.Data(), Parameters.Content.Data());
        Windows::OutputDebugString(OutputString.Data());
    }

    static void Main()
    {
        /* TODO: Open a file stream */

        while (!bStopThread)
        {
            LogMessageData Parameters = {};

            {
                std::unique_lock Lock = std::unique_lock(MessageQueueMutex);

                WorkCondition.wait(Lock,
                                   []()
                                   {
                                       return MessageQueue.size() > 0u || bStopThread;
                                   });

                if (MessageQueue.size() > 0u)
                {
                    Parameters = std::move(MessageQueue.front());
                    MessageQueue.pop();
                }

                bEmptyMessageQueue.store(MessageQueue.size() == 0u, std::memory_order::memory_order_release);
            }

            if (Parameters.Content.Length() > 0u)
            {
                Log(Parameters);
            }
        }

        /* TODO: Close filestream */
    }

    static void Start()
    {
        Thread = std::thread(Main);

        Windows::SetThreadDescription(Thread.native_handle(), kThreadName.c_str());
    }

    static void Stop()
    {
        bStopThread = true;
        WorkCondition.notify_one();

        Thread.join();
    }
}

bool const Logging::Initialise()
{
    bool bResult = true;

    LoggingThread::Start();

    return bResult;
}

void Logging::Destroy()
{
    LoggingThread::Stop();
}

void Logging::Log(Logging::LogTypes LogType, String Message)
{
    LoggingThread::LogMessageData Parameters = LoggingThread::LogMessageData { std::move(Message), LogType };

    {
        std::scoped_lock Lock = std::scoped_lock(LoggingThread::MessageQueueMutex);

        LoggingThread::MessageQueue.emplace(std::move(Parameters));

        LoggingThread::bEmptyMessageQueue.store(false, std::memory_order::memory_order_release);
    }

    LoggingThread::WorkCondition.notify_one();
}

void Logging::Flush()
{
    /* This will work, but would be better to do other work in the meantime */
    while (!LoggingThread::bEmptyMessageQueue);
}

void Logging::DebugLog(Windows::TCHAR const * Message)
{
    Windows::OutputDebugString(Message);
}

void Logging::FatalError(Windows::TCHAR const * Message)
{
    Windows::MessageBox(Windows::MessageBoxTypes::Ok, Message, PBR_TEXT("Fatal Error"));
}