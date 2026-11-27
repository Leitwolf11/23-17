// SPDX-License-Identifier: MIT
#pragma once

#include "Rendering/World.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    // Append a static, opaque GLB before BakeLighting adds reflection probes.
    [[nodiscard]] auto
    AppendTexturedModel(WorldMesh &mesh,
                        const std::string &path,
                        Float3 origin,
                        float scale = 1.0f,
                        float heading = 0.0f)
        -> tl::expected<void, std::string>;

    // Read a validated metric GLB into the resident frame mesh. UVs address a
    // packed tile.
    auto
    AppendScan(WorldMesh &mesh,
               const std::string &path,
               Float3 origin,
               float scale,
               float heading,
               int atlasTile) -> tl::expected<void, std::string>;
} // namespace TwentyThreeSeventeen::Rendering
