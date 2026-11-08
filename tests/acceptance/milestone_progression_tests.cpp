// SPDX-License-Identifier: GPL-3.0-or-later
// Sequential milestone proposals preserve history, exact intervals and current deficits.
#include "../fixtures/milestone_progression_vectors.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <span>
#include <string>
#include <subathon/core/milestone_progression.hpp>
#include <typeinfo>
#include <utility>

namespace {
using namespace subathon::core;
namespace literal = milestone_vectors;
using Error = MilestoneProgressionError;
constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
constexpr std::array reference_names{"track_ref",
                                     "definition_revision_ref",
                                     "progression_context_ref",
                                     "target.milestone_ref",
                                     "completed.milestone_ref",
                                     "completed.definition_revision_ref",
                                     "completed.completion_context_ref"};

MilestoneProgressionInput base(std::int64_t score = -1) { return {"t", "v", "c", score, {}, {}}; }
MilestoneCompletion completion(const literal::Completion &value) {
    return {std::string(value.id), std::string(value.revision), value.target, value.score,
            std::string(value.context)};
}
MilestoneProgressionInput input_for(const literal::Row &row) {
    MilestoneProgressionInput input{std::string(literal::track),
                                    std::string(literal::revision),
                                    std::string(literal::context),
                                    row.score,
                                    {},
                                    {}};
    for (const auto &target : row.targets)
        input.targets.push_back({std::string(target.id), target.value});
    for (const auto &record : row.before)
        input.completed_prefix.push_back(completion(record));
    return input;
}
void expect_target(const MilestoneTarget &actual, const MilestoneTarget &expected) {
    EXPECT_EQ(actual.milestone_ref, expected.milestone_ref);
    EXPECT_EQ(actual.target_micro_points, expected.target_micro_points);
}
void expect_completion(const MilestoneCompletion &actual, const MilestoneCompletion &expected) {
    EXPECT_EQ(actual.milestone_ref, expected.milestone_ref);
    EXPECT_EQ(actual.definition_revision_ref, expected.definition_revision_ref);
    EXPECT_EQ(actual.target_micro_points, expected.target_micro_points);
    EXPECT_EQ(actual.score_micro_points, expected.score_micro_points);
    EXPECT_EQ(actual.completion_context_ref, expected.completion_context_ref);
}
void expect_input(const MilestoneProgressionInput &actual,
                  const MilestoneProgressionInput &expected) {
    EXPECT_EQ(actual.track_ref, expected.track_ref);
    EXPECT_EQ(actual.definition_revision_ref, expected.definition_revision_ref);
    EXPECT_EQ(actual.progression_context_ref, expected.progression_context_ref);
    EXPECT_EQ(actual.score_micro_points, expected.score_micro_points);
    EXPECT_EQ(actual.targets.size(), expected.targets.size());
    for (std::size_t i = 0; i < std::min(actual.targets.size(), expected.targets.size()); ++i) {
        SCOPED_TRACE(i);
        expect_target(actual.targets[i], expected.targets[i]);
    }
    EXPECT_EQ(actual.completed_prefix.size(), expected.completed_prefix.size());
    for (std::size_t i = 0;
         i < std::min(actual.completed_prefix.size(), expected.completed_prefix.size()); ++i) {
        SCOPED_TRACE(i);
        expect_completion(actual.completed_prefix[i], expected.completed_prefix[i]);
    }
}
MilestoneTrackState state(literal::State value) {
    switch (value) {
    case literal::State::empty:
        return MilestoneTrackState::empty;
    case literal::State::before_first:
        return MilestoneTrackState::before_first;
    case literal::State::between_targets:
        return MilestoneTrackState::between_targets;
    case literal::State::all_completed:
        return MilestoneTrackState::all_completed;
    }
    ADD_FAILURE() << "invalid independent fixture state";
    return MilestoneTrackState::empty;
}
void expect_optional_target(const std::optional<MilestoneTarget> &actual,
                            const std::optional<literal::Target> &expected) {
    EXPECT_EQ(actual.has_value(), expected.has_value());
    if (actual && expected)
        expect_target(*actual, {std::string(expected->id), expected->value});
}
void expect_result(const MilestoneProgressionResult &result, const literal::Row &expected) {
    const auto &actual = result.data();
    EXPECT_EQ(actual.track_ref, literal::track);
    EXPECT_EQ(actual.definition_revision_ref, literal::revision);
    EXPECT_EQ(actual.progression_context_ref, literal::context);
    EXPECT_EQ(actual.score_micro_points, expected.score);
    EXPECT_EQ(actual.completed_prefix.size(), expected.after.size());
    for (std::size_t i = 0; i < std::min(actual.completed_prefix.size(), expected.after.size());
         ++i) {
        SCOPED_TRACE(i);
        expect_completion(actual.completed_prefix[i], completion(expected.after[i]));
    }
    EXPECT_EQ(actual.newly_completed_begin, expected.begin);
    EXPECT_EQ(actual.newly_completed_count, expected.count);
    const auto &p = actual.projection;
    const auto &e = expected.projection;
    EXPECT_EQ(p.state, state(e.state));
    EXPECT_EQ(p.completed_deficits.size(), e.deficits.size());
    for (std::size_t i = 0; i < std::min(p.completed_deficits.size(), e.deficits.size()); ++i) {
        SCOPED_TRACE(i);
        expect_target(p.completed_deficits[i].current_target,
                      {std::string(e.deficits[i].current.id), e.deficits[i].current.value});
        EXPECT_EQ(p.completed_deficits[i].deficit_micro_points, e.deficits[i].amount);
    }
    expect_optional_target(p.previous, e.previous);
    expect_optional_target(p.next, e.next);
    EXPECT_EQ(p.track_deficit_micro_points, e.track_deficit);
    EXPECT_EQ(p.remaining_micro_points, e.remaining);
    EXPECT_EQ(p.interval.has_value(), e.interval.has_value());
    if (p.interval && e.interval) {
        EXPECT_EQ(p.interval->earned_micro_points, e.interval->earned);
        EXPECT_EQ(p.interval->span_micro_points, e.interval->span);
        EXPECT_EQ(p.interval->fraction, Exact(e.interval->numerator) / e.interval->denominator);
        EXPECT_EQ(boost::multiprecision::numerator(p.interval->fraction), e.interval->numerator);
        EXPECT_EQ(boost::multiprecision::denominator(p.interval->fraction),
                  e.interval->denominator);
    }
}

// Nonfatal for each caller-labelled row; an unrelated marker is an explicit failure.
void expect_failure(const MilestoneProgressionInput &input,
                    const MilestoneProgressionLimits &limits, Error expected) {
    const auto before = input;
    try {
        (void)advance_milestone_progression(input, limits);
        ADD_FAILURE() << "expected MilestoneProgressionFailure";
    } catch (const MilestoneProgressionFailure &error) {
        EXPECT_EQ(error.code(), expected);
        EXPECT_STREQ(error.what(), "Invalid milestone progression input");
    } catch (const std::exception &error) {
        ADD_FAILURE() << "wrong exception " << typeid(error).name() << ": " << error.what();
    } catch (...) {
        ADD_FAILURE() << "wrong nonstandard exception";
    }
    expect_input(input, before);
}
void expect_empty(const MilestoneProgressionResult &result, std::int64_t score = -1) {
    const auto &d = result.data();
    EXPECT_EQ(d.track_ref, "t");
    EXPECT_EQ(d.definition_revision_ref, "v");
    EXPECT_EQ(d.progression_context_ref, "c");
    EXPECT_EQ(d.score_micro_points, score);
    EXPECT_TRUE(d.completed_prefix.empty());
    EXPECT_EQ(d.newly_completed_begin, 0U);
    EXPECT_EQ(d.newly_completed_count, 0U);
    EXPECT_EQ(d.projection.state, MilestoneTrackState::empty);
    EXPECT_TRUE(d.projection.completed_deficits.empty());
    EXPECT_FALSE(d.projection.previous);
    EXPECT_FALSE(d.projection.next);
    EXPECT_EQ(d.projection.track_deficit_micro_points, 0U);
    EXPECT_FALSE(d.projection.remaining_micro_points);
    EXPECT_FALSE(d.projection.interval);
}
MilestoneProgressionInput many_targets(std::size_t count, bool completed = false) {
    auto input = base();
    for (std::size_t i = 0; i < count; ++i) {
        auto id = "m" + std::to_string(i);
        input.targets.push_back({id, static_cast<std::int64_t>(i)});
        if (completed)
            input.completed_prefix.push_back({id, "old", 0, 0, "old-command"});
    }
    return input;
}
MilestoneProgressionInput byte_interval() {
    return {"t", "v", "c", 1, {{"A", 0}, {"B", 3}}, {{"A", "old", 0, 0, "z"}}};
}

class MilestoneVectors : public testing::TestWithParam<literal::Row> {};
TEST_P(MilestoneVectors, MatchesIndependentLiteral) {
    const auto &row = GetParam();
    SCOPED_TRACE(row.label);
    const auto input = input_for(row);
    const auto before = input;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input);
        expect_result(result, row);
    });
    expect_input(input, before);
}
INSTANTIATE_TEST_SUITE_P(IndependentMP, MilestoneVectors, testing::ValuesIn(literal::rows),
                         [](const testing::TestParamInfo<literal::Row> &value) {
                             return std::string(value.param.label);
                         });

TEST(MilestoneIdentity, PreservesOpaqueNulAndHighBytesWithoutSorting) {
    const std::string first("\xff\0A", 3), second("\x80\0B", 3);
    auto input = base(1);
    input.track_ref = std::string("t\0\xff", 3);
    input.definition_revision_ref = std::string("v\0\x80", 3);
    input.progression_context_ref = std::string("c\0x", 3);
    input.targets = {{first, 0}, {second, 1}};
    const auto result = advance_milestone_progression(input);
    const auto &d = result.data();
    EXPECT_EQ(d.track_ref, std::string("t\0\xff", 3));
    EXPECT_EQ(d.definition_revision_ref, std::string("v\0\x80", 3));
    EXPECT_EQ(d.progression_context_ref, std::string("c\0x", 3));
    EXPECT_EQ(d.completed_prefix.size(), 2U);
    if (d.completed_prefix.size() == 2) {
        expect_completion(d.completed_prefix[0],
                          {first, std::string("v\0\x80", 3), 0, 1, std::string("c\0x", 3)});
        expect_completion(d.completed_prefix[1],
                          {second, std::string("v\0\x80", 3), 1, 1, std::string("c\0x", 3)});
    }
    EXPECT_EQ(d.newly_completed_begin, 0U);
    EXPECT_EQ(d.newly_completed_count, 2U);
    EXPECT_EQ(d.projection.state, MilestoneTrackState::all_completed);
}
TEST(MilestoneIdentity, BytesAfterNulDistinguishMilestones) {
    auto input = base();
    input.targets = {{std::string("A\0x", 3), 0}, {std::string("A\0y", 3), 1}};
    const auto result = advance_milestone_progression(input);
    EXPECT_EQ(result.data().projection.state, MilestoneTrackState::before_first);
    EXPECT_TRUE(result.data().completed_prefix.empty());
    EXPECT_TRUE(result.data().projection.next);
    if (result.data().projection.next)
        expect_target(*result.data().projection.next, {std::string("A\0x", 3), 0});
}
TEST(MilestoneIdentity, ExactOpaqueDuplicateIsRejected) {
    auto input = base();
    input.targets = {{std::string("\xff\0x", 3), 0}, {std::string("\xff\0x", 3), 1}};
    expect_failure(input, {}, Error::duplicate_milestone);
}
TEST(MilestoneEndpoints, MinimumScoreEqualsMinimumTarget) {
    auto input = base(minimum);
    input.targets = {{"A", minimum}};
    const auto result = advance_milestone_progression(input);
    const auto &d = result.data();
    EXPECT_EQ(d.score_micro_points, minimum);
    EXPECT_EQ(d.completed_prefix.size(), 1U);
    if (d.completed_prefix.size() == 1)
        expect_completion(d.completed_prefix[0], {"A", "v", minimum, minimum, "c"});
    EXPECT_EQ(d.newly_completed_begin, 0U);
    EXPECT_EQ(d.newly_completed_count, 1U);
    EXPECT_EQ(d.projection.state, MilestoneTrackState::all_completed);
    EXPECT_EQ(d.projection.track_deficit_micro_points, 0U);
    EXPECT_TRUE(d.projection.previous);
    if (d.projection.previous)
        expect_target(*d.projection.previous, {"A", minimum});
    EXPECT_FALSE(d.projection.next);
    EXPECT_FALSE(d.projection.remaining_micro_points);
    EXPECT_FALSE(d.projection.interval);
}
TEST(MilestoneEndpoints, MaximumScoreCrossesBothSignedExtremes) {
    auto input = base(maximum);
    input.targets = {{"A", minimum}, {"B", maximum}};
    const auto result = advance_milestone_progression(input);
    const auto &d = result.data();
    EXPECT_EQ(d.score_micro_points, maximum);
    EXPECT_EQ(d.completed_prefix.size(), 2U);
    if (d.completed_prefix.size() == 2) {
        expect_completion(d.completed_prefix[0], {"A", "v", minimum, maximum, "c"});
        expect_completion(d.completed_prefix[1], {"B", "v", maximum, maximum, "c"});
    }
    EXPECT_EQ(d.newly_completed_begin, 0U);
    EXPECT_EQ(d.newly_completed_count, 2U);
    EXPECT_EQ(d.projection.state, MilestoneTrackState::all_completed);
    EXPECT_EQ(d.projection.track_deficit_micro_points, 0U);
    EXPECT_TRUE(d.projection.previous);
    if (d.projection.previous)
        expect_target(*d.projection.previous, {"B", maximum});
    EXPECT_FALSE(d.projection.next);
    EXPECT_FALSE(d.projection.remaining_micro_points);
    EXPECT_FALSE(d.projection.interval);
}

TEST(MilestoneValidation, EveryRequiredReferenceMustBeNonempty) {
    for (int field = 0; field < 7; ++field) {
        SCOPED_TRACE(reference_names[static_cast<std::size_t>(field)]);
        auto input = byte_interval();
        std::array<std::string *, 7> refs{&input.track_ref,
                                          &input.definition_revision_ref,
                                          &input.progression_context_ref,
                                          &input.targets[0].milestone_ref,
                                          &input.completed_prefix[0].milestone_ref,
                                          &input.completed_prefix[0].definition_revision_ref,
                                          &input.completed_prefix[0].completion_context_ref};
        refs[static_cast<std::size_t>(field)]->clear();
        expect_failure(input, {}, Error::invalid_reference);
    }
}
TEST(MilestoneValidation, DuplicateIdsAreRejectedEvenWithDifferentTargets) {
    const std::array labels{"same-target", "different-target", "separated-and-descending"};
    const std::array cases{std::vector<MilestoneTarget>{{"A", 0}, {"A", 0}},
                           std::vector<MilestoneTarget>{{"A", 0}, {"A", 1}},
                           std::vector<MilestoneTarget>{{"A", 2}, {"B", 1}, {"A", 0}}};
    for (std::size_t row = 0; row < cases.size(); ++row) {
        SCOPED_TRACE(labels[row]);
        auto input = base();
        input.targets = cases[row];
        expect_failure(input, {}, Error::duplicate_milestone);
    }
}
TEST(MilestoneValidation, CurrentTargetsMustBeStrictlyAscending) {
    for (const auto &targets : {std::vector<MilestoneTarget>{{"A", 0}, {"B", 0}},
                                std::vector<MilestoneTarget>{{"A", 1}, {"B", 0}},
                                std::vector<MilestoneTarget>{{"A", maximum}, {"B", minimum}}}) {
        SCOPED_TRACE(targets.front().target_micro_points);
        auto input = base();
        input.targets = targets;
        expect_failure(input, {}, Error::nonascending_targets);
    }
}
TEST(MilestoneValidation, CompletedIdentitiesMustBeExactlyTheFirstK) {
    const std::array<std::vector<MilestoneCompletion>, 5> prefixes{
        std::vector<MilestoneCompletion>{{"B", "old", 9, -3, "old"}},
        std::vector<MilestoneCompletion>{{"A", "old", 9, -3, "old"}, {"C", "old", 8, -4, "old"}},
        std::vector<MilestoneCompletion>{{"B", "old", 9, -3, "old"}, {"A", "old", 8, -4, "old"}},
        std::vector<MilestoneCompletion>{{"A", "old", 9, -3, "old"}, {"A", "old", 8, -4, "old"}},
        std::vector<MilestoneCompletion>{
            {"A", "old", 9, -3, "old"}, {"B", "old", 8, -4, "old"}, {"C", "old", 7, -5, "old"}}};
    for (std::size_t row = 0; row < prefixes.size(); ++row) {
        SCOPED_TRACE(row);
        auto input = base();
        input.targets = {{"A", 0}, {"B", 1}};
        input.completed_prefix = prefixes[row];
        expect_failure(input, {}, Error::invalid_completed_prefix);
    }
    auto empty = base();
    empty.completed_prefix = {{"A", "old", 0, 0, "old"}};
    expect_failure(empty, {}, Error::invalid_completed_prefix);
}
TEST(MilestoneValidation, HistoricalTargetsAndScoresNeedNotBeAscendingOrSufficient) {
    auto input = base(5);
    input.targets = {{"A", 0}, {"B", 10}, {"C", 20}};
    input.completed_prefix = {{"A", "old-A", 99, -9, "same-command"},
                              {"B", "old-B", -99, -10, "same-command"}};
    const auto result = advance_milestone_progression(input);
    const auto &d = result.data();
    EXPECT_EQ(d.completed_prefix.size(), 2U);
    if (d.completed_prefix.size() == 2) {
        expect_completion(d.completed_prefix[0], {"A", "old-A", 99, -9, "same-command"});
        expect_completion(d.completed_prefix[1], {"B", "old-B", -99, -10, "same-command"});
    }
    EXPECT_EQ(d.newly_completed_begin, 2U);
    EXPECT_EQ(d.newly_completed_count, 0U);
    EXPECT_EQ(d.projection.track_deficit_micro_points, 5U);
    EXPECT_EQ(d.projection.remaining_micro_points, 15U);
    EXPECT_EQ(d.projection.completed_deficits.size(), 2U);
    if (d.projection.completed_deficits.size() == 2) {
        EXPECT_EQ(d.projection.completed_deficits[0].deficit_micro_points, 0U);
        EXPECT_EQ(d.projection.completed_deficits[1].deficit_micro_points, 5U);
    }
    EXPECT_TRUE(d.projection.interval);
    if (d.projection.interval) {
        EXPECT_EQ(d.projection.interval->earned_micro_points, 0U);
        EXPECT_EQ(d.projection.interval->span_micro_points, 10U);
        EXPECT_EQ(d.projection.interval->fraction, Exact(0));
    }
}

TEST(MilestoneResources, EveryReferenceAccepts256Bytes) {
    const std::array labels{"track_ref",
                            "definition_revision_ref",
                            "progression_context_ref",
                            "matching-target-and-completion-id",
                            "captured-definition",
                            "captured-context"};
    for (int field = 0; field < 6; ++field) {
        SCOPED_TRACE(labels[static_cast<std::size_t>(field)]);
        auto input = byte_interval();
        const std::string boundary(256, 'x');
        if (field == 0)
            input.track_ref = boundary;
        if (field == 1)
            input.definition_revision_ref = boundary;
        if (field == 2)
            input.progression_context_ref = boundary;
        if (field == 3) {
            input.targets[0].milestone_ref = boundary;
            input.completed_prefix[0].milestone_ref = boundary;
        }
        if (field == 4)
            input.completed_prefix[0].definition_revision_ref = boundary;
        if (field == 5)
            input.completed_prefix[0].completion_context_ref = boundary;
        const auto before = input;
        EXPECT_NO_THROW({
            const auto result = advance_milestone_progression(input);
            EXPECT_EQ(result.data().completed_prefix.size(), 1U);
            if (result.data().completed_prefix.size() == 1)
                expect_completion(result.data().completed_prefix[0], before.completed_prefix[0]);
            EXPECT_EQ(result.data().track_ref, before.track_ref);
            EXPECT_EQ(result.data().definition_revision_ref, before.definition_revision_ref);
            EXPECT_EQ(result.data().progression_context_ref, before.progression_context_ref);
        });
        expect_input(input, before);
    }
}
TEST(MilestoneResources, EveryReferenceRejects257Bytes) {
    for (std::size_t field = 0; field < 7; ++field) {
        SCOPED_TRACE(reference_names[field]);
        auto input = byte_interval();
        std::array<std::string *, 7> refs{&input.track_ref,
                                          &input.definition_revision_ref,
                                          &input.progression_context_ref,
                                          &input.targets[0].milestone_ref,
                                          &input.completed_prefix[0].milestone_ref,
                                          &input.completed_prefix[0].definition_revision_ref,
                                          &input.completed_prefix[0].completion_context_ref};
        *refs[field] = std::string(257, 'x');
        expect_failure(input, {}, Error::resource_limit);
    }
}
TEST(MilestoneResources, ReferenceBudgetCountsBytesAfterNul) {
    auto input = base();
    input.track_ref = std::string(256, 'x');
    input.track_ref[1] = '\0';
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input);
        EXPECT_EQ(result.data().track_ref, input.track_ref);
        EXPECT_EQ(result.data().track_ref.size(), 256U);
    });
    input.track_ref.push_back('y');
    expect_failure(input, {}, Error::resource_limit);
}
TEST(MilestoneResources, SmallerReferenceCeilingAcceptsExactAndRefusesOneOver) {
    MilestoneProgressionLimits limits;
    limits.reference_bytes = 1;
    EXPECT_NO_THROW(expect_empty(advance_milestone_progression(base(), limits)));
    auto input = base();
    input.progression_context_ref = "cc";
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, SmallerTargetCeilingAcceptsExactAndRefusesOneOver) {
    auto input = base();
    input.targets = {{"A", 0}};
    MilestoneProgressionLimits limits;
    limits.targets = 1;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        EXPECT_EQ(result.data().projection.state, MilestoneTrackState::before_first);
        EXPECT_EQ(result.data().projection.remaining_micro_points, 1U);
    });
    input.targets.push_back({"B", 1});
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, Accepts4096TargetsWithoutFabricatingInitialProgress) {
    const auto input = many_targets(4096);
    const auto result = advance_milestone_progression(input);
    const auto &d = result.data();
    EXPECT_TRUE(d.completed_prefix.empty());
    EXPECT_EQ(d.newly_completed_begin, 0U);
    EXPECT_EQ(d.newly_completed_count, 0U);
    EXPECT_EQ(d.projection.state, MilestoneTrackState::before_first);
    EXPECT_FALSE(d.projection.previous);
    EXPECT_FALSE(d.projection.interval);
    EXPECT_EQ(d.projection.remaining_micro_points, 1U);
    EXPECT_TRUE(d.projection.next);
    if (d.projection.next)
        expect_target(*d.projection.next, {"m0", 0});
}
TEST(MilestoneResources, Rejects4097Targets) {
    expect_failure(many_targets(4097), {}, Error::resource_limit);
}
TEST(MilestoneResources, Accepts4096CompletedRowsWithoutRepeatingThem) {
    const auto input = many_targets(4096, true);
    const auto result = advance_milestone_progression(input);
    const auto &d = result.data();
    EXPECT_EQ(d.completed_prefix.size(), 4096U);
    for (std::size_t i = 0; i < std::min(d.completed_prefix.size(), input.completed_prefix.size());
         ++i) {
        SCOPED_TRACE(i);
        expect_completion(d.completed_prefix[i], input.completed_prefix[i]);
    }
    EXPECT_EQ(d.newly_completed_begin, 4096U);
    EXPECT_EQ(d.newly_completed_count, 0U);
    EXPECT_EQ(d.projection.state, MilestoneTrackState::all_completed);
    EXPECT_EQ(d.projection.track_deficit_micro_points, 4096U);
    EXPECT_FALSE(d.projection.next);
    EXPECT_FALSE(d.projection.remaining_micro_points);
    EXPECT_FALSE(d.projection.interval);
    EXPECT_EQ(d.projection.completed_deficits.size(), 4096U);
    if (d.projection.completed_deficits.size() == 4096) {
        expect_target(d.projection.completed_deficits[0].current_target, {"m0", 0});
        EXPECT_EQ(d.projection.completed_deficits[0].deficit_micro_points, 1U);
        expect_target(d.projection.completed_deficits[2048].current_target, {"m2048", 2048});
        EXPECT_EQ(d.projection.completed_deficits[2048].deficit_micro_points, 2049U);
        expect_target(d.projection.completed_deficits[4095].current_target, {"m4095", 4095});
        EXPECT_EQ(d.projection.completed_deficits[4095].deficit_micro_points, 4096U);
    }
}
TEST(MilestoneResources, Rejects4097CompletedRowsBeforePrefixShape) {
    auto input = many_targets(4096, true);
    input.completed_prefix.push_back({"extra", "old", 0, 0, "old"});
    expect_failure(input, {}, Error::resource_limit);
}
TEST(MilestoneResources, SmallerCompletedCeilingAcceptsExactOutputAndRefusesOneOver) {
    auto input = base(1);
    input.targets = {{"A", 0}, {"B", 1}};
    MilestoneProgressionLimits limits;
    limits.completed_prefix = 2;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        EXPECT_EQ(result.data().completed_prefix.size(), 2U);
        if (result.data().completed_prefix.size() == 2) {
            expect_completion(result.data().completed_prefix[0], {"A", "v", 0, 1, "c"});
            expect_completion(result.data().completed_prefix[1], {"B", "v", 1, 1, "c"});
        }
        EXPECT_EQ(result.data().newly_completed_count, 2U);
    });
    limits.completed_prefix = 1;
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, SmallerCompletedCeilingRefusesInputOneOver) {
    auto input = base();
    input.targets = {{"A", 0}, {"B", 1}};
    input.completed_prefix = {{"A", "old", 0, 0, "old"}};
    MilestoneProgressionLimits limits;
    limits.completed_prefix = 1;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        EXPECT_EQ(result.data().completed_prefix.size(), 1U);
        EXPECT_EQ(result.data().newly_completed_count, 0U);
    });
    input.completed_prefix.push_back({"B", "old", 0, 0, "old"});
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, ItemBudgetMeasuresInputIndependently) {
    const auto input = byte_interval(); // n=2, k=1: input3; result prefix1+deficits1=2.
    MilestoneProgressionLimits limits;
    limits.items = 3;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        EXPECT_EQ(result.data().completed_prefix.size(), 1U);
        EXPECT_EQ(result.data().projection.completed_deficits.size(), 1U);
    });
    limits.items = 2;
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, ItemBudgetMeasuresWholeOutputBeforeGrowth) {
    auto input = base(1);
    input.targets = {{"A", 0}, {"B", 1}}; // Input2; completed2+deficits2=4.
    MilestoneProgressionLimits limits;
    limits.items = 4;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        EXPECT_EQ(result.data().completed_prefix.size(), 2U);
        EXPECT_EQ(result.data().projection.completed_deficits.size(), 2U);
    });
    limits.items = 3;
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, ByteBudgetMeasuresExactInput56AndOneOver) {
    auto input = base();
    input.targets = {{"A", 100}, {"B", 200}, {"C", 300}, {"D", 400}, {"E", 500}};
    // Input context/score11+five targets45=56; unanchored result53.
    MilestoneProgressionLimits limits;
    limits.owned_bytes = 56;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        EXPECT_EQ(result.data().projection.remaining_micro_points, 101U);
        EXPECT_EQ(result.data().projection.state, MilestoneTrackState::before_first);
    });
    limits.owned_bytes = 55;
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, ByteBudgetMeasuresExactEmptyOutput36AndOneOver) {
    MilestoneProgressionLimits limits;
    limits.owned_bytes = 36;
    EXPECT_NO_THROW(expect_empty(advance_milestone_progression(base(0), limits), 0));
    limits.owned_bytes = 35;
    expect_failure(base(0), limits, Error::resource_limit);
}
TEST(MilestoneResources, ByteBudgetIncludesAllIntervalCopiesAt118) {
    const auto input = byte_interval(); // Approved API literal input50/result118.
    MilestoneProgressionLimits limits;
    limits.owned_bytes = 118;
    EXPECT_NO_THROW({
        const auto result = advance_milestone_progression(input, limits);
        const auto &d = result.data();
        EXPECT_EQ(d.completed_prefix.size(), 1U);
        if (d.completed_prefix.size() == 1)
            expect_completion(d.completed_prefix[0], {"A", "old", 0, 0, "z"});
        EXPECT_EQ(d.newly_completed_begin, 1U);
        EXPECT_EQ(d.newly_completed_count, 0U);
        EXPECT_EQ(d.projection.completed_deficits.size(), 1U);
        EXPECT_EQ(d.projection.remaining_micro_points, 2U);
        EXPECT_TRUE(d.projection.previous);
        EXPECT_TRUE(d.projection.next);
        if (d.projection.previous)
            expect_target(*d.projection.previous, {"A", 0});
        if (d.projection.next)
            expect_target(*d.projection.next, {"B", 3});
        EXPECT_TRUE(d.projection.interval);
        if (d.projection.interval) {
            EXPECT_EQ(d.projection.interval->earned_micro_points, 1U);
            EXPECT_EQ(d.projection.interval->span_micro_points, 3U);
            EXPECT_EQ(d.projection.interval->fraction, Exact(1) / 3);
        }
    });
    limits.owned_bytes = 117;
    expect_failure(input, limits, Error::resource_limit);
}
TEST(MilestoneResources, DeclaredMaximumOptionsAreAccepted) {
    const MilestoneProgressionLimits limits{256, 4096, 4096, 16384, 8388608};
    expect_empty(advance_milestone_progression(base(), limits));
}
TEST(MilestoneResources, EveryOptionRejectsZeroMaxPlusOneAndSizeMaximum) {
    struct Option {
        const char *name;
        std::size_t MilestoneProgressionLimits::*member;
        std::size_t maximum;
    };
    const std::array options{
        Option{"reference_bytes", &MilestoneProgressionLimits::reference_bytes, 256},
        Option{"targets", &MilestoneProgressionLimits::targets, 4096},
        Option{"completed_prefix", &MilestoneProgressionLimits::completed_prefix, 4096},
        Option{"items", &MilestoneProgressionLimits::items, 16384},
        Option{"owned_bytes", &MilestoneProgressionLimits::owned_bytes, 8388608}};
    for (const auto &option : options) {
        for (const std::size_t value :
             {std::size_t{0}, option.maximum + 1, std::numeric_limits<std::size_t>::max()}) {
            SCOPED_TRACE(option.name);
            SCOPED_TRACE(value);
            MilestoneProgressionLimits limits;
            limits.*option.member = value;
            expect_failure(base(), limits, Error::invalid_options);
        }
    }
}

TEST(MilestoneValidation, FirstFailureOrderAppliesAcrossAllInputAndOutputPhases) {
    struct Row {
        const char *label;
        MilestoneProgressionInput input;
        MilestoneProgressionLimits limits;
        Error code;
    };
    auto option = base();
    option.track_ref.clear();
    option.progression_context_ref.assign(257, 'x');
    auto bytes = option;
    auto count = base();
    count.track_ref.clear();
    count.targets = {{"A", 0}, {"B", 1}};
    auto reference = base();
    reference.track_ref.clear();
    reference.targets = {{"A", 1}, {"A", 0}};
    auto duplicate = base();
    duplicate.targets = {{"A", 1}, {"A", 0}};
    duplicate.completed_prefix = {{"B", "old", 0, 0, "old"}};
    auto order = duplicate;
    order.targets[1].milestone_ref = "B";
    auto prefix = base(1);
    prefix.targets = {{"A", 0}, {"B", 1}};
    prefix.completed_prefix = {{"B", "old", 0, 0, "old"}};
    auto historical_reference = duplicate;
    historical_reference.completed_prefix[0].completion_context_ref.clear();
    const std::array rows{
        Row{"options-before-resource-and-empty",
            option,
            {256, 0, 4096, 16384, 8388608},
            Error::invalid_options},
        Row{"resource-before-earlier-empty", bytes, {}, Error::resource_limit},
        Row{"input-count-before-reference",
            count,
            {256, 1, 4096, 16384, 8388608},
            Error::resource_limit},
        Row{"empty-before-duplicate", reference, {}, Error::invalid_reference},
        Row{"duplicate-before-order-and-prefix", duplicate, {}, Error::duplicate_milestone},
        Row{"order-before-prefix", order, {}, Error::nonascending_targets},
        Row{"prefix-before-output-item-refusal",
            prefix,
            {256, 4096, 4096, 3, 8388608},
            Error::invalid_completed_prefix},
        Row{"captured-empty-before-duplicate", historical_reference, {}, Error::invalid_reference}};
    for (const auto &row : rows) {
        SCOPED_TRACE(row.label);
        expect_failure(row.input, row.limits, row.code);
    }
}

TEST(MilestoneOwnership, RetainsAllFieldsAfterInputMutationAndDestruction) {
    const auto result = [] {
        auto input = input_for(literal::rows[3]); // Independent MP04 half-interval.
        auto proposed = advance_milestone_progression(input);
        input.track_ref.assign("changed");
        input.definition_revision_ref.clear();
        input.progression_context_ref.clear();
        input.score_micro_points = minimum;
        input.targets.clear();
        input.completed_prefix.clear();
        return proposed;
    }();
    expect_result(result, literal::rows[3]);
}
TEST(MilestoneOwnership, LaterProposalDoesNotRewriteEarlierResult) {
    const auto first = advance_milestone_progression(input_for(literal::rows[3]));
    const auto second = advance_milestone_progression(input_for(literal::rows[11]));
    expect_result(first, literal::rows[3]);
    expect_result(second, literal::rows[11]);
}
TEST(MilestoneOwnership, CopyAndMoveKeepOwnedPrefixRangeAndProjection) {
    const auto original = advance_milestone_progression(input_for(literal::rows[7]));
    auto copied = original;
    auto moved = std::move(copied);
    expect_result(original, literal::rows[7]);
    expect_result(moved, literal::rows[7]);
    auto assigned = advance_milestone_progression(input_for(literal::rows[0]));
    assigned = original;
    expect_result(assigned, literal::rows[7]);
    assigned = std::move(moved);
    expect_result(assigned, literal::rows[7]);
    expect_result(original, literal::rows[7]);
}
TEST(MilestoneOwnership, RefusalDoesNotChangeEarlierResult) {
    const auto first = advance_milestone_progression(input_for(literal::rows[4]));
    auto invalid = base();
    invalid.targets = {{"A", 0}, {"A", 1}};
    expect_failure(invalid, {}, Error::duplicate_milestone);
    expect_result(first, literal::rows[4]);
}

} // namespace
