// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

namespace TwentyThreeSeventeen::Investigation
{
    enum class CaseId : std::uint8_t
    {
        kApartment,
        kArchive,
        kCinema,
    };

    enum class Subject : std::uint8_t
    {
        kNone,
        kDispatch,
        kCaretaker,
        kCamera,
        kBreaker,
        kDoorLog,
        kElectrician,
        kRecorder,
        kRevision,
        kInventory,
        kArchivist,
        kTransfer,
        kShelf,
        kDuty,
        kIntake,
        kReceiver,
        kSchedule,
        kUsher,
        kScreening,
        kDiary,
        kExit,
        kLeader,
        kReceipt,
    };

    enum class Claim : std::uint8_t
    {
        kFirst,
        kSecond,
    };

    enum class ChallengeResult
    {
        kMissingSource,
        kUnrelated,
        kClarified,
    };

    enum class ReportFinding
    {
        kIncomplete,
        kUnsupported,
        kSupported,
    };

    struct ClaimRule
    {
        Subject witness;
        Subject record;
    };

    struct CaseDefinition
    {
        std::string_view key;
        std::array<Subject, 8> sources;
        std::array<Subject, 2> comparison;
        std::array<ClaimRule, 2> claims;
        std::array<Subject, 4> timeline;
        std::array<std::uint8_t, 3> supportedAnswers;
    };

    [[nodiscard]] auto
    Definition(CaseId id) noexcept -> const CaseDefinition &;

    class Case final
    {
    public:
        Case() noexcept = default;
        explicit Case(CaseId id) noexcept;

        [[nodiscard]] auto
        Id() const noexcept -> CaseId;
        [[nodiscard]] auto
        HasProgress() const noexcept -> bool;
        [[nodiscard]] auto
        Timeline() const noexcept -> const std::array<std::uint8_t, 4> &;
        auto
        MoveEvent(std::size_t position, int direction) noexcept -> void;
        [[nodiscard]] auto
        Answer(std::size_t question) const noexcept -> std::uint8_t;
        auto
        SetAnswer(std::size_t question, std::uint8_t answer) noexcept -> void;
        [[nodiscard]] auto
        Grade() const noexcept -> ReportFinding;
        [[nodiscard]] auto
        Submit() noexcept -> ReportFinding;
        [[nodiscard]] auto
        Filed() const noexcept -> bool;
        auto
        Observe(Subject subject) noexcept -> void;

        [[nodiscard]] auto
        HasObserved(Subject subject) const noexcept -> bool;

        [[nodiscard]] auto
        CompareRecords(Subject first, Subject second) noexcept -> bool;

        [[nodiscard]] auto
        HasComparedRecords() const noexcept -> bool;

        [[nodiscard]] auto
        Challenge(Claim claim, Subject source) noexcept -> ChallengeResult;

        [[nodiscard]] auto
        IsClarified(Claim claim) const noexcept -> bool;

        [[nodiscard]] auto
        IsComplete() const noexcept -> bool;

        [[nodiscard]] auto
        Serialize() const -> std::string;

        [[nodiscard]] static auto
        Parse(std::string_view encoded) noexcept
            -> tl::expected<Case, std::string_view>;

    private:
        CaseId id_{};
        std::array<std::uint8_t, 4> timeline_{ 2, 0, 3, 1 };
        std::array<std::uint8_t, 3> answers_{};
        bool filed_{};
        std::uint32_t observed_{};
        std::uint32_t clarified_{};
        bool comparedRecords_{};
    };
} // namespace TwentyThreeSeventeen::Investigation
