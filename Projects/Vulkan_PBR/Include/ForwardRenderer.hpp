#pragma once

#include "Graphics/Vulkan.hpp"

namespace ForwardRenderer
{
    extern bool const Initialise(VkApplicationInfo const & ApplicationInfo);

    extern bool const Shutdown();

    extern void Render();
}