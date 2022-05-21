#include <Vulkan.hpp>

#include "Common.hpp"

#include "Platform/Windows.hpp"
#include "Logging.hpp"

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
            if (Result < VK_SUCCESS)\
            {\
                std::basic_string<Platform::Windows::TCHAR> ErrorMessage = PBR_TEXT(""); \
                ErrorMessage += PBR_TEXT("Function ["); \
                ErrorMessage += PBR_TEXT(#Function); \
                ErrorMessage += PBR_TEXT("]\nFailed with Error Code ["); \
                ErrorMessage += TO_STRING(Result); \
                ErrorMessage += PBR_TEXT("]\nIn File ["); \
                ErrorMessage += __FILE__; \
                ErrorMessage += PBR_TEXT("]\nLine ["); \
                ErrorMessage += TO_STRING(__LINE__); \
                ErrorMessage += PBR_TEXT("]"); \
                Logging::FatalError(ErrorMessage.c_str()); \
                Platform::Windows::PostQuitMessage(EXIT_FAILURE); \
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