#include <vulkan/vulkan.h>

namespace VulkanFunctions
{
#define VK_FUNCTION_DECLARATION(Function)\
    extern PFN_##Function Function

#define VK_EXPORTED_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_GLOBAL_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_INSTANCE_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_DEVICE_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#include "VulkanFunctions.inl"

#undef VK_FUNCTION_DECLARATION
}

namespace VulkanModule
{
    extern bool const Load();

    extern bool const LoadGlobalFunctions();
}