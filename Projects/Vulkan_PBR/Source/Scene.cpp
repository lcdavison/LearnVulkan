#include "Scene.hpp"

#include "Common.hpp"
#include "Logging.hpp"

bool const Scene::CreateActor(Scene::SceneData & Scene, Scene::ActorData const & ActorData, uint32 & OutputActorHandle)
{
    uint32 NewActorHandle = {};

    if (Scene.FreeActorHandles.size() == 0u)
    {
        Scene.ActorCount++;
        NewActorHandle = Scene.ActorCount;

        Scene.ComponentMasks.emplace_back();
    }
    else
    {
        NewActorHandle = Scene.FreeActorHandles.front();
        Scene.FreeActorHandles.pop();
    }

    Scene.NewActorHandles.push_back(NewActorHandle);
    Scene.ActorNameToHandleMap [ActorData.DebugName] = NewActorHandle;

    OutputActorHandle = NewActorHandle;

    Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Creating New Actor [ID = %d]"), NewActorHandle));

    return true;
}

bool const Scene::DoesActorHaveComponent(Scene::SceneData const & Scene, uint32 const ActorHandle, Scene::ComponentMasks const ComponentMask)
{
    uint32 const ActorIndex = ActorHandle - 1u;
    return (Scene.ComponentMasks [ActorIndex] & static_cast<uint32>(ComponentMask)) > 0u;
}