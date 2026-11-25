// SPDX-License-Identifier: MIT
#include <Progmasoft/Catch3/TestMacros.hpp>

#include "Game/Settings.hpp"

namespace Game = TwentyThreeSeventeen::Game;
namespace Rendering = TwentyThreeSeventeen::Rendering;

TEST_CASE("Desktop settings retain every choice when reloaded")
{
    Game::Settings selected;
    selected.backend = Rendering::Backend::kOpenGl;
    selected.resolution = 2;
    selected.fullscreen = true;
    selected.sync = false;
    selected.preset = Game::Preset::kCustom;
    selected.custom = { 1, 4 };
    selected.language = Game::Language::kFrench;
    selected.sensitivity = 4;
    selected.invertMouse = true;
    selected.ambienceVolume = 1;
    selected.effectsVolume = 0;
    selected.captions = false;
    const auto restored = Game::LoadSettings(Game::SerializeSettings(selected),
                                             Game::Language::kGerman);
    REQUIRE(restored == selected);
    REQUIRE(Game::Quality(restored) == Game::SceneQuality{ 1, 4 });
    REQUIRE(Game::kResolutions[2] == Rendering::WindowSize{ 1366, 768 });
}

TEST_CASE("Older settings and invalid fields load safe defaults")
{
    const auto old = Game::LoadSettings("opengl", Game::Language::kGerman);
    REQUIRE(old.backend == Rendering::Backend::kOpenGl);
    REQUIRE(old.language == Game::Language::kGerman);
    const auto malformed
        = Game::LoadSettings("opengl 500 0 1 2 2 2 1 2 0", old.language);
    REQUIRE(malformed == old);
    const auto legacy
        = Game::LoadSettings("opengl 2 1 0 2 2 2 1 4 1", old.language);
    REQUIRE(legacy.resolution == 2);
    REQUIRE(legacy.fullscreen);
    REQUIRE(legacy.ambienceVolume == 4);
    REQUIRE(legacy.effectsVolume == 4);
    REQUIRE(legacy.captions);
    REQUIRE(Game::Quality(old) == Game::SceneQuality{ 2, 2 });
}
