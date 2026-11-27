// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <string_view>

namespace TwentyThreeSeventeen::Rendering
{
    enum class Backend
    {
        kAuto,
        kVulkan,
        kOpenGl,
        kMetal,
    };

#if defined(__APPLE__)
    inline constexpr auto kNativeAlternative = Backend::kMetal;
#else
    inline constexpr auto kNativeAlternative = Backend::kOpenGl;
#endif

    constexpr auto
    Name(Backend backend) -> std::string_view
    {
        switch (backend)
        {
            case Backend::kVulkan:
                return "Vulkan";
            case Backend::kOpenGl:
                return "OpenGL 4.2";
            case Backend::kMetal:
                return "Metal 3";
            case Backend::kAuto:
                return "Automatic";
        }
        return "Automatic";
    }

    constexpr auto
    Key(Backend backend) -> std::string_view
    {
        switch (backend)
        {
            case Backend::kVulkan:
                return "vulkan";
            case Backend::kOpenGl:
                return "opengl";
            case Backend::kMetal:
                return "metal";
            case Backend::kAuto:
                return "auto";
        }
        return "auto";
    }

    constexpr auto
    ParseBackend(std::string_view text) -> Backend
    {
        if (text == Key(Backend::kMetal))
            return Backend::kMetal;
        if (text == Key(Backend::kVulkan))
            return Backend::kVulkan;
        if (text == Key(Backend::kOpenGl))
            return Backend::kOpenGl;
        return Backend::kAuto;
    }

    constexpr auto
    StartupOrder(Backend preferred) -> std::array<Backend, 2>
    {
        if (preferred == kNativeAlternative)
            return { kNativeAlternative, Backend::kVulkan };
        return { Backend::kVulkan, kNativeAlternative };
    }
} // namespace TwentyThreeSeventeen::Rendering
