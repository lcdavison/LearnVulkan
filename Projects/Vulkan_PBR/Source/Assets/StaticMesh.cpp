#include "Assets/StaticMesh.hpp"

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"

#include <Math/Vector.hpp>
#include <OBJLoader/OBJLoader.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace Assets::StaticMesh::Private
{
    static std::vector StaticMeshes = std::vector<Assets::StaticMesh::Types::StaticMesh>();
    static std::vector NewAssetHandles = std::vector<uint32>();
}

static void GenerateTangentVectors(std::vector<Math::Vector3> const & kVertices, 
                                   std::vector<Math::Vector3> const & kNormals, 
                                   std::vector<Math::Vector3> const & kUVs, 
                                   std::vector<uint32> const & kMeshIndices, 
                                   std::vector<Math::Vector4> & OutputTangents)
{
    std::vector TangentVectors = std::vector<Math::Vector4>(kVertices.size());
    std::vector BitangentVectors = std::vector<Math::Vector3>(kVertices.size());

    for (uint32 CurrentIndexOffset = {};
         CurrentIndexOffset < kMeshIndices.size();
         CurrentIndexOffset += 3u)
    {
        std::array const kIndices = std::array<uint32, 3u>
        {
            kMeshIndices [CurrentIndexOffset + 0u],
            kMeshIndices [CurrentIndexOffset + 1u],
            kMeshIndices [CurrentIndexOffset + 2u],
        };

        std::array const kEdges = std::array<Math::Vector3, 2u>
        {
            kVertices [kIndices [1u]] - kVertices [kIndices [0u]],
            kVertices [kIndices [2u]] - kVertices [kIndices [0u]],
        };

        std::array const kComponentDifferences = std::array<float, 4u>
        {
            kUVs [kIndices [1u]].X - kUVs [kIndices [0u]].X, kUVs [kIndices [2u]].X - kUVs [kIndices [0u]].X,
            kUVs [kIndices [1u]].Y - kUVs [kIndices [0u]].Y, kUVs [kIndices [2u]].Y - kUVs [kIndices [0u]].Y,
        };

        /* TODO: Need to implement a division operator for the vectors */
        float const kInverseDeterminant = 1.0f / (kComponentDifferences [0u] * kComponentDifferences [3u] - kComponentDifferences [1u] * kComponentDifferences [2u]);

        Math::Vector3 const kTangentVector = (kEdges [0u] * kComponentDifferences [3u] - kEdges [1u] * kComponentDifferences [2u]) * kInverseDeterminant;
        Math::Vector3 const kBitangentVector = (kEdges [1u] * kComponentDifferences [0u] - kEdges [0u] * kComponentDifferences [1u]) * kInverseDeterminant;

        std::array const kOutputTangentVectors = std::array<Math::Vector3 *, 3u>
        {
            reinterpret_cast<Math::Vector3 *>(&(TangentVectors [kIndices [0u]])),
            reinterpret_cast<Math::Vector3 *>(&(TangentVectors [kIndices [1u]])),
            reinterpret_cast<Math::Vector3 *>(&(TangentVectors [kIndices [2u]])),
        };

        *kOutputTangentVectors [0u] = *kOutputTangentVectors [0u] + kTangentVector;
        *kOutputTangentVectors [1u] = *kOutputTangentVectors [1u] + kTangentVector;
        *kOutputTangentVectors [2u] = *kOutputTangentVectors [2u] + kTangentVector;

        BitangentVectors [kIndices [0u]] = BitangentVectors [kIndices [0u]] + kBitangentVector;
        BitangentVectors [kIndices [1u]] = BitangentVectors [kIndices [1u]] + kBitangentVector;
        BitangentVectors [kIndices [2u]] = BitangentVectors [kIndices [2u]] + kBitangentVector;
    }

    for (uint32 VertexIndex = { 0u };
         VertexIndex < kVertices.size();
         VertexIndex++)
    {
        Math::Vector3 & TangentVector = reinterpret_cast<Math::Vector3 &>(TangentVectors [VertexIndex]);
        Math::Vector3 & BitangentVector = BitangentVectors [VertexIndex];

        /* Make sure tangent vector is perpendicular to the normal vector */
        Math::Vector3 const kAdjustedTangent = TangentVector - (TangentVector * kNormals [VertexIndex]) * kNormals [VertexIndex];
        TangentVector = Math::Vector3::Normalize(kAdjustedTangent);

        /* Make sure the bitangent vector is perpendicular to both the normal and tangent vectors */
        Math::Vector3 const kAdjustedBitangent = BitangentVector - ((BitangentVector * TangentVector) * TangentVector) - ((BitangentVector * kNormals [VertexIndex]) * kNormals [VertexIndex]);
        BitangentVector = Math::Vector3::Normalize(kAdjustedBitangent);

        /*
        *   Cross product between tangent and bitangent to get normal.
        *   Use dot product of result with normal to determine the sign.
        *   Use this sign in the shader to flip the bitangent, this ensures consistency.
        */
        TangentVectors [VertexIndex].W = ((TangentVector ^ BitangentVector) * kNormals [VertexIndex]) > 0.0f ? 1.0f : -1.0f;
    }

    OutputTangents = std::move(TangentVectors);
}

static void ProcessMeshData(OBJLoader::OBJMeshData const & kOBJMeshData, Assets::StaticMesh::Types::StaticMesh & OutputStaticMesh)
{
    /* Need to find a better way, rather than using a string */
    std::unordered_map<std::string, uint32> VertexLookUpTable = {};

    std::vector<Math::Vector3> Vertices = {};
    std::vector<Math::Vector3> Normals = {};
    std::vector<Math::Vector3> UVs = {};

    std::vector<uint32> Indices = {};

    OutputStaticMesh.Status.bHasNormals = kOBJMeshData.FaceNormalIndices.size() > 0u;
    OutputStaticMesh.Status.bHasUVs = kOBJMeshData.FaceTextureCoordinateIndices.size() > 0u;

    for (uint32 const kFaceOffset : kOBJMeshData.FaceOffsets)
    {
        for (uint8 FaceVertexOffset = {};
             FaceVertexOffset < 3u; // may not necessarily be 3 vertices to a face (Quads instead of Tris), so this will need to be changed some time
             FaceVertexOffset++)
        {
            uint32 const kVertexIndex = kOBJMeshData.FaceVertexIndices [kFaceOffset + FaceVertexOffset];

            uint32 const kNormalIndex = OutputStaticMesh.Status.bHasNormals
                ? kOBJMeshData.FaceNormalIndices [kFaceOffset + FaceVertexOffset]
                : 0u;

            uint32 const kUVIndex = OutputStaticMesh.Status.bHasUVs
                ? kOBJMeshData.FaceTextureCoordinateIndices [kFaceOffset + FaceVertexOffset]
                : 0u;

            uint32 IndexBufferValue = {};

            std::string IndexString = std::to_string(kVertexIndex) + std::to_string(kNormalIndex) + std::to_string(kUVIndex);

            decltype(VertexLookUpTable)::const_iterator const kVertexIterator = VertexLookUpTable.find(IndexString);

            if (kVertexIterator != VertexLookUpTable.cend())
            {
                auto const & [Key, VertexIndex] = *kVertexIterator;
                IndexBufferValue = VertexIndex;
            }
            else
            {
                IndexBufferValue = static_cast<uint32>(Vertices.size());

                OBJLoader::OBJVertex const & kVertex = kOBJMeshData.Positions [kVertexIndex - 1u];
                Vertices.push_back(Math::Vector3 { kVertex.X, kVertex.Y, kVertex.Z });

                if (OutputStaticMesh.Status.bHasNormals)
                {
                    OBJLoader::OBJNormal const & kNormal = kOBJMeshData.Normals [kNormalIndex - 1u];
                    Normals.push_back(Math::Vector3 { kNormal.X, kNormal.Y, kNormal.Z });
                }

                if (OutputStaticMesh.Status.bHasUVs)
                {
                    OBJLoader::OBJTextureCoordinate const & kUV = kOBJMeshData.TextureCoordinates [kUVIndex - 1u];
                    UVs.emplace_back(Math::Vector3 { kUV.U, kUV.V, kUV.W });
                }

                VertexLookUpTable [std::move(IndexString)] = IndexBufferValue;
            }

            Indices.push_back(IndexBufferValue);
        }
    }

    std::vector Tangents = std::vector<Math::Vector4>();
    ::GenerateTangentVectors(Vertices, Normals, UVs, Indices, Tangents);

    OutputStaticMesh.VertexCount = static_cast<uint32>(Vertices.size());
    OutputStaticMesh.IndexCount = static_cast<uint32>(Indices.size());

    uint64 const kVertexDataSizeInBytes = { Vertices.size() * sizeof(Math::Vector3) };
    uint64 const kTangentDataSizeInBytes = { Tangents.size() * sizeof(Math::Vector4) };

    OutputStaticMesh.MeshDataSizeInBytes = kVertexDataSizeInBytes * 3u + Tangents.size() * sizeof(Math::Vector4);

    OutputStaticMesh.MeshData = new std::byte [OutputStaticMesh.MeshDataSizeInBytes];
    OutputStaticMesh.IndexData = new uint32 [OutputStaticMesh.IndexCount];

    OutputStaticMesh.NormalDataOffsetInBytes = kVertexDataSizeInBytes;
    OutputStaticMesh.TangentDataOffsetInBytes = OutputStaticMesh.NormalDataOffsetInBytes + kVertexDataSizeInBytes;
    OutputStaticMesh.UVDataOffsetInBytes = OutputStaticMesh.TangentDataOffsetInBytes + kTangentDataSizeInBytes;

    Math::Vector3 * const kVertexData = reinterpret_cast<Math::Vector3 *>(OutputStaticMesh.MeshData);
    Math::Vector3 * const kNormalData = reinterpret_cast<Math::Vector3 *>(OutputStaticMesh.MeshData + OutputStaticMesh.NormalDataOffsetInBytes);
    Math::Vector4 * const kTangentData = reinterpret_cast<Math::Vector4 *>(OutputStaticMesh.MeshData + OutputStaticMesh.TangentDataOffsetInBytes);
    Math::Vector3 * const kUVData = reinterpret_cast<Math::Vector3 *>(OutputStaticMesh.MeshData + OutputStaticMesh.UVDataOffsetInBytes);

    /* Vertices | Normals | Tangents | UVs */
    std::copy(Vertices.cbegin(), Vertices.cend(), kVertexData);
    std::copy(Normals.cbegin(), Normals.cend(), kNormalData);
    std::copy(Tangents.cbegin(), Tangents.cend(), kTangentData);
    std::copy(UVs.cbegin(), UVs.cend(), kUVData);

    std::copy(Indices.cbegin(), Indices.cend(), OutputStaticMesh.IndexData);
}

static void CreateGPUResources(uint32 const kAssetIndex, Vulkan::Device::DeviceState const & kDeviceState)
{
    using namespace Assets::StaticMesh;
    /* TODO : Check static mesh flags before creating some of these buffers */

    Types::StaticMesh & StaticMesh = Private::StaticMeshes [kAssetIndex];

    uint64 const kIndexBufferSizeInBytes = { StaticMesh.IndexCount * sizeof(uint32) };

    Vulkan::Resource::BufferDescriptor BufferDesc = Vulkan::Resource::BufferDescriptor
    {
        StaticMesh.MeshDataSizeInBytes,
        0u,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    Vulkan::Device::CreateBuffer(kDeviceState, BufferDesc, StaticMesh.MeshBufferHandle);

    BufferDesc.SizeInBytes = kIndexBufferSizeInBytes;
    BufferDesc.UsageFlags ^= (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    Vulkan::Device::CreateBuffer(kDeviceState, BufferDesc, StaticMesh.IndexBufferHandle);
}

static void TransferToGPU(uint32 const kAssetIndex, VkCommandBuffer const kCommandBuffer, Vulkan::Device::DeviceState const & kDeviceState, VkFence const kTransferFence, std::vector<VkBufferMemoryBarrier> & OutputMemoryBarriers)
{
    using namespace Assets::StaticMesh;

    Types::StaticMesh const & StaticMesh = Private::StaticMeshes [kAssetIndex];

    uint64 const kIndexBufferSizeInBytes = { StaticMesh.IndexCount * sizeof(uint32) };

    Vulkan::Resource::BufferDescriptor BufferDesc = Vulkan::Resource::BufferDescriptor
    {
        StaticMesh.MeshDataSizeInBytes,
        0u,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    std::array StagingBufferHandles = std::array<uint32, 2u>();

    Vulkan::Device::CreateBuffer(kDeviceState, BufferDesc, StagingBufferHandles [0u]);

    BufferDesc.SizeInBytes = kIndexBufferSizeInBytes;

    Vulkan::Device::CreateBuffer(kDeviceState, BufferDesc, StagingBufferHandles [1u]);

    std::array StagingBuffers = std::array<Vulkan::Resource::Buffer, 2u>();
    Vulkan::Resource::GetBuffer(StagingBufferHandles [0u], StagingBuffers [0u]);
    Vulkan::Resource::GetBuffer(StagingBufferHandles [1u], StagingBuffers [1u]);

    std::array AllocationInfos = std::array<Vulkan::Memory::AllocationInfo, 2u>();
    Vulkan::Memory::GetAllocationInfo(StagingBuffers [0u].MemoryAllocationHandle, AllocationInfos [0u]);
    Vulkan::Memory::GetAllocationInfo(StagingBuffers [1u].MemoryAllocationHandle, AllocationInfos [1u]);

    std::memcpy(AllocationInfos [0u].MappedAddress, StaticMesh.MeshData, StaticMesh.MeshDataSizeInBytes);
    std::memcpy(AllocationInfos [1u].MappedAddress, StaticMesh.IndexData, kIndexBufferSizeInBytes);
    
    std::array MeshBuffers = std::array<Vulkan::Resource::Buffer, 2u>();
    Vulkan::Resource::GetBuffer(StaticMesh.MeshBufferHandle, MeshBuffers [0u]);
    Vulkan::Resource::GetBuffer(StaticMesh.IndexBufferHandle, MeshBuffers [1u]);

    std::array const kCopyRegions = std::array<VkBufferCopy, 2u>
    {
        VkBufferCopy { 0u, 0u, StaticMesh.MeshDataSizeInBytes },
        VkBufferCopy { 0u, 0u, kIndexBufferSizeInBytes },
    };

    vkCmdCopyBuffer(kCommandBuffer, StagingBuffers [0u].Resource, MeshBuffers [0u].Resource, 1u, &kCopyRegions [0u]);
    vkCmdCopyBuffer(kCommandBuffer, StagingBuffers [1u].Resource, MeshBuffers [1u].Resource, 1u, &kCopyRegions [1u]);

    /* Make the transferred data available and visible to vertex/index reads */
    OutputMemoryBarriers.emplace_back(Vulkan::BufferMemoryBarrier(MeshBuffers [0u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT));
    OutputMemoryBarriers.emplace_back(Vulkan::BufferMemoryBarrier(MeshBuffers [1u].Resource, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT));

    Vulkan::Device::DestroyBuffer(kDeviceState, StagingBufferHandles [0u], kTransferFence);
    Vulkan::Device::DestroyBuffer(kDeviceState, StagingBufferHandles [1u], kTransferFence);
}

bool const Assets::StaticMesh::ImportStaticMesh(std::filesystem::path const & kFilePath, std::string AssetName, uint32 & OutputAssetHandle)
{
    using namespace Assets::StaticMesh;

    bool bResult = false;

    if (kFilePath.has_extension() && kFilePath.extension() == ".obj")
    {
        OBJLoader::OBJMeshData MeshData = {};
        std::vector Materials = std::vector<OBJLoader::OBJMaterialData>();

        bResult = OBJLoader::LoadFile(kFilePath, MeshData, Materials);

        Types::StaticMesh & StaticMeshData = Private::StaticMeshes.emplace_back();
        ::ProcessMeshData(MeshData, StaticMeshData);

        OutputAssetHandle = { static_cast<uint32>(Private::StaticMeshes.size()) };
        Private::NewAssetHandles.push_back(OutputAssetHandle);
    }

    return bResult;
}

bool const Assets::StaticMesh::InitialiseGPUResources(VkCommandBuffer const kCommandBuffer, Vulkan::Device::DeviceState const & kDeviceState, VkFence const kTransferFence)
{
    std::vector MemoryBarriers = std::vector<VkBufferMemoryBarrier>();

    for (uint32 CurrentAssetIndex = {};
         CurrentAssetIndex < Private::NewAssetHandles.size();
         CurrentAssetIndex++)
    {
        uint32 const AssetIndex = Private::NewAssetHandles [CurrentAssetIndex] - 1u;

        ::CreateGPUResources(AssetIndex, kDeviceState);

        ::TransferToGPU(AssetIndex, kCommandBuffer, kDeviceState, kTransferFence, MemoryBarriers);
    }

    Private::NewAssetHandles.clear();

    if (MemoryBarriers.size() > 0u)
    {
        vkCmdPipelineBarrier(kCommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                             0u,
                             0u, nullptr,
                             static_cast<uint32>(MemoryBarriers.size()), MemoryBarriers.data(),
                             0u, nullptr);
    }

    return true;
}

bool const Assets::StaticMesh::GetAssetData(uint32 const kAssetHandle, Assets::StaticMesh::Types::StaticMesh & OutputStaticMesh)
{
    if (kAssetHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to get static mesh for NULL handle."));
        return false;
    }

    uint32 const kAssetIndex = { kAssetHandle - 1u };

    OutputStaticMesh = Private::StaticMeshes [kAssetIndex]; sizeof(Assets::StaticMesh::Types::StaticMesh);

    return true;
}