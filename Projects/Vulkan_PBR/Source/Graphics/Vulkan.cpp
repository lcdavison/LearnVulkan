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

#include <Windows.h>

#include "CommonTypes.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"

using namespace Vulkan;

static HMODULE VulkanLibraryHandle = {};

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};

/* The implementation for these functions can be found in MacroMagic.cpp */
/* Separated because the function implementation is a lil bit messy */
extern bool const LoadExportedFunctions(HMODULE);
extern bool const LoadGlobalFunctions();
extern bool const LoadInstanceFunctions(VkInstance);

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

    Device::CreateDevice(InstanceState.Instance, DeviceState);

    return bResult;
}

bool const VulkanModule::Stop()
{
    bool bResult = false;

    Device::DestroyDevice(DeviceState);

    bResult = ::FreeLibrary(VulkanLibraryHandle) != 0u;

    return bResult;
}