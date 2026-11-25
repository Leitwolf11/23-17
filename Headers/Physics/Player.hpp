// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <memory>
#include <span>

namespace TwentyThreeSeventeen::Physics
{
    struct Position
    {
        float x{}, y{}, z{};
    };

    struct Obstacle
    {
        Position center;
        Position halfSize;
        std::uint64_t subject{};
    };

    class Player final
    {
    public:
        explicit Player(std::span<const Obstacle> obstacles,
                        Position spawn = { 0, 0.08f, 3.9f });
        ~Player();
        Player(const Player &) = delete;
        auto
        operator=(const Player &) -> Player & = delete;

        auto
        Move(float x, float z, float seconds) -> void;
        [[nodiscard]] auto
        Eye() const -> Position;
        [[nodiscard]] auto
        LookAt(Position direction) const -> std::uint64_t;

    private:
        Position spawn_;
        struct World;
        std::unique_ptr<World> world_;
    };
} // namespace TwentyThreeSeventeen::Physics
