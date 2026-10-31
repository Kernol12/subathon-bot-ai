// SPDX-License-Identifier: GPL-3.0-or-later
// Specification v0.45 §4/TIMER-01/02; timer-lifecycle-v1.md revision 1.
// All durations and transition sequences below are manually derived examples.
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/timer_lifecycle.hpp>

namespace {
using namespace subathon::core;

TimerState main_state(std::int64_t remaining_ms = 1000) {
    return {TimerLifecycle::Active, CountdownPhase::Main, remaining_ms, 120'000, false};
}
TimerState grace_state(std::int64_t remaining_ms = 90'000) {
    return {TimerLifecycle::Active, CountdownPhase::Grace, 0, remaining_ms, false};
}

TEST(TimerLifecycleAcceptance, DefaultExactMainExpiryStartsSeparateTwoMinuteGrace) {
    const TimerConfig config;
    EXPECT_EQ(config.end_mode, TimerEndMode::Grace);
    EXPECT_EQ(config.grace_duration_ms, 120'000);
    EXPECT_EQ(config.grace_reuse, GraceReuse::Fresh);
    const auto result = advance_timer(main_state(), config, {1000, 1000, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.main_remaining_ms, 0);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
    EXPECT_FALSE(result.state.awaiting_confirmation);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::MainExpired, TimerTransition::GraceStarted}));
}

TEST(TimerLifecycleAcceptance, MainCountdownBeforeZeroPreservesGraceBudget) {
    const auto result = advance_timer(main_state(), {}, {999, 999, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Main);
    EXPECT_EQ(result.state.main_remaining_ms, 1);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
    EXPECT_TRUE(result.transitions.empty());
}

TEST(TimerLifecycleAcceptance, MainOvershootImmediatelyConsumesGrace) {
    const auto result = advance_timer(main_state(), {}, {1001, 1001, false});
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.main_remaining_ms, 0);
    EXPECT_EQ(result.state.grace_remaining_ms, 119'999);
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
}

TEST(TimerLifecycleAcceptance, OneStepCanCrossMainAndAllGraceWithoutLosingTransitions) {
    const auto result = advance_timer(main_state(), {}, {121'000, 121'000, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.main_remaining_ms, 0);
    EXPECT_EQ(result.state.grace_remaining_ms, 0);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::MainExpired, TimerTransition::GraceStarted,
                           TimerTransition::GraceExpired, TimerTransition::Finished}));
}

TEST(TimerLifecycleAcceptance, GraceStaysActiveUntilItsExactLastMillisecond) {
    const auto result = advance_timer(grace_state(1000), {}, {999, 999, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 1);
    EXPECT_TRUE(result.transitions.empty());
}

TEST(TimerLifecycleAcceptance, ExactGraceExpiryFinishesAndNeverMakesGraceNegative) {
    const auto result = advance_timer(grace_state(1), {}, {1, 1, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.state.grace_remaining_ms, 0);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::GraceExpired, TimerTransition::Finished}));
}

TEST(TimerLifecycleAcceptance, ImmediateModeFinishesAtMainZeroWithoutGrace) {
    TimerConfig config;
    config.end_mode = TimerEndMode::Immediate;
    const auto result = advance_timer(main_state(), config, {1000, 1000, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.state.phase, CountdownPhase::Main);
    EXPECT_EQ(result.state.main_remaining_ms, 0);
    EXPECT_FALSE(result.state.awaiting_confirmation);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::MainExpired, TimerTransition::Finished}));
}

TEST(TimerLifecycleAcceptance, ConfirmationModeWaitsWithoutConsumingGrace) {
    TimerConfig config;
    config.end_mode = TimerEndMode::Confirmation;
    const auto result = advance_timer(main_state(), config, {121'000, 121'000, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Main);
    EXPECT_EQ(result.state.main_remaining_ms, 0);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
    EXPECT_TRUE(result.state.awaiting_confirmation);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::MainExpired, TimerTransition::ConfirmationRequired}));
    const auto waiting = advance_timer(result.state, config, {500'000, 621'000, false});
    EXPECT_EQ(waiting.state.lifecycle, TimerLifecycle::Active);
    EXPECT_TRUE(waiting.state.awaiting_confirmation);
    EXPECT_EQ(waiting.state.grace_remaining_ms, 120'000);
    EXPECT_TRUE(waiting.transitions.empty());
}

TEST(TimerLifecycleAcceptance, PreauthorizedConfirmationFinishesOnlyAwaitingState) {
    const TimerState waiting{TimerLifecycle::Active, CountdownPhase::Main, 0, 120'000, true};
    const auto result = confirm_timer_finish(waiting);
    EXPECT_EQ(result.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.phase, CountdownPhase::Main);
    EXPECT_EQ(result.main_remaining_ms, 0);
    EXPECT_FALSE(result.awaiting_confirmation);
    EXPECT_THROW(static_cast<void>(confirm_timer_finish(main_state())), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(confirm_timer_finish(grace_state())), std::invalid_argument);
}

TEST(TimerLifecycleAcceptance, PositiveApplicableCreditRescuesGraceAndPreservesItsBudget) {
    const auto result = apply_applicable_time_credit(grace_state(), 5000);
    EXPECT_EQ(result.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.phase, CountdownPhase::Main);
    EXPECT_EQ(result.main_remaining_ms, 5000);
    EXPECT_EQ(result.grace_remaining_ms, 90'000);
    EXPECT_FALSE(result.awaiting_confirmation);
}

TEST(TimerLifecycleAcceptance, ZeroActualCreditDoesNotRescueGrace) {
    const auto result = apply_applicable_time_credit(grace_state(), 0);
    EXPECT_EQ(result.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.main_remaining_ms, 0);
    EXPECT_EQ(result.grace_remaining_ms, 90'000);
}

TEST(TimerLifecycleAcceptance, FreshModeRestartsFullGraceOnRepeatedMainExpiry) {
    const auto rescued = apply_applicable_time_credit(grace_state(), 5000);
    const auto result = advance_timer(rescued, {}, {5000, 5000, false});
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::MainExpired, TimerTransition::GraceStarted}));
}

TEST(TimerLifecycleAcceptance, SharedModeReusesOnlyUnusedGraceAfterRescue) {
    TimerConfig config;
    config.grace_reuse = GraceReuse::Shared;
    const auto rescued = apply_applicable_time_credit(grace_state(), 5000);
    const auto result = advance_timer(rescued, config, {5000, 5000, false});
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 90'000);
}

TEST(TimerLifecycleAcceptance, SharedGraceConsumesOnlyTheOvershootAfterMain) {
    TimerConfig config;
    config.grace_reuse = GraceReuse::Shared;
    const auto rescued = apply_applicable_time_credit(grace_state(), 5000);
    const auto main = advance_timer(rescued, config, {4000, 4000, false});
    EXPECT_EQ(main.state.main_remaining_ms, 1000);
    EXPECT_EQ(main.state.grace_remaining_ms, 90'000);
    const auto result = advance_timer(main.state, config, {1500, 5500, false});
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 89'500);
}

TEST(TimerLifecycleAcceptance, PausedMainPreservesBothClocksAndPhase) {
    const auto result = advance_timer(main_state(), {}, {500'000, 500'000, true});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Main);
    EXPECT_EQ(result.state.main_remaining_ms, 1000);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
    EXPECT_TRUE(result.transitions.empty());
}

TEST(TimerLifecycleAcceptance, PausedGracePreservesSharedBudget) {
    TimerConfig config;
    config.grace_reuse = GraceReuse::Shared;
    const auto result = advance_timer(grace_state(), config, {500'000, 500'000, true});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 90'000);
    EXPECT_TRUE(result.transitions.empty());
}

TEST(TimerLifecycleAcceptance, AcceptedRescueDoesNotOverrideAnExternalPause) {
    const auto rescued = apply_applicable_time_credit(grace_state(), 5000);
    const auto result = advance_timer(rescued, {}, {1000, 1000, true});
    EXPECT_EQ(result.state.phase, CountdownPhase::Main);
    EXPECT_EQ(result.state.main_remaining_ms, 5000);
    EXPECT_EQ(result.state.grace_remaining_ms, 90'000);
}

TEST(TimerLifecycleAcceptance, HardEndIsNotReachedOneUtcMillisecondEarly) {
    TimerConfig config;
    config.hard_end_utc_ms = 1000;
    const auto result = advance_timer(main_state(), config, {0, 999, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.main_remaining_ms, 1000);
    EXPECT_TRUE(result.transitions.empty());
}

TEST(TimerLifecycleAcceptance, ExactHardEndFinishesBeforeCountdownProjection) {
    TimerConfig config;
    config.hard_end_utc_ms = 1000;
    const auto result = advance_timer(main_state(), config, {1000, 1000, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    // Snapshot retention is a representation choice, not elapsed-at-deadline history.
    EXPECT_EQ(result.state.phase, CountdownPhase::Main);
    EXPECT_EQ(result.state.main_remaining_ms, 1000);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::HardEndReached, TimerTransition::Finished}));
}

TEST(TimerLifecycleAcceptance, HardEndFinishesPausedGraceWithoutPostponement) {
    TimerConfig config;
    config.hard_end_utc_ms = 1000;
    const auto result = advance_timer(grace_state(), config, {5000, 1000, true});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 90'000);
    EXPECT_EQ(result.transitions,
              (std::vector{TimerTransition::HardEndReached, TimerTransition::Finished}));
}

TEST(TimerLifecycleAcceptance, HardEndFinishesConfirmationWaitWithoutConfirmation) {
    TimerConfig config;
    config.end_mode = TimerEndMode::Confirmation;
    config.hard_end_utc_ms = 1000;
    const TimerState waiting{TimerLifecycle::Active, CountdownPhase::Main, 0, 120'000, true};
    const auto result = advance_timer(waiting, config, {0, 1000, true});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_FALSE(result.state.awaiting_confirmation);
}

TEST(TimerLifecycleAcceptance, UtcObservationDoesNotBecomeCountdownElapsed) {
    const auto result =
        advance_timer(main_state(), {}, {100, std::numeric_limits<std::int64_t>::max(), false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.main_remaining_ms, 900);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
}

TEST(TimerLifecycleAcceptance, FullSigned64MainDurationDoesNotOverflowPhaseArithmetic) {
    constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
    const auto result = advance_timer(main_state(maximum), {}, {maximum, 0, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 120'000);
}

TEST(TimerLifecycleAcceptance, VeryLargeOvershootFinishesWithoutNegativeDurations) {
    const auto result =
        advance_timer(main_state(), {}, {std::numeric_limits<std::int64_t>::max(), 0, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.state.main_remaining_ms, 0);
    EXPECT_EQ(result.state.grace_remaining_ms, 0);
    EXPECT_EQ(result.transitions.size(), 4U);
}

TEST(TimerLifecycleAcceptance, ReadyRepresentationRequiresASeparateStartBoundary) {
    const TimerState ready;
    EXPECT_EQ(ready.lifecycle, TimerLifecycle::Ready);
    EXPECT_THROW(static_cast<void>(advance_timer(ready, {}, {})), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(apply_applicable_time_credit(ready, 1000)),
                 std::invalid_argument);
}

TEST(TimerLifecycleAcceptance, FinishedStateNeverAdvancesOrAcceptsAnImplicitReopen) {
    const TimerState finished{TimerLifecycle::Finished, CountdownPhase::Grace, 0, 12'345, false};
    const auto result = advance_timer(finished, {}, {500'000, 500'000, false});
    EXPECT_EQ(result.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(result.state.grace_remaining_ms, 12'345);
    EXPECT_TRUE(result.transitions.empty());
    EXPECT_THROW(static_cast<void>(apply_applicable_time_credit(finished, 1000)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(confirm_timer_finish(finished)), std::invalid_argument);
}

TEST(TimerLifecycleAcceptance, ExpiryObservationPrecedesSameInstantRescueCommand) {
    const auto expired = advance_timer(grace_state(1), {}, {1, 1, false});
    EXPECT_EQ(expired.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_THROW(static_cast<void>(apply_applicable_time_credit(expired.state, 5000)),
                 std::invalid_argument);
    const auto still_available = advance_timer(grace_state(1), {}, {0, 0, false});
    const auto rescued = apply_applicable_time_credit(still_available.state, 5000);
    EXPECT_EQ(rescued.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(rescued.phase, CountdownPhase::Main);
}

TEST(TimerLifecycleAcceptance, HardEndObservationPrecedesSameInstantCreditCommand) {
    TimerConfig config;
    config.hard_end_utc_ms = 1000;
    const auto expired = advance_timer(grace_state(), config, {0, 1000, true});
    EXPECT_THROW(static_cast<void>(apply_applicable_time_credit(expired.state, 5000)),
                 std::invalid_argument);
}

TEST(TimerLifecycleAcceptance, NegativeClockDurationsAreRejected) {
    EXPECT_THROW(static_cast<void>(advance_timer(main_state(), {}, {-1, 0, false})),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(advance_timer(main_state(-1), {}, {})), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(advance_timer(grace_state(-1), {}, {})), std::invalid_argument);
    TimerConfig config;
    config.grace_duration_ms = -1;
    EXPECT_THROW(static_cast<void>(advance_timer(main_state(), config, {})), std::invalid_argument);
}

TEST(TimerLifecycleAcceptance, ApplicableCreditRejectsNegativeAndOverflowingAdjustments) {
    EXPECT_THROW(static_cast<void>(apply_applicable_time_credit(main_state(), -1)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(apply_applicable_time_credit(
                     main_state(std::numeric_limits<std::int64_t>::max()), 1)),
                 std::overflow_error);
    const auto result = apply_applicable_time_credit(main_state(), 5000);
    EXPECT_EQ(result.main_remaining_ms, 6000);
    EXPECT_EQ(result.phase, CountdownPhase::Main);
}
} // namespace
