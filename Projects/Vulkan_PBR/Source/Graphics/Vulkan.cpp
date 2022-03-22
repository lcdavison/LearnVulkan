#include "Graphics/Vulkan.hpp"

namespace VulkanFunctions
{
#define VK_FUNCTION_DEFINITION(Function)\
    PFN_##Function Function

#define VK_EXPORTED_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_GLOBAL_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_INSTANCE_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_DEVICE_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_DEVICE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DEFINITION(Function)

#include "Graphics/VulkanFunctions.inl"

#undef VK_FUNCTION_DEFINITION
}

#include <Windows.h>

#include "CommonTypes.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"

using namespace Vulkan;

/* The implementation for these functions can be found in MacroMagic.cpp */
/* Separated because the function implementation is a lil bit messy */
extern bool const LoadExportedFunctions(HMODULE);
extern bool const LoadGlobalFunctions();

static HMODULE VulkanLibraryHandle = {};

static Instance::InstanceState InstanceState = {};
static Device::DeviceState DeviceState = {};

bool const VulkanModule::Start(VkApplicationInfo const & ApplicationInfo)
{
    bool bResult = false;

    VulkanLibraryHandle = ::LoadLibrary(TEXT("vulkan-1.dll"));
    if (VulkanLibraryHandle)
    {
        bResult = ::LoadExportedFunctions(VulkanLibraryHandle) &&
                  ::LoadGlobalFunctions() &&
                  Instance::CreateInstance(ApplicationInfo, InstanceState) &&
                  Device::CreateDevice(InstanceState.Instance, DeviceState);
    }
    else
    {
        ::MessageBox(nullptr, TEXT("Failed to load vulkan module"), TEXT("Fatal Error"), MB_OK);
    }

    return bResult;
}

bool const VulkanModule::Stop()
{
    Device::DestroyDevice(DeviceState);

    Instance::DestroyInstance(InstanceState);

    return ::FreeLibrary(VulkanLibraryHandle) != 0u;
}