#pragma once

#include "VulkanModule.hpp"

#include "CommonTypes.hpp"

#include <vector>

namespace Vulkan::Instance
{
    struct InstanceState;
}

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Vulkan::Viewport
{
    struct ViewportState
    {
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;

        std::vector<VkFramebuffer> FrameBuffers;

        VkExtent2D ImageExtents;

        VkSwapchainKHR SwapChain;
        VkSurfaceFormatKHR SurfaceFormat;

        uint32 CurrentBackBufferIndex;
    };

    extern bool const CreateViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, ViewportState & OutputState);

    extern void CreateFrameBuffers(Vulkan::Device::DeviceState const & DeviceState, VkRenderPass RenderPass, ViewportState & State);

    extern bool const ResizeViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, ViewportState & State);

    extern void DestroyViewport(Vulkan::Device::DeviceState const & DeviceState, ViewportState & State);
}