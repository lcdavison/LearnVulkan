#include "Math/Matrix.hpp"

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