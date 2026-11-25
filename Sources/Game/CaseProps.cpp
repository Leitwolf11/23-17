// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <array>
#include <cgltf.h>
#include <memory>
#include <span>
#include <string>

#include "Game/CaseProps.hpp"
#include "rules_cc/cc/runfiles/runfiles.h"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        using Physics::Position;
        using Runfiles = rules_cc::cc::runfiles::Runfiles;

        auto
        AddPrimitive(std::vector<ActorPoint> &fixtures,
                     const cgltf_primitive &primitive,
                     const std::array<float, 16> &transform,
                     Position origin) -> void
        {
            if (primitive.type != cgltf_primitive_type_triangles)
                return;
            const auto *positions
                = cgltf_find_accessor(&primitive,
                                      cgltf_attribute_type_position,
                                      0);
            const auto *colors = cgltf_find_accessor(&primitive,
                                                     cgltf_attribute_type_color,
                                                     0);
            if (positions == nullptr)
                return;
            const auto count = primitive.indices == nullptr
                                   ? positions->count
                                   : primitive.indices->count;
            if (count % 3 != 0)
                return;
            Position material{ 1, 1, 1 };
            if (primitive.material != nullptr
                && primitive.material->has_pbr_metallic_roughness)
            {
                const auto &factor = primitive.material->pbr_metallic_roughness
                                         .base_color_factor;
                material = { factor[0], factor[1], factor[2] };
            }
            std::vector<ActorPoint> points;
            points.reserve(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                const auto vertex
                    = primitive.indices == nullptr
                          ? index
                          : cgltf_accessor_read_index(primitive.indices, index);
                std::array<float, 3> p{};
                if (vertex >= positions->count
                    || !cgltf_accessor_read_float(positions,
                                                  vertex,
                                                  p.data(),
                                                  p.size()))
                    return;
                std::array color{ 1.0f, 1.0f, 1.0f, 1.0f };
                if (colors != nullptr
                    && (vertex >= colors->count
                        || !cgltf_accessor_read_float(colors,
                                                      vertex,
                                                      color.data(),
                                                      color.size())))
                    return;
                points.push_back(
                    { { origin.x + transform[0] * p[0] + transform[4] * p[1]
                            + transform[8] * p[2] + transform[12],
                        origin.y + transform[1] * p[0] + transform[5] * p[1]
                            + transform[9] * p[2] + transform[13],
                        origin.z + transform[2] * p[0] + transform[6] * p[1]
                            + transform[10] * p[2] + transform[14] },
                      { material.x * color[0],
                        material.y * color[1],
                        material.z * color[2] } });
            }
            fixtures.insert(fixtures.end(), points.begin(), points.end());
        }

        auto
        Append(std::vector<ActorPoint> &fixtures,
               const std::string &path,
               std::span<const Position> instances) -> bool
        {
            std::size_t size = 0;
            const std::unique_ptr<void, decltype(&SDL_free)> bytes(
                path.empty() ? nullptr : SDL_LoadFile(path.c_str(), &size),
                SDL_free);
            if (bytes == nullptr)
                return false;
            cgltf_options options{};
            options.type = cgltf_file_type_glb;
            cgltf_data *parsed = nullptr;
            if (cgltf_parse(&options, bytes.get(), size, &parsed)
                != cgltf_result_success)
                return false;
            const std::unique_ptr<cgltf_data, decltype(&cgltf_free)> asset(
                parsed,
                cgltf_free);
            for (const auto &buffer :
                 std::span(asset->buffers, asset->buffers_count))
                if (buffer.uri != nullptr)
                    return false;
            if (cgltf_load_buffers(&options, asset.get(), nullptr)
                    != cgltf_result_success
                || cgltf_validate(asset.get()) != cgltf_result_success)
                return false;
            const auto before = fixtures.size();
            for (const auto &node : std::span(asset->nodes, asset->nodes_count))
            {
                if (node.mesh == nullptr)
                    continue;
                std::array<float, 16> transform{};
                cgltf_node_transform_world(&node, transform.data());
                for (const auto &primitive :
                     std::span(node.mesh->primitives,
                               node.mesh->primitives_count))
                    for (const auto origin : instances)
                        AddPrimitive(fixtures, primitive, transform, origin);
            }
            return fixtures.size() > before;
        }
    } // namespace

    auto
    AddCaseProps(Apartment &scene,
                 Investigation::CaseId id,
                 const char *executable) -> void
    {
        const auto *base = SDL_GetBasePath();
        const std::unique_ptr<Runfiles> runfiles(Runfiles::Create(executable));
        const auto load = [&](const char *name,
                              std::span<const Position> instances) {
            if (runfiles
                && Append(scene.fixtures,
                          runfiles->Rlocation(
                              std::string("_main/Assets/Geometry/") + name),
                          instances))
                return;
            if (base != nullptr
                && Append(scene.fixtures,
                          std::string(base) + "geometry/" + name,
                          instances))
                return;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Could not load %s.",
                        name);
        };
        switch (id)
        {
            case Investigation::CaseId::kApartment:
            {
                constexpr std::array positions{
                    Position{ 1.73f, 1.75f, -2.73f }
                };
                load("stair-recorder.glb", positions);
                break;
            }
            case Investigation::CaseId::kArchive:
            {
                constexpr std::array positions{
                    Position{ -1.45f, 1.18f, 1.65f },
                    Position{ -1.45f, 1.18f, -6.0f }
                };
                load("desk-telephone.glb", positions);
                break;
            }
            case Investigation::CaseId::kCinema:
            {
                constexpr std::array positions{
                    Position{ 3.00f, 0.85f, -0.62f }
                };
                load("projector.glb", positions);
                break;
            }
        }
    }
} // namespace TwentyThreeSeventeen::Game
