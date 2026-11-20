// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/finished_timer.hpp>

#include <algorithm>
#include <map>
#include <new>
#include <set>
#include <string_view>
#include <utility>

namespace subathon::core {
namespace {
using Refusal = FinishedRefusal;
using Disposition = EffectDisposition;
using Access = FinishedAccessState;
using Choice = ReopenGroupChoice;

// Logical bytes exclude container headers, capacity and C++ padding. As in
// CORE-008, enum and presence tags cost one byte, signed/unsigned balances
// and revisions cost eight, and strings contribute their payload.
struct Size {
    const FinishedLimits &limits;
    std::uint64_t bytes_left;
    std::uint64_t records_left;
    Refusal refusal;

    void bytes(std::uint64_t count) {
        if (count > bytes_left)
            throw refusal;
        bytes_left -= count;
    }

    template <typename Container>
    void collection(const Container &items, bool records = true, bool groups_or_choices = false) {
        if (items.size() > items.max_size() ||
            (groups_or_choices && items.size() > limits.groups_or_choices))
            throw refusal;
        if (records) {
            if (items.size() > records_left)
                throw refusal;
            records_left -= items.size();
        }
    }

    void count(const std::string &value) {
        if (value.size() > limits.reference_bytes || value.size() > value.max_size())
            throw refusal;
        bytes(value.size());
    }

    template <typename T> void count(const std::optional<T> &value) {
        bytes(1);
        if (value)
            count(*value);
    }

    template <typename T> void count(const std::vector<T> &items, bool groups_or_choices = false) {
        collection(items, true, groups_or_choices);
        for (const auto &item : items)
            count(item);
    }

    void count(std::int64_t) { bytes(8); }
    void count(const TimerState &) { bytes(19); } // Two enum tags, two i64, bool.
    void count(const PauseReasons &reasons) {
        collection(reasons, false);
        bytes(reasons.size());
    }
    void count(const PauseState &pause) {
        count(pause.reasons);
        count(pause.stream_offline_pending_ms);
        count(pause.client_disconnect_pending_ms);
        count(pause.stream_unknown_pending_ms);
        bytes(1);
    }
    void count(const TimerEditorialValues &values) {
        count(values.timer);
        bytes(18); // Debt, two config tags and configured grace.
        count(values.maximum_remaining_ms);
        count(values.config.hard_end_utc_ms);
    }
    void count(const FinishedScope &scope) {
        count(scope.installation);
        count(scope.lineage);
        count(scope.revision);
    }
    void count(const FinishedObservation &observation) {
        count(observation.ref);
        count(observation.utc_ms);
        bytes(1);
    }
    void count(const PostFinishPolicy &policy) {
        count(policy.revision);
        bytes(2);
    }
    void count(const FinishedLifecycleMirror &mirror) {
        count(mirror.revision);
        count(mirror.timer);
        bytes(10); // Debt and the two policy gates.
        count(mirror.deadline_utc_ms);
        count(mirror.pause_reasons);
    }
    void count(const PendingHold &hold) {
        bytes(1);
        count(hold.source_ref);
    }
    void count(const GroupJobFact &job) {
        count(job.capture_ref);
        count(job.job_ref);
        bytes(17);
    }
    void count(const FinishedGroup &group) {
        count(group.occurrence);
        count(group.capture_ref);
        bytes(25); // Order, commit, score and gap.
        count(group.active_holds);
        count(group.jobs);
        count(group.bonus_receipts);
        count(group.presentation_ms);
    }
    void count(const FinishedSnapshot &snapshot) {
        count(snapshot.scope);
        count(snapshot.values);
        count(snapshot.observation);
        count(snapshot.pause);
        count(snapshot.policy);
        count(snapshot.mirror);
        bytes(9); // Score and group-availability tag.
        count(snapshot.finish_history_ref);
        if (snapshot.groups)
            count(*snapshot.groups, true);
    }
    void count(const FinishedEffect &effect) {
        count(effect.ref);
        bytes(10);
        count(effect.blocker_ref);
    }
    void count(const FinishedAction &action) {
        count(action.ref);
        count(action.route_ref);
        count(action.sender_ref);
        count(action.required_effects);
        bytes(2);
        count(action.blocker_ref);
    }
    void count(const FinishedSupport &support) {
        count(support.receipt_ref);
        count(support.input_ref);
        count(support.source_ref);
        count(support.capture_ref);
        bytes(8);
        count(support.effects);
        count(support.actions);
    }
    void count(const ReopenChoice &choice) {
        count(choice.occurrence);
        bytes(1);
    }
    void count(const ReopenGroupAccess &access) {
        count(access.choice);
        bytes(1);
    }
    void count(const FinishedCommand &command) {
        count(command.expected);
        count(command.operation_ref);
        count(command.observation_ref);
        bytes(1); // Intent discriminator, including a valueless variant.
        if (const auto *reopen = std::get_if<ReopenIntent>(&command.intent)) {
            bytes(reopen->grace ? 10 : 9);
            count(reopen->choices, true);
        } else if (const auto *debt = std::get_if<DebtCorrectionIntent>(&command.intent)) {
            bytes(8);
            count(debt->reason);
        }
    }
    void count(const FinishedAccess &access) {
        bytes(1);
        count(access.bound_command);
        count(access.actor);
        count(access.session);
        count(access.permission_revision);
        count(access.permission_catalog);
        count(access.group, true);
    }
};

void preflight_input_proposal(const FinishedSnapshot &current, const FinishedSupport &support,
                              const FinishedLimits &limits) {
    Size input{limits, limits.input_bytes, limits.records, Refusal::input_limit};
    input.count(current);
    input.count(support);

    Size result{limits, limits.result_bytes, limits.records, Refusal::result_limit};
    result.bytes(33); // Result discriminator and four actual time effects.
    result.count(current.scope);
    result.count(current.observation);
    if (current.policy)
        result.count(*current.policy);
    result.count(support);
    const auto disposition_capacity = std::vector<Disposition>{}.max_size();
    if (support.effects.size() > disposition_capacity ||
        support.actions.size() > disposition_capacity)
        throw Refusal::result_limit;
    result.bytes(support.effects.size());
    result.bytes(support.actions.size());
}

void preflight_command_proposal(const FinishedSnapshot &current, const FinishedCommand &command,
                                const FinishedAccess &access,
                                const std::optional<ReopenPreview> &preview,
                                const FinishedLimits &limits) {
    Size input{limits, limits.input_bytes, limits.records, Refusal::input_limit};
    input.count(current);
    input.count(command);
    input.count(access);
    input.bytes(1);
    if (preview)
        input.count(preview->pending, true);

    Size result{limits, limits.result_bytes, limits.records, Refusal::result_limit};
    result.bytes(9); // Result discriminator and debt repaid. Transitions are empty.
    result.count(current);
    result.count(access);
    result.count(current.observation);
    if (current.values) {
        result.count(*current.values); // Audit before.
        result.count(*current.values); // Audit after has the same scalar and optional widths.
    }
    if (std::holds_alternative<ReopenIntent>(command.intent) && current.groups) {
        std::size_t pending = 0;
        for (const auto &group : *current.groups) {
            if (group.committed != GroupCommit::pending)
                continue;
            if (pending == std::vector<ReopenGroupProposal>{}.max_size() ||
                pending == limits.groups_or_choices || result.records_left == 0)
                throw Refusal::result_limit;
            ++pending;
            --result.records_left;
            result.count(group.occurrence);
            result.bytes(2); // Choice and disposition.
            if (group.active_holds)
                result.count(*group.active_holds);
        }
    }
}

void require(bool condition) {
    if (!condition)
        throw Refusal::invalid_input;
}

// The enums checked here have contiguous declared values starting at zero.
template <typename E> bool valid(E value, E last) { return value >= E{} && value <= last; }

bool nonnegative(const std::optional<std::int64_t> &value) { return !value || *value >= 0; }

bool valid_scope(const FinishedScope &scope) {
    return !scope.installation.empty() && !scope.lineage.empty() && !scope.revision.empty();
}

void validate_timer(const TimerState &timer) {
    require(valid(timer.lifecycle, TimerLifecycle::Finished) &&
            valid(timer.phase, CountdownPhase::Grace) && timer.main_remaining_ms >= 0 &&
            timer.grace_remaining_ms >= 0);
}

void validate_pause_reasons(const PauseReasons &reasons) {
    for (const auto reason : reasons)
        require(valid(reason, PauseReason::StreamStatusUnknown));
}

void validate_group(const FinishedGroup &group) {
    require(!group.occurrence.empty() && !group.capture_ref.empty() &&
            valid(group.committed, GroupCommit::skipped) && nonnegative(group.presentation_ms) &&
            group.gap_after_ms >= 0);
    if (group.active_holds)
        for (const auto &hold : *group.active_holds)
            require(!hold.source_ref.empty() && valid(hold.cause, PendingHoldCause::refund_review));
    for (const auto &job : group.jobs)
        require(!job.capture_ref.empty() && !job.job_ref.empty() &&
                valid(job.state, GroupJobState::delivered));
    for (const auto &receipt : group.bonus_receipts)
        require(!receipt.empty());
}

void validate_snapshot(const FinishedSnapshot &current) {
    require(valid_scope(current.scope) && !current.observation.ref.empty() &&
            valid(current.observation.current_setting, DeadlineReached::reached));
    if (current.values) {
        const auto &values = *current.values;
        validate_timer(values.timer);
        require(values.time_debt_ms >= 0 && nonnegative(values.maximum_remaining_ms) &&
                values.config.grace_duration_ms >= 0 &&
                valid(values.config.end_mode, TimerEndMode::Confirmation) &&
                valid(values.config.grace_reuse, GraceReuse::Shared));
    }
    if (current.pause) {
        const auto &pause = *current.pause;
        validate_pause_reasons(pause.reasons);
        require(nonnegative(pause.stream_offline_pending_ms) &&
                nonnegative(pause.client_disconnect_pending_ms) &&
                nonnegative(pause.stream_unknown_pending_ms));
    }
    if (current.mirror) {
        validate_timer(current.mirror->timer);
        require(current.mirror->debt_ms >= 0);
        validate_pause_reasons(current.mirror->pause_reasons);
    }
    if (current.groups)
        for (const auto &group : *current.groups)
            validate_group(group);
}

void validate_support(const FinishedSupport &support) {
    require(!support.receipt_ref.empty() && !support.input_ref.empty() &&
            !support.source_ref.empty() && !support.capture_ref.empty());
    for (const auto &effect : support.effects)
        require(!effect.ref.empty() && valid(effect.kind, FinishedEffectKind::accounting) &&
                valid(effect.eligibility, EffectEligibility::invalid) &&
                (!effect.blocker_ref || !effect.blocker_ref->empty()));
    for (const auto &action : support.actions) {
        require(!action.ref.empty() && !action.route_ref.empty() && !action.sender_ref.empty() &&
                valid(action.kind, FinishedActionKind::milestone) &&
                valid(action.eligibility, EffectEligibility::invalid) &&
                (!action.blocker_ref || !action.blocker_ref->empty()));
        for (const auto &ref : action.required_effects)
            require(!ref.empty());
    }
}

void validate_choice(const ReopenChoice &choice) {
    require(!choice.occurrence.empty() && valid(choice.choice, Choice::skip));
}

void validate_intent(const FinishedIntent &intent) {
    require(!intent.valueless_by_exception());
    if (const auto *reopen = std::get_if<ReopenIntent>(&intent)) {
        require(!reopen->grace || valid(*reopen->grace, ReopenGrace::reset));
        for (const auto &choice : reopen->choices)
            validate_choice(choice);
    } else {
        const auto &debt = std::get<DebtCorrectionIntent>(intent);
        require(debt.absolute_debt_ms >= 0 && !debt.reason.empty());
    }
}

bool same_timer(const TimerState &first, const TimerState &second) {
    return first.lifecycle == second.lifecycle && first.phase == second.phase &&
           first.main_remaining_ms == second.main_remaining_ms &&
           first.grace_remaining_ms == second.grace_remaining_ms &&
           first.awaiting_confirmation == second.awaiting_confirmation;
}

bool coherent(const FinishedSnapshot &current) {
    if (!current.values || !current.pause || !current.policy || current.policy->revision.empty())
        return false;

    const auto &values = *current.values;
    const auto &timer = values.timer;
    const auto &pause = *current.pause;
    if ((timer.lifecycle == TimerLifecycle::Finished && current.finish_history_ref.empty()) ||
        (timer.phase == CountdownPhase::Grace && timer.main_remaining_ms != 0) ||
        (timer.lifecycle == TimerLifecycle::Ready && timer.phase != CountdownPhase::Main) ||
        (timer.awaiting_confirmation &&
         (timer.lifecycle != TimerLifecycle::Active || timer.phase != CountdownPhase::Main ||
          timer.main_remaining_ms != 0 || values.config.end_mode != TimerEndMode::Confirmation)) ||
        (pause.stream_offline_pending_ms && pause.reasons.contains(PauseReason::StreamOffline)) ||
        (pause.client_disconnect_pending_ms &&
         pause.reasons.contains(PauseReason::DesignatedClientDisconnected)) ||
        (pause.stream_unknown_pending_ms &&
         (!pause.stream_unknown_notice ||
          pause.reasons.contains(PauseReason::StreamStatusUnknown))))
        return false;

    if (!current.mirror)
        return true;
    const auto &mirror = *current.mirror;
    return mirror.revision == current.scope.revision && same_timer(mirror.timer, timer) &&
           mirror.debt_ms == values.time_debt_ms &&
           mirror.deadline_utc_ms == values.config.hard_end_utc_ms &&
           mirror.pause_reasons == pause.reasons &&
           mirror.progression == current.policy->progression &&
           mirror.automation == current.policy->automation;
}

std::optional<Disposition> blocked(EffectEligibility eligibility, bool blocker) {
    switch (eligibility) {
    case EffectEligibility::eligible:
        return blocker ? std::optional{Disposition::held} : std::nullopt;
    case EffectEligibility::held:
        return Disposition::held;
    case EffectEligibility::unknown:
        return Disposition::unavailable;
    case EffectEligibility::duplicate:
        return Disposition::duplicate;
    case EffectEligibility::invalid:
        return Disposition::invalid;
    }
    return Disposition::invalid; // All eligibility enums were validated.
}

Disposition project_effect(const FinishedEffect &effect, const PostFinishPolicy &policy) {
    if (const auto refusal = blocked(effect.eligibility, effect.blocker_ref.has_value()))
        return *refusal;
    if (effect.kind == FinishedEffectKind::timer_credit)
        return Disposition::finished;
    if (effect.kind == FinishedEffectKind::milestone_award && !policy.progression)
        return Disposition::progression_off;
    return Disposition::pass_to_owner;
}

using EffectsByRef = std::map<std::string_view, std::optional<Disposition>>;

Disposition project_action(const FinishedAction &action, const PostFinishPolicy &policy,
                           const EffectsByRef &effects) {
    if (const auto refusal = blocked(action.eligibility, action.blocker_ref.has_value()))
        return *refusal;
    if (!policy.automation)
        return Disposition::automation_off;
    if (action.kind == FinishedActionKind::milestone && !policy.progression)
        return Disposition::trigger_suppressed;

    bool unavailable = false;
    bool suppressed = false;
    for (const auto &ref : action.required_effects) {
        const auto found = effects.find(ref);
        if (found == effects.end() || !found->second || *found->second == Disposition::unavailable)
            unavailable = true;
        else if (*found->second == Disposition::held)
            return Disposition::held;
        else if (*found->second != Disposition::pass_to_owner)
            suppressed = true;
    }
    if (unavailable)
        return Disposition::unavailable;
    if (suppressed)
        return Disposition::trigger_suppressed;
    return action.kind == FinishedActionKind::milestone ? Disposition::consider_in_group
                                                        : Disposition::pass_to_owner;
}

using ChoicesByOccurrence = std::map<std::string_view, Choice>;
using Decisions = std::map<std::pair<std::string_view, Choice>, Access>;

Choice selected_choice(const ChoicesByOccurrence &choices, const FinishedGroup &group) {
    const auto found = choices.find(group.occurrence);
    return found == choices.end() ? Choice::keep_deferred : found->second;
}

// Bind group authority only to requested Resume/Skip pairs. A definitive denial
// wins even when the group or preview has since changed.
Decisions group_decisions(const FinishedAccess &access, const ChoicesByOccurrence &choices,
                          const ReopenIntent &reopen) {
    Decisions decisions;
    for (const auto &grant : access.group) {
        const auto &choice = grant.choice;
        if (choice.choice == Choice::keep_deferred || !choices.contains(choice.occurrence))
            continue;
        const auto [entry, inserted] = decisions.emplace(
            std::pair{std::string_view(choice.occurrence), choice.choice}, grant.state);
        if (!inserted) {
            if (entry->second == Access::denied || grant.state == Access::denied)
                entry->second = Access::denied;
            else
                entry->second = Access::unknown;
        }
    }
    for (const auto &choice : reopen.choices)
        if (const auto found = decisions.find({choice.occurrence, choice.choice});
            found != decisions.end() && found->second == Access::denied)
            throw Refusal::access_denied;
    return decisions;
}

using PendingGroups = std::vector<const FinishedGroup *>;

bool matches_preview(const PendingGroups &pending, const ReopenPreview &preview) {
    return std::ranges::equal(pending, preview.pending, {},
                              [](const auto *group) -> const auto & { return *group; });
}

bool matches_canonical(const ReopenIntent &bound, const ChoicesByOccurrence &choices,
                       const PendingGroups &pending) {
    return std::ranges::equal(pending, bound.choices, [&](const auto *group, const auto &echo) {
        return echo.occurrence == group->occurrence &&
               echo.choice == selected_choice(choices, *group);
    });
}

void check_deadline(const TimerEditorialValues &values, const FinishedObservation &observation) {
    const auto deadline = values.config.hard_end_utc_ms;
    if (!deadline)
        return;
    if (observation.current_setting == DeadlineReached::reached ||
        (observation.utc_ms && *observation.utc_ms >= *deadline))
        throw Refusal::deadline_passed;
    if (!observation.utc_ms || observation.current_setting == DeadlineReached::unknown)
        throw Refusal::deadline_unknown;
}

ReopenGroupDisposition group_disposition(Choice choice) {
    switch (choice) {
    case Choice::keep_deferred:
        return ReopenGroupDisposition::deferred;
    case Choice::resume:
        return ReopenGroupDisposition::ordinary_consideration;
    case Choice::skip:
        return ReopenGroupDisposition::omit_pending;
    }
    throw Refusal::invalid_input;
}

struct ReopenSelection {
    PendingGroups pending;
    ChoicesByOccurrence choices;
};

ReopenSelection check_reopen(const FinishedSnapshot &current, const ReopenIntent &reopen,
                             const ReopenIntent &bound, const std::optional<ReopenPreview> &preview,
                             ChoicesByOccurrence choices, bool duplicate_choice,
                             const Decisions &decisions) {
    PendingGroups pending;
    bool coherent_groups = true;
    std::size_t matching_choices = 0;
    if (current.groups) {
        std::set<std::string_view> occurrences;
        std::set<std::uint64_t> ranks;
        for (const auto &group : *current.groups) {
            const bool unique_occurrence = occurrences.insert(group.occurrence).second;
            const bool unique_rank = ranks.insert(group.queue_order).second;
            coherent_groups &= unique_occurrence && unique_rank;
            if (group.committed != GroupCommit::pending)
                continue;
            pending.push_back(&group);
            matching_choices += choices.contains(group.occurrence);
            coherent_groups &= group.active_holds.has_value() && group.bonus_receipts.empty() &&
                               std::ranges::all_of(group.jobs, [](const GroupJobFact &job) {
                                   return job.state == GroupJobState::queued;
                               });
        }
        std::ranges::sort(pending, {}, &FinishedGroup::queue_order);
        if (preview && !matches_preview(pending, *preview))
            throw Refusal::stale_basis;
    }

    const bool invalid_choice = duplicate_choice || matching_choices != choices.size();
    // For a malformed selection there is no canonical intent. An exact raw echo
    // can reach the choice gate, but can never authorize a proposal.
    if (current.groups && !invalid_choice) {
        if (!matches_canonical(bound, choices, pending))
            throw Refusal::stale_basis;
    } else if (bound.choices != reopen.choices) {
        throw Refusal::stale_basis;
    }

    if (!coherent(current) || !current.groups || !preview || !coherent_groups)
        throw Refusal::facts_unavailable;
    const auto &values = *current.values;
    if (values.timer.lifecycle != TimerLifecycle::Finished)
        throw Refusal::wrong_lifecycle;
    check_deadline(values, current.observation);
    if (!reopen.grace)
        throw Refusal::grace_choice_required;
    if (reopen.absolute_main_ms <= 0 ||
        (values.maximum_remaining_ms && reopen.absolute_main_ms > *values.maximum_remaining_ms))
        throw Refusal::main_out_of_range;
    if (invalid_choice)
        throw Refusal::invalid_group_choice;
    for (const auto *group : pending)
        if (selected_choice(choices, *group) == Choice::skip && !group->active_holds->empty())
            throw Refusal::group_held;
    for (const auto &[occurrence, choice] : choices) {
        if (choice == Choice::keep_deferred)
            continue;
        const auto grant = decisions.find({occurrence, choice});
        if (grant == decisions.end() || grant->second != Access::allowed)
            throw Refusal::group_decision_unavailable;
    }
    return {std::move(pending), std::move(choices)};
}

FinishedCommandProposal materialize(const FinishedSnapshot &current, const FinishedAccess &access,
                                    const TimerEditorialValues &after) {
    FinishedCommandProposal proposal{
        current, {access, current.observation, *current.values, after}, {}, 0, {}};
    proposal.after.values = after;
    if (proposal.after.mirror) {
        proposal.after.mirror->timer = after.timer;
        proposal.after.mirror->debt_ms = after.time_debt_ms;
    }
    return proposal;
}

FinishedCommandResult project_command(const FinishedSnapshot &current,
                                      const FinishedCommand &command, const FinishedAccess &access,
                                      const std::optional<ReopenPreview> &preview) {
    validate_snapshot(current);
    require(valid_scope(command.expected) && !command.operation_ref.empty() &&
            !command.observation_ref.empty() && valid(access.state, Access::unknown));
    validate_intent(command.intent);
    if (access.state == Access::allowed)
        validate_intent(access.bound_command.intent);
    for (const auto &grant : access.group) {
        validate_choice(grant.choice);
        require(valid(grant.state, Access::unknown));
    }
    if (preview)
        for (const auto &group : preview->pending)
            validate_group(group);

    if (access.state == Access::denied)
        return Refusal::access_denied;
    if (access.state == Access::unknown || access.actor.empty() || access.session.empty() ||
        access.permission_revision.empty() || access.permission_catalog.empty())
        return Refusal::access_unknown;

    const auto *reopen = std::get_if<ReopenIntent>(&command.intent);
    ChoicesByOccurrence choices;
    Decisions decisions;
    bool duplicate_choice = false;
    if (reopen) {
        for (const auto &choice : reopen->choices)
            if (!choices.emplace(choice.occurrence, choice.choice).second)
                duplicate_choice = true;
        decisions = group_decisions(access, choices, *reopen);
    }

    const auto &bound = access.bound_command;
    if (command.expected != current.scope || command.observation_ref != current.observation.ref ||
        command.expected != bound.expected || command.operation_ref != bound.operation_ref ||
        command.observation_ref != bound.observation_ref)
        return Refusal::stale_basis;

    if (!reopen) {
        if (command.intent != bound.intent)
            return Refusal::stale_basis;
        if (!coherent(current))
            return Refusal::facts_unavailable;
        if (current.values->timer.lifecycle == TimerLifecycle::Ready)
            return Refusal::wrong_lifecycle;
        auto after = *current.values;
        after.time_debt_ms = std::get<DebtCorrectionIntent>(command.intent).absolute_debt_ms;
        return materialize(current, access, after);
    }

    const auto *bound_reopen = std::get_if<ReopenIntent>(&bound.intent);
    if (!bound_reopen || reopen->absolute_main_ms != bound_reopen->absolute_main_ms ||
        reopen->grace != bound_reopen->grace)
        return Refusal::stale_basis;
    const auto selection = check_reopen(current, *reopen, *bound_reopen, preview,
                                        std::move(choices), duplicate_choice, decisions);
    auto after = *current.values;
    after.timer = {TimerLifecycle::Active, CountdownPhase::Main, reopen->absolute_main_ms,
                   *reopen->grace == ReopenGrace::retain ? after.timer.grace_remaining_ms
                                                         : after.config.grace_duration_ms,
                   false};
    auto proposal = materialize(current, access, after);
    proposal.groups.reserve(selection.pending.size());
    for (const auto *group : selection.pending) {
        const auto choice = selected_choice(selection.choices, *group);
        proposal.groups.push_back(
            {{group->occurrence, choice}, group_disposition(choice), *group->active_holds});
    }
    return proposal;
}
} // namespace

FinishedInputResult project_finished_input(const FinishedSnapshot &current,
                                           const FinishedSupport &support,
                                           const FinishedLimits &limits) {
    try {
        preflight_input_proposal(current, support, limits);
        validate_snapshot(current);
        validate_support(support);
        if (!coherent(current))
            return Refusal::facts_unavailable;
        if (current.values->timer.lifecycle != TimerLifecycle::Finished)
            return Refusal::wrong_lifecycle;

        const auto &policy = *current.policy;
        FinishedInputProposal proposal{current.scope, current.observation, policy, support, {}, {}};
        proposal.effects.reserve(support.effects.size());
        proposal.actions.reserve(support.actions.size());
        EffectsByRef effects;
        for (const auto &effect : support.effects) {
            const auto disposition = project_effect(effect, policy);
            proposal.effects.push_back(disposition);
            if (const auto [entry, inserted] = effects.emplace(effect.ref, disposition); !inserted)
                entry->second.reset(); // An ambiguous ref cannot establish trigger clearance.
        }
        for (const auto &action : support.actions)
            proposal.actions.push_back(project_action(action, policy, effects));
        return proposal;
    } catch (Refusal refusal) {
        return refusal;
    } catch (const std::bad_alloc &) {
        return Refusal::allocation_failure;
    } catch (const std::length_error &) {
        return Refusal::allocation_failure;
    }
}

FinishedCommandResult project_finished_command(const FinishedSnapshot &current,
                                               const FinishedCommand &command,
                                               const FinishedAccess &access,
                                               const std::optional<ReopenPreview> &preview,
                                               const FinishedLimits &limits) {
    try {
        preflight_command_proposal(current, command, access, preview, limits);
        return project_command(current, command, access, preview);
    } catch (Refusal refusal) {
        return refusal;
    } catch (const std::bad_alloc &) {
        return Refusal::allocation_failure;
    } catch (const std::length_error &) {
        return Refusal::allocation_failure;
    }
}
} // namespace subathon::core
