#pragma once

#include "Graphics/VulkanModule.hpp"

#include "GPUResourceManager.hpp"

#include <vector>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Primitives::StaticMesh
{
    struct StaticMeshCollection
    {
        std::vector<GPUResourceManager::BufferHandle> VertexBuffers = {};
        std::vector<GPUResourceManager::BufferHandle> NormalBuffers = {};
        std::vector<GPUResourceManager::BufferHandle> IndexBuffers = {};
    };

    struct StaticMeshData
    {
        GPUResourceManager::BufferHandle VertexBuffer;
        GPUResourceManager::BufferHandle NormalBuffer;
        GPUResourceManager::BufferHandle IndexBuffer;
    };

    extern StaticMeshCollection StaticMeshes;

    extern bool const CreateGPUResources(VkCommandBuffer const CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, uint32 const AssetID, VkFence const FrameFence, uint32 & OutputStaticMeshID);

    extern bool const GetStaticMeshResources(uint32 const StaticMeshID, StaticMeshData & OutputStaticMeshData);
}