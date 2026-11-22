// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../storage_026/catalog_fixture.hpp"
#include "../support/contribution_review_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "missing_capture.hpp"
#include <subathon/server/sqlite_connector_intake.hpp>

namespace modifier_005_fixture {
namespace review = contribution_review_fixture;
namespace activation = modifier_activation_fixture;
namespace finance = review::finance;
namespace core = subathon::core;
using namespace subathon::server;
using qt_test_support::require;

using Failure = ModifierCaptureRefusal;
using Kind = core::ModifierTargetKind;
using Operator = core::ModifierStackOperator;
using ReceiptDisposition = core::ReceiptDisposition;

inline const ModifierTraceProfile profile{"fixture-modifier", "profile-A"};
inline const core::ModifierStackLimits limits{262144, 262144, 16384};
inline const core::ModifierTargetKey main_target{Kind::added_time, "timer.main"};
inline const core::ModifierTargetKey counter_target{Kind::counter, "counter.synthetic"};
inline const core::ModifierOccurrenceKey original_main{"I", "j1", "z-old", 1};
inline const core::ModifierOccurrenceKey original_counter{"I", "j1", "z-old", 2};
inline const core::ModifierOccurrenceKey replacement_main{"J", "k0", "a-new", 1};
inline const std::vector<ModifierBaseEffect> bases{
    {1, "time-A", "v1", "rule-A", main_target, false, core::Exact{60000}, "reverse-actual"}};
inline const ApplicationOperationKey bundle_key{
    "review-application",
    "review-8667e356b837986403e69d83bc64efa15813405e28a5d3274e4ea932cd5155d9"};
inline const core::PlanOverrideDecision authorized{core::OverrideAuthority::authorized,
                                                   "reviewer-A", "Use qualified replacement bonus",
                                                   "override-A-2"};

inline RetainedKey context_key(std::string ref) {
    return {RetainedKind::arrival_context, std::move(ref)};
}
inline RetainedDocument document(std::string ref, std::string config, QByteArray bytes) {
    return {context_key(std::move(ref)), std::move(bytes),
            ConfigurationReference{std::move(config)}};
}

inline core::ModifierTargetPolicy target(core::ModifierTargetKey key, std::string group,
                                         core::ModifierOccurrenceKey member) {
    return {std::move(key),
            Operator::multiply,
            {{std::move(group), Operator::multiply, {}, {{std::move(member), {}}}}}};
}
inline QualifiedModifierCut original_cut() {
    return {{"lineage-A", "clock-A", 1000},
            0,
            profile,
            {"policy-A",
             "cfg-A",
             {target(main_target, "g-main", original_main),
              target(counter_target, "g-counter", original_counter)}},
            {"payment"},
            {{{"I", "j1"}, "z-old", 180000}},
            {}};
}
inline QualifiedModifierCut override_cut() {
    return {{"lineage-A", "clock-override-A", 620000},
            0,
            profile,
            {"policy-override",
             "cfg-override-A",
             {target(main_target, "g-override", replacement_main)}},
            {"payment"},
            {{{"J", "k0"}, "a-new", 600000}},
            {}};
}
inline ModifierArrivalInput arrival_a() {
    return {{{"synthetic", "account-A"}, "event-v1", "receipt-A"},
            {1000, "conn-A", "cfg-A", "ctx-A"},
            {"event-A", "event-A", {}},
            original_cut()};
}
inline core::ReceiptFacts award(std::uint32_t version = 1) {
    return {"synthetic-award", version, {}, {}, {}, {}, {}, {}};
}
inline core::PlanEvent plan_event(const ModifierArrivalInput &input, const ContributionReceipt &r,
                                  std::optional<core::ContributionId> cid) {
    return {input.event.event_ref,
            input.event.parent_ref,
            input.event.root_ref,
            r.id,
            r.identity,
            r.facts,
            r.arrival,
            cid};
}

inline PreparedModifierArrival prepare(const ModifierArrivalInput &input) {
    auto result = prepare_modifier_arrival(input, limits);
    require(std::holds_alternative<PreparedModifierArrival>(result), "Arrival prepared");
    auto prepared = std::get<PreparedModifierArrival>(std::move(result));
    require(prepared.retained.key == context_key(input.arrival.modifier_context_ref) &&
                prepared.retained.configuration ==
                    ConfigurationReference{input.arrival.configuration_ref} &&
                prepared.retained.document.startsWith(QByteArray("MCA1\0", 5)),
            "Observed delivery binds its own MCA1 role-0 key and typed configuration");
    return prepared;
}

// Preserve the exception raised on the writer owner, and observe the writer's
// independent completion even when the caller deliberately throws after commit.
template <class Body> auto run(review::CatalogFile &db, Body body) {
    std::exception_ptr failure;
    auto result = qt_test_support::run_work(
        db.writer(),
        [&](auto context) -> QVariant {
            try {
                body(context);
                return 1;
            } catch (...) {
                failure = std::current_exception();
                throw;
            }
        },
        10000);
    return std::pair{std::move(result), failure};
}
inline void normal(const std::pair<SqliteWorkResult, std::exception_ptr> &out) {
    if (out.second)
        std::rethrow_exception(out.second);
    finance::normal(out.first);
}

struct Case {
    review::CatalogFile db{review::profile()};

    Case() {
        db.advance_literal();
        db.reopen(storage_026_fixture::migration_profile());
        const auto catalog = connector_intake_repository_manifest();
        review::report(review::migrate(db.writer(), {MigrationMode::advance, catalog, {}, {}}),
                       CatalogState::current, 20, 20, 1);
        db.reopen(review::profile());
        require(db.rows(storage_026_fixture::history_query) ==
                    storage_026_fixture::expected_history(storage_026_fixture::successor()),
                "Protected v20 catalog matches the independent #760 prefix");
    }

    template <class Read> auto read(Read body) {
        std::optional<std::invoke_result_t<Read, const SqliteWorkSession &>> answer;
        db.write([&](const auto &s) { answer.emplace(body(s)); });
        return std::move(answer.value());
    }

    void config(std::string ref) {
        db.write([&](const auto &s) {
            require(SqliteRetainedContextStore(s).retain({{RetainedKind::configuration, ref},
                                                          QByteArray::fromStdString(ref + "-data"),
                                                          {}}) == RetainDisposition::inserted,
                    "Configuration committed by its owner with non-null literal bytes");
        });
    }

    void initial_main() {
        config("cfg-apply-A");
        db.write([](const auto &s) {
            require(SqliteApplicationBundleStore(s).retain_initial_state(
                        {"main-before-A",
                         finance::document(),
                         "cfg-apply-A",
                         finance::timer(finance::Life::Active, 3540000),
                         0,
                         {}}) == SqliteBundleInitialDisposition::inserted,
                    "Application owner stages the exact initial head");
        });
    }

    void append_old() {
        db.write([](const auto &s) {
            const auto old =
                SqliteModifierActivationStore(s).append_definition(activation::old_definition, {});
            require(old.definition == activation::old_definition && old.position == 1,
                    "Protected owner appends D/z-old");
        });
    }

    void live_original() {
        append_old();
        db.write([](const auto &s) {
            auto initial = SqliteModifierActivationStore(s).activate(
                "D", "z-old", {}, {"I", "j0", activation::moderator, {}});
            require(initial.revision_to_append == activation::initial_instance,
                    "Protected owner activates I/j0");
        });
        db.write([](const auto &s) {
            const auto next =
                SqliteModifierActivationStore(s).checkpoint_remaining({"I", "j0"}, "j1", 180000);
            require(next.instance == activation::checkpointed_instance,
                    "Separate ordered owner action retains I/j1 at 180000");
        });
    }

    void replace() {
        db.write([](const auto &s) {
            auto next = SqliteModifierActivationStore(s).append_definition(
                activation::new_definition, "z-old");
            require(next.definition == activation::new_definition && next.position == 2,
                    "Later definition D/a-new commits before activation");
        });
        db.write([](const auto &s) {
            const auto result = SqliteModifierActivationStore(s).activate(
                "D", "a-new", ModifierInstanceKey{"I", "j1"},
                {"J", "k0", activation::rule, activation::subsequent});
            require(result.for_trigger == activation::checkpointed_instance &&
                        result.revision_to_append == activation::k0 &&
                        result.superseded_instance_ref == "I",
                    "Subsequent Only preserves the old trigger view and replaces the live head");
        });
    }

    void expire() {
        db.write([](const auto &s) {
            const auto result =
                SqliteModifierActivationStore(s).checkpoint_remaining({"J", "k0"}, "k1", 0);
            require(result.instance == activation::k1 &&
                        result.lifetime == ModifierInstanceLifetime::expired,
                    "Same writer terminates replacement at exact zero");
        });
        db.reopen();
    }

    std::optional<RetainedDocument> retained(const std::string &ref) {
        return read(
            [&](const auto &s) { return SqliteRetainedContextStore(s).find(context_key(ref)); });
    }

    core::ReceiptIntakeResult intake(const ModifierArrivalInput &input,
                                     const core::ReceiptFacts &facts,
                                     const PreparedModifierArrival &prepared) {
        std::optional<core::ReceiptIntakeResult> result;
        normal(run(db, [&](auto ctx) {
            auto s = ctx.begin();
            require(retain_modifier_arrival(s, prepared) == RetainDisposition::inserted,
                    "Modifier adapter stages context in the caller's active unit");
            require(SqliteRetainedContextStore(s).find(prepared.retained.key) == prepared.retained,
                    "Only the active transaction sees its staged exact bytes and link");
            SqliteReceiptRepository owner(adopt_active_receipt_session, s);
            result = ReceiptIntake(owner).ingest({input.identity, facts, input.arrival});
            // No use of the handed-off ticket after the receipt owns the unit.
        }));
        require(retained(input.arrival.modifier_context_ref) == prepared.retained,
                "The same adopted commit publishes exact context");
        return result.value();
    }

    std::optional<core::ReceiptRecord> receipt(const core::ReceiptIdentity &identity) {
        std::optional<core::ReceiptRecord> result;
        normal(run(db, [&](auto ctx) {
            SqliteReceiptRepository repo(ctx);
            auto read = repo.begin();
            result = read->find(identity);
            read->commit();
        }));
        return result;
    }

    ContributionReceipt original(const ModifierArrivalInput &input,
                                 const core::ReceiptFacts &facts) {
        require(intake(input, facts, prepare(input)).disposition == ReceiptDisposition::created,
                "First receipt is created by the adopted owner");
        auto row = receipt(input.identity);
        const std::vector<core::ReceiptAttempt> attempts{
            {input.arrival, facts, ReceiptDisposition::created}};
        require(row && row->id.value == 1 && row->original_facts == facts &&
                    row->original_arrival == input.arrival && row->attempts == attempts,
                "Receipt 1 and attempt 1 retain the same exact input");
        return {row->id, row->identity, row->original_facts, row->original_arrival};
    }

    std::pair<core::ContributionId, core::ContributionState>
    support(const ContributionReceipt &r, std::string activity = "support-A",
            std::string proof = "proof-A") {
        core::ContributionState state;
        state.payment_evidence = core::PaymentEvidence::confirmed;
        state.conditions.valuation = {
            r.facts.money.has_value(), r.facts.money, {}, r.facts.money.has_value()};
        state.conditions.review = core::ContributionReview::pending;
        state.conditions.evidence_admission = {
            core::EvidencePolicy::accept_provider_report, "evidence-A", {}};
        state.conditions.settlement = core::SettlementGate::clear;
        const VerifiedActivityLink link{{r.identity.origin, "fixture-support", std::move(activity)},
                                        std::move(proof)};
        normal(run(db, [&](auto ctx) {
            SqliteContributionRepository repo(ctx);
            auto result = ContributionEvidenceService(repo).observe(
                {r, ContributionObservationKind::support, state.payment_evidence, link,
                 state.conditions});
            require(result.disposition == ContributionLinkDisposition::created &&
                        result.contribution_id == core::ContributionId{0},
                    "Separate evidence owner allocates present CID 0");
        }));
        normal(run(db, [&](auto ctx) {
            SqliteContributionRepository repo(ctx);
            auto reader = repo.begin();
            const auto stored = reader->find_activity(link.identity);
            const auto observation = reader->find_receipt(r.id);
            require(stored && stored->id.value == 0 && stored->original_receipt_id == r.id &&
                        stored->original_facts == r.facts &&
                        stored->original_arrival == r.arrival && stored->state == state &&
                        observation && observation->contribution_id == core::ContributionId{0} &&
                        observation->observation.receipt == r &&
                        observation->observation.verified_activity &&
                        observation->observation.verified_activity->identity == link.identity &&
                        observation->observation.verified_activity->verification_ref ==
                            link.verification_ref,
                    "Committed CID 0 retains its support observation and original anchor");
            reader->commit();
        }));
        return {{0}, state};
    }

    ModifierCaptureSelection
    select(const ContributionReceipt &r, const IssuedModifierEvent &event,
           std::optional<core::ContributionId> cid = {},
           std::optional<ModifierDecisionMode> mode = {}, ModifierExactInstanceRead resolver = {},
           std::optional<std::span<const core::ModifierSelection>> rows = {}) {
        return read([&](const auto &s) {
            return select_modifier_capture(s, r, event, cid, mode, profile, limits, resolver, rows);
        });
    }

    void no_application() {
        require(db.rows("SELECT operation_ref FROM main.subathon_app_operations").isEmpty() &&
                    db.rows("SELECT subject_id FROM main.subathon_app_claims").isEmpty() &&
                    db.rows("SELECT request_ordinal FROM main.subathon_app_effects").isEmpty() &&
                    db.rows("SELECT job_ref FROM main.subathon_app_jobs").isEmpty(),
                "No actual operation, claim, effect or job");
    }
};

inline ModifierStackEvaluation
evaluate(const SelectedModifierCapture &selected,
         std::span<const ModifierBaseEffect> actions = bases,
         std::optional<std::span<const core::ModifierSelection>> rows = {},
         std::span<const ModifierTraceLink> links = {},
         const std::optional<ModifierDecisionMode> &mode = {}) {
    std::vector<SelectedModifierRevision> exact;
    for (const auto &r : selected.eligible)
        exact.push_back({r.observed.key.instance_ref, r.observed.key.revision_ref,
                         r.observed.definition_revision_ref, &r.exact_instance,
                         &r.exact_definition});
    std::vector<std::string_view> qualifiers(selected.matched_qualifiers.begin(),
                                             selected.matched_qualifiers.end());
    std::optional<ModifierRecalculation> recalculation;
    if (mode)
        recalculation = ModifierRecalculation{mode->plan_ref, mode->revision, mode->decision};
    if (!rows && selected.recorded_selection) {
        rows = std::span(*selected.recorded_selection);
        links = selected.recorded_links;
    }
    return evaluate_captured_modifiers(
        {selected.event.arrival.modifier_context_ref, selected.event.arrival, recalculation,
         &selected.context, &profile, &selected.policy, std::span(exact), qualifiers, rows, links},
        actions, limits);
}

inline SelectedModifierCapture selected(ModifierCaptureSelection result,
                                        const core::PlanEvent &event) {
    require(std::holds_alternative<SelectedModifierCapture>(result),
            "Bound original/decision selects");
    auto value = std::get<SelectedModifierCapture>(std::move(result));
    require(value.event == event, "Exact issuer ancestry, original receipt/facts/arrival and CID");
    return value;
}

inline ModifierStackProposal proposal(ModifierStackEvaluation evaluation, std::int64_t expected) {
    require(std::holds_alternative<ModifierStackProposal>(evaluation),
            "Shipped #219 computes proposal");
    auto result = std::get<ModifierStackProposal>(std::move(evaluation));
    require(result.effects.size() == 1 && result.effects[0].base == bases[0] &&
                result.effects[0].exact_units == core::Exact{expected} &&
                result.effects[0].rounded_units == expected,
            "One exact Main request, rounded once, absent counter base");
    return result;
}

struct RuleSource final : core::RulePlanSource {
    std::vector<core::RuleSelection> capture(const core::PlanEvent &) override {
        return {{"rule-A", "v1", true, "synthetic-award v1"}};
    }
};
struct ModifierSource final : core::ModifierPlanSource {
    const core::PlanEvent &event;
    const std::vector<core::ModifierSelection> &rows;
    ModifierSource(const core::PlanEvent &e, const std::vector<core::ModifierSelection> &r)
        : event(e), rows(r) {}
    std::vector<core::ModifierSelection> capture(const core::PlanEvent &supplied) override {
        require(supplied == event, "Plan source consumes the evaluator's same selected event view");
        return rows;
    }
};
inline core::RequestedEffect request(const ModifierScaledEffect &effect) {
    const auto &base = effect.base;
    return {base.action_ref,
            base.action_version_ref,
            base.rule_ref,
            base.target.ref,
            core::RequestedUnit::milliseconds,
            base.requires_historical_valuation,
            effect.exact_units,
            effect.rounded_units.value(),
            "qualified modifier request",
            base.reversal_behavior_ref};
}

inline void expect_original(const SelectedModifierCapture &value) {
    require(value.context.document_ref == "ctx-A" && value.context.configuration_ref == "cfg-A" &&
                value.context.policy_ref == "policy-A" && value.context.profile == profile &&
                !value.context.recalculation &&
                value.matched_qualifiers == std::vector<std::string>{"payment"} &&
                value.eligible.size() == 1 &&
                value.eligible[0].observed == original_cut().eligible[0] &&
                value.eligible[0].exact_instance == activation::checkpointed_instance &&
                value.eligible[0].exact_definition == activation::old_definition,
            "Original view is complete I/j1/D/z-old with 180000 observed milliseconds");
}
inline ModifierStackProposal original_proposal(const SelectedModifierCapture &value) {
    const std::vector<core::ModifierSelection> expected{
        {"z-old", "j1", "timer.main", "g-main", "multiply", "multiply", "2.00"},
        {"z-old", "j1", "counter.synthetic", "g-counter", "multiply", "multiply", "1.250"}};
    const std::vector<ModifierTraceLink> links{{1, original_main}, {2, original_counter}};
    auto result = proposal(evaluate(value), 120000);
    require(result.selection == expected && result.links == links && result.targets.size() == 2 &&
                result.targets[1].target == counter_target &&
                result.targets[1].reduction.factor == core::Exact{5} / core::Exact{4},
            "Ordered immutable Main and unused counter provenance, including factor spelling");
    const auto verified = proposal(evaluate(value, bases, std::span(expected), links), 120000);
    require(verified.selection == expected && verified.links == links,
            "Shipped evaluator verifies the independently supplied persisted rows and ordinals");
    auto crossed = links;
    std::swap(crossed[0].occurrence, crossed[1].occurrence);
    require(std::get<core::ModifierStackFailure>(evaluate(
                value, bases, std::span(expected), crossed)) == core::ModifierStackFailure::invalid,
            "Row ordinals do not authorize different occurrence keys");
    return result;
}

inline core::ApplicationPlan initial_plan(Case &c, const SelectedModifierCapture &value,
                                          const core::ContributionState &state) {
    expect_original(value);
    const auto p = original_proposal(value);
    RuleSource rules;
    ModifierSource mods(value.event, p.selection);
    auto plan = core::capture_application_plan("plan-A",
                                               {value.event,
                                                state,
                                                {core::PlanValuationState::not_applicable, {}},
                                                {request(p.effects[0])}},
                                               rules, mods);
    c.db.write([&](const auto &s) {
        require(SqliteApplicationPlanStore(s).retain_initial("plan-A", plan.revisions[0]) ==
                    SqlitePlanInitialDisposition::inserted,
                "Shipped plan owner retains exactly revision 1 before application");
    });
    require(c.read([](const auto &s) {
                 return SqliteApplicationPlanStore(s).find_revision("plan-A", 1);
             })
                    .value()
                    .revision == plan.revisions[0],
            "Revision 1 persists with immutable event, rows and request");
    return plan;
}

} // namespace modifier_005_fixture
