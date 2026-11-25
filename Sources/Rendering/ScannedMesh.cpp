// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <cgltf.h>
#include <cmath>
#include <span>

#include "Rendering/ScannedMesh.hpp"
#include "Vector.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    auto
    AppendScan(WorldMesh &mesh,
               const std::string &path,
               Float3 origin,
               float scale,
               float heading,
               float tile) -> tl::expected<void, std::string>
    {
        std::size_t length = 0;
        const std::unique_ptr<void, decltype(&SDL_free)> bytes(
            SDL_LoadFile(path.c_str(), &length),
            SDL_free);
        if (!bytes)
            return tl::unexpected(std::string(SDL_GetError()));
        cgltf_options opts{};
        opts.type = cgltf_file_type_glb;
        cgltf_data *parsed = nullptr;
        if (cgltf_parse(&opts, bytes.get(), length, &parsed)
            != cgltf_result_success)
            return tl::unexpected(std::string("Invalid GLB header."));
        std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(parsed,
                                                                cgltf_free);
        if (data->buffers_count != 1 || data->buffers[0].uri != nullptr
            || cgltf_load_buffers(&opts, data.get(), nullptr)
                   != cgltf_result_success
            || cgltf_validate(data.get()) != cgltf_result_success)
            return tl::unexpected(std::string("Invalid embedded GLB mesh."));
        WorldMesh imported;
        const float cs = std::cos(heading), sn = std::sin(heading);
        const auto rotate = [&](Float3 p) {
            return Float3{ cs * p[0] + sn * p[2],
                           p[1],
                           -sn * p[0] + cs * p[2] };
        };
        for (const auto &node : std::span(data->nodes, data->nodes_count))
        {
            if (!node.mesh)
                continue;
            std::array<float, 16> transform{};
            cgltf_node_transform_world(&node, transform.data());
            const Float3 x{ transform[0], transform[1], transform[2] },
                y{ transform[4], transform[5], transform[6] },
                z{ transform[8], transform[9], transform[10] };
            const auto nx = Cross(y, z), ny = Cross(z, x), nz = Cross(x, y);
            const float normalSign = std::copysign(1.0f, Dot(x, nx));
            for (const auto &prim :
                 std::span(node.mesh->primitives, node.mesh->primitives_count))
            {
                const auto *positions
                    = cgltf_find_accessor(&prim,
                                          cgltf_attribute_type_position,
                                          0);
                const auto *normals
                    = cgltf_find_accessor(&prim,
                                          cgltf_attribute_type_normal,
                                          0);
                const auto *uv
                    = cgltf_find_accessor(&prim,
                                          cgltf_attribute_type_texcoord,
                                          0);
                if (prim.type != cgltf_primitive_type_triangles || !positions
                    || !normals || !uv || !prim.indices
                    || prim.indices->count % 3 != 0)
                    return tl::unexpected(std::string(
                        "Expected indexed triangles with normals and UVs."));
                const auto base
                    = static_cast<std::uint32_t>(imported.vertices.size());
                std::vector<Float3> tangents(positions->count),
                    bitangents(positions->count);
                for (std::size_t i = 0; i < positions->count; ++i)
                {
                    Float3 p{}, n{};
                    std::array<float, 2> st{};
                    if (!cgltf_accessor_read_float(positions, i, p.data(), 3)
                        || !cgltf_accessor_read_float(normals, i, n.data(), 3)
                        || !cgltf_accessor_read_float(uv, i, st.data(), 2))
                        return tl::unexpected(
                            std::string("Unreadable GLB attribute."));
                    Float3 wp{}, wn{};
                    for (std::size_t k = 0; k < 3; ++k)
                    {
                        wp[k] = x[k] * p[0] + y[k] * p[1] + z[k] * p[2]
                                + transform[12 + k];
                        wn[k] = normalSign
                                * (nx[k] * n[0] + ny[k] * n[1] + nz[k] * n[2]);
                    }
                    wp = rotate(wp);
                    wn = Unit(rotate(wn));
                    for (std::size_t k = 0; k < 3; ++k)
                        wp[k] = wp[k] * scale + origin[k];
                    imported.vertices.push_back({ wp,
                                                  wn,
                                                  {},
                                                  { 0.54f, 0.54f, 0.54f },
                                                  { st[0], st[1], tile },
                                                  {},
                                                  { 0.35f, 0, 0 } });
                }
                for (std::size_t i = 0; i < prim.indices->count; i += 3)
                {
                    std::array<std::uint32_t, 3> tri{};
                    for (std::size_t k = 0; k < 3; ++k)
                    {
                        const auto index
                            = cgltf_accessor_read_index(prim.indices, i + k);
                        if (index >= positions->count
                            || index > UINT32_MAX - base)
                            return tl::unexpected(
                                std::string("Invalid GLB triangle index."));
                        tri[k] = static_cast<std::uint32_t>(index);
                        imported.indices.push_back(base + tri[k]);
                    }
                    const auto &a = imported.vertices[base + tri[0]],
                               &b = imported.vertices[base + tri[1]],
                               &c = imported.vertices[base + tri[2]];
                    const auto ab = Sub(b.position, a.position),
                               ac = Sub(c.position, a.position),
                               u = Sub(b.uvMaterial, a.uvMaterial),
                               v = Sub(c.uvMaterial, a.uvMaterial);
                    const auto det = u[0] * v[1] - u[1] * v[0];
                    if (std::abs(det) < 1e-10f)
                        continue;
                    for (auto id : tri)
                        for (std::size_t k = 0; k < 3; ++k)
                        {
                            tangents[id][k]
                                += (ab[k] * v[1] - ac[k] * u[1]) / det;
                            bitangents[id][k]
                                += (ac[k] * u[0] - ab[k] * v[0]) / det;
                        }
                }
                for (std::size_t i = 0; i < positions->count; ++i)
                {
                    auto &v = imported.vertices[base + i];
                    auto t = tangents[i];
                    const auto proj = Dot(v.normal, t);
                    for (std::size_t k = 0; k < 3; ++k)
                        t[k] -= v.normal[k] * proj;
                    if (Dot(t, t) < 1e-10f)
                        t = Cross(v.normal,
                                  std::abs(v.normal[1]) > 0.9f
                                      ? Float3{ 1, 0, 0 }
                                      : Float3{ 0, 1, 0 });
                    t = Unit(t);
                    v.tangent = { t[0],
                                  t[1],
                                  t[2],
                                  std::copysign(
                                      1.0f,
                                      Dot(Cross(v.normal, t), bitangents[i])) };
                }
            }
        }
        if (imported.indices.empty())
            return tl::unexpected(std::string("Empty GLB geometry."));
        const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(),
                             imported.vertices.begin(),
                             imported.vertices.end());
        for (const auto index : imported.indices)
            mesh.indices.push_back(base + index);
        return {};
    }
} // namespace TwentyThreeSeventeen::Rendering
