// SPDX-License-Identifier: GPL-3.0-or-later
#include "missing_port.hpp"
#include "populated_fixture.hpp"

#include "../support/milestone_target_edit_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "../support/sqlite_rate_override_writer_fixture.hpp"

#include <array>
#include <subathon/server/sqlite_reference_authority.hpp>

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
    constexpr qsizetype new_objects = 252;
    constexpr qsizetype old_projection = 127248;
    constexpr qsizetype new_projection = old_projection + 12939;
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

void stage_scope(CatalogFile &file) {
    file.write([](const auto &s) {
        (void)s.execute({"INSERT INTO main.subathon_intake_policies VALUES(?,1,?)",
                         {QByteArray("install"), QByteArray("review-policy")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_members VALUES(?,1,?,?,?,?,?, ?,1,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("support"), QByteArray("score"), QByteArray("overlap"),
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

    ApplicationOperationKey refund() {
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
            require(stage_financial_refund(s, request, candidate, authority, effects).disposition ==
                        FinancialDisposition::applied,
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
                manifest.steps.back().expected_schema.size() == 252 && delta_objects.size() == 27 &&
                    delta_sql.size() == 14,
                "225 protected objects plus 27 literal delta objects, including 13 autoindexes");
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
            constexpr qsizetype bytes = 1593615;
            constexpr qsizetype longest_id = 29; // reference-initial-captures-v1
            require(tally(manifest) == bytes && manifest.steps.back().id.size() == 27,
                    "Independent delta+protected UTF-8 total and literal ID bytes");
            const std::array<qsizetype MigrationLimits::*, 4> field{
                &MigrationLimits::versions, &MigrationLimits::id_bytes,
                &MigrationLimits::schema_objects, &MigrationLimits::manifest_utf8_bytes};
            const std::array<qsizetype, 4> edge{target, longest_id, 252, bytes};
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
            require(charge.rows == 774 && charge.result_bytes == 902993 && charge.bind_bytes == 83,
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
            else
                mutate(f.db, "INSERT INTO main.subathon_migration_history VALUES(?,?,?)",
                       {version + 1, QString("overflow-witness"), QByteArray(32, 'z')});
            auto fresh = f.service();
            const auto before = protected_data(f.db);
            const auto result = f.lookup(fresh);
            require(std::get_if<ReviewRefusal>(&result) &&
                        std::get<ReviewRefusal>(result) == ReviewRefusal::unavailable &&
                        protected_data(f.db) == before,
                    "Review discloses no journal or typed outcome after malformed history/schema");
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
        QTest::newRow("head") << true;
        QTest::newRow("saved-revision") << false;
    }

    void rateNormalEntries() {
        QFETCH(bool, head);
        check([&] {
            namespace rate = rate_fixture;
            Populated f;
            rate::Authority authority;
            const auto preview = rate::first_preview();
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
                const auto snapshot = head ? unit.head() : unit.revision("S");
                const auto *selected = std::get_if<core::EffectiveRateSnapshot>(&snapshot);
                require(
                    selected &&
                        rate::snapshot_bytes(*selected) ==
                            rate::snapshot_bytes(preview.proposal.candidate) &&
                        selected->data().snapshot_ref == "S" &&
                        selected->data().components.size() == 1 &&
                        selected->data().components.front().members.size() == 2 &&
                        selected->data().components.front().members[0].currency == "EUR" &&
                        selected->data().components.front().members[0].weight == rate::ratio(1) &&
                        selected->data().components.front().members[1].currency == "USD" &&
                        selected->data().components.front().members[1].weight == rate::ratio(6, 5),
                    "Reconstruct committed rate revision and select exact 6/5");
            };
            f.db.write(read);
            advance(f.db);
            f.db.write(read);
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
            const auto entry = f.seed("dense", Case::positive());
            f.db.reopen();
            auto review = f.service();
            const auto command = f.bind("dense", {entry}, f.preview(review, {entry}));
            const auto result = f.execute(review, command);
            expected_row(f.reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
            const auto key = *f.reply(result).rows.front().outcome->facts->bundle;
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
            stage_scope(f.db);
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
            stage_scope(f.db);
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
            stage_scope(f.db);
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
            f.db.write([](const auto &s) {
                const SqliteRetainedContextStore store(s);
                require(store.retain(
                            {{RetainedKind::configuration, "cfg-new"}, "configuration-2", {}}) ==
                                RetainDisposition::inserted &&
                            store.retain({{RetainedKind::arrival_context, "ctx-new"},
                                          "context-2",
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
            auto bound = selected;
            if (change == 2)
                bound.receipt.facts.money = core::ReceiptMoney{"19.00", "EUR"};
            if (change == 3)
                bound.receipt.arrival.configuration_ref = "cfg";
            if (change == 4)
                bound.receipt.arrival.modifier_context_ref = "ctx";
            const auto before = protected_data(f.db);
            const auto result = read_missing(f.db, [&](const auto &s) {
                return read_attempt(s, bound, {core::ReceiptId{1}, change == 1 ? 1 : 2});
            });
            const auto expected =
                std::array{Stop::exact_replay, Stop::different_ordinal, Stop::different_facts,
                           Stop::different_configuration, Stop::different_context}
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
            stage_scope(f.db);
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
    }

    void sourcePolicyFence() {
        QFETCH(int, scenario);
        check([&] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db);
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
                                     "authorized-new-cut"};
            auto durable = [&] {
                return std::array{
                    f.db.rows("SELECT epoch,generation,revision,policy_revision,cut,frontier "
                              "FROM main.subathon_intake_sources"),
                    f.db.rows("SELECT revision,document FROM main.subathon_intake_policies "
                              "ORDER BY revision"),
                    f.db.rows("SELECT policy_revision,document FROM main.subathon_intake_members "
                              "ORDER BY policy_revision"),
                    f.db.rows("SELECT epoch,policy_revision,start_frontier,document "
                              "FROM main.subathon_intake_gaps")};
            };
            const auto before = durable();
            require(before[0] == SqlRows{{1, QByteArray("g3"), 9, 1, QByteArray("cut"),
                                          QByteArray("F")}} &&
                        before[1] == SqlRows{{1, QByteArray("review-policy")}} &&
                        before[2] == SqlRows{{1, QByteArray("review-lane")}} &&
                        before[3] == SqlRows{{1, 1, QByteArray("F"), QByteArray("gap-basis")}},
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
                            durable() == before,
                        "Stale or missing CAS leaves every retained head and gap unchanged");
            require(**result == (scenario == 0 ? Stop::committed : Stop::stale),
                    "A single owner unit compares both captured heads and generation");
            if (scenario == 0) {
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
            }
        });
    }

    void operationBinding_data() {
        QTest::addColumn<int>("scenario");
        QTest::newRow("first-binding") << 0;
        QTest::newRow("same-immutable-intent") << 1;
        QTest::newRow("same-key-different-utc-start") << 2;
        QTest::newRow("different-active-operation") << 3;
    }

    void operationBinding() {
        QFETCH(int, scenario);
        check([&] {
            Populated f;
            f.install();
            advance(f.db);
            stage_scope(f.db);
            const auto bound = QByteArray::fromHex(
                "0000000a323032362d31312d3231000000035554430000000162000001a1fb671800");
            require(bound.size() == 34, "Literal local-date, zone, basis and signed UTC frame");
            if (scenario != 0)
                mutate(
                    f.db,
                    "INSERT INTO main.subathon_intake_operations VALUES("
                    "X'696e7374616c6c',X'4f',X'50',X'41',X'53',1,1,NULL,1,X'6733',1,?,X'54',1,0,0)",
                    {bound});
            OperationIntent intent{"install",
                                   "P",
                                   "A",
                                   "S",
                                   scenario == 3 ? "Q" : "O",
                                   1,
                                   1,
                                   "g3",
                                   "T",
                                   "2026-11-21",
                                   "UTC",
                                   "b",
                                   1795219200000LL + (scenario == 2 ? 1 : 0)};
            // The resolved instant is retained as a bound value, separate from provider order.
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
                std::array{Stop::committed, Stop::exact_replay, Stop::conflict, Stop::busy}.at(
                    scenario);
            const auto after = f.db.rows("SELECT operation_ref,intent,revision,next_ordinal,"
                                         "continuation,slots FROM main.subathon_intake_operations");
            if (**result != Stop::committed)
                require(work.transaction == TransactionOutcome::rolled_back && !work.value &&
                            after == before,
                        "Exact selection or known refusal changes no durable history slot");
            require(
                **result == expected,
                "Same intent reuses binding; changed UTC conflicts; another active key is busy");
            if (expected == Stop::committed) {
                finance::normal(work);
                require(after == SqlRows{{QByteArray("O"), bound, 1, 1, QByteArray("T"), 0}},
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
            const auto streams = scenario == 3 ? 2 : scenario == 4 ? 4 : scenario >= 2 ? 1 : 0;
            const auto stream_name = [](int i) {
                return QByteArray("stream-") + QByteArray::number(i);
            };
            const QByteArray provenance("fixture-proof");
            // Test input documents use one tag and two BE32 length fields.
            // The SQL anchor stores the normalized/provenance components and
            // the full independently derived declared charge, not an unframed estimate.
            const auto document = [&](int bytes) {
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
            const auto requested_charge = scenario == 1 ? 32769 : charge;
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
                           Stop::quarantine_full, Stop::quarantine_full, Stop::exact_replay}
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
                                 "6733',2,?,X'54',1,2,0)",
                                 {QByteArray("literal-catchup-intent")}});
                (void)s.execute({"INSERT INTO main.subathon_intake_pages "
                                 "VALUES(X'696e7374616c6c',X'4f',1,?,X'54',2,?,0,NULL)",
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
                                      "qualified-contiguous-proof"};
            const auto expected =
                std::array{Stop::incomplete_prefix, Stop::committed, Stop::stale,    Stop::conflict,
                           Stop::conflict,          Stop::committed, Stop::committed}
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
            stage_scope(f.db);
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
            const auto selected = read_missing(
                f.db, [&](const auto &s) { return select_evidence(s, phase == 4 ? first : peer); });
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
            stage_scope(f.db);
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
