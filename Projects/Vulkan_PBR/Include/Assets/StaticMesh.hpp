#pragma once

#include "Common.hpp"

#include <Math/Vector.hpp>

#include "Graphics/VulkanModule.hpp"

#include <filesystem>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Assets::StaticMesh
{
    struct StaticMeshStatus
    {
        bool bHasNormals : 1;
        bool bHasUVs : 1;
    };

    struct AssetData
    {
        Math::Vector3 const * VertexData = {};
        Math::Vector3 const * NormalData = {};
        Math::Vector3 const * UVData = {};
        uint32 const * IndexData = {};
        
        uint32 VertexCount = {};
        uint32 IndexCount = {};

        uint32 VertexBufferHandle = {};
        uint32 NormalBufferHandle = {};
        uint32 UVBufferHandle = {};
        uint32 IndexBufferHandle = {};

        StaticMeshStatus Status = {};
    };

    extern bool const ImportStaticMesh(std::filesystem::path const & FilePath, std::string AssetName, uint32 & OutputAssetHandle);

    extern bool const InitialiseGPUResources(VkCommandBuffer CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence);

    extern bool const GetAssetData(uint32 const AssetHandle, AssetData & OutputAssetData);
}