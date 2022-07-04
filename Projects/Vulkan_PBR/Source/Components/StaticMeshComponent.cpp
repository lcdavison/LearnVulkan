#include "Components/StaticMeshComponent.hpp"

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"

#include <array>

Components::StaticMesh::StaticMeshCollection Components::StaticMesh::StaticMeshes = {};

bool const Components::StaticMesh::CreateComponent(uint32 const ActorHandle, AssetManager::AssetHandle<AssetManager::MeshAsset> const AssetHandle)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create static mesh component for NULL actor handle."));
        return false;
    }

    Components::StaticMesh::StaticMeshes.MeshAssetHandles.push_back(AssetHandle);
    Components::StaticMesh::StaticMeshes.VertexBuffers.emplace_back();
    Components::StaticMesh::StaticMeshes.NormalBuffers.emplace_back();
    Components::StaticMesh::StaticMeshes.IndexBuffers.emplace_back();
    Components::StaticMesh::StaticMeshes.IndexCounts.emplace_back();

    uint32 const NewComponentHandle = static_cast<uint32>(Components::StaticMesh::StaticMeshes.MeshAssetHandles.size());
    Components::StaticMesh::StaticMeshes.ActorIDToStaticMeshIndex [ActorHandle] = NewComponentHandle;

    return true;
}

bool const Components::StaticMesh::CreateGPUResources(uint32 const ActorHandle, Vulkan::Device::DeviceState const & DeviceState)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create static mesh GPU resources for NULL actor."));
        return false;
    }

    uint32 const ComponentIndex = { Components::StaticMesh::StaticMeshes.ActorIDToStaticMeshIndex [ActorHandle] - 1u };

    AssetManager::MeshAsset const * MeshData = {};
    bool bResult = AssetManager::GetMeshData(Components::StaticMesh::StaticMeshes.MeshAssetHandles [ComponentIndex], MeshData);

    if (MeshData)
    {
        uint64 const VertexBufferSizeInBytes = { MeshData->Vertices.size() * sizeof(decltype(MeshData->Vertices [0u])) };
        uint64 const NormalBufferSizeInBytes = { MeshData->Normals.size() * sizeof(decltype(MeshData->Normals [0u])) };
        uint64 const IndexBufferSizeInBytes = { MeshData->Indices.size() * sizeof(decltype(MeshData->Indices [0u])) };

        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Creating GPU resources for Actor [ID = %d]. Sizes: [VB = %llu Bytes | NB = %llu Bytes | IB = %llu Bytes]"), ActorHandle, VertexBufferSizeInBytes, NormalBufferSizeInBytes, IndexBufferSizeInBytes));

        /* TODO: Need to return whether creation was successful */
        Vulkan::Device::CreateBuffer(DeviceState, VertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Components::StaticMesh::StaticMeshes.VertexBuffers [ComponentIndex]);
        Vulkan::Device::CreateBuffer(DeviceState, NormalBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Components::StaticMesh::StaticMeshes.NormalBuffers [ComponentIndex]);
        Vulkan::Device::CreateBuffer(DeviceState, IndexBufferSizeInBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Components::StaticMesh::StaticMeshes.IndexBuffers [ComponentIndex]);

        Components::StaticMesh::StaticMeshes.IndexCounts [ComponentIndex] = static_cast<uint32>(MeshData->Indices.size());
    }

    return bResult;
}

bool const Components::StaticMesh::TransferToGPU(uint32 const ActorHandle, VkCommandBuffer const CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence)
{
    uint32 const ComponentIndex = { Components::StaticMesh::StaticMeshes.ActorIDToStaticMeshIndex [ActorHandle] - 1u };

    AssetManager::MeshAsset const * MeshData = {};
    bool bResult = AssetManager::GetMeshData(Components::StaticMesh::StaticMeshes.MeshAssetHandles [ComponentIndex], MeshData);

    if (MeshData)
    {
        uint64 const VertexBufferSizeInBytes = { MeshData->Vertices.size() * sizeof(decltype(MeshData->Vertices [0u])) };
        uint64 const NormalBufferSizeInBytes = { MeshData->Normals.size() * sizeof(decltype(MeshData->Normals [0u])) };
        uint64 const IndexBufferSizeInBytes = { MeshData->Indices.size() * sizeof(decltype(MeshData->Indices [0u])) };

        std::array<uint32, 3u> StagingBufferHandles = {};

        Vulkan::Device::CreateBuffer(DeviceState, VertexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [0u]);
        Vulkan::Device::CreateBuffer(DeviceState, NormalBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [1u]);
        Vulkan::Device::CreateBuffer(DeviceState, IndexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [2u]);

        std::array<Vulkan::Resource::Buffer, 3u> StagingBuffers = {};

        Vulkan::Resource::GetBuffer(StagingBufferHandles [0u], StagingBuffers [0u]);
        Vulkan::Resource::GetBuffer(StagingBufferHandles [1u], StagingBuffers [1u]);
        Vulkan::Resource::GetBuffer(StagingBufferHandles [2u], StagingBuffers [2u]);

        ::memcpy_s(StagingBuffers [0u].MemoryAllocation->MappedAddress, StagingBuffers [0u].MemoryAllocation->SizeInBytes, MeshData->Vertices.data(), VertexBufferSizeInBytes);
        ::memcpy_s(StagingBuffers [1u].MemoryAllocation->MappedAddress, StagingBuffers [1u].MemoryAllocation->SizeInBytes, MeshData->Normals.data(), NormalBufferSizeInBytes);
        ::memcpy_s(StagingBuffers [2u].MemoryAllocation->MappedAddress, StagingBuffers [2u].MemoryAllocation->SizeInBytes, MeshData->Indices.data(), IndexBufferSizeInBytes);

        std::array<Vulkan::Resource::Buffer, 3u> Buffers = {};

        Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.VertexBuffers [ComponentIndex], Buffers [0u]);
        Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.NormalBuffers [ComponentIndex], Buffers [1u]);
        Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.IndexBuffers [ComponentIndex], Buffers [2u]);

        std::array<VkBufferMemoryBarrier, 6u> BufferBarriers =
        {
            Vulkan::BufferMemoryBarrier(StagingBuffers [0u].Resource, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            Vulkan::BufferMemoryBarrier(StagingBuffers [1u].Resource, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            Vulkan::BufferMemoryBarrier(StagingBuffers [2u].Resource, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),

            Vulkan::BufferMemoryBarrier(Buffers [0u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
            Vulkan::BufferMemoryBarrier(Buffers [1u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
            Vulkan::BufferMemoryBarrier(Buffers [2u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT),
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

        vkCmdCopyBuffer(CommandBuffer, StagingBuffers [0u].Resource, Buffers [0u].Resource, 1u, &CopyRegions [0u]);
        vkCmdCopyBuffer(CommandBuffer, StagingBuffers [1u].Resource, Buffers [1u].Resource, 1u, &CopyRegions [1u]);
        vkCmdCopyBuffer(CommandBuffer, StagingBuffers [2u].Resource, Buffers [2u].Resource, 1u, &CopyRegions [2u]);

        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                             0u,
                             0u, nullptr,
                             3u, &BufferBarriers [3u],
                             0u, nullptr);

        Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [0u], TransferFence);
        Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [1u], TransferFence);
        Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [2u], TransferFence);
    }

    return bResult;
}

bool const Components::StaticMesh::GetStaticMeshResources(uint32 const ActorID, Components::StaticMesh::StaticMeshData & OutputStaticMeshData)
{
    bool bResult = false;

    if (ActorID == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot get static mesh resources for NULL actor."));
        return false;
    }

    uint32 const StaticMeshID = Components::StaticMesh::StaticMeshes.ActorIDToStaticMeshIndex [ActorID] - 1u;

    if (StaticMeshID >= 0u && StaticMeshID < Components::StaticMesh::StaticMeshes.MeshAssetHandles.size())
    {
        OutputStaticMeshData.VertexBuffer = Components::StaticMesh::StaticMeshes.VertexBuffers [StaticMeshID];
        OutputStaticMeshData.NormalBuffer = Components::StaticMesh::StaticMeshes.NormalBuffers [StaticMeshID];
        OutputStaticMeshData.IndexBuffer = Components::StaticMesh::StaticMeshes.IndexBuffers [StaticMeshID];
        OutputStaticMeshData.IndexCount = Components::StaticMesh::StaticMeshes.IndexCounts [StaticMeshID];

        bResult = OutputStaticMeshData.VertexBuffer > 0u
                && OutputStaticMeshData.NormalBuffer > 0u
                && OutputStaticMeshData.IndexBuffer > 0u;
    }
    else
    {
        Logging::Log(Logging::LogTypes::Error, String::Format(PBR_TEXT("Failed to get Static Mesh for Actor [ID = %d]"), ActorID));
    }

    return bResult;
}