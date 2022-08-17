#include "Components/StaticMeshComponent.hpp"

#include "Scene.hpp"

#include <array>

using namespace Components;

StaticMesh::StaticMeshCollection Components::StaticMesh::StaticMeshes = {};

bool const StaticMesh::CreateComponent(Scene::SceneData & Scene, uint32 const ActorHandle, uint32 const AssetHandle)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create static mesh component for NULL actor handle."));
        return false;
    }

    uint32 const NewMeshIndex = static_cast<uint32>(StaticMesh::StaticMeshes.MeshAssetHandles.size());
    StaticMesh::StaticMeshes.ActorIDToMeshIndex [ActorHandle] = NewMeshIndex;

    StaticMesh::StaticMeshes.MeshAssetHandles.push_back(AssetHandle);

    uint32 const kActorIndex = ActorHandle - 1u;
    Scene.ComponentMasks [kActorIndex] |= static_cast<uint32>(Scene::ComponentMasks::StaticMesh);

    return true;
}

bool const StaticMesh::GetComponentData(Scene::SceneData const & Scene, uint32 const ActorHandle, StaticMesh::ComponentData & OutputComponentData)
{
    if (ActorHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint32 const kActorIndex = ActorHandle - 1u;

    if ((Scene.ComponentMasks [kActorIndex] & static_cast<uint32>(Scene::ComponentMasks::StaticMesh)) == 0u)
    {
        /* ERROR */
        return false;
    }

    uint32 const kComponentIndex = StaticMesh::StaticMeshes.ActorIDToMeshIndex [kActorIndex];
    OutputComponentData.AssetHandle = StaticMesh::StaticMeshes.MeshAssetHandles [kComponentIndex];

    return true;
}
