// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>

#include "Rendering/Renderer.hpp"

TEST_CASE("Renderer preferences define the startup order")
{
    using namespace TwentyThreeSeventeen::Rendering;
    REQUIRE(StartupOrder(ParseBackend("auto"))
            == std::array{ Backend::kVulkan, Backend::kOpenGl });
    REQUIRE(StartupOrder(ParseBackend("vulkan"))
            == std::array{ Backend::kVulkan, Backend::kOpenGl });
    REQUIRE(StartupOrder(ParseBackend("opengl"))
            == std::array{ Backend::kOpenGl, Backend::kVulkan });
    REQUIRE(ParseBackend("unknown") == Backend::kAuto);
    REQUIRE(ParseBackend(Key(Backend::kOpenGl)) == Backend::kOpenGl);
}

TEST_CASE("The render target fits the window without stretching")
{
    using namespace TwentyThreeSeventeen::Rendering;
    REQUIRE(FitViewport({ 1280, 720 }, { 1280, 720 })
            == Viewport{ 0, 0, { 1280, 720 } });
    REQUIRE(FitViewport({ 1280, 720 }, { 732, 712 })
            == Viewport{ 0, 150, { 732, 411 } });
    REQUIRE(FitViewport({ 960, 540 }, { 1600, 720 })
            == Viewport{ 160, 0, { 1280, 720 } });
    REQUIRE(FitViewport({ 1280, 720 }, { 1, 1 }) == Viewport{ 0, 0, { 1, 1 } });
}
