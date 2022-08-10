#pragma once

#include "Common.hpp"

#include "Graphics/VulkanModule.hpp"

#include <filesystem>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Assets::Texture
{
    struct TextureData
    {
        std::byte * Data = {};
        uint32 WidthInPixels = {};
        uint32 HeightInPixels = {};

        uint32 ImageHandle = {};
        uint32 ViewHandle = {};
    };

    /* This will load from a file such as a .bmp and create a texture asset */
    extern bool const ImportTexture(std::filesystem::path const & FilePath, std::string AssetName, uint32 & OutputAssetHandle);

    extern bool const FindTexture(std::string const & AssetName, uint32 & OutputAssetHandle);

    extern bool const GetTextureData(uint32 const AssetHandle, TextureData & OutputTextureData);

    /* Run through all the new textures and create the resources + buffer the transfer */
    extern bool const InitialiseNewTextureGPUResources(VkCommandBuffer CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence);

    /* Loads an existing texture asset from a file. This will pretty much just be loaded straight into memory */
    //extern bool const LoadTexture(std::filesystem::path const & FilePath, uint32 & OutputAssetHandle);
}