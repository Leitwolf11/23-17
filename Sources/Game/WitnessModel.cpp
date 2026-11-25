// SPDX-License-Identifier: MIT
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>

#include "Game/WitnessModel.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        using Physics::Position;
        constexpr float kPi = std::numbers::pi_v<float>;

        auto
        Shade(Position color, Position normal) noexcept -> Position
        {
            const auto light
                = 0.74f
                  + 0.30f
                        * std::max(0.0f,
                                   normal.x * -0.37f + normal.y * 0.63f
                                       + normal.z * 0.67f);
            return { color.x * light, color.y * light, color.z * light };
        }

        auto
        Gaussian(float x,
                 float y,
                 float centerX,
                 float centerY,
                 float width,
                 float height) noexcept -> float
        {
            const auto u = (x - centerX) / width;
            const auto v = (y - centerY) / height;
            return std::exp(-(u * u + v * v));
        }

        template<typename Sample>
        auto
        Surface(Actor &actor, int sectors, int rings, const Sample &point)
            -> void
        {
            std::vector<ActorPoint> samples;
            samples.reserve(static_cast<std::size_t>((rings + 1) * sectors));
            for (int row = 0; row <= rings; ++row)
            {
                const auto t
                    = static_cast<float>(row) / static_cast<float>(rings);
                for (int sector = 0; sector < sectors; ++sector)
                    samples.push_back(
                        point(t,
                              2.0f * kPi * static_cast<float>(sector)
                                  / static_cast<float>(sectors)));
            }
            for (int row = 0; row < rings; ++row)
            {
                for (int sector = 0; sector < sectors; ++sector)
                {
                    const auto index
                        = static_cast<std::size_t>(row * sectors + sector);
                    const auto next = static_cast<std::size_t>(
                        row * sectors + (sector + 1) % sectors);
                    const auto upper = static_cast<std::size_t>(sectors);
                    for (const auto vertex : { index,
                                               next,
                                               index + upper,
                                               next,
                                               next + upper,
                                               index + upper })
                        actor.triangles.push_back(samples[vertex]);
                }
            }
        }

        auto
        Oval(Actor &actor,
             Position center,
             Position radius,
             Position color,
             float lean = 0.0f,
             int sectors = 12,
             int rings = 8) -> void
        {
            Surface(actor, sectors, rings, [&](float t, float heading) {
                const auto latitude = (t - 0.5f) * kPi;
                const auto plane = std::cos(latitude);
                const Position normal{ plane * std::sin(heading),
                                       std::sin(latitude),
                                       plane * std::cos(heading) };
                const auto y = normal.y * radius.y;
                return ActorPoint{ { center.x + normal.x * radius.x + y * lean,
                                     center.y + y,
                                     center.z + normal.z * radius.z },
                                   Shade(color, normal) };
            });
        }

        struct ClothingSection
        {
            Position position;
            float width;
            float depth;
        };

        auto
        Garment(Actor &actor,
                std::span<const ClothingSection> sections,
                Position tint) -> void
        {
            const auto intervals = static_cast<int>(sections.size()) - 1;
            Surface(actor, 18, intervals * 3, [&](float t, float heading) {
                const auto along = static_cast<float>(intervals) * t;
                const auto index
                    = std::min(intervals - 1, static_cast<int>(along));
                const auto weight = along - static_cast<float>(index);
                const auto &lower = sections[static_cast<std::size_t>(index)];
                const auto &upper
                    = sections[static_cast<std::size_t>(index + 1)];
                const auto across = std::sin(heading);
                const auto depth = std::cos(heading);
                const auto fold
                    = 1.0f + 0.033f * std::sin(heading * 6.0f + along * 2.0f)
                      + 0.015f * std::cos(heading * 11.0f - along * 4.7f);
                const Position position{
                    std::lerp(lower.position.x, upper.position.x, weight)
                        + across * std::lerp(lower.width, upper.width, weight)
                              * fold,
                    std::lerp(lower.position.y, upper.position.y, weight),
                    std::lerp(lower.position.z, upper.position.z, weight)
                        + depth * std::lerp(lower.depth, upper.depth, weight)
                              * fold,
                };
                return ActorPoint{ position,
                                   Shade(tint, { across, 0.10f, depth }) };
            });
        }

        auto
        Head(Actor &actor,
             Position center,
             bool electrician,
             Position complexion) -> void
        {
            const auto profile = [&](float t, float heading) -> Position {
                const auto latitude = (t - 0.5f) * kPi;
                const auto plane = std::cos(latitude);
                const auto y = std::sin(latitude) * 0.124f;
                const auto jaw
                    = 0.79f
                      + 0.21f * std::clamp((y + 0.077f) / 0.084f, 0.0f, 1.0f);
                const auto x = plane * std::sin(heading)
                               * (electrician ? 0.078f : 0.084f) * jaw;
                const auto front
                    = std::clamp((std::cos(heading) - 0.38f) / 0.62f,
                                 0.0f,
                                 1.0f);
                const auto nose
                    = 0.031f * Gaussian(x, y, 0, -0.011f, 0.015f, 0.022f)
                      + 0.009f * Gaussian(x, y, 0, 0.018f, 0.009f, 0.034f);
                const auto cheek = 0.010f
                                   * Gaussian(std::abs(x),
                                              y,
                                              0.043f,
                                              -0.016f,
                                              0.023f,
                                              0.030f);
                const auto socket = 0.010f
                                    * Gaussian(std::abs(x),
                                               y,
                                               0.033f,
                                               0.020f,
                                               0.018f,
                                               0.013f);
                const auto chin
                    = 0.013f * Gaussian(x, y, 0, -0.080f, 0.040f, 0.030f);
                const auto z = plane * std::cos(heading) * 0.089f
                               + front * (nose + cheek + chin - socket);
                return { x, y, z };
            };
            Surface(actor, 32, 26, [&](float t, float heading) {
                const auto point = profile(t, heading);
                const auto angleBefore = profile(t, heading - 0.01f);
                const auto angleAfter = profile(t, heading + 0.01f);
                const auto above = profile(std::min(1.0f, t + 0.005f), heading);
                const auto below = profile(std::max(0.0f, t - 0.005f), heading);
                const Position axisX{ angleAfter.x - angleBefore.x,
                                      angleAfter.y - angleBefore.y,
                                      angleAfter.z - angleBefore.z };
                const Position axisY{ above.x - below.x,
                                      above.y - below.y,
                                      above.z - below.z };
                const Position cross{ axisX.y * axisY.z - axisX.z * axisY.y,
                                      axisX.z * axisY.x - axisX.x * axisY.z,
                                      axisX.x * axisY.y - axisX.y * axisY.x };
                const auto length
                    = std::max(1.0e-9f,
                               std::sqrt(cross.x * cross.x + cross.y * cross.y
                                         + cross.z * cross.z));
                const Position normal{ cross.x / length,
                                       cross.y / length,
                                       cross.z / length };
                const auto front = std::max(0.0f, std::cos(heading));
                const auto flush = front * 0.032f
                                   * Gaussian(std::abs(point.x),
                                              point.y,
                                              0.043f,
                                              -0.016f,
                                              0.023f,
                                              0.028f);
                auto skin = Shade({ complexion.x + flush,
                                    complexion.y - flush,
                                    complexion.z - flush },
                                  normal);
                if (!electrician)
                {
                    const auto stubble = front * 0.18f
                                         * Gaussian(point.x,
                                                    point.y,
                                                    0,
                                                    -0.068f,
                                                    0.070f,
                                                    0.043f);
                    skin = { skin.x * (1.0f - stubble),
                             skin.y * (1.0f - stubble),
                             skin.z * (1.0f - stubble) };
                }
                return ActorPoint{ { center.x + point.x,
                                     center.y + point.y,
                                     center.z + point.z },
                                   skin };
            });

            const Position hair{ 0.095f, 0.073f, 0.054f };
            const Position eye{ 0.47f, 0.41f, 0.36f };
            const Position lip = electrician ? Position{ 0.33f, 0.17f, 0.12f }
                                             : Position{ 0.22f, 0.13f, 0.10f };
            for (float side : { -1.0f, 1.0f })
            {
                Oval(actor,
                     { center.x + side * 0.077f,
                       center.y - 0.004f,
                       center.z - 0.005f },
                     { 0.013f, 0.026f, 0.012f },
                     complexion);
                Oval(actor,
                     { center.x + side * 0.032f,
                       center.y + 0.019f,
                       center.z + 0.078f },
                     { 0.013f, 0.0046f, 0.004f },
                     eye,
                     0.0f,
                     10,
                     6);
                Oval(actor,
                     { center.x + side * 0.032f,
                       center.y + 0.019f,
                       center.z + 0.083f },
                     { 0.005f, 0.0048f, 0.0018f },
                     hair,
                     0.0f,
                     10,
                     6);
                Oval(actor,
                     { center.x + side * 0.033f,
                       center.y + 0.033f,
                       center.z + 0.085f },
                     { 0.016f, 0.0030f, 0.0025f },
                     hair);
            }
            Oval(actor,
                 { center.x, center.y - 0.052f, center.z + 0.089f },
                 { 0.025f, 0.0033f, 0.005f },
                 lip,
                 0.0f,
                 12,
                 6);

            if (electrician)
            {
                const Position shell{ 0.65f, 0.46f, 0.15f };
                Oval(actor,
                     { center.x, center.y + 0.086f, center.z - 0.004f },
                     { 0.105f, 0.060f, 0.105f },
                     shell,
                     0.0f,
                     20,
                     10);
                Oval(actor,
                     { center.x, center.y + 0.067f, center.z + 0.016f },
                     { 0.110f, 0.007f, 0.119f },
                     { 0.48f, 0.34f, 0.11f },
                     0.0f,
                     20,
                     6);
            }
            else
            {
                Surface(actor, 22, 9, [&](float t, float heading) {
                    const auto front
                        = std::clamp((std::cos(heading) + 0.25f) / 1.19f,
                                     0.0f,
                                     1.0f);
                    const auto line = -0.57f + front * 1.23f
                                      + 0.11f * std::sin(heading * 5.0f);
                    const auto latitude = std::lerp(line, kPi * 0.5f, t);
                    const Position normal{
                        std::cos(latitude) * std::sin(heading),
                        std::sin(latitude),
                        std::cos(latitude) * std::cos(heading)
                    };
                    return ActorPoint{ { center.x + normal.x * 0.086f,
                                         center.y + normal.y * 0.129f,
                                         center.z + normal.z * 0.093f },
                                       Shade({ 0.19f, 0.165f, 0.143f },
                                             normal) };
                });
            }
        }
    } // namespace

    auto
    CreateWitness(Physics::Position feet, bool electrician) -> Actor
    {
        Actor actor{ { feet.x, feet.y + 0.85f, feet.z }, {} };
        const Position jacket = electrician ? Position{ 0.18f, 0.30f, 0.37f }
                                            : Position{ 0.22f, 0.29f, 0.23f };
        const Position lining{ 0.12f, 0.14f, 0.13f };
        const Position trousers{ 0.18f, 0.19f, 0.18f };
        const Position shoe{ 0.095f, 0.080f, 0.067f };
        const Position skin = electrician ? Position{ 0.57f, 0.37f, 0.27f }
                                          : Position{ 0.61f, 0.45f, 0.37f };
        const auto at = [&](float x, float y, float z) -> Position {
            return { feet.x + x, feet.y + y, feet.z + z };
        };

        const std::array jacketShape{
            ClothingSection{ at(0, 0.84f, 0.002f), 0.152f, 0.109f },
            ClothingSection{ at(0, 0.92f, 0.002f), 0.176f, 0.125f },
            ClothingSection{ at(0, 1.11f, 0.000f), 0.187f, 0.131f },
            ClothingSection{ at(0, 1.31f, 0.000f), 0.212f, 0.129f },
            ClothingSection{ at(0, 1.40f, 0.002f), 0.174f, 0.105f },
            ClothingSection{ at(0, 1.47f, 0.008f), 0.076f, 0.073f },
            ClothingSection{ at(0, 1.49f, 0.012f), 0.054f, 0.058f },
        };
        Garment(actor, jacketShape, jacket);
        Oval(actor, at(0, 1.47f, 0.006f), { 0.078f, 0.044f, 0.073f }, lining);
        Oval(actor, at(0, 1.505f, 0.012f), { 0.044f, 0.086f, 0.045f }, skin);
        Oval(actor, at(0, 1.15f, 0.13f), { 0.006f, 0.25f, 0.006f }, lining);
        Oval(actor,
             at(0, 1.35f, 0.125f),
             { 0.012f, 0.019f, 0.008f },
             { 0.39f, 0.36f, 0.29f });
        for (float side : { -1.0f, 1.0f })
        {
            const std::array leg{
                ClothingSection{ at(side * 0.098f, 0.09f, 0.034f),
                                 0.060f,
                                 0.059f },
                ClothingSection{ at(side * 0.102f, 0.27f, 0.019f),
                                 0.066f,
                                 0.064f },
                ClothingSection{ at(side * 0.106f, 0.435f, 0.015f),
                                 0.077f,
                                 0.077f },
                ClothingSection{ at(side * 0.102f, 0.70f, 0.002f),
                                 0.087f,
                                 0.093f },
                ClothingSection{ at(side * 0.096f, 0.94f, 0.002f),
                                 0.086f,
                                 0.100f },
            };
            Garment(actor, leg, trousers);
            Oval(actor,
                 at(side * 0.098f, 0.062f, 0.067f),
                 { 0.068f, 0.055f, 0.135f },
                 shoe);
            const std::array sleeve{
                ClothingSection{ at(side * 0.266f, 0.803f, 0.040f),
                                 0.041f,
                                 0.044f },
                ClothingSection{ at(side * 0.266f, 0.866f, 0.033f),
                                 0.049f,
                                 0.051f },
                ClothingSection{ at(side * 0.255f, 1.04f, 0.021f),
                                 0.058f,
                                 0.065f },
                ClothingSection{ at(side * 0.240f, 1.24f, 0.001f),
                                 0.076f,
                                 0.078f },
                ClothingSection{ at(side * 0.198f, 1.39f, 0.002f),
                                 0.053f,
                                 0.063f },
                ClothingSection{ at(side * 0.150f, 1.425f, 0.003f),
                                 0.013f,
                                 0.016f },
            };
            Garment(actor, sleeve, jacket);
            Oval(actor,
                 at(side * 0.267f, 0.755f, 0.061f),
                 { 0.035f, 0.064f, 0.030f },
                 skin);
            Oval(actor,
                 at(side * 0.248f, 0.765f, 0.081f),
                 { 0.015f, 0.036f, 0.014f },
                 skin);
            if (electrician)
                Oval(actor,
                     at(side * 0.114f, 1.21f, 0.108f),
                     { 0.016f, 0.145f, 0.014f },
                     { 0.55f, 0.51f, 0.26f });
            else
                Oval(actor,
                     at(side * 0.123f, 1.025f, 0.105f),
                     { 0.051f, 0.014f, 0.010f },
                     lining);
        }
        Head(actor, at(0, 1.64f, 0.010f), electrician, skin);
        return actor;
    }
} // namespace TwentyThreeSeventeen::Game
