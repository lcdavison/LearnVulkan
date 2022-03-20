#include "Graphics/Vulkan.hpp"

#include <Windows.h>

#include <unordered_set>
#include <string>

extern bool const LoadExportedFunctions(HMODULE LibraryHandle)
{
#define VK_EXPORTED_FUNCTION(FunctionName)\
	VulkanFunctions::FunctionName = reinterpret_cast<PFN_##FunctionName>(::GetProcAddress(LibraryHandle, #FunctionName));\
	if (!VulkanFunctions::FunctionName) {\
		return false;\
	}\
	{\
		std::basic_string ErrorMessage = TEXT("Loaded Exported Function: "); \
		ErrorMessage += TEXT(#FunctionName); \
		ErrorMessage += TEXT("\n");\
		::OutputDebugString(ErrorMessage.c_str());\
	}

#include "Graphics/VulkanFunctions.inl"

	return true;
}

extern bool const LoadGlobalFunctions()
{
#define VK_GLOBAL_FUNCTION(FunctionName)\
	VulkanFunctions::FunctionName = reinterpret_cast<PFN_##FunctionName>(VulkanFunctions::vkGetInstanceProcAddr(nullptr, #FunctionName));\
	if (!VulkanFunctions::FunctionName) {\
		return false;\
	}\
	{\
		std::basic_string ErrorMessage = TEXT("Loaded Global Function: "); \
		ErrorMessage += TEXT(#FunctionName); \
		ErrorMessage += TEXT("\n");\
		::OutputDebugString(ErrorMessage.c_str());\
	}

#include "Graphics/VulkanFunctions.inl"

	return true;
}

extern bool const LoadInstanceFunctions(VkInstance Instance)
{
#define VK_INSTANCE_FUNCTION(FunctionName)\
	VulkanFunctions::FunctionName = reinterpret_cast<PFN_##FunctionName>(VulkanFunctions::vkGetInstanceProcAddr(Instance, #FunctionName));\
	if (!VulkanFunctions::FunctionName) {\
		return false;\
	}\
	{\
		std::basic_string ErrorMessage = TEXT("Loaded Instance Function: "); \
		ErrorMessage += TEXT(#FunctionName); \
		ErrorMessage += TEXT("\n");\
		::OutputDebugString(ErrorMessage.c_str());\
	}

#include "Graphics/VulkanFunctions.inl"

	return true;
}

extern bool const LoadInstanceExtensionFunctions(VkInstance Instance, std::unordered_set<std::string> const & ExtensionNames)
{
#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)\
	{\
		auto ExtensionFound = ExtensionNames.find(std::string(Extension));\
		if (ExtensionFound != ExtensionNames.end()) {\
			VulkanFunctions::Function = reinterpret_cast<PFN_##Function>(VulkanFunctions::vkGetInstanceProcAddr(Instance, #Function));\
			if (!VulkanFunctions::Function) {\
				return false;\
			}\
			{\
				std::basic_string ErrorMessage = TEXT("Loaded Instance Extension Function: ");\
				ErrorMessage += TEXT(#Function);\
				ErrorMessage += TEXT("\n");\
				::OutputDebugString(ErrorMessage.c_str());\
			}\
		}\
	}
	
#include "Graphics/VulkanFunctions.inl"

	return true;
}