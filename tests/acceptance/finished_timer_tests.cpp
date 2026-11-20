// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/finished_timer.hpp>
#include <subathon/core/milestone_progression.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <utility>

namespace subathon::core {
namespace {

using Refusal = FinishedRefusal;
using Life = TimerLifecycle;
using Phase = CountdownPhase;
using Grace = ReopenGrace;
using Choice = ReopenGroupChoice;
using Access = FinishedAccessState;
using Reached = DeadlineReached;
using Effect = EffectDisposition;
using Transition = TimerTransition;
using GroupDisposition = ReopenGroupDisposition;

constexpr TimerState active_main{Life::Active, Phase::Main, 50, 120'000, false};
constexpr TimerConfig immediate{TimerEndMode::Immediate, 120'000, GraceReuse::Fresh, std::nullopt};

class FinishedTimer : public ::testing::Test {
  protected:
    // Main50 mirrors the dev initializer. Clock, grants and selected version
    // identities are synthetic. Producer calls supply the observed timer values.
    FinishedSnapshot current{
        .scope = {"installation-a", "timer-a", "rev-finished"},
        .values =
            TimerEditorialValues{advance_timer(active_main, immediate, {50, 950, false}).state, 0,
                                 100, immediate},
        .observation = {"observe-950", 950, Reached::unknown},
        .pause = PauseState{},
        .policy = PostFinishPolicy{"policy-3", false, true},
        .mirror = std::nullopt,
        .score_micro_points = 0,
        .finish_history_ref = "finish-original",
        .groups = std::vector<FinishedGroup>{}};
    FinishedCommand reopen{current.scope, "reopen-op", current.observation.ref,
                           ReopenIntent{20, Grace::retain, {}}};
    FinishedAccess access{Access::allowed, reopen,      "actor-a", "session-a",
                          "permissions-9", "catalog-2", {}};
    ReopenPreview preview{};

    void use_editorial(TimerEditorialPayload payload, TimerEditorialValues values,
                       std::int64_t utc) {
        const auto proposal = project_timer_editorial_command(
            {{"before-editorial", values},
             {"editorial-op", "before-editorial", "actor-a", std::nullopt, payload},
             {TimerEditorialAccessState::allowed, "editorial-permissions"},
             {{0, utc, false}},
             utc});
        ASSERT_EQ(proposal.data().disposition, TimerEditorialDisposition::accepted);
        ASSERT_TRUE(proposal.data().accepted_after);
        current.values = proposal.data().accepted_after;
    }
};

class FinishedRefundHead : public FinishedTimer {
  protected:
    void SetUp() override {
        // Literal projection of w06ContinuationObservesFinish (ordinary-main-expiry)
        // and Mixed::check_review in sqlite_financial_refunds_tests.cpp. No writer is
        // opened here. Scope/config/capture binding are supplied synthetic facts.
        current.score_micro_points = 30'000'000;
        current.finish_history_ref = "finish-close";
        current.groups = std::vector<FinishedGroup>{
            {.occurrence = "achieved-35",
             .queue_order = 1,
             .committed = GroupCommit::pending,
             .active_holds =
                 std::vector<PendingHold>{{PendingHoldCause::refund_review, "refund-5"}},
             .capture_ref = "original-capture",
             .achieved_score_micro_points = 40'000'000,
             .jobs = {{"original-capture", "original-job", GroupJobState::queued, 1, 0}},
             .bonus_receipts = {},
             .presentation_ms = std::nullopt}};
        preview.pending = *current.groups;
    }
};

TEST_F(FinishedRefundHead, RecordsSupportAndGatesExactScoreAndAutomationIndependently) {
    // Prospective post-finish capture. Arrival precedes the authoritative finish.
    const FinishedSupport support{
        "receipt-61",
        "support-61",
        "source-61",
        940,
        "capture-61",
        {{"time-61", FinishedEffectKind::timer_credit, 50, EffectEligibility::eligible, {}},
         {"score-61",
          FinishedEffectKind::milestone_award,
          6'000'000,
          EffectEligibility::eligible,
          {}},
         {"ledger-61", FinishedEffectKind::accounting, 23, EffectEligibility::eligible, {}}},
        {{"say", FinishedActionKind::ordinary, "chat", "bot", {}, EffectEligibility::eligible, {}},
         {"extra",
          FinishedActionKind::milestone,
          "overlay",
          "server",
          {"score-61"},
          EffectEligibility::eligible,
          {}}}};
    struct Row {
        bool progression;
        bool automation;
        Effect score;
        Effect say;
        Effect extra;
        std::int64_t owner_award;
        std::uint64_t new_completions;
    };
    const Row cases[]{{false, false, Effect::progression_off, Effect::automation_off,
                       Effect::automation_off, 0, 0},
                      {false, true, Effect::progression_off, Effect::pass_to_owner,
                       Effect::trigger_suppressed, 0, 0},
                      {true, false, Effect::pass_to_owner, Effect::automation_off,
                       Effect::automation_off, 6'000'000, 1},
                      {true, true, Effect::pass_to_owner, Effect::pass_to_owner,
                       Effect::consider_in_group, 6'000'000, 1}};
    for (const auto &row : cases) {
        SCOPED_TRACE(::testing::Message() << row.progression << '/' << row.automation);
        current.policy = PostFinishPolicy{"policy-fixture", row.progression, row.automation};
        const auto result = project_finished_input(current, support);
        const auto *plan = std::get_if<FinishedInputProposal>(&result);
        ASSERT_NE(plan, nullptr);
        EXPECT_EQ(plan->scope, current.scope);
        EXPECT_EQ(plan->observation, current.observation);
        EXPECT_EQ(plan->policy, *current.policy);
        EXPECT_EQ(plan->retained, support);
        EXPECT_EQ(plan->effects, (std::vector{Effect::finished, row.score, Effect::pass_to_owner}));
        ASSERT_EQ(plan->actions.size(), 2U);
        EXPECT_EQ(plan->actions[0], row.say);
        if (!row.progression && !row.automation)
            EXPECT_TRUE(plan->actions[1] == Effect::automation_off ||
                        plan->actions[1] == Effect::trigger_suppressed);
        else
            EXPECT_EQ(plan->actions[1], row.extra);
        EXPECT_EQ(plan->main_added_ms, 0);
        EXPECT_EQ(plan->grace_consumed_ms, 0);
        EXPECT_EQ(plan->debt_repaid_ms, 0);
        EXPECT_EQ(plan->debt_added_ms, 0);
        ASSERT_EQ(plan->effects.size(), 3U);
        ASSERT_EQ(plan->retained.effects.size(), 3U);
        const auto award =
            plan->effects[1] == Effect::pass_to_owner ? plan->retained.effects[1].requested : 0;
        EXPECT_EQ(award, row.owner_award);
        // Separate shipped owner. The new target/configuration is prospective.
        const auto progression = advance_milestone_progression(
            {"track-a",
             "track-new",
             "support-61",
             30'000'000 + award,
             {{"target-35", 35'000'000}, {"peak-new", 35'000'001}},
             {{"target-35", "milestone-v1", 35'000'000, 40'000'000, "ctx"}}});
        EXPECT_EQ(progression.data().score_micro_points, 30'000'000 + row.owner_award);
        EXPECT_EQ(progression.data().newly_completed_count, row.new_completions);
        ASSERT_EQ(progression.data().completed_prefix.size(), 1 + row.new_completions);
        EXPECT_EQ(progression.data().completed_prefix.front().score_micro_points, 40'000'000);
        if (row.new_completions != 0) {
            EXPECT_EQ(progression.data().completed_prefix.back().milestone_ref, "peak-new");
            EXPECT_EQ(progression.data().completed_prefix.back().score_micro_points, 36'000'000);
        }
    }
}

TEST_F(FinishedTimer, SameObservationFinishKeepsBlockedAndUnknownEffectsOut) {
    const auto cut = advance_timer(active_main, immediate, {50, 950, false});
    ASSERT_EQ(cut.transitions, (std::vector{Transition::MainExpired, Transition::Finished}));
    current.values->timer = cut.state;
    current.policy = PostFinishPolicy{"allow-both", true, true};
    const FinishedSupport support{
        "receipt-62",
        "input-62",
        "arrival-62",
        950,
        "capture-62",
        {{"time", FinishedEffectKind::timer_credit, 50, EffectEligibility::eligible, {}},
         {"points", FinishedEffectKind::milestone_award, 6'000'000, EffectEligibility::held,
          "recovery-42"},
         {"ledger", FinishedEffectKind::accounting, 23, EffectEligibility::unknown, {}},
         {"duplicate", FinishedEffectKind::accounting, 17, EffectEligibility::duplicate, {}},
         {"invalid", FinishedEffectKind::accounting, 11, EffectEligibility::invalid, {}}},
        {{"after-time",
          FinishedActionKind::ordinary,
          "r1",
          "s1",
          {"time"},
          EffectEligibility::eligible,
          {}},
         {"unknown-authority",
          FinishedActionKind::ordinary,
          "r2",
          "s2",
          {},
          EffectEligibility::unknown,
          {}},
         {"held-action",
          FinishedActionKind::ordinary,
          "r3",
          "s3",
          {},
          EffectEligibility::held,
          "review-41"}}};
    const auto result = project_finished_input(current, support);
    const auto *plan = std::get_if<FinishedInputProposal>(&result);
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->retained, support);
    EXPECT_EQ(plan->effects, (std::vector{Effect::finished, Effect::held, Effect::unavailable,
                                          Effect::duplicate, Effect::invalid}));
    EXPECT_EQ(plan->actions,
              (std::vector{Effect::trigger_suppressed, Effect::unavailable, Effect::held}));
    EXPECT_EQ(plan->main_added_ms, 0);
}

TEST_F(FinishedTimer, NaturalGraceExhaustionRetainsZeroOrResetsConfiguredBudget) {
    current.values->config.end_mode = TimerEndMode::Grace;
    const auto finished = advance_timer(active_main, current.values->config, {120'050, 950, false});
    ASSERT_EQ(finished.transitions, (std::vector{Transition::MainExpired, Transition::GraceStarted,
                                                 Transition::GraceExpired, Transition::Finished}));
    current.values->timer = finished.state;
    for (const auto choice : {Grace::retain, Grace::reset}) {
        std::get<ReopenIntent>(reopen.intent).grace = choice;
        access.bound_command = reopen;
        const auto result = project_finished_command(current, reopen, access, preview);
        const auto *plan = std::get_if<FinishedCommandProposal>(&result);
        ASSERT_NE(plan, nullptr);
        ASSERT_TRUE(plan->after.values);
        EXPECT_EQ(plan->audit.before.timer.phase, Phase::Grace);
        EXPECT_EQ(plan->audit.before.timer.grace_remaining_ms, 0);
        EXPECT_EQ(plan->after.values->timer.lifecycle, Life::Active);
        EXPECT_EQ(plan->after.values->timer.phase, Phase::Main);
        EXPECT_EQ(plan->after.values->timer.main_remaining_ms, 20);
        EXPECT_EQ(plan->after.values->timer.grace_remaining_ms,
                  choice == Grace::retain ? 0 : 120'000);
        EXPECT_FALSE(plan->after.values->timer.awaiting_confirmation);
        EXPECT_EQ(plan->after.values->config.grace_reuse, GraceReuse::Fresh);
        EXPECT_EQ(plan->after.values->config.end_mode, TimerEndMode::Grace);
        EXPECT_EQ(plan->after.finish_history_ref, "finish-original");
        EXPECT_TRUE(plan->transitions.empty());
        EXPECT_TRUE(plan->groups.empty());
    }
}

TEST_F(FinishedTimer, ConfirmationFinishRequiresFreshExplicitGraceAndHonorsEditedCap) {
    current.values->config.end_mode = TimerEndMode::Confirmation;
    const auto wait = advance_timer(active_main, current.values->config, {50, 950, false});
    ASSERT_TRUE(wait.state.awaiting_confirmation);
    current.values->timer = confirm_timer_finish(wait.state);
    ASSERT_EQ(current.values->timer.lifecycle, Life::Finished);
    ASSERT_NO_FATAL_FAILURE(use_editorial(TimerCapReplacement{15, {}, {}}, *current.values, 950));
    ASSERT_EQ(current.values->maximum_remaining_ms, 15);
    std::get<ReopenIntent>(reopen.intent).grace.reset();
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::grace_choice_required);
    std::get<ReopenIntent>(reopen.intent).grace = Grace::retain;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::main_out_of_range);
    std::get<ReopenIntent>(reopen.intent).absolute_main_ms = 15;
    access.bound_command = reopen;
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    ASSERT_TRUE(plan->after.values);
    EXPECT_EQ(plan->after.values->timer.main_remaining_ms, 15);
    EXPECT_EQ(plan->after.values->maximum_remaining_ms, 15);
    EXPECT_EQ(plan->after.values->timer.grace_remaining_ms, 120'000);
    EXPECT_EQ(plan->after.values->config.end_mode, TimerEndMode::Confirmation);
    EXPECT_FALSE(plan->after.values->timer.awaiting_confirmation);
}

TEST_F(FinishedTimer, SharedGracePersistsAcrossReopenAndNextExpiry) {
    const TimerConfig shared{TimerEndMode::Grace, 120'000, GraceReuse::Shared, 200'000};
    const auto partial = advance_timer(active_main, shared, {120'027, 120'927, false});
    ASSERT_EQ(partial.state.grace_remaining_ms, 23);
    const auto finished = advance_timer(partial.state, shared, {79'073, 200'000, true});
    ASSERT_EQ(finished.transitions,
              (std::vector{Transition::HardEndReached, Transition::Finished}));
    ASSERT_NO_FATAL_FAILURE(
        use_editorial(TimerDeadlineReplacement{}, {finished.state, 0, 100, shared}, 200'001));
    current.observation.utc_ms = 200'001;
    std::get<ReopenIntent>(reopen.intent).absolute_main_ms = 30;
    for (const auto choice : {Grace::retain, Grace::reset}) {
        std::get<ReopenIntent>(reopen.intent).grace = choice;
        access.bound_command = reopen;
        const auto result = project_finished_command(current, reopen, access, preview);
        const auto *plan = std::get_if<FinishedCommandProposal>(&result);
        ASSERT_NE(plan, nullptr);
        ASSERT_TRUE(plan->after.values);
        EXPECT_EQ(plan->after.values->config.grace_reuse, GraceReuse::Shared);
        EXPECT_EQ(plan->after.values->timer.grace_remaining_ms,
                  choice == Grace::retain ? 23 : 120'000);
        const auto next = advance_timer(plan->after.values->timer, plan->after.values->config,
                                        {50, 200'051, false});
        EXPECT_EQ(next.state.phase, Phase::Grace);
        EXPECT_EQ(next.state.grace_remaining_ms, choice == Grace::retain ? 3 : 119'980);
    }
}

TEST_F(FinishedTimer, HardEndRequiresSeparateAcceptedEditAndFreshBinding) {
    ASSERT_NO_FATAL_FAILURE(
        use_editorial(TimerDeadlineReplacement{1'000}, {active_main, 0, 100, immediate}, 900));
    current.pause = set_pause_reason({}, PauseReason::Manual, true);
    current.pause = set_pause_reason(*current.pause, PauseReason::RecoveryReview, true);
    current.pause->client_disconnect_pending_ms = 5; // Synthetic independent signal evidence.
    const auto finished =
        advance_timer(current.values->timer, current.values->config, {100, 1'000, true});
    ASSERT_EQ(finished.transitions,
              (std::vector{Transition::HardEndReached, Transition::Finished}));
    ASSERT_EQ(finished.state.main_remaining_ms, 50);
    current.values->timer = finished.state;
    current.observation.utc_ms = 1'000;
    current.observation.current_setting = Reached::reached;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::deadline_passed);

    ASSERT_NO_FATAL_FAILURE(use_editorial(TimerDeadlineReplacement{2'000}, *current.values, 1'001));
    ASSERT_EQ(current.values->timer.lifecycle, Life::Finished);
    current.scope.revision = "rev-deadline-edit";
    current.observation = {"observe-1200", 1'200, Reached::not_reached};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);

    reopen.expected = current.scope;
    reopen.observation_ref = current.observation.ref;
    std::get<ReopenIntent>(reopen.intent).absolute_main_ms = 30;
    access.bound_command = reopen;
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    ASSERT_TRUE(plan->after.values);
    ASSERT_TRUE(plan->after.pause);
    EXPECT_EQ(plan->after.values->timer.lifecycle, Life::Active);
    EXPECT_EQ(plan->after.values->timer.main_remaining_ms, 30);
    EXPECT_EQ(plan->after.values->timer.grace_remaining_ms, 120'000);
    EXPECT_EQ(plan->after.values->config.hard_end_utc_ms, 2'000);
    EXPECT_EQ(plan->after.pause->reasons,
              (PauseReasons{PauseReason::Manual, PauseReason::RecoveryReview}));
    EXPECT_EQ(plan->after.pause->client_disconnect_pending_ms, 5);
    EXPECT_EQ(plan->after.finish_history_ref, "finish-original");
    EXPECT_EQ(plan->audit.observation, current.observation);
    EXPECT_EQ(plan->audit.before.timer.main_remaining_ms, 50);
    EXPECT_EQ(plan->audit.after.timer.main_remaining_ms, 30);
    EXPECT_TRUE(plan->transitions.empty());

    // Separate attempt on the still-Finished snapshot, one millisecond before and at equality.
    current.observation = {"observe-1999", 1'999, Reached::not_reached};
    reopen.observation_ref = current.observation.ref;
    access.bound_command = reopen;
    EXPECT_TRUE(std::holds_alternative<FinishedCommandProposal>(
        project_finished_command(current, reopen, access, preview)));
    current.observation = {"observe-2000", 2'000, Reached::unknown};
    reopen.observation_ref = current.observation.ref;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::deadline_passed);
}

TEST_F(FinishedTimer, DeadlineEditObservesOldDeadlineBeforeChangingTheSetting) {
    ASSERT_NO_FATAL_FAILURE(
        use_editorial(TimerDeadlineReplacement{1'000}, {active_main, 0, 100, immediate}, 900));
    ASSERT_EQ(current.values->timer.lifecycle, Life::Active);
    const auto replacement = project_timer_editorial_command(
        {{"before-replacement", *current.values},
         {"replace-deadline", "before-replacement", "actor-a", std::nullopt,
          TimerDeadlineReplacement{2'000}},
         {TimerEditorialAccessState::allowed, "editorial-permissions"},
         {{100, 1'000, true}},
         1'000});
    const auto &edit = replacement.data();
    ASSERT_EQ(edit.disposition, TimerEditorialDisposition::accepted);
    ASSERT_EQ(edit.observation_transitions.size(), 1U);
    const auto &observed = edit.observation_transitions.front();
    ASSERT_EQ(observed.count, 2);
    EXPECT_EQ(observed.entries[0], Transition::HardEndReached);
    EXPECT_EQ(observed.entries[1], Transition::Finished);
    EXPECT_EQ(edit.editorial_transitions.count, 0);
    ASSERT_TRUE(edit.observed_before);
    ASSERT_TRUE(edit.accepted_after);
    EXPECT_EQ(edit.observed_before->config.hard_end_utc_ms, 1'000);
    EXPECT_EQ(edit.observed_before->timer.lifecycle, Life::Finished);
    EXPECT_EQ(edit.accepted_after->config.hard_end_utc_ms, 2'000);
    EXPECT_EQ(edit.accepted_after->timer.lifecycle, Life::Finished);
    EXPECT_EQ(edit.accepted_after->timer.main_remaining_ms, 50);
    EXPECT_EQ(edit.accepted_after->timer.grace_remaining_ms, 120'000);

    current.values = edit.accepted_after;
    current.pause = set_pause_reason({}, PauseReason::Manual, true);
    current.scope.revision = "selected-replacement";
    current.observation = {"observed-at-edit", 1'000, Reached::not_reached};
    reopen.expected = current.scope;
    reopen.observation_ref = current.observation.ref;
    access.bound_command = reopen;
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    ASSERT_TRUE(plan->after.values);
    EXPECT_EQ(plan->audit.before.timer.lifecycle, Life::Finished);
    EXPECT_EQ(plan->after.values->timer.lifecycle, Life::Active);
    EXPECT_EQ(plan->after.values->config.hard_end_utc_ms, 2'000);
    ASSERT_TRUE(plan->after.pause);
    EXPECT_EQ(plan->after.pause->reasons, PauseReasons{PauseReason::Manual});
    EXPECT_TRUE(plan->transitions.empty());
}

TEST_F(FinishedTimer, ReachedEvidenceSurvivesUtcRegressionOrLoss) {
    // Natural Immediate finish at 950 can precede the same setting's deadline.
    current.values->config.hard_end_utc_ms = 1'000;
    struct Row {
        std::optional<std::int64_t> utc;
        Reached reached;
        Refusal expected;
    };
    const Row rows[]{{999, Reached::reached, Refusal::deadline_passed},
                     {std::nullopt, Reached::reached, Refusal::deadline_passed},
                     {std::nullopt, Reached::not_reached, Refusal::deadline_unknown},
                     {999, Reached::unknown, Refusal::deadline_unknown},
                     {1'001, Reached::unknown, Refusal::deadline_passed}};
    for (const auto &row : rows) {
        current.observation.utc_ms = row.utc;
        current.observation.current_setting = row.reached;
        EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
                  row.expected);
    }
    current.values->config.hard_end_utc_ms.reset();
    current.observation.utc_ms.reset();
    current.observation.current_setting = Reached::unknown;
    EXPECT_TRUE(std::holds_alternative<FinishedCommandProposal>(
        project_finished_command(current, reopen, access, preview)));
}

TEST_F(FinishedTimer, ReopenKeepsDebtForNextEligibleAddition) {
    ASSERT_NO_FATAL_FAILURE(use_editorial(TimerSignedDelta{-80, RefundDeficitPolicy::CarryTimeDebt},
                                          {active_main, 0, 100, immediate}, 950));
    ASSERT_EQ(current.values->timer.lifecycle, Life::Finished);
    ASSERT_EQ(current.values->time_debt_ms, 30);
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    ASSERT_TRUE(plan->after.values);
    EXPECT_EQ(plan->after.values->timer.main_remaining_ms, 20);
    EXPECT_EQ(plan->after.values->time_debt_ms, 30);
    EXPECT_EQ(plan->debt_repaid_ms, 0);
    const auto next = plan_time_addition(
        {plan->after.values->timer.main_remaining_ms, plan->after.values->time_debt_ms},
        {"later-support", 35}, plan->after.values->maximum_remaining_ms);
    EXPECT_EQ(next.debt_repaid_ms, 30);
    EXPECT_EQ(next.main_added_ms, 5);
    EXPECT_EQ(next.balance_after.main_remaining_ms, 25);
    EXPECT_EQ(next.balance_after.time_debt_ms, 0);

    // Same established debt on the original Finished branch; ordinary support stays finished.
    const FinishedSupport support{
        "receipt-late",
        "input-late",
        "online",
        951,
        "capture-late",
        {{"time-late", FinishedEffectKind::timer_credit, 35, EffectEligibility::eligible, {}}},
        {}};
    const auto post_finish = project_finished_input(current, support);
    const auto *effects = std::get_if<FinishedInputProposal>(&post_finish);
    ASSERT_NE(effects, nullptr);
    EXPECT_EQ(effects->retained, support);
    EXPECT_EQ(effects->effects, (std::vector{Effect::finished}));
    EXPECT_EQ(effects->debt_repaid_ms, 0);
    EXPECT_EQ(effects->debt_added_ms, 0);
    EXPECT_EQ(effects->main_added_ms, 0);
}

TEST_F(FinishedTimer, DebtCorrectionHasSeparateIntentAuditAndNoTimerEdit) {
    ASSERT_NO_FATAL_FAILURE(use_editorial(TimerSignedDelta{-80, RefundDeficitPolicy::CarryTimeDebt},
                                          {active_main, 0, 100, immediate}, 950));
    ASSERT_EQ(current.values->time_debt_ms, 30);
    // Enabled deadline with unavailable time and unknown group state do not gate correction.
    current.values->config.hard_end_utc_ms = 1'000;
    current.observation.utc_ms.reset();
    current.groups.reset();
    const FinishedCommand correction{current.scope, "correct-5", current.observation.ref,
                                     DebtCorrectionIntent{5, "correct operator entry"}};
    // The reopen access can express a perfectly valid grant, for another intent.
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, correction, access)),
              Refusal::stale_basis);
    access.bound_command = correction;
    const auto result = project_finished_command(current, correction, access);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    ASSERT_TRUE(plan->after.values);
    EXPECT_EQ(plan->audit.authority.bound_command, correction);
    EXPECT_EQ(plan->audit.authority.actor, "actor-a");
    EXPECT_EQ(plan->audit.authority.session, "session-a");
    EXPECT_EQ(plan->audit.authority.permission_revision, "permissions-9");
    EXPECT_EQ(plan->audit.authority.permission_catalog, "catalog-2");
    EXPECT_EQ(plan->audit.observation, current.observation);
    EXPECT_EQ(plan->audit.before.time_debt_ms, 30);
    EXPECT_EQ(plan->audit.after.time_debt_ms, 5);
    EXPECT_EQ(plan->after.values->time_debt_ms, 5);
    EXPECT_EQ(plan->after.values->timer.lifecycle, Life::Finished);
    EXPECT_EQ(plan->after.values->timer.main_remaining_ms, 0);
    EXPECT_EQ(plan->after.values->timer.grace_remaining_ms, 120'000);
    EXPECT_EQ(plan->after.values->config.hard_end_utc_ms, 1'000);
    EXPECT_EQ(plan->after.finish_history_ref, "finish-original");
    EXPECT_EQ(plan->debt_repaid_ms, 0);
    EXPECT_TRUE(plan->transitions.empty());
    EXPECT_TRUE(plan->groups.empty());

    current.values = plan->after.values;
    current.scope.revision = "selected-after-correction"; // Caller selects the committed revision.
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, correction, access)),
              Refusal::stale_basis);
}

TEST_F(FinishedTimer, ActiveDebtCorrectionAcceptsClearEqualAndSignedMaximum) {
    // Synthetic caller-selected post-reopen head from the frozen preserved-debt example.
    current.values->timer = {Life::Active, Phase::Main, 20, 120'000, false};
    current.values->time_debt_ms = 30;
    for (const auto debt :
         {std::int64_t{0}, std::int64_t{30}, std::numeric_limits<std::int64_t>::max()}) {
        SCOPED_TRACE(debt);
        const FinishedCommand command{current.scope, "absolute-debt", current.observation.ref,
                                      DebtCorrectionIntent{debt, "verified balance"}};
        access.bound_command = command;
        const auto result = project_finished_command(current, command, access);
        const auto *plan = std::get_if<FinishedCommandProposal>(&result);
        ASSERT_NE(plan, nullptr);
        ASSERT_TRUE(plan->after.values);
        EXPECT_EQ(plan->audit.before.time_debt_ms, 30);
        EXPECT_EQ(plan->audit.after.time_debt_ms, debt);
        EXPECT_EQ(plan->after.values->time_debt_ms, debt);
        EXPECT_EQ(plan->after.values->timer.main_remaining_ms, 20);
        EXPECT_EQ(plan->after.values->timer.grace_remaining_ms, 120'000);
        EXPECT_EQ(plan->after.values->timer.lifecycle, Life::Active);
        EXPECT_EQ(plan->debt_repaid_ms, 0);
    }
    for (const auto &intent :
         {DebtCorrectionIntent{-1, "verified balance"}, DebtCorrectionIntent{0, ""}}) {
        const FinishedCommand command{current.scope, "absolute-debt", current.observation.ref,
                                      intent};
        access.bound_command = command;
        EXPECT_EQ(std::get<Refusal>(project_finished_command(current, command, access)),
                  Refusal::invalid_input);
    }
}

TEST_F(FinishedTimer, DebtCorrectionChecksItsOwnAccessBeforeFactsAndRequiresStartedTimer) {
    FinishedCommand command{current.scope, "correct-ready", current.observation.ref,
                            DebtCorrectionIntent{5, "verified balance"}};
    access.bound_command = command;
    auto missing_values = current;
    missing_values.values.reset();
    access.state = Access::denied;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(missing_values, command, access)),
              Refusal::access_denied);
    access.state = Access::unknown;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(missing_values, command, access)),
              Refusal::access_unknown);
    access.state = Access::allowed;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(missing_values, command, access)),
              Refusal::facts_unavailable);

    current.values->timer = TimerState{};
    current.finish_history_ref.clear();
    current.scope.revision = "ready";
    command.expected = current.scope;
    access.bound_command = command;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, command, access)),
              Refusal::wrong_lifecycle);
}

TEST_F(FinishedTimer, ReopenRefusesInOrderWithoutExposingAfterState) {
    current.values->timer = active_main;
    current.values->config.hard_end_utc_ms = 1'000;
    current.observation.utc_ms = 1'000;
    current.pause.reset();
    reopen.expected.revision = "rev-stale";
    reopen.intent = ReopenIntent{0, {}, {{"extra", Choice::skip}}};
    access.bound_command = reopen;
    access.state = Access::denied;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::access_denied);
    access.state = Access::unknown;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::access_unknown);
    access.state = Access::allowed;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
    reopen.expected = current.scope;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::facts_unavailable);
    current.pause = PauseState{};
    // Select a coherent earlier Active observation ahead of the hard end.
    current.observation.utc_ms = 999;
    current.observation.current_setting = Reached::not_reached;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::wrong_lifecycle);
    const auto finished =
        advance_timer(current.values->timer, current.values->config, {1, 1'000, false});
    current.values->timer = finished.state;
    current.observation.utc_ms = 1'000;
    current.observation.current_setting = Reached::reached;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::deadline_passed);
    ASSERT_NO_FATAL_FAILURE(use_editorial(TimerDeadlineReplacement{}, *current.values, 1'001));
    current.scope.revision = "rev-disabled";
    current.observation = {"observe-1001", 1'001, Reached::unknown};
    reopen.expected = current.scope;
    reopen.observation_ref = current.observation.ref;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::grace_choice_required);
    std::get<ReopenIntent>(reopen.intent).grace = Grace::retain;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::main_out_of_range);
    std::get<ReopenIntent>(reopen.intent).absolute_main_ms = 30;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::invalid_group_choice);
}

TEST_F(FinishedTimer, AccessForOneIntentCannotAuthorizeAnotherOnTheSameRevision) {
    auto &intent = std::get<ReopenIntent>(reopen.intent);
    intent.absolute_main_ms = 21;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
    intent.absolute_main_ms = 20;
    intent.grace = Grace::reset;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
    intent.grace = Grace::retain;
    reopen.observation_ref = "older-observation";
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
}

TEST_F(FinishedTimer, CurrentMirrorCannotOverrideDeadlineOrSupplyMissingFacts) {
    current.values->config.hard_end_utc_ms = 1'000;
    current.observation.utc_ms = 1'000;
    // A mirror presented as current contradicts the current selected configuration.
    current.mirror = FinishedLifecycleMirror{
        current.scope.revision, current.values->timer, 0, 2'000, {}, false, true};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::facts_unavailable);
    current.mirror->deadline_utc_ms = 1'000;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::deadline_passed);
    current.values.reset();
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::facts_unavailable);
}

TEST_F(FinishedTimer, MissingGroupFactsDifferFromAuthoritativeEmptyPreview) {
    current.groups.reset();
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::facts_unavailable);
    current.groups = std::vector<FinishedGroup>{};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access)),
              Refusal::facts_unavailable);
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    EXPECT_TRUE(plan->groups.empty());
}

TEST_F(FinishedRefundHead, HeldSkipRefusesWithCurrentExactGrantsAndNeverClearsReview) {
    reopen.intent = ReopenIntent{30, Grace::retain, {{"achieved-35", Choice::skip}}};
    access.bound_command = reopen;
    access.group = {{{"achieved-35", Choice::skip}, Access::allowed}};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::group_held);
    ASSERT_TRUE(current.groups);
    ASSERT_TRUE(current.groups->front().active_holds);
    EXPECT_EQ(*current.groups->front().active_holds,
              (std::vector<PendingHold>{{PendingHoldCause::refund_review, "refund-5"}}));

    // Model a separate writer-owned hold resolution with Continue. The stale preview
    // still has the exact old hold key, so removal requires a fresh command and preview.
    current.groups->front().active_holds->clear();
    current.scope.revision = "rev-resolved";
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
    reopen.expected = current.scope;
    access.bound_command = reopen;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
    preview.pending = *current.groups;
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->groups,
              (std::vector<ReopenGroupProposal>{
                  {{"achieved-35", Choice::skip}, GroupDisposition::omit_pending, {}}}));
    EXPECT_EQ(plan->after.score_micro_points, 30'000'000);
    EXPECT_EQ(plan->after.finish_history_ref, "finish-close");
}

TEST_F(FinishedRefundHead, DebtCorrectionPreservesSeparateRefundReviewAndCapture) {
    const FinishedCommand correction{current.scope, "confirm-zero-debt", current.observation.ref,
                                     DebtCorrectionIntent{0, "reconciled independently"}};
    access.bound_command = correction;
    const auto result = project_finished_command(current, correction, access);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->audit.before.time_debt_ms, 0);
    EXPECT_EQ(plan->audit.after.time_debt_ms, 0);
    EXPECT_EQ(plan->after.groups, current.groups);
    EXPECT_EQ(plan->after.score_micro_points, 30'000'000);
    EXPECT_TRUE(plan->groups.empty());
}

TEST_F(FinishedTimer, GroupProposalsFollowDurableOrderAndAuditCanonicalChoices) {
    // Literal durable lane projection from milestone_group_restart_tests.cpp and
    // milestone_group_decisions_tests.cpp. Policy, clock and authority are synthetic.
    // The 350->250 Continue creates no hold. The 250->199 Review at target200
    // contributes refund-17; the pre-existing Defer retains accept-31.
    const std::vector<PendingHold> holds{{PendingHoldCause::acceptance_defer, "accept-31"},
                                         {PendingHoldCause::refund_review, "refund-17"}};
    const FinishedGroup first{"z-first",
                              1,
                              GroupCommit::released,
                              std::vector<PendingHold>{},
                              "add10",
                              150,
                              {{"add10", "job", GroupJobState::queued, 1, 0},
                               {"add10", "done", GroupJobState::delivered, 3, 2},
                               {"add10", "stranded", GroupJobState::uncertain, 3, 2}},
                              {"effect-first"},
                              2'500,
                              400};
    const FinishedGroup second{"a-second",
                               2,
                               GroupCommit::pending,
                               holds,
                               "negative_stop",
                               350,
                               {{"negative_stop", "job", GroupJobState::queued, 1, 0}},
                               {},
                               {},
                               0};
    const FinishedGroup third{"m-third",
                              3,
                              GroupCommit::pending,
                              std::vector<PendingHold>{},
                              "negative_stop",
                              350,
                              {},
                              {},
                              {},
                              0};
    const FinishedGroup empty{"b-empty",
                              4,
                              GroupCommit::pending,
                              std::vector<PendingHold>{},
                              "negative_stop",
                              350,
                              {},
                              {},
                              {},
                              0};
    current.groups = std::vector{first, second, third, empty};
    current.score_micro_points = 199;
    preview.pending = {second, third, empty};
    reopen.intent =
        ReopenIntent{30, Grace::retain, {{"b-empty", Choice::skip}, {"a-second", Choice::resume}}};
    access.bound_command = reopen;
    const std::vector<ReopenChoice> canonical{{"a-second", Choice::resume},
                                              {"m-third", Choice::keep_deferred},
                                              {"b-empty", Choice::skip}};
    access.group = {{{"a-second", Choice::resume}, Access::allowed},
                    {{"b-empty", Choice::skip}, Access::allowed}};

    // Current grants are sufficient, but the bound intent must include defaulted m-third.
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::stale_basis);
    std::get<ReopenIntent>(access.bound_command.intent).choices = canonical;
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->groups,
              (std::vector<ReopenGroupProposal>{
                  {{"a-second", Choice::resume}, GroupDisposition::ordinary_consideration, holds},
                  {{"m-third", Choice::keep_deferred}, GroupDisposition::deferred, {}},
                  {{"b-empty", Choice::skip}, GroupDisposition::omit_pending, {}}}));
    EXPECT_EQ(plan->after.groups, current.groups);
    EXPECT_EQ(plan->after.score_micro_points, 199);
    EXPECT_EQ(plan->after.policy, current.policy);
    EXPECT_EQ(plan->audit.authority.bound_command, access.bound_command);
    EXPECT_EQ(std::get<ReopenIntent>(plan->audit.authority.bound_command.intent).choices,
              canonical);
    EXPECT_EQ(plan->audit.authority.group, access.group);
    EXPECT_EQ(plan->audit.authority.actor, "actor-a");
    EXPECT_EQ(plan->audit.authority.session, "session-a");
    EXPECT_EQ(plan->audit.authority.permission_revision, "permissions-9");
    EXPECT_EQ(plan->audit.authority.permission_catalog, "catalog-2");
    EXPECT_TRUE(plan->transitions.empty());

    // Revoked Resume wins over an obsolete preview in which the queued sibling
    // incorrectly made a committed release look pending.
    auto old = first;
    old.committed = GroupCommit::pending;
    auto stale = preview;
    stale.pending.insert(stale.pending.begin(), old);
    access.group.front().state = Access::denied;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, stale)),
              Refusal::access_denied);
    access.group.front().state = Access::allowed;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, stale)),
              Refusal::stale_basis);

    std::swap(stale.pending[1], stale.pending[2]);
    stale.pending.erase(stale.pending.begin());
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, stale)),
              Refusal::stale_basis);

    // With the current valid preview, selecting a released occurrence is invalid.
    reopen.intent = ReopenIntent{30,
                                 Grace::retain,
                                 {{"a-second", Choice::keep_deferred},
                                  {"m-third", Choice::keep_deferred},
                                  {"b-empty", Choice::keep_deferred},
                                  {"z-first", Choice::resume}}};
    access.bound_command = reopen;
    access.group = {{{"z-first", Choice::resume}, Access::allowed}};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::invalid_group_choice);
}

TEST_F(FinishedRefundHead, DuplicateChoiceAndUnavailableDecisionAreDistinct) {
    reopen.intent = ReopenIntent{
        30, Grace::retain, {{"achieved-35", Choice::resume}, {"achieved-35", Choice::resume}}};
    access.bound_command = reopen;
    access.group = {{{"achieved-35", Choice::resume}, Access::allowed}};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::invalid_group_choice);
    auto &choices = std::get<ReopenIntent>(reopen.intent).choices;
    choices.front().choice = Choice::skip;
    access.bound_command = reopen;
    access.group = {{{"achieved-35", Choice::skip}, Access::allowed},
                    {{"achieved-35", Choice::resume}, Access::allowed}};
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::invalid_group_choice);

    choices.pop_back();
    access.bound_command = reopen;
    access.group.clear();
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::group_held);

    choices.front().choice = Choice::resume;
    access.bound_command = reopen;
    access.group.clear();
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::group_decision_unavailable);
}

TEST_F(FinishedTimer, PreflightCountsAllChoicesAndAcceptsExactReferenceLimit) {
    auto oversized = reopen;
    std::get<ReopenIntent>(oversized.intent)
        .choices.assign(4'097, {"group", Choice::keep_deferred});
    access.state = Access::denied;
    access.bound_command = oversized;
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, oversized, access, preview)),
              Refusal::input_limit);

    access.state = Access::allowed;
    access.bound_command = reopen;
    access.permission_catalog.assign(4'096, 'c');
    const auto result = project_finished_command(current, reopen, access, preview);
    const auto *plan = std::get_if<FinishedCommandProposal>(&result);
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->audit.authority.permission_catalog, access.permission_catalog);
}

TEST_F(FinishedRefundHead, PreflightCoversEchoesAndHoldKeysBeforeAccessWithoutPartialResult) {
    access.state = Access::denied;
    access.permission_catalog = std::string(4'097, 'c');
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::input_limit);
    access.permission_catalog = "catalog-2";
    preview.pending.front().active_holds->front().source_ref = std::string(4'097, 'h');
    EXPECT_EQ(std::get<Refusal>(project_finished_command(current, reopen, access, preview)),
              Refusal::input_limit);

    const FinishedSupport support{"receipt", "input", "source", 950, "capture", {}, {}};
    FinishedLimits limits;
    limits.result_bytes = 1;
    EXPECT_EQ(std::get<Refusal>(project_finished_input(current, support, limits)),
              Refusal::result_limit);
}

} // namespace
} // namespace subathon::core
