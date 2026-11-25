// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <vector>

namespace TwentyThreeSeventeen::Rendering
{
    using Float3 = std::array<float, 3>;
    using Float4 = std::array<float, 4>;

    struct WorldVertex
    {
        Float3 position;
        Float3 normal;
        Float4 tangent; // XYZ points along U, W is the bitangent sign.
        Float3 tint;
        Float3 uvMaterial;
        Float4 visibility; // Baked line of sight to each fixed light.
        Float4 response;   // Ambient exposure, emission, event mask and idle
                           // motion weight.
    };

    struct MaterialAtlas
    {
        static constexpr int kTileSize = 512;
        int materialCount{ 5 };
        int probeCount{ 1 };
        [[nodiscard]] auto
        LayerCount() const -> int
        {
            return materialCount * 2 + probeCount;
        }
        std::vector<std::uint8_t> rgba;
    };

    [[nodiscard]] auto
    LoadMaterials(const char *executable, bool reference = false)
        -> tl::expected<MaterialAtlas, std::string>;

    struct WorldMesh
    {
        MaterialAtlas materials;
        std::vector<WorldVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::array<std::uint32_t, 5> detailEnds{};
        [[nodiscard]] auto
        ValidForUpload() const -> bool;
    };

    // Matches the vertex shader's std140 block, including all padding.
    struct alignas(16) WorldView
    {
        Float4 eye;
        Float4 right;
        Float4 up;
        Float4 forward;
        std::array<Float4, 4> lights; // World position and radiant intensity.
        std::array<Float4, 4> colors;
        Float4 probeCenter{}, probeHalfSize{};
        Float4 streetCenter{}, streetHalfSize{}, streetLamp{};
    };

    void
    BakeLighting(WorldMesh &mesh, const WorldView &lighting);

    struct WorldFrame
    {
        std::shared_ptr<const WorldMesh> mesh;
        WorldView view;
        std::uint32_t indexCount;
    };
} // namespace TwentyThreeSeventeen::Rendering
