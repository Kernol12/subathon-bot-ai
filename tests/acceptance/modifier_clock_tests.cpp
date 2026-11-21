// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include <subathon/core/finished_timer.hpp>
#include <subathon/core/modifier_clock.hpp>

#include <array>
#include <chrono>
#include <string_view>
#include <utility>

namespace {
using namespace subathon::core;
using namespace std::chrono_literals;

using Life = TimerLifecycle;
using Phase = CountdownPhase;
using T = TimerTransition;
using Event = ModifierRuleEvent;
using Mode = ModifierClockMode;
using Timing = ModifierTriggerTiming;
using Policy = DowntimePolicy;
using Choice = RecoveryOfflineChoice;
using Basis = RecoveryTransitionBasis;

constexpr auto manual = PauseReason::Manual;
constexpr auto review = PauseReason::RecoveryReview;
constexpr auto schedule = PauseReason::Schedule;
constexpr std::string_view score_target = "milestone-score";

TimerState main_seed(std::int64_t main) {
    // Synthetic S output, matching the positive Main seed admitted unchanged by
    // ReferenceIntake::make_initial_work. The process anchor is supplied below.
    return {Life::Active, Phase::Main, main, 120'000, false};
}

TimerState grace_seed(std::int64_t grace) {
    return advance_timer(main_seed(1000), {}, {121'000 - grace, 121'000 - grace, false}).state;
}

ModifierClockObservation at(std::string ref, std::int64_t utc) {
    return {"lineage-A", std::move(ref), utc};
}

ModifierTarget score(std::string factor, std::optional<std::string> qualifier = {}) {
    return {ModifierTargetKind::milestone_score, std::string(score_target), std::move(qualifier),
            std::move(factor)};
}

ModifierClockValue exact_head(const ModifierInstance &revision, Mode mode = Mode::ActiveSubathon) {
    return {revision, revision.remaining_ms, mode};
}

ModifierClockValue activate_seed(std::int64_t duration, std::vector<ModifierTarget> targets,
                                 std::string id, Mode mode = Mode::ActiveSubathon) {
    const ModifierDefinition definition{"definition-" + id, "def-1", duration, std::move(targets)};
    const ModifierActivation request{
        "instance-" + id, "revision-1", {ModifierActivationKind::rule, "rule-seed"}, {}};
    return exact_head(
        propose_modifier_activation(definition, {}, request).revision_to_append.value(), mode);
}

ModifierClockValue active_seed(std::int64_t duration, bool also_score = false) {
    std::vector<ModifierTarget> targets{{ModifierTargetKind::counter, "counter-A", {}, "2"}};
    if (also_score)
        targets.push_back(score("2"));
    return activate_seed(duration, std::move(targets), "active");
}

ModifierClockValue real_seed(std::int64_t duration) {
    return activate_seed(duration, {{ModifierTargetKind::counter, "counter-E", {}, "2"}}, "real",
                         Mode::RealElapsed);
}

ModifierClockValue score_seed(std::int64_t duration, Mode mode = Mode::ActiveSubathon) {
    return activate_seed(duration, {score("2")}, "score", mode);
}

void expect_timer(const TimerState &state, Life life, Phase phase, std::int64_t main,
                  std::int64_t grace = 120'000, bool confirmation = false) {
    EXPECT_EQ(state.lifecycle, life);
    EXPECT_EQ(state.phase, phase);
    EXPECT_EQ(state.main_remaining_ms, main);
    EXPECT_EQ(state.grace_remaining_ms, grace);
    EXPECT_EQ(state.awaiting_confirmation, confirmation);
}

void expect_head(const std::optional<ModifierClockValue> &head, Mode mode, std::int64_t remaining) {
    ASSERT_TRUE(head);
    EXPECT_EQ(head->mode, mode);
    EXPECT_EQ(head->remaining_ms, remaining);
}

struct LiveCut {
    ModifierClockObservation before;
    ModifierClockObservation now;
    TimerState timer_before;
    TimerClockStep step;
    TimerAdvance timer;

    [[nodiscard]] ModifierCutProposal
    project(const std::optional<ModifierClockValue> &head,
            const std::optional<ModifierActivationAtCut> &activation = {}) const {
        return project_modifier_cut(before, now, timer_before, step, timer, head, activation);
    }
};

LiveCut cut(const ModifierClockObservation &before, const ModifierClockObservation &now,
            const TimerState &timer, std::int64_t elapsed, const PauseState &pause = {},
            const TimerConfig &config = {}) {
    const TimerClockStep step{elapsed, now.utc_ms, !pause.reasons.empty()};
    return {before, now, timer, step, advance_timer(timer, config, step)};
}

ModifierRuleCandidate expiry(const ModifierClockObservation &now, std::string instance,
                             std::optional<RecoveryUtcInstant> crossing = {}) {
    return {Event::Expired, now, std::move(instance), "revision-1", crossing};
}

void expect_cut(const ModifierCutProposal &proposal, Mode mode, std::int64_t remaining,
                const std::vector<ModifierCutEvent> &events = {}) {
    expect_head(proposal.head, mode, remaining);
    EXPECT_EQ(proposal.events, events);
    EXPECT_FALSE(proposal.activation);
}

ModifierScoreArrival arrival(const ModifierClockObservation &observation, std::string id,
                             bool post_finish_modifiers = false, bool score_progression = false,
                             bool visible = true) {
    return {observation, std::move(id),         score_target,      10'000'000,
            {},          post_finish_modifiers, score_progression, visible};
}

void expect_score(const ModifierScoreBasis &basis, std::optional<std::int64_t> requested,
                  std::optional<std::string_view> revision = {},
                  const PauseReasons &blockers = {}) {
    EXPECT_EQ(basis.blockers, blockers);
    if (revision) {
        ASSERT_TRUE(basis.selection);
        EXPECT_EQ(basis.selection->instance_revision_ref, *revision);
        EXPECT_EQ(basis.selection->target.kind, ModifierTargetKind::milestone_score);
        EXPECT_EQ(basis.selection->target.target_ref, score_target);
    } else if (requested) {
        // The requested base score should have no modifier provenance.
        EXPECT_FALSE(basis.selection);
    }
    const auto proposal = request_modifier_score(basis);
    EXPECT_TRUE(proposal.record_input);
    EXPECT_EQ(proposal.requested_micro_points, requested);
}

ModifierScoreBasis capture(const ModifierScoreArrival &input, const TimerState &timer,
                           const PauseState &pause, const std::optional<ModifierClockValue> &head) {
    const auto basis = capture_modifier_score(input, timer, pause, head);
    EXPECT_EQ(basis.observation, input.observation);
    EXPECT_EQ(basis.input_ref, input.input_ref);
    EXPECT_EQ(basis.base_micro_points, 10'000'000);
    return basis;
}

FinishedInputProposal finished_owner(const ModifierScoreArrival &input, const TimerState &timer,
                                     const PauseState &pause, const TimerConfig &config) {
    const FinishedSnapshot current{
        .scope = {"installation-A", input.observation.timer_lineage, "finished-revision"},
        .values = TimerEditorialValues{timer, 0, {}, config},
        .observation = {input.observation.ref, input.observation.utc_ms, DeadlineReached::unknown},
        .pause = pause,
        .policy = PostFinishPolicy{"finished-policy", input.post_finish_score_progression, false},
        .mirror = {},
        .score_micro_points = 0,
        .finish_history_ref = "finished-history",
        .groups = {},
    };
    const FinishedSupport support{
        "receipt-A",
        input.input_ref,
        "source-A",
        input.observation.utc_ms,
        "capture-A",
        {{"ordinary-time", FinishedEffectKind::timer_credit, 10, EffectEligibility::eligible, {}},
         {"ordinary-score",
          FinishedEffectKind::milestone_award,
          10'000'000,
          EffectEligibility::eligible,
          {}}},
        {}};
    return std::get<FinishedInputProposal>(project_finished_input(current, support));
}

TEST(ModifierClock, L1MainThroughGraceKeepsBothClocksProgressing) {
    const auto first = cut(at("start", 0), at("main-999", 999), main_seed(1000), 999);
    expect_timer(first.timer.state, Life::Active, Phase::Main, 1);
    EXPECT_TRUE(first.timer.transitions.empty());
    // First stub: valid Main cut and two independent #215 initial captures.
    const auto a1 = first.project(active_seed(5000));
    const auto e1 = first.project(real_seed(5000));
    expect_cut(a1, Mode::ActiveSubathon, 4001);
    expect_cut(e1, Mode::RealElapsed, 4001);

    const auto edge = cut(first.now, at("grace-start", 1000), first.timer.state, 1);
    expect_timer(edge.timer.state, Life::Active, Phase::Grace, 0, 120'000);
    EXPECT_EQ(edge.timer.transitions, (std::vector{T::MainExpired, T::GraceStarted}));
    const auto a2 = edge.project(a1.head);
    const auto e2 = edge.project(e1.head);
    expect_cut(a2, Mode::ActiveSubathon, 4000, {T::MainExpired, T::GraceStarted});
    expect_cut(e2, Mode::RealElapsed, 4000, {T::MainExpired, T::GraceStarted});

    const auto last = cut(edge.now, at("grace-250", 1250), edge.timer.state, 250);
    expect_timer(last.timer.state, Life::Active, Phase::Grace, 0, 119'750);
    EXPECT_TRUE(last.timer.transitions.empty());
    expect_cut(last.project(a2.head), Mode::ActiveSubathon, 3750);
    expect_cut(last.project(e2.head), Mode::RealElapsed, 3750);
}

TEST(ModifierClock, L2ClearingManualAffectsOnlyTheNextCut) {
    const auto pause = set_pause_reason({}, manual, true);
    const auto first =
        cut(at("grace-10", 120990), at("paused-4", 120994), grace_seed(10), 4, pause);
    // First stub: T supplied positive Grace and P supplied Manual for the full cut.
    const auto a1 = first.project(active_seed(5));
    const auto e1 = first.project(real_seed(5));
    expect_timer(first.timer.state, Life::Active, Phase::Grace, 0, 10);
    expect_cut(a1, Mode::ActiveSubathon, 5);
    expect_cut(e1, Mode::RealElapsed, 1);

    const auto edge = cut(first.now, at("clear-manual", 120995), first.timer.state, 1, pause);
    const auto a2 = edge.project(a1.head);
    const auto e2 = edge.project(e1.head);
    const auto cleared = set_pause_reason(pause, manual, false);
    EXPECT_TRUE(cleared.reasons.empty());
    expect_timer(edge.timer.state, Life::Active, Phase::Grace, 0, 10);
    expect_cut(a2, Mode::ActiveSubathon, 5);
    expect_cut(e2, Mode::RealElapsed, 0, {expiry(edge.now, "instance-real")});

    const auto last = cut(edge.now, at("resumed", 120996), edge.timer.state, 1, cleared);
    expect_timer(last.timer.state, Life::Active, Phase::Grace, 0, 9);
    expect_cut(last.project(a2.head), Mode::ActiveSubathon, 4);
    expect_cut(last.project(e2.head), Mode::RealElapsed, 0);
}

TEST(ModifierClock, L3FinishInsideCutDebitsOnlyProgressingPartForActive) {
    const auto finish = cut(at("grace-1", 120999), at("observed-finish", 121002), grace_seed(1), 3);
    // First stub: T consumes the final 1 ms of Grace in a qualified 3 ms cut.
    const auto a1 = finish.project(active_seed(5));
    const auto e1 = finish.project(real_seed(5));
    expect_timer(finish.timer.state, Life::Finished, Phase::Grace, 0, 0);
    EXPECT_EQ(finish.timer.transitions, (std::vector{T::GraceExpired, T::Finished}));
    expect_cut(a1, Mode::ActiveSubathon, 4, {T::GraceExpired, T::Finished});
    expect_cut(e1, Mode::RealElapsed, 2, {T::GraceExpired, T::Finished});

    const auto last = cut(finish.now, at("observed-expiry", 121012), finish.timer.state, 10);
    expect_timer(last.timer.state, Life::Finished, Phase::Grace, 0, 0);
    expect_cut(last.project(a1.head), Mode::ActiveSubathon, 4);
    expect_cut(last.project(e1.head), Mode::RealElapsed, 0, {expiry(last.now, "instance-real")});
}

TEST(ModifierClock, L4ConfirmationWaitAndInstantRescue) {
    const TimerConfig config{.end_mode = TimerEndMode::Confirmation, .hard_end_utc_ms = {}};
    const auto entered = advance_timer(main_seed(3), config, {3, 3, false});
    const auto wait = cut(at("confirmation", 3), at("rescue", 8), entered.state, 5, {}, config);
    // First stub: T reached Confirmation with no independent pause reason.
    const auto a1 = wait.project(active_seed(8));
    const auto e1 = wait.project(real_seed(8));
    EXPECT_EQ(entered.transitions, (std::vector{T::MainExpired, T::ConfirmationRequired}));
    expect_timer(wait.timer.state, Life::Active, Phase::Main, 0, 120'000, true);
    expect_cut(a1, Mode::ActiveSubathon, 8);
    expect_cut(e1, Mode::RealElapsed, 3);

    const auto rescued = apply_applicable_time_credit(wait.timer.state, 4);
    expect_timer(rescued, Life::Active, Phase::Main, 4);
    const LiveCut rescue{wait.now, wait.now, wait.timer.state, {0, 8, false}, {rescued, {}}};
    const auto a2 = rescue.project(a1.head);
    const auto e2 = rescue.project(e1.head);
    expect_cut(a2, Mode::ActiveSubathon, 8);
    expect_cut(e2, Mode::RealElapsed, 3);

    const auto last = cut(rescue.now, at("running-again", 9), rescue.timer.state, 1, {}, config);
    expect_timer(last.timer.state, Life::Active, Phase::Main, 3);
    expect_cut(last.project(a2.head), Mode::ActiveSubathon, 7);
    expect_cut(last.project(e2.head), Mode::RealElapsed, 2);
}

void hard_end_cut(bool manually_paused, std::int64_t prior_main, std::int64_t active_remaining) {
    const TimerConfig config{.hard_end_utc_ms = 1000};
    const auto pause = set_pause_reason({}, manual, manually_paused);
    const auto first =
        cut(at("start", 998), at("before-deadline", 999), main_seed(100), 1, pause, config);
    // L5/L6 first stub: one admitted millisecond before the deadline with P's current reason set.
    const auto a1 = first.project(active_seed(2, manually_paused));
    const auto e1 = first.project(real_seed(2));
    expect_timer(first.timer.state, Life::Active, Phase::Main, prior_main);
    expect_cut(a1, Mode::ActiveSubathon, active_remaining);
    expect_cut(e1, Mode::RealElapsed, 1);

    const auto finish =
        cut(first.now, at("at-deadline", 1000), first.timer.state, 1, pause, config);
    const auto a2 = finish.project(a1.head);
    const auto e2 = finish.project(e1.head);
    expect_timer(finish.timer.state, Life::Finished, Phase::Main, prior_main);
    EXPECT_EQ(finish.timer.transitions, (std::vector{T::HardEndReached, T::Finished}));
    expect_cut(a2, Mode::ActiveSubathon, active_remaining, {T::HardEndReached, T::Finished});
    expect_cut(e2, Mode::RealElapsed, 0,
               {T::HardEndReached, T::Finished, expiry(finish.now, "instance-real")});
    if (manually_paused) {
        // A also targets score here so the disabled post-finish modifier policy
        // can matter. Manual remains an independent application blocker.
        const auto input = arrival(finish.now, "arrives-at-deadline", false, true);
        const auto basis = capture(input, finish.timer.state, pause, a2.head);
        EXPECT_FALSE(basis.selection);
        expect_score(basis, {}, {}, {manual});
    }
}

TEST(ModifierClock, L5HardEndDuringManualPausePrecedesRealExpiry) { hard_end_cut(true, 100, 2); }

TEST(ModifierClock, L6HardEndPreemptsActiveDebitButNotRealDebit) { hard_end_cut(false, 99, 1); }

TEST(ModifierClock, L7QualifiedElapsedWinsOverUtcInBothDirections) {
    const auto forward = cut(at("start", 1000), at("forward", 1200), main_seed(100), 3);
    // First stub: same lineage, no hard end, 3 qualified ms despite the UTC change.
    const auto a1 = forward.project(active_seed(20));
    const auto e1 = forward.project(real_seed(20));
    expect_timer(forward.timer.state, Life::Active, Phase::Main, 97);
    expect_cut(a1, Mode::ActiveSubathon, 17);
    expect_cut(e1, Mode::RealElapsed, 17);

    const auto backward = cut(forward.now, at("backward", 900), forward.timer.state, 2);
    expect_timer(backward.timer.state, Life::Active, Phase::Main, 95);
    expect_cut(backward.project(a1.head), Mode::ActiveSubathon, 15);
    expect_cut(backward.project(e1.head), Mode::RealElapsed, 15);
}

DowntimeRecoveryInput offline(std::int64_t main, Policy policy, std::int64_t restart = 1008) {
    return {
        .basis = {"lineage-A", "checkpoint-A", "configuration-A", "facts-A"},
        .policy = policy,
        .timer = main_seed(main),
        .config = {},
        .pause = {},
        .checkpoint = {"offline-checkpoint", RecoveryUtcInstant{1000}},
        .restart = {"restart", RecoveryUtcInstant{restart}},
        .gap = RecoveryContinuousGap{"continuous-A", {1000}, {restart}},
        .schedule = RecoveryScheduleCoverage{"schedule-A", "delivery-A", {}, {}, {}},
    };
}

DowntimeRecoveryPreview preview(const DowntimeRecoveryInput &input) {
    auto result = std::get<DowntimeRecoveryPreview>(preview_downtime_recovery(input));
    EXPECT_EQ(result.basis.timer_lineage, input.basis.timer_lineage);
    EXPECT_EQ(result.basis.checkpoint_ref, input.basis.checkpoint_ref);
    EXPECT_EQ(result.basis.configuration_revision, input.basis.configuration_revision);
    EXPECT_EQ(result.basis.facts_bundle_ref, input.basis.facts_bundle_ref);
    return result;
}

void expect_trace(const RecoveryTimerProposal &proposal,
                  const std::vector<RecoveryTransition> &expected) {
    ASSERT_EQ(proposal.transitions.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(proposal.transitions[i].transition, expected[i].transition);
        EXPECT_EQ(proposal.transitions[i].utc, expected[i].utc);
        EXPECT_EQ(proposal.transitions[i].basis, expected[i].basis);
    }
}

void expect_recovery(const ModifierRecoveryProjection &proposal, Mode mode,
                     std::optional<std::int64_t> remaining,
                     const std::vector<ModifierRuleCandidate> &events = {}) {
    EXPECT_EQ(proposal.value.mode, mode);
    EXPECT_EQ(proposal.value.remaining_ms, remaining);
    EXPECT_EQ(proposal.events, events);
}

ModifierClockObservation clock_at(const RecoveryReviewSnapshot &snapshot) {
    return {snapshot.timer_lineage, snapshot.observed_at.observation_ref,
            snapshot.observed_at.utc.value().ms};
}

RecoveryReviewSnapshot restart_review(const DowntimeRecoveryInput &input,
                                      const RecoveryTimerProposal &held,
                                      std::optional<std::int64_t> cap = {}) {
    return {"episode-A",
            input.basis.timer_lineage,
            input.basis.facts_bundle_ref,
            {"restart-revision", {held.timer, 0, cap, input.config}},
            held.pause,
            input.restart};
}

struct ReviewCut {
    RecoveryReviewSnapshot observed;
    RecoveryReviewWait timer;
    TimerClockStep step;

    [[nodiscard]] RecoveryReviewSnapshot accepted(std::string revision) const {
        auto next = observed;
        next.current.revision = std::move(revision);
        next.current.values.timer = timer.held.timer;
        next.pause = timer.held.pause;
        return next;
    }
};

ReviewCut review_cut(const RecoveryReviewSnapshot &previous, std::string ref, std::int64_t utc,
                     std::int64_t elapsed) {
    auto observed = previous;
    observed.observed_at = {std::move(ref), RecoveryUtcInstant{utc}};
    const auto timer = std::get<RecoveryReviewWait>(observe_recovery_review_wait(observed));
    return {std::move(observed), timer, {elapsed, utc, !previous.pause.reasons.empty()}};
}

auto resolve(const DowntimeRecoveryInput &input, const RecoveryReviewSnapshot &snapshot,
             Choice choice, std::string operation,
             std::optional<TimerSignedDelta> correction = {}) {
    const RecoveryReviewCommand command{std::move(operation),
                                        snapshot.episode_ref,
                                        snapshot.current.revision,
                                        snapshot.facts_bundle_ref,
                                        choice,
                                        correction,
                                        "synthetic explicit choice"};
    RecoveryReviewAuthorization access{command,
                                       snapshot.current.revision,
                                       snapshot.observed_at.observation_ref,
                                       "owner-A",
                                       "permission-A",
                                       {}};
    if (correction)
        access.correction_access =
            TimerEditorialAccess{TimerEditorialAccessState::allowed, "editorial-permission-A"};
    return resolve_recovery_review(input, snapshot, command, access);
}

void expect_resolution_binding(const RecoveryReviewResolution &resolved,
                               const RecoveryReviewSnapshot &snapshot, Choice choice) {
    EXPECT_EQ(resolved.basis.timer_lineage, "lineage-A");
    EXPECT_EQ(resolved.basis.checkpoint_ref, "checkpoint-A");
    EXPECT_EQ(resolved.basis.configuration_revision, "configuration-A");
    EXPECT_EQ(resolved.basis.facts_bundle_ref, "facts-A");
    EXPECT_EQ(resolved.command.expected_episode_ref, snapshot.episode_ref);
    EXPECT_EQ(resolved.command.expected_revision, snapshot.current.revision);
    EXPECT_EQ(resolved.command.expected_facts_bundle_ref, snapshot.facts_bundle_ref);
    EXPECT_EQ(resolved.command.choice, choice);
    EXPECT_EQ(resolved.decision_observation.observation_ref, snapshot.observed_at.observation_ref);
    EXPECT_EQ(resolved.decision_observation.utc, snapshot.observed_at.utc);
}

TEST(ModifierClock, R1FreezeStillDebitsTheQualifiedRealGap) {
    const auto input = offline(4, Policy::Freeze);
    const auto timer = preview(input);
    expect_timer(timer.freeze.timer, Life::Active, Phase::Main, 4);
    expect_trace(timer.freeze, {});
    // First stub: CORE-006 Freeze with a continuous qualified gap and complete empty schedule.
    expect_recovery(project_modifier_restart(input, timer, active_seed(10)), Mode::ActiveSubathon,
                    10);
    expect_recovery(project_modifier_restart(input, timer, real_seed(10)), Mode::RealElapsed, 2);
}

TEST(ModifierClock, R1CountDebitsAcrossReconstructedGrace) {
    const auto input = offline(4, Policy::CountElapsed);
    const auto timer = preview(input);
    ASSERT_TRUE(timer.count_elapsed);
    expect_timer(timer.count_elapsed->timer, Life::Active, Phase::Grace, 0, 119'996);
    expect_trace(*timer.count_elapsed, {{T::MainExpired, {1004}, Basis::ReconstructedUtc},
                                        {T::GraceStarted, {1004}, Basis::ReconstructedUtc}});
    // First stub: same qualified gap as Freeze, with the core Count branch available.
    expect_recovery(project_modifier_restart(input, timer, active_seed(10)), Mode::ActiveSubathon,
                    2);
    expect_recovery(project_modifier_restart(input, timer, real_seed(10)), Mode::RealElapsed, 2);
}

TEST(ModifierClock, R2RetainedScheduleStopBoundsActiveDebit) {
    auto input = offline(20, Policy::CountElapsed);
    input.pause = set_pause_reason(input.pause, schedule, true);
    const std::array retained{RecoveryRetainedPause{
        "retired-schedule", {ScheduleLocalTime{750ms}}, {999, 1006}, RecoveryUtcInstant{1003}}};
    std::get<RecoveryScheduleCoverage>(input.schedule).retained = retained;
    const auto timer = preview(input);
    ASSERT_TRUE(timer.count_elapsed);
    expect_timer(timer.count_elapsed->timer, Life::Active, Phase::Main, 15);
    EXPECT_TRUE(timer.count_elapsed->pause.reasons.empty());
    expect_trace(*timer.count_elapsed, {});
    // First stub: retained row covers the paused checkpoint, with a valid stop inside its window.
    expect_recovery(project_modifier_restart(input, timer, active_seed(20)), Mode::ActiveSubathon,
                    15);
    expect_recovery(project_modifier_restart(input, timer, real_seed(20)), Mode::RealElapsed, 12);
}

TEST(ModifierClock, R3ReconstructedDeadlineWinsTieWithNaturalFinish) {
    auto input = offline(10, Policy::CountElapsed, 1012);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 1010;
    const auto timer = preview(input);
    ASSERT_TRUE(timer.count_elapsed);
    expect_timer(timer.count_elapsed->timer, Life::Finished, Phase::Main, 1);
    expect_trace(*timer.count_elapsed, {{T::HardEndReached, {1010}, Basis::ReconstructedUtc},
                                        {T::Finished, {1010}, Basis::ReconstructedUtc}});
    // First stub: continuous Count reaches the reconstructible deadline with Main still at 1.
    expect_recovery(project_modifier_restart(input, timer, active_seed(10)), Mode::ActiveSubathon,
                    1);
    const auto real = real_seed(10);
    const auto expired = expiry(at("restart", 1012), "instance-real", RecoveryUtcInstant{1010});
    expect_recovery(project_modifier_restart(input, timer, real), Mode::RealElapsed, 0, {expired});
    // Both calls are immutable previews. Only one candidate per projection, no delivery sink.
    expect_recovery(project_modifier_restart(input, timer, real), Mode::RealElapsed, 0, {expired});
}

TEST(ModifierClock, R4ReviewWaitDebitsRealOnceAndPreservesNewManualPause) {
    const auto input = offline(30, Policy::Review);
    const auto timer = preview(input);
    ASSERT_TRUE(timer.review_hold);
    const auto active = active_seed(20, true);
    const auto real = real_seed(11);
    // First stub: CORE-006 supplies the matching Review hold and qualified offline gap.
    const auto a1 = project_modifier_restart(input, timer, active);
    const auto e1 = project_modifier_restart(input, timer, real);
    expect_recovery(a1, Mode::ActiveSubathon, 20);
    expect_recovery(e1, Mode::RealElapsed, 3);

    auto held = restart_review(input, *timer.review_hold);
    expect_timer(held.current.values.timer, Life::Active, Phase::Main, 30);
    EXPECT_EQ(held.pause.reasons, (PauseReasons{review}));
    expect_score(capture(arrival(clock_at(held), "arrives-at-restart"), held.current.values.timer,
                         held.pause, a1.value),
                 {}, {}, {review});

    held.pause = set_pause_reason(held.pause, manual, true);
    held.current.revision = "review-with-manual";
    EXPECT_TRUE(input.pause.reasons.empty());
    const auto first = review_cut(held, "wait-two", 1010, 2);
    const auto a2 =
        project_modifier_review_wait(held, first.observed, first.timer, first.step, a1.value);
    const auto e2 =
        project_modifier_review_wait(held, first.observed, first.timer, first.step, e1.value);
    expect_recovery(a2, Mode::ActiveSubathon, 20);
    expect_recovery(e2, Mode::RealElapsed, 1);
    const auto waiting = first.accepted("first-wait-revision");
    expect_timer(waiting.current.values.timer, Life::Active, Phase::Main, 30);
    expect_trace(first.timer.held, {});
    expect_score(capture(arrival(clock_at(waiting), "arrives-during-wait"),
                         waiting.current.values.timer, waiting.pause, a2.value),
                 {}, {}, {review, manual});

    const auto second = review_cut(waiting, "wait-one", 1011, 1);
    const auto a3 =
        project_modifier_review_wait(waiting, second.observed, second.timer, second.step, a2.value);
    const auto e3 =
        project_modifier_review_wait(waiting, second.observed, second.timer, second.step, e2.value);
    const auto pending = second.accepted("second-wait-revision");
    expect_timer(pending.current.values.timer, Life::Active, Phase::Main, 30);
    expect_trace(second.timer.held, {});
    expect_recovery(a3, Mode::ActiveSubathon, 20);
    expect_recovery(e3, Mode::RealElapsed, 0, {expiry(clock_at(pending), "instance-real")});
    expect_score(capture(arrival(clock_at(pending), "arrives-before-choice"),
                         pending.current.values.timer, pending.pause, a3.value),
                 {}, {}, {review, manual});

    const auto decision = resolve(input, pending, Choice::CountElapsed, "count-op");
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewResolution>(decision));
    const auto &resolved = std::get<RecoveryReviewResolution>(decision);
    expect_resolution_binding(resolved, pending, Choice::CountElapsed);
    expect_timer(resolved.resolved.timer, Life::Active, Phase::Main, 22);
    EXPECT_EQ(resolved.pause.reasons, (PauseReasons{manual}));
    const auto a4 = project_modifier_review_resolution(input, pending, decision, active, a3.value);
    expect_recovery(a4, Mode::ActiveSubathon, 12);
    expect_recovery(project_modifier_review_resolution(input, pending, decision, real, e3.value),
                    Mode::RealElapsed, 0);
    expect_score(capture(arrival(clock_at(pending), "arrives-after-choice"),
                         resolved.resolved.timer, resolved.pause, a4.value),
                 {}, {}, {manual});
}

TEST(ModifierClock, R5ObservedWaitFinishWinsOverTheHistoricalCountdown) {
    auto input = offline(30, Policy::Review);
    input.config.hard_end_utc_ms = 1015;
    const auto timer = preview(input);
    ASSERT_TRUE(timer.review_hold);
    const auto active = active_seed(20);
    const auto real = real_seed(40);
    // First stub: Review is open at restart; the captured deadline is still in the future.
    const auto a1 = project_modifier_restart(input, timer, active);
    const auto e1 = project_modifier_restart(input, timer, real);
    expect_recovery(a1, Mode::ActiveSubathon, 20);
    expect_recovery(e1, Mode::RealElapsed, 32);
    const auto held = restart_review(input, *timer.review_hold);
    const auto wait = review_cut(held, "wait-at-deadline", 1015, 7);
    const auto a2 =
        project_modifier_review_wait(held, wait.observed, wait.timer, wait.step, a1.value);
    const auto e2 =
        project_modifier_review_wait(held, wait.observed, wait.timer, wait.step, e1.value);
    expect_timer(wait.timer.held.timer, Life::Finished, Phase::Main, 30);
    expect_trace(wait.timer.held, {{T::HardEndReached, {1015}, Basis::ObservedUtc},
                                   {T::Finished, {1015}, Basis::ObservedUtc}});
    expect_recovery(a2, Mode::ActiveSubathon, 20);
    expect_recovery(e2, Mode::RealElapsed, 25);

    const auto pending = wait.accepted("finished-wait-revision");
    EXPECT_EQ(pending.pause.reasons, (PauseReasons{review}));
    const auto decision = resolve(input, pending, Choice::CountElapsed, "count-op");
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewResolution>(decision));
    const auto &resolved = std::get<RecoveryReviewResolution>(decision);
    expect_resolution_binding(resolved, pending, Choice::CountElapsed);
    expect_timer(resolved.selected_at_restart.timer, Life::Active, Phase::Main, 22);
    expect_timer(resolved.resolved.timer, Life::Finished, Phase::Main, 30);
    EXPECT_TRUE(resolved.pause.reasons.empty());
    expect_recovery(project_modifier_review_resolution(input, pending, decision, active, a2.value),
                    Mode::ActiveSubathon, 12);
    expect_recovery(project_modifier_review_resolution(input, pending, decision, real, e2.value),
                    Mode::RealElapsed, 25);
}

TEST(ModifierClock, R6MissingScheduleFactsRefuseCountButExplicitFreezeKeepsRealDebit) {
    auto input = offline(30, Policy::CountElapsed);
    input.schedule = RecoveryScheduleUnavailable::DeliveryContext;
    const auto timer = preview(input);
    ASSERT_TRUE(timer.review_hold);
    EXPECT_FALSE(timer.count_elapsed);
    EXPECT_EQ(timer.schedule_unavailable, RecoveryScheduleUnavailable::DeliveryContext);
    const auto active = active_seed(20);
    const auto real = real_seed(30);
    // First stub: the unavailable schedule forces Review; supplied clock endpoints still qualify.
    const auto a1 = project_modifier_restart(input, timer, active);
    const auto e1 = project_modifier_restart(input, timer, real);
    const auto pending = restart_review(input, *timer.review_hold);
    expect_timer(pending.current.values.timer, Life::Active, Phase::Main, 30);
    EXPECT_EQ(pending.pause.reasons, (PauseReasons{review}));
    expect_recovery(a1, Mode::ActiveSubathon, 20);
    expect_recovery(e1, Mode::RealElapsed, 22);

    const auto refused = resolve(input, pending, Choice::CountElapsed, "count-op");
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewRefusal>(refused));
    EXPECT_EQ(std::get<RecoveryReviewRefusal>(refused), RecoveryReviewRefusal::ChoiceUnavailable);
    const auto unchanged_a =
        project_modifier_review_resolution(input, pending, refused, active, a1.value);
    const auto unchanged_e =
        project_modifier_review_resolution(input, pending, refused, real, e1.value);
    EXPECT_EQ(unchanged_a.value, a1.value);
    EXPECT_EQ(unchanged_e.value, e1.value);
    EXPECT_TRUE(unchanged_a.events.empty());
    EXPECT_TRUE(unchanged_e.events.empty());

    const auto decision = resolve(input, pending, Choice::Freeze, "explicit-freeze-op");
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewResolution>(decision));
    const auto &resolved = std::get<RecoveryReviewResolution>(decision);
    expect_resolution_binding(resolved, pending, Choice::Freeze);
    expect_timer(resolved.resolved.timer, Life::Active, Phase::Main, 30);
    EXPECT_TRUE(resolved.pause.reasons.empty());
    expect_recovery(
        project_modifier_review_resolution(input, pending, decision, active, unchanged_a.value),
        Mode::ActiveSubathon, 20);
    expect_recovery(
        project_modifier_review_resolution(input, pending, decision, real, unchanged_e.value),
        Mode::RealElapsed, 22);
}

TEST(ModifierClock, R7ReviewAloneHoldsAndTimerFreezeLeavesRealUnknown) {
    auto input = offline(30, Policy::CountElapsed);
    input.config.hard_end_utc_ms = 1007;
    input.gap = RecoveryGapUnavailable::Discontinuous;
    input.schedule = RecoveryScheduleUnavailable::Source;
    const auto timer = preview(input);
    ASSERT_TRUE(timer.review_hold);
    EXPECT_FALSE(timer.count_elapsed);
    EXPECT_EQ(timer.gap_unavailable, RecoveryGapUnavailable::Discontinuous);
    EXPECT_EQ(timer.schedule_unavailable, RecoveryScheduleUnavailable::Source);
    expect_timer(timer.review_hold->timer, Life::Finished, Phase::Main, 30);
    expect_trace(*timer.review_hold, {{T::HardEndReached, {1008}, Basis::ObservedUtc},
                                      {T::Finished, {1008}, Basis::ObservedUtc}});
    ASSERT_EQ(timer.review_hold->pause.reasons, (PauseReasons{review}));
    EXPECT_TRUE(input.pause.reasons.empty());
    const auto active = active_seed(20, true);
    const auto real = real_seed(30);
    // First stub: core observed the finish and can hold despite both unavailable fact sources.
    const auto a1 = project_modifier_restart(input, timer, active);
    const auto e1 = project_modifier_restart(input, timer, real);
    expect_recovery(a1, Mode::ActiveSubathon, 20);
    expect_recovery(e1, Mode::RealElapsed, std::nullopt);
    EXPECT_EQ(e1.value.revision, real.revision);
    EXPECT_EQ(e1.value.revision.remaining_ms, 30);

    auto pending = restart_review(input, *timer.review_hold);
    const auto arrival_at_review = arrival(clock_at(pending), "eligible-except-review", true, true);
    // Counterfactual for this same arrival at restart. CORE-006's Freeze preview
    // observes the very same finish without the Review hold. It is never selected here.
    expect_timer(timer.freeze.timer, Life::Finished, Phase::Main, 30);
    ASSERT_TRUE(timer.freeze.pause.reasons.empty());
    const auto eligible =
        finished_owner(arrival_at_review, timer.freeze.timer, timer.freeze.pause, input.config);
    EXPECT_EQ(eligible.effects,
              (std::vector{EffectDisposition::finished, EffectDisposition::pass_to_owner}));
    expect_score(capture(arrival_at_review, timer.freeze.timer, timer.freeze.pause, a1.value),
                 20'000'000, "revision-1");
    const auto held_input =
        capture(arrival_at_review, pending.current.values.timer, pending.pause, a1.value);
    expect_score(held_input, {}, {}, {review});

    // Add Manual only after the arrival. Neither the retained pause set nor the
    // observed finish is replaced by a fabricated offline timer.
    pending.pause = set_pause_reason(pending.pause, manual, true);
    pending.current.revision = "finished-review-manual-revision";
    EXPECT_EQ(pending.pause.reasons, (PauseReasons{review, manual}));
    EXPECT_TRUE(input.pause.reasons.empty());
    expect_timer(pending.current.values.timer, Life::Finished, Phase::Main, 30);
    const auto refused = resolve(input, pending, Choice::CountElapsed, "count-op");
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewRefusal>(refused));
    EXPECT_EQ(std::get<RecoveryReviewRefusal>(refused), RecoveryReviewRefusal::ChoiceUnavailable);
    const auto unchanged_a =
        project_modifier_review_resolution(input, pending, refused, active, a1.value);
    const auto unchanged_e =
        project_modifier_review_resolution(input, pending, refused, real, e1.value);
    EXPECT_EQ(unchanged_a.value, a1.value);
    EXPECT_EQ(unchanged_e.value, e1.value);
    EXPECT_TRUE(unchanged_a.events.empty());
    EXPECT_TRUE(unchanged_e.events.empty());

    const auto decision = resolve(input, pending, Choice::Freeze, "explicit-freeze-op");
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewResolution>(decision));
    const auto &resolved = std::get<RecoveryReviewResolution>(decision);
    expect_resolution_binding(resolved, pending, Choice::Freeze);
    expect_timer(resolved.resolved.timer, Life::Finished, Phase::Main, 30);
    EXPECT_EQ(resolved.pause.reasons, (PauseReasons{manual}));
    EXPECT_FALSE(resolved.correction);
    expect_recovery(
        project_modifier_review_resolution(input, pending, decision, active, unchanged_a.value),
        Mode::ActiveSubathon, 20);
    const auto unresolved =
        project_modifier_review_resolution(input, pending, decision, real, unchanged_e.value);
    expect_recovery(unresolved, Mode::RealElapsed, std::nullopt);
    EXPECT_EQ(unresolved.value.revision, real.revision);
    EXPECT_EQ(unresolved.value.revision.remaining_ms, 30);
}

TEST(ModifierClock, R8CappedCorrectionChangesMainWithoutModifierElapsed) {
    const auto input = offline(30, Policy::Review);
    const auto timer = preview(input);
    ASSERT_TRUE(timer.review_hold);
    const auto active = active_seed(20);
    const auto real = real_seed(20);
    // First stub: matching Review hold and continuous 8 ms gap, before applying any correction.
    const auto a1 = project_modifier_restart(input, timer, active);
    const auto e1 = project_modifier_restart(input, timer, real);
    const auto held = restart_review(input, *timer.review_hold, 35);
    expect_timer(held.current.values.timer, Life::Active, Phase::Main, 30);
    EXPECT_EQ(held.pause.reasons, (PauseReasons{review}));
    expect_recovery(a1, Mode::ActiveSubathon, 20);
    expect_recovery(e1, Mode::RealElapsed, 12);

    const auto wait = review_cut(held, "wait-three", 1011, 3);
    const auto a2 =
        project_modifier_review_wait(held, wait.observed, wait.timer, wait.step, a1.value);
    const auto e2 =
        project_modifier_review_wait(held, wait.observed, wait.timer, wait.step, e1.value);
    expect_timer(wait.timer.held.timer, Life::Active, Phase::Main, 30);
    expect_recovery(a2, Mode::ActiveSubathon, 20);
    expect_recovery(e2, Mode::RealElapsed, 9);

    const auto pending = wait.accepted("wait-revision");
    const auto decision = resolve(input, pending, Choice::CountElapsed, "count-with-correction-op",
                                  TimerSignedDelta{40, {}});
    ASSERT_TRUE(std::holds_alternative<RecoveryReviewResolution>(decision));
    const auto &resolved = std::get<RecoveryReviewResolution>(decision);
    expect_resolution_binding(resolved, pending, Choice::CountElapsed);
    expect_timer(resolved.selected_at_restart.timer, Life::Active, Phase::Main, 22);
    expect_timer(resolved.resolved.timer, Life::Active, Phase::Main, 35);
    EXPECT_EQ(resolved.resolved.maximum_remaining_ms, 35);
    EXPECT_EQ(resolved.resolved.time_debt_ms, 0);
    EXPECT_TRUE(resolved.pause.reasons.empty());
    ASSERT_TRUE(resolved.correction);
    EXPECT_EQ(resolved.correction->requested_ms, 40);
    EXPECT_EQ(resolved.correction->main_adjustment_ms, 13);
    EXPECT_EQ(resolved.correction->cap_limited_ms, 27);
    EXPECT_EQ(resolved.correction_transitions.count, 0);
    expect_recovery(project_modifier_review_resolution(input, pending, decision, active, a2.value),
                    Mode::ActiveSubathon, 12);
    expect_recovery(project_modifier_review_resolution(input, pending, decision, real, e2.value),
                    Mode::RealElapsed, 9);
}

ModifierActivationAtCut rule(std::string new_instance, std::string new_revision,
                             std::string definition_revision, std::int64_t duration,
                             ModifierTarget target, Timing timing) {
    return {{"definition-score", std::move(definition_revision), duration, {std::move(target)}},
            {std::move(new_instance),
             std::move(new_revision),
             {ModifierActivationKind::rule, "q-rule"},
             ModifierTriggeringContribution{"rule-trigger", timing}},
            {}};
}

ModifierRuleCandidate activation_event(const ModifierClockObservation &now, std::string instance,
                                       std::string revision) {
    return {Event::Activated, now, std::move(instance), std::move(revision), {}};
}

std::optional<ModifierClockValue> trigger_head(const ModifierActivationProposal &proposal) {
    if (proposal.for_trigger)
        return exact_head(*proposal.for_trigger);
    return {};
}

void at_first_activation(Timing timing, std::int64_t trigger_request,
                         std::optional<std::string_view> trigger_revision) {
    const auto start = at("activation-observation", 1000);
    const auto same = cut(start, start, main_seed(100), 0);
    const std::array<std::string_view, 1> q{"q"};
    auto ordinary_before = arrival(start, "ordinary-before");
    ordinary_before.matched_qualifiers = q;
    // E1 first stub: an ordinary eligible input with an empty live head, before activation.
    expect_score(capture(ordinary_before, same.timer.state, {}, {}), 10'000'000);

    const auto command = rule("instance-score", "revision-1", "def-1", 5, score("2", "q"), timing);
    const auto proposed = same.project({}, command);
    expect_timer(same.timer.state, Life::Active, Phase::Main, 100);
    expect_head(proposed.head, Mode::ActiveSubathon, 5);
    EXPECT_EQ(proposed.events, (std::vector<ModifierCutEvent>{
                                   activation_event(start, "instance-score", "revision-1")}));
    ASSERT_TRUE(proposed.activation);
    ASSERT_TRUE(proposed.activation->revision_to_append);
    EXPECT_EQ(proposed.activation->revision_to_append->instance_ref, "instance-score");
    EXPECT_EQ(proposed.activation->for_trigger.has_value(), trigger_revision.has_value());

    auto trigger = arrival(start, "rule-trigger");
    trigger.matched_qualifiers = q;
    expect_score(capture(trigger, same.timer.state, {}, trigger_head(*proposed.activation)),
                 trigger_request, trigger_revision);
    auto following = arrival(start, "ordinary-after");
    following.matched_qualifiers = q;
    const auto after = capture(following, same.timer.state, {}, proposed.head);
    expect_score(after, 20'000'000, "revision-1");
    ASSERT_TRUE(after.selection);
    EXPECT_EQ(after.selection->target.qualifier_ref, "q");
}

TEST(ModifierClock, E1IncludeUsesNewRevisionForTriggerAtSameObservation) {
    at_first_activation(Timing::include_trigger, 20'000'000, "revision-1");
}

TEST(ModifierClock, E1SubsequentUsesNoRevisionForTriggerAtSameObservation) {
    at_first_activation(Timing::subsequent_only, 10'000'000, {});
}

TEST(ModifierClock, E2FreshExpiryWinsWhilePendingArrivalKeepsItsBasis) {
    const auto first = cut(at("activated", 1000), at("still-live", 1004), main_seed(100), 4);
    // First stub: #215's 5 ms score revision and a qualified 4 ms Main cut.
    const auto a1 = first.project(score_seed(5));
    expect_timer(first.timer.state, Life::Active, Phase::Main, 96);
    expect_cut(a1, Mode::ActiveSubathon, 1);
    const auto pending =
        capture(arrival(first.now, "pending-arrival"), first.timer.state, {}, a1.head);
    expect_score(pending, 20'000'000, "revision-1");

    const auto edge = cut(first.now, at("at-zero", 1005), first.timer.state, 1);
    const auto a2 = edge.project(a1.head);
    expect_timer(edge.timer.state, Life::Active, Phase::Main, 95);
    expect_cut(a2, Mode::ActiveSubathon, 0, {expiry(edge.now, "instance-score")});
    expect_score(capture(arrival(edge.now, "fresh-at-zero"), edge.timer.state, {}, a2.head),
                 10'000'000);

    const auto last = cut(edge.now, at("past-zero", 1006), edge.timer.state, 1);
    const auto a3 = last.project(a2.head);
    expect_timer(last.timer.state, Life::Active, Phase::Main, 94);
    expect_cut(a3, Mode::ActiveSubathon, 0);
    expect_score(capture(arrival(last.now, "fresh-after-zero"), last.timer.state, {}, a3.head),
                 10'000'000);
    EXPECT_EQ(pending.observation, at("still-live", 1004));
    expect_score(pending, 20'000'000, "revision-1");
}

void extend_at_expiry(Timing timing, std::int64_t trigger_request,
                      std::optional<std::string_view> trigger_revision) {
    const auto edge = cut(at("activated", 1000), at("at-zero", 1005), main_seed(100), 5);
    const auto command = rule("instance-new", "revision-2", "def-2", 7, score("3"), timing);
    // E3 first stub: both the old #215 head and incoming default Extend reach
    // the same cut. No fixture clears the old head before this call.
    const auto proposed = edge.project(score_seed(5), command);
    expect_timer(edge.timer.state, Life::Active, Phase::Main, 95);
    expect_head(proposed.head, Mode::ActiveSubathon, 7);
    EXPECT_EQ(proposed.events, (std::vector<ModifierCutEvent>{
                                   expiry(edge.now, "instance-score"),
                                   activation_event(edge.now, "instance-new", "revision-2")}));
    ASSERT_TRUE(proposed.activation);
    ASSERT_TRUE(proposed.activation->revision_to_append);
    const auto &revision = *proposed.activation->revision_to_append;
    EXPECT_EQ(revision.instance_ref, "instance-new");
    EXPECT_EQ(revision.revision_ref, "revision-2");
    EXPECT_EQ(revision.remaining_ms, 7);
    EXPECT_FALSE(revision.parent_revision_ref);
    EXPECT_FALSE(proposed.activation->superseded_instance_ref);
    EXPECT_EQ(proposed.activation->for_trigger.has_value(), trigger_revision.has_value());
    EXPECT_EQ(revision.captured_definition.revision_ref, "def-2");
    ASSERT_EQ(revision.captured_definition.targets.size(), 1U);
    EXPECT_EQ(revision.captured_definition.targets.front().factor_decimal, "3");
    expect_score(capture(arrival(edge.now, "rule-trigger"), edge.timer.state, {},
                         trigger_head(*proposed.activation)),
                 trigger_request, trigger_revision);
    expect_score(capture(arrival(edge.now, "ordinary-after"), edge.timer.state, {}, proposed.head),
                 30'000'000, "revision-2");
}

TEST(ModifierClock, E3AtExpiryExtendIncludeStartsNewHeadBeforeTrigger) {
    extend_at_expiry(Timing::include_trigger, 30'000'000, "revision-2");
}

TEST(ModifierClock, E3AtExpiryExtendSubsequentDoesNotUseExpiredTriggerHead) {
    extend_at_expiry(Timing::subsequent_only, 10'000'000, {});
}

TEST(ModifierClock, E4ExtendUsesCutRemainingAndRetainsCapturedFactor) {
    const auto before_expiry =
        cut(at("activated", 1000), at("before-expiry", 1004), main_seed(100), 4);
    const auto command =
        rule("unused-new-instance", "revision-2", "def-2", 7, score("3"), Timing::subsequent_only);
    // First stub: live #215 head and an Extend request share the 4 ms cut.
    const auto proposed = before_expiry.project(score_seed(5), command);
    expect_timer(before_expiry.timer.state, Life::Active, Phase::Main, 96);
    expect_head(proposed.head, Mode::ActiveSubathon, 8);
    ASSERT_TRUE(proposed.activation);
    ASSERT_TRUE(proposed.activation->for_trigger);
    EXPECT_EQ(proposed.activation->for_trigger->remaining_ms, 1);
    ASSERT_TRUE(proposed.activation->revision_to_append);
    const auto &revision = *proposed.activation->revision_to_append;
    EXPECT_EQ(revision.instance_ref, "instance-score");
    EXPECT_EQ(revision.revision_ref, "revision-2");
    EXPECT_EQ(revision.parent_revision_ref, "revision-1");
    EXPECT_EQ(revision.remaining_ms, 8);
    EXPECT_FALSE(proposed.activation->superseded_instance_ref);
    EXPECT_EQ(revision.captured_definition.revision_ref, "def-1");
    ASSERT_EQ(revision.captured_definition.targets.size(), 1U);
    EXPECT_EQ(revision.captured_definition.targets.front().factor_decimal, "2");
    EXPECT_EQ(proposed.events, (std::vector<ModifierCutEvent>{activation_event(
                                   before_expiry.now, "instance-score", "revision-2")}));

    expect_score(capture(arrival(before_expiry.now, "rule-trigger"), before_expiry.timer.state, {},
                         trigger_head(*proposed.activation)),
                 20'000'000, "revision-1");
    expect_score(capture(arrival(before_expiry.now, "ordinary-after"), before_expiry.timer.state,
                         {}, proposed.head),
                 20'000'000, "revision-2");
}

void finished_policy(bool modifiers, bool progression, std::optional<std::int64_t> request,
                     std::optional<std::string_view> revision,
                     EffectDisposition score_disposition) {
    const auto finish = cut(at("grace-1", 120999), at("finish", 121000), grace_seed(1), 1);
    // E5 first stub: the T-produced final Grace millisecond and a live #215 score head.
    const auto a1 = finish.project(score_seed(5));
    expect_timer(finish.timer.state, Life::Finished, Phase::Grace, 0, 0);
    expect_cut(a1, Mode::ActiveSubathon, 4, {T::GraceExpired, T::Finished});
    const auto after = cut(finish.now, at("after-finish", 121001), finish.timer.state, 1);
    const auto a2 = after.project(a1.head);
    expect_timer(after.timer.state, Life::Finished, Phase::Grace, 0, 0);
    expect_cut(a2, Mode::ActiveSubathon, 4);
    const auto input = arrival(after.now, "ordinary-finished-arrival", modifiers, progression);
    const auto owner = finished_owner(input, after.timer.state, {}, {});
    EXPECT_EQ(owner.retained.input_ref, "ordinary-finished-arrival");
    EXPECT_EQ(owner.effects, (std::vector{EffectDisposition::finished, score_disposition}));
    EXPECT_EQ(owner.main_added_ms, 0);
    const auto captured = capture(input, after.timer.state, {}, a2.head);
    expect_score(captured, request, revision);
}

TEST(ModifierClock, E5FinishedRecordsWithBothPoliciesOff) {
    finished_policy(false, false, {}, {}, EffectDisposition::progression_off);
}

TEST(ModifierClock, E5FinishedModifiersAloneDoNotEnableScoreProgression) {
    finished_policy(true, false, {}, {}, EffectDisposition::progression_off);
}

TEST(ModifierClock, E5FinishedScoreProgressionWithoutModifiersRequestsBaseScore) {
    finished_policy(false, true, 10'000'000, {}, EffectDisposition::pass_to_owner);
}

TEST(ModifierClock, E5FinishedBothPoliciesAllowModifiedScore) {
    finished_policy(true, true, 20'000'000, "revision-1", EffectDisposition::pass_to_owner);
}

void real_visibility(bool visible) {
    const auto first = cut(at("activated", 1000), at("visibility-change", 1002), main_seed(100), 2);
    // E6 first stub: a shipped score activation with an explicitly chosen Real clock.
    const auto e1 = first.project(score_seed(5, Mode::RealElapsed));
    expect_timer(first.timer.state, Life::Active, Phase::Main, 98);
    expect_cut(e1, Mode::RealElapsed, 3);
    const auto before_toggle = arrival(first.now, "before-visibility-choice");
    expect_score(capture(before_toggle, first.timer.state, {}, e1.head), 20'000'000, "revision-1");

    // Same bound observation. Changing visibility supplies no new clock cut.
    const auto now = arrival(first.now, "after-visibility-choice", false, false, visible);
    expect_score(capture(now, first.timer.state, {}, e1.head), 20'000'000, "revision-1");
    const auto edge = cut(first.now, at("at-real-expiry", 1005), first.timer.state, 3);
    const auto e2 = edge.project(e1.head);
    expect_timer(edge.timer.state, Life::Active, Phase::Main, 95);
    expect_cut(e2, Mode::RealElapsed, 0, {expiry(edge.now, "instance-score")});
    expect_score(capture(arrival(edge.now, "fresh-at-expiry", false, false, visible),
                         edge.timer.state, {}, e2.head),
                 10'000'000);
}

TEST(ModifierClock, E6VisiblePresentationDoesNotControlRealExpiryOrScore) { real_visibility(true); }

TEST(ModifierClock, E6HiddenPresentationDoesNotControlRealExpiryOrScore) { real_visibility(false); }

TEST(ModifierClockDebitEvidence, LiveRejectsMissingDebitOnChangedMain) {
    const LiveCut observation{
        at("before", 1000), at("now", 1005), main_seed(20), {5, 1005, false}, {main_seed(15), {}}};
    EXPECT_THROW((void)observation.project(active_seed(10)), std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, LiveRejectsPositiveDebitOnUnchangedMain) {
    const LiveCut observation{at("before", 1000),
                              at("now", 1005),
                              main_seed(20),
                              {5, 1005, false},
                              {main_seed(20), {}, 5}};
    EXPECT_THROW((void)observation.project(active_seed(10)), std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, LiveRejectsShortDebitOnMainToOngoingGrace) {
    const LiveCut observation{
        at("before", 1000),
        at("now", 1007),
        main_seed(3),
        {7, 1007, false},
        {{Life::Active, Phase::Grace, 0, 6, false}, {T::MainExpired, T::GraceStarted}, 3}};
    EXPECT_THROW((void)observation.project(active_seed(10)), std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, LiveRejectsFinishDebitShorterThanMain) {
    const LiveCut observation{at("before", 1000),
                              at("now", 1007),
                              main_seed(3),
                              {7, 1007, false},
                              {{Life::Finished, Phase::Grace, 0, 0, false},
                               {T::MainExpired, T::GraceStarted, T::GraceExpired, T::Finished},
                               2}};
    EXPECT_THROW((void)observation.project(active_seed(10)), std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, LiveAcceptsFreshAndSharedGraceFinishWithOvershoot) {
    auto seed = main_seed(3);
    seed.grace_remaining_ms = 2;
    const TimerConfig fresh{TimerEndMode::Grace, 4, GraceReuse::Fresh, {}};
    const TimerConfig shared{TimerEndMode::Grace, 4, GraceReuse::Shared, {}};
    const auto before = at("before", 1000);
    const auto now = at("now", 1010);
    const std::vector<ModifierCutEvent> events{T::MainExpired, T::GraceStarted, T::GraceExpired,
                                               T::Finished};
    expect_cut(cut(before, now, seed, 10, {}, fresh).project(active_seed(12)), Mode::ActiveSubathon,
               5, events);
    expect_cut(cut(before, now, seed, 10, {}, shared).project(active_seed(12)),
               Mode::ActiveSubathon, 7, events);
}

TEST(ModifierClockDebitEvidence, LiveAcceptsZeroDebitOnPausedZeroPhaseBoundary) {
    const TimerConfig fresh_zero{TimerEndMode::Grace, 0, GraceReuse::Fresh, {}};
    const auto observation = cut(at("before", 1000), at("now", 1005), main_seed(0), 5,
                                 set_pause_reason({}, manual, true), fresh_zero);
    expect_cut(observation.project(active_seed(10)), Mode::ActiveSubathon, 10,
               {T::MainExpired, T::GraceStarted, T::GraceExpired, T::Finished});
}

TEST(ModifierClockDebitEvidence, RestartRejectsOmittedCountProgression) {
    const auto input = offline(30, Policy::CountElapsed);
    auto timer = preview(input);
    ASSERT_TRUE(timer.count_elapsed);
    expect_timer(timer.count_elapsed->timer, Life::Active, Phase::Main, 22);
    timer.count_elapsed->progression.clear();

    EXPECT_THROW((void)project_modifier_restart(input, timer, active_seed(20)),
                 std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, RestartRejectsShortCountProgressionAtDeadline) {
    auto input = offline(10, Policy::CountElapsed, 1012);
    input.config.end_mode = TimerEndMode::Immediate;
    input.config.hard_end_utc_ms = 1010;
    auto timer = preview(input);
    ASSERT_TRUE(timer.count_elapsed);
    expect_timer(timer.count_elapsed->timer, Life::Finished, Phase::Main, 1);
    timer.count_elapsed->progression = {{{1000}, 8}}; // D-1 requires nine milliseconds.

    EXPECT_THROW((void)project_modifier_restart(input, timer, active_seed(20)),
                 std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, ReviewRejectsCountSliceAfterModifierWouldExpire) {
    const auto input = offline(4, Policy::Review);
    const auto timer = preview(input);
    ASSERT_TRUE(timer.review_hold);
    const auto held = restart_review(input, *timer.review_hold);
    auto decision = resolve(input, held, Choice::CountElapsed, "count-op");
    auto *resolved = std::get_if<RecoveryReviewResolution>(&decision);
    ASSERT_NE(resolved, nullptr);
    expect_timer(resolved->selected_at_restart.timer, Life::Active, Phase::Grace, 0, 119'996);
    // The first slice expires the modifier. The overlapping second slice is invalid.
    resolved->selected_at_restart.progression = {{{1000}, 4}, {{1003}, 4}};
    const auto head = active_seed(2);
    EXPECT_THROW((void)project_modifier_review_resolution(input, held, decision, head, head),
                 std::invalid_argument);
}

TEST(ModifierClockDebitEvidence, RestartAcceptsEmptyCountProgressionDuringManualPause) {
    auto input = offline(30, Policy::CountElapsed);
    input.pause = set_pause_reason({}, manual, true);
    const auto timer = preview(input);
    ASSERT_TRUE(timer.count_elapsed);
    EXPECT_TRUE(timer.count_elapsed->progression.empty());
    expect_recovery(project_modifier_restart(input, timer, active_seed(20)), Mode::ActiveSubathon,
                    20);
}

} // namespace
