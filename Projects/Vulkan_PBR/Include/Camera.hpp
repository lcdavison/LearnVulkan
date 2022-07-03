#pragma once

#include <Math/Vector.hpp>
#include <Math/Matrix.hpp>

namespace Camera
{
    struct CameraState
    {
        Math::Vector3 Position = Math::Vector3::Zero();
        Math::Vector3 ForwardAxis = Math::Vector3 { 0.0f, 1.0f, 0.0f };
        Math::Vector3 RightAxis = Math::Vector3 { 1.0f, 0.0f, 0.0f };
        Math::Vector3 UpAxis = Math::Vector3 { 0.0f, 0.0f, -1.0f };
    };

    extern void GetViewMatrix(CameraState const & Camera, Math::Matrix4x4 & OutputViewMatrix);

    extern void UpdateOrientation(CameraState & Camera, float const ChangeInPitchInDegrees, float const ChangeInYawInDegrees);

    extern void UpdatePosition(CameraState & Camera, float const ForwardSpeed, float const StrafeSpeed);
}