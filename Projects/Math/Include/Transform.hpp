#pragma once

#include "Vector.hpp"
#include "Matrix.hpp"

namespace Math
{
    extern Vector4 const operator * (Matrix4x4 const & Matrix, struct Vector4 const & Vector);

    extern constexpr Matrix4x4 const TranslationMatrix(Vector3 const & Direction);

    extern constexpr Matrix4x4 const ScaleMatrix(Vector3 const & Scale);

    extern Matrix4x4 const PerspectiveMatrix(float const HorizontalFieldOfViewInRadians, float const AspectRatio, float const NearPlaneDistance, float const FarPlaneDistance);
}