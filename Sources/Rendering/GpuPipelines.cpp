// SPDX-License-Identifier: MIT
#include <cstddef>

#include "GpuPipelines.hpp"
#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    namespace
    {
#include "SceneSpirv.inc"
#if defined(__APPLE__)
#    include "SceneMetallib.inc"
        constexpr std::span<const Uint8> kEmbeddedMetal = kMetalLibrary;
#else
        constexpr std::span<const Uint8> kEmbeddedMetal{};
#endif

        struct Stage
        {
            std::span<const Uint8> spirv;
            const char *metalEntry;
        };

        struct PipelineShaders
        {
            Stage vertex, fragment;
            std::span<const SDL_GPUVertexAttribute> attributes;
            Uint32 stride, uniforms, samplers;
        };

        auto
        Pipeline(SDL_GPUDevice *device,
                 Backend backend,
                 SDL_GPUTextureFormat colorFormat,
                 SDL_GPUTextureFormat depthFormat,
                 const PipelineShaders &shaders) -> SDL_GPUGraphicsPipeline *
        {
            const auto compile = [=](const Stage &stage,
                                     SDL_GPUShaderStage kind,
                                     Uint32 textures) {
                SDL_GPUShaderCreateInfo shader{};
                shader.code_size = stage.spirv.size();
                shader.code = stage.spirv.data();
                shader.entrypoint = "main";
                shader.format = SDL_GPU_SHADERFORMAT_SPIRV;
                if (backend == Backend::kMetal)
                {
                    shader.code = kEmbeddedMetal.data();
                    shader.code_size = kEmbeddedMetal.size();
                    shader.entrypoint = stage.metalEntry;
                    shader.format = SDL_GPU_SHADERFORMAT_METALLIB;
                }
                shader.stage = kind;
                shader.num_uniform_buffers = shaders.uniforms;
                shader.num_samplers = textures;
                return SDL_CreateGPUShader(device, &shader);
            };
            auto *vertex
                = compile(shaders.vertex, SDL_GPU_SHADERSTAGE_VERTEX, 0);
            if (!vertex)
                return nullptr;
            auto *fragment = compile(shaders.fragment,
                                     SDL_GPU_SHADERSTAGE_FRAGMENT,
                                     shaders.samplers);
            if (!fragment)
            {
                SDL_ReleaseGPUShader(device, vertex);
                return nullptr;
            }
            SDL_GPUVertexBufferDescription buffer{};
            buffer.pitch = shaders.stride;
            SDL_GPUColorTargetDescription color{};
            color.format = colorFormat;
            SDL_GPUGraphicsPipelineCreateInfo layout{};
            layout.vertex_shader = vertex;
            layout.fragment_shader = fragment;
            if (!shaders.attributes.empty())
                layout.vertex_input_state
                    = { &buffer,
                        1,
                        shaders.attributes.data(),
                        static_cast<Uint32>(shaders.attributes.size()) };
            layout.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
            layout.rasterizer_state.enable_depth_clip = true;
            if (shaders.uniforms > 0)
                layout.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
            layout.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
            layout.depth_stencil_state.enable_depth_test = shaders.stride > 0;
            layout.depth_stencil_state.enable_depth_write = shaders.stride > 0;
            layout.target_info.color_target_descriptions = &color;
            layout.target_info.num_color_targets = 1;
            layout.target_info.depth_stencil_format = depthFormat;
            layout.target_info.has_depth_stencil_target = shaders.stride > 0;
            auto *pipeline = SDL_CreateGPUGraphicsPipeline(device, &layout);
            SDL_ReleaseGPUShader(device, vertex);
            SDL_ReleaseGPUShader(device, fragment);
            return pipeline;
        }

    } // namespace

    auto
    CreateGpuPipelines(SDL_GPUDevice *device,
                       Backend backend,
                       SDL_GPUTextureFormat color,
                       SDL_GPUTextureFormat depth) -> GpuPipelines
    {
        constexpr SDL_GPUVertexAttribute overlayAttributes[]{
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0 },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, red) }
        };
        std::array<SDL_GPUVertexAttribute, kWorldAttributes.size()>
            worldAttributes{};
        for (Uint32 i = 0; i < worldAttributes.size(); ++i)
        {
            const auto attribute = kWorldAttributes[i];
            worldAttributes[i] = { i,
                                   0,
                                   attribute.components == 4
                                       ? SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4
                                       : SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                   attribute.offset };
        }
        GpuPipelines pipelines{};
        pipelines.overlay = Pipeline(device,
                                     backend,
                                     color,
                                     depth,
                                     { { kVertSpirv, "SceneVert" },
                                       { kFragSpirv, "SceneFrag" },
                                       overlayAttributes,
                                       sizeof(Vertex),
                                       0,
                                       0 });
        if (!pipelines.overlay)
            return pipelines;
        pipelines.world = Pipeline(device,
                                   backend,
                                   color,
                                   depth,
                                   { { kWorldVertSpirv, "WorldVert" },
                                     { kWorldFragSpirv, "WorldFrag" },
                                     worldAttributes,
                                     sizeof(WorldVertex),
                                     1,
                                     1 });
        if (!pipelines.world)
            return pipelines;
        pipelines.present = Pipeline(device,
                                     backend,
                                     color,
                                     depth,
                                     { { kPresentVertSpirv, "PresentVert" },
                                       { kPresentFragSpirv, "PresentFrag" },
                                       {},
                                       0,
                                       0,
                                       1 });
        return pipelines;
    }
} // namespace TwentyThreeSeventeen::Rendering
