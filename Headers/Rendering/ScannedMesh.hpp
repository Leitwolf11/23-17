// SPDX-License-Identifier: MIT
#pragma once

#include "Rendering/World.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    // Read a validated metric GLB into the resident frame mesh. UVs address a
    // packed tile.
    auto
    AppendScan(WorldMesh &mesh,
               const std::string &path,
               Float3 origin,
               float scale,
               float heading,
               float tile) -> tl::expected<void, std::string>;
} // namespace TwentyThreeSeventeen::Rendering
