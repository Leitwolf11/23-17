// SPDX-License-Identifier: MIT
#include <nanobench.h>

#include "Investigation/Case.hpp"

auto
main() -> int
{
    using namespace TwentyThreeSeventeen::Investigation;
    const auto subject = Subject::kDispatch;
    ankerl::nanobench::Bench().run("Record an observation", [&] {
        Case investigation;
        investigation.Observe(subject);
        ankerl::nanobench::doNotOptimizeAway(
            investigation.HasObserved(subject));
    });
}
