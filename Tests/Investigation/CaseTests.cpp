// SPDX-License-Identifier: MIT
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Generators/Generators.hpp>

#include "Investigation/Case.hpp"

namespace
{
    using namespace TwentyThreeSeventeen::Investigation;

    auto
    CollectSources(Case &investigation) -> void
    {
        for (const auto source : Definition(investigation.Id()).sources)
            investigation.Observe(source);
    }

    auto
    PutEventsInOrder(Case &investigation) -> void
    {
        investigation.MoveEvent(0, 1);
        investigation.MoveEvent(3, -1);
        investigation.MoveEvent(2, -1);
    }
} // namespace

TEST_CASE("A claim needs the witness and its specific collected record")
{
    const auto id
        = GENERATE(CaseId::kApartment, CaseId::kArchive, CaseId::kCinema);
    const auto claim = GENERATE(Claim::kFirst, Claim::kSecond);
    const auto &definition = Definition(id);
    const auto &rule = definition.claims[static_cast<std::size_t>(claim)];
    const auto unrelated = definition.sources.front();

    Case investigation(id);
    investigation.Observe(rule.witness);
    REQUIRE(investigation.Challenge(claim, rule.record)
            == ChallengeResult::kMissingSource);
    investigation.Observe(unrelated);
    REQUIRE(investigation.Challenge(claim, unrelated)
            == ChallengeResult::kUnrelated);
    REQUIRE_FALSE(investigation.IsClarified(claim));
    investigation.Observe(rule.record);
    REQUIRE(investigation.Challenge(claim, rule.record)
            == ChallengeResult::kClarified);
}

TEST_CASE("Three cases can close independently with ordered evidence and a "
          "revisable report")
{
    const auto id
        = GENERATE(CaseId::kApartment, CaseId::kArchive, CaseId::kCinema);
    Case investigation(id);
    const auto &definition = Definition(id);
    const auto &pair = definition.comparison;
    REQUIRE_FALSE(investigation.CompareRecords(pair[0], pair[1]));
    investigation.MoveEvent(0, 1);
    REQUIRE(investigation.Timeline().front() == 2);
    REQUIRE(investigation.Grade() == ReportFinding::kIncomplete);
    CollectSources(investigation);
    REQUIRE_FALSE(investigation.CompareRecords(pair[0], pair[0]));
    REQUIRE(investigation.CompareRecords(pair[1], pair[0]));
    REQUIRE(investigation.Challenge(Claim::kFirst, definition.claims[0].record)
            == ChallengeResult::kClarified);
    REQUIRE(investigation.Challenge(Claim::kSecond, definition.claims[1].record)
            == ChallengeResult::kClarified);

    for (std::size_t question = 0;
         question < definition.supportedAnswers.size();
         ++question)
        investigation.SetAnswer(question,
                                definition.supportedAnswers[question]);

    REQUIRE(investigation.Submit() == ReportFinding::kUnsupported);
    REQUIRE_FALSE(investigation.Filed());
    PutEventsInOrder(investigation);
    REQUIRE(investigation.Submit() == ReportFinding::kSupported);
    const auto filed = Case::Parse(investigation.Serialize());
    REQUIRE(filed);
    REQUIRE(filed->Id() == id);
    REQUIRE(filed->Filed());
    REQUIRE(filed->Timeline() == investigation.Timeline());
    REQUIRE(filed->Answer(0) == definition.supportedAnswers[0]);
    investigation.SetAnswer(0, definition.supportedAnswers[0] == 1 ? 2 : 1);
    REQUIRE_FALSE(investigation.Filed());
    REQUIRE(investigation.Grade() == ReportFinding::kUnsupported);
}

TEST_CASE("The old native Apartment save preserves its original evidence")
{
    const auto saved = Case::Parse("2317-case-1 126 3 1");
    REQUIRE(saved);
    REQUIRE(saved->Id() == CaseId::kApartment);
    REQUIRE(saved->HasComparedRecords());
    REQUIRE(saved->HasObserved(Subject::kDoorLog));
    REQUIRE(saved->IsClarified(Claim::kFirst));
    REQUIRE(saved->IsClarified(Claim::kSecond));
    REQUIRE_FALSE(saved->HasObserved(Subject::kRecorder));
    REQUIRE_FALSE(saved->Filed());
    REQUIRE_FALSE(Case::Parse("2317-case-1 254 3 1"));
    REQUIRE_FALSE(Case::Parse("2317-case-1 4 1 0"));
    REQUIRE_FALSE(Case::Parse("2317-case-1 0 0 1"));
    REQUIRE_FALSE(Case::Parse("2317-case-1 126 3 1 extra"));
}

TEST_CASE("Current saves reject evidence from a different case and invalid "
          "chronology")
{
    Case archive(CaseId::kArchive);
    archive.Observe(Subject::kDispatch);
    REQUIRE_FALSE(archive.HasObserved(Subject::kDispatch));
    REQUIRE_FALSE(Case::Parse("2317-case-2 1 2 0 0 2 0 3 1 0 0 0 0"));
    REQUIRE_FALSE(Case::Parse("2317-case-2 0 0 0 0 2 0 3 3 0 0 0 0"));
    REQUIRE_FALSE(Case::Parse("2317-case-2 0 0 0 0 2 0 3 1 0 0 0 1"));
}
