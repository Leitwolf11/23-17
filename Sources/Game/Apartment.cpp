// SPDX-License-Identifier: MIT
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "Game/Apartment.hpp"
#include "Game/WitnessModel.hpp"
#include "Investigation/Case.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        using Physics::Position;

        auto
        Dot(Position a, Position b) noexcept -> float
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        constexpr std::array<std::array<unsigned, 6>, 6> kFaces{ {
            { { 0, 1, 2, 1, 3, 2 } },
            { { 4, 6, 5, 5, 6, 7 } },
            { { 0, 4, 1, 1, 4, 5 } },
            { { 2, 3, 6, 3, 7, 6 } },
            { { 0, 2, 4, 2, 6, 4 } },
            { { 1, 5, 3, 3, 5, 7 } },
        } };
        constexpr std::array<float, 6> kLight{
            { 0.76f, 0.90f, 0.58f, 1.00f, 0.69f, 0.84f }
        };
        constexpr std::array kNormals{
            Position{ 0, 0, -1 }, Position{ 0, 0, 1 },  Position{ 0, -1, 0 },
            Position{ 0, 1, 0 },  Position{ -1, 0, 0 }, Position{ 1, 0, 0 }
        };

        auto
        SegmentHits(Position from,
                    Position direction,
                    const Physics::Obstacle &obstacle) noexcept -> bool
        {
            const auto clip = [](float start,
                                 float delta,
                                 float center,
                                 float half,
                                 float &near,
                                 float &far) {
                if (std::abs(delta) < 0.0001f)
                    return std::abs(start - center) < half;
                const auto a = (center - half - start) / delta;
                const auto b = (center + half - start) / delta;
                near = std::max(near, std::min(a, b));
                far = std::min(far, std::max(a, b));
                return near < far;
            };
            float near = 0.01f;
            float far = 0.96f;
            return clip(from.x,
                        direction.x,
                        obstacle.center.x,
                        obstacle.halfSize.x,
                        near,
                        far)
                   && clip(from.y,
                           direction.y,
                           obstacle.center.y,
                           obstacle.halfSize.y,
                           near,
                           far)
                   && clip(from.z,
                           direction.z,
                           obstacle.center.z,
                           obstacle.halfSize.z,
                           near,
                           far);
        }

        auto
        BakeSurfaces(Apartment &scene) -> void
        {
            constexpr std::array order{ 0u, 1u, 2u, 1u, 3u, 2u };
            for (auto &surface : scene.surfaces)
            {
                for (std::size_t face = 0; face < kNormals.size(); ++face)
                {
                    const auto normal = kNormals[face];
                    const bool horizontal = face == 2 || face == 3;
                    const auto halfU
                        = face >= 4 ? surface.halfSize.z : surface.halfSize.x;
                    const auto halfV
                        = horizontal ? surface.halfSize.z : surface.halfSize.y;
                    const auto count = [&](float half) {
                        if (surface.exterior || surface.stairLamp
                            || half <= 0.84f)
                            return 1;
                        return std::clamp(
                            static_cast<int>(std::ceil(half * 2.0f / 0.91f)),
                            1,
                            16);
                    };
                    const int columns = count(halfU);
                    const int rows = count(halfV);
                    std::vector<SurfaceVertex> grid;
                    grid.reserve(
                        static_cast<std::size_t>((columns + 1) * (rows + 1)));
                    for (int row = 0; row <= rows; ++row)
                    {
                        for (int column = 0; column <= columns; ++column)
                        {
                            const float u = (2.0f * static_cast<float>(column)
                                                 / static_cast<float>(columns)
                                             - 1.0f)
                                            * halfU;
                            const float v = (2.0f * static_cast<float>(row)
                                                 / static_cast<float>(rows)
                                             - 1.0f)
                                            * halfV;
                            const Position point{
                                surface.center.x
                                    + (face >= 4 ? normal.x * surface.halfSize.x
                                                 : u),
                                surface.center.y
                                    + (horizontal
                                           ? normal.y * surface.halfSize.y
                                           : v),
                                surface.center.z
                                    + (face >= 4 ? u
                                       : horizontal
                                           ? v
                                           : normal.z * surface.halfSize.z)
                            };
                            SurfaceVertex vertex{ point,
                                                  face >= 4 ? point.z : point.x,
                                                  horizontal ? point.z
                                                             : point.y,
                                                  kLight[face],
                                                  {} };
                            if (!surface.exterior && !surface.stairLamp)
                            {
                                const Position sample{
                                    point.x + normal.x * 0.027f,
                                    point.y + normal.y * 0.027f,
                                    point.z + normal.z * 0.027f
                                };
                                for (std::size_t lamp = 0;
                                     lamp < scene.lights.size();
                                     ++lamp)
                                {
                                    const Position direction{
                                        scene.lights[lamp].x - sample.x,
                                        scene.lights[lamp].y - sample.y,
                                        scene.lights[lamp].z - sample.z
                                    };
                                    const float squared
                                        = std::max(0.01f,
                                                   Dot(direction, direction));
                                    const float incidence
                                        = std::max(0.0f,
                                                   Dot(direction, normal)
                                                       / std::sqrt(squared));
                                    if (incidence < 0.015f)
                                        continue;
                                    const bool blocked = std::ranges::any_of(
                                        scene.obstacles,
                                        [&](const auto &obstacle) {
                                            return obstacle.subject == 0
                                                   && SegmentHits(sample,
                                                                  direction,
                                                                  obstacle);
                                        });
                                    vertex.direct[lamp]
                                        = incidence * (blocked ? 0.16f : 1.0f)
                                          / (0.73f + 0.38f * squared);
                                }
                            }
                            grid.push_back(vertex);
                        }
                    }
                    for (int row = 0; row < rows; ++row)
                    {
                        for (int column = 0; column < columns; ++column)
                        {
                            const auto first = static_cast<std::size_t>(
                                row * (columns + 1) + column);
                            const auto stride
                                = static_cast<std::size_t>(columns + 1);
                            const std::array indices{ first,
                                                      first + 1,
                                                      first + stride,
                                                      first + stride + 1 };
                            for (const auto index : order)
                                surface.faces[face].push_back(
                                    grid[indices[index]]);
                        }
                    }
                }
            }
        }

        auto
        OutsideView(const Surface &surface,
                    Position eye,
                    const Camera &camera,
                    float aspect) noexcept -> bool
        {
            const Position offset{ surface.center.x - eye.x,
                                   surface.center.y - eye.y,
                                   surface.center.z - eye.z };
            const auto radius = [&](Position normal) {
                return std::abs(normal.x) * surface.halfSize.x
                       + std::abs(normal.y) * surface.halfSize.y
                       + std::abs(normal.z) * surface.halfSize.z;
            };
            const auto farthest
                = Dot(offset, camera.forward) + radius(camera.forward);
            return farthest <= 0.06f
                   || std::abs(Dot(offset, camera.right)) - radius(camera.right)
                          > farthest * aspect / 1.48f
                   || std::abs(Dot(offset, camera.up)) - radius(camera.up)
                          > farthest / 1.48f;
        }

        struct Paint
        {
            Position color;
            Material material;

            constexpr Paint(float red, float green, float blue)
                : color{ red, green, blue }
                , material{ Material::kFlat }
            {}

            explicit constexpr Paint(Position tint,
                                     Material basis = Material::kFlat)
                : color{ tint }
                , material{ basis }
            {}
        };

        auto
        AddWetGlint(Apartment &apartment,
                    Position center,
                    float length,
                    float width,
                    Position light,
                    Position asphalt) -> void
        {
            constexpr int sectors = 10;
            constexpr auto cycle = 2.0f * std::numbers::pi_v<float>;
            const auto edge = [&](int index) {
                const auto angle = static_cast<float>(index) * cycle
                                   / static_cast<float>(sectors);
                const auto irregularity
                    = 0.86f + 0.14f * std::sin(angle * 4.0f + center.z);
                return Position{
                    center.x + std::sin(angle) * width * irregularity,
                    center.y,
                    center.z + std::cos(angle) * length * irregularity
                };
            };
            for (int index = 0; index < sectors; ++index)
            {
                apartment.fixtures.push_back({ center, light });
                apartment.fixtures.push_back({ edge(index), asphalt });
                apartment.fixtures.push_back(
                    { edge((index + 1) % sectors), asphalt });
            }
        }

        auto
        AddHandrail(Apartment &apartment) -> void
        {
            constexpr int ring = 12;
            constexpr auto cycle = 2.0f * std::numbers::pi_v<float>;
            const Position railTint{ 0.26f, 0.16f, 0.095f };
            std::array<ActorPoint, ring> front;
            std::array<ActorPoint, ring> back;
            for (int index = 0; index < ring; ++index)
            {
                const auto angle = static_cast<float>(index) * cycle
                                   / static_cast<float>(ring);
                const auto across = std::cos(angle);
                const auto up = std::sin(angle);
                const Position offset{ across * 0.045f,
                                       up * 0.043f,
                                       up * 0.020f };
                const auto light = 0.68f + 0.40f * std::max(0.0f, up)
                                   + 0.10f * std::max(0.0f, across);
                const Position color{ railTint.x * light,
                                      railTint.y * light,
                                      railTint.z * light };
                front[static_cast<std::size_t>(index)] = {
                    { 1.77f + offset.x, 1.03f + offset.y, -1.06f + offset.z },
                    color
                };
                back[static_cast<std::size_t>(index)] = {
                    { 1.77f + offset.x, 3.38f + offset.y, -6.34f + offset.z },
                    color
                };
            }
            for (std::size_t index = 0; index < front.size(); ++index)
            {
                const auto next = (index + 1) % front.size();
                for (auto vertex : { front[index],
                                     front[next],
                                     back[index],
                                     front[next],
                                     back[next],
                                     back[index] })
                    apartment.fixtures.push_back(vertex);
            }
        }

        auto
        BuildExterior(Apartment &apartment) -> void
        {
            const auto add = [&](Position center,
                                 Position halfSize,
                                 Paint paint,
                                 int detail = 0) {
                apartment.surfaces.push_back({ center,
                                               halfSize,
                                               paint.color,
                                               detail,
                                               true,
                                               paint.material });
            };
            const Paint asphalt{ { 0.036f, 0.049f, 0.064f },
                                 Material::kAsphalt };
            const Paint pavement{ { 0.074f, 0.092f, 0.107f },
                                  Material::kStone };
            const Paint brick{ { 0.080f, 0.093f, 0.108f }, Material::kPlaster };
            const Paint trim{ { 0.115f, 0.133f, 0.151f }, Material::kStone };
            const Position voidColor{ 0.024f, 0.033f, 0.044f };

            add({ 0, -0.24f, 6.05f }, { 13, 0.19f, 0.99f }, pavement);
            add({ 0, -0.30f, 11.05f }, { 13, 0.16f, 4.3f }, asphalt);
            add({ 0, -0.26f, 15.15f }, { 13, 0.20f, 0.15f }, trim);
            add({ 0, -0.26f, 16.17f }, { 13, 0.20f, 0.93f }, pavement);
            add({ 0, 2.46f, 17.2f }, { 13, 2.57f, 0.25f }, brick);
            add({ 0, 5.01f, 17.2f }, { 13.2f, 0.12f, 0.43f }, trim);
            add({ 0, 2.10f, 16.95f }, { 13, 0.11f, 0.34f }, trim);
            for (int bay = -4; bay <= 4; ++bay)
            {
                const auto x = static_cast<float>(bay) * 2.65f + 0.25f;
                add({ x, 3.28f, 16.89f }, { 0.60f, 0.69f, 0.09f }, trim);
                const Position glass
                    = bay == -1  ? Position{ 0.54f, 0.39f, 0.23f }
                      : bay == 1 ? Position{ 0.29f, 0.32f, 0.29f }
                                 : voidColor;
                add({ x, 3.30f, 16.78f },
                    { 0.49f, 0.55f, 0.026f },
                    Paint{ glass });
                add({ x, 3.30f, 16.74f }, { 0.025f, 0.55f, 0.024f }, trim, 1);
                add({ x, 0.91f, 16.84f },
                    { 0.73f, 1.02f, 0.07f },
                    Paint{ voidColor });
                add({ x + 0.78f, 1.44f, 16.81f },
                    { 0.055f, 1.46f, 0.07f },
                    trim,
                    2);
                if (bay % 2 == 0)
                    add({ x, 1.91f, 16.61f }, { 0.84f, 0.08f, 0.31f }, trim, 1);
            }

            const Position pole{ 0.052f, 0.059f, 0.062f };
            add({ -1.38f, 1.89f, 14.55f },
                { 0.041f, 1.91f, 0.043f },
                Paint{ pole });
            add({ -1.38f, 3.76f, 14.35f },
                { 0.04f, 0.042f, 0.22f },
                Paint{ pole });
            add({ -1.38f, 3.71f, 14.12f },
                { 0.18f, 0.041f, 0.15f },
                { 0.87f, 0.61f, 0.33f });
            const Position reflectionEdge{ 0.035f, 0.045f, 0.055f };
            for (int band = 0; band < 19; ++band)
            {
                const auto index = static_cast<float>(band);
                const auto tone = 0.045f + 0.15f * (1.0f - index / 20.0f);
                const auto offset = std::sin(index * 2.7f) * 0.23f;
                AddWetGlint(apartment,
                            { -1.38f + offset,
                              -0.132f + index * 0.00018f,
                              14.0f - index * 0.37f },
                            0.28f + index * 0.011f,
                            0.39f + index * 0.011f,
                            { tone * 1.23f, tone * 0.89f, tone * 0.64f },
                            reflectionEdge);
            }
            for (int patch = -4; patch <= 4; ++patch)
            {
                const auto index = static_cast<float>(patch);
                AddWetGlint(apartment,
                            { index * 1.31f + 0.2f,
                              -0.136f,
                              9.85f + std::sin(index * 6.7f) * 2.2f },
                            0.55f,
                            0.88f,
                            { 0.062f, 0.083f, 0.102f },
                            reflectionEdge);
            }

            // Rain-darkened service alley, visible from the mid-stair window.
            add({ -5.9f, -0.19f, -2.5f }, { 3.9f, 0.12f, 7.3f }, asphalt);
            add({ -9.3f, 2.42f, -2.5f }, { 0.22f, 2.55f, 7.3f }, brick);
            add({ -9.09f, 1.1f, -3.24f },
                { 0.055f, 1.15f, 0.76f },
                Paint{ voidColor });
            add({ -9.07f, 2.52f, -3.24f },
                { 0.05f, 0.095f, 0.29f },
                { 0.31f, 0.39f, 0.43f });
            for (int index = 0; index < 5; ++index)
            {
                const auto z = 1.9f - static_cast<float>(index) * 1.8f;
                add({ -9.06f, 3.60f, z }, { 0.12f, 0.48f, 0.53f }, trim);
                add({ -8.94f, 3.61f, z },
                    { 0.018f, 0.36f, 0.42f },
                    Paint{ voidColor });
            }
        }

        auto
        Random(std::uint32_t &seed) noexcept -> float
        {
            seed = seed * 1664525U + 1013904223U;
            return static_cast<float>(seed >> 8U) * (1.0f / 16777216.0f);
        }

        template<typename Project>
        auto
        Rain(const Project &project,
             std::vector<Rendering::Vertex> &vertices,
             Position origin,
             Position extent,
             float seconds,
             int count) -> void
        {
            std::uint32_t seed = 2317;
            for (int index = 0; index < count; ++index)
            {
                const auto x = origin.x + extent.x * Random(seed);
                const auto z = origin.z + extent.z * Random(seed);
                const auto time
                    = Random(seed) + seconds * (0.78f + Random(seed) * 0.55f);
                const auto y
                    = origin.y + extent.y * (1.0f - (time - std::floor(time)));
                const auto shine = 0.16f + Random(seed) * 0.20f;
                const auto length = 0.13f + Random(seed) * 0.21f;
                const auto head = project({ x, y, z });
                if (head.w <= 0.10f || std::abs(head.x) > head.w * 1.25f
                    || std::abs(head.y) > head.w * 1.3f)
                    continue;
                const auto middle
                    = project({ x + 0.030f, y + length * 0.45f, z - 0.008f });
                const auto tail
                    = project({ x + 0.066f, y + length, z - 0.014f });
                auto base = middle;
                base.x += 0.009f * (1.0f + 0.22f * middle.w);
                auto streak = [&](Rendering::Vertex vertex, float level) {
                    vertex.red = level * 0.66f;
                    vertex.green = level * 0.82f;
                    vertex.blue = level;
                    vertices.push_back(vertex);
                };
                streak(head, shine * 0.55f);
                streak(middle, shine);
                streak(base, shine * 0.8f);
                streak(middle, shine);
                streak(tail, shine * 0.37f);
                streak(base, shine * 0.8f);
            }
        }
    } // namespace

    static auto
    CreateApartment() -> Apartment
    {
        using Investigation::Subject;
        Apartment apartment;
        apartment.checkpoint = { 0, 2.48f, -7.62f };
        const Paint plaster{ { 0.53f, 0.49f, 0.43f }, Material::kPlaster };
        const Paint stone{ { 0.32f, 0.34f, 0.34f }, Material::kStone };
        const Paint wood{ { 0.29f, 0.20f, 0.15f }, Material::kWood };
        const Paint steel{ { 0.16f, 0.20f, 0.22f }, Material::kMetal };
        const auto box = [&](Position center,
                             Position halfSize,
                             Paint paint,
                             bool solid = false,
                             int detail = 0) {
            apartment.surfaces.push_back({ center,
                                           halfSize,
                                           paint.color,
                                           detail,
                                           false,
                                           paint.material });
            if (solid)
                apartment.obstacles.push_back({ center, halfSize });
        };

        box({ 0, -0.16f, 2.0f }, { 1.99f, 0.16f, 3.00f }, stone, true);
        box({ 0, 2.26f, -7.72f }, { 1.99f, 0.14f, 1.08f }, stone, true);
        box({ 0, 5.08f, -2.0f }, { 2.2f, 0.12f, 7.15f }, plaster, true);
        // Glazing blocks traversal even though the street is visible through
        // it.
        apartment.obstacles.push_back(
            { { 0, 2.41f, 5.06f }, { 2.04f, 2.60f, 0.08f } });
        box({ 0, 0.22f, 5.06f }, { 2.04f, 0.25f, 0.09f }, wood);
        box({ 0, 3.77f, 5.06f }, { 2.04f, 1.24f, 0.09f }, plaster);
        box({ 0, 2.49f, 5.00f }, { 2.04f, 0.052f, 0.12f }, steel);
        box({ 0, 0.86f, 5.00f }, { 2.04f, 0.034f, 0.085f }, steel);
        for (int mullion = -2; mullion <= 2; ++mullion)
        {
            const auto x = static_cast<float>(mullion) * 0.985f;
            box({ x, 1.48f, 5.02f }, { 0.048f, 1.04f, 0.082f }, steel);
            box({ x + 0.034f, 1.48f, 4.934f },
                { 0.009f, 1.01f, 0.012f },
                { 0.24f, 0.29f, 0.30f },
                false,
                2);
        }
        box({ -0.13f, 1.03f, 4.91f },
            { 0.022f, 0.14f, 0.028f },
            { 0.47f, 0.34f, 0.22f });
        box({ 0, 2.52f, -8.88f }, { 2.04f, 2.61f, 0.10f }, plaster, true);
        for (const auto sign : { -1.0f, 1.0f })
        {
            if (sign > 0.0f)
                box({ 2.08f, 2.48f, -1.90f },
                    { 0.09f, 2.60f, 7.0f },
                    plaster,
                    true);
            else
            {
                apartment.obstacles.push_back(
                    { { -2.08f, 2.48f, -1.90f }, { 0.09f, 2.60f, 7.0f } });
                box({ -2.08f, 2.48f, 0.66f }, { 0.09f, 2.60f, 1.96f }, plaster);
                box({ -2.08f, 2.48f, 4.77f }, { 0.09f, 2.60f, 0.34f }, plaster);
                box({ -2.08f, 0.45f, 3.52f }, { 0.09f, 0.57f, 0.91f }, plaster);
                box({ -2.08f, 3.747f, 3.52f },
                    { 0.09f, 1.333f, 0.91f },
                    plaster);
                for (const auto z : { 2.62f, 3.52f, 4.42f })
                    box({ -1.99f, 1.72f, z }, { 0.075f, 0.70f, 0.036f }, steel);
                box({ -1.98f, 1.04f, 3.52f }, { 0.14f, 0.054f, 0.94f }, stone);
                box({ -1.99f, 2.40f, 3.52f }, { 0.078f, 0.045f, 0.91f }, steel);
                box({ -2.08f, 2.48f, -6.20f },
                    { 0.09f, 2.60f, 2.70f },
                    plaster);
                box({ -2.08f, 0.79f, -2.40f },
                    { 0.09f, 0.91f, 1.11f },
                    plaster);
                box({ -2.08f, 4.34f, -2.40f },
                    { 0.09f, 0.74f, 1.11f },
                    plaster);
                for (const auto z : { -3.48f, -2.40f, -1.32f })
                    box({ -1.99f, 2.66f, z }, { 0.09f, 0.96f, 0.048f }, steel);
                box({ -1.98f, 1.71f, -2.40f }, { 0.15f, 0.05f, 1.11f }, stone);
                box({ -1.99f, 2.66f, -2.40f }, { 0.08f, 0.034f, 1.08f }, steel);
                box({ -1.99f, 3.60f, -2.40f }, { 0.09f, 0.055f, 1.11f }, steel);
            }
            box({ sign * 1.98f, 0.28f, 2.0f }, { 0.025f, 0.27f, 2.95f }, wood);
            box({ sign * 1.94f, 0.55f, 2.0f }, { 0.04f, 0.025f, 2.95f }, wood);
        }

        for (int step = 0; step < 15; ++step)
        {
            const auto height = 0.16f * static_cast<float>(step + 1);
            const auto depth = -1.19f - static_cast<float>(step) * 0.36f;
            box({ 0, height / 2, depth },
                { 1.99f, height / 2, 0.18f },
                stone,
                true);
            box({ 0, height + 0.005f, depth + 0.16f },
                { 1.94f, 0.005f, 0.021f },
                wood);
            if (step % 3 == 0)
                box({ 1.77f, height + 0.47f, depth },
                    { 0.035f, 0.47f, 0.035f },
                    steel);
        }

        for (float z = 0.0f; z < 5.0f; z += 0.64f)
            box({ 0, 0.002f, z },
                { 1.96f, 0.002f, 0.008f },
                { 0.20f, 0.22f, 0.22f });
        for (float x = -1.28f; x <= 1.28f; x += 0.64f)
            box({ x, 0.004f, 2.0f },
                { 0.007f, 0.002f, 2.95f },
                { 0.20f, 0.22f, 0.22f });

        box({ -1.5f, 0.42f, 2.18f }, { 0.35f, 0.42f, 0.41f }, wood, true);
        box({ -1.5f, 0.84f, 2.18f }, { 0.39f, 0.045f, 0.46f }, wood);
        // Extra trim and fittings stay independent from the collision geometry.
        for (float side : { -1.0f, 1.0f })
        {
            box({ side * 1.968f, 0.115f, 0.48f },
                { 0.036f, 0.09f, 3.85f },
                wood,
                false,
                1);
            for (int bay = 0; bay < 7; ++bay)
            {
                const auto z = 4.75f - static_cast<float>(bay) * 1.55f;
                const auto y = z > -1.0f ? 0.7f : 1.06f + (-z - 1.0f) * 0.38f;
                box({ side * 1.970f, y, z },
                    { 0.020f, 0.32f, 0.015f },
                    stone,
                    false,
                    2);
                box({ side * 1.976f, y + 0.34f, z - 0.67f },
                    { 0.018f, 0.012f, 0.67f },
                    wood,
                    false,
                    3);
                box({ side * 1.972f, y + 0.25f, z + 0.11f },
                    { 0.021f, 0.009f, 0.08f },
                    { 0.46f, 0.39f, 0.31f },
                    false,
                    4);
            }
        }
        for (int index = 0; index < 3; ++index)
        {
            const auto z = 3.9f - static_cast<float>(index) * 5.75f;
            const auto y = index == 0
                               ? 2.56f
                               : 3.68f + static_cast<float>(index) * 0.52f;
            box({ 0, y, z }, { 0.38f, 0.065f, 0.13f }, steel, false, 1);
            box({ 0, y - 0.061f, z },
                { 0.29f, 0.023f, 0.09f },
                { 0.89f, 0.77f, 0.48f },
                false,
                2);
            apartment.surfaces.back().stairLamp = index == 1;
        }

        const Position dispatch{ -1.5f, 0.925f, 2.20f };
        box(dispatch, { 0.19f, 0.021f, 0.14f }, { 0.68f, 0.61f, 0.44f });
        apartment.obstacles.push_back(
            { dispatch,
              { 0.23f, 0.075f, 0.17f },
              static_cast<std::uint64_t>(Subject::kDispatch) });
        for (int line = 0; line < 4; ++line)
            box({ -1.5f, 0.948f, 2.13f + static_cast<float>(line) * 0.037f },
                { 0.13f, 0.002f, 0.003f },
                steel);

        const auto record = [&](Subject subject,
                                Position center,
                                Position halfSize,
                                Paint paint) {
            box(center, halfSize, paint);
            apartment.obstacles.push_back(
                { center, halfSize, static_cast<std::uint64_t>(subject) });
        };
        box({ 1.5f, 0.42f, 2.18f }, { 0.35f, 0.42f, 0.41f }, wood, true);
        box({ 1.5f, 0.84f, 2.18f }, { 0.39f, 0.045f, 0.46f }, wood);
        record(Subject::kCamera,
               { 1.5f, 1.10f, 2.20f },
               { 0.24f, 0.21f, 0.13f },
               steel);
        box({ 1.5f, 1.12f, 2.34f },
            { 0.19f, 0.14f, 0.014f },
            { 0.23f, 0.49f, 0.44f });

        // The two records sit at opposite ends of the route and use distinct
        // silhouettes.
        record(Subject::kDoorLog,
               { -1.85f, 1.40f, 0.12f },
               { 0.10f, 0.23f, 0.27f },
               steel);
        box({ -1.735f, 1.46f, 0.12f },
            { 0.015f, 0.12f, 0.21f },
            { 0.30f, 0.57f, 0.51f });
        record(Subject::kBreaker,
               { -1.85f, 3.58f, -7.32f },
               { 0.12f, 0.32f, 0.27f },
               steel);
        for (const float offset : { -0.15f, 0.0f, 0.15f })
            box({ -1.72f, 3.60f, -7.32f + offset },
                { 0.015f, 0.14f, 0.040f },
                { 0.64f, 0.47f, 0.28f });

        apartment.actors.push_back(
            CreateWitness({ -1.05f, 2.4f, -8.20f }, true));
        apartment.obstacles.push_back(
            { { -1.05f, 3.23f, -8.20f },
              { 0.30f, 0.83f, 0.17f },
              static_cast<std::uint64_t>(Subject::kElectrician) });
        apartment.actors.push_back(
            CreateWitness({ 0.85f, 2.4f, -8.08f }, false));
        apartment.obstacles.push_back(
            { { 0.85f, 3.23f, -8.08f },
              { 0.35f, 0.83f, 0.19f },
              static_cast<std::uint64_t>(Subject::kCaretaker) });
        box({ -0.42f, 3.54f, -8.75f }, { 0.51f, 1.10f, 0.05f }, wood);
        for (const auto stile : { -0.98f, 0.14f })
            box({ stile, 3.52f, -8.69f }, { 0.065f, 1.12f, 0.033f }, wood);
        box({ -0.42f, 4.64f, -8.69f }, { 0.62f, 0.055f, 0.035f }, wood);
        for (const auto inset : { 3.01f, 4.07f })
            box({ -0.42f, inset, -8.688f },
                { 0.36f, 0.41f, 0.019f },
                Paint{ { 0.22f, 0.153f, 0.11f }, Material::kWood });
        box({ -0.06f, 3.43f, -8.68f },
            { 0.065f, 0.022f, 0.024f },
            { 0.60f, 0.52f, 0.33f });
        apartment.obstacles.push_back(
            { { 1.79f, 1.75f, -2.73f },
              { 0.22f, 0.18f, 0.27f },
              static_cast<std::uint64_t>(Subject::kRecorder) });
        record(Subject::kRevision,
               { 1.85f, 3.69f, -7.03f },
               { 0.13f, 0.24f, 0.24f },
               wood);
        box({ 1.707f, 3.69f, -7.03f },
            { 0.017f, 0.19f, 0.15f },
            { 0.75f, 0.69f, 0.52f });
        AddHandrail(apartment);
        BuildExterior(apartment);
        return apartment;
    }

    auto
    CreateInterior(Investigation::CaseId id) -> Apartment;

    auto
    CreateScene(Investigation::CaseId id) -> Apartment
    {
        auto scene = id == Investigation::CaseId::kApartment
                         ? CreateApartment()
                         : CreateInterior(id);
        BakeSurfaces(scene);
        return scene;
    }

    auto
    View(float yaw, float pitch) -> Camera
    {
        const auto horizontal = std::cos(pitch);
        return {
            { std::sin(yaw) * horizontal,
              std::sin(pitch),
              -std::cos(yaw) * horizontal },
            { std::cos(yaw), 0, std::sin(yaw) },
            { -std::sin(yaw) * std::sin(pitch),
              horizontal,
              std::cos(yaw) * std::sin(pitch) },
        };
    }

    auto
    BuildFrame(const Apartment &apartment,
               Physics::Position eye,
               const Camera &camera,
               float aspect,
               bool isTargeted,
               SceneQuality quality,
               std::vector<Rendering::Vertex> &vertices,
               float weatherSeconds,
               float stairPower,
               float projection) -> void
    {
        vertices.clear();
        constexpr float kNear = 0.06f;
        constexpr float kFar = 46.0f;
        constexpr float kFieldOfView = 1.48f;
        constexpr float kDepth = kFar / (kFar - kNear);
        const auto project = [&](Position world) {
            const Position point{ world.x - eye.x,
                                  world.y - eye.y,
                                  world.z - eye.z };
            const auto depth = Dot(point, camera.forward);
            return Rendering::Vertex{ Dot(point, camera.right) * kFieldOfView
                                          / aspect,
                                      Dot(point, camera.up) * kFieldOfView,
                                      (depth - kNear) * kDepth,
                                      depth,
                                      0,
                                      0,
                                      0 };
        };
        constexpr std::array tints{ Position{ 1.00f, 0.76f, 0.54f },
                                    Position{ 0.74f, 0.88f, 1.00f },
                                    Position{ 1.00f, 0.81f, 0.60f },
                                    Position{ 0.65f, 0.92f, 0.88f } };
        for (const auto &surface : apartment.surfaces)
        {
            if (surface.detail > quality.details
                || OutsideView(surface, eye, camera, aspect))
                continue;
            const auto stairWeight
                = surface.exterior
                      ? 0.0f
                      : std::clamp(1.0f
                                       - std::abs(surface.center.z
                                                  - apartment.eventDepth)
                                             / 4.0f,
                                   0.0f,
                                   1.0f);
            const auto ambientPower
                = 1.0f - (1.0f - stairPower) * stairWeight * 0.62f;
            const auto material = quality.details > 0
                                      ? static_cast<float>(surface.material)
                                      : 0.0f;
            const Position relative{ eye.x - surface.center.x,
                                     eye.y - surface.center.y,
                                     eye.z - surface.center.z };
            const bool inside = std::abs(relative.x) <= surface.halfSize.x
                                && std::abs(relative.y) <= surface.halfSize.y
                                && std::abs(relative.z) <= surface.halfSize.z;
            for (std::size_t face = 0; face < kNormals.size(); ++face)
            {
                const auto normal = kNormals[face];
                if (!inside
                    && Dot(relative, normal)
                           <= std::abs(Dot(surface.halfSize, normal)))
                    continue;
                for (const auto &point : surface.faces[face])
                {
                    Position light;
                    if (surface.exterior || surface.stairLamp
                        || quality.lighting == 0)
                    {
                        const auto level
                            = point.ambient
                              * (surface.stairLamp ? stairPower : ambientPower);
                        light = { level, level, level };
                    }
                    else
                    {
                        const auto level = point.ambient * 0.58f * ambientPower;
                        light = { level * 0.83f, level * 0.91f, level };
                        for (int index = 0; index < quality.lighting; ++index)
                        {
                            const auto lamp = static_cast<std::size_t>(index);
                            const auto amount
                                = point.direct[lamp]
                                  * (lamp == 1 ? stairPower : 1.0f);
                            light.x += amount * tints[lamp].x;
                            light.y += amount * tints[lamp].y;
                            light.z += amount * tints[lamp].z;
                        }
                    }
                    auto vertex = project(point.position);
                    vertex.red = std::min(1.0f, surface.color.x * light.x);
                    vertex.green = std::min(1.0f, surface.color.y * light.y);
                    vertex.blue = std::min(1.0f, surface.color.z * light.z);
                    vertex.textureU = point.textureU;
                    vertex.textureV = point.textureV;
                    vertex.material = material;
                    vertices.push_back(vertex);
                }
            }
        }

        for (const auto &point : apartment.fixtures)
        {
            auto vertex = project(point.position);
            vertex.red = point.color.x;
            vertex.green = point.color.y;
            vertex.blue = point.color.z;
            vertices.push_back(vertex);
        }

        for (const auto &actor : apartment.actors)
        {
            const Position offset{ actor.center.x - eye.x,
                                   actor.center.y - eye.y,
                                   actor.center.z - eye.z };
            if (Dot(offset, offset) > 121.0f
                || Dot(offset, camera.forward) < -0.55f)
                continue;
            const auto drift
                = std::sin(weatherSeconds * 1.20f + actor.center.x) * 0.008f;
            for (const auto &point : actor.triangles)
            {
                auto position = point.position;
                position.x += drift
                              * std::clamp(position.y - actor.center.y + 0.35f,
                                           0.0f,
                                           1.0f);
                auto vertex = project(position);
                vertex.red = point.color.x;
                vertex.green = point.color.y;
                vertex.blue = point.color.z;
                vertices.push_back(vertex);
            }
        }

        if (apartment.cinema && projection > 0.0f)
        {
            const auto exposure
                = projection
                  * (0.91f + 0.09f * std::sin(weatherSeconds * 74.1f));
            const auto screenRect = [&](float x,
                                        float y,
                                        float width,
                                        float height,
                                        float depth,
                                        Position tint) {
                const std::array quad{
                    Position{ x, y, depth },
                    Position{ x + width, y, depth },
                    Position{ x, y + height, depth },
                    Position{ x + width, y + height, depth }
                };
                for (const auto index : kFaces[1])
                {
                    auto vertex = project(quad[index - 4]);
                    vertex.red = tint.x * exposure;
                    vertex.green = tint.y * exposure;
                    vertex.blue = tint.z * exposure;
                    vertices.push_back(vertex);
                }
            };
            screenRect(-3.49f,
                       0.99f,
                       6.98f,
                       2.10f,
                       -16.04f,
                       { 0.78f, 0.72f, 0.59f });
            for (int row = 0; row < 3; ++row)
            {
                const auto y = 1.09f + static_cast<float>(row) * 0.36f;
                for (int seat = -4; seat <= 4; ++seat)
                    screenRect(static_cast<float>(seat) * 0.67f - 0.15f,
                               y,
                               0.41f,
                               0.18f,
                               -16.03f,
                               { 0.25f, 0.20f, 0.16f });
            }
            // The revived image differs from the empty balcony seen in the
            // film.
            if (projection > 0.50f)
            {
                screenRect(0.56f,
                           1.98f,
                           0.27f,
                           0.34f,
                           -16.02f,
                           { 0.17f, 0.16f, 0.15f });
                screenRect(0.60f,
                           2.30f,
                           0.18f,
                           0.22f,
                           -16.02f,
                           { 0.17f, 0.16f, 0.15f });
            }
        }

        if (apartment.rain && eye.z > 1.5f && camera.forward.z > -0.38f)
        {
            Rain(project,
                 vertices,
                 { -8.5f, 0.0f, 5.19f },
                 { 17, 6, 11.7f },
                 weatherSeconds,
                 360 + quality.details * 150);
            Rain(project,
                 vertices,
                 { -1.90f, 0.48f, 5.14f },
                 { 3.8f, 2.0f, 0.21f },
                 weatherSeconds * 0.59f,
                 32 + quality.details * 12);
        }
        if (apartment.rain && eye.z > -6.0f && eye.z < 4.65f
            && camera.forward.x < 0.41f)
            Rain(project,
                 vertices,
                 { -8.7f, 0.0f, -6.5f },
                 { 6.52f, 6, 11.6f },
                 weatherSeconds,
                 240 + quality.details * 80);

        const auto rect
            = [&](float left, float bottom, float right, float top) {
                  const float green = isTargeted ? 0.76f : 0.88f;
                  const float blue = isTargeted ? 0.29f : 0.88f;
                  for (const auto point : std::array<Position, 6>{ {
                           { left, bottom },
                           { right, bottom },
                           { left, top },
                           { right, bottom },
                           { right, top },
                           { left, top },
                       } })
                      vertices.push_back(
                          { point.x, point.y, 0, 1, 0.95f, green, blue });
              };
        rect(-0.008f / aspect, -0.0017f, 0.008f / aspect, 0.0017f);
        rect(-0.0017f / aspect, -0.008f, 0.0017f / aspect, 0.008f);
    }
} // namespace TwentyThreeSeventeen::Game
