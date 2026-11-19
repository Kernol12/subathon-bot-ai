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

namespace subathon::server {

// SCHEDULE-001 #720, contract revision 1. Application boundary for configured-zone
// previews and same-lineage timer execution. Clock, configuration, access and timer
// state come from their authoritative owners on the command queue. These data and
// proposals carry no command, clock-trust, persistence or publication authority.

struct ScheduleZoneRequest {
    std::string configured_zone;
    // Closed interval. A bounded source may certify a prefix starting at this
    // lower endpoint. A full preview requires the entire requested interval.
    std::int64_t from_utc_ms;
    std::int64_t through_utc_ms;
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
// Match the requested zone exactly. One immutable rules revision must bind all
// facts used for a given zone in an operation, including all chunk boundaries.
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
};

struct ScheduledPauseKey {
    std::string timer_lineage;
    std::string schedule_id;
    core::ScheduleOccurrenceId occurrence;
    bool operator==(const ScheduledPauseKey &) const = default;
};

struct DeliveredScheduledPause {
    ScheduledPauseKey key;
    core::ScheduledPauseWindow window;
};

// Bounded work per operation. The authoritative caller supplies positive limits
// for complete input, retained same-lineage deliveries, requested duration/lookback,
// projection chunks, clock segments, output rows and reference bytes. Preflight
// counts, sizes and checked time arithmetic before proportional work or allocation.
// Each core start query spans <=366 days. Larger admitted lookbacks tile adjacent
// (after, through] ranges exactly, with overlap in facts for padding, no duplicate
// boundary starts and no mixing revisions. Bound the total work as well as each
// individual call. Reject invalid input or report unavailable coverage on exhaustion.
// Resource exhaustion cannot prove a slot undelivered or an interval unpaused.

struct ScheduledPausePreviewBasis {
    std::string timer_lineage;
    std::string state_revision;
    std::string configuration_revision;
    std::string delivery_revision;
    std::string access_revision;
    std::optional<std::string> current_zone_rules_revision;
    std::string clock_observation;
    std::int64_t observed_utc_ms;
};

struct ScheduledPausePreviewRow {
    ScheduledPauseKey key;
    core::ScheduledPauseOccurrence window;
};

struct ScheduledPausePreview {
    ScheduledPausePreviewBasis basis;
    std::string configured_zone;
    std::optional<std::string> rules_revision;
    std::vector<ScheduledPauseRule> proposed_rules;
    std::int64_t requested_through_utc_ms;
    std::optional<std::int64_t> resolved_through_utc_ms;
    std::optional<ScheduleZoneUnavailable> coverage_failure;
    std::vector<ScheduledPausePreviewRow> upcoming;
    std::vector<DeliveredScheduledPause> active_deliveries;
};

// Preview is a read-only proposal for exact edits, including weekdays, gap/fold
// policies and positive elapsed duration. Capture a consistent current basis,
// exact proposed configuration, rule facts and same-lineage delivery view. Select
// future starts in (observed UTC, requested through], in UTC order. Break ties
// by the proposed configuration's rule order, then core order within a rule.
// Return actual local start/end in the proposed zone plus UTC and the
// original nominal key. Show retained active deliveries separately by original
// UTC bounds. A local label for one of those bounds needs its own certified facts
// and zone attribution. Rendering an end in another zone never changes that end.
// Full results may be empty. An incomplete result is explicit and cannot satisfy
// the pre-save preview. Report only certified rows, with unavailable at the anchor
// represented by coverage_failure and no resolved future portion.
//
// Save requires current permission and the exact proposed configuration. Re-read
// or revalidate state, config, delivery, access, rule revisions and the trusted
// clock on the queue, before mutation. An expired/invalid observation, crossed
// window or coverage boundary, changed bound revision, or changed preview result
// requires a fresh preview. A newer trusted observation may revalidate the basis
// only if the displayed starts, active deliveries, coverage and edit consequences
// still match for the remaining preview interval. Recheck the old state at the
// actual cutover under existing timer command precedence. Stale/incomplete queries,
// rejected edits and observation proposals cannot authorize writes or publication.
//
// CORE-004 is the resolution authority. Evaluate weekdays at the nominal local
// slot. For a gap with no inverse, default ShiftForward uses nominal minus the
// pre-transition offset of the LAST forward crossing containing the nominal slot.
// Project the resulting UTC under the offset actually in force there. Do not clamp
// to the transition, add a guessed hour or re-resolve the resulting local label.
// Skip emits nothing. For a fold, default RunOnce selects the earlier UTC before
// any filtering. RunTwice assigns First and Second in UTC order before filtering.
// For each accepted start S the immutable UTC window is [S,S+D), with checked
// addition of elapsed D. Projection of the end uses the effective offset at S+D.
// Overlap uses set semantics for the Schedule reason, with separate delivery keys.
//
// Identity is (lineage, stable schedule ID, exact nominal local date/time, run).
// Zone, offsets, UTC bounds and revisions are absent from that identity. For a slot
// with one run, either previous run suppresses it. For two runs, suppress only the
// matching run. Retain all delivered keys relevant to later resolution in this
// lineage. An edit/reopen cannot reset delivery identity, mint a replacement ID
// for the same schedule, or evict evidence needed for deduplication. Repeating a
// delivery is idempotent. Record deliveries only when admitted by the authoritative
// transition. A key and its original UTC bounds, the Schedule reason, clock and
// state revisions become visible together after successful authoritative application.
// Rejected or failed application leaves no provisional delivery visible.
//
// A zone edit observes old configuration through the trusted cutover C. New-zone
// starts are eligible only strictly after C. A new projection at or before C
// cannot create an earlier delivery. Every already-delivered still-active window
// retains its original UTC end through edits or source failure.
// A matched delivered key can never prolong or shorten that window. Reconcile the
// union of retained active deliveries and newly admitted windows. At a shared
// start/end instant apply the union once. At exact end the old window is inactive.
//
// Start and authorized reopen preserve existing lifecycle admission including hard
// end, Main and grace/debt/reopen rules. For an otherwise eligible window active
// at trusted observation T, apply the Schedule reason from T to its original end.
// For a new discovery use the proven window from its applicable configuration and
// record the same key and original bounds. For an already delivered active window
// reuse the retained bounds even if the new rules would suppress its projected row.
// Candidate starts for entry lie in (T-D,T] for each duration D. An end exactly T
// is inactive and a start exactly T is active. Enter only the remaining interval,
// without advancing/rebuilding prior clock state or delivering expired starts.
// Ready/Finished do not consume countdown time or deliver ordinary scheduled starts.
// If eligibility at T is unknown, the coverage hold applies at T.

// Exact coverage accounting for a bounded observation. The start frontier uses
// closed UTC facts [L,U]. For a start query (A,B], P=48h. Require L<=A-P, and
// B<=U-P, with checked arithmetic, valid mappings and a complete range. Include
// the necessary lookback for entry or other active-window searches. The upper
// limit for local start/end preview of one duration D is U-max(P,D). Execution
// may use complete starts through U-P with exact checked UTC ends; projecting a
// local end requires its own coverage. These are upper bounds. Use the greatest
// contiguous prefix actually certified inside the requested range and within
// all bounds. Take the minimum over applicable inputs. Empty/disabled schedules
// and retained UTC ends need no lookup. A failed later chunk cannot erase a
// previously certified prefix, extend it over a hole or settle new-zone coverage.
struct ScheduledPauseProgress {
    // Inclusive bound on resolved starts, including a certified empty result.
    // Absent when there is no newly certified start coverage at the observation.
    std::optional<std::int64_t> starts_resolved_through_utc_ms;
    // End of the evaluated half-open interval. This endpoint was actually reached
    // with certified schedule knowledge and a trusted elapsed split.
    std::int64_t evaluated_through_utc_ms;
    // Present from the first interval requiring schedule knowledge we lack.
    std::optional<ScheduleZoneUnavailable> coverage_hold;
    bool elapsed_split_unavailable;
};

// A contiguous start frontier F includes F. The first unresolved start is F+1ms
// when representable. With trustworthy segmentation the outgoing interval before
// that boundary is known, and the outgoing interval at it is held. If entry state
// is already unknown or the trusted elapsed split cannot be established, stop at
// the last actually certified/evaluated boundary. Do not report a projected
// frontier as reached. Keep notification/observation time separate from the
// evaluated boundary. Re-observation cannot double-charge the evaluated prefix
// or turn a held interval into pending elapsed debt. Re-anchor prospective
// advancement at a newly admitted observation after a hold, subject to the
// authorized continuation policy.
//
// Before each timer reduction split trustworthy monotonic elapsed at UTC schedule
// boundaries, other pause changes, coverage limits, and command observations.
// A pair of UTC timestamps alone never establishes elapsed or where to divide it.
// An ambiguous/discontinuous span supplies no invented split. Keep the Schedule
// reason for certified active windows and clear it at their known UTC ends. Keep
// the coverage gate separate from Schedule and from Manual, stream, client and
// RecoveryReview policy. Unknown coverage suspends schedule-dependent Main/Grace
// progression, including active-subathon time derived from it. It cannot clear a
// separate reason, synthesize a delivery or supply an unpaused eligibility fact.
// Signal policies and real-elapsed clocks retain their independently trusted inputs.
// Show an unavailable-coverage notice to authorized users while the gate applies.
// Later complete coverage cannot retroactively charge unfounded elapsed or deliver
// expired windows. Clearing this gate never clears an independent pause reason.
//
// Evaluate trusted wall observations for hard end even while coverage is held.
// At a reached enabled hard end, apply existing CORE-002 priority to the ending
// observation. Keep prior committed countdown fields and earlier valid advances.
// Do not deliver a schedule start at or beyond terminal finish, or postpone hard
// end awaiting timezone data. At equal UTC, hard end wins over schedule changes
// and expiry for that observation. A natural finish established at an earlier
// trusted observation stays finished. A failed/untrusted UTC observation cannot
// itself prove a deadline was reached. Start/reopen at or past an enabled hard
// end remains blocked, independent of schedule eligibility.
//
// Capability boundary. #177 supplies bounded pure resolution and the Schedule
// reason reducer. #720 composes proposals with authoritative same-lineage state.
// No real timezone source is provisioned by this contract. The existing selected
// timezone API is a candidate subject to complete-interval and compound-gap
// applicability verification on the supported server environment. Client preview
// rendering does not establish server timezone authority. #318 owns durable
// delivery/checkpoint footprint and supplying restored lineage facts. #181 owns
// pure offline reconstruction. Absence of restored facts is not an empty ledger.

} // namespace subathon::server
