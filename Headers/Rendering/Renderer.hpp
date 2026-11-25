// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <tl/expected.hpp>

#include "Rendering/Backend.hpp"
#include "Rendering/World.hpp"

struct SDL_Window;

namespace TwentyThreeSeventeen::Rendering
{
    struct Vertex
    {
        float x, y, z, w;
        float red, green, blue;
    };

    struct WindowSize
    {
        int width;
        int height;

        auto
        operator==(const WindowSize &) const -> bool = default;
    };

    struct Viewport
    {
        int x;
        int y;
        WindowSize size;

        auto
        operator==(const Viewport &) const -> bool = default;
    };

    [[nodiscard]] constexpr auto
    FitViewport(WindowSize target, WindowSize output) noexcept -> Viewport
    {
        if (target.width <= 0 || target.height <= 0 || output.width <= 0
            || output.height <= 0)
            return { 0, 0, output };
        const auto horizontal = static_cast<std::int64_t>(output.height)
                                * target.width / target.height;
        if (horizontal < output.width)
        {
            const auto width = std::max(1, static_cast<int>(horizontal));
            return { (output.width - width) / 2, 0, { width, output.height } };
        }
        const auto height
            = std::max(1,
                       static_cast<int>(static_cast<std::int64_t>(output.width)
                                        * target.height / target.width));
        return { 0, (output.height - height) / 2, { output.width, height } };
    }

    class Renderer
    {
    public:
        Renderer() = default;
        Renderer(const Renderer &) = delete;
        auto
        operator=(const Renderer &) -> Renderer & = delete;
        virtual ~Renderer() = default;

        [[nodiscard]] virtual auto
        Window() const noexcept -> SDL_Window * = 0;
        [[nodiscard]] virtual auto
        ActiveBackend() const noexcept -> Backend = 0;
        [[nodiscard]] virtual auto
        Draw(std::span<const Vertex> vertices,
             WindowSize renderSize = {},
             const WorldFrame *world = nullptr)
            -> tl::expected<void, std::string> = 0;
    };

    [[nodiscard]] auto
    Open(Backend backend,
         WindowSize size,
         bool sync,
         bool requireHardware = true)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>;
} // namespace TwentyThreeSeventeen::Rendering
