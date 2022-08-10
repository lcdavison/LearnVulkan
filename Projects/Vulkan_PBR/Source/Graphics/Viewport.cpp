#include "Graphics/Viewport.hpp"

#include "VulkanPBR.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"

#include <vector>
#include <algorithm>

VkFormat const kRequiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
VkColorSpaceKHR const kRequiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

uint32 const kRequiredImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
uint32 const kRequiredImageCount = 2u;

inline static bool const HasRequiredImageUsage(VkSurfaceCapabilitiesKHR const & SurfaceCapabilities)
{
    return (SurfaceCapabilities.supportedUsageFlags & kRequiredImageUsageFlags) == kRequiredImageUsageFlags;
}

inline static uint32 const GetSwapChainImageCount(VkSurfaceCapabilitiesKHR const& SurfaceCapabilities)
{
    uint32 ImageCount = { SurfaceCapabilities.minImageCount };

    /* maxImageCount == 0u indicates there is no limit */
    bool bRestrictImageCount = SurfaceCapabilities.maxImageCount > 0u && ImageCount > SurfaceCapabilities.maxImageCount;

    return bRestrictImageCount ? SurfaceCapabilities.maxImageCount : ImageCount;
}

static VkExtent2D const GetSwapChainImageExtents(VkExtent2D DesiredExtents, VkSurfaceCapabilitiesKHR const& SurfaceCapabilities)
{
    VkExtent2D ImageExtents = {};

    /* currentExtent.width == 0xFFFFFFFF indicates the image size controls window size */
    if (SurfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
        ImageExtents.width = std::clamp(DesiredExtents.width, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
        ImageExtents.height = std::clamp(DesiredExtents.height, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);
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
    VERIFY_VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &FormatCount, nullptr));

    if (FormatCount > 0u)
    {
        std::vector SupportedFormats = std::vector<VkSurfaceFormatKHR>(FormatCount);

        VERIFY_VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &FormatCount, SupportedFormats.data()));

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

static void StoreSwapChainImages(Vulkan::Device::DeviceState const & DeviceState, Vulkan::Viewport::ViewportState & State)
{
    uint32 SwapChainImageCount = {};
    VERIFY_VKRESULT(vkGetSwapchainImagesKHR(DeviceState.Device, State.SwapChain, &SwapChainImageCount, nullptr));

    State.SwapChainImages.resize(SwapChainImageCount);
    State.SwapChainImageViews.resize(SwapChainImageCount);

    VERIFY_VKRESULT(vkGetSwapchainImagesKHR(DeviceState.Device, State.SwapChain, &SwapChainImageCount, State.SwapChainImages.data()));

    uint32 CurrentImageIndex = { 0u };

    for (VkImageView & ImageView : State.SwapChainImageViews)
    {
        VkImageViewCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        CreateInfo.image = State.SwapChainImages [CurrentImageIndex];
        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        CreateInfo.format = State.SurfaceFormat.format;
        CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        CreateInfo.subresourceRange.baseMipLevel = 0u;
        CreateInfo.subresourceRange.levelCount = 1u;
        CreateInfo.subresourceRange.baseArrayLayer = 0u;
        CreateInfo.subresourceRange.layerCount = 1u;

        VERIFY_VKRESULT(vkCreateImageView(DeviceState.Device, &CreateInfo, nullptr, &ImageView));

        CurrentImageIndex++;
    }
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
    VERIFY_VKRESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &SurfaceCapabilities));

    VkExtent2D DesiredExtents = {};
    DesiredExtents.width = Application::State.CurrentWindowWidth;
    DesiredExtents.height = Application::State.CurrentWindowHeight;

    IntermediateState.ImageExtents = ::GetSwapChainImageExtents(DesiredExtents, SurfaceCapabilities);
    
    if (::HasRequiredImageUsage(SurfaceCapabilities) &&
        ::SupportsRequiredSurfaceFormat(InstanceState, DeviceState, IntermediateState.SurfaceFormat))
    {
        VkSwapchainCreateInfoKHR SwapChainCreateInfo = Vulkan::SwapChain(VK_NULL_HANDLE, InstanceState.Surface, IntermediateState.ImageExtents, IntermediateState.SurfaceFormat.format, IntermediateState.SurfaceFormat.colorSpace, kRequiredImageUsageFlags, SurfaceCapabilities.minImageCount);

        VERIFY_VKRESULT(vkCreateSwapchainKHR(DeviceState.Device, &SwapChainCreateInfo, nullptr, &IntermediateState.SwapChain));

        ::StoreSwapChainImages(DeviceState, IntermediateState);

        IntermediateState.DynamicViewport.width = static_cast<float>(IntermediateState.ImageExtents.width);
        IntermediateState.DynamicViewport.height = static_cast<float>(IntermediateState.ImageExtents.height);
        IntermediateState.DynamicViewport.x = 0.0f;
        IntermediateState.DynamicViewport.y = 0.0f;
        IntermediateState.DynamicViewport.minDepth = 0.0f;
        IntermediateState.DynamicViewport.maxDepth = 1.0f;

        IntermediateState.DynamicScissorRect.extent = IntermediateState.ImageExtents;
        IntermediateState.DynamicScissorRect.offset = VkOffset2D { 0u, 0u };

        OutputState = std::move(IntermediateState);
        bResult = true;
    }

    return bResult;
}

bool const Vulkan::Viewport::ResizeViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, Vulkan::Viewport::ViewportState & State)
{
    bool bResult = false;

    VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
    VkResult SurfaceCapabilityError = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceState.PhysicalDevice, InstanceState.Surface, &SurfaceCapabilities);

    if (SurfaceCapabilityError != VK_ERROR_SURFACE_LOST_KHR
        && SurfaceCapabilities.currentExtent.width > 0u
        && SurfaceCapabilities.currentExtent.height > 0u)
    {
        for (VkImageView & ImageView : State.SwapChainImageViews)
        {
            if (ImageView)
            {
                vkDestroyImageView(DeviceState.Device, ImageView, nullptr);
                ImageView = VK_NULL_HANDLE;
            }
        }

        Vulkan::Viewport::ViewportState IntermediateState = {};

        VkExtent2D NewImageExtents = {};
        NewImageExtents.width = Application::State.CurrentWindowWidth;
        NewImageExtents.height = Application::State.CurrentWindowHeight;

        IntermediateState.ImageExtents = ::GetSwapChainImageExtents(NewImageExtents, SurfaceCapabilities);
        IntermediateState.SurfaceFormat = State.SurfaceFormat;

        /* Need to find out whether we should keep track of the old swapchains and destroy them ourselves */
        /* Reading the spec images that aren't in use should get cleand up */
        VkSwapchainCreateInfoKHR SwapChainCreateInfo = Vulkan::SwapChain(State.SwapChain, InstanceState.Surface, IntermediateState.ImageExtents, IntermediateState.SurfaceFormat.format, IntermediateState.SurfaceFormat.colorSpace, kRequiredImageUsageFlags, SurfaceCapabilities.minImageCount);

        VERIFY_VKRESULT(vkCreateSwapchainKHR(DeviceState.Device, &SwapChainCreateInfo, nullptr, &IntermediateState.SwapChain));

        ::StoreSwapChainImages(DeviceState, IntermediateState);

        IntermediateState.DynamicViewport.width = static_cast<float>(IntermediateState.ImageExtents.width);
        IntermediateState.DynamicViewport.height = static_cast<float>(IntermediateState.ImageExtents.height);
        IntermediateState.DynamicViewport.x = 0.0f;
        IntermediateState.DynamicViewport.y = 0.0f;
        IntermediateState.DynamicViewport.minDepth = 0.0f;
        IntermediateState.DynamicViewport.maxDepth = 1.0f;

        IntermediateState.DynamicScissorRect.extent = IntermediateState.ImageExtents;
        IntermediateState.DynamicScissorRect.offset = VkOffset2D { 0u, 0u };

        if (SwapChainCreateInfo.oldSwapchain)
        {
            vkDestroySwapchainKHR(DeviceState.Device, SwapChainCreateInfo.oldSwapchain, nullptr);
        }

        State = std::move(IntermediateState);
        bResult = true;
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, PBR_TEXT("Surface lost or degenerate extents"));
    }

    return bResult;
}

void Vulkan::Viewport::DestroyViewport(Vulkan::Device::DeviceState const & DeviceState, Vulkan::Viewport::ViewportState & State)
{
    for (VkImageView & ImageView : State.SwapChainImageViews)
    {
        if (ImageView)
        {
            vkDestroyImageView(DeviceState.Device, ImageView, nullptr);
            ImageView = VK_NULL_HANDLE;
        }
    }

    if (State.SwapChain)
    {
        vkDestroySwapchainKHR(DeviceState.Device, State.SwapChain, nullptr);
        State.SwapChain = VK_NULL_HANDLE;
    }
}
