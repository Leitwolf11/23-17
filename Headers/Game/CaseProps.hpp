// SPDX-License-Identifier: MIT
#pragma once

#include "Game/Apartment.hpp"

namespace TwentyThreeSeventeen::Game
{
    [[nodiscard]] auto
    AddTexturedCaseProps(Rendering::WorldMesh &mesh,
                         Investigation::CaseId id,
                         const char *executable)
        -> tl::expected<void, std::string>;

    auto
    AddCaseProps(Apartment &scene,
                 Investigation::CaseId id,
                 const char *executable) -> void;
} // namespace TwentyThreeSeventeen::Game
