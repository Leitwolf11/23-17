// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstddef>

#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    namespace
    {
#include "SceneSpirv.inc"

        constexpr Uint32 kVertexBufferSize = 6 * 1024 * 1024;
        constexpr auto kDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

        class VulkanRenderer final: public Renderer
        {
        public:
            ~VulkanRenderer() override
            {
                if (device_ != nullptr)
                {
                    SDL_WaitForGPUIdle(device_);
                    ReleaseWorld();
                    if (worldPipeline_)
                        SDL_ReleaseGPUGraphicsPipeline(device_, worldPipeline_);
                    if (presentPipeline_)
                        SDL_ReleaseGPUGraphicsPipeline(device_,
                                                       presentPipeline_);
                    if (sampler_)
                        SDL_ReleaseGPUSampler(device_, sampler_);
                    if (edgeSampler_)
                        SDL_ReleaseGPUSampler(device_, edgeSampler_);
                    if (colorTarget_ != nullptr)
                        SDL_ReleaseGPUTexture(device_, colorTarget_);
                    if (depth_ != nullptr)
                        SDL_ReleaseGPUTexture(device_, depth_);
                    if (pipeline_ != nullptr)
                        SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
                    if (upload_ != nullptr)
                        SDL_ReleaseGPUTransferBuffer(device_, upload_);
                    if (vertices_ != nullptr)
                        SDL_ReleaseGPUBuffer(device_, vertices_);
                    if (claimed_)
                        SDL_ReleaseWindowFromGPUDevice(device_, window_);
                    SDL_DestroyGPUDevice(device_);
                }
                SDL_DestroyWindow(window_);
            }

            auto
            Initialize(WindowSize size, bool sync, bool requireHardware)
                -> tl::expected<void, std::string>
            {
                const auto properties = SDL_CreateProperties();
                SDL_SetStringProperty(properties,
                                      SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING,
                                      "vulkan");
                SDL_SetBooleanProperty(
                    properties,
                    SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN,
                    true);
                SDL_SetBooleanProperty(
                    properties,
                    SDL_PROP_GPU_DEVICE_CREATE_VULKAN_REQUIRE_HARDWARE_ACCELERATION_BOOLEAN,
                    requireHardware);
                // These optional features are unnecessary for the apartment
                // pass.
                SDL_SetBooleanProperty(
                    properties,
                    SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN,
                    false);
                SDL_SetBooleanProperty(
                    properties,
                    SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN,
                    false);
                SDL_SetBooleanProperty(
                    properties,
                    SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN,
                    false);
                SDL_SetBooleanProperty(
                    properties,
                    SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN,
                    false);
                device_ = SDL_CreateGPUDeviceWithProperties(properties);
                SDL_DestroyProperties(properties);
                if (device_ == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));

                window_ = SDL_CreateWindow("23.17",
                                           size.width,
                                           size.height,
                                           SDL_WINDOW_RESIZABLE
                                               | SDL_WINDOW_HIDDEN);
                if (window_ == nullptr
                    || !SDL_ClaimWindowForGPUDevice(device_, window_))
                    return tl::unexpected(std::string(SDL_GetError()));
                claimed_ = true;

                if (!sync)
                {
                    if (!SDL_WindowSupportsGPUPresentMode(
                            device_,
                            window_,
                            SDL_GPU_PRESENTMODE_IMMEDIATE))
                        return tl::unexpected(std::string(
                            "Vulkan cannot disable VSync on this display."));
                    if (!SDL_SetGPUSwapchainParameters(
                            device_,
                            window_,
                            SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                            SDL_GPU_PRESENTMODE_IMMEDIATE))
                        return tl::unexpected(std::string(SDL_GetError()));
                }

                SDL_GPUBufferCreateInfo vertexStorage{};
                vertexStorage.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
                vertexStorage.size = kVertexBufferSize;
                vertices_ = SDL_CreateGPUBuffer(device_, &vertexStorage);
                SDL_GPUTransferBufferCreateInfo transferStorage{};
                transferStorage.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
                transferStorage.size = kVertexBufferSize;
                upload_
                    = SDL_CreateGPUTransferBuffer(device_, &transferStorage);
                if (vertices_ == nullptr || upload_ == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));

                colorFormat_
                    = SDL_GetGPUSwapchainTextureFormat(device_, window_);
                constexpr SDL_GPUVertexAttribute overlayAttributes[]{
                    { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0 },
                    { 1,
                      0,
                      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                      offsetof(Vertex, red) }
                };
                std::array<SDL_GPUVertexAttribute, kWorldAttributes.size()>
                    worldAttributes{};
                for (Uint32 i = 0; i < worldAttributes.size(); ++i)
                {
                    const auto attribute = kWorldAttributes[i];
                    worldAttributes[i]
                        = { i,
                            0,
                            attribute.components == 4
                                ? SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4
                                : SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            attribute.offset };
                }
                pipeline_ = Pipeline(kVertSpirv,
                                     kFragSpirv,
                                     overlayAttributes,
                                     sizeof(Vertex),
                                     0,
                                     0);
                worldPipeline_ = Pipeline(kWorldVertSpirv,
                                          kWorldFragSpirv,
                                          worldAttributes,
                                          sizeof(WorldVertex),
                                          1,
                                          1);
                presentPipeline_ = Pipeline(kPresentVertSpirv,
                                            kPresentFragSpirv,
                                            {},
                                            0,
                                            0,
                                            1);
                SDL_GPUSamplerCreateInfo sampler{};
                sampler.min_filter = SDL_GPU_FILTER_LINEAR;
                sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
                sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
                sampler.max_lod = 9;
                sampler_ = SDL_CreateGPUSampler(device_, &sampler);
                sampler.address_mode_u
                    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
                sampler.address_mode_v
                    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
                edgeSampler_ = SDL_CreateGPUSampler(device_, &sampler);
                if (!pipeline_ || !worldPipeline_ || !presentPipeline_
                    || !sampler_ || !edgeSampler_)
                    return tl::unexpected(std::string(SDL_GetError()));
                SDL_Log("Vulkan renderer: %s", SDL_GetGPUDeviceDriver(device_));
                return {};
            }

            auto
            ActiveBackend() const noexcept -> Backend override
            {
                return Backend::kVulkan;
            }

            auto
            Window() const noexcept -> SDL_Window * override
            {
                return window_;
            }

            auto
            Draw(std::span<const Vertex> frame,
                 WindowSize renderSize,
                 const WorldFrame *world)
                -> tl::expected<void, std::string> override
            {
                if (world && !world->Valid(cached_))
                    return tl::unexpected(std::string(
                        "Invalid resident scene geometry or texture data."));
                if (world && cached_ != world->mesh)
                {
                    if (!UploadWorld(*world->mesh))
                        return tl::unexpected(
                            std::string("Vulkan world upload: ")
                            + SDL_GetError());
                    cached_ = world->mesh;
                }
                if (frame.size_bytes() > kVertexBufferSize)
                    return tl::unexpected(
                        std::string("The Vulkan vertex buffer is full."));

                auto *mapped = SDL_MapGPUTransferBuffer(device_, upload_, true);
                if (mapped == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));
                const auto mappedBytes
                    = std::span(static_cast<std::byte *>(mapped),
                                static_cast<std::size_t>(kVertexBufferSize));
                std::ranges::copy(std::as_bytes(frame), mappedBytes.begin());
                SDL_UnmapGPUTransferBuffer(device_, upload_);

                auto *commands = SDL_AcquireGPUCommandBuffer(device_);
                if (commands == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));
                SDL_GPUTexture *target = nullptr;
                Uint32 width = 0;
                Uint32 height = 0;
                if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands,
                                                           window_,
                                                           &target,
                                                           &width,
                                                           &height))
                {
                    const std::string error = SDL_GetError();
                    SDL_CancelGPUCommandBuffer(commands);
                    return tl::unexpected(error);
                }
                if (target == nullptr)
                    return Submit(commands);

                const WindowSize windowSize{ static_cast<int>(width),
                                             static_cast<int>(height) };
                if (renderSize.width <= 0 || renderSize.height <= 0)
                    renderSize = windowSize;
                const bool renderOffscreen
                    = world != nullptr || renderSize != windowSize;
                const auto renderWidth = static_cast<Uint32>(renderSize.width);
                const auto renderHeight
                    = static_cast<Uint32>(renderSize.height);
                const auto depthReady = ResizeDepth(renderWidth, renderHeight);
                if (!depthReady
                    || (renderOffscreen && !ResizeColor(renderSize)))
                {
                    const std::string error
                        = depthReady ? SDL_GetError() : depthReady.error();
                    SDL_SubmitGPUCommandBuffer(commands);
                    return tl::unexpected(error);
                }

                if (!frame.empty())
                {
                    auto *copy = SDL_BeginGPUCopyPass(commands);
                    const SDL_GPUTransferBufferLocation source{ upload_, 0 };
                    const SDL_GPUBufferRegion destination{
                        vertices_,
                        0,
                        static_cast<Uint32>(frame.size_bytes())
                    };
                    SDL_UploadToGPUBuffer(copy, &source, &destination, true);
                    SDL_EndGPUCopyPass(copy);
                }

                SDL_GPUColorTargetInfo color{};
                color.texture = renderOffscreen ? colorTarget_ : target;
                color.cycle = renderOffscreen;
                color.clear_color = { 0.035f, 0.044f, 0.058f, 1.0f };
                color.load_op = SDL_GPU_LOADOP_CLEAR;
                color.store_op = SDL_GPU_STOREOP_STORE;
                SDL_GPUDepthStencilTargetInfo depth{};
                depth.texture = depth_;
                depth.clear_depth = 1.0f;
                depth.load_op = SDL_GPU_LOADOP_CLEAR;
                depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
                depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
                depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
                depth.cycle = true;
                auto *pass
                    = SDL_BeginGPURenderPass(commands, &color, 1, &depth);
                if (world)
                {
                    SDL_BindGPUGraphicsPipeline(pass, worldPipeline_);
                    SDL_PushGPUVertexUniformData(commands,
                                                 0,
                                                 &world->view,
                                                 sizeof(WorldView));
                    SDL_PushGPUFragmentUniformData(commands,
                                                   0,
                                                   &world->view,
                                                   sizeof(WorldView));
                    const SDL_GPUTextureSamplerBinding materials{ atlas_,
                                                                  sampler_ };
                    SDL_BindGPUFragmentSamplers(pass, 0, &materials, 1);
                    const SDL_GPUBufferBinding vb{ worldVbo_, 0 },
                        ib{ worldIbo_, 0 };
                    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
                    SDL_BindGPUIndexBuffer(pass,
                                           &ib,
                                           SDL_GPU_INDEXELEMENTSIZE_32BIT);
                    SDL_DrawGPUIndexedPrimitives(pass,
                                                 world->indexCount,
                                                 1,
                                                 0,
                                                 0,
                                                 0);
                }
                SDL_BindGPUGraphicsPipeline(pass, pipeline_);
                const SDL_GPUBufferBinding binding{ vertices_, 0 };
                SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
                if (!frame.empty())
                    SDL_DrawGPUPrimitives(pass,
                                          static_cast<Uint32>(frame.size()),
                                          1,
                                          0,
                                          0);
                SDL_EndGPURenderPass(pass);
                if (renderOffscreen)
                {
                    const auto destination
                        = FitViewport(renderSize, windowSize);
                    SDL_GPUColorTargetInfo output{};
                    output.texture = target;
                    output.load_op = SDL_GPU_LOADOP_CLEAR;
                    output.store_op = SDL_GPU_STOREOP_STORE;
                    output.clear_color = color.clear_color;
                    auto *present
                        = SDL_BeginGPURenderPass(commands, &output, 1, nullptr);
                    const SDL_GPUViewport viewport{
                        static_cast<float>(destination.x),
                        static_cast<float>(destination.y),
                        static_cast<float>(destination.size.width),
                        static_cast<float>(destination.size.height),
                        0,
                        1
                    };
                    SDL_SetGPUViewport(present, &viewport);
                    SDL_BindGPUGraphicsPipeline(present, presentPipeline_);
                    const SDL_GPUTextureSamplerBinding input{ colorTarget_,
                                                              edgeSampler_ };
                    SDL_BindGPUFragmentSamplers(present, 0, &input, 1);
                    SDL_DrawGPUPrimitives(present, 3, 1, 0, 0);
                    SDL_EndGPURenderPass(present);
                }
                return Submit(commands);
            }

        private:
            auto
            Pipeline(std::span<const Uint8> vert,
                     std::span<const Uint8> frag,
                     std::span<const SDL_GPUVertexAttribute> attributes,
                     Uint32 stride,
                     Uint32 uniforms,
                     Uint32 samplers) -> SDL_GPUGraphicsPipeline *
            {
                SDL_GPUShaderCreateInfo shader{};
                shader.code_size = vert.size();
                shader.code = vert.data();
                shader.entrypoint = "main";
                shader.format = SDL_GPU_SHADERFORMAT_SPIRV;
                shader.stage = SDL_GPU_SHADERSTAGE_VERTEX;
                shader.num_uniform_buffers = uniforms;
                auto *vertex = SDL_CreateGPUShader(device_, &shader);
                if (!vertex)
                    return nullptr;
                shader.code_size = frag.size();
                shader.code = frag.data();
                shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
                shader.num_uniform_buffers = uniforms;
                shader.num_samplers = samplers;
                auto *fragment = SDL_CreateGPUShader(device_, &shader);
                if (!fragment)
                {
                    SDL_ReleaseGPUShader(device_, vertex);
                    return nullptr;
                }
                SDL_GPUVertexBufferDescription buffer{};
                buffer.pitch = stride;
                SDL_GPUColorTargetDescription color{};
                color.format = colorFormat_;
                SDL_GPUGraphicsPipelineCreateInfo layout{};
                layout.vertex_shader = vertex;
                layout.fragment_shader = fragment;
                if (!attributes.empty())
                    layout.vertex_input_state
                        = { &buffer,
                            1,
                            attributes.data(),
                            static_cast<Uint32>(attributes.size()) };
                layout.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
                layout.rasterizer_state.enable_depth_clip = true;
                layout.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
                layout.depth_stencil_state.enable_depth_test = stride > 0;
                layout.depth_stencil_state.enable_depth_write = stride > 0;
                layout.target_info.color_target_descriptions = &color;
                layout.target_info.num_color_targets = 1;
                layout.target_info.depth_stencil_format = kDepthFormat;
                layout.target_info.has_depth_stencil_target = stride > 0;
                auto *pipeline
                    = SDL_CreateGPUGraphicsPipeline(device_, &layout);
                SDL_ReleaseGPUShader(device_, vertex);
                SDL_ReleaseGPUShader(device_, fragment);
                return pipeline;
            }

            void
            ReleaseWorld()
            {
                if (worldVbo_)
                    SDL_ReleaseGPUBuffer(device_, worldVbo_);
                if (worldIbo_)
                    SDL_ReleaseGPUBuffer(device_, worldIbo_);
                if (atlas_)
                    SDL_ReleaseGPUTexture(device_, atlas_);
                worldVbo_ = nullptr;
                worldIbo_ = nullptr;
                atlas_ = nullptr;
                cached_.reset();
            }

            auto
            UploadWorld(const WorldMesh &mesh) -> bool
            {
                ReleaseWorld();
                const auto vertices = std::as_bytes(std::span(mesh.vertices));
                const auto indices = std::as_bytes(std::span(mesh.indices));
                const auto texels
                    = std::as_bytes(std::span(mesh.materials.rgba));
                if (vertices.size() + indices.size() + texels.size()
                    > UINT32_MAX)
                    return SDL_SetError("World exceeds the upload size limit.");
                SDL_GPUBufferCreateInfo buffer{};
                buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
                buffer.size = static_cast<Uint32>(vertices.size());
                worldVbo_ = SDL_CreateGPUBuffer(device_, &buffer);
                buffer.usage = SDL_GPU_BUFFERUSAGE_INDEX;
                buffer.size = static_cast<Uint32>(indices.size());
                worldIbo_ = SDL_CreateGPUBuffer(device_, &buffer);
                SDL_GPUTextureCreateInfo texture{};
                texture.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
                texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
                texture.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
                                | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
                texture.width = MaterialAtlas::kTileSize;
                texture.height = MaterialAtlas::kTileSize;
                texture.layer_count_or_depth
                    = static_cast<Uint32>(mesh.materials.LayerCount());
                texture.num_levels = 10;
                atlas_ = SDL_CreateGPUTexture(device_, &texture);
                if (!worldVbo_ || !worldIbo_ || !atlas_)
                    return false;
                SDL_GPUTransferBufferCreateInfo staging{};
                staging.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
                staging.size = static_cast<Uint32>(
                    vertices.size() + indices.size() + texels.size());
                auto *upload = SDL_CreateGPUTransferBuffer(device_, &staging);
                if (!upload)
                    return false;
                auto *mapped = static_cast<std::byte *>(
                    SDL_MapGPUTransferBuffer(device_, upload, false));
                if (!mapped)
                {
                    SDL_ReleaseGPUTransferBuffer(device_, upload);
                    return false;
                }
                // Texels start at a naturally aligned offset for Vulkan image
                // copies.
                auto destination
                    = std::span(mapped, static_cast<std::size_t>(staging.size));
                for (const auto part : { texels, vertices, indices })
                {
                    std::ranges::copy(part, destination.begin());
                    destination = destination.subspan(part.size());
                }
                SDL_UnmapGPUTransferBuffer(device_, upload);
                auto *commands = SDL_AcquireGPUCommandBuffer(device_);
                if (!commands)
                {
                    SDL_ReleaseGPUTransferBuffer(device_, upload);
                    return false;
                }
                auto *copy = SDL_BeginGPUCopyPass(commands);
                for (Uint32 layer = 0; layer < texture.layer_count_or_depth;
                     ++layer)
                {
                    const SDL_GPUTextureTransferInfo src{
                        upload,
                        layer * texture.width * texture.height * 4,
                        0,
                        0
                    };
                    const SDL_GPUTextureRegion dst{
                        atlas_,         0, layer, 0, 0, 0, texture.width,
                        texture.height, 1
                    };
                    SDL_UploadToGPUTexture(copy, &src, &dst, false);
                }
                SDL_GPUTransferBufferLocation src{ upload,
                                                   static_cast<Uint32>(
                                                       texels.size()) };
                SDL_GPUBufferRegion dst{ worldVbo_,
                                         0,
                                         static_cast<Uint32>(vertices.size()) };
                SDL_UploadToGPUBuffer(copy, &src, &dst, false);
                src.offset += dst.size;
                dst = { worldIbo_, 0, static_cast<Uint32>(indices.size()) };
                SDL_UploadToGPUBuffer(copy, &src, &dst, false);
                SDL_EndGPUCopyPass(copy);
                SDL_GenerateMipmapsForGPUTexture(commands, atlas_);
                const bool submitted = SDL_SubmitGPUCommandBuffer(commands);
                SDL_ReleaseGPUTransferBuffer(device_, upload);
                return submitted;
            }

            static auto
            Submit(SDL_GPUCommandBuffer *commands)
                -> tl::expected<void, std::string>
            {
                if (!SDL_SubmitGPUCommandBuffer(commands))
                    return tl::unexpected(std::string(SDL_GetError()));
                return {};
            }

            auto
            ResizeColor(WindowSize size) -> bool
            {
                if (colorTarget_ != nullptr && colorSize_ == size)
                    return true;
                SDL_GPUTextureCreateInfo description{};
                description.type = SDL_GPU_TEXTURETYPE_2D;
                description.format = colorFormat_;
                description.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
                                    | SDL_GPU_TEXTUREUSAGE_SAMPLER;
                description.width = static_cast<Uint32>(size.width);
                description.height = static_cast<Uint32>(size.height);
                description.layer_count_or_depth = 1;
                description.num_levels = 1;
                description.sample_count = SDL_GPU_SAMPLECOUNT_1;
                auto *texture = SDL_CreateGPUTexture(device_, &description);
                if (texture == nullptr)
                    return false;
                if (colorTarget_ != nullptr)
                    SDL_ReleaseGPUTexture(device_, colorTarget_);
                colorTarget_ = texture;
                colorSize_ = size;
                return true;
            }

            auto
            ResizeDepth(Uint32 width, Uint32 height)
                -> tl::expected<void, std::string>
            {
                if (depth_ != nullptr && width == depthWidth_
                    && height == depthHeight_)
                    return {};

                SDL_GPUTextureCreateInfo description{};
                description.type = SDL_GPU_TEXTURETYPE_2D;
                description.format = kDepthFormat;
                description.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
                description.width = width;
                description.height = height;
                description.layer_count_or_depth = 1;
                description.num_levels = 1;
                description.sample_count = SDL_GPU_SAMPLECOUNT_1;
                auto *newDepth = SDL_CreateGPUTexture(device_, &description);
                if (newDepth == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));
                if (depth_ != nullptr)
                    SDL_ReleaseGPUTexture(device_, depth_);
                depth_ = newDepth;
                depthWidth_ = width;
                depthHeight_ = height;
                return {};
            }

            SDL_GPUGraphicsPipeline *worldPipeline_{}, *presentPipeline_{};
            SDL_GPUBuffer *worldVbo_{}, *worldIbo_{};
            SDL_GPUTexture *atlas_{};
            SDL_GPUSampler *sampler_{}, *edgeSampler_{};
            std::shared_ptr<const WorldMesh> cached_;
            SDL_Window *window_{};
            SDL_GPUDevice *device_{};
            SDL_GPUTexture *depth_{};
            SDL_GPUTexture *colorTarget_{};
            SDL_GPUTextureFormat colorFormat_{};
            WindowSize colorSize_{};
            SDL_GPUBuffer *vertices_{};
            SDL_GPUTransferBuffer *upload_{};
            SDL_GPUGraphicsPipeline *pipeline_{};
            Uint32 depthWidth_{};
            Uint32 depthHeight_{};
            bool claimed_{};
        };
    } // namespace

    auto
    Vulkan(WindowSize size, bool sync, bool requireHardware)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>
    {
        auto renderer = std::make_unique<VulkanRenderer>();
        if (const auto initialized
            = renderer->Initialize(size, sync, requireHardware);
            !initialized)
            return tl::unexpected(initialized.error());
        return std::unique_ptr<Renderer>(std::move(renderer));
    }
} // namespace TwentyThreeSeventeen::Rendering
