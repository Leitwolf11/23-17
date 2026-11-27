// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cmath>
#include <map>
#include <span>

#include "Game/Apartment.hpp"

namespace TwentyThreeSeventeen::Game
{
    using Physics::Position;
    namespace
    {
        constexpr std::array<float, 6> kLight{
            { 0.76f, 0.90f, 0.58f, 1.00f, 0.69f, 0.84f }
        };
        constexpr std::array kNormals{
            Position{ 0, 0, -1 }, Position{ 0, 0, 1 },  Position{ 0, -1, 0 },
            Position{ 0, 1, 0 },  Position{ -1, 0, 0 }, Position{ 1, 0, 0 }
        };

        // The authored figure meshes share points across their triangle strips.
        // Reusing those points preserves curved normals and keeps the resident
        // vertex buffer small.
        void
        AppendTriangles(Rendering::WorldMesh &mesh,
                        std::span<const ActorPoint> points,
                        const Actor *actor)
        {
            const auto first = mesh.vertices.size();
            std::map<std::array<float, 6>, std::uint32_t> shared;
            for (std::size_t i = 0; i + 2 < points.size(); i += 3)
            {
                const auto &a = points[i].position, &b = points[i + 1].position,
                           &c = points[i + 2].position;
                const Position ab{ b.x - a.x, b.y - a.y, b.z - a.z },
                    ac{ c.x - a.x, c.y - a.y, c.z - a.z };
                const Position normal{ ab.y * ac.z - ab.z * ac.y,
                                       ab.z * ac.x - ab.x * ac.z,
                                       ab.x * ac.y - ab.y * ac.x };
                if (normal.x * normal.x + normal.y * normal.y
                        + normal.z * normal.z
                    < 1e-16f)
                    continue;
                for (const auto &p : points.subspan(i, 3))
                {
                    const std::array key{ p.position.x, p.position.y,
                                          p.position.z, p.color.x,
                                          p.color.y,    p.color.z };
                    const auto [item, created] = shared.try_emplace(
                        key,
                        static_cast<std::uint32_t>(mesh.vertices.size()));
                    if (created)
                    {
                        const float flex
                            = actor ? std::clamp(p.position.y - actor->center.y
                                                     + 0.35f,
                                                 0.0f,
                                                 1.0f)
                                    : 0.0f;
                        mesh.vertices.push_back(
                            { { p.position.x, p.position.y, p.position.z },
                              {},
                              {},
                              { p.color.x, p.color.y, p.color.z },
                              {},
                              {},
                              { 0.52f, 0, 0, flex } });
                    }
                    auto &n = mesh.vertices[item->second].normal;
                    n[0] += normal.x;
                    n[1] += normal.y;
                    n[2] += normal.z;
                    mesh.indices.push_back(item->second);
                }
            }
            for (auto &v : std::span(mesh.vertices).subspan(first))
            {
                auto &n = v.normal;
                const float length
                    = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
                if (length < 1e-9f)
                    n = { 0, 1, 0 };
                else
                    for (auto &component : n)
                        component /= length;
                const auto horizontal = std::hypot(n[0], n[2]);
                v.tangent = horizontal > 0.001f
                                ? Rendering::Float4{ n[2] / horizontal,
                                                     0,
                                                     -n[0] / horizontal,
                                                     1 }
                                : Rendering::Float4{ 1, 0, 0, 1 };
            }
        }

    } // namespace
    auto
    BuildWorld(const Apartment &scene, Rendering::WorldMesh initial)
        -> std::shared_ptr<Rendering::WorldMesh>
    {
        constexpr std::array order{ 0u, 1u, 2u, 1u, 3u, 2u };
        auto mesh = std::make_shared<Rendering::WorldMesh>(std::move(initial));
        AppendTriangles(*mesh, scene.fixtures, nullptr);
        for (const auto &actor : scene.actors)
            AppendTriangles(*mesh, actor.triangles, &actor);
        for (std::size_t detail = 0; detail < mesh->detailEnds.size(); ++detail)
        {
            for (const auto &surface : scene.surfaces)
            {
                if (surface.detail != static_cast<int>(detail))
                    continue;
                const auto &half = surface.halfSize;
                const float thinnest = std::min({ half.x, half.y, half.z });
                const bool bevel
                    = !surface.exterior && surface.detail < 2
                      && thinnest > 0.014f
                      && std::max({ half.x, half.y, half.z }) < 3.5f;
                const float radius
                    = bevel ? std::min(0.026f, thinnest * 0.32f) : 0.0f;
                for (std::size_t face = 0; face < kNormals.size(); ++face)
                {
                    const auto normal = kNormals[face];
                    const bool horizontal = face == 2 || face == 3;
                    const auto halfU
                        = face >= 4 ? surface.halfSize.z : surface.halfSize.x;
                    const auto halfV
                        = horizontal ? surface.halfSize.z : surface.halfSize.y;
                    const auto count = [&](float extent) {
                        if (surface.exterior || surface.stairLamp
                            || extent <= 0.84f)
                            return 1;
                        return std::clamp(
                            static_cast<int>(std::ceil(extent * 2.0f / 0.62f)),
                            1,
                            16);
                    };
                    const int columns = count(halfU) + (bevel ? 2 : 0);
                    const int rows = count(halfV) + (bevel ? 2 : 0);
                    const auto coordinate = [&](int i,
                                                int steps,
                                                float extent) {
                        if (i == 0)
                            return -extent;
                        if (i == steps)
                            return extent;
                        const float t
                            = static_cast<float>(i - (bevel ? 1 : 0))
                              / static_cast<float>(steps - (bevel ? 2 : 0));
                        return std::lerp(radius - extent, extent - radius, t);
                    };
                    const auto base
                        = static_cast<std::uint32_t>(mesh->vertices.size());
                    for (int row = 0; row <= rows; ++row)
                    {
                        for (int column = 0; column <= columns; ++column)
                        {
                            const float u = coordinate(column, columns, halfU),
                                        v = coordinate(row, rows, halfV);
                            const Position point{
                                surface.center.x
                                    + (face >= 4 ? normal.x * surface.halfSize.x
                                                 : u),
                                surface.center.y
                                    + (horizontal
                                           ? normal.y * surface.halfSize.y
                                           : v),
                                surface.center.z
                                    + (face >= 4 ? u
                                       : horizontal
                                           ? v
                                           : normal.z * surface.halfSize.z)
                            };
                            const float event
                                = surface.stairLamp
                                      ? 1.0f
                                      : (surface.exterior
                                             ? 0.0f
                                             : 0.65f
                                                   * std::clamp(
                                                       1.0f
                                                           - std::abs(
                                                                 point.z
                                                                 - scene
                                                                       .eventDepth)
                                                                 / 4.0f,
                                                       0.0f,
                                                       1.0f));
                            const bool side = face >= 4;
                            const float sign
                                = side ? -normal.x
                                       : (horizontal ? -normal.y : normal.z);
                            Rendering::WorldVertex vertex{
                                { point.x, point.y, point.z },
                                { normal.x, normal.y, normal.z },
                                { side ? 0.0f : 1.0f,
                                  0.0f,
                                  side ? 1.0f : 0.0f,
                                  sign },
                                { surface.color.x,
                                  surface.color.y,
                                  surface.color.z },
                                { (side ? point.z : point.x) * 0.5f,
                                  (horizontal ? point.z : point.y) * 0.5f,
                                  static_cast<float>(surface.material) },
                                {},
                                { kLight[face]
                                      * (surface.exterior ? 0.80f : 0.23f),
                                  surface.stairLamp
                                      ? 1.65f
                                      : (surface.exterior
                                                 && surface.color.x > 0.50f
                                             ? 2.2f
                                             : 0.0f),
                                  event }
                            };
                            if (bevel)
                            {
                                const Rendering::Float3 center{
                                    surface.center.x,
                                    surface.center.y,
                                    surface.center.z
                                };
                                const Rendering::Float3 core{ half.x - radius,
                                                              half.y - radius,
                                                              half.z - radius };
                                Rendering::Float3 nearest{}, offset{};
                                float squared = 0;
                                for (std::size_t i = 0; i < 3; ++i)
                                {
                                    nearest[i] = center[i]
                                                 + std::clamp(vertex.position[i]
                                                                  - center[i],
                                                              -core[i],
                                                              core[i]);
                                    offset[i] = vertex.position[i] - nearest[i];
                                    squared += offset[i] * offset[i];
                                }
                                const float length
                                    = std::sqrt(std::max(squared, 1e-12f));
                                for (std::size_t i = 0; i < 3; ++i)
                                {
                                    vertex.normal[i] = offset[i] / length;
                                    vertex.position[i]
                                        = nearest[i]
                                          + radius * vertex.normal[i];
                                }
                            }
                            mesh->vertices.push_back(vertex);
                        }
                    }
                    for (int row = 0; row < rows; ++row)
                    {
                        for (int column = 0; column < columns; ++column)
                        {
                            const auto first
                                = base
                                  + static_cast<std::uint32_t>(
                                      row * (columns + 1) + column);
                            const auto stride
                                = static_cast<std::uint32_t>(columns + 1);
                            const std::array indices{ first,
                                                      first + 1,
                                                      first + stride,
                                                      first + stride + 1 };
                            for (const auto index : order)
                                mesh->indices.push_back(indices[index]);
                        }
                    }
                }
            }
            mesh->detailEnds[detail]
                = static_cast<std::uint32_t>(mesh->indices.size());
        }
        const auto lighting
            = FrameView(scene, {}, View(0, 0), 1.0f, { 4, 4 }, 1.0f);
        Rendering::BakeLighting(*mesh, lighting);
        return mesh;
    }

    auto
    FrameView(const Apartment &scene,
              Position eye,
              const Camera &camera,
              float aspect,
              SceneQuality quality,
              float stairPower) -> Rendering::WorldView
    {
        const auto vector = [](Position p, float w) {
            return Rendering::Float4{ p.x, p.y, p.z, w };
        };
        Rendering::WorldView view{ vector(eye, stairPower),
                                   vector(camera.right, 1.48f / aspect),
                                   vector(camera.up, 1.48f),
                                   vector(camera.forward,
                                          static_cast<float>(quality.details)),
                                   {},
                                   { { { 1.0f, 0.71f, 0.42f, 0 },
                                       { 0.64f, 0.80f, 1.0f, 0 },
                                       { 1.0f, 0.78f, 0.50f, 0 },
                                       { 0.57f, 0.91f, 0.82f, 0 } } } };
        for (int i = 0; i < quality.lighting; ++i)
        {
            const auto index = static_cast<std::size_t>(i);
            view.lights[index] = vector(scene.lights[index],
                                        i == 1 ? 7.5f * stairPower : 7.5f);
        }
        const float back
            = scene.rain ? -8.76f : (scene.cinema ? -16.4f : -13.1f);
        const float height = scene.rain ? 5.0f : 3.5f;
        view.probeCenter = { 0, height * 0.5f, (4.95f + back) * 0.5f, 0 };
        view.probeHalfSize
            = { scene.rain ? 1.98f : (scene.cinema ? 5.2f : 4.5f),
                height * 0.5f,
                (4.95f - back) * 0.5f,
                0 };
        view.probeHalfSize[3] = scene.rain ? 2.0f : 1.0f;
        if (scene.rain)
        {
            view.streetCenter = { -1.0f, 2.60f, 11.3f, 0 };
            view.streetHalfSize = { 12.0f, 2.8f, 6.0f, 0 };
            view.streetLamp = { -1.38f, 3.63f, 14.12f, 12.0f };
        }
        return view;
    }
} // namespace TwentyThreeSeventeen::Game
