#include "Graphics/VulkanModule.hpp"
#include <Windows.h>

#include "CommonTypes.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"

using namespace Vulkan;

static HMODULE VulkanLibraryHandle = {};

bool const VulkanModule::Start()
{
    bool bResult = false;

    VulkanLibraryHandle = ::LoadLibrary(TEXT("vulkan-1.dll"));
    if (VulkanLibraryHandle)
    {
        bResult = ::LoadExportedFunctions(VulkanLibraryHandle) &&
                  ::LoadGlobalFunctions();
    }
    else
    {
        ::MessageBox(nullptr, TEXT("Failed to load vulkan module"), TEXT("Fatal Error"), MB_OK);
    }

    return bResult;
}

bool const VulkanModule::Stop()
{
    return ::FreeLibrary(VulkanLibraryHandle) == TRUE;
}