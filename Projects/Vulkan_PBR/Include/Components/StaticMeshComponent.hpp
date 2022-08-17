#pragma once

#include "Common.hpp"

#include <vector>
#include <unordered_map>

namespace Scene
{
    struct SceneData;
}

namespace Components::StaticMesh
{
    struct StaticMeshCollection
    {
        std::unordered_map<uint32, uint32> ActorIDToMeshIndex = {};

        std::vector<uint32> MeshAssetHandles = {};
    };

    struct ComponentData
    {
        uint32 AssetHandle = {};
    };

    extern StaticMeshCollection StaticMeshes;

    extern bool const CreateComponent(Scene::SceneData & Scene, uint32 const ActorHandle, uint32 const AssetHandle);

    extern bool const GetComponentData(Scene::SceneData const & Scene, uint32 const ActorHandle, ComponentData & OutputComponentData);
}