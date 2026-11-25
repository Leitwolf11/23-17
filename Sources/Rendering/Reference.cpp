// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <numbers>
#include <span>

#include "Rendering/Renderer.hpp"
#include "Rendering/ScannedMesh.hpp"
#include "rules_cc/cc/runfiles/runfiles.h"

namespace TwentyThreeSeventeen::Rendering
{
    namespace
    {
        constexpr auto kPi = std::numbers::pi_v<float>;
        template<class Sample>
        void
        Patch(WorldMesh &mesh, int columns, int rows, const Sample &sample)
        {
            const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
            for (int y = 0; y <= rows; ++y)
                for (int x = 0; x <= columns; ++x)
                    mesh.vertices.push_back(sample(
                        static_cast<float>(x) / static_cast<float>(columns),
                        static_cast<float>(y) / static_cast<float>(rows)));
            for (int y = 0; y < rows; ++y)
                for (int x = 0; x < columns; ++x)
                {
                    const auto i
                        = base
                          + static_cast<std::uint32_t>(y * (columns + 1) + x);
                    const auto j = i + static_cast<std::uint32_t>(columns + 1);
                    for (auto n : { i, i + 1, j, j, i + 1, j + 1 })
                        mesh.indices.push_back(n);
                }
        }
        void
        Plane(WorldMesh &mesh,
              Float3 origin,
              Float3 width,
              Float3 height,
              Float3 normal,
              Float4 tangent,
              Float3 tint,
              float tile)
        {
            Patch(mesh, 18, 14, [&](float u, float v) {
                Float3 p{};
                for (std::size_t k = 0; k < 3; ++k)
                    p[k] = origin[k] + u * width[k] + v * height[k];
                const auto length = [](Float3 a) {
                    return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
                };
                return WorldVertex{ p,
                                    normal,
                                    tangent,
                                    tint,
                                    { u * length(width) * 0.5f,
                                      v * length(height) * 0.5f,
                                      tile },
                                    {},
                                    { 0.35f, 0, 0 } };
            });
        }
        void
        Pedestal(WorldMesh &mesh, float x, float z, float radius, float top)
        {
            // Turned plinth with a rolled lip, concave foot and stone drum.
            constexpr std::array<std::array<float, 2>, 9> profile{
                { { 1.05f, 0 },
                  { 1.05f, 0.035f },
                  { 0.99f, 0.07f },
                  { 0.90f, 0.10f },
                  { 0.87f, 0.14f },
                  { 0.87f, 0.88f },
                  { 0.94f, 0.94f },
                  { 1, 0.97f },
                  { 1, 1 } }
            };
            for (std::size_t ring = 1; ring < profile.size(); ++ring)
            {
                const auto a = profile[ring - 1], b = profile[ring];
                Patch(mesh, 80, 4, [&](float u, float v) {
                    const float phi = u * 2 * kPi, c = std::cos(phi),
                                s = std::sin(phi);
                    const float r = radius * std::lerp(a[0], b[0], v),
                                y = top * std::lerp(a[1], b[1], v);
                    const float slope
                        = (a[0] - b[0]) * radius / ((b[1] - a[1]) * top);
                    const float norm = 1.0f / std::sqrt(1.0f + slope * slope);
                    const bool band = ring >= 7 || ring == 2;
                    return WorldVertex{ { x + r * c, y, z + r * s },
                                        { c * norm, slope * norm, s * norm },
                                        { -s, 0, c, -1 },
                                        band ? Float3{ 0.54f, 0.42f, 0.32f }
                                             : Float3{ 0.30f, 0.34f, 0.32f },
                                        { u * 2, y, band ? 5.0f : 2.0f },
                                        {},
                                        { 0.36f, 0, 0 } };
                });
            }
            Patch(mesh, 80, 5, [&](float u, float v) {
                const float c = std::cos(u * 2 * kPi),
                            s = std::sin(u * 2 * kPi);
                return WorldVertex{
                    { x + radius * v * c, top, z + radius * v * s },
                    { 0, 1, 0 },
                    { 1, 0, 0, -1 },
                    { 0.32f, 0.34f, 0.31f },
                    { v * c, v * s, 2 },
                    {},
                    { 0.36f, 0, 0 }
                };
            });
        }
        auto
        Gallery(const char *executable, WorldView &view)
            -> tl::expected<std::shared_ptr<WorldMesh>, std::string>
        {
            auto maps = LoadMaterials(executable, true);
            if (!maps)
                return tl::unexpected(maps.error());
            auto mesh = std::make_shared<WorldMesh>();
            mesh->materials = std::move(*maps);
            Plane(*mesh,
                  { -3.6f, 0, -4 },
                  { 7.2f, 0, 0 },
                  { 0, 0, 8 },
                  { 0, 1, 0 },
                  { 1, 0, 0, -1 },
                  { 0.40f, 0.42f, 0.39f },
                  2);
            Plane(*mesh,
                  { -3.6f, 0, -2.15f },
                  { 7.2f, 0, 0 },
                  { 0, 3.7f, 0 },
                  { 0, 0, 1 },
                  { 1, 0, 0, 1 },
                  { 0.32f, 0.37f, 0.36f },
                  1);
            for (float side : { -1.0f, 1.0f })
                Plane(*mesh,
                      { side * 3.5f, 0, -2.1f },
                      { 0, 0, 6.1f },
                      { 0, 3.7f, 0 },
                      { -side, 0, 0 },
                      { 0, 0, 1, side },
                      { 0.42f, 0.40f, 0.33f },
                      1);
            // A continuous barrel vault, with three concentric courses over the
            // alcove.
            for (int rib = 0; rib < 4; ++rib)
            {
                const float radius = rib == 0 ? 2.16f : 2.08f;
                const float depth = rib == 0 ? 4.7f : 0.12f;
                const float start
                    = rib == 0 ? -2.08f
                               : -2.04f + static_cast<float>(rib - 1) * 1.65f;
                Patch(*mesh, 64, 12, [&](float u, float v) {
                    const auto a = u * kPi, c = std::cos(a), s = std::sin(a);
                    return WorldVertex{
                        { radius * c, 1.45f + radius * s, start + depth * v },
                        { -c, -s, 0 },
                        { -s, c, 0, -1 },
                        rib == 0 ? Float3{ 0.39f, 0.39f, 0.35f }
                                 : Float3{ 0.37f, 0.32f, 0.25f },
                        { 3.2f * u, depth * v * 0.5f, rib == 0 ? 1.0f : 2.0f },
                        {},
                        { 0.32f, 0, 0 }
                    };
                });
            }
            for (float side : { -1.0f, 1.0f })
            {
                Plane(*mesh,
                      { side * 2.16f, 0, -2.08f },
                      { 0, 0, 4.7f },
                      { 0, 1.45f, 0 },
                      { -side, 0, 0 },
                      { 0, 0, 1, side },
                      { 0.39f, 0.39f, 0.35f },
                      1);
            }
            for (float side : { -1.0f, 1.0f })
            {
                // Frosted opal diffuser, cast end caps and a brass mounting
                // plate.
                const float x = side * 1.44f;
                for (int part = 0; part < 4; ++part)
                {
                    const bool diffuser = part == 0, plate = part == 3;
                    const float r
                        = plate ? 0.145f : (diffuser ? 0.063f : 0.085f);
                    const float baseY
                        = diffuser ? 1.23f
                                   : (part == 1 ? 1.15f
                                                : (part == 2 ? 1.97f : 1.57f));
                    const float length = diffuser ? 0.74f : 0.08f;
                    Patch(*mesh, 32, 8, [&](float u, float v) {
                        const float angle = u * 2 * kPi, c = std::cos(angle),
                                    sine = std::sin(angle);
                        Float3 point{ x + r * c,
                                      baseY + v * length,
                                      -1.98f + r * sine };
                        Float3 normal{ c, 0, sine };
                        Float4 tangent{ -sine, 0, c, -1 };
                        if (plate)
                        {
                            const float latitude = (v - 0.5f) * kPi;
                            normal = { std::cos(latitude) * c,
                                       std::sin(latitude),
                                       std::cos(latitude) * sine };
                            point = { x + 0.15f * normal[0],
                                      baseY + 0.23f * normal[1],
                                      -2.13f + 0.11f * normal[2] };
                        }
                        const Float3 glow = side < 0
                                                ? Float3{ 0.95f, 0.73f, 0.49f }
                                                : Float3{ 0.65f, 0.80f, 0.92f };
                        return WorldVertex{
                            point,
                            normal,
                            tangent,
                            diffuser ? glow : Float3{ 0.52f, 0.39f, 0.26f },
                            { u, v, diffuser ? 0.0f : 5.0f },
                            {},
                            { 0.32f, diffuser ? 1.70f : 0.0f, 0, 0 }
                        };
                    });
                }
            }
            Pedestal(*mesh, -0.43f, -0.78f, 0.43f, 0.97f);
            Pedestal(*mesh, 0.72f, -1.10f, 0.30f, 0.78f);
            using rules_cc::cc::runfiles::Runfiles;
            const std::unique_ptr<Runfiles> files(Runfiles::Create(executable));
            const auto load = [&](const std::string &name,
                                  Float3 p,
                                  float scale,
                                  float yaw,
                                  float tile) {
                const auto path
                    = files ? files->Rlocation("_main/Assets/Reference/" + name)
                            : std::string{};
                if (!path.empty())
                    return AppendScan(*mesh, path, p, scale, yaw, tile);
                const auto *base = SDL_GetBasePath();
                return AppendScan(*mesh,
                                  std::string(base ? base : "") + "reference/"
                                      + name,
                                  p,
                                  scale,
                                  yaw,
                                  tile);
            };
            if (auto result = load("marble_bust_01.glb",
                                   { -0.43f, 0.97f, -0.78f },
                                   2.2f,
                                   0.16f,
                                   6);
                !result)
                return tl::unexpected(result.error());
            if (auto result = load("brass_vase_01.glb",
                                   { 0.72f, 0.78f, -1.10f },
                                   1.40f,
                                   -0.35f,
                                   7);
                !result)
                return tl::unexpected(result.error());
            view.lights = { { { -1.10f, 2.65f, 0.55f, 4.40f },
                              { 1.5f, 2.1f, 0.2f, 2.1f },
                              { -1.55f, 1.65f, -1.83f, 1.7f },
                              { 1.52f, 1.65f, -1.83f, 1.35f } } };
            view.colors = { { { 1, 0.79f, 0.56f, 0 },
                              { 0.58f, 0.76f, 1, 0 },
                              { 1, 0.63f, 0.37f, 0 },
                              { 0.58f, 0.78f, 1, 0 } } };
            view.probeCenter = { 0, 1.60f, -0.05f, 0 };
            view.probeHalfSize = { 2.16f, 1.6f, 2.15f, 1 };
            mesh->detailEnds.fill(
                static_cast<std::uint32_t>(mesh->indices.size()));
            BakeLighting(*mesh, view);
            return mesh;
        }
        auto
        Run(const char *executable,
            Backend requested,
            int limit,
            bool software,
            bool fullscreen) -> int
        {
            std::unique_ptr<Renderer> renderer;
            for (const auto backend : StartupOrder(requested))
            {
                if (requested != Backend::kAuto && backend != requested)
                    continue;
                auto opened = Open(backend, { 1280, 720 }, true, !software);
                if (opened)
                {
                    renderer = std::move(*opened);
                    break;
                }
                SDL_Log("%s: %s", Name(backend).data(), opened.error().c_str());
            }
            if (!renderer)
                return 1;
            WorldView view{};
            auto scene = Gallery(executable, view);
            if (!scene)
            {
                SDL_Log("%s", scene.error().c_str());
                return 1;
            }
            SDL_SetWindowTitle(renderer->Window(),
                               "23.17 Engine | Material study");
            if (fullscreen)
                SDL_SetWindowFullscreen(renderer->Window(), true);
            SDL_ShowWindow(renderer->Window());
            float angle = 0.08f, distance = 2.35f;
            for (int frame = 0; limit == 0 || frame < limit; ++frame)
            {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_QUIT
                        || (event.type == SDL_EVENT_KEY_DOWN
                            && event.key.key == SDLK_ESCAPE))
                        return 0;
                    if (event.type == SDL_EVENT_MOUSE_MOTION
                        && (event.motion.state & SDL_BUTTON_LMASK))
                        angle = std::clamp(angle + event.motion.xrel * 0.006f,
                                           -1.18f,
                                           1.18f);
                    if (event.type == SDL_EVENT_MOUSE_WHEEL)
                        distance = std::clamp(distance - event.wheel.y * 0.18f,
                                              1.25f,
                                              3.6f);
                }
                const float c = std::cos(angle), s = std::sin(angle),
                            pitch = -0.08f;
                view.eye = { distance * s, 1.63f, -0.8f + distance * c, 1 };
                view.forward = { -s * std::cos(pitch),
                                 std::sin(pitch),
                                 -c * std::cos(pitch),
                                 2 };
                view.right = { c, 0, -s, 1.75f * 720.0f / 1280.0f };
                view.up = { s * std::sin(pitch),
                            std::cos(pitch),
                            c * std::sin(pitch),
                            1.75f };
                const WorldFrame packet{ *scene,
                                         view,
                                         (*scene)->detailEnds.back() };
                if (auto result = renderer->Draw({}, { 1280, 720 }, &packet);
                    !result)
                {
                    SDL_Log("%s", result.error().c_str());
                    return 1;
                }
            }
            return 0;
        }
    } // namespace
} // namespace TwentyThreeSeventeen::Rendering
int
main(int argc, char **argv)
{
    using namespace TwentyThreeSeventeen::Rendering;
    Backend backend = Backend::kAuto;
    int frames = 0;
    bool software = false, fullscreen = false;
    for (auto arg : std::span(argv + 1, static_cast<std::size_t>(argc - 1)))
    {
        const std::string_view value(arg);
        if (value.starts_with("--graphics="))
        {
            const auto key = value.substr(11);
            backend = ParseBackend(key);
            if (key != Key(backend))
                return 2;
        }
        else if (value.starts_with("--frames="))
        {
            const auto end = value.data() + value.size();
            const auto result = std::from_chars(value.data() + 9, end, frames);
            if (result.ec != std::errc() || result.ptr != end || frames < 1)
                return 2;
        }
        else if (value == "--allow-software")
            software = true;
        else if (value == "--fullscreen")
            fullscreen = true;
        else
            return 2;
    }
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("%s", SDL_GetError());
        return 1;
    }
    const int result = Run(argv[0], backend, frames, software, fullscreen);
    SDL_Quit();
    return result;
}
