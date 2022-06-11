#pragma once

#include "Primitives/StaticMesh.hpp"
#include "AssetManager.hpp"

#include <string>
#include <vector>

namespace Scene
{
    struct ActorData
    {
        std::string DebugName = {};
        AssetManager::AssetHandle<AssetManager::MeshAsset> MeshHandle = {};
        /* ... */
    };

    struct SceneData
    {
        std::unordered_map<std::string, uint32> ActorNameToIDMap = {};

        std::vector<uint32> NewActorIndices;

        std::vector<AssetManager::AssetHandle<AssetManager::MeshAsset>> ActorMeshHandles;
    };

    extern bool const CreateActor(SceneData & Scene, ActorData const & NewActorData);
}