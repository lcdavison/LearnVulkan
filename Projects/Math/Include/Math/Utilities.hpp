#pragma once

namespace Math
{
    static constexpr float kPi = 3.14159265359f;

    inline constexpr float const ConvertDegreesToRadians(float const AngleInDegrees)
    {
        constexpr float ConversionMultiplier = kPi / 180.0f;
        return ConversionMultiplier * AngleInDegrees;
    }
}