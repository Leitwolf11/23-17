// SPDX-License-Identifier: MIT
#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Game
{
    auto
    AppendCaption(std::string_view caption,
                  int width,
                  int height,
                  std::vector<Rendering::Vertex> &vertices) -> void;

    class Dialog final
    {
    public:
        explicit Dialog(Rendering::Renderer &renderer) noexcept;

        auto
        Choose(std::string_view title,
               std::string_view body,
               const char *close,
               std::span<const char *const> options) -> int;

        [[nodiscard]] auto
        WantsQuit() const noexcept -> bool;

    private:
        Rendering::Renderer &renderer_;
        bool wantsQuit_{};
    };
} // namespace TwentyThreeSeventeen::Game
