// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/scheduled_pause.hpp>

namespace subathon::server {
enum class ScheduleCommandKind { preview, save, start, reopen };
enum class ScheduleRefusal {
    not_admitted, access_denied, stale_observation, fresh_preview_required,
    cutover_uncertified, forecast_incomplete, beyond_supported_timezone_transitions,
    invalid_input, limit_exceeded, hard_end_reached
};
struct ScheduleContext { std::string installation, timer, session_ref; };
struct SchedulePermit { std::string actor, grant, permission_revision; };
struct ScheduleDenied {};
struct ScheduleUnknown {};
using ScheduleAccess = std::variant<SchedulePermit, ScheduleDenied, ScheduleUnknown>;
struct ScheduleCommandRef { std::string value; };
struct ScheduleIssuedRef { std::string value; };
struct SchedulePresentedRef { std::string value; };
struct ScheduleConfirmation {
    ScheduleIssuedRef issued;
    std::optional<SchedulePresentedRef> shown;
    std::string basis;
};
struct ScheduleBase { std::string snapshot_ref, state_revision; };
struct ScheduleEdit { ScheduledPauseConfiguration proposed; std::int64_t through_utc_ms; };
struct ScheduleSave { ScheduleEdit edit; ScheduleConfirmation confirm; };
struct ScheduleEntrySelection { std::string owning_timer_intent_ref; };
struct ScheduleRequest {
    ScheduleContext context;
    ScheduleCommandKind kind;
    ScheduleBase expected;
    std::variant<ScheduleEdit, ScheduleSave, ScheduleEntrySelection> intent;
};
struct ScheduleAdmission {
    ScheduleCommandRef ref;
    ScheduleRequest request;
    SchedulePermit admitted;
    ScheduledPauseObservation target;
    std::optional<core::TimerState> entry_timer;
};
struct ScheduleClock {
    std::string snapshot_ref, lineage;
    ScheduledPauseObservation anchor, target;
    std::vector<ScheduledPauseElapsedCut> cuts;
};
struct SchedulePreviewRecord {
    ScheduleAdmission issuer;
    ScheduleClock clock;
    ScheduledPausePreview projection;
    std::variant<ScheduledPauseCutover, ScheduledPauseCoverageFailure> cutover;
    ScheduleIssuedRef issued;
    std::string basis;
};
struct ScheduleShownEffect {
    ScheduledPauseKey key;
    core::ScheduledPauseWindow captured;
    std::optional<ScheduledPauseStopCause> stop_at_save;
};
struct ScheduleForecastNotice {
    std::variant<ScheduledPauseCoverageFailure, ForecastSupportFailure> cause;
    bool old_side; // True only for ScheduledPauseCoverageFailure.
};
struct SchedulePublication {
    ScheduleConfirmation confirm;
    std::string checked_state_revision;
    ScheduledPauseObservation as_of;
    std::int64_t through_utc_ms;
    std::optional<std::int64_t> certified_through_utc_ms;
    bool incomplete;
    std::vector<ScheduledPausePreviewRow> upcoming;
    std::vector<ScheduleShownEffect> effects;
    std::optional<core::PauseState> pause_after_cutover;
    std::optional<ScheduleForecastNotice> notice;
};
struct ScheduleScopeUnproved {};
struct ScheduleProofMissing {}; // Same scope independently established, including eviction.
struct SchedulePresentedRecord { SchedulePreviewRecord preview; SchedulePublication released; };
enum class ScheduleWriteOutcome { acknowledged, not_committed, outcome_unacknowledged };
using ScheduleCandidate = std::variant<ScheduledPauseCutover, ScheduledPauseProgress>;

struct ScheduleAdmissionPort {
    virtual ~ScheduleAdmissionPort() = default;
    virtual std::variant<ScheduleCommandRef, ScheduleRefusal> admit(const ScheduleRequest &) = 0;
    virtual std::optional<ScheduleAdmission> active(const ScheduleCommandRef &) = 0;
};
struct ScheduleAccessPort {
    virtual ~ScheduleAccessPort() = default;
    virtual ScheduleAccess command(const ScheduleRequest &, const ScheduleCandidate *) = 0;
    virtual ScheduleAccess snapshot(const ScheduleContext &) = 0;
};
struct ScheduleClockPort {
    virtual ~ScheduleClockPort() = default;
    virtual std::variant<ScheduleClock, ScheduleRefusal>
    observe(const ScheduleAdmission &, const ScheduledPauseState &) = 0;
};
struct SchedulePreviewPort {
    virtual ~SchedulePreviewPort() = default;
    virtual std::variant<ScheduleIssuedRef, ScheduleRefusal> issue(SchedulePreviewRecord) = 0;
    template<class Record>
    using Lookup = std::variant<Record, ScheduleScopeUnproved, ScheduleProofMissing>;
    virtual Lookup<SchedulePreviewRecord> issued(const ScheduleAdmission &, const ScheduleIssuedRef &) = 0;
    virtual Lookup<SchedulePresentedRecord> presented(const ScheduleAdmission &, const ScheduleConfirmation &) = 0;
    virtual std::variant<SchedulePublication, ScheduleRefusal>
    release(const ScheduleAdmission &, const SchedulePreviewRecord &, SchedulePublication) = 0;
};
struct ScheduleWriterPort {
    virtual ~ScheduleWriterPort() = default;
    virtual ScheduledPauseState live(const ScheduleContext &) = 0;
    virtual ScheduleWriteOutcome commit(const ScheduleAdmission &, const ScheduleCandidate &) = 0;
};
struct SchedulePorts {
    ScheduleAdmissionPort &queue;
    ScheduleAccessPort &access;
    ScheduleClockPort &clock;
    ScheduleZoneSource &zones;
    SchedulePreviewPort &previews;
    ScheduleWriterPort &writer;
};
using ScheduleApply = std::variant<ScheduleWriteOutcome, ScheduleRefusal>;
std::variant<ScheduleIssuedRef, ScheduleRefusal>
issue_schedule_preview(const ScheduleCommandRef &, SchedulePorts &);
std::variant<SchedulePublication, ScheduleRefusal>
present_schedule_preview(const ScheduleCommandRef &, const ScheduleIssuedRef &, SchedulePorts &);
ScheduleApply apply_schedule_save(const ScheduleCommandRef &, SchedulePorts &);
ScheduleApply apply_schedule_entry(ScheduleCommandKind, const ScheduleCommandRef &, SchedulePorts &);
std::optional<ScheduledPauseCoverageNotice> disclose_schedule_notice(const ScheduleContext &, SchedulePorts &);

// Owner-queue order (the numbered rules below govern admission, release and apply).
// 1. Missing, inactive or wrong-kind command => not_admitted.
// 2. Current whole-command authority first => access_denied for denied, unknown or mismatched authority.
// 3. Stale exact command basis/clock => stale_observation. Preserve #720 input/limit refusals.
// 4. Exact scoped proof and confirmed horizon; authorize again before release or apply.
// 5. Save requires certified old-side effects and a fresh complete forecast through that horizon.
// 6. Writer commits the current candidate in one fenced owner turn. Acknowledge once.
} // namespace subathon::server
