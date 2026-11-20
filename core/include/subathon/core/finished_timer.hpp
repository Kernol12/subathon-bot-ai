// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/pause_policy.hpp>
#include <subathon/core/timer_editorial_commands.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace subathon::core {

// CORE-007 r3. These are supplied facts and owned proposals. The caller owns
// receipt retention, access resolution, atomic commit, and every external effect.
enum class FinishedRefusal : std::uint8_t {
    input_limit,
    result_limit,
    allocation_failure,
    invalid_input,
    access_denied,
    access_unknown,
    stale_basis,
    facts_unavailable,
    wrong_lifecycle,
    deadline_passed,
    deadline_unknown,
    grace_choice_required,
    main_out_of_range,
    invalid_group_choice,
    group_held,
    group_decision_unavailable
};

struct FinishedLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t groups_or_choices = 4096;
    std::uint64_t records = 16384;
    std::uint64_t input_bytes = 8388608;
    std::uint64_t result_bytes = 16777216;
};

struct FinishedScope {
    std::string installation;
    std::string lineage;
    std::string revision;
    bool operator==(const FinishedScope &) const = default;
};

enum class DeadlineReached : std::uint8_t { unknown, not_reached, reached };

struct FinishedObservation {
    std::string ref;
    std::optional<std::int64_t> utc_ms;
    DeadlineReached current_setting = DeadlineReached::unknown;
    bool operator==(const FinishedObservation &) const = default;
};

struct PostFinishPolicy {
    std::string revision;
    bool progression{};
    bool automation{};
    bool operator==(const PostFinishPolicy &) const = default;
};

enum class PendingHoldCause : std::uint8_t { acceptance_defer, refund_review };
struct PendingHold {
    PendingHoldCause cause{};
    std::string source_ref;
    bool operator==(const PendingHold &) const = default;
};

enum class GroupCommit : std::uint8_t { pending, released, skipped };
enum class GroupJobState : std::uint8_t { queued, in_flight, uncertain, delivered };

struct GroupJobFact {
    std::string capture_ref;
    std::string job_ref;
    GroupJobState state{};
    std::uint64_t revision{};
    std::uint64_t attempt{};
    bool operator==(const GroupJobFact &) const = default;
};

struct FinishedGroup {
    std::string occurrence;
    std::uint64_t queue_order{};
    GroupCommit committed{};
    std::optional<std::vector<PendingHold>> active_holds;
    std::string capture_ref;
    std::int64_t achieved_score_micro_points{};
    std::vector<GroupJobFact> jobs;
    std::vector<std::string> bonus_receipts;
    std::optional<std::int64_t> presentation_ms;
    std::int64_t gap_after_ms{};
    bool operator==(const FinishedGroup &) const = default;
};

// Overlapping evidence from a current lifecycle mirror. It cannot supply authority.
// Target-edit-only fields are deliberately outside the CORE-007 projection.
struct FinishedLifecycleMirror {
    std::string revision;
    TimerState timer;
    std::int64_t debt_ms{};
    std::optional<std::int64_t> deadline_utc_ms;
    PauseReasons pause_reasons;
    bool progression{};
    bool automation{};
};

struct FinishedSnapshot {
    FinishedScope scope;
    std::optional<TimerEditorialValues> values;
    FinishedObservation observation;
    std::optional<PauseState> pause;
    std::optional<PostFinishPolicy> policy;
    std::optional<FinishedLifecycleMirror> mirror;
    std::int64_t score_micro_points{};
    std::string finish_history_ref;
    // includes committed historical occurrences; nullopt means unavailable
    std::optional<std::vector<FinishedGroup>> groups;
};

enum class EffectEligibility : std::uint8_t { eligible, held, unknown, duplicate, invalid };
enum class FinishedEffectKind : std::uint8_t { timer_credit, milestone_award, accounting };
enum class FinishedActionKind : std::uint8_t { ordinary, milestone };

struct FinishedEffect {
    std::string ref;
    FinishedEffectKind kind{};
    std::int64_t requested{};
    EffectEligibility eligibility{};
    std::optional<std::string> blocker_ref;
    bool operator==(const FinishedEffect &) const = default;
};

struct FinishedAction {
    std::string ref;
    FinishedActionKind kind{};
    std::string route_ref;
    std::string sender_ref;
    std::vector<std::string> required_effects;
    EffectEligibility eligibility{};
    std::optional<std::string> blocker_ref;
    bool operator==(const FinishedAction &) const = default;
};

struct FinishedSupport {
    std::string receipt_ref;
    std::string input_ref;
    std::string source_ref;
    std::int64_t arrival_utc_ms{};
    std::string capture_ref;
    std::vector<FinishedEffect> effects;
    std::vector<FinishedAction> actions;
    bool operator==(const FinishedSupport &) const = default;
};

enum class EffectDisposition : std::uint8_t {
    finished,
    progression_off,
    automation_off,
    trigger_suppressed,
    held,
    unavailable,
    duplicate,
    invalid,
    pass_to_owner,
    consider_in_group
};
struct FinishedInputProposal {
    FinishedScope scope;
    FinishedObservation observation;
    PostFinishPolicy policy;
    FinishedSupport retained;
    std::vector<EffectDisposition> effects; // aligned with retained.effects
    std::vector<EffectDisposition> actions; // aligned with retained.actions
    std::int64_t main_added_ms{};
    std::int64_t grace_consumed_ms{};
    std::int64_t debt_repaid_ms{};
    std::int64_t debt_added_ms{};
};
using FinishedInputResult = std::variant<FinishedRefusal, FinishedInputProposal>;

enum class ReopenGrace : std::uint8_t { retain, reset };
enum class ReopenGroupChoice : std::uint8_t { keep_deferred, resume, skip };

struct ReopenChoice {
    std::string occurrence;
    ReopenGroupChoice choice{};
    bool operator==(const ReopenChoice &) const = default;
};

struct ReopenIntent {
    std::int64_t absolute_main_ms{};
    std::optional<ReopenGrace> grace;
    std::vector<ReopenChoice> choices;
    bool operator==(const ReopenIntent &) const = default;
};

struct DebtCorrectionIntent {
    std::int64_t absolute_debt_ms{};
    std::string reason;
    bool operator==(const DebtCorrectionIntent &) const = default;
};

using FinishedIntent = std::variant<ReopenIntent, DebtCorrectionIntent>;
struct FinishedCommand {
    FinishedScope expected;
    std::string operation_ref;
    std::string observation_ref;
    FinishedIntent intent;
    bool operator==(const FinishedCommand &) const = default;
};

struct ReopenPreview {
    // The exact ordered pending members, rank, committed facts, and active hold keys.
    std::vector<FinishedGroup> pending;
    bool operator==(const ReopenPreview &) const = default;
};

enum class FinishedAccessState : std::uint8_t { allowed, denied, unknown };
struct ReopenGroupAccess {
    ReopenChoice choice;
    FinishedAccessState state = FinishedAccessState::unknown;
    bool operator==(const ReopenGroupAccess &) const = default;
};
struct FinishedAccess {
    FinishedAccessState state = FinishedAccessState::unknown;
    FinishedCommand bound_command; // binds canonical intent, including default choices
    std::string actor;
    std::string session;
    std::string permission_revision;
    std::string permission_catalog;
    std::vector<ReopenGroupAccess> group;
};

// A deferred head consumes no presentation/gap and blocks the suffix. Ordinary
// consideration is subject to the actual queue head, holds, pause and linkage.
enum class ReopenGroupDisposition : std::uint8_t { deferred, ordinary_consideration, omit_pending };
struct ReopenGroupProposal {
    ReopenChoice choice;
    ReopenGroupDisposition disposition{};
    std::vector<PendingHold> retained_holds;
    bool operator==(const ReopenGroupProposal &) const = default;
};

struct FinishedAudit {
    FinishedAccess authority;
    FinishedObservation observation;
    TimerEditorialValues before;
    TimerEditorialValues after;
};

struct FinishedCommandProposal {
    // Caller selects the next revision after acceptance. Group changes are
    // proposed separately below, leaving the supplied durable facts intact.
    FinishedSnapshot after;
    FinishedAudit audit;
    std::vector<ReopenGroupProposal> groups; // in durable order, with no delivery claims
    std::int64_t debt_repaid_ms{};
    std::vector<TimerTransition> transitions;
};
using FinishedCommandResult = std::variant<FinishedRefusal, FinishedCommandProposal>;

class FinishedTimerNotImplemented final : public std::logic_error {
  public:
    FinishedTimerNotImplemented()
        : std::logic_error("CORE-007 finished timer policy is intentionally unimplemented") {}
};

[[nodiscard]] FinishedInputResult project_finished_input(const FinishedSnapshot &current,
                                                         const FinishedSupport &support,
                                                         const FinishedLimits &limits = {});
[[nodiscard]] FinishedCommandResult project_finished_command(
    const FinishedSnapshot &current, const FinishedCommand &command, const FinishedAccess &access,
    const std::optional<ReopenPreview> &preview = std::nullopt, const FinishedLimits &limits = {});

} // namespace subathon::core
