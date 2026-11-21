// SPDX-License-Identifier: GPL-3.0-or-later
#include "missing_port.hpp"
#include "populated_fixture.hpp"

#include "../../../server/src/sqlite_selected_record_read.hpp"
#include "../support/milestone_target_edit_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "../support/sqlite_rate_override_writer_fixture.hpp"

#include <array>
#include <subathon/server/sqlite_reference_authority.hpp>
#include <subathon/server/sqlite_reference_capture.hpp>

namespace {
using namespace storage_026_fixture;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &failure) {
        QFAIL(failure.what());
    }
}

void mutate(CatalogFile &file, const QString &sql, QList<QVariant> bindings = {}) {
    file.write([&](const auto &s) { (void)s.execute({sql, bindings}); });
}

MigrationResult enqueue_catalog(CatalogFile &file, MigrationRequest request) {
    return finance::completed<MigrationResult>([&](auto done) {
        return SqliteMigrationRunner::enqueue(file.writer(), std::move(request), std::move(done))
            .error;
    });
}

void catalog_refusal(CatalogFile &file, MigrationRequest request, MigrationReason expected) {
    const auto result = enqueue_catalog(file, std::move(request));
    require(result.reason == expected && !result.report &&
                result.work.transaction == TransactionOutcome::rolled_back,
            "Runner rejects the exact catalog damage in a rolled-back unit");
}

SqlRows protected_objects(CatalogFile &file) {
    return file.rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema WHERE "
                     "substr(tbl_name,1,16)<>'subathon_intake_' "
                     "ORDER BY type COLLATE BINARY,name COLLATE BINARY,tbl_name COLLATE BINARY");
}

auto protected_data(CatalogFile &file) {
    return std::array{
        file.rows("SELECT * FROM main.subathon_receipts ORDER BY receipt_id"),
        file.rows("SELECT * FROM main.subathon_receipt_attempts ORDER BY receipt_id,ordinal"),
        file.rows("SELECT * FROM main.subathon_contributions ORDER BY contribution_id"),
        file.rows("SELECT * FROM main.subathon_contribution_observations ORDER BY receipt_id"),
        file.rows("SELECT * FROM main.subathon_retained_configurations ORDER BY reference"),
        file.rows("SELECT * FROM main.subathon_retained_contexts ORDER BY reference"),
        file.rows("SELECT * FROM main.subathon_plan_revisions ORDER BY plan_ref,revision"),
        file.rows("SELECT * FROM main.subathon_app_results ORDER BY operation_scope,operation_ref"),
        file.rows("SELECT * FROM main.subathon_review_work ORDER BY work_scope,work_ref"),
        file.rows(
            "SELECT * FROM main.subathon_review_entries ORDER BY work_scope,work_ref,ordinal"),
        file.rows("SELECT * FROM main.subathon_review_terminal_requests "
                  "ORDER BY operation_scope,operation_ref,request_ordinal")};
}

struct Budget {
    qsizetype statements;
    qsizetype sql_bytes;
    qsizetype bind_bytes;
    qsizetype rows;
    qsizetype result_bytes;
};

Budget advance_charge(const MigrationManifest &manifest) {
    constexpr qsizetype old_objects = 225;
    constexpr qsizetype new_objects = 253;
    constexpr qsizetype old_projection = 127248;
    constexpr qsizetype new_projection = old_projection + 13592;
    const auto &tail = manifest.steps.back();
    // Each schema row has 12 cells, four 4-byte type names, and two
    // occurrences of each present object field's UTF-8 bytes.
    const auto schema_bytes = [](auto rows, auto projection) {
        return rows * (12 * 8 + 4 * 4) + 2 * projection;
    };
    auto history_bytes = [](const MigrationStep &step) {
        return 7 * 8 + 7 + 4 + 4 + 32 + 2 * step.id.toUtf8().size();
    };
    qsizetype history_pair = history_bytes(tail);
    for (qsizetype i = 0; i + 1 < manifest.steps.size(); ++i)
        history_pair += 2 * history_bytes(manifest.steps[i]);
    constexpr qsizetype singleton_bytes = 7 * 8 + 7 + 7 + 4 + 2 * 21;
    // Pragma encoding (one row, "UTF-8"), integrity (one "ok"), FK
    // check (no row) and catalog singleton, before and after advancement.
    constexpr qsizetype fixed_bytes = 2 * (13 + 10 + singleton_bytes);
    return {28, 3075, 24 + tail.id.toUtf8().size() + 32,
            old_objects + 2 * new_objects + (2 * tail.version - 1) + 6,
            schema_bytes(old_objects, old_projection) +
                2 * schema_bytes(new_objects, new_projection) + history_pair + fixed_bytes};
}

void expect_empty_delta(CatalogFile &file) {
    for (const auto &object : delta_objects)
        if (object.type == "table")
            require(file.rows("SELECT count(*) FROM main." + object.name) == SqlRows{{0}},
                    "Additive upgrade creates empty relations, with no inferred legacy state");
}

template <class Body> auto read_missing(CatalogFile &file, Body body) {
    using Result = std::invoke_result_t<Body, const SqliteWorkSession &>;
    std::optional<Result> result;
    const auto work = qt_test_support::run_work(
        file.writer(),
        [&](auto ctx) -> QVariant {
            const auto s = ctx.begin();
            result = body(s);
            s.rollback();
            return {};
        },
        10000);
    require(result && work.transaction == TransactionOutcome::rolled_back && !work.value &&
                work.error == WorkError::not_committed,
            "Selected seam was entered inside a real writer unit with an acknowledged rollback");
    return *result;
}

std::pair<ManagedResult, SqliteWorkResult> manage(CatalogFile &file, const ManagedInput &input,
                                                  const std::function<bool()> &before_commit = {}) {
    auto result = std::make_shared<std::optional<ManagedResult>>();
    const auto work = qt_test_support::run_work(
        file.writer(),
        [result, input, before_commit](auto ctx) -> QVariant {
            const auto s = ctx.begin();
            result->emplace(stage_managed(s, input, before_commit));
            if (result->value().stop == Stop::not_implemented)
                s.rollback();
            // On adoption the managed port owns the receipt transaction.
            return 1;
        },
        10000);
    require(result->has_value(), "Managed port reached after real BEGIN");
    if (result->value().stop == Stop::not_implemented)
        require(work.transaction == TransactionOutcome::rolled_back &&
                    work.error == WorkError::not_committed && !work.value,
                "Missing managed port returns only an explicit rolled-back first stop");
    return {std::move(result->value()), work};
}

ReferenceRule capture_rule() {
    return {"R", "r", "a", "b", "T", 600000, {ApplicationPrincipalKind::automation, "I", "U"}, "K"};
}

constexpr auto receipt_namespace = "00000001530000000152000000014e";
ManagedInput managed(std::string ref, qint64 arrival, QByteArray capture) {
    const auto ns = QByteArray::fromHex(receipt_namespace).toStdString();
    return {QByteArray::fromStdString(ref),
            {{{"P", "A"}, ns, ref},
             {"support", 1, 45, {}, {}, core::ReceiptMoney{"20.00", "EUR"}, {}, {}},
             {arrival, "connection", "cfg", "ctx"}},
            {{{"P", "A"}, "S/link", "K"}, "verify"},
            std::move(capture)};
}

core::ReceiptId legacy_receipt(CatalogFile &file, const ManagedInput &input,
                               qsizetype attempts = 1) {
    std::optional<core::ReceiptId> id;
    const auto work = qt_test_support::run_work(
        file.writer(),
        [&](auto ctx) {
            SqliteReceiptRepository repo(ctx);
            auto unit = repo.begin();
            require(!unit->find(input.receipt.identity), "Synthetic receipt key starts unoccupied");
            id = unit->create(input.receipt.identity, input.receipt.facts, input.receipt.arrival);
            for (qsizetype ordinal = 1; ordinal <= attempts; ++ordinal) {
                auto arrival = input.receipt.arrival;
                arrival.server_timestamp_utc_ms += ordinal - 1;
                unit->append_attempt(*id, {arrival, input.receipt.facts,
                                           ordinal == 1 ? core::ReceiptDisposition::created
                                                        : core::ReceiptDisposition::retry});
            }
            unit->commit();
            return 1;
        },
        10000);
    finance::normal(work);
    require(id.has_value(), "Real legacy receipt unit committed the selected receipt");
    require(file.rows("SELECT count(*),min(ordinal),max(ordinal) "
                      "FROM main.subathon_receipt_attempts WHERE receipt_id=?",
                      {blob8(id->value)}) == SqlRows{{attempts, 1, attempts}},
            "Independent physical ordinal range is contiguous by PK and exact count");
    return *id;
}

void stage_scope(CatalogFile &file, const QByteArray &effect_scope = "score") {
    file.write([&](const auto &s) {
        (void)s.execute({"INSERT INTO main.subathon_intake_policies VALUES(?,1,?)",
                         {QByteArray("install"), QByteArray("review-policy")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_members VALUES(?,1,?,?,?,?,?, ?,1,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("support"), effect_scope, QByteArray("overlap"),
                          QByteArray("review-lane")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_sources "
                         "VALUES(?,?,?,?,1,?,9,1,1,?,?,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("g3"), QByteArray("cut"), QByteArray("F"),
                          QByteArray("cursor-contract")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_gaps VALUES(?,?,?,?,1,?,1,1,0,?,?,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("G"), QByteArray("F"), QVariant{}, QByteArray("gap-basis")}});
    });
}

void entry_anchor(const SqliteWorkSession &s, const ManagedInput &input, core::ReceiptId id,
                  qint64 ordinal) {
    (void)s.execute(
        {"INSERT INTO main.subathon_intake_entries("
         "installation,provider,account,stream,intake_ref,epoch,generation,policy_revision,"
         "input,provenance,arrival_ms,configuration_ref,modifier_context_ref,"
         "activity_namespace,activity_id,verification_ref,phase,disposition,receipt_id,"
         "attempt_ordinal) VALUES(?,?,?,?,?,1,?,1,?,?,?,?,?,?,?,?,1,0,?,?)",
         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
          input.intake_ref, QByteArray("g3"), QByteArray("observation"), input.capture,
          qint64(input.receipt.arrival.server_timestamp_utc_ms),
          QByteArray::fromStdString(input.receipt.arrival.configuration_ref),
          QByteArray::fromStdString(input.receipt.arrival.modifier_context_ref),
          QByteArray("S/link"), QByteArray("K"), QByteArray("verify"), blob8(id.value), ordinal}});
}

void peer_anchors(CatalogFile &file, const ManagedInput &first, core::ReceiptId first_id,
                  const ManagedInput &peer, core::ReceiptId peer_id, bool retain_basis = true) {
    file.write([&](const auto &s) {
        // Only owned new relations get literal rows. The receipt IDs and
        // attempts come from independently committed ACCT-001 transactions.
        entry_anchor(s, first, first_id, 1);
        entry_anchor(s, peer, peer_id, 1);
        if (retain_basis)
            (void)s.execute({"INSERT INTO main.subathon_intake_basis VALUES(?,?,?,?,?,?,?,?,1,?)",
                             {QByteArray("install"), QByteArray("P"), QByteArray("A"),
                              QByteArray("S/link"), QByteArray("K"), QByteArray("S"),
                              first.intake_ref, blob8(first_id.value), first.capture}});
    });
}

std::pair<std::optional<ContributionLinkResult>, SqliteWorkResult>
evidence_work(CatalogFile &file, const ManagedInput &input, core::ReceiptId id,
              bool abnormal = false) {
    std::optional<ContributionLinkResult> result;
    const auto work = qt_test_support::run_work(
        file.writer(),
        [&](auto ctx) {
            core::ContributionConditions conditions;
            conditions.valuation = {true, input.receipt.facts.money, "quote", false};
            conditions.review = core::ContributionReview::pending;
            conditions.evidence_admission = {
                core::EvidencePolicy::accept_provider_report, "review-policy", {}};
            conditions.settlement = core::SettlementGate::clear;
            SqliteContributionRepository repo(ctx);
            result = ContributionEvidenceService(repo).observe(
                {{id, input.receipt.identity, input.receipt.facts, input.receipt.arrival},
                 ContributionObservationKind::support,
                 core::PaymentEvidence::confirmed,
                 input.activity,
                 conditions});
            if (abnormal)
                throw std::runtime_error("Abnormal completion after real owning evidence COMMIT");
            return 1;
        },
        10000);
    return {std::move(result), work};
}

ContributionLinkResult evidence(CatalogFile &file, const ManagedInput &input, core::ReceiptId id) {
    auto [result, work] = evidence_work(file, input, id);
    finance::normal(work);
    require(result.has_value(), "ACCT-002 owns its separate evidence COMMIT");
    return *result;
}

struct Finished : Populated {
    ReviewEntryIntent original;
    ReviewCommand command;
    ApplicationOperationKey key;

    explicit Finished(bool with_milestone = false) : Populated(99, 7) {
        original = seed("terminal",
                        {finance::request(finance::Unit::milliseconds, 20, "main"),
                         finance::request(finance::Unit::micro_points, 15, "score"),
                         finance::request(finance::Unit::whole_count, 1, "count")},
                        false, true);
        db.reopen({64, 768, 65536, 8585216, 768, 16105472});
        auto review = service();
        facts.current->timer.hard_end_utc_ms = 50;
        facts.current->clock.paused = true;
        facts.current->clock.utc_now_ms = 50;
        facts.current->finished_siblings = std::vector{finance::Unit::micro_points};
        if (with_milestone) {
            facts.current->milestones = {{"threshold-100", 100}};
            original.milestones.reset();
            const auto rows = preview(review, {original});
            require(rows.size() == 1 && rows[0].facts && rows[0].facts->occurrences.size() == 1,
                    "Actual sparse score crosses exactly one milestone");
            original.milestones = std::vector<MilestoneAcceptanceChoice>{
                {rows[0].facts->occurrences.front(), MilestoneActionChoice::defer}};
        }
        command = bind("terminal-apply", {original}, preview(review, {original}));
        const auto result = execute(review, command);
        expected_row(reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
        const auto &detail = reply(result).rows.front().outcome->facts;
        require(detail && detail->bundle, "Existing Review owns the actual SAR2 commit");
        key = *detail->bundle;
        head(99, 7, 105);
        require(db.rows("SELECT request_ordinal,disposition "
                        "FROM main.subathon_review_terminal_requests ORDER BY request_ordinal") ==
                        SqlRows{{1, 1}, {3, 2}} &&
                    db.rows("SELECT request_ordinal,component_kind,units "
                            "FROM main.subathon_app_components") == SqlRows{{2, 2, 15}},
                "Finished has two terminal ordinals and one actual score effect");
        const auto persisted = db.rows("SELECT format_ref,document FROM main.subathon_app_results");
        require(persisted.size() == 1 &&
                    persisted[0][0].toByteArray() == "subathon.internal.application-result.v2" &&
                    persisted[0][1].toByteArray().startsWith("SAR2") &&
                    persisted[0][1].toByteArray().endsWith(
                        QByteArray::fromHex("0000000300000002010002")),
                "Independent literal SAR2 mask and result tag");
    }

    void stored() {
        db.write([&](const auto &s) {
            const auto bundle = SqliteApplicationBundleStore(s).lookup(key);
            require(
                bundle && bundle->effects.size() == 1 &&
                    bundle->effects.front().request_ordinal == 2 &&
                    bundle->result.document.endsWith(QByteArray::fromHex("0000000300000002010002")),
                "Owning bundle lookup resolves exact committed SAR2");
        });
    }

    void claim() {
        db.write([&](const auto &s) {
            const auto claim = SqliteApplicationBundleStore(s).lookup_original_claim(
                {ApplicationSubjectKind::contribution, original.selector.receipt,
                 original.selector.cid});
            require(claim && finance::same(claim->existing.intent.operation, key),
                    "Original claim retains the same sparse bundle");
        });
    }

    void reversal() {
        db.write([&](const auto &s) {
            SqliteApplicationBundleStore store(s);
            require(!store.read_reversal_basis({key, 1, finance::CK::main_delta}).basis &&
                        !store.read_reversal_basis({key, 3, finance::CK::whole_count}).basis,
                    "Terminal ordinals have no reversible credit");
            const auto score = store.read_reversal_basis({key, 2, finance::CK::score});
            require(score.basis && score.basis->actual_credit_units == 15 &&
                        score.basis->effect.request_ordinal == 2,
                    "Reversal basis ignores terminal masks and reads exactly the actual effect");
        });
    }

    ApplicationOperationKey refund(FinancialRefundEffectPort *port = nullptr) {
        FinancialRefundCommand request;
        db.write([&](const auto &s) {
            const ApplicationPlanKey plan{"terminal-plan", 1};
            const auto selected = SqliteApplicationBundleStore(s).read_inputs(
                plan, ApplicationSubjectKind::contribution);
            require(selected.inputs && selected.inputs->current_contribution,
                    "Real retained plan and original anchor resolve for the refund");
            request = {{"refund", "one-fifth"},
                       {0},
                       VerifiedRefundCorrelation{{{"review-fixture", "account"},
                                                  {"payment-v1", "payment"},
                                                  original.selector.receipt},
                                                 {"refund-v1", "one-fifth"},
                                                 "verified-synthetic",
                                                 original.selector.cid,
                                                 key,
                                                 plan},
                       {"one-fifth-financial", 1},
                       0,
                       selected.inputs->current_state.token,
                       selected.inputs->current_contribution->contribution.state,
                       finance::principal,
                       finance::audit("one-fifth")};
        });
        request.receipt = intake("refund-receipt", core::ReceiptMoney{"4.00", "EUR"}, "refund").id;
        const auto fifth = finance::fraction(1, 5);
        const FinancialEntitlement candidate{
            0,     4,  fifth,
            false, {}, {{1, {key, 2, finance::CK::score}, 0, 0, 3, -3, fifth, 3, 0, false}}};
        finance::Authority authority;
        finance::Effects effects;
        db.write([&](const auto &s) {
            auto &selected_port = port ? *port : static_cast<FinancialRefundEffectPort &>(effects);
            require(stage_financial_refund(s, request, candidate, authority, selected_port)
                            .disposition == FinancialDisposition::applied,
                    "The actual financial producer applies one fifth of +15 at ordinal 2");
        });
        head(99, 7, 102);
        require(
            db.rows("SELECT original_ordinal,kind,position FROM main.subathon_financial_bases") ==
                SqlRows{{2, 2, 1}},
            "Only physical score becomes a financial basis link");
        return request.operation;
    }

    ContributionReview::Result lookup(ContributionReview &review) {
        return await<ContributionReview::Result>([&](auto done) {
            review.lookup(command.key, principal, "synthetic", std::move(done));
        });
    }
};

std::vector<FinancialTimerHold> seed_mixed_refund(finance::File &f, const std::string &ref,
                                                  bool hold_timer) {
    const auto main = hold_timer ? 50000 : 125000;
    f.move_main("financial-fixture-elapsed", main);
    const auto quarter = finance::fraction(1, 4);
    const FinancialEntitlement entitlement{
        0,
        5,
        quarter,
        false,
        GroupedRefundEntitlement{{finance::original, 1, finance::CK::main_delta},
                                 1,
                                 0,
                                 {},
                                 {75000, 0},
                                 75000,
                                 0,
                                 -75000,
                                 0},
        {{2,
          {finance::original, 2, finance::CK::score},
          0,
          0,
          10000000,
          -10000000,
          quarter,
          10000000,
          0,
          false},
         {3,
          {finance::original, 3, finance::CK::whole_count},
          0,
          0,
          0,
          0,
          quarter,
          finance::fraction(7, 4),
          finance::fraction(7, 4),
          false}}};
    finance::Effects effects;
    if (!hold_timer)
        effects.apply(-75000, 0, {75000});
    const auto refund = f.refund(ref, core::ReceiptMoney{"5.00", "EUR"});
    (void)f.accept(f.command(ref, refund, 0), entitlement, effects);
    auto financial = f.financial_active();
    if (hold_timer) {
        require(financial.size() == 1 && effects.adjustments == 0,
                "Real #199 mixed refund held time and credited its score adjustment once");
    } else {
        require(financial.empty() && effects.adjustments == 1,
                "Real #199 mixed refund with enough Main applies time and leaves no hold");
    }
    return financial;
}

void upgrade_financial(finance::File &file) {
    const auto manifest = successor();
    const auto head = manifest.steps.back().version;
    report(migrate(*file.writer, {MigrationMode::inspect, manifest, {}, {}}),
           CatalogState::upgrade_required, head - 1, head, 0);
    report(migrate(*file.writer, {MigrationMode::advance, manifest, {}, {}}), CatalogState::current,
           head, head, 1);
    auto stopped = std::make_shared<bool>(false);
    require(file.writer->shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                QTest::qWaitFor([stopped] { return *stopped; }, 10000),
            "Release the old financial writer before cold readback");
    file.observer.reset();
    file.writer.reset();
    file.writer = std::make_unique<SqliteWriter>();
    file.observer = std::make_unique<SqliteReader>();
    require(file.writer->open(file.path, {64, 768, 65536, 8585216, 768, 16105472}) ==
                    StorageError::none &&
                file.observer->open(file.path) == StorageError::none,
            "Reopened financial fixture uses the unchanged Review minimum profile");
    report(migrate(*file.writer, {MigrationMode::inspect, manifest, {}, {}}), CatalogState::current,
           head, head, 0);
}

struct CreditedHold {
    static constexpr std::int64_t deficit = -400000;
    finance::File file{finance::OriginalCredit{.beside_complete_request = deficit}};
    std::optional<ApplicationHoldVersionLocator> financial;
    ApplicationHoldDetection detection;
    Facts facts;

    explicit CreditedHold(const std::string &refund_ref, bool hold_timer = true) {
        const auto holds = seed_mixed_refund(file, refund_ref, hold_timer);
        if (!holds.empty())
            financial = holds.front().locator;
        // Give the separate stores equal current observations via a genuine committed operation.
        file.move_main("credited-preview-head", 50000);
        file.check_head(50000, 0, 30000000, 7);
        detection = file.ordinary_detection("beside-complete", false);
        auto &cause = detection.cause;
        cause.command.operation = finance::original;
        cause.kind = ApplicationHoldCauseKind::timer_deficit;
        cause.missing_policy_reason.reset();
        cause.plan_evidence = {};
        cause.plan_evidence.plan = finance::original_plan;
        for (const auto &request : file.support_capture.captured.requested_effects) {
            const auto ordinal =
                static_cast<std::int64_t>(cause.plan_evidence.occurrences.size() + 1);
            cause.plan_evidence.occurrences.push_back({ordinal,
                                                       {},
                                                       {},
                                                       {},
                                                       {},
                                                       request.unit,
                                                       request.exact_requested_units,
                                                       request.requested_units,
                                                       {}});
        }
        cause.timer = {4,
                       finance::CK::unapplied_time,
                       deficit,
                       finance::Policy::RequireReview,
                       core::TimeReversalDisposition::RequiresReview,
                       deficit,
                       ApplicationHoldComposition::beside_complete_application,
                       finance::document(),
                       ApplicationHoldAssociatedApplication{finance::original, 4}};
        (void)file.detect(detection);
        record_second_version();
        report(migrate(*file.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
               CatalogState::current, 19, 19, 4);
        facts.configuration_document = "configuration";
    }

    void record_second_version() {
        detection.request.mutation_ref = "beside-complete-second-detection";
        detection.request.expected_version_ref = "v1";
        detection.request.proposed_version_ref = "v2";
        file.write([&](const auto &s) {
            require(SqliteApplicationHoldStore(s).record_detection(detection).disposition ==
                        SqliteHoldMutationDisposition::recorded,
                    "Owning store records the exact second active beside-complete version");
        });
    }

    ReviewEntryIntent intent(ReviewVerb verb) const {
        ReviewEntryIntent result;
        result.selector = {file.cid, file.payment.support_receipt};
        result.verb = verb;
        if (verb == ReviewVerb::reject_held || verb == ReviewVerb::retain_held)
            result.timer = ReviewTimerSelector{{detection.cause.hold_ref, "v2"}, 4, deficit};
        if (verb == ReviewVerb::reject_held)
            result.ordinary_closures = {
                {detection.cause.hold_ref, "v2", "v3", finance::document(), {}}};
        return result;
    }

    static ReviewCommand bind(ContributionReview &service, std::string key,
                              ReviewEntryIntent intent) {
        auto result = await<ContributionReview::PreviewResult>([&](auto done) {
            service.preview({principal, {intent}}, "synthetic", std::move(done));
        });
        const auto *preview = std::get_if<std::vector<ReviewPreviewRow>>(&result);
        require(preview, "Credited original returns a same-row preview for this review verb");
        auto command = Populated::bind(key, {intent}, *preview);
        command.entries.front().decision_ref = std::move(key);
        return command;
    }
};

struct MilestoneOrigin : Finished {
    using Group = CapturedMilestoneGroup;
    using ms = std::chrono::milliseconds;
    const MilestoneOccurrenceKey application{"sar2-occurrence"};
    const Group group{
        {{"threshold-100", "rule-1", 100, 105, "proof"}, {}, {ms(10), ms(2)}, application}, {}};
    const PausedMilestoneAcceptance acceptance{PausedAcceptanceMode::normal,
                                               {"operator", "milestone-decision"},
                                               {{application, MilestoneActionChoice::run}}};

    void enroll() {
        db.write([&](const auto &s) { SqliteMilestoneGroupStore(s).stage(key, {group}); });
    }

    void record() {
        db.write([&](const auto &s) {
            SqliteMilestoneGroupDecisions(s).record_acceptance(key, acceptance);
        });
    }

    MilestoneTrackSeed track() const {
        return {
            milestone_target_edit_fixture::track,
            "cfg",
            "initial-def",
            "initial-prefix",
            {{"threshold-100", 100}, {"B", 200}},
            std::vector<MilestoneTrackSeed::Member>{{group.group.achievement, key, application}}};
    }
};

struct EditRelease final : MilestoneEditReleaseAuthority {
    int calls{};
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        ++calls;
        require(track == milestone_target_edit_fixture::track &&
                    occurrence == milestone_target_edit_fixture::b,
                "Edit release authorizes only the exact selected pending edit");
        return true;
    }
};

} // namespace

#include "transition_fixture.hpp"

namespace {

// ACCT-001/002 and the generic plan owner can create this exact provider original
// on v19. The reference-intake capture index is specific to its synthetic provider.
struct OldSourcePending {
    ManagedInput input{managed("old-pending", 8000, {})};
    ReviewEntryIntent review;
    core::CapturedPlanRevision capture;

    explicit OldSourcePending(Populated &f) {
        input.receipt.arrival.configuration_ref = "old-rule";
        input.receipt.arrival.modifier_context_ref = "old-arrival";
        input.receipt.arrival.connection_ref = "g3-connection";
        input.capture =
            frame_fields({"install", "P", "A", "S", "g3", "review-policy", "old-admission-decision",
                          "old-rule", "old-arrival", "old-plan", "1", "60000", "quote"});
        f.db.write([&](const auto &s) {
            const SqliteRetainedContextStore store(s);
            require(store.retain({{RetainedKind::configuration, "old-rule"},
                                  "old-rule-30s/arrival-modifier-x2",
                                  {}}) == RetainDisposition::inserted &&
                        store.retain({{RetainedKind::arrival_context, "old-arrival"},
                                      input.capture,
                                      ConfigurationReference{"old-rule"}}) ==
                            RetainDisposition::inserted,
                    "The retained owner commits the exact old admission and arrival provenance");
        });
        const auto id = legacy_receipt(f.db, input);
        const auto linked = evidence(f.db, input, id);
        require(id == core::ReceiptId{1} &&
                    linked.disposition == ContributionLinkDisposition::created &&
                    linked.contribution_id == core::ContributionId{0},
                "Real receipt and evidence owners anchor one pending old-source original");
        review.selector = {*linked.contribution_id, id};
        review.milestones = std::vector<MilestoneAcceptanceChoice>{};
        core::ContributionState state;
        state.payment_evidence = core::PaymentEvidence::confirmed;
        state.conditions.valuation = {true, input.receipt.facts.money, "quote", false};
        state.conditions.review = core::ContributionReview::pending;
        state.conditions.evidence_admission = {
            core::EvidencePolicy::accept_provider_report, "review-policy", {}};
        state.conditions.settlement = core::SettlementGate::clear;
        auto request = finance::request(finance::Unit::milliseconds, 60000, "main");
        request.rule_ref = "old-timer-rule";
        capture = {1,
                   {},
                   {{"old-event",
                     {},
                     "old-event",
                     id,
                     input.receipt.identity,
                     input.receipt.facts,
                     input.receipt.arrival,
                     linked.contribution_id},
                    state,
                    {core::PlanValuationState::resolved,
                     core::HistoricalValuation{"quote",
                                               *input.receipt.facts.money,
                                               "CHF",
                                               "1.20",
                                               {},
                                               "old-rate",
                                               "fixture-feed",
                                               40,
                                               41,
                                               {}}},
                    {request}},
                   {{"old-timer-rule", "old-rule", true, "arrival rule requests 30000 ms"}},
                   {{"old-modifier", "old-modifier-instance", "main", "time", "multiply",
                     "multiply", "2"}},
                   {}};
        f.db.write([&](const auto &s) {
            require(SqliteApplicationPlanStore(s).retain_initial("old-plan", capture) ==
                        SqlitePlanInitialDisposition::inserted,
                    "Normal plan store retains the old rule, modifier, valuation and 60 s request");
        });
    }

    void link_intake(CatalogFile &db) const {
        db.write([&](const auto &s) {
            entry_anchor(s, input, review.selector.receipt, 1);
            (void)s.execute({"UPDATE main.subathon_intake_entries SET phase=2,contribution_id=? "
                             "WHERE intake_ref=? AND receipt_id=? AND attempt_ordinal=1",
                             {blob8(review.selector.cid.value), input.intake_ref,
                              blob8(review.selector.receipt.value)}});
            (void)s.execute(
                {"INSERT INTO main.subathon_intake_basis VALUES(?,?,?,?,?,?,?,?,1,?)",
                 {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S/link"),
                  QByteArray("K"), QByteArray("S"), input.intake_ref,
                  blob8(review.selector.receipt.value), input.capture}});
        });
        require(
            db.rows(
                "SELECT installation,provider,account,stream,intake_ref,epoch,generation,"
                "policy_revision,configuration_ref,modifier_context_ref,receipt_id,"
                "attempt_ordinal,contribution_id,provenance FROM main.subathon_intake_entries") ==
                SqlRows{{QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                         input.intake_ref, 1, QByteArray("g3"), 1, QByteArray("old-rule"),
                         QByteArray("old-arrival"), blob8(1), 1, blob8(0), input.capture}},
            "Old intake membership binds the exact receipt attempt, original and authority");
    }

    void read_member(CatalogFile &db) const {
        require(db.rows("SELECT award_class,effect_scope,overlap_set,primary_slot,document "
                        "FROM main.subathon_intake_members WHERE installation=? AND "
                        "policy_revision=1 AND provider=? AND account=? AND stream=?",
                        {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S")}) ==
                    SqlRows{{QByteArray("support"), QByteArray("main"), QByteArray("overlap"), 1,
                             QByteArray("review-lane")}},
                "The retained Review lane is primary for the old capture's Main time award");
    }

    void read_capture(CatalogFile &db) const {
        require(
            db.rows("SELECT provider,source_account,identifier_namespace,opaque_id,"
                    "server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_ref "
                    "FROM main.subathon_receipts WHERE receipt_id=?",
                    {blob8(1)}) == SqlRows{{QByteArray("P"), QByteArray("A"),
                                            QByteArray::fromHex(receipt_namespace),
                                            input.intake_ref, 8000, QByteArray("g3-connection"),
                                            QByteArray("old-rule"), QByteArray("old-arrival")}} &&
                db.rows("SELECT contribution_id,activity_provider,activity_source_account,"
                        "activity_namespace,activity_id,verification_ref,support_policy_ref "
                        "FROM main.subathon_contribution_observations WHERE receipt_id=?",
                        {blob8(1)}) ==
                    SqlRows{{blob8(0), QByteArray("P"), QByteArray("A"), QByteArray("S/link"),
                             QByteArray("K"), QByteArray("verify"), QByteArray("review-policy")}} &&
                db.rows("SELECT receipt_id,ordinal,configuration_ref,modifier_context_ref "
                        "FROM main.subathon_receipt_attempts") ==
                    SqlRows{{blob8(1), 1, QByteArray("old-rule"), QByteArray("old-arrival")}} &&
                db.rows("SELECT contribution_id,original_receipt_id "
                        "FROM main.subathon_contributions") == SqlRows{{blob8(0), blob8(1)}},
            "The real receipt and evidence rows retain old source, activity and policy");
        db.write([&](const auto &s) {
            const auto plan = SqliteApplicationPlanStore(s).find("old-plan");
            const auto arrival =
                SqliteRetainedContextStore(s).find({RetainedKind::arrival_context, "old-arrival"});
            require(plan && !plan->revisions.empty() && plan->revisions.front() == capture &&
                        arrival && arrival->document == input.capture,
                    "Normal readers select the exact old capture and decision provenance");
            const auto &retained = plan->revisions.front();
            require(retained.captured.requested_effects.size() == 1 &&
                        retained.rules.size() == 1 && retained.rules[0].selected &&
                        retained.rules[0].version_ref == "old-rule" &&
                        retained.rules[0].rule_ref == "old-timer-rule" &&
                        retained.captured.requested_effects[0].rule_ref ==
                            retained.rules[0].rule_ref &&
                        retained.captured.requested_effects[0].target_ref == "main" &&
                        retained.captured.requested_effects[0].unit == finance::Unit::milliseconds &&
                        retained.captured.requested_effects[0].requested_units == 60000,
                    "The exact old selected rule produced one captured 60 s Main request");
        });
    }

    void continue_review(Populated &f, bool deny) const {
        read_capture(f.db);
        const auto immutable = protected_data(f.db);
        require(f.db.rows("SELECT count(*) FROM main.subathon_app_claims") == SqlRows{{0}},
                "The old pending original has no automatic claim");
        f.head(80000, 0, 90);
        f.facts.current->cap = 100000;
        f.facts.current->clock = {0, 8001, true};
        auto service = f.service();
        const auto preview = f.preview(service, {review});
        require(preview.size() == 1 && preview[0].facts && preview[0].facts->may_apply &&
                    preview[0].facts->prospective_effects.size() == 1 &&
                    preview[0].facts->prospective_effects[0].requested_units == 60000,
                "Current Review uses the retained 60 s request under today's live cap");
        const auto command = f.bind("old-continuation", {review}, preview);
        if (deny)
            f.facts.execution_denied = review.selector;
        const auto result = f.execute(service, command, "refreshed");
        const auto &reply = f.reply(result);
        expected_row(reply, 1, ReviewDisposition::committed,
                     deny ? ReviewStatus::denied : ReviewStatus::applied);
        read_capture(f.db);
        const auto after = protected_data(f.db);
        require(after[0] == immutable[0] && after[1] == immutable[1] && after[3] == immutable[3] &&
                    after[4] == immutable[4] && after[5] == immutable[5] &&
                    after[6] == immutable[6],
                "An authorized or denied continuation preserves original receipt and capture");
        if (deny) {
            require(!reply.rows[0].outcome->facts && after[2] == immutable[2] &&
                        after[7] == immutable[7] &&
                        f.db.rows("SELECT count(*) FROM main.subathon_app_claims") == SqlRows{{0}},
                    "Revocation redacts the result and leaves the pending original unclaimed");
            f.head(80000, 0, 90);
        } else {
            require(reply.rows[0].outcome->facts &&
                        reply.rows[0].outcome->facts->verified == review.selector &&
                        f.db.rows("SELECT plan_ref,plan_revision,request_ordinal,requested_units "
                                  "FROM main.subathon_app_effects") ==
                            SqlRows{{QByteArray("old-plan"), blob8(1), 1, 60000}} &&
                        f.db.rows("SELECT component_kind,units FROM main.subathon_app_components "
                                  "ORDER BY component_kind") ==
                            SqlRows{{0, 20000}, {1, 0}, {5, 40000}} &&
                        f.db.rows("SELECT subject_kind,subject_id FROM main.subathon_app_claims") ==
                            SqlRows{{int(ApplicationSubjectKind::contribution), blob8(0)}},
                    "Current authorization produces one award from the exact original");
            f.head(100000, 0, 90);
        }
    }
};

} // namespace

ApplicationOperationKey seed_dense(Populated &f) {
    const auto entry = f.seed("dense", Case::positive());
    f.db.reopen();
    auto review = f.service();
    const auto command = f.bind("dense", {entry}, f.preview(review, {entry}));
    const auto result = f.execute(review, command);
    expected_row(f.reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
    return *f.reply(result).rows.front().outcome->facts->bundle;
}

class Storage026Tests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void compiledManifest() {
        check([] {
            Finished f;
            f.stored();
            const auto fixture = successor();
            const auto manifest = compiled_connector_manifest();
            require(std::holds_alternative<MigrationManifest>(manifest),
                    "Missing compiled connector-intake manifest after healthy shipped v19");
            const auto &actual = std::get<MigrationManifest>(manifest);
            require(!actual.steps.isEmpty() &&
                        actual.steps.back().id == QStringLiteral("connector-intake-storage-v1"),
                    "The manifest variant returns the exact case-sensitive successor marker");
            require(actual.store_identity == fixture.store_identity &&
                        actual.catalog_format == fixture.catalog_format &&
                        actual.supported_starting_versions == fixture.supported_starting_versions &&
                        actual.steps.size() == fixture.steps.size(),
                    "Compiler supplies exactly the reviewed starting set and one new step");
            for (qsizetype i = 0; i < actual.steps.size(); ++i)
                require(actual.steps[i].version == fixture.steps[i].version &&
                            actual.steps[i].id == fixture.steps[i].id &&
                            actual.steps[i].sql == fixture.steps[i].sql &&
                            actual.steps[i].expected_schema == fixture.steps[i].expected_schema,
                        "Every step has the exact ordered SQL and projected tuples");
        });
    }

    void literalPopulatedUpgrade() {
        check([] {
            Finished f;
            f.install();
            f.stored();
            auto &db = f.db;
            const auto old = protected_data(db);
            const auto old_schema = db.rows(schema_query);
            const auto history = db.rows(history_query);
            auto manifest = successor();
            require(
                manifest.steps.back().expected_schema.size() == 253 && delta_objects.size() == 28 &&
                    delta_sql.size() == 14,
                "225 protected objects plus 28 literal delta objects, including 14 autoindexes");
            // Text-only reconstruction from the protected v19 and proposed DDL.
            // A moved protected tail requires a refreshed prefix, coordinate
            // and digest under independent review before implementation entry.
            require(digest(manifest.steps.back()).toHex() ==
                        "eb1758529cd052df4f8ed80ab693a6219afa519ae9db5f1b00f2f6bae05bf7e1",
                    "Full fixture step matches the independent canonical frame digest");
            advance(db);
            expect_schema(db.rows(schema_query), manifest.steps.back());
            const auto after = db.rows(history_query);
            require(after == expected_history(manifest) && after.mid(0, history.size()) == history,
                    "History and independently framed digests preserve the exact prefix");
            require(protected_objects(db) == old_schema && protected_data(db) == old,
                    "All selected preexisting rows and protected catalog bytes survive reopen");
            expect_empty_delta(db);
            require(db.rows("SELECT * FROM pragma_integrity_check()") == SqlRows{{QString("ok")}} &&
                        db.rows("SELECT * FROM pragma_foreign_key_check()").isEmpty(),
                    "Reopened file independently satisfies physical and FK integrity");
            const auto old_request =
                request(MigrationMode::inspect, contribution_review_repository_manifest());
            catalog_refusal(db, old_request, MigrationReason::unsupported_version);
            for (const auto mode : {MigrationMode::inspect, MigrationMode::advance})
                catalog_refusal(db, {mode, reference_authority_repository_manifest(), {}, {}},
                                MigrationReason::unsupported_version);
        });
    }

    void legacyRunnerAtItsOwnV10() {
        check([] {
            QTemporaryDir directory;
            SqliteWriter writer;
            require(directory.isValid() && writer.open(directory.filePath("legacy.sqlite"),
                                                       finance::limits()) == StorageError::none,
                    "Real single writer opens a disposable host/dev legacy fixture");
            const auto legacy = reference_authority_repository_manifest();
            report(migrate(writer, {MigrationMode::advance, legacy, {}, {}}), CatalogState::current,
                   10, 10, 10);
            report(migrate(writer, {MigrationMode::inspect, legacy, {}, {}}), CatalogState::current,
                   10, 10, 0);
        });
    }

    void legacySelectedOwnersAfterUpgrade() {
        check([] {
            Populated f;
            const auto input = managed("legacy-receipt", 8000, "legacy-closure");
            const auto id = legacy_receipt(f.db, input, 2);
            const auto linked = evidence(f.db, input, id);
            require(linked.disposition == ContributionLinkDisposition::created &&
                        linked.contribution_id == core::ContributionId{0},
                    "Real evidence creates an original on the same committed legacy receipt");
            const auto plan = f.seed("other-original", Case::positive());
            const auto verify = [&] {
                const auto receipt = qt_test_support::run_work(
                    f.db.writer(),
                    [&](auto ctx) {
                        SqliteReceiptRepository repo(ctx);
                        auto unit = repo.begin();
                        const auto selected = unit->find(input.receipt.identity);
                        auto retry_arrival = input.receipt.arrival;
                        ++retry_arrival.server_timestamp_utc_ms;
                        const std::vector<core::ReceiptAttempt> expected{
                            {input.receipt.arrival, input.receipt.facts,
                             core::ReceiptDisposition::created},
                            {retry_arrival, input.receipt.facts, core::ReceiptDisposition::retry}};
                        require(selected && selected->id == id &&
                                    selected->original_arrival == input.receipt.arrival &&
                                    selected->original_facts == input.receipt.facts &&
                                    selected->attempts == expected,
                                "ACCT-001 reconstructs immutable arrival and selected retry");
                        unit->commit();
                        return 1;
                    },
                    10000);
                finance::normal(receipt);
                const auto observation = qt_test_support::run_work(
                    f.db.writer(),
                    [&](auto ctx) {
                        SqliteContributionRepository repo(ctx);
                        auto unit = repo.begin();
                        const auto row = unit->find_activity(input.activity.identity);
                        const auto link = unit->find_receipt(id);
                        require(row && link && row->id == core::ContributionId{0} &&
                                    row->original_receipt_id == id &&
                                    link->contribution_id == row->id,
                                "ACCT-002 reconstructs original and observation binding");
                        unit->commit();
                        return 1;
                    },
                    10000);
                finance::normal(observation);
                f.db.write([&](const auto &s) {
                    auto context =
                        SqliteRetainedContextStore(s).find({RetainedKind::arrival_context, "ctx"});
                    auto capture =
                        SqliteApplicationPlanStore(s).find_revision("other-original-plan", 1);
                    require(
                        context && context->configuration == ConfigurationReference{"cfg"} &&
                            context->document == "arrival" && capture &&
                            capture->revision.captured.event.contribution_id == plan.selector.cid &&
                            capture->revision.captured.event.receipt_id == plan.selector.receipt &&
                            capture->revision.captured.requested_effects.size() == 2,
                        "Retained context and original plan expose their exact protected anchors");
                });
            };
            verify();
            advance(f.db);
            verify();
        });
    }

    void manifestLimits_data() {
        QTest::addColumn<int>("dimension");
        QTest::newRow("versions") << 0;
        QTest::newRow("id-bytes") << 1;
        QTest::newRow("schema-objects") << 2;
        QTest::newRow("manifest-bytes") << 3;
    }

    void manifestLimits() {
        QFETCH(int, dimension);
        check([&] {
            Populated f;
            auto request = successor_request(MigrationMode::advance);
            const auto &manifest = request.manifest;
            const auto target = manifest.steps.back().version;
            constexpr qsizetype bytes = 1594845;
            constexpr qsizetype longest_id = 29; // reference-initial-captures-v1
            require(tally(manifest) == bytes && manifest.steps.back().id.size() == 27,
                    "Independent delta+protected UTF-8 total and literal ID bytes");
            const std::array<qsizetype MigrationLimits::*, 4> field{
                &MigrationLimits::versions, &MigrationLimits::id_bytes,
                &MigrationLimits::schema_objects, &MigrationLimits::manifest_utf8_bytes};
            const std::array<qsizetype, 4> edge{target, longest_id, 253, bytes};
            const auto member = field.at(dimension);
            const auto exact = edge.at(dimension);
            const auto before = f.db.rows(history_query);
            request.limits.*member = exact - 1;
            bool called = false;
            const auto forbidden = SqliteMigrationRunner::enqueue(f.db.writer(), request,
                                                                  [&](auto) { called = true; });
            require(forbidden.error == StorageError::invalid_options &&
                        forbidden.reason == MigrationReason::invalid_manifest && !called &&
                        f.db.rows(history_query) == before,
                    "One below the independent manifest cap refuses before enqueue");
            request.limits.*member = exact;
            report(migrate(f.db.writer(), std::move(request)), CatalogState::current, target,
                   target, 1);
        });
    }

    void migrationWriterLimits_data() {
        QTest::addColumn<int>("dimension");
        QTest::newRow("statement-attempts") << 0;
        QTest::newRow("peak-utf8") << 1;
        QTest::newRow("cumulative-binds") << 2;
        QTest::newRow("result-rows") << 3;
        QTest::newRow("result-bytes") << 4;
    }

    void migrationWriterLimits() {
        QFETCH(int, dimension);
        check([&] {
            Populated f;
            const auto manifest = successor();
            const auto target = manifest.steps.back().version;
            const auto charge = advance_charge(manifest);
            require(charge.rows == 776 && charge.result_bytes == 905829 && charge.bind_bytes == 83,
                    "Three schema reads, both histories, six fixed rows, and one history insert");
            const std::array<qsizetype StorageLimits::*, 5> field{
                &StorageLimits::statements_per_batch, &StorageLimits::statement_utf8_bytes,
                &StorageLimits::bound_value_bytes, &StorageLimits::result_rows,
                &StorageLimits::result_value_bytes};
            const std::array edge{charge.statements, charge.sql_bytes, charge.bind_bytes,
                                  charge.rows, charge.result_bytes};
            const auto member = field.at(dimension);
            const auto exact = edge.at(dimension);
            auto limits = migration_profile();
            const auto before = f.db.rows(history_query);
            limits.*member = exact - 1;
            f.db.reopen(limits);
            const auto refused = enqueue_catalog(f.db, successor_request(MigrationMode::advance));
            require(refused.reason == MigrationReason::resource_limit &&
                        refused.work.error ==
                            (dimension < 3 ? WorkError::resource_limit : WorkError::result_limit) &&
                        refused.work.transaction == TransactionOutcome::rolled_back &&
                        !refused.report,
                    "One below the exact real writer charge rolls back with its owning error");
            require(f.db.rows(history_query) == before, "Failed writer left history intact");
            limits.*member = exact;
            f.db.reopen(limits);
            report(migrate(f.db.writer(), successor_request(MigrationMode::advance)),
                   CatalogState::current, target, target, 1);
            f.db.reopen();
            const auto after = f.db.rows(history_query);
            require(after.mid(0, before.size()) == before && after == expected_history(manifest),
                    "Only the at-limit advancement persists and preserves the protected prefix");
        });
    }

    void malformedCatalog_data() {
        QTest::addColumn<int>("damage");
        QTest::newRow("lookalike") << 0;
        QTest::newRow("latest-digest") << 1;
        QTest::newRow("earlier-digest") << 2;
        QTest::newRow("extra-marker") << 3;
        QTest::newRow("missing-index") << 4;
        QTest::newRow("altered-index") << 5;
        QTest::newRow("before-history-cancellation") << 6;
    }

    void malformedCatalog() {
        QFETCH(int, damage);
        check([&] {
            Populated f;
            const auto tail = successor().steps.back();
            if (damage == 6) {
                const auto schema = f.db.rows(schema_query);
                const auto history = f.db.rows(history_query);
                auto cancel = successor_request(MigrationMode::advance);
                cancel.cancel = [&](auto p) {
                    return p.version == tail.version &&
                           p.checkpoint == MigrationCheckpoint::before_history;
                };
                catalog_refusal(f.db, std::move(cancel), MigrationReason::cancelled);
                require(f.db.rows(schema_query) == schema && f.db.rows(history_query) == history,
                        "Cancellation after DDL rolls back the entire additive unit");
                return;
            }
            advance(f.db);
            switch (damage) {
            case 0:
                mutate(f.db,
                       "UPDATE main.subathon_migration_history SET migration_id=? "
                       "WHERE version=?",
                       {QString("Connector-intake-storage-v1"), tail.version});
                break;
            case 1:
            case 2:
                mutate(f.db,
                       "UPDATE main.subathon_migration_history SET content_sha256=? "
                       "WHERE version=?",
                       {QByteArray(32, 'x'), damage == 1 ? tail.version : 1});
                break;
            case 3:
                mutate(f.db, "INSERT INTO main.subathon_migration_history VALUES(?,?,?)",
                       {tail.version + 1, QString("unknown-successor-v1"), QByteArray(32, 'x')});
                break;
            case 4:
            case 5:
                mutate(f.db, "DROP INDEX main.subathon_intake_gap_status");
                if (damage == 5)
                    mutate(f.db, "CREATE INDEX subathon_intake_gap_status "
                                 "ON subathon_intake_gaps(revision)");
                break;
            }
            const auto schema = f.db.rows(schema_query);
            const auto history = f.db.rows(history_query);
            const auto reason = damage == 3   ? MigrationReason::unsupported_version
                                : damage >= 4 ? MigrationReason::schema_mismatch
                                              : MigrationReason::catalog_mismatch;
            catalog_refusal(f.db, successor_request(MigrationMode::advance), reason);
            require(f.db.rows(schema_query) == schema && f.db.rows(history_query) == history,
                    "Rejected damage does not advance or repair the file");
        });
    }

    void sparseBundleNormalEntries_data() {
        QTest::addColumn<int>("reader");
        QTest::newRow("bundle") << 0;
        QTest::newRow("original-claim") << 1;
        QTest::newRow("reversal") << 2;
    }

    void sparseBundleNormalEntries() {
        QFETCH(int, reader);
        check([&] {
            Finished f;
            const auto entry =
                std::array{&Finished::stored, &Finished::claim, &Finished::reversal}.at(reader);
            (f.*entry)();
            advance(f.db);
            (f.*entry)();
        });
    }

    void terminalCorruption_data() {
        QTest::addColumn<bool>("upgrade");
        QTest::addColumn<bool>("missing");
        QTest::newRow("v19-missing") << false << true;
        QTest::newRow("v19-wrong-disposition") << false << false;
        QTest::newRow("successor-missing") << true << true;
        QTest::newRow("successor-wrong-disposition") << true << false;
    }

    void terminalCorruption() {
        QFETCH(bool, upgrade);
        QFETCH(bool, missing);
        check([&] {
            Finished f;
            f.stored();
            if (upgrade)
                advance(f.db);
            mutate(f.db, missing
                             ? "DELETE FROM main.subathon_review_terminal_requests "
                               "WHERE request_ordinal=1"
                             : "UPDATE main.subathon_review_terminal_requests SET disposition=2 "
                               "WHERE request_ordinal=1");
            bool refused = false;
            try {
                f.stored();
            } catch (const SqliteBundleStorageFailure &error) {
                refused = error.reason() == SqliteBundleStorageReason::invalid_stored_data;
            }
            require(refused, "Owning bundle read refuses the corrupt terminal partition");
        });
    }

    void reviewNormalEntries_data() {
        QTest::addColumn<int>("reader");
        QTest::newRow("lookup") << 0;
        QTest::newRow("execute-retry") << 1;
        QTest::newRow("fresh-preview") << 2;
        QTest::newRow("fresh-execute") << 3;
        QTest::newRow("denied-scope") << 4;
    }

    void reviewNormalEntries() {
        QFETCH(int, reader);
        check([&] {
            Finished f;
            auto fresh = f.seed("next-terminal",
                                {finance::request(finance::Unit::micro_points, 7, "score")});
            auto review = f.service();
            expected_row(f.reply(f.lookup(review)), 1, ReviewDisposition::existing,
                         ReviewStatus::applied);
            expected_row(f.reply(f.execute(review, f.command)), 1, ReviewDisposition::existing,
                         ReviewStatus::applied);
            const auto fresh_command = f.bind("fresh", {fresh}, f.preview(review, {fresh}));
            const auto before = protected_data(f.db);
            advance(f.db);
            auto upgraded = f.service();
            f.trace.events.clear();
            if (reader == 2) {
                const auto rows = f.preview(upgraded, {fresh});
                require(rows.size() == 1 && rows.front().facts &&
                            rows.front().facts->selector == fresh.selector,
                        "Fresh preview uses the selected pre-upgrade application inputs");
            } else if (reader == 3) {
                expected_row(f.reply(f.execute(upgraded, fresh_command)), 1,
                             ReviewDisposition::committed, ReviewStatus::applied);
                require(f.trace.ordinals(ReviewUnit::initial_read, ReviewUnitPhase::began) ==
                                std::vector{0} &&
                            f.trace.ordinals(ReviewUnit::final_read, ReviewUnitPhase::began) ==
                                std::vector{0},
                        "Fresh execute reaches independent initial and final reconciliation");
                require(f.db.rows("SELECT next_ordinal FROM main.subathon_review_work "
                                  "WHERE work_ref=?",
                                  {QByteArray("fresh")}) == SqlRows{{2}},
                        "Fresh execute durably closes just its first ordinal");
            } else if (reader == 4) {
                f.facts.batch_allowed = false;
                const auto denied = f.lookup(upgraded);
                require(std::get_if<ReviewRefusal>(&denied) &&
                            std::get<ReviewRefusal>(denied) == ReviewRefusal::denied,
                        "Denied current scope cannot read the protected journal");
            } else {
                const auto result =
                    reader == 0 ? f.lookup(upgraded) : f.execute(upgraded, f.command);
                expected_row(f.reply(result), 1, ReviewDisposition::existing,
                             ReviewStatus::applied);
                require(f.trace.completions().empty(),
                        "Readback/retry never admits a second application unit");
            }
            if (reader != 3)
                require(protected_data(f.db) == before,
                        "Read-only entry changes no protected rows");
        });
    }

    void sixteenEntryReviewReopensAtShippedProfile() {
        check([] {
            Populated f;
            std::vector<ReviewEntryIntent> entries;
            for (int i = 1; i <= 16; ++i)
                entries.push_back(f.seed("bounded-" + std::to_string(i), {}, true, true));
            std::ranges::reverse(entries);
            f.db.reopen({64, 768, 65536, 8585216, 768, 16105472});
            auto old = f.service();
            const auto command = f.bind("sixteen", entries, f.preview(old, entries));
            f.trace.control = [](const auto &event) {
                if (event.unit == ReviewUnit::row && event.ordinal == 16 &&
                    event.phase == ReviewUnitPhase::before_commit)
                    throw std::runtime_error("known sixteenth row rollback");
            };
            const auto original = f.execute(old, command);
            for (int ordinal = 1; ordinal <= 15; ++ordinal)
                expected_row(f.reply(original), ordinal, ReviewDisposition::committed,
                             ReviewStatus::accepted_pending);
            expected_row(f.reply(original), 16, ReviewDisposition::write_failed);
            const auto prefix =
                f.db.rows("SELECT * FROM main.subathon_review_entries ORDER BY ordinal");
            require(prefix.size() == 15 &&
                        f.db.rows("SELECT next_ordinal FROM main.subathon_review_work") ==
                            SqlRows{{16}},
                    "Known v19 failure leaves exactly fifteen durable ordinals");
            advance(f.db); // Reopens with exactly the shipped minimum Review profile.
            f.trace = {};
            auto upgraded = f.service();
            const auto result = f.execute(upgraded, command);
            for (int ordinal = 1; ordinal <= 15; ++ordinal)
                expected_row(f.reply(result), ordinal, ReviewDisposition::existing,
                             ReviewStatus::accepted_pending);
            expected_row(f.reply(result), 16, ReviewDisposition::committed,
                         ReviewStatus::accepted_pending);
            require(
                f.trace.ordinals(ReviewUnit::row, ReviewUnitPhase::began) == std::vector{16} &&
                    f.trace.ordinals(ReviewUnit::final_read, ReviewUnitPhase::began) ==
                        std::vector{0} &&
                    f.db.rows("SELECT next_ordinal FROM main.subathon_review_work") ==
                        SqlRows{{17}} &&
                    f.db.rows("SELECT * FROM main.subathon_review_entries ORDER BY ordinal")
                            .mid(0, 15) == prefix,
                "Successor keeps all sixteen Review entries and reconciles only the final suffix");
            require(f.db.rows("SELECT count(*) FROM main.subathon_app_claims") == SqlRows{{0}},
                    "Pending Review entries create zero claims");
            f.head(95, 0, 90);
        });
    }

    void reviewCatalogCorruption_data() {
        QTest::addColumn<int>("damage");
        QTest::addColumn<bool>("successor");
        QTest::newRow("v19-earlier-digest") << 0 << false;
        QTest::newRow("v19-latest-digest") << 1 << false;
        QTest::newRow("v19-review-schema") << 2 << false;
        QTest::newRow("successor-earlier-digest-masked") << 0 << true;
        QTest::newRow("successor-latest-digest-masked") << 1 << true;
        QTest::newRow("successor-review-schema-masked") << 2 << true;
        QTest::newRow("successor-overflow-marker-masked") << 3 << true;
        QTest::newRow("v19-typed-history-overflow") << 4 << false;
    }

    void reviewCatalogCorruption() {
        QFETCH(int, damage);
        QFETCH(bool, successor);
        check([&] {
            Finished f;
            auto legacy = f.service();
            expected_row(f.reply(f.lookup(legacy)), 1, ReviewDisposition::existing,
                         ReviewStatus::applied);
            if (successor)
                advance(f.db);
            const auto version =
                successor ? storage_026_fixture::successor().steps.back().version : 19;
            if (damage < 2)
                mutate(f.db,
                       "UPDATE main.subathon_migration_history SET content_sha256=? "
                       "WHERE version=?",
                       {QByteArray(32, 'z'), damage == 0 ? 1 : version});
            else if (damage == 2)
                mutate(f.db, "DROP INDEX main.subathon_review_entries_cid");
            else if (damage == 3)
                // This later extra row still fails the earlier latest-marker gate.
                mutate(f.db, "INSERT INTO main.subathon_migration_history VALUES(?,?,?)",
                       {version + 1, QString("overflow-witness"), QByteArray(32, 'z')});
            else
                mutate(f.db, "INSERT INTO main.subathon_migration_history VALUES(?,?,?)",
                       {0, QString("overflow-v0"), QByteArray(32, 'z')});
            const auto history = f.db.rows(history_query);
            const auto schema = f.db.rows(schema_query);
            if (damage == 4) {
                require(history.size() == 20 && history.mid(1) == expected_history(literal_v19()) &&
                            f.db.rows("SELECT version,typeof(version),typeof(migration_id),"
                                      "typeof(content_sha256),length(content_sha256) "
                                      "FROM main.subathon_migration_history WHERE version=0") ==
                                SqlRows{{0, QString("integer"), QString("text"), QString("blob"),
                                         32}} &&
                            f.db.rows(
                                "SELECT version,migration_id FROM main.subathon_migration_history "
                                "ORDER BY version DESC LIMIT 1") ==
                                SqlRows{{19, QString("contribution-review-v1")}},
                        "A twentieth typed history row leaves the exact latest marker intact");
            }
            auto fresh = f.service();
            const auto before = protected_data(f.db);
            const auto app_rows = [&] {
                return std::array{f.db.rows("SELECT * FROM main.subathon_app_head"),
                                  f.db.rows("SELECT * FROM main.subathon_app_claims"),
                                  f.db.rows("SELECT * FROM main.subathon_app_effects"),
                                  f.db.rows("SELECT * FROM main.subathon_app_components")};
            };
            const auto application = damage == 4 ? std::optional{app_rows()} : std::nullopt;
            f.trace.events.clear();
            const auto result = f.lookup(fresh);
            require(std::get_if<ReviewRefusal>(&result) &&
                        std::get<ReviewRefusal>(result) == ReviewRefusal::unavailable &&
                        protected_data(f.db) == before && f.db.rows(history_query) == history &&
                        f.db.rows(schema_query) == schema,
                    "Review discloses no journal or typed outcome after malformed history/schema");
            if (damage == 4) {
                require(f.trace.ordinals(ReviewUnit::initial_read, ReviewUnitPhase::began) ==
                                std::vector{0} &&
                            f.trace.completions().empty() && app_rows() == *application,
                        "The admitted lookup reaches the catalog read without row work or writes");
                f.head(99, 7, 105);
            }
        });
    }

    void financialSparseHistory() {
        check([] {
            Finished f;
            f.reversal();
            const auto operation = f.refund();
            auto read = [&](const auto &s) {
                const auto history = SqliteFinancialRefundStore(s).lookup(operation);
                require(history && history->entitlement.components.size() == 1 &&
                            history->entitlement.components.front().original.request_ordinal == 2,
                        "Owning #199 read reconstructs the exact committed sparse basis");
            };
            f.db.write(read);
            advance(f.db);
            f.db.write(read);
            f.head(99, 7, 102);
        });
    }

    void creditedOrdinaryAndFinancialHolds_data() {
        QTest::addColumn<int>("entry");
        QTest::newRow("active-financial") << 0;
        QTest::newRow("find-ordinary") << 1;
        QTest::newRow("financial-identity-reserved") << 2;
        QTest::newRow("ordinary-review-closure") << 3;
    }

    void creditedOrdinaryAndFinancialHolds() {
        QFETCH(int, entry);
        check([&] {
            CreditedHold f("financial-hold");
            require(f.financial.has_value(),
                    "Existing credited original has an actual financial hold");
            auto read = [&](const auto &s) {
                const SqliteApplicationHoldStore ordinary(s);
                if (entry == 0) {
                    const auto selected = SqliteFinancialRefundStore(s).active({f.file.cid});
                    require(selected.size() == 1 &&
                                finance::same(selected.front().locator, *f.financial),
                            "Financial hold retains the separate version bound to actual refund");
                } else if (entry == 1) {
                    const auto found = ordinary.find(f.detection.cause.hold_ref);
                    require(found && found->cause.timer &&
                                found->cause.timer->composition ==
                                    ApplicationHoldComposition::beside_complete_application &&
                                found->current_version.locator.version_ref == "v2",
                            "Selected ordinary hold retains its credited original and v2");
                } else if (entry == 2) {
                    auto attempted = f.file.ordinary_detection(f.financial->hold_ref, false);
                    require(ordinary.record_detection(attempted).disposition ==
                                SqliteHoldMutationDisposition::hold_conflict,
                            "Ordinary detection cannot claim a reserved financial hold identity");
                }
            };
            f.file.write(read);
            ContributionReview before(*f.file.writer, f.facts);
            auto command =
                entry == 3 ? std::optional{CreditedHold::bind(before, "close-ordinary",
                                                              f.intent(ReviewVerb::reject_held))}
                           : std::nullopt;
            const auto financial = f.file.rows("SELECT * FROM main.subathon_financial_refunds");
            upgrade_financial(f.file);
            if (entry < 3) {
                f.file.write(read);
            } else {
                ContributionReview after(*f.file.writer, f.facts);
                const auto result = Populated::execute(after, *command);
                expected_row(Populated::reply(result), 1, ReviewDisposition::committed,
                             ReviewStatus::rejected);
                f.file.write([&](const auto &s) {
                    const auto ordinary =
                        SqliteApplicationHoldStore(s).find(f.detection.cause.hold_ref);
                    const auto remaining = SqliteFinancialRefundStore(s).active({f.file.cid});
                    require(ordinary &&
                                ordinary->current_version.state == ApplicationHoldState::resolved &&
                                ordinary->current_version.parent_version_ref == "v2" &&
                                remaining.size() == 1 &&
                                finance::same(remaining.front().locator, *f.financial),
                            "Exact ordinary closure leaves its coexisting financial hold intact");
                });
            }
            require(f.file.rows("SELECT * FROM main.subathon_financial_refunds") == financial,
                    "The committed financial result stays byte-identical");
            f.file.check_head(50000, 0, 30000000, 7);
        });
    }

    void modifierNormalEntries_data() {
        QTest::addColumn<int>("entry");
        QTest::newRow("current-definition") << 0;
        QTest::newRow("live-instance") << 1;
        QTest::newRow("non-head-definition") << 2;
        QTest::newRow("prior-instance-revision") << 3;
    }

    void modifierNormalEntries() {
        QFETCH(int, entry);
        check([&] {
            namespace m = modifier_activation_fixture;
            using Operation = ModifierInstanceOperation;
            using Lifetime = ModifierInstanceLifetime;
            Populated f;
            f.db.write([](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.append_definition(m::old_definition, {}) ==
                                SqliteModifierDefinitionRevision{m::old_definition, 1} &&
                            store.activate("D", "z-old", {}, {"I", "j0", m::moderator, {}})
                                    .revision_to_append == m::initial_instance &&
                            store.checkpoint_remaining({"I", "j0"}, "j1", 180000).instance ==
                                m::checkpointed_instance &&
                            store.append_definition(m::new_definition, "z-old") ==
                                SqliteModifierDefinitionRevision{m::new_definition, 2},
                        "Owning writer produces live capture plus non-head definition/instance");
            });
            const auto read = [=](const auto &s) {
                const SqliteModifierActivationStore store(s);
                switch (entry) {
                case 0:
                    require(store.current_definition("D") ==
                                SqliteModifierDefinitionRevision{m::new_definition, 2},
                            "Current ordered definition revision and targets");
                    break;
                case 1:
                    require(store.live_instance("D") ==
                                SqliteModifierInstanceRevision{m::checkpointed_instance,
                                                               2,
                                                               Operation::checkpoint,
                                                               {},
                                                               Lifetime::live,
                                                               {}},
                            "Selected live revision still captures the old definition and 180 s");
                    break;
                case 2:
                    require(store.definition_revision("z-old") ==
                                SqliteModifierDefinitionRevision{m::old_definition, 1},
                            "Exact non-head definition retains the original factors");
                    break;
                case 3:
                    require(store.instance_revision("j0") ==
                                SqliteModifierInstanceRevision{m::initial_instance,
                                                               1,
                                                               Operation::activation,
                                                               "z-old",
                                                               Lifetime::live,
                                                               {}},
                            "Exact non-head instance contains the captured 480 s");
                    break;
                }
            };
            f.db.write(read);
            advance(f.db);
            f.db.write(read);
        });
    }

    void rateNormalEntries_data() {
        QTest::addColumn<bool>("head");
        QTest::addColumn<bool>("recover");
        QTest::newRow("head") << true << false;
        QTest::newRow("saved-revision") << false << false;
        QTest::newRow("retained-selection") << false << true;
    }

    void rateNormalEntries() {
        QFETCH(bool, head);
        QFETCH(bool, recover);
        check([&] {
            namespace rate = rate_fixture;
            Populated f;
            rate::Authority authority;
            const auto preview = recover ? rate::fetched_preview() : rate::first_preview();
            f.db.write([&](const auto &s) {
                const auto saved = SqliteRateOverrideUnit(s).save(
                    authority, rate::literal_rate_trust(), preview.edit, preview, preview.basis,
                    [] { return std::optional<std::int64_t>{3}; });
                const auto *receipt = std::get_if<RateReceiptSelection>(&saved);
                require(receipt && receipt->origin == RateReceiptOrigin::newly_staged &&
                            receipt->receipt.successor == "S",
                        "Actual authorized rate save stages S before a normal commit");
            });
            const auto read = [&](const auto &s) {
                SqliteRateOverrideUnit unit(s);
                if (recover) {
                    rate::Authority current;
                    const auto recovered =
                        unit.recover(current, rate::literal_rate_trust(), preview.edit, preview);
                    const auto *receipt = std::get_if<RateReceiptSelection>(&recovered);
                    require(receipt && current.reads == 1,
                            "Current authority and the exact retained selection context return a "
                            "receipt");
                    rate::expect_receipt(*receipt, RateReceiptOrigin::retained, preview);
                    const auto &change = receipt->receipt.changes.at(1);
                    require(receipt->receipt.successor == "S" && change.from_currency == "EUR" &&
                                change.to_currency == "USD" && change.after == rate::ratio(6, 5),
                            "Retained decision selected profile S and its exact EUR/USD 6/5 rate");
                }
                const auto snapshot = head ? unit.head() : unit.revision("S");
                const auto *selected = std::get_if<core::EffectiveRateSnapshot>(&snapshot);
                require(
                    selected &&
                        rate::snapshot_bytes(*selected) ==
                            rate::snapshot_bytes(preview.proposal.candidate) &&
                        selected->data().snapshot_ref == "S" &&
                        selected->data().components.size() == 1 &&
                        selected->data().components.front().members.size() == (recover ? 3 : 2) &&
                        selected->data().components.front().members[0].currency == "EUR" &&
                        selected->data().components.front().members[0].weight == rate::ratio(1) &&
                        selected->data().components.front().members.back().currency == "USD" &&
                        selected->data().components.front().members.back().weight ==
                            rate::ratio(6, 5),
                    "Reconstruct committed rate revision and select exact 6/5");
            };
            const auto rate_rows = [&] {
                return std::array{
                    f.db.rows(
                        "SELECT * FROM main.subathon_rate_observations ORDER BY observation_ref"),
                    f.db.rows("SELECT * FROM main.subathon_rate_revisions ORDER BY snapshot_ref"),
                    f.db.rows("SELECT * FROM main.subathon_rate_decisions ORDER BY decision_ref"),
                    f.db.rows("SELECT * FROM main.subathon_rate_occurrences ORDER BY override_ref"),
                    f.db.rows("SELECT * FROM main.subathon_rate_head")};
            };
            const auto before = recover ? std::optional{rate_rows()} : std::nullopt;
            f.db.write(read);
            if (before)
                require(rate_rows() == *before, "Recovery adds no retained observations or writes");
            advance(f.db);
            f.db.write(read);
            if (before)
                require(rate_rows() == *before,
                        "Upgrade and recovery preserve all rate provenance");
        });
    }

    void pendingSar2Milestone() {
        check([] {
            Finished f(true);
            const auto read = [&](const auto &s) {
                const auto pending = SqliteMilestoneGroupStore(s).pending();
                require(pending.size() == 1 &&
                            pending.front().group.achievement.milestone_ref == "threshold-100",
                        "Normal pending reader retains the actual SAR2 milestone origin");
            };
            f.db.write(read);
            advance(f.db);
            f.db.write(read);
        });
    }

    void milestonePartitionedOrigin_data() {
        QTest::addColumn<int>("entry");
        QTest::addColumn<bool>("damage");
        QTest::newRow("stage") << 0 << false;
        QTest::newRow("stage-corrupt-masked") << 0 << true;
        QTest::newRow("acceptance") << 1 << false;
        QTest::newRow("acceptance-corrupt-masked") << 1 << true;
        QTest::newRow("initialize") << 2 << false;
        QTest::newRow("initialize-corrupt-masked") << 2 << true;
        QTest::newRow("pending") << 3 << false;
        QTest::newRow("pending-corrupt-masked") << 3 << true;
    }

    void milestonePartitionedOrigin() {
        QFETCH(int, entry);
        QFETCH(bool, damage);
        check([&] {
            MilestoneOrigin f;
            f.stored();
            if (entry != 0)
                f.enroll();
            const auto read_or_stage = [&](const auto &s) {
                switch (entry) {
                case 0:
                    SqliteMilestoneGroupStore(s).stage(f.key, {f.group});
                    break;
                case 1:
                    SqliteMilestoneGroupDecisions(s).record_acceptance(f.key, f.acceptance);
                    break;
                case 2:
                    require(initialize_milestone_track(s, f.track()),
                            "Retained application membership initializes the exact prefix");
                    break;
                case 3: {
                    const auto pending = SqliteMilestoneGroupStore(s).pending();
                    require(pending.size() == 1 &&
                                pending.front().group.occurrence == f.application &&
                                pending.front().group.achievement.score_micro_points == 105,
                            "Selected pending group retains the actual SAR2 origin");
                    break;
                }
                }
                return true;
            };
            (void)read_missing(f.db, read_or_stage); // Full owning v19 path, without consumption.
            advance(f.db);
            if (!damage) {
                f.db.write([&](const auto &s) { (void)read_or_stage(s); });
                return;
            }
            mutate(f.db,
                   "DELETE FROM main.subathon_review_terminal_requests WHERE request_ordinal=1");
            const auto groups = f.db.rows("SELECT * FROM main.subathon_milestone_groups");
            const auto decisions =
                f.db.rows("SELECT * FROM main.subathon_milestone_acceptance_decisions");
            const auto heads = f.db.rows("SELECT * FROM main.subathon_milestone_prefix_heads");
            bool rejected = false;
            try {
                f.db.write([&](const auto &s) { (void)read_or_stage(s); });
            } catch (const SqliteBundleStorageFailure &e) {
                rejected = e.reason() == SqliteBundleStorageReason::invalid_stored_data;
            }
            require(rejected &&
                        f.db.rows("SELECT * FROM main.subathon_milestone_groups") == groups &&
                        f.db.rows("SELECT * FROM main.subathon_milestone_acceptance_decisions") ==
                            decisions &&
                        f.db.rows("SELECT * FROM main.subathon_milestone_prefix_heads") == heads,
                    "Corrupt origin cannot publish membership, decision, prefix or queue advances");
        });
    }

    void orderedApplicationAndEditNormalEntries_data() {
        QTest::addColumn<int>("entry");
        QTest::newRow("pending-queue") << 0;
        QTest::newRow("target-edit-read") << 1;
        QTest::newRow("target-edit-preview") << 2;
        QTest::newRow("edit-job") << 3;
        QTest::newRow("application-and-edit-release") << 4;
        QTest::newRow("corrupt-head-cannot-be-bypassed-masked") << 5;
    }

    void orderedApplicationAndEditNormalEntries() {
        QFETCH(int, entry);
        check([&] {
            namespace edit = milestone_target_edit_fixture;
            MilestoneOrigin f;
            f.enroll();
            f.record();
            edit::Authority authority;
            authority.facts.lifecycle.timer = finance::timer(core::TimerLifecycle::Finished, 99);
            authority.facts.captures = {edit::capture_b(true)};
            const auto intent =
                edit::single("lower-b", {"B", 104}, {{edit::b, MilestoneActionChoice::run}});
            f.db.write([&](const auto &s) {
                require(initialize_milestone_track(s, f.track()),
                        "Application-origin member anchors the real edit track");
                const auto preview =
                    preview_milestone_target_edit(s, authority, edit::actor, intent);
                edit::File::confirmable(preview);
                const auto confirmed = confirm_milestone_target_edit(s, authority, edit::actor,
                                                                     intent, *preview.confirmation);
                require(confirmed.result && confirmed.result->achievements.size() == 1 &&
                            confirmed.result->achievements.front().order == 2,
                        "Only the actual edit occurrence follows application queue order 1");
            });
            const MilestoneEditJobKey job{intent.key, edit::b, 1};
            const auto read = [&](const auto &s) {
                switch (entry) {
                case 0: {
                    const auto pending = SqliteMilestoneGroupStore(s).pending();
                    require(pending.size() == 2 && pending[0].group.occurrence == f.application &&
                                pending[1].group.occurrence == edit::b &&
                                pending[1].group.edit_membership &&
                                pending[1].group.edit_membership->jobs ==
                                    std::vector<MilestoneEditJobKey>{job},
                            "Mixed queue preserves application then edit membership");
                    break;
                }
                case 1: {
                    const auto found =
                        read_milestone_target_edit(s, authority, edit::actor, intent);
                    require(found && found->achievements.size() == 1 &&
                                found->achievements.front().order == 2,
                            "Read selects the retained edit and its application prefix");
                    break;
                }
                case 2: {
                    const auto preview = preview_milestone_target_edit(
                        s, authority, edit::actor, edit::single("keep", {"B", 104}));
                    require(preview.score == 105 && preview.previous_targets.size() == 2,
                            "Preview traverses both retained completion origins against the head");
                    break;
                }
                case 3: {
                    const auto found = lookup_milestone_edit_job(s, job);
                    require(found && found->action_document == QByteArray::fromHex("0081") &&
                                found->capture.sender_ref == "edit-sender",
                            "Normal job reader validates exact edit-origin membership");
                    break;
                }
                }
            };
            f.db.write(read);
            advance(f.db);
            if (entry < 4) {
                f.db.write(read);
                return;
            }
            const auto release = [&](auto key, bool edit_lane) {
                return finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    if (edit_lane)
                        return release_milestone_head(
                            f.db.writer(), key, std::make_shared<EditRelease>(), std::move(done));
                    return release_milestone_head(f.db.writer(), key, std::move(done));
                });
            };
            if (entry == 5) {
                mutate(f.db, "UPDATE main.subathon_review_terminal_requests SET disposition=2 "
                             "WHERE request_ordinal=1");
                const auto blocked = release(edit::b, true);
                require(blocked.work.transaction == TransactionOutcome::rolled_back &&
                            !blocked.disposition,
                        "Selecting the later edit cannot bypass corrupt earlier application head");
                require(f.db.rows("SELECT released FROM main.subathon_milestone_groups") ==
                                SqlRows{{0}} &&
                            f.db.rows(
                                "SELECT released FROM main.subathon_milestone_edit_occurrences") ==
                                SqlRows{{0}},
                        "Corrupt origin releases neither occurrence");
                return;
            }
            const auto application = release(f.application, false);
            finance::normal(application.work);
            require(application.disposition == MilestoneReleaseDisposition::released,
                    "Actual application release validates SAR2 and advances the queue head");
            const auto edited = release(edit::b, true);
            finance::normal(edited.work);
            require(edited.disposition == MilestoneReleaseDisposition::released,
                    "Edit release then consumes exactly the next edit");
        });
    }

    void densePre19AndUnknownMarker() {
        check([] {
            // The shipped early manifest and actual original precede terminal storage.
            finance::File original;
            original.write([](const auto &s) {
                require(SqliteApplicationBundleStore(s).lookup(finance::original).has_value(),
                        "Genuine pre-v19 dense lookup remains supported");
            });
            Populated f;
            const auto key = seed_dense(f);
            const auto read = [&](const auto &s) {
                require(SqliteApplicationBundleStore(s).lookup(key).has_value(),
                        "Genuine dense bundle resolves under the admitted catalog");
            };
            f.db.write(read);
            advance(f.db);
            f.db.write(read);
            const auto tail = successor().steps.back().version;
            mutate(f.db,
                   "UPDATE main.subathon_migration_history SET migration_id=? WHERE version=?",
                   {QString("Connector-intake-storage-v1"), tail});
            bool refused = false;
            try {
                f.db.write(read);
            } catch (const SqliteBundleStorageFailure &e) {
                refused = e.reason() == SqliteBundleStorageReason::invalid_stored_data;
            }
            require(refused, "Lookalike newer marker cannot use the historical dense dispatch");
        });
    }

    void denseExtraTerminalRow_data() {
        QTest::addColumn<bool>("upgraded");
        QTest::newRow("genuine-v19") << false;
        QTest::newRow("exact-successor") << true;
    }

    void denseExtraTerminalRow() {
        QFETCH(bool, upgraded);
        check([&] {
            Populated f;
            const auto key = seed_dense(f);
            QByteArray dense_document;
            const auto read = [&](const auto &s) {
                const auto row = SqliteApplicationBundleStore(s).lookup(key);
                require(
                    row && row->result.format_ref == "subathon.internal.application-result.v1" &&
                        QByteArrayView(row->result.document).startsWith("SAR1") &&
                        row->effects.size() == 2 && row->effects.front().request_ordinal == 1,
                    "The real producer committed a dense SAR1 result and both physical effects");
                if (dense_document.isEmpty())
                    dense_document = row->result.document;
                require(row->result.document == dense_document,
                        "The normal lookup reconstructs the exact pre-upgrade dense result");
            };
            f.db.write(read);
            if (upgraded) {
                advance(f.db);
                f.db.write(read);
            }
            const auto before = protected_data(f.db);
            require(before.back().isEmpty(), "Valid dense result requires zero terminal rows");
            const QList<QVariant> keys{QByteArray::fromStdString(key.operation_scope),
                                       QByteArray::fromStdString(key.operation_ref)};
            mutate(f.db,
                   "INSERT INTO main.subathon_review_terminal_requests "
                   "SELECT operation_scope,operation_ref,plan_ref,plan_revision,1,1 "
                   "FROM main.subathon_app_operations WHERE operation_scope=? AND operation_ref=?",
                   keys);
            const auto damaged = protected_data(f.db);
            require(
                damaged.back().size() == 1 && damaged.back().front().mid(4) == SqlRow{1, 1} &&
                    std::equal(before.begin(), before.end() - 1, damaged.begin()) &&
                    f.db.rows("SELECT * FROM pragma_foreign_key_check()").isEmpty(),
                "Only one extra, FK-valid terminal row was inserted on an existing effect ordinal");
            bool refused = false;
            try {
                f.db.write(read);
            } catch (const SqliteBundleStorageFailure &e) {
                refused = e.reason() == SqliteBundleStorageReason::invalid_stored_data;
            }
            require(protected_data(f.db) == damaged, "Lookup leaves the evidence unchanged");
            require(refused,
                    "Dense lookup must reject one extra terminal row after exact admission");
        });
    }

    void managedOrdinalEdges_data() {
        QTest::addColumn<int>("existing");
        QTest::newRow("empty-prefix") << 0;
        QTest::newRow("last-managed-append") << 1023;
        QTest::newRow("first-refused-append") << 1024;
        QTest::newRow("legacy-over-cap") << 1025;
        QTest::newRow("noncontiguous") << -3;
    }

    void managedOrdinalEdges() {
        QFETCH(int, existing);
        check([&] {
            Populated f;
            f.install();
            auto input = managed("ordinal", 8000, "60s/VL");
            std::optional<core::ReceiptId> id;
            if (existing)
                id = legacy_receipt(f.db, input, std::abs(existing));
            advance(f.db);
            stage_scope(f.db, "main");
            if (existing < 0)
                mutate(
                    f.db,
                    "DELETE FROM main.subathon_receipt_attempts WHERE receipt_id=? AND ordinal=2",
                    {blob8(id->value)});
            // The 1,025-row probe is one part of this unit. Keep separate
            // room for bounded metadata, retained closure and write readback.
            f.db.reopen({64, 256, 65536, 2097152, 1536, 4194304});
            const auto before = protected_data(f.db);
            const auto [result, work] = manage(f.db, input);
            if (result.stop == Stop::not_implemented)
                require(protected_data(f.db) == before,
                        "Missing managed receipt port cannot mutate existing ownership");
            const auto expected =
                existing < 0 || existing >= 1024 ? Stop::intake_limit : Stop::committed;
            require(
                result.stop == expected,
                "Managed receipt must prove the exact contiguous prefix and honor ordinal 1024");
            if (expected == Stop::committed) {
                finance::normal(work);
                const auto receipt = id.value_or(core::ReceiptId{1});
                require(result.attempt == AttemptLocator{receipt, existing + 1} &&
                            f.db.rows(
                                "SELECT count(*),max(ordinal) FROM main.subathon_receipt_attempts "
                                "WHERE receipt_id=?",
                                {blob8(receipt.value)}) == SqlRows{{existing + 1, existing + 1}},
                        "Managed commit returns the actual ordinal under the adopted owner");
            } else {
                require(work.transaction == TransactionOutcome::rolled_back &&
                            protected_data(f.db) == before &&
                            f.db.rows("SELECT count(*) FROM main.subathon_intake_entries") ==
                                SqlRows{{0}} &&
                            f.db.rows("SELECT count(*) FROM main.subathon_intake_basis") ==
                                SqlRows{{0}},
                        "Over-cap refusal publishes no attempt, retained context, intake or basis");
            }
        });
    }

    void managedReceiptOriginalRetryConflictAndReplay() {
        check([] {
            Populated f;
            f.install();
            for (int i = 0; i != 6; ++i)
                (void)f.intake("unmanaged-" + std::to_string(i));
            const auto original = managed("receipt", 8000, "60s/VL");
            auto retry = original;
            retry.intake_ref = "retry";
            retry.receipt.arrival = {9000, "connection-2", "cfg-2", "ctx-2"};
            retry.capture = "120s/VH";
            auto conflict = retry;
            conflict.intake_ref = "conflict";
            conflict.receipt.facts.money = core::ReceiptMoney{"21.00", "EUR"};
            f.db.write([](const auto &s) {
                SqliteRetainedContextStore retained(s);
                require(retained.retain({{RetainedKind::configuration, "cfg-2"}, "C2", {}}) ==
                                RetainDisposition::inserted &&
                            retained.retain({{RetainedKind::arrival_context, "ctx-2"},
                                             "M2",
                                             ConfigurationReference{"cfg-2"}}) ==
                                RetainDisposition::inserted,
                        "Retry's distinct configuration and arrival are real retained parents");
            });
            advance(f.db);
            stage_scope(f.db, "main");
            for (const auto &[request, ordinal] :
                 {std::pair{original, 1}, std::pair{retry, 2}, std::pair{conflict, 3}}) {
                const auto [result, work] = manage(f.db, request);
                require(result.stop == Stop::committed &&
                            result.attempt == AttemptLocator{core::ReceiptId{7}, ordinal},
                        "Managed receipt composition returns the actually inserted attempt");
                finance::normal(work);
                require(f.db.rows("SELECT receipt_id,attempt_ordinal,capture FROM "
                                  "main.subathon_intake_basis") ==
                            SqlRows{{blob8(7), 1, QByteArray("60s/VL")}},
                        "Original receipt COMMIT selects the first captured basis; retry and "
                        "conflict cannot replace it");
            }
            const auto physical = f.db.rows(
                "SELECT ordinal,disposition,server_timestamp_utc_ms,configuration_ref,"
                "modifier_context_ref FROM main.subathon_receipt_attempts WHERE receipt_id=? "
                "ORDER BY ordinal",
                {blob8(7)});
            require(
                physical == SqlRows{{1, 0, 8000, QByteArray("cfg"), QByteArray("ctx")},
                                    {2, 1, 9000, QByteArray("cfg-2"), QByteArray("ctx-2")},
                                    {3, 2, 9000, QByteArray("cfg-2"), QByteArray("ctx-2")}},
                "Receipt owns immutable first facts while retry and conflict retain their attempt");
            const auto before = protected_data(f.db);
            const auto [replay, work] = manage(f.db, retry);
            require(replay.stop == Stop::exact_replay &&
                        replay.attempt == AttemptLocator{core::ReceiptId{7}, 2} &&
                        protected_data(f.db) == before &&
                        f.db.rows("SELECT count(*) FROM main.subathon_contribution_observations") ==
                            SqlRows{{0}},
                    "Exact intake repeat reuses 7/2; no fourth attempt or evidence write");
        });
    }

    void managedRollbackLeavesPeerEligible() {
        check([] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db, "main");
            const auto left = managed("left", 8000, "60s/VL");
            const auto right = managed("right", 9000, "120s/VH");
            const auto before = protected_data(f.db);
            bool before_commit = false;
            const auto [rolled, reply] = manage(f.db, left, [&] {
                before_commit = true;
                return false; // Stop after actual staging, before its adopted COMMIT.
            });
            require(
                protected_data(f.db) == before &&
                    f.db.rows("SELECT count(*) FROM main.subathon_intake_basis") == SqlRows{{0}} &&
                    f.db.rows("SELECT count(*) FROM main.subathon_intake_entries") == SqlRows{{0}},
                "No original, attempt, context, entry or basis survives known rollback");
            require(rolled.stop == Stop::known_rollback && before_commit &&
                        reply.transaction == TransactionOutcome::rolled_back && !reply.value,
                    "First candidate reaches owned pre-COMMIT boundary before known cleanup");
            const auto [winner, work] = manage(f.db, right);
            finance::normal(work);
            require(
                winner.stop == Stop::committed &&
                    winner.attempt == AttemptLocator{core::ReceiptId{1}, 1} &&
                    f.db.rows("SELECT intake_ref,receipt_id,attempt_ordinal,capture "
                              "FROM main.subathon_intake_basis") ==
                        SqlRows{{QByteArray("right"), blob8(1), 1, QByteArray("120s/VH")}},
                "Peer's first successful receipt commit retains its own observation and capture");
        });
    }

    void fullManagedOrdinalWitness_data() {
        QTest::addColumn<bool>("atEdge");
        QTest::newRow("default-would-truncate") << false;
        QTest::newRow("ordinal-plus-two-closure-rows") << true;
    }

    void fullManagedOrdinalWitness() {
        QFETCH(bool, atEdge);
        check([&] {
            Populated f;
            const auto input = managed("ordinal-witness", 8000, "60s/VL");
            const auto id = legacy_receipt(f.db, input, 1025);
            auto limits = migration_profile();
            // This exact witness is only the indexed probe, original key, and
            // selected context footprint. The managed acceptance uses a
            // separate wider profile to leave room for all other unit reads.
            limits.result_rows = atEdge ? 1027 : 1026;
            f.db.reopen(limits);
            bool completed_probe = false;
            const auto work = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    const auto s = ctx.begin();
                    require(
                        s.execute(
                            {"SELECT receipt_id FROM main.subathon_receipts WHERE receipt_id=?",
                             {blob8(id.value)}}) == SqlRows{{blob8(id.value)}} &&
                            s.execute({"SELECT reference FROM main.subathon_retained_contexts "
                                       "WHERE reference=?",
                                       {QByteArray("ctx")}}) == SqlRows{{QByteArray("ctx")}},
                        "Original and selected context are proven before the ordinal-only probe");
                    const auto rows =
                        s.execute({"SELECT ordinal FROM main.subathon_receipt_attempts "
                                   "WHERE receipt_id=? ORDER BY ordinal LIMIT 1025",
                                   {blob8(id.value)}});
                    require(rows.size() == 1025, "Probe materializes the cap-plus-one witness");
                    for (qsizetype i = 0; i < rows.size(); ++i)
                        require(rows[i] == SqlRow{i + 1}, "Every ordinal is in its exact position");
                    completed_probe = true;
                    s.commit();
                    return 1;
                },
                10000);
            if (atEdge) {
                finance::normal(work);
                require(completed_probe, "All 1,027 rows fit in exactly one selected work unit");
            } else {
                require(!completed_probe && work.error == WorkError::result_limit &&
                            work.transaction == TransactionOutcome::rolled_back,
                        "One row below the witness rolls back without silently truncating");
            }
        });
    }

    void selectedHistoricalAttempt() {
        check([] {
            Populated f;
            f.install();
            const auto input = managed("long-legacy", 8000, "60s/VL");
            const auto id = legacy_receipt(f.db, input, 1025);
            // Legacy find materializes its limited attempt history and refuses.
            bool legacy_limited = false;
            (void)qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) -> QVariant {
                    SqliteReceiptRepository repo(ctx);
                    auto unit = repo.begin();
                    try {
                        (void)unit->find(input.receipt.identity);
                    } catch (const SqliteReceiptStorageFailure &e) {
                        legacy_limited = e.reason() == SqliteReceiptStorageReason::result_limit;
                    }
                    return {};
                },
                10000);
            require(legacy_limited, "Legacy full-history reader keeps its existing refusal");
            advance(f.db);
            stage_scope(f.db);
            f.db.reopen(); // 768 rows: whole attempt history cannot fit.
            const auto result = read_missing(
                f.db, [&](const auto &s) { return read_attempt(s, input, {id, 1025}); });
            auto arrival = input.receipt.arrival;
            arrival.server_timestamp_utc_ms = 9024;
            require(
                result.stop == Stop::exact_replay && result.attempt == AttemptLocator{id, 1025} &&
                    result.selected == core::ReceiptAttempt{arrival, input.receipt.facts,
                                                            core::ReceiptDisposition::retry},
                "Fixed-key readback returns selected historical attempt and its own provenance");
        });
    }

    void selectedAttemptExactClosure_data() {
        QTest::addColumn<int>("change");
        QTest::newRow("same-attempt") << 0;
        QTest::newRow("another-valid-ordinal") << 1;
        QTest::newRow("other-observed-facts") << 2;
        QTest::newRow("other-valid-configuration") << 3;
        QTest::newRow("other-valid-context") << 4;
        QTest::newRow("exact-reference-wrong-document-role") << 5;
        QTest::newRow("wrong-retained-scope") << 6;
    }

    void selectedAttemptExactClosure() {
        QFETCH(int, change);
        check([&] {
            Populated f;
            f.install();
            auto original = managed("closure", 8000, "60s/VL");
            (void)legacy_receipt(f.db, original);
            auto selected = original;
            selected.intake_ref = "retry";
            selected.receipt.arrival = {9000, "new-connection", "cfg-new", "ctx-new"};
            const auto wrong_role = change == 5
                                        ? encode_reference_execution({"ctx-new",
                                                                      "ctx",
                                                                      "I",
                                                                      selected.receipt.identity,
                                                                      {1},
                                                                      {0},
                                                                      {"closure-plan", 1},
                                                                      capture_rule()})
                                              .document
                                        : QByteArray{};
            f.db.write([&](const auto &s) {
                const SqliteRetainedContextStore store(s);
                require(store.retain(
                            {{RetainedKind::configuration, "cfg-new"}, "configuration-2", {}}) ==
                                RetainDisposition::inserted &&
                            store.retain({{RetainedKind::arrival_context, "ctx-new"},
                                          change == 5 ? wrong_role : QByteArray("context-2"),
                                          ConfigurationReference{"cfg-new"}}) ==
                                RetainDisposition::inserted,
                        "Both potentially selected context edges refer to real retained documents");
            });
            const auto appended = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    SqliteReceiptRepository repo(ctx);
                    const auto reply = ReceiptIntake(repo).ingest(selected.receipt);
                    require(reply.receipt_id == core::ReceiptId{1} &&
                                reply.disposition == core::ReceiptDisposition::retry,
                            "ACCT-001 commits a distinct retry without changing the original");
                    return 1;
                },
                10000);
            finance::normal(appended);
            advance(f.db);
            stage_scope(f.db);
            f.db.write([&](const auto &s) { entry_anchor(s, selected, core::ReceiptId{1}, 2); });
            require(f.db.rows("SELECT ordinal,server_timestamp_utc_ms,configuration_ref,"
                              "modifier_context_ref FROM main.subathon_receipt_attempts "
                              "ORDER BY ordinal") ==
                        SqlRows{{1, 8000, QByteArray("cfg"), QByteArray("ctx")},
                                {2, 9000, QByteArray("cfg-new"), QByteArray("ctx-new")}},
                    "Both ordinals and both pairs of retained references are present");
            if (change == 5)
                require(
                    f.db.rows(
                        "SELECT document FROM main.subathon_retained_contexts WHERE reference=?",
                        {QByteArray("ctx-new")}) == SqlRows{{wrong_role}} &&
                        wrong_role.startsWith("SRX1"),
                    "Receipt and attempt agree on one real retained execution-role document");
            auto bound = selected;
            if (change == 2)
                bound.receipt.facts.money = core::ReceiptMoney{"19.00", "EUR"};
            if (change == 3)
                bound.receipt.arrival.configuration_ref = "cfg";
            if (change == 4)
                bound.receipt.arrival.modifier_context_ref = "ctx";
            if (change == 6)
                bound.stream = QByteArray("S\0", 2);
            const auto before = protected_data(f.db);
            const auto result = read_missing(f.db, [&](const auto &s) {
                return read_attempt(s, bound, {core::ReceiptId{1}, change == 1 ? 1 : 2});
            });
            const auto expected = std::array{Stop::exact_replay,      Stop::different_ordinal,
                                             Stop::different_facts,   Stop::different_configuration,
                                             Stop::different_context, Stop::wrong_document_role,
                                             Stop::wrong_scope}
                                      .at(change);
            require(result.stop == expected && protected_data(f.db) == before,
                    "A valid independent FK cannot stand in for the exact selected closure");
            if (change == 0)
                require(result.attempt == AttemptLocator{core::ReceiptId{1}, 2} &&
                            result.selected ==
                                core::ReceiptAttempt{selected.receipt.arrival,
                                                     selected.receipt.facts,
                                                     core::ReceiptDisposition::retry},
                        "Selected attempt preserves its own actual observed facts and arrival");
            else
                require(!result.attempt && !result.selected,
                        "Malformed typed closure discloses no attempt or selected facts");
        });
    }

    void managedReceiptRecovery_data() {
        QTest::addColumn<bool>("lostAcknowledgment");
        QTest::newRow("lost-receipt-ack") << true;
        QTest::newRow("committed-reply-unavailable") << false;
    }

    void managedReceiptRecovery() {
        QFETCH(bool, lostAcknowledgment);
        check([&] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db, "main");
            auto input = managed("receipt-recovery", 8000, "60s/VL");
            if (lostAcknowledgment)
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Arm exactly the next managed receipt owning COMMIT while idle");
            bool reached = false;
            bool missing = false;
            const auto actual = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    const auto s = ctx.begin();
                    reached = true;
                    const auto result = stage_managed(s, input);
                    if (result.stop == Stop::not_implemented) {
                        missing = true;
                        s.rollback();
                        return 1;
                    }
                    require(result.stop == Stop::committed,
                            "Only acknowledgment of the adopted unit may return committed");
                    if (!lostAcknowledgment)
                        throw std::runtime_error("Abnormal completion after owning receipt COMMIT");
                    return 1;
                },
                10000);
            require(reached && !actual.value, "The selected fault path publishes no success");
            if (missing)
                require(actual.transaction == TransactionOutcome::rolled_back &&
                            f.db.rows("SELECT count(*) FROM main.subathon_intake_basis") ==
                                SqlRows{{0}},
                        "Test-only NI performs known cleanup before the future COMMIT boundary");
            require(!missing &&
                        actual.transaction == (lostAcknowledgment
                                                   ? TransactionOutcome::outcome_unknown
                                                   : TransactionOutcome::committed) &&
                        actual.error == (lostAcknowledgment ? WorkError::uncertain_outcome
                                                            : WorkError::work_exception),
                    "Missing or uncertain receipt reply must keep its real transaction category");
            if (lostAcknowledgment)
                require(f.db.writer().enqueue_work([](auto) { return QVariant{}; }, [](auto) {}) ==
                            StorageError::not_open,
                        "Uncertain owner disables later admission on the same writer");
            f.db.reopen();
            require(
                f.db.rows("SELECT receipt_id,ordinal FROM main.subathon_receipt_attempts") ==
                        SqlRows{{blob8(1), 1}} &&
                    f.db.rows("SELECT intake_ref,receipt_id,attempt_ordinal,capture "
                              "FROM main.subathon_intake_basis") ==
                        SqlRows{
                            {QByteArray("receipt-recovery"), blob8(1), 1, QByteArray("60s/VL")}} &&
                    f.db.rows("SELECT count(*) FROM main.subathon_contribution_observations") ==
                        SqlRows{{0}},
                "Actual media contains exactly the first selected receipt and its atomic basis");
            const auto settled = read_missing(f.db, [&](const auto &s) {
                return read_attempt(s, input, {core::ReceiptId{1}, 1});
            });
            require(settled.stop == Stop::exact_replay &&
                        settled.attempt == AttemptLocator{core::ReceiptId{1}, 1},
                    "Recovery selects the known attempt and consumes no second ordinal");
        });
    }

    void sourcePolicyFence_data() {
        QTest::addColumn<int>("scenario");
        QTest::newRow("new-only") << 0;
        QTest::newRow("stale-source-head") << 1;
        QTest::newRow("stale-policy-head") << 2;
        QTest::newRow("stale-generation") << 3;
        QTest::newRow("pre-switch-bind-control") << 4;
        QTest::newRow("pre-switch-finalize-control") << 5;
        QTest::newRow("new-only-denied-continuation") << 6;
        QTest::newRow("bind-current-generation-only-mismatch") << 7;
        QTest::newRow("finalize-current-generation-only-mismatch") << 8;
    }

    void sourcePolicyFence() {
        QFETCH(int, scenario);
        check([&] {
            Populated f(80000);
            f.install();
            const OldSourcePending pending(f);
            pending.read_capture(f.db);
            advance(f.db);
            stage_scope(f.db, "main");
            pending.read_member(f.db);
            pending.link_intake(f.db);
            mutate(f.db,
                   "UPDATE main.subathon_intake_gaps SET upper_cut=X'5a' WHERE gap_ref=X'47'");
            const HistoricalHead empty{1, 6, 1, "g1", {}, {}};
            const HistoricalHead baseline{
                1, 7, 1, "g1", QByteArray("initial-cut"), QByteArray("A")};
            const HistoricalHead old{1, 9, 1, "g3", QByteArray("cut"), QByteArray("F")};
            const HistoricalHead current{2, 10, 2, "g4", QByteArray("authorized-new-cut"), {}};
            const auto first = source_history(empty, baseline, "history-v1", "initial-authority",
                                              "first-connect-proof");
            const auto adoption = source_history(baseline, old, "history-v1", "adoption-authority",
                                                 "legacy-award-envelope-proof");
            anchor_source_history(f.db, "baseline", Purpose::baseline, empty, baseline, first);
            anchor_source_history(f.db, "adoption", Purpose::adoption, baseline, old, adoption);
            catchup(f.db);
            auto old_page = page_admission(1, {});
            old_page.binding.qualified_proof = coverage_document(
                SegmentCoverage::covered, "F", "E", "empty-proved-range", "g3/profile-qualified");
            old_page.binding.page += frame_fields(
                {old_page.binding.next_continuation, old_page.binding.qualified_proof});
            anchor_page(f.db, old_page.binding);
            const auto old_basis = f.db.rows("SELECT * FROM main.subathon_intake_basis");
            const auto page_before = intake_cursors(f.db);
            require(page_before[0] == SqlRows{{9, QByteArray("g3"), QByteArray("F")}} &&
                        page_before[1] ==
                            SqlRows{{QByteArray("O"), 2, 1, QByteArray("T"), 0, 0},
                                    {QByteArray("adoption"), 1, 1, QVariant{}, 0, 1},
                                    {QByteArray("baseline"), 1, 1, QVariant{}, 0, 1}} &&
                        page_before[2] == SqlRows{{1, 0, 0, 0}} && page_before[4].isEmpty(),
                    "Both page entries use the exact active g3 cursor and complete empty prefix");
            if (scenario == 4) {
                const auto before = fenced_history(f.db);
                std::optional<PageBindResult> result;
                const auto selected = transition(f.db, [&](const auto &s) {
                    result = bind_page(s, old_page);
                    return result->stop;
                });
                require(fenced_history(f.db) == before,
                        "Matched pre-switch bind retry preserves the exact page and all history");
                require(selected == Stop::exact_replay && result->committed_prefix == 0,
                        "The qualified old page is accepted as the exact bound page before switch");
                return;
            }
            if (scenario == 5) {
                const auto selected = transition(
                    f.db, [&](const auto &s) { return finalize_page(s, old_page.binding); });
                const auto after = intake_cursors(f.db);
                if (selected != Stop::committed)
                    require(after == page_before, "Missing finalization preserves pre-switch work");
                require(selected == Stop::committed && after[0].size() == 1 &&
                            after[0][0][2].toByteArray() == "E" && after[1].size() == 3 &&
                            after[1][0] == SqlRow{QByteArray("O"), 3, 2, QByteArray("U"), 0, 0} &&
                            after[2] == SqlRows{{1, 0, 0, 1}} && after[4].size() == 1 &&
                            after[4][0][2].toInt() == int(SegmentCoverage::covered) &&
                            after[4][0][3].toByteArray() == old_page.binding.qualified_proof &&
                            after[5] == page_before[5] &&
                            f.db.rows("SELECT proof FROM main.subathon_intake_pages") ==
                                SqlRows{{old_page.binding.qualified_proof}},
                        "The matched empty page advances its cursor and qualified frontier before "
                        "switch");
                return;
            }
            if (scenario == 7 || scenario == 8) {
                // Change only the live source generation. The request, the active
                // operation and the qualified proof remain the exact g3 specimen.
                auto expected = fenced_history(f.db);
                require(expected[0].size() == 1 && expected[0][0][5].toByteArray() == "g3",
                        "The matched source starts at the qualified generation");
                expected[0][0][5] = QByteArray("g4");
                const auto policy = [&] {
                    return std::array{
                        f.db.rows("SELECT * FROM main.subathon_intake_policies "
                                  "ORDER BY installation,revision"),
                        f.db.rows("SELECT * FROM main.subathon_intake_members ORDER BY "
                                  "installation,policy_revision,provider,account,stream,"
                                  "award_class,effect_scope")};
                };
                const auto retained_policy = policy();
                const auto protected_before = protected_data(f.db);
                mutate(f.db, "UPDATE main.subathon_intake_sources SET generation=? "
                             "WHERE installation=X'696e7374616c6c' AND provider=X'50' "
                             "AND account=X'41' AND stream=X'53'",
                       {QByteArray("g4")});
                const auto unchanged = [&] {
                    return fenced_history(f.db) == expected && policy() == retained_policy &&
                           protected_data(f.db) == protected_before &&
                           f.db.rows("SELECT * FROM main.subathon_intake_basis") == old_basis;
                };
                require(unchanged(),
                        "Only the current source generation differs from the matched control");
                std::optional<PageBindResult> binding;
                const auto result = transition(f.db, [&](const auto &s) {
                    const auto changes = s.execute({"SELECT total_changes()", {}});
                    Stop selected;
                    if (scenario == 7) {
                        binding = bind_page(s, old_page);
                        selected = binding->stop;
                    } else {
                        selected = finalize_page(s, old_page.binding);
                    }
                    require(s.execute({"SELECT total_changes()", {}}) == changes,
                            "No row is changed inside the refused page unit");
                    return selected;
                });
                require(unchanged(),
                        "Generation-only refusal changes no source, policy, history or intake row");
                require(result == Stop::stale && (!binding || !binding->committed_prefix),
                        "The current source generation alone fences the otherwise matched page");
                return;
            }
            const bool switch_expected = scenario == 0 || scenario == 6;
            const SourceChange input{"install",
                                     "P",
                                     "A",
                                     "S",
                                     1,
                                     scenario == 3 ? "stale-generation" : "g3",
                                     scenario == 1 ? 8 : 9,
                                     scenario == 2 ? 2 : 1,
                                     "g4",
                                     "P2",
                                     "new-only-member",
                                     "authorized-new-cut",
                                     "handoff",
                                     "handoff-authority",
                                     "history-v2",
                                     "new-only-both-sides-proof"};
            auto durable = [&] {
                return std::array{
                    f.db.rows("SELECT epoch,generation,revision,policy_revision,cut,frontier "
                              "FROM main.subathon_intake_sources"),
                    f.db.rows("SELECT revision,document FROM main.subathon_intake_policies "
                              "ORDER BY revision"),
                    f.db.rows("SELECT policy_revision,document FROM main.subathon_intake_members "
                              "ORDER BY policy_revision"),
                    f.db.rows("SELECT epoch,policy_revision,start_frontier,document,upper_cut "
                              "FROM main.subathon_intake_gaps")};
            };
            const SqlRows prior_history{
                {QByteArray("baseline"), int(Purpose::baseline), 1, 1, QByteArray("g1"), first,
                 QVariant{}, QVariant{}, 1, 0, int(HistoryOutcome::completed), QVariant{},
                 QByteArray("initial-cut")},
                {QByteArray("adoption"), int(Purpose::adoption), 1, 1, QByteArray("g3"), adoption,
                 QVariant{}, QVariant{}, 1, 0, int(HistoryOutcome::completed),
                 QByteArray("initial-cut"), QByteArray("cut")}};
            const auto old_work = fenced_history(f.db);
            const auto protected_before = protected_data(f.db);
            const auto before = durable();
            require(before[0] == SqlRows{{1, QByteArray("g3"), 9, 1, QByteArray("cut"),
                                          QByteArray("F")}} &&
                        before[1] == SqlRows{{1, QByteArray("review-policy")}} &&
                        before[2] == SqlRows{{1, QByteArray("review-lane")}} &&
                        before[3] == SqlRows{{1, 1, QByteArray("F"), QByteArray("gap-basis"),
                                              QByteArray("Z")}} &&
                        selected_source_history(f.db) == prior_history &&
                        !protected_before[2].isEmpty() && !protected_before[6].isEmpty(),
                    "The selected source, retained policy and old gap are independently bound");
            auto result = std::make_shared<std::optional<Stop>>();
            const auto work = qt_test_support::run_work(
                f.db.writer(),
                [result, input](auto context) -> QVariant {
                    const auto session = context.begin();
                    result->emplace(switch_source_policy(session, input));
                    if (**result == Stop::committed)
                        session.commit();
                    else
                        session.rollback();
                    return 1;
                },
                10000);
            require(result->has_value(), "Source and policy seam reached after real BEGIN");
            if (**result != Stop::committed)
                require(work.transaction == TransactionOutcome::rolled_back && !work.value &&
                            durable() == before && fenced_history(f.db) == old_work,
                        "Stale or missing CAS leaves every retained head and gap unchanged");
            require(**result == (switch_expected ? Stop::committed : Stop::stale),
                    "A single owner unit compares both captured heads and generation");
            if (switch_expected) {
                finance::normal(work);
                const auto after = durable();
                require(
                    after[0].size() == 1 &&
                        after[0][0].mid(0, 5) ==
                            SqlRow{2, QByteArray("g4"), 10, 2, QByteArray("authorized-new-cut")} &&
                        after[0][0][5].isNull() &&
                        after[1] ==
                            SqlRows{{1, QByteArray("review-policy")}, {2, QByteArray("P2")}} &&
                        after[2] == SqlRows{{1, QByteArray("review-lane")},
                                            {2, QByteArray("new-only-member")}} &&
                        after[3] == before[3],
                    "Commit fences the source and policy together and carries old gap provenance");
                auto expected_history = prior_history;
                expected_history.append(
                    SqlRow{QByteArray("handoff"), int(Purpose::handoff), 2, 2, QByteArray("g4"),
                           source_history(old, current, "history-v2", "handoff-authority",
                                          "new-only-both-sides-proof"),
                           QVariant{}, QVariant{}, 1, 0, int(HistoryOutcome::completed),
                           QByteArray("cut"), QByteArray("authorized-new-cut")});
                f.db.reopen({64, 768, 65536, 8585216, 768, 16105472});
                require(selected_source_history(f.db) == expected_history && durable() == after &&
                            protected_data(f.db) == protected_before &&
                            f.db.rows("SELECT * FROM main.subathon_intake_basis") == old_basis,
                        "Reopen reads original baseline and adoption cuts and authority, plus the "
                        "exact two-sided handoff");
                pending.read_capture(f.db);
                pending.read_member(f.db);
                const auto switched_work = fenced_history(f.db);
                require(
                    f.db.rows("SELECT epoch,policy_revision,generation,next_ordinal,"
                              "continuation,slots,outcome FROM main.subathon_intake_operations "
                              "WHERE operation_ref=X'4f'") ==
                            SqlRows{{1, 1, QByteArray("g3"), 1, QByteArray("T"), 0,
                                     int(HistoryOutcome::superseded)}} &&
                        switched_work[2] == old_work[2] && switched_work[3] == old_work[3] &&
                        switched_work[4] == old_work[4] && switched_work[5] == old_work[5],
                    "The switch retires old history work and retains its bound page and coverage");
                const auto unchanged = [&] {
                    require(
                        fenced_history(f.db) == switched_work && durable() == after &&
                            protected_data(f.db) == protected_before &&
                            f.db.rows("SELECT * FROM main.subathon_intake_basis") == old_basis,
                        "Old-generation work cannot change the switched heads, cursor or coverage");
                };
                std::optional<PageBindResult> binding;
                const auto bind_stop = transition(f.db, [&](const auto &s) {
                    binding = bind_page(s, old_page);
                    return binding->stop;
                });
                unchanged();
                const auto finalize_stop = transition(
                    f.db, [&](const auto &s) { return finalize_page(s, old_page.binding); });
                unchanged();
                require(bind_stop == Stop::stale && !binding->committed_prefix &&
                            finalize_stop == Stop::stale,
                        "The old generation refuses the matched request at both page entries");
                pending.continue_review(f, scenario == 6);
                require(fenced_history(f.db) == switched_work &&
                            f.db.rows("SELECT * FROM main.subathon_intake_basis") == old_basis,
                        "Review continuation preserves the old intake binding and fenced history");
            }
        });
    }

    void oldSourceReviewContinuation_data() {
        QTest::addColumn<bool>("deny");
        QTest::newRow("genuine-v19-current-authorized") << false;
        QTest::newRow("genuine-v19-current-denied") << true;
    }

    void oldSourceReviewContinuation() {
        QFETCH(bool, deny);
        check([&] {
            Populated f(80000);
            f.install();
            const OldSourcePending pending(f);
            f.db.reopen({64, 768, 65536, 8585216, 768, 16105472});
            pending.continue_review(f, deny);
        });
    }

    void baselineAndAdoptionHistory_data() {
        QTest::addColumn<bool>("adopt");
        QTest::newRow("first-connect") << false;
        QTest::newRow("legacy-adoption") << true;
    }

    void baselineAndAdoptionHistory() {
        QFETCH(bool, adopt);
        check([&] {
            Populated f;
            intake_origin(f);
            mutate(f.db, "DELETE FROM main.subathon_intake_gaps");
            mutate(
                f.db,
                "UPDATE main.subathon_intake_sources SET baseline_state=0,cut=NULL,frontier=NULL");
            const auto purpose = adopt ? Purpose::adoption : Purpose::baseline;
            const QByteArray proof = adopt ? "outside-old-award-envelope" : "first-connect-fence";
            const SourceChange input{"install",
                                     "P",
                                     "A",
                                     "S",
                                     1,
                                     "g3",
                                     9,
                                     1,
                                     "g4",
                                     "P2",
                                     "new-only-member",
                                     "new-cut",
                                     "history",
                                     "current-decision",
                                     "history-v2",
                                     proof,
                                     purpose};
            const auto before = fenced_history(f.db);
            require(before[1].isEmpty() && before[2].isEmpty() && before[3].isEmpty(),
                    "Pending source has no baseline evidence, historical pages or inferred gap");
            const auto stop =
                transition(f.db, [&](const auto &s) { return switch_source_policy(s, input); });
            if (stop != Stop::committed)
                require(fenced_history(f.db) == before,
                        "A missing or refused baseline writes no history");
            require(stop == Stop::committed,
                    "Qualified baseline activates its head and records its authority");
            f.db.reopen();
            const HistoricalHead prior{1, 9, 1, "g3", {}, {}};
            const HistoricalHead selected{2, 10, 2, "g4", QByteArray("new-cut"), {}};
            require(
                selected_source_history(f.db) ==
                        SqlRows{{QByteArray("history"), int(purpose), 2, 2, QByteArray("g4"),
                                 source_history(prior, selected, "history-v2", "current-decision",
                                                proof),
                                 QVariant{}, QVariant{}, 1, 0, int(HistoryOutcome::completed),
                                 QVariant{}, QByteArray("new-cut")}} &&
                    f.db.rows("SELECT epoch,generation,revision,policy_revision,baseline_state,cut "
                              "FROM main.subathon_intake_sources") ==
                        SqlRows{{2, QByteArray("g4"), 10, 2, int(BaselineState::qualified),
                                 QByteArray("new-cut")}} &&
                    f.db.rows("SELECT count(*) FROM main.subathon_intake_pages") == SqlRows{{0}} &&
                    f.db.rows("SELECT count(*) FROM main.subathon_intake_gaps") == SqlRows{{0}},
                "Baseline/adoption retains explicit absent prior cut and exact proof without "
                "history paging");
        });
    }

    void operationBinding_data() {
        QTest::addColumn<int>("scenario");
        QTest::newRow("first-binding") << 0;
        QTest::newRow("same-immutable-intent") << 1;
        QTest::newRow("same-key-different-utc-start") << 2;
        QTest::newRow("different-active-operation") << 3;
        QTest::newRow("same-key-different-exact-source") << 4;
        QTest::newRow("same-key-different-upper-cut") << 5;
        QTest::newRow("replay-old-cut-at-new-fence") << 6;
        QTest::newRow("same-key-requalified-at-new-fence") << 7;
    }

    void operationBinding() {
        QFETCH(int, scenario);
        check([&] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db);
            const auto bound = import_frame();
            if (scenario != 0)
                mutate(f.db,
                       "INSERT INTO main.subathon_intake_operations VALUES("
                       "X'696e7374616c6c',X'4f',X'50',X'41',X'53',1,1,NULL,1,X'6733',1,?,X'54',1,0,"
                       "0,NULL,NULL)",
                       {bound});
            if (scenario == 4) {
                mutate(f.db, "INSERT INTO main.subathon_intake_members VALUES("
                             "X'696e7374616c6c',1,X'50',X'41',X'5300',X'737570706f7274',"
                             "X'73636f7265',X'6f7665726c61792d32',1,X'6c')");
                mutate(f.db, "INSERT INTO main.subathon_intake_sources "
                             "SELECT installation,provider,account,X'5300',epoch,generation,"
                             "revision,policy_revision,baseline_state,cut,frontier,document "
                             "FROM main.subathon_intake_sources");
            }
            auto intent = import_intent();
            // Exercise the exact immutable comparison at the storage boundary.
            // Recalculating the supplied zone result belongs to API-006.
            if (scenario == 2)
                ++intent.resolved_utc_start;
            if (scenario == 3)
                intent.operation_ref = "Q";
            if (scenario == 4)
                intent.stream = QByteArray("S\0", 2);
            if (scenario == 5) {
                intent.upper_cut.position = "Z2";
                intent.upper_cut.proof = "proof-for-Z2";
            }
            if (scenario >= 6) {
                // Anchor the result of a matching-cursor reconnect in owned fixture rows.
                // The source and operation fences move; the cut and retained decision do not.
                f.db.write([](const auto &s) {
                    for (const auto *table :
                         {"subathon_intake_sources", "subathon_intake_operations"})
                        (void)s.execute({QString("UPDATE main.%1 SET generation=?,"
                                                 "revision=revision+1")
                                             .arg(table),
                                         {QByteArray("g4")}});
                });
                f.db.reopen();
                intent.generation = "g4";
                intent.current_authority = "actor/reauthorized";
                if (scenario == 7)
                    intent.upper_cut.qualified_generation = "g4";
                require(f.db.rows("SELECT generation,revision FROM main.subathon_intake_sources") ==
                                SqlRows{{QByteArray("g4"), 10}} &&
                            f.db.rows("SELECT generation,revision,intent "
                                      "FROM main.subathon_intake_operations") ==
                                SqlRows{{QByteArray("g4"), 2, bound}},
                        "The operative fence moved while the old cut and decision stayed exact");
            }
            const auto retained =
                scenario >= 6 ? std::optional{fenced_history(f.db)} : std::nullopt;
            const auto before =
                f.db.rows("SELECT operation_ref,intent,revision,next_ordinal,"
                          "continuation,slots FROM main.subathon_intake_operations");
            auto result = std::make_shared<std::optional<Stop>>();
            const auto work = qt_test_support::run_work(
                f.db.writer(),
                [result, intent](auto context) -> QVariant {
                    const auto session = context.begin();
                    result->emplace(bind_history_operation(session, intent));
                    if (**result == Stop::committed)
                        session.commit();
                    else
                        session.rollback();
                    return 1;
                },
                10000);
            require(result->has_value(), "Operation binding seam reached after writer ownership");
            const auto expected =
                std::array{Stop::committed, Stop::exact_replay, Stop::conflict,     Stop::busy,
                           Stop::conflict,  Stop::conflict,     Stop::exact_replay, Stop::conflict}
                    .at(scenario);
            const auto after = f.db.rows("SELECT operation_ref,intent,revision,next_ordinal,"
                                         "continuation,slots FROM main.subathon_intake_operations");
            if (**result != Stop::committed)
                require(work.transaction == TransactionOutcome::rolled_back && !work.value &&
                            after == before,
                        "Exact selection or known refusal changes no durable history slot");
            if (retained)
                require(fenced_history(f.db) == *retained,
                        "Replay or changed qualification cannot rewrite the historical proof, "
                        "retained authority or operative fence");
            require(
                **result == expected,
                "Exact reuse, changed immutable intent and a competing active key are distinct");
            if (expected == Stop::committed) {
                finance::normal(work);
                require(after == SqlRows{{QByteArray("O"), bound, 1, 1, QByteArray("T"), 0}} &&
                            f.db.rows("SELECT installation,provider,account,stream,epoch,"
                                      "policy_revision,purpose,gap_ref,generation "
                                      "FROM main.subathon_intake_operations") ==
                                SqlRows{{QByteArray("install"), QByteArray("P"), QByteArray("A"),
                                         QByteArray("S"), 1, 1, int(Purpose::import), QVariant{},
                                         QByteArray("g3")}},
                        "The empty operation starts with the independent exact intent and cursor");
            }
        });
    }

    void quarantineLimits_data() {
        QTest::addColumn<int>("scenario");
        QTest::newRow("entry-at-32768") << 0;
        QTest::newRow("entry-over-provenance-inclusive") << 1;
        QTest::newRow("stream-count") << 2;
        QTest::newRow("installation-bytes-spare-count") << 3;
        QTest::newRow("installation-count-spare-bytes") << 4;
        QTest::newRow("exact-replay-at-full-stream") << 5;
        QTest::newRow("maximum-item-too-large-with-provenance") << 6;
    }

    void quarantineLimits() {
        QFETCH(int, scenario);
        check([&] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db);
            f.db.reopen(finance::limits());
            const auto charge = scenario == 3 || scenario < 2 ? 32768 : 1024;
            const auto streams = scenario == 3                    ? 2
                                 : scenario == 4                  ? 4
                                 : scenario == 2 || scenario == 5 ? 1
                                                                  : 0;
            const auto stream_name = [](int i) {
                return QByteArray("stream-") + QByteArray::number(i);
            };
            const QByteArray provenance("fixture-proof");
            // Test input documents use one tag and two BE32 length fields.
            // The SQL anchor stores the normalized/provenance components and
            // the full independently derived declared charge, not an unframed estimate.
            const auto document = [&](qsizetype bytes) {
                return QByteArray(bytes - 9 - provenance.size(), 'n');
            };
            const auto append = [&](const auto &s, int index) {
                (void)s.execute(
                    {"INSERT INTO main.subathon_intake_sources "
                     "VALUES(X'696e7374616c6c',X'50',X'41',?,1,X'6733',1,1,1,NULL,NULL,X'73')",
                     {stream_name(index)}});
                (void)s.execute({"INSERT INTO main.subathon_intake_members "
                                 "VALUES(X'696e7374616c6c',1,X'50',X'41',?,X'737570706f7274',X'"
                                 "73636f7265',?,1,X'6c')",
                                 {stream_name(index), stream_name(index)}});
            };
            for (int i = 0; i <= streams; ++i) {
                f.db.write([&](const auto &s) {
                    append(s, i);
                    if (i == streams)
                        return;
                    for (int n = 0; n < 256; ++n)
                        (void)s.execute(
                            {"INSERT INTO main.subathon_intake_entries("
                             "installation,provider,account,stream,intake_ref,epoch,generation,"
                             "policy_revision,"
                             "input,provenance,arrival_ms,phase,disposition,quarantine_bytes)"
                             "VALUES(X'696e7374616c6c',X'50',X'41',?,?,1,X'6733',1,?,?,8000,2,1,?)",
                             {stream_name(i), QByteArray::number(n), document(charge), provenance,
                              charge}});
                });
            }
            f.db.reopen();
            const auto image = [&] {
                return f.db.rows(
                    "SELECT stream,count(*),sum(quarantine_bytes) "
                    "FROM main.subathon_intake_entries GROUP BY stream ORDER BY stream");
            };
            const auto before = image();
            require(before.size() == streams,
                    "Only populated fixture streams count towards the installation quota");
            for (int i = 0; i < streams; ++i)
                require(before[i] == SqlRow{stream_name(i), 256, 256 * charge},
                        "Beyond individual count, aggregate canonical bytes are independently "
                        "asserted");
            const auto target = scenario == 2 || scenario == 5 ? 0 : streams;
            const auto requested_charge = scenario == 1   ? 32769
                                          : scenario == 6 ? 32768 + 9 + provenance.size()
                                                          : charge;
            const QuarantineInput input{"install",
                                        "P",
                                        "A",
                                        stream_name(target),
                                        scenario == 5 ? "17" : "new",
                                        document(requested_charge),
                                        provenance};
            require(input.normalized.size() + input.provenance.size() + 9 == requested_charge &&
                        input.normalized.size() <= 32768,
                    "Over-limit entry is independently caused by charged framing and provenance");
            auto selected = std::make_shared<std::optional<Stop>>();
            const auto result = qt_test_support::run_work(
                f.db.writer(),
                [=](auto ctx) {
                    const auto s = ctx.begin();
                    selected->emplace(admit_quarantine(s, input));
                    if (**selected == Stop::committed)
                        s.commit();
                    else
                        s.rollback();
                    return 1;
                },
                10000);
            require(selected->has_value(), "Quarantine limit seam reached after real scoped setup");
            if (**selected != Stop::committed)
                require(result.transaction == TransactionOutcome::rolled_back && !result.value &&
                            image() == before,
                        "Typed overflow or replay leaves exact committed count and bytes");
            const auto expected =
                std::array{Stop::committed,       Stop::intake_limit,    Stop::quarantine_full,
                           Stop::quarantine_full, Stop::quarantine_full, Stop::exact_replay,
                           Stop::intake_limit}
                    .at(scenario);
            require(**selected == expected,
                    "Component cap, stream cap, installation caps and exact retry are independent");
            if (expected == Stop::committed) {
                finance::normal(result);
                require(image() == SqlRows{{stream_name(0), 1, 32768}},
                        "Inclusive entry cap consumes one exact canonical charge");
            }
        });
    }

    void pageCas_data() {
        QTest::addColumn<int>("scenario");
        QTest::newRow("prefix-missing-second-item") << 0;
        QTest::newRow("complete-effect-free-prefix") << 1;
        QTest::newRow("stale-source-revision") << 2;
        QTest::newRow("different-continuation") << 3;
        QTest::newRow("different-page-bytes") << 4;
        QTest::newRow("lost-final-page-ack") << 5;
        QTest::newRow("known-before-commit-rollback") << 6;
        QTest::newRow("stale-generation") << 7;
    }

    void pageCas() {
        QFETCH(int, scenario);
        check([&] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db);
            f.db.write([&](const auto &s) {
                (void)s.execute({"INSERT INTO main.subathon_intake_operations "
                                 "VALUES(X'696e7374616c6c',X'4f',X'50',X'41',X'53',1,1,X'47',0,X'"
                                 "6733',2,?,X'54',1,2,0,NULL,NULL)",
                                 {QByteArray("literal-catchup-intent")}});
                (void)s.execute({"INSERT INTO main.subathon_intake_pages "
                                 "VALUES(X'696e7374616c6c',X'4f',1,?,X'54',2,?,0,NULL,0)",
                                 {QByteArray("two-exact-items"), scenario == 0 ? 1 : 2}});
                for (int ordinal = 1; ordinal <= (scenario == 0 ? 1 : 2); ++ordinal)
                    (void)s.execute(
                        {"INSERT INTO main.subathon_intake_entries("
                         "installation,provider,account,stream,intake_ref,epoch,generation,policy_"
                         "revision,"
                         "operation_ref,page_ordinal,item_ordinal,input,provenance,arrival_ms,"
                         "phase,disposition)"
                         "VALUES(X'696e7374616c6c',X'50',X'41',X'53',?,1,X'6733',1,X'4f',1,?,?,?,"
                         "8000,2,1)",
                         {QByteArray::number(ordinal), ordinal, QByteArray("verified-hold"),
                          QByteArray("effect-free-disposition")}});
            });
            auto durable = [&] {
                return std::array{
                    f.db.rows("SELECT revision,next_ordinal,continuation FROM "
                              "main.subathon_intake_operations"),
                    f.db.rows("SELECT revision,frontier FROM main.subathon_intake_sources"),
                    f.db.rows(
                        "SELECT ordinal,committed_prefix,outcome FROM main.subathon_intake_pages"),
                    f.db.rows("SELECT * FROM main.subathon_intake_segments")};
            };
            const auto before = durable();
            const auto fence_before =
                scenario == 7 ? std::optional{fenced_history(f.db)} : std::nullopt;
            require(before[0] == SqlRows{{2, 1, QByteArray("T")}} &&
                        before[1] == SqlRows{{9, QByteArray("F")}} &&
                        before[2] == SqlRows{{1, scenario == 0 ? 1 : 2, 0}} && before[3].isEmpty(),
                    "Operation cursor and last proved frontier are independently bound");
            const PageBinding request{"O",
                                      scenario == 2 ? 8 : 9,
                                      2,
                                      scenario == 3 ? "wrong-token" : "T",
                                      scenario == 4 ? "changed-bytes" : "two-exact-items",
                                      1,
                                      2,
                                      "U",
                                      "F",
                                      "E",
                                      "qualified-contiguous-proof",
                                      PageBinding::Coverage::contiguous,
                                      {},
                                      1,
                                      1,
                                      1,
                                      scenario == 7 ? "g2" : "g3"};
            const auto expected =
                std::array{Stop::incomplete_prefix, Stop::committed, Stop::stale,
                           Stop::conflict,          Stop::conflict,  Stop::committed,
                           Stop::committed,         Stop::stale}
                    .at(scenario);
            if (scenario == 5)
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Fault is scoped to the final page COMMIT, after bound prefix");
            auto selected = std::make_shared<std::optional<Stop>>();
            const auto work = qt_test_support::run_work(
                f.db.writer(),
                [=](auto ctx) {
                    const auto s = ctx.begin();
                    selected->emplace(finalize_page(s, request));
                    if (**selected == Stop::committed && scenario != 6)
                        s.commit();
                    else
                        s.rollback();
                    return 1;
                },
                10000);
            require(selected->has_value(), "Final-page seam reached in its own real writer unit");
            if (**selected != Stop::committed)
                require(work.transaction == TransactionOutcome::rolled_back && !work.value &&
                            durable() == before,
                        "Known first stop preserves separately committed prefix, cursor and gap");
            if (fence_before)
                require(fenced_history(f.db) == *fence_before,
                        "Stale generation leaves every source, operation, page, gap and item row "
                        "unchanged");
            require(
                **selected == expected,
                "Final CAS distinguishes incomplete prefix, exact completion, stale and conflict");
            if (expected == Stop::committed) {
                if (scenario == 6) {
                    require(
                        work.transaction == TransactionOutcome::rolled_back && !work.value &&
                            durable() == before,
                        "Known failure before final COMMIT preserves bound page and both items");
                    return;
                }
                if (scenario == 5) {
                    require(work.error == WorkError::uncertain_outcome &&
                                work.transaction == TransactionOutcome::outcome_unknown &&
                                !work.value &&
                                f.db.writer().enqueue_work([](auto) { return QVariant{}; },
                                                           [](auto) {}) == StorageError::not_open,
                            "Uncertain final COMMIT yields no reply or suffix on old writer");
                    f.db.reopen();
                } else {
                    finance::normal(work);
                }
                const auto after = durable();
                require(after[0] == SqlRows{{3, 2, QByteArray("U")}} &&
                            after[1] == SqlRows{{10, QByteArray("E")}} &&
                            after[2] == SqlRows{{1, 2, 1}} && after[3].size() == 1,
                        "One CAS advances token and evidence-qualified contiguous frontier");
                if (scenario == 5) {
                    const auto retry = read_missing(
                        f.db, [&](const auto &s) { return finalize_page(s, request); });
                    require(retry == Stop::exact_replay && durable() == after,
                            "Recovered final CAS prefix selects the exact durable result once");
                }
            }
        });
    }

    void peerEvidenceAfterDurableRecovery_data() {
        QTest::addColumn<bool>("reverse");
        QTest::addColumn<int>("phase");
        QTest::newRow("live-restart-before-evidence") << false << 0;
        QTest::newRow("history-restart-before-evidence") << true << 0;
        QTest::newRow("live-lost-evidence-ack") << false << 1;
        QTest::newRow("history-lost-evidence-ack") << true << 1;
        QTest::newRow("live-abnormal-evidence-completion") << false << 2;
        QTest::newRow("history-abnormal-evidence-completion") << true << 2;
        QTest::newRow("legacy-original-no-basis") << false << 3;
        QTest::newRow("wrong-original-with-live-basis") << false << 4;
    }

    void peerEvidenceAfterDurableRecovery() {
        QFETCH(bool, reverse);
        QFETCH(int, phase);
        check([&] {
            Populated f;
            f.install();
            auto live = managed("live", reverse ? 9000 : 8000, "60s/VL");
            auto history = managed("history", reverse ? 8000 : 9000, "120s/VH");
            const auto live_id = legacy_receipt(f.db, live);
            const auto history_id = legacy_receipt(f.db, history);
            const auto &first = reverse ? history : live;
            const auto first_id = reverse ? history_id : live_id;
            const auto &peer = reverse ? live : history;
            const auto peer_id = reverse ? live_id : history_id;
            advance(f.db);
            stage_scope(f.db, "main");
            peer_anchors(f.db, first, first_id, peer, peer_id, phase != 3);
            if (phase == 1) {
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Existing idle writer arms the exact next owning evidence COMMIT");
            }
            if (phase == 1 || phase == 2) {
                const auto [staged, work] = evidence_work(f.db, first, first_id, phase == 2);
                require(!work.value &&
                            work.transaction == (phase == 1 ? TransactionOutcome::outcome_unknown
                                                            : TransactionOutcome::committed) &&
                            work.error == (phase == 1 ? WorkError::uncertain_outcome
                                                      : WorkError::work_exception),
                        "Real evidence COMMIT outcomes distinguish lost ACK and abnormal body");
                if (phase == 1)
                    require(!staged &&
                                f.db.writer().enqueue_work([](auto) { return QVariant{}; },
                                                           [](auto) {}) == StorageError::not_open,
                            "Lost acknowledgment closes admission and produces no typed reply");
            } else if (phase >= 3) {
                const auto result =
                    phase == 3 ? evidence(f.db, first, first_id) : evidence(f.db, peer, peer_id);
                require(result.disposition == ContributionLinkDisposition::created &&
                            result.contribution_id == core::ContributionId{0},
                        "Existing producer owns exact original and CID before the gate");
            }
            f.db.reopen();
            const auto before = protected_data(f.db);
            const auto anchor =
                f.db.rows("SELECT original_receipt_id FROM main.subathon_contributions");
            const auto exact =
                phase == 0 ? SqlRows{} : SqlRows{{blob8((phase == 4 ? peer_id : first_id).value)}};
            require(anchor == exact &&
                        f.db.rows("SELECT count(*) FROM main.subathon_intake_basis") ==
                            SqlRows{{phase == 3 ? 0 : 1}},
                    "Independently inspect actual media and exact binding before retrying");
            const auto basis = f.db.rows("SELECT * FROM main.subathon_intake_basis");
            const auto selected = read_missing(
                f.db, [&](const auto &s) { return select_evidence(s, phase == 4 ? first : peer); });
            if (phase == 4) {
                const auto other =
                    read_missing(f.db, [&](const auto &s) { return select_evidence(s, peer); });
                require(
                    other.stop == Stop::inconsistent_original && !other.selected_basis &&
                        !other.original && !other.capture && !selected.selected_basis &&
                        !selected.original && !selected.capture,
                    "Both the selected basis and contradictory original are held without reuse");
                require(f.db.rows("SELECT * FROM main.subathon_intake_basis") == basis,
                        "Neither side can replace the selected basis");
            }
            require(protected_data(f.db) == before,
                    "Recovered evidence remains separate from the gate unit");
            const auto expected = phase == 0   ? Stop::pending_original
                                  : phase == 4 ? Stop::inconsistent_original
                                               : Stop::may_link;
            require(selected.stop == expected &&
                        (phase != 3 || (!selected.capture && !selected.selected_basis)),
                    "Recover the exact durable prefix; reuse earlier unbound original without "
                    "recapture");
            if (expected == Stop::may_link)
                require(selected.original == core::ContributionId{0},
                        "Peer links only to the one known durable CID");
        });
    }

    void gapOpening_data() {
        QTest::addColumn<QString>("edge");
        for (const auto *name : {"exact", "stale-source", "cut-result-at", "cut-result-over",
                                 "cut-request-at", "cut-request-over", "proof-at", "proof-over",
                                 "reference-at", "reference-over", "position-at", "position-over"})
            QTest::newRow(name) << QString(name);
    }

    void gapOpening() {
        QFETCH(QString, edge);
        check([&] {
            Populated f;
            intake_origin(f);
            mutate(f.db, "DELETE FROM main.subathon_intake_gaps");
            GapOpening request{"install",
                               "P",
                               "A",
                               "S",
                               "G",
                               1,
                               9,
                               1,
                               "g3",
                               "g4",
                               "F",
                               "Z",
                               "cursor-contract",
                               "loss-observation",
                               "bounded-cut-request"};
            const bool over = edge.endsWith("-over");
            const auto frame = [](int bytes) {
                return QByteArray("Q") + frame_fields({QByteArray(bytes - 5, 'p')});
            };
            if (edge.startsWith("cut-result"))
                request.upper_cut = frame(16384 + over);
            if (edge.startsWith("cut-request"))
                request.cut_request = frame(16384 + over);
            if (edge.startsWith("proof"))
                request.observation = frame(16384 + over);
            if (edge.startsWith("reference"))
                request.gap_ref = QByteArray(4096 + over, 'g');
            if (edge.startsWith("position")) {
                request.frontier = QByteArray(4096 + over, 'f');
                mutate(f.db, "UPDATE main.subathon_intake_sources SET frontier=?",
                       {QByteArray(4096, 'f')});
            }
            if (edge == "stale-source")
                --request.source_revision;
            const auto before = intake_cursors(f.db);
            const auto selected =
                transition(f.db, [&](const auto &s) { return open_gap(s, request); });
            const auto after = intake_cursors(f.db);
            if (selected != Stop::committed)
                require(after == before,
                        "An unopened gap leaves both the source head and gap history unchanged");
            const auto expected = over                     ? Stop::intake_limit
                                  : edge == "stale-source" ? Stop::stale
                                                           : Stop::committed;
            require(selected == expected,
                    "Gap opening fences the source and charges each qualified field independently");
            if (expected == Stop::committed)
                require(
                    after[0] == SqlRows{{10, QByteArray("g4"), request.frontier}} &&
                        after[3] == SqlRows{{1, 0, request.frontier, request.upper_cut}} &&
                        after[4].isEmpty(),
                    "New generation retains the last proved frontier without asserting coverage");
        });
    }

    void emptyCoverage_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("unproved-exhaustion") << 0;
        QTest::newRow("proved-empty-range") << 1;
        QTest::newRow("proved-loss") << 2;
        QTest::newRow("append-recovery-after-loss") << 3;
        QTest::newRow("append-fresh-baseline-after-loss") << 4;
        QTest::newRow("unproved-frontier-advance-refused") << 5;
    }

    void emptyCoverage() {
        QFETCH(int, kind);
        check([&] {
            Populated f;
            intake_origin(f);
            catchup(f.db);
            auto page = page_admission(1, {}).binding;
            const bool fresh_baseline = kind == 4;
            const bool earlier_loss = kind == 3 || kind == 4;
            if (earlier_loss) {
                mutate(f.db, "INSERT INTO main.subathon_intake_operations VALUES("
                             "X'696e7374616c6c',X'6c6f7373',X'50',X'41',X'53',1,1,X'47',0,"
                             "X'6733',3,X'6c',X'54',2,0,1,NULL,NULL)");
                page.gap_revision = 2;
                mutate(f.db, "UPDATE main.subathon_intake_gaps SET revision=2,status=1");
                mutate(f.db,
                       "INSERT INTO main.subathon_intake_segments VALUES("
                       "X'696e7374616c6c',X'50',X'41',X'53',1,X'47',X'6c6f7373',2,1,1,?)",
                       {coverage_document(SegmentCoverage::lost, "F", "E", "provider-retention",
                                          "prior-loss-proof")});
            }
            using Coverage = PageBinding::Coverage;
            page.coverage =
                std::array{Coverage::unproved,  Coverage::contiguous,   Coverage::lost,
                           Coverage::recovered, Coverage::new_baseline, Coverage::contiguous}
                    .at(kind);
            if (kind == 0)
                page.frontier_after = "F";
            if (kind > 0 && kind < 4) {
                const QByteArray reason =
                    std::array<QByteArray, 3>{"empty-proved-range", "provider-retention",
                                              "recovered-range"}
                        .at(kind - 1);
                page.qualified_proof =
                    coverage_document(kind == 2 ? SegmentCoverage::lost : SegmentCoverage::covered,
                                      "F", "E", reason, "profile-qualified-proof");
            }
            QByteArray fresh_cut;
            SqlRows unresolved_range;
            SqlRows prior_loss;
            const auto original_range = [&] {
                return f.db.rows("SELECT installation,provider,account,stream,epoch,gap_ref,"
                                 "policy_revision,status,start_frontier,upper_cut,document "
                                 "FROM main.subathon_intake_gaps WHERE gap_ref=X'47'");
            };
            const auto loss_segment = [&] {
                return f.db.rows("SELECT * FROM main.subathon_intake_segments "
                                 "WHERE gap_ref=X'47' AND operation_ref=X'6c6f7373'");
            };
            if (fresh_baseline) {
                // Profile evidence orders F < E < Y < Z. The source still fences at F,
                // the original lost range is (F,E], and the new segment proves only (Y,Z].
                mutate(f.db, "UPDATE main.subathon_intake_gaps SET upper_cut=? WHERE gap_ref=X'47'",
                       {QByteArray("E")});
                unresolved_range = original_range();
                prior_loss = loss_segment();
                fresh_cut = QByteArray("CUT1\x01", 5) + frame_fields({"key-v1", "S/history"}) +
                            present_bytes(QByteArray("Z")) +
                            frame_fields({"g3", "qualified-resume-after-E"});
                page.upper_bound = "Z";
                page.frontier_after = "Z";
                page.qualified_proof = coverage_document(SegmentCoverage::covered, "Y", "Z",
                                                         "fresh-qualified-cut", fresh_cut);
                page.page += frame_fields({page.next_continuation, page.qualified_proof});
                require(unresolved_range ==
                                SqlRows{{QByteArray("install"), QByteArray("P"), QByteArray("A"),
                                         QByteArray("S"), 1, QByteArray("G"), 1,
                                         int(GapStatus::loss_recorded), QByteArray("F"),
                                         QByteArray("E"), QByteArray("gap-basis")}} &&
                            prior_loss.size() == 1 &&
                            prior_loss[0].back().toByteArray() ==
                                coverage_document(SegmentCoverage::lost, "F", "E",
                                                  "provider-retention", "prior-loss-proof"),
                        "The baseline starts with an exact unresolved original loss");
            }
            anchor_page(f.db, page);
            const auto before = intake_cursors(f.db);
            const auto durable_before =
                fresh_baseline ? std::optional{fenced_history(f.db)} : std::nullopt;
            const auto selected =
                transition(f.db, [&](const auto &s) { return finalize_page(s, page); });
            const auto after = intake_cursors(f.db);
            if (selected != Stop::committed)
                require(after == before &&
                            (!durable_before || fenced_history(f.db) == *durable_before),
                        "A missing finalization leaves even the bound empty page intact");
            if (kind == 5) {
                require(
                    selected == Stop::unproved_range && after == before,
                    "An asserted frontier advance without qualified evidence refuses atomically");
                return;
            }
            require(selected == Stop::committed,
                    "An empty page can settle its cursor independently of coverage");
            require(
                after[1].size() == (earlier_loss ? 2 : 1) &&
                    after[1].front() == SqlRow{QByteArray("O"), 3, 2, QByteArray("U"), 0, 0} &&
                    (!earlier_loss || after[1].back() == before[1].back()) &&
                    after[2] == SqlRows{{1, 0, 0, 1}} && after[5] == SqlRows{{0}},
                "The empty page changes no item disposition and consumes exactly one page ordinal");
            require(f.db.rows("SELECT proof FROM main.subathon_intake_pages WHERE ordinal=1") ==
                        SqlRows{{page.qualified_proof.isNull() ? QVariant{}
                                                              : QVariant{page.qualified_proof}}},
                    "The page retains its exact qualified proof or explicit absence");
            const auto frontier = QByteArray(fresh_baseline           ? "Z"
                                             : kind == 1 || kind == 3 ? "E"
                                                                      : "F");
            require(
                after[0].size() == 1 && after[0][0][2].toByteArray() == frontier,
                "Only qualified coverage, recovery or a separate baseline changes the frontier");
            if (kind == 0) {
                require(after[3] == before[3] && after[4].isEmpty(),
                        "Token-only exhaustion leaves the range unresolved");
            } else {
                require(after[4].size() == (earlier_loss ? 2 : 1) &&
                            after[4].back()[2].toInt() ==
                                int(kind == 2 ? SegmentCoverage::lost : SegmentCoverage::covered) &&
                            after[4].back()[3].toByteArray() == page.qualified_proof &&
                            (!earlier_loss || after[4].front() == before[4].front()),
                        "Coverage appends its own proof and preserves any prior loss verbatim");
            }
            if (fresh_baseline) {
                require(
                    original_range() == unresolved_range && loss_segment() == prior_loss &&
                        f.db.rows("SELECT cut,frontier FROM main.subathon_intake_sources") ==
                            SqlRows{{fresh_cut, QByteArray("Z")}} &&
                        f.db.rows("SELECT gap_ref,coverage,document "
                                  "FROM main.subathon_intake_segments WHERE operation_ref=X'4f'") ==
                            SqlRows{{QByteArray("G"), int(SegmentCoverage::covered),
                                     page.qualified_proof}},
                    "The new cut and segment advance independently and leave the original "
                    "range unresolved, with no frontier bridge");
            }
        });
    }

    void pageAdmission_data() {
        QTest::addColumn<QString>("edge");
        for (const auto *name :
             {"new",         "exact-retry",          "changed-token",     "changed-input",
              "stale",       "stale-generation",     "item-at",           "item-over",
              "envelope-at", "envelope-over",        "count-at",          "count-over",
              "cursor-at",   "cursor-over",          "ordinal-at",        "ordinal-over",
              "slots-at",    "slots-and-count-over", "bound-cursor-over", "resume-bound-prefix"})
            QTest::newRow(name) << QString(name);
    }

    void pageAdmission() {
        QFETCH(QString, edge);
        check([&] {
            Populated f;
            intake_origin(f);
            const bool slot_edge = edge.startsWith("slots");
            const bool ordinal = edge.startsWith("ordinal");
            const bool over = edge.endsWith("-over");
            const bool resume = edge == "resume-bound-prefix";
            const auto prior = ordinal && over ? 256 : slot_edge || ordinal ? 255 : 0;
            catchup(f.db, prior + 1, slot_edge ? 4080 : 0);
            if (prior)
                completed_prefix(f.db, prior, slot_edge ? 16 : 0);
            QList<QByteArray> items{normalized_item(18)};
            if (resume)
                items.append(normalized_item(19));
            if (edge.startsWith("item"))
                items = {normalized_item(32768 + over)};
            if (edge.startsWith("count") || slot_edge)
                items = QList<QByteArray>(16 + over, normalized_item(18));
            if (edge.startsWith("envelope")) {
                // 1 tag + 4 count + 8 four-byte lengths; all eight items fit individually.
                items = QList<QByteArray>(7, normalized_item(32768));
                items.append(normalized_item(32731 + over));
            }
            auto request = page_admission(prior + 1, items);
            if (edge.startsWith("envelope"))
                require(request.binding.page.size() == 262144 + over,
                        "Page envelope edge includes every item tag and length");
            const bool anchored = edge == "exact-retry" || resume || edge.startsWith("changed");
            if (anchored) {
                mutate(f.db, "UPDATE main.subathon_intake_operations SET slots=?", {items.size()});
                anchor_page(f.db, request.binding);
            }
            if (resume) {
                f.db.write([&](const auto &s) {
                    (void)s.execute({"INSERT INTO main.subathon_intake_entries("
                                     "installation,provider,account,stream,intake_ref,epoch,"
                                     "generation,policy_revision,"
                                     "operation_ref,page_ordinal,item_ordinal,input,provenance,"
                                     "arrival_ms,phase,disposition)"
                                     "VALUES(X'696e7374616c6c',X'50',X'41',X'53',X'31',1,X'6733',1,"
                                     "X'4f',1,1,?,X'70',8000,2,1)",
                                     {items.front()}});
                    (void)s.execute({"UPDATE main.subathon_intake_pages SET committed_prefix=1 "
                                     "WHERE operation_ref=X'4f' AND ordinal=1",
                                     {}});
                });
                f.db.reopen();
            }
            if (edge == "changed-input")
                request = page_admission(1, {normalized_item(19)});
            if (edge == "changed-token")
                request.binding.expected_continuation = "wrong";
            if (edge == "stale")
                --request.binding.operation_revision;
            if (edge == "stale-generation")
                request.binding.generation = "g2";
            if (edge.startsWith("cursor")) {
                request.binding.expected_continuation = QByteArray(4096 + over, 't');
                mutate(f.db, "UPDATE main.subathon_intake_operations SET continuation=?",
                       {QByteArray(4096, 't')});
            }
            if (edge == "bound-cursor-over") {
                mutate(f.db, "UPDATE main.subathon_intake_operations SET slots=1");
                anchor_page(f.db, request.binding);
                request.binding.expected_continuation = QByteArray(4097, 't');
            }
            // Storage proves the bound prefix. API-006 selects which item to dispatch.
            const auto fence_before = edge == "stale-generation" || resume
                                          ? std::optional{fenced_history(f.db)}
                                          : std::nullopt;
            const auto before = intake_cursors(f.db);
            if (resume)
                require(before[2] == SqlRows{{1, 2, 1, 0}} && before[5] == SqlRows{{1}},
                        "A genuine bound page has exactly one durable effect-free item");
            std::optional<PageBindResult> result;
            const auto selected = transition(f.db, [&](const auto &s) {
                result = bind_page(s, request);
                return result->stop;
            });
            const auto after = intake_cursors(f.db);
            if (selected != Stop::committed)
                require(
                    after == before,
                    "Refusal or exact retry leaves the bound prefix, scope and slots unchanged");
            if (fence_before)
                require(fenced_history(f.db) == *fence_before,
                        "Stale bind or exact prefix read leaves all retained rows unchanged");
            // Byte admission precedes exact token/page comparison, even on a bound page.
            const auto expected = over                              ? Stop::intake_limit
                                  : edge.startsWith("stale")        ? Stop::stale
                                  : edge == "exact-retry" || resume ? Stop::exact_replay
                                  : anchored                        ? Stop::conflict
                                                                    : Stop::committed;
            require(selected == expected, "Page admission compares the exact key, cursor and bytes "
                                          "and enforces inclusive caps");
            if (selected == Stop::exact_replay)
                require(result->committed_prefix == (resume ? 1 : 0),
                        "Exact retry reports the proved ordinal prefix for the orchestrator");
            if (selected == Stop::committed) {
                require(f.db.rows(
                            "SELECT page,expected_continuation,item_count,committed_prefix,outcome "
                            "FROM main.subathon_intake_pages WHERE ordinal=?",
                            {prior + 1}) == SqlRows{{request.binding.page,
                                                     request.binding.expected_continuation,
                                                     items.size(), 0, 0}} &&
                            after[5] == before[5] && after[0] == before[0] &&
                            after[4] == before[4] && after[1][0][2].toInt() == prior + 1 &&
                            after[1][0][4].toInt() == (slot_edge ? 4096 : items.size()),
                        "Binding charges the unprocessed slots without starting any item unit");
            }
        });
    }

    void capRollover_data() {
        QTest::addColumn<bool>("stale");
        QTest::newRow("exact-cap") << false;
        QTest::newRow("stale-operation-revision") << true;
    }

    void capRollover() {
        QFETCH(bool, stale);
        check([&] {
            Populated f;
            intake_origin(f);
            catchup(f.db, 257, 4096);
            completed_prefix(f.db, 256, 16);
            const Rollover request{
                "O", "next", "T", "F", "catchup:F:Z", "current-authority", stale ? 256 : 258, 9};
            const auto before = intake_cursors(f.db);
            const auto selected =
                transition(f.db, [&](const auto &s) { return rollover_operation(s, request); });
            const auto after = intake_cursors(f.db);
            if (selected != Stop::committed)
                require(after == before,
                        "Refused rollover preserves the full operation and coverage");
            require(selected == (stale ? Stop::stale : Stop::committed),
                    "Only the exact authorized cap head can roll over");
            if (!stale) {
                require(
                    f.db.rows("SELECT operation_ref,intent,continuation,next_ordinal,slots,epoch,"
                              "gap_ref FROM main.subathon_intake_operations WHERE outcome=0") ==
                            SqlRows{{QByteArray("next"), QByteArray("catchup:F:Z"), QByteArray("T"),
                                     1, 0, 1, QByteArray("G")}} &&
                        after[0] == before[0] && after[2] == before[2] && after[3] == before[3] &&
                        after[4] == before[4] && after[5] == before[5],
                    "Rollover retains exact bounds, frontier, token, gap and completed prefix");
            }
        });
    }

    void intentFraming_data() {
        QTest::addColumn<bool>("over");
        QTest::addColumn<bool>("bound");
        QTest::newRow("at-16384") << false << false;
        QTest::newRow("over-16385") << true << false;
        QTest::newRow("bound-over-16385") << true << true;
    }

    void intentFraming() {
        QFETCH(bool, over);
        QFETCH(bool, bound);
        check([&] {
            Populated f;
            intake_origin(f);
            // The variable field is an opaque transcript of an already qualified cut.
            // Its fixed CUT1 fields take 44 bytes. The outer import fields, including
            // the cut length, take 165 bytes. The cut itself fits on both edges.
            constexpr int cut_overhead = 44;
            constexpr int outer_overhead = 165;
            const QByteArray proof(16384 + over - outer_overhead - cut_overhead, 'p');
            const auto frame = import_frame(proof);
            require(frame.size() == 16384 + over && proof.size() + cut_overhead <= 16384,
                    "The canonical intent alone crosses its cap with a valid bounded cut");
            const auto request = import_intent(proof);
            if (bound)
                mutate(f.db,
                       "INSERT INTO main.subathon_intake_operations VALUES("
                       "X'696e7374616c6c',X'4f',X'50',X'41',X'53',1,1,NULL,1,X'6733',1,?,X'54',1,0,"
                       "0,NULL,NULL)",
                       {import_frame(QByteArray(16384 - outer_overhead - cut_overhead, 'p'))});
            const auto before = fenced_history(f.db);
            const auto selected =
                transition(f.db, [&](const auto &s) { return bind_history_operation(s, request); });
            const auto rows =
                f.db.rows("SELECT intent,next_ordinal,slots FROM main.subathon_intake_operations");
            if (selected != Stop::committed)
                require(fenced_history(f.db) == before,
                        "Admission refusal creates no operation or alters the existing intent");
            require(selected == (over ? Stop::intake_limit : Stop::committed),
                    "Intent size is charged before staging or comparing preexisting intent bytes");
            if (!over)
                require(rows == SqlRows{{frame, 1, 0}}, "Within limit retains exact framed intent");
        });
    }

    void selectedIntakePage_data() {
        QTest::addColumn<QString>("edge");
        for (const auto *name : {"entry-keyset", "byte-keyset", "omit-documents",
                                 "indivisible-exact", "wrong-installation"})
            QTest::newRow(name) << QString(name);
    }

    void selectedIntakePage() {
        QFETCH(QString, edge);
        check([&] {
            Populated f;
            f.install();
            const bool exact = edge == "indivisible-exact";
            auto input = managed("01", 8000, "large-selected-parent");
            core::ReceiptId id{};
            if (exact) {
                input.receipt.arrival.configuration_ref = "big-config";
                f.db.write([](const auto &s) {
                    require(SqliteRetainedContextStore(s).retain(
                                {{RetainedKind::configuration, "big-config"},
                                 QByteArray(262144, 'c'),
                                 {}}) == RetainDisposition::inserted,
                            "Real retained configuration owns a 256 KiB document");
                });
                id = legacy_receipt(f.db, input);
            }
            advance(f.db);
            stage_scope(f.db);
            const bool wide = edge == "byte-keyset" || edge == "omit-documents";
            const QByteArray doc(wide ? 32768 : 64, 'n');
            if (exact) {
                f.db.write([&](const auto &s) { entry_anchor(s, input, id, 1); });
                require(
                    f.db.rows("SELECT length(document) FROM main.subathon_retained_configurations "
                              "WHERE reference=X'6269672d636f6e666967'") == SqlRows{{262144}},
                    "The indivisible retained parent alone fills the exact readback budget");
            } else {
                f.db.write([&](const auto &s) {
                    for (int i = 1; i <= 17; ++i)
                        (void)s.execute(
                            {"INSERT INTO main.subathon_intake_entries("
                             "installation,provider,account,stream,intake_ref,epoch,generation,"
                             "policy_revision,input,provenance,arrival_ms,phase,disposition) "
                             "VALUES("
                             "X'696e7374616c6c',X'50',X'41',X'53',?,1,X'6733',1,?,X'70',8000,2,1)",
                             {QByteArray::number(i).rightJustified(2, '0'), doc}});
                });
                require(f.db.rows("SELECT count(*),min(length(input)),max(length(input)) "
                                  "FROM main.subathon_intake_entries") ==
                            SqlRows{{17, doc.size(), doc.size()}},
                        "Exact ordered population is independent of the readback port");
            }
            SelectedKey key{edge == "wrong-installation" ? "elsewhere" : "install",
                            "P",
                            "A",
                            "S",
                            exact ? "01" : "",
                            edge != "omit-documents",
                            exact};
            const auto read = [&](const SelectedKey &request) {
                return read_missing(f.db,
                                    [&](const auto &s) { return inspect_intake(s, request); });
            };
            const auto page = read(key);
            if (exact || edge == "wrong-installation") {
                require(page.stop == (exact ? Stop::intake_limit : Stop::wrong_scope) &&
                            page.entries.isEmpty() && !page.next_after,
                        "Oversized exact closure or foreign installation returns no projection");
                return;
            }
            // A nonempty key + framing costs bytes beyond each payload. Eight
            // full-size documents alone exhaust the budget. Seven fit.
            const int count = edge == "byte-keyset" ? 7 : 16;
            const auto first = [](int n) { return QByteArray::number(n).rightJustified(2, '0'); };
            const auto compare = [&](const SelectedPage &p, int from, int through) {
                QList<SelectedEntry> expected;
                for (int i = from; i <= through; ++i)
                    expected.append(
                        {first(i), key.include_documents ? std::optional{doc} : std::nullopt});
                require(p.stop == Stop::exact_replay && p.entries == expected,
                        "Selected projection is bounded and ordered by exact BLOB keys");
            };
            compare(page, 1, count);
            require(page.next_after == first(count), "Continuation is the last included exact key");
            key.after = *page.next_after;
            const auto later = read(key);
            compare(later, count + 1, std::min(17, count * 2));
            if (count == 7) {
                require(later.next_after == "14", "Second byte-limited closure preserves its key");
                key.after = *later.next_after;
                const auto last = read(key);
                compare(last, 15, 17);
                require(!last.next_after, "Final byte-limited page is exhausted");
            } else {
                require(!later.next_after, "Final count-limited page is exhausted");
            }
        });
    }

    void resolveQuarantineScope_data() {
        QTest::addColumn<int>("scenario");
        QTest::newRow("authorized-release-and-identical-retry") << 0;
        QTest::newRow("foreign-installation") << 1;
        QTest::newRow("foreign-admission-separate-store") << 2;
    }

    void resolveQuarantineScope() {
        QFETCH(int, scenario);
        const bool foreign = scenario != 0;
        check([&] {
            Populated f;
            intake_origin(f);
            f.db.write([](const auto &s) {
                (void)s.execute(
                    {"WITH RECURSIVE e(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM e WHERE n<256) "
                     "INSERT INTO main.subathon_intake_entries("
                     "installation,provider,account,stream,intake_ref,epoch,generation,"
                     "policy_revision,input,provenance,arrival_ms,phase,disposition,quarantine_"
                     "bytes) "
                     "SELECT X'696e7374616c6c',X'50',X'41',X'53',CAST(printf('%d',n) AS BLOB),"
                     "1,X'6733',1,?,X'70',8000,2,1,32768 FROM e",
                     {normalized_item(32758)}});
            });
            const auto quota = [&] {
                return f.db.rows("SELECT count(quarantine_bytes),sum(quarantine_bytes),count(*) "
                                 "FROM main.subathon_intake_entries");
            };
            if (scenario == 2) {
                Populated other;
                other.install("other");
                advance(other.db);
                const auto foreign_state = other.db.rows(
                    "SELECT installation_ref FROM main.subathon_account_installation");
                require(
                    foreign_state == SqlRows{{QByteArray("other")}} &&
                        other.db.rows("SELECT count(*) FROM main.subathon_intake_entries") ==
                            SqlRows{{0}},
                    "A second disposable store owns its own installation and independent quota");
                const QuarantineInput wrong{"install", "P", "A", "S", "17", normalized_item(32758),
                                            "p"};
                const auto denied =
                    transition(other.db, [&](const auto &s) { return admit_quarantine(s, wrong); });
                require(other.db.rows("SELECT count(*) FROM main.subathon_intake_entries") ==
                            SqlRows{{0}},
                        "Wrong-installation admission changes no row in the second store");
                require(denied == Stop::wrong_scope && quota() == SqlRows{{256, 8388608, 256}},
                        "Read the local singleton before any opaque key in either store");
                return;
            }
            const auto before = quota();
            require(before == SqlRows{{256, 8388608, 256}},
                    "Full stream charges all unresolved entries");
            const QuarantineInput new_entry{"install", "P", "A", "S", "new", normalized_item(32758),
                                            "p"};
            catchup(f.db);
            auto first_page = page_admission(1, {new_entry.normalized});
            first_page.quarantine_preflight = new_entry;
            const auto bind = [&] {
                return transition(f.db,
                                  [&](const auto &s) { return bind_page(s, first_page).stop; });
            };
            const auto admit = [&] {
                return transition(f.db,
                                  [&](const auto &s) { return admit_quarantine(s, new_entry); });
            };
            if (!foreign) {
                const auto bound_before = intake_cursors(f.db);
                require(bind() == Stop::quarantine_full && intake_cursors(f.db) == bound_before &&
                            quota() == before,
                        "The otherwise valid page refuses before binding, reserving slots or "
                        "starting item work");
            }
            auto selected = new_entry;
            selected.key = "17";
            selected.installation = foreign ? "other" : "install";
            const auto result = transition(f.db, [&](const auto &s) {
                return resolve_quarantine(s, {selected, "current-authority"});
            });
            if (result != Stop::committed)
                require(quota() == before, "Refusal leaves the full stream intact");
            require(result == (foreign ? Stop::wrong_scope : Stop::committed),
                    "Resolution is qualified to the installation and exact selected hold");
            if (!foreign) {
                require(quota() == SqlRows{{255, 8355840, 256}},
                        "One resolved entry remains auditable and releases one charge");
                require(
                    bind() == Stop::committed && quota() == SqlRows{{255, 8355840, 256}} &&
                        f.db.rows("SELECT page,item_count,committed_prefix FROM "
                                  "main.subathon_intake_pages") ==
                            SqlRows{{first_page.binding.page, 1, 0}},
                    "The identical page now binds without charging or processing its item twice");
                require(admit() == Stop::committed && quota() == SqlRows{{256, 8388608, 257}},
                        "The admitted hold consumes exactly the one released charge");
            }
        });
    }

    void managedFirstCommit_data() {
        QTest::addColumn<bool>("historyFirst");
        QTest::addColumn<int>("fault");
        QTest::newRow("live-first-restart") << false << 0;
        QTest::newRow("history-first-restart") << true << 0;
        QTest::newRow("live-rollback-history-first") << true << 1;
        QTest::newRow("history-rollback-live-first") << false << 1;
        QTest::newRow("live-first-receipt-ack-loss") << false << 2;
        QTest::newRow("history-first-receipt-ack-loss") << true << 2;
        QTest::newRow("live-first-receipt-abnormal") << false << 3;
        QTest::newRow("history-first-receipt-abnormal") << true << 3;
        QTest::newRow("live-first-evidence-ack-loss") << false << 4;
        QTest::newRow("history-first-evidence-ack-loss") << true << 4;
        QTest::newRow("live-first-evidence-abnormal") << false << 5;
        QTest::newRow("history-first-evidence-abnormal") << true << 5;
    }

    void managedFirstCommit() {
        QFETCH(bool, historyFirst);
        QFETCH(int, fault);
        check([&] {
            Populated f;
            intake_origin(f, "main");
            const StorageLimits managed_limits{64, 256, 65536, 2097152, 1536, 4194304};
            f.db.reopen(managed_limits);
            for (int i = 1; i <= 6; ++i)
                (void)f.intake("unmanaged-" + std::to_string(i));
            auto live = managed("L", historyFirst ? 9000 : 8000, "60s/VL");
            auto history = managed("H", historyFirst ? 8000 : 9000, "120s/VH");
            catchup(f.db, 1, 1);
            anchor_page(f.db, page_admission(1, {normalized_item(18)}).binding);
            history.history = ManagedInput::PageSlot{"O", 1, 1};
            // Use distinct real captures for the two arrivals. No intake or
            // basis row is staged outside the managed receipt port.
            live.receipt.arrival.modifier_context_ref = "arrival-L";
            history.receipt.arrival.modifier_context_ref = "arrival-H";
            f.db.write([&](const auto &s) {
                for (const auto &input : {live, history})
                    require(SqliteRetainedContextStore(s).retain(
                                {{RetainedKind::arrival_context,
                                  input.receipt.arrival.modifier_context_ref},
                                 input.capture,
                                 ConfigurationReference{"cfg"}}) == RetainDisposition::inserted,
                            "Each selected arrival has its own shipped retained capture");
            });
            const auto &first = historyFirst ? history : live;
            const auto &peer = historyFirst ? live : history;
            const auto basis = [&] {
                return f.db.rows("SELECT intake_ref,receipt_id,attempt_ordinal,capture "
                                 "FROM main.subathon_intake_basis");
            };
            const auto initial = protected_data(f.db);
            if (fault == 1) {
                bool witnessed = false;
                const auto [stopped, work] = manage(f.db, peer, [&] {
                    witnessed = true;
                    return false;
                });
                require(
                    basis().isEmpty() && protected_data(f.db) == initial &&
                        f.db.rows("SELECT count(*) FROM main.subathon_intake_entries") ==
                            SqlRows{{0}},
                    "Neither the basis nor the first proposed receipt/attempt survives rollback");
                require(stopped.stop == Stop::known_rollback && witnessed &&
                            work.transaction == TransactionOutcome::rolled_back && !work.value,
                        "The rejected candidate reached the actual adopted pre-COMMIT callback");
            }
            const auto stage = [&](const ManagedInput &input, std::uint64_t id_value) {
                const auto before = protected_data(f.db);
                const auto prior = basis();
                const auto [selected, work] = manage(f.db, input);
                if (selected.stop == Stop::not_implemented)
                    require(protected_data(f.db) == before && basis() == prior,
                            "Typed missing behavior leaves independently inspected durable media "
                            "unchanged");
                require(selected.stop == Stop::committed &&
                            selected.attempt == AttemptLocator{core::ReceiptId{id_value}, 1},
                        "The managed receipt owner commits the exact new receipt and attempt");
                finance::normal(work);
            };
            const auto arm_lost_ack = [&] {
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Fault is armed on the idle writer for the exact next owning commit");
            };
            const auto stopped_completion = [&](const SqliteWorkResult &work, bool lost_ack) {
                require(!work.value &&
                            work.transaction == (lost_ack ? TransactionOutcome::outcome_unknown
                                                          : TransactionOutcome::committed) &&
                            work.error == (lost_ack ? WorkError::uncertain_outcome
                                                    : WorkError::work_exception),
                        "Interrupted owner reports the real transaction category with no reply");
                if (lost_ack)
                    require(f.db.writer().enqueue_work([](auto) { return QVariant{}; },
                                                       [](auto) {}) == StorageError::not_open,
                            "Unknown commit stops the suffix on the same writer");
            };
            const bool receipt_fault = fault == 2 || fault == 3;
            if (receipt_fault) {
                if (fault == 2)
                    arm_lost_ack();
                bool entered = false;
                bool missing = false;
                const auto work = qt_test_support::run_work(
                    f.db.writer(),
                    [&](auto ctx) {
                        const auto session = ctx.begin();
                        entered = true;
                        const auto selected = stage_managed(session, first);
                        if (selected.stop == Stop::not_implemented) {
                            missing = true;
                            session.rollback();
                            return 1;
                        }
                        require(selected.stop == Stop::committed &&
                                    selected.attempt == AttemptLocator{core::ReceiptId{7}, 1},
                                "Acknowledged adopted receipt has the exact selected attempt");
                        throw std::runtime_error(
                            "Abnormal completion after adopted receipt commit");
                    },
                    10000);
                require(entered, "The fault fixture enters the managed receipt seam");
                if (missing)
                    require(
                        work.transaction == TransactionOutcome::rolled_back && !work.value &&
                            basis().isEmpty() && protected_data(f.db) == initial,
                        "Typed missing behavior performs known cleanup before the fault boundary");
                require(!missing, "Managed receipt must reach its real commit fault boundary");
                stopped_completion(work, fault == 2);
            } else {
                stage(first, 7);
            }
            f.db.reopen(managed_limits);
            const auto bound = SqlRows{{first.intake_ref, blob8(7), 1, first.capture}};
            require(basis() == bound,
                    "First completed receipt COMMIT selects the immutable capture");
            if (receipt_fault) {
                const auto before = protected_data(f.db);
                const auto settled = read_missing(
                    f.db, [&](const auto &s) { return read_attempt(s, first, {{7}, 1}); });
                require(settled.stop == Stop::exact_replay &&
                            settled.attempt == AttemptLocator{core::ReceiptId{7}, 1} &&
                            settled.selected ==
                                core::ReceiptAttempt{first.receipt.arrival, first.receipt.facts,
                                                     core::ReceiptDisposition::created} &&
                            protected_data(f.db) == before,
                        "Reopened receipt closes only the original exact attempt before peer "
                        "admission");
            }
            stage(peer, 8);
            require(f.db.rows("SELECT intake_ref,operation_ref,page_ordinal,item_ordinal "
                              "FROM main.subathon_intake_entries ORDER BY intake_ref") ==
                        SqlRows{{QByteArray("H"), QByteArray("O"), 1, 1},
                                {QByteArray("L"), QVariant{}, QVariant{}, QVariant{}}},
                    "History owns its exact bound slot; live input uses the same activity key "
                    "without a page");
            require(
                basis() == bound &&
                    f.db.rows(
                        "SELECT receipt_id,ordinal,server_timestamp_utc_ms,modifier_context_ref "
                        "FROM main.subathon_receipt_attempts WHERE receipt_id>=? ORDER BY "
                        "receipt_id",
                        {blob8(7)}) == SqlRows{{blob8(7), 1, 8000,
                                                QByteArray::fromStdString(
                                                    first.receipt.arrival.modifier_context_ref)},
                                               {blob8(8), 1, 9000,
                                                QByteArray::fromStdString(
                                                    peer.receipt.arrival.modifier_context_ref)}} &&
                    f.db.rows("SELECT count(*) FROM main.subathon_contributions") == SqlRows{{0}},
                "Both receipts are auditable while evidence and awards remain absent");
            const auto gate = [&](const ManagedInput &input) {
                return read_missing(f.db, [&](const auto &s) { return select_evidence(s, input); });
            };
            const auto wait = gate(peer);
            const auto create = gate(first);
            require(wait.stop == Stop::pending_original && !wait.original &&
                        create.stop == Stop::may_create &&
                        create.selected_basis == AttemptLocator{core::ReceiptId{7}, 1} &&
                        create.capture == first.capture,
                    "A peer requesting evidence first cannot create or replace the first basis");
            constexpr core::ContributionId cid{0};
            if (fault == 4 || fault == 5) {
                if (fault == 4)
                    arm_lost_ack();
                const auto [staged, work] = evidence_work(f.db, first, {7}, fault == 5);
                stopped_completion(work, fault == 4);
                require(fault == 4 ? !staged : staged && staged->contribution_id == cid,
                        "No acknowledged evidence reply is published across the fault");
                f.db.reopen(managed_limits);
                require(
                    f.db.rows("SELECT contribution_id,original_receipt_id "
                              "FROM main.subathon_contributions") ==
                            SqlRows{{blob8(0), blob8(7)}} &&
                        basis() == bound,
                    "Recovered evidence media contains the same sole original and immutable basis");
                const auto replay = evidence(f.db, first, {7});
                require(replay.disposition == ContributionLinkDisposition::duplicate &&
                            replay.contribution_id == cid,
                        "Owning evidence replay selects the same durable original");
            } else {
                const auto original = evidence(f.db, first, {7});
                require(original.disposition == ContributionLinkDisposition::created &&
                            original.contribution_id == cid,
                        "Only the ACCT-002 owner produces the selected original");
            }
            const auto ready = gate(peer);
            require(ready.stop == Stop::may_link && ready.original == cid,
                    "The peer selects the sole durable original after its evidence COMMIT");
            const auto linked = evidence(f.db, peer, {8});
            require(linked.disposition == ContributionLinkDisposition::linked &&
                        linked.contribution_id == cid && basis() == bound &&
                        f.db.rows("SELECT original_receipt_id FROM main.subathon_contributions") ==
                            SqlRows{{blob8(7)}} &&
                        f.db.rows("SELECT count(*) FROM main.subathon_app_effects") == SqlRows{{0}},
                    "One original remains under Review with zero awards");
        });
    }

    void managedNoncreatingBasis_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("audit-only") << 0;
        QTest::newRow("confirmation-only") << 1;
        QTest::newRow("legacy-retry") << 2;
        QTest::newRow("legacy-conflict") << 3;
    }

    void managedNoncreatingBasis() {
        QFETCH(int, kind);
        check([&] {
            Populated f;
            f.install();
            auto input = managed("noncreating", 8000, "60s/VL");
            if (kind >= 2) {
                require(legacy_receipt(f.db, input) == core::ReceiptId{1},
                        "Shipped legacy receipt creates a real original with no managed basis");
                input.intake_ref = "new-observation";
                input.receipt.arrival.server_timestamp_utc_ms = 9000;
                if (kind == 3)
                    input.receipt.facts.money = core::ReceiptMoney{"21.00", "EUR"};
            } else {
                input.admission = kind == 0 ? ManagedInput::Admission::audit_only
                                            : ManagedInput::Admission::confirmation_only;
                if (kind == 1)
                    input.receipt.facts.event_type = "confirmation";
            }
            advance(f.db);
            stage_scope(f.db);
            const auto before = protected_data(f.db);
            const auto [selected, work] = manage(f.db, input);
            require(f.db.rows("SELECT count(*) FROM main.subathon_intake_basis") == SqlRows{{0}} &&
                        f.db.rows("SELECT count(*) FROM main.subathon_contributions") ==
                            SqlRows{{0}},
                    "A noncreating observation cannot establish even an otherwise eligible K");
            if (selected.stop == Stop::not_implemented)
                require(protected_data(f.db) == before,
                        "Typed missing behavior leaves existing receipt and evidence untouched");
            require(selected.stop == Stop::committed &&
                        selected.attempt == AttemptLocator{core::ReceiptId{1}, kind >= 2 ? 2 : 1},
                    "Real managed observation commits its own exact attempt for audit");
            finance::normal(work);
            require(f.db.rows("SELECT disposition FROM main.subathon_receipt_attempts "
                              "ORDER BY ordinal") ==
                        (kind >= 2 ? SqlRows{{0}, {kind - 1}} : SqlRows{{0}}),
                    "Receipt owner preserves exact independent retry and conflict dispositions");
        });
    }

    void initialCaptureAfterUpgrade() {
        check([] {
            Populated f;
            const core::ReceiptInput input{
                {{"synthetic.reference.v1", "A"}, "N", "E"},
                {"synthetic.reference.support.v1", 1, {}, {}, {}, {}, {}, {}},
                {0, "Q", "C", "X"}};
            const auto complete = ReferenceFactsState::complete;
            const ReferenceConfiguration config{
                "C",
                "W",
                input.identity.origin,
                "N",
                capture_rule(),
                {},
                {core::TimerEndMode::Grace, 120000, core::GraceReuse::Fresh, {}},
                complete,
                complete,
                complete,
                complete,
                complete};
            f.install("I");
            const ReferenceInitialCapture expected{
                {"I", input.identity}, {1}, {0}, "C", "X", "V", {"P", 1}, "Y"};
            f.db.write([&](const auto &s) {
                const SqliteRetainedContextStore store(s);
                require(
                    store.retain({{RetainedKind::configuration, "C"},
                                  encode_reference_configuration(config).document,
                                  {}}) == RetainDisposition::inserted &&
                        store.retain({{RetainedKind::arrival_context, "X"},
                                      encode_reference_arrival(
                                          {"X", "V", 0, config, {"Q", "E", {}, QByteArray("", 0)}})
                                          .document,
                                      ConfigurationReference{"C"}}) == RetainDisposition::inserted,
                    "Shipped retained owner stores typed configuration and arrival");
            });
            const auto receipt = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    SqliteReceiptRepository repo(ctx);
                    const auto result = ReceiptIntake(repo).ingest(input);
                    require(result.receipt_id == expected.receipt_id &&
                                result.disposition == core::ReceiptDisposition::created,
                            "Shipped receipt owner commits the first exact synthetic receipt");
                    return 1;
                },
                10000);
            finance::normal(receipt);
            core::ContributionState support;
            support.payment_evidence = core::PaymentEvidence::not_applicable;
            support.conditions.valuation = {false, {}, {}, false};
            support.conditions.review = core::ContributionReview::not_required;
            support.conditions.evidence_admission = {
                core::EvidencePolicy::require_evidence_review, "C", {}};
            support.conditions.settlement = core::SettlementGate::clear;
            const auto evidence = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    SqliteContributionRepository repo(ctx);
                    const auto linked = ContributionEvidenceService(repo).observe(
                        {{expected.receipt_id, input.identity, input.facts, input.arrival},
                         ContributionObservationKind::support,
                         support.payment_evidence,
                         {},
                         support.conditions});
                    require(linked.disposition == ContributionLinkDisposition::created &&
                                linked.contribution_id == expected.contribution_id,
                            "Shipped evidence owner commits present CID zero");
                    return 1;
                },
                10000);
            finance::normal(evidence);
            const core::CapturedPlanRevision plan{
                1,
                {},
                {{"V",
                  {},
                  "V",
                  expected.receipt_id,
                  input.identity,
                  input.facts,
                  input.arrival,
                  expected.contribution_id},
                 support,
                 {core::PlanValuationState::not_applicable, {}},
                 {{"a", "b", "R", "T", finance::Unit::milliseconds, false,
                   finance::fraction(600000), 600000, "reference.integer-ms.v1",
                   "reference.original-time.v1"}}},
                {{"R", "r", true, "reference.single-rule.match.v1"}},
                {},
                {}};
            f.db.write([&](const auto &s) {
                require(SqliteApplicationPlanStore(s).retain_initial("P", plan) ==
                                SqlitePlanInitialDisposition::inserted &&
                            SqliteRetainedContextStore(s).retain(
                                {{RetainedKind::arrival_context, "Y"},
                                 encode_reference_execution(
                                     {"Y", "X", "I", input.identity, expected.receipt_id,
                                      expected.contribution_id, expected.plan, capture_rule()})
                                     .document,
                                 ConfigurationReference{"C"}}) == RetainDisposition::inserted &&
                            SqliteReferenceCaptureStore(s).retain_initial(expected) ==
                                ReferenceCaptureRetainDisposition::inserted,
                        "Normal plan, retained context and initial capture owners commit complete "
                        "closure");
            });
            const auto read = [&](const auto &s) {
                const auto found = SqliteReferenceCaptureStore(s).find(expected.key);
                require(found && found->key.installation_ref == "I" &&
                            found->key.source == input.identity &&
                            found->receipt_id == core::ReceiptId{1} &&
                            found->contribution_id == core::ContributionId{0} &&
                            found->configuration_ref == "C" && found->arrival_context_ref == "X" &&
                            found->event_ref == "V" && found->plan.revision == 1 &&
                            found->plan.plan_ref == "P" && found->execution_context_ref == "Y",
                        "Normal reader preserves complete selected initial capture");
            };
            f.db.write(read);
            const auto media = f.db.rows("SELECT * FROM main.subathon_reference_initial_captures");
            advance(f.db);
            f.db.write(read);
            require(media.size() == 1 &&
                        f.db.rows("SELECT * FROM main.subathon_reference_initial_captures") ==
                            media,
                    "Initial capture index survives the explicit upgrade and reopen byte for byte");
        });
    }

    void selectedSar2_data() {
        QTest::addColumn<bool>("corrupt");
        QTest::newRow("exact") << false;
        QTest::newRow("missing-terminal-masked") << true;
    }

    void selectedSar2() {
        QFETCH(bool, corrupt);
        check([&] {
            Finished f;
            const auto read = [&](const auto &s) {
                const auto selected = selected_record_read::read_application(s, f.key, {});
                if (corrupt) {
                    const auto *error = std::get_if<selected_record_read::Failure>(&selected);
                    require(error && *error == selected_record_read::Failure::invalid_data,
                            "Selected metadata refuses corrupted terminal closure");
                } else {
                    const auto *bundle = std::get_if<RecordedApplicationBundle>(&selected);
                    require(bundle && bundle->effects.size() == 1 &&
                                bundle->effects[0].request_ordinal == 2 &&
                                bundle->effects[0].components.size() == 1 &&
                                bundle->effects[0].components[0].units == 15 &&
                                bundle->result.document.endsWith(
                                    QByteArray::fromHex("0000000300000002010002")),
                            "SelectedMetadata returns the real SAR2 active effect and exact mask");
                }
            };
            if (corrupt)
                f.stored();
            else
                f.db.write(read);
            advance(f.db);
            if (corrupt)
                mutate(
                    f.db,
                    "DELETE FROM main.subathon_review_terminal_requests WHERE request_ordinal=1");
            f.db.write(read);
        });
    }

    void sar2LinkedHold_data() {
        QTest::addColumn<int>("entry");
        QTest::newRow("linked-hold-find") << 0;
        QTest::newRow("selected-metadata-linked-hold") << 1;
        QTest::newRow("stage-linked-resolution") << 2;
    }

    void sar2LinkedHold() {
        QFETCH(int, entry);
        const bool mutateAfterUpgrade = entry == 2;
        check([&] {
            Finished f;
            const ApplicationHoldSubject subject{f.original.selector.cid};
            ApplicationHoldCause cause;
            cause.hold_ref = "linked-sar2";
            cause.source = {subject, f.original.selector.receipt, {}, {}, {}, "cfg", "ctx"};
            cause.command = {{"test", "hold-sar2"}, finance::document()};
            cause.first_observation = {
                "review-initial", finance::document(),  "policy", "permissions",
                "tester",         "sar2-hold-decision", 50,       0};
            cause.kind = ApplicationHoldCauseKind::unresolved_policy;
            cause.missing_policy_reason = "independent unresolved policy";
            cause.plan_evidence = {ApplicationHoldPlanKind::failed_capture,
                                   {},
                                   ApplicationHoldExtent::known_prefix,
                                   ApplicationHoldStoppedFor::unresolved_capture_policy,
                                   finance::document(),
                                   {}};
            const ApplicationHoldDetection detection{
                cause, {"detect-sar2", {}, "v1", cause.first_observation, finance::document()}};
            const ApplicationHoldResolutionBatch resolution{
                "resolve-sar2",
                subject,
                {{"linked-sar2", "v1"}},
                {{"linked-sar2", "v1", "v2", finance::document(), {}}},
                "tester",
                "sar2-resolution",
                "clear selected SAR2 link",
                finance::document(),
                ApplicationHoldApplicationLink{ApplicationHoldApplicationLinkKind::whole_input,
                                               f.key,
                                               f.original.selector.receipt,
                                               {"terminal-plan", 1},
                                               {}}};
            f.db.write([&](const auto &s) {
                require(SqliteApplicationHoldStore(s).record_detection(detection).disposition ==
                            SqliteHoldMutationDisposition::recorded,
                        "Real hold owner stages the original hold");
            });
            const auto close = [&](const auto &s) {
                require(
                    SqliteApplicationHoldStore(s).stage_resolution(resolution).disposition ==
                        SqliteHoldMutationDisposition::recorded,
                    "Ordinary linked resolution traverses and retains the selected SAR2 bundle");
            };
            const auto read = [&](const auto &s) {
                std::optional<ApplicationHoldView> found;
                if (entry == 1) {
                    auto selected = selected_record_read::read_hold(s, "linked-sar2", {});
                    if (auto *view = std::get_if<ApplicationHoldView>(&selected))
                        found = std::move(*view);
                } else {
                    found = SqliteApplicationHoldStore(s).find("linked-sar2");
                }
                require(found && found->current_version.state == ApplicationHoldState::resolved &&
                            found->current_version.locator.version_ref == "v2" &&
                            found->current_version.parent_version_ref == "v1" &&
                            found->current_event,
                        "Hold read preserves exact version and its producing resolution");
                const auto *saved =
                    std::get_if<ApplicationHoldRecordedResolution>(&*found->current_event);
                require(saved && saved->request.application &&
                            finance::same(saved->request.application->operation, f.key),
                        "Selected hold closure preserves the actual SAR2 operation key");
            };
            f.stored();
            if (!mutateAfterUpgrade) {
                f.db.write(close);
                f.db.write(read);
            }
            advance(f.db);
            if (mutateAfterUpgrade)
                f.db.write(close);
            f.db.write(read);
        });
    }

    void financialSelectedResolution_data() {
        QTest::addColumn<QString>("entry");
        for (const auto *name :
             {"resolution", "grouped-bundle", "reserved-stage-key", "stale-mixed-hold-set",
              "exact-mixed-hold-set", "exact-mixed-hold-set-v19"})
            QTest::newRow(name) << QString(name);
    }

    void financialSelectedResolution() {
        QFETCH(QString, entry);
        check([&] {
            finance::File f;
            const auto holds = seed_mixed_refund(
                f, "grouped", entry != "grouped-bundle" && entry != "reserved-stage-key");
            report(migrate(
                       *f.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
                   CatalogState::current, 19, 19, 4);
            if (entry.startsWith("stale") || entry.startsWith("exact")) {
                require(holds.size() == 1,
                        "Real grouped refund has its own held timer continuation");
                const auto financial = holds.front().locator;
                const auto ordinary = f.detect(f.ordinary_detection("ordinary-mixed", false));
                auto active = f.continuation("selection", holds.front(), {ordinary, financial})
                                  .expected_active;
                if (entry.startsWith("stale"))
                    active = {ordinary};
                const ApplicationHoldResolutionBatch batch{
                    "close-mixed",
                    {f.cid},
                    active,
                    {{ordinary.hold_ref, ordinary.version_ref, "v2", finance::document(), {}}},
                    "tester",
                    "close-mixed",
                    "clear only the selected ordinary hold",
                    finance::document(),
                    f.ordinary_link(false, ordinary)};
                const auto apply = [&] {
                    return f.run_reply([&](const auto &s) {
                        return stage_financial_aware_linked_resolution(s, batch);
                    });
                };
                const bool stale = entry.startsWith("stale");
                if (!stale) {
                    // Read the complete current set under v19 without consuming the closure.
                    const auto actual = f.financial_active();
                    require(actual.size() == 1 && finance::same(actual.front().locator, financial),
                            "Owning v19 selector sees the actual held version before upgrade");
                } else {
                    require(apply().disposition == FinancialDisposition::stale_hold,
                            "Shipped owner refuses a missing financial version");
                }
                if (entry != "exact-mixed-hold-set-v19")
                    upgrade_financial(f);
                require(apply().disposition == (stale ? FinancialDisposition::stale_hold
                                                      : FinancialDisposition::ordinary_resolved),
                        "Mixed stale and complete sets remain distinct at the owning entry");
                const auto remaining = f.financial_active();
                require(remaining.size() == 1 &&
                            finance::same(remaining.front().locator, financial),
                        "Financial hold and exact version remain untouched");
                f.write([&](const auto &s) {
                    const auto view = SqliteApplicationHoldStore(s).find(ordinary.hold_ref);
                    require(
                        view &&
                            view->current_version.locator.version_ref == (stale ? "v1" : "v2") &&
                            view->current_version.state == (stale ? ApplicationHoldState::active
                                                                  : ApplicationHoldState::resolved),
                        "Only the exact complete set can close the ordinary version");
                });
                return;
            }
            if (entry == "resolution") {
                require(holds.size() == 1, "Actual refundable timer continuation is held");
                f.move_main("fund-grouped", 100000);
                const auto cmd =
                    f.continuation("resolve-grouped", holds.front(), {holds.front().locator});
                finance::Effects effects;
                effects.apply(-75000, 0, {75000});
                require(f.continue_reply(cmd, effects).disposition ==
                            FinancialDisposition::resolved,
                        "Owning timer continuation commits 75000 ms of reversal");
                const auto read = [&](const auto &s) {
                    const auto result =
                        SqliteFinancialRefundStore(s).lookup_resolution(cmd.operation);
                    require(
                        result && result->after.timer.main_remaining_ms == 25000 &&
                            result->closed.state == ApplicationHoldState::resolved &&
                            finance::same(result->held.locator, holds.front().locator),
                        "Selected resolution reconstructs its exact held origin and funded result");
                };
                f.write(read);
                upgrade_financial(f);
                f.write(read);
                f.check_head(25000, 0, 30000000, 7);
                return;
            }
            const ApplicationOperationKey op{"test", "grouped"};
            std::optional<PreparedApplicationBundle> ordinary;
            if (entry == "reserved-stage-key") {
                f.write([&](const auto &s) {
                    const auto baseline = SqliteApplicationBundleStore(s).lookup(finance::original);
                    require(
                        baseline && baseline->reversals.empty(),
                        "Shipped dense SAR1 original supplies structurally valid ordinary input");
                    ordinary.emplace(PreparedApplicationBundle{baseline->intent,
                                                               baseline->advanced_before,
                                                               baseline->after,
                                                               baseline->advance_explanation,
                                                               baseline->effects,
                                                               baseline->jobs,
                                                               baseline->transitions,
                                                               {},
                                                               baseline->result,
                                                               baseline->audit});
                    ordinary->intent.operation = op;
                });
            }
            const auto read = [&](const auto &s) {
                const SqliteApplicationBundleStore store(s);
                if (ordinary) {
                    require(store.stage(*ordinary).disposition ==
                                SqliteBundleStageDisposition::operation_conflict,
                            "Ordinary stage cannot take a reserved financial operation key");
                    return;
                }
                const auto result = store.lookup(op);
                require(result && result->effects.size() == 3 && result->reversals.size() == 2 &&
                            result->after.timer.main_remaining_ms == 50000 &&
                            result->result.document == "accepted",
                        "Grouped reversal returns time, score and count with two non-time links");
                const auto &time = result->effects.front();
                require(
                    time.request_ordinal == 1 && time.requested_units == -75000 &&
                        time.components.size() == 3 &&
                        time.components[0].kind == finance::CK::main_delta &&
                        time.components[0].units == -75000 &&
                        time.components[1].kind == finance::CK::debt_delta &&
                        time.components[1].units == 0 &&
                        time.components[2].kind == finance::CK::unapplied_time &&
                        time.components[2].units == 0,
                    "One-quarter group removes 75000 ms and adds no restoration, debt or residual");
            };
            f.write(read);
            upgrade_financial(f);
            f.write(read);
        });
    }

    void freshSar2BundleStage_data() {
        QTest::addColumn<bool>("upgrade");
        QTest::newRow("v19-positive") << false;
        QTest::newRow("successor-financial-gate-masks-stage") << true;
    }

    void freshSar2BundleStage() {
        QFETCH(bool, upgrade);
        check([&] {
            Finished origin;
            std::optional<RecordedApplicationBundle> stored;
            origin.db.write([&](const auto &s) {
                stored = SqliteApplicationBundleStore(s).lookup(origin.key);
                require(stored && stored->contribution && stored->reversals.empty(),
                        "Shipped Review commits the real Finished request partition");
            });
            Populated fresh(99, 7);
            const auto selected =
                fresh.seed("terminal",
                           {finance::request(finance::Unit::milliseconds, 20, "main"),
                            finance::request(finance::Unit::micro_points, 15, "score"),
                            finance::request(finance::Unit::whole_count, 1, "count")},
                           false, true);
            require(selected.selector == origin.original.selector,
                    "Fresh fixture independently allocates the same receipt and contribution keys");
            const auto decision = qt_test_support::run_work(
                fresh.db.writer(),
                [&](auto ctx) {
                    SqliteContributionRepository repo(ctx);
                    auto unit = repo.begin();
                    const auto current = unit->find_activity(
                        {{"review-fixture", "account"}, "fixture-support", "terminal"});
                    require(current && current->state.conditions.review ==
                                           core::ContributionReview::pending,
                            "Fresh original awaits its accepted review state");
                    auto accepted = current->state;
                    accepted.conditions.review = core::ContributionReview::accepted;
                    unit->update_state(selected.selector.cid, accepted);
                    unit->commit();
                    return 1;
                },
                10000);
            finance::normal(decision);
            require(fresh.db.rows("SELECT count(*) FROM main.subathon_app_claims") == SqlRows{{0}},
                    "Prepared source has no previous claim or result");
            if (upgrade)
                advance(fresh.db);
            const PreparedApplicationBundle prepared{
                stored->intent,      stored->advanced_before,
                stored->after,       stored->advance_explanation,
                stored->effects,     stored->jobs,
                stored->transitions, {},
                stored->result,      stored->audit};
            fresh.db.write([&](const auto &s) {
                require(
                    SqliteApplicationBundleStore(s).stage(prepared).disposition ==
                        SqliteBundleStageDisposition::staged,
                    "Normal bundle stage accepts a fresh SAR2 on a qualified Finished original");
            });
            const auto original = fresh.db.rows("SELECT document FROM main.subathon_app_results");
            fresh.db.write([&](const auto &s) {
                const auto replay = SqliteApplicationBundleStore(s).stage(prepared);
                require(replay.disposition == SqliteBundleStageDisposition::already_present &&
                            replay.existing &&
                            replay.existing->result.document == original.at(0).at(0).toByteArray(),
                        "Exact replay returns the same physical committed bundle");
            });
            require(
                original.size() == 1 &&
                    fresh.db.rows("SELECT document FROM main.subathon_app_results") == original &&
                    fresh.db.rows("SELECT count(*) FROM main.subathon_app_claims") ==
                        SqlRows{{1}} &&
                    fresh.db.rows(
                        "SELECT request_ordinal,disposition "
                        "FROM main.subathon_review_terminal_requests ORDER BY request_ordinal") ==
                        SqlRows{{1, 1}, {3, 2}},
                "One claim and immutable terminal dispositions survive exact replay");
            fresh.head(99, 7, 105);
        });
    }

    void milestoneApplicationDelivery_data() {
        QTest::addColumn<QString>("entry");
        for (const auto *name : {"lookup", "unreleased-dispatch", "released-dispatch", "stranded"})
            QTest::newRow(name) << QString(name);
    }

    void milestoneApplicationDelivery() {
        QFETCH(QString, entry);
        check([&] {
            finance::File f(finance::OriginalCredit{.milestone = true});
            report(migrate(
                       *f.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
                   CatalogState::current, 19, 19, 4);
            struct Ports final : DeliveryDispatchAuthority, DeliveryAdapter {
                unsigned decisions{};
                std::vector<DeliveryJob> started;
                bool allows(const DeliveryJob &job) override {
                    ++decisions;
                    require(job.key.job_ref == "original-job" &&
                                job.capture.route_ref == "local-fixture" &&
                                job.action_document == "synthetic-action",
                            "Exact captured job enters local authority");
                    return true;
                }
                void start(DeliveryJob job) override { started.push_back(std::move(job)); }
            };
            const DeliveryJobKey key{finance::original, "original-job"};
            auto ports = std::make_shared<Ports>();
            const auto dispatch = [&](DeliveryDisposition expected) {
                const auto result = finance::completed<DeliveryDispatchCompletion>([&](auto done) {
                    return dispatch_delivery_job(*f.writer, key, 1, ports, ports, std::move(done));
                });
                finance::normal(result.work);
                require(
                    result.result && result.result->disposition == expected &&
                        !result.adapter_threw,
                    "Dispatch obeys exact occurrence release and returns an acknowledged outcome");
                const bool started = expected == DeliveryDisposition::changed;
                require(ports->started.size() == std::size_t(started),
                        "Only a committed sending attempt reaches the bounded fake adapter");
                if (started)
                    require(ports->started[0].state == DeliveryJobState::sending &&
                                ports->started[0].revision == 2 && ports->started[0].attempt == 2,
                            "Adapter sees the committed immutable job at sending attempt 2");
            };
            const auto lookup = [&](DeliveryJobState expected, int revision) {
                f.write([&](const auto &s) {
                    const auto saved = SqliteDeliveryJobStore(s).lookup(key);
                    require(saved && saved->state == expected && saved->revision == revision &&
                                saved->action_document == "synthetic-action" &&
                                saved->capture.sender_ref == "fixture-sender",
                            "Exact enrolled job lookup validates its real occurrence membership");
                });
            };
            const auto release = [&] {
                const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    return release_milestone_head(*f.writer, finance::achievement, std::move(done));
                });
                finance::normal(result.work);
                require(result.disposition == MilestoneReleaseDisposition::released,
                        "Real application head is released through its owning writer");
            };
            const auto scan = [&] {
                f.write([&](const auto &s) {
                    const auto page = SqliteDeliveryJobStore(s).stranded({}, 1);
                    require(page.attempts.size() == 1 &&
                                finance::same(page.attempts[0].key.operation, key.operation) &&
                                page.attempts[0].key.job_ref == key.job_ref &&
                                page.attempts[0].sending_revision == 2,
                            "Bounded stranded scan returns exactly the committed sending attempt");
                });
            };
            lookup(DeliveryJobState::queued, 1);
            if (entry == "unreleased-dispatch" || entry == "released-dispatch")
                dispatch(DeliveryDisposition::refused);
            if (entry == "stranded") {
                release();
                dispatch(DeliveryDisposition::changed);
                scan();
            }
            upgrade_financial(f);
            if (entry == "lookup")
                lookup(DeliveryJobState::queued, 1);
            if (entry == "unreleased-dispatch")
                dispatch(DeliveryDisposition::refused);
            if (entry == "released-dispatch") {
                release();
                dispatch(DeliveryDisposition::changed);
                lookup(DeliveryJobState::sending, 2);
            }
            if (entry == "stranded")
                scan();
        });
    }

    void milestoneEditFollowThrough_data() {
        QTest::addColumn<QString>("entry");
        for (const auto *name :
             {"edit-stranded", "edit-bonus-v19", "edit-bonus-successor", "refund-v19",
              "refund-successor", "refund-financial-v19", "refund-financial-successor"})
            QTest::newRow(name) << QString(name);
    }

    void milestoneEditFollowThrough() {
        QFETCH(QString, entry);
        check([&] {
            namespace edit = milestone_target_edit_fixture;
            const bool bonus = entry.startsWith("edit-bonus");
            const bool refund = entry.startsWith("refund");
            MilestoneOrigin f;
            f.enroll();
            f.record();
            edit::Authority authority;
            authority.facts.lifecycle.timer = finance::timer(core::TimerLifecycle::Finished, 99);
            authority.facts.captures = {edit::capture_b(!bonus)};
            const auto intent =
                edit::single("earn-B", {"B", 104}, {{edit::b, MilestoneActionChoice::run}});
            f.db.write([&](const auto &s) {
                require(initialize_milestone_track(s, f.track()),
                        "Real SAR2 member seeds the edit track");
                const auto preview =
                    preview_milestone_target_edit(s, authority, edit::actor, intent);
                edit::File::confirmable(preview);
                const auto saved = confirm_milestone_target_edit(s, authority, edit::actor, intent,
                                                                 *preview.confirmation);
                require(
                    saved.result && saved.result->achievements.size() == 1 &&
                        saved.result->achievements[0].order == 2,
                    "Exact edit-owned occurrence follows the released-by-owner application lane");
            });
            const auto release = [&](const MilestoneOccurrenceKey &key, bool is_edit) {
                const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    if (is_edit)
                        return release_milestone_head(
                            f.db.writer(), key, std::make_shared<EditRelease>(), std::move(done));
                    return release_milestone_head(f.db.writer(), key, std::move(done));
                });
                finance::normal(result.work);
                require(result.disposition == MilestoneReleaseDisposition::released,
                        "Each owning release consumes exactly its pending head");
            };
            if (refund) {
                const auto verify_pending = [&](const auto &s) {
                    SqliteMilestoneGroupDecisions store(s);
                    const auto a = store.acceptance_audit(f.application);
                    require(a && a->choice == MilestoneActionChoice::run &&
                                store.pending_disposition(edit::b) ==
                                    MilestonePendingDisposition::ready,
                            "Retained actual acceptance and edit pending state are readable");
                };
                f.db.write(verify_pending);
                if (entry.endsWith("successor"))
                    advance(f.db);
                const bool through_financial = entry.startsWith("refund-financial");
                const std::string ref = through_financial ? "one-fifth" : "score-refund";
                if (through_financial) {
                    struct RefundEffect final : FinancialRefundEffectPort {
                        finance::Effects delegate;
                        FinancialEffectObservation
                        observe(const SqliteWorkSession &s,
                                const ApplicationStateSnapshot &state) override {
                            return delegate.observe(s, state);
                        }
                        RefundTimerActual
                        timer_effect(const SqliteWorkSession &s,
                                     const GroupedRefundEntitlement &entitlement,
                                     const FinancialEffectObservation &observation,
                                     core::RefundDeficitPolicy policy) override {
                            return delegate.timer_effect(s, entitlement, observation, policy);
                        }
                        std::vector<FinancialMilestoneTarget>
                        milestones(const SqliteWorkSession &) override {
                            // The edit owns the new target 104. The old 200 cannot show a crossing.
                            return {{edit::b, 200, MilestoneRefundPolicy::review}};
                        }
                    } effects;
                    (void)f.refund(&effects);
                }
                f.db.write([&](const auto &s) {
                    SqliteMilestoneGroupDecisions store(s);
                    if (!through_financial)
                        store.observe_refund(
                            {edit::b, ref, 105, 102, 104, MilestoneRefundPolicy::review});
                    const auto saved = store.refund_audit(edit::b, ref);
                    require(saved && saved->deficit_micro_points == 2 &&
                                saved->current_target_micro_points == 104 &&
                                store.pending_disposition(edit::b) ==
                                    MilestonePendingDisposition::held,
                            "Refund reaches the edit occurrence and records exactly 104 minus 102");
                });
                require(f.db.rows("SELECT occurrence_ref,refund_ref "
                                  "FROM main.subathon_milestone_edit_refund_decisions") ==
                                SqlRows{{QByteArray("b-edit"), QByteArray::fromStdString(ref)}} &&
                            f.db.rows(
                                "SELECT count(*) FROM main.subathon_milestone_refund_decisions") ==
                                SqlRows{{0}},
                        "Only edit-origin refund history is written");
                return;
            }
            if (bonus) {
                struct Bonus final : MilestoneBonusAuthority, MilestoneBonusEffect {
                    unsigned calls{};
                    bool allows(const MilestoneBonusKey &key,
                                const MilestoneAction &action) override {
                        return key.occurrence == edit::b && key.action_ordinal == 1 &&
                               action ==
                                   MilestoneAction{MilestoneActionKind::bonus, "award", "award-v1"};
                    }
                    std::string apply(const SqliteWorkSession &, const MilestoneBonusKey &,
                                      const MilestoneAction &) override {
                        ++calls;
                        return "bonus-effect-B";
                    }
                } ports;
                const auto apply = [&](MilestoneBonusDisposition expected) {
                    f.db.write([&](const auto &s) {
                        require(SqliteMilestoneGroupStore(s).apply_bonus({edit::b, 1}, ports,
                                                                         ports) == expected,
                                "Exact edit bonus observes the persisted release flag");
                    });
                };
                apply(MilestoneBonusDisposition::refused);
                if (entry == "edit-bonus-successor") {
                    advance(f.db);
                    apply(MilestoneBonusDisposition::refused);
                }
                require(ports.calls == 0, "Unreleased edit never reaches the effect port");
                release(f.application, false);
                release(edit::b, true);
                apply(MilestoneBonusDisposition::applied);
                apply(MilestoneBonusDisposition::already_applied);
                require(ports.calls == 1 &&
                            f.db.rows("SELECT occurrence_ref,action_ordinal,effect_ref "
                                      "FROM main.subathon_milestone_edit_bonus_receipts") ==
                                SqlRows{{QByteArray("b-edit"), 1, QByteArray("bonus-effect-B")}},
                        "Released bonus records the exact receipt and does not apply twice");
                return;
            }
            struct EditDispatch final : MilestoneEditDispatchAuthority {
                bool allows(const MilestoneEditJob &job) override {
                    return job.key.occurrence == edit::b &&
                           job.action_document == QByteArray::fromHex("0081");
                }
                std::optional<std::string> retry_policy(const MilestoneEditJob &) override {
                    return {};
                }
            } port;
            const MilestoneEditJobKey key{intent.key, edit::b, 1};
            f.db.write([&](const auto &s) {
                require(claim_milestone_edit_job(s, key, 1, port).disposition ==
                            DeliveryDisposition::refused,
                        "Unreleased edit cannot claim its captured job");
            });
            release(f.application, false);
            release(edit::b, true);
            f.db.write([&](const auto &s) {
                const auto claim = claim_milestone_edit_job(s, key, 1, port);
                require(claim.disposition == DeliveryDisposition::changed && claim.job &&
                            claim.job->state == DeliveryJobState::sending &&
                            claim.job->revision == 2,
                        "Real edit owner commits the sending state");
            });
            const auto scan = [&](const auto &s) {
                const auto page = stranded_milestone_delivery(s, {}, 1);
                require(page.attempts.size() == 1 && page.attempts[0].revision == 2 &&
                            std::get_if<MilestoneEditJobKey>(&page.attempts[0].key) &&
                            std::get<MilestoneEditJobKey>(page.attempts[0].key) == key &&
                            page.next_after &&
                            std::get<MilestoneEditJobKey>(*page.next_after) == key,
                        "Stranded selection preserves the edit-lane discriminator and last key");
            };
            f.db.write(scan);
            advance(f.db);
            f.db.write(scan);
        });
    }

    void evidenceOwnership_data() {
        QTest::addColumn<bool>("historyFirst");
        QTest::addColumn<int>("phase");
        QTest::newRow("live-bound") << false << 0;
        QTest::newRow("history-bound") << true << 0;
        QTest::newRow("live-first-peer-waits") << false << 1;
        QTest::newRow("history-first-peer-waits") << true << 1;
        QTest::newRow("live-first-peer-links") << false << 2;
        QTest::newRow("history-first-peer-links") << true << 2;
    }

    void evidenceOwnership() {
        QFETCH(bool, historyFirst);
        QFETCH(int, phase);
        check([&] {
            Populated f;
            f.install();
            for (int i = 1; i <= 6; ++i)
                (void)f.intake("unmanaged-" + std::to_string(i));
            const auto live = managed("L", historyFirst ? 9000 : 8000, "60s/VL");
            const auto history = managed("H", historyFirst ? 8000 : 9000, "120s/VH");
            const auto &first = historyFirst ? history : live;
            const auto &peer = historyFirst ? live : history;
            // These owning legacy receipts are prerequisites, not a proxy
            // managed race. The literal binding fixes either selection order.
            const auto live_id = legacy_receipt(f.db, live);
            const auto history_id = legacy_receipt(f.db, history);
            require(live_id.value == 7 && history_id.value == 8,
                    "Live and history retain independently fixed receipt identities");
            const auto first_id = historyFirst ? history_id : live_id;
            const auto peer_id = historyFirst ? live_id : history_id;
            advance(f.db);
            stage_scope(f.db, "main");
            peer_anchors(f.db, first, first_id, peer, peer_id);
            f.db.reopen();
            std::optional<core::ContributionId> cid;
            if (phase == 2) {
                const auto result = evidence(f.db, first, first_id);
                require(
                    result.disposition == ContributionLinkDisposition::created &&
                        result.contribution_id == core::ContributionId{0} &&
                        f.db.rows("SELECT original_receipt_id FROM main.subathon_contributions") ==
                            SqlRows{{blob8(first_id.value)}},
                    "Real ACCT-002 original anchors on the bound receipt; CID zero is present");
                cid = result.contribution_id;
            }
            require(f.db.rows("SELECT count(*) FROM main.subathon_app_effects") == SqlRows{{0}},
                    "Receipt and evidence selection create no awards");
            const auto before = protected_data(f.db);
            const auto result = read_missing(
                f.db, [&](const auto &s) { return select_evidence(s, phase == 0 ? first : peer); });
            require(protected_data(f.db) == before, "Gate does not own the evidence writes");
            const auto expected = phase == 0   ? Stop::may_create
                                  : phase == 1 ? Stop::pending_original
                                               : Stop::may_link;
            require(result.stop == expected &&
                        result.selected_basis == AttemptLocator{first_id, 1} &&
                        result.capture == first.capture && result.original == cid,
                    "Only the bound intake may create; peer waits then links to the same original");
            if (phase == 2) {
                const auto linked = evidence(f.db, peer, peer_id);
                require(
                    linked.disposition == ContributionLinkDisposition::linked &&
                        linked.contribution_id == cid &&
                        f.db.rows("SELECT count(*) FROM main.subathon_contributions") ==
                            SqlRows{{1}},
                    "Owning producer links the peer's own observation without a second original");
            }
        });
    }
};

QTEST_GUILESS_MAIN(Storage026Tests)
#include "storage_026_tests.moc"
