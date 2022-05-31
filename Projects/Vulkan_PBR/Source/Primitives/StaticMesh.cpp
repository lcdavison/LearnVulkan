#include "Primitives/StaticMesh.hpp"

#include "Graphics/Device.hpp"

#include "AssetManager.hpp"
#include "GPUResourceManager.hpp"

#include <array>

Primitives::StaticMesh::StaticMeshCollection Primitives::StaticMesh::StaticMeshes = {};

bool const Primitives::StaticMesh::CreateGPUResources(VkCommandBuffer const CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, uint32 const /*AssetID*/, VkFence const FrameFence, uint32 & OutputStaticMeshID)
{
    bool bResult = true;

    /* TODO: Map asset id to existing resource ID if we have already created the gpu resources */

    AssetManager::MeshAsset const * MeshData = {};
    /* Hard coded at the moment, will make general when asset manager loading is async */
    bResult = AssetManager::GetMeshData("Cube", MeshData);

    if (MeshData)
    {
        uint64 const VertexBufferSizeInBytes = { MeshData->Vertices.size() * sizeof(decltype(MeshData->Vertices [0u])) };
        uint64 const NormalBufferSizeInBytes = { MeshData->Normals.size() * sizeof(decltype(MeshData->Normals [0u])) };
        uint64 const IndexBufferSizeInBytes = { MeshData->Indices.size() * sizeof(decltype(MeshData->Indices [0u])) };

        GPUResourceManager::BufferHandle VertexStagingBufferHandle = {};
        Vulkan::Device::CreateBuffer(DeviceState, VertexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VertexStagingBufferHandle);

        GPUResourceManager::BufferHandle NormalStagingBufferHandle = {};
        Vulkan::Device::CreateBuffer(DeviceState, NormalBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, NormalStagingBufferHandle);

        GPUResourceManager::BufferHandle IndexStagingBufferHandle = {};
        Vulkan::Device::CreateBuffer(DeviceState, IndexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, IndexStagingBufferHandle);

        GPUResource::Buffer VertexStagingBuffer = {};
        GPUResource::Buffer NormalStagingBuffer = {};
        GPUResource::Buffer IndexStagingBuffer = {};

        GPUResourceManager::GetResource(VertexStagingBufferHandle, VertexStagingBuffer);
        GPUResourceManager::GetResource(NormalStagingBufferHandle, NormalStagingBuffer);
        GPUResourceManager::GetResource(IndexStagingBufferHandle, IndexStagingBuffer);

        ::memcpy_s(VertexStagingBuffer.MemoryAllocation.MappedAddress, VertexStagingBuffer.MemoryAllocation.SizeInBytes, MeshData->Vertices.data(), VertexBufferSizeInBytes);
        ::memcpy_s(NormalStagingBuffer.MemoryAllocation.MappedAddress, NormalStagingBuffer.MemoryAllocation.SizeInBytes, MeshData->Normals.data(), NormalBufferSizeInBytes);
        ::memcpy_s(IndexStagingBuffer.MemoryAllocation.MappedAddress, IndexStagingBuffer.MemoryAllocation.SizeInBytes, MeshData->Indices.data(), IndexBufferSizeInBytes);

        GPUResourceManager::BufferHandle VertexBufferHandle = {};
        Vulkan::Device::CreateBuffer(DeviceState, VertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBufferHandle);

        GPUResourceManager::BufferHandle NormalBufferHandle = {};
        Vulkan::Device::CreateBuffer(DeviceState, NormalBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, NormalBufferHandle);

        GPUResourceManager::BufferHandle IndexBufferHandle = {};
        Vulkan::Device::CreateBuffer(DeviceState, IndexBufferSizeInBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IndexBufferHandle);

        GPUResource::Buffer VertexBuffer = {};
        GPUResource::Buffer NormalBuffer = {};
        GPUResource::Buffer IndexBuffer = {};

        GPUResourceManager::GetResource(VertexBufferHandle, VertexBuffer);
        GPUResourceManager::GetResource(NormalBufferHandle, NormalBuffer);
        GPUResourceManager::GetResource(IndexBufferHandle, IndexBuffer);

        std::array<VkBufferMemoryBarrier, 6u> BufferBarriers =
        {
            Vulkan::BufferMemoryBarrier(VertexStagingBuffer.VkResource, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            Vulkan::BufferMemoryBarrier(NormalStagingBuffer.VkResource, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            Vulkan::BufferMemoryBarrier(IndexStagingBuffer.VkResource, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),

            Vulkan::BufferMemoryBarrier(VertexBuffer.VkResource, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
            Vulkan::BufferMemoryBarrier(NormalBuffer.VkResource, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
            Vulkan::BufferMemoryBarrier(IndexBuffer.VkResource, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_INDEX_READ_BIT),
        };

        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0u,
                             0u, nullptr,
                             3u, &BufferBarriers [0u],
                             0u, nullptr);

        std::array<VkBufferCopy, 3u> CopyRegions =
        {
            VkBufferCopy { 0u, 0u, VertexBufferSizeInBytes },
            VkBufferCopy { 0u, 0u, NormalBufferSizeInBytes },
            VkBufferCopy { 0u, 0u, IndexBufferSizeInBytes },
        };

        vkCmdCopyBuffer(CommandBuffer, VertexStagingBuffer.VkResource, VertexBuffer.VkResource, 1u, &CopyRegions [0u]);
        vkCmdCopyBuffer(CommandBuffer, NormalStagingBuffer.VkResource, NormalBuffer.VkResource, 1u, &CopyRegions [1u]);
        vkCmdCopyBuffer(CommandBuffer, IndexStagingBuffer.VkResource, IndexBuffer.VkResource, 1u, &CopyRegions [2u]);

        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                             0u,
                             0u, nullptr,
                             3u, &BufferBarriers [3u],
                             0u, nullptr);

        GPUResourceManager::QueueResourceDeletion(FrameFence, VertexStagingBufferHandle);
        GPUResourceManager::QueueResourceDeletion(FrameFence, NormalStagingBufferHandle);
        GPUResourceManager::QueueResourceDeletion(FrameFence, IndexStagingBufferHandle);

        OutputStaticMeshID = static_cast<uint32>(Primitives::StaticMesh::StaticMeshes.VertexBuffers.size());

        Primitives::StaticMesh::StaticMeshes.VertexBuffers.push_back(VertexBufferHandle);
        Primitives::StaticMesh::StaticMeshes.NormalBuffers.push_back(NormalBufferHandle);
        Primitives::StaticMesh::StaticMeshes.IndexBuffers.push_back(IndexBufferHandle);
    }

    return bResult;
}

bool const Primitives::StaticMesh::GetStaticMeshResources(uint32 const StaticMeshID, Primitives::StaticMesh::StaticMeshData & OutputStaticMeshData)
{
    bool bResult = false;

    if (StaticMeshID < Primitives::StaticMesh::StaticMeshes.VertexBuffers.size())
    {
        OutputStaticMeshData.VertexBuffer = Primitives::StaticMesh::StaticMeshes.VertexBuffers [StaticMeshID];
        OutputStaticMeshData.NormalBuffer = Primitives::StaticMesh::StaticMeshes.NormalBuffers [StaticMeshID];
        OutputStaticMeshData.IndexBuffer = Primitives::StaticMesh::StaticMeshes.IndexBuffers [StaticMeshID];

        bResult = true;
    }

    return bResult;
}