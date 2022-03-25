#include "Graphics/Viewport.hpp"

#include "VulkanPBR.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"

#include "CommonTypes.hpp"

#include <vector>
#include <algorithm>

VkFormat const kRequiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
VkColorSpaceKHR const kRequiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

uint32 const kRequiredImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

inline static bool const HasRequiredImageUsage(VkSurfaceCapabilitiesKHR const & SurfaceCapabilities)
{
    return (SurfaceCapabilities.supportedUsageFlags & kRequiredImageUsageFlags) == kRequiredImageUsageFlags;
}

inline static uint32 const GetSwapChainImageCount(VkSurfaceCapabilitiesKHR const& SurfaceCapabilities)
{
    uint32 ImageCount = { SurfaceCapabilities.minImageCount + 1u };

    /* maxImageCount == 0u indicates there is no limit */
    bool bRestrictImageCount = SurfaceCapabilities.maxImageCount > 0u && ImageCount > SurfaceCapabilities.maxImageCount;

    return bRestrictImageCount ? SurfaceCapabilities.maxImageCount : ImageCount;
}

static VkExtent2D const GetSwapChainImageExtents(VkSurfaceCapabilitiesKHR const& SurfaceCapabilities)
{
    VkExtent2D ImageExtents = {};

    /* currentExtent.width == 0xFFFFFFFF indicates the image size controls window size */
    if (SurfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
        ImageExtents.width = std::clamp(Application::kDefaultWindowWidth, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
        ImageExtents.height = std::clamp(Application::kDefaultWindowHeight, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);
    }
    else
    {
        ImageExtents = SurfaceCapabilities.currentExtent;
    }

    return ImageExtents;
}

static bool const SupportsRequiredSurfaceFormat(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, VkSurfaceFormatKHR & OutputSurfaceFormat)
{
    bool bResult = false;

    uint32 FormatCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkGetPhysicalDeviceSurfaceFormatsKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &FormatCount, nullptr));

    if (FormatCount > 0u)
    {
        std::vector SupportedFormats = std::vector<VkSurfaceFormatKHR>(FormatCount);

        VERIFY_VKRESULT(VulkanFunctions::vkGetPhysicalDeviceSurfaceFormatsKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &FormatCount, SupportedFormats.data()));

        /* One element with VK_FORMAT_UNDEFINED means that we can choose whatever surface format we want apparently */
        if (SupportedFormats.size() == 1 && SupportedFormats[0u].format == VK_FORMAT_UNDEFINED)
        {
            OutputSurfaceFormat.format = kRequiredFormat;
            OutputSurfaceFormat.colorSpace = kRequiredColorSpace;
            bResult = true;
        }
        else
        {
            for (VkSurfaceFormatKHR const& Format : SupportedFormats)
            {
                if (Format.format == kRequiredFormat &&
                    Format.colorSpace == kRequiredColorSpace)
                {
                    OutputSurfaceFormat = Format;
                    bResult = true;
                    break;
                }
            }
        }
    }

    return bResult;
}

bool const Vulkan::Viewport::CreateViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, Vulkan::Viewport::ViewportState & OutputState)
{
    if (!InstanceState.Instance || 
        !DeviceState.PhysicalDevice || 
        !DeviceState.Device)
    {
        return false;
    }

    bool bResult = false;

    Vulkan::Viewport::ViewportState IntermediateState = {};

    VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
    VERIFY_VKRESULT(VulkanFunctions::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &SurfaceCapabilities));

    uint32 ImageCount = ::GetSwapChainImageCount(SurfaceCapabilities);
    
    VkExtent2D ImageExtents = ::GetSwapChainImageExtents(SurfaceCapabilities);
    
    VkSurfaceFormatKHR SurfaceFormat = {};

    if (::HasRequiredImageUsage(SurfaceCapabilities) &&
        ::SupportsRequiredSurfaceFormat(InstanceState, DeviceState, SurfaceFormat))
    {
        VkSwapchainCreateInfoKHR SwapChainCreateInfo = {};
        SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        SwapChainCreateInfo.surface = InstanceState.Surface;
        SwapChainCreateInfo.imageExtent = ImageExtents;
        SwapChainCreateInfo.minImageCount = ImageCount;
        SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
        SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
        SwapChainCreateInfo.imageArrayLayers = 1u;
        SwapChainCreateInfo.imageUsage = kRequiredImageUsageFlags;
        SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapChainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
        SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        SwapChainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        SwapChainCreateInfo.clipped = VK_TRUE;

        VERIFY_VKRESULT(VulkanFunctions::vkCreateSwapchainKHR(DeviceState.Device, &SwapChainCreateInfo, nullptr, &IntermediateState.SwapChain));

        {
            uint32 SwapChainImageCount = {};
            VERIFY_VKRESULT(VulkanFunctions::vkGetSwapchainImagesKHR(DeviceState.Device, IntermediateState.SwapChain, &SwapChainImageCount, nullptr));

            IntermediateState.SwapChainImages.resize(SwapChainImageCount);

            VERIFY_VKRESULT(VulkanFunctions::vkGetSwapchainImagesKHR(DeviceState.Device, IntermediateState.SwapChain, &SwapChainImageCount, IntermediateState.SwapChainImages.data()));
        }

        OutputState = IntermediateState;
        bResult = true;
    }

    return bResult;
}

void Vulkan::Viewport::DestroyViewport(Vulkan::Device::DeviceState const & DeviceState, Vulkan::Viewport::ViewportState & State)
{
    if (State.SwapChain)
    {
        VulkanFunctions::vkDestroySwapchainKHR(DeviceState.Device, State.SwapChain, nullptr);
        State.SwapChain = VK_NULL_HANDLE;
    }
}