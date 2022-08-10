#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "Common.hpp"

#include <Math/Vector.hpp>

namespace AssetManager
{
    struct MeshAsset
    {
        std::vector<Math::Vector3> Vertices = {};
        std::vector<Math::Vector3> Normals = {};
        std::vector<Math::Vector3> UVs = {}; // TODO: Implement vector2 for these 
        std::vector<uint32> Indices = {};
    };

    extern void Initialise();

    extern void Destroy();

    extern bool const LoadMeshAsset(std::filesystem::path const & RelativeFilePath, uint32 & OutputAssetHandle);

    extern bool const UnloadMeshAsset(uint32 const AssetHandle);

    extern bool const GetMeshAsset(uint32 const AssetHandle, MeshAsset const *& OutputMeshData);
}