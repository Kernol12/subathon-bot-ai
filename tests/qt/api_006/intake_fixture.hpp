// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../storage_026/receipt_fixture.hpp"
#include "../storage_026/transition_fixture.hpp"

#include <subathon/server/connector_intake.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace api_006_fixture {
namespace intake = subathon::server::connector_intake;
namespace storage = storage_026_fixture;
using namespace subathon::server;
namespace core = subathon::core;
using storage::blob8;
using storage::check;
using storage::require;
using storage::SqlRows;

constexpr StorageLimits work_profile{64, 256, 65536, 2097152, 1536, 4194304};
inline const intake::Selection source{{"install", "P", "A", "S"}, {1, 9, 1, "g3"}, "history-v1"};

class History final : public intake::ConnectorHistoryAdapter {
  public:
    bool available{true};
    QByteArray upper{"10"};
    QByteArray proof{"ordered-profile-proof"};
    int cuts{}, pages{};
    std::optional<intake::Page> current_page;

    intake::CutResult observe_cut(const intake::CutRequest &request) override {
        ++cuts;
        if (!available)
            return intake::HistoryFailure::unknown_capability;
        return intake::CutObservation{
            request,
            {"key-v1", "S/history", upper, request.source.fence.generation, proof},
            request.purpose == intake::CutPurpose::import_upper ? intake::Endpoint::exclusive
                                                                : intake::Endpoint::inclusive};
    }

    std::variant<intake::Page, intake::HistoryFailure>
    fetch_page(const intake::HistoryRequest &) override {
        ++pages;
        if (!current_page)
            return intake::HistoryFailure::temporarily_unavailable;
        return *current_page;
    }
};

inline intake::Item item(QByteArray key = "L", QByteArray position = "12") {
    return {key,
            "delivery",
            "N",
            key,
            {"support", 1, 1000, {}, {}, core::ReceiptMoney{"12.34", "CHF"}, {}, {}},
            position,
            "ordered-membership",
            {},
            VerifiedActivityLink{{{"P", "A"}, "S/link", "K"}, "synthetic-verification"},
            intake::SourceTimeQuality::reliable,
            core::PaymentEvidence::confirmed,
            ContributionObservationKind::support,
            "individual",
            {},
            storage::normalized_item(18)};
}

struct Fixture {
    storage::Populated store{80000};
    History history;
    intake::Authorization authority{intake::Authorization::allowed};
    qint64 arrival{8000};
    bool missing_rate{};
    bool awaiting_evidence{};
    bool ordinary_review{};
    int requested_ms{};
    std::optional<intake::ZoneBasis> zone;
    intake::OwnerInputs inputs{
        [this](intake::Action, const intake::Scope &scope) {
            if (scope.installation != source.scope.installation || scope.provider != "P" ||
                (scope.account != "A" && scope.account != "B") ||
                (scope.stream != "S" && scope.stream != "other"))
                return intake::Authorization::denied;
            return authority;
        },
        history,
        [this](const intake::Item &input) {
            const bool from_history = input.key == "H";
            const auto suffix = from_history ? "H" : "L";
            const auto valuation = from_history ? "VH" : "VL";
            const auto units = requested_ms ? requested_ms : (from_history ? 120000 : 60000);
            core::ContributionConditions conditions;
            conditions.review = ordinary_review ? core::ContributionReview::pending
                                                : core::ContributionReview::not_required;
            conditions.valuation = {
                true, input.facts.money,
                missing_rate ? std::nullopt : std::optional<std::string>{valuation}, missing_rate};
            conditions.settlement = core::SettlementGate::clear;
            conditions.evidence_admission = {
                awaiting_evidence ? core::EvidencePolicy::require_evidence_review
                                  : core::EvidencePolicy::accept_provider_report,
                "arrival-evidence-policy",
                awaiting_evidence
                    ? std::optional<core::EvidenceReviewDecision>{{core::EvidenceReviewChoice::
                                                                       await_confirmation,
                                                                   "await-confirmation",
                                                                   "operator"}}
                    : std::nullopt};
            core::PlanValuation plan{core::PlanValuationState::missing_historical, {}};
            if (!missing_rate && input.facts.money)
                plan = {core::PlanValuationState::resolved,
                        core::HistoricalValuation{valuation,
                                                  *input.facts.money,
                                                  "CHF",
                                                  "1.00",
                                                  {},
                                                  "snapshot",
                                                  "fixture-feed",
                                                  40,
                                                  41,
                                                  {}}};
            return intake::CapturedSupport{
                {arrival, "connection", "cfg", std::string("arrival-") + suffix},
                conditions,
                plan,
                {{"arrival-rule", std::string("rule-") + suffix, true, "selected at arrival"}},
                {{"modifier-definition", std::string("modifier-") + suffix, "main", "arrival-group",
                  "product", "multiply", from_history ? "2" : "1"}},
                {storage::finance::request(core::RequestedUnit::milliseconds, units, "main")}};
        },
        [this](const QByteArray &id, const QByteArray &basis, QDate) {
            if (basis != "frozen-zone-rules")
                return std::optional<intake::ZoneBasis>{};
            if (id == "UTC")
                return std::optional{intake::ZoneBasis{0, {}}};
            return id == "Synthetic/Frozen" ? zone : std::nullopt;
        }};

    explicit Fixture(bool preseed = false, bool bind_page = true) {
        store.facts.current->cap = 100000;
        store.install();
        storage::advance(store.db);
        store.db.reopen(work_profile);
        storage::report(storage::migrate(store.db.writer(),
                                         storage::request(MigrationMode::inspect,
                                                          connector_intake_repository_manifest())),
                        CatalogState::current, 20, 20, 0);
        require(store.db.rows(storage::history_query) ==
                    storage::expected_history(storage::successor()),
                "Exact installed v20 closure includes all protected v19 entries");
        require(inputs.current_authority(intake::Action::inspect, source.scope) ==
                    intake::Authorization::allowed,
                "Current synthetic authority binds exact fixture scope");
        if (preseed) {
            storage::stage_scope(store.db, "main");
            storage::mutate(store.db, "UPDATE main.subathon_intake_sources SET frontier=X'3131'");
            storage::mutate(store.db,
                            "UPDATE main.subathon_intake_gaps SET start_frontier=X'3131'");
            if (bind_page) {
                storage::catchup(store.db, 1, 1);
                storage::anchor_page(
                    store.db, storage::page_admission(1, {storage::normalized_item(18)}).binding);
            }
        }
        store.db.write([](const auto &work) {
            SqliteRetainedContextStore retained(work);
            for (const auto &suffix : {"L", "H"})
                require(retained.retain(
                            {{RetainedKind::arrival_context, std::string("arrival-") + suffix},
                             QByteArray("arrival-capture-") + suffix,
                             ConfigurationReference{"cfg"}}) == RetainDisposition::inserted,
                        "Distinct real retained parent for each arrival");
        });
    }

    intake::IntakeResult submit(const intake::Command &command,
                                const intake::Selection &selection = source) {
        return intake::submit(store.db.writer(), inputs, selection, command);
    }

    void no_award() {
        for (const auto *table :
             {"subathon_app_claims", "subathon_app_effects", "subathon_app_jobs"})
            require(store.db.rows(QString("SELECT count(*) FROM main.") + table) == SqlRows{{0}},
                    "No claim, applied effect or queued job exists");
    }

    auto cursors() {
        return store.db.rows(
            "SELECT o.next_ordinal,o.continuation,s.frontier,g.status "
            "FROM subathon_intake_operations o JOIN subathon_intake_sources s "
            "USING(installation,provider,account,stream) JOIN subathon_intake_gaps g "
            "USING(installation,provider,account,stream) WHERE o.purpose=0");
    }

    auto attempts() {
        return store.db.rows(
            "SELECT receipt_id,ordinal,server_timestamp_utc_ms,modifier_context_ref "
            "FROM main.subathon_receipt_attempts ORDER BY receipt_id,ordinal");
    }

    void arm_loss() {
        using namespace subathon::server::testing;
        require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                    store.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                    StorageError::none,
                "Idle owner targets the selected real COMMIT");
    }

    void stopped(const SqliteWorkResult &result) {
        require(!result.value && result.transaction == TransactionOutcome::outcome_unknown &&
                    result.error == WorkError::uncertain_outcome,
                "Real COMMIT acknowledgement loss suppresses result");
        require(store.db.writer().enqueue_work([](auto) { return QVariant{}; }, [](auto) {}) ==
                    StorageError::shutting_down,
                "Original writer refuses suffix work after unknown outcome");
    }

    struct PhaseCompletion {
        bool entered{};
        std::optional<intake::IntakeResult> result;
        SqliteWorkResult work;
    };

    PhaseCompletion phase(QByteArray key, intake::Phase selected,
                          std::optional<intake::Item> received = {},
                          std::optional<intake::PageSlot> slot = {},
                          const std::function<bool()> &before_commit = {},
                          const intake::Selection &selection = source) {
        PhaseCompletion completion;
        completion.work = qt_test_support::run_work(
            store.db.writer(),
            [&](auto ctx) -> QVariant {
                completion.entered = true;
                completion.result = intake::run_phase(
                    ctx, inputs, {selection, key, selected, received, slot}, before_commit);
                return 1;
            },
            10000);
        require(completion.entered, "Owner work dispatch entered the selected phase");
        return completion;
    }

    static intake::IntakeResult result(const PhaseCompletion &completion,
                                       intake::IntakeStatus expected, const char *message) {
        require(completion.result && completion.result->status == expected, message);
        return *completion.result;
    }
};

inline intake::Baseline baseline(intake::CatchUpMode mode = intake::CatchUpMode::review) {
    return {intake::CutPurpose::first_connect,
            {{source.scope, "individual", "main", "overlap", true, "selected-primary"}},
            mode,
            {}};
}

inline intake::BeginHistory history_import(QDate day, QByteArray zone = "UTC") {
    return {"import", intake::HistoryPurpose::import, {}, day, zone, "frozen-zone-rules"};
}

inline intake::Page page(QList<intake::Item> items = {},
                         std::optional<intake::RangeProof> proof = {}) {
    return {{{source},
             "O",
             1,
             QByteArray("T"),
             QByteArray("11"),
             {intake::Endpoint::exclusive, intake::Endpoint::inclusive, "ordered-membership",
              "ordered-key"},
             {"key-v1", "S/history", QByteArray("14"), "g3", "upper-cut"},
             {}},
            std::move(items),
            QByteArray("token-opaque"),
            std::move(proof)};
}

} // namespace api_006_fixture
