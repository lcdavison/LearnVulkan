#include "Graphics/ShaderLibrary.hpp"

#include "ShaderCompiler/ShaderCompiler.hpp"

#include "Graphics/Device.hpp"

#include <vector>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>

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

static ShaderCollection Shaders;

namespace ShaderCompilationThread
{
    static std::wstring const kThreadName = L"Shader Compilation Thread";

    static std::thread Thread;

    static std::atomic_bool bStopThread;

    static std::mutex WorkQueueMutex;
    static std::condition_variable WorkCondition;
    static std::queue<std::packaged_task<ShaderData(void)>> WorkQueue;

    static void Main()
    {
        while (!bStopThread)
        {
            std::packaged_task<ShaderData(void)> CurrentTask;

            {
                std::unique_lock QueueLock = std::unique_lock(WorkQueueMutex);

                WorkCondition.wait(QueueLock,
                                   []()
                                   {
                                       return WorkQueue.size() > 0u || bStopThread;
                                   });

                if (WorkQueue.size() > 0u)
                {
                    CurrentTask = std::move(WorkQueue.front());
                    WorkQueue.pop();
                }
            }

            if (CurrentTask.valid())
            {
                CurrentTask();
            }
        }
    }

    static void Start()
    {
        bStopThread = false;
        Thread = std::thread(Main);

        ::SetThreadDescription(Thread.native_handle(), kThreadName.c_str());
    }

    static void Stop()
    {
        bStopThread = true;
        WorkCondition.notify_one();

        Thread.join();
    }
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

static ShaderData CompileShader(std::filesystem::path const FilePath)
{
    ShaderData CompiledOutput = {};

    bool bShaderCompiledSuccessfully = false;

    while (!bShaderCompiledSuccessfully)
    {
        std::basic_string<TCHAR> ErrorMessage = {};
        bShaderCompiledSuccessfully = ShaderCompiler::CompileShader(FilePath, CompiledOutput.ByteCode, CompiledOutput.ByteCodeSizeInBytes, ErrorMessage);

        if (!bShaderCompiledSuccessfully)
        {
            std::basic_stringstream ErrorOutput = std::basic_stringstream<TCHAR>();
            ErrorOutput << TEXT("Error Compiling: ") << FilePath.generic_string() << TEXT("\n");
            ErrorOutput << ErrorMessage << TEXT("\n");
            ErrorOutput << TEXT("Would you like to retry?");

            int32 MessageBoxResult = ::MessageBox(nullptr, ErrorOutput.str().c_str(), TEXT("Compile Error"), MB_YESNO | MB_ICONEXCLAMATION);

            if (MessageBoxResult == IDNO)
            {
                break;
            }
        }
    }

    return CompiledOutput;
}

uint16 ShaderLibrary::LoadShader(std::filesystem::path const FilePath)
{
    uint16 NewShaderIndex = {};

    /* Lets not duplicate any shader data */
    auto FoundShaderIndex = Shaders.ShaderIndices.find(FilePath.filename().string());

    if (FoundShaderIndex != Shaders.ShaderIndices.end())
    {
        NewShaderIndex = FoundShaderIndex->second;
    }
    else
    {
        NewShaderIndex = { Shaders.ShaderCount };
        Shaders.ShaderCount++;

        Shaders.ByteCodes.emplace_back();
        Shaders.ByteCodeSizesInBytes.emplace_back();
        Shaders.ReadyShaders.emplace_back(false);
        Shaders.ShaderModules.emplace_back(VK_NULL_HANDLE);
        Shaders.ShaderIndices [FilePath.filename().string()] = NewShaderIndex;

        {
            std::scoped_lock QueueLock = std::scoped_lock(ShaderCompilationThread::WorkQueueMutex);

            std::packaged_task NewTask = std::packaged_task(
                [FilePath]()
                {
                    return ::CompileShader(FilePath);
                });

            Shaders.PendingShaderMap [NewShaderIndex] = NewTask.get_future();

            ShaderCompilationThread::WorkQueue.emplace(std::move(NewTask));
        }

        ShaderCompilationThread::WorkCondition.notify_one();
    }

    return NewShaderIndex;
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

VkShaderModule ShaderLibrary::CreateShaderModule(Vulkan::Device::DeviceState const & DeviceState, uint16 const ShaderIndex)
{
    VkShaderModule & ShaderModule = Shaders.ShaderModules [ShaderIndex];

    if (ShaderModule == VK_NULL_HANDLE)
    {
        if (WaitForShaderToLoad(ShaderIndex))
        {
            VkShaderModuleCreateInfo CreateInfo = {};
            CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            CreateInfo.codeSize = Shaders.ByteCodeSizesInBytes [ShaderIndex];
            CreateInfo.pCode = Shaders.ByteCodes [ShaderIndex].get();

            VERIFY_VKRESULT(vkCreateShaderModule(DeviceState.Device, &CreateInfo, nullptr, &ShaderModule));
        }
    }

    return ShaderModule;
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