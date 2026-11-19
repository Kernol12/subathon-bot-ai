// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_test_support.hpp"
#include <QTemporaryDir>
#include <algorithm>
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_financial_refunds.hpp>
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

namespace financial_refund_fixture {
namespace core = subathon::core;
using namespace subathon::server;
using qt_test_support::require;
using D = FinancialDisposition;
using CK = ApplicationComponentKind;
using TD = RefundTimerDisposition;
using Policy = core::RefundDeficitPolicy;
using Life = core::TimerLifecycle;
using Transition = core::TimerTransition;
using Unit = core::RequestedUnit;
using Number = std::int64_t;

inline const ApplicationOperationKey original{"test", "original"};
inline const ApplicationPlanKey original_plan{"original-plan", 1};
inline const MilestoneOccurrenceKey achievement{"achieved-35"};
inline const ApplicationPrincipalBinding principal{ApplicationPrincipalKind::account, "install",
                                                   "tester"};

inline ApplicationEnvelope document(const char *value = "fixture") {
    return {"test.fixture", QByteArray(value)};
}
inline ApplicationAudit audit(const std::string &decision) {
    return {"tester", decision, "authorized synthetic test command", document()};
}
inline core::Exact fraction(Number n, Number d = 1) { return core::Exact(n) / d; }
inline QByteArray bytes(const std::string &s) { return QByteArray(s.data(), qsizetype(s.size())); }
inline bool same(const ApplicationOperationKey &a, const ApplicationOperationKey &b) {
    return a.operation_scope == b.operation_scope && a.operation_ref == b.operation_ref;
}
inline bool same(const ApplicationHoldVersionLocator &a, const ApplicationHoldVersionLocator &b) {
    return a.hold_ref == b.hold_ref && a.version_ref == b.version_ref;
}
inline core::TimerState timer(Life life, Number main) {
    return {life, core::CountdownPhase::Main, main, 120000, false};
}

struct Authority final : FinancialRefundAuthority {
    bool execution{true};
    bool disclosure{true};
    bool override_timer{true};

    bool allows(const SqliteWorkSession &, FinancialAuthorityUse use,
                const ApplicationPrincipalBinding &who, const ApplicationOperationKey &,
                const FinancialPaymentKey &) override {
        require(who.kind == ApplicationPrincipalKind::account &&
                    who.installation_ref == "install" && who.principal_ref == "tester",
                "Probe receives the selected installation/account");
        if (use == FinancialAuthorityUse::disclose)
            return disclosure;
        if (use == FinancialAuthorityUse::override_timer)
            return override_timer;
        return execution;
    }
};

// Only the #201 seam is modeled here. No financial or hold store writes occur in
// the port. Each call supplies literal clock and physical effects for one work.
struct Effects final : FinancialRefundEffectPort {
    std::optional<core::TimerState> observed;
    std::vector<Transition> transitions;
    std::optional<Number> hard_end;
    bool paused{false};
    Number elapsed{};
    std::optional<RefundTimerActual> actual;
    std::optional<std::pair<Number, Number>> requested;
    Policy expected_policy{Policy::RequireReview};
    bool milestone{false};
    int observations{};
    int adjustments{};
    int milestone_reads{};

    FinancialEffectObservation observe(const SqliteWorkSession &s,
                                       const ApplicationStateSnapshot &head) override {
        ++observations;
        const auto persisted = SqliteApplicationBundleStore(s).read_state();
        require(persisted && persisted->token == head.token,
                "Observation uses the same session and the selected current head");
        return {{"test-clock",
                 "observation-" + head.token,
                 head.configuration_ref,
                 "current-policy",
                 head.token,
                 100,
                 {{elapsed, 100, paused}}},
                {head.timer.awaiting_confirmation ? core::TimerEndMode::Confirmation
                                                  : core::TimerEndMode::Immediate,
                 120000, core::GraceReuse::Fresh, hard_end},
                observed.value_or(head.timer),
                transitions,
                Policy::RequireReview,
                900000,
                "new-rule-v9",
                "1.731200"};
    }

    RefundTimerActual timer_effect(const SqliteWorkSession &s, const GroupedRefundEntitlement &time,
                                   const FinancialEffectObservation &observation,
                                   Policy policy) override {
        ++adjustments;
        require(s.execute({"SELECT 1", {}}) == SqlRows{{1}}, "Active owner session at effect port");
        require(observation.observed_timer.lifecycle == Life::Active && actual && requested &&
                    requested == std::pair{time.main_request, time.restoration_request} &&
                    policy == expected_policy,
                "Only the selected whole active request and authorized policy can reach the effect "
                "port");
        return *actual;
    }

    std::vector<FinancialMilestoneTarget> milestones(const SqliteWorkSession &s) override {
        ++milestone_reads;
        require(s.execute({"SELECT 1", {}}) == SqlRows{{1}}, "Active session at milestone port");
        if (!milestone)
            return {};
        return {{achievement, 35000000, MilestoneRefundPolicy::review}};
    }

    void apply(Number main_request, Number restore_request, RefundTimerActual deltas,
               Policy policy = Policy::RequireReview) {
        requested = {main_request, restore_request};
        actual = deltas;
        expected_policy = policy;
    }
};

inline StorageLimits limits() {
    StorageLimits options;
    options.statements_per_batch = 16384;
    options.statement_utf8_bytes = 4194304;
    options.bound_value_bytes = 33554432;
    options.result_rows = 65536;
    options.result_value_bytes = 33554432;
    return options;
}

struct OriginalCredit {
    std::string amount{"20.0000"};
    Number requested{600000};
    Number main{300000};
    Number repayment{};
    Number cap_tail{300000};
    Number score{40000000};
    Number count{7};
    bool milestone{false};
    bool ready{false};
};

inline core::RequestedEffect request(Unit unit, Number units, std::string target) {
    return {target + "-action",
            "historical-action-v2",
            "historical-rule",
            std::move(target),
            unit,
            false,
            fraction(units),
            units,
            "independent integral input",
            "refund"};
}
inline ApplicationEffect direct(Number ordinal, const core::RequestedEffect &q, CK kind) {
    return {ordinal,
            ApplicationArithmeticRole::direct,
            q.unit,
            q.exact_requested_units,
            q.requested_units,
            fraction(0),
            {},
            {},
            {},
            "actual direct",
            {{kind, q.requested_units, q.target_ref}}};
}
inline ApplicationEffect addition(Number requested, Number main, Number repayment, Number tail) {
    return {1,
            ApplicationArithmeticRole::time_addition,
            Unit::milliseconds,
            fraction(requested),
            requested,
            fraction(0),
            tail ? std::optional{main} : std::nullopt,
            {},
            {},
            "actual time",
            {{CK::main_delta, main, {}},
             {CK::debt_delta, -repayment, {}},
             {CK::cap_limited_time, tail, {}}}};
}

template <class Result, class Start> Result completed(Start start) {
    auto result = std::make_shared<std::optional<Result>>();
    require(start([result](Result reply) { result->emplace(std::move(reply)); }) ==
                StorageError::none,
            "Real writer admits the selected operation");
    require(QTest::qWaitFor([result] { return result->has_value(); }, 10000),
            "Owner receives the completion");
    return std::move(result->value());
}

inline void normal(const SqliteWorkResult &result) {
    require(result.transaction == TransactionOutcome::committed &&
                result.error == WorkError::none && result.value,
            "Real acknowledged commit with normal work completion");
}

class File final {
  public:
    explicit File(OriginalCredit credit = {}) {
        require(dir.isValid(), "Disposable database directory");
        open();
        auto manifest = milestone_group_decisions_repository_manifest();
        require(manifest.steps.size() == 13 && manifest.steps.back().version == 13 &&
                    manifest.steps.back().id == "milestone-group-decisions-v1",
                "Select the shipped v13 prefix explicitly");
        const auto migration = completed<MigrationResult>([&](auto done) {
            return SqliteMigrationRunner::enqueue(
                       *writer, {MigrationMode::advance, manifest, {}, {}}, std::move(done))
                .error;
        });
        normal(migration.work);
        require(migration.reason == MigrationReason::none && migration.report &&
                    migration.report->state == CatalogState::current &&
                    migration.report->version == 13 && migration.report->applied == 13,
                "Real v13 catalog selected on a fresh database");
        write([](const auto &s) {
            SqliteRetainedContextStore retained(s);
            require(retained.retain({{RetainedKind::configuration, "cfg"}, "configuration", {}}) ==
                            RetainDisposition::inserted &&
                        retained.retain({{RetainedKind::arrival_context, "ctx"},
                                         "arrival",
                                         ConfigurationReference{"cfg"}}) ==
                            RetainDisposition::inserted,
                    "Retain immutable arrival/configuration context");
        });

        const auto support = intake("support", core::ReceiptMoney{credit.amount, "EUR"}, "support");
        payment = {support.identity.origin, {"payment-v1", "p1"}, support.id};
        auto &conditions = original_state.conditions;
        conditions.valuation = {true, support.facts.money, "quote", false};
        conditions.review = core::ContributionReview::not_required;
        conditions.evidence_admission = {
            core::EvidencePolicy::accept_provider_report, "policy", {}};
        conditions.settlement = core::SettlementGate::clear;
        original_state.payment_evidence = core::PaymentEvidence::confirmed;
        const auto evidence = qt_test_support::run_work(
            *writer,
            [&](auto context) {
                SqliteContributionRepository repo(context);
                const auto linked = ContributionEvidenceService(repo).observe(
                    {support, ContributionObservationKind::support, original_state.payment_evidence,
                     VerifiedActivityLink{{support.identity.origin, "support-v1", "s1"}, "proof"},
                     conditions});
                require(linked.disposition == ContributionLinkDisposition::created &&
                            linked.contribution_id,
                        "Real support observation created a contribution");
                cid = *linked.contribution_id;
                return 1;
            },
            10000);
        normal(evidence);

        std::vector requested{request(Unit::milliseconds, credit.requested, "main")};
        if (credit.score != 0)
            requested.push_back(request(Unit::micro_points, credit.score, "score"));
        if (credit.count != 0)
            requested.push_back(request(Unit::whole_count, credit.count, "count"));
        support_capture = {
            1,
            {},
            {event(support, cid),
             original_state,
             {core::PlanValuationState::resolved, core::HistoricalValuation{"quote",
                                                                            *support.facts.money,
                                                                            "CHF",
                                                                            "0.972500",
                                                                            "0.972500",
                                                                            "rate-snapshot",
                                                                            "historical-feed",
                                                                            40,
                                                                            41,
                                                                            {}}},
             std::move(requested)},
            {{"historical-rule", "historical-rule-v1", true, "selected"}},
            {},
            {}};
        write([&](const auto &s) {
            const ApplicationStateSnapshot before{
                "initial",
                document(),
                "cfg",
                timer(credit.ready ? Life::Ready : Life::Active, 0),
                credit.repayment,
                {{ApplicationBalanceKind::score, "score", 0},
                 {ApplicationBalanceKind::whole_count, "count", 0}}};
            SqliteApplicationBundleStore store(s);
            require(store.retain_initial_state(before) == SqliteBundleInitialDisposition::inserted,
                    "Retain a real initial state");
            retain_plan(s, original_plan, support_capture);
            if (credit.ready)
                return;
            PreparedApplicationBundle b;
            b.intent = {original,
                        ApplicationOperationKind::original,
                        ApplicationSubjectKind::contribution,
                        support.id,
                        cid,
                        original_plan,
                        before.token,
                        original_state,
                        document(),
                        {}};
            b.advanced_before = before;
            b.after = before;
            b.after.token = "awarded";
            b.after.timer.main_remaining_ms = credit.main;
            b.after.time_debt_ms = 0;
            b.after.balances[0].units = credit.score;
            b.after.balances[1].units = credit.count;
            b.effects = {
                addition(credit.requested, credit.main, credit.repayment, credit.cap_tail)};
            const auto &captured = support_capture.captured.requested_effects;
            for (std::size_t index = 1; index < captured.size(); ++index) {
                const auto &effect = captured[index];
                const auto kind = effect.unit == Unit::micro_points ? CK::score : CK::whole_count;
                b.effects.push_back(direct(static_cast<Number>(index + 1), effect, kind));
            }
            b.result = document();
            b.audit = audit("original");
            if (credit.milestone) {
                b.jobs.push_back({"original-job",
                                  {},
                                  achievement.ref,
                                  1,
                                  {delivery_job_format, "synthetic-action"}});
                const auto result = SqliteDeliveryJobStore(s).stage(
                    b,
                    {{"original-job", "local-fixture", DeliverySenderSelection::configured_default,
                      "fixture-sender", "achievement-action", "achievement-v1"}});
                require(result.disposition == SqliteBundleStageDisposition::staged,
                        "Real original and job capture staged in one unit");
                SqliteMilestoneGroupStore(s).stage(
                    original,
                    {{{{"target-35", "milestone-v1", 35000000, 40000000, "ctx"},
                       {{MilestoneActionKind::automation, "achievement-action", "achievement-v1"}},
                       {},
                       achievement},
                      {{1, {original, "original-job"}}}}});
            } else {
                require(store.stage(b).disposition == SqliteBundleStageDisposition::staged,
                        "Real original bundle staged");
            }
        });
        if (!credit.ready)
            original_state.conditions.application = core::ContributionApplication::applied;
    }

    void open() {
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, limits()) == StorageError::none, "Actual writer opens");
        observer = std::make_unique<SqliteReader>();
        require(observer->open(path) == StorageError::none, "Independent read-only observer opens");
    }

    void reopen() {
        auto closed = std::make_shared<bool>(false);
        require(writer->shutdown([closed] { *closed = true; }) == StorageError::none &&
                    QTest::qWaitFor([closed] { return *closed; }, 10000),
                "Old writer drains and closes before reopening");
        reopen_closed();
    }

    void reopen_closed() {
        observer.reset();
        writer.reset();
        open();
        require(rows("SELECT max(version) FROM subathon_migration_history") == SqlRows{{13}},
                "Reopened database keeps the v13 catalog");
    }

    template <class Body> void write(Body body) {
        normal(work([&](auto context) -> QVariant {
            const auto s = context.begin();
            body(s);
            s.commit();
            return 1;
        }));
    }

    SqliteWorkResult work(const SqliteWriter::Work &body, bool rethrow_body = true) {
        auto failure = std::make_shared<std::exception_ptr>();
        const auto result = qt_test_support::run_work(
            *writer,
            [failure, body](auto context) -> QVariant {
                try {
                    return body(context);
                } catch (...) {
                    *failure = std::current_exception();
                    throw;
                }
            },
            10000);
        if (*failure) {
            if (rethrow_body)
                std::rethrow_exception(*failure);
            // A named missing entry must remain distinguishable from a fault
            // expected later in the same work item.
            try {
                std::rethrow_exception(*failure);
            } catch (const SqliteFinancialRefundNotImplemented &) {
                throw;
            } catch (...) {
                // The caller asserts the actual durability and reached fault.
            }
        }
        return result;
    }

    core::ReceiptId refund(const std::string &id, std::optional<core::ReceiptMoney> money) {
        return intake(id, std::move(money), "refund").id;
    }

    FinancialRefundCommand command(const std::string &ref, core::ReceiptId id, Number position,
                                   const std::string &refund_ref = {}) {
        std::optional<ApplicationBundleInputs> selected;
        write([&](const auto &s) {
            const auto result = SqliteApplicationBundleStore(s).read_inputs(
                original_plan, ApplicationSubjectKind::contribution);
            require(result.disposition == SqliteBundleInputDisposition::ready && result.inputs &&
                        result.inputs->current_contribution,
                    "Select real contribution and head independently of the entitlement");
            selected = result.inputs;
        });
        return {{"test", ref},
                id,
                VerifiedRefundCorrelation{payment,
                                          {"refund-v1", refund_ref.empty() ? ref : refund_ref},
                                          "verified-link",
                                          cid,
                                          original,
                                          original_plan},
                {"capture-" + ref, 1},
                position,
                selected->current_state.token,
                selected->current_contribution->contribution.state,
                principal,
                audit(ref)};
    }

    FinancialTimerCommand continuation(const std::string &ref, const FinancialTimerHold &hold,
                                       std::vector<ApplicationHoldVersionLocator> active,
                                       std::optional<Policy> override_policy = {}) {
        std::sort(active.begin(), active.end(),
                  [](const auto &a, const auto &b) { return a.hold_ref < b.hold_ref; });
        return {{"test", ref}, payment,         hold.locator, std::move(active),
                head().token,  override_policy, principal,    audit(ref)};
    }

    FinancialRefundRecord accept(const FinancialRefundCommand &c, const FinancialEntitlement &p,
                                 Effects &effects, D disposition = D::applied) {
        const auto result = refund_reply(c, p, effects);
        require(result.disposition == disposition, "Expected acceptance/retry disposition");
        return std::get<FinancialRefundRecord>(result.record);
    }

    FinancialWorkReply refund_reply(const FinancialRefundCommand &c, const FinancialEntitlement &p,
                                    Effects &effects) {
        return run_reply(
            [&](const auto &s) { return stage_financial_refund(s, c, p, authority, effects); });
    }

    FinancialWorkReply continue_reply(const FinancialTimerCommand &c, Effects &effects) {
        return run_reply([&](const auto &s) {
            return stage_financial_timer_resolution(s, c, authority, effects);
        });
    }

    template <class Stage> FinancialWorkReply run_reply(Stage stage) {
        const auto result = work([&](auto context) -> QVariant {
            const auto s = context.begin();
            auto response = stage(s);
            s.commit();
            return QVariant::fromValue(std::move(response));
        });
        normal(result);
        auto response = acknowledged_financial_reply(result);
        require(response.has_value(), "Normal work publishes its matching typed reply");
        return std::move(*response);
    }

    std::optional<FinancialPaymentHistory> history() {
        std::optional<FinancialPaymentHistory> result;
        write([&](const auto &s) { result = SqliteFinancialRefundStore(s).history(payment, 16); });
        return result;
    }

    std::vector<FinancialTimerHold> financial_active() {
        std::vector<FinancialTimerHold> result;
        write([&](const auto &s) { result = SqliteFinancialRefundStore(s).active({cid}); });
        return result;
    }

    ApplicationStateSnapshot head() {
        std::optional<ApplicationStateSnapshot> state;
        write([&](const auto &s) { state = SqliteApplicationBundleStore(s).read_state(); });
        require(state.has_value(), "Real selected state head");
        return *state;
    }

    void check_head(Number main, Number debt, Number score, Number count,
                    Life life = Life::Active) {
        const auto state = head();
        require(state.timer.lifecycle == life && state.timer.main_remaining_ms == main &&
                    state.time_debt_ms == debt && state.balances.size() == 2 &&
                    state.balances[0].units == score && state.balances[1].units == count,
                "Committed lifecycle, physical timer/debt, score and count match the oracle");
    }

    void check_raw(core::ReceiptId id) {
        require(rows("SELECT count(*) FROM subathon_receipts WHERE receipt_id=?",
                     {encode_receipt_uint64(id.value)}) == SqlRows{{1}},
                "Raw intake remains committed");
    }

    SqlRows rows(const QString &sql, const QList<QVariant> &bindings = {}) {
        const auto result = observer->read({sql, bindings});
        require(result.error == StorageError::none, "Independent committed reader succeeds");
        return result.rows;
    }

    void move_main(const std::string &ref, Number main, std::optional<Life> finish = {},
                   const std::vector<Transition> &transitions = {}) {
        const auto receipt = intake(ref, {}, "authorized-fixture-command");
        write([&](const auto &s) {
            auto b = receipt_application(s, receipt, ref);
            if (main > b.advanced_before.timer.main_remaining_ms) {
                const auto credit = main - b.advanced_before.timer.main_remaining_ms;
                retain_plan(
                    s, b.intent.plan,
                    plain_capture(receipt, {}, {request(Unit::milliseconds, credit, "main")}));
                b.effects.push_back(addition(credit, credit, 0, 0));
            } else {
                retain_plan(s, b.intent.plan, plain_capture(receipt, {}, {}));
                b.advanced_before.token = ref + "-observation";
                b.advanced_before.timer.main_remaining_ms = main;
                if (finish)
                    b.advanced_before.timer.lifecycle = *finish;
                b.advanced_before.timer.awaiting_confirmation =
                    main == 0 && b.advanced_before.timer.lifecycle == Life::Active;
                b.advance_explanation = "literal authorized clock observation";
            }
            b.after.timer = b.advanced_before.timer;
            b.after.timer.main_remaining_ms = main;
            if (main > 0)
                b.after.timer.awaiting_confirmation = false;
            b.transitions = transitions;
            if (b.advanced_before.timer.awaiting_confirmation && b.transitions.empty())
                b.transitions = {Transition::MainExpired, Transition::ConfirmationRequired};
            require(SqliteApplicationBundleStore(s).stage(b).disposition ==
                        SqliteBundleStageDisposition::staged,
                    "Independent state movement is an actual original-kind writer operation");
        });
    }

    ApplicationHoldDetection ordinary_detection(const std::string &name, bool timer_hold) {
        const auto observation_token = head().token;
        ApplicationHoldCause c;
        c.hold_ref = name;
        c.source = {{cid}, payment.support_receipt, {}, {}, {}, "cfg", "ctx"};
        c.command = {{"test", name + "-command"}, document()};
        c.first_observation = {observation_token, document(),         "policy", "permissions",
                               "tester",          name + "-decision", 100,      0};
        if (timer_hold) {
            const ApplicationPlanKey plan{name + "-hold-plan", 1};
            const auto &e = support_capture.captured.event;
            const ContributionReceipt receipt{e.receipt_id, e.identity, e.facts, e.arrival};
            auto capture =
                plain_capture(receipt, cid, {request(Unit::milliseconds, -75000, "main")});
            capture.captured.event.event_ref = name + "-event";
            capture.captured.contribution_state = original_state;
            write([&](const auto &s) { retain_plan(s, plan, capture); });
            c.kind = ApplicationHoldCauseKind::timer_deficit;
            c.plan_evidence.plan = plan;
            c.plan_evidence.occurrences = {
                {1, {}, {}, {}, {}, Unit::milliseconds, fraction(-75000), -75000, {}}};
            c.timer = {1,
                       CK::unapplied_time,
                       -75000,
                       Policy::RequireReview,
                       core::TimeReversalDisposition::RequiresReview,
                       -75000,
                       ApplicationHoldComposition::defer_whole_input,
                       document(),
                       {}};
        } else {
            c.kind = ApplicationHoldCauseKind::unresolved_policy;
            c.missing_policy_reason = "independent unresolved policy";
            c.plan_evidence = {ApplicationHoldPlanKind::failed_capture,
                               {},
                               ApplicationHoldExtent::known_prefix,
                               ApplicationHoldStoppedFor::unresolved_capture_policy,
                               document(),
                               {}};
        }
        return {c, {name + "-detect", {}, "v1", c.first_observation, document()}};
    }

    ApplicationHoldVersionLocator detect(const ApplicationHoldDetection &d) {
        write([&](const auto &s) {
            require(SqliteApplicationHoldStore(s).record_detection(d).disposition ==
                        SqliteHoldMutationDisposition::recorded,
                    "Ordinary hold retained through the shipped store");
        });
        return {d.cause.hold_ref, "v1"};
    }

    ApplicationHoldApplicationLink ordinary_link(bool correction,
                                                 const ApplicationHoldVersionLocator &origin) {
        if (!correction)
            return {ApplicationHoldApplicationLinkKind::whole_input,
                    original,
                    payment.support_receipt,
                    original_plan,
                    {}};
        // Bind a complete receipt-kind correction of the independent ordinary
        // request. The signed original has its own receipt and no financial link.
        move_main("fund-ordinary-correction", 125000);
        const auto receipt = intake("ordinary-correction", {}, "authorized-fixture-command");
        write([&](const auto &s) {
            auto b = receipt_application(s, receipt, "ordinary-correction");
            const auto q = request(Unit::milliseconds, -75000, "main");
            retain_plan(s, b.intent.plan, plain_capture(receipt, {}, {q}));
            b.effects = {manual_subtraction(-75000)};
            b.after.timer.main_remaining_ms = 50000;
            require(SqliteApplicationBundleStore(s).stage(b).disposition ==
                        SqliteBundleStageDisposition::staged,
                    "Real distinct signed correction bundle for the exact ordinary origin");
        });
        return {ApplicationHoldApplicationLinkKind::timer_correction,
                {"test", "ordinary-correction"},
                receipt.id,
                {"ordinary-correction-plan", 1},
                ApplicationHoldHeldOrigin{origin, 1, CK::unapplied_time, -75000}};
    }

    PreparedApplicationBundle legacy_reversal(const std::string &ref, CK kind, bool full) {
        const auto receipt = intake(ref, {}, "nonfinancial-correction");
        PreparedApplicationBundle result;
        write([&](const auto &s) {
            const auto ordinal = kind == CK::main_delta ? 1 : kind == CK::score ? 2 : 3;
            const auto unit = kind == CK::main_delta ? Unit::milliseconds
                              : kind == CK::score    ? Unit::micro_points
                                                     : Unit::whole_count;
            const auto target = kind == CK::main_delta ? "main"
                                : kind == CK::score    ? "score"
                                                       : "count";
            const ApplicationOriginalComponent occurrence{original, ordinal, kind};
            const auto selected = SqliteApplicationBundleStore(s).read_reversal_basis(occurrence);
            require(selected.disposition == SqliteBundleBasisDisposition::ready && selected.basis,
                    "Read the shipped path's fresh exact basis position");
            const auto &basis = *selected.basis;
            const auto cumulative = full ? basis.actual_credit_units : Number{0};
            const auto increment = basis.progress.cumulative - cumulative;
            const auto q = request(unit, increment, target);
            result = receipt_application(s, receipt, ref);
            retain_plan(s, result.intent.plan, plain_capture(receipt, {}, {q}));
            result.intent.kind = ApplicationOperationKind::reversal;
            result.intent.reversal_ref = ref;
            if (kind == CK::main_delta) {
                result.effects.push_back(manual_subtraction(increment));
                result.after.timer.main_remaining_ms += increment;
            } else {
                result.effects.push_back(direct(1, q, kind));
                result.after.balances[kind == CK::score ? 0 : 1].units += increment;
            }
            result.reversals.push_back(
                {1, occurrence, basis.progress.position, basis.progress.cumulative, cumulative,
                 increment, fraction(full ? 1 : 0), fraction(cumulative), fraction(0), full});
        });
        return result;
    }

    QTemporaryDir dir;
    const QString path{dir.filePath("refund.sqlite")};
    std::unique_ptr<SqliteWriter> writer;
    std::unique_ptr<SqliteReader> observer;
    FinancialPaymentKey payment{};
    core::ContributionId cid{};
    core::ContributionState original_state;
    core::CapturedPlanRevision support_capture;
    Authority authority;

  private:
    static ApplicationEffect manual_subtraction(Number delta) {
        return {
            1,
            ApplicationArithmeticRole::time_reversal,
            Unit::milliseconds,
            fraction(delta),
            delta,
            fraction(0),
            {},
            Policy::StopAtZero,
            core::TimeReversalDisposition::Applied,
            "actual signed correction",
            {{CK::main_delta, delta, {}}, {CK::debt_delta, 0, {}}, {CK::unapplied_time, 0, {}}}};
    }

    ContributionReceipt intake(const std::string &ref, std::optional<core::ReceiptMoney> money,
                               const std::string &kind) {
        const core::ReceiptInput input{
            {{"fixture-provider", "fixture-account"}, "delivery-v1", ref},
            {kind,
             1,
             45,
             {},
             {},
             std::move(money),
             core::ReceiptLinkReference{"payment-v1", "p1"},
             {}},
            {50, "source-connection", "cfg", "ctx"}};
        std::optional<core::ReceiptId> id;
        normal(work([&](auto context) -> QVariant {
            SqliteReceiptRepository repo(context);
            const auto result = ReceiptIntake(repo).ingest(input);
            require(result.disposition == core::ReceiptDisposition::created,
                    "New raw synthetic receipt is committed separately");
            id = result.receipt_id;
            return 1;
        }));
        require(id.has_value(), "Real receipt work returned an ID");
        return {*id, input.identity, input.facts, input.arrival};
    }

    static core::PlanEvent event(const ContributionReceipt &r,
                                 std::optional<core::ContributionId> cid) {
        return {r.identity.opaque_id + "-event",
                {},
                r.identity.opaque_id + "-event",
                r.id,
                r.identity,
                r.facts,
                r.arrival,
                cid};
    }

    static core::CapturedPlanRevision plain_capture(const ContributionReceipt &r,
                                                    std::optional<core::ContributionId> cid,
                                                    std::vector<core::RequestedEffect> requests) {
        return {
            1,
            {},
            {event(r, cid), {}, {core::PlanValuationState::not_applicable, {}}, std::move(requests)},
            {},
            {},
            {}};
    }

    static void retain_plan(const SqliteWorkSession &s, const ApplicationPlanKey &key,
                            const core::CapturedPlanRevision &capture) {
        require(SqliteApplicationPlanStore(s).retain_initial(key.plan_ref, capture) ==
                    SqlitePlanInitialDisposition::inserted,
                "Real immutable plan capture retained");
    }

    static PreparedApplicationBundle receipt_application(const SqliteWorkSession &s,
                                                         const ContributionReceipt &r,
                                                         const std::string &ref) {
        const auto state = SqliteApplicationBundleStore(s).read_state();
        require(state.has_value(), "Prior real state for receipt-command fixture");
        PreparedApplicationBundle b;
        b.intent = {{"test", ref},
                    ApplicationOperationKind::original,
                    ApplicationSubjectKind::receipt,
                    r.id,
                    {},
                    {ref + "-plan", 1},
                    state->token,
                    {},
                    document(),
                    {}};
        b.advanced_before = *state;
        b.after = *state;
        b.after.token = ref + "-state";
        b.result = document();
        b.audit = audit(ref);
        return b;
    }
};
} // namespace financial_refund_fixture
