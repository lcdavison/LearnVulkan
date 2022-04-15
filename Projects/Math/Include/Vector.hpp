#pragma once

#include <cmath>

namespace Math
{
    /* This is currently just used as a container for SSE */
    struct alignas(16) Vector4
    {
        float X;
        float Y;
        float Z;
        float W;
    };

    struct Vector3
    {
        float X;
        float Y;
        float Z;

        inline static float const Length(Vector3 const & Vector)
        {
            return std::sqrt(Vector.X * Vector.X + Vector.Y * Vector.Y + Vector.Z * Vector.Z);
        }

        inline static Vector3 const Normalize(Vector3 const & Vector)
        {
            return Vector * (1.0f / Length(Vector));
        }

        static constexpr Vector3 const Zero()
        {
            return Vector3 { 0.0f, 0.0f, 0.0f };
        }

        static constexpr Vector3 const One()
        {
            return Vector3 { 1.0f, 1.0f, 1.0f };
        }
    };

    extern Vector3 operator + (Vector3 const & Left, Vector3 const & Right);

    /* In an abstract sense we can also use this for difference between 3D Points */
    extern Vector3 operator - (Vector3 const & Left, Vector3 const & Right);

    /* Dot product */
    extern float operator * (Vector3 const & Left, Vector3 const & Right);

    /* Cross product */
    extern Vector3 operator ^ (Vector3 const & Left, Vector3 const & Right);

    extern Vector3 operator * (Vector3 const & Vector, float const Scalar);

    extern Vector3 operator * (float const Scalar, Vector3 const & Vector);
}
