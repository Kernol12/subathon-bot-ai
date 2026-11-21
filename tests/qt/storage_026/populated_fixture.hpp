// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "catalog_fixture.hpp"
#include <subathon/server/sqlite_account_sessions.hpp>

namespace storage_026_fixture {

// Same synthetic authority and real producer dependencies as the pinned Review
// specimen, with the shipped compiled v19 catalog used for the upgrade origin.
struct Populated {
    CatalogFile db;
    Facts facts;
    Trace trace;

    Populated(std::int64_t main = 95, std::int64_t debt = 0, std::int64_t score = 90,
              std::int64_t count = 3) {
        const auto shipped = contribution_review_repository_manifest();
        report(migrate(db.writer(), request(MigrationMode::advance, shipped)),
               CatalogState::current, 19, 19, 1);
        require(db.rows(history_query) == expected_history(literal_v19()),
                "Shipped v19 matches the independent protected history");
        db.write([&](const auto &session) {
            SqliteRetainedContextStore retained(session);
            require(
                retained.retain(
                    {{RetainedKind::configuration, "cfg"}, "synthetic review configuration", {}}) ==
                        RetainDisposition::inserted &&
                    retained.retain({{RetainedKind::arrival_context, "ctx"},
                                     "arrival",
                                     ConfigurationReference{"cfg"}}) == RetainDisposition::inserted,
                "Shipped retained storage commits synthetic configuration and arrival");
            const ApplicationStateSnapshot start{
                "review-initial",
                finance::document(),
                "cfg",
                finance::timer(finance::Life::Active, main),
                debt,
                {{ApplicationBalanceKind::score, "score", score},
                 {ApplicationBalanceKind::whole_count, "count", count}}};
            require(SqliteApplicationBundleStore(session).retain_initial_state(start) ==
                        SqliteBundleInitialDisposition::inserted,
                    "Shipped bundle store retains the real application head");
        });
    }

    ContributionReceipt intake(std::string ref, std::optional<core::ReceiptMoney> money = {},
                               std::string kind = "support") {
        const core::ReceiptInput input{{{"review-fixture", "account"}, "delivery", std::move(ref)},
                                       {std::move(kind),
                                        1,
                                        45,
                                        {},
                                        {},
                                        std::move(money),
                                        core::ReceiptLinkReference{"payment-v1", "payment"},
                                        {}},
                                       {50, "connection", "cfg", "ctx"}};
        std::optional<core::ReceiptId> id;
        auto work = qt_test_support::run_work(
            db.writer(),
            [&](auto context) {
                SqliteReceiptRepository repo(context);
                const auto result = ReceiptIntake(repo).ingest(input);
                require(result.disposition == core::ReceiptDisposition::created,
                        "Synthetic immutable original receipt commits before review");
                id = result.receipt_id;
                return 1;
            },
            10000);
        finance::normal(work);
        require(id.has_value(), "Real receipt allocation returned a positive ID");
        return {*id, input.identity, input.facts, input.arrival};
    }

    ReviewEntryIntent seed(std::string ref, std::vector<core::RequestedEffect> requests,
                           bool missing_rate = false, bool monetary = false) {
        require(!missing_rate || monetary, "Missing rate fixture has an actual monetary receipt");
        const auto receipt = intake(
            ref, monetary ? std::optional{core::ReceiptMoney{"20.0000", "EUR"}} : std::nullopt);
        core::ContributionState state;
        state.payment_evidence = core::PaymentEvidence::confirmed;
        state.conditions.valuation = {
            monetary, receipt.facts.money,
            monetary && !missing_rate ? std::optional<std::string>{"arrival-quote"} : std::nullopt,
            missing_rate};
        state.conditions.review = core::ContributionReview::pending;
        state.conditions.evidence_admission = {
            core::EvidencePolicy::accept_provider_report, "evidence-policy", {}};
        state.conditions.settlement = core::SettlementGate::clear;
        std::optional<core::ContributionId> cid;
        auto work = qt_test_support::run_work(
            db.writer(),
            [&](auto context) {
                SqliteContributionRepository repo(context);
                const auto linked = ContributionEvidenceService(repo).observe(
                    {receipt, ContributionObservationKind::support, state.payment_evidence,
                     VerifiedActivityLink{{receipt.identity.origin, "fixture-support", ref},
                                          "proof"},
                     state.conditions});
                require(linked.disposition == ContributionLinkDisposition::created,
                        "Real evidence creates the selected contribution before review");
                cid = linked.contribution_id;
                return 1;
            },
            10000);
        finance::normal(work);
        require(cid.has_value(), "Contribution allocation returned a typed ID");
        core::PlanValuation valuation{core::PlanValuationState::not_applicable, {}};
        if (missing_rate)
            valuation.state = core::PlanValuationState::missing_historical;
        else if (monetary)
            valuation = {core::PlanValuationState::resolved,
                         core::HistoricalValuation{"arrival-quote",
                                                   *receipt.facts.money,
                                                   "CHF",
                                                   "1.00",
                                                   {},
                                                   "rate-snapshot",
                                                   "fixture-feed",
                                                   40,
                                                   41,
                                                   {}}};
        const core::CapturedPlanRevision capture{
            1,
            {},
            {{ref + "-event",
              {},
              ref + "-event",
              receipt.id,
              receipt.identity,
              receipt.facts,
              receipt.arrival,
              cid},
             state,
             valuation,
             std::move(requests)},
            {{"historical-rule", "arrival-rule-1", true, "immutable arrival"}},
            {},
            {}};
        db.write([&](const auto &s) {
            require(SqliteApplicationPlanStore(s).retain_initial(ref + "-plan", capture) ==
                        SqlitePlanInitialDisposition::inserted,
                    "Shipped plan store commits immutable original request sequence");
        });
        require(db.rows("SELECT original_receipt_id,review,application FROM subathon_contributions "
                        "WHERE contribution_id=?",
                        {blob8(cid->value)}) == SqlRows{{blob8(receipt.id.value), 0, 0}},
                "Independent observer confirms the exact original anchor and pending review");
        ReviewEntryIntent intent;
        intent.selector = {*cid, receipt.id};
        intent.milestones = std::vector<MilestoneAcceptanceChoice>{};
        return intent;
    }

    ContributionReview service() {
        return {db.writer(), facts, [this](const auto &event) { trace.observe(event); }};
    }

    std::vector<ReviewPreviewRow> preview(ContributionReview &review,
                                          const std::vector<ReviewEntryIntent> &entries) {
        auto result = await<ContributionReview::PreviewResult>(
            [&](auto done) { review.preview({principal, entries}, "synthetic", std::move(done)); });
        require(std::holds_alternative<std::vector<ReviewPreviewRow>>(result),
                "Selected preview returns per-row facts or redaction");
        return std::get<std::vector<ReviewPreviewRow>>(std::move(result));
    }

    static ReviewCommand bind(std::string key, const std::vector<ReviewEntryIntent> &entries,
                              const std::vector<ReviewPreviewRow> &preview) {
        auto ordered = entries;
        std::ranges::sort(ordered, {}, [](const auto &e) { return e.selector.receipt.value; });
        require(preview.size() == ordered.size(), "Preview covers each selected receipt");
        ReviewCommand cmd{{"review", std::move(key)}, principal, "operator", {}};
        for (std::size_t i = 0; i < ordered.size(); ++i) {
            const auto &row = preview[i];
            require(row.ordinal == int(i + 1) && row.facts &&
                        row.facts->selector == ordered[i].selector && !row.facts->basis.isEmpty(),
                    "Every disclosed fixture preview is bound in committed ReceiptId order");
            const auto ordinal = std::to_string(i + 1);
            cmd.entries.push_back({ordered[i], row.facts->basis, "decision-" + ordinal,
                                   "synthetic reason " + ordinal});
        }
        return cmd;
    }

    static ContributionReview::Result execute(ContributionReview &review, ReviewCommand cmd,
                                              std::string proof = "synthetic") {
        return await<ContributionReview::Result>(
            [&](auto done) { review.execute(std::move(cmd), std::move(proof), std::move(done)); });
    }

    static const ReviewReply &reply(const ContributionReview::Result &result) {
        require(std::holds_alternative<ReviewReply>(result), "Review returns a per-row report");
        return std::get<ReviewReply>(result);
    }

    void head(int main, int debt, int score, int count = 3) {
        require(db.rows("SELECT s.main_remaining_ms,s.time_debt_ms FROM subathon_app_head h "
                        "JOIN subathon_app_states s ON h.token=s.token") == SqlRows{{main, debt}},
                "Independent committed Main and debt");
        require(db.rows("SELECT b.units FROM subathon_app_head h JOIN subathon_app_balances b "
                        "ON h.token=b.token ORDER BY b.balance_kind") == SqlRows{{score}, {count}},
                "Independent committed score and count");
    }

    void install() {
        db.write([](const auto &s) {
            require(SqliteAccountSessionUnit(s, {}).insert_installation(
                        {{"install", "epoch"}, "revision"}) == LifecycleWriteResult::staged,
                    "The owning account store commits the fixture installation");
        });
    }
};
} // namespace storage_026_fixture
