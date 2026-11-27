// SPDX-License-Identifier: MIT
#include <Progmasoft/Catch3/TestMacros.hpp>

#include "Rendering/Renderer.hpp"

TEST_CASE("Renderer preferences define the startup order")
{
    using namespace TwentyThreeSeventeen::Rendering;
    REQUIRE(StartupOrder(ParseBackend("auto"))
            == std::array{ Backend::kVulkan, kNativeAlternative });
    REQUIRE(StartupOrder(ParseBackend("vulkan"))
            == std::array{ Backend::kVulkan, kNativeAlternative });
    REQUIRE(StartupOrder(kNativeAlternative)
            == std::array{ kNativeAlternative, Backend::kVulkan });
    REQUIRE(ParseBackend("metal") == Backend::kMetal);
    REQUIRE(Key(Backend::kMetal) == "metal");
    REQUIRE(Name(Backend::kMetal) == "Metal 3");
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
