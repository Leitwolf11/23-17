// SPDX-License-Identifier: MIT
#include <Progmasoft/Catch3/TestMacros.hpp>

#include "Rendering/World.hpp"

namespace
{
    using namespace TwentyThreeSeventeen::Rendering;

    auto
    Point(Float3 position) -> WorldVertex
    {
        return { position, { 0, 1, 0 }, { 1, 0, 0, -1 }, { 1, 1, 1 },
                 {},       {},          { 1, 0, 0, 0 } };
    }
} // namespace

TEST_CASE("Area lights preserve partial penumbrae and valid reflection layers")
{
    WorldMesh mesh;
    mesh.materials.materialCount = 1;
    constexpr auto tileBytes
        = MaterialAtlas::kTileSize * MaterialAtlas::kTileSize * 4;
    mesh.materials.rgba.resize(2 * tileBytes);
    mesh.vertices = { Point({ 0, 0, 0 }),
                      Point({ 1, 0, 0 }),
                      Point({ 0, 0, 1 }),
                      Point({ 0.02f, 0.5f, -0.25f }),
                      Point({ 0.70f, 0.5f, -0.25f }),
                      Point({ 0.70f, 0.5f, 0.25f }),
                      Point({ 0.02f, 0.5f, 0.25f }) };
    mesh.indices = { 0, 1, 2, 3, 4, 5, 3, 5, 6 };
    WorldView view{};
    view.lights[0] = { 0, 1.5f, 0, 5 };
    view.colors[0] = { 1, 1, 1, 0 };
    view.probeCenter = { 0, 0.2f, 0, 0 };
    view.probeHalfSize = { 1, 1, 1, 1 };
    BakeLighting(mesh, view);
    REQUIRE(mesh.ValidForUpload());
    REQUIRE(mesh.vertices[0].visibility[0] > 0.05f);
    REQUIRE(mesh.vertices[0].visibility[0] < 0.95f);
    REQUIRE(mesh.vertices[0].visibility[1] < 0.001f);
    REQUIRE(mesh.materials.rgba[2 * tileBytes + 3] == 255);
    mesh.indices[0] = 100;
    REQUIRE_FALSE(mesh.ValidForUpload());
}
