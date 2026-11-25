// SPDX-License-Identifier: MIT
#include <algorithm>
#include <charconv>

#include "Investigation/Case.hpp"

namespace TwentyThreeSeventeen::Investigation
{
    namespace
    {
        using enum Subject;
        constexpr std::array<CaseDefinition, 3> kCases{ {
            { "apartment",
              { kDispatch,
                kCaretaker,
                kCamera,
                kBreaker,
                kDoorLog,
                kElectrician,
                kRecorder,
                kRevision },
              { kDispatch, kCamera },
              { { { kCaretaker, kBreaker }, { kCaretaker, kDoorLog } } },
              { kBreaker, kCamera, kRecorder, kDoorLog },
              { 1, 2, 1 } },
            { "archive",
              { kInventory,
                kArchivist,
                kTransfer,
                kShelf,
                kDuty,
                kIntake,
                kReceiver,
                kNone },
              { kTransfer, kShelf },
              { { { kArchivist, kTransfer }, { kArchivist, kIntake } } },
              { kShelf, kTransfer, kDuty, kIntake },
              { 2, 1, 2 } },
            { "cinema",
              { kSchedule,
                kUsher,
                kScreening,
                kDiary,
                kExit,
                kLeader,
                kReceipt,
                kNone },
              { kScreening, kDiary },
              { { { kUsher, kLeader }, { kUsher, kExit } } },
              { kLeader, kDiary, kExit, kReceipt },
              { 1, 2, 2 } },
        } };
        constexpr std::array<std::uint8_t, 4> kChronological{ 0, 1, 2, 3 };
        constexpr std::string_view kOld = "2317-case-1 ";
        constexpr std::string_view kCurrent = "2317-case-2 ";
        constexpr std::string_view kInvalid = "Invalid case save.";

        template<typename Enum>
        constexpr auto
        Flag(Enum value) noexcept -> std::uint32_t
        {
            const auto index = static_cast<unsigned>(value);
            return index < 32 ? std::uint32_t{ 1 } << index : 0;
        }

        template<std::size_t Count>
        auto
        ReadFields(std::string_view encoded,
                   std::array<std::uint32_t, Count> &fields) noexcept -> bool
        {
            for (auto &field : fields)
            {
                const auto end = encoded.find(' ');
                if ((end == std::string_view::npos)
                    != (&field == &fields.back()))
                    return false;
                const auto part = encoded.substr(0, end);
                if (part.empty())
                    return false;
                const auto parsed = std::from_chars(part.data(),
                                                    part.data() + part.size(),
                                                    field);
                if (parsed.ec != std::errc()
                    || parsed.ptr != part.data() + part.size())
                    return false;
                encoded.remove_prefix(
                    end == std::string_view::npos ? part.size() : end + 1);
            }
            return true;
        }
    } // namespace

    auto
    Definition(CaseId id) noexcept -> const CaseDefinition &
    {
        const auto index = static_cast<std::size_t>(id);
        return kCases[index < kCases.size() ? index : 0];
    }

    Case::Case(CaseId id) noexcept
        : id_(id)
    {}

    auto
    Case::Id() const noexcept -> CaseId
    {
        return id_;
    }
    auto
    Case::HasProgress() const noexcept -> bool
    {
        return observed_ != 0 || std::ranges::any_of(answers_, [](auto answer) {
                   return answer != 0;
               });
    }

    auto
    Case::Observe(Subject subject) noexcept -> void
    {
        if (subject != Subject::kNone
            && std::ranges::find(Definition(id_).sources, subject)
                   != Definition(id_).sources.end())
            observed_ |= Flag(subject);
    }

    auto
    Case::HasObserved(Subject subject) const noexcept -> bool
    {
        return (observed_ & Flag(subject)) != 0;
    }

    auto
    Case::CompareRecords(Subject first, Subject second) noexcept -> bool
    {
        const auto &sources = Definition(id_).comparison;
        if (!HasObserved(first) || !HasObserved(second))
            return false;
        if (!((first == sources[0] && second == sources[1])
              || (second == sources[0] && first == sources[1])))
            return false;
        comparedRecords_ = true;
        return true;
    }

    auto
    Case::HasComparedRecords() const noexcept -> bool
    {
        return comparedRecords_;
    }

    auto
    Case::Challenge(Claim claim, Subject source) noexcept -> ChallengeResult
    {
        const auto &rules = Definition(id_).claims;
        const auto index = static_cast<std::size_t>(claim);
        if (index >= rules.size())
            return ChallengeResult::kUnrelated;
        const auto &rule = rules[index];
        if (!HasObserved(rule.witness) || !HasObserved(source))
            return ChallengeResult::kMissingSource;
        if (source != rule.record)
            return ChallengeResult::kUnrelated;
        clarified_ |= Flag(claim);
        return ChallengeResult::kClarified;
    }

    auto
    Case::IsClarified(Claim claim) const noexcept -> bool
    {
        return (clarified_ & Flag(claim)) != 0;
    }

    auto
    Case::IsComplete() const noexcept -> bool
    {
        return comparedRecords_ && clarified_ == 3
               && std::ranges::all_of(Definition(id_).sources,
                                      [this](Subject source) {
                                          return source == Subject::kNone
                                                 || HasObserved(source);
                                      });
    }

    auto
    Case::Timeline() const noexcept -> const std::array<std::uint8_t, 4> &
    {
        return timeline_;
    }

    auto
    Case::MoveEvent(std::size_t position, int direction) noexcept -> void
    {
        const auto &events = Definition(id_).timeline;
        if (position >= timeline_.size() || direction == 0
            || !HasObserved(events[timeline_[position]]))
            return;
        const auto step = direction < 0 ? -1 : 1;
        for (int next = static_cast<int>(position) + step;
             next >= 0 && next < static_cast<int>(timeline_.size());
             next += step)
        {
            const auto adjacent = static_cast<std::size_t>(next);
            if (!HasObserved(events[timeline_[adjacent]]))
                continue;
            std::swap(timeline_[position], timeline_[adjacent]);
            filed_ = false;
            return;
        }
    }

    auto
    Case::Answer(std::size_t question) const noexcept -> std::uint8_t
    {
        return question < answers_.size() ? answers_[question] : 0;
    }

    auto
    Case::SetAnswer(std::size_t question, std::uint8_t answer) noexcept -> void
    {
        if (question < answers_.size() && answer <= 2
            && answers_[question] != answer)
        {
            answers_[question] = answer;
            filed_ = false;
        }
    }

    auto
    Case::Grade() const noexcept -> ReportFinding
    {
        if (!IsComplete() || std::ranges::find(answers_, 0) != answers_.end())
            return ReportFinding::kIncomplete;
        if (timeline_ != kChronological
            || answers_ != Definition(id_).supportedAnswers)
            return ReportFinding::kUnsupported;
        return ReportFinding::kSupported;
    }

    auto
    Case::Submit() noexcept -> ReportFinding
    {
        const auto finding = Grade();
        filed_ = finding == ReportFinding::kSupported;
        return finding;
    }

    auto
    Case::Filed() const noexcept -> bool
    {
        return filed_;
    }

    auto
    Case::Serialize() const -> std::string
    {
        auto encoded = std::string(kCurrent)
                       + std::to_string(static_cast<unsigned>(id_)) + " "
                       + std::to_string(observed_) + " "
                       + std::to_string(clarified_) + " "
                       + (comparedRecords_ ? "1" : "0");
        for (const auto event : timeline_)
            encoded += " " + std::to_string(event);
        for (const auto answer : answers_)
            encoded += " " + std::to_string(answer);
        encoded += filed_ ? " 1" : " 0";
        return encoded;
    }

    auto
    Case::Parse(std::string_view encoded) noexcept
        -> tl::expected<Case, std::string_view>
    {
        std::array<std::uint32_t, 12> fields{};
        if (encoded.starts_with(kOld))
        {
            std::array<std::uint32_t, 3> old{};
            if (!ReadFields(encoded.substr(kOld.size()), old)
                || (old[0] & ~std::uint32_t{ 126 }) != 0)
                return tl::unexpected(kInvalid);
            fields = { 0, old[0], old[1], old[2], 2, 0, 3, 1, 0, 0, 0, 0 };
        }
        else if (!encoded.starts_with(kCurrent)
                 || !ReadFields(encoded.substr(kCurrent.size()), fields))
            return tl::unexpected(kInvalid);
        if (fields[0] >= kCases.size() || fields[2] > 3 || fields[3] > 1
            || fields[11] > 1)
            return tl::unexpected(kInvalid);
        Case restored(static_cast<CaseId>(fields[0]));
        const auto &definition = Definition(restored.id_);
        for (const auto source : definition.sources)
        {
            if ((fields[1] & Flag(source)) != 0)
                restored.Observe(source);
        }
        if (restored.observed_ != fields[1]
            || (fields[3] == 1
                && !restored.CompareRecords(definition.comparison[0],
                                            definition.comparison[1])))
            return tl::unexpected(kInvalid);

        for (std::size_t index = 0; index < definition.claims.size(); ++index)
        {
            const auto claim = static_cast<Claim>(index);
            if ((fields[2] & Flag(claim)) != 0
                && restored.Challenge(claim, definition.claims[index].record)
                       != ChallengeResult::kClarified)
                return tl::unexpected(kInvalid);
        }
        std::array<bool, 4> seen{};
        for (std::size_t index = 0; index < restored.timeline_.size(); ++index)
        {
            const auto event = fields[4 + index];
            if (event >= seen.size() || seen[event])
                return tl::unexpected(kInvalid);
            seen[event] = true;
            restored.timeline_[index] = static_cast<std::uint8_t>(event);
        }
        for (std::size_t index = 0; index < restored.answers_.size(); ++index)
        {
            if (fields[8 + index] > 2)
                return tl::unexpected(kInvalid);
            restored.SetAnswer(index,
                               static_cast<std::uint8_t>(fields[8 + index]));
        }
        if (fields[11] == 1 && restored.Submit() != ReportFinding::kSupported)
            return tl::unexpected(kInvalid);
        return restored;
    }
} // namespace TwentyThreeSeventeen::Investigation
