// SPDX-License-Identifier: GPL-3.0-or-later
#include "reference_state_internal.hpp"
#include <chrono>
#include <type_traits>
#include <utility>
#ifdef __linux__
#include <sys/random.h>
#endif
namespace subathon::server {
using namespace reference_runtime_detail;
ReferenceClockSampleResult SystemReferenceClockSampler::sample() {
    const auto steady = std::chrono::steady_clock::now().time_since_epoch();
    const auto utc = std::chrono::system_clock::now().time_since_epoch();
    auto convert = [](auto d) -> std::optional<std::int64_t> {
        using Duration = decltype(d);
        using Ratio = std::ratio_divide<typename Duration::period, std::milli>;
        // The supported clocks use integral submillisecond ticks. Division is
        // overflow-free and duration_cast truncates negative values toward zero.
        if constexpr (!std::is_integral_v<typename Duration::rep> || Ratio::num != 1) {
            return {};
        } else {
            if (!std::in_range<std::int64_t>(d.count()))
                return {};
            using Checked = std::chrono::duration<std::int64_t, typename Duration::period>;
            using Millis = std::chrono::duration<std::int64_t, std::milli>;
            return std::chrono::duration_cast<Millis>(Checked(d.count())).count();
        }
    };
    auto a = convert(steady), b = convert(utc);
    if (!a || !b)
        return ReferenceUnavailable{ReferenceGate::not_ready, "reference.clock.v1"};
    return ReferenceClockSample{*a, *b};
}
ReferenceEntropyRead OsReferenceEntropy::read32() {
    ReferenceEntropyRead r;
#ifdef __linux__
    const auto n = ::getrandom(r.bytes.data(), r.bytes.size(), 0);
    if (n == 32) {
        r.received = 32;
        r.error = ReferenceEntropyError::none;
        return r;
    }
    r.received = n >= 0 ? std::uint64_t(n) : 0;
    r.error = n >= 0 ? ReferenceEntropyError::short_read : ReferenceEntropyError::unavailable;
    volatile std::uint8_t *p = r.bytes.data();
    for (std::size_t i = 0; i < r.bytes.size(); ++i)
        p[i] = 0;
#endif
    return r;
}
ReferenceCommittedState decode_reference_state(const ApplicationStateSnapshot &s,
                                               const ReferenceAdapterLimits &l) {
    snapshot_input(s, l);
    scan_state(s, l);
    return allocate([&] {
        StateReader r{{s.envelope.document.constData(), std::size_t(s.envelope.document.size())},
                      l};
        r.take(4);
        auto clock = [](StateReader &reader) {
            ApplicationClockObservation c;
            c.clock_lineage_ref = reader.reference();
            c.observation_ref = reader.reference();
            c.observed_utc_ms = reader.i64();
            auto n = reader.count(reader.l.value_items);
            for (std::size_t i = 0; i < n; ++i)
                c.segments.push_back({reader.i64(), reader.i64(), bool(reader.u8(1))});
            return c;
        };
        auto m = state_codec::read(r, clock, timer, l.value_items);
        m.clock.configuration_ref = m.configuration;
        m.clock.policy_revision_ref = m.policy;
        m.clock.checked_head_token = m.basis ? *m.basis : s.token;
        return ReferenceCommittedState{s, std::move(m.basis), std::move(m.clock),
                                       std::move(m.policy)};
    });
}
ApplicationStateSnapshot encode_reference_state(const ReferenceCommittedState &v,
                                                const ReferenceAdapterLimits &l) {
    valid_limits(l);
    auto in = measure(l);
    in(v);
    snapshot_input(v.snapshot, l);
    Tags{}(v);
    std::int64_t elapsed = 0;
    for (const auto &step : v.observation.segments) {
        require(step.elapsed_ms >= 0 && step.elapsed_ms <= INT64_MAX - elapsed,
                Error::invalid_value);
        elapsed += step.elapsed_ms;
    }
    require(v.observation.segments.empty() ||
                v.observation.segments.back().utc_now_ms == v.observation.observed_utc_ms,
            Error::invalid_binding);
    require(v.basis || v.observation.segments.empty(), Error::invalid_binding);
    require(!v.policy_revision_ref.empty() && !v.observation.clock_lineage_ref.empty() &&
                !v.observation.observation_ref.empty(),
            Error::invalid_reference);
    require(v.observation.configuration_ref == v.snapshot.configuration_ref &&
                v.observation.policy_revision_ref == v.policy_revision_ref &&
                v.observation.checked_head_token == (v.basis ? *v.basis : v.snapshot.token),
            Error::invalid_binding);
    std::optional<std::string_view> basis;
    if (v.basis)
        basis = *v.basis;
    StateWriter size{std::min(l.document_bytes, std::uint64_t(UINT32_MAX))};
    state_codec::write(size, v.snapshot, basis, v.observation, v.policy_revision_ref);
    auto p = measure(l, Error::result_limit);
    p(v.snapshot.token, state_format, v.snapshot.configuration_ref, v.snapshot.timer,
      v.snapshot.time_debt_ms, v.snapshot.balances);
    p.p.fixed(size.size);
    return allocate([&] {
        auto s = v.snapshot;
        s.envelope.format_ref = state_format;
        s.envelope.document = QByteArray("", 0);
        s.envelope.document.reserve(qsizetype(size.size));
        StateWriter w{size.cap, 0, &s.envelope.document};
        state_codec::write(w, s, basis, v.observation, v.policy_revision_ref);
        scan_state(s, l, false);
        return s;
    });
}
void ReferenceApplicationAdapters::validate_limits() const {
    runtime_limits(limits_);
    require(limits_.pending_reservations < pending_.max_size() &&
                limits_.values.value_items < issued_.max_size() &&
                limits_.values.value_items < spent_.max_size() &&
                limits_.values.clock_journal_entries < observations_.max_size(),
            Error::invalid_limits);
}
ReferenceApplicationAdapters::ReferenceApplicationAdapters(
    ReferenceConfigurationSource &configuration, ReferenceAuthoritySource &authority,
    ReferenceClockSampler &clock, ReferenceEntropy &entropy,
    std::optional<ReferenceClockAnchor> anchor, ReferenceRuntimeLimits limits)
    : configuration_(configuration), authority_(authority), clock_(clock), entropy_(entropy),
      anchor_(std::move(anchor)), limits_(limits) {}
namespace {
ReferenceUnavailable runtime_refusal(const ReferenceRuntimeLimits &limits,
                                     std::string_view reason = "reference.runtime-unavailable.v1",
                                     ReferenceGate gate = ReferenceGate::not_ready,
                                     bool optional = false) {
    auto out = runtime_measure(limits, Error::result_limit);
    if (optional)
        out.p.fixed(1);
    out(gate, reason);
    return {gate, std::string(reason)};
}
ApplicationUnavailable port_refusal(const ReferenceRuntimeLimits &limits,
                                    std::string_view reason = "reference.runtime-unavailable.v1") {
    auto out = service_measure(limits, Error::result_limit);
    out(reason);
    return {std::string(reason)};
}
void command_input(const OriginalApplicationCommand &c, const ReferenceRuntimeLimits &l) {
    runtime_limits(l);
    auto p = runtime_measure(l);
    p(c);
    Tags{}(c);
    require(c.choices.size() <= std::uint64_t(l.service.choices_per_command), Error::input_limit);
    auto nested = l.values;
    nested.reference_bytes =
        std::min(nested.reference_bytes, std::uint64_t(l.service.reference_bytes));
    nested.document_bytes = std::min(nested.document_bytes, std::uint64_t(l.service.choice_bytes));
    nested.value_owned_bytes =
        std::min(nested.value_owned_bytes, std::uint64_t(l.service.input_owned_bytes));
    auto n = measure(nested);
    n(c);
    require(unsigned(c.principal.kind) <= 1 && unsigned(c.subject_kind) <= 1 &&
                unsigned(c.purpose) <= 1 && unsigned(c.preview_mode) <= 1,
            Error::invalid_tag);
    require(!c.operation.operation_scope.empty() && !c.operation.operation_ref.empty() &&
                !c.plan.plan_ref.empty() && !c.principal.installation_ref.empty() &&
                !c.principal.principal_ref.empty(),
            Error::invalid_reference);
    for (const auto &choice : c.choices)
        require(!choice.format_ref.empty(), Error::invalid_reference);
    for (const auto *ref : {&c.preview_state_token, &c.actor_ref, &c.decision_ref})
        if (*ref)
            require(!(*ref)->empty(), Error::invalid_reference);
    for (const auto &choice : c.choices)
        require(!choice.document.isNull(), Error::invalid_shape);
    require((c.subject_kind == ApplicationSubjectKind::contribution) == bool(c.contribution_id),
            Error::invalid_shape);
    require(c.preview_mode != ApplicationPreviewMode::require_exact || bool(c.preview_state_token),
            Error::invalid_shape);
    require(c.receipt_id.value && c.plan.revision, Error::invalid_value);
}
class BoundedAuthority final : public ReferenceAuthoritySource {
    ReferenceAuthoritySource &source_;
    const ReferenceAdapterLimits &limits_;

  public:
    BoundedAuthority(ReferenceAuthoritySource &s, const ReferenceAdapterLimits &l)
        : source_(s), limits_(l) {}
    ReferenceAuthorityResolution resolve(const ApplicationReferenceReadView &v,
                                         const ReferenceAuthorityRequest &q) override {
        auto result = borrow([&] { return source_.resolve(v, q); });
        auto p = projection(limits_, Error::result_limit);
        p(result.value);
        return result;
    }
};
ReferenceAdmissionResolution configuration(ReferenceConfigurationSource &source,
                                           const std::string &ref,
                                           const ReferenceAdapterLimits &l) {
    auto r = borrow([&] { return source.snapshot(ref); });
    return admit_reference_configuration(ref, r, l);
}
} // namespace
std::variant<ReferenceCommittedState, ReferenceUnavailable>
ReferenceApplicationAdapters::current(const ApplicationStateSnapshot &s) const try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    snapshot_input(s, limits_.values);
    thread(owner_);
    if (stopped_ || reconciliation_ || !anchor_)
        return runtime_refusal(limits_);
    auto cfg = configuration(config_source, s.configuration_ref, limits_.values);
    if (auto u = std::get_if<ReferenceUnavailable>(&cfg))
        return runtime_refusal(limits_, u->reason_ref, u->gate);
    if (!active(s) || !bundle_detail::same_state(s, anchor_->committed.snapshot))
        return runtime_refusal(limits_);
    auto p = runtime_measure(limits_, Error::result_limit);
    p(anchor_->committed);
    return anchor_->committed;
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
std::optional<ReferenceUnavailable>
ReferenceApplicationAdapters::reserve(const ReferenceCommittedState &head,
                                      const OriginalApplicationCommand &c,
                                      ReferenceZeroStep mode) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    auto p = runtime_measure(limits_);
    p(head, c, mode);
    Tags{}(head, c, mode);
    require(unsigned(mode) <= 1, Error::invalid_tag);
    // Decode independently; supplied duplicates never rebind the live lineage.
    auto decoded = decode_reference_state(head.snapshot, limits_.values);
    require(same_committed(head, decoded), Error::invalid_binding);
    thread(owner_);
    if (anchor_)
        require(same_committed(head, anchor_->committed), Error::invalid_binding);
    auto live = current(head.snapshot);
    if (auto u = std::get_if<ReferenceUnavailable>(&live))
        return runtime_refusal(limits_, u->reason_ref, u->gate, true);
    if (pending_.size() >= limits_.pending_reservations ||
        observations_.size() + 1 >= limits_.values.clock_journal_entries)
        return runtime_refusal(limits_, "reference.journal-capacity.v1", ReferenceGate::not_ready,
                               true);
    auto journal = runtime_measure(limits_, Error::result_limit);
    journal(observations_);
    journal.p.elements(1);
    journal.p.fixed(36);
    auto candidate = runtime_measure(limits_, Error::result_limit);
    candidate(head.observation.clock_lineage_ref, head.snapshot.configuration_ref,
              head.policy_revision_ref, head.snapshot.token);
    candidate.p.fixed(36 + 8 + 17);
    require(candidate.p.items < candidate.p.item_cap, Error::result_limit);
    if (limits_.values.reference_bytes < 36)
        return runtime_refusal(limits_, "reference.runtime-unavailable.v1",
                               ReferenceGate::not_ready, true);
    if (initial_observation_.empty())
        initial_observation_ = anchor_->committed.observation.observation_ref;
    auto sampled = borrow([&] { return clock_.sample(); });
    auto result = runtime_measure(limits_, Error::result_limit);
    result(sampled);
    if (auto u = std::get_if<ReferenceUnavailable>(&sampled))
        return runtime_refusal(limits_, u->reason_ref, u->gate, true);
    const auto sample = std::get<ReferenceClockSample>(sampled);
    if (sample.monotonic_ms < anchor_->sample.monotonic_ms ||
        (anchor_->sample.monotonic_ms < 0 &&
         sample.monotonic_ms > INT64_MAX + anchor_->sample.monotonic_ms))
        return runtime_refusal(limits_, "reference.runtime-unavailable.v1",
                               ReferenceGate::not_ready, true);
    auto elapsed = sample.monotonic_ms - anchor_->sample.monotonic_ms;
    if (elapsed >= head.snapshot.timer.main_remaining_ms)
        return runtime_refusal(limits_, "reference.runtime-unavailable.v1",
                               ReferenceGate::not_ready, true);
    const bool fresh =
        elapsed || sample.utc_ms != anchor_->sample.utc_ms || mode == ReferenceZeroStep::fresh;
    // Project the entire pending value, including the NEW observation, before
    // cloning head strings or building its segment vector.
    auto pending_size = runtime_measure(limits_, Error::result_limit);
    pending_size(head.observation.clock_lineage_ref);
    if (fresh)
        pending_size.p.fixed(36); // generated RCO1; reference ceiling checked above
    else
        pending_size(head.observation.observation_ref);
    pending_size(head.observation.configuration_ref, head.observation.policy_revision_ref,
                 head.snapshot.token, sample.utc_ms);
    if (fresh) {
        pending_size.p.elements(1);
        pending_size(elapsed, sample.utc_ms, false);
    }
    pending_size(head, c, sample);
    auto e = borrow([&] { return entropy_.read32(); });
    require(unsigned(e.error) <= 2, Error::invalid_tag);
    if (e.error != ReferenceEntropyError::none || e.received != 32)
        return runtime_refusal(limits_, "reference.entropy.v1", ReferenceGate::not_ready, true);
    std::string ref = "RCO1";
    ref.append(reinterpret_cast<const char *>(e.bytes.data()), 32);
    if (ref == initial_observation_ ||
        std::find(observations_.begin(), observations_.end(), ref) != observations_.end())
        return runtime_refusal(limits_, "reference.collision.v1", ReferenceGate::conflict, true);
    auto observation = head.observation;
    observation.checked_head_token = head.snapshot.token;
    observation.segments.clear();
    observation.observed_utc_ms = sample.utc_ms;
    if (fresh) {
        observation.observation_ref = ref;
        observation.segments = {{elapsed, sample.utc_ms, false}};
    }
    auto out = runtime_measure(limits_, Error::result_limit);
    out(observation, head, c, sample);
    observations_.push_back(std::move(ref));
    pending_.push_back({c, head, std::move(observation), sample});
    return {};
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
void ReferenceApplicationAdapters::settle(const ApplicationServiceCompletion &c,
                                          const std::optional<ReferenceCommittedState> &state) try {
    validate_limits();
    auto input = runtime_measure(limits_);
    input(c);
    Tags{}(c);
    input.p.fixed(1);
    if (state)
        input(*state);
    thread(owner_);
    if (c.transaction == TransactionOutcome::outcome_unknown) {
        stopped_ = true;
        return;
    }
    if (c.transaction == TransactionOutcome::committed && c.work_error != WorkError::none) {
        reconciliation_ = true;
        return;
    }
    require(!pending_.empty(), Error::invalid_binding);
    if (c.transaction == TransactionOutcome::committed && c.work_error == WorkError::none &&
        c.response && !c.failure) {
        require(c.response->disposition != ApplicationServiceDisposition::applied ||
                    std::holds_alternative<RecordedApplicationBundle>(c.response->payload),
                Error::invalid_binding);
        if (auto r = std::get_if<RecordedApplicationBundle>(&c.response->payload);
            r && c.response->disposition == ApplicationServiceDisposition::applied) {
            require(state && bundle_detail::same_state(state->snapshot, r->after),
                    Error::invalid_binding);
            auto decoded = decode_reference_state(state->snapshot, limits_.values);
            const auto &pending = pending_.front();
            require(same_committed(*state, decoded) && state->basis &&
                        *state->basis == pending.head.snapshot.token &&
                        equal(state->observation, pending.observation) &&
                        state->policy_revision_ref == pending.observation.policy_revision_ref &&
                        bundle_detail::same_key(r->intent.operation, pending.command.operation) &&
                        r->intent.plan.plan_ref == pending.command.plan.plan_ref &&
                        r->intent.plan.revision == pending.command.plan.revision &&
                        r->intent.receipt_id == pending.command.receipt_id &&
                        r->intent.contribution_id == pending.command.contribution_id &&
                        bundle_detail::same_state(r->persisted_head, pending.head.snapshot) &&
                        r->intent.expected_state_token == pending.head.snapshot.token,
                    Error::invalid_binding);
            auto anchor_size = runtime_measure(limits_, Error::result_limit);
            anchor_size(*state, pending.sample);
            anchor_ = ReferenceClockAnchor{*state, pending_.front().sample};
        }
    } else if (c.transaction != TransactionOutcome::rolled_back) {
        reconciliation_ = true;
        return;
    }
    pending_.erase(pending_.begin());
    issued_.clear();
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
void ReferenceApplicationAdapters::reconcile(const ReferenceCommittedState &s) try {
    validate_limits();
    auto p = runtime_measure(limits_);
    p(s);
    auto decoded = decode_reference_state(s.snapshot, limits_.values);
    require(same_committed(s, decoded), Error::invalid_binding);
    thread(owner_);
    require(!stopped_ && reconciliation_ && anchor_ && !pending_.empty() &&
                s.observation.clock_lineage_ref ==
                    anchor_->committed.observation.clock_lineage_ref &&
                s.basis && *s.basis == pending_.front().head.snapshot.token &&
                s.observation.observation_ref == pending_.front().observation.observation_ref &&
                equal(s.observation, pending_.front().observation) &&
                s.policy_revision_ref == pending_.front().observation.policy_revision_ref,
            Error::invalid_binding);
    auto anchor_size = runtime_measure(limits_, Error::result_limit);
    anchor_size(s, pending_.front().sample);
    anchor_ = ReferenceClockAnchor{s, pending_.front().sample};
    pending_.erase(pending_.begin());
    issued_.clear();
    reconciliation_ = false;
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
AccessDecision ReferenceApplicationAdapters::check(const ApplicationReferenceReadView &v,
                                                   const OriginalApplicationCommand &c) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    thread(owner_);
    BoundedAuthority authority(authority_, limits_.values);
    auto r = check_reference_execution(v, c, config_source, authority, limits_.values,
                                       limits_.authority);
    auto p = service_measure(limits_, Error::result_limit);
    p(r);
    return r;
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
bool ReferenceApplicationAdapters::disclose_application(const ApplicationReferenceReadView &v,
                                                        const OriginalApplicationCommand &c,
                                                        const RecordedApplicationBundle &r) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    auto input = runtime_measure(limits_);
    input(c, r);
    Tags{}(c, r);
    service_input(limits_, r);
    thread(owner_);
    return std::holds_alternative<ApplicationAccessAllowed>(reference_detail::disclose(
        v, c, config_source, authority_, ReferenceAuthorityUse::disclose_application,
        r.intent.operation, {}, limits_.values, limits_.authority, r.intent.choices));
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
bool ReferenceApplicationAdapters::disclose_holds(const ApplicationReferenceReadView &v,
                                                  const OriginalApplicationCommand &c,
                                                  const ApplicationHoldActiveSet &holds) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    auto input = runtime_measure(limits_);
    input(c, holds);
    Tags{}(c, holds);
    service_input(limits_, holds);
    require(!holds.holds.empty(), Error::invalid_shape);
    thread(owner_);
    bool allowed = true;
    for (const auto &h : holds.holds)
        allowed &= std::holds_alternative<ApplicationAccessAllowed>(reference_detail::disclose(
            v, c, config_source, authority_, ReferenceAuthorityUse::disclose_holds,
            h.cause.command.operation, h.cause.hold_ref, limits_.values, limits_.authority,
            h.cause.command.intent));
    return allowed;
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
PolicyResolution ReferenceApplicationAdapters::resolve(const ApplicationReferenceReadView &v,
                                                       const OriginalApplicationCommand &c,
                                                       const ApplicationBundleInputs &i,
                                                       const AccessDecision &access) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    auto input = runtime_measure(limits_);
    input(c, i, access);
    Tags{}(c, i, access);
    service_input(limits_, i, access);
    thread(owner_);
    const auto *a = std::get_if<ApplicationAccessAllowed>(&access);
    if (!a)
        return port_refusal(limits_);
    auto x = reference_detail::captured_binding(v, c, config_source, limits_.values);
    auto captured = config_source.load(v, i.selected_plan.captured.event.arrival.configuration_ref);
    auto before = admit_reference_configuration(
        i.selected_plan.captured.event.arrival.configuration_ref, captured, limits_.values);
    auto now = config_source.load(v, i.current_state.configuration_ref);
    auto current =
        admit_reference_configuration(i.current_state.configuration_ref, now, limits_.values);
    if (std::holds_alternative<ReferenceUnavailable>(before) ||
        std::holds_alternative<ReferenceUnavailable>(current))
        return port_refusal(limits_);
    const auto &old = std::get<ReferenceAdmittedConfiguration>(before).configuration;
    const auto &cfg = std::get<ReferenceAdmittedConfiguration>(current).configuration;
    data(reference_detail::same(x.rule, old.rule) && i.subject_kind == c.subject_kind &&
         i.plan.plan_ref == c.plan.plan_ref && i.plan.revision == c.plan.revision &&
         i.selected_plan.revision == c.plan.revision &&
         i.selected_plan.captured.event.receipt_id == c.receipt_id &&
         i.selected_plan.captured.event.contribution_id == c.contribution_id &&
         i.selected_plan.captured.event.identity == x.source &&
         i.selected_plan.captured.requested_effects.size() == 1 &&
         i.selected_plan.captured.requested_effects[0].requested_units == x.rule.add_ms &&
         i.selected_plan.captured.requested_effects[0].exact_requested_units ==
             core::Exact(x.rule.add_ms));
    if (old.synthetic_origin != cfg.synthetic_origin ||
        old.receipt_namespace != cfg.receipt_namespace || old.rule.rule_ref != cfg.rule.rule_ref ||
        old.rule.action_ref != cfg.rule.action_ref ||
        old.rule.action_version_ref != cfg.rule.action_version_ref ||
        old.rule.timer_ref != cfg.rule.timer_ref ||
        !reference_detail::same(old.rule.automation, cfg.rule.automation) ||
        !reference_detail::same(old.timer_config, cfg.timer_config) || !active(i.current_state))
        return port_refusal(limits_);
    if (std::holds_alternative<ReferenceUnavailable>(this->current(i.current_state)))
        return port_refusal(limits_);
    auto projected = service_measure(limits_, Error::result_limit);
    projected(c.operation, c.subject_kind, c.receipt_id, c.contribution_id, c.plan,
              i.current_state.token, cfg.configuration_ref, a->permission_revision_ref,
              cfg.policy_revision_ref, cfg.maximum_main_ms, cfg.timer_config, false, false,
              std::string_view("reference.fresh-runtime.v1"));
    projected.document_text(std::string_view("reference.current-policy.v1"));
    projected(ApplicationPolicyAdmission::allow_active);
    BoundApplicationPolicy p{c.operation,
                             {c.subject_kind, c.receipt_id, c.contribution_id},
                             c.plan,
                             i.current_state.token,
                             cfg.configuration_ref,
                             a->permission_revision_ref,
                             cfg.policy_revision_ref,
                             cfg.maximum_main_ms,
                             cfg.timer_config,
                             false,
                             false,
                             "reference.fresh-runtime.v1",
                             "reference.current-policy.v1",
                             {ApplicationPolicyAdmission::allow_active, {}},
                             {}};
    auto out = service_measure(limits_, Error::result_limit);
    out(p);
    return p;
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ClockResolution ReferenceApplicationAdapters::observe(const ApplicationReferenceReadView &view,
                                                      const OriginalApplicationCommand &c,
                                                      const ApplicationBundleInputs &i,
                                                      const BoundApplicationPolicy &p) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    auto input = runtime_measure(limits_);
    input(c, i, p);
    Tags{}(c, i, p);
    service_input(limits_, i, p);
    require(bundle_detail::same_key(p.operation, c.operation) &&
                p.source.subject_kind == c.subject_kind && p.source.receipt_id == c.receipt_id &&
                p.source.contribution_id == c.contribution_id &&
                p.checked_state_token == i.current_state.token &&
                p.configuration_ref == i.current_state.configuration_ref &&
                p.plan.plan_ref == c.plan.plan_ref && p.plan.revision == c.plan.revision,
            Error::invalid_binding);
    thread(owner_);
    auto complete = config_source.load(view, i.current_state.configuration_ref);
    auto admitted =
        admit_reference_configuration(i.current_state.configuration_ref, complete, limits_.values);
    if (std::holds_alternative<ReferenceUnavailable>(admitted))
        return port_refusal(limits_);
    for (const auto &r : pending_)
        if (bundle_detail::same_key(r.command.operation, c.operation) &&
            r.head.snapshot.token == i.current_state.token) {
            require(equal(r.command, c) &&
                        bundle_detail::same_state(r.head.snapshot, i.current_state),
                    Error::invalid_binding);
            auto out = service_measure(limits_, Error::result_limit);
            out(r.observation);
            return r.observation;
        }
    return port_refusal(limits_);
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ActionResolution ReferenceApplicationAdapters::prepare(const ApplicationReferenceReadView &v,
                                                       const OriginalApplicationCommand &c,
                                                       const ApplicationBundleInputs &i,
                                                       const BoundApplicationPolicy &p,
                                                       const ApplicationCalculatedProposal &q) try {
    BorrowedConfiguration config_source(configuration_);

    validate_limits();
    command_input(c, limits_);
    auto input = runtime_measure(limits_);
    input(c, i, p, q);
    Tags{}(c, i, p, q);
    service_input(limits_, i, p, q);
    thread(owner_);
    auto cfg = resolve(v, c, i, ApplicationAccessAllowed{p.permission_revision_ref});
    if (!std::holds_alternative<BoundApplicationPolicy>(cfg))
        return port_refusal(limits_);
    require(equal(std::get<BoundApplicationPolicy>(cfg), p), Error::invalid_binding);
    require(q.debt_after >= 0 && q.timer_after.main_remaining_ms >= 0 &&
                q.timer_after.grace_remaining_ms >= 0,
            Error::invalid_value);
    auto binding = reference_detail::captured_binding(v, c, config_source, limits_.values);
    require(q.effects.size() == 1 && q.effects[0].request_ordinal == 1 &&
                q.balances_after.empty() && q.transitions.empty(),
            Error::invalid_shape);
    const auto &effect = q.effects[0];
    require(effect.arithmetic_role == ApplicationArithmeticRole::time_addition &&
                effect.unit == core::RequestedUnit::milliseconds &&
                effect.requested_units == binding.rule.add_ms &&
                effect.exact_requested_units == core::Exact(binding.rule.add_ms) &&
                effect.rounding_residual == 0 && effect.maximum_main_ms == p.maximum_main_ms,
            Error::invalid_binding);
    ApplicationStateSnapshot before = i.current_state;
    before.timer = q.clock_advanced_timer;
    bundle_detail::validate_effect(effect, 1, before, limits_.service.bundles);
    require(
        q.timer_after.lifecycle == q.clock_advanced_timer.lifecycle &&
            q.timer_after.phase == q.clock_advanced_timer.phase &&
            q.timer_after.grace_remaining_ms == q.clock_advanced_timer.grace_remaining_ms &&
            q.timer_after.awaiting_confirmation == q.clock_advanced_timer.awaiting_confirmation &&
            q.debt_after == 0 &&
            effect.components[0].units <= INT64_MAX - q.clock_advanced_timer.main_remaining_ms &&
            q.timer_after.main_remaining_ms ==
                q.clock_advanced_timer.main_remaining_ms + effect.components[0].units,
        Error::invalid_binding);
    auto projected = service_measure(limits_, Error::result_limit);
    projected.document_text(std::string_view("reference.no-external-actions.v1"));
    PreparedApplicationActions out{{}, "reference.no-external-actions.v1"};
    auto result = service_measure(limits_, Error::result_limit);
    result(out);
    return out;
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ReferenceResolution ReferenceApplicationAdapters::issue(ApplicationReferencePurpose purpose,
                                                        const ApplicationReferenceBinding &b) try {
    validate_limits();
    auto p = runtime_measure(limits_);
    p(purpose, b);
    service_input(limits_, purpose, b);
    Tags{}(purpose, b);
    require(unsigned(purpose) <= 4 && unsigned(b.purpose) <= 4 &&
                unsigned(b.source.subject_kind) <= 1,
            Error::invalid_tag);
    require(!b.persisted_head_token.empty() && !b.plan.plan_ref.empty() &&
                !b.operation.operation_ref.empty() && !b.operation.operation_scope.empty(),
            Error::invalid_reference);
    require((b.source.subject_kind == ApplicationSubjectKind::contribution) ==
                bool(b.source.contribution_id),
            Error::invalid_shape);
    require(b.source.receipt_id.value && b.plan.revision &&
                (!b.request_ordinal || *b.request_ordinal > 0),
            Error::invalid_value);
    require(purpose == b.purpose, Error::invalid_binding);
    thread(owner_);
    if (stopped_ || reconciliation_)
        return port_refusal(limits_);
    for (const auto &x : issued_)
        if (x.purpose == purpose && bundle_detail::same_key(x.operation, b.operation) &&
            x.persisted_head_token == b.persisted_head_token &&
            x.request_ordinal == b.request_ordinal && x.plan.plan_ref == b.plan.plan_ref &&
            x.plan.revision == b.plan.revision && x.source.subject_kind == b.source.subject_kind &&
            x.source.receipt_id == b.source.receipt_id &&
            x.source.contribution_id == b.source.contribution_id)
            return port_refusal(limits_, "reference.collision.v1");
    // Both direct issue and reserved issue use this finite process spent budget.
    const auto cap = std::min(limits_.values.value_items, limits_.values.value_owned_bytes / 37);
    if (spent_.size() >= cap || issued_.size() >= cap)
        return port_refusal(limits_, "reference.journal-capacity.v1");
    if (std::min(limits_.values.reference_bytes, std::uint64_t(limits_.service.reference_bytes)) <
        37)
        return port_refusal(limits_);
    auto out = service_measure(limits_, Error::result_limit);
    out(b);
    out.p.fixed(37);
    auto journal = runtime_measure(limits_, Error::result_limit);
    journal(issued_, b);
    journal.p.elements(1);
    auto e = borrow([&] { return entropy_.read32(); });
    require(unsigned(e.error) <= 2, Error::invalid_tag);
    if (e.error != ReferenceEntropyError::none || e.received != 32)
        return port_refusal(limits_, "reference.entropy.v1");
    std::string ref = "RRF1";
    ref.push_back(char(purpose));
    ref.append(reinterpret_cast<const char *>(e.bytes.data()), 32);
    if (std::find(spent_.begin(), spent_.end(), ref) != spent_.end())
        return port_refusal(limits_, "reference.collision.v1");
    spent_.push_back(ref);
    issued_.push_back(b);
    return IssuedApplicationReference{b, std::move(ref)};
} catch (const DependencyAllocation &e) {
    std::rethrow_exception(e.original);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
} // namespace subathon::server
