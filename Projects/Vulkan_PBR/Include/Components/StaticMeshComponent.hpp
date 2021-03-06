#pragma once

#include "Graphics/VulkanModule.hpp"

#include "AssetManager.hpp"

#include <vector>
#include <unordered_map>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Components::StaticMesh
{
    struct StaticMeshCollection
    {
        std::unordered_map<uint32, uint32> ActorIDToStaticMeshIndex = {};

        std::vector<uint32> MeshAssetHandles = {};
        std::vector<uint32> VertexBuffers = {};
        std::vector<uint32> NormalBuffers = {};
        std::vector<uint32> IndexBuffers = {};
        std::vector<uint32> IndexCounts = {};
    };

    struct StaticMeshData
    {
        uint32 VertexBuffer;
        uint32 NormalBuffer;
        uint32 IndexBuffer;
        uint32 IndexCount;
    };

    extern StaticMeshCollection StaticMeshes;

    extern bool const CreateComponent(uint32 const ActorHandle, uint32 const AssetHandle);

    //extern bool const DestroyComponent(uint32 const ActorHandle);

    extern bool const CreateGPUResources(uint32 const ActorHandle, Vulkan::Device::DeviceState const & DeviceState);

    extern bool const TransferToGPU(uint32 const ActorHandle, VkCommandBuffer const CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence);

    //extern bool const GetComponentData(uint32 const ActorID, StaticMeshData & OutputStaticMeshData);

    extern bool const GetStaticMeshResources(uint32 const ActorID, StaticMeshData & OutputStaticMeshData);
}