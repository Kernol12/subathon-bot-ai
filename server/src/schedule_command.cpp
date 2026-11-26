// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/schedule_command.hpp>

#include <algorithm>
#include <set>
#include <string_view>
#include <tuple>
#include <utility>

namespace subathon::server {
namespace {
using Kind = ScheduleCommandKind;
using Refusal = ScheduleRefusal;
template <class T> using Checked = std::variant<T, Refusal>;
using Cutover = decltype(SchedulePreviewRecord::cutover);

Refusal translate(ScheduledPauseRefusal refusal) {
    switch (refusal) {
    case ScheduledPauseRefusal::invalid_input:
        return Refusal::invalid_input;
    case ScheduledPauseRefusal::limit_exceeded:
        return Refusal::limit_exceeded;
    case ScheduledPauseRefusal::stale_observation:
        return Refusal::stale_observation;
    case ScheduledPauseRefusal::hard_end_reached:
        return Refusal::hard_end_reached;
    }
    return Refusal::invalid_input;
}

bool same(const ScheduledPauseObservation &a, const ScheduledPauseObservation &b) {
    return a.utc_ms == b.utc_ms && a.observation_ref == b.observation_ref;
}

bool same(const core::PauseState &a, const core::PauseState &b) {
    return std::tie(a.reasons, a.stream_offline_pending_ms, a.client_disconnect_pending_ms,
                    a.stream_unknown_pending_ms, a.stream_unknown_notice) ==
           std::tie(b.reasons, b.stream_offline_pending_ms, b.client_disconnect_pending_ms,
                    b.stream_unknown_pending_ms, b.stream_unknown_notice);
}

bool same(const ScheduledPauseConfiguration &a, const ScheduledPauseConfiguration &b) {
    return a.configured_zone == b.configured_zone &&
           std::ranges::equal(a.rules, b.rules, [](const auto &x, const auto &y) {
               const auto &r = x.recurrence;
               const auto &s = y.recurrence;
               return std::tie(x.schedule_id, x.enabled, x.duration, r.local_time, r.weekdays,
                               r.gap_policy, r.fold_policy) ==
                      std::tie(y.schedule_id, y.enabled, y.duration, s.local_time, s.weekdays,
                               s.gap_policy, s.fold_policy);
           });
}

bool valid_permit(const SchedulePermit &permit) {
    return !permit.actor.empty() && !permit.grant.empty() && !permit.permission_revision.empty();
}

bool same_grant(const SchedulePermit &a, const SchedulePermit &b) {
    return valid_permit(a) && valid_permit(b) && a.actor == b.actor && a.grant == b.grant;
}

bool authorized(const ScheduleAdmission &a, SchedulePorts &ports,
                const ScheduleCandidate *candidate = nullptr) {
    const auto access = ports.access.command(a.request, candidate);
    const auto *permit = std::get_if<SchedulePermit>(&access);
    return permit && same_grant(*permit, a.admitted);
}

bool snapshot_allowed(const ScheduleContext &context, SchedulePorts &ports,
                      const SchedulePermit *issuer = nullptr) {
    const auto access = ports.access.snapshot(context);
    const auto *permit = std::get_if<SchedulePermit>(&access);
    return permit && valid_permit(*permit) && (!issuer || permit->actor == issuer->actor);
}

Checked<ScheduleAdmission> admit_active(Kind kind, const ScheduleCommandRef &ref,
                                        SchedulePorts &ports) {
    auto a = ports.queue.active(ref);
    if (!a || ref.value.empty() || a->ref.value != ref.value || a->request.kind != kind) {
        return Refusal::not_admitted;
    }
    const auto &intent = a->request.intent;
    const auto *entry = std::get_if<ScheduleEntrySelection>(&intent);
    const bool valid = kind == Kind::preview ? std::holds_alternative<ScheduleEdit>(intent)
                       : kind == Kind::save
                           ? std::holds_alternative<ScheduleSave>(intent)
                           : entry && !entry->owning_timer_intent_ref.empty() && a->entry_timer &&
                                 a->entry_timer->lifecycle == core::TimerLifecycle::Active;
    if (!valid) {
        return Refusal::not_admitted;
    }
    if (!authorized(*a, ports)) {
        return Refusal::access_denied;
    }
    return std::move(*a);
}

struct Current {
    ScheduledPauseState state;
    ScheduleClock clock;
};

Checked<Current> current(const ScheduleAdmission &a, SchedulePorts &ports) {
    auto state = ports.writer.live(a.request.context);
    const auto &expected = a.request.expected;
    if (expected.snapshot_ref != state.snapshot_ref ||
        expected.state_revision != state.state_revision) {
        return Refusal::stale_observation;
    }
    auto observed = ports.clock.observe(a, state);
    if (const auto *refusal = std::get_if<Refusal>(&observed)) {
        return *refusal;
    }
    auto &clock = std::get<ScheduleClock>(observed);
    if (clock.snapshot_ref != state.snapshot_ref || clock.lineage != state.clock_lineage ||
        !same(clock.anchor, state.cursor.last_observation) || !same(clock.target, a.target) ||
        clock.target.utc_ms <= clock.anchor.utc_ms ||
        clock.target.observation_ref == clock.anchor.observation_ref) {
        return Refusal::stale_observation;
    }
    if (a.request.kind == Kind::preview || a.request.kind == Kind::save) {
        if (clock.cuts.size() > ScheduledPauseLimits::max_clock_cuts) {
            return Refusal::limit_exceeded;
        }
        if (clock.cuts.empty()) {
            return Refusal::invalid_input;
        }
        if (!same(clock.cuts.back().observation, clock.target)) {
            return Refusal::stale_observation;
        }
        std::set<std::string_view> refs{clock.anchor.observation_ref};
        auto previous = clock.anchor.utc_ms;
        for (const auto &cut : clock.cuts) {
            if (cut.observation.utc_ms <= previous ||
                !refs.insert(cut.observation.observation_ref).second) {
                return Refusal::stale_observation;
            }
            previous = cut.observation.utc_ms;
        }
    }
    return Current{std::move(state), std::move(clock)};
}

bool same_cuts(const ScheduleClock &a, const ScheduleClock &b, bool entry = false) {
    return a.snapshot_ref == b.snapshot_ref && a.lineage == b.lineage && same(a.anchor, b.anchor) &&
           same(a.target, b.target) &&
           (entry || std::ranges::equal(a.cuts, b.cuts, [](const auto &x, const auto &y) {
                return same(x.observation, y.observation) &&
                       x.elapsed_since_previous == y.elapsed_since_previous &&
                       same(x.independent_pause, y.independent_pause);
            }));
}

auto clock_input(const ScheduleClock &clock) {
    return ScheduledPauseClockInput{clock.snapshot_ref, clock.lineage, clock.anchor, clock.cuts};
}

Checked<Cutover> cutover(const ScheduledPauseState &state, const ScheduleClock &clock,
                         const ScheduleEdit &edit, ScheduleZoneSource &zones) {
    auto result = project_scheduled_pause_cutover(state, clock_input(clock), edit.proposed, zones);
    if (const auto *refusal = std::get_if<ScheduledPauseRefusal>(&result)) {
        return translate(*refusal);
    }
    if (const auto *failure = std::get_if<ScheduledPauseCoverageFailure>(&result)) {
        return Cutover{*failure};
    }
    return Cutover{std::get<ScheduledPauseCutover>(std::move(result))};
}

Checked<ScheduledPausePreview> forecast(const ScheduledPauseState &state,
                                        const ScheduleClock &clock, const ScheduleEdit &edit,
                                        ScheduleZoneSource &zones) {
    auto result = project_scheduled_pause_edit(state, clock_input(clock), edit.proposed,
                                               edit.through_utc_ms, zones);
    if (const auto *refusal = std::get_if<ScheduledPauseRefusal>(&result)) {
        return translate(*refusal);
    }
    return std::get<ScheduledPausePreview>(std::move(result));
}

bool same_scope(const ScheduleAdmission &a, const ScheduleAdmission &b) {
    const auto &x = a.request.context;
    const auto &y = b.request.context;
    return x.installation == y.installation && x.timer == y.timer &&
           same_grant(a.admitted, b.admitted);
}

bool same_base(const ScheduleBase &a, const ScheduleBase &b) {
    return a.snapshot_ref == b.snapshot_ref && a.state_revision == b.state_revision;
}

bool same(const core::TimerState &a, const core::TimerState &b) {
    return std::tie(a.lifecycle, a.phase, a.main_remaining_ms, a.grace_remaining_ms,
                    a.awaiting_confirmation) == std::tie(b.lifecycle, b.phase, b.main_remaining_ms,
                                                         b.grace_remaining_ms,
                                                         b.awaiting_confirmation);
}

bool same_confirm(const ScheduleConfirmation &a, const ScheduleConfirmation &b) {
    return a.issued.value == b.issued.value && a.basis == b.basis &&
           a.shown.has_value() == b.shown.has_value() &&
           (!a.shown || a.shown->value == b.shown->value);
}

bool same_intent(const ScheduleAdmission &a, const ScheduleAdmission &b) {
    if (const auto *save = std::get_if<ScheduleSave>(&a.request.intent)) {
        const auto &other = std::get<ScheduleSave>(b.request.intent);
        return same(save->edit.proposed, other.edit.proposed) &&
               save->edit.through_utc_ms == other.edit.through_utc_ms &&
               same_confirm(save->confirm, other.confirm);
    }
    return std::get<ScheduleEntrySelection>(a.request.intent).owning_timer_intent_ref ==
               std::get<ScheduleEntrySelection>(b.request.intent).owning_timer_intent_ref &&
           same(*a.entry_timer, *b.entry_timer);
}

bool same_binding(const ScheduledPauseFactBinding &a, const ScheduledPauseFactBinding &b) {
    return a.old_epoch_index == b.old_epoch_index &&
           a.facts.configured_zone == b.facts.configured_zone &&
           a.facts.rules_revision == b.facts.rules_revision;
}

auto transitions_after(const ScheduleZoneFacts &facts, std::int64_t at) {
    return std::ranges::upper_bound(facts.transitions, at, {},
                                    &core::ScheduleZoneTransition::at_utc_ms);
}

bool agree(const ScheduleZoneFacts &a, const ScheduleZoneFacts &b) {
    const auto first = std::max(a.from_utc_ms, b.from_utc_ms);
    const auto last = std::min(a.through_utc_ms, b.through_utc_ms);
    if (first > last) {
        return true;
    }
    const auto x = transitions_after(a, first);
    const auto y = transitions_after(b, first);
    const auto a_offset =
        x == a.transitions.begin() ? a.offset_at_start_seconds : (x - 1)->offset_after_seconds;
    const auto b_offset =
        y == b.transitions.begin() ? b.offset_at_start_seconds : (y - 1)->offset_after_seconds;
    return a_offset == b_offset &&
           std::equal(x, transitions_after(a, last), y, transitions_after(b, last),
                      [](const auto &p, const auto &q) {
                          return p.at_utc_ms == q.at_utc_ms &&
                                 p.offset_after_seconds == q.offset_after_seconds;
                      });
}

bool same_facts(const ScheduledPausePreview &a, const ScheduledPausePreview &b) {
    return std::ranges::equal(a.facts, b.facts, [](const auto &x, const auto &y) {
        return same_binding(x, y) && x.facts.from_utc_ms == y.facts.from_utc_ms &&
               x.facts.through_utc_ms == y.facts.through_utc_ms && agree(x.facts, y.facts);
    });
}

bool overlapping(const ScheduledPauseFactBinding &a, const ScheduledPauseFactBinding &b) {
    return std::max(a.facts.from_utc_ms, b.facts.from_utc_ms) <=
           std::min(a.facts.through_utc_ms, b.facts.through_utc_ms);
}

bool compatible_facts(const ScheduledPausePreview &a, const ScheduledPausePreview &b) {
    const auto supported = [](const auto &left, const auto &right) {
        return std::ranges::all_of(left, [&](const auto &x) {
            return std::ranges::any_of(
                right, [&](const auto &y) { return same_binding(x, y) && overlapping(x, y); });
        });
    };
    return supported(a.facts, b.facts) && supported(b.facts, a.facts) &&
           std::ranges::none_of(a.facts, [&](const auto &x) {
               return std::ranges::any_of(b.facts, [&](const auto &y) {
                   return same_binding(x, y) && !agree(x.facts, y.facts);
               });
           });
}

bool same_stop(const std::optional<ScheduledPauseStop> &a,
               const std::optional<ScheduledPauseStop> &b, const ScheduledPauseObservation &x,
               const ScheduledPauseObservation &y) {
    if (!a || !b) {
        return a.has_value() == b.has_value();
    }
    return a->cause == b->cause &&
           (a->effective_utc_ms == b->effective_utc_ms ||
            (a->cause != ScheduledPauseStopCause::expired && a->effective_utc_ms == x.utc_ms &&
             b->effective_utc_ms == y.utc_ms)) &&
           (same(a->reconciled_at, b->reconciled_at) ||
            (same(a->reconciled_at, x) && same(b->reconciled_at, y)));
}

bool same_delivery(const DeliveredScheduledPause &a, const DeliveredScheduledPause &b,
                   const ScheduledPauseObservation &x, const ScheduledPauseObservation &y) {
    return a.key == b.key && a.window.start_utc_ms == b.window.start_utc_ms &&
           a.window.end_utc_ms == b.window.end_utc_ms && same_stop(a.stop, b.stop, x, y);
}

bool same_effects(const ScheduledPausePreview &a, const ScheduledPausePreview &b) {
    return a.pause_after_cutover.has_value() == b.pause_after_cutover.has_value() &&
           (!a.pause_after_cutover || same(*a.pause_after_cutover, *b.pause_after_cutover)) &&
           std::ranges::equal(a.upcoming, b.upcoming,
                              [](const auto &x, const auto &y) {
                                  return x.key == y.key && x.window == y.window;
                              }) &&
           std::ranges::equal(
               a.cutover_effects, b.cutover_effects, [&](const auto &x, const auto &y) {
                   return same_delivery(x.delivery, y.delivery, a.as_of, b.as_of) &&
                          same_stop(x.stop_at_cutover, y.stop_at_cutover, a.as_of, b.as_of);
               });
}

bool same_projection(const ScheduledPausePreview &a, const ScheduledPausePreview &b) {
    return a.source_snapshot_ref == b.source_snapshot_ref && a.timer_lineage == b.timer_lineage &&
           a.clock_lineage == b.clock_lineage && a.state_revision == b.state_revision &&
           a.configuration_revision == b.configuration_revision &&
           a.delivery_revision == b.delivery_revision && same(a.as_of, b.as_of) &&
           same(a.proposed, b.proposed) &&
           a.requested_through_utc_ms == b.requested_through_utc_ms &&
           a.resolved_through_utc_ms == b.resolved_through_utc_ms && a.incomplete == b.incomplete &&
           same_facts(a, b) && same_effects(a, b);
}

bool same_cutover(const Cutover &a, const Cutover &b) {
    if (a.index() != b.index()) {
        return false;
    }
    if (const auto *failure = std::get_if<ScheduledPauseCoverageFailure>(&a)) {
        return *failure == std::get<ScheduledPauseCoverageFailure>(b);
    }
    const auto &x = std::get<ScheduledPauseCutover>(a);
    const auto &y = std::get<ScheduledPauseCutover>(b);
    const auto &p = x.progress;
    const auto &q = y.progress;
    const auto &t = p.timer;
    const auto &u = q.timer;
    const auto &xh = p.cursor.hold;
    const auto &yh = q.cursor.hold;
    return x.closed_zone_rules_revision == y.closed_zone_rules_revision &&
           std::tie(t.lifecycle, t.phase, t.awaiting_confirmation) ==
               std::tie(u.lifecycle, u.phase, u.awaiting_confirmation) &&
           (!same(x.cutover, y.cutover) ||
            std::tie(t.main_remaining_ms, t.grace_remaining_ms) ==
                std::tie(u.main_remaining_ms, u.grace_remaining_ms)) &&
           same(p.pause, q.pause) && p.next_known_expiry_utc_ms == q.next_known_expiry_utc_ms &&
           xh.has_value() == yh.has_value() &&
           (!xh || (xh->cause == yh->cause && (xh->from_utc_ms == yh->from_utc_ms ||
                                               (xh->from_utc_ms == x.cutover.utc_ms &&
                                                yh->from_utc_ms == y.cutover.utc_ms)))) &&
           std::ranges::equal(p.new_deliveries, q.new_deliveries,
                              [&](const auto &d, const auto &e) {
                                  return same_delivery(d, e, x.cutover, y.cutover);
                              }) &&
           std::ranges::equal(p.stops, q.stops,
                              [&](const auto &d, const auto &e) {
                                  return d.key == e.key &&
                                         same_stop(d.stop, e.stop, x.cutover, y.cutover);
                              }) &&
           std::ranges::equal(
               p.timer_transitions, q.timer_transitions, [](const auto &d, const auto &e) {
                   return same(d.observation, e.observation) && d.transitions == e.transitions;
               });
}

bool breach(const ScheduledPausePreview &p) {
    return p.incomplete && std::holds_alternative<ForecastSupportFailure>(*p.incomplete);
}

// #813's proven breach attests to a change one millisecond after the final
// forecast page. Compare that edge as well as the offsets inside retained facts.
bool contradicts_breach(const ScheduledPausePreview &old, const ScheduledPausePreview &now) {
    const auto &edge = now.facts.back().facts;
    for (const auto &x : old.facts) {
        for (const auto &y : now.facts) {
            if (x.facts.configured_zone == y.facts.configured_zone &&
                x.facts.rules_revision == y.facts.rules_revision && !agree(x.facts, y.facts)) {
                return true;
            }
        }
        const auto &prior = x.facts;
        if (prior.configured_zone == edge.configured_zone &&
            prior.rules_revision == edge.rules_revision &&
            prior.from_utc_ms <= edge.through_utc_ms &&
            prior.through_utc_ms > edge.through_utc_ms) {
            const auto next = transitions_after(prior, edge.through_utc_ms);
            if (next == prior.transitions.end() || next->at_utc_ms != edge.through_utc_ms + 1) {
                return true;
            }
        }
    }
    return false;
}

SchedulePublication publication(const SchedulePreviewRecord &record, bool snapshot_use) {
    const auto &p = record.projection;
    SchedulePublication result{.confirm = {record.issued, {}, record.basis},
                               .checked_state_revision = p.state_revision,
                               .as_of = p.as_of,
                               .through_utc_ms = p.requested_through_utc_ms,
                               .certified_through_utc_ms = p.resolved_through_utc_ms,
                               .incomplete = p.incomplete.has_value(),
                               .upcoming = p.upcoming,
                               .effects = {},
                               .pause_after_cutover = p.pause_after_cutover,
                               .notice = {}};
    for (const auto &effect : p.cutover_effects) {
        result.effects.push_back(
            {effect.delivery.key, effect.delivery.window,
             effect.stop_at_cutover ? std::optional{effect.stop_at_cutover->cause} : std::nullopt});
    }
    // Display captured windows in start order, keeping the projector order for ties.
    std::stable_sort(result.effects.begin(), result.effects.end(),
                     [](const auto &a, const auto &b) {
                         return a.captured.start_utc_ms < b.captured.start_utc_ms;
                     });
    if (p.incomplete) {
        if (!snapshot_use) {
            result.certified_through_utc_ms.reset();
        } else if (const auto *cause = std::get_if<ScheduledPauseCoverageFailure>(&*p.incomplete)) {
            result.notice = ScheduleForecastNotice{
                *cause, std::holds_alternative<ScheduledPauseCoverageFailure>(record.cutover)};
        } else {
            result.notice =
                ScheduleForecastNotice{std::get<ForecastSupportFailure>(*p.incomplete), false};
        }
    }
    return result;
}

bool matches_release(const SchedulePreviewRecord &record, const SchedulePublication &released) {
    const auto expected = publication(record, released.notice.has_value());
    return expected.checked_state_revision == released.checked_state_revision &&
           same(expected.as_of, released.as_of) &&
           expected.through_utc_ms == released.through_utc_ms &&
           expected.certified_through_utc_ms == released.certified_through_utc_ms &&
           expected.incomplete == released.incomplete &&
           expected.notice.has_value() == released.notice.has_value() &&
           (!expected.notice || (expected.notice->cause == released.notice->cause &&
                                 expected.notice->old_side == released.notice->old_side)) &&
           expected.pause_after_cutover.has_value() == released.pause_after_cutover.has_value() &&
           (!expected.pause_after_cutover ||
            same(*expected.pause_after_cutover, *released.pause_after_cutover)) &&
           std::ranges::equal(expected.upcoming, released.upcoming,
                              [](const auto &a, const auto &b) {
                                  return a.key == b.key && a.window == b.window;
                              }) &&
           std::ranges::equal(expected.effects, released.effects, [](const auto &a, const auto &b) {
               return a.key == b.key && a.captured.start_utc_ms == b.captured.start_utc_ms &&
                      a.captured.end_utc_ms == b.captured.end_utc_ms &&
                      a.stop_at_save == b.stop_at_save;
           });
}

ScheduleApply commit(const ScheduleAdmission &a, const ScheduleClock &clock,
                     const ScheduleCandidate &candidate, SchedulePorts &ports) {
    // The caller owns the serialized owner turn through this final validation
    // and the writer's single commit.
    auto active = admit_active(a.request.kind, a.ref, ports);
    if (const auto *refusal = std::get_if<Refusal>(&active)) {
        return *refusal;
    }
    const auto &again = std::get<ScheduleAdmission>(active);
    if (!same_scope(a, again) ||
        a.request.context.session_ref != again.request.context.session_ref ||
        !same_intent(a, again) || !authorized(again, ports, &candidate)) {
        return Refusal::access_denied;
    }
    if (!same_base(a.request.expected, again.request.expected) || !same(a.target, again.target)) {
        return Refusal::stale_observation;
    }
    auto checked = current(again, ports);
    if (const auto *refusal = std::get_if<Refusal>(&checked)) {
        return *refusal;
    }
    const bool entry = std::holds_alternative<ScheduledPauseProgress>(candidate);
    if (!same_cuts(clock, std::get<Current>(checked).clock, entry)) {
        return Refusal::stale_observation;
    }
    return ports.writer.commit(again, candidate);
}
} // namespace

Checked<ScheduleIssuedRef> issue_schedule_preview(const ScheduleCommandRef &ref,
                                                  SchedulePorts &ports) {
    auto admitted = admit_active(Kind::preview, ref, ports);
    if (const auto *refusal = std::get_if<Refusal>(&admitted)) {
        return *refusal;
    }
    auto &a = std::get<ScheduleAdmission>(admitted);
    auto checked = current(a, ports);
    if (const auto *refusal = std::get_if<Refusal>(&checked)) {
        return *refusal;
    }
    auto &[state, clock] = std::get<Current>(checked);
    const auto &edit = std::get<ScheduleEdit>(a.request.intent);
    auto predicted = forecast(state, clock, edit, ports.zones);
    if (const auto *refusal = std::get_if<Refusal>(&predicted)) {
        return *refusal;
    }
    auto plan = cutover(state, clock, edit, ports.zones);
    if (const auto *refusal = std::get_if<Refusal>(&plan)) {
        return *refusal;
    }
    // The command ref identifies the immutable admitted edit. The store binds
    // this basis to each independently issued selector.
    return ports.previews.issue({std::move(a),
                                 std::move(clock),
                                 std::get<ScheduledPausePreview>(std::move(predicted)),
                                 std::get<Cutover>(std::move(plan)),
                                 {},
                                 ref.value});
}

Checked<SchedulePublication> present_schedule_preview(const ScheduleCommandRef &ref,
                                                      const ScheduleIssuedRef &issued,
                                                      SchedulePorts &ports) {
    auto admitted = admit_active(Kind::preview, ref, ports);
    if (const auto *refusal = std::get_if<Refusal>(&admitted)) {
        return *refusal;
    }
    const auto &a = std::get<ScheduleAdmission>(admitted);
    auto checked = current(a, ports);
    if (const auto *refusal = std::get_if<Refusal>(&checked)) {
        return *refusal;
    }
    const auto &[state, clock] = std::get<Current>(checked);
    const auto lookup = ports.previews.issued(a, issued);
    if (std::holds_alternative<ScheduleScopeUnproved>(lookup)) {
        return Refusal::access_denied;
    }
    const auto *record = std::get_if<SchedulePreviewRecord>(&lookup);
    if (!record) {
        return Refusal::fresh_preview_required;
    }
    const auto &edit = std::get<ScheduleEdit>(a.request.intent);
    if (!same_scope(a, record->issuer) || !same(edit.proposed, record->projection.proposed)) {
        return Refusal::access_denied;
    }
    if (record->issuer.ref.value != a.ref.value || record->issued.value != issued.value ||
        !same_base(a.request.expected, record->issuer.request.expected) ||
        edit.through_utc_ms != record->projection.requested_through_utc_ms ||
        !same_cuts(record->clock, clock)) {
        return Refusal::fresh_preview_required;
    }
    const auto predicted = forecast(state, clock, edit, ports.zones);
    const auto *projection = std::get_if<ScheduledPausePreview>(&predicted);
    const auto plan = cutover(state, clock, edit, ports.zones);
    const auto *replayed = std::get_if<Cutover>(&plan);
    const bool snapshot_use =
        record->projection.incomplete && snapshot_allowed(a.request.context, ports, &a.admitted);
    if (!authorized(a, ports)) {
        return Refusal::access_denied;
    }
    if (!projection || !replayed || !same_projection(record->projection, *projection) ||
        !same_cutover(record->cutover, *replayed)) {
        return Refusal::fresh_preview_required;
    }
    return ports.previews.release(a, *record, publication(*record, snapshot_use));
}

ScheduleApply apply_schedule_save(const ScheduleCommandRef &ref, SchedulePorts &ports) {
    auto admitted = admit_active(Kind::save, ref, ports);
    if (const auto *refusal = std::get_if<Refusal>(&admitted)) {
        return *refusal;
    }
    const auto &a = std::get<ScheduleAdmission>(admitted);
    auto checked = current(a, ports);
    if (const auto *refusal = std::get_if<Refusal>(&checked)) {
        return *refusal;
    }
    const auto &[state, clock] = std::get<Current>(checked);
    const auto &[edit, confirm] = std::get<ScheduleSave>(a.request.intent);
    const auto lookup = ports.previews.presented(a, confirm);
    if (std::holds_alternative<ScheduleScopeUnproved>(lookup)) {
        return Refusal::access_denied;
    }
    const auto *shown = std::get_if<SchedulePresentedRecord>(&lookup);
    if (!shown) {
        return Refusal::fresh_preview_required;
    }
    const auto &[record, released] = *shown;
    const auto &original = record.projection;
    if (!same_scope(a, record.issuer) || !same(edit.proposed, original.proposed)) {
        return Refusal::access_denied;
    }
    const auto issued_lookup = ports.previews.issued(a, confirm.issued);
    if (std::holds_alternative<ScheduleScopeUnproved>(issued_lookup)) {
        return Refusal::access_denied;
    }
    const auto *issued = std::get_if<SchedulePreviewRecord>(&issued_lookup);
    if (!confirm.shown || !issued || !same_confirm(confirm, released.confirm) ||
        confirm.issued.value != record.issued.value ||
        confirm.issued.value != issued->issued.value || confirm.basis != record.basis ||
        confirm.basis != issued->basis || record.issuer.ref.value != issued->issuer.ref.value ||
        !same_scope(a, issued->issuer) || !same_cuts(record.clock, issued->clock) ||
        !same_projection(original, issued->projection) ||
        !same_cutover(record.cutover, issued->cutover) ||
        !same_base(a.request.expected, record.issuer.request.expected) ||
        released.checked_state_revision != state.state_revision ||
        released.through_utc_ms != edit.through_utc_ms ||
        original.requested_through_utc_ms != edit.through_utc_ms ||
        !matches_release(record, released) || clock.target.utc_ms < original.as_of.utc_ms ||
        clock.target.utc_ms > edit.through_utc_ms) {
        return Refusal::fresh_preview_required;
    }
    auto projected = cutover(state, clock, edit, ports.zones);
    if (const auto *refusal = std::get_if<Refusal>(&projected)) {
        return *refusal;
    }
    auto &plan = std::get<Cutover>(projected);
    const auto *actual = std::get_if<ScheduledPauseCutover>(&plan);
    if (!actual) {
        return Refusal::cutover_uncertified;
    }
    ScheduleCandidate candidate{*actual};
    if (!authorized(a, ports, &candidate)) {
        return Refusal::access_denied;
    }
    const bool exact_cut = same_cuts(record.clock, clock);
    const auto old_plan = exact_cut ? projected : cutover(state, record.clock, edit, ports.zones);
    if (const auto *refusal = std::get_if<Refusal>(&old_plan)) {
        return *refusal;
    }
    if (!std::holds_alternative<ScheduledPauseCutover>(std::get<Cutover>(old_plan))) {
        return Refusal::cutover_uncertified;
    }
    const auto replayed = forecast(state, record.clock, edit, ports.zones);
    if (const auto *refusal = std::get_if<Refusal>(&replayed)) {
        return *refusal;
    }
    const auto &replay = std::get<ScheduledPausePreview>(replayed);
    if (original.incomplete) {
        return breach(original) && breach(replay) && !contradicts_breach(original, replay)
                   ? Refusal::beyond_supported_timezone_transitions
                   : Refusal::forecast_incomplete;
    }
    if (breach(replay)) {
        return contradicts_breach(original, replay)
                   ? Refusal::forecast_incomplete
                   : Refusal::beyond_supported_timezone_transitions;
    }
    const auto rolled = exact_cut ? replayed : forecast(state, clock, edit, ports.zones);
    if (const auto *refusal = std::get_if<Refusal>(&rolled)) {
        return *refusal;
    }
    const auto &now = std::get<ScheduledPausePreview>(rolled);
    if (breach(now)) {
        return contradicts_breach(original, now) || contradicts_breach(replay, now)
                   ? Refusal::forecast_incomplete
                   : Refusal::beyond_supported_timezone_transitions;
    }
    if (replay.incomplete || now.incomplete ||
        replay.resolved_through_utc_ms != edit.through_utc_ms ||
        now.resolved_through_utc_ms != edit.through_utc_ms) {
        return Refusal::forecast_incomplete;
    }
    if ((clock.target.utc_ms == original.as_of.utc_ms && !exact_cut) ||
        !same_projection(original, replay) ||
        !same_cutover(record.cutover, std::get<Cutover>(old_plan)) ||
        !compatible_facts(replay, now) || !same_effects(replay, now) ||
        !same_cutover(record.cutover, plan)) {
        return Refusal::fresh_preview_required;
    }
    return commit(a, clock, candidate, ports);
}

ScheduleApply apply_schedule_entry(ScheduleCommandKind kind, const ScheduleCommandRef &ref,
                                   SchedulePorts &ports) {
    if (kind != Kind::start && kind != Kind::reopen) {
        return Refusal::not_admitted;
    }
    auto admitted = admit_active(kind, ref, ports);
    if (const auto *refusal = std::get_if<Refusal>(&admitted)) {
        return *refusal;
    }
    const auto &a = std::get<ScheduleAdmission>(admitted);
    auto checked = current(a, ports);
    if (const auto *refusal = std::get_if<Refusal>(&checked)) {
        return *refusal;
    }
    const auto &[state, clock] = std::get<Current>(checked);
    const auto required =
        kind == Kind::start ? core::TimerLifecycle::Ready : core::TimerLifecycle::Finished;
    if (state.timer.lifecycle != required) {
        return Refusal::invalid_input;
    }
    auto entered = enter_scheduled_pauses(state, {state.snapshot_ref, clock.target, *a.entry_timer},
                                          ports.zones);
    if (const auto *refusal = std::get_if<ScheduledPauseRefusal>(&entered)) {
        return translate(*refusal);
    }
    return commit(a, clock, ScheduleCandidate{std::get<ScheduledPauseProgress>(std::move(entered))},
                  ports);
}

std::optional<ScheduledPauseCoverageNotice> disclose_schedule_notice(const ScheduleContext &context,
                                                                     SchedulePorts &ports) {
    if (!snapshot_allowed(context, ports)) {
        return {};
    }
    const auto state = ports.writer.live(context);
    if (!state.cursor.hold) {
        return {};
    }
    return ScheduledPauseCoverageNotice{*state.cursor.hold, state.cursor.last_observation};
}
} // namespace subathon::server
