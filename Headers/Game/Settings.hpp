// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <string>
#include <string_view>

#include "Game/CasePanel.hpp"
#include "Rendering/Renderer.hpp"

namespace TwentyThreeSeventeen::Game
{
    enum class Preset
    {
        kLow,
        kMedium,
        kHigh,
        kXhigh,
        kMax,
        kCustom
    };

    struct SceneQuality
    {
        int details;
        int lighting;

        auto
        operator==(const SceneQuality &) const -> bool = default;
    };

    inline constexpr std::array<Rendering::WindowSize, 5> kResolutions{ {
        { 960, 540 },
        { 1280, 720 },
        { 1366, 768 },
        { 1920, 1080 },
        { 2560, 1440 },
    } };

    inline constexpr std::array<float, 5> kSensitivities{
        0.0012f, 0.0018f, 0.0024f, 0.0030f, 0.0036f,
    };

    struct Settings
    {
        Rendering::Backend backend{ Rendering::Backend::kAuto };
        int resolution{ 1 };
        bool fullscreen{};
        bool sync{ true };
        Preset preset{ Preset::kHigh };
        SceneQuality custom{ 2, 2 };
        Language language{ Language::kEnglish };
        int sensitivity{ 2 };
        bool invertMouse{};
        int ambienceVolume{ 4 };
        int effectsVolume{ 4 };
        bool captions{ true };

        auto
        operator==(const Settings &) const -> bool = default;
    };

    [[nodiscard]] auto
    Quality(const Settings &settings) noexcept -> SceneQuality;

    [[nodiscard]] auto
    LoadSettings(std::string_view line, Language fallback) noexcept -> Settings;

    [[nodiscard]] auto
    SerializeSettings(const Settings &settings) -> std::string;
} // namespace TwentyThreeSeventeen::Game
