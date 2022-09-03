#include "Math/Matrix.hpp"

#include "Math/Vector.hpp"

#if USE_SSE2
#include <xmmintrin.h>
#endif

Math::Matrix4x4 const Math::operator * (Matrix4x4 const & Left, Matrix4x4 const & Right)
{
#if USE_SSE2
    Matrix4x4 Result = {};

    __m128 LeftColumn0 = _mm_load_ps(&Left.Data [0u << 2u]);
    __m128 LeftColumn1 = _mm_load_ps(&Left.Data [1u << 2u]);
    __m128 LeftColumn2 = _mm_load_ps(&Left.Data [2u << 2u]);
    __m128 LeftColumn3 = _mm_load_ps(&Left.Data [3u << 2u]);

    for (std::uint8_t CurrentColumnIndex = { 0u }; CurrentColumnIndex < 4u; CurrentColumnIndex++)
    {
        __m128 Scalar0 = _mm_set1_ps(Right [Math::Matrix4x4::Index { 0u, CurrentColumnIndex }]);
        __m128 CurrentOutputColumn = _mm_mul_ps(LeftColumn0, Scalar0);

        __m128 Scalar1 = _mm_set1_ps(Right [Math::Matrix4x4::Index { 1u, CurrentColumnIndex }]);
        CurrentOutputColumn = _mm_add_ps(CurrentOutputColumn, _mm_mul_ps(LeftColumn1, Scalar1));

        __m128 Scalar2 = _mm_set1_ps(Right [Math::Matrix4x4::Index { 2u, CurrentColumnIndex }]);
        CurrentOutputColumn = _mm_add_ps(CurrentOutputColumn, _mm_mul_ps(LeftColumn2, Scalar2));

        __m128 Scalar3 = _mm_set1_ps(Right [Math::Matrix4x4::Index { 3u, CurrentColumnIndex }]);
        CurrentOutputColumn = _mm_add_ps(CurrentOutputColumn, _mm_mul_ps(LeftColumn3, Scalar3));

        _mm_store_ps(&Result.Data [CurrentColumnIndex << 2u], CurrentOutputColumn);
    }

    return Result;
#else
    Matrix4x4 Result = {};

    Result [Matrix4x4::Index {0u, 0u}] = 
        Left [Matrix4x4::Index { 0u, 0u }] * Right [Matrix4x4::Index { 0u, 0u }] + 
        Left [Matrix4x4::Index { 0u, 1u }] * Right [Matrix4x4::Index { 1u, 0u }] + 
        Left [Matrix4x4::Index { 0u, 2u }] * Right [Matrix4x4::Index { 2u, 0u }] + 
        Left [Matrix4x4::Index { 0u, 3u }] * Right [Matrix4x4::Index { 3u, 0u }];

    Result [Matrix4x4::Index { 1u, 0u }] = 
        Left [Matrix4x4::Index { 1u, 0u }] * Right [Matrix4x4::Index { 0u, 0u }] +
        Left [Matrix4x4::Index { 1u, 1u }] * Right [Matrix4x4::Index { 1u, 0u }] + 
        Left [Matrix4x4::Index { 1u, 2u }] * Right [Matrix4x4::Index { 2u, 0u }] + 
        Left [Matrix4x4::Index { 1u, 3u }] * Right [Matrix4x4::Index { 3u, 0u }];

    Result [Matrix4x4::Index { 2u, 0u }] = 
        Left [Matrix4x4::Index { 2u, 0u }] * Right [Matrix4x4::Index { 0u, 0u }] + 
        Left [Matrix4x4::Index { 2u, 1u }] * Right [Matrix4x4::Index { 1u, 0u }] + 
        Left [Matrix4x4::Index { 2u, 2u }] * Right [Matrix4x4::Index { 2u, 0u }] + 
        Left [Matrix4x4::Index { 2u, 3u }] * Right [Matrix4x4::Index { 3u, 0u }];

    Result [Matrix4x4::Index { 3u, 0u }] = 
        Left [Matrix4x4::Index { 3u, 0u }] * Right [Matrix4x4::Index { 0u, 0u }] + 
        Left [Matrix4x4::Index { 3u, 1u }] * Right [Matrix4x4::Index { 1u, 0u }] + 
        Left [Matrix4x4::Index { 3u, 2u }] * Right [Matrix4x4::Index { 2u, 0u }] + 
        Left [Matrix4x4::Index { 3u, 3u }] * Right [Matrix4x4::Index { 3u, 0u }];

    Result [Matrix4x4::Index { 0u, 1u }] = 
        Left [Matrix4x4::Index { 0u, 0u }] * Right [Matrix4x4::Index { 0u, 1u }] +
        Left [Matrix4x4::Index { 0u, 1u }] * Right [Matrix4x4::Index { 1u, 1u }] +
        Left [Matrix4x4::Index { 0u, 2u }] * Right [Matrix4x4::Index { 2u, 1u }] +
        Left [Matrix4x4::Index { 0u, 3u }] * Right [Matrix4x4::Index { 3u, 1u }];
    
    Result [Matrix4x4::Index { 1u, 1u }] = 
        Left [Matrix4x4::Index { 1u, 0u }] * Right [Matrix4x4::Index { 0u, 1u }] +
        Left [Matrix4x4::Index { 1u, 1u }] * Right [Matrix4x4::Index { 1u, 1u }] +
        Left [Matrix4x4::Index { 1u, 2u }] * Right [Matrix4x4::Index { 2u, 1u }] +
        Left [Matrix4x4::Index { 1u, 3u }] * Right [Matrix4x4::Index { 3u, 1u }];
    
    Result [Matrix4x4::Index { 2u, 1u }] = 
        Left [Matrix4x4::Index { 2u, 0u }] * Right [Matrix4x4::Index { 0u, 1u }] +
        Left [Matrix4x4::Index { 2u, 1u }] * Right [Matrix4x4::Index { 1u, 1u }] +
        Left [Matrix4x4::Index { 2u, 2u }] * Right [Matrix4x4::Index { 2u, 1u }] +
        Left [Matrix4x4::Index { 2u, 3u }] * Right [Matrix4x4::Index { 3u, 1u }];

    Result [Matrix4x4::Index { 3u, 1u }] = 
        Left [Matrix4x4::Index { 3u, 0u }] * Right [Matrix4x4::Index { 0u, 1u }] +
        Left [Matrix4x4::Index { 3u, 1u }] * Right [Matrix4x4::Index { 1u, 1u }] +
        Left [Matrix4x4::Index { 3u, 2u }] * Right [Matrix4x4::Index { 2u, 1u }] +
        Left [Matrix4x4::Index { 3u, 3u }] * Right [Matrix4x4::Index { 3u, 1u }];

    Result [Matrix4x4::Index { 0u, 2u }] =
        Left [Matrix4x4::Index { 0u, 0u }] * Right [Matrix4x4::Index { 0u, 2u }] +
        Left [Matrix4x4::Index { 0u, 1u }] * Right [Matrix4x4::Index { 1u, 2u }] +
        Left [Matrix4x4::Index { 0u, 2u }] * Right [Matrix4x4::Index { 2u, 2u }] +
        Left [Matrix4x4::Index { 0u, 3u }] * Right [Matrix4x4::Index { 3u, 2u }];
    
    Result [Matrix4x4::Index { 1u, 2u }] =
        Left [Matrix4x4::Index { 1u, 0u }] * Right [Matrix4x4::Index { 0u, 2u }] +
        Left [Matrix4x4::Index { 1u, 1u }] * Right [Matrix4x4::Index { 1u, 2u }] +
        Left [Matrix4x4::Index { 1u, 2u }] * Right [Matrix4x4::Index { 2u, 2u }] +
        Left [Matrix4x4::Index { 1u, 3u }] * Right [Matrix4x4::Index { 3u, 2u }];
    
    Result [Matrix4x4::Index { 2u, 2u }] =
        Left [Matrix4x4::Index { 2u, 0u }] * Right [Matrix4x4::Index { 0u, 2u }] +
        Left [Matrix4x4::Index { 2u, 1u }] * Right [Matrix4x4::Index { 1u, 2u }] +
        Left [Matrix4x4::Index { 2u, 2u }] * Right [Matrix4x4::Index { 2u, 2u }] +
        Left [Matrix4x4::Index { 2u, 3u }] * Right [Matrix4x4::Index { 3u, 2u }];
    
    Result [Matrix4x4::Index { 3u, 2u }] =
        Left [Matrix4x4::Index { 3u, 0u }] * Right [Matrix4x4::Index { 0u, 2u }] +
        Left [Matrix4x4::Index { 3u, 1u }] * Right [Matrix4x4::Index { 1u, 2u }] +
        Left [Matrix4x4::Index { 3u, 2u }] * Right [Matrix4x4::Index { 2u, 2u }] +
        Left [Matrix4x4::Index { 3u, 3u }] * Right [Matrix4x4::Index { 3u, 2u }];

    Result [Matrix4x4::Index { 0u, 3u }] =
        Left [Matrix4x4::Index { 0u, 0u }] * Right [Matrix4x4::Index { 0u, 3u }] +
        Left [Matrix4x4::Index { 0u, 1u }] * Right [Matrix4x4::Index { 1u, 3u }] +
        Left [Matrix4x4::Index { 0u, 2u }] * Right [Matrix4x4::Index { 2u, 3u }] +
        Left [Matrix4x4::Index { 0u, 3u }] * Right [Matrix4x4::Index { 3u, 3u }];
    
    Result [Matrix4x4::Index { 1u, 3u }] =
        Left [Matrix4x4::Index { 1u, 0u }] * Right [Matrix4x4::Index { 0u, 3u }] +
        Left [Matrix4x4::Index { 1u, 1u }] * Right [Matrix4x4::Index { 1u, 3u }] +
        Left [Matrix4x4::Index { 1u, 2u }] * Right [Matrix4x4::Index { 2u, 3u }] +
        Left [Matrix4x4::Index { 1u, 3u }] * Right [Matrix4x4::Index { 3u, 3u }];
    
    Result [Matrix4x4::Index { 2u, 3u }] =
        Left [Matrix4x4::Index { 2u, 0u }] * Right [Matrix4x4::Index { 0u, 3u }] +
        Left [Matrix4x4::Index { 2u, 1u }] * Right [Matrix4x4::Index { 1u, 3u }] +
        Left [Matrix4x4::Index { 2u, 2u }] * Right [Matrix4x4::Index { 2u, 3u }] +
        Left [Matrix4x4::Index { 2u, 3u }] * Right [Matrix4x4::Index { 3u, 3u }];
    
    Result [Matrix4x4::Index { 3u, 3u }] =
        Left [Matrix4x4::Index { 3u, 0u }] * Right [Matrix4x4::Index { 0u, 3u }] +
        Left [Matrix4x4::Index { 3u, 1u }] * Right [Matrix4x4::Index { 1u, 3u }] +
        Left [Matrix4x4::Index { 3u, 2u }] * Right [Matrix4x4::Index { 2u, 3u }] +
        Left [Matrix4x4::Index { 3u, 3u }] * Right [Matrix4x4::Index { 3u, 3u }];

    return Result;
#endif
}

/* Eric Lengyel, FGED Vol 1 */
Math::Matrix4x4 const Math::Matrix4x4::Inverse(Matrix4x4 const & Matrix)
{
    /* This can be derived using Grassmann algebra, making use of (A ^ B ^ C ^ D) as the 4D Hypervolume (Determinant) */

    std::array<Math::Vector3 const *, 4u> const kColumns =
    {
        reinterpret_cast<Vector3 const *>(&Matrix.Data [0u << 2u]),
        reinterpret_cast<Vector3 const *>(&Matrix.Data [1u << 2u]),
        reinterpret_cast<Vector3 const *>(&Matrix.Data [2u << 2u]),
        reinterpret_cast<Vector3 const *>(&Matrix.Data [3u << 2u]),
    };

    std::array<Math::Vector3, 4u> ColumnFactors =
    {
        *(kColumns [0u]) ^ *(kColumns [1u]),
        *(kColumns [2u]) ^ *(kColumns [3u]),
        (*(kColumns [0u]) * Matrix [Math::Matrix4x4::Index { 3u, 1u }]) - (*(kColumns [1u]) * Matrix [Math::Matrix4x4::Index { 3u, 0u }]),
        (*(kColumns [2u]) * Matrix [Math::Matrix4x4::Index { 3u, 3u }]) - (*(kColumns [3u]) * Matrix [Math::Matrix4x4::Index { 3u, 2u }]),
    };

    float const kDeterminant = (ColumnFactors [0u] * ColumnFactors [3u]) + (ColumnFactors [1u] * ColumnFactors [2u]);
    /* Need to handle this properly */
    if (kDeterminant == 0.0f)
    {
        return Math::Matrix4x4 {};
    }

    float const kInverseDeterminant = 1.0f / kDeterminant;
    ColumnFactors [0u] = ColumnFactors [0u] * kInverseDeterminant;
    ColumnFactors [1u] = ColumnFactors [1u] * kInverseDeterminant;
    ColumnFactors [2u] = ColumnFactors [2u] * kInverseDeterminant;
    ColumnFactors [3u] = ColumnFactors [3u] * kInverseDeterminant;

    std::array<Math::Vector3, 4u> const kOutputRows =
    {
        (*(kColumns [1u]) ^ ColumnFactors [3u]) + ColumnFactors [1u] * Matrix [Math::Matrix4x4::Index { 3u, 1u }],
        (ColumnFactors [3u] ^ *(kColumns [0u])) - ColumnFactors [1u] * Matrix [Math::Matrix4x4::Index { 3u, 0u }],
        (*(kColumns [3u]) ^ ColumnFactors [2u]) + ColumnFactors [0u] * Matrix [Math::Matrix4x4::Index { 3u, 3u }],
        (ColumnFactors [2u] ^ *(kColumns [2u])) - ColumnFactors [0u] * Matrix [Math::Matrix4x4::Index { 3u, 2u }],
    };

    return Math::Matrix4x4
    {
        kOutputRows [0u].X, kOutputRows [1u].X, kOutputRows [2u].X, kOutputRows [3u].X,
        kOutputRows [0u].Y, kOutputRows [1u].Y, kOutputRows [2u].Y, kOutputRows [3u].Y,
        kOutputRows [0u].Z, kOutputRows [1u].Z, kOutputRows [2u].Z, kOutputRows [3u].Z,
        -(*(kColumns [1u]) * ColumnFactors [1u]), *(kColumns [0u]) * ColumnFactors [1u], -(*(kColumns [3u]) * ColumnFactors [0u]), *(kColumns [2u]) * ColumnFactors [0u],
    };
}