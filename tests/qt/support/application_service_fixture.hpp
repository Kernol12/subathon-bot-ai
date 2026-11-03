// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Literal DATA and old storage primitives only. No service entry or codec is called here.
#include "sqlite_application_hold_fixture.hpp"
#include <subathon/server/application_service.hpp>

namespace application_service_fixture {
namespace core = subathon::core;
using namespace subathon::server;
using application_hold_fixture::blob;
using application_hold_fixture::check_rows;
using application_hold_fixture::envelope;
using application_hold_fixture::exact;
using application_hold_fixture::execute;
using application_hold_fixture::identifier;
using application_hold_fixture::integer;
using application_hold_fixture::limits;
using application_hold_fixture::Observer;
using application_hold_fixture::operation;
using application_hold_fixture::optional_text;
using application_hold_fixture::plan_key;
using application_hold_fixture::require;
using application_hold_fixture::run;
using application_hold_fixture::text;

inline core::TimerState timer(const QJsonValue &value) {
    const auto o = value.toObject();
    return {static_cast<core::TimerLifecycle>(o["lifecycle"].toInt()),
            static_cast<core::CountdownPhase>(o["phase"].toInt()), integer(o["main"]),
            integer(o["grace"]), o["wait"].toBool()};
}
inline std::vector<ApplicationBalance> balances(const QJsonValue &value) {
    std::vector<ApplicationBalance> result;
    for (const auto &v : value.toArray()) {
        const auto o = v.toObject();
        result.push_back({static_cast<ApplicationBalanceKind>(o["kind"].toInt()), text(o["target"]),
                          integer(o["units"])});
    }
    return result;
}
inline ApplicationStateSnapshot state(const QJsonValue &value) {
    const auto o = value.toObject();
    return {text(o["token"]),  envelope(o["envelope"]), text(o["configuration"]),
            timer(o["timer"]), integer(o["debt"]),      balances(o["balances"])};
}
inline OriginalApplicationCommand command(const QJsonValue &value) {
    const auto o = value.toObject(), p = o["principal"].toObject();
    OriginalApplicationCommand c;
    c.operation = operation(o["operation"]);
    c.subject_kind = static_cast<ApplicationSubjectKind>(o["subject"].toInt());
    c.receipt_id = {identifier(o["receipt"])};
    if (!o["cid"].isNull())
        c.contribution_id = core::ContributionId{identifier(o["cid"])};
    c.plan = plan_key(o["plan"]);
    c.principal = {static_cast<ApplicationPrincipalKind>(p["kind"].toInt()),
                   text(p["installation"]), text(p["principal"])};
    c.purpose = static_cast<OriginalApplicationPurpose>(o["purpose"].toInt());
    c.preview_mode = static_cast<ApplicationPreviewMode>(o["preview_mode"].toInt());
    c.preview_state_token = optional_text(o["preview"]);
    c.actor_ref = optional_text(o["actor"]);
    c.decision_ref = optional_text(o["decision"]);
    c.reason = optional_text(o["reason"]);
    for (const auto &e : o["choices"].toArray())
        c.choices.push_back(envelope(e));
    return c;
}
inline ApplicationOriginalClaimSelector selector(const QJsonObject &o) {
    ApplicationOriginalClaimSelector s;
    s.subject_kind = static_cast<ApplicationSubjectKind>(o["subject"].toInt());
    s.receipt_id = {identifier(o["receipt"])};
    if (!o["cid"].isNull())
        s.contribution_id = core::ContributionId{identifier(o["cid"])};
    return s;
}
inline BoundApplicationPolicy policy(const QJsonValue &value) {
    const auto o = value.toObject(), t = o["timer_config"].toObject();
    BoundApplicationPolicy p;
    p.operation = operation(o["operation"]);
    p.source = selector(o);
    p.plan = plan_key(o["plan"]);
    p.checked_state_token = text(o["head"]);
    p.configuration_ref = text(o["configuration"]);
    p.permission_revision_ref = text(o["permission"]);
    p.policy_revision_ref = text(o["policy"]);
    if (!o["cap"].isNull())
        p.maximum_main_ms = integer(o["cap"]);
    p.timer_config.end_mode = static_cast<core::TimerEndMode>(t["end"].toInt());
    p.timer_config.grace_duration_ms = integer(t["grace_duration"]);
    p.timer_config.grace_reuse = static_cast<core::GraceReuse>(t["reuse"].toInt());
    if (!t["hard_end"].isNull())
        p.timer_config.hard_end_utc_ms = integer(t["hard_end"]);
    p.paused = o["paused"].toBool();
    p.recovery_pending = o["recovery"].toBool();
    p.pause_recovery_revision = text(o["pause_recovery"]);
    p.explanation = text(o["explanation"]);
    p.admission = {static_cast<ApplicationPolicyAdmission>(o["admission"].toInt()),
                   optional_text(o["admission_reason"])};
    for (const auto &entry : o["corrections"].toArray()) {
        const auto e = entry.toObject();
        p.correction_policies.push_back(
            {integer(e["ordinal"]), static_cast<core::RefundDeficitPolicy>(e["policy"].toInt())});
    }
    return p;
}
inline ApplicationClockObservation clock(const QJsonValue &value) {
    const auto o = value.toObject();
    ApplicationClockObservation c;
    c.clock_lineage_ref = text(o["lineage"]);
    c.observation_ref = text(o["observation"]);
    c.configuration_ref = text(o["configuration"]);
    c.policy_revision_ref = text(o["policy"]);
    c.checked_head_token = text(o["head"]);
    c.observed_utc_ms = integer(o["utc"]);
    for (const auto &entry : o["segments"].toArray()) {
        const auto e = entry.toObject();
        c.segments.push_back({integer(e["elapsed"]), integer(e["utc"]), e["paused"].toBool()});
    }
    return c;
}
inline std::vector<core::RequestedEffect> requests(const QJsonValue &value) {
    std::vector<core::RequestedEffect> result;
    for (const auto &entry : value.toArray()) {
        const auto r = entry.toObject();
        result.push_back({text(r["action"]), text(r["version"]), text(r["rule"]), text(r["target"]),
                          static_cast<core::RequestedUnit>(r["unit"].toInt()),
                          r["requires_historical"].toBool(), exact(r["exact"]), integer(r["final"]),
                          text(r["explanation"]), text(r["reversal"])});
    }
    return result;
}
inline ApplicationEffect effect(const QJsonValue &value) {
    const auto e = value.toObject();
    ApplicationEffect r;
    r.request_ordinal = integer(e["ordinal"]);
    r.arithmetic_role = static_cast<ApplicationArithmeticRole>(e["role"].toInt());
    r.unit = static_cast<core::RequestedUnit>(e["unit"].toInt());
    r.exact_requested_units = exact(e["exact"]);
    r.requested_units = integer(e["final"]);
    r.rounding_residual = exact(e["residual"]);
    if (!e["cap"].isNull())
        r.maximum_main_ms = integer(e["cap"]);
    if (!e["deficit_policy"].isNull())
        r.deficit_policy = static_cast<core::RefundDeficitPolicy>(e["deficit_policy"].toInt());
    if (!e["deficit_disposition"].isNull())
        r.deficit_disposition =
            static_cast<core::TimeReversalDisposition>(e["deficit_disposition"].toInt());
    r.application_explanation = text(e["explanation"]);
    for (const auto &entry : e["components"].toArray()) {
        const auto c = entry.toObject();
        r.components.push_back({static_cast<ApplicationComponentKind>(c["kind"].toInt()),
                                integer(c["units"]), optional_text(c["target"])});
    }
    return r;
}
inline ApplicationJobIntent job(const QJsonValue &value) {
    const auto o = value.toObject();
    ApplicationJobIntent j;
    j.job_ref = text(o["ref"]);
    if (!o["source"].isNull())
        j.source_request_ordinal = integer(o["source"]);
    j.group_ref = optional_text(o["group"]);
    if (!o["group_ordinal"].isNull())
        j.group_ordinal = integer(o["group_ordinal"]);
    j.envelope = envelope(o["envelope"]);
    return j;
}
inline PreparedApplicationActions actions(const QJsonValue &value) {
    const auto o = value.toObject();
    PreparedApplicationActions a;
    for (const auto &j : o["jobs"].toArray())
        a.jobs.push_back(job(j));
    a.explanation = text(o["explanation"]);
    return a;
}
inline ApplicationCalculatedProposal proposal(const QJsonObject &c) {
    const auto a = c["arithmetic"].toObject();
    require(a["kind"] == "success", "Literal complete successful numeric proposal");
    ApplicationCalculatedProposal r;
    r.clock_advanced_timer = timer(c["head"].toObject()["timer"]);
    r.timer_after = timer(a["timer_after"]);
    r.debt_after = integer(a["debt_after"]);
    r.balances_after = balances(a["balances_after"]);
    for (const auto &e : a["effects"].toArray())
        r.effects.push_back(effect(e));
    for (const auto &t : a["transitions"].toArray())
        r.transitions.push_back(static_cast<core::TimerTransition>(t.toInt()));
    r.observation = clock(c["clock"]);
    return r;
}
inline core::ContributionState contribution(const QJsonValue &value) {
    const auto o = value.toObject();
    core::ContributionState c;
    c.payment_evidence = static_cast<core::PaymentEvidence>(o["payment_evidence"].toInt());
    auto &s = c.conditions;
    s.valuation.monetary = o["monetary"].toInt() != 0;
    if (!o["original_decimal"].isNull())
        s.valuation.original_money =
            core::ReceiptMoney{text(o["original_decimal"]), text(o["currency"])};
    s.valuation.resolution_ref = optional_text(o["resolution_ref"]);
    s.valuation.missing_rate = o["missing_rate"].toInt() != 0;
    s.review = static_cast<core::ContributionReview>(o["review"].toInt());
    s.evidence_admission.policy = static_cast<core::EvidencePolicy>(o["evidence_policy"].toInt());
    s.evidence_admission.policy_ref = text(o["policy_ref"]);
    if (!o["decision_choice"].isNull())
        s.evidence_admission.review_decision = core::EvidenceReviewDecision{
            static_cast<core::EvidenceReviewChoice>(o["decision_choice"].toInt()),
            text(o["decision_ref"]), text(o["decision_actor_ref"])};
    s.application = static_cast<core::ContributionApplication>(o["application"].toInt());
    s.reversal = static_cast<core::ContributionReversal>(o["reversal"].toInt());
    s.settlement = static_cast<core::SettlementGate>(o["settlement"].toInt());
    return c;
}
inline RecordedApplicationBundle record(const QJsonValue &value) {
    const auto o = value.toObject();
    const auto c = command(o["command"]);
    RecordedApplicationBundle r;
    r.intent.operation = c.operation;
    r.intent.subject_kind = c.subject_kind;
    r.intent.receipt_id = c.receipt_id;
    r.intent.contribution_id = c.contribution_id;
    r.intent.plan = c.plan;
    r.persisted_head = state(o["head"]);
    r.intent.expected_state_token = r.persisted_head.token;
    if (!o["expected_contribution"].isNull()) {
        r.intent.expected_contribution_state = contribution(o["expected_contribution"]);
        r.contribution = ApplicationContributionSnapshot{*c.contribution_id,
                                                         *r.intent.expected_contribution_state,
                                                         core::ContributionApplication::applied};
    }
    r.intent.choices = envelope(o["command_envelope"]);
    r.advanced_before = state(o["before"]);
    r.after = state(o["after"]);
    r.advance_explanation = text(o["advance_explanation"]);
    for (const auto &e : o["effects"].toArray())
        r.effects.push_back(effect(e));
    for (const auto &j : o["jobs"].toArray())
        r.jobs.push_back(job(j));
    for (const auto &t : o["transitions"].toArray())
        r.transitions.push_back(static_cast<core::TimerTransition>(t.toInt()));
    r.result = envelope(o["result"]);
    r.audit = {c.actor_ref, c.decision_ref, c.reason, envelope(o["audit"])};
    return r;
}
inline PreparedApplicationBundle prepared(const QJsonValue &value) {
    const auto r = record(value);
    return {r.intent,      r.advanced_before,
            r.after,       r.advance_explanation,
            r.effects,     r.jobs,
            r.transitions, {},
            r.result,      r.audit};
}

// Explicit field comparison for scripted input DATA. No codec/equality from the UUT.
inline bool same_command(const OriginalApplicationCommand &a, const OriginalApplicationCommand &b) {
    if (a.operation.operation_scope != b.operation.operation_scope ||
        a.operation.operation_ref != b.operation.operation_ref ||
        a.subject_kind != b.subject_kind || a.receipt_id != b.receipt_id ||
        a.contribution_id != b.contribution_id || a.plan.plan_ref != b.plan.plan_ref ||
        a.plan.revision != b.plan.revision || a.principal.kind != b.principal.kind ||
        a.principal.installation_ref != b.principal.installation_ref ||
        a.principal.principal_ref != b.principal.principal_ref || a.purpose != b.purpose ||
        a.preview_mode != b.preview_mode || a.preview_state_token != b.preview_state_token ||
        a.actor_ref != b.actor_ref || a.decision_ref != b.decision_ref || a.reason != b.reason ||
        a.choices.size() != b.choices.size())
        return false;
    for (size_t n = 0; n < a.choices.size(); ++n)
        if (a.choices[n].format_ref != b.choices[n].format_ref ||
            a.choices[n].document != b.choices[n].document ||
            a.choices[n].document.isNull() != b.choices[n].document.isNull())
            return false;
    return true;
}
inline bool same_timer(const core::TimerState &a, const core::TimerState &b) {
    return a.lifecycle == b.lifecycle && a.phase == b.phase &&
           a.main_remaining_ms == b.main_remaining_ms &&
           a.grace_remaining_ms == b.grace_remaining_ms &&
           a.awaiting_confirmation == b.awaiting_confirmation;
}
inline void check_state(const ApplicationStateSnapshot &a, const ApplicationStateSnapshot &b) {
    require(a.token == b.token && a.configuration_ref == b.configuration_ref &&
                a.envelope.format_ref == b.envelope.format_ref &&
                a.envelope.document == b.envelope.document && !a.envelope.document.isNull() &&
                same_timer(a.timer, b.timer) && a.time_debt_ms == b.time_debt_ms &&
                a.balances.size() == b.balances.size(),
            "Complete literal retained snapshot");
    for (size_t n = 0; n < b.balances.size(); ++n)
        require(a.balances[n].kind == b.balances[n].kind &&
                    a.balances[n].target_ref == b.balances[n].target_ref &&
                    a.balances[n].units == b.balances[n].units,
                "Every ordered literal balance");
}
inline void check_effect(const ApplicationEffect &a, const ApplicationEffect &b) {
    require(a.request_ordinal == b.request_ordinal && a.arithmetic_role == b.arithmetic_role &&
                a.unit == b.unit && a.exact_requested_units == b.exact_requested_units &&
                a.requested_units == b.requested_units &&
                a.rounding_residual == b.rounding_residual &&
                a.maximum_main_ms == b.maximum_main_ms && a.deficit_policy == b.deficit_policy &&
                a.deficit_disposition == b.deficit_disposition &&
                a.application_explanation == b.application_explanation &&
                a.components.size() == b.components.size(),
            "Complete literal effect");
    for (size_t n = 0; n < b.components.size(); ++n)
        require(a.components[n].kind == b.components[n].kind &&
                    a.components[n].units == b.components[n].units &&
                    a.components[n].target_ref == b.components[n].target_ref,
                "Every actual literal component");
}
inline void check_record(const RecordedApplicationBundle &a, const RecordedApplicationBundle &b) {
    const auto &x = a.intent, &y = b.intent;
    require(x.operation.operation_scope == y.operation.operation_scope &&
                x.operation.operation_ref == y.operation.operation_ref && x.kind == y.kind &&
                x.subject_kind == y.subject_kind && x.receipt_id == y.receipt_id &&
                x.contribution_id == y.contribution_id && x.plan.plan_ref == y.plan.plan_ref &&
                x.plan.revision == y.plan.revision &&
                x.expected_state_token == y.expected_state_token &&
                x.expected_contribution_state == y.expected_contribution_state &&
                x.reversal_ref == y.reversal_ref,
            "Complete historical literal intent");
    original_claim_fixture::same_envelope(x.choices, y.choices);
    check_state(a.persisted_head, b.persisted_head);
    check_state(a.advanced_before, b.advanced_before);
    check_state(a.after, b.after);
    require(a.advance_explanation == b.advance_explanation &&
                a.contribution.has_value() == b.contribution.has_value() &&
                a.effects.size() == b.effects.size() && a.jobs.size() == b.jobs.size() &&
                a.transitions == b.transitions && a.reversals.empty() && b.reversals.empty(),
            "Complete historical literal relation and child sets");
    if (b.contribution)
        require(a.contribution->contribution_id == b.contribution->contribution_id &&
                    a.contribution->before == b.contribution->before &&
                    a.contribution->application_after == b.contribution->application_after,
                "Complete historical contribution dimensions");
    for (size_t n = 0; n < b.effects.size(); ++n)
        check_effect(a.effects[n], b.effects[n]);
    for (size_t n = 0; n < b.jobs.size(); ++n) {
        const auto &j = a.jobs[n], &k = b.jobs[n];
        require(j.job_ref == k.job_ref && j.source_request_ordinal == k.source_request_ordinal &&
                    j.group_ref == k.group_ref && j.group_ordinal == k.group_ordinal,
                "Complete historical synthetic job binding");
        original_claim_fixture::same_envelope(j.envelope, k.envelope);
    }
    original_claim_fixture::same_envelope(a.result, b.result);
    original_claim_fixture::same_envelope(a.audit.envelope, b.audit.envelope);
    require(a.audit.actor_ref == b.audit.actor_ref &&
                a.audit.decision_ref == b.audit.decision_ref && a.audit.reason == b.audit.reason,
            "Complete optional audit DATA");
}

struct Data {
    application_hold_fixture::Data catalog;
    QJsonObject frozen{original_claim_fixture::load(SUBATHON_SERVICE_VECTOR_PATH)};
    QJsonObject case_data(const QString &name) const {
        const auto c = frozen["cases"].toObject()[name].toObject();
        require(!c.isEmpty(), "Known frozen service case: " + name);
        return c;
    }
    void schema(const SqliteWorkSession &s) const { catalog.schema(s); }
    void retain_sources(const SqliteWorkSession &s) const {
        execute(s, frozen["source_setup"].toArray());
    }
    void populate_case(const SqliteWorkSession &s, const QString &name) const {
        execute(s, case_data(name)["setup"].toArray());
    }
    OriginalApplicationCommand command(const QString &name) const {
        return application_service_fixture::command(case_data(name)["command"]);
    }
    ApplicationStateSnapshot head(const QString &name) const {
        return state(case_data(name)["head"]);
    }
    BoundApplicationPolicy policy(const QString &name) const {
        return application_service_fixture::policy(case_data(name)["policy"]);
    }
    ApplicationClockObservation clock(const QString &name) const {
        return application_service_fixture::clock(case_data(name)["clock"]);
    }
    void populate_arrangement(const SqliteWorkSession &s, const QString &name) const {
        const auto a = frozen["arrangements"].toObject()[name].toObject();
        require(!a.isEmpty(), "Known independent service arrangement");
        execute(s, a["setup"].toArray());
    }
    application_hold_fixture::Data holds() const {
        auto d = catalog;
        d.frozen = frozen["hold_vectors"].toObject();
        return d;
    }
    void apply_fault(const SqliteWorkSession &s, const QString &name) const {
        const auto f = frozen["fault_data"].toObject()[name].toObject();
        require(!f.isEmpty(), "Known literal before-call fault DATA");
        execute(s, f["setup"].toArray());
    }
    std::vector<SqlRows> snapshot(Observer &observer) const {
        std::vector<SqlRows> result;
        for (const auto &q : frozen["snapshot_queries"].toArray())
            result.push_back(observer.rows(q.toObject()["sql"].toString()));
        return result;
    }
};

// Unlike success-only run(), this retains actual negative/unknown completion DATA.
template <class Body> SqliteWorkResult capture(SqliteWriter &writer, Body body) {
    struct Capture {
        bool done{false};
        SqliteWorkResult result;
    };
    const auto captured = std::make_shared<Capture>();
    require(writer.enqueue_work(std::move(body),
                                [captured](auto r) {
                                    captured->result = std::move(r);
                                    captured->done = true;
                                }) == StorageError::none,
            "Old writer admits explicit fixture work");
    require(QTest::qWaitFor([captured] { return captured->done; }, 10000),
            "Actual old-writer completion");
    return captured->result;
}
inline void open_sources(SqliteWriter &writer, const QString &path, const Data &data) {
    require(writer.open(path, limits()) == StorageError::none, "Old writer opens disposable file");
    (void)run(writer, [&](auto context) -> QVariant {
        const auto session = context.begin();
        data.schema(session);
        data.retain_sources(session);
        session.commit();
        return 1;
    });
}
inline void retain_case(SqliteWriter &writer, const Data &data, const QString &name) {
    (void)run(writer, [&](auto context) -> QVariant {
        const auto session = context.begin();
        data.populate_case(session, name);
        session.commit();
        return 1;
    });
}

// Synthetic scripted DATA only: no SQL/writer/session/reentrant mutation/read-view call.
// Output bindings are constructed from frozen inputs, never echoed from a UUT request.
class ScriptedPorts final : public ExecutionAccess,
                            public CurrentApplicationPolicy,
                            public SegmentedApplicationClock,
                            public RequiredApplicationActions,
                            public FreshApplicationReferences {
  public:
    OriginalApplicationCommand expected_command;
    AccessDecision access{ApplicationAccessAllowed{"grant"}};
    PolicyResolution bound_policy;
    ClockResolution observed_clock;
    ActionResolution prepared_actions{PreparedApplicationActions{}};
    bool application_visible{true}, holds_visible{true};
    std::vector<IssuedApplicationReference> issued;
    std::vector<std::string> calls;
    std::optional<ApplicationCalculatedProposal> received_proposal;

    ScriptedPorts(const Data &data, const QString &name)
        : expected_command(data.command(name)), bound_policy(data.policy(name)),
          observed_clock(data.clock(name)) {
        const auto c = data.case_data(name);
        if (c.contains("actions"))
            prepared_actions = actions(c["actions"]);
        reference_script(c["fake_references"].toArray(), text(c["head"].toObject()["token"]));
    }
    void reference_script(const QJsonArray &references, const std::string &persisted_head) {
        issued.clear();
        for (const auto &entry : references) {
            const auto r = entry.toObject();
            const auto purpose = static_cast<ApplicationReferencePurpose>(r["purpose"].toInt());
            ApplicationReferenceBinding binding{expected_command.operation,
                                                {expected_command.subject_kind,
                                                 expected_command.receipt_id,
                                                 expected_command.contribution_id},
                                                expected_command.plan,
                                                persisted_head,
                                                purpose,
                                                std::nullopt};
            if (!r["request_ordinal"].isNull())
                binding.request_ordinal = integer(r["request_ordinal"]);
            issued.push_back({std::move(binding), text(r["reference"])});
        }
    }
    AccessDecision check(const ApplicationReferenceReadView &,
                         const OriginalApplicationCommand &c) override {
        expect(c);
        calls.push_back("access");
        return access;
    }
    bool disclose_application(const ApplicationReferenceReadView &,
                              const OriginalApplicationCommand &c,
                              const RecordedApplicationBundle &) override {
        expect(c);
        calls.push_back("disclose_application");
        return application_visible;
    }
    bool disclose_holds(const ApplicationReferenceReadView &, const OriginalApplicationCommand &c,
                        const ApplicationHoldActiveSet &) override {
        expect(c);
        calls.push_back("disclose_holds");
        return holds_visible;
    }
    PolicyResolution resolve(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &,
                             const AccessDecision &) override {
        expect(c);
        calls.push_back("policy");
        return bound_policy;
    }
    ClockResolution observe(const ApplicationReferenceReadView &,
                            const OriginalApplicationCommand &c, const ApplicationBundleInputs &,
                            const BoundApplicationPolicy &) override {
        expect(c);
        calls.push_back("clock");
        return observed_clock;
    }
    ActionResolution prepare(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &,
                             const BoundApplicationPolicy &,
                             const ApplicationCalculatedProposal &p) override {
        expect(c);
        calls.push_back("actions");
        received_proposal = p;
        return prepared_actions;
    }
    ReferenceResolution issue(ApplicationReferencePurpose p,
                              const ApplicationReferenceBinding &binding) override {
        calls.push_back("reference:" + std::to_string(static_cast<int>(p)));
        for (const auto &r : issued) {
            const auto &b = r.binding;
            if (b.purpose == p &&
                b.operation.operation_scope == binding.operation.operation_scope &&
                b.operation.operation_ref == binding.operation.operation_ref &&
                b.source.subject_kind == binding.source.subject_kind &&
                b.source.receipt_id == binding.source.receipt_id &&
                b.source.contribution_id == binding.source.contribution_id &&
                b.plan.plan_ref == binding.plan.plan_ref &&
                b.plan.revision == binding.plan.revision &&
                b.persisted_head_token == binding.persisted_head_token &&
                b.request_ordinal == binding.request_ordinal && binding.purpose == p)
                return r;
        }
        return ApplicationUnavailable{"unscripted-reference"};
    }
    ApplicationServicePorts ports() { return {*this, *this, *this, *this, *this}; }

  private:
    void expect(const OriginalApplicationCommand &c) const {
        require(same_command(c, expected_command),
                "Script receives independent complete command DATA");
    }
};
} // namespace application_service_fixture
