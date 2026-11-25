// SPDX-License-Identifier: MIT
#pragma once

#include <SDL3/SDL_audio.h>
#include <array>
#include <string>
#include <vector>

#include "Game/Apartment.hpp"

namespace TwentyThreeSeventeen::Game
{
    enum class Sound
    {
        kNone,
        kFootstep,
        kTelephone,
        kProjector,
    };

    class CaseAudio
    {
    public:
        explicit CaseAudio(const char *executable);
        ~CaseAudio();
        CaseAudio(const CaseAudio &) = delete;
        auto
        operator=(const CaseAudio &) -> CaseAudio & = delete;

        auto
        Cue(Sound sound, Physics::Position position) noexcept -> void;
        auto
        Update(Investigation::CaseId id,
               Physics::Position eye,
               const Camera &camera,
               float ambience,
               float effects) -> void;
        auto
        Silence() noexcept -> void;

    private:
        SDL_AudioStream *stream_{};
        std::array<std::vector<float>, 7> recording_;
        std::size_t ambienceFrame_{};
        std::size_t cueFrame_{};
        std::size_t stepIndex_{};
        Physics::Position cuePosition_{};
        Sound sound_{};
    };
} // namespace TwentyThreeSeventeen::Game
