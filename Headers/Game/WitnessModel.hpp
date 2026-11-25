// SPDX-License-Identifier: MIT
#pragma once

#include "Game/Apartment.hpp"

namespace TwentyThreeSeventeen::Game
{
    [[nodiscard]] auto
    CreateWitness(Physics::Position feet, bool electrician) -> Actor;
} // namespace TwentyThreeSeventeen::Game
