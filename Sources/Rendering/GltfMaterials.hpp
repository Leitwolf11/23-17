// SPDX-License-Identifier: MIT
#pragma once

#include <cgltf.h>

#include "Rendering/World.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    struct GltfMaterialTile
    {
        std::vector<std::uint8_t> color;
        std::vector<std::uint8_t> surface;
        Float3 tint{ 1, 1, 1 };
        float emission{};
    };

    [[nodiscard]] auto
    PackGltfMaterial(const cgltf_material *material)
        -> tl::expected<GltfMaterialTile, std::string>;
} // namespace TwentyThreeSeventeen::Rendering
