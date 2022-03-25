#include "Graphics/Viewport.hpp"

#include "VulkanPBR.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"

#include "CommonTypes.hpp"

#include <algorithm>

uint32 const kRequiredImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

inline static bool const HasRequiredImageUsage(VkSurfaceCapabilitiesKHR const & SurfaceCapabilities)
{
    return (SurfaceCapabilities.supportedUsageFlags & kRequiredImageUsageFlags) == kRequiredImageUsageFlags;
}

static bool const SupportsRequiredSurfaceFormat(Vulkan::Device::DeviceState const & DeviceState)
{
    bool bResult = false;

    return bResult;
}

bool const Vulkan::Viewport::CreateViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, Vulkan::Viewport::ViewportState & OutputState)
{
    if (!InstanceState.Instance || 
        !DeviceState.PhysicalDevice || 
        !DeviceState.PhysicalDevice)
    {
        return false;
    }

    Vulkan::Viewport::ViewportState IntermediateState = {};

    VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
    VERIFY_VKRESULT(VulkanFunctions::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &SurfaceCapabilities));

    uint32 SwapChainImageCount = SurfaceCapabilities.minImageCount + 1u;

    /* maxImageCount == 0u indicates there is no limit */
    if (SurfaceCapabilities.maxImageCount > 0u && SwapChainImageCount > SurfaceCapabilities.maxImageCount)
    {
        SwapChainImageCount = SurfaceCapabilities.maxImageCount;
    }
    
    VkExtent2D ImageSize = {};

    /* currentExtent.width == 0xFFFFFFFF indicates the image size controls window size */
    if (SurfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
        ImageSize.width = std::clamp(Application::kDefaultWindowWidth, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
        ImageSize.height = std::clamp(Application::kDefaultWindowHeight, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);
    }
    else
    {
        ImageSize = SurfaceCapabilities.currentExtent;
    }

    

    VkSwapchainCreateInfoKHR SwapChainCreateInfo = {};
    SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    VERIFY_VKRESULT(VulkanFunctions::vkCreateSwapchainKHR(DeviceState.Device, &SwapChainCreateInfo, nullptr, &IntermediateState.SwapChain));

    return true;
}