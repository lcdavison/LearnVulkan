#include "Math/Transform.hpp"

#include <cmath>

#if USE_SSE2
    #include <xmmintrin.h>
#endif

Math::Vector4 const Math::operator * (Math::Matrix4x4 const & Matrix, Math::Vector4 const & Vector)
{
#if USE_SSE2
    Vector4 Result = {};

    __m128 MatrixColumn0 = _mm_load_ps(&Matrix.Data [0u << 2u]);
    __m128 MatrixColumn1 = _mm_load_ps(&Matrix.Data [1u << 2u]);
    __m128 MatrixColumn2 = _mm_load_ps(&Matrix.Data [2u << 2u]);
    __m128 MatrixColumn3 = _mm_load_ps(&Matrix.Data [3u << 2u]);

    __m128 Scalar0 = _mm_set1_ps(Vector.X);
    __m128 CurrentOutputColumn = _mm_mul_ps(MatrixColumn0, Scalar0);

    __m128 Scalar1 = _mm_set1_ps(Vector.Y);
    CurrentOutputColumn = _mm_add_ps(CurrentOutputColumn, _mm_mul_ps(MatrixColumn1, Scalar1));

    __m128 Scalar2 = _mm_set1_ps(Vector.Z);
    CurrentOutputColumn = _mm_add_ps(CurrentOutputColumn, _mm_mul_ps(MatrixColumn2, Scalar2));

    __m128 Scalar3 = _mm_set1_ps(Vector.W);
    CurrentOutputColumn = _mm_add_ps(CurrentOutputColumn, _mm_mul_ps(MatrixColumn3, Scalar3));

    _mm_store_ps(reinterpret_cast<float *>(&Result), CurrentOutputColumn);

    return Result;
#else
    /* TODO: Implement fallback Matrix-Vector multiply when SSE is not available */
    return Vector4 {};
#endif
}

constexpr Math::Matrix4x4 const Math::TranslationMatrix(Vector3 const & Direction)
{
    Math::Matrix4x4 Matrix = Math::Matrix4x4::Identity();
    Matrix.Data [(3u << 2u) + 0u] = Direction.X;
    Matrix.Data [(3u << 2u) + 1u] = Direction.Y;
    Matrix.Data [(3u << 2u) + 2u] = Direction.Z;

    return Matrix;
}

constexpr Math::Matrix4x4 const Math::ScaleMatrix(Vector3 const & Scale)
{
    Math::Matrix4x4 Matrix = {};
    Matrix [Math::Matrix4x4::Index { 0u, 0u }] = Scale.X;
    Matrix [Math::Matrix4x4::Index { 1u, 1u }] = Scale.Y;
    Matrix [Math::Matrix4x4::Index { 2u, 2u }] = Scale.Z;
    Matrix [Math::Matrix4x4::Index { 3u, 3u }] = 1.0f;

    return Matrix;
}

Math::Matrix4x4 const Math::PerspectiveMatrix(float const HorizontalFieldOfViewInRadians, float const AspectRatio, float const NearPlaneDistance, float const FarPlaneDistance)
{
    Math::Matrix4x4 Matrix = {};

    float const ProjectionPlaneDistance = AspectRatio / std::tan(HorizontalFieldOfViewInRadians * 0.5f);

    /* Project View Space X */
    Matrix [Math::Matrix4x4::Index { 0u, 0u }] = ProjectionPlaneDistance / AspectRatio;

    /* Project View Space Y */
    Matrix [Math::Matrix4x4::Index { 1u, 1u }] = ProjectionPlaneDistance;

    /* Remap View Space Z between [0, 1] */
    float const RemapMultiplier = FarPlaneDistance / (FarPlaneDistance - NearPlaneDistance);

    Matrix [Math::Matrix4x4::Index { 2u, 2u }] = RemapMultiplier;
    Matrix [Math::Matrix4x4::Index { 2u, 3u }] = -NearPlaneDistance * RemapMultiplier;

    /* Carry View Space Z for Perspective Divide */
    Matrix [Math::Matrix4x4::Index { 3u, 2u }] = 1.0f;

    return Matrix;
}