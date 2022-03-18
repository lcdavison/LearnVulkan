#include "Vulkan.hpp"

#include <Windows.h>

#include <string>

bool const VulkanModule::Load()
{
	bool bResult = false;

	HMODULE VulkanLibrary = ::LoadLibrary(TEXT("vulkan-1.dll"));
	if (bResult = VulkanLibrary; !bResult)
	{
		::MessageBox(nullptr, TEXT("Failed to load vulkan module"), TEXT("Fatal Error"), MB_OK);
	}

	VulkanFunctions::vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(::GetProcAddress(VulkanLibrary, "vkGetInstanceProcAddr"));
	if (bResult = VulkanFunctions::vkGetInstanceProcAddr; !bResult)
	{
		::MessageBox(nullptr, TEXT("Failed to load \"vkGetInstanceProcAddr\""), TEXT("Fatal Error"), MB_OK);
	}

	LoadGlobalFunctions();

	return bResult;
}

bool const VulkanModule::LoadGlobalFunctions()
{
#define VK_GLOBAL_FUNCTION(FunctionName)\
	VulkanFunctions::FunctionName = reinterpret_cast<PFN_##FunctionName>(VulkanFunctions::vkGetInstanceProcAddr(nullptr, #FunctionName));\
	if (!VulkanFunctions::FunctionName) {\
		return false;\
	}\
	{\
		std::basic_string ErrorMessage = TEXT("Loaded Function: "); \
		ErrorMessage += TEXT(#FunctionName); \
		ErrorMessage += TEXT("\n");\
		::OutputDebugString(ErrorMessage.c_str());\
	}

#include "VulkanFunctions.inl"

	return true;
}