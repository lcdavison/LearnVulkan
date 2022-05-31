#pragma once

struct VkApplicationInfo;

namespace ForwardRenderer
{
    extern bool const Initialise(VkApplicationInfo const & ApplicationInfo);

    extern bool const Shutdown();

    extern void PreRender();

    extern void Render();

    extern void PostRender();
}