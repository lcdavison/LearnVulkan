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

    Vector3 const & Column0 = reinterpret_cast<Vector3 const &>(Matrix [Math::Matrix4x4::Index { 0u, 0u }]);
    Vector3 const & Column1 = reinterpret_cast<Vector3 const &>(Matrix [Math::Matrix4x4::Index { 0u, 1u }]);
    Vector3 const & Column2 = reinterpret_cast<Vector3 const &>(Matrix [Math::Matrix4x4::Index { 0u, 2u }]);
    Vector3 const & Column3 = reinterpret_cast<Vector3 const &>(Matrix [Math::Matrix4x4::Index { 0u, 3u }]);

    Math::Vector3 Col0CrossCol1 = Column0 ^ Column1;
    Math::Vector3 Col2CrossCol3 = Column2 ^ Column3;
    Math::Vector3 Col0MinusCol1 = (Column0 * Matrix [Math::Matrix4x4::Index { 3u, 1u }]) - (Column1 * Matrix [Math::Matrix4x4::Index { 3u, 0u }]);
    Math::Vector3 Col2MinusCol3 = (Column2 * Matrix [Math::Matrix4x4::Index { 3u, 3u }]) - (Column3 * Matrix [Math::Matrix4x4::Index { 3u, 2u }]);

    float const Determinant = (Col0CrossCol1 * Col2MinusCol3) + (Col2CrossCol3 * Col0MinusCol1);
    /* Need to handle this properly */
    if (Determinant == 0.0f)
    {
        return Math::Matrix4x4 {};
    }

    float const InverseDeterminant = 1.0f / Determinant;
    Col0CrossCol1 = Col0CrossCol1 * InverseDeterminant;
    Col2CrossCol3 = Col2CrossCol3 * InverseDeterminant;
    Col0MinusCol1 = Col0MinusCol1 * InverseDeterminant;
    Col2MinusCol3 = Col2MinusCol3 * InverseDeterminant;

    Vector3 const OutputRow0 = (Column1 ^ Col2MinusCol3) + Col2CrossCol3 * Matrix [Math::Matrix4x4::Index { 3u, 1u }];
    Vector3 const OutputRow1 = (Col2MinusCol3 ^ Column0) - Col2CrossCol3 * Matrix [Math::Matrix4x4::Index { 3u, 0u }];
    Vector3 const OutputRow2 = (Column3 ^ Col0MinusCol1) + Col0CrossCol1 * Matrix [Math::Matrix4x4::Index { 3u, 3u }];
    Vector3 const OutputRow3 = (Col0MinusCol1 ^ Column2) - Col0CrossCol1 * Matrix [Math::Matrix4x4::Index { 3u, 2u }];

    return Math::Matrix4x4
    {
        OutputRow0.X, OutputRow1.X, OutputRow2.X, OutputRow3.X,
        OutputRow0.Y, OutputRow1.Y, OutputRow2.Y, OutputRow3.Y,
        OutputRow0.Z, OutputRow1.Z, OutputRow2.Z, OutputRow3.Z,
        -(Column1 * Col2CrossCol3), Column0 * Col2CrossCol3, -(Column3 * Col0CrossCol1), Column2 * Col0CrossCol1,
    };
}