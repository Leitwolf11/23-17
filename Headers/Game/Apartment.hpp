// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <vector>

#include "Game/Settings.hpp"
#include "Investigation/Case.hpp"
#include "Physics/Player.hpp"
#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Game
{
    enum class Material
    {
        kFlat,
        kPlaster,
        kStone,
        kWood,
        kAsphalt,
        kMetal
    };

    struct SurfaceVertex
    {
        Physics::Position position;
        float textureU;
        float textureV;
        float ambient;
        std::array<float, 4> direct;
    };

    struct Surface
    {
        Physics::Position center;
        Physics::Position halfSize;
        Physics::Position color;
        int detail{};
        bool exterior{};
        Material material{ Material::kFlat };
        bool stairLamp{};
        std::array<std::vector<SurfaceVertex>, 6> faces{};
    };

    struct ActorPoint
    {
        Physics::Position position;
        Physics::Position color;
    };

    struct Actor
    {
        Physics::Position center;
        std::vector<ActorPoint> triangles;
    };

    struct Apartment
    {
        std::vector<Surface> surfaces;
        std::vector<Physics::Obstacle> obstacles;
        std::vector<Actor> actors;
        std::vector<ActorPoint> fixtures;
        std::array<Physics::Position, 4> lights{ { { 0, 2.53f, 3.90f },
                                                   { 0, 4.17f, -1.85f },
                                                   { 0, 4.69f, -7.60f },
                                                   { -1.2f, 1.20f, 1.60f } } };
        Physics::Position checkpoint{};
        bool rain{ true };
        bool cinema{};
        float eventDepth{ -2.9f };
    };

    struct Camera
    {
        Physics::Position forward;
        Physics::Position right;
        Physics::Position up;
    };

    [[nodiscard]] auto
    CreateScene(Investigation::CaseId id) -> Apartment;
    [[nodiscard]] auto
    View(float yaw, float pitch) -> Camera;

    auto
    BuildFrame(const Apartment &apartment,
               Physics::Position eye,
               const Camera &camera,
               float aspect,
               bool isTargeted,
               SceneQuality quality,
               std::vector<Rendering::Vertex> &vertices,
               float weatherSeconds = 0.0f,
               float stairPower = 1.0f,
               float projection = 0.0f) -> void;
} // namespace TwentyThreeSeventeen::Game
