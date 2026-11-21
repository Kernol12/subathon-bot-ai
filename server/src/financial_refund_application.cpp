// SPDX-License-Identifier: GPL-3.0-or-later
#include "financial_refund_live_p.hpp"

#include "reference_state_internal.hpp"
#include "sqlite_milestone_track_read.hpp"

#include <limits>
#include <tuple>

namespace subathon::server::financial_live {
namespace {

using core::TimerLifecycle;
using financial_sql::corrupt;
namespace wire = reference_runtime_detail;

bool same_timer(const core::TimerState &a, const core::TimerState &b) {
    return std::tie(a.lifecycle, a.phase, a.main_remaining_ms, a.grace_remaining_ms,
                    a.awaiting_confirmation) == std::tie(b.lifecycle, b.phase, b.main_remaining_ms,
                                                         b.grace_remaining_ms,
                                                         b.awaiting_confirmation);
}

bool valid_evidence(const Evidence &e, const ApplicationStateSnapshot &head) {
    const auto &clock = e.clock;
    const auto bounded = milestone_sql::bounded_ref;
    if (!bounded(clock.clock_lineage_ref) || !bounded(clock.observation_ref) ||
        !bounded(clock.policy_revision_ref) || !bounded(e.pause_recovery_revision) ||
        clock.configuration_ref != head.configuration_ref ||
        clock.checked_head_token != head.token || clock.segments.empty() ||
        clock.segments.size() > size_t(financial_sql::collection_limit) ||
        clock.observed_utc_ms != clock.segments.back().utc_now_ms ||
        e.paused != clock.segments.back().paused || (e.cap && *e.cap < 0) ||
        !bundle_detail::enum_in(e.policy, 2) || !bundle_detail::enum_in(e.config.end_mode, 2) ||
        !bundle_detail::enum_in(e.config.grace_reuse, 1) || e.config.grace_duration_ms < 0)
        return false;

    std::int64_t elapsed = 0;
    for (const auto &step : clock.segments) {
        if (step.elapsed_ms < 0 ||
            step.elapsed_ms > std::numeric_limits<std::int64_t>::max() - elapsed)
            return false;
        elapsed += step.elapsed_ms;
    }

    const ReferenceAdapterLimits limits{};
    wire::snapshot_input(head, limits);
    wire::scan_state(head, limits, false);
    wire::StateReader reader{
        {head.envelope.document.constData(), size_t(head.envelope.document.size())}, limits};
    reader.take(4);
    (void)reader.ref();
    (void)reader.opt([&] { return reader.ref(); });
    return reader.ref() == clock.clock_lineage_ref;
}

core::TimerAdvance replay(const core::TimerState &before, const Evidence &e) {
    core::TimerAdvance result{before, {}};
    for (const auto &step : e.clock.segments) {
        const bool deadline_due =
            e.config.hard_end_utc_ms && step.utc_now_ms >= *e.config.hard_end_utc_ms;
        if (!step.elapsed_ms && !deadline_due)
            continue;
        auto step_result = core::advance_timer(result.state, e.config, step);
        result.state = step_result.state;
        result.transitions.insert(result.transitions.end(), step_result.transitions.begin(),
                                  step_result.transitions.end());
    }
    return result;
}

} // namespace

std::optional<Observation> observe(const SqliteWorkSession &session,
                                   const ApplicationStateSnapshot &head,
                                   LiveFinancialRefundInputs &inputs) {
    auto facts = inputs.select(session, head);
    if (!facts || !facts->deficit_policy || facts->recovery_pending)
        return {};

    Evidence evidence{std::move(facts->clock),
                      facts->timer,
                      std::move(facts->pause_recovery_revision_ref),
                      facts->paused,
                      *facts->deficit_policy,
                      facts->main_cap_ms,
                      0};
    try {
        if (!valid_evidence(evidence, head))
            return {};
        auto observed = replay(head.timer, evidence);
        evidence.observation_transitions = observed.transitions.size();
        return Observation{std::move(evidence), observed.state, std::move(observed.transitions),
                           std::move(facts->track), facts->milestone_refund_policy};
    } catch (const ReferenceInputFailure &) {
        return {};
    } catch (const std::invalid_argument &) {
        return {};
    }
}

RefundTimerActual timer_effect(const GroupedRefundEntitlement &time, const core::TimerState &timer,
                               std::int64_t debt, core::RefundDeficitPolicy policy) {
    // Use CORE-005 for Main and new deficit debt. The independently retained
    // restoration leg adds debt even when there is no Main reversal.
    const auto adjustment =
        core::plan_time_reversal({timer.main_remaining_ms, debt},
                                 {time.original.operation.operation_ref,
                                  time.original.operation.operation_ref, time.main_request},
                                 policy);
    if (time.restoration_request >
        std::numeric_limits<std::int64_t>::max() - adjustment.balance_after.time_debt_ms)
        throw std::overflow_error("Financial refund restoration exceeds debt headroom");
    return {-adjustment.main_adjustment_ms, time.restoration_request, adjustment.debt_added_ms,
            -adjustment.unapplied_adjustment_ms, 0};
}

std::optional<std::vector<FinancialMilestoneTarget>>
milestone_crossings(const SqliteWorkSession &session, const Observation &observation,
                    const ApplicationPrincipalBinding &principal, const std::string &configuration,
                    std::int64_t before_score, std::int64_t after_score) {
    namespace ms = milestone_edit_sql;
    const auto &track = observation.track;
    if (track.installation_ref != principal.installation_ref ||
        !ms::reference_fields({track.installation_ref, track.track_ref}) ||
        !observation.milestone_policy || !bundle_detail::enum_in(*observation.milestone_policy, 2))
        return {};

    try {
        auto definition = ms::definition(session, track);
        auto prefix = ms::prefix(session, track);
        if (!definition || !prefix || definition->configuration != configuration)
            return {};

        const auto projected = core::advance_milestone_progression(
            {track.track_ref, definition->revision, prefix->revision, after_score,
             std::move(definition->targets), std::move(prefix->members)},
            {.reference_bytes = 4096});

        // The reducer may propose a suffix, but only the existing committed
        // prefix can supply refund crossings. No new occurrence is staged.
        std::vector<FinancialMilestoneTarget> targets;
        for (qsizetype i = 0; i < prefix->origins.size(); ++i) {
            const auto &target =
                projected.data().projection.completed_deficits[size_t(i)].current_target;
            if (before_score < target.target_micro_points ||
                after_score >= target.target_micro_points)
                continue;
            const auto &origin = prefix->origins[i];
            targets.push_back({{ms::reference(origin[ms::integer(origin[0]) == 0 ? 3 : 5])},
                               target.target_micro_points,
                               *observation.milestone_policy});
        }
        return targets;
    } catch (const SqliteBundleStorageFailure &error) {
        if (error.reason() != SqliteBundleStorageReason::invalid_stored_data)
            throw;
    } catch (const core::MilestoneProgressionFailure &) {
    }
    return {};
}

void bind_envelope(ApplicationStateSnapshot &snapshot, const Evidence &e) {
    const std::optional<std::string_view> basis{e.clock.checked_head_token};
    wire::StateWriter size{financial_sql::document_limit};
    state_codec::write(size, snapshot, basis, e.clock, e.clock.policy_revision_ref);
    ApplicationEnvelope envelope{std::string(wire::state_format), QByteArray("", 0)};
    envelope.document.reserve(qsizetype(size.size));
    wire::StateWriter writer{size.cap, 0, &envelope.document};
    state_codec::write(writer, snapshot, basis, e.clock, e.clock.policy_revision_ref);
    snapshot.envelope = std::move(envelope);
}

void append_effect_transitions(ApplicationStateSnapshot &after, const RefundTimerActual &actual,
                               const Evidence &e, std::vector<core::TimerTransition> &transitions) {
    if (!actual.main_removed || after.timer.main_remaining_ms != 0)
        return;
    auto advanced =
        core::advance_timer(after.timer, e.config, {0, e.clock.observed_utc_ms, e.paused});
    after.timer = advanced.state;
    transitions.insert(transitions.end(), advanced.transitions.begin(), advanced.transitions.end());
}

void write_evidence(financial_sql::Document &document, const Evidence &e) {
    const auto &clock = e.clock;
    document.string(clock.checked_head_token);
    document.string(clock.clock_lineage_ref);
    document.string(clock.observation_ref);
    document.string(clock.configuration_ref);
    document.string(clock.policy_revision_ref);
    document.string(e.pause_recovery_revision);
    document.range(qint64(clock.segments.size()), financial_sql::collection_limit);
    for (const auto &step : clock.segments) {
        document.number(step.elapsed_ms);
        document.number(step.utc_now_ms);
        document.range(step.paused, 1);
    }
    document.number(clock.observed_utc_ms);
    document.range(int(e.config.end_mode), 2);
    document.number(e.config.grace_duration_ms);
    document.range(int(e.config.grace_reuse), 1);
    if (document.range(e.config.hard_end_utc_ms.has_value(), 1))
        document.number(*e.config.hard_end_utc_ms);
    document.range(e.paused, 1);
    document.range(qint64(e.observation_transitions), financial_sql::collection_limit);
    document.range(int(e.policy), 2);
    if (document.range(e.cap.has_value(), 1))
        document.number(*e.cap);
}

std::optional<Evidence> read_evidence(financial_sql::Document &document) {
    if (document.version() == 1)
        return {};
    Evidence e{};
    auto &clock = e.clock;
    clock.checked_head_token = document.string();
    clock.clock_lineage_ref = document.string();
    clock.observation_ref = document.string();
    clock.configuration_ref = document.string();
    clock.policy_revision_ref = document.string();
    e.pause_recovery_revision = document.string();
    const auto count = document.range(0, financial_sql::collection_limit);
    for (qint64 i = 0; i < count; ++i)
        clock.segments.push_back(
            {document.number(), document.number(), bool(document.range(0, 1))});
    clock.observed_utc_ms = document.number();
    e.config.end_mode = core::TimerEndMode(document.range(0, 2));
    e.config.grace_duration_ms = document.number();
    e.config.grace_reuse = core::GraceReuse(document.range(0, 1));
    if (document.range(0, 1))
        e.config.hard_end_utc_ms = document.number();
    e.paused = bool(document.range(0, 1));
    e.observation_transitions = size_t(document.range(0, financial_sql::collection_limit));
    e.policy = core::RefundDeficitPolicy(document.range(0, 2));
    if (document.range(0, 1))
        e.cap = document.number();
    return e;
}

void validate(const Evidence &e, const ApplicationStateSnapshot &before,
              const ApplicationStateSnapshot &advanced, const ApplicationStateSnapshot &after,
              const RefundTimerActual &actual,
              const std::vector<core::TimerTransition> &transitions, bool balances_may_change) {
    try {
        if (!valid_evidence(e, before))
            corrupt();
        auto observation = replay(before.timer, e);
        if (observation.transitions.size() != e.observation_transitions)
            corrupt();

        auto expected_advanced = before;
        if (!same_timer(observation.state, before.timer) || !observation.transitions.empty()) {
            if (advanced.token == before.token)
                corrupt();
            expected_advanced.token = advanced.token;
            expected_advanced.timer = observation.state;
            bind_envelope(expected_advanced, e);
        }
        if (!bundle_detail::same_state(expected_advanced, advanced))
            corrupt();

        auto expected_after = after;
        expected_after.configuration_ref = advanced.configuration_ref;
        if (!balances_may_change)
            expected_after.balances = advanced.balances;
        const auto debt =
            core::Exact(advanced.time_debt_ms) + actual.restored + actual.deficit_debt;
        if (actual.main_removed < 0 || actual.main_removed > advanced.timer.main_remaining_ms ||
            debt < 0 || debt > std::numeric_limits<std::int64_t>::max())
            corrupt();
        expected_after.timer = advanced.timer;
        expected_after.timer.main_remaining_ms -= actual.main_removed;
        expected_after.time_debt_ms = debt.convert_to<std::int64_t>();
        append_effect_transitions(expected_after, actual, e, observation.transitions);
        bind_envelope(expected_after, e);
        if (!bundle_detail::same_state(expected_after, after) ||
            observation.transitions != transitions)
            corrupt();
    } catch (const ReferenceInputFailure &) {
        corrupt();
    } catch (const std::invalid_argument &) {
        corrupt();
    }
}

} // namespace subathon::server::financial_live
