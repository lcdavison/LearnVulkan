#include "Graphics/ShaderLibrary.hpp"

#include "ShaderCompiler/ShaderCompiler.hpp"

#include "Graphics/Device.hpp"

#include <vector>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace Platform;

struct ShaderData
{
    uint64 ByteCodeSizeInBytes;
    unsigned int * ByteCode;
};

struct ShaderCollection
{
    std::unordered_map<std::string, uint16> ShaderIndices;

    std::unordered_map<uint16, std::future<ShaderData>> PendingShaderMap;

    std::vector<VkShaderModule> ShaderModules;
    std::vector<std::unique_ptr<unsigned int []>> ByteCodes;
    std::vector<uint64> ByteCodeSizesInBytes;

    std::vector<bool> ReadyShaders;

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

        Windows::SetThreadDescription(Thread.native_handle(), kThreadName.c_str());
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
        std::basic_string<Windows::TCHAR> ErrorMessage = {};
        bShaderCompiledSuccessfully = ShaderCompiler::CompileShader(FilePath, CompiledOutput.ByteCode, CompiledOutput.ByteCodeSizeInBytes, ErrorMessage);

        if (!bShaderCompiledSuccessfully)
        {
            std::basic_string<Windows::TCHAR> ErrorOutput = PBR_TEXT("");
            ErrorOutput += PBR_TEXT("Error Compiling: ");
            ErrorOutput += FilePath.generic_string();
            ErrorOutput += PBR_TEXT("\n");
            ErrorOutput += ErrorMessage;
            ErrorOutput += PBR_TEXT("\n");
            ErrorOutput += PBR_TEXT("Would you like to retry?");

            Windows::MessageBoxResults MessageBoxResult = Windows::MessageBox(Windows::MessageBoxTypes::YesNo, ErrorOutput.c_str(), PBR_TEXT("Shader Compile Error"));

            if (MessageBoxResult == Windows::MessageBoxResults::No)
            {
                break;
            }
        }
    }

    return CompiledOutput;
}

static bool const WaitForShaderToLoad(uint16 const ShaderIndex)
{
    if (!Shaders.ReadyShaders [ShaderIndex])
    {
        ShaderData CompilerOutput = Shaders.PendingShaderMap [ShaderIndex].get();

        Shaders.ByteCodes [ShaderIndex].reset(CompilerOutput.ByteCode);
        Shaders.ByteCodeSizesInBytes [ShaderIndex] = CompilerOutput.ByteCodeSizeInBytes;

        Shaders.PendingShaderMap.erase(ShaderIndex);

        Shaders.ReadyShaders [ShaderIndex] = CompilerOutput.ByteCode != nullptr;
    }

    return Shaders.ReadyShaders [ShaderIndex];
}

bool const ShaderLibrary::Initialise()
{
    bool bResult = ShaderCompiler::Initialise();

    Shaders.ByteCodes.reserve(16u);
    Shaders.ByteCodeSizesInBytes.reserve(16u);

    ShaderCompilationThread::Start();

    return bResult;
}

void ShaderLibrary::Destroy()
{
    ShaderCompilationThread::Stop();

    ShaderCompiler::Destroy();
}

bool const ShaderLibrary::LoadShader(std::filesystem::path const FilePath, uint16 & OutputShaderIndex)
{
    bool bResult = false;
    uint16 NewShaderIndex = {};

    std::filesystem::path const ShaderFilePath = kShaderDirectoryPath / FilePath;
    std::string const ShaderFileName = FilePath.filename().string();

    /* Lets not duplicate any shader data */
    auto FoundShaderIndex = Shaders.ShaderIndices.find(ShaderFileName);

    if (FoundShaderIndex != Shaders.ShaderIndices.end())
    {
        NewShaderIndex = FoundShaderIndex->second;
    }
    else
    {
        if (std::filesystem::exists(ShaderFilePath))
        {
            NewShaderIndex = { Shaders.ShaderCount };
            Shaders.ShaderCount++;

            Shaders.ByteCodes.emplace_back();
            Shaders.ByteCodeSizesInBytes.emplace_back();
            Shaders.ReadyShaders.emplace_back(false);
            Shaders.ShaderModules.emplace_back(VK_NULL_HANDLE);
            Shaders.ShaderIndices [ShaderFileName] = NewShaderIndex;

            ShaderCompilationThread::CompileTask NewTask = ShaderCompilationThread::CompileTask(
                [](ShaderCompilationThread::CompileTaskParameters const & Parameters)
                {
                    return ::CompileShader(Parameters.ShaderFilePath);
                });

            Shaders.PendingShaderMap [NewShaderIndex] = NewTask.get_future();

            {
                std::scoped_lock QueueLock = std::scoped_lock(ShaderCompilationThread::WorkQueueMutex);

                ShaderCompilationThread::TaskParameterQueue.emplace(ShaderCompilationThread::CompileTaskParameters { std::move(ShaderFilePath) });
                ShaderCompilationThread::TaskQueue.emplace(std::move(NewTask));
            }

            ShaderCompilationThread::WorkCondition.notify_one();
            
            bResult = true;
        }
    }

    OutputShaderIndex = NewShaderIndex;

    return bResult;
}

bool const ShaderLibrary::CreateShaderModule(Vulkan::Device::DeviceState const & DeviceState, uint16 const ShaderIndex, VkShaderModule & OutputShaderModule)
{
    VkShaderModule & ShaderModule = Shaders.ShaderModules [ShaderIndex];

    if (ShaderModule == VK_NULL_HANDLE)
    {
        ::WaitForShaderToLoad(ShaderIndex);

        PBR_ASSERT(Shaders.ByteCodes [ShaderIndex] != nullptr)

        VkShaderModuleCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        CreateInfo.codeSize = Shaders.ByteCodeSizesInBytes [ShaderIndex];
        CreateInfo.pCode = Shaders.ByteCodes [ShaderIndex].get();

        VERIFY_VKRESULT(vkCreateShaderModule(DeviceState.Device, &CreateInfo, nullptr, &ShaderModule));

        /* We don't really need the bytecode after this */
        Shaders.ByteCodes [ShaderIndex].reset();
    }

    OutputShaderModule = ShaderModule;

    return ShaderModule != VK_NULL_HANDLE;
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