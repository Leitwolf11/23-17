// SPDX-License-Identifier: MIT
#pragma once

#include "Game/Settings.hpp"

namespace TwentyThreeSeventeen::Game
{
    enum class MenuAction
    {
        kContinue,
        kSettings,
        kCaseList,
        kQuit
    };

    [[nodiscard]] auto
    StartMenu(Dialog &dialog, Language language) -> MenuAction;

    // Return true only after the player applies the draft.
    [[nodiscard]] auto
    EditSettings(Dialog &dialog,
                 Settings &settings,
                 Rendering::Backend active,
                 Rendering::WindowSize actualSize) -> bool;

    auto
    ShowSettingsError(Dialog &dialog, Language language, std::string_view error)
        -> void;
} // namespace TwentyThreeSeventeen::Game
