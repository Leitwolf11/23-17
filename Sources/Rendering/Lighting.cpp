// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cmath>

#include "Rendering/World.hpp"
#include "Vector.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    auto
    WorldMesh::ValidForUpload() const -> bool
    {
        constexpr std::size_t tileBytes
            = MaterialAtlas::kTileSize * MaterialAtlas::kTileSize * 4;
        return materials.materialCount >= 1
               && materials.materialCount <= MaterialAtlas::kMaximumMaterials
               && materials.probeCount >= 1 && materials.probeCount <= 2
               && materials.rgba.size()
                      == static_cast<std::size_t>(materials.LayerCount())
                             * tileBytes
               && !vertices.empty() && !indices.empty()
               && indices.size() % 3 == 0
               && std::ranges::all_of(indices, [&](auto index) {
                      return index < vertices.size();
                  });
    }
    namespace
    {
        struct Bounds
        {
            Float3 low{ 1e20f, 1e20f, 1e20f }, high{ -1e20f, -1e20f, -1e20f };
            void
            Include(Float3 p)
            {
                for (std::size_t i = 0; i < 3; ++i)
                {
                    low[i] = std::min(low[i], p[i]);
                    high[i] = std::max(high[i], p[i]);
                }
            }
            auto
            Intersects(Float3 origin, Float3 inv, float limit) const -> bool
            {
                float first = 0.0f;
                for (std::size_t i = 0; i < 3; ++i)
                {
                    const float a = (low[i] - origin[i]) * inv[i],
                                b = (high[i] - origin[i]) * inv[i];
                    first = std::max(first, std::min(a, b));
                    limit = std::min(limit, std::max(a, b));
                }
                return first <= limit;
            }
        };
        struct Triangle
        {
            Float3 a, ab, ac, center;
            std::size_t index;
        };
        struct Node
        {
            Bounds bounds;
            std::size_t first{}, end{}, left{}, right{};
        };
        struct Hit
        {
            float distance;
            std::size_t triangle{};
            float u{}, v{};
        };
        class RayScene
        {
        public:
            explicit RayScene(const WorldMesh &mesh)
            {
                triangles_.reserve(mesh.indices.size() / 3);
                for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
                {
                    const auto a = mesh.vertices[mesh.indices[i]].position;
                    const auto b = mesh.vertices[mesh.indices[i + 1]].position;
                    const auto c = mesh.vertices[mesh.indices[i + 2]].position;
                    triangles_.push_back(
                        { a,
                          Sub(b, a),
                          Sub(c, a),
                          Scale(Add(Add(a, b), c), 1.0f / 3.0f),
                          i });
                }
                if (!triangles_.empty())
                    Build(0, triangles_.size());
            }
            auto
            Trace(Float3 origin, Float3 direction, Hit &hit, bool any) const
                -> bool
            {
                Float3 inv;
                for (std::size_t i = 0; i < 3; ++i)
                    inv[i] = 1.0f
                             / std::copysign(
                                 std::max(1e-12f, std::abs(direction[i])),
                                 direction[i]);
                return !nodes_.empty()
                       && Visit(0, origin, direction, inv, hit, any);
            }

        private:
            auto
            Build(std::size_t first, std::size_t end) -> std::size_t
            {
                const auto index = nodes_.size();
                nodes_.emplace_back();
                Node node;
                node.first = first;
                node.end = end;
                Bounds centers;
                for (auto i = first; i < end; ++i)
                {
                    const auto &t = triangles_[i];
                    node.bounds.Include(t.a);
                    node.bounds.Include(Add(t.a, t.ab));
                    node.bounds.Include(Add(t.a, t.ac));
                    centers.Include(t.center);
                }
                if (end - first > 8)
                {
                    const auto extent = Sub(centers.high, centers.low);
                    const auto axis = static_cast<std::size_t>(
                        std::ranges::max_element(extent) - extent.begin());
                    const auto mid = first + (end - first) / 2;
                    const auto at = [&](std::size_t i) {
                        return triangles_.begin()
                               + static_cast<std::ptrdiff_t>(i);
                    };
                    std::nth_element(
                        at(first),
                        at(mid),
                        at(end),
                        [axis](const Triangle &a, const Triangle &b) {
                            return a.center[axis] < b.center[axis];
                        });
                    node.left = Build(first, mid);
                    node.right = Build(mid, end);
                }
                nodes_[index] = node;
                return index;
            }
            auto
            Visit(std::size_t index,
                  Float3 origin,
                  Float3 direction,
                  Float3 inv,
                  Hit &hit,
                  bool any) const -> bool
            {
                const auto &node = nodes_[index];
                if (!node.bounds.Intersects(origin, inv, hit.distance))
                    return false;
                if (node.left != 0)
                {
                    const bool left
                        = Visit(node.left, origin, direction, inv, hit, any);
                    if (any && left)
                        return true;
                    return Visit(node.right, origin, direction, inv, hit, any)
                           || left;
                }
                bool found = false;
                for (auto i = node.first; i < node.end; ++i)
                {
                    const auto &t = triangles_[i];
                    const auto p = Cross(direction, t.ac);
                    const float det = Dot(t.ab, p);
                    if (std::abs(det) < 1e-8f)
                        continue;
                    const auto delta = Sub(origin, t.a);
                    const float u = Dot(delta, p) / det;
                    if (u < 0 || u > 1)
                        continue;
                    const auto q = Cross(delta, t.ab);
                    const float v = Dot(direction, q) / det;
                    if (v < 0 || u + v > 1)
                        continue;
                    const float distance = Dot(t.ac, q) / det;
                    if (distance < 0.001f || distance >= hit.distance)
                        continue;
                    hit = { distance, t.index, u, v };
                    if (any)
                        return true;
                    found = true;
                }
                return found;
            }
            std::vector<Triangle> triangles_;
            std::vector<Node> nodes_;
        };

        auto
        Lamp(const WorldView &lights, Float3 point, std::size_t index) -> Float4
        {
            if (index == 0 && lights.probeHalfSize[3] > 1.5f)
                for (std::size_t k = 0; k < 3; ++k)
                    if (std::abs(point[k] - lights.probeCenter[k])
                        > lights.probeHalfSize[k] + 0.12f)
                        return lights.streetLamp;
            return lights.lights[index];
        }
        auto
        ProbeColor(const WorldMesh &mesh,
                   const WorldView &lights,
                   const Hit &hit) -> Float3
        {
            Float3 color{}, base{}, position{}, normal{}, uv{};
            Float4 visibility{};
            float ambient = 0, emission = 0;
            const std::array weights{ 1.0f - hit.u - hit.v, hit.u, hit.v };
            for (std::size_t i = 0; i < 3; ++i)
            {
                const auto &vertex
                    = mesh.vertices[mesh.indices[hit.triangle + i]];
                position = Add(position, Scale(vertex.position, weights[i]));
                normal = Add(normal, Scale(vertex.normal, weights[i]));
                base = Add(base, Scale(vertex.tint, weights[i]));
                uv = Add(uv, Scale(vertex.uvMaterial, weights[i]));
                ambient += vertex.response[0] * weights[i];
                emission += vertex.response[1] * weights[i];
                for (std::size_t l = 0; l < 4; ++l)
                    visibility[l] += vertex.visibility[l] * weights[i];
            }
            const int material = static_cast<int>(std::round(uv[2]));
            if (material > 0 && material <= mesh.materials.materialCount)
            {
                constexpr auto size = MaterialAtlas::kTileSize;
                const auto texel = [](float coord) {
                    return static_cast<int>((coord - std::floor(coord)) * size)
                           % size;
                };
                const auto pixel = static_cast<std::size_t>(
                    ((material - 1) * size * size + texel(uv[1]) * size
                     + texel(uv[0]))
                    * 4);
                for (std::size_t k = 0; k < 3; ++k)
                    base[k]
                        *= 1.84f
                           * static_cast<float>(mesh.materials.rgba[pixel + k])
                           / 255.0f;
            }
            color = { ambient + emission,
                      ambient + emission,
                      ambient + emission };
            for (std::size_t l = 0; l < 4; ++l)
            {
                const auto lamp = Lamp(lights, position, l);
                const auto offset = Sub(Xyz(lamp), position);
                const float amount = std::max(0.0f, Dot(normal, Unit(offset)))
                                     * visibility[l] * lamp[3]
                                     / (1 + Dot(offset, offset));
                color = Add(color, Scale(Xyz(lights.colors[l]), amount));
            }
            for (std::size_t k = 0; k < 3; ++k)
                color[k] *= base[k] * base[k];
            return color;
        }
    } // namespace

    void
    BakeLighting(WorldMesh &mesh, const WorldView &lights)
    {
        // Normalize imported and generated winding before submitting
        // single-sided surfaces.
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            const auto &a = mesh.vertices[mesh.indices[i]],
                       &b = mesh.vertices[mesh.indices[i + 1]],
                       &c = mesh.vertices[mesh.indices[i + 2]];
            const auto geometric = Cross(Sub(b.position, a.position),
                                         Sub(c.position, a.position));
            if (Dot(geometric, Add(Add(a.normal, b.normal), c.normal)) < 0)
                std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
        }
        const RayScene scene(mesh);
        constexpr std::array<Float3, 5> aperture{ { { 0, 0, 0 },
                                                    { -0.21f, 0, 0 },
                                                    { 0.21f, 0, 0 },
                                                    { 0, 0, -0.21f },
                                                    { 0, 0, 0.21f } } };
        for (auto &v : mesh.vertices)
        {
            const auto origin = Add(v.position, Scale(v.normal, 0.010f));
            const auto tangent = Xyz(v.tangent),
                       bitangent = Cross(v.normal, tangent);
            float ambient = 1;
            for (const auto flank : { tangent,
                                      Scale(tangent, -1),
                                      bitangent,
                                      Scale(bitangent, -1) })
            {
                Hit contact{ 0.85f };
                if (scene.Trace(origin,
                                Unit(Add(flank, Scale(v.normal, 0.72f))),
                                contact,
                                false))
                    ambient -= 0.18f * (1.0f - contact.distance / 0.85f);
            }
            v.response[0] *= ambient;
            for (std::size_t l = 0; l < 4; ++l)
            {
                const auto lamp = Lamp(lights, v.position, l);
                const auto center = Sub(Xyz(lamp), origin);
                if (Dot(center, v.normal) < -0.02f || lamp[3] <= 0)
                    continue;
                float visible = 0;
                for (const auto offset : aperture)
                {
                    const auto direction = Add(center, offset);
                    Hit shadow{ std::sqrt(Dot(direction, direction)) - 0.16f };
                    if (!scene.Trace(origin, Unit(direction), shadow, true))
                        visible += 0.2f;
                }
                v.visibility[l] = visible;
            }
        }
        // The last array layer carries local incident radiance in a square-root
        // encoding.
        constexpr int size = MaterialAtlas::kTileSize, width = 192,
                      height = 192;
        mesh.materials.probeCount = lights.probeHalfSize[3] > 1.5f ? 2 : 1;
        mesh.materials.rgba.resize(static_cast<std::size_t>(
            size * size * 4 * mesh.materials.LayerCount()));
        for (int probe = 0; probe < mesh.materials.probeCount; ++probe)
        {
            const auto base = static_cast<std::size_t>(
                size * size * 4 * (2 * mesh.materials.materialCount + probe));
            std::vector<Float3> capture(width * height);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    const float u
                        = 2.0f * (static_cast<float>(x) + 0.5f) / width - 1.0f;
                    const float v
                        = 2.0f * (static_cast<float>(y) + 0.5f) / height - 1.0f;
                    Float3 octant{ u, v, 1.0f - std::abs(u) - std::abs(v) };
                    if (octant[2] < 0)
                    {
                        octant[0] = std::copysign(1.0f - std::abs(v), u);
                        octant[1] = std::copysign(1.0f - std::abs(u), v);
                    }
                    const auto direction = Unit(octant);
                    Hit hit{ 60.0f };
                    capture[static_cast<std::size_t>(y * width + x)]
                        = scene.Trace(Xyz(probe == 0 ? lights.probeCenter
                                                     : lights.streetCenter),
                                      direction,
                                      hit,
                                      false)
                              ? ProbeColor(mesh, lights, hit)
                              : Float3{ 0.04f, 0.061f, 0.095f };
                }
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                {
                    const auto pixel
                        = base + static_cast<std::size_t>((y * size + x) * 4);
                    const auto color = capture[static_cast<std::size_t>(
                        (y * height / size) * width + x * width / size)];
                    for (std::size_t k = 0; k < 3; ++k)
                        mesh.materials.rgba[pixel + k]
                            = static_cast<std::uint8_t>(
                                255.0f
                                * std::sqrt(
                                    std::clamp(color[k] * 0.25f, 0.0f, 1.0f)));
                    mesh.materials.rgba[pixel + 3] = 255;
                }
        }
    }
} // namespace TwentyThreeSeventeen::Rendering
