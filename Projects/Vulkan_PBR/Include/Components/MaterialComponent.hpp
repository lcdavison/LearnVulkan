#pragma once

#include "Common.hpp"

namespace Scene
{
    struct SceneData;
}

namespace Components::Material
{
    struct ComponentData
    {
        uint32 MaterialHandle = {};
    };

    extern bool const CreateComponent(Scene::SceneData & Scene, uint32 const ActorHandle, uint32 const MaterialHandle);

    extern bool const GetComponentData(Scene::SceneData const & Scene, uint32 const ActorHandle, ComponentData & OutputComponentData);
}