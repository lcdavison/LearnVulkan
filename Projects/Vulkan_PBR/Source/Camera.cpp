#include "Camera.hpp"

#include <Math/Transform.hpp>

void Camera::GetViewMatrix(Camera::CameraState const & Camera, Math::Matrix4x4 & OutputViewMatrix)
{
    Math::Matrix4x4 ViewMatrix = Math::Matrix4x4 {};
    ViewMatrix [Math::Matrix4x4::Index { 0u, 0u }] = Camera.RightAxis.X;
    ViewMatrix [Math::Matrix4x4::Index { 1u, 0u }] = Camera.RightAxis.Y;
    ViewMatrix [Math::Matrix4x4::Index { 2u, 0u }] = Camera.RightAxis.Z;

    ViewMatrix [Math::Matrix4x4::Index { 0u, 1u }] = Camera.UpAxis.X;
    ViewMatrix [Math::Matrix4x4::Index { 1u, 1u }] = Camera.UpAxis.Y;
    ViewMatrix [Math::Matrix4x4::Index { 2u, 1u }] = Camera.UpAxis.Z;

    ViewMatrix [Math::Matrix4x4::Index { 0u, 2u }] = Camera.ForwardAxis.X;
    ViewMatrix [Math::Matrix4x4::Index { 1u, 2u }] = Camera.ForwardAxis.Y;
    ViewMatrix [Math::Matrix4x4::Index { 2u, 2u }] = Camera.ForwardAxis.Z;

    ViewMatrix [Math::Matrix4x4::Index { 0u, 3u }] = Camera.Position.X;
    ViewMatrix [Math::Matrix4x4::Index { 1u, 3u }] = Camera.Position.Y;
    ViewMatrix [Math::Matrix4x4::Index { 2u, 3u }] = Camera.Position.Z;
    ViewMatrix [Math::Matrix4x4::Index { 3u, 3u }] = 1.0f;

    OutputViewMatrix = ViewMatrix;
}

void Camera::UpdateOrientation(Camera::CameraState & Camera, float const ChangeInPitchInDegrees, float const ChangeInYawInDegrees)
{
    Math::Vector4 RightAxis = Math::Vector4 { Camera.RightAxis.X, Camera.RightAxis.Y, Camera.RightAxis.Z, 0.0f };
    Math::Vector4 UpAxis = Math::Vector4 { Camera.UpAxis.X, Camera.UpAxis.Y, Camera.UpAxis.Z, 0.0f };
    Math::Vector4 ForwardAxis = Math::Vector4 { Camera.ForwardAxis.X, Camera.ForwardAxis.Y, Camera.ForwardAxis.Z, 0.0f };

    {
        Math::Matrix4x4 const PitchRotation = Math::RotateAxisAngle(Camera.RightAxis, ChangeInPitchInDegrees);
        UpAxis = PitchRotation * UpAxis;
        ForwardAxis = PitchRotation * ForwardAxis;
    }

    {
        Math::Matrix4x4 const YawRotation = Math::RotateZAxis(ChangeInYawInDegrees);
        RightAxis = YawRotation * RightAxis;
        UpAxis = YawRotation * UpAxis;
        ForwardAxis = YawRotation * ForwardAxis;
    }

    Camera.RightAxis = Math::Vector3 { RightAxis.X, RightAxis.Y, RightAxis.Z };
    Camera.RightAxis = Math::Vector3::Normalize(Camera.RightAxis);

    Camera.UpAxis = Math::Vector3 { UpAxis.X, UpAxis.Y, UpAxis.Z };
    Camera.UpAxis = Math::Vector3::Normalize(Camera.UpAxis);

    Camera.ForwardAxis = Math::Vector3 { ForwardAxis.X, ForwardAxis.Y, ForwardAxis.Z };
    Camera.ForwardAxis = Math::Vector3::Normalize(Camera.ForwardAxis);
}

void Camera::UpdatePosition(Camera::CameraState & Camera, float const ForwardSpeed, float const StrafeSpeed)
{
    Camera.Position = Camera.Position + Camera.ForwardAxis * ForwardSpeed;
    Camera.Position = Camera.Position + Camera.RightAxis * StrafeSpeed;
}