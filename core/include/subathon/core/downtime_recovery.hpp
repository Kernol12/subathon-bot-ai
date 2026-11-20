// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/recurring_schedule.hpp>
#include <subathon/core/timer_editorial_commands.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace subathon::core {

// CORE-006 candidate revision 1. All results are pure proposals. References bind
// supplied facts for an authoritative caller and confer no access or write rights.
enum class DowntimePolicy { Freeze, CountElapsed, Review };

struct RecoveryUtcInstant {
    std::int64_t ms;
    bool operator==(const RecoveryUtcInstant &) const = default;
};

struct RecoveryUtcObservation {
    std::string observation_ref;
    // Missing means unavailable from the clock owner for this observation.
    std::optional<RecoveryUtcInstant> utc;
};

enum class RecoveryGapUnavailable { Unknown, Discontinuous, Unrepresentable };

struct RecoveryContinuousGap {
    std::string evidence_ref;
    // Qualified closed UTC span; endpoints must match the two observations.
    RecoveryUtcInstant checkpoint;
    RecoveryUtcInstant restart;
};

using RecoveryGapClock = std::variant<RecoveryGapUnavailable, RecoveryContinuousGap>;

enum class RecoveryScheduleUnavailable { Coverage, DeliveryContext, Source, ReconstructionLimit };

struct RecoveryRetainedPause {
    std::string schedule_ref;
    ScheduleOccurrenceId occurrence;
    ScheduledPauseWindow window;
    // A retained stop shortens the effective window, possibly to empty.
    std::optional<RecoveryUtcInstant> stopped_at;
};

struct RecoveryRecurringPause {
    std::string schedule_ref;
    RecurringSchedule recurrence;
    std::chrono::milliseconds duration;
    // Exact IDs already delivered for this schedule and timer lineage.
    // May include expired occurrences outside the retained-window span.
    std::span<const ScheduleOccurrenceId> delivered;
};

struct RecoveryScheduleCoverage {
    std::string facts_ref;
    std::string delivery_revision;
    // Core selects offline starts via CORE-004 using the captured active schedules
    // and configured zone. An empty active configuration needs no zone facts.
    std::optional<ScheduleZoneRules> zone;
    std::span<const RecoveryRecurringPause> schedules;
    std::span<const RecoveryRetainedPause> retained;
};

using RecoveryScheduleFacts = std::variant<RecoveryScheduleUnavailable, RecoveryScheduleCoverage>;

struct RecoveryBasis {
    std::string timer_lineage;
    std::string checkpoint_ref;
    std::string configuration_revision;
    // Immutable binding of the complete input, including explicit unavailable facts.
    std::string facts_bundle_ref;
};

struct DowntimeRecoveryInput {
    RecoveryBasis basis;
    DowntimePolicy policy{DowntimePolicy::Freeze};
    TimerState timer;
    TimerConfig config;
    PauseState pause;
    RecoveryUtcObservation checkpoint;
    RecoveryUtcObservation restart;
    RecoveryGapClock gap;
    RecoveryScheduleFacts schedule;
};

enum class RecoveryTransitionBasis { ReconstructedUtc, ObservedUtc };

struct RecoveryTransition {
    TimerTransition transition;
    RecoveryUtcInstant utc;
    RecoveryTransitionBasis basis;
};

struct RecoveryTimerProposal {
    TimerState timer;
    PauseState pause;
    // At most four ordered lifecycle proposals. A trace is not a delivery or receipt.
    std::vector<RecoveryTransition> transitions;
};

enum class RecoveryDeadlineStatus { Disabled, NotReached, Reached, Unknown };

struct DowntimeRecoveryPreview {
    RecoveryBasis basis;
    DowntimePolicy policy;
    RecoveryTimerProposal freeze;
    std::optional<RecoveryTimerProposal> count_elapsed;
    // The policy selects this hold for Review or when CountElapsed is unavailable.
    // Start from Freeze, including a reached hard end, and add RecoveryReview.
    std::optional<RecoveryTimerProposal> review_hold;
    RecoveryDeadlineStatus deadline_at_restart;
    std::optional<RecoveryGapUnavailable> gap_unavailable;
    std::optional<RecoveryScheduleUnavailable> schedule_unavailable;
    // Original clock evidence and nominal identities remain in the immutable bundle.
};

enum class RecoveryPreviewRefusal { InputLimit, InvalidInput, ReviewAlreadyPending };

// Freeze preserves the durable countdown, subject to an independently observed
// hard end. A complete schedule supplies its restart Schedule reason; otherwise
// retain the checkpoint reason and its uncertainty. Count elapsed is published
// only for a fully reconstructible gap. If CountElapsed has no result, select the
// Review hold. The unavailable CountElapsed choice stays unavailable in Review.
// An explicit Freeze choice and an authorized correction follow the normal
// resolution rules. A checkpoint already in RecoveryReview requires its original
// episode binding. Preview does not admit support.
[[nodiscard]] std::variant<DowntimeRecoveryPreview, RecoveryPreviewRefusal>
preview_downtime_recovery(const DowntimeRecoveryInput &input);

struct RecoveryReviewSnapshot {
    std::string episode_ref;
    std::string timer_lineage;
    std::string facts_bundle_ref;
    // Current whole revision includes the recovery and pause state as well as
    // the timer, configuration, cap and debt used for any editorial correction.
    TimerEditorialSnapshot current;
    PauseState pause;
    RecoveryUtcObservation observed_at;
};

enum class RecoveryWaitRefusal { InputLimit, InvalidInput, NoReviewPending };

struct RecoveryReviewWait {
    RecoveryTimerProposal held;
    RecoveryDeadlineStatus deadline;
};

// Independent of a resolution command. Waiting contributes zero countdown debit.
// Observe the hard end against current config and retain all current pause fields.
[[nodiscard]] std::variant<RecoveryReviewWait, RecoveryWaitRefusal>
observe_recovery_review_wait(const RecoveryReviewSnapshot &snapshot);

enum class RecoveryOfflineChoice { Freeze, CountElapsed };

struct RecoveryReviewCommand {
    std::string operation_ref;
    std::string expected_episode_ref;
    std::string expected_revision;
    std::string expected_facts_bundle_ref;
    // Resolution requires an explicit choice. Absence never selects Freeze.
    std::optional<RecoveryOfflineChoice> choice;
    std::optional<TimerSignedDelta> correction;
    std::optional<std::string> reason;
};

enum class RecoveryReviewAccessRefusal { Denied, Unknown };

struct RecoveryReviewAuthorization {
    // Trusted echo of the exact command, including choice, correction and reason.
    // Core compares every field. The command service establishes actual authority.
    RecoveryReviewCommand command;
    std::string revision;
    std::string observation_ref;
    std::string actor_ref;
    std::string permission_revision;
    std::optional<TimerEditorialAccess> correction_access;
};

using RecoveryReviewAccess = std::variant<RecoveryReviewAccessRefusal, RecoveryReviewAuthorization>;

enum class RecoveryReviewRefusal {
    InputLimit,
    InvalidInput,
    AccessDenied,
    AccessUnknown,
    StaleReview,
    NoReviewPending,
    ChoiceUnavailable,
    DeadlineUnknown,
    WaitObservationRequired,
    CorrectionUnavailable
};

struct RecoveryReviewResolution {
    RecoveryReviewCommand command;
    RecoveryReviewAuthorization authorization;
    RecoveryBasis basis;
    RecoveryUtcObservation decision_observation;
    RecoveryTimerProposal selected_at_restart;
    TimerEditorialValues resolved;
    PauseState pause;
    // Optional accepted correction retains the owning editorial audit arithmetic.
    std::optional<TimerEditorialAdjustment> correction;
    TimerEditorialTransitionBatch correction_transitions;
    // This result removes only the clock-recovery eligibility blocker. The owner
    // re-evaluates each held input against its unchanged independent blockers.
};

struct RecoveryCorrectionRefusal {
    TimerEditorialDisposition editorial_disposition;
};

// Recompute the retained offline preview. Recheck exact revision/episode/facts
// binding and current permission facts. On success, use the current independent
// pause fields and clear RecoveryReview alone. The countdown is never advanced
// through the decision wait. No refusal applies a partial selection or correction.
[[nodiscard]] std::variant<RecoveryReviewResolution, RecoveryReviewRefusal,
                           RecoveryCorrectionRefusal>
resolve_recovery_review(const DowntimeRecoveryInput &preview_input,
                        const RecoveryReviewSnapshot &snapshot,
                        const RecoveryReviewCommand &command, const RecoveryReviewAccess &access);

// The aggregate limit covers captured schedules, delivered IDs and retained rows.
inline constexpr std::size_t recovery_max_schedule_entries = 16'384;
inline constexpr std::size_t recovery_max_schedule_previews = 4'096;
inline constexpr std::size_t recovery_max_schedule_windows = 16'384;
inline constexpr std::size_t recovery_max_reference_bytes = 4'096;
// Applies to the combined logical input of each call before allocation.
inline constexpr std::size_t recovery_max_input_bytes = 8'388'608;

} // namespace subathon::core
