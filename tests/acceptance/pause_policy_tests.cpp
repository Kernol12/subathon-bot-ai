// SPDX-License-Identifier: GPL-3.0-or-later
// Specification v0.45 §4/TIMER-03/05 and §18; pause-policy-v1.md proposed revision 1.
// Expected snapshots are manually derived from durations and independent set membership.
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/pause_policy.hpp>

namespace {
using namespace subathon::core;
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
constexpr auto minimum = std::numeric_limits<std::int64_t>::min();

void expect_snapshot(const PauseState &actual, const PauseState &expected) {
    EXPECT_EQ(actual.reasons, expected.reasons);
    EXPECT_EQ(actual.stream_offline_pending_ms, expected.stream_offline_pending_ms);
    EXPECT_EQ(actual.client_disconnect_pending_ms, expected.client_disconnect_pending_ms);
    EXPECT_EQ(actual.stream_unknown_pending_ms, expected.stream_unknown_pending_ms);
    EXPECT_EQ(actual.stream_unknown_notice, expected.stream_unknown_notice);
}

TEST(PausePolicyAcceptance, OfflineDefaultDelayDoesNotPauseUntilItsExactBoundary) {
    const PauseConfig config;
    EXPECT_EQ(config.stream_offline_delay_ms, 30'000);
    EXPECT_EQ(config.client_disconnect_delay_ms, 30'000);
    EXPECT_FALSE(config.stream_unknown_timeout_ms.has_value());
    auto state = observe_pause_signals({}, config, {0, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 0, std::nullopt, std::nullopt, false});
    state = observe_pause_signals(state, config, {29'999, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 29'999, std::nullopt, std::nullopt, false});
    state = observe_pause_signals(state, config, {1, StreamStatus::Offline, true});
    expect_snapshot(state, {{PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, DesignatedClientHasItsOwnDefaultThirtySecondDelay) {
    auto state = observe_pause_signals({}, {}, {0, StreamStatus::Online, false});
    expect_snapshot(state, {{}, std::nullopt, 0, std::nullopt, false});
    state = observe_pause_signals(state, {}, {29'999, StreamStatus::Online, false});
    expect_snapshot(state, {{}, std::nullopt, 29'999, std::nullopt, false});
    state = observe_pause_signals(state, {}, {1, StreamStatus::Online, false});
    expect_snapshot(state, {{PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, CustomOfflineAndClientDelaysExpireIndependently) {
    const PauseConfig config{10, 20, std::nullopt};
    auto state = observe_pause_signals({}, config, {9, StreamStatus::Offline, false});
    expect_snapshot(state, {{}, 9, 9, std::nullopt, false});
    state = observe_pause_signals(state, config, {1, StreamStatus::Offline, false});
    expect_snapshot(state, {{PauseReason::StreamOffline}, std::nullopt, 10});
    state = observe_pause_signals(state, config, {10, StreamStatus::Offline, false});
    expect_snapshot(state,
                    {{PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, EarlierClientExpiryDoesNotRestartTheOfflineDelay) {
    const PauseConfig config{20, 10, std::nullopt};
    auto state = observe_pause_signals({}, config, {10, StreamStatus::Offline, false});
    expect_snapshot(state, {{PauseReason::DesignatedClientDisconnected}, 10});
    state = observe_pause_signals(state, config, {9, StreamStatus::Offline, false});
    expect_snapshot(state, {{PauseReason::DesignatedClientDisconnected}, 19});
    state = observe_pause_signals(state, config, {1, StreamStatus::Offline, false});
    expect_snapshot(state,
                    {{PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, ZeroDelaysActivateWithoutAnExtraTick) {
    const auto state =
        observe_pause_signals({}, {0, 0, std::nullopt}, {0, StreamStatus::Offline, false});
    expect_snapshot(state,
                    {{PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, ZeroOfflineDelayDoesNotMakeClientDelayImmediate) {
    const auto state =
        observe_pause_signals({}, {0, 30'000, std::nullopt}, {0, StreamStatus::Offline, false});
    expect_snapshot(state, {{PauseReason::StreamOffline}, std::nullopt, 0});
}

TEST(PausePolicyAcceptance, OfflineRecoveryBeforeExpiryCancelsAndNextEpisodeStartsFresh) {
    auto state = observe_pause_signals({}, {}, {12'000, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 12'000});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Online, true});
    expect_snapshot(state, {});
    state = observe_pause_signals(state, {}, {29'999, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 29'999});
    state = observe_pause_signals(state, {}, {1, StreamStatus::Offline, true});
    expect_snapshot(state, {{PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, ClientFlappingCancelsItsPendingDelayWithoutClearingManual) {
    PauseState state{{PauseReason::Manual}};
    state = observe_pause_signals(state, {}, {29'999, StreamStatus::Online, false});
    expect_snapshot(state, {{PauseReason::Manual}, std::nullopt, 29'999});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Online, true});
    expect_snapshot(state, {{PauseReason::Manual}});
    state = observe_pause_signals(state, {}, {1, StreamStatus::Online, false});
    expect_snapshot(state, {{PauseReason::Manual}, std::nullopt, 1});
    state = observe_pause_signals(state, {}, {29'999, StreamStatus::Online, false});
    expect_snapshot(state, {{PauseReason::Manual, PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, RepeatedOfflineSignalDoesNotRestartPendingOrDuplicateActiveReason) {
    auto state = observe_pause_signals({}, {}, {12'000, StreamStatus::Offline, true});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 12'000});
    state = observe_pause_signals(state, {}, {17'999, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 29'999});
    state = observe_pause_signals(state, {}, {1, StreamStatus::Offline, true});
    state = observe_pause_signals(state, {}, {maximum, StreamStatus::Offline, true});
    expect_snapshot(state, {{PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, OfflineRecoveryRetainsManualScheduleRecoveryAndClientReasons) {
    const PauseState state{{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                            PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}};
    const auto result = observe_pause_signals(state, {}, {0, StreamStatus::Online, false});
    expect_snapshot(result,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, ClientRecoveryRetainsOfflineUnknownAndOtherIndependentHolds) {
    const PauseState state{{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                            PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected,
                            PauseReason::StreamStatusUnknown},
                           std::nullopt,
                           std::nullopt,
                           std::nullopt,
                           true};
    const auto result =
        observe_pause_signals(state, {30'000, 30'000, 10}, {0, StreamStatus::Unknown, true});
    expect_snapshot(result,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::StreamStatusUnknown},
                     std::nullopt,
                     std::nullopt,
                     std::nullopt,
                     true});
}

TEST(PausePolicyAcceptance, ExternalPauseReasonsAreSetAndClearedIndependently) {
    PauseState state{{PauseReason::StreamOffline}};
    state = set_pause_reason(state, PauseReason::Manual, true);
    state = set_pause_reason(state, PauseReason::Schedule, true);
    state = set_pause_reason(state, PauseReason::RecoveryReview, true);
    expect_snapshot(state, {{PauseReason::Manual, PauseReason::Schedule,
                             PauseReason::RecoveryReview, PauseReason::StreamOffline}});
    state = set_pause_reason(state, PauseReason::RecoveryReview, false);
    expect_snapshot(state,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::StreamOffline}});
    state = set_pause_reason(state, PauseReason::Manual, false);
    expect_snapshot(state, {{PauseReason::Schedule, PauseReason::StreamOffline}});
    state = set_pause_reason(state, PauseReason::Schedule, false);
    expect_snapshot(state, {{PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, RepeatedExternalSetAndClearPreservePendingSignals) {
    const PauseState initial{{PauseReason::Schedule}, 5, 7};
    auto state = set_pause_reason(initial, PauseReason::Manual, true);
    state = set_pause_reason(state, PauseReason::Manual, true);
    expect_snapshot(state, {{PauseReason::Manual, PauseReason::Schedule}, 5, 7});
    state = set_pause_reason(state, PauseReason::Manual, false);
    state = set_pause_reason(state, PauseReason::Manual, false);
    expect_snapshot(state, initial);
}

TEST(PausePolicyAcceptance, UnknownDefaultRaisesNoticeWithoutAddingAnyPauseOrPendingTimeout) {
    auto state = observe_pause_signals({}, {}, {maximum, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, std::nullopt, true});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, std::nullopt, true});
}

TEST(PausePolicyAcceptance, UnknownDefaultRetainsConfirmedOfflineAndExternalReasons) {
    const PauseState state{{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                            PauseReason::StreamOffline}};
    const auto result = observe_pause_signals(state, {}, {maximum, StreamStatus::Unknown, true});
    expect_snapshot(result, {{PauseReason::Manual, PauseReason::Schedule,
                              PauseReason::RecoveryReview, PauseReason::StreamOffline},
                             std::nullopt,
                             std::nullopt,
                             std::nullopt,
                             true});
}

TEST(PausePolicyAcceptance, UnknownIsNotOfflineEvenWhenOfflineDelayIsZero) {
    const auto result = observe_pause_signals({}, {0, 30'000, std::nullopt},
                                              {maximum, StreamStatus::Unknown, true});
    expect_snapshot(result, {{}, std::nullopt, std::nullopt, std::nullopt, true});
}

TEST(PausePolicyAcceptance, KnownOnlineClearsNoticeWhenNoOptionalUnknownPauseExists) {
    auto state = observe_pause_signals({}, {}, {5, StreamStatus::Unknown, true});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Online, true});
    expect_snapshot(state, {});
}

TEST(PausePolicyAcceptance, OptionalUnknownTimeoutAddsOnlyItsReasonAtTheExactBoundary) {
    const PauseConfig config{30'000, 30'000, 10};
    auto state = observe_pause_signals({}, config, {0, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, 0, true});
    state = observe_pause_signals(state, config, {9, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, 9, true});
    state = observe_pause_signals(state, config, {1, StreamStatus::Unknown, true});
    expect_snapshot(
        state,
        {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
    state = observe_pause_signals(state, config, {maximum, StreamStatus::Unknown, true});
    expect_snapshot(
        state,
        {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
}

TEST(PausePolicyAcceptance, UnknownTimeoutAddsAlongsideConfirmedOfflineAndEveryOtherHold) {
    const PauseState state{{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                            PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}};
    const auto result =
        observe_pause_signals(state, {30'000, 30'000, 10}, {10, StreamStatus::Unknown, false});
    expect_snapshot(result,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected,
                      PauseReason::StreamStatusUnknown},
                     std::nullopt,
                     std::nullopt,
                     std::nullopt,
                     true});
}

TEST(PausePolicyAcceptance, ZeroUnknownTimeoutAddsAnIndependentReasonWithoutAnExtraTick) {
    const PauseState state{{PauseReason::Manual, PauseReason::StreamOffline}};
    const auto result =
        observe_pause_signals(state, {30'000, 30'000, 0}, {0, StreamStatus::Unknown, true});
    expect_snapshot(result, {{PauseReason::Manual, PauseReason::StreamOffline,
                              PauseReason::StreamStatusUnknown},
                             std::nullopt,
                             std::nullopt,
                             std::nullopt,
                             true});
}

TEST(PausePolicyAcceptance, LargestSignedDisconnectDelaysReachTheirExactBoundary) {
    const PauseConfig config{maximum, maximum, std::nullopt};
    auto state = observe_pause_signals({}, config, {maximum - 1, StreamStatus::Offline, false});
    expect_snapshot(state, {{}, maximum - 1, maximum - 1});
    state = observe_pause_signals(state, config, {1, StreamStatus::Offline, false});
    expect_snapshot(state,
                    {{PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, OversizedElapsedCannotOverflowEitherPendingDisconnectAccumulator) {
    const PauseConfig config{maximum, maximum, std::nullopt};
    const PauseState state{{}, maximum - 1, maximum - 1};
    const auto result =
        observe_pause_signals(state, config, {maximum, StreamStatus::Offline, false});
    expect_snapshot(result,
                    {{PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, LargestUnknownTimeoutAndOvershootStayWithinSignedBounds) {
    const PauseConfig config{30'000, 30'000, maximum};
    const PauseState pending{{}, std::nullopt, std::nullopt, maximum - 1, true};
    const auto exact = observe_pause_signals(pending, config, {1, StreamStatus::Unknown, true});
    expect_snapshot(
        exact,
        {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
    const auto overshoot =
        observe_pause_signals(pending, config, {maximum, StreamStatus::Unknown, true});
    expect_snapshot(
        overshoot,
        {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
}

TEST(PausePolicyAcceptance, NegativeConfigurationDurationsRejectBeforeProposingChanges) {
    for (const auto &invalid :
         {PauseConfig{-1, 30'000, std::nullopt}, PauseConfig{30'000, minimum, std::nullopt},
          PauseConfig{30'000, 30'000, -1}}) {
        EXPECT_THROW((void)observe_pause_signals({}, invalid, {0, StreamStatus::Online, true}),
                     std::invalid_argument);
    }
}

TEST(PausePolicyAcceptance, NegativeElapsedIncludingSignedMinimumRejects) {
    for (const auto elapsed : {std::int64_t{-1}, minimum}) {
        EXPECT_THROW((void)observe_pause_signals({}, {}, {elapsed, StreamStatus::Offline, false}),
                     std::invalid_argument);
    }
}

TEST(PausePolicyAcceptance, NegativePendingDurationsRejectRatherThanRepairingSnapshots) {
    const PauseConfig config{30'000, 30'000, 10};
    for (const auto &invalid : {PauseState{{}, -1}, PauseState{{}, std::nullopt, minimum},
                                PauseState{{}, std::nullopt, std::nullopt, -1, true}}) {
        EXPECT_THROW(
            (void)observe_pause_signals(invalid, config, {0, StreamStatus::Unknown, false}),
            std::invalid_argument);
        EXPECT_THROW((void)set_pause_reason(invalid, PauseReason::Manual, true),
                     std::invalid_argument);
    }
}

TEST(PausePolicyAcceptance, AnActiveReasonCannotAlsoHaveItsOwnPendingDelay) {
    const PauseConfig config{30'000, 30'000, 10};
    for (const auto &invalid :
         {PauseState{{PauseReason::StreamOffline}, 0},
          PauseState{{PauseReason::DesignatedClientDisconnected}, std::nullopt, 0},
          PauseState{{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, 0, true}}) {
        EXPECT_THROW(
            (void)observe_pause_signals(invalid, config, {0, StreamStatus::Unknown, false}),
            std::invalid_argument);
        EXPECT_THROW((void)set_pause_reason(invalid, PauseReason::Manual, true),
                     std::invalid_argument);
    }
}

TEST(PausePolicyAcceptance, PendingAtOrBeyondConfiguredThresholdIsAnIncoherentSnapshot) {
    const PauseConfig config{10, 20, 30};
    for (const auto &invalid :
         {PauseState{{}, 10}, PauseState{{}, 11}, PauseState{{}, std::nullopt, 20},
          PauseState{{}, std::nullopt, std::nullopt, 30, true}}) {
        EXPECT_THROW(
            (void)observe_pause_signals(invalid, config, {0, StreamStatus::Unknown, false}),
            std::invalid_argument);
    }
    EXPECT_THROW((void)observe_pause_signals(PauseState{{}, 0}, {0, 20, 30},
                                             {0, StreamStatus::Offline, true}),
                 std::invalid_argument);
}

TEST(PausePolicyAcceptance, UnknownPendingRequiresEnabledTimeoutAndUncertaintyNotice) {
    const PauseState pending{{}, std::nullopt, std::nullopt, 0, true};
    EXPECT_THROW((void)observe_pause_signals(pending, {}, {0, StreamStatus::Unknown, true}),
                 std::invalid_argument);
    const PauseState no_notice{{}, std::nullopt, std::nullopt, 0, false};
    EXPECT_THROW((void)observe_pause_signals(no_notice, {30'000, 30'000, 10},
                                             {0, StreamStatus::Unknown, true}),
                 std::invalid_argument);
    EXPECT_THROW((void)set_pause_reason(no_notice, PauseReason::Manual, true),
                 std::invalid_argument);
}

TEST(PausePolicyAcceptance, ExternalReasonCommandsCannotBypassSignalOwnership) {
    for (const auto reason : {PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected,
                              PauseReason::StreamStatusUnknown}) {
        EXPECT_THROW((void)set_pause_reason({}, reason, true), std::invalid_argument);
        EXPECT_THROW((void)set_pause_reason(PauseState{{reason}}, reason, false),
                     std::invalid_argument);
    }
}

TEST(PausePolicyAcceptance, UnknownCancelsPendingOfflineEvenAcrossALongUnknownInterval) {
    auto state = observe_pause_signals({}, {}, {29'999, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 29'999});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, std::nullopt, true});
    state = observe_pause_signals(state, {}, {maximum, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, std::nullopt, true});
    state = observe_pause_signals(state, {}, {0, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 0});
    state = observe_pause_signals(state, {}, {29'999, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 29'999});
    state = observe_pause_signals(state, {}, {1, StreamStatus::Offline, true});
    expect_snapshot(state, {{PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, UnknownCancellationRestartsTheFullCustomOfflineDelay) {
    const PauseConfig config{7, 30'000, 20};
    auto state = observe_pause_signals({}, config, {6, StreamStatus::Offline, true});
    state = observe_pause_signals(state, config, {3, StreamStatus::Unknown, true});
    expect_snapshot(state, {{}, std::nullopt, std::nullopt, 3, true});
    state = observe_pause_signals(state, config, {6, StreamStatus::Offline, true});
    expect_snapshot(state, {{}, 6});
    state = observe_pause_signals(state, config, {1, StreamStatus::Offline, true});
    expect_snapshot(state, {{PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, EitherKnownStatusCancelsPendingUnknownAndNextEpisodeStartsFresh) {
    const PauseConfig config{30'000, 30'000, 10};
    for (const auto known : {StreamStatus::Online, StreamStatus::Offline}) {
        auto state = observe_pause_signals({}, config, {9, StreamStatus::Unknown, true});
        expect_snapshot(state, {{}, std::nullopt, std::nullopt, 9, true});
        state = observe_pause_signals(state, config, {0, known, true});
        const PauseState recovered =
            known == StreamStatus::Offline ? PauseState{{}, 0} : PauseState{};
        expect_snapshot(state, recovered);
        state = observe_pause_signals(state, config, {9, StreamStatus::Unknown, true});
        expect_snapshot(state, {{}, std::nullopt, std::nullopt, 9, true});
        state = observe_pause_signals(state, config, {1, StreamStatus::Unknown, true});
        expect_snapshot(
            state,
            {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
    }
}

TEST(PausePolicyAcceptance, KnownOfflineClearsActivatedUnknownWhileRetainingAllUnrecoveredHolds) {
    const PauseConfig config{30'000, 30'000, 10};
    PauseState state{{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}};
    state = observe_pause_signals(state, config, {10, StreamStatus::Unknown, false});
    expect_snapshot(state,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected,
                      PauseReason::StreamStatusUnknown},
                     std::nullopt,
                     std::nullopt,
                     std::nullopt,
                     true});
    state = observe_pause_signals(state, config, {0, StreamStatus::Offline, false});
    expect_snapshot(state,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected}});
    state = observe_pause_signals(state, config, {9, StreamStatus::Unknown, false});
    expect_snapshot(state,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected},
                     std::nullopt,
                     std::nullopt,
                     9,
                     true});
    state = observe_pause_signals(state, config, {1, StreamStatus::Unknown, false});
    expect_snapshot(state,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected,
                      PauseReason::StreamStatusUnknown},
                     std::nullopt,
                     std::nullopt,
                     std::nullopt,
                     true});
}

TEST(PausePolicyAcceptance, KnownOnlineComposesOfflineAndUnknownRecoveryWithoutGlobalResume) {
    const PauseConfig config{30'000, 30'000, 10};
    const PauseState state{{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                            PauseReason::StreamOffline, PauseReason::DesignatedClientDisconnected,
                            PauseReason::StreamStatusUnknown},
                           std::nullopt,
                           std::nullopt,
                           std::nullopt,
                           true};
    const auto result = observe_pause_signals(state, config, {0, StreamStatus::Online, false});
    expect_snapshot(result,
                    {{PauseReason::Manual, PauseReason::Schedule, PauseReason::RecoveryReview,
                      PauseReason::DesignatedClientDisconnected}});
}

TEST(PausePolicyAcceptance, KnownOfflineAndConnectedClientEachClearTheirOwnReason) {
    const PauseState state{{PauseReason::Manual, PauseReason::StreamOffline,
                            PauseReason::DesignatedClientDisconnected,
                            PauseReason::StreamStatusUnknown},
                           std::nullopt,
                           std::nullopt,
                           std::nullopt,
                           true};
    const auto result =
        observe_pause_signals(state, {30'000, 30'000, 10}, {0, StreamStatus::Offline, true});
    expect_snapshot(result, {{PauseReason::Manual, PauseReason::StreamOffline}});
}

TEST(PausePolicyAcceptance, ZeroUnknownTimeoutRearmsImmediatelyAfterEitherKnownStatus) {
    const PauseConfig config{30'000, 30'000, 0};
    for (const auto known : {StreamStatus::Online, StreamStatus::Offline}) {
        auto state = observe_pause_signals({}, config, {0, StreamStatus::Unknown, true});
        expect_snapshot(
            state,
            {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
        state = observe_pause_signals(state, config, {0, known, true});
        const PauseState recovered =
            known == StreamStatus::Offline ? PauseState{{}, 0} : PauseState{};
        expect_snapshot(state, recovered);
        state = observe_pause_signals(state, config, {0, StreamStatus::Unknown, true});
        expect_snapshot(
            state,
            {{PauseReason::StreamStatusUnknown}, std::nullopt, std::nullopt, std::nullopt, true});
    }
}
} // namespace
