// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include <subathon/core/downtime_recovery.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace subathon::core;

using Deadline = RecoveryDeadlineStatus;
using Gap = RecoveryGapUnavailable;
using ScheduleUnavailable = RecoveryScheduleUnavailable;
using Choice = RecoveryOfflineChoice;
using ReviewRefusal = RecoveryReviewRefusal;
using Transition = TimerTransition;

constexpr auto manual = PauseReason::Manual;
constexpr auto schedule = PauseReason::Schedule;
constexpr auto review = PauseReason::RecoveryReview;
constexpr auto reconstructed = RecoveryTransitionBasis::ReconstructedUtc;
constexpr auto observed = RecoveryTransitionBasis::ObservedUtc;
constexpr auto noon = duration_cast<milliseconds>(12h).count();
constexpr ScheduleZoneRules utc_zone{-4 * noon - 100'000, 4 * noon + 300'000, 0};

TimerState main_timer(std::int64_t main, std::int64_t grace = 120'000) {
    return {TimerLifecycle::Active, CountdownPhase::Main, main, grace, false};
}

TimerState grace_timer(std::int64_t grace) {
    return {TimerLifecycle::Active, CountdownPhase::Grace, 0, grace, false};
}

TimerState finished(TimerState timer) {
    timer.lifecycle = TimerLifecycle::Finished;
    return timer;
}

RecoveryTimerProposal at(TimerState timer, PauseState pause = {},
                         std::vector<RecoveryTransition> trace = {}) {
    return {timer, pause, trace};
}

std::vector<RecoveryTransition> hard_end(std::int64_t utc, RecoveryTransitionBasis basis) {
    return {{Transition::HardEndReached, {utc}, basis}, {Transition::Finished, {utc}, basis}};
}

DowntimeRecoveryInput complete(TimerState timer = main_timer(600'000),
                               std::int64_t checkpoint = noon,
                               std::int64_t restart = noon + 300'000) {
    return {
        .basis = {"lineage-A", "checkpoint-A", "config-A", "facts-A"},
        .timer = timer,
        .config = {},
        .pause = {},
        .checkpoint = {"checkpoint-observation", RecoveryUtcInstant{checkpoint}},
        .restart = {"restart-observation", RecoveryUtcInstant{restart}},
        .gap = RecoveryContinuousGap{"continuous-A", {checkpoint}, {restart}},
        .schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", {}, {}, {}},
    };
}

void set_restart(DowntimeRecoveryInput &input, std::int64_t utc) {
    input.restart.utc = RecoveryUtcInstant{utc};
    std::get<RecoveryContinuousGap>(input.gap).restart = {utc};
}

RecoveryReviewSnapshot pending(const DowntimeRecoveryInput &input, TimerState held,
                               PauseState pause, std::optional<RecoveryUtcInstant> decision_utc) {
    return {
        "episode-A",
        input.basis.timer_lineage,
        input.basis.facts_bundle_ref,
        {"r2", {held, 0, {}, input.config}},
        pause,
        {"decision-observation", decision_utc},
    };
}

RecoveryReviewCommand decide(const RecoveryReviewSnapshot &snapshot, Choice choice) {
    return {"operation-A",
            snapshot.episode_ref,
            snapshot.current.revision,
            snapshot.facts_bundle_ref,
            choice,
            {},
            "reviewed offline gap"};
}

RecoveryReviewAuthorization authorize(const RecoveryReviewSnapshot &snapshot,
                                      const RecoveryReviewCommand &command) {
    return {command,   snapshot.current.revision, snapshot.observed_at.observation_ref,
            "actor-A", "permissions-A",           {}};
}

const TimerEditorialAccess editorial_allowed{TimerEditorialAccessState::allowed, "editorial-A"};

void expect_timer(const TimerState &actual, const TimerState &expected) {
    EXPECT_EQ(actual.lifecycle, expected.lifecycle);
    EXPECT_EQ(actual.phase, expected.phase);
    EXPECT_EQ(actual.main_remaining_ms, expected.main_remaining_ms);
    EXPECT_EQ(actual.grace_remaining_ms, expected.grace_remaining_ms);
    EXPECT_EQ(actual.awaiting_confirmation, expected.awaiting_confirmation);
}

void expect_pause(const PauseState &actual, const PauseState &expected) {
    EXPECT_EQ(actual.reasons, expected.reasons);
    EXPECT_EQ(actual.stream_offline_pending_ms, expected.stream_offline_pending_ms);
    EXPECT_EQ(actual.client_disconnect_pending_ms, expected.client_disconnect_pending_ms);
    EXPECT_EQ(actual.stream_unknown_pending_ms, expected.stream_unknown_pending_ms);
    EXPECT_EQ(actual.stream_unknown_notice, expected.stream_unknown_notice);
}

void expect_proposal(const RecoveryTimerProposal &actual, const RecoveryTimerProposal &expected) {
    expect_timer(actual.timer, expected.timer);
    expect_pause(actual.pause, expected.pause);
    ASSERT_EQ(actual.transitions.size(), expected.transitions.size());
    for (std::size_t i = 0; i < expected.transitions.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(actual.transitions[i].transition, expected.transitions[i].transition);
        EXPECT_EQ(actual.transitions[i].utc, expected.transitions[i].utc);
        EXPECT_EQ(actual.transitions[i].basis, expected.transitions[i].basis);
    }
}

void expect_basis(const RecoveryBasis &actual, const RecoveryBasis &expected) {
    EXPECT_EQ(actual.timer_lineage, expected.timer_lineage);
    EXPECT_EQ(actual.checkpoint_ref, expected.checkpoint_ref);
    EXPECT_EQ(actual.configuration_revision, expected.configuration_revision);
    EXPECT_EQ(actual.facts_bundle_ref, expected.facts_bundle_ref);
}

template <typename Refusal, typename... Alternatives>
void expect_refusal(const std::variant<Alternatives...> &result, Refusal expected) {
    const auto *refusal = std::get_if<Refusal>(&result);
    ASSERT_NE(refusal, nullptr) << "Variant index: " << result.index();
    EXPECT_EQ(*refusal, expected);
}

void expect_preview(const DowntimeRecoveryInput &input, const RecoveryTimerProposal &freeze,
                    const std::optional<RecoveryTimerProposal> &count,
                    const std::optional<RecoveryTimerProposal> &hold = {},
                    Deadline deadline = Deadline::Disabled, std::optional<Gap> gap = {},
                    std::optional<ScheduleUnavailable> schedule_gap = {}) {
    const auto result = preview_downtime_recovery(input);
    const auto *preview = std::get_if<DowntimeRecoveryPreview>(&result);
    ASSERT_NE(preview, nullptr) << "Preview refused";
    expect_basis(preview->basis, input.basis);
    EXPECT_EQ(preview->policy, input.policy);
    EXPECT_EQ(preview->deadline_at_restart, deadline);
    EXPECT_EQ(preview->gap_unavailable, gap);
    EXPECT_EQ(preview->schedule_unavailable, schedule_gap);
    {
        SCOPED_TRACE("Freeze");
        expect_proposal(preview->freeze, freeze);
    }
    EXPECT_EQ(preview->count_elapsed.has_value(), count.has_value());
    if (preview->count_elapsed && count) {
        SCOPED_TRACE("Count elapsed");
        expect_proposal(*preview->count_elapsed, *count);
    }
    EXPECT_EQ(preview->review_hold.has_value(), hold.has_value());
    if (preview->review_hold && hold) {
        SCOPED_TRACE("Review hold");
        expect_proposal(*preview->review_hold, *hold);
    }
}

void expect_wait(const RecoveryReviewSnapshot &snapshot, const RecoveryTimerProposal &held,
                 Deadline deadline) {
    const auto result = observe_recovery_review_wait(snapshot);
    const auto *wait = std::get_if<RecoveryReviewWait>(&result);
    ASSERT_NE(wait, nullptr) << "Wait observation refused";
    EXPECT_EQ(wait->deadline, deadline);
    expect_proposal(wait->held, held);
}

void expect_command(const RecoveryReviewCommand &actual, const RecoveryReviewCommand &expected) {
    EXPECT_EQ(actual.operation_ref, expected.operation_ref);
    EXPECT_EQ(actual.expected_episode_ref, expected.expected_episode_ref);
    EXPECT_EQ(actual.expected_revision, expected.expected_revision);
    EXPECT_EQ(actual.expected_facts_bundle_ref, expected.expected_facts_bundle_ref);
    EXPECT_EQ(actual.choice, expected.choice);
    EXPECT_EQ(actual.reason, expected.reason);
    ASSERT_EQ(actual.correction.has_value(), expected.correction.has_value());
    if (expected.correction) {
        EXPECT_EQ(actual.correction->requested_ms, expected.correction->requested_ms);
        EXPECT_EQ(actual.correction->negative_policy, expected.correction->negative_policy);
    }
}

void expect_resolution(const DowntimeRecoveryInput &input, const RecoveryReviewSnapshot &snapshot,
                       const RecoveryReviewCommand &command,
                       const RecoveryReviewAuthorization &authorization,
                       const RecoveryTimerProposal &selected, const TimerState &timer,
                       const PauseState &pause,
                       const std::optional<TimerEditorialAdjustment> &adjustment = {}) {
    const auto result = resolve_recovery_review(input, snapshot, command, authorization);
    const auto *resolution = std::get_if<RecoveryReviewResolution>(&result);
    ASSERT_NE(resolution, nullptr) << "Resolution refused";
    expect_command(resolution->command, command);
    expect_command(resolution->authorization.command, authorization.command);
    EXPECT_EQ(resolution->authorization.revision, authorization.revision);
    EXPECT_EQ(resolution->authorization.observation_ref, authorization.observation_ref);
    EXPECT_EQ(resolution->authorization.actor_ref, authorization.actor_ref);
    EXPECT_EQ(resolution->authorization.permission_revision, authorization.permission_revision);
    const auto &access = resolution->authorization.correction_access;
    EXPECT_EQ(access.has_value(), authorization.correction_access.has_value());
    if (access && authorization.correction_access) {
        EXPECT_EQ(access->state, authorization.correction_access->state);
        EXPECT_EQ(access->permission_revision,
                  authorization.correction_access->permission_revision);
    }
    expect_basis(resolution->basis, input.basis);
    EXPECT_EQ(resolution->decision_observation.observation_ref,
              snapshot.observed_at.observation_ref);
    EXPECT_EQ(resolution->decision_observation.utc, snapshot.observed_at.utc);
    expect_proposal(resolution->selected_at_restart, selected);
    expect_timer(resolution->resolved.timer, timer);
    expect_pause(resolution->pause, pause);

    const auto &values = resolution->resolved;
    const auto &current = snapshot.current.values;
    EXPECT_EQ(values.maximum_remaining_ms, current.maximum_remaining_ms);
    EXPECT_EQ(values.time_debt_ms, current.time_debt_ms); // All resolution vectors use zero debt.
    EXPECT_EQ(values.config.end_mode, current.config.end_mode);
    EXPECT_EQ(values.config.grace_duration_ms, current.config.grace_duration_ms);
    EXPECT_EQ(values.config.grace_reuse, current.config.grace_reuse);
    EXPECT_EQ(values.config.hard_end_utc_ms, current.config.hard_end_utc_ms);
    EXPECT_EQ(resolution->correction_transitions.count, 0); // No accepted vector expires a phase.
    ASSERT_EQ(resolution->correction.has_value(), adjustment.has_value());
    if (adjustment) {
        const auto &actual = *resolution->correction;
        EXPECT_EQ(actual.requested_ms, adjustment->requested_ms);
        EXPECT_EQ(actual.main_adjustment_ms, adjustment->main_adjustment_ms);
        EXPECT_EQ(actual.debt_repaid_ms, adjustment->debt_repaid_ms);
        EXPECT_EQ(actual.debt_added_ms, adjustment->debt_added_ms);
        EXPECT_EQ(actual.cap_limited_ms, adjustment->cap_limited_ms);
        EXPECT_EQ(actual.unapplied_ms, adjustment->unapplied_ms);
    }
}

TEST(DowntimeRecovery, R01DefaultFreezeRestoresDurableMain) {
    const auto input = complete();
    ASSERT_EQ(input.policy, DowntimePolicy::Freeze);
    // 12:00 to 12:05 is 300,000 ms. Only Count deducts it from the durable 600,000.
    expect_preview(input, at(main_timer(600'000)), at(main_timer(300'000)));
}

TEST(DowntimeRecovery, R01ReviewHoldsDurableMainAndOffersBothAlternatives) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    expect_preview(input, at(main_timer(600'000)), at(main_timer(300'000)),
                   at(main_timer(600'000), {{review}}));
}

TEST(DowntimeRecovery, R02ManualPauseCoversOfflineGap) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    input.pause = {{manual}};
    expect_preview(input, at(main_timer(600'000), {{manual}}), at(main_timer(600'000), {{manual}}),
                   at(main_timer(600'000), {{manual, review}}));
}

TEST(DowntimeRecovery, R02WaitPreservesCurrentReasonsNoticeAndPendingClientDelay) {
    const auto input = complete();
    const PauseState held{{manual, review, PauseReason::StreamStatusUnknown}, {}, 4'000, {}, true};
    const auto snapshot =
        pending(input, main_timer(600'000), held, RecoveryUtcInstant{noon + 420'000});
    expect_wait(snapshot, at(main_timer(600'000), held), Deadline::Disabled);
}

TEST(DowntimeRecovery, R02CountClearsReviewAloneAfterTwoMinutesOfWaiting) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    input.pause = {{manual}};
    const PauseState held{{manual, review, PauseReason::StreamStatusUnknown}, {}, 4'000, {}, true};
    const auto snapshot =
        pending(input, main_timer(600'000), held, RecoveryUtcInstant{noon + 420'000});
    const auto command = decide(snapshot, Choice::CountElapsed);
    expect_resolution(input, snapshot, command, authorize(snapshot, command),
                      at(main_timer(600'000), {{manual}}), main_timer(600'000),
                      {{manual, PauseReason::StreamStatusUnknown}, {}, 4'000, {}, true});
}

TEST(DowntimeRecovery, R03HardEndFinishesPausedGraceAtRestart) {
    auto input = complete(grace_timer(75'000), 0, 90'000);
    input.config.hard_end_utc_ms = 90'000;
    input.pause = {{manual}};
    const auto ended = finished(grace_timer(75'000));
    const auto expected = at(ended, {{manual}}, hard_end(90'000, observed));
    expect_preview(input, expected, expected, {}, Deadline::Reached);
}

TEST(DowntimeRecovery, R03WaitObservesHardEndDespiteManualAndReview) {
    auto input = complete(grace_timer(75'000), 0, 60'000);
    input.config.hard_end_utc_ms = 90'000;
    const PauseState held{{manual, review}};
    const auto snapshot = pending(input, grace_timer(75'000), held, RecoveryUtcInstant{90'000});
    expect_wait(snapshot, at(finished(grace_timer(75'000)), held, hard_end(90'000, observed)),
                Deadline::Reached);
}

TEST(DowntimeRecovery, R03FreezeResolutionKeepsActualFinishedGraceAndManual) {
    auto input = complete(grace_timer(75'000), 0, 60'000);
    input.policy = DowntimePolicy::Review;
    input.config.hard_end_utc_ms = 90'000;
    input.pause = {{manual}};
    const auto ended = finished(grace_timer(75'000));
    const auto snapshot = pending(input, ended, {{manual, review}}, RecoveryUtcInstant{95'000});
    const auto command = decide(snapshot, Choice::Freeze);
    expect_resolution(input, snapshot, command, authorize(snapshot, command),
                      at(grace_timer(75'000), {{manual}}), ended, {{manual}});
}

TEST(DowntimeRecovery, R04ScheduleUnionAndNaturalPhaseCuts) {
    auto input = complete(main_timer(60'000), 0, 125'000);
    input.config.grace_duration_ms = 50'000;
    input.pause = {{schedule}};
    const std::array retained{
        RecoveryRetainedPause{"retired", {ScheduleLocalTime{-30s}}, {-30'000, 20'000}, {}},
    };
    const std::array rules{RecoveryRecurringPause{"next", {.local_time = 70s}, 25s, {}}};
    input.schedule =
        RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, retained};

    // [20,000,70,000) uses 50,000 Main; [95,000,105,000) uses the last 10,000.
    const std::vector<RecoveryTransition> into_grace{
        {Transition::MainExpired, {105'000}, reconstructed},
        {Transition::GraceStarted, {105'000}, reconstructed},
    };
    {
        SCOPED_TRACE("restart during Grace");
        expect_preview(input, at(main_timer(60'000)), at(grace_timer(30'000), {}, into_grace));
    }
    set_restart(input, 160'000);
    const std::vector<RecoveryTransition> through_finish{
        {Transition::MainExpired, {105'000}, reconstructed},
        {Transition::GraceStarted, {105'000}, reconstructed},
        {Transition::GraceExpired, {155'000}, reconstructed},
        {Transition::Finished, {155'000}, reconstructed},
    };
    {
        SCOPED_TRACE("restart after Grace");
        expect_preview(input, at(main_timer(60'000)),
                       at(finished(grace_timer(0)), {}, through_finish));
    }

    // A new window at the finish would pause an Active timer at restart. The
    // finished Count proposal must keep the bit from before the finishing cut.
    const std::array with_finish_edge{
        rules.front(),
        RecoveryRecurringPause{"at-finish", {.local_time = 155s}, 25s, {}},
    };
    std::get<RecoveryScheduleCoverage>(input.schedule).schedules = with_finish_edge;
    expect_preview(input, at(main_timer(60'000), {{schedule}}),
                   at(finished(grace_timer(0)), {}, through_finish));
}

TEST(DowntimeRecovery, R05CapturedNominalDeliverySuppressesReplayAndRetainedWindowsOverlap) {
    auto input = complete(main_timer(100'000), 0, 60'000);
    input.pause = {{schedule}};
    // At +40 seconds, local Jan 1 00:00:10 was UTC -30 seconds. The same
    // nominal key under the captured UTC zone would otherwise start at +10 seconds.
    const std::array delivered{
        ScheduleOccurrenceId{ScheduleLocalTime{10s}, ScheduleFoldRun::First}};
    const std::array retained{
        RecoveryRetainedPause{"shifted", delivered.front(), {-30'000, 30'000}, {}},
    };
    std::array rules{
        RecoveryRecurringPause{"shifted", {.local_time = 10s}, 60s, delivered},
        RecoveryRecurringPause{"independent", {.local_time = 20s}, 20s, {}},
    };
    input.schedule =
        RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, retained};

    // [0,30,000) and [20,000,40,000) union to [0,40,000), leaving 20,000 debit.
    expect_preview(input, at(main_timer(100'000)), at(main_timer(80'000)));

    // A separate coherent checkpoint with no delivery admits [10,000,70,000),
    // containing the independent window. Only [0,10,000) counts, and Schedule is set.
    input.basis.facts_bundle_ref = "facts-no-prior-delivery";
    input.pause = {};
    rules.front().delivered = {};
    input.schedule =
        RecoveryScheduleCoverage{"schedule-control", "delivery-empty", utc_zone, rules, {}};
    expect_preview(input, at(main_timer(100'000), {{schedule}}),
                   at(main_timer(90'000), {{schedule}}));
}

TEST(DowntimeRecovery, R06DeadlineTieUsesDMinusOneAndPreemptsScheduleOpening) {
    auto input = complete(main_timer(100), 0, 200);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 100;
    const std::array rules{RecoveryRecurringPause{"at-D", {.local_time = 100ms}, 150ms, {}}};
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, {}};
    // Elapsed strictly before D is 99 ms. The opening at D has no effect on Count.
    expect_preview(input, at(finished(main_timer(100)), {}, hard_end(200, observed)),
                   at(finished(main_timer(1)), {}, hard_end(100, reconstructed)), {},
                   Deadline::Reached);
}

TEST(DowntimeRecovery, R06NaturalExpiryStrictlyBeforeDeadlineWins) {
    auto input = complete(main_timer(100), 0, 200);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 101;
    expect_preview(input, at(finished(main_timer(100)), {}, hard_end(200, observed)),
                   at(finished(main_timer(0)), {},
                      {{Transition::MainExpired, {100}, reconstructed},
                       {Transition::Finished, {100}, reconstructed}}),
                   {}, Deadline::Reached);
}

TEST(DowntimeRecovery, R06ScheduleClosingAtDeadlineKeepsPreFinishPause) {
    auto input = complete(main_timer(100), 0, 200);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 100;
    const std::array rules{RecoveryRecurringPause{"before-D", {.local_time = 99ms}, 1ms, {}}};
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, {}};
    // [0,99) counts. Schedule opens at 99 and is still the preceding reason at D.
    expect_preview(input, at(finished(main_timer(100)), {}, hard_end(200, observed)),
                   at(finished(main_timer(1)), {{schedule}}, hard_end(100, reconstructed)), {},
                   Deadline::Reached);
}

TEST(DowntimeRecovery, R07SharedGraceFreshGraceAndConfirmationAfterSchedule) {
    auto input = complete(main_timer(20, 8), 0, 40);
    input.config.grace_reuse = GraceReuse::Shared;
    input.config.grace_duration_ms = 12;
    const std::array rules{RecoveryRecurringPause{"middle", {.local_time = 15ms}, 10ms, {}}};
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, {}};
    // Main takes [0,15) and [25,30). Shared Grace expires eight milliseconds later.
    const std::vector<RecoveryTransition> shared{
        {Transition::MainExpired, {30}, reconstructed},
        {Transition::GraceStarted, {30}, reconstructed},
        {Transition::GraceExpired, {38}, reconstructed},
        {Transition::Finished, {38}, reconstructed},
    };
    expect_preview(input, at(main_timer(20, 8)), at(finished(grace_timer(0)), {}, shared));
    input.config.grace_reuse = GraceReuse::Fresh;
    expect_preview(input, at(main_timer(20, 8)),
                   at(grace_timer(2), {},
                      {{Transition::MainExpired, {30}, reconstructed},
                       {Transition::GraceStarted, {30}, reconstructed}}));

    input.config.end_mode = TimerEndMode::Confirmation;
    auto confirming = main_timer(0, 8);
    confirming.awaiting_confirmation = true;
    expect_preview(input, at(main_timer(20, 8)),
                   at(confirming, {},
                      {{Transition::MainExpired, {30}, reconstructed},
                       {Transition::ConfirmationRequired, {30}, reconstructed}}));
}

TEST(DowntimeRecovery, R08CountExcludesWaitAndEditorialAdditionUsesCurrentCap) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    const PauseState current{{review, PauseReason::DesignatedClientDisconnected}, 3'000};
    auto snapshot =
        pending(input, main_timer(600'000), current, RecoveryUtcInstant{noon + 420'000});
    snapshot.current.values.maximum_remaining_ms = 325'000;
    auto command = decide(snapshot, Choice::CountElapsed);
    expect_resolution(input, snapshot, command, authorize(snapshot, command),
                      at(main_timer(300'000)), main_timer(300'000),
                      {{PauseReason::DesignatedClientDisconnected}, 3'000});
    command.correction = TimerSignedDelta{40'000, {}};
    auto authorization = authorize(snapshot, command);
    authorization.correction_access = editorial_allowed;
    // Offline leaves 300,000. The current cap permits 25,000 of the 40,000 request.
    expect_resolution(input, snapshot, command, authorization, at(main_timer(300'000)),
                      main_timer(325'000), {{PauseReason::DesignatedClientDisconnected}, 3'000},
                      TimerEditorialAdjustment{40'000, 25'000, 0, 0, 15'000, 0});
}

TEST(DowntimeRecovery, R08DeniedOrUnknownCorrectionRefusesWholeResolution) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    const auto snapshot = pending(input, main_timer(600'000),
                                  {{review, PauseReason::DesignatedClientDisconnected}, 3'000},
                                  RecoveryUtcInstant{noon + 420'000});
    auto command = decide(snapshot, Choice::CountElapsed);
    command.correction = TimerSignedDelta{40'000, {}};
    auto authorization = authorize(snapshot, command);
    for (const auto &[access, disposition] :
         {std::pair{TimerEditorialAccessState::denied, TimerEditorialDisposition::access_denied},
          std::pair{TimerEditorialAccessState::unknown,
                    TimerEditorialDisposition::access_unknown}}) {
        SCOPED_TRACE(static_cast<int>(access));
        authorization.correction_access = TimerEditorialAccess{access, {}};
        const auto result = resolve_recovery_review(input, snapshot, command, authorization);
        const auto *refusal = std::get_if<RecoveryCorrectionRefusal>(&result);
        ASSERT_NE(refusal, nullptr);
        EXPECT_EQ(refusal->editorial_disposition, disposition);
    }
}

TEST(DowntimeRecovery, R09CountFallbackHoldsFinishedAfterTrustedHardEnd) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.config.hard_end_utc_ms = noon + 120'000;
    const auto ended = finished(main_timer(600'000));
    const auto trace = hard_end(noon + 300'000, observed);
    for (const auto gap : {Gap::Discontinuous, Gap::Unknown}) {
        SCOPED_TRACE(static_cast<int>(gap));
        input.gap = gap;
        expect_preview(input, at(ended, {}, trace), {}, at(ended, {{review}}, trace),
                       Deadline::Reached, gap);
    }
}

TEST(DowntimeRecovery, R09CountUnavailableOnFinishedHoldAfterBindingAndWait) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.config.hard_end_utc_ms = noon + 120'000;
    input.gap = Gap::Discontinuous;
    const auto snapshot = pending(input, finished(main_timer(600'000)), {{review}},
                                  RecoveryUtcInstant{noon + 420'000});
    const auto command = decide(snapshot, Choice::CountElapsed);
    expect_refusal(resolve_recovery_review(input, snapshot, command, authorize(snapshot, command)),
                   ReviewRefusal::ChoiceUnavailable);
}

TEST(DowntimeRecovery, R09ExplicitFreezeResolvesFinishedFallback) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.config.hard_end_utc_ms = noon + 120'000;
    input.gap = Gap::Discontinuous;
    const auto ended = finished(main_timer(600'000));
    const auto snapshot = pending(input, ended, {{review}}, RecoveryUtcInstant{noon + 420'000});
    const auto command = decide(snapshot, Choice::Freeze);
    expect_resolution(input, snapshot, command, authorize(snapshot, command),
                      at(ended, {}, hard_end(noon + 300'000, observed)), ended, {});
}

TEST(DowntimeRecovery, R10BackwardAndMissingRestartUtcSelectHoldAndDistinguishDeadline) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.config.hard_end_utc_ms = noon + 120'000;
    input.restart.utc = RecoveryUtcInstant{noon - 60'000};
    input.gap = Gap::Discontinuous;
    input.schedule = ScheduleUnavailable::Coverage;
    input.pause = {{}, 2'000, 4'000};
    const PauseState held{{review}, 2'000, 4'000};
    expect_preview(input, at(main_timer(600'000), input.pause), {}, at(main_timer(600'000), held),
                   Deadline::NotReached, Gap::Discontinuous, ScheduleUnavailable::Coverage);

    input.restart.utc.reset();
    input.gap = Gap::Unknown;
    expect_preview(input, at(main_timer(600'000), input.pause), {}, at(main_timer(600'000), held),
                   Deadline::Unknown, Gap::Unknown, ScheduleUnavailable::Coverage);
}

TEST(DowntimeRecovery, R10FreshBeforeDeadlineObservationPermitsExplicitFreeze) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.config.hard_end_utc_ms = noon + 120'000;
    input.restart.utc.reset();
    input.gap = Gap::Unknown;
    input.schedule = ScheduleUnavailable::Coverage;
    input.pause = {{}, 2'000, 4'000};
    const auto snapshot = pending(input, main_timer(600'000), {{review}, 2'000, 4'000},
                                  RecoveryUtcInstant{noon + 60'000});
    const auto freeze = decide(snapshot, Choice::Freeze);
    expect_resolution(input, snapshot, freeze, authorize(snapshot, freeze),
                      at(main_timer(600'000), {{}, 2'000, 4'000}), main_timer(600'000),
                      {{}, 2'000, 4'000});
    const auto count = decide(snapshot, Choice::CountElapsed);
    expect_refusal(resolve_recovery_review(input, snapshot, count, authorize(snapshot, count)),
                   ReviewRefusal::ChoiceUnavailable);
}

TEST(DowntimeRecovery, R10BackwardSpanAssertedAsContinuousIsInvalid) {
    auto input = complete();
    set_restart(input, noon - 60'000);
    input.schedule = ScheduleUnavailable::Coverage;
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::InvalidInput);
}

TEST(DowntimeRecovery, R11MissingDeliveryContextKeepsCheckpointScheduleAndSelectsHold) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.pause = {{schedule}};
    input.schedule = ScheduleUnavailable::DeliveryContext;
    expect_preview(input, at(main_timer(600'000), {{schedule}}), {},
                   at(main_timer(600'000), {{schedule, review}}), Deadline::Disabled, {},
                   ScheduleUnavailable::DeliveryContext);
}

TEST(DowntimeRecovery, R11ExplicitFreezeAndAuthorizedCorrectionWithMissingDeliveryContext) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.pause = {{schedule}};
    input.schedule = ScheduleUnavailable::DeliveryContext;
    const auto snapshot = pending(input, main_timer(600'000), {{schedule, review}},
                                  RecoveryUtcInstant{noon + 420'000});
    auto count = decide(snapshot, Choice::CountElapsed);
    count.correction = TimerSignedDelta{20'000, {}};
    // Choice is checked before the missing correction authority.
    expect_refusal(resolve_recovery_review(input, snapshot, count, authorize(snapshot, count)),
                   ReviewRefusal::ChoiceUnavailable);

    auto freeze = decide(snapshot, Choice::Freeze);
    freeze.correction = TimerSignedDelta{20'000, {}};
    auto authorization = authorize(snapshot, freeze);
    expect_refusal(resolve_recovery_review(input, snapshot, freeze, authorization),
                   ReviewRefusal::CorrectionUnavailable);
    authorization.correction_access = editorial_allowed;
    expect_resolution(input, snapshot, freeze, authorization, at(main_timer(600'000), {{schedule}}),
                      main_timer(620'000), {{schedule}},
                      TimerEditorialAdjustment{20'000, 20'000, 0, 0, 0, 0});
}

TEST(DowntimeRecovery, R11CompleteEmptyScheduleSelectsCountWithoutHold) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    expect_preview(input, at(main_timer(600'000)), at(main_timer(300'000)));
}

TEST(DowntimeRecovery, R11MalformedRetainedWindowAndContradictoryEndpointAreInvalid) {
    auto input = complete();
    const std::array malformed{
        RecoveryRetainedPause{"retired", {ScheduleLocalTime{noon * 1ms}}, {noon, noon}, {}},
    };
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", {}, {}, malformed};
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::InvalidInput);

    input = complete();
    std::get<RecoveryContinuousGap>(input.gap).restart.ms += 1;
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::InvalidInput);
}

TEST(DowntimeRecovery, R11DuplicateNominalDeliveryKeyIsInvalid) {
    auto input = complete(main_timer(600'000), 0, 300'000);
    // Dec 31 23:59:00, delivered before the checkpoint; its window has expired.
    const ScheduleOccurrenceId key{ScheduleLocalTime{-60s}, ScheduleFoldRun::First};
    const std::array duplicate{key, key};
    const std::array rules{
        RecoveryRecurringPause{"rule-A", {.local_time = 23h + 59min}, 1s, duplicate},
    };
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, {}};
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::InvalidInput);
}

TEST(DowntimeRecovery, R11OversizedRetainedSpanIsInputLimitBeforeProjection) {
    auto input = complete();
    // Each row belongs to a different retired rule and expired before checkpoint.
    std::vector<RecoveryRetainedPause> retained;
    for (int i = 0; i < 16'385; ++i) {
        retained.push_back(
            {"retired-" + std::to_string(i), {ScheduleLocalTime{-10s}}, {-10'000, -9'000}, {}});
    }
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", {}, {}, retained};
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::InputLimit);
}

TEST(DowntimeRecovery, R12BoundsAndStructurePrecedeAccess) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    const auto snapshot =
        pending(input, main_timer(600'000), {{manual, review}}, RecoveryUtcInstant{noon + 420'000});
    auto command = decide(snapshot, Choice::Freeze);
    command.operation_ref.assign(4'097, 'x');
    expect_refusal(
        resolve_recovery_review(input, snapshot, command, RecoveryReviewAccessRefusal::Denied),
        ReviewRefusal::InputLimit);

    command = decide(snapshot, Choice::Freeze);
    std::get<RecoveryContinuousGap>(input.gap).checkpoint.ms += 1;
    expect_refusal(
        resolve_recovery_review(input, snapshot, command, RecoveryReviewAccessRefusal::Denied),
        ReviewRefusal::InvalidInput);
}

TEST(DowntimeRecovery, R12AccessRefusalPrecedesStaleBinding) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    const auto snapshot =
        pending(input, main_timer(600'000), {{manual, review}}, RecoveryUtcInstant{noon + 420'000});
    auto command = decide(snapshot, Choice::Freeze);
    command.expected_revision = "r1";
    expect_refusal(
        resolve_recovery_review(input, snapshot, command, RecoveryReviewAccessRefusal::Denied),
        ReviewRefusal::AccessDenied);
    expect_refusal(
        resolve_recovery_review(input, snapshot, command, RecoveryReviewAccessRefusal::Unknown),
        ReviewRefusal::AccessUnknown);
}

TEST(DowntimeRecovery, R12StaleRevisionPrecedesMissingReviewAndDeadlineWait) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    input.config.hard_end_utc_ms = noon + 360'000;
    const auto snapshot =
        pending(input, main_timer(600'000), {{manual}}, RecoveryUtcInstant{noon + 420'000});
    auto replay = decide(snapshot, Choice::Freeze);
    replay.expected_revision = "r1";
    auto authorization = authorize(snapshot, replay);
    authorization.revision = "r1";
    expect_refusal(resolve_recovery_review(input, snapshot, replay, authorization),
                   ReviewRefusal::StaleReview);

    const auto current = decide(snapshot, Choice::Freeze);
    expect_refusal(resolve_recovery_review(input, snapshot, current, authorize(snapshot, current)),
                   ReviewRefusal::NoReviewPending);
}

TEST(DowntimeRecovery, R12WaitWithoutReviewIsTyped) {
    auto input = complete();
    input.config.hard_end_utc_ms = noon + 360'000;
    const auto snapshot =
        pending(input, main_timer(600'000), {{manual}}, RecoveryUtcInstant{noon + 420'000});
    expect_refusal(observe_recovery_review_wait(snapshot), RecoveryWaitRefusal::NoReviewPending);
}

TEST(DowntimeRecovery, R12StaleEpisodeFactsLineageAndHeldCountdownPrecedeDeadlineWait) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    input.config.hard_end_utc_ms = noon + 360'000;
    auto snapshot =
        pending(input, main_timer(600'000), {{manual, review}}, RecoveryUtcInstant{noon + 420'000});
    const auto current = decide(snapshot, Choice::Freeze);
    auto stale = current;
    stale.expected_episode_ref = "episode-old";
    expect_refusal(resolve_recovery_review(input, snapshot, stale, authorize(snapshot, stale)),
                   ReviewRefusal::StaleReview);
    stale = current;
    stale.expected_facts_bundle_ref = "facts-other";
    expect_refusal(resolve_recovery_review(input, snapshot, stale, authorize(snapshot, stale)),
                   ReviewRefusal::StaleReview);

    snapshot.timer_lineage = "lineage-other";
    expect_refusal(resolve_recovery_review(input, snapshot, current, authorize(snapshot, current)),
                   ReviewRefusal::StaleReview);
    snapshot.timer_lineage = input.basis.timer_lineage;
    snapshot.current.values.timer.main_remaining_ms = 600'001;
    expect_refusal(resolve_recovery_review(input, snapshot, current, authorize(snapshot, current)),
                   ReviewRefusal::StaleReview);
}

TEST(DowntimeRecovery, R12AuthorizationBindsRevisionObservationAndEveryCommandField) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    input.config.hard_end_utc_ms = noon + 360'000;
    const auto snapshot =
        pending(input, main_timer(600'000), {{manual, review}}, RecoveryUtcInstant{noon + 420'000});
    auto command = decide(snapshot, Choice::Freeze);
    command.correction = TimerSignedDelta{-20'000, RefundDeficitPolicy::StopAtZero};
    auto trusted = authorize(snapshot, command);
    trusted.correction_access = editorial_allowed;

    // Both signed corrections in these comparisons are individually well formed.
    auto stale = trusted;
    stale.command.operation_ref = "other-operation";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.expected_episode_ref = "episode-other";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.expected_revision = "different-command-revision";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.expected_facts_bundle_ref = "facts-other";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.revision = "r1";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.observation_ref = "earlier-observation";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.choice = Choice::CountElapsed;
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.correction = TimerSignedDelta{20'000, {}};
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.correction->negative_policy = RefundDeficitPolicy::CarryTimeDebt;
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.correction.reset();
    stale.correction_access.reset();
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.reason = "different review reason";
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
    stale = trusted;
    stale.command.reason.reset();
    expect_refusal(resolve_recovery_review(input, snapshot, command, stale),
                   ReviewRefusal::StaleReview);
}

TEST(DowntimeRecovery, R12CurrentDeadlineAndWaitPrecedeUnavailableChoiceAndCorrection) {
    auto input = complete();
    input.policy = DowntimePolicy::CountElapsed;
    input.gap = Gap::Unknown;
    const PauseState held{{manual, review}, {}, 4'000};
    auto snapshot = pending(input, main_timer(600'000), held, RecoveryUtcInstant{noon + 420'000});
    // Current config has a deadline that was disabled in the offline facts.
    snapshot.current.values.config.hard_end_utc_ms = noon + 360'000;
    auto command = decide(snapshot, Choice::CountElapsed);
    command.correction = TimerSignedDelta{20'000, {}};
    expect_refusal(resolve_recovery_review(input, snapshot, command, authorize(snapshot, command)),
                   ReviewRefusal::WaitObservationRequired);
    snapshot.observed_at = {"missing-utc-observation", {}};
    expect_refusal(resolve_recovery_review(input, snapshot, command, authorize(snapshot, command)),
                   ReviewRefusal::DeadlineUnknown);
    snapshot.observed_at = {"before-deadline-observation", RecoveryUtcInstant{noon + 350'000}};
    expect_refusal(resolve_recovery_review(input, snapshot, command, authorize(snapshot, command)),
                   ReviewRefusal::ChoiceUnavailable);
}

TEST(DowntimeRecovery, R12IndependentWaitObservesCurrentDeadlineWithoutDecisionAuthority) {
    const auto input = complete();
    const PauseState held{{manual, review}, {}, 4'000};
    auto snapshot = pending(input, main_timer(600'000), held, RecoveryUtcInstant{noon + 420'000});
    snapshot.current.values.config.hard_end_utc_ms = noon + 360'000;
    expect_wait(snapshot,
                at(finished(main_timer(600'000)), held, hard_end(noon + 420'000, observed)),
                Deadline::Reached);
    snapshot.observed_at = {"missing-utc-observation", {}};
    expect_wait(snapshot, at(main_timer(600'000), held), Deadline::Unknown);
}

TEST(DowntimeRecovery, R13WaitFinishesProvisionalMainAtObservedDecisionUtc) {
    auto input = complete();
    input.config.hard_end_utc_ms = noon + 360'000;
    const PauseState held{{manual, review}, {}, 4'000};
    const auto snapshot =
        pending(input, main_timer(600'000), held, RecoveryUtcInstant{noon + 420'000});
    expect_wait(snapshot,
                at(finished(main_timer(600'000)), held, hard_end(noon + 420'000, observed)),
                Deadline::Reached);
}

TEST(DowntimeRecovery, R13ResolutionRetainsActualFinishAndHistoricalCount) {
    auto input = complete();
    input.policy = DowntimePolicy::Review;
    input.config.hard_end_utc_ms = noon + 360'000;
    const auto ended = finished(main_timer(600'000));
    const auto snapshot =
        pending(input, ended, {{manual, review}, {}, 4'000}, RecoveryUtcInstant{noon + 420'000});
    auto command = decide(snapshot, Choice::CountElapsed);
    expect_resolution(input, snapshot, command, authorize(snapshot, command),
                      at(main_timer(300'000)), ended, {{manual}, {}, 4'000});

    command.correction = TimerSignedDelta{40'000, {}};
    auto authorization = authorize(snapshot, command);
    authorization.correction_access = editorial_allowed;
    expect_refusal(resolve_recovery_review(input, snapshot, command, authorization),
                   ReviewRefusal::CorrectionUnavailable);
}

TEST(DowntimeRecovery, R13BackwardWallClockDoesNotReopenAuthoritativeFinish) {
    auto input = complete();
    input.config.hard_end_utc_ms = noon + 360'000;
    const auto ended = finished(main_timer(600'000));
    const PauseState held{{review, PauseReason::StreamOffline}};
    const auto snapshot = pending(input, ended, held, RecoveryUtcInstant{noon + 350'000});
    expect_wait(snapshot, at(ended, held), Deadline::NotReached);
}

TEST(DowntimeRecovery, R14KeepsEstablishedOfflineAndUnadvancedClientDelay) {
    auto input = complete(main_timer(60'000), 0, 120'000);
    input.policy = DowntimePolicy::CountElapsed;
    input.pause = {{manual, PauseReason::StreamOffline}, {}, 4'000};
    const auto expected = at(main_timer(60'000), {{manual, PauseReason::StreamOffline}, {}, 4'000});
    expect_preview(input, expected, expected);
}

TEST(DowntimeRecovery, R14PendingUnknownAndDisconnectDelaysStayPendingWithoutPausing) {
    auto input = complete(main_timer(180'000), 0, 120'000);
    input.pause = {{}, {}, 4'000, 8'000, true};
    // Pending reasons were never established, so all 120,000 ms count.
    const PauseState pending_signals{{}, {}, 4'000, 8'000, true};
    expect_preview(input, at(main_timer(180'000), pending_signals),
                   at(main_timer(60'000), pending_signals));
}

TEST(DowntimeRecovery, R15EqualEndpointsRequireExplicitContinuityForZeroDebit) {
    auto input = complete(main_timer(600'000), noon, noon);
    input.policy = DowntimePolicy::CountElapsed;
    const auto unchanged = at(main_timer(600'000));
    expect_preview(input, unchanged, unchanged);
    input.gap = Gap::Unknown;
    expect_preview(input, unchanged, {}, at(main_timer(600'000), {{review}}), Deadline::Disabled,
                   Gap::Unknown);
}

TEST(DowntimeRecovery, R15SpanBeyondSigned64NeverWrapsIntoElapsedDebit) {
    constexpr auto first = std::numeric_limits<std::int64_t>::min();
    constexpr auto last = std::numeric_limits<std::int64_t>::max();
    auto input = complete(main_timer(600'000), first, last);
    input.policy = DowntimePolicy::CountElapsed;
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::InvalidInput);
    input.gap = Gap::Unrepresentable;
    expect_preview(input, at(main_timer(600'000)), {}, at(main_timer(600'000), {{review}}),
                   Deadline::Disabled, Gap::Unrepresentable);
}

TEST(DowntimeRecovery, R16SecondCrashCannotReplaceOriginalPendingEpisode) {
    auto input = complete();
    input.pause = {{manual, review}, {}, 4'000};
    expect_refusal(preview_downtime_recovery(input), RecoveryPreviewRefusal::ReviewAlreadyPending);
}

TEST(DowntimeRecovery, R17ScheduleAtRestartUsesHalfOpenWindow) {
    auto input = complete(main_timer(200), 0, 100);
    const std::array rules{RecoveryRecurringPause{"restart-edge", {.local_time = 100ms}, 50ms, {}}};
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", utc_zone, rules, {}};
    // The edge at 100 cannot retroactively pause [0,100).
    expect_preview(input, at(main_timer(200), {{schedule}}), at(main_timer(100), {{schedule}}));
    set_restart(input, 150);
    // [100,150) remains paused, but the Schedule bit clears exactly at 150.
    expect_preview(input, at(main_timer(200)), at(main_timer(100)));
}

TEST(DowntimeRecovery, R18FallbackSecondRunCountsOnlyTheUnpausedInterveningSpan) {
    constexpr auto day = 2026y / October / 25;
    constexpr auto midnight = duration_cast<milliseconds>(sys_days{day}.time_since_epoch()).count();
    constexpr auto checkpoint = midnight + 31 * 60'000;
    constexpr auto restart = midnight + 91 * 60'000;
    const std::array zone_changes{ScheduleZoneTransition{midnight + 60 * 60'000, 3'600}};
    const ScheduleZoneRules zone{midnight - 3 * 86'400'000, midnight + 3 * 86'400'000, 7'200,
                                 zone_changes};

    // Sunday 02:30 is 00:30Z at +02:00 and 01:30Z at +01:00.
    const std::array delivered{
        ScheduleOccurrenceId{local_days{day} + 2h + 30min, ScheduleFoldRun::First}};
    const std::array retained{
        RecoveryRetainedPause{
            "sunday", delivered.front(), {midnight + 30 * 60'000, midnight + 32 * 60'000}, {}},
    };
    std::array rules{
        RecoveryRecurringPause{"sunday",
                               {.local_time = 2h + 30min,
                                .weekdays = {false, false, false, false, false, false, true},
                                .fold_policy = ScheduleFoldPolicy::RunTwice},
                               2min,
                               delivered}};
    auto input = complete(main_timer(4'000'000), checkpoint, restart);
    input.pause = {{schedule}};
    input.schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", zone, rules, retained};
    // [00:32,01:30) counts 58 minutes (3,480,000 ms). The second window is active.
    expect_preview(input, at(main_timer(4'000'000), {{schedule}}),
                   at(main_timer(520'000), {{schedule}}));

    rules.front().recurrence.fold_policy = ScheduleFoldPolicy::RunOnce;
    input.basis.configuration_revision = "config-once";
    input.basis.facts_bundle_ref = "facts-once";
    // Only the First window was admitted. [00:32,01:31) counts 3,540,000 ms.
    expect_preview(input, at(main_timer(4'000'000)), at(main_timer(460'000)));
}

TEST(DowntimeRecovery, R19RestartDeadlineUsesObservedUtcAfterDMinusOneDebit) {
    auto input = complete(main_timer(100), 0, 100);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 100;
    expect_preview(input, at(finished(main_timer(100)), {}, hard_end(100, observed)),
                   at(finished(main_timer(1)), {}, hard_end(100, observed)), {}, Deadline::Reached);
}

TEST(DowntimeRecovery, R19CheckpointDeadlineUsesCheckpointObservationAndZeroDebit) {
    auto input = complete(main_timer(100), 0, 200);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 0;
    const auto ended = finished(main_timer(100));
    expect_preview(input, at(ended, {}, hard_end(200, observed)),
                   at(ended, {}, hard_end(0, observed)), {}, Deadline::Reached);
}

TEST(DowntimeRecovery, R19NaturalExpiryExactlyAtObservedRestartStillUsesReconstructedUtc) {
    auto input = complete(main_timer(100), 0, 100);
    input.config.end_mode = TimerEndMode::Immediate;
    expect_preview(input, at(main_timer(100)),
                   at(finished(main_timer(0)), {},
                      {{Transition::MainExpired, {100}, reconstructed},
                       {Transition::Finished, {100}, reconstructed}}));
}

} // namespace
