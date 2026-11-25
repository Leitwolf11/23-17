// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    namespace
    {
#include "SceneOpenGl.inc"

        class OpenGlRenderer final: public Renderer
        {
        public:
            ~OpenGlRenderer() override
            {
                if (context_ != nullptr)
                    SDL_GL_DestroyContext(context_);
                SDL_DestroyWindow(window_);
            }

            auto
            Initialize(WindowSize size, bool sync)
                -> tl::expected<void, std::string>
            {
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                    SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
                SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
                const auto createWindow = [&] {
                    return SDL_CreateWindow("23.17",
                                            size.width,
                                            size.height,
                                            SDL_WINDOW_OPENGL
                                                | SDL_WINDOW_RESIZABLE
                                                | SDL_WINDOW_HIDDEN);
                };
                window_ = createWindow();
                const auto *driver = SDL_GetCurrentVideoDriver();
                if (window_ == nullptr && driver != nullptr
                    && std::string_view(driver) == "x11"
                    && SDL_GetHint(SDL_HINT_VIDEO_FORCE_EGL) == nullptr)
                {
                    // The Linux SDL build can provide EGL without GLX.
                    SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "1");
                    window_ = createWindow();
                }
                if (window_ == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));

                context_ = SDL_GL_CreateContext(window_);
                if (context_ == nullptr)
                    return tl::unexpected(std::string(SDL_GetError()));

                // These entry points are required by a 4.2 core context.
#define GAME_GL_LOAD(member, name)                                             \
    member = reinterpret_cast<decltype(member)>(SDL_GL_GetProcAddress(name));  \
    if (member == nullptr)                                                     \
    return tl::unexpected(std::string("Missing OpenGL entry point: ") + name)
                GAME_GL_LOAD(genVertexArrays_, "glGenVertexArrays");
                GAME_GL_LOAD(bindVertexArray_, "glBindVertexArray");
                GAME_GL_LOAD(genBuffers_, "glGenBuffers");
                GAME_GL_LOAD(bindBuffer_, "glBindBuffer");
                GAME_GL_LOAD(bufferData_, "glBufferData");
                GAME_GL_LOAD(vertexAttribPointer_, "glVertexAttribPointer");
                GAME_GL_LOAD(enableVertexAttribArray_,
                             "glEnableVertexAttribArray");
                GAME_GL_LOAD(createShader_, "glCreateShader");
                GAME_GL_LOAD(shaderSource_, "glShaderSource");
                GAME_GL_LOAD(compileShader_, "glCompileShader");
                GAME_GL_LOAD(getShaderiv_, "glGetShaderiv");
                GAME_GL_LOAD(getShaderInfoLog_, "glGetShaderInfoLog");
                GAME_GL_LOAD(deleteShader_, "glDeleteShader");
                GAME_GL_LOAD(createProgram_, "glCreateProgram");
                GAME_GL_LOAD(attachShader_, "glAttachShader");
                GAME_GL_LOAD(linkProgram_, "glLinkProgram");
                GAME_GL_LOAD(getProgramiv_, "glGetProgramiv");
                GAME_GL_LOAD(getProgramInfoLog_, "glGetProgramInfoLog");
                GAME_GL_LOAD(useProgram_, "glUseProgram");
                GAME_GL_LOAD(clear_, "glClear");
                GAME_GL_LOAD(clearColor_, "glClearColor");
                GAME_GL_LOAD(viewport_, "glViewport");
                GAME_GL_LOAD(enable_, "glEnable");
                GAME_GL_LOAD(drawArrays_, "glDrawArrays");
                GAME_GL_LOAD(getString_, "glGetString");
                GAME_GL_LOAD(genFramebuffers_, "glGenFramebuffers");
                GAME_GL_LOAD(bindFramebuffer_, "glBindFramebuffer");
                GAME_GL_LOAD(genRenderbuffers_, "glGenRenderbuffers");
                GAME_GL_LOAD(bindRenderbuffer_, "glBindRenderbuffer");
                GAME_GL_LOAD(renderbufferStorage_, "glRenderbufferStorage");
                GAME_GL_LOAD(framebufferRenderbuffer_,
                             "glFramebufferRenderbuffer");
                GAME_GL_LOAD(framebufferStatus_, "glCheckFramebufferStatus");
                GAME_GL_LOAD(blitFramebuffer_, "glBlitFramebuffer");
#undef GAME_GL_LOAD

                const auto vertex = Compile(GL_VERTEX_SHADER, kVertSource);
                if (!vertex)
                    return tl::unexpected(vertex.error());
                const auto fragment = Compile(GL_FRAGMENT_SHADER, kFragSource);
                if (!fragment)
                {
                    deleteShader_(*vertex);
                    return tl::unexpected(fragment.error());
                }

                program_ = createProgram_();
                attachShader_(program_, *vertex);
                attachShader_(program_, *fragment);
                linkProgram_(program_);
                deleteShader_(*vertex);
                deleteShader_(*fragment);

                GLint linked = 0;
                getProgramiv_(program_, GL_LINK_STATUS, &linked);
                if (linked == 0)
                {
                    char message[1024]{};
                    getProgramInfoLog_(program_,
                                       sizeof(message),
                                       nullptr,
                                       message);
                    return tl::unexpected(std::string(message));
                }

                GLuint array = 0;
                genVertexArrays_(1, &array);
                bindVertexArray_(array);
                genBuffers_(1, &buffer_);
                bindBuffer_(GL_ARRAY_BUFFER, buffer_);
                vertexAttribPointer_(0,
                                     4,
                                     GL_FLOAT,
                                     GL_FALSE,
                                     sizeof(Vertex),
                                     nullptr);
                vertexAttribPointer_(
                    1,
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    sizeof(Vertex),
                    reinterpret_cast<const void *>(offsetof(Vertex, red)));
                vertexAttribPointer_(
                    2,
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    sizeof(Vertex),
                    reinterpret_cast<const void *>(offsetof(Vertex, textureU)));
                enableVertexAttribArray_(2);
                enableVertexAttribArray_(0);
                enableVertexAttribArray_(1);
                useProgram_(program_);
                enable_(GL_DEPTH_TEST);
                clearColor_(0.035f, 0.044f, 0.058f, 1.0f);
                const int interval = sync ? 1 : 0;
                if (!SDL_GL_SetSwapInterval(interval))
                {
                    int active = -1;
                    if (!SDL_GL_GetSwapInterval(&active) || active != interval)
                        return tl::unexpected(
                            std::string("OpenGL could not set VSync: ")
                            + SDL_GetError());
                }
                SDL_Log("OpenGL renderer: %s (%s)",
                        reinterpret_cast<const char *>(getString_(GL_RENDERER)),
                        reinterpret_cast<const char *>(getString_(GL_VERSION)));
                return {};
            }

            auto
            Window() const noexcept -> SDL_Window * override
            {
                return window_;
            }

            auto
            ActiveBackend() const noexcept -> Backend override
            {
                return Backend::kOpenGl;
            }

            auto
            Draw(std::span<const Vertex> vertices, WindowSize renderSize)
                -> tl::expected<void, std::string> override
            {
                if (!SDL_GL_MakeCurrent(window_, context_))
                    return tl::unexpected(std::string(SDL_GetError()));
                int width = 0;
                int height = 0;
                SDL_GetWindowSizeInPixels(window_, &width, &height);
                const WindowSize windowSize{ width, height };
                if (width <= 0 || height <= 0)
                    return {};
                if (renderSize.width <= 0 || renderSize.height <= 0)
                    renderSize = windowSize;
                const bool renderOffscreen = renderSize != windowSize;
                if (renderOffscreen)
                {
                    if (!BindFrameTarget(renderSize))
                        return tl::unexpected(std::string(
                            "Could not create the OpenGL render target."));
                }
                else
                    bindFramebuffer_(GL_FRAMEBUFFER, 0);
                viewport_(0, 0, renderSize.width, renderSize.height);
                clear_(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                bufferData_(GL_ARRAY_BUFFER,
                            static_cast<GLsizeiptr>(vertices.size_bytes()),
                            vertices.data(),
                            GL_STREAM_DRAW);
                drawArrays_(GL_TRIANGLES,
                            0,
                            static_cast<GLsizei>(vertices.size()));
                if (renderOffscreen)
                {
                    bindFramebuffer_(GL_DRAW_FRAMEBUFFER, 0);
                    clear_(GL_COLOR_BUFFER_BIT);
                    const auto destination
                        = FitViewport(renderSize, windowSize);
                    const auto bottom
                        = height - destination.y - destination.size.height;
                    blitFramebuffer_(0,
                                     0,
                                     renderSize.width,
                                     renderSize.height,
                                     destination.x,
                                     bottom,
                                     destination.x + destination.size.width,
                                     bottom + destination.size.height,
                                     GL_COLOR_BUFFER_BIT,
                                     GL_LINEAR);
                    bindFramebuffer_(GL_READ_FRAMEBUFFER, 0);
                }
                if (!SDL_GL_SwapWindow(window_))
                    return tl::unexpected(std::string(SDL_GetError()));
                return {};
            }

        private:
            auto
            BindFrameTarget(WindowSize size) -> bool
            {
                if (target_ == 0)
                {
                    genFramebuffers_(1, &target_);
                    genRenderbuffers_(static_cast<GLsizei>(attachments_.size()),
                                      attachments_.data());
                    if (target_ == 0)
                        return false;
                }
                bindFramebuffer_(GL_FRAMEBUFFER, target_);
                if (size == targetSize_)
                    return true;
                targetSize_ = {};
                constexpr std::array<GLenum, 2> formats{ GL_RGBA8,
                                                         GL_DEPTH_COMPONENT24 };
                constexpr std::array<GLenum, 2> slots{ GL_COLOR_ATTACHMENT0,
                                                       GL_DEPTH_ATTACHMENT };
                for (std::size_t index = 0; index < attachments_.size();
                     ++index)
                {
                    bindRenderbuffer_(GL_RENDERBUFFER, attachments_[index]);
                    renderbufferStorage_(GL_RENDERBUFFER,
                                         formats[index],
                                         size.width,
                                         size.height);
                    framebufferRenderbuffer_(GL_FRAMEBUFFER,
                                             slots[index],
                                             GL_RENDERBUFFER,
                                             attachments_[index]);
                }
                if (framebufferStatus_(GL_FRAMEBUFFER)
                    != GL_FRAMEBUFFER_COMPLETE)
                    return false;
                targetSize_ = size;
                return true;
            }

            auto
            Compile(GLenum type, const char *source)
                -> tl::expected<GLuint, std::string>
            {
                const auto shader = createShader_(type);
                shaderSource_(shader, 1, &source, nullptr);
                compileShader_(shader);
                GLint compiled = 0;
                getShaderiv_(shader, GL_COMPILE_STATUS, &compiled);
                if (compiled == 0)
                {
                    char message[1024]{};
                    getShaderInfoLog_(shader,
                                      sizeof(message),
                                      nullptr,
                                      message);
                    deleteShader_(shader);
                    return tl::unexpected(std::string(message));
                }
                return shader;
            }

            SDL_Window *window_{};
            SDL_GLContext context_{};
            GLuint buffer_{};
            GLuint program_{};
            GLuint target_{};
            std::array<GLuint, 2> attachments_{};
            WindowSize targetSize_{};
            PFNGLGENFRAMEBUFFERSPROC genFramebuffers_{};
            PFNGLBINDFRAMEBUFFERPROC bindFramebuffer_{};
            PFNGLGENRENDERBUFFERSPROC genRenderbuffers_{};
            PFNGLBINDRENDERBUFFERPROC bindRenderbuffer_{};
            PFNGLRENDERBUFFERSTORAGEPROC renderbufferStorage_{};
            PFNGLFRAMEBUFFERRENDERBUFFERPROC framebufferRenderbuffer_{};
            PFNGLCHECKFRAMEBUFFERSTATUSPROC framebufferStatus_{};
            PFNGLBLITFRAMEBUFFERPROC blitFramebuffer_{};
            PFNGLGENVERTEXARRAYSPROC genVertexArrays_{};
            PFNGLBINDVERTEXARRAYPROC bindVertexArray_{};
            PFNGLGENBUFFERSPROC genBuffers_{};
            PFNGLBINDBUFFERPROC bindBuffer_{};
            PFNGLBUFFERDATAPROC bufferData_{};
            PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer_{};
            PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray_{};
            PFNGLCREATESHADERPROC createShader_{};
            PFNGLSHADERSOURCEPROC shaderSource_{};
            PFNGLCOMPILESHADERPROC compileShader_{};
            PFNGLGETSHADERIVPROC getShaderiv_{};
            PFNGLGETSHADERINFOLOGPROC getShaderInfoLog_{};
            PFNGLDELETESHADERPROC deleteShader_{};
            PFNGLCREATEPROGRAMPROC createProgram_{};
            PFNGLATTACHSHADERPROC attachShader_{};
            PFNGLLINKPROGRAMPROC linkProgram_{};
            PFNGLGETPROGRAMIVPROC getProgramiv_{};
            PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog_{};
            PFNGLUSEPROGRAMPROC useProgram_{};
            decltype(&glClear) clear_{};
            decltype(&glClearColor) clearColor_{};
            decltype(&glViewport) viewport_{};
            decltype(&glEnable) enable_{};
            decltype(&glDrawArrays) drawArrays_{};
            decltype(&glGetString) getString_{};
        };
    } // namespace

    auto
    OpenGl(WindowSize size, bool sync)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>
    {
        auto renderer = std::make_unique<OpenGlRenderer>();
        if (const auto initialized = renderer->Initialize(size, sync);
            !initialized)
            return tl::unexpected(initialized.error());
        return std::unique_ptr<Renderer>(std::move(renderer));
    }
} // namespace TwentyThreeSeventeen::Rendering
