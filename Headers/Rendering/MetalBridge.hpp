// SPDX-License-Identifier: MIT
#pragma once

namespace TwentyThreeSeventeen::Rendering
{
#if defined(__APPLE__)
    [[nodiscard]] auto
    HasMetal3Device() noexcept -> bool;
#else
    [[nodiscard]] constexpr auto
    HasMetal3Device() noexcept -> bool
    {
        return false;
    }
#endif
} // namespace TwentyThreeSeventeen::Rendering
