// SPDX-License-Identifier: MIT
#pragma once

#include "Game/Apartment.hpp"

namespace TwentyThreeSeventeen::Game
{
    auto
    AddCaseProps(Apartment &scene,
                 Investigation::CaseId id,
                 const char *executable) -> void;
} // namespace TwentyThreeSeventeen::Game
