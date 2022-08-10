#include "Components/MaterialComponent.hpp"

#include "Scene.hpp"

#include <vector>
#include <unordered_map>

struct ComponentCollection
{
    std::unordered_map<uint32, uint32> ActorToComponentMap = {};

    std::vector<uint32> MaterialHandles = {};
};

static ComponentCollection MaterialComponents = {};

bool const Components::Material::CreateComponent(Scene::SceneData & Scene, uint32 const ActorHandle, uint32 const MaterialHandle)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create material component for NULL actor."));
        return false;
    }

    uint32 const ComponentIndex = static_cast<uint32>(MaterialComponents.MaterialHandles.size());
    MaterialComponents.MaterialHandles.push_back(MaterialHandle);

    MaterialComponents.ActorToComponentMap [ActorHandle] = ComponentIndex;

    Scene.ComponentMasks [ActorHandle - 1u] |= static_cast<uint32>(Scene::ComponentMasks::Material);

    return true;
}

bool const Components::Material::GetComponentData(Scene::SceneData const & Scene, uint32 const ActorHandle, Components::Material::ComponentData & OutputComponentData)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot get component for NULL actor."));
        return false;
    }

    if ((Scene.ComponentMasks [ActorHandle - 1u] & static_cast<uint32>(Scene::ComponentMasks::Material)) == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Actor doesn't have a material component."));
        return false;
    }

    uint32 const ComponentIndex = MaterialComponents.ActorToComponentMap [ActorHandle];

    OutputComponentData.MaterialHandle = MaterialComponents.MaterialHandles [ComponentIndex];

    return true;
}