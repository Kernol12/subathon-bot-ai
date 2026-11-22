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
    std::optional<Number> beside_complete_request{};
    std::string milestone_job_ref{"original-job"};
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

// Opt-in seed inputs for later-catalog composition tests. The v15 controls keep
// the same initial state, balances, catalog and envelope.
struct FileSeed {
    std::optional<MigrationManifest> manifest{};
    Number main_before{};
    Number debt_after{};
    Number score_before{};
    Number count_before{};
    std::optional<Number> other_score{};
    std::optional<Number> maximum_main{};
    bool omit_time{};
    std::function<ApplicationStateSnapshot(ApplicationStateSnapshot,
                                           std::optional<std::string>)> encode_state{};
};

class File final {
  public:
    explicit File(OriginalCredit credit = {}, FileSeed seed = {}) : seed_(std::move(seed)) {
        require(dir.isValid(), "Disposable database directory");
        open();
        auto manifest =
            seed_.manifest ? *seed_.manifest : financial_refunds_repository_manifest();
        version_ = manifest.steps.back().version;
        require(manifest.steps.size() == version_ &&
                    manifest.steps.at(14).id == "financial-refunds-v1",
                "Select the financial catalog and its exact additive prefix explicitly");
        const auto migration = completed<MigrationResult>([&](auto done) {
            return SqliteMigrationRunner::enqueue(
                       *writer, {MigrationMode::advance, manifest, {}, {}}, std::move(done))
                .error;
        });
        normal(migration.work);
        require(migration.reason == MigrationReason::none && migration.report &&
                    migration.report->state == CatalogState::current &&
                    migration.report->version == version_ && migration.report->applied == version_,
                "Real requested catalog selected on a fresh database");
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
        cid = link_support(support, original_state);
        std::optional<ContributionReceipt> prior_support;
        core::ContributionState prior_state;
        core::ContributionId prior_cid{};
        if (seed_.debt_after) {
            require(!credit.ready && seed_.main_before == 0 &&
                        seed_.debt_after <= std::numeric_limits<Number>::max() - credit.repayment,
                    "An earlier debt-only credit and the selected debt-plus-Main award both fit");
            prior_support = intake("prior-debt-support", core::ReceiptMoney{"1", "EUR"},
                                   "support", "earlier-debt-payment");
            prior_cid = link_support(*prior_support, prior_state);
        }
        const ApplicationPlanKey prior_plan{"earlier-debt-plan", 1};
        const ApplicationOperationKey prior_award{"test", "earlier-debt-award"};

        std::vector<core::RequestedEffect> requested;
        if (!seed_.omit_time)
            requested.push_back(request(Unit::milliseconds, credit.requested, "main"));
        if (credit.score != 0)
            requested.push_back(request(Unit::micro_points, credit.score, "score"));
        if (credit.count != 0)
            requested.push_back(request(Unit::whole_count, credit.count, "count"));
        if (credit.beside_complete_request)
            requested.push_back(
                request(Unit::milliseconds, *credit.beside_complete_request, "main"));
        support_capture = valued_capture(support, cid, original_state, std::move(requested));
        write([&](const auto &s) {
            ApplicationStateSnapshot before{
                "initial",
                document(),
                "cfg",
                timer(credit.ready ? Life::Ready : Life::Active, seed_.main_before),
                credit.repayment + seed_.debt_after,
                {{ApplicationBalanceKind::score, "score", seed_.score_before},
                 {ApplicationBalanceKind::whole_count, "count", seed_.count_before}}};
            if (seed_.other_score)
                before.balances.insert(before.balances.begin(),
                                       {ApplicationBalanceKind::score, "other-score",
                                        *seed_.other_score});
            before = encoded(std::move(before), {});
            SqliteApplicationBundleStore store(s);
            require(store.retain_initial_state(before) == SqliteBundleInitialDisposition::inserted,
                    "Retain a real initial state");
            if (prior_support) {
                // An older payment repays only its slice of outstanding debt.
                // The selected original then repays the final debt and adds Main.
                const auto q = request(Unit::milliseconds, seed_.debt_after, "main");
                retain_plan(s, prior_plan, valued_capture(*prior_support, prior_cid, prior_state, {q}));
                PreparedApplicationBundle earlier;
                earlier.intent = {prior_award, ApplicationOperationKind::original,
                                  ApplicationSubjectKind::contribution, prior_support->id,
                                  prior_cid, prior_plan, before.token, prior_state, document(), {}};
                earlier.advanced_before = before;
                earlier.after = before;
                earlier.after.token = "prior-debt-awarded";
                earlier.after.time_debt_ms = credit.repayment;
                earlier.after = encoded(std::move(earlier.after), before.token);
                earlier.effects = {addition(seed_.debt_after, 0, seed_.debt_after, 0)};
                earlier.audit = audit(prior_award.operation_ref);
                earlier.result = document();
                require(store.stage(earlier).disposition == SqliteBundleStageDisposition::staged,
                        "Commit real earlier debt-only credit with its own receipt and effect");
                before = earlier.after;
            }
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
            b.after.timer.main_remaining_ms += credit.main;
            b.after.time_debt_ms -= credit.repayment;
            b.after.balances.at(b.after.balances.size() - 2).units += credit.score;
            b.after.balances.back().units += credit.count;
            b.after = encoded(std::move(b.after), before.token);
            if (!seed_.omit_time) {
                b.effects.push_back(addition(credit.requested, credit.main, credit.repayment,
                                             credit.cap_tail));
                if (seed_.maximum_main)
                    b.effects.front().maximum_main_ms = seed_.maximum_main;
            }
            const auto &captured = support_capture.captured.requested_effects;
            for (std::size_t index = seed_.omit_time ? 0 : 1; index < captured.size(); ++index) {
                const auto &effect = captured[index];
                const auto ordinal = static_cast<Number>(index + 1);
                if (effect.unit == Unit::milliseconds) {
                    b.effects.push_back({ordinal,
                                         ApplicationArithmeticRole::time_reversal,
                                         effect.unit,
                                         effect.exact_requested_units,
                                         effect.requested_units,
                                         fraction(0),
                                         {},
                                         Policy::RequireReview,
                                         core::TimeReversalDisposition::RequiresReview,
                                         "beside-complete deficit",
                                         {{CK::main_delta, 0, {}},
                                          {CK::debt_delta, 0, {}},
                                          {CK::unapplied_time, effect.requested_units, {}}}});
                } else {
                    const auto kind =
                        effect.unit == Unit::micro_points ? CK::score : CK::whole_count;
                    b.effects.push_back(direct(ordinal, effect, kind));
                }
            }
            b.result = document();
            b.audit = audit("original");
            if (credit.milestone) {
                b.jobs.push_back({credit.milestone_job_ref,
                                  {},
                                  achievement.ref,
                                  1,
                                  {delivery_job_format, "synthetic-action"}});
                const auto result = SqliteDeliveryJobStore(s).stage(
                    b,
                    {{credit.milestone_job_ref, "local-fixture", DeliverySenderSelection::configured_default,
                      "fixture-sender", "achievement-action", "achievement-v1"}});
                require(result.disposition == SqliteBundleStageDisposition::staged,
                        "Real original and job capture staged in one unit");
                SqliteMilestoneGroupStore(s).stage(
                    original,
                    {{{{"target-35", "milestone-v1", 35000000, 40000000, "ctx"},
                       {{MilestoneActionKind::automation, "achievement-action", "achievement-v1"}},
                       {},
                       achievement},
                      {{1, {original, credit.milestone_job_ref}}}}});
            } else {
                require(store.stage(b).disposition == SqliteBundleStageDisposition::staged,
                        "Real original bundle staged");
            }
        });
        if (!credit.ready)
            original_state.conditions.application = core::ContributionApplication::applied;
        if (prior_support) {
            const auto ref = "restore-earlier-debt";
            const auto payment_ref = *prior_support->facts.payment_reference;
            const auto intake_refund = intake(ref, core::ReceiptMoney{"1", "EUR"}, "refund",
                                              payment_ref.opaque_id);
            prior_state.conditions.application = core::ContributionApplication::applied;
            const auto before = head();
            const FinancialRefundCommand command{{"test", ref},
                                                  intake_refund.id,
                                                  VerifiedRefundCorrelation{
                                                      {prior_support->identity.origin, payment_ref,
                                                       prior_support->id},
                                                      {"refund-v1", ref}, "verified-earlier-link",
                                                      prior_cid, prior_award, prior_plan},
                                                  {"earlier-debt-refund-plan", 1},
                                                  0, before.token, prior_state, principal, audit(ref)};
            const auto debt = seed_.debt_after;
            const FinancialEntitlement entitlement{
                0, 1, 1, true,
                GroupedRefundEntitlement{{prior_award, 1, CK::main_delta},
                                         1, 0, {}, {0, debt}, fraction(debt), 0, 0, debt},
                {}};
            Effects legacy;
            legacy.apply(0, debt, {0, debt});
            const auto restored = accept(command, entitlement, legacy);
            require(restored.timer == RefundTimerOutcome{TD::applied, {0, debt}} &&
                        restored.application.after.time_debt_ms == debt &&
                        restored.application.after.timer.main_remaining_ms ==
                            before.timer.main_remaining_ms,
                    "Earlier distinct payment restores debt through genuine grouped v1 writes");
            // v1 kept its old envelope. An independent state-only fixture
            // application installs a fresh coherent owner frame before live work.
            const auto r = intake("restore-owner-envelope", {}, "synthetic-owner-command");
            write([&](const auto &s) {
                auto bundle = receipt_application(s, r, "restore-owner-envelope");
                bundle.after = encoded(std::move(bundle.after), bundle.advanced_before.token);
                retain_plan(s, bundle.intent.plan, plain_capture(r, {}, {}));
                require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                            SqliteBundleStageDisposition::staged,
                        "Coherent current SAS1 on the committed Main plus restored debt");
            });
        }
    }

    void open(StorageLimits storage_limits = limits()) {
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, storage_limits) == StorageError::none, "Actual writer opens");
        observer = std::make_unique<SqliteReader>();
        require(observer->open(path) == StorageError::none, "Independent read-only observer opens");
    }

    void reopen(StorageLimits storage_limits = limits()) {
        auto closed = std::make_shared<bool>(false);
        require(writer->shutdown([closed] { *closed = true; }) == StorageError::none &&
                    QTest::qWaitFor([closed] { return *closed; }, 10000),
                "Old writer drains and closes before reopening");
        reopen_closed(storage_limits);
    }

    void reopen_closed(StorageLimits storage_limits = limits()) {
        observer.reset();
        writer.reset();
        open(storage_limits);
        require(rows("SELECT max(version) FROM subathon_migration_history") == SqlRows{{version_}},
                "Reopened database keeps the selected financial catalog");
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

    ApplicationStateSnapshot encoded(ApplicationStateSnapshot state,
                                     std::optional<std::string> basis) const {
        if (seed_.encode_state)
            return seed_.encode_state(std::move(state), std::move(basis));
        return state;
    }

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
                               const std::string &kind, const std::string &payment_ref = "p1") {
        const core::ReceiptInput input{
            {{"fixture-provider", "fixture-account"}, "delivery-v1", ref},
            {kind,
             1,
             45,
             {},
             {},
             std::move(money),
             core::ReceiptLinkReference{"payment-v1", payment_ref},
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

    core::ContributionId link_support(const ContributionReceipt &support,
                                      core::ContributionState &state) {
        auto &conditions = state.conditions;
        conditions.valuation = {true, support.facts.money, "quote", false};
        conditions.review = core::ContributionReview::not_required;
        conditions.evidence_admission = {
            core::EvidencePolicy::accept_provider_report, "policy", {}};
        conditions.settlement = core::SettlementGate::clear;
        state.payment_evidence = core::PaymentEvidence::confirmed;
        std::optional<core::ContributionId> linked_cid;
        const auto evidence = qt_test_support::run_work(
            *writer,
            [&](auto context) {
                SqliteContributionRepository repo(context);
                const auto linked = ContributionEvidenceService(repo).observe(
                    {support, ContributionObservationKind::support, state.payment_evidence,
                     VerifiedActivityLink{{support.identity.origin, "support-v1",
                                           support.identity.opaque_id == "support" ? "s1"
                                                                                  : support.identity.opaque_id},
                                          "proof"},
                     conditions});
                require(linked.disposition == ContributionLinkDisposition::created &&
                            linked.contribution_id,
                        "Real support observation created a contribution");
                linked_cid = linked.contribution_id;
                return 1;
            },
            10000);
        normal(evidence);
        return linked_cid.value();
    }

    static core::CapturedPlanRevision valued_capture(
        const ContributionReceipt &support, core::ContributionId subject,
        const core::ContributionState &state, std::vector<core::RequestedEffect> requested) {
        return {1,
                {},
                {event(support, subject), state,
                 {core::PlanValuationState::resolved,
                  core::HistoricalValuation{"quote", *support.facts.money, "CHF", "0.972500",
                                            "0.972500", "rate-snapshot", "historical-feed", 40, 41,
                                            {}}},
                 std::move(requested)},
                {{"historical-rule", "historical-rule-v1", true, "selected"}},
                {},
                {}};
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

  private:
    FileSeed seed_;
    qint64 version_{};

};
} // namespace financial_refund_fixture
