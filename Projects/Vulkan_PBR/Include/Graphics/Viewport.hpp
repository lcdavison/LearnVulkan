#pragma once

#include "Vulkan.hpp"

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
    };

    extern bool const CreateViewport(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState const & DeviceState, ViewportState & OutputState);
}