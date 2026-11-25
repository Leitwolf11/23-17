// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>

#include "Investigation/Case.hpp"

namespace TwentyThreeSeventeen::Game
{
    class Dialog;
    enum class Language : std::size_t
    {
        kEnglish,
        kGerman,
        kFrench,
    };

    class CasePanel final
    {
    public:
        explicit CasePanel(Language language,
                           Investigation::CaseId id
                           = Investigation::CaseId::kApartment) noexcept;

        [[nodiscard]] auto
        Title() const noexcept -> const char *;
        [[nodiscard]] auto
        Brief() const noexcept -> const char *;

        [[nodiscard]] auto
        SourceName(Investigation::Subject subject) const noexcept -> const
            char *;

        auto
        Observe(Dialog &dialog,
                Investigation::Case &investigation,
                Investigation::Subject source) const -> void;

        auto
        Journal(Dialog &dialog, Investigation::Case &investigation) const
            -> void;

        auto
        FileReport(Dialog &dialog, Investigation::Case &investigation) const
            -> void;

    private:
        Language language_;
        Investigation::CaseId id_;
    };
} // namespace TwentyThreeSeventeen::Game
