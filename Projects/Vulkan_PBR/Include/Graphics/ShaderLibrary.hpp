#pragma once

#include "CommonTypes.hpp"

#include <Vulkan.hpp>

#include <filesystem>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace ShaderLibrary
{
    extern bool const Initialise();
    extern void Destroy();

    /* This could event be uint8, not planning on having 2^16 shaders */
    extern uint16 LoadShader(std::filesystem::path const FilePath);

    extern VkShaderModule CreateShaderModule(Vulkan::Device::DeviceState const & DeviceState, uint16 const ShaderIndex);

    extern void DestroyShaderModules(Vulkan::Device::DeviceState const & DeviceState);
}