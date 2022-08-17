#include "Assets/StaticMesh.hpp"

#include <OBJLoader/OBJLoader.hpp>

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"

#include <vector>
#include <queue>
#include <unordered_map>

/* TODO: Write an allocator for more efficient buffer allocations with larger numbers of meshes */

struct StaticMeshCollection
{
    std::vector<std::unique_ptr<Math::Vector3[]>> RawVertexBuffers = {};
    std::vector<std::unique_ptr<Math::Vector3[]>> RawNormalBuffers = {};
    std::vector<std::unique_ptr<Math::Vector3[]>> RawUVBuffers = {};
    std::vector<std::unique_ptr<uint32[]>> RawIndexBuffers = {};

    std::vector<uint32> VertexCounts = {};
    std::vector<uint32> IndexCounts = {};

    std::vector<uint32> VertexBufferHandles = {};
    std::vector<uint32> NormalBufferHandles = {};
    std::vector<uint32> UVBufferHandles = {};
    std::vector<uint32> IndexBufferHandles = {};

    std::vector<Assets::StaticMesh::StaticMeshStatus> StatusBits = {};
};

static StaticMeshCollection StaticMeshes = {};

static std::queue<uint32> NewAssetHandles = {};

static void ProcessMeshData(OBJLoader::OBJMeshData const & OBJMeshData, 
                            Assets::StaticMesh::StaticMeshStatus & OutputStatus,
                            uint32 & OutputVertexCount,
                            uint32 & OutputIndexCount,
                            Math::Vector3 *& OutputVertices,
                            Math::Vector3 *& OutputNormals,
                            Math::Vector3 *& OutputUVs,
                            uint32 *& OutputIndices)
{
    /* Need to find a better way, rather than using a string */
    std::unordered_map<std::string, uint32> VertexLookUpTable = {};

    std::vector<Math::Vector3> Vertices = {};
    std::vector<Math::Vector3> Normals = {};
    std::vector<Math::Vector3> UVs = {};

    std::vector<uint32> Indices = {};

    OutputStatus.bHasNormals = OBJMeshData.FaceNormalIndices.size() > 0u;
    OutputStatus.bHasUVs = OBJMeshData.FaceTextureCoordinateIndices.size() > 0u;

    /* Create unique vertices based on face data */
    for (uint32 const CurrentFaceOffset : OBJMeshData.FaceOffsets)
    {
        /* Currently this just uses whatever winding order is used in the OBJ file */
        /* We could process the vertices further afterwards to set them to either CW or CCW */
        for (uint8 CurrentFaceVertexIndex = { 0u };
             CurrentFaceVertexIndex < 3u;
             CurrentFaceVertexIndex++)
        {
            uint32 const VertexIndex = OBJMeshData.FaceVertexIndices [CurrentFaceOffset + CurrentFaceVertexIndex];

            uint32 NormalIndex = {};
            if (OutputStatus.bHasNormals)
            {
                NormalIndex = OBJMeshData.FaceNormalIndices [CurrentFaceOffset + CurrentFaceVertexIndex];
            }

            uint32 TextureCoordinateIndex = {};
            if (OutputStatus.bHasUVs)
            {
                TextureCoordinateIndex = OBJMeshData.FaceTextureCoordinateIndices [CurrentFaceOffset + CurrentFaceVertexIndex];
            }

            uint32 Index = {};

            std::string IndexString = std::to_string(VertexIndex) + std::to_string(NormalIndex) + std::to_string(TextureCoordinateIndex);

            auto FoundVertex = VertexLookUpTable.find(IndexString);

            if (FoundVertex != VertexLookUpTable.end())
            {
                Index = FoundVertex->second;
            }
            else
            {
                Index = static_cast<uint32>(Vertices.size());

                OBJLoader::OBJVertex const & VertexData = OBJMeshData.Positions [VertexIndex - 1u];
                Vertices.push_back(Math::Vector3 { VertexData.X, VertexData.Y, VertexData.Z });

                if (OutputStatus.bHasNormals)
                {
                    OBJLoader::OBJNormal const & NormalData = OBJMeshData.Normals [NormalIndex - 1u];
                    Normals.push_back(Math::Vector3 { NormalData.X, NormalData.Y, NormalData.Z });
                }

                if (OutputStatus.bHasUVs)
                {
                    OBJLoader::OBJTextureCoordinate const & UVData = OBJMeshData.TextureCoordinates [TextureCoordinateIndex - 1u];
                    UVs.emplace_back(Math::Vector3 { UVData.U, UVData.V, UVData.W });
                }

                VertexLookUpTable [std::move(IndexString)] = Index;
            }

            Indices.push_back(Index);
        }
    }

    Vertices.shrink_to_fit();
    Normals.shrink_to_fit();
    UVs.shrink_to_fit();

    Indices.shrink_to_fit();

    /* 
        TBH we can probably just straight up write to this memory,
        though we would still overallocate and need to resize to shrink memory usage (or just remove these raw buffers completely, which we can do after the data is uploaded to the GPU)
    */
    OutputVertices = new Math::Vector3 [Vertices.size()];
    OutputNormals = new Math::Vector3 [Normals.size()];
    OutputUVs = new Math::Vector3 [UVs.size()];
    OutputIndices = new uint32 [Indices.size()];

    OutputVertexCount = static_cast<uint32>(Vertices.size());
    OutputIndexCount = static_cast<uint32>(Indices.size());

    std::copy(Vertices.cbegin(), Vertices.cend(), OutputVertices);
    std::copy(Normals.cbegin(), Normals.cend(), OutputNormals);
    std::copy(UVs.cbegin(), UVs.cend(), OutputUVs);
    std::copy(Indices.cbegin(), Indices.cend(), OutputIndices);
}

static void CreateGPUResources(uint32 const AssetHandle, Vulkan::Device::DeviceState const & DeviceState)
{
    /* TODO : Check static mesh flags before creating some of these buffers */

    uint32 const kAssetIndex = AssetHandle - 1u;
    uint64 const kVertexBufferSizeInBytes = { StaticMeshes.VertexCounts [kAssetIndex] * sizeof(Math::Vector3) };
    uint64 const kIndexBufferSizeInBytes = { StaticMeshes.IndexCounts [kAssetIndex] * sizeof(uint32) };

    Vulkan::Device::CreateBuffer(DeviceState, kVertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMeshes.VertexBufferHandles [kAssetIndex]);
    Vulkan::Device::CreateBuffer(DeviceState, kVertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMeshes.NormalBufferHandles [kAssetIndex]);
    Vulkan::Device::CreateBuffer(DeviceState, kVertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMeshes.UVBufferHandles [kAssetIndex]);
    Vulkan::Device::CreateBuffer(DeviceState, kIndexBufferSizeInBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, StaticMeshes.IndexBufferHandles [kAssetIndex]);
}

static void TransferToGPU(uint32 const AssetHandle, VkCommandBuffer CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence, std::vector<VkBufferMemoryBarrier> & OutputMemoryBarriers)
{
    uint32 const kAssetIndex = AssetHandle - 1u;
    uint64 const kVertexBufferSizeInBytes = { StaticMeshes.VertexCounts [kAssetIndex] * sizeof(Math::Vector3) };
    uint64 const kIndexBufferSizeInBytes = { StaticMeshes.IndexCounts [kAssetIndex] * sizeof(uint32) };

    std::array<uint32, 4u> StagingBufferHandles = {};
    Vulkan::Device::CreateBuffer(DeviceState, kVertexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [0u]);
    Vulkan::Device::CreateBuffer(DeviceState, kVertexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [1u]);
    Vulkan::Device::CreateBuffer(DeviceState, kVertexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [2u]);
    Vulkan::Device::CreateBuffer(DeviceState, kIndexBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandles [3u]);

    std::array<Vulkan::Resource::Buffer, 4u> StagingBuffers = {};
    Vulkan::Resource::GetBuffer(StagingBufferHandles [0u], StagingBuffers [0u]);
    Vulkan::Resource::GetBuffer(StagingBufferHandles [1u], StagingBuffers [1u]);
    Vulkan::Resource::GetBuffer(StagingBufferHandles [2u], StagingBuffers [2u]);
    Vulkan::Resource::GetBuffer(StagingBufferHandles [3u], StagingBuffers [3u]);

    ::memcpy_s(StagingBuffers [0u].MemoryAllocation->MappedAddress, StagingBuffers [0u].MemoryAllocation->SizeInBytes, StaticMeshes.RawVertexBuffers [kAssetIndex].get(), kVertexBufferSizeInBytes);
    ::memcpy_s(StagingBuffers [1u].MemoryAllocation->MappedAddress, StagingBuffers [1u].MemoryAllocation->SizeInBytes, StaticMeshes.RawNormalBuffers [kAssetIndex].get(), kVertexBufferSizeInBytes);
    ::memcpy_s(StagingBuffers [2u].MemoryAllocation->MappedAddress, StagingBuffers [2u].MemoryAllocation->SizeInBytes, StaticMeshes.RawUVBuffers [kAssetIndex].get(), kVertexBufferSizeInBytes);
    ::memcpy_s(StagingBuffers [3u].MemoryAllocation->MappedAddress, StagingBuffers [3u].MemoryAllocation->SizeInBytes, StaticMeshes.RawIndexBuffers [kAssetIndex].get(), kIndexBufferSizeInBytes);
    
    std::array<Vulkan::Resource::Buffer, 4u> MeshBuffers = {};
    Vulkan::Resource::GetBuffer(StaticMeshes.VertexBufferHandles [kAssetIndex], MeshBuffers [0u]);
    Vulkan::Resource::GetBuffer(StaticMeshes.NormalBufferHandles [kAssetIndex], MeshBuffers [1u]);
    Vulkan::Resource::GetBuffer(StaticMeshes.UVBufferHandles [kAssetIndex], MeshBuffers [2u]);
    Vulkan::Resource::GetBuffer(StaticMeshes.IndexBufferHandles [kAssetIndex], MeshBuffers [3u]);

    std::array<VkBufferCopy, 4u> const CopyRegions =
    {
        VkBufferCopy { 0u, 0u, kVertexBufferSizeInBytes },
        VkBufferCopy { 0u, 0u, kVertexBufferSizeInBytes },
        VkBufferCopy { 0u, 0u, kVertexBufferSizeInBytes },
        VkBufferCopy { 0u, 0u, kIndexBufferSizeInBytes },
    };

    vkCmdCopyBuffer(CommandBuffer, StagingBuffers [0u].Resource, MeshBuffers [0u].Resource, 1u, &CopyRegions [0u]);
    vkCmdCopyBuffer(CommandBuffer, StagingBuffers [1u].Resource, MeshBuffers [1u].Resource, 1u, &CopyRegions [1u]);
    vkCmdCopyBuffer(CommandBuffer, StagingBuffers [2u].Resource, MeshBuffers [2u].Resource, 1u, &CopyRegions [2u]);
    vkCmdCopyBuffer(CommandBuffer, StagingBuffers [3u].Resource, MeshBuffers [3u].Resource, 1u, &CopyRegions [3u]);

    /* Make the transferred data available and visible to vertex/index reads */
    OutputMemoryBarriers.emplace_back(Vulkan::BufferMemoryBarrier(MeshBuffers [0u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT));
    OutputMemoryBarriers.emplace_back(Vulkan::BufferMemoryBarrier(MeshBuffers [1u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT));
    OutputMemoryBarriers.emplace_back(Vulkan::BufferMemoryBarrier(MeshBuffers [2u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT));
    OutputMemoryBarriers.emplace_back(Vulkan::BufferMemoryBarrier(MeshBuffers [3u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT));

    Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [0u], TransferFence);
    Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [1u], TransferFence);
    Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [2u], TransferFence);
    Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandles [3u], TransferFence);
}

bool const Assets::StaticMesh::ImportStaticMesh(std::filesystem::path const & FilePath, std::string AssetName, uint32 & OutputAssetHandle)
{
    bool bResult = false;

    /* Only support obj files atm */
    if (FilePath.extension() == ".obj")
    {
        OBJLoader::OBJMeshData MeshData = {};
        std::vector<OBJLoader::OBJMaterialData> Materials = {};
        bResult = OBJLoader::LoadFile(FilePath, MeshData, Materials);

        if (bResult)
        {
            Math::Vector3 * Vertices = {};
            Math::Vector3 * Normals = {};
            Math::Vector3 * UVs = {};
            uint32 * Indices = {};

            Assets::StaticMesh::StaticMeshStatus Status = {};
            uint32 VertexCount = {};
            uint32 IndexCount = {};

            ::ProcessMeshData(MeshData,
                              Status,
                              VertexCount,
                              IndexCount,
                              Vertices,
                              Normals,
                              UVs,
                              Indices);

            std::unique_ptr<Math::Vector3[]> & RawVertexBuffer = StaticMeshes.RawVertexBuffers.emplace_back();
            std::unique_ptr<Math::Vector3[]> & RawNormalBuffer = StaticMeshes.RawNormalBuffers.emplace_back();
            std::unique_ptr<Math::Vector3[]> & RawUVBuffer = StaticMeshes.RawUVBuffers.emplace_back();
            std::unique_ptr<uint32[]> & RawIndexBuffer = StaticMeshes.RawIndexBuffers.emplace_back();

            RawVertexBuffer.reset(Vertices);
            RawNormalBuffer.reset(Normals);
            RawUVBuffer.reset(UVs);
            RawIndexBuffer.reset(Indices);

            StaticMeshes.StatusBits.push_back(Status);
            StaticMeshes.VertexCounts.push_back(VertexCount);
            StaticMeshes.IndexCounts.push_back(IndexCount);

            StaticMeshes.VertexBufferHandles.emplace_back();
            StaticMeshes.NormalBufferHandles.emplace_back();
            StaticMeshes.UVBufferHandles.emplace_back();
            StaticMeshes.IndexBufferHandles.emplace_back();

            OutputAssetHandle = static_cast<uint32>(StaticMeshes.RawVertexBuffers.size());
            NewAssetHandles.push(OutputAssetHandle);
        }
    }

    return bResult;
}

bool const Assets::StaticMesh::InitialiseGPUResources(VkCommandBuffer CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence)
{
    /* Again a memory allocator would be good for this stuff on a frame by frame basis */
    std::vector<VkBufferMemoryBarrier> MemoryBarriers = {};

    while (NewAssetHandles.size() > 0u)
    {
        uint32 const AssetHandle = NewAssetHandles.front();

        ::CreateGPUResources(AssetHandle, DeviceState);
        
        ::TransferToGPU(AssetHandle, CommandBuffer, DeviceState, TransferFence, MemoryBarriers);

        NewAssetHandles.pop();
    }

    vkCmdPipelineBarrier(CommandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0u, nullptr,
                         static_cast<uint32>(MemoryBarriers.size()), MemoryBarriers.data(),
                         0u, nullptr);

    return true;
}

bool const Assets::StaticMesh::GetAssetData(uint32 const AssetHandle, Assets::StaticMesh::AssetData & OutputAssetData)
{
    if (AssetHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint32 const kAssetIndex = { AssetHandle - 1u };

    OutputAssetData.VertexData = StaticMeshes.RawVertexBuffers [kAssetIndex].get();
    OutputAssetData.NormalData = StaticMeshes.RawNormalBuffers [kAssetIndex].get();
    OutputAssetData.UVData = StaticMeshes.RawUVBuffers [kAssetIndex].get();
    OutputAssetData.IndexData = StaticMeshes.RawIndexBuffers [kAssetIndex].get();

    OutputAssetData.VertexBufferHandle = StaticMeshes.VertexBufferHandles [kAssetIndex];
    OutputAssetData.NormalBufferHandle = StaticMeshes.NormalBufferHandles [kAssetIndex];
    OutputAssetData.UVBufferHandle = StaticMeshes.UVBufferHandles [kAssetIndex];
    OutputAssetData.IndexBufferHandle = StaticMeshes.IndexBufferHandles [kAssetIndex];

    OutputAssetData.VertexCount = StaticMeshes.VertexCounts [kAssetIndex];
    OutputAssetData.IndexCount = StaticMeshes.IndexCounts [kAssetIndex];

    OutputAssetData.Status = StaticMeshes.StatusBits [kAssetIndex];

    return true;
}