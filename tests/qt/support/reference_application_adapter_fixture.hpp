// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "reference_intake_fixture.hpp"
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace reference_adapter_fixture {
using namespace reference_intake_fixture;
// Probe obtains a genuine revocable SAME-session view from OLD service, calls the
// selected NEW method directly, then denies. This is not an allowing policy/clock/
// actions substitute. Each isolated port test can reach its own intended NI.
struct Probe final : ExecutionAccess {
    std::function<void(const ApplicationReferenceReadView &, const OriginalApplicationCommand &)> f;
    Witness witness;
    ApplicationReferenceReadView copied;
    AccessDecision check(const ApplicationReferenceReadView &v,
                         const OriginalApplicationCommand &c) override {
        copied = v;
        witness.call([&] { f(v, c); });
        return ApplicationAccessDenied{};
    }
    bool disclose_application(const ApplicationReferenceReadView &,
                              const OriginalApplicationCommand &,
                              const RecordedApplicationBundle &) override {
        throw std::logic_error("Unexpected protected read in isolated probe");
    }
    bool disclose_holds(const ApplicationReferenceReadView &, const OriginalApplicationCommand &,
                        const ApplicationHoldActiveSet &) override {
        throw std::logic_error("Unexpected protected hold read in isolated probe");
    }
};
// Forward ALL actual new ports, capture original exceptions before service/writer
// error mapping. No fabricated port success values or fallback implementation.
struct Ports final : ExecutionAccess,
                     CurrentApplicationPolicy,
                     SegmentedApplicationClock,
                     RequiredApplicationActions,
                     FreshApplicationReferences {
    ReferenceApplicationAdapters &actual;
    Witness witness;
    unsigned checks = 0, policies = 0, clocks = 0, actions = 0, refs = 0;
    explicit Ports(ReferenceApplicationAdapters &a) : actual(a) {}
    AccessDecision check(const ApplicationReferenceReadView &v,
                         const OriginalApplicationCommand &c) override {
        ++checks;
        return witness.call([&] { return actual.check(v, c); });
    }
    bool disclose_application(const ApplicationReferenceReadView &v,
                              const OriginalApplicationCommand &c,
                              const RecordedApplicationBundle &r) override {
        return witness.call([&] { return actual.disclose_application(v, c, r); });
    }
    bool disclose_holds(const ApplicationReferenceReadView &v, const OriginalApplicationCommand &c,
                        const ApplicationHoldActiveSet &r) override {
        return witness.call([&] { return actual.disclose_holds(v, c, r); });
    }
    PolicyResolution resolve(const ApplicationReferenceReadView &v,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                             const AccessDecision &a) override {
        ++policies;
        return witness.call([&] { return actual.resolve(v, c, i, a); });
    }
    ClockResolution observe(const ApplicationReferenceReadView &v,
                            const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                            const BoundApplicationPolicy &p) override {
        ++clocks;
        return witness.call([&] { return actual.observe(v, c, i, p); });
    }
    ActionResolution prepare(const ApplicationReferenceReadView &v,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                             const BoundApplicationPolicy &p,
                             const ApplicationCalculatedProposal &r) override {
        ++actions;
        return witness.call([&] { return actual.prepare(v, c, i, p, r); });
    }
    ReferenceResolution issue(ApplicationReferencePurpose p,
                              const ApplicationReferenceBinding &b) override {
        ++refs;
        return witness.call([&] { return actual.issue(p, b); });
    }
};
// Independently derived observation compared BEFORE any decoder result can become
// journal input. Typed snapshot duplicates are compared in full, not just Main.
inline void same_state(const ReferenceCommittedState &a, const ReferenceCommittedState &b) {
    const auto &x = a.snapshot;
    const auto &y = b.snapshot;
    require(x.token == y.token && x.configuration_ref == y.configuration_ref &&
                x.envelope.format_ref == y.envelope.format_ref &&
                x.envelope.document == y.envelope.document &&
                x.timer.lifecycle == y.timer.lifecycle && x.timer.phase == y.timer.phase &&
                x.timer.main_remaining_ms == y.timer.main_remaining_ms &&
                x.timer.grace_remaining_ms == y.timer.grace_remaining_ms &&
                x.timer.awaiting_confirmation == y.timer.awaiting_confirmation &&
                x.time_debt_ms == y.time_debt_ms && x.balances.size() == y.balances.size() &&
                a.basis == b.basis && a.policy_revision_ref == b.policy_revision_ref,
            "Full independently expected snapshot/basis/policy");
    for (std::size_t n = 0; n < x.balances.size(); ++n)
        require(x.balances[n].kind == y.balances[n].kind &&
                    x.balances[n].target_ref == y.balances[n].target_ref &&
                    x.balances[n].units == y.balances[n].units,
                "Every typed balance");
    const auto &o = a.observation;
    const auto &e = b.observation;
    require(o.clock_lineage_ref == e.clock_lineage_ref && o.observation_ref == e.observation_ref &&
                o.configuration_ref == e.configuration_ref &&
                o.policy_revision_ref == e.policy_revision_ref &&
                o.checked_head_token == e.checked_head_token &&
                o.observed_utc_ms == e.observed_utc_ms && o.segments.size() == e.segments.size(),
            "Derived clock bindings checked against independent expectation");
    for (std::size_t n = 0; n < o.segments.size(); ++n)
        require(o.segments[n].elapsed_ms == e.segments[n].elapsed_ms &&
                    o.segments[n].utc_now_ms == e.segments[n].utc_now_ms &&
                    o.segments[n].paused == e.segments[n].paused,
                "Every clock segment");
}
inline ReferenceCommittedState checked_after(const RecordedApplicationBundle &record,
                                             const ReferenceCommittedState &before,
                                             ReferenceClockSample sample,
                                             unsigned reservation_draw) {
    auto observation = before.observation;
    observation.checked_head_token = before.snapshot.token;
    observation.configuration_ref = before.snapshot.configuration_ref;
    observation.policy_revision_ref = "W";
    observation.observed_utc_ms = sample.utc_ms;
    observation.segments.clear();
    const auto elapsed = sample.monotonic_ms - 100;
    if (elapsed != 0 || sample.utc_ms != 1000) {
        observation.observation_ref = "RCO1" + std::string(32, char(reservation_draw));
        observation.segments = {{elapsed, sample.utc_ms, false}};
    }
    ReferenceCommittedState expected{record.after, before.snapshot.token, observation, "W"};
    const auto decoded = decode_reference_state(record.after);
    same_state(decoded, expected);
    return expected; // Never feed decoder-owned output back into its own setup.
}
struct Rig {
    File file;
    ReferenceCommittedState current;
    ReferenceConfigurationArchive configuration;
    SqliteReferenceAuthoritySource authority;
    Sampler sampler;
    Entropy entropy;
    ReferenceApplicationAdapters adapters;
    Ports ports;
    ApplicationServicePorts service_ports;
    OriginalApplicationService service;
    ReferenceIntake intake;
    explicit Rig(const char *name = "standard", ReferenceRuntimeLimits limits = {},
                 std::int64_t amount = 600000)
        : file(name, true, amount), current(committed(name)), configuration([&] {
              auto v = versions(std::string(name) == "lower_cap" ? 3540000 : 3600000);
              for (auto &version : v) {
                  auto &f = std::get<ReferenceConfigurationFacts>(version.contents);
                  f.projection.rule.add_ms = amount;
                  f.enabled_rules[0].rule.add_ms = amount;
              }
              return v;
          }()),
          adapters(configuration, authority, sampler, entropy,
                   ReferenceClockAnchor{committed(name), {100, 1000}}, limits),
          ports(adapters), service_ports{ports, ports, ports, ports, ports},
          service(file.db.writer, service_ports, limits.service),
          intake(configuration, sampler, entropy, file.db.clock, connector(), &adapters, limits) {}
    ApplicationServiceCompletion attempt(OriginalApplicationCommand c = command(),
                                         std::function<QVariant(QVariant)> tail = {},
                                         bool settle_normal = true) {
        std::optional<ApplicationServiceCompletion> result;
        auto prepared = service.make_work(c, [&](auto value) { result = std::move(value); });
        auto body = std::move(prepared.body);
        const auto head = current;
        unsigned reservation_draw = 0;
        const auto raw = qt_test_support::run_work(
            file.db.writer,
            [&](auto context) -> QVariant {
                // The queue wait is over; sample/reserve now, BEFORE real service body.
                ports.witness.call([&] {
                    const auto refusal = adapters.reserve(head, c);
                    require(!refusal, "Live supported journal reservation");
                    reservation_draw = entropy.calls;
                });
                auto v = body(context);
                return tail ? tail(std::move(v)) : v;
            },
            10000);
        prepared.completion(raw);
        ports.witness.rethrow();
        require(result.has_value(), "Actual matching typed service completion");
        if (result->transaction == TransactionOutcome::committed &&
            result->work_error == WorkError::none && result->response && settle_normal) {
            if (const auto *record =
                    std::get_if<RecordedApplicationBundle>(&result->response->payload)) {
                if (result->response->disposition == ApplicationServiceDisposition::applied) {
                    current =
                        checked_after(*record, head, std::get<ReferenceClockSample>(sampler.next),
                                      reservation_draw);
                    adapters.settle(*result, current);
                } else
                    adapters.settle(*result, {});
            } else
                adapters.settle(*result, {});
        }
        return *result;
    }
    template <class F> void probe(F f) {
        Probe p;
        p.f = f;
        ApplicationServicePorts selected{p, ports, ports, ports, ports};
        OriginalApplicationService selected_service(file.db.writer, selected);
        std::optional<ApplicationServiceCompletion> result;
        require(selected_service.enqueue(command(), [&](auto v) { result = std::move(v); }).error ==
                    StorageError::none,
                "OLD service admits finite-view probe");
        require(QTest::qWaitFor([&] { return result.has_value(); }, 10000), "Probe completes");
        p.witness.rethrow_ni();
        if (p.witness.error)
            std::rethrow_exception(p.witness.error);
        require(result->transaction == TransactionOutcome::committed &&
                    result->work_error == WorkError::none && result->response &&
                    result->response->disposition == ApplicationServiceDisposition::denied,
                "Probe denies after direct selected API; no synthetic allow");
    }
};
// Obtain actual current revision inside the SAME finite view; do not invoke the
// new resolve API in isolated observe/prepare setup, preserving their own NI site.
inline BoundApplicationPolicy policy(Rig &r, const ApplicationReferenceReadView &v,
                                     const OriginalApplicationCommand &c,
                                     const ApplicationBundleInputs &i) {
    auto request_data = request();
    request_data.requested_operation = c.operation;
    const auto result = r.authority.resolve(v, request_data);
    const auto *f = std::get_if<ReferenceAuthorityFacts>(&result.value);
    require(f, "SETUP: OLD real current authority, not placeholder permission revision");
    BoundApplicationPolicy p;
    p.operation = c.operation;
    p.source = {c.subject_kind, c.receipt_id, c.contribution_id};
    p.plan = c.plan;
    p.checked_state_token = i.current_state.token;
    p.configuration_ref = i.current_state.configuration_ref;
    p.permission_revision_ref = f->permission_revision_ref;
    p.policy_revision_ref = "W";
    p.maximum_main_ms = 3600000;
    p.timer_config = {TimerEndMode::Grace, 120000, GraceReuse::Fresh, std::nullopt};
    p.pause_recovery_revision = "reference.fresh-runtime.v1";
    p.explanation = "reference.current-policy.v1";
    p.admission = {ApplicationPolicyAdmission::allow_active, {}};
    return p;
}
inline const RecordedApplicationBundle &applied(const ApplicationServiceCompletion &r) {
    require(r.transaction == TransactionOutcome::committed && r.work_error == WorkError::none &&
                r.response && r.response->disposition == ApplicationServiceDisposition::applied,
            "Actual normal committed applied response required");
    return std::get<RecordedApplicationBundle>(r.response->payload);
}
inline void accounting(const RecordedApplicationBundle &r, std::int64_t requested,
                       std::int64_t actual, std::int64_t capped, std::int64_t main) {
    require(r.effects.size() == 1, "Exactly one effect, no partial prefix");
    const auto &e = r.effects[0];
    require(e.requested_units == requested && e.exact_requested_units == Exact{requested} &&
                e.rounding_residual == Exact{0},
            "Independent exact request/final/residual");
    require(e.components.size() == 3 &&
                e.components[0].kind == ApplicationComponentKind::main_delta &&
                e.components[0].units == actual &&
                e.components[1].kind == ApplicationComponentKind::debt_delta &&
                e.components[1].units == 0 &&
                e.components[2].kind == ApplicationComponentKind::cap_limited_time &&
                e.components[2].units == capped,
            "Literal exact actual/debt/capped components");
    require(r.after.timer.main_remaining_ms == main && r.after.timer.grace_remaining_ms == 120000 &&
                r.after.time_debt_ms == 0 && r.after.balances.empty() &&
                r.after.token != r.intent.expected_state_token && r.jobs.empty(),
            "New coherent after-token, unchanged Grace, no jobs");
}
} // namespace reference_adapter_fixture
