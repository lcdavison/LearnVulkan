#pragma once

#include "Common.hpp"

#include "Graphics/VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

/* TODO: Have shaders for the materials */

namespace Assets::Material
{
    struct MaterialData
    {
        uint32 AlbedoTexture = {};
        uint32 NormalTexture = {};
        uint32 SpecularTexture = {};
        uint32 RoughnessTexture = {};
        uint32 AmbientOcclusionTexture = {};
    };

    extern bool const CreateMaterial(MaterialData const & MaterialDesc, std::string AssetName, uint32 & OutputMaterialHandle);
}