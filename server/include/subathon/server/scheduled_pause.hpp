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

// SCHEDULE-001 #720, contract revision 2. Application boundary for configured-zone
// previews and same-lineage timer execution. Clock, configuration, access and timer
// state come from their authoritative owners on the command queue. These data and
// proposals carry no command, clock-trust, persistence or publication authority.

struct ScheduleZoneRequest {
    std::string configured_zone;
    // Closed interval. A bounded source may certify a prefix starting at this
    // lower endpoint. A full preview requires the entire requested interval.
    std::int64_t from_utc_ms;
    std::int64_t through_utc_ms;
    // A closed configuration epoch may require an archived rules revision.
    // Absent requests the source's current facts.
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

// Use the explicitly selected zone, never the host/client default or the zone's
// current offset extrapolated into an uncertified interval. An explicitly configured
// fixed-offset zone may certify constant facts. Deterministic synthetic sources
// implement the same port with exact supplied facts, including failure cases.
// Success certifies every effective offset in the returned closed interval. The
// offset at its lower endpoint already includes any transition there. Include all
// subsequent offset changes up to and including its upper endpoint, in order, with
// the new offset effective at each boundary. An empty list is meaningful only
// with certified complete coverage. Offsets are integral seconds east of UTC,
// |offset| <= 24h, with at most two inverse mappings and 64 transitions per core
// input. Incoherent tags, bounds, rows or conflicting overlapping facts are unusable.
// Match the requested zone exactly. One immutable rules revision binds each zone
// view across chunks. Archived views and the current view have separate bindings.
// Unknown, insufficient, unsupported and over-budget input grant no coverage beyond
// a separately complete, coherent prefix. Never truncate rows to claim coverage.
class ScheduleZoneSource {
  public:
    virtual ~ScheduleZoneSource() = default;
    virtual std::variant<ScheduleZoneFacts, ScheduleZoneUnavailable>
    read(const ScheduleZoneRequest &) = 0;
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

// Old epochs are in cutover order. Each supplies starts in (after, through];
// absent after is the original configuration, absent through is the open epoch.
// A closed epoch requires its certified rules revision for any lookup it needs.
// A missing revision for such a lookup is unavailable. Retain the complete
// history relevant to entry, including disable/delete and re-enable boundaries.
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
    // Effective end of the contribution is separate from the immutable window.
    // Once stopped, this delivery can never contribute again.
    std::optional<ScheduledPauseStop> stop;
};

struct ScheduledPauseDeliveryStop {
    ScheduledPauseKey key;
    ScheduledPauseStop stop;
};

struct ScheduledPauseLedger {
    std::string timer_lineage;
    std::string revision;
    // Complete same-lineage deduplication evidence, including stopped deliveries.
    // Unknown/restored-incomplete is represented by an absent ledger.
    std::vector<DeliveredScheduledPause> deliveries;
};

// Fixed ceilings per call, including old and proposed inputs and all source work.
// Each core query remains within its 366d/64-transition limits. Tile longer query
// ranges at exclusive/inclusive boundaries with complete padding. Preflight sizes,
// counts and arithmetic before proportional work/copying, including source reads;
// the port must bound acquisition before materializing a response. Count aggregate
// input and output bytes (including container elements and owned references).
// A cap cannot certify a partial tie group, a missing key or an uncovered interval.
// Do not evict required keys/history or partially admit a tie group at a cap.
// Publish only a separately certified prefix and hold when further work or
// retention cannot be admitted. Wall-only hard end does not depend on this work.
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

// Queue-owned observations belong to the supplied timer/clock lineage. In queue
// order, each cut is a trusted wall boundary with optional independently certified
// monotonic elapsed since the preceding cut or supplied anchor. A missing split
// is distinct from zero elapsed. The clock owner supplies all necessary splits
// (schedule, independent pause and command boundaries); no UTC subtraction,
// interpolation or synthesis of a missed boundary is allowed. The target is
// itself an admitted wall observation, even if its elapsed is unavailable.
struct ScheduledPauseElapsedCut {
    ScheduledPauseObservation observation;
    std::optional<std::chrono::milliseconds> elapsed_since_previous;
    // Non-Schedule state in force for the outgoing interval, from its owners.
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

struct ScheduledPauseCoverageHold {
    ScheduledPauseCoverageFailure cause;
    // First outgoing interval that must be held.
    std::int64_t from_utc_ms;
};

struct ScheduledPauseCoverageNotice {
    ScheduledPauseCoverageHold hold;
    ScheduledPauseObservation observed_at;
};

struct ScheduledPauseCursor {
    ScheduledPauseObservation last_observation;
    // Last endpoint actually evaluated with certified knowledge and elapsed.
    // This can lag wall reconciliation or a prospective new anchor. It does not
    // certify elapsed across a held gap. Absent before any evaluation.
    std::optional<std::int64_t> evaluated_through_utc_ms;
    std::optional<ScheduledPauseCoverageHold> hold;
};

struct ScheduledPauseState {
    // Queue-owned identity of this exact basis: timer and clock lineages, state,
    // timer/config/access revisions, cursor, ordered epoch data and complete
    // ledger revision/contents or explicit absence. Never use caller-supplied
    // identities as evidence or reuse the reference for different facts.
    std::string snapshot_ref;
    std::string timer_lineage;
    std::string clock_lineage;
    std::string state_revision;
    std::string configuration_revision;
    std::string access_revision;
    core::TimerConfig timer_config;
    core::TimerState timer;
    core::PauseState pause;
    ScheduledPauseCursor cursor;
    // Last epoch is open/current. An absent or incomplete relevant history
    // cannot prove an empty search where historical facts are needed.
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
    // Absent preserves the original end. A deletion/disable stops at the save
    // cutover, illustrated by effective_utc_ms at the preview observation.
    std::optional<ScheduledPauseStop> stop_on_save;
};

struct ScheduledPauseFactBinding {
    // Index in the bound old epoch list. Absent denotes the proposed view.
    std::optional<std::size_t> old_epoch_index;
    ScheduleZoneFacts facts;
};

struct ScheduledPausePreview {
    std::string source_snapshot_ref;
    std::string preview_command_ref;
    std::string timer_lineage;
    std::string clock_lineage;
    std::string state_revision;
    std::string configuration_revision;
    std::string delivery_revision;
    std::string access_revision;
    std::string actor_ref;
    ScheduledPauseObservation as_of;
    ScheduledPauseConfiguration proposed;
    std::int64_t requested_through_utc_ms;
    std::optional<std::int64_t> resolved_through_utc_ms;
    std::optional<ScheduledPauseCoverageFailure> incomplete;
    // Exact certified facts used, bound to the old/current and proposed views.
    std::vector<ScheduledPauseFactBinding> facts;
    std::vector<ScheduledPausePreviewRow> upcoming;
    std::vector<ScheduledPauseCutoverEffect> cutover_effects;
    // Absent when the cutover pause union cannot be certified.
    std::optional<core::PauseState> pause_after_cutover;
};

struct ScheduledPauseTransitions {
    ScheduledPauseObservation observation;
    std::vector<core::TimerTransition> transitions;
};

struct ScheduledPauseProgress {
    std::string source_snapshot_ref;
    // Preview/Save/Start/Reopen proposals retain their owning command identity.
    // Independent progression has no admitted command.
    std::optional<std::string> admitted_command_ref;
    core::TimerState timer;
    core::PauseState pause;
    ScheduledPauseCursor cursor;
    // Inclusive frontier F of newly certified starts, even for an empty prefix.
    std::optional<std::int64_t> starts_resolved_through_utc_ms;
    // Earliest end among still-contributing known windows, even past F or the
    // evaluated endpoint. Wall reconciliation can reach it while elapsed is held.
    std::optional<std::int64_t> next_known_expiry_utc_ms;
    std::optional<ScheduledPauseCoverageNotice> notice; // Present iff held.
    std::vector<DeliveredScheduledPause> new_deliveries;
    std::vector<ScheduledPauseDeliveryStop> stops;
    std::vector<ScheduledPauseTransitions> timer_transitions;
};

struct ScheduledPauseSave {
    ScheduledPauseObservation cutover;
    ScheduledPauseConfiguration configuration;
    // Facts to close the formerly open epoch. The new epoch starts strictly
    // after cutover. No certified old revision is implied if this is absent.
    std::optional<std::string> closed_zone_rules_revision;
    ScheduledPauseProgress progress;
};

enum class ScheduledPauseRefusal {
    invalid_input,
    limit_exceeded,
    access_denied,
    stale_observation,
    fresh_preview_required,
    entry_not_admitted,
    hard_end_reached
};

// These four calls propose results on the authoritative queue. Callers supply
// a coherent current state, session/permission decision and clock-owned facts.
// Enforce current timer configuration edit permission (including any separately
// required timezone authority) before protected preview or save reads. Ordinary
// timer snapshot read permission governs notice disclosure, checked at disclosure.
// Verify trusted input bindings before computing. Compare command and clock
// references against the current snapshot, and require the edit's admission
// observation to match the target cut and the authorized proposal by value.
// An access/proposal mismatch refuses access; a stale basis or clock refuses
// as stale. Retain and recheck bindings plus exact proposal at apply. Commit
// config, clock, timer, Schedule reason and ledger changes
// together, including any notice, and bind entry to its owning command. Only
// successful application publishes them; a refusal or failed application supplies
// no provisional delivery, expiry, configuration or clock state.
// A trusted owner assembles this evidence from one admitted command. The
// reference binds its exact purpose, source snapshot and admitted observation.
struct ScheduledPauseCommandBinding {
    std::string command_ref;
    std::string source_snapshot_ref;
    ScheduledPauseObservation observation;
};

struct ScheduledPauseEditAccess {
    ScheduledPauseCommandBinding admission;
    // The queue binds the current actor and grant to the bound timer, access
    // revision, operation (preview or save), and exact authorized proposal.
    // Denied or unknown is absent. The proposal is borrowed for the call.
    std::optional<std::string> authorized_actor_ref;
    const ScheduledPauseConfiguration *authorized_proposal;
};

[[nodiscard]] std::variant<ScheduledPausePreview, ScheduledPauseRefusal>
preview_scheduled_pause_edit(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                             const ScheduledPauseConfiguration &proposed,
                             std::int64_t through_utc_ms, const ScheduledPauseEditAccess &,
                             ScheduleZoneSource &);

[[nodiscard]] std::variant<ScheduledPauseSave, ScheduledPauseRefusal>
revalidate_scheduled_pause_save(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                                const ScheduledPauseConfiguration &proposed,
                                const ScheduledPausePreview &shown,
                                const ScheduledPauseEditAccess &, ScheduleZoneSource &);

enum class ScheduledPauseEntryKind { start, reopen };

struct ScheduledPauseEntry {
    ScheduledPauseEntryKind kind;
    ScheduledPauseCommandBinding admission;
    // Absent unless this same admitted Start/Reopen supplied both its trusted
    // observation and this Active proposal for the bound snapshot, with current
    // permission and all lifecycle, Main, grace, debt and reopen choices.
    std::optional<core::TimerState> admitted_timer;
};

// Entry consumes no prior elapsed. Existing entry admission and hard end take
// precedence. Mixed, unmatched or stale admission facts refuse entry. Use the
// bound observation, timer/clock lineage and exact state, epoch and ledger basis;
// check the same command and observation again before applying. Unknown schedule
// eligibility holds schedule-dependent progression at entry. Other reasons stay.
[[nodiscard]] std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
enter_scheduled_pauses(const ScheduledPauseState &, const ScheduledPauseEntry &,
                       ScheduleZoneSource &);

[[nodiscard]] std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
progress_scheduled_pauses(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                          ScheduleZoneSource &);

// Resolution and identity
//
// CORE-004 owns nominal weekdays, gap/fold resolution and delivery filtering,
// including the last containing forward crossing for a compound gap. Project an
// end using the effective offset at its captured UTC instant. Preserve the
// assigned nominal key across edits. A sole resolved run is suppressed by either
// prior run for the slot. With two resolved runs consult each key. The ledger
// retains stopped keys and all evidence needed by later resolution. An edit or
// reopen cannot reset identity or substitute a new ID for the same schedule.
// An admitted start S captures [S,S+D) using checked positive elapsed D. Apply the
// union once at each instant, including simultaneous starts and ends. Ordinary
// edits (zone, recurrence, duration) leave captured contributions and ends intact.
//
// Preview and cutover
//
// Preview observes old configuration through proposed cutover C under the same
// rules as save, without publishing deliveries. Show proposed starts in (C,H] in
// UTC order (ties by rule order, then CORE-004 order), using enabled proposed
// rules and suppressing all matching deliveries through C. Show original keys and
// actual local start/end in the proposed zone. Show active deliveries with original
// UTC bounds and each exact retain/delete/disable consequence, including the
// immediate post-cutover pause union. Unknown cutover effects make the preview
// incomplete even if its future rows are complete. A stop at C is disclosed
// as stopping at the actual save cutover, with the preview's as-of C. A local label
// for a retained end needs its own certified zone facts.
// Save takes the exact preview the server issued and presented to this actor.
// Bind it to the actor, issued preview command, exact proposal, source snapshot,
// revisions, rule facts and cutover consequences. A client assertion supplies no
// evidence. Save uses its own bound current admission and the same shown actor.
// Revalidate the original certified ranges and facts (including current old and
// proposed rules revisions), then recompute at the trusted save observation.
// A newer observation can revalidate only with unchanged bindings, displayed
// rows, coverage of the remaining range and consequences, with no crossed
// window or coverage boundary. The as-of time and time of the disclosed
// stop-at-save may move. An incomplete or stale result requires a fresh preview.
// A certified empty preview can satisfy save.
// Observe old starts through C, then save reconciling all effects at C. Bind
// certified old facts for the pre-cutover lookback of schedules that remain
// enabled, including in Ready/Finished, so later entry can use that old view.
// These facts may come from retained certified data via the same source port.
// Deleting or disabling ends that schedule's contribution at C, including an
// old start at C. Keep its key/window and never reinstate the contribution.
// Append any due delivery and its stop together and reconcile the union once.
// For disabled/deleted schedules with no active delivery, there is no stop row.
// Other windows and independent reasons survive. An ordinary edit creates no
// new start at C.
//
// Entry and resumption
//
// At Start/Reopen T, consider only schedules enabled at T. Use unsuppressed
// delivered windows still active at T without reprojecting them. For undelivered
// candidates use each applicable epoch's own D and facts: intersect (T-D,T] with
// that epoch's start range. Old facts own starts at/before cutover, new facts
// strictly after it. A schedule added or re-enabled starts fresh on the new side
// of that boundary; windows from before its latest disable/delete are ineligible,
// including undelivered candidates. Match retained keys before admitting anything;
// process eligible candidates by UTC start and stable configuration/core order,
// carrying forward admitted keys within the proposal. Admit only the remainder
// from T, never an expired start. Ready/Finished do not progress ordinary starts.
//
// Coverage and observation
//
// For closed facts [L,U] and query (A,B], require L<=A-48h and B<=U-48h.
// For local preview also require B<=U-D. Take the greatest coherent contiguous
// prefix across required inputs within the checked work and coverage bounds.
// Empty/disabled schedules and captured UTC ends require no timezone lookup.
// F includes F; with a supplied split at F+1ms, outgoing elapsed may be evaluated
// up to F+1ms exclusively. Otherwise hold from the last evaluable supplied cut;
// the projected frontier is not an elapsed observation. A failed later chunk
// cannot erase the certified prefix or extend it across unknown facts.
// While held, block Main/Grace and dependent active-subathon elapsed; preserve
// independent signal/real-elapsed policies. Reconcile known UTC expiries at
// trusted wall observations even without an elapsed split or start coverage.
// Clear only Schedule when the last known contributor ends; leave the gate/notice
// in effect while knowledge is missing. The known expiry is effective at the
// captured end, while its reconciliation is annotated by the wall observation.
// Resume at the first newly admitted observation after the hold with complete
// current eligibility and a trusted prospective anchor.
// Reconcile active remaining windows there as at entry, clear the gate/notice and
// advance only prospectively. Never charge held elapsed or replay expired starts.
// Validate freshness and order against the clock owner and committed cursor.
// Reject expired or stale observations; retries cannot reapply elapsed, stops
// or deliveries.
//
// At a supplied trusted observation reaching an enabled hard end, CORE-002
// applies before any elapsed reduction, schedule start, stop or expiry at that
// observation, independently of the schedule facts or available elapsed cuts.
// Keep previous valid reductions and any earlier natural finish.
// Admit no start at or after an observation which finishes the timer.
// Annotate hard or natural finish with the actual admitting observation, never an
// interpolated instant. Failed/untrusted observations cannot prove a reached
// deadline. Start/Reopen at or past an enabled hard end remains blocked.
//
// #177 supplies the pure resolver and Schedule reducer. Platforms must separately
// verify complete configured-zone facts and applicability of compound gaps (10A).
// #318 supplies durable delivery/checkpoint and restored lineage facts; #181 owns
// offline reconstruction. This declaration grants no clock or write authority.

} // namespace subathon::server
