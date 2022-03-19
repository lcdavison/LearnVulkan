#include "Graphics/Vulkan.hpp"

#include <Windows.h>

#include <string>

bool const VulkanModule::LoadExportedFunctions(HMODULE LibraryHandle)
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

bool const VulkanModule::LoadGlobalFunctions()
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

bool const VulkanModule::LoadInstanceFunctions(VkInstance Instance)
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