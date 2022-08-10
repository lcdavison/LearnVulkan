#pragma once

#include "AssetManager.hpp"

#include "Camera.hpp"

#include <unordered_map>
#include <string>
#include <vector>
#include <queue>

namespace Scene
{
    struct ActorData
    {
        std::string DebugName = {};
        /* ... */
    };

    enum class ComponentMasks : uint32
    {
        Transform = 0x1,
        StaticMesh = Transform << 1u,
        Material = StaticMesh << 1u,
    };

    struct SceneData
    {
        std::unordered_map<std::string, uint32> ActorNameToHandleMap = {};

        std::queue<uint32> FreeActorHandles = {};

        std::vector<uint32> ComponentMasks = {};
        std::vector<uint32> NewActorHandles = {};

        Camera::CameraState MainCamera = {};

        uint32 ActorCount = {};
    };

    extern bool const CreateActor(SceneData & Scene, ActorData const & NewActorData, uint32 & OutputActorHandle);

    //extern bool const DestroyActor(SceneData & Scene, uint32 const ActorHandle);

    extern bool const DoesActorHaveComponents(SceneData const & Scene, uint32 const ActorHandle, uint32 const ComponentMask);
}