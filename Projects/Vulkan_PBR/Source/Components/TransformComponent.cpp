#include "Components/TransformComponent.hpp"

#include "Scene.hpp"

#include <Math/Transform.hpp>

Components::Transform::TransformCollection Transforms = {};

bool const Components::Transform::CreateComponent(uint32 const ActorHandle, Scene::SceneData & Scene)
{
    if (ActorHandle == 0u)
    {
        /* LOG ERROR */
        return false;
    }

    /* TODO: Reserve memory on the first components creation */

    Transforms.Positions.emplace_back();
    Transforms.Orientations.emplace_back();
    Transforms.Scales.emplace_back();

    uint32 const NewComponentHandle = static_cast<uint32>(Transforms.Positions.size());
    Transforms.ActorHandleToComponentIndex [ActorHandle] = NewComponentHandle;

    Scene.ComponentMasks [ActorHandle - 1u] |= static_cast<uint32>(Scene::ComponentMasks::Transform);

    return true;
}

bool const Components::Transform::SetTransform(uint32 const ActorHandle, Math::Vector3 const * const NewPosition, Math::Vector3 const * const NewOrientation, float const * const NewScale)
{
    if (ActorHandle == 0u)
    {
        /* LOG ERROR */
        return false;
    }

    uint32 const ComponentIndex = Transforms.ActorHandleToComponentIndex [ActorHandle] - 1u;

    if (NewPosition)
    {
        Transforms.Positions [ComponentIndex] = *NewPosition;
    }

    if (NewOrientation)
    {
        Transforms.Orientations [ComponentIndex] = *NewOrientation;
    }

    if (NewScale)
    {
        Transforms.Scales [ComponentIndex] = *NewScale;
    }

    return true;
}

bool const Components::Transform::GetTransform(uint32 const ActorHandle, Components::Transform::TransformData & /*OutputTransformData*/)
{
    if (ActorHandle == 0u)
    {
        /* LOG ERROR */
        return false;
    }

    return true;
}

bool const Components::Transform::GetTransformationMatrix(uint32 const ActorHandle, Math::Matrix4x4 & OutputTransformation)
{
    if (ActorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot compute transformation matrix for NULL actor."));
        return false;
    }

    if (Transforms.ActorHandleToComponentIndex [ActorHandle] == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("The actor provided doesn't have an associated transform component."));
        return false;
    }

    uint32 const ComponentIndex = { Transforms.ActorHandleToComponentIndex [ActorHandle] - 1u };

    OutputTransformation = Math::TranslationMatrix(Transforms.Positions [ComponentIndex]) * Math::ScaleMatrix(Transforms.Scales [ComponentIndex]);

    return true;
}