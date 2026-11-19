// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_modifier_activations_fixture.hpp"

#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

namespace modifier_activation_fixture {

inline const core::ReceiptInput receipt_a{
    {{"synthetic", "fixture-account"}, "events", "receipt-A"},
    {"synthetic-award", 1, {}, {}, {}, {}, {}, {}},
    {1000, "arrival-A", "cfg-arrival-A", "modifier-context-A"}};
inline const core::ReceiptInput receipt_b{
    {{"synthetic", "fixture-account"}, "events", "receipt-B"},
    {"synthetic-unrelated", 1, {}, {}, {}, {}, {}, {}},
    {1001, "arrival-B", "cfg-arrival-B", "modifier-context-B"}};
inline const core::ContributionState contribution_state{
    core::PaymentEvidence::not_applicable,
    {{false, {}, {}, false},
     core::ContributionReview::not_required,
     {core::EvidencePolicy::accept_provider_report, "fixture-policy", {}},
     core::ContributionApplication::not_applied,
     core::ContributionReversal::none,
     core::SettlementGate::clear}};
inline const core::CapturedPlanRevision plan_a{
    1,
    {},
    {{"event-A",
      {},
      "event-A",
      {7},
      receipt_a.identity,
      receipt_a.facts,
      receipt_a.arrival,
      core::ContributionId{0}},
     contribution_state,
     {core::PlanValuationState::not_applicable, {}},
     {{"action-A", "v1", "rule-A", "timer.main", core::RequestedUnit::milliseconds, false,
       core::Exact(120000), 120000, "60000 ms at factor 2.00", "reverse-actual"}}},
    {{"rule-A", "v1", true, "synthetic match"}},
    {{"z-old", "j1", "timer.main", "g-A", "multiply", "multiply", "2.00"}},
    {}};
inline const core::CapturedPlanRevision plan_b{1,
                                               {},
                                               {{"event-B-plan",
                                                 {},
                                                 "event-B-plan",
                                                 {8},
                                                 receipt_b.identity,
                                                 receipt_b.facts,
                                                 receipt_b.arrival,
                                                 {}},
                                                contribution_state,
                                                {core::PlanValuationState::not_applicable, {}},
                                                {}},
                                               {},
                                               {},
                                               {}};
inline const ApplicationOperationKey applied_a{"synthetic", "apply-A"};
inline const ApplicationStateSnapshot main_before{
    "main-before-A",
    {"state-v1", QByteArray("before-A")},
    "cfg-apply-A",
    {core::TimerLifecycle::Active, core::CountdownPhase::Main, 3540000, 120000, false},
    0,
    {}};
inline const ApplicationStateSnapshot main_after{
    "main-after-A",
    {"state-v1", QByteArray("after-A")},
    "cfg-apply-A",
    {core::TimerLifecycle::Active, core::CountdownPhase::Main, 3600000, 120000, false},
    0,
    {}};
inline const ApplicationEffect effect_a{1,
                                        ApplicationArithmeticRole::time_addition,
                                        core::RequestedUnit::milliseconds,
                                        core::Exact(120000),
                                        120000,
                                        core::Exact(0),
                                        3600000,
                                        {},
                                        {},
                                        "Main cap",
                                        {{ApplicationComponentKind::main_delta, 60000, {}},
                                         {ApplicationComponentKind::debt_delta, 0, {}},
                                         {ApplicationComponentKind::cap_limited_time, 60000, {}}}};
inline const ApplicationAudit audit_a{
    "operator-A", "decision-A", "synthetic reason", {"audit-v1", QByteArray("original-A")}};

inline void retain_receipt(File &file, const core::ReceiptInput &input, std::uint64_t id) {
    normal(work(*file.writer, [input, id](auto context) -> QVariant {
        SqliteReceiptRepository repository(context);
        const auto stored = ReceiptIntake(repository).ingest(input);
        require(stored.receipt_id == core::ReceiptId{id} &&
                    stored.disposition == core::ReceiptDisposition::created,
                "Committed receipt has its independently prescribed sequential ID");
        return 1;
    }));
}

// These records enter through existing persistence ports. Neither modifier
// selection nor requested/actual arithmetic is calculated by the fixture.
inline void seed_original_application(File &file) {
    (void)committed(*file.writer, [](const auto &s) {
        const SqliteRetainedContextStore retained(s);
        for (const auto *ref : {"cfg-arrival-A", "cfg-arrival-B", "cfg-apply-A"})
            require(retained.retain({{RetainedKind::configuration, ref}, QByteArray(ref), {}}) ==
                        RetainDisposition::inserted,
                    "Fixture configuration is retained");
        for (const auto *receipt : {&receipt_a, &receipt_b}) {
            const auto &arrival = receipt->arrival;
            require(retained.retain({{RetainedKind::arrival_context, arrival.modifier_context_ref},
                                     QByteArray("synthetic-context"),
                                     ConfigurationReference{arrival.configuration_ref}}) ==
                        RetainDisposition::inserted,
                    "Fixture context is linked to its arrival configuration");
        }
        return true;
    });
    for (std::uint64_t id = 1; id <= 6; ++id) {
        auto prefix = receipt_a;
        prefix.identity.opaque_id = "prefix-" + std::to_string(id);
        retain_receipt(file, prefix, id);
    }
    retain_receipt(file, receipt_a, 7);
    retain_receipt(file, receipt_b, 8);
    normal(work(*file.writer, [](auto context) -> QVariant {
        SqliteContributionRepository repository(context);
        auto tx = repository.begin();
        const ContributionReceipt source{
            {7}, receipt_a.identity, receipt_a.facts, receipt_a.arrival};
        require(tx->create(source, {}, contribution_state) == core::ContributionId{0},
                "The first nonmonetary contribution is CID zero");
        tx->append_observation({{source,
                                 ContributionObservationKind::support,
                                 core::PaymentEvidence::not_applicable,
                                 {},
                                 contribution_state.conditions},
                                core::ContributionId{0}});
        tx->commit();
        return 1;
    }));
    (void)committed(*file.writer, [](const auto &s) {
        const SqliteApplicationPlanStore plans(s);
        const SqliteApplicationBundleStore bundles(s);
        require(plans.retain_initial("plan-A", plan_a) == SqlitePlanInitialDisposition::inserted,
                "The fixture stores the original captured plan");
        require(bundles.retain_initial_state(main_before) ==
                    SqliteBundleInitialDisposition::inserted,
                "The fixture stores the pre-cap Main state");
        const PreparedApplicationBundle bundle{{applied_a,
                                                ApplicationOperationKind::original,
                                                ApplicationSubjectKind::contribution,
                                                {7},
                                                core::ContributionId{0},
                                                {"plan-A", 1},
                                                "main-before-A",
                                                contribution_state,
                                                {"choices-v1", QByteArray("choices-A")},
                                                {}},
                                               main_before,
                                               main_after,
                                               "",
                                               {effect_a},
                                               {},
                                               {},
                                               {},
                                               {"result-v1", QByteArray("result-A")},
                                               audit_a};
        require(bundles.stage(bundle).disposition == SqliteBundleStageDisposition::staged,
                "Existing bundle store stages the supplied, capped effect");
        return true;
    });
}

inline void require_effect(const ApplicationEffect &effect) {
    require(effect.request_ordinal == 1 &&
                effect.arithmetic_role == ApplicationArithmeticRole::time_addition &&
                effect.unit == core::RequestedUnit::milliseconds &&
                effect.exact_requested_units == core::Exact(120000) &&
                effect.requested_units == 120000 && effect.rounding_residual == core::Exact(0) &&
                effect.maximum_main_ms == 3600000 && !effect.deficit_policy &&
                !effect.deficit_disposition && effect.components.size() == 3,
            "Selected effect keeps the full exact request and cap");
    const auto &parts = effect.components;
    require(parts[0].kind == ApplicationComponentKind::main_delta && parts[0].units == 60000 &&
                parts[1].kind == ApplicationComponentKind::debt_delta && parts[1].units == 0 &&
                parts[2].kind == ApplicationComponentKind::cap_limited_time &&
                parts[2].units == 60000 && !parts[0].target_ref && !parts[1].target_ref &&
                !parts[2].target_ref,
            "Selected effect keeps actual credit, debt and cap-limited time separately");
}

inline void require_original(const SqliteWorkSession &s) {
    const SqliteApplicationPlanStore plans(s);
    const SqliteApplicationBundleStore bundles(s);
    const auto plan = plans.find_revision("plan-A", 1);
    require(plan && plan->plan_ref == "plan-A" && plan->revision == plan_a,
            "Selected plan retains original arrival, exact modifier refs, factor and request");
    const auto record = bundles.lookup(applied_a);
    require(record && record->intent.operation.operation_scope == "synthetic" &&
                record->intent.operation.operation_ref == "apply-A" &&
                record->intent.kind == ApplicationOperationKind::original &&
                record->intent.receipt_id == core::ReceiptId{7} &&
                record->intent.contribution_id == core::ContributionId{0} &&
                record->intent.plan.plan_ref == "plan-A" && record->intent.plan.revision == 1 &&
                record->advanced_before.token == "main-before-A" &&
                record->advanced_before.configuration_ref == "cfg-apply-A" &&
                record->advanced_before.timer.main_remaining_ms == 3540000 &&
                record->advanced_before.time_debt_ms == 0 &&
                record->after.token == "main-after-A" &&
                record->after.configuration_ref == "cfg-apply-A" &&
                record->after.timer.main_remaining_ms == 3600000 &&
                record->after.time_debt_ms == 0 && record->effects.size() == 1,
            "Selected original operation points to the exact source and before/after states");
    require_effect(record->effects[0]);
    require(record->audit.actor_ref == "operator-A" && record->audit.decision_ref == "decision-A" &&
                record->audit.reason == "synthetic reason" &&
                record->audit.envelope.format_ref == "audit-v1" &&
                record->audit.envelope.document == QByteArray("original-A"),
            "Original audit bytes and decision survive");
    const auto reversal =
        bundles.read_reversal_basis({applied_a, 1, ApplicationComponentKind::main_delta});
    require(reversal.disposition == SqliteBundleBasisDisposition::ready && reversal.basis &&
                reversal.basis->actual_credit_units == 60000 &&
                reversal.basis->selected_plan == plan_a &&
                reversal.basis->contribution_original_receipt_id == core::ReceiptId{7} &&
                reversal.basis->application_configuration_ref == "cfg-apply-A" &&
                reversal.basis->progress.position == 0 &&
                reversal.basis->progress.cumulative == 0 &&
                reversal.basis->progress.cumulative_fraction == core::Exact(0) &&
                !reversal.basis->progress.full_refund && !reversal.basis->progress.last,
            "Reversal basis is the immutable actual credit with no prior reversal");
}

inline void require_original_rows(const File &file) {
    const auto blob = [](const char *s) { return QByteArray(s); };
    const auto one = QByteArray::fromHex("0000000000000001");
    require(file.reads("SELECT receipt_id,provider,source_account,identifier_namespace,opaque_id,"
                       "event_type,schema_version,server_timestamp_utc_ms,connection_ref,"
                       "configuration_ref,modifier_context_ref FROM main.subathon_receipts "
                       "WHERE receipt_id IN (X'0000000000000007',X'0000000000000008') "
                       "ORDER BY receipt_id") ==
                SqlRows{{QByteArray::fromHex("0000000000000007"), blob("synthetic"),
                         blob("fixture-account"), blob("events"), blob("receipt-A"),
                         blob("synthetic-award"), 1, 1000, blob("arrival-A"), blob("cfg-arrival-A"),
                         blob("modifier-context-A")},
                        {QByteArray::fromHex("0000000000000008"), blob("synthetic"),
                         blob("fixture-account"), blob("events"), blob("receipt-B"),
                         blob("synthetic-unrelated"), 1, 1001, blob("arrival-B"),
                         blob("cfg-arrival-B"), blob("modifier-context-B")}},
            "Independent receipt rows retain exact arrivals A and B");
    require(
        file.reads("SELECT plan_ref,revision,ordinal,definition_version_ref,instance_revision_ref,"
                   "target_ref,group_ref,group_operator,target_operator,original_factor_decimal "
                   "FROM main.subathon_plan_modifiers ORDER BY plan_ref,revision,ordinal") ==
            SqlRows{{blob("plan-A"), one, 1, blob("z-old"), blob("j1"), blob("timer.main"),
                     blob("g-A"), blob("multiply"), blob("multiply"), blob("2.00")}},
        "Independent plan modifier row retains exact selected bytes");
    require(file.reads("SELECT plan_ref,revision,ordinal,action_ref,action_version_ref,rule_ref,"
                       "target_ref,exact_numerator,exact_denominator,requested_units "
                       "FROM main.subathon_plan_requests ORDER BY plan_ref,revision,ordinal") ==
                SqlRows{{blob("plan-A"), one, 1, blob("action-A"), blob("v1"), blob("rule-A"),
                         blob("timer.main"), blob("120000"), blob("1"), 120000}},
            "Independent plan request row keeps its action and exact requested time");
    require(
        file.reads("SELECT operation_scope,operation_ref,receipt_id,contribution_id,plan_ref,"
                   "plan_revision,before_token,after_token FROM main.subathon_app_operations") ==
            SqlRows{{blob("synthetic"), blob("apply-A"), QByteArray::fromHex("0000000000000007"),
                     QByteArray::fromHex("0000000000000000"), blob("plan-A"), one,
                     blob("main-before-A"), blob("main-after-A")}},
        "Independent operation row retains its exact receipt, CID and state lineage");
    require(
        file.reads("SELECT request_ordinal,exact_numerator,exact_denominator,requested_units,"
                   "residual_numerator,residual_denominator,maximum_main_ms "
                   "FROM main.subathon_app_effects") ==
                SqlRows{{1, blob("120000"), blob("1"), 120000, blob("0"), blob("1"), 3600000}} &&
            file.reads(
                "SELECT component_kind,units,target_ref IS NULL FROM main.subathon_app_components "
                "ORDER BY component_kind") == SqlRows{{0, 60000, 1}, {1, 0, 1}, {5, 60000, 1}},
        "Independent effect rows distinguish exact request, actual and cap components");
    require(file.reads("SELECT actor_ref,decision_ref,reason,format_ref,document "
                       "FROM main.subathon_app_audit") ==
                SqlRows{{blob("operator-A"), blob("decision-A"), blob("synthetic reason"),
                         blob("audit-v1"), blob("original-A")}},
            "Independent audit row preserves the original bytes");
}
} // namespace modifier_activation_fixture
