// SPDX-License-Identifier: GPL-3.0-or-later
#include "reference_intake_internal.hpp"
namespace subathon::server {
using namespace reference_intake_detail;
namespace {
struct SessionBridge final : CurrentSessionSource, CurrentSessionClock {
    SqliteAccountSessionUnit &source;
    CurrentSessionClock &clock;
    std::exception_ptr failure;
    SessionBridge(SqliteAccountSessionUnit &s, CurrentSessionClock &c) : source(s), clock(c) {}
    SessionLookup load_current(const SessionLookupRequest &r, const SessionLimits &l) override {
        try {
            auto loaded = source.read_lifecycle_actor(r, l);
            if (auto p = std::get_if<LifecycleCurrentSession>(&loaded))
                return std::move(p->current);
            return SessionSourceUnavailable{};
        } catch (...) {
            failure = std::current_exception();
            return SessionSourceUnavailable{};
        }
    }
    SessionClockResolution sample_current() override {
        try {
            return clock.sample_current();
        } catch (...) {
            failure = std::current_exception();
            return SessionSourceUnavailable{};
        }
    }
};
std::string head_configuration(const SqliteWorkSession &s, const ReferenceRuntimeLimits &l) {
    auto meta = s.execute(
        {"SELECT typeof(s.configuration_ref),length(CAST(s.configuration_ref AS BLOB)) "
         "FROM subathon_app_head h LEFT JOIN subathon_app_states s ON s.token=h.token LIMIT 1",
         {}});
    if (meta.empty())
        refuse(l, ReferenceGate::not_ready, "reference.runtime-unavailable.v1");
    data(meta.size() == 1 && meta[0][0].toString() == "blob");
    require(meta[0][1].toULongLong() <= l.values.reference_bytes, Error::result_limit);
    auto row = s.execute({"SELECT s.configuration_ref FROM subathon_app_head h JOIN "
                          "subathon_app_states s ON s.token=h.token LIMIT 1",
                          {}});
    data(row.size() == 1);
    return row[0][0].toByteArray().toStdString();
}
ResolvedAccountSession authenticate_session(const SqliteWorkSession &s,
                                            const SessionLookupRequest &r, bool initial,
                                            CurrentSessionClock &clock,
                                            const ReferenceRuntimeLimits &l) {
    SqliteAccountSessionUnit accounts(s, {{}}, l.authority.accounts);
    SessionBridge bridge(accounts, clock);
    auto resolved =
        resolve_current_account_session(r, initial, bridge, bridge, l.authority.sessions);
    if (bridge.failure)
        std::rethrow_exception(bridge.failure);
    if (!std::holds_alternative<ResolvedAccountSession>(resolved))
        refuse(l, ReferenceGate::denied, "reference.access-denied.v1");
    return std::get<ResolvedAccountSession>(std::move(resolved));
}
void authorize(const SqliteWorkSession &s, const SessionLookupRequest &r, bool initial,
               const ResolvedAccountSession &actor, const std::string &timer,
               const ReferenceRuntimeLimits &l) {
    auto trust = reference_permission_graph_trust();
    auto graph_limits = l.authority.graph;
    graph_limits.selected_bytes = std::min(graph_limits.selected_bytes, l.values.value_owned_bytes);
    graph_limits.items = std::min(graph_limits.items, l.values.value_items);
    SqlitePermissionGraphUnit graph(s, r.installation_ref, trust, graph_limits);
    auto result = graph.load();
    if (!std::holds_alternative<StoredPermissionGraph>(result))
        refuse(l, ReferenceGate::denied, "reference.access-denied.v1");
    const auto &g = std::get<StoredPermissionGraph>(result);
    auto m = measure(l.values, Error::result_limit);
    m(g);
    if (initial && (!g.owner.account || *g.owner.account != actor.account_ref))
        refuse(l, ReferenceGate::denied, "reference.access-denied.v1");
    auto found = std::find_if(g.use.accounts.begin(), g.use.accounts.end(),
                              [&](const auto &a) { return a.account == actor.account_ref; });
    data(found != g.use.accounts.end());
    core::PermissionPolicyFacts policy{core::PermissionFactsState::complete, found->assigned_roles,
                                       found->individual_entries};
    for (const auto &role : found->assigned_roles) {
        auto it = std::find_if(g.use.roles.begin(), g.use.roles.end(),
                               [&](const auto &v) { return v.role == role; });
        data(it != g.use.roles.end());
        require(it->entries.size() <= l.values.value_items - policy.entries.size(),
                Error::result_limit);
        policy.entries.insert(policy.entries.end(), it->entries.begin(), it->entries.end());
    }
    core::PermissionRequest request{initial ? "access.manage" : "timer.snapshot.read", {}, {}};
    if (!initial)
        request.resources = {{"timer", timer}};
    core::EffectivePermissionInput input{actor.account_ref, actor.snapshot_ref, g.catalog,
                                         g.owner,           std::move(policy),  std::move(request)};
    auto p = measure(l.values, Error::result_limit);
    p(input);
    if (core::evaluate_effective_permission(input, l.authority.permissions).data().outcome !=
        core::PermissionOutcome::permit)
        refuse(l, ReferenceGate::denied, "reference.access-denied.v1");
}
void authenticate(const SqliteWorkSession &s, const SessionLookupRequest &r, bool initial,
                  CurrentSessionClock &clock, const std::string &timer,
                  const ReferenceRuntimeLimits &l) {
    const auto actor = authenticate_session(s, r, initial, clock, l);
    authorize(s, r, initial, actor, timer, l);
}
void preflight(const ReferenceRuntimeLimits &l, const ReferenceConnectorRegistration &connector) {
    runtime_limits(l);
    auto p = measure(l.values);
    p(connector);
    require(!connector.connection_ref.empty() && !connector.origin.provider.empty() &&
                !connector.origin.source_account.empty() && !connector.identifier_namespace.empty(),
            Error::invalid_reference);
}
void session_input(const SessionLookupRequest &r) {
    require(!r.installation_ref.empty(), Error::invalid_reference);
}
core::ContributionConditions conditions(const std::string &configuration) {
    core::ContributionConditions c;
    c.valuation = {false, {}, {}, false};
    c.review = core::ContributionReview::not_required;
    c.evidence_admission = {core::EvidencePolicy::require_evidence_review, configuration, {}};
    c.settlement = core::SettlementGate::clear;
    return c;
}
} // namespace
ReferenceIntake::ReferenceIntake(ReferenceConfigurationSource &configuration,
                                 ReferenceClockSampler &clock, ReferenceEntropy &entropy,
                                 CurrentSessionClock &sessions,
                                 ReferenceConnectorRegistration connector,
                                 ReferenceApplicationAdapters *runtime,
                                 ReferenceRuntimeLimits limits)
    : configuration_(configuration), clock_(clock), entropy_(entropy), sessions_(sessions),
      connector_(std::move(connector)), runtime_(runtime), limits_(limits) {}
ReferencePreparedWork
ReferenceIntake::make_initial_work(const ReferenceInitialRequest &r,
                                   std::function<void(ReferenceInitialCompletion)> callback) const
    try {
    preflight(limits_, connector_);
    auto p = measure(limits_.values);
    p(r, connector_);
    require(unsigned(r.timer.lifecycle) <= 2 && unsigned(r.timer.phase) <= 1, Error::invalid_tag);
    session_input(r.owner_session);
    for (const auto *s :
         {&r.configuration_ref, &r.state_token, &r.clock_lineage_ref, &r.observation_ref})
        require(!s->empty(), Error::invalid_reference);
    require(r.timer.main_remaining_ms >= 0 && r.timer.grace_remaining_ms >= 0,
            Error::invalid_value);
    thread(owner_);
    require(pending_->load() < limits_.pending_reservations, Error::input_limit);
    return pair<ReferenceClockAnchor, ReferenceInitialCompletion>(
        limits_, pending_, std::move(callback),
        [this, r](const SqliteWorkSession &s, auto project, Cleanup &cleanup) -> QVariant {
            (void)cleanup;
            auto cfg = admitted_config(configuration_, r.configuration_ref, limits_);
            require(cfg.synthetic_origin == connector_.origin &&
                        cfg.receipt_namespace == connector_.identifier_namespace,
                    Error::invalid_binding);
            authenticate(s, r.owner_session, true, sessions_, cfg.rule.timer_ref, limits_);
            if (r.timer.lifecycle != core::TimerLifecycle::Active ||
                r.timer.phase != core::CountdownPhase::Main || r.timer.main_remaining_ms <= 0 ||
                r.timer.awaiting_confirmation)
                refuse(limits_, ReferenceGate::not_ready, "reference.runtime-unavailable.v1");
            SqliteApplicationBundleStore store(s, bundle_limits(limits_));
            if (store.read_state())
                refuse(limits_, ReferenceGate::conflict, "reference.initial-conflict.v1");
            auto sample = borrow([&] { return clock_.sample(); });
            auto m = measure(limits_.values, Error::result_limit);
            m(sample);
            if (auto u = std::get_if<ReferenceUnavailable>(&sample))
                refuse(limits_, *u);
            auto time = std::get<ReferenceClockSample>(sample);
            ReferenceCommittedState state{{r.state_token,
                                           {std::string(state_format), QByteArray("", 0)},
                                           r.configuration_ref,
                                           r.timer,
                                           0,
                                           {}},
                                          {},
                                          {r.clock_lineage_ref,
                                           r.observation_ref,
                                           r.configuration_ref,
                                           cfg.policy_revision_ref,
                                           r.state_token,
                                           time.utc_ms,
                                           {}},
                                          cfg.policy_revision_ref};
            state.snapshot = encode_reference_state(state, limits_.values);
            ReferenceClockAnchor anchor{state, time};
            project(anchor);
            auto document = encode_reference_configuration(cfg, limits_.values);
            auto retained = SqliteRetainedContextStore(s, retained_limits(limits_))
                                .retain({{RetainedKind::configuration, r.configuration_ref},
                                         document.document,
                                         {}});
            if (retained == RetainDisposition::conflict ||
                retained == RetainDisposition::missing_reference)
                refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
            if (store.retain_initial_state(state.snapshot) !=
                SqliteBundleInitialDisposition::inserted)
                refuse(limits_, ReferenceGate::conflict, "reference.initial-conflict.v1");
            s.commit();
            return QVariant::fromValue(anchor);
        },
        [](auto &out, const auto &a) { out.anchor = a; }, 2, runtime_);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ReferencePreparedWork
ReferenceIntake::make_receipt_work(const std::string &ref, const ReferenceSyntheticInput &input,
                                   std::function<void(ReferenceReceiptCompletion)> callback) const
    try {
    preflight(limits_, connector_);
    auto p = measure(limits_.values);
    p(ref, input, connector_);
    require(!ref.empty() && !input.connection_ref.empty() && !input.event_id.empty(),
            Error::invalid_reference);
    require(!input.original_payload.isNull(), Error::invalid_shape);
    require(input.connection_ref == connector_.connection_ref, Error::invalid_binding);
    thread(owner_);
    require(pending_->load() < limits_.pending_reservations, Error::input_limit);
    return pair<ReceiptValue, ReferenceReceiptCompletion>(
        limits_, pending_, std::move(callback),
        [this, ref, input](const SqliteWorkSession &s, auto project, Cleanup &cleanup) -> QVariant {
            auto cfg = admitted_config(configuration_, ref, limits_);
            live(s, runtime_, limits_);
            if (head_configuration(s, limits_) != ref)
                refuse(limits_, ReferenceGate::not_ready, "reference.runtime-unavailable.v1");
            data(cfg.synthetic_origin == connector_.origin &&
                 cfg.receipt_namespace == connector_.identifier_namespace);
            auto sampled = borrow([&] { return clock_.sample(); });
            auto m = measure(limits_.values, Error::result_limit);
            m(sampled);
            if (auto u = std::get_if<ReferenceUnavailable>(&sampled))
                refuse(limits_, *u);
            auto time = std::get<ReferenceClockSample>(sampled);
            auto context = draw(entropy_, "RCT1", limits_);
            auto event = context;
            event.replace(0, 4, "REV1");
            auto projected = measure(limits_.values, Error::result_limit);
            projected(context, event, time.utc_ms, cfg, input);
            ReferenceArrivalCapture a{context, event, time.utc_ms, cfg, input};
            auto config_doc = encode_reference_configuration(cfg, limits_.values),
                 arrival_doc = encode_reference_arrival(a, limits_.values);
            SqliteRetainedContextStore retained(s, retained_limits(limits_));
            auto rc =
                retained.retain({{RetainedKind::configuration, ref}, config_doc.document, {}});
            if (rc != RetainDisposition::inserted && rc != RetainDisposition::already_present)
                refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
            if (retained.retain({{RetainedKind::arrival_context, context},
                                 arrival_doc.document,
                                 ConfigurationReference{ref}}) != RetainDisposition::inserted)
                refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
            core::ReceiptInput in{
                {connector_.origin, connector_.identifier_namespace, input.event_id},
                {"synthetic.reference.support.v1", 1, input.source_utc_ms, {}, {}, {}, {}, {}},
                {time.utc_ms, input.connection_ref, ref, context}};
            SqliteReceiptLimits rl{qsizetype(limits_.values.reference_bytes),
                                   qsizetype(limits_.values.document_bytes),
                                   qsizetype(limits_.values.value_items)};
            auto ids = s.execute(
                {"SELECT CASE WHEN typeof(receipt_id)='blob' AND length(receipt_id)=8 THEN "
                 "receipt_id ELSE NULL END FROM subathon_receipts WHERE provider=? AND "
                 "source_account=? "
                 "AND identifier_namespace=? AND opaque_id=? LIMIT 1",
                 {bytes(in.identity.origin.provider), bytes(in.identity.origin.source_account),
                  bytes(in.identity.identifier_namespace), bytes(in.identity.opaque_id)}});
            std::optional<ContributionReceipt> old;
            if (!ids.empty()) {
                data(ids.size() == 1);
                auto id = decode_receipt_uint64(ids[0][0].toByteArray());
                require(id != 0, Error::invalid_value);
                old = receipt(s, {id}, limits_);
                a = arrival(s, *old, configuration_, limits_);
            }
            ReceiptValue value{{old ? old->id : core::ReceiptId{0},
                                old ? (old->facts == in.facts ? core::ReceiptDisposition::retry
                                                              : core::ReceiptDisposition::conflict)
                                    : core::ReceiptDisposition::created},
                               a};
            project(value);
            SqliteReceiptRepository repository(adopt_active_receipt_session, s, rl);
            ReceiptOwnerRepository adopted(repository, cleanup);
            ReceiptIntake service(adopted);
            value.receipt = service.ingest(in);
            // Result shape/size is independent of the allocated receipt ID.
            return QVariant::fromValue(value);
        },
        [](auto &out, const auto &v) {
            out.receipt = v.receipt;
            out.original = v.original;
        },
        3, runtime_);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ReferencePreparedWork ReferenceIntake::make_evidence_work(
    core::ReceiptId id, std::function<void(ReferenceEvidenceCompletion)> callback) const try {
    preflight(limits_, connector_);
    require(id.value != 0, Error::invalid_value);
    thread(owner_);
    require(pending_->load() < limits_.pending_reservations, Error::input_limit);
    return pair<ContributionLinkResult, ReferenceEvidenceCompletion>(
        limits_, pending_, std::move(callback),
        [this, id](const SqliteWorkSession &s, auto project, Cleanup &cleanup) -> QVariant {
            live(s, runtime_, limits_);
            auto original = receipt(s, id, limits_);
            auto a = arrival(s, original, configuration_, limits_);
            (void)a;
            ContributionObservation observation{original,
                                                ContributionObservationKind::support,
                                                core::PaymentEvidence::not_applicable,
                                                {},
                                                conditions(original.arrival.configuration_ref)};
            auto input = measure(limits_.values);
            input(observation);
            SqliteContributionLimits nested{
                qsizetype(std::min(limits_.values.reference_bytes, std::uint64_t{4096})),
                qsizetype(std::min(limits_.values.document_bytes, std::uint64_t{262144})),
                qsizetype(std::min(limits_.values.value_items, std::uint64_t{1024}))};
            SqliteContributionRepository repository(adopt_active_contribution_session, s, nested);
            // Every supported result has a present unsigned64 CID and one disposition.
            project(ContributionLinkResult{core::ContributionId{0},
                                           ContributionLinkDisposition::created});
            ContributionOwnerRepository adopted(repository, cleanup);
            ContributionEvidenceService service(adopted);
            auto result = service.observe(observation);
            return QVariant::fromValue(result);
        },
        [](auto &out, const auto &v) { out.evidence = v; }, 2, runtime_);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ReferencePreparedWork
ReferenceIntake::make_capture_work(core::ReceiptId id, core::ContributionId cid,
                                   std::function<void(ReferenceCaptureCompletion)> callback) const
    try {
    preflight(limits_, connector_);
    require(id.value != 0, Error::invalid_value);
    thread(owner_);
    require(pending_->load() < limits_.pending_reservations, Error::input_limit);
    return pair<ReferenceInitialCapture, ReferenceCaptureCompletion>(
        limits_, pending_, std::move(callback),
        [this, id, cid](const SqliteWorkSession &s, auto project, Cleanup &cleanup) -> QVariant {
            (void)cleanup;
            live(s, runtime_, limits_);
            auto original = receipt(s, id, limits_);
            auto a = arrival(s, original, configuration_, limits_);
            ReferenceCaptureLimits cl{qsizetype(limits_.values.reference_bytes),
                                      qsizetype(limits_.values.document_bytes),
                                      qsizetype(limits_.values.value_owned_bytes)};
            SqliteReferenceCaptureStore captures(s, cl);
            ReferenceCaptureKey key{a.configuration.rule.automation.installation_ref,
                                    original.identity};
            if (auto old = captures.find(key)) {
                if (old->receipt_id != id || old->contribution_id != cid)
                    refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
                project(*old);
                s.commit();
                return QVariant::fromValue(*old);
            }
            auto plan = draw(entropy_, "RPL1", limits_),
                 execution = draw(entropy_, "REX1", limits_);
            metadata(s, "subathon_contributions", contribution_columns(), "contribution_id",
                     encode_contribution_id(cid), limits_);
            auto parent =
                s.execute({"SELECT CASE WHEN typeof(original_receipt_id)='blob' AND "
                           "length(original_receipt_id)=8 THEN original_receipt_id ELSE NULL END "
                           "FROM subathon_contributions WHERE contribution_id=? LIMIT 1",
                           {encode_contribution_id(cid)}});
            data(parent.size() == 1 &&
                 parent[0][0].toByteArray() == encode_receipt_uint64(id.value));
            auto contribution = Access(s, bundle_limits(limits_)).contribution_by_id(cid);
            data(contribution && contribution->original_receipt_id == id);
            auto complete =
                borrow([&] { return configuration_.snapshot(a.configuration.configuration_ref); });
            auto evaluated = evaluate_reference_rule(a, complete, limits_.values);
            if (auto u = std::get_if<ReferenceUnavailable>(&evaluated))
                refuse(limits_, *u);
            auto rule = std::get<ReferenceRuleEvaluation>(evaluated);
            auto bound = measure(limits_.values, Error::result_limit);
            bound(plan, a.event_ref, a.event_ref, id, original.identity, original.facts,
                  original.arrival, cid, contribution->state, rule.request, rule.selection);
            bound.p.fixed(8 + 1);
            require(bound.p.items <= bound.p.item_cap - 3, Error::result_limit);
            struct Rules final : core::RulePlanSource {
                core::RuleSelection selection;
                explicit Rules(core::RuleSelection s) : selection(std::move(s)) {}
                std::vector<core::RuleSelection> capture(const core::PlanEvent &) override {
                    return {selection};
                }
            } rules(rule.selection);
            struct Modifiers final : core::ModifierPlanSource {
                std::vector<core::ModifierSelection> capture(const core::PlanEvent &) override {
                    return {};
                }
            } modifiers;
            core::PlanCapture captured{{a.event_ref,
                                        {},
                                        a.event_ref,
                                        id,
                                        original.identity,
                                        original.facts,
                                        original.arrival,
                                        cid},
                                       contribution->state,
                                       {core::PlanValuationState::not_applicable, {}},
                                       {rule.request}};
            auto application = core::capture_application_plan(plan, captured, rules, modifiers);
            const auto &revision = application.revisions.front();
            ReferenceExecutionBinding binding{
                execution, a.arrival_context_ref, key.installation_ref, original.identity, id, cid,
                {plan, 1}, a.configuration.rule};
            ReferenceInitialCapture value{key,
                                          id,
                                          cid,
                                          a.configuration.configuration_ref,
                                          a.arrival_context_ref,
                                          a.event_ref,
                                          {plan, 1},
                                          execution};
            project(value);
            if (SqliteApplicationPlanStore(s).retain_initial(plan, revision) !=
                SqlitePlanInitialDisposition::inserted)
                refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
            auto doc = encode_reference_execution(binding, limits_.values);
            if (SqliteRetainedContextStore(s, retained_limits(limits_))
                    .retain({{RetainedKind::arrival_context, execution},
                             doc.document,
                             ConfigurationReference{a.configuration.configuration_ref}}) !=
                RetainDisposition::inserted)
                refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
            if (captures.retain_initial(value) != ReferenceCaptureRetainDisposition::inserted)
                refuse(limits_, ReferenceGate::conflict, "reference.collision.v1");
            s.commit();
            return QVariant::fromValue(value);
        },
        [](auto &out, const auto &v) { out.capture = v; }, 2, runtime_);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
ReferencePreparedWork
ReferenceIntake::make_snapshot_work(const SessionLookupRequest &r,
                                    std::function<void(ReferenceSnapshotCompletion)> callback) const
    try {
    preflight(limits_, connector_);
    auto p = measure(limits_.values);
    p(r);
    session_input(r);
    thread(owner_);
    require(pending_->load() < limits_.pending_reservations, Error::input_limit);
    return pair<ReferenceSnapshot, ReferenceSnapshotCompletion>(
        limits_, pending_, std::move(callback),
        [this, r](const SqliteWorkSession &s, auto project, Cleanup &cleanup) -> QVariant {
            (void)cleanup;
            const auto actor = authenticate_session(s, r, false, sessions_, limits_);
            auto ref = head_configuration(s, limits_);
            auto cfg = admitted_config(configuration_, ref, limits_);
            authorize(s, r, false, actor, cfg.rule.timer_ref, limits_);
            auto head = SqliteApplicationBundleStore(s, bundle_limits(limits_)).read_state();
            data(bool(head) && head->configuration_ref == ref);
            (void)retained_config(s, configuration_, ref, limits_);
            auto decoded = decode_reference_state(*head, limits_.values);
            if (!active(*head))
                refuse(limits_, ReferenceGate::not_ready, "reference.runtime-unavailable.v1");
            ReferenceSnapshot snapshot{decoded, cfg.maximum_main_ms};
            project(snapshot);
            s.commit();
            return QVariant::fromValue(snapshot);
        },
        [](auto &out, const auto &v) { out.snapshot = v; }, 2, runtime_);
} catch (const std::bad_alloc &) {
    fail(Error::allocation_failure);
} catch (const std::length_error &) {
    fail(Error::allocation_failure);
}
} // namespace subathon::server
