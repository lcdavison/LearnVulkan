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
    #include <string>

    #if UNICODE
        #define TO_STRING(Value) std::to_wstring(Value)
    #else
        #define TO_STRING(Value) std::to_string(Value)
    #endif

    #define VERIFY_VKRESULT(Function)\
        {\
            VkResult Result = Function;\
            if (Result != VK_SUCCESS)\
            {\
                std::basic_string<TCHAR> ErrorMessage = TEXT("");\
                ErrorMessage += TEXT("Function [");\
                ErrorMessage += TEXT(#Function);\
                ErrorMessage += TEXT("]\nFailed with Error Code [");\
                ErrorMessage += TO_STRING(Result);\
                ErrorMessage += TEXT("]\nIn File [");\
                ErrorMessage += __FILE__;\
                ErrorMessage += TEXT("]\nLine [");\
                ErrorMessage += TO_STRING(__LINE__);\
                ErrorMessage += TEXT("]");\
                ::MessageBox(nullptr, ErrorMessage.c_str(), TEXT("Vulkan Error"), MB_OK);\
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

VkResult vkCreateInstance(VkInstanceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkInstance * pInstance);