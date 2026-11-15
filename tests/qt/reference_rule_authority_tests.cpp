// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/reference_rule_fixture.hpp"
#include "support/sqlite_test_support.hpp"
#include <limits>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
using namespace reference_rule_fixture;
namespace {
template <class F> void input_failure(F f, ReferenceInputError code) {
    bool caught = false;
    try {
        f();
    } catch (const ReferenceInputFailure &e) {
        caught = true;
        require(e.code() == code, "Exact input failure");
    }
    require(caught, "Typed failure required; NI does not qualify");
}
void unavailable(const ReferenceAdmissionResolution &r, ReferenceGate gate) {
    const auto *v = std::get_if<ReferenceUnavailable>(&r);
    require(v && v->gate == gate &&
                v->reason_ref == (gate == ReferenceGate::not_ready
                                      ? "reference.configuration-unavailable.v1"
                                      : "reference.configuration-unsupported.v1"),
            "Exact admission refusal");
}
void semantic_failure(Ports &p, Store &d, OriginalApplicationCommand c = command()) {
    const auto completion = p.attempt(d, c, false);
    require(p.witness != nullptr, "A healthy semantic failure must be observed at new helper");
    bool exact = false;
    try {
        std::rethrow_exception(p.witness);
    } catch (const ReferenceExecutionFailure &e) {
        exact = e.code() == ReferenceAuthorityFailure::invalid_data;
    }
    require(exact && completion.transaction == TransactionOutcome::rolled_back &&
                completion.work_error == WorkError::work_exception && !completion.response,
            "Healthy semantic error causes actual whole cleanup; never a fake rolled-back reader");
    require(p.policies == 0 && p.clocks == 0 && p.disclosures == 0,
            "No protected policy/clock/disclosure after failure");
}
void initial_state(Store &d) {
    d.work([&](const auto &s) {
        ApplicationStateSnapshot head{
            "S0",
            {"subathon.internal.application-state.v1",
             QByteArray::fromHex(
                 "5341533100000001430000000012636f6e74726f6c6c65642e6c696e6561676500000013696e69746"
                 "9616c2e6f62736572766174696f6e00000000000003e8000000000000000157010000000000000000"
                 "01000000000001d4c000000000000000000000000000")},
            "C",
            {TimerLifecycle::Active, CountdownPhase::Main, 1, 120000, false},
            0,
            {}};
        require(SqliteApplicationBundleStore(s).retain_initial_state(head) ==
                    SqliteBundleInitialDisposition::inserted,
                "Real initial state only");
    });
}
} // namespace
class ReferenceRuleAuthorityTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void admissionRequiresCompleteOwningFactsAndReturnsDerivedSubset() {
        scenario([] {
            const auto r = admit_reference_configuration("C", configuration_facts());
            const auto *a = std::get_if<ReferenceAdmittedConfiguration>(&r);
            require(a && a->configuration_owner_revision_ref == "owner-revision",
                    "Complete immutable owner revision retained");
            require(encode_reference_configuration(a->configuration).document ==
                        encode_reference_configuration(configuration()).document,
                    "ALL admitted fields retained");
            auto f = configuration_facts();
            f.rules_state = ReferenceFactsState::unknown;
            f.enabled_rules.clear();
            f.projection.other_rules = ReferenceFactsState::unknown;
            unavailable(admit_reference_configuration("C", f), ReferenceGate::not_ready);
        });
    }
    void evaluatesConfiguredIntegerNotPayloadAndPreservesEveryProvenanceField() {
        scenario([] {
            auto a = arrival();
            a.observation.original_payload = QByteArray("{\"award\":99999999}");
            const auto r = evaluate_reference_rule(a, configuration_facts());
            const auto *q = std::get_if<ReferenceRuleEvaluation>(&r);
            require(q && q->selection.selected && q->selection.rule_ref == "R" &&
                        q->selection.version_ref == "r" &&
                        q->selection.explanation == "reference.single-rule.match.v1",
                    "Captured rule selection");
            const RequestedEffect expected{"a",
                                           "b",
                                           "R",
                                           "T",
                                           RequestedUnit::milliseconds,
                                           false,
                                           Exact{600000},
                                           600000,
                                           "reference.integer-ms.v1",
                                           "reference.original-time.v1"};
            require(q->request == expected &&
                        q->request.exact_requested_units - Exact{q->request.requested_units} == 0,
                    "Exact configured integer, no money/rate/modifier history");
            for (const auto amount : {INT64_C(0), INT64_C(9223372036854775807)}) {
                auto f = configuration_facts();
                f.projection.rule.add_ms = f.enabled_rules[0].rule.add_ms = amount;
                a.configuration = f.projection;
                const auto z = evaluate_reference_rule(a, f);
                const auto *e = std::get_if<ReferenceRuleEvaluation>(&z);
                require(e && e->request.requested_units == amount &&
                            numerator(e->request.exact_requested_units) == amount &&
                            denominator(e->request.exact_requested_units) == 1,
                        "Zero/MAX exact without narrowing");
            }
        });
    }
    void zeroMultipleRulesAndEveryUnsupportedShapeRefuseKnownConfiguration() {
        scenario([] {
            for (int shape = 0; shape < 14; ++shape) {
                auto f = configuration_facts();
                switch (shape) {
                case 0:
                    f.enabled_rules.clear();
                    break;
                case 1: {
                    auto r = f.enabled_rules[0];
                    r.rule.rule_ref = "R2";
                    f.enabled_rules.push_back(r);
                    break;
                }
                case 2:
                    f.enabled_rules[0].trigger_types = {"other"};
                    break;
                case 3:
                    f.enabled_rules[0].action_kinds = {"other"};
                    break;
                case 4:
                    f.enabled_rules[0].action_units = {"other"};
                    break;
                case 5:
                    f.enabled_rules[0].conditions = {"condition"};
                    break;
                case 6:
                    f.enabled_rules[0].exclusions = {"exclude"};
                    break;
                case 7:
                    f.enabled_rules[0].execution_limits = {"limit"};
                    break;
                case 8:
                    f.modifiers.enabled_refs = {"modifier"};
                    break;
                case 9:
                    f.milestones.enabled_refs = {"milestone"};
                    break;
                case 10:
                    f.outbound_actions.enabled_refs = {"action"};
                    break;
                case 11:
                    f.generated_events.enabled_refs = {"event"};
                    break;
                case 12:
                    f.projection.timer_config.hard_end_utc_ms = 1234;
                    break;
                case 13:
                    f.projection.synthetic_origin.provider = "known.other-provider";
                    break;
                }
                unavailable(admit_reference_configuration("C", f),
                            ReferenceGate::unsupported_configuration);
            }
        });
    }
    void unknownFactsContradictionsDuplicatesAndLaterMalformedMembersKeepPriority() {
        scenario([] {
            for (int field = 0; field < 4; ++field) {
                auto f = configuration_facts();
                auto &category = field == 0   ? f.modifiers
                                 : field == 1 ? f.milestones
                                 : field == 2 ? f.outbound_actions
                                              : f.generated_events;
                auto &tag = field == 0   ? f.projection.modifiers
                            : field == 1 ? f.projection.milestones
                            : field == 2 ? f.projection.outbound_actions
                                         : f.projection.generated_events;
                category.state = tag = ReferenceFactsState::unknown;
                unavailable(admit_reference_configuration("C", f), ReferenceGate::not_ready);
                tag = ReferenceFactsState::complete;
                input_failure([&] { (void)admit_reference_configuration("C", f); },
                              ReferenceInputError::invalid_binding);
                tag = ReferenceFactsState::unknown;
                category.enabled_refs = {"present"};
                input_failure([&] { (void)admit_reference_configuration("C", f); },
                              ReferenceInputError::invalid_shape);
            }
            auto partial = configuration_facts();
            partial.rules_state = partial.projection.other_rules = ReferenceFactsState::unknown;
            input_failure([&] { (void)admit_reference_configuration("C", partial); },
                          ReferenceInputError::invalid_shape);
            auto f = configuration_facts();
            f.enabled_rules.push_back(f.enabled_rules[0]);
            input_failure([&] { (void)admit_reference_configuration("C", f); },
                          ReferenceInputError::invalid_shape);
            f = configuration_facts();
            f.modifiers.enabled_refs = {"m", "m"};
            input_failure([&] { (void)admit_reference_configuration("C", f); },
                          ReferenceInputError::invalid_shape);
            f = configuration_facts();
            f.enabled_rules[0].action_units = {"other"};
            auto later = f.enabled_rules[0];
            later.rule.rule_ref = "later";
            later.rule.add_ms = -1;
            f.enabled_rules.push_back(later);
            input_failure([&] { (void)admit_reference_configuration("C", f); },
                          ReferenceInputError::invalid_value);
            f.enabled_rules.back().rule.add_ms = 0;
            f.enabled_rules.back().rule.timer_ref.clear();
            input_failure([&] { (void)admit_reference_configuration("C", f); },
                          ReferenceInputError::invalid_reference);
            f.enabled_rules.back().rule.timer_ref = "T";
            f.generated_events.state = static_cast<ReferenceFactsState>(99);
            input_failure([&] { (void)admit_reference_configuration("C", f); },
                          ReferenceInputError::invalid_tag);
        });
    }
    void malformedValuesRefsTagsAndSourceRefusalsHaveExactOutcomes() {
        scenario([] {
            for (int field = 0; field < 10; ++field) {
                auto f = configuration_facts();
                auto code = ReferenceInputError::invalid_binding;
                switch (field) {
                case 0:
                    f.configuration_ref = "wrong";
                    break;
                case 1:
                    f.projection.configuration_ref = "wrong";
                    break;
                case 2:
                    f.configuration_owner_revision_ref.clear();
                    code = ReferenceInputError::invalid_reference;
                    break;
                case 3:
                    f.enabled_rules[0].rule.action_version_ref = "other";
                    break;
                case 4:
                    f.projection.maximum_main_ms = -1;
                    code = ReferenceInputError::invalid_value;
                    break;
                case 5:
                    f.projection.rule.add_ms = -1;
                    code = ReferenceInputError::invalid_value;
                    break;
                case 6:
                    f.projection.timer_config.grace_duration_ms = -1;
                    code = ReferenceInputError::invalid_value;
                    break;
                case 7:
                    f.projection.timer_config.end_mode = static_cast<TimerEndMode>(9);
                    code = ReferenceInputError::invalid_tag;
                    break;
                case 8:
                    f.enabled_rules[0].trigger_types = {""};
                    code = ReferenceInputError::invalid_reference;
                    break;
                case 9:
                    f.projection.rule.automation.kind = static_cast<ApplicationPrincipalKind>(9);
                    code = ReferenceInputError::invalid_tag;
                    break;
                }
                input_failure([&] { (void)admit_reference_configuration("C", f); }, code);
            }
            const ReferenceUnavailable u{ReferenceGate::not_ready, "specific.unavailable"};
            const auto r = admit_reference_configuration("C", u);
            require(std::get<ReferenceUnavailable>(r).reason_ref == u.reason_ref,
                    "Preserve exact source refusal");
            input_failure(
                [&] {
                    (void)admit_reference_configuration(
                        "C", ReferenceUnavailable{ReferenceGate::denied, "wrong-channel"});
                },
                ReferenceInputError::invalid_shape);
        });
    }
    void retainedArrivalRequiresWholeConfigEqualityIncludingUnselectedFields() {
        scenario([] {
            // Sixteen distinct value changes plus the five category tags below.
            for (int field = 0; field < 16; ++field) {
                auto f = configuration_facts();
                auto &c = f.projection;
                switch (field) {
                case 0:
                    c.policy_revision_ref = "W2";
                    break;
                case 1:
                    c.rule.rule_version_ref = "r2";
                    c.rule.add_ms = 900000;
                    break;
                case 2:
                    c.maximum_main_ms = 4000000;
                    break;
                case 3:
                    c.timer_config.grace_duration_ms = 1;
                    break;
                case 4:
                    c.timer_config.grace_reuse = GraceReuse::Shared;
                    break;
                case 5:
                    c.timer_config.end_mode = TimerEndMode::Confirmation;
                    break;
                case 6:
                    c.receipt_namespace = "N2";
                    break;
                case 7:
                    c.synthetic_origin.source_account = "A2";
                    break;
                case 8:
                    c.rule.automation_capability_ref = "K2";
                    break;
                case 9:
                    c.rule.action_version_ref = "b2";
                    break;
                case 10:
                    c.rule.rule_ref = "R2";
                    break;
                case 11:
                    c.rule.action_ref = "a2";
                    break;
                case 12:
                    c.rule.timer_ref = "T2";
                    break;
                case 13:
                    c.rule.add_ms = 1;
                    break;
                case 14:
                    c.rule.automation.installation_ref = "I2";
                    break;
                case 15:
                    c.rule.automation.principal_ref = "U2";
                    break;
                }
                f.enabled_rules[0].rule = c.rule;
                input_failure([&] { (void)evaluate_reference_rule(arrival(), f); },
                              ReferenceInputError::invalid_binding);
            }
            for (int tag = 0; tag < 5; ++tag) {
                auto a = arrival();
                auto &c = a.configuration;
                (tag == 0   ? c.other_rules
                 : tag == 1 ? c.modifiers
                 : tag == 2 ? c.milestones
                 : tag == 3 ? c.outbound_actions
                            : c.generated_events) = ReferenceFactsState::unknown;
                input_failure([&] { (void)evaluate_reference_rule(a, configuration_facts()); },
                              ReferenceInputError::invalid_binding);
            }
            auto a = arrival();
            a.observation.original_payload = QByteArray{};
            input_failure([&] { (void)evaluate_reference_rule(a, configuration_facts()); },
                          ReferenceInputError::invalid_shape);
        });
    }
    void wholeInputByteItemAndPerFieldBoundsIncludeUnsupportedMembers() {
        scenario([] {
            // From declared fields: projection60; rule fact57; owner14+ref1,
            // rules tag1 + four category tags4 => facts137, expected ref1 =>138;
            // I=one rule +3 shape strings=4. No serialization/implementation oracle.
            ReferenceAdapterLimits l;
            l.value_owned_bytes = 138;
            l.value_items = 4;
            require(std::holds_alternative<ReferenceAdmittedConfiguration>(
                        admit_reference_configuration("C", configuration_facts(), l)),
                    "Full input exact138/4 and output74/0 fit");
            l.value_owned_bytes = 137;
            input_failure(
                [&] { (void)admit_reference_configuration("C", configuration_facts(), l); },
                ReferenceInputError::input_limit);
            l.value_owned_bytes = 138;
            l.value_items = 3;
            input_failure(
                [&] { (void)admit_reference_configuration("C", configuration_facts(), l); },
                ReferenceInputError::input_limit);
            l = {};
            auto f = configuration_facts();
            f.enabled_rules[0].action_units = {"other"};
            f.generated_events.enabled_refs = {std::string(4096, 'g')};
            unavailable(admit_reference_configuration("C", f, l),
                        ReferenceGate::unsupported_configuration);
            f.generated_events.enabled_refs[0].push_back('g');
            input_failure([&] { (void)admit_reference_configuration("C", f, l); },
                          ReferenceInputError::input_limit);
            l.value_items = 0;
            input_failure([&] { (void)admit_reference_configuration("C", f, l); },
                          ReferenceInputError::invalid_limits);
        });
    }
    void genuineLifecycleCurrentRevokeRestoreAndCapturedDataStayImmutable() {
        scenario([] {
            Store d;
            Observer o(d.path);
            const auto before = captures(o);
            Ports p;
            p.attempt(d);
            allowed(p);
            const auto first =
                std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref;
            require(p.source.observed->permission_revision_ref == permission_revision() &&
                        p.source.observed->automation_revision_ref == automation_revision(),
                    "Independent revision preimages for authenticated lifecycle at head3");
            require(p.config.loaded == std::vector<std::string>{"C"},
                    "Captured version loaded through callback");
            d.state(3, ReferenceAuthorityState::revoked);
            p.attempt(d);
            require(p.decision && std::holds_alternative<ApplicationAccessDenied>(*p.decision),
                    "Fresh revoke denies previous allow");
            d.state(4, ReferenceAuthorityState::authorized);
            p.attempt(d);
            allowed(p);
            require(std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref ==
                            permission_revision(1, 5, 5) &&
                        std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref !=
                            first,
                    "Fresh restored exact revision5");
            require(p.source.calls == 1 && captures(o) == before,
                    "Each attempt resolves anew without a capture rewrite");
        });
    }
    void fullQuantityUsesAuthorBothChannelsAndSeparateExecutionSubject() {
        scenario([] {
            for (int channel = 0; channel < 3; ++channel) {
                Store d;
                Ports p;
                p.attempt(d);
                allowed(p);
                const auto old =
                    std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref;
                d.narrow(channel == 1 ? 1 : 0, channel == 2 ? "E" : "A", 120000);
                p.full_accounting = true;
                const auto c = p.attempt(d);
                require(p.decision &&
                            std::holds_alternative<ApplicationAccessDenied>(*p.decision) &&
                            c.response &&
                            c.response->disposition == ApplicationServiceDisposition::denied,
                        "Full600000 exceeds120000 authority before any policy/cap lookup");
                require(p.policies == 0 && p.clocks == 0 && p.disclosures == 0,
                        "Nonallow precedes protected inputs/policy/clock");
                d.narrow(channel == 1 ? 1 : 0, channel == 2 ? "E" : "A", 600000);
                p.full_accounting = false;
                p.attempt(d);
                allowed(p);
                require(std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref !=
                            old,
                        "Restoration requires fresh graph revision");
            }
        });
    }
    void distrustsEveryDuplicatedReturnedBindingAndRawPolicyIdentity() {
        scenario([] {
            // Independent one-field corruptions after ACTUAL source resolution.
            // These demonstrate validation, not source currentness.
            const std::vector<std::function<void(ReferenceAuthorityFacts &)>> faults{
                [](auto &f) { f.binding.use = ReferenceAuthorityUse::disclose_application; },
                [](auto &f) { f.binding.principal.installation_ref = "other"; },
                [](auto &f) { f.binding.captured.installation_ref = "other"; },
                [](auto &f) { f.binding.captured.execution_context_ref = "other"; },
                [](auto &f) { f.binding.captured.arrival_context_ref = "other"; },
                [](auto &f) { f.binding.captured.plan.plan_ref = "other"; },
                [](auto &f) { f.binding.captured.source.origin.provider = "other"; },
                [](auto &f) { f.binding.captured.source.identifier_namespace = "other"; },
                [](auto &f) { f.binding.captured.rule.add_ms = 120000; },
                [](auto &f) { f.binding.captured.rule.automation_capability_ref = "other"; },
                [](auto &f) { f.binding.captured.rule.rule_ref = "other"; },
                [](auto &f) { f.binding.captured.rule.action_ref = "other"; },
                [](auto &f) { f.binding.captured.rule.timer_ref = "other"; },
                [](auto &f) { f.binding.captured.source.origin.source_account = "other"; },
                [](auto &f) { f.binding.principal.kind = ApplicationPrincipalKind::account; },
                [](auto &f) { f.binding.historical_operation = ApplicationOperationKey{"s", "a"}; },
                [](auto &f) { f.binding.historical_hold = "h"; },
                [](auto &f) { f.automation_revision_ref.clear(); },
                [](auto &f) { f.automation_revision_ref.back() ^= 1; },
                [](auto &f) { f.registry.created_revision = f.registry.state_revision + 1; },
                [](auto &f) { f.registry.state = static_cast<ReferenceAuthorityState>(255); },
                [](auto &f) { f.permission_input.owner.state = PermissionOwnerState::unknown; },
                [](auto &f) { f.registry.binding.rule_version = "other"; },
                [](auto &f) { f.registry.binding.action_version = "other"; },
                [](auto &f) { f.registry.binding.origin.provider = "other"; },
                [](auto &f) { f.registry.created_revision = 0; },
                [](auto &f) { f.permission_input.owner.account = "E"; },
                [](auto &f) { f.original_author_input.catalog.revision = "other"; },
                [](auto &f) {
                    f.permission_input.request.operation = "reference.application-read.v1";
                },
                [](auto &f) { f.original_author_input.actions.clear(); },

                [](auto &f) { f.binding.requested_operation.operation_ref = "other"; },
                [](auto &f) { f.binding.principal.principal_ref = "other"; },
                [](auto &f) { f.binding.captured.receipt_id = {8}; },
                [](auto &f) { f.binding.captured.contribution_id = {1}; },
                [](auto &f) { f.binding.captured.plan.revision = 2; },
                [](auto &f) { f.binding.captured.source.opaque_id = "other"; },
                [](auto &f) { f.binding.captured.rule.action_version_ref = "other"; },
                [](auto &f) { f.binding.captured.rule.rule_version_ref = "other"; },
                [](auto &f) { f.registry.binding.installation = "other"; },
                [](auto &f) { f.registry.binding.capability = "other"; },
                [](auto &f) { f.registry.binding.principal = "other"; },
                [](auto &f) { f.registry.binding.rule = "other"; },
                [](auto &f) { f.registry.binding.action = "other"; },
                [](auto &f) { f.registry.binding.timer = "other"; },
                [](auto &f) { f.registry.binding.origin.source_account = "other"; },
                [](auto &f) { f.registry.binding.identifier_namespace = "other"; },
                [](auto &f) { f.registry.binding.requested_ms = 120000; },
                [](auto &f) { f.registry.original_author = "O"; },
                [](auto &f) { f.registry.execution_account = "O"; },
                [](auto &f) { f.registry.state_revision = f.registry.head_revision + 1; },
                [](auto &f) { f.original_author_input.author = "U"; },
                [](auto &f) { f.permission_input.actor = "U"; },
                [](auto &f) { f.permission_input.snapshot = "stale"; },
                [](auto &f) { f.original_author_input.snapshot = "stale"; },
                [](auto &f) { f.original_author_input.owner.account = "E"; },
                [](auto &f) { f.permission_input.catalog.revision = "other"; },
                [](auto &f) {
                    f.original_author_input.catalog.operations[0].classification =
                        PermissionOperationClass::protected_access_management;
                },
                [](auto &f) {
                    f.permission_input.request.quantities[0].value.numerator = "120000";
                },
                [](auto &f) {
                    f.original_author_input.actions[0].quantities[0].upper =
                        PermissionRational{"120000", "1"};
                },
                [](auto &f) { f.permission_input.request.resources[2].resource = "other"; },
                [](auto &f) {
                    f.original_author_input.actions[0].resources[1].allowed = {"other"};
                }};
            for (const auto &fault : faults) {
                Store d;
                Ports p;
                p.source.corrupt_facts = fault;
                semantic_failure(p, d);
            }
        });
    }
    void sharedAuthorAndExecutionAccountUsesRealCurrentAuthority() {
        scenario([] {
            Store d(600000, "A");
            Ports p;
            p.attempt(d);
            allowed(p, "A");
            require(p.source.calls == 1 && p.source.observed->registry.original_author == "A" &&
                        p.source.observed->registry.execution_account == "A",
                    "Real enrollment and consumer allow author=subject; principal stays U");
        });
    }
    void inactiveRegistryCannotUseNominalFactsSuccess() {
        scenario([] {
            for (const auto state :
                 {ReferenceAuthorityState::pending, ReferenceAuthorityState::revoked}) {
                Store d;
                Ports p;
                p.source.corrupt_facts = [state](auto &f) { f.registry.state = state; };
                p.attempt(d);
                require(p.decision &&
                            std::holds_alternative<ApplicationAccessDenied>(*p.decision) &&
                            !p.witness && p.policies == 0 && p.clocks == 0 && p.disclosures == 0,
                        "Known inactive state denies before fingerprint/policy re-evaluation");
            }
        });
    }
    void independentlyReevaluatesRawAuthorUseGrantAndSubjectUse() {
        scenario([] {
            for (int channel = 0; channel < 3; ++channel) {
                Store d;
                Ports p;
                p.source.corrupt_facts = [channel](auto &f) {
                    auto &policy = channel == 0   ? f.original_author_input.use
                                   : channel == 1 ? f.original_author_input.delegation
                                                  : f.permission_input.policy;
                    policy.entries[0].quantities[0].upper = PermissionRational{"120000", "1"};
                };
                p.attempt(d);
                require(p.decision && std::holds_alternative<ApplicationAccessDenied>(*p.decision),
                        "Nominal Facts variant cannot bypass raw policy re-evaluation");
            }
        });
    }
    void sourceRefusalAndSemanticFailureAreDistinctAndCleanupIsReal() {
        scenario([] {
            for (const auto f :
                 {ReferenceAuthorityFailure::denied, ReferenceAuthorityFailure::missing,
                  ReferenceAuthorityFailure::unavailable,
                  ReferenceAuthorityFailure::resource_limit}) {
                Store d;
                Ports p;
                p.source.refuse = f;
                p.attempt(d);
                require(p.decision &&
                            (f == ReferenceAuthorityFailure::denied
                                 ? std::holds_alternative<ApplicationAccessDenied>(*p.decision)
                                 : std::holds_alternative<ApplicationAccessUnknown>(*p.decision)),
                        "Exact nonallow mapping");
                require(p.policies == 0 && p.clocks == 0 && p.disclosures == 0,
                        "No protected continuation");
            }
            Store d;
            Observer o(d.path);
            const auto before = captures(o);
            Ports p;
            for (const auto f :
                 {ReferenceAuthorityFailure::invalid_data, ReferenceAuthorityFailure::stale,
                  ReferenceAuthorityFailure::collision,
                  static_cast<ReferenceAuthorityFailure>(255)}) {
                p.source.refuse = f;
                semantic_failure(p, d);
            }
            require(captures(o) == before &&
                        o.rows("SELECT * FROM subathon_app_operations").isEmpty(),
                    "Actual whole rollback, no completion/claim");
            // Follow-up healthy callback proves the service did not poison writer.
            p.source.refuse.reset();
            p.attempt(d);
            allowed(p);
        });
    }
    void commandBindingsAndMalformedStoredCaptureNeverReachAuthority() {
        scenario([] {
            for (int field = 0; field < 6; ++field) {
                Store d;
                Ports p;
                auto c = command();
                switch (field) {
                case 0:
                    c.principal.principal_ref = "wrong";
                    break;
                case 1:
                    c.receipt_id = {8};
                    break;
                case 2:
                    c.contribution_id = ContributionId{1};
                    break;
                case 3:
                    c.plan.revision = 2;
                    break;
                case 4:
                    c.plan.plan_ref = "other";
                    break;
                case 5:
                    c.choices[0].document.append('x');
                    break;
                }
                semantic_failure(p, d, c);
                require(p.source.calls == 0, "Binding validation before source");
            }
            Store d;
            Ports p;
            d.work([](const auto &s) {
                (void)s.execute({"UPDATE subathon_retained_contexts SET document=X'5352583100' "
                                 "WHERE reference=X'59'",
                                 {}});
            });
            semantic_failure(p, d);
            require(p.source.calls == 0, "Known malformed SRX1 is semantic corruption");
        });
    }
    void emptyRevisionIsMalformedAndOversizedFactsAreNotClonedOrEvaluated() {
        scenario([] {
            Store d;
            Ports p;
            p.source.corrupt_facts = [](auto &f) { f.permission_revision_ref.clear(); };
            semantic_failure(p, d);
            p.source.corrupt_facts = [](auto &f) {
                f.permission_input.policy.entries[0].source = std::string(4097, 'x');
            };
            // Complete raw authority result per-field preflight is a semantic resource
            // refusal (Unknown), not a tiny requested tuple bypass or allocation clone.
            p.attempt(d);
            require(p.decision && std::holds_alternative<ApplicationAccessUnknown>(*p.decision),
                    "Oversized source result fails closed");
        });
    }
    void actualAuthorityWholeResultLimitsAndCapturedVersionAreEnforced() {
        scenario([] {
            Store d;
            Observer o(d.path);
            const auto before = captures(o);
            Ports p;
            // Existing public authority declaration gives this complete Facts DATA
            // B2440/I131. Includes both catalogs/policies/snapshots and full binding.
            p.authority_limits.result_bytes = 2440;
            p.authority_limits.result_items = 131;
            p.attempt(d);
            allowed(p);
            p.authority_limits.result_bytes = 2439;
            p.attempt(d);
            require(p.decision && std::holds_alternative<ApplicationAccessUnknown>(*p.decision),
                    "One-less whole result byte limit");
            p.authority_limits.result_bytes = 2440;
            p.authority_limits.result_items = 130;
            p.attempt(d);
            require(p.decision && std::holds_alternative<ApplicationAccessUnknown>(*p.decision),
                    "One-less whole result item limit");
            p.authority_limits = {};
            auto r2 = configuration_facts();
            r2.configuration_ref = r2.projection.configuration_ref = "C2";
            r2.projection.rule.rule_version_ref = r2.enabled_rules[0].rule.rule_version_ref = "r2";
            r2.projection.rule.add_ms = r2.enabled_rules[0].rule.add_ms = 900000;
            p.config.data = r2;
            semantic_failure(p, d);
            require(p.source.calls == 0 && p.config.loaded == std::vector<std::string>{"C"} &&
                        captures(o) == before,
                    "Request capturedC, never silently substitute currentC2/r2");
        });
    }
    void retainedPayloadBoundaryIsAuditOnlyAndCannotHideMalformedFacts() {
        scenario([] {
            auto a = arrival();
            a.observation.original_payload = QByteArray(65536, 'p');
            auto r = evaluate_reference_rule(a, configuration_facts());
            require(std::holds_alternative<ReferenceRuleEvaluation>(r) &&
                        std::get<ReferenceRuleEvaluation>(r).request.requested_units == 600000,
                    "Exact payload bound fits, still configured amount");
            a.observation.original_payload.append('p');
            input_failure([&] { (void)evaluate_reference_rule(a, configuration_facts()); },
                          ReferenceInputError::input_limit);
        });
    }
    void callbackDefaultExpiredForeignAndSqlFailureKeepOriginalTypes() {
        scenario([] {
            ConfigData config;
            Source source;
            bool caught = false;
            try {
                (void)check_reference_execution({}, command(), config, source);
            } catch (const ApplicationReferenceViewFailure &e) {
                caught = e.error() == ApplicationReferenceViewError::expired_callback;
            }
            require(caught, "Default capability cannot acquire session");
            Store d;
            Ports p;
            p.attempt(d);
            allowed(p);
            caught = false;
            try {
                (void)check_reference_execution(p.retained, command(), config, source);
            } catch (const ApplicationReferenceViewFailure &e) {
                caught = e.error() == ApplicationReferenceViewError::expired_callback;
            }
            require(caught, "Copied expired view remains revoked");
            p.before = [&](const auto &v, const auto &c) {
                auto job = std::async(std::launch::async, [&, v, c] {
                    bool wrong = false;
                    try {
                        (void)check_reference_execution(v, c, config, source);
                    } catch (const ApplicationReferenceViewFailure &e) {
                        wrong = e.error() == ApplicationReferenceViewError::wrong_thread;
                    }
                    require(wrong, "Foreign thread preserves exact callback failure");
                });
                job.get();
            };
            p.attempt(d);
            allowed(p);
        });
    }
    void currentAccountDisableMembershipAndRevisionFencesAreFresh() {
        scenario([] {
            for (const auto *account : {"A", "E"}) {
                Store d;
                Observer o(d.path);
                const auto before = captures(o);
                Ports p;
                p.attempt(d);
                allowed(p);
                const auto original =
                    std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref;
                d.work([&](const auto &s) {
                    (void)s.execute(
                        {"UPDATE subathon_accounts SET "
                         "application_access_enabled=0,revision_ref=X'7632' WHERE account_ref=?",
                         {QByteArray(account)}});
                });
                p.attempt(d);
                require(p.decision && std::holds_alternative<ApplicationAccessDenied>(*p.decision),
                        "Current disabled author/subject cannot fall back to owner");
                d.work([&](const auto &s) {
                    (void)s.execute(
                        {"UPDATE subathon_accounts SET "
                         "application_access_enabled=1,revision_ref=X'7633' WHERE account_ref=?",
                         {QByteArray(account)}});
                });
                p.attempt(d);
                allowed(p);
                require(std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref !=
                                original &&
                            captures(o) == before,
                        "Current account fences change revision, never captured rule");
            }
            Store d;
            Ports p;
            // Real role membership on BOTH original-author channels, then remove
            // one. Source must expand actual graph; no combined supplied policy.
            d.work([&](const auto &s) {
                for (int channel = 0; channel < 2; ++channel) {
                    auto rows = profile();
                    for (auto &e : rows) {
                        e.origin = PermissionOrigin::role;
                        e.role = "role";
                    }
                    (void)s.execute(
                        {"INSERT INTO subathon_permission_roles VALUES(?,?,?,?)",
                         {QByteArray("I"), channel, QByteArray("role"), encoded(rows)}});
                    (void)s.execute(
                        {"UPDATE subathon_permission_accounts SET memberships=?,entries=? WHERE "
                         "channel=? AND account_ref=X'41'",
                         {encoded(std::vector<std::string>{"role"}),
                          encoded(std::vector<PermissionPolicyEntry>{}), channel}});
                }
                (void)s.execute(
                    {"UPDATE subathon_permission_head SET revision=X'0000000000000002'", {}});
            });
            p.attempt(d);
            allowed(p);
            d.work([](const auto &s) {
                (void)s.execute({"UPDATE subathon_permission_accounts SET memberships=? WHERE "
                                 "channel=1 AND account_ref=X'41'",
                                 {encoded(std::vector<std::string>{})}});
                (void)s.execute(
                    {"UPDATE subathon_permission_head SET revision=X'0000000000000003'", {}});
            });
            p.attempt(d);
            require(p.decision && std::holds_alternative<ApplicationAccessDenied>(*p.decision),
                    "Actual author membership revocation removes GRANT");
        });
    }
    void originalSqlFailureAndLostCleanupAcknowledgementAreNotSemanticSuccess() {
        scenario([] {
            Store d;
            Ports p;
            // Known schema damage provokes an actual SQL failure in the producer,
            // not a source double throwing an invented writer exception.
            d.work([](const auto &s) {
                (void)s.execute({"DROP TABLE subathon_reference_authority_events", {}});
            });
            const auto c = p.attempt(d, command(), false);
            bool original = false;
            require(p.witness != nullptr, "Original exception witness");
            try {
                std::rethrow_exception(p.witness);
            } catch (const SqliteWorkFailure &e) {
                original = e.error() == WorkError::sql_failure;
            }
            require(original && c.work_error == WorkError::sql_failure &&
                        c.transaction == TransactionOutcome::rolled_back && !c.response,
                    "Original SQL error survives helper/service, no substituted invalid_data");
            Store other;
            Ports invalid;
            invalid.source.refuse = ReferenceAuthorityFailure::invalid_data;
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        other.writer, testing::WorkControlFault::lose_next_rollback_ack) ==
                        StorageError::none,
                    "Existing real rollback acknowledgement seam");
            const auto unknown = invalid.attempt(other, command(), false);
            require(unknown.transaction == TransactionOutcome::outcome_unknown && !unknown.response,
                    "Healthy semantic failure really requests cleanup; lost ack remains unknown");
        });
    }
    void derivedMaximumWithRealAuthorityReachesWholeInputNumericHold() {
        scenario([] {
            Store d(INT64_C(9223372036854775807));
            initial_state(d);
            Observer o(d.path);
            const auto before = captures(o);
            Ports p;
            p.full_accounting = true;
            auto f = configuration_facts();
            f.projection.rule.add_ms = f.enabled_rules[0].rule.add_ms =
                INT64_C(9223372036854775807);
            p.config.data = f;
            const auto result = p.attempt(d);
            allowed(p);
            require(result.transaction == TransactionOutcome::committed &&
                        result.work_error == WorkError::none && result.response &&
                        result.response->disposition == ApplicationServiceDisposition::held_numeric,
                    "Existing service holds Main1+MAX as a whole, not MAX-1 partial award");
            require(p.policies == 1 && p.clocks == 1 && p.actions == 0,
                    "Real arithmetic after new derivation/current consumer");
            require(
                o.rows("SELECT main_remaining_ms FROM subathon_app_states WHERE token=X'5330'") ==
                        SqlRows{{1}} &&
                    o.rows("SELECT * FROM subathon_app_operations").isEmpty() &&
                    o.rows("SELECT * FROM subathon_app_effects").isEmpty() &&
                    o.rows("SELECT * FROM subathon_app_jobs").isEmpty(),
                "No head advance, claim, effect or job");
            require(
                o.rows("SELECT exact_numerator,requested_units FROM subathon_plan_requests") ==
                        SqlRows{{QByteArray("9223372036854775807"),
                                 QVariant::fromValue<qlonglong>(INT64_C(9223372036854775807))}} &&
                    captures(o) == before,
                "Original exact capture and request survive hold");
            require(o.rows("SELECT count(*) FROM subathon_hold_causes")[0][0].toInt() == 1,
                    "One durable whole-input hold");
        });
    }
};
QTEST_GUILESS_MAIN(ReferenceRuleAuthorityTests)
#include "reference_rule_authority_tests.moc"
