#pragma once

#include "Vector.hpp"
#include "Matrix.hpp"

namespace Math
{
    extern Vector4 const operator * (Matrix4x4 const & Matrix, struct Vector4 const & Vector);

    extern constexpr Matrix4x4 const TranslationMatrix(Vector3 const & Direction);

    extern constexpr Matrix4x4 const ScaleMatrix(Vector3 const & Scale);

    extern constexpr Matrix4x4 const ScaleMatrix(float const Scale);

    extern Matrix4x4 const RotateZAxis(float const AngleInDegrees);

    extern Matrix4x4 const RotateAxisAngle(Vector3 const & Axis, float const AngleInDegrees);

    /* Using Z-Up as a convention, so this will be useful */
    constexpr Matrix4x4 const YAxisUpToZAxisUp()
    {
        Matrix4x4 Matrix = {};
        Matrix [Matrix4x4::Index { 0u, 0u }] = 1.0f; // +X
        Matrix [Matrix4x4::Index { 2u, 1u }] = 1.0f; // +Z
        Matrix [Matrix4x4::Index { 1u, 2u }] = -1.0f; // -Y
        Matrix [Matrix4x4::Index { 4u, 4u }] = 1.0f;

        return Matrix;
    }

    extern Matrix4x4 const PerspectiveMatrix(float const HorizontalFieldOfViewInRadians, float const AspectRatio, float const NearPlaneDistance, float const FarPlaneDistance);
}