// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include <subathon/core/recurring_schedule.hpp>
#include <subathon/core/timer_lifecycle.hpp>

namespace subathon::server {

struct ScheduleZoneRequest {
    std::string configured_zone;
    std::int64_t from_utc_ms;
    std::int64_t through_utc_ms;
    std::optional<std::string> required_rules_revision;
};

struct ScheduleZoneFacts {
    std::string configured_zone;
    std::string rules_revision;
    std::int64_t from_utc_ms;
    std::int64_t through_utc_ms;
    std::int32_t offset_at_start_seconds;
    std::vector<core::ScheduleZoneTransition> transitions;
};

enum class ScheduleZoneUnavailable {
    unknown_zone,
    insufficient_coverage,
    unsupported_rules,
    source_unavailable,
    limit_exceeded
};

struct ReachedRequestEnd {};
struct TransitionCap {};
struct TerminalPrefix {
    ScheduleZoneUnavailable cause;
};

struct ForecastPage {
    ScheduleZoneFacts facts;
    std::variant<ReachedRequestEnd, TransitionCap, TerminalPrefix> tail;
};

class ScheduleZoneSource {
  public:
    virtual ~ScheduleZoneSource() = default;
    virtual std::variant<ScheduleZoneFacts, ScheduleZoneUnavailable>
    read(const ScheduleZoneRequest &) = 0;

    // Closed forecast-page contract. Stage 1 exposes the seam before the bridge is implemented.
    virtual std::variant<ForecastPage, ScheduleZoneUnavailable>
    read_forecast_page(const ScheduleZoneRequest &) {
        return ScheduleZoneUnavailable::source_unavailable;
    }
};

struct ScheduledPauseRule {
    std::string schedule_id;
    core::RecurringSchedule recurrence;
    std::chrono::milliseconds duration;
    bool enabled;
};

struct ScheduledPauseConfiguration {
    std::string configured_zone;
    std::vector<ScheduledPauseRule> rules;
};

struct ScheduledPauseEpoch {
    ScheduledPauseConfiguration configuration;
    std::optional<std::int64_t> after_utc_ms;
    std::optional<std::int64_t> through_utc_ms;
    std::optional<std::string> closed_zone_rules_revision;
};

struct ScheduledPauseKey {
    std::string timer_lineage;
    std::string schedule_id;
    // Exact nominal local date/time and First or Second, as assigned by CORE-004.
    core::ScheduleOccurrenceId occurrence;
    bool operator==(const ScheduledPauseKey &) const = default;
};

struct ScheduledPauseObservation {
    std::string observation_ref;
    std::int64_t utc_ms;
};

enum class ScheduledPauseStopCause { expired, deleted, disabled };

struct ScheduledPauseStop {
    ScheduledPauseStopCause cause;
    std::int64_t effective_utc_ms;
    ScheduledPauseObservation reconciled_at;
};

struct DeliveredScheduledPause {
    ScheduledPauseKey key;
    core::ScheduledPauseWindow window;
    std::optional<ScheduledPauseStop> stop;
};

struct ScheduledPauseDeliveryStop {
    ScheduledPauseKey key;
    ScheduledPauseStop stop;
};

struct ScheduledPauseLedger {
    std::string timer_lineage;
    std::string revision;
    std::vector<DeliveredScheduledPause> deliveries;
};

struct ScheduledPauseLimits {
    static constexpr std::size_t max_rules = 128; // Per configuration.
    static constexpr std::size_t max_epochs = 64;
    static constexpr std::size_t max_deliveries = 16'384;
    static constexpr std::size_t max_projection_queries = 4096; // Source reads and core calls.
    static constexpr std::size_t max_clock_cuts = 4096;
    static constexpr std::size_t max_result_rows = 16'384;   // Per result sequence.
    static constexpr std::size_t max_reference_bytes = 4096; // Per opaque reference.
    static constexpr std::size_t max_input_bytes = 8'388'608;
    static constexpr std::size_t max_result_bytes = 16'777'216;
    static constexpr std::chrono::days max_duration{366};
    static constexpr std::chrono::days max_preview_span{366};
    static constexpr std::chrono::days max_progression_span{366}; // Bounded start scan.
};

struct ScheduledPauseElapsedCut {
    ScheduledPauseObservation observation;
    std::optional<std::chrono::milliseconds> elapsed_since_previous;
    core::PauseState independent_pause;
};

struct ScheduledPauseClockInput {
    std::string source_snapshot_ref;
    std::string clock_lineage;
    ScheduledPauseObservation anchor;               // Must match the current cursor and lineage.
    std::span<const ScheduledPauseElapsedCut> cuts; // Ordered, target last, nonempty.
};

enum class ScheduledPauseKnowledgeUnavailable { history, ledger, elapsed_split };
using ScheduledPauseCoverageFailure =
    std::variant<ScheduleZoneUnavailable, ScheduledPauseKnowledgeUnavailable>;

enum class ForecastSupportFailure { beyond_supported_timezone_transitions };
using ForecastFailure = std::variant<ScheduledPauseCoverageFailure, ForecastSupportFailure>;

struct ScheduledPauseCoverageHold {
    ScheduledPauseCoverageFailure cause;
    std::int64_t from_utc_ms;
};

struct ScheduledPauseCoverageNotice {
    ScheduledPauseCoverageHold hold;
    ScheduledPauseObservation observed_at;
};

struct ScheduledPauseCursor {
    ScheduledPauseObservation last_observation;
    std::optional<std::int64_t> evaluated_through_utc_ms;
    std::optional<ScheduledPauseCoverageHold> hold;
};

struct ScheduledPauseState {
    std::string snapshot_ref;
    std::string timer_lineage;
    std::string clock_lineage;
    std::string state_revision;
    std::string configuration_revision;
    core::TimerConfig timer_config;
    core::TimerState timer;
    core::PauseState pause;
    ScheduledPauseCursor cursor;
    std::span<const ScheduledPauseEpoch> epochs;
    bool epoch_history_complete;
    const ScheduledPauseLedger *ledger;
};

struct ScheduledPausePreviewRow {
    ScheduledPauseKey key;
    core::ScheduledPauseOccurrence window;
};

struct ScheduledPauseCutoverEffect {
    DeliveredScheduledPause delivery;
    std::optional<ScheduledPauseStop> stop_at_cutover;
};

struct ScheduledPauseFactBinding {
    std::optional<std::size_t> old_epoch_index;
    ScheduleZoneFacts facts;
};

struct ScheduledPausePreview {
    std::string source_snapshot_ref;
    std::string timer_lineage;
    std::string clock_lineage;
    std::string state_revision;
    std::string configuration_revision;
    std::string delivery_revision;
    ScheduledPauseObservation as_of;
    ScheduledPauseConfiguration proposed;
    std::int64_t requested_through_utc_ms;
    std::optional<std::int64_t> resolved_through_utc_ms;
    std::optional<ForecastFailure> incomplete;
    std::vector<ScheduledPauseFactBinding> facts;
    std::vector<ScheduledPausePreviewRow> upcoming;
    std::vector<ScheduledPauseCutoverEffect> cutover_effects;
    std::optional<core::PauseState> pause_after_cutover;
};

struct ScheduledPauseTransitions {
    ScheduledPauseObservation observation;
    std::vector<core::TimerTransition> transitions;
};

struct ScheduledPauseProgress {
    std::string source_snapshot_ref;
    core::TimerState timer;
    core::PauseState pause;
    ScheduledPauseCursor cursor;
    std::optional<std::int64_t> starts_resolved_through_utc_ms;
    std::optional<std::int64_t> next_known_expiry_utc_ms;
    std::optional<ScheduledPauseCoverageNotice> notice; // Internal facts, present iff held.
    std::vector<DeliveredScheduledPause> new_deliveries;
    std::vector<ScheduledPauseDeliveryStop> stops;
    std::vector<ScheduledPauseTransitions> timer_transitions;
};

struct ScheduledPauseCutover {
    ScheduledPauseObservation cutover;
    ScheduledPauseConfiguration configuration;
    std::optional<std::string> closed_zone_rules_revision;
    ScheduledPauseProgress progress;
};

enum class ScheduledPauseRefusal {
    invalid_input,
    limit_exceeded,
    stale_observation,
    hard_end_reached
};

// The owner supplies the exact trusted observation and admitted Start/Reopen timer proposal.
struct ScheduledPauseEntry {
    std::string source_snapshot_ref;
    ScheduledPauseObservation observation;
    core::TimerState admitted_timer;
};

// These projections use one trusted basis. They return proposal data to the owner.
[[nodiscard]] std::variant<ScheduledPausePreview, ScheduledPauseRefusal>
project_scheduled_pause_edit(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                             const ScheduledPauseConfiguration &, std::int64_t through_utc_ms,
                             ScheduleZoneSource &);

// Return coverage failure when the old-side effects at cutover cannot be certified.
// A complete cutover may carry a progress hold when post-cutover starts lack coverage.
[[nodiscard]] std::variant<ScheduledPauseCutover, ScheduledPauseCoverageFailure,
                           ScheduledPauseRefusal>
project_scheduled_pause_cutover(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                                const ScheduledPauseConfiguration &, ScheduleZoneSource &);

// Entry requires a Ready or Finished basis and a valid Active proposal. Malformed input,
// stale observations and a reached hard end have distinct refusals. Missing schedule
// knowledge gives held progress. The trusted entry observation anchors later progression.
[[nodiscard]] std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
enter_scheduled_pauses(const ScheduledPauseState &, const ScheduledPauseEntry &,
                       ScheduleZoneSource &);

[[nodiscard]] std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
progress_scheduled_pauses(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                          ScheduleZoneSource &);

} // namespace subathon::server
