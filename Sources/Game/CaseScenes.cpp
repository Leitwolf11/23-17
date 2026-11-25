// SPDX-License-Identifier: MIT
#include <utility>

#include "Game/Apartment.hpp"
#include "Game/WitnessModel.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        using Physics::Position;
        using enum Investigation::Subject;
        using Subject = Investigation::Subject;

        class SceneBuilder
        {
        public:
            Apartment scene;

            auto
            Box(Position center,
                Position half,
                Position color,
                Material material,
                bool solid = false,
                int detail = 0) -> void
            {
                scene.surfaces.push_back(
                    { center, half, color, detail, false, material });
                if (solid)
                    scene.obstacles.push_back({ center, half });
            }

            auto
            Record(Subject source,
                   Position center,
                   Position half,
                   Position color,
                   Material material = Material::kMetal) -> void
            {
                Box(center, half, color, material);
                scene.obstacles.push_back(
                    { center, half, static_cast<std::uint64_t>(source) });
            }

            auto
            Witness(Subject source, Position feet, bool worker) -> void
            {
                scene.actors.push_back(CreateWitness(feet, worker));
                scene.obstacles.push_back(
                    { { feet.x, feet.y + 0.83f, feet.z },
                      { 0.30f, 0.83f, 0.18f },
                      static_cast<std::uint64_t>(source) });
            }

            auto
            Shell(Position wall, Position floor, float width, float back)
                -> void
            {
                const auto center = (5.0f + back) * 0.5f;
                const auto depth = (5.0f - back) * 0.5f;
                Box({ 0, -0.15f, center },
                    { width, 0.15f, depth },
                    floor,
                    Material::kStone,
                    true);
                Box({ 0, 3.60f, center },
                    { width, 0.10f, depth },
                    wall,
                    Material::kPlaster,
                    true);
                for (const auto side : { -1.0f, 1.0f })
                {
                    Box({ side * width, 1.76f, center },
                        { 0.10f, 1.76f, depth },
                        wall,
                        Material::kPlaster,
                        true);
                    Box({ side * width * 0.975f, 0.18f, center },
                        { 0.045f, 0.18f, depth },
                        { 0.16f, 0.16f, 0.15f },
                        Material::kWood);
                }
                for (const auto end : { 5.0f, back })
                    Box({ 0, 1.76f, end },
                        { width, 1.76f, 0.10f },
                        wall,
                        Material::kPlaster,
                        true);
                scene.rain = false;
            }
        };

        auto
        CreateArchive() -> Apartment
        {
            SceneBuilder room;
            constexpr Position steel{ 0.28f, 0.37f, 0.38f };
            constexpr Position paper{ 0.60f, 0.51f, 0.33f };
            constexpr Position wood{ 0.28f, 0.22f, 0.15f };
            constexpr Position lamp{ 0.72f, 0.84f, 0.78f };
            using enum Material;
            room.Shell({ 0.37f, 0.44f, 0.43f },
                       { 0.25f, 0.32f, 0.31f },
                       4.6f,
                       -13.2f);
            room.scene.checkpoint = { 0, 0.08f, -9.65f };
            room.scene.eventDepth = -5.4f;
            room.scene.lights = { { { 0, 3.12f, 2 },
                                    { 0, 3.12f, -5.4f },
                                    { 0, 3.12f, -10.8f },
                                    { -1.25f, 1.8f, 2 } } };
            for (const auto depth : { 2.0f, -2.0f, -5.4f, -8.1f, -11.5f })
            {
                room.Box({ 0, 3.25f, depth },
                         { 0.61f, 0.06f, 0.14f },
                         steel,
                         kMetal);
                room.Box({ 0, 3.18f, depth },
                         { 0.54f, 0.03f, 0.10f },
                         lamp,
                         kFlat);
                room.scene.surfaces.back().stairLamp
                    = depth < -5.3f && depth > -5.5f;
            }
            for (const auto side : { -1.0f, 1.0f })
            {
                for (int bay = 0; bay < 3; ++bay)
                {
                    const auto depth = -2.1f - static_cast<float>(bay) * 2.4f;
                    for (const auto end : { -1.08f, 1.08f })
                        room.Box({ side * 2.76f, 1.30f, depth + end },
                                 { 0.79f, 1.30f, 0.055f },
                                 steel,
                                 kMetal,
                                 true);
                    for (const auto shelf : { 0.30f, 0.92f, 1.57f, 2.19f })
                    {
                        room.Box({ side * 2.76f, shelf, depth },
                                 { 0.79f, 0.036f, 1.10f },
                                 steel,
                                 kMetal,
                                 true);
                        for (int index = -2; index <= 2; ++index)
                        {
                            if (side < 0 && bay == 1 && index == 0
                                && shelf < 1.0f)
                                continue;
                            const auto shade
                                = static_cast<float>(index + 2) * 0.035f;
                            room.Box(
                                { side * 2.62f,
                                  shelf + 0.19f,
                                  depth + 0.38f * static_cast<float>(index) },
                                { 0.48f, 0.155f + shade * 0.26f, 0.16f },
                                { paper.x - shade, paper.y - shade, paper.z },
                                kPlaster,
                                false,
                                1);
                            room.Box(
                                { side * 2.142f,
                                  shelf + 0.21f,
                                  depth + 0.38f * static_cast<float>(index) },
                                { 0.012f, 0.055f, 0.13f },
                                paper,
                                kFlat,
                                false,
                                2);
                        }
                    }
                }
            }
            room.Box({ -1.56f, 0.42f, 2.29f },
                     { 0.58f, 0.42f, 0.51f },
                     wood,
                     kWood,
                     true);
            room.Record(kInventory,
                        { -1.55f, 1.10f, 2.22f },
                        { 0.27f, 0.24f, 0.14f },
                        steel);
            room.Box({ -1.54f, 1.13f, 2.37f },
                     { 0.20f, 0.14f, 0.017f },
                     { 0.21f, 0.53f, 0.43f },
                     kFlat);
            room.Witness(kArchivist, { 1.46f, 0, 2.27f }, false);
            room.Box({ 1.7f, 0.47f, -0.7f },
                     { 0.53f, 0.47f, 0.35f },
                     wood,
                     kWood,
                     true);
            room.Record(kTransfer,
                        { 1.67f, 1.04f, -0.69f },
                        { 0.27f, 0.055f, 0.19f },
                        paper,
                        kWood);
            room.Record(kShelf,
                        { -1.86f, 1.23f, -4.55f },
                        { 0.17f, 0.24f, 0.25f },
                        paper,
                        kWood);
            room.Record(kDuty,
                        { 1.67f, 1.22f, -7.65f },
                        { 0.18f, 0.24f, 0.19f },
                        paper,
                        kWood);
            room.Box({ 0, 1.65f, -12.91f },
                     { 1.08f, 1.65f, 0.10f },
                     steel,
                     kMetal);
            for (const auto stile : { -1.15f, 0.0f, 1.15f })
                room.Box({ stile, 1.68f, -12.78f },
                         { 0.045f, 1.68f, 0.07f },
                         wood,
                         kMetal);
            for (const auto side : { -1.0f, 1.0f })
                room.Box({ side * 1.64f, 0.53f, -11.7f },
                         { 0.50f, 0.53f, 0.40f },
                         steel,
                         kMetal,
                         true);
            room.Record(kIntake,
                        { -1.65f, 1.24f, -11.59f },
                        { 0.27f, 0.12f, 0.19f },
                        paper,
                        kWood);
            room.Record(kReceiver,
                        { 1.65f, 1.24f, -11.59f },
                        { 0.27f, 0.12f, 0.19f },
                        paper,
                        kWood);
            for (const auto depth : { 1.65f, -6.0f })
            {
                room.Box({ -1.46f, 1.165f, depth },
                         { 0.26f, 0.028f, 0.25f },
                         steel,
                         kMetal);
                room.Box({ -1.64f, 1.02f, depth },
                         { 0.042f, 0.143f, 0.16f },
                         steel,
                         kMetal);
            }
            return std::move(room.scene);
        }

        auto
        CreateCinema() -> Apartment
        {
            SceneBuilder room;
            constexpr Position brass{ 0.52f, 0.36f, 0.19f };
            constexpr Position carpet{ 0.18f, 0.11f, 0.13f };
            constexpr Position velvet{ 0.36f, 0.12f, 0.15f };
            constexpr Position film{ 0.25f, 0.28f, 0.29f };
            constexpr Position paper{ 0.63f, 0.56f, 0.42f };
            using enum Material;
            room.Shell({ 0.26f, 0.19f, 0.22f }, carpet, 5.3f, -16.5f);
            room.scene.cinema = true;
            room.scene.eventDepth = -10.2f;
            room.scene.checkpoint = { 0, 0.08f, -12.25f };
            room.scene.lights = { { { 0, 3.25f, 2.7f },
                                    { 0, 3.25f, -10.2f },
                                    { 0, 3.25f, -14.8f },
                                    { 3.4f, 1.4f, -6.2f } } };
            for (const auto side : { -1.0f, 1.0f })
            {
                room.Box({ side * 3.58f, 1.73f, 0.05f },
                         { 1.68f, 1.73f, 0.13f },
                         velvet,
                         kWood,
                         true);
                for (const auto depth : { 0.9f, -3.3f, -7.5f, -11.7f })
                {
                    room.Box({ side * 5.11f, 2.35f, depth },
                             { 0.085f, 0.23f, 0.13f },
                             brass,
                             kMetal);
                    room.Box({ side * 5.00f, 2.35f, depth },
                             { 0.04f, 0.15f, 0.090f },
                             { 0.68f, 0.39f, 0.17f },
                             kFlat);
                }
                for (int row = 0; row < 5; ++row)
                {
                    const auto z = -2.0f - static_cast<float>(row) * 1.74f;
                    for (int seat = 0; seat < 4; ++seat)
                    {
                        const auto x
                            = side * (1.69f + 0.69f * static_cast<float>(seat));
                        room.Box({ x, 0.44f, z - 0.15f },
                                 { 0.29f, 0.075f, 0.30f },
                                 velvet,
                                 kWood,
                                 true);
                        room.Box({ x, 0.69f, z + 0.13f },
                                 { 0.30f, 0.37f, 0.086f },
                                 velvet,
                                 kWood);
                        room.Box({ x, 0.75f, z + 0.21f },
                                 { 0.085f, 0.045f, 0.012f },
                                 brass,
                                 kMetal,
                                 false,
                                 2);
                        room.Box({ x, 0.25f, z - 0.09f },
                                 { 0.055f, 0.25f, 0.11f },
                                 film,
                                 kMetal,
                                 false,
                                 1);
                    }
                }
            }
            room.Box({ 0, 1.93f, -16.24f },
                     { 3.80f, 1.34f, 0.095f },
                     film,
                     kMetal);
            room.Box({ 0, 2.04f, -16.11f },
                     { 3.58f, 1.09f, 0.046f },
                     { 0.51f, 0.49f, 0.43f },
                     kPlaster);
            room.Box({ 0, 0.12f, -15.35f },
                     { 3.65f, 0.12f, 0.92f },
                     carpet,
                     kWood,
                     true);
            room.Box({ -1.39f, 0.44f, 2.52f },
                     { 0.55f, 0.44f, 0.49f },
                     film,
                     kWood,
                     true);
            room.Record(kSchedule,
                        { -1.42f, 1.04f, 2.53f },
                        { 0.26f, 0.12f, 0.17f },
                        paper,
                        kWood);
            room.Witness(kUsher, { 1.44f, 0, 2.20f }, false);
            room.Record(kScreening,
                        { -1.58f, 1.23f, -0.30f },
                        { 0.25f, 0.22f, 0.14f },
                        film);
            room.Record(kDiary,
                        { 1.54f, 1.22f, -0.29f },
                        { 0.23f, 0.19f, 0.15f },
                        paper,
                        kWood);
            room.Record(kLeader,
                        { -1.25f, 1.24f, -10.77f },
                        { 0.20f, 0.20f, 0.135f },
                        film);
            room.Box({ -1.23f, 0.50f, -10.79f },
                     { 0.37f, 0.50f, 0.36f },
                     film,
                     kMetal,
                     true);
            room.Record(kExit,
                        { -1.55f, 1.22f, -13.23f },
                        { 0.20f, 0.23f, 0.14f },
                        film);
            room.Record(kReceipt,
                        { 1.58f, 1.15f, -13.12f },
                        { 0.22f, 0.16f, 0.13f },
                        paper,
                        kWood);
            for (const auto side : { -1.0f, 1.0f })
            {
                room.Box({ side * 1.85f, 0.46f, -13.18f },
                         { 0.49f, 0.46f, 0.35f },
                         film,
                         kMetal,
                         true);
                room.Box({ side * 4.64f, 1.19f, -16.32f },
                         { 0.55f, 1.18f, 0.07f },
                         velvet,
                         kWood);
                for (int fold = 0; fold < 7; ++fold)
                    room.Box(
                        { side * (3.65f + static_cast<float>(fold) * 0.22f),
                          2.05f,
                          -15.96f },
                        { 0.096f, 1.34f, 0.16f },
                        velvet,
                        kWood,
                        false,
                        1);
            }
            room.Box({ 3.0f, 0.42f, -0.55f },
                     { 0.44f, 0.42f, 0.42f },
                     film,
                     kMetal,
                     true);
            return std::move(room.scene);
        }
    } // namespace

    auto
    CreateInterior(Investigation::CaseId id) -> Apartment
    {
        return id == Investigation::CaseId::kArchive ? CreateArchive()
                                                     : CreateCinema();
    }
} // namespace TwentyThreeSeventeen::Game
