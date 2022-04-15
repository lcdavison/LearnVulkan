#include <Vulkan.hpp>

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
                ::PostQuitMessage(EXIT_FAILURE);\
            }\
        } while(false)
#else
    #define VERIFY_VKRESULT(Function) Function
#endif // _DEBUG

namespace VulkanModule
{
    extern bool const Start();
    extern bool const Stop();
}

#include "CommonTypes.hpp"