#pragma once

#include "Common.hpp"

#include <Math/Vector.hpp>
#include <Math/Matrix.hpp>

#include <vector>
#include <unordered_map>

namespace Scene
{
    struct SceneData;
}

namespace Components::Transform
{
    struct TransformCollection
    {
        std::unordered_map<uint32, uint32> ActorHandleToComponentIndex = {};

        std::vector<Math::Vector3> Positions = {};
        std::vector<Math::Vector3> Orientations = {};
        std::vector<float> Scales = {};
    };

    struct TransformData
    {
        Math::Vector3 Position = {};
        Math::Vector3 Orientation = {};
        float Scale = {};
    };

    extern bool const CreateComponent(uint32 const ActorHandle, Scene::SceneData & Scene);

    extern bool const SetTransform(uint32 const ActorHandle, Math::Vector3 const * const NewPosition, Math::Vector3 const * const NewOrientation, float const * const NewScale);

    extern bool const GetTransform(uint32 const ActorHandle, TransformData & OutputTransformData);

    extern bool const GetTransformationMatrix(uint32 const ActorHandle, Math::Matrix4x4 & OutputTransformation);
}