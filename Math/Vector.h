#pragma once

/* Check this can't remember intrinsic header */
//#include <xmmintrin.h>

namespace Math
{
  struct Vector3
  {
      float X;
      float Y;
      float Z;
  };
  
  extern Vector3 operator + (Vector3 const & Left, Vector3 const & Right);
  
  /* In an abstract sense we can also use this for difference between 3D Points */
  extern Vector3 operator - (Vector3 const & Left, Vector3 const & Right);
  
  /* Dot product */
  extern Vector3 operator * (Vector3 const & Left, Vector3 const & Right);
  
  /* Cross product */
  extern Vector3 operator ^ (Vector3 const & Left, Vector3 const & Right);
  
  extern Vector3 operator * (Vector3 const & Vector, float const Scalar);
  
  extern Vector3 operator * (float const Scalar, Vector3 const & Vector);
}
