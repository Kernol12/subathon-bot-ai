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
                                         std::function<QVariant(QVariant)> tail = {}) {
        std::optional<ApplicationServiceCompletion> result;
        auto prepared = service.make_work(c, [&](auto value) { result = std::move(value); });
        auto body = std::move(prepared.body);
        const auto head = current;
        const auto raw = qt_test_support::run_work(
            file.db.writer,
            [&](auto context) -> QVariant {
                // The queue wait is over; sample/reserve now, BEFORE real service body.
                ports.witness.call([&] {
                    const auto refusal = adapters.reserve(head, c);
                    require(!refusal, "Live supported journal reservation");
                });
                auto v = body(context);
                return tail ? tail(std::move(v)) : v;
            },
            10000);
        prepared.completion(raw);
        ports.witness.rethrow_ni();
        require(result.has_value(), "Actual matching typed service completion");
        if (result->transaction == TransactionOutcome::committed &&
            result->work_error == WorkError::none && result->response) {
            if (const auto *record =
                    std::get_if<RecordedApplicationBundle>(&result->response->payload)) {
                current = decode_reference_state(record->after);
                adapters.settle(*result, current);
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
inline BoundApplicationPolicy policy(const ApplicationBundleInputs &i) {
    BoundApplicationPolicy p;
    const auto c = command();
    p.operation = c.operation;
    p.source = {c.subject_kind, c.receipt_id, c.contribution_id};
    p.plan = c.plan;
    p.checked_state_token = i.current_state.token;
    p.configuration_ref = i.current_state.configuration_ref;
    p.permission_revision_ref = "independently-replaced-by-real-authority-in-probe";
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
