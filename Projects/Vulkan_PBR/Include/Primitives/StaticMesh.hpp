#pragma once

#include "Graphics/VulkanModule.hpp"

#include "GPUResourceManager.hpp"
#include "AssetManager.hpp"

#include <vector>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Primitives::StaticMesh
{
    struct StaticMeshCollection
    {
        std::unordered_map<uint32, uint32> ActorIDToStaticMeshIndex = {};

        std::vector<GPUResourceManager::BufferHandle> VertexBuffers = {};
        std::vector<GPUResourceManager::BufferHandle> NormalBuffers = {};
        std::vector<GPUResourceManager::BufferHandle> IndexBuffers = {};
        std::vector<uint32> ActorIDs = {};
    };

    struct StaticMeshData
    {
        GPUResourceManager::BufferHandle VertexBuffer;
        GPUResourceManager::BufferHandle NormalBuffer;
        GPUResourceManager::BufferHandle IndexBuffer;
    };

    extern StaticMeshCollection StaticMeshes;

    extern bool const CreateGPUResources(VkCommandBuffer const CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, AssetManager::AssetHandle<AssetManager::MeshAsset> const AssetHandle, uint32 const ActorID, VkFence const CleanupFence);

    extern bool const GetStaticMeshResources(uint32 const ActorID, StaticMeshData & OutputStaticMeshData);
}