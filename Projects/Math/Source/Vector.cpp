#include "Vector.hpp"

Math::Vector3 Math::operator + (Math::Vector3 const & Left, Math::Vector3 const & Right)
{
    return Vector3
    {
        Left.X + Right.X,
        Left.Y + Right.Y,
        Left.Z + Right.Z,
    };
}

Math::Vector3 Math::operator - (Math::Vector3 const & Left, Math::Vector3 const & Right)
{
    return Vector3
    {
        Left.X - Right.X,
        Left.Y - Right.Y,
        Left.Z - Right.Z,
    };
}

float Math::operator * (Math::Vector3 const & Left, Math::Vector3 const & Right)
{
    return Left.X * Right.X +
           Left.Y * Right.Y +
           Left.Z * Right.Z;
}

Math::Vector3 Math::operator ^ (Math::Vector3 const & Left, Math::Vector3 const & Right)
{
    return Vector3
    {
        Left.Y * Right.Z - Left.Z * Right.Y,
        Left.Z * Right.X - Left.X * Right.Z,
        Left.X * Right.Y - Left.Y * Right.X,
    };
}

Math::Vector3 Math::operator * (Math::Vector3 const & Vector, float const Scalar)
{
    return Vector3
    {
        Vector.X * Scalar,
        Vector.Y * Scalar,
        Vector.Z * Scalar,
    };
}

Math::Vector3 Math::operator * (float const Scalar, Math::Vector3 const & Vector)
{
    return Vector * Scalar;
}