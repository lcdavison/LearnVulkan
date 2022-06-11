#include "Scene.hpp"

bool const Scene::CreateActor(Scene::SceneData & Scene, Scene::ActorData const & ActorData)
{
    uint32 const NewActorIndex = { static_cast<uint32>(Scene.ActorMeshHandles.size() + 1u) };

    Scene.ActorNameToIDMap [ActorData.DebugName] = NewActorIndex;

    Scene.ActorMeshHandles.push_back(ActorData.MeshHandle);
    Scene.NewActorIndices.push_back(NewActorIndex);

    Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Creating New Actor [ID = %d]"), NewActorIndex));

    return true;
}