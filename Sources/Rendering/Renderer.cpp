// SPDX-License-Identifier: MIT
#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    auto
    OpenGl(WindowSize size, bool sync)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>;
    auto
    Vulkan(WindowSize size, bool sync, bool requireHardware)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>;

    auto
    Open(Backend backend, WindowSize size, bool sync, bool requireHardware)
        -> tl::expected<std::unique_ptr<Renderer>, std::string>
    {
        switch (backend)
        {
            case Backend::kVulkan:
                return Vulkan(size, sync, requireHardware);
            case Backend::kOpenGl:
                return OpenGl(size, sync);
            case Backend::kAuto:
                break;
        }
        return tl::unexpected(std::string("Specify a concrete renderer."));
    }
} // namespace TwentyThreeSeventeen::Rendering
