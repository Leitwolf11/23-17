// SPDX-License-Identifier: MIT
#include <charconv>

#include "Game/Settings.hpp"

namespace TwentyThreeSeventeen::Game
{
    auto
    Quality(const Settings &settings) noexcept -> SceneQuality
    {
        switch (settings.preset)
        {
            case Preset::kLow:
                return { 0, 0 };
            case Preset::kMedium:
                return { 1, 1 };
            case Preset::kHigh:
                return { 2, 2 };
            case Preset::kXhigh:
                return { 3, 3 };
            case Preset::kMax:
                return { 4, 4 };
            case Preset::kCustom:
                return settings.custom;
        }
        return {};
    }

    auto
    LoadSettings(std::string_view line, Language fallback) noexcept -> Settings
    {
        Settings settings;
        settings.language = fallback;
        const auto delimiter = line.find(' ');
        const auto mode = line.substr(0, delimiter);
        settings.backend = Rendering::ParseBackend(mode);
        if (delimiter == std::string_view::npos)
            return settings;
        if (mode != Rendering::Key(settings.backend))
            return settings;
        line.remove_prefix(delimiter + 1);

        constexpr std::array maximum{ 4, 1, 1, 5, 4, 4, 2, 4, 1, 5, 5, 1 };
        std::array<int, maximum.size()> fields{ 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 4, 4, 1 };
        for (std::size_t index = 0; index < fields.size(); ++index)
        {
            const auto end = line.find(' ');
            const auto value = line.substr(0, end);
            const auto parsed = std::from_chars(value.data(),
                                                value.data() + value.size(),
                                                fields[index]);
            const bool isLast = index + 1 == fields.size();
            const bool ends = end == std::string_view::npos;
            if (parsed.ec != std::errc()
                || parsed.ptr != value.data() + value.size()
                || fields[index] < 0 || fields[index] > maximum[index]
                || (isLast && !ends) || (ends && index != 8 && !isLast))
                return settings;
            if (ends)
                break;
            line.remove_prefix(end + 1);
        }
        settings.resolution = fields[0];
        settings.fullscreen = fields[1] != 0;
        settings.sync = fields[2] != 0;
        settings.preset = static_cast<Preset>(fields[3]);
        settings.custom = { fields[4], fields[5] };
        settings.language = static_cast<Language>(fields[6]);
        settings.sensitivity = fields[7];
        settings.invertMouse = fields[8] != 0;
        settings.ambienceVolume = fields[9];
        settings.effectsVolume = fields[10];
        settings.captions = fields[11] != 0;
        return settings;
    }

    auto
    SerializeSettings(const Settings &settings) -> std::string
    {
        std::string line(Rendering::Key(settings.backend));
        for (const int field : { settings.resolution,
                                 static_cast<int>(settings.fullscreen),
                                 static_cast<int>(settings.sync),
                                 static_cast<int>(settings.preset),
                                 settings.custom.details,
                                 settings.custom.lighting,
                                 static_cast<int>(settings.language),
                                 settings.sensitivity,
                                 static_cast<int>(settings.invertMouse),
                                 settings.ambienceVolume,
                                 settings.effectsVolume,
                                 static_cast<int>(settings.captions) })
        {
            line += ' ';
            line += std::to_string(field);
        }
        return line;
    }
} // namespace TwentyThreeSeventeen::Game
