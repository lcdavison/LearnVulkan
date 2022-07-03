#pragma once

#include <array>
#include <cstdint>

namespace Math
{
    struct alignas(16u) Matrix4x4
    {
        struct Index
        {
            std::uint8_t RowIndex : 2;
            std::uint8_t ColumnIndex : 2;
        };

        /* Matrix data will be stored in column major order */
        std::array<float, 16u> Data;

        inline constexpr float & operator [](Index MatrixIndex)
        {
            /* The columns are stored as rows */
            return Data [(MatrixIndex.ColumnIndex << 2u) + MatrixIndex.RowIndex];
        }

        inline constexpr float const & operator [](Index MatrixIndex) const
        {
            return Data [(MatrixIndex.ColumnIndex << 2u) + MatrixIndex.RowIndex];
        }

        static constexpr Matrix4x4 const Identity()
        {
            Matrix4x4 IdentityMatrix = {};
            IdentityMatrix.Data [(0u << 2u) + 0u] = 1.0f;
            IdentityMatrix.Data [(1u << 2u) + 1u] = 1.0f;
            IdentityMatrix.Data [(2u << 2u) + 2u] = 1.0f;
            IdentityMatrix.Data [(3u << 2u) + 3u] = 1.0f;

            return IdentityMatrix;
        }

        static Matrix4x4 const Inverse(Matrix4x4 const & Matrix);
    };

    extern Matrix4x4 const operator * (Matrix4x4 const & Left, Matrix4x4 const & Right);
}