#include "Vulkan.hpp"

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

#include "VulkanFunctions.inl"

#undef VK_FUNCTION_DEFINITION
}