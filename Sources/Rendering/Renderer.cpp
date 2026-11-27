// SPDX-License-Identifier: MIT
#include "Rendering/MetalBridge.hpp"
#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    auto
    OpenGl(WindowSize size, bool sync)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>;
    auto
    Gpu(Backend backend, WindowSize size, bool sync, bool requireHardware)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>;

    auto
    Open(Backend backend, WindowSize size, bool sync, bool requireHardware)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>
    {
        switch (backend)
        {
            case Backend::kVulkan:
                return Gpu(backend, size, sync, requireHardware);
            case Backend::kMetal:
                if (!HasMetal3Device())
                    return tl::unexpected(std::string(
                        "A Metal 3 GPU on macOS 13 or later is required."));
                return Gpu(backend, size, sync, true);
            case Backend::kOpenGl:
#if defined(__APPLE__)
                return tl::unexpected(
                    std::string("OpenGL 4.2 Core is unavailable on macOS."));
#else
                return OpenGl(size, sync);
#endif
            case Backend::kAuto:
                break;
        }
        return tl::unexpected(std::string("Specify a concrete renderer."));
    }
} // namespace TwentyThreeSeventeen::Rendering
