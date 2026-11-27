// SPDX-License-Identifier: MIT
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

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
        }

        for (float z = 0.0f; z < 5.0f; z += 0.64f)
            box({ 0, 0.002f, z },
                { 1.96f, 0.002f, 0.008f },
                { 0.20f, 0.22f, 0.22f });
        for (float x = -1.28f; x <= 1.28f; x += 0.64f)
            box({ x, 0.004f, 2.0f },
                { 0.007f, 0.002f, 2.95f },
                { 0.20f, 0.22f, 0.22f });

        // Furniture in apartment-joinery.glb uses these Jolt footprints.
        for (float side : { -1.0f, 1.0f })
            apartment.obstacles.push_back(
                { { side * 1.5f, 0.42f, 2.18f }, { 0.35f, 0.42f, 0.41f } });
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
        apartment.obstacles.push_back(
            { dispatch,
              { 0.23f, 0.075f, 0.17f },
              static_cast<std::uint64_t>(Subject::kDispatch) });

        const auto record = [&](Subject subject,
                                Position center,
                                Position halfSize,
                                Paint paint) {
            box(center, halfSize, paint);
            apartment.obstacles.push_back(
                { center, halfSize, static_cast<std::uint64_t>(subject) });
        };
        apartment.obstacles.push_back(
            { { 1.5f, 1.10f, 2.20f },
              { 0.24f, 0.21f, 0.13f },
              static_cast<std::uint64_t>(Subject::kCamera) });

        apartment.obstacles.push_back(
            { { -1.85f, 1.40f, 0.12f },
              { 0.10f, 0.23f, 0.27f },
              static_cast<std::uint64_t>(Subject::kDoorLog) });
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
        BuildExterior(apartment);
        return apartment;
    }

    auto
    CreateInterior(Investigation::CaseId id) -> Apartment;

    auto
    CreateScene(Investigation::CaseId id) -> Apartment
    {
        return id == Investigation::CaseId::kApartment ? CreateApartment()
                                                       : CreateInterior(id);
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
