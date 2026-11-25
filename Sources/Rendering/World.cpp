// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <span>

#include "Rendering/World.hpp"
#include "rules_cc/cc/runfiles/runfiles.h"

namespace TwentyThreeSeventeen::Rendering
{
    static auto
    ReadTiles(const char *executable, bool reference)
        -> tl::expected<MaterialAtlas, std::string>
    {
        using rules_cc::cc::runfiles::Runfiles;
        using Surface
            = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
        const std::unique_ptr<Runfiles> files(Runfiles::Create(executable));
        Surface image(nullptr, SDL_DestroySurface);
        const std::string directory = reference ? "Reference" : "Materials";
        if (files)
            image.reset(SDL_LoadPNG(
                files->Rlocation("_main/Assets/" + directory + "/surfaces.png")
                    .c_str()));
        if (!image)
            if (const auto *base = SDL_GetBasePath())
                image.reset(
                    SDL_LoadPNG((std::string(base)
                                 + (reference ? "reference/surfaces.png"
                                              : "materials/surfaces.png"))
                                    .c_str()));
        if (!image)
            return tl::unexpected(std::string("Could not read surfaces.png: ")
                                  + SDL_GetError());
        MaterialAtlas atlas;
        atlas.materialCount = reference ? 2 : 5;
        if (image->w != MaterialAtlas::kTileSize
            || image->h != MaterialAtlas::kTileSize * (atlas.LayerCount() - 1))
            return tl::unexpected(
                std::string("Unexpected material atlas dimensions."));
        Surface rgba(SDL_ConvertSurface(image.get(), SDL_PIXELFORMAT_RGBA32),
                     SDL_DestroySurface);
        if (!rgba)
            return tl::unexpected(std::string(SDL_GetError()));
        constexpr std::size_t rowBytes = MaterialAtlas::kTileSize * 4;
        atlas.rgba.reserve(rowBytes * static_cast<std::size_t>(rgba->h));
        const auto *pixels = static_cast<const std::uint8_t *>(rgba->pixels);
        for (int row = 0; row < rgba->h; ++row)
        {
            const auto line
                = std::span(pixels
                                + static_cast<std::size_t>(row)
                                      * static_cast<std::size_t>(rgba->pitch),
                            rowBytes);
            atlas.rgba.insert(atlas.rgba.end(), line.begin(), line.end());
        }
        return atlas;
    }
    auto
    LoadMaterials(const char *executable, bool reference)
        -> tl::expected<MaterialAtlas, std::string>
    {
        auto atlas = ReadTiles(executable, false);
        if (!atlas || !reference)
            return atlas;
        auto extra = ReadTiles(executable, true);
        if (!extra)
            return tl::unexpected(extra.error());
        const auto half = static_cast<std::ptrdiff_t>(atlas->rgba.size() / 2);
        const auto middle
            = extra->rgba.begin()
              + static_cast<std::ptrdiff_t>(extra->rgba.size() / 2);
        atlas->rgba.reserve(atlas->rgba.size() + extra->rgba.size());
        atlas->rgba.insert(atlas->rgba.begin() + half,
                           extra->rgba.begin(),
                           middle);
        atlas->rgba.insert(atlas->rgba.end(), middle, extra->rgba.end());
        atlas->materialCount += extra->materialCount;
        return atlas;
    }
} // namespace TwentyThreeSeventeen::Rendering
