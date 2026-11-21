// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/modifier_clock.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace subathon::core {
namespace {

using Milliseconds = std::int64_t;

void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}

bool live(const ModifierClockValue &value) {
    if (!value.remaining_ms)
        return false;
    require(*value.remaining_ms >= 0, "Modifier remaining duration must be nonnegative");
    return *value.remaining_ms > 0;
}

bool debit(ModifierClockValue &value, Milliseconds elapsed) {
    if (!live(value))
        return false;
    auto &remaining = *value.remaining_ms;
    remaining -= std::min(remaining, elapsed);
    return remaining == 0;
}

ModifierRuleCandidate candidate(ModifierRuleEvent event,
                                const ModifierClockObservation &observation,
                                const ModifierInstance &instance,
                                std::optional<RecoveryUtcInstant> crossing = {}) {
    return {event, observation, instance.instance_ref, instance.revision_ref, crossing};
}

bool same_basis(const RecoveryBasis &left, const RecoveryBasis &right) {
    return left.timer_lineage == right.timer_lineage &&
           left.checkpoint_ref == right.checkpoint_ref &&
           left.configuration_revision == right.configuration_revision &&
           left.facts_bundle_ref == right.facts_bundle_ref;
}

std::optional<Milliseconds> qualified_gap(const DowntimeRecoveryInput &input) {
    const auto *gap = std::get_if<RecoveryContinuousGap>(&input.gap);
    if (!gap)
        return std::nullopt;
    const auto from = gap->checkpoint.ms;
    const auto to = gap->restart.ms;
    const auto span = static_cast<std::uint64_t>(to) - static_cast<std::uint64_t>(from);
    require(!gap->evidence_ref.empty() && input.checkpoint.utc == gap->checkpoint &&
                input.restart.utc == gap->restart && from <= to &&
                span <= static_cast<std::uint64_t>(std::numeric_limits<Milliseconds>::max()),
            "Modifier recovery gap must bind the supplied observations");
    return static_cast<Milliseconds>(span);
}

ModifierClockObservation restart_observation(const DowntimeRecoveryInput &input) {
    return {input.basis.timer_lineage, input.restart.observation_ref, input.restart.utc.value().ms};
}

ModifierRecoveryProjection project_offline_progress(const DowntimeRecoveryInput &input,
                                                    const RecoveryTimerProposal &selected,
                                                    const ModifierClockValue &checkpoint) {
    ModifierRecoveryProjection result{checkpoint, {}};
    if (!live(checkpoint) || selected.progression.empty())
        return result;

    require(qualified_gap(input).has_value(), "Offline progression requires a qualified gap");
    auto through = input.checkpoint.utc->ms;
    const auto restart = input.restart.utc->ms;
    for (const auto &slice : selected.progression) {
        const auto from = slice.from.ms;
        require(through <= from && from <= restart && slice.elapsed_ms > 0 &&
                    slice.elapsed_ms <= restart - from,
                "Modifier recovery slice falls outside the qualified gap");
        const auto remaining = *result.value.remaining_ms;
        if (debit(result.value, slice.elapsed_ms)) {
            result.events.push_back(candidate(ModifierRuleEvent::Expired,
                                              restart_observation(input), checkpoint.revision,
                                              RecoveryUtcInstant{from + remaining}));
            break;
        }
        through = from + slice.elapsed_ms;
    }
    return result;
}

} // namespace

ModifierCutProposal project_modifier_cut(const ModifierClockObservation &before,
                                         const ModifierClockObservation &now,
                                         const TimerState &timer_before, const TimerClockStep &step,
                                         const TimerAdvance &timer_advance,
                                         const std::optional<ModifierClockValue> &head,
                                         const std::optional<ModifierActivationAtCut> &activation) {
    require(!now.timer_lineage.empty() && before.timer_lineage == now.timer_lineage &&
                !before.ref.empty() && !now.ref.empty() && step.utc_now_ms == now.utc_ms &&
                step.elapsed_ms >= 0 &&
                (before.ref != now.ref || (step.elapsed_ms == 0 && before.utc_ms == now.utc_ms)),
            "Modifier cut must bind an admitted step on the same lineage");

    const auto progressed = timer_advance.progressed_ms;
    require(0 <= progressed && progressed <= step.elapsed_ms &&
                (progressed == 0 || (timer_before.lifecycle == TimerLifecycle::Active &&
                                     !timer_before.awaiting_confirmation && !step.paused)),
            "Modifier cut requires valid timer progression");

    ModifierCutProposal result{head, {}, {}};
    result.events.assign(timer_advance.transitions.begin(), timer_advance.transitions.end());
    if (result.head) {
        const auto elapsed =
            result.head->mode == ModifierClockMode::RealElapsed ? step.elapsed_ms : progressed;
        if (debit(*result.head, elapsed)) {
            result.events.emplace_back(
                candidate(ModifierRuleEvent::Expired, now, result.head->revision));
        }
    }
    if (!activation)
        return result;

    std::optional<ModifierInstance> active;
    if (result.head) {
        require(result.head->remaining_ms.has_value(),
                "Modifier activation requires a resolved current lifetime");
        if (live(*result.head)) {
            require(result.head->revision.captured_definition.definition_ref ==
                        activation->definition.definition_ref,
                    "Modifier activation requires the live head of its definition");
            active = result.head->revision;
            active->remaining_ms = *result.head->remaining_ms;
        }
    }
    result.activation =
        propose_modifier_activation(activation->definition, active, activation->request);
    if (const auto &revision = result.activation->revision_to_append) {
        require(revision->remaining_ms > 0, "A modifier activation requires positive duration");
        const auto mode = revision->parent_revision_ref
                              ? result.head->mode
                              : activation->clock.value_or(ModifierClockMode::ActiveSubathon);
        result.head = ModifierClockValue{*revision, revision->remaining_ms, mode};
        result.events.emplace_back(candidate(ModifierRuleEvent::Activated, now, *revision));
    }
    return result;
}

ModifierRecoveryProjection project_modifier_restart(const DowntimeRecoveryInput &input,
                                                    const DowntimeRecoveryPreview &preview,
                                                    const ModifierClockValue &checkpoint) {
    require(same_basis(input.basis, preview.basis) && input.policy == preview.policy,
            "Modifier restart requires the matching timer preview");
    if (checkpoint.mode == ModifierClockMode::ActiveSubathon) {
        if (preview.policy == DowntimePolicy::CountElapsed && preview.count_elapsed)
            return project_offline_progress(input, *preview.count_elapsed, checkpoint);
        return {checkpoint, {}};
    }

    ModifierRecoveryProjection result{checkpoint, {}};
    if (!live(checkpoint))
        return result;
    const auto elapsed = qualified_gap(input);
    if (!elapsed) {
        result.value.remaining_ms.reset();
    } else if (debit(result.value, *elapsed)) {
        const auto crossing = input.checkpoint.utc->ms + *checkpoint.remaining_ms;
        result.events.push_back(candidate(ModifierRuleEvent::Expired, restart_observation(input),
                                          checkpoint.revision, RecoveryUtcInstant{crossing}));
    }
    return result;
}

ModifierRecoveryProjection project_modifier_review_wait(const RecoveryReviewSnapshot &previous,
                                                        const RecoveryReviewSnapshot &observation,
                                                        const RecoveryReviewWait &timer_wait,
                                                        const TimerClockStep &step,
                                                        const ModifierClockValue &current) {
    constexpr auto review = PauseReason::RecoveryReview;
    require(previous.timer_lineage == observation.timer_lineage &&
                previous.episode_ref == observation.episode_ref &&
                previous.facts_bundle_ref == observation.facts_bundle_ref &&
                previous.pause.reasons.contains(review) &&
                observation.pause.reasons.contains(review) &&
                timer_wait.held.pause.reasons == observation.pause.reasons,
            "Modifier wait requires the same pending Recovery Review");
    require(
        step.elapsed_ms >= 0 &&
            (previous.observed_at.observation_ref != observation.observed_at.observation_ref ||
             step.elapsed_ms == 0) &&
            (!observation.observed_at.utc || observation.observed_at.utc->ms == step.utc_now_ms),
        "Modifier wait requires an admitted step at the supplied observation");

    ModifierRecoveryProjection result{current, {}};
    if (current.mode == ModifierClockMode::RealElapsed && debit(result.value, step.elapsed_ms)) {
        // The admitted step supplies the cut's UTC observation, not the expiry instant.
        const ModifierClockObservation now{
            observation.timer_lineage, observation.observed_at.observation_ref, step.utc_now_ms};
        result.events.push_back(candidate(ModifierRuleEvent::Expired, now, current.revision));
    }
    return result;
}

ModifierRecoveryProjection project_modifier_review_resolution(
    const DowntimeRecoveryInput &input, const RecoveryReviewSnapshot &snapshot,
    const std::variant<RecoveryReviewResolution, RecoveryReviewRefusal, RecoveryCorrectionRefusal>
        &timer_resolution,
    const ModifierClockValue &checkpoint, const ModifierClockValue &current) {
    const auto *resolved = std::get_if<RecoveryReviewResolution>(&timer_resolution);
    if (!resolved)
        return {current, {}};
    require(same_basis(input.basis, resolved->basis) &&
                snapshot.timer_lineage == input.basis.timer_lineage &&
                snapshot.facts_bundle_ref == input.basis.facts_bundle_ref &&
                resolved->command.expected_episode_ref == snapshot.episode_ref &&
                resolved->command.expected_revision == snapshot.current.revision &&
                resolved->command.expected_facts_bundle_ref == snapshot.facts_bundle_ref &&
                resolved->decision_observation.observation_ref ==
                    snapshot.observed_at.observation_ref &&
                resolved->decision_observation.utc == snapshot.observed_at.utc &&
                snapshot.pause.reasons.contains(PauseReason::RecoveryReview) &&
                resolved->command.choice.has_value(),
            "Modifier resolution requires the matching Recovery Review decision");
    require(checkpoint.mode == current.mode && checkpoint.revision == current.revision,
            "Modifier resolution requires the checkpoint and current capture of one revision");

    if (current.mode == ModifierClockMode::RealElapsed || current.remaining_ms == 0)
        return {current, {}};
    if (resolved->command.choice == RecoveryOfflineChoice::Freeze)
        return {checkpoint, {}};
    // Only the selected offline branch progresses Active time. The resolved
    // timer can also include a current finish and an editorial correction.
    return project_offline_progress(input, resolved->selected_at_restart, checkpoint);
}

ModifierScoreBasis capture_modifier_score(const ModifierScoreArrival &arrival,
                                          const TimerState &timer, const PauseState &pause,
                                          const std::optional<ModifierClockValue> &head) {
    const bool finished = timer.lifecycle == TimerLifecycle::Finished;
    const bool progression = timer.lifecycle == TimerLifecycle::Active ||
                             (finished && arrival.post_finish_score_progression);
    ModifierScoreBasis result{arrival.observation, arrival.input_ref, arrival.base_micro_points, {},
                              pause.reasons,       progression};
    if (!progression || (finished && !arrival.post_finish_modifiers) ||
        pause.reasons.contains(PauseReason::RecoveryReview) || !head ||
        (head->remaining_ms && !live(*head)))
        return result;

    // Independent pauses hold application of this basis. They still allow
    // capture of a known arrival-time selection for later evaluation.
    auto targets = select_modifier_targets(head->revision, arrival.matched_qualifiers);
    for (auto &selection : targets) {
        if (selection.target.kind != ModifierTargetKind::milestone_score ||
            selection.target.target_ref != arrival.target_ref)
            continue;
        require(head->remaining_ms.has_value(),
                "Score capture requires a resolved modifier lifetime");
        require(!result.selection, "Score capture requires a single matching modifier target");
        result.selection = std::move(selection);
    }
    return result;
}

ModifierScoreProposal request_modifier_score(const ModifierScoreBasis &basis) {
    if (!basis.score_progression || !basis.blockers.empty())
        return {true, {}};
    if (!basis.selection)
        return {true, basis.base_micro_points};
    return {true, round_effect_units(Exact{basis.base_micro_points} * basis.selection->factor)};
}

} // namespace subathon::core
