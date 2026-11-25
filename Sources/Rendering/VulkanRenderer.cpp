// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <cstddef>
#include <cstring>

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
            Initialize(WindowSize size, bool sync)
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
                    true);
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

                SDL_GPUShaderCreateInfo shader{};
                shader.code_size = sizeof(kVertSpirv);
                shader.code = kVertSpirv;
                shader.entrypoint = "main";
                shader.format = SDL_GPU_SHADERFORMAT_SPIRV;
                shader.stage = SDL_GPU_SHADERSTAGE_VERTEX;
                auto *vertex = SDL_CreateGPUShader(device_, &shader);
                if (vertex == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));
                shader.code_size = sizeof(kFragSpirv);
                shader.code = kFragSpirv;
                shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
                auto *fragment = SDL_CreateGPUShader(device_, &shader);
                if (fragment == nullptr)
                {
                    const std::string error = SDL_GetError();
                    SDL_ReleaseGPUShader(device_, vertex);
                    return tl::unexpected(error);
                }

                SDL_GPUVertexBufferDescription buffer{};
                buffer.pitch = sizeof(Vertex);
                buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
                const SDL_GPUVertexAttribute attributes[]{
                    { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0 },
                    { 1,
                      0,
                      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                      offsetof(Vertex, red) },
                    { 2,
                      0,
                      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                      offsetof(Vertex, textureU) },
                };
                SDL_GPUColorTargetDescription color{};
                colorFormat_
                    = SDL_GetGPUSwapchainTextureFormat(device_, window_);
                color.format = colorFormat_;
                SDL_GPUGraphicsPipelineCreateInfo layout{};
                layout.vertex_shader = vertex;
                layout.fragment_shader = fragment;
                layout.vertex_input_state = { &buffer, 1, attributes, 3 };
                layout.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
                layout.rasterizer_state.enable_depth_clip = true;
                layout.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
                layout.depth_stencil_state.enable_depth_test = true;
                layout.depth_stencil_state.enable_depth_write = true;
                layout.target_info.color_target_descriptions = &color;
                layout.target_info.num_color_targets = 1;
                layout.target_info.depth_stencil_format = kDepthFormat;
                layout.target_info.has_depth_stencil_target = true;
                pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &layout);
                const std::string error = SDL_GetError();
                SDL_ReleaseGPUShader(device_, vertex);
                SDL_ReleaseGPUShader(device_, fragment);
                if (pipeline_ == nullptr)
                    return tl::unexpected(error);

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
            Draw(std::span<const Vertex> frame, WindowSize renderSize)
                -> tl::expected<void, std::string> override
            {
                if (frame.size_bytes() > kVertexBufferSize)
                    return tl::unexpected(
                        std::string("The Vulkan vertex buffer is full."));

                auto *mapped = SDL_MapGPUTransferBuffer(device_, upload_, true);
                if (mapped == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));
                if (!frame.empty())
                    std::memcpy(mapped, frame.data(), frame.size_bytes());
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
                const bool renderOffscreen = renderSize != windowSize;
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
                    SDL_GPUBlitInfo blit{};
                    blit.source = { colorTarget_, 0,           0, 0, 0,
                                    renderWidth,  renderHeight };
                    blit.destination
                        = { target,
                            0,
                            0,
                            static_cast<Uint32>(destination.x),
                            static_cast<Uint32>(destination.y),
                            static_cast<Uint32>(destination.size.width),
                            static_cast<Uint32>(destination.size.height) };
                    blit.load_op = SDL_GPU_LOADOP_CLEAR;
                    blit.clear_color = color.clear_color;
                    blit.filter = SDL_GPU_FILTER_LINEAR;
                    SDL_BlitGPUTexture(commands, &blit);
                }
                return Submit(commands);
            }

        private:
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
    Vulkan(WindowSize size, bool sync)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>
    {
        auto renderer = std::make_unique<VulkanRenderer>();
        if (const auto initialized = renderer->Initialize(size, sync);
            !initialized)
            return tl::unexpected(initialized.error());
        return std::unique_ptr<Renderer>(std::move(renderer));
    }
} // namespace TwentyThreeSeventeen::Rendering
