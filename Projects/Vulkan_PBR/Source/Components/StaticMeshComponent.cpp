#include "Components/StaticMeshComponent.hpp"

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"

#include <array>

using namespace Components;

StaticMesh::StaticMeshCollection Components::StaticMesh::StaticMeshes = {};

bool const StaticMesh::CreateComponent(uint32 const ActorHandle, uint32 const AssetHandle)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create static mesh component for NULL actor handle."));
        return false;
    }

    auto FoundMesh = StaticMesh::StaticMeshes.AssetHandleToMeshIndex.find(AssetHandle);
    if (FoundMesh == StaticMesh::StaticMeshes.AssetHandleToMeshIndex.cend())
    {
        uint32 const NewMeshIndex = static_cast<uint32>(StaticMesh::StaticMeshes.MeshAssetHandles.size());
        StaticMesh::StaticMeshes.ActorIDToMeshIndex [ActorHandle] = NewMeshIndex;
        StaticMesh::StaticMeshes.AssetHandleToMeshIndex [AssetHandle] = NewMeshIndex;

        StaticMesh::StaticMeshes.MeshAssetHandles.push_back(AssetHandle);
        StaticMesh::StaticMeshes.VertexBuffers.emplace_back();
        StaticMesh::StaticMeshes.NormalBuffers.emplace_back();
        StaticMesh::StaticMeshes.UVBuffers.emplace_back();
        StaticMesh::StaticMeshes.IndexBuffers.emplace_back();
        StaticMesh::StaticMeshes.IndexCounts.emplace_back();
        StaticMesh::StaticMeshes.StatusFlags.emplace_back();
    }
    else
    {
        uint32 const NewMeshIndex = FoundMesh->second;
        StaticMesh::StaticMeshes.ActorIDToMeshIndex [ActorHandle] = NewMeshIndex;
    }

    return true;
}

bool const StaticMesh::CreateGPUResources(uint32 const ActorHandle, Vulkan::Device::DeviceState const & DeviceState)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create static mesh GPU resources for NULL actor."));
        return false;
    }

    bool bResult = true;

    uint32 const MeshIndex = { StaticMesh::StaticMeshes.ActorIDToMeshIndex [ActorHandle] };

    if (!StaticMesh::StaticMeshes.StatusFlags [MeshIndex].bReady 
        && !StaticMesh::StaticMeshes.StatusFlags [MeshIndex].bPendingTransfer)
    {
        AssetManager::MeshAsset const * MeshData = {};
        bResult = AssetManager::GetMeshAsset(StaticMesh::StaticMeshes.MeshAssetHandles [MeshIndex], MeshData);

        if (MeshData)
        {
            uint64 const VertexBufferSizeInBytes = { MeshData->Vertices.size() * sizeof(decltype(MeshData->Vertices [0u])) };
            uint64 const NormalBufferSizeInBytes = { MeshData->Normals.size() * sizeof(decltype(MeshData->Normals [0u])) };
            uint64 const UVBufferSizeInBytes = { MeshData->UVs.size() * sizeof(decltype(MeshData->UVs [0u])) };
            uint64 const IndexBufferSizeInBytes = { MeshData->Indices.size() * sizeof(decltype(MeshData->Indices [0u])) };

            Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Creating GPU resources for Actor [ID = %d]. Sizes: [VB = %llu Bytes | NB = %llu Bytes | UVB = %llu | IB = %llu Bytes]"), ActorHandle, VertexBufferSizeInBytes, NormalBufferSizeInBytes, UVBufferSizeInBytes, IndexBufferSizeInBytes));

            /* TODO: Need to return whether creation was successful */
            Vulkan::Device::CreateBuffer(DeviceState, VertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMesh::StaticMeshes.VertexBuffers [MeshIndex]);
            Vulkan::Device::CreateBuffer(DeviceState, NormalBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMesh::StaticMeshes.NormalBuffers [MeshIndex]);
            Vulkan::Device::CreateBuffer(DeviceState, UVBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMesh::StaticMeshes.UVBuffers [MeshIndex]);
            Vulkan::Device::CreateBuffer(DeviceState, IndexBufferSizeInBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMesh::StaticMeshes.IndexBuffers [MeshIndex]);

            StaticMesh::StaticMeshes.IndexCounts [MeshIndex] = static_cast<uint32>(MeshData->Indices.size());

            StaticMesh::StaticMeshes.StatusFlags [MeshIndex].bPendingTransfer = true;
        }
    }

    return bResult;
}

bool const StaticMesh::TransferToGPU(uint32 const ActorHandle, VkCommandBuffer const CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence)
{
    bool bResult = true;

    uint32 const MeshIndex = { StaticMesh::StaticMeshes.ActorIDToMeshIndex [ActorHandle] };

    if (StaticMesh::StaticMeshes.StatusFlags [MeshIndex].bPendingTransfer)
    {
        AssetManager::MeshAsset const * MeshData = {};
        bResult = AssetManager::GetMeshAsset(StaticMesh::StaticMeshes.MeshAssetHandles [MeshIndex], MeshData);

        if (MeshData)
        {
            uint64 const VertexBufferSizeInBytes = { MeshData->Vertices.size() * sizeof(decltype(MeshData->Vertices [0u])) };
            uint64 const NormalBufferSizeInBytes = { MeshData->Normals.size() * sizeof(decltype(MeshData->Normals [0u])) };
            uint64 const UVBufferSizeInBytes = { MeshData->UVs.size() * sizeof(decltype(MeshData->UVs [0u])) };
            uint64 const IndexBufferSizeInBytes = { MeshData->Indices.size() * sizeof(decltype(MeshData->Indices [0u])) };

            /* TODO: Just allocate one massive buffer here for the transfer. May need to adjust allocation block size for device memory allocator */

            std::array<uint32, 4u> StagingBufferHandles = {};

            Vulkan::Device::CreateBuffer(DeviceState, VertexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [0u]);
            Vulkan::Device::CreateBuffer(DeviceState, NormalBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [1u]);
            Vulkan::Device::CreateBuffer(DeviceState, UVBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [2u]);
            Vulkan::Device::CreateBuffer(DeviceState, IndexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [3u]);

            std::array<Vulkan::Resource::Buffer, 4u> StagingBuffers = {};

            Vulkan::Resource::GetBuffer(StagingBufferHandles [0u], StagingBuffers [0u]);
            Vulkan::Resource::GetBuffer(StagingBufferHandles [1u], StagingBuffers [1u]);
            Vulkan::Resource::GetBuffer(StagingBufferHandles [2u], StagingBuffers [2u]);
            Vulkan::Resource::GetBuffer(StagingBufferHandles [3u], StagingBuffers [3u]);

            ::memcpy_s(StagingBuffers [0u].MemoryAllocation->MappedAddress, StagingBuffers [0u].MemoryAllocation->SizeInBytes, MeshData->Vertices.data(), VertexBufferSizeInBytes);
            ::memcpy_s(StagingBuffers [1u].MemoryAllocation->MappedAddress, StagingBuffers [1u].MemoryAllocation->SizeInBytes, MeshData->Normals.data(), NormalBufferSizeInBytes);
            ::memcpy_s(StagingBuffers [2u].MemoryAllocation->MappedAddress, StagingBuffers [2u].MemoryAllocation->SizeInBytes, MeshData->UVs.data(), UVBufferSizeInBytes);
            ::memcpy_s(StagingBuffers [3u].MemoryAllocation->MappedAddress, StagingBuffers [3u].MemoryAllocation->SizeInBytes, MeshData->Indices.data(), IndexBufferSizeInBytes);

            std::array<Vulkan::Resource::Buffer, 4u> Buffers = {};

            Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.VertexBuffers [MeshIndex], Buffers [0u]);
            Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.NormalBuffers [MeshIndex], Buffers [1u]);
            Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.UVBuffers [MeshIndex], Buffers [2u]);
            Vulkan::Resource::GetBuffer(Components::StaticMesh::StaticMeshes.IndexBuffers [MeshIndex], Buffers [3u]);

            std::array<VkBufferCopy, 4u> CopyRegions =
            {
                VkBufferCopy { 0u, 0u, VertexBufferSizeInBytes },
                VkBufferCopy { 0u, 0u, NormalBufferSizeInBytes },
                VkBufferCopy { 0u, 0u, UVBufferSizeInBytes },
                VkBufferCopy { 0u, 0u, IndexBufferSizeInBytes },
            };

            vkCmdCopyBuffer(CommandBuffer, StagingBuffers [0u].Resource, Buffers [0u].Resource, 1u, &CopyRegions [0u]);
            vkCmdCopyBuffer(CommandBuffer, StagingBuffers [1u].Resource, Buffers [1u].Resource, 1u, &CopyRegions [1u]);
            vkCmdCopyBuffer(CommandBuffer, StagingBuffers [2u].Resource, Buffers [2u].Resource, 1u, &CopyRegions [2u]);
            vkCmdCopyBuffer(CommandBuffer, StagingBuffers [3u].Resource, Buffers [3u].Resource, 1u, &CopyRegions [3u]);

            std::array<VkBufferMemoryBarrier, 4u> BufferBarriers =
            {
                Vulkan::BufferMemoryBarrier(Buffers [0u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
                Vulkan::BufferMemoryBarrier(Buffers [1u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
                Vulkan::BufferMemoryBarrier(Buffers [2u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT),
                Vulkan::BufferMemoryBarrier(Buffers [3u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT),
            };

            vkCmdPipelineBarrier(CommandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                 0u,
                                 0u, nullptr,
                                 static_cast<uint32>(BufferBarriers.size()), BufferBarriers.data(),
                                 0u, nullptr);

            Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [0u], TransferFence);
            Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [1u], TransferFence);
            Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [2u], TransferFence);
            Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [3u], TransferFence);

            StaticMesh::StaticMeshes.StatusFlags [MeshIndex].bPendingTransfer = false;
            StaticMesh::StaticMeshes.StatusFlags [MeshIndex].bReady = true;
        }
    }

    return bResult;
}

bool const StaticMesh::GetStaticMeshResources(uint32 const ActorID, StaticMesh::StaticMeshData & OutputStaticMeshData)
{
    bool bResult = false;

    if (ActorID == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot get static mesh resources for NULL actor."));
        return false;
    }

    /* TODO: Make ActorHandle a 32bit component mask and 32bit actor handle */

    uint32 const MeshIndex = StaticMesh::StaticMeshes.ActorIDToMeshIndex [ActorID];

    if (MeshIndex < StaticMesh::StaticMeshes.MeshAssetHandles.size())
    {
        OutputStaticMeshData.VertexBuffer = StaticMesh::StaticMeshes.VertexBuffers [MeshIndex];
        OutputStaticMeshData.NormalBuffer = StaticMesh::StaticMeshes.NormalBuffers [MeshIndex];
        OutputStaticMeshData.UVBuffer = StaticMesh::StaticMeshes.UVBuffers [MeshIndex];
        OutputStaticMeshData.IndexBuffer = StaticMesh::StaticMeshes.IndexBuffers [MeshIndex];
        OutputStaticMeshData.IndexCount = StaticMesh::StaticMeshes.IndexCounts [MeshIndex];

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

bool const StaticMesh::GetComponentStatus(uint32 const ActorID, StaticMeshStatus & OutputStatus)
{
    if (ActorID == 0u)
    {
        /* ERROR */
        return false;
    }

    bool bResult = false;

    auto FoundMesh = StaticMesh::StaticMeshes.ActorIDToMeshIndex.find(ActorID);
    if (FoundMesh == StaticMesh::StaticMeshes.ActorIDToMeshIndex.cend())
    {
        /* ERROR */
    }
    else
    {
        OutputStatus = StaticMesh::StaticMeshes.StatusFlags [FoundMesh->second];
    }

    return bResult;
}