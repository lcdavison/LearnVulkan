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

#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_DEVICE_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_DEVICE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DECLARATION(Function)

#include "VulkanFunctions.inl"

#undef VK_FUNCTION_DECLARATION
}

#if defined(_DEBUG)
#include <cstdio>

#define VERIFY_VKRESULT(Function)\
    {\
        VkResult Result = Function;\
        if (Result != VK_SUCCESS)\
        {\
            char ErrorMessage [512] = {};\
            std::sprintf(ErrorMessage, "Function [%s] Failed with Error Code [%d]\nIn File [%s] Line [%d]", #Function, static_cast<unsigned long>(Result), __FILE__, __LINE__);\
        }\
    }
#else
#define VERIFY_VKRESULT(Function)
#endif // _DEBUG

namespace VulkanModule
{
    extern bool const Start(VkApplicationInfo const & ApplicationInfo);

    extern bool const Stop();
}