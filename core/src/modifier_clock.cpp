// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/modifier_clock.hpp>

#include <algorithm>
#include <initializer_list>
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

bool same_timer(const TimerState &left, const TimerState &right) {
    return left.lifecycle == right.lifecycle && left.phase == right.phase &&
           left.main_remaining_ms == right.main_remaining_ms &&
           left.grace_remaining_ms == right.grace_remaining_ms &&
           left.awaiting_confirmation == right.awaiting_confirmation;
}

bool same_pause(const PauseState &left, const PauseState &right) {
    return left.reasons == right.reasons &&
           left.stream_offline_pending_ms == right.stream_offline_pending_ms &&
           left.client_disconnect_pending_ms == right.client_disconnect_pending_ms &&
           left.stream_unknown_pending_ms == right.stream_unknown_pending_ms &&
           left.stream_unknown_notice == right.stream_unknown_notice;
}

bool coherent_cut(const TimerState &before, const TimerClockStep &step,
                  const TimerAdvance &advance) {
    using Life = TimerLifecycle;
    using Phase = CountdownPhase;
    using T = TimerTransition;

    if (before.main_remaining_ms < 0 || before.grace_remaining_ms < 0 ||
        (before.phase != Phase::Main &&
         (before.phase != Phase::Grace || before.main_remaining_ms != 0)) ||
        (before.awaiting_confirmation &&
         (before.lifecycle != Life::Active || before.phase != Phase::Main ||
          before.main_remaining_ms != 0))) {
        return false;
    }

    const auto elapsed = step.paused ? Milliseconds{0} : step.elapsed_ms;
    const auto progressed = advance.progressed_ms;
    if (progressed < 0 || progressed > elapsed)
        return false;

    auto expected = before;
    const auto matches = [&](std::initializer_list<T> transitions) {
        return same_timer(expected, advance.state) &&
               std::ranges::equal(advance.transitions, transitions);
    };

    if (before.lifecycle != Life::Active) {
        return (before.lifecycle == Life::Ready || before.lifecycle == Life::Finished) &&
               progressed == 0 && matches({});
    }
    if (!advance.transitions.empty() && advance.transitions.front() == T::HardEndReached) {
        expected.lifecycle = Life::Finished;
        expected.awaiting_confirmation = false;
        return progressed == 0 && matches({T::HardEndReached, T::Finished});
    }
    if (before.awaiting_confirmation) {
        if (progressed != 0 || !advance.transitions.empty())
            return false;
        // A separately accepted credit can rescue Main at the same observation.
        // The credit changes the timer balance without advancing either clock.
        if (step.elapsed_ms == 0 && advance.state.main_remaining_ms > 0)
            expected = apply_applicable_time_credit(before, advance.state.main_remaining_ms);
        return matches({});
    }

    if (before.phase == Phase::Grace) {
        const auto consumed = std::min(elapsed, before.grace_remaining_ms);
        expected.grace_remaining_ms -= consumed;
        if (expected.grace_remaining_ms > 0)
            return progressed == consumed && matches({});
        expected.lifecycle = Life::Finished;
        return progressed == consumed && matches({T::GraceExpired, T::Finished});
    }

    const auto main = before.main_remaining_ms;
    if (elapsed < main) {
        expected.main_remaining_ms -= elapsed;
        return progressed == elapsed && matches({});
    }

    expected.main_remaining_ms = 0;
    if (advance.state.phase == Phase::Main) {
        if (advance.state.awaiting_confirmation) {
            expected.awaiting_confirmation = true;
            return progressed == main && matches({T::MainExpired, T::ConfirmationRequired});
        }
        expected.lifecycle = Life::Finished;
        return progressed == main && matches({T::MainExpired, T::Finished});
    }

    expected.phase = Phase::Grace;
    if (advance.state.lifecycle == Life::Active) {
        // Ongoing Grace consumes all overshoot. Its starting balance must fit in
        // the timer's duration range, even when Fresh replaces the retained balance.
        const auto remaining = advance.state.grace_remaining_ms;
        expected.grace_remaining_ms = remaining;
        return progressed == elapsed && remaining > 0 &&
               remaining <= std::numeric_limits<Milliseconds>::max() - (elapsed - main) &&
               matches({T::MainExpired, T::GraceStarted});
    }

    // When the cut also exhausts Grace, only the producer knows its starting
    // balance. Shared and Fresh are indistinguishable here. Check the bounds
    // and full trace without inferring a configuration from the retained balance.
    expected.lifecycle = Life::Finished;
    expected.grace_remaining_ms = 0;
    return progressed >= main &&
           matches({T::MainExpired, T::GraceStarted, T::GraceExpired, T::Finished});
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

void validate_count_progression(const DowntimeRecoveryInput &input,
                                const RecoveryTimerProposal &selected) {
    // Reuse CORE-006's reconstruction, including schedules and the D-1 rule,
    // to verify the complete selected branch before any modifier projection.
    const auto outcome = preview_downtime_recovery(input);
    const auto *preview = std::get_if<DowntimeRecoveryPreview>(&outcome);
    require(preview && preview->count_elapsed,
            "Modifier recovery requires available Count evidence");
    const auto &expected = *preview->count_elapsed;
    require(
        same_timer(selected.timer, expected.timer) && same_pause(selected.pause, expected.pause) &&
            std::ranges::equal(selected.transitions, expected.transitions,
                               [](const RecoveryTransition &left, const RecoveryTransition &right) {
                                   return left.transition == right.transition &&
                                          left.utc == right.utc && left.basis == right.basis;
                               }) &&
            std::ranges::equal(
                selected.progression, expected.progression,
                [](const RecoveryProgressSlice &left, const RecoveryProgressSlice &right) {
                    return left.from == right.from && left.elapsed_ms == right.elapsed_ms;
                }),
        "Modifier recovery requires the complete selected Count evidence");
}

ModifierRecoveryProjection project_offline_progress(const DowntimeRecoveryInput &input,
                                                    const RecoveryTimerProposal &selected,
                                                    const ModifierClockValue &checkpoint) {
    validate_count_progression(input, selected);
    ModifierRecoveryProjection result{checkpoint, {}};
    if (!live(checkpoint))
        return result;

    for (const auto &slice : selected.progression) {
        const auto remaining = *result.value.remaining_ms;
        if (debit(result.value, slice.elapsed_ms)) {
            result.events.push_back(candidate(ModifierRuleEvent::Expired,
                                              restart_observation(input), checkpoint.revision,
                                              RecoveryUtcInstant{slice.from.ms + remaining}));
            break;
        }
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

    require(coherent_cut(timer_before, step, timer_advance),
            "Modifier cut requires coherent timer progression");

    ModifierCutProposal result{head, {}, {}};
    result.events.assign(timer_advance.transitions.begin(), timer_advance.transitions.end());
    if (result.head) {
        const auto elapsed = result.head->mode == ModifierClockMode::RealElapsed
                                 ? step.elapsed_ms
                                 : timer_advance.progressed_ms;
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

    if (current.mode == ModifierClockMode::RealElapsed)
        return {current, {}};
    const auto &source = current.remaining_ms == 0 ? current : checkpoint;
    if (resolved->command.choice == RecoveryOfflineChoice::Freeze)
        return {source, {}};
    // Only the selected offline branch progresses Active time. The resolved
    // timer can also include a current finish and an editorial correction.
    return project_offline_progress(input, resolved->selected_at_restart, source);
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
