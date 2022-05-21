#pragma once

#include <string>
#include <vector>

#include "CommonTypes.hpp"

#include <Math/Vector.hpp>

namespace AssetManager
{
    struct MeshAsset
    {
        std::vector<Math::Vector3> Vertices;
        std::vector<Math::Vector3> Normals;
        std::vector<uint32> Indices;
    };

    extern void Initialise();

    /* TODO: Do this async */
    extern bool const LoadMeshAsset(std::string const & AssetName, uint32 & OutputAssetHandle);

    extern bool const GetMeshData(uint32 const AssetHandle, AssetManager::MeshAsset const *& OutputMeshData);

    extern bool const GetMeshData(std::string const & AssetName, AssetManager::MeshAsset const *& OutputMeshData);
}