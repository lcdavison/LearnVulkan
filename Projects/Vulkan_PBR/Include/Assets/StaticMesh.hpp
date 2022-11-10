#pragma once

#include "Common.hpp"
#include "Graphics/VulkanModule.hpp"

#include <filesystem>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Assets::StaticMesh::Types
{
    struct StaticMesh
    {
        struct StatusFlags
        {
            bool bHasNormals = {};
            bool bHasUVs = {};
        };

        uint64 NormalDataOffsetInBytes = {};
        uint64 TangentDataOffsetInBytes = {};
        uint64 UVDataOffsetInBytes = {};

        uint64 MeshDataSizeInBytes = {};

        std::byte * MeshData = {};
        uint32 * IndexData = {};

        uint32 MeshBufferHandle = {};
        uint32 IndexBufferHandle = {};

        uint32 VertexCount = {};
        uint32 IndexCount = {};

        StatusFlags Status = {};
    };
}

namespace Assets::StaticMesh
{
    extern bool const ImportStaticMesh(std::filesystem::path const & kFilePath, std::string AssetName, uint32 & OutputAssetHandle);

    extern bool const InitialiseGPUResources(VkCommandBuffer const kCommandBuffer, Vulkan::Device::DeviceState const & kDeviceState, VkFence const kTransferFence);

    extern bool const GetAssetData(uint32 const kAssetHandle, Assets::StaticMesh::Types::StaticMesh & OutputAssetData);
}