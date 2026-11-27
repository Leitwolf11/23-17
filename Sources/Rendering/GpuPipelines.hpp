// SPDX-License-Identifier: MIT
#pragma once

#include <SDL3/SDL_gpu.h>

#include "Rendering/Backend.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    struct GpuPipelines
    {
        SDL_GPUGraphicsPipeline *overlay{};
        SDL_GPUGraphicsPipeline *world{};
        SDL_GPUGraphicsPipeline *present{};

        [[nodiscard]] explicit
        operator bool() const noexcept
        {
            return overlay != nullptr && world != nullptr && present != nullptr;
        }
    };

    [[nodiscard]] auto
    CreateGpuPipelines(SDL_GPUDevice *device,
                       Backend backend,
                       SDL_GPUTextureFormat color,
                       SDL_GPUTextureFormat depth) -> GpuPipelines;
} // namespace TwentyThreeSeventeen::Rendering
