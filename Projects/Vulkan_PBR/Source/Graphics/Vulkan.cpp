#include "Graphics/Vulkan.hpp"

namespace VulkanFunctions
{
#define VK_FUNCTION_DEFINITION(FunctionName)\
    PFN_##FunctionName FunctionName

#define VK_EXPORTED_FUNCTION(FunctionName)\
    VK_FUNCTION_DEFINITION(FunctionName)

#define VK_GLOBAL_FUNCTION(FunctionName)\
    VK_FUNCTION_DEFINITION(FunctionName)

#define VK_INSTANCE_FUNCTION(FunctionName)\
    VK_FUNCTION_DEFINITION(FunctionName)

#define VK_DEVICE_FUNCTION(FunctionName)\
    VK_FUNCTION_DEFINITION(FunctionName)

#include "Graphics/VulkanFunctions.inl"

#undef VK_FUNCTION_DEFINITION
}

#include "Graphics/Instance.hpp"

using namespace Vulkan;

static HMODULE VulkanLibraryHandle = {};

static Vulkan::Instance::InstanceState InstanceState = {};

bool const VulkanModule::Start(VkApplicationInfo const & ApplicationInfo)
{
    bool bResult = false;

    VulkanLibraryHandle = ::LoadLibrary(TEXT("vulkan-1.dll"));
    if (bResult = VulkanLibraryHandle; !bResult)
    {
        ::MessageBox(nullptr, TEXT("Failed to load vulkan module"), TEXT("Fatal Error"), MB_OK);
    }

    LoadExportedFunctions(VulkanLibraryHandle);

    LoadGlobalFunctions();

    bResult = Instance::CreateInstance(ApplicationInfo, InstanceState);

    LoadInstanceFunctions(InstanceState.Instance);

    return bResult;
}

bool const VulkanModule::Stop()
{
    bool bResult = false;
    return bResult;
}