// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numbers>
#include <string_view>

#include "Game/CaseAudio.hpp"
#include "rules_cc/cc/runfiles/runfiles.h"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        constexpr int kSampleRate = 24000;
        constexpr unsigned kBlock = 512;
        constexpr float kTau = 2.0f * std::numbers::pi_v<float>;

        auto
        LoadRecording(const std::string &path) -> std::vector<float>
        {
            SDL_AudioSpec original{};
            Uint8 *raw = nullptr;
            Uint32 bytes = 0;
            if (path.empty()
                || !SDL_LoadWAV(path.c_str(), &original, &raw, &bytes))
                return {};
            const std::unique_ptr<Uint8, decltype(&SDL_free)> data(raw,
                                                                   SDL_free);
            const SDL_AudioSpec mono{ SDL_AUDIO_F32, 1, kSampleRate };
            Uint8 *converted = nullptr;
            int length = 0;
            if (!SDL_ConvertAudioSamples(&original,
                                         data.get(),
                                         static_cast<int>(bytes),
                                         &mono,
                                         &converted,
                                         &length))
                return {};
            const std::unique_ptr<Uint8, decltype(&SDL_free)> output(converted,
                                                                     SDL_free);
            std::vector<float> samples(static_cast<std::size_t>(length)
                                       / sizeof(float));
            SDL_memcpy(samples.data(),
                       output.get(),
                       static_cast<std::size_t>(length));
            return samples;
        }

        auto
        Loop(const std::vector<float> &samples, std::size_t index) noexcept
            -> float
        {
            if (samples.empty())
                return 0;
            const auto fade = std::min<std::size_t>(samples.size() / 4, 1800);
            const auto position = index % (samples.size() - fade);
            if (position >= fade || fade == 0)
                return samples[position];
            const auto mix
                = static_cast<float>(position) / static_cast<float>(fade);
            return samples[position] * mix
                   + samples[samples.size() - fade + position] * (1.0f - mix);
        }
    } // namespace

    CaseAudio::CaseAudio(const char *executable)
    {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "%s", SDL_GetError());
            return;
        }
        const SDL_AudioSpec stereo{ SDL_AUDIO_F32, 2, kSampleRate };
        stream_ = SDL_OpenAudioDeviceStream(
            std::numeric_limits<SDL_AudioDeviceID>::max(),
            &stereo,
            nullptr,
            nullptr);
        if (stream_ == nullptr)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "%s", SDL_GetError());
            return;
        }
        using Runfiles = rules_cc::cc::runfiles::Runfiles;
        const std::unique_ptr<Runfiles> runfiles(Runfiles::Create(executable));
        const auto *base = SDL_GetBasePath();
        constexpr std::array names{ "entrance-rain.wav", "fluorescent-hum.wav",
                                    "service-vent.wav",  "case-score.wav",
                                    "stone-step-1.wav",  "stone-step-2.wav",
                                    "stone-step-3.wav" };
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            if (runfiles)
                recording_[index] = LoadRecording(runfiles->Rlocation(
                    std::string("_main/LegacyDemo/assets/audio/")
                    + names[index]));
            if (recording_[index].empty() && base != nullptr)
                recording_[index] = LoadRecording(std::string(base) + "audio/"
                                                  + names[index]);
            if (recording_[index].empty())
                SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                            "Could not load %s.",
                            names[index]);
        }
        SDL_ResumeAudioStreamDevice(stream_);
    }

    CaseAudio::~CaseAudio()
    {
        SDL_DestroyAudioStream(stream_);
    }

    auto
    CaseAudio::Cue(Sound sound, Physics::Position position) noexcept -> void
    {
        cuePosition_ = position;
        cueFrame_ = 0;
        sound_ = sound;
        if (sound == Sound::kFootstep)
            stepIndex_ = (stepIndex_ + 1) % 3;
    }

    auto
    CaseAudio::Silence() noexcept -> void
    {
        sound_ = Sound::kNone;
        if (stream_)
            SDL_ClearAudioStream(stream_);
    }

    auto
    CaseAudio::Update(Investigation::CaseId id,
                      Physics::Position eye,
                      const Camera &camera,
                      float ambience,
                      float effects) -> void
    {
        constexpr auto kBytes = static_cast<int>(2 * kBlock * sizeof(float));
        if (stream_ == nullptr)
            return;
        const int queued = SDL_GetAudioStreamQueued(stream_);
        if (queued < 0 || queued >= kBytes * 2)
            return;

        const auto dx = cuePosition_.x - eye.x;
        const auto dz = cuePosition_.z - eye.z;
        const auto distance = std::hypot(dx, dz);
        const auto pan = std::clamp((dx * camera.right.x + dz * camera.right.z)
                                        / std::max(1.0f, distance),
                                    -0.86f,
                                    0.86f);
        const auto level = effects / (1.0f + 0.29f * distance);
        const auto rain
            = id == Investigation::CaseId::kApartment
                  ? std::clamp((eye.z + 2.3f) * 0.11f, 0.035f, 0.65f)
                  : 0.0f;
        const auto bed = id == Investigation::CaseId::kArchive ? 1u : 2u;
        for (int offset = queued; offset < kBytes * 2; offset += kBytes)
        {
            std::array<float, kBlock * 2> samples{};
            for (std::size_t frame = 0; frame < kBlock;
                 ++frame, ++ambienceFrame_)
            {
                const auto base
                    = (rain * Loop(recording_[0], ambienceFrame_)
                       + 0.13f * Loop(recording_[bed], ambienceFrame_)
                       + 0.095f * Loop(recording_[3], ambienceFrame_))
                      * ambience;
                float cue = 0;
                const auto t = static_cast<float>(cueFrame_)
                               / static_cast<float>(kSampleRate);
                switch (sound_)
                {
                    case Sound::kNone:
                        break;
                    case Sound::kFootstep:
                    {
                        const auto &sample = recording_[4 + stepIndex_];
                        if (cueFrame_ < sample.size())
                            cue = sample[cueFrame_] * 0.64f;
                        else
                            sound_ = Sound::kNone;
                        break;
                    }
                    case Sound::kTelephone:
                    {
                        const auto pulse = std::fmod(t, 0.54f);
                        if (t > 1.25f)
                            sound_ = Sound::kNone;
                        else if (pulse < 0.29f)
                            cue = (std::sin(kTau * 438.0f * t)
                                   + std::sin(kTau * 532.0f * t))
                                  * 0.12f * std::min(1.0f, pulse * 70.0f)
                                  * std::min(1.0f, (0.29f - pulse) * 35.0f);
                        break;
                    }
                    case Sound::kProjector:
                        if (t > 3.6f)
                            sound_ = Sound::kNone;
                        else
                        {
                            const auto pulse = std::fmod(24.0f * t, 1.0f);
                            cue = (0.085f * std::sin(kTau * 61.0f * t)
                                   + 0.19f * pulse * pulse * pulse
                                         * std::sin(kTau * 930.0f * t))
                                  * std::min(1.0f, t * 8.0f)
                                  * std::min(1.0f, (3.6f - t) * 4.0f);
                        }
                        break;
                }
                if (sound_ != Sound::kNone)
                    ++cueFrame_;
                samples[frame * 2]
                    = std::clamp(base + cue * level * (0.65f - 0.35f * pan),
                                 -1.0f,
                                 1.0f);
                samples[frame * 2 + 1]
                    = std::clamp(base + cue * level * (0.65f + 0.35f * pan),
                                 -1.0f,
                                 1.0f);
            }
            if (!SDL_PutAudioStreamData(stream_, samples.data(), kBytes))
                return;
        }
    }
} // namespace TwentyThreeSeventeen::Game
