// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <cgltf.h>
#include <cmath>
#include <span>

#include "GltfMaterials.hpp"
#include "Rendering/ScannedMesh.hpp"
#include "Vector.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    namespace
    {
        constexpr auto kTileBytes
            = static_cast<std::size_t>(MaterialAtlas::kTileSize)
              * MaterialAtlas::kTileSize * 4;

        struct MaterialSlot
        {
            const cgltf_material *source;
            GltfMaterialTile data;
        };

        struct MaterialChoice
        {
            float index;
            Float3 tint;
            float emission;
        };

        auto
        MaterialIndex(std::vector<MaterialSlot> &slots,
                      int existing,
                      const cgltf_material *source)
            -> tl::expected<MaterialChoice, std::string>
        {
            auto match
                = std::ranges::find(slots, source, &MaterialSlot::source);
            if (match == slots.end())
            {
                if (existing + static_cast<int>(slots.size())
                    >= MaterialAtlas::kMaximumMaterials)
                    return tl::unexpected(
                        std::string("The world material atlas is full."));
                auto maps = PackGltfMaterial(source);
                if (!maps)
                    return tl::unexpected(maps.error());
                slots.push_back({ source, std::move(*maps) });
                match = slots.end() - 1;
            }
            return MaterialChoice{ static_cast<float>(
                                       existing + (match - slots.begin()) + 1),
                                   match->data.tint,
                                   match->data.emission };
        }

        void
        AppendMaterials(MaterialAtlas &atlas,
                        const std::vector<MaterialSlot> &slots)
        {
            if (slots.empty())
                return;
            const auto baseEnd
                = static_cast<std::size_t>(atlas.materialCount) * kTileBytes;
            const auto newBytes = slots.size() * kTileBytes;
            auto &pixels = atlas.rgba;
            pixels.resize(pixels.size() + 2 * newBytes);
            auto split = pixels.begin() + static_cast<std::ptrdiff_t>(baseEnd);
            auto normal = split + static_cast<std::ptrdiff_t>(newBytes);
            std::move_backward(split,
                               split + static_cast<std::ptrdiff_t>(baseEnd),
                               normal + static_cast<std::ptrdiff_t>(baseEnd));
            normal += static_cast<std::ptrdiff_t>(baseEnd);
            for (const auto &slot : slots)
            {
                split = std::copy(slot.data.color.begin(),
                                  slot.data.color.end(),
                                  split);
                normal = std::copy(slot.data.surface.begin(),
                                   slot.data.surface.end(),
                                   normal);
            }
            atlas.materialCount += static_cast<int>(slots.size());
        }

        auto
        AppendModel(WorldMesh &mesh,
                    const std::string &path,
                    Float3 origin,
                    float scale,
                    float heading,
                    int atlasTile) -> tl::expected<void, std::string>
        {
            const auto &atlas = mesh.materials;
            if (atlas.materialCount < 0
                || atlas.materialCount > MaterialAtlas::kMaximumMaterials
                || atlasTile < 0 || atlasTile > atlas.materialCount
                || atlas.rgba.size()
                       != 2 * static_cast<std::size_t>(atlas.materialCount)
                              * kTileBytes)
                return tl::unexpected(
                    std::string("Import models before baking world lighting."));
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
                return tl::unexpected(
                    std::string("Invalid embedded GLB mesh."));
            if (data->extensions_required_count != 0)
                return tl::unexpected(std::string(
                    "The static GLB requires an unsupported extension."));
            std::vector<MaterialSlot> slots;
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
                if (data->scene != nullptr)
                {
                    const auto *root = &node;
                    while (root->parent != nullptr)
                        root = root->parent;
                    const auto visible = std::span(data->scene->nodes,
                                                   data->scene->nodes_count);
                    if (std::ranges::find(visible, root) == visible.end())
                        continue;
                }
                if (node.skin != nullptr)
                    return tl::unexpected(
                        std::string("Expected a static GLB model."));
                std::array<float, 16> transform{};
                cgltf_node_transform_world(&node, transform.data());
                const Float3 x{ transform[0], transform[1], transform[2] },
                    y{ transform[4], transform[5], transform[6] },
                    z{ transform[8], transform[9], transform[10] };
                const auto nx = Cross(y, z), ny = Cross(z, x), nz = Cross(x, y);
                const float normalSign = std::copysign(1.0f, Dot(x, nx));
                for (const auto &prim : std::span(node.mesh->primitives,
                                                  node.mesh->primitives_count))
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
                    if (prim.type != cgltf_primitive_type_triangles
                        || !positions || !normals || !uv || !prim.indices
                        || prim.indices->count % 3 != 0)
                        return tl::unexpected(
                            std::string("Expected indexed triangles with "
                                        "normals and UVs."));
                    const auto tile
                        = atlasTile > 0
                              ? tl::expected<MaterialChoice, std::string>(
                                    MaterialChoice{
                                        static_cast<float>(atlasTile),
                                        { 0.54f, 0.54f, 0.54f },
                                        0 })
                              : MaterialIndex(slots,
                                              atlas.materialCount,
                                              prim.material);
                    if (!tile)
                        return tl::unexpected(tile.error());
                    const auto *color
                        = cgltf_find_accessor(&prim,
                                              cgltf_attribute_type_color,
                                              0);
                    if (positions->count
                        > UINT32_MAX - imported.vertices.size())
                        return tl::unexpected(
                            std::string("GLB has too many vertices."));
                    const auto base
                        = static_cast<std::uint32_t>(imported.vertices.size());
                    std::vector<Float3> tangents(positions->count),
                        bitangents(positions->count);
                    for (std::size_t i = 0; i < positions->count; ++i)
                    {
                        Float3 p{}, n{};
                        std::array<float, 2> st{};
                        if (!cgltf_accessor_read_float(positions,
                                                       i,
                                                       p.data(),
                                                       3)
                            || !cgltf_accessor_read_float(normals,
                                                          i,
                                                          n.data(),
                                                          3)
                            || !cgltf_accessor_read_float(uv, i, st.data(), 2))
                            return tl::unexpected(
                                std::string("Unreadable GLB attribute."));
                        auto tint = tile->tint;
                        if (atlasTile == 0 && color != nullptr)
                        {
                            Float3 multiplier{};
                            if (!cgltf_accessor_read_float(color,
                                                           i,
                                                           multiplier.data(),
                                                           3))
                                return tl::unexpected(std::string(
                                    "Unreadable GLB vertex color."));
                            for (std::size_t channel = 0; channel < tint.size();
                                 ++channel)
                                tint[channel]
                                    *= std::sqrt(std::clamp(multiplier[channel],
                                                            0.0f,
                                                            1.0f));
                        }
                        Float3 wp{}, wn{};
                        for (std::size_t k = 0; k < 3; ++k)
                        {
                            wp[k] = x[k] * p[0] + y[k] * p[1] + z[k] * p[2]
                                    + transform[12 + k];
                            wn[k] = normalSign
                                    * (nx[k] * n[0] + ny[k] * n[1]
                                       + nz[k] * n[2]);
                        }
                        wp = rotate(wp);
                        wn = Unit(rotate(wn));
                        for (std::size_t k = 0; k < 3; ++k)
                            wp[k] = wp[k] * scale + origin[k];
                        imported.vertices.push_back(
                            { wp,
                              wn,
                              {},
                              tint,
                              { st[0], st[1], tile->index },
                              {},
                              { 0.35f, tile->emission, 0, 0 } });
                    }
                    for (std::size_t i = 0; i < prim.indices->count; i += 3)
                    {
                        std::array<std::uint32_t, 3> tri{};
                        for (std::size_t k = 0; k < 3; ++k)
                        {
                            const auto index
                                = cgltf_accessor_read_index(prim.indices,
                                                            i + k);
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
                                      std::copysign(1.0f,
                                                    Dot(Cross(v.normal, t),
                                                        bitangents[i])) };
                    }
                }
            }
            if (imported.vertices.size() > UINT32_MAX - mesh.vertices.size())
                return tl::unexpected(
                    std::string("The world vertex buffer is full."));
            if (imported.indices.empty())
                return tl::unexpected(std::string("Empty GLB geometry."));
            const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
            mesh.vertices.insert(mesh.vertices.end(),
                                 imported.vertices.begin(),
                                 imported.vertices.end());
            for (const auto index : imported.indices)
                mesh.indices.push_back(base + index);
            AppendMaterials(mesh.materials, slots);
            return {};
        }
    } // namespace

    auto
    AppendTexturedModel(WorldMesh &mesh,
                        const std::string &path,
                        Float3 origin,
                        float scale,
                        float heading) -> tl::expected<void, std::string>
    {
        return AppendModel(mesh, path, origin, scale, heading, 0);
    }

    auto
    AppendScan(WorldMesh &mesh,
               const std::string &path,
               Float3 origin,
               float scale,
               float heading,
               int atlasTile) -> tl::expected<void, std::string>
    {
        return AppendModel(mesh, path, origin, scale, heading, atlasTile);
    }
} // namespace TwentyThreeSeventeen::Rendering
