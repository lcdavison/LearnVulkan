#include "Components/StaticMeshComponent.hpp"

#include "Scene.hpp"

#include <array>
#include <unordered_map>
#include <vector>

using namespace Components;

namespace Components::StaticMesh::Private
{
    struct StaticMeshCollection
    {
        std::unordered_map<uint32, uint32> ActorIDToMeshIndex = {};

        std::vector<uint32> ParentActorHandles = {};
        std::vector<uint32> MeshHandles = {};
        std::vector<uint32> MaterialHandles = {};
    };
}

static StaticMesh::Private::StaticMeshCollection StaticMeshes = {};

StaticMesh::Types::Iterator::Iterator(uint32 const Index)
    : Utilities::IteratorBase(Index)
{
}

StaticMesh::Types::Iterator StaticMesh::Types::Iterator::Begin()
{
    return StaticMesh::Types::Iterator(0u);
}

StaticMesh::Types::Iterator StaticMesh::Types::Iterator::End()
{
    return StaticMesh::Types::Iterator(static_cast<uint32>(StaticMeshes.MeshHandles.size()));
}

StaticMesh::Types::ComponentData StaticMesh::Types::Iterator::operator * () const
{
    return ComponentData
    {
        StaticMeshes.ParentActorHandles [CurrentIndex_],
        StaticMeshes.MeshHandles [CurrentIndex_],
        StaticMeshes.MaterialHandles [CurrentIndex_],
    };
}

bool const StaticMesh::CreateComponent(Scene::SceneData & Scene, uint32 const ActorHandle, uint32 const StaticMeshHandle, uint32 const MaterialHandle)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create static mesh component for NULL actor handle."));
        return false;
    }

    uint32 const NewMeshIndex = static_cast<uint32>(StaticMeshes.MeshHandles.size());
    StaticMeshes.ActorIDToMeshIndex [ActorHandle] = NewMeshIndex;

    StaticMeshes.ParentActorHandles.push_back(ActorHandle);
    StaticMeshes.MeshHandles.push_back(StaticMeshHandle);
    StaticMeshes.MaterialHandles.push_back(MaterialHandle);

    uint32 const kActorIndex = ActorHandle - 1u;
    Scene.ComponentMasks [kActorIndex] |= static_cast<uint32>(Scene::ComponentMasks::StaticMesh);

    return true;
}

bool const StaticMesh::GetComponentData(Scene::SceneData const & Scene, uint32 const ActorHandle, StaticMesh::Types::ComponentData & OutputComponentData)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to get static mesh component data for NULL handle."));
        return false;
    }

    uint32 const kActorIndex = ActorHandle - 1u;

    if ((Scene.ComponentMasks [kActorIndex] & static_cast<uint32>(Scene::ComponentMasks::StaticMesh)) == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to get component data. Provided actor does not have a static mesh component."));
        return false;
    }

    uint32 const kComponentIndex = StaticMeshes.ActorIDToMeshIndex [kActorIndex];
    OutputComponentData.ParentActorHandle = StaticMeshes.ParentActorHandles [kComponentIndex];
    OutputComponentData.MeshHandle = StaticMeshes.MeshHandles [kComponentIndex];
    OutputComponentData.MaterialHandle = StaticMeshes.MaterialHandles [kComponentIndex];

    return true;
}