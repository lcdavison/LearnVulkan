#include "Graphics/VulkanModule.hpp"
#include <Windows.h>

#include "CommonTypes.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"

using namespace Vulkan;

bool const VulkanModule::Start()
{
    bool bResult = ::InitialiseVulkanWrapper();

    if (bResult)
    {
        bResult = ::LoadExportedFunctions() &&
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
    return ::ShutdownVulkanWrapper();
}