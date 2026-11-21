// SPDX-License-Identifier: GPL-3.0-or-later
#include "receipt_fixture.hpp"

#include "../../../server/src/sqlite_selected_record_read.hpp"
#include "../support/milestone_target_edit_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "../support/sqlite_rate_override_writer_fixture.hpp"

#include <subathon/server/sqlite_reference_authority.hpp>
#include <subathon/server/sqlite_reference_capture.hpp>

namespace {
using namespace storage_026_fixture;

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

ApplicationOperationKey seed_dense(Populated &f) {
    const auto entry = f.seed("dense", Case::positive());
    f.db.reopen();
    auto review = f.service();
    const auto command = f.bind("dense", {entry}, f.preview(review, {entry}));
    const auto result = f.execute(review, command);
    expected_row(f.reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
    return *f.reply(result).rows.front().outcome->facts->bundle;
}

} // namespace

class Storage026CatalogTests final : public QObject {
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
            auto read = [&](const auto &s,
                            const std::optional<ApplicationHoldDetection> &attempted) {
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
                    require(ordinary.record_detection(*attempted).disposition ==
                                SqliteHoldMutationDisposition::hold_conflict,
                            "Ordinary detection cannot claim a reserved financial hold identity");
                }
            };
            auto read_committed = [&] {
                // Construct against the committed head before entering the writer.
                std::optional<ApplicationHoldDetection> attempted;
                if (entry == 2)
                    attempted = f.file.ordinary_detection(f.financial->hold_ref, false);
                f.file.write([&](const auto &s) { read(s, attempted); });
            };
            read_committed();
            ContributionReview before(*f.file.writer, f.facts);
            auto command =
                entry == 3 ? std::optional{CreditedHold::bind(before, "close-ordinary",
                                                              f.intent(ReviewVerb::reject_held))}
                           : std::nullopt;
            const auto financial = f.file.rows("SELECT * FROM main.subathon_financial_refunds");
            upgrade_financial(f.file);
            if (entry < 3) {
                read_committed();
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
};

QTEST_GUILESS_MAIN(Storage026CatalogTests)
#include "catalog_tests.moc"
