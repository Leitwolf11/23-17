// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include "GltfMaterials.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    namespace
    {
        using Surface
            = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
        constexpr int kSize = MaterialAtlas::kTileSize;
        constexpr float kColorScale
            = 3.4f; // Matches the world shader and bake.

        auto
        ReadTexture(const cgltf_texture_view &view)
            -> tl::expected<Surface, std::string>
        {
            if (view.texture == nullptr)
                return Surface(nullptr, SDL_DestroySurface);
            const auto &texture = *view.texture;
            if (view.texcoord != 0 || view.has_transform
                || texture.image == nullptr)
                return tl::unexpected(
                    std::string("Expected an embedded TEXCOORD_0 texture "
                                "without a transform."));
            if (const auto *sampler = texture.sampler;
                sampler != nullptr
                && (sampler->wrap_s != cgltf_wrap_mode_repeat
                    || sampler->wrap_t != cgltf_wrap_mode_repeat))
                return tl::unexpected(
                    std::string("GLB textures must use repeat wrapping."));

            const auto *image = texture.image;
            const auto *payload = image->buffer_view;
            if (image->uri != nullptr || payload == nullptr
                || image->mime_type == nullptr
                || std::strcmp(image->mime_type, "image/png") != 0)
                return tl::unexpected(
                    std::string("GLB textures must be embedded PNG images."));
            const auto *png = cgltf_buffer_view_data(payload);
            if (png == nullptr || payload->size < 24)
                return tl::unexpected(std::string("Missing PNG image data."));

            // Width and height in the PNG IHDR are big-endian.
            const auto extent = [png](std::size_t start) -> std::uint32_t {
                std::uint32_t dimension = 0;
                for (std::size_t i = start; i < start + 4; ++i)
                    dimension = (dimension << 8) | png[i];
                return dimension;
            };
            if (extent(16) > 4096 || extent(20) > 4096)
                return tl::unexpected(std::string(
                    "GLB textures are limited to 4096 pixels per side."));
            auto *stream = SDL_IOFromConstMem(png, payload->size);
            if (stream == nullptr)
                return tl::unexpected(std::string(SDL_GetError()));
            const Surface decoded(SDL_LoadPNG_IO(stream, true),
                                  SDL_DestroySurface);
            if (!decoded)
                return tl::unexpected(std::string(SDL_GetError()));
            const Surface rgba(
                SDL_ConvertSurface(decoded.get(), SDL_PIXELFORMAT_RGBA32),
                SDL_DestroySurface);
            if (!rgba)
                return tl::unexpected(std::string(SDL_GetError()));
            Surface tile(SDL_ScaleSurface(rgba.get(),
                                          kSize,
                                          kSize,
                                          SDL_SCALEMODE_LINEAR),
                         SDL_DestroySurface);
            if (!tile)
                return tl::unexpected(std::string(SDL_GetError()));
            return tile;
        }

        auto
        Texel(const Surface &image, std::size_t x, std::size_t y)
            -> std::array<std::uint8_t, 4>
        {
            if (!image)
                return { 255, 255, 255, 255 };
            const auto pixels
                = static_cast<const std::uint8_t *>(image->pixels);
            const auto offset
                = y * static_cast<std::size_t>(image->pitch) + x * 4;
            return { pixels[offset],
                     pixels[offset + 1],
                     pixels[offset + 2],
                     pixels[offset + 3] };
        }

        auto
        Sample(const Surface &image, std::size_t x, std::size_t y) -> Float4
        {
            const auto texel = Texel(image, x, y);
            return { texel[0] / 255.0f,
                     texel[1] / 255.0f,
                     texel[2] / 255.0f,
                     texel[3] / 255.0f };
        }

        auto
        ToLinear(float value) -> float
        {
            return value <= 0.04045f
                       ? value / 12.92f
                       : std::pow((value + 0.055f) / 1.055f, 2.4f);
        }

        auto
        LinearColor(const Surface &image, std::size_t x, std::size_t y)
            -> Float3
        {
            static const auto lookup = [] {
                std::array<float, 256> values{};
                for (std::size_t i = 0; i < values.size(); ++i)
                    values[i] = ToLinear(static_cast<float>(i) / 255.0f);
                return values;
            }();
            const auto rgb = Texel(image, x, y);
            return { lookup[rgb[0]], lookup[rgb[1]], lookup[rgb[2]] };
        }

        auto
        TextureTint(const Surface &image) -> Float3
        {
            if (!image)
                return { 1, 1, 1 };
            Float3 total{}, brightest{};
            for (std::size_t y = 0; y < kSize; ++y)
                for (std::size_t x = 0; x < kSize; ++x)
                {
                    const auto rgb = LinearColor(image, x, y);
                    for (std::size_t channel = 0; channel < rgb.size();
                         ++channel)
                    {
                        total[channel] += rgb[channel];
                        brightest[channel]
                            = std::max(brightest[channel], rgb[channel]);
                    }
                }
            // Low shades with the tint alone. Leave enough range in the packed
            // texture to reconstruct every highlight when sampling is enabled.
            for (std::size_t channel = 0; channel < total.size(); ++channel)
                total[channel] = std::max(
                    { total[channel] / static_cast<float>(kSize * kSize),
                      brightest[channel] / kColorScale,
                      0.0001f });
            return total;
        }

        auto
        Quantize(float value) -> std::uint8_t
        {
            return static_cast<std::uint8_t>(
                std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
        }
        auto
        EmissionGain(const cgltf_material *material,
                     const cgltf_pbr_metallic_roughness &pbr)
            -> tl::expected<float, std::string>
        {
            if (material == nullptr)
                return 0.0f;
            const auto &factor = material->emissive_factor;
            const float maximum = std::max({ factor[0], factor[1], factor[2] });
            if (maximum <= 0)
                return 0.0f;
            const float base = std::max({ pbr.base_color_factor[0],
                                          pbr.base_color_factor[1],
                                          pbr.base_color_factor[2] });
            const auto image = [](const cgltf_texture_view &view) {
                return view.texture == nullptr ? nullptr : view.texture->image;
            };
            const auto &emissive = material->emissive_texture;
            if (base <= 0 || image(emissive) != image(pbr.base_color_texture)
                || emissive.has_transform || emissive.texcoord != 0)
                return tl::unexpected(std::string(
                    "Emission must share the GLB base color image and UVs."));
            const float ratio = maximum / base;
            for (std::size_t c = 0; c < 3; ++c)
                if (std::abs(factor[c] - pbr.base_color_factor[c] * ratio)
                    > 0.001f)
                    return tl::unexpected(
                        std::string("The emissive factor must be proportional "
                                    "to the base color."));
            return ratio
                   * (material->has_emissive_strength
                          ? material->emissive_strength.emissive_strength
                          : 1.0f);
        }
    } // namespace

    auto
    PackGltfMaterial(const cgltf_material *material)
        -> tl::expected<GltfMaterialTile, std::string>
    {
        cgltf_pbr_metallic_roughness defaults{};
        std::ranges::fill(defaults.base_color_factor, 1.0f);
        defaults.roughness_factor = 1;
        defaults.metallic_factor = 1;
        const auto &pbr
            = material != nullptr && material->has_pbr_metallic_roughness
                  ? material->pbr_metallic_roughness
                  : defaults;
        const cgltf_texture_view empty{};
        const auto &normalView
            = material != nullptr ? material->normal_texture : empty;
        const auto &aoView
            = material != nullptr ? material->occlusion_texture : empty;
        if (material != nullptr
            && (material->alpha_mode != cgltf_alpha_mode_opaque
                || material->double_sided || material->unlit
                || material->has_pbr_specular_glossiness
                || material->has_transmission))
            return tl::unexpected(
                std::string("Expected an opaque, single-sided "
                            "metallic/roughness GLB material."));

        const auto base = ReadTexture(pbr.base_color_texture);
        const auto orm = ReadTexture(pbr.metallic_roughness_texture);
        const auto normal = ReadTexture(normalView);
        const auto ao = ReadTexture(aoView);
        for (const auto *texture : { &base, &orm, &normal, &ao })
            if (!*texture)
                return tl::unexpected(texture->error());

        const auto emission = EmissionGain(material, pbr);
        if (!emission)
            return tl::unexpected(emission.error());
        GltfMaterialTile result;
        result.emission = *emission;
        const auto textureTint = TextureTint(*base);
        for (std::size_t c = 0; c < 3; ++c)
            result.tint[c]
                = std::sqrt(std::clamp(pbr.base_color_factor[c], 0.0f, 1.0f)
                            * textureTint[c]);
        constexpr auto bytes = static_cast<std::size_t>(kSize) * kSize * 4;
        result.color.resize(bytes);
        result.surface.resize(bytes);
        for (std::size_t y = 0; y < kSize; ++y)
            for (std::size_t x = 0; x < kSize; ++x)
            {
                const auto index = (y * kSize + x) * 4;
                const auto rgb = LinearColor(*base, x, y);
                const auto mr = Sample(*orm, x, y);
                const auto occlusion = Sample(*ao, x, y)[0];
                const auto n = *normal ? Sample(*normal, x, y)
                                       : Float4{ 0.5f, 0.5f, 1, 1 };
                const Float3 scaledNormal{ (n[0] * 2 - 1) * normalView.scale,
                                           (n[1] * 2 - 1) * normalView.scale,
                                           n[2] * 2 - 1 };
                const float length = std::max(std::hypot(scaledNormal[0],
                                                         scaledNormal[1],
                                                         scaledNormal[2]),
                                              0.001f);
                for (std::size_t c = 0; c < rgb.size(); ++c)
                    result.color[index + c] = Quantize(
                        std::sqrt(rgb[c] / (kColorScale * textureTint[c])));
                result.color[index + 3]
                    = Quantize(std::lerp(1.0f, occlusion, aoView.scale));
                result.surface[index]
                    = Quantize(0.5f + 0.5f * scaledNormal[0] / length);
                result.surface[index + 1]
                    = Quantize(0.5f + 0.5f * scaledNormal[1] / length);
                result.surface[index + 2]
                    = Quantize(pbr.roughness_factor * mr[1]);
                result.surface[index + 3]
                    = Quantize(pbr.metallic_factor * mr[2]);
            }
        return result;
    }
} // namespace TwentyThreeSeventeen::Rendering
