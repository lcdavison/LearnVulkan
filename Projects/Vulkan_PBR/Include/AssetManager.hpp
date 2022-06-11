#pragma once

#include <string>
#include <vector>

#include "CommonTypes.hpp"

#include <Math/Vector.hpp>

namespace AssetManager
{
    template<typename TAssetType>
    struct AssetHandle
    {
        uint32 AssetIndex;
    };

    struct MeshAsset
    {
        std::vector<Math::Vector3> Vertices;
        std::vector<Math::Vector3> Normals;
        std::vector<uint32> Indices;
    };

    extern void Initialise();

    extern void Destroy();

    extern bool const LoadMeshAsset(std::string const & AssetName, AssetHandle<MeshAsset> & OutputAssetHandle);

    extern bool const GetMeshData(uint32 const AssetHandle, AssetManager::MeshAsset const *& OutputMeshData);

    extern bool const GetMeshData(AssetHandle<MeshAsset> const AssetHandle, AssetManager::MeshAsset const *& OutputMeshData);
}