#pragma once

#include "Vulkan.hpp"

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
        VkSurfaceFormatKHR SurfaceFormat;
        VkSwapchainKHR SwapChain;
        std::vector<VkImage> SwapChainImages;
    };

    extern bool const CreateViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, ViewportState & OutputState);

    extern void DestroyViewport(Vulkan::Device::DeviceState const & DeviceState, ViewportState & State);
}