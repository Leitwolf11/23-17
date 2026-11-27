// SPDX-License-Identifier: MIT
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <algorithm>
#include <memory>

#include "Rendering/ScannedMesh.hpp"
#include "rules_cc/cc/runfiles/runfiles.h"

TEST_CASE("Multiple Blender GLBs preserve the existing world material layers")
{
    using namespace TwentyThreeSeventeen::Rendering;
    using rules_cc::cc::runfiles::Runfiles;
    const std::unique_ptr<Runfiles> files(Runfiles::CreateForTest());
    REQUIRE(files != nullptr);
    const auto path
        = files->Rlocation("_main/Assets/Geometry/security-monitor.glb");
    REQUIRE_FALSE(path.empty());

    constexpr std::size_t bytes
        = MaterialAtlas::kTileSize * MaterialAtlas::kTileSize * 4;
    WorldMesh world;
    world.materials.materialCount = 1;
    world.materials.rgba.resize(bytes * 2, 37);
    std::ranges::fill_n(world.materials.rgba.begin(),
                        bytes,
                        std::uint8_t{ 19 });

    REQUIRE(AppendTexturedModel(world, path, { 1, 2, 3 }).has_value());
    REQUIRE(world.materials.materialCount == 6);
    CHECK(world.materials.rgba.size() == bytes * 12);
    CHECK(world.materials.rgba[0] == 19);
    CHECK(world.materials.rgba[6 * bytes] == 37);
    REQUIRE_FALSE(world.indices.empty());

    const auto emissive
        = std::ranges::find_if(world.vertices, [](const WorldVertex &vertex) {
              return vertex.response[1] > 0.75f;
          });
    REQUIRE(emissive != world.vertices.end());
    CHECK(emissive->position[1] > 2.0f);
    const auto colorLayer
        = static_cast<std::size_t>(emissive->uvMaterial[2]) - 1;
    const auto first = world.materials.rgba.begin()
                       + static_cast<std::ptrdiff_t>(colorLayer * bytes);
    const auto last = first + static_cast<std::ptrdiff_t>(bytes);
    const auto [minimum, maximum] = std::minmax_element(first, last);
    CHECK(*minimum < 70);
    CHECK(*maximum == 255);
    const auto surfaceLayer
        = colorLayer + static_cast<std::size_t>(world.materials.materialCount);
    CHECK(world.materials.rgba[surfaceLayer * bytes + 2] == 48);
    CHECK(world.materials.rgba[surfaceLayer * bytes + 3] == 0);

    const auto joinery
        = files->Rlocation("_main/Assets/Geometry/apartment-joinery.glb");
    const auto oldVertexCount = world.vertices.size();
    const auto oldIndexCount = world.indices.size();
    REQUIRE(AppendTexturedModel(world, joinery, {}).has_value());
    CHECK(world.vertices.size() > oldVertexCount);
    REQUIRE(world.indices.size() > oldIndexCount);
    const auto firstNewTriangle
        = world.indices.begin() + static_cast<std::ptrdiff_t>(oldIndexCount);
    CHECK(std::ranges::all_of(firstNewTriangle,
                              world.indices.end(),
                              [oldVertexCount](std::uint32_t index) {
                                  return index >= oldVertexCount;
                              }));
    REQUIRE(world.materials.materialCount == 11);
    CHECK(world.materials.rgba.size() == bytes * 22);
    CHECK(world.materials.rgba[0] == 19);
    CHECK(world.materials.rgba[11 * bytes] == 37);
    CHECK(world.materials.rgba[(colorLayer + 11) * bytes + 2] == 48);
    CHECK(world.materials.rgba[(colorLayer + 11) * bytes + 3] == 0);

    // Once reflection probes exist, inserting more materials would move their
    // layers.
    world.materials.rgba.resize(bytes * 23);
    const auto vertexCount = world.vertices.size();
    const auto indexCount = world.indices.size();
    CHECK_FALSE(AppendTexturedModel(world, path, { 0, 0, 0 }).has_value());
    CHECK(world.vertices.size() == vertexCount);
    CHECK(world.indices.size() == indexCount);
    CHECK(world.materials.materialCount == 11);
    CHECK(world.materials.rgba.size() == bytes * 23);
}
