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
    };

    constexpr auto
    Name(Backend backend) -> std::string_view
    {
        switch (backend)
        {
            case Backend::kVulkan:
                return "Vulkan";
            case Backend::kOpenGl:
                return "OpenGL 4.2";
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
            case Backend::kAuto:
                return "auto";
        }
        return "auto";
    }

    constexpr auto
    ParseBackend(std::string_view text) -> Backend
    {
        if (text == Key(Backend::kVulkan))
            return Backend::kVulkan;
        if (text == Key(Backend::kOpenGl))
            return Backend::kOpenGl;
        return Backend::kAuto;
    }

    constexpr auto
    StartupOrder(Backend preferred) -> std::array<Backend, 2>
    {
        if (preferred == Backend::kOpenGl)
            return { Backend::kOpenGl, Backend::kVulkan };
        return { Backend::kVulkan, Backend::kOpenGl };
    }
} // namespace TwentyThreeSeventeen::Rendering
