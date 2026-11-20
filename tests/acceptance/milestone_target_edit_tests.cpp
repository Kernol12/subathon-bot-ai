// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/milestone_target_edits.hpp>

namespace {
using namespace subathon::core;
using Status = MilestoneEditStatus;
using Target = MilestoneTarget;
constexpr auto high = std::numeric_limits<std::int64_t>::max();
constexpr auto low = std::numeric_limits<std::int64_t>::min();
constexpr MilestoneEditPolicy eligible{true, true, true, true, 7};

MilestoneProgressionInput active(std::int64_t score) {
    // The configured identity order deliberately differs from lexical order.
    return {"track",
            "original",
            "edit",
            score,
            {{"z-A", 100}, {"c-B", 200}, {"b-C", 300}, {"a-D", 400}},
            {{"z-A", "earned", 100, 120, "original-input"}}};
}

void interval(const MilestoneProjection &p, std::uint64_t remaining, std::uint64_t earned,
              std::uint64_t span, const Exact &fraction) {
    ASSERT_EQ(p.remaining_micro_points, remaining);
    ASSERT_TRUE(p.interval);
    EXPECT_EQ(p.interval->earned_micro_points, earned);
    EXPECT_EQ(p.interval->span_micro_points, span);
    EXPECT_EQ(p.interval->fraction, fraction);
}

TEST(MilestoneTargetEdits, SingleTargetProgressAndEquality) {
    const auto progress =
        plan_milestone_target_edits(active(160), std::array{Target{"c-B", 180}}, eligible);
    ASSERT_EQ(progress.status, Status::eligible);
    EXPECT_TRUE(progress.newly_completed.empty());
    interval(progress.before, 40, 60, 100, Exact(3) / 5);
    interval(progress.after, 20, 60, 80, Exact(3) / 4);

    const auto equality =
        plan_milestone_target_edits(active(175), std::array{Target{"c-B", 175}}, eligible);
    ASSERT_EQ(equality.status, Status::eligible);
    ASSERT_EQ(equality.newly_completed.size(), 1U);
    EXPECT_EQ(equality.newly_completed[0].milestone_ref, "c-B");
    EXPECT_EQ(equality.newly_completed[0].target_micro_points, 175);
    ASSERT_TRUE(equality.after.next);
    EXPECT_EQ(equality.after.next->milestone_ref, "b-C");
    interval(equality.after, 125, 0, 125, Exact(0));
}

TEST(MilestoneTargetEdits, StableIdentityRejectsInvalidPatchAndNoop) {
    const auto input = active(160);
    for (const auto &patch : {std::vector{Target{"c-B", 100}}, std::vector{Target{"c-B", 301}},
                              std::vector{Target{"unknown", 250}},
                              std::vector{Target{"c-B", 180}, Target{"c-B", 190}}}) {
        const auto rejected = plan_milestone_target_edits(input, patch, eligible);
        EXPECT_EQ(rejected.status, Status::invalid_patch);
        EXPECT_TRUE(rejected.newly_completed.empty());
    }
    EXPECT_EQ(plan_milestone_target_edits(input, std::array{Target{"c-B", 200}}, eligible).status,
              Status::no_change);
}

TEST(MilestoneTargetEdits, PatchAndSuffixFollowConfigurationAndPreflightWholeTail) {
    const std::array patch{Target{"a-D", 250}, Target{"b-C", 175}, Target{"c-B", 150}};
    const auto p = plan_milestone_target_edits(active(175), patch, eligible);
    ASSERT_EQ(p.status, Status::eligible);
    ASSERT_EQ(p.targets.size(), 4U);
    EXPECT_EQ(p.targets[0].milestone_ref, "z-A");
    EXPECT_EQ(p.targets[1].milestone_ref, "c-B");
    EXPECT_EQ(p.targets[2].milestone_ref, "b-C");
    EXPECT_EQ(p.targets[3].milestone_ref, "a-D");
    ASSERT_EQ(p.newly_completed.size(), 2U);
    EXPECT_EQ(p.newly_completed[0].milestone_ref, "c-B");
    EXPECT_EQ(p.newly_completed[0].target_micro_points, 150);
    EXPECT_EQ(p.newly_completed[1].milestone_ref, "b-C");
    EXPECT_EQ(p.newly_completed[1].target_micro_points, 175);
    interval(p.after, 75, 0, 75, Exact(0));

    auto exhausted = eligible;
    exhausted.durable_queue_tail = high - 1;
    const auto rejected = plan_milestone_target_edits(active(175), patch, exhausted);
    EXPECT_EQ(rejected.status, Status::order_exhausted);
}

TEST(MilestoneTargetEdits, CorrectionUsesCurrentTargetAndExactUnsignedDistance) {
    const auto p = plan_milestone_target_edits(active(130), std::array{Target{"z-A", 150}},
                                               {false, true, false, false, 7});
    ASSERT_EQ(p.status, Status::eligible);
    EXPECT_TRUE(p.newly_completed.empty());
    EXPECT_EQ(p.after.track_deficit_micro_points, 20U);
    ASSERT_EQ(p.after.completed_deficits.size(), 1U);
    EXPECT_EQ(p.after.completed_deficits[0].current_target.target_micro_points, 150);
    interval(p.after, 70, 0, 50, Exact(0));

    auto edge = active(low);
    edge.targets = {{"z-A", high - 1}};
    edge.completed_prefix = {{"z-A", "earned", high - 1, high - 1, "original-input"}};
    const auto exact = plan_milestone_target_edits(edge, std::array{Target{"z-A", high}}, eligible);
    ASSERT_EQ(exact.status, Status::eligible);
    EXPECT_EQ(exact.before.track_deficit_micro_points, 18'446'744'073'709'551'614ULL);
    EXPECT_EQ(exact.after.track_deficit_micro_points, 18'446'744'073'709'551'615ULL);
    EXPECT_FALSE(exact.after.next);
    EXPECT_FALSE(exact.after.interval);
}

TEST(MilestoneTargetEdits, MixedPatchNeedsBothCurrentGrants) {
    auto input = active(175);
    input.targets.resize(2);
    const std::array patch{Target{"z-A", 110}, Target{"c-B", 175}};
    for (const auto policy : {MilestoneEditPolicy{true, false, true, true, 7},
                              MilestoneEditPolicy{false, true, true, true, 7}}) {
        EXPECT_EQ(plan_milestone_target_edits(input, patch, policy).status, Status::denied);
    }
    const auto p = plan_milestone_target_edits(input, patch, eligible);
    ASSERT_EQ(p.status, Status::eligible);
    ASSERT_EQ(p.newly_completed.size(), 1U);
    EXPECT_EQ(p.newly_completed[0].milestone_ref, "c-B");
    EXPECT_FALSE(p.after.next);
    EXPECT_FALSE(p.after.interval);
}

TEST(MilestoneTargetEdits, BacklogRefusalDiffersFromFinishedSuppressionAndAutomationGate) {
    auto backlog = active(190);
    backlog.targets[1].target_micro_points = 180;
    EXPECT_EQ(plan_milestone_target_edits(backlog, std::array{Target{"c-B", 170}}, eligible).status,
              Status::old_reachable_backlog);

    const MilestoneEditPolicy disabled{true, true, false, false, 7};
    EXPECT_EQ(
        plan_milestone_target_edits(active(175), std::array{Target{"c-B", 175}}, disabled).status,
        Status::finished_progression_disabled);
    const auto suppressed =
        plan_milestone_target_edits(backlog, std::array{Target{"z-A", 150}}, disabled);
    ASSERT_EQ(suppressed.status, Status::eligible);
    EXPECT_TRUE(suppressed.suppressed);
    EXPECT_TRUE(suppressed.newly_completed.empty());
    ASSERT_TRUE(suppressed.after.next);
    EXPECT_EQ(suppressed.after.next->milestone_ref, "c-B");
    EXPECT_EQ(suppressed.after.remaining_micro_points, 0U);
    EXPECT_FALSE(suppressed.after.interval);

    const auto gated = plan_milestone_target_edits(active(175), std::array{Target{"c-B", 175}},
                                                   {true, true, true, false, 7});
    ASSERT_EQ(gated.status, Status::eligible);
    EXPECT_TRUE(gated.automation_gated);
    ASSERT_EQ(gated.newly_completed.size(), 1U);
    EXPECT_EQ(gated.newly_completed[0].milestone_ref, "c-B");
}
} // namespace
