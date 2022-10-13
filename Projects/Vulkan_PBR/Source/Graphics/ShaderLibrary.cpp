#include "Graphics/ShaderLibrary.hpp"

#include "ShaderCompiler/ShaderCompiler.hpp"

#include "Graphics/Device.hpp"

#include <vector>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace Platform::Windows;
using namespace Platform::Windows::Types;

struct ShaderStatus
{
    bool bCompiled : 1;
    bool bPendingCompilation : 1;
};

struct ShaderData
{
    uint64 ByteCodeSizeInBytes;
    unsigned int * ByteCode;
};

struct ShaderCollection
{
    std::unordered_map<std::string, uint16> ShaderPathToIndex = {};

    std::unordered_map<uint16, std::future<ShaderData>> PendingShaders = {};

    std::vector<std::unique_ptr<unsigned int[]>> ShaderDatas = {};
    std::vector<VkShaderModule> ShaderModules = {};
    std::vector<uint64> ShaderDataSizesInBytes = {};

    std::vector<ShaderStatus> ShaderStatusFlags = {};

    uint16 ShaderCount;
};

static std::filesystem::path const kShaderDirectoryPath = std::filesystem::current_path() / "Shaders";

static ShaderCollection Shaders;

namespace ShaderCompilationThread
{
    struct CompileTaskParameters
    {
        std::filesystem::path ShaderFilePath;
    };

    using CompileTask = std::packaged_task<ShaderData(CompileTaskParameters const &)>;

    static std::wstring const kThreadName = L"Shader Compilation Thread";

    static std::thread Thread;

    static std::atomic_bool bStopThread;

    static std::mutex WorkQueueMutex;
    static std::condition_variable WorkCondition;

    static std::queue<CompileTaskParameters> TaskParameterQueue;
    static std::queue<CompileTask> TaskQueue;

    static void Main()
    {
        while (!bStopThread)
        {
            CompileTaskParameters CurrentTaskParameters;
            CompileTask CurrentTask;

            {
                std::unique_lock QueueLock = std::unique_lock(WorkQueueMutex);

                WorkCondition.wait(QueueLock,
                                   []()
                                   {
                                       return TaskQueue.size() > 0u || bStopThread;
                                   });

                if (TaskQueue.size() > 0u)
                {
                    CurrentTaskParameters = std::move(TaskParameterQueue.front());
                    CurrentTask = std::move(TaskQueue.front());

                    TaskParameterQueue.pop();
                    TaskQueue.pop();
                }
            }

            if (CurrentTask.valid())
            {
                CurrentTask(CurrentTaskParameters);
            }
        }
    }

    static void Start()
    {
        bStopThread = false;
        Thread = std::thread(Main);

        SetThreadDescription(Thread.native_handle(), kThreadName.c_str());
    }

    static void Stop()
    {
        bStopThread = true;
        WorkCondition.notify_one();

        Thread.join();
    }
}

static ShaderData CompileShader(std::filesystem::path const FilePath)
{
    ShaderData CompiledOutput = {};

    bool bShaderCompiledSuccessfully = false;

    while (!bShaderCompiledSuccessfully)
    {
        std::basic_string<Char> ErrorMessage = {};
        bShaderCompiledSuccessfully = ShaderCompiler::CompileShader(FilePath, CompiledOutput.ByteCode, CompiledOutput.ByteCodeSizeInBytes, ErrorMessage);

        if (!bShaderCompiledSuccessfully)
        {
            std::basic_string<Char> ErrorOutput = PBR_TEXT("");
            ErrorOutput += PBR_TEXT("Error Compiling: ");
            ErrorOutput += FilePath.generic_string();
            ErrorOutput += PBR_TEXT("\n");
            ErrorOutput += ErrorMessage;
            ErrorOutput += PBR_TEXT("\n");
            ErrorOutput += PBR_TEXT("Would you like to retry?");

            MessageBoxResults MessageBoxResult = MessageBox(MessageBoxTypes::YesNo, ErrorOutput.c_str(), PBR_TEXT("Shader Compile Error"));

            if (MessageBoxResult == MessageBoxResults::No)
            {
                break;
            }
        }
    }

    return CompiledOutput;
}

static bool const WaitForShaderToLoad(uint16 const ShaderIndex)
{
    ShaderData CompilerOutput = Shaders.PendingShaders [ShaderIndex].get();

    Shaders.ShaderDatas [ShaderIndex].reset(CompilerOutput.ByteCode);
    Shaders.ShaderDataSizesInBytes [ShaderIndex] = CompilerOutput.ByteCodeSizeInBytes;

    Shaders.PendingShaders.erase(ShaderIndex);

    return CompilerOutput.ByteCode != nullptr;
}

bool const ShaderLibrary::Initialise()
{
    bool bResult = ShaderCompiler::Initialise();

    ShaderCompilationThread::Start();

    return bResult;
}

void ShaderLibrary::Destroy()
{
    ShaderCompilationThread::Stop();

    ShaderCompiler::Destroy();
}

bool const ShaderLibrary::LoadShader(std::filesystem::path const FilePath, uint16 & OutputShaderHandle)
{
    bool bResult = false;
    uint16 NewShaderHandle = {};

    std::filesystem::path const ShaderFilePath = kShaderDirectoryPath / FilePath;
    std::string const ShaderFileName = FilePath.filename().string();

    auto FoundShaderIndex = Shaders.ShaderPathToIndex.find(ShaderFileName);

    if (FoundShaderIndex != Shaders.ShaderPathToIndex.end())
    {
        NewShaderHandle = FoundShaderIndex->second + 1u;
    }
    else
    {
        if (std::filesystem::exists(ShaderFilePath))
        {
            Shaders.ShaderDatas.emplace_back();
            Shaders.ShaderDataSizesInBytes.emplace_back();
            Shaders.ShaderStatusFlags.emplace_back();
            Shaders.ShaderModules.emplace_back(VK_NULL_HANDLE);

            NewShaderHandle = static_cast<uint16>(Shaders.ShaderModules.size());

            uint16 const NewShaderIndex = { NewShaderHandle - 1u };
            Shaders.ShaderPathToIndex [ShaderFileName] = NewShaderIndex;

            ShaderCompilationThread::CompileTask NewTask = ShaderCompilationThread::CompileTask(
                [](ShaderCompilationThread::CompileTaskParameters const & Parameters)
                {
                    return ::CompileShader(Parameters.ShaderFilePath);
                });

            Shaders.PendingShaders [NewShaderIndex] = NewTask.get_future();
            Shaders.ShaderStatusFlags [NewShaderIndex].bPendingCompilation = true;

            {
                std::scoped_lock QueueLock = std::scoped_lock(ShaderCompilationThread::WorkQueueMutex);

                ShaderCompilationThread::TaskParameterQueue.emplace(std::move(ShaderCompilationThread::CompileTaskParameters { std::move(ShaderFilePath) }));
                ShaderCompilationThread::TaskQueue.emplace(std::move(NewTask));
            }

            ShaderCompilationThread::WorkCondition.notify_one();
            
            bResult = true;
        }
    }

    OutputShaderHandle = NewShaderHandle;

    return bResult;
}

bool const ShaderLibrary::CreateShaderModule(Vulkan::Device::DeviceState const & DeviceState, uint16 const ShaderHandle, VkShaderModule & OutputShaderModule)
{
    if (ShaderHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create shader module for NULL shader."));
        return false;
    }

    bool bResult = true;

    uint16 const ShaderIndex = ShaderHandle - 1u;

    ShaderStatus const & Status = Shaders.ShaderStatusFlags [ShaderIndex];

    if (Status.bPendingCompilation)
    {
        if (!::WaitForShaderToLoad(ShaderIndex))
        {
            /* TODO: Log shader name */
            Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to compile shader"));
            bResult = false;
        }
        else
        {
            VkShaderModuleCreateInfo const CreateInfo =
            {
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                nullptr,
                0u,
                Shaders.ShaderDataSizesInBytes [ShaderIndex],
                Shaders.ShaderDatas [ShaderIndex].get(),
            };

            VERIFY_VKRESULT(vkCreateShaderModule(DeviceState.Device, &CreateInfo, nullptr, &Shaders.ShaderModules [ShaderIndex]));
         
            /* We don't really need the bytecode after this */
            Shaders.ShaderDatas [ShaderIndex].reset();

            Shaders.ShaderStatusFlags [ShaderIndex].bCompiled = true;
            Shaders.ShaderStatusFlags [ShaderIndex].bPendingCompilation = false;
        }
    }

    if (Status.bCompiled)
    {
        OutputShaderModule = Shaders.ShaderModules [ShaderIndex];
    }

    return bResult;
}

void ShaderLibrary::DestroyShaderModules(Vulkan::Device::DeviceState const & DeviceState)
{
    for (VkShaderModule & ShaderModule : Shaders.ShaderModules)
    {
        if (ShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(DeviceState.Device, ShaderModule, nullptr);
            ShaderModule = VK_NULL_HANDLE;
        }
    }
}