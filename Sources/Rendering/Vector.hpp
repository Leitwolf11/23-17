// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>

#include "Rendering/World.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    inline auto
    Add(Float3 a, Float3 b) -> Float3
    {
        return { a[0] + b[0], a[1] + b[1], a[2] + b[2] };
    }
    inline auto
    Sub(Float3 a, Float3 b) -> Float3
    {
        return { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
    }
    inline auto
    Scale(Float3 a, float s) -> Float3
    {
        return { a[0] * s, a[1] * s, a[2] * s };
    }
    inline auto
    Dot(Float3 a, Float3 b) -> float
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }
    inline auto
    Cross(Float3 a, Float3 b) -> Float3
    {
        return { a[1] * b[2] - a[2] * b[1],
                 a[2] * b[0] - a[0] * b[2],
                 a[0] * b[1] - a[1] * b[0] };
    }
    inline auto
    Unit(Float3 a) -> Float3
    {
        return Scale(a, 1.0f / std::sqrt(std::max(1e-14f, Dot(a, a))));
    }
    inline auto
    Xyz(Float4 a) -> Float3
    {
        return { a[0], a[1], a[2] };
    }

} // namespace TwentyThreeSeventeen::Rendering
