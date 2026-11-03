// SPDX-License-Identifier: GPL-3.0-or-later
// Independent foundation ONLY. No new service, codec or reference-view entry is invoked.
#include "support/application_service_fixture.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

using namespace application_service_fixture;
namespace {
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
void catalog(Observer &observer, const Data &data) {
    const auto wanted = data.catalog.frozen["manifest"].toObject()["expected_schema"].toArray();
    const auto seen =
        observer.rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
    require(wanted.size() == 89 && seen.size() == wanted.size(),
            "Literal full89 catalog inventory");
    for (qsizetype n = 0; n < wanted.size(); ++n) {
        const auto e = wanted[n].toObject();
        require(seen[n][0].toString() == e["type"].toString() &&
                    seen[n][1].toString() == e["name"].toString() &&
                    seen[n][2].toString() == e["table_name"].toString() &&
                    (e["sql"].isNull() ? seen[n][3].isNull()
                                       : seen[n][3].metaType().id() == QMetaType::QString &&
                                             seen[n][3].toString() == e["sql"].toString()),
                "Every literal physical SQL/NULL catalog field");
    }
    const auto history = observer.rows("SELECT version,migration_id,content_sha256 FROM "
                                       "subathon_migration_history ORDER BY version");
    require(history.size() == 6, "All six migration history records");
    for (qsizetype n = 0; n < history.size(); ++n) {
        const auto m =
            n == 5 ? data.catalog.frozen["manifest"].toObject() : data.catalog.old.manifests[n];
        require(history[n][0].toLongLong() == n + 1 &&
                    history[n][1].metaType().id() == QMetaType::QString &&
                    history[n][1].toString() == m["id"].toString() &&
                    history[n][2].metaType().id() == QMetaType::QByteArray &&
                    history[n][2].toByteArray().toHex() == m["sha256_hex"].toString().toLatin1(),
                "Immutable old-prefix and step6 migration identity/hash");
    }
    require(observer.rows("PRAGMA foreign_key_check").empty() &&
                observer.rows("PRAGMA integrity_check") == SqlRows{{QString("ok")}},
            "Actual complete raw foreign-key and integrity closure");
}
void arrangement(SqliteWriter &writer, const Data &data, const QString &name) {
    (void)run(writer, [&](auto context) -> QVariant {
        const auto session = context.begin();
        data.populate_arrangement(session, name);
        session.commit();
        return 1;
    });
}
size_t command_bytes(const OriginalApplicationCommand &c) {
    auto total = c.operation.operation_scope.size() + c.operation.operation_ref.size() + 1 + 8 +
                 (c.contribution_id ? 8 : 0) + c.plan.plan_ref.size() + 8 + 1 +
                 c.principal.installation_ref.size() + c.principal.principal_ref.size() + 1 + 1;
    for (const auto *s : {&c.preview_state_token, &c.actor_ref, &c.decision_ref, &c.reason})
        if (*s)
            total += (*s)->size();
    for (const auto &e : c.choices)
        total += e.format_ref.size() + size_t(e.document.size());
    return total;
}
void check_observation(const ApplicationHoldObservation &a, const ApplicationHoldObservation &b) {
    require(a.persisted_state_token == b.persisted_state_token &&
                a.policy_revision_ref == b.policy_revision_ref &&
                a.permission_revision_ref == b.permission_revision_ref &&
                a.actor_ref == b.actor_ref && a.decision_ref == b.decision_ref &&
                a.observed_utc_ms == b.observed_utc_ms && a.elapsed_ms == b.elapsed_ms,
            "Every known/unknown hold observation member");
    original_claim_fixture::same_envelope(a.annotation, b.annotation);
}
void check_resolution(const ApplicationHoldRecordedResolution &a,
                      const ApplicationHoldRecordedResolution &b) {
    const auto &x = a.request, &y = b.request;
    require(x.mutation_ref == y.mutation_ref && x.subject.id == y.subject.id &&
                x.expected_active.size() == y.expected_active.size() &&
                x.entries.size() == y.entries.size() && x.actor_ref == y.actor_ref &&
                x.decision_ref == y.decision_ref && x.reason == y.reason && !x.application &&
                !y.application && a.produced_versions.size() == b.produced_versions.size(),
            "Complete finite independent resolution payload");
    original_claim_fixture::same_envelope(x.options, y.options);
    for (size_t n = 0; n < y.expected_active.size(); ++n)
        require(x.expected_active[n].hold_ref == y.expected_active[n].hold_ref &&
                    x.expected_active[n].version_ref == y.expected_active[n].version_ref,
                "Complete exact expected active set DATA");
    for (size_t n = 0; n < y.entries.size(); ++n) {
        const auto &p = x.entries[n], &q = y.entries[n];
        require(p.hold_ref == q.hold_ref && p.expected_version_ref == q.expected_version_ref &&
                    p.proposed_version_ref == q.proposed_version_ref && !p.replacement &&
                    !q.replacement,
                "Complete independent no-application/no-replacement resolution entry");
        original_claim_fixture::same_envelope(p.decision, q.decision);
    }
    for (size_t n = 0; n < b.produced_versions.size(); ++n) {
        const auto &p = a.produced_versions[n], &q = b.produced_versions[n];
        require(p.locator.hold_ref == q.locator.hold_ref &&
                    p.locator.version_ref == q.locator.version_ref && p.position == q.position &&
                    p.parent_version_ref == q.parent_version_ref && p.state == q.state &&
                    p.producer_mutation_ref == q.producer_mutation_ref && !p.replacement &&
                    !q.replacement,
                "Every complete produced historical version");
    }
}
void check_hold(const ApplicationHoldView &a, const ApplicationHoldView &b) {
    const auto &x = a.cause, &y = b.cause;
    require(x.hold_ref == y.hold_ref && x.source.subject.id == y.source.subject.id &&
                x.source.receipt_id == y.source.receipt_id &&
                x.source.event_ref == y.source.event_ref &&
                x.source.parent_event_ref == y.source.parent_event_ref &&
                x.source.root_event_ref == y.source.root_event_ref &&
                x.source.configuration_ref == y.source.configuration_ref &&
                x.source.arrival_context_ref == y.source.arrival_context_ref && x.kind == y.kind &&
                x.command.operation.operation_scope == y.command.operation.operation_scope &&
                x.command.operation.operation_ref == y.command.operation.operation_ref &&
                x.plan_evidence.kind == y.plan_evidence.kind &&
                x.plan_evidence.extent == y.plan_evidence.extent && x.plan_evidence.plan &&
                y.plan_evidence.plan &&
                x.plan_evidence.plan->plan_ref == y.plan_evidence.plan->plan_ref &&
                x.plan_evidence.plan->revision == y.plan_evidence.plan->revision &&
                !x.plan_evidence.stopped_for && !x.plan_evidence.failed_capture &&
                x.plan_evidence.occurrences.size() == y.plan_evidence.occurrences.size() &&
                x.failure.has_value() == y.failure.has_value() && !x.timer &&
                x.missing_policy_reason == y.missing_policy_reason &&
                x.supersedes_hold_ref == y.supersedes_hold_ref,
            "Complete independent captured cause/source/command groups");
    original_claim_fixture::same_envelope(x.command.intent, y.command.intent);
    check_observation(x.first_observation, y.first_observation);
    for (size_t n = 0; n < y.plan_evidence.occurrences.size(); ++n) {
        const auto &p = x.plan_evidence.occurrences[n], &q = y.plan_evidence.occurrences[n];
        require(p.ordinal == q.ordinal && p.action_ref == q.action_ref &&
                    p.action_version_ref == q.action_version_ref && p.rule_ref == q.rule_ref &&
                    p.target_ref == q.target_ref && p.unit == q.unit &&
                    p.attempted_units == q.attempted_units &&
                    p.finalized_units == q.finalized_units && p.explanation == q.explanation,
                "Every literal complete captured occurrence");
    }
    if (y.failure) {
        require(x.failure->stage == y.failure->stage &&
                    x.failure->failing_ordinal == y.failure->failing_ordinal &&
                    x.failure->failing_target_ref == y.failure->failing_target_ref &&
                    x.failure->attempted_units == y.failure->attempted_units,
                "Exact known whole-input overflow evidence");
        original_claim_fixture::same_envelope(x.failure->annotation, y.failure->annotation);
    }
    require(a.first_detection.mutation_ref == b.first_detection.mutation_ref &&
                a.first_detection.expected_version_ref == b.first_detection.expected_version_ref &&
                a.first_detection.proposed_version_ref == b.first_detection.proposed_version_ref &&
                a.first_version.locator.hold_ref == b.first_version.locator.hold_ref &&
                a.first_version.locator.version_ref == b.first_version.locator.version_ref &&
                a.first_version.position == 1 && !a.first_version.parent_version_ref &&
                a.first_version.state == ApplicationHoldState::active &&
                a.first_version.producer_mutation_ref == b.first_version.producer_mutation_ref &&
                !a.first_version.replacement &&
                a.current_version.locator.hold_ref == b.current_version.locator.hold_ref &&
                a.current_version.locator.version_ref == b.current_version.locator.version_ref &&
                a.current_version.position == b.current_version.position &&
                a.current_version.parent_version_ref == b.current_version.parent_version_ref &&
                a.current_version.state == b.current_version.state &&
                a.current_version.producer_mutation_ref ==
                    b.current_version.producer_mutation_ref &&
                !a.current_version.replacement &&
                a.current_event.has_value() == b.current_event.has_value(),
            "Complete finite version lineage and event presence");
    check_observation(a.first_detection.observation, b.first_detection.observation);
    original_claim_fixture::same_envelope(a.first_detection.audit, b.first_detection.audit);
    if (b.current_event) {
        require(std::holds_alternative<ApplicationHoldRecordedResolution>(*a.current_event) &&
                    std::holds_alternative<ApplicationHoldRecordedResolution>(*b.current_event),
                "Actual current event is the literal finite resolution");
        check_resolution(std::get<ApplicationHoldRecordedResolution>(*a.current_event),
                         std::get<ApplicationHoldRecordedResolution>(*b.current_event));
    }
}
} // namespace

class ApplicationServiceEnvironment final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void literalFramesAndOwnedDataRemainIndependent() {
        scenario([] {
            const Data data;
            const auto anchors = data.frozen["codec_anchors"].toObject();
            require(anchors.size() == 4, "Four independently frozen command anchors");
            for (auto it = anchors.begin(); it != anchors.end(); ++it) {
                const auto a = it.value().toObject();
                const auto c = command(a["command"]);
                const auto e = envelope(a["envelope"]);
                require(
                    e.document.size() == a["wire_bytes"].toInt() && !e.document.isNull() &&
                        command_bytes(c) == size_t(a["owned_command_bytes"].toInt()) &&
                        c.choices.size() == size_t(a["items"].toInt()) &&
                        QCryptographicHash::hash(e.document, QCryptographicHash::Sha256).toHex() ==
                            a["sha256"].toString().toLatin1(),
                    "Literal framed bytes, manual logical ownership and digest agree");
            }
            require(envelope(anchors["C0"].toObject()["envelope"]).document.toHex() ==
                        "534143310000000173000000016f010000000000000007000000000170"
                        "0000000000000001000000000169000000017500000000000000000000",
                    "Explicit independent58-byte C0 grammar literal");
            const auto empty = command(anchors["empty_reason"].toObject()["command"]);
            require(empty.reason && empty.reason->empty() &&
                        !command(anchors["C0"].toObject()["command"]).reason,
                    "Absent versus present-empty command DATA remain distinct");
            require(
                command(anchors["CID0"].toObject()["command"]).contribution_id ==
                        std::optional{core::ContributionId{0}} &&
                    command(anchors["choice"].toObject()["command"]).choices[0].document.toHex() ==
                        "efbbbf00ff41",
                "CID0 and raw BOM/NUL/FF are not normalized");
            const auto cases = data.frozen["cases"].toObject();
            require(cases.size() == 30, "Bounded independently authored scenario inventory");
            for (auto it = cases.begin(); it != cases.end(); ++it) {
                const auto c = it.value().toObject(), o = c["oracles"].toObject();
                require(command_bytes(command(c["command"])) ==
                                size_t(o["command_bytes"].toInt()) &&
                            envelope(c["command_envelope"]).document.size() ==
                                o["encoded_document_bytes"].toInt(),
                        "Every frozen command's actual owned input and encoded length");
            }
            const auto max = data.command("maximum_subject");
            require(max.receipt_id.value == std::numeric_limits<std::uint64_t>::max() &&
                        max.contribution_id->value == std::numeric_limits<std::uint64_t>::max() &&
                        requests(data.case_data("min_fits")["requests"])[0].requested_units ==
                            std::numeric_limits<std::int64_t>::min(),
                    "UnsignedMAX and signedMIN stay typed");
            const auto fractional = proposal(data.case_data("fractional"));
            require(fractional.effects[0].exact_requested_units == core::Exact(3) / 2 &&
                        fractional.effects[0].requested_units == 2 &&
                        fractional.effects[0].rounding_residual == -core::Exact(1) / 2 &&
                        fractional.effects[1].rounding_residual == core::Exact(1) / 2,
                    "Captured half-even finals and opposite residuals are frozen independently");
            const auto boundary = data.frozen["boundary_anchor"].toObject();
            const auto boundary_command = command(boundary["command"]);
            const auto boundary_envelope = envelope(boundary["envelope"]);
            require(boundary_command.choices.size() == 2 && command_bytes(boundary_command) == 27 &&
                        boundary_envelope.document.size() == 76 &&
                        boundary_envelope.format_ref.size() +
                                size_t(boundary_envelope.document.size()) ==
                            116 &&
                        !boundary_command.choices[0].document.isNull() &&
                        boundary_command.choices[0].document.isEmpty(),
                    "CS17 two present-empty choices own27/I2; opaque encoded envelope116/I0");
            const auto clock_failure = data.case_data("clock_overflow");
            require(state(clock_failure["head"]).timer.main_remaining_ms == 100 &&
                        timer(clock_failure["clock_advanced_timer"]).main_remaining_ms == 90 &&
                        clock(clock_failure["clock"]).segments[0].elapsed_ms == 10,
                    "CS04 known clock-only proposal100→90 remains distinct from retained head");
            const auto elapsed_case = data.case_data("elapsed_success");
            const auto elapsed = record(elapsed_case["expected_record"]);
            const auto elapsed_proposal = proposal(elapsed_case);
            require(elapsed.persisted_head.timer.main_remaining_ms == 100 &&
                        elapsed.advanced_before.timer.main_remaining_ms == 90 &&
                        elapsed.after.timer.main_remaining_ms == 100 &&
                        elapsed_proposal.clock_advanced_timer.main_remaining_ms == 90 &&
                        elapsed_proposal.timer_after.main_remaining_ms == 100 &&
                        elapsed_proposal.observation.segments[0].elapsed_ms == 10 &&
                        elapsed.persisted_head.token == "a" &&
                        elapsed.advanced_before.token == "b" && elapsed.after.token == "c" &&
                        elapsed.advanced_before.time_debt_ms ==
                            elapsed.persisted_head.time_debt_ms &&
                        elapsed.advanced_before.configuration_ref ==
                            elapsed.persisted_head.configuration_ref &&
                        elapsed.advanced_before.envelope.format_ref ==
                            elapsed.persisted_head.envelope.format_ref &&
                        elapsed.advanced_before.balances.size() ==
                            elapsed.persisted_head.balances.size(),
                    "AS19 literal100→90 clock-before→100, distinct fresh snapshots and unchanged "
                    "debt/config/format");
            for (size_t n = 0; n < elapsed.persisted_head.balances.size(); ++n)
                require(elapsed.advanced_before.balances[n].kind ==
                                elapsed.persisted_head.balances[n].kind &&
                            elapsed.advanced_before.balances[n].target_ref ==
                                elapsed.persisted_head.balances[n].target_ref &&
                            elapsed.advanced_before.balances[n].units ==
                                elapsed.persisted_head.balances[n].units,
                        "AS19 clock-only before snapshot preserves every counter key/value");
        });
    }
    void rawSourceStateAndPlanBranchesHaveCompleteOldClosure_data() {
        QTest::addColumn<QString>("name");
        const auto cases = Data{}.frozen["cases"].toObject();
        for (auto it = cases.begin(); it != cases.end(); ++it)
            QTest::newRow(it.key().toUtf8().constData()) << it.key();
    }
    void rawSourceStateAndPlanBranchesHaveCompleteOldClosure() {
        QFETCH(QString, name);
        scenario([&] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable source/state branch");
            const auto path = directory.filePath("source.sqlite");
            SqliteWriter writer;
            open_sources(writer, path,
                         data); // Earlier intake/capture COMMIT, separate from case/work.
            retain_case(writer, data, name);
            Observer observer(path);
            catalog(observer, data);
            require(observer.rows("SELECT hex(last_allocated) FROM subathon_receipt_allocator") ==
                            SqlRows{{QString("FFFFFFFFFFFFFFFF")}} &&
                        observer.rows(
                            "SELECT hex(last_allocated) FROM subathon_contribution_allocator") ==
                            SqlRows{{QString("FFFFFFFFFFFFFFFF")}},
                    "Real allocatorMAX exhausted anchors");
            require(observer.rows("SELECT COUNT(*) FROM subathon_receipts") ==
                            SqlRows{{qint64(5)}} &&
                        observer.rows("SELECT COUNT(*) FROM subathon_receipt_attempts") ==
                            SqlRows{{qint64(5)}},
                    "Five actual retained receipts and transport attempts");
            const auto confirmation_rows = observer.rows(
                "SELECT kind,evidence,support_monetary,typeof(support_policy_ref) FROM "
                "subathon_contribution_observations WHERE receipt_id=X'0000000000000009'");
            require(confirmation_rows.size() == 1 && confirmation_rows[0].size() == 4 &&
                        confirmation_rows[0][0].toLongLong() == 1 &&
                        confirmation_rows[0][1].toLongLong() == 2 &&
                        confirmation_rows[0][2].isNull() &&
                        confirmation_rows[0][3].toString() == "null",
                    "Receipt9 is a genuine linked confirmation with absent support group");
            const auto unknowns = observer.rows(
                "SELECT source_timestamp_utc_ms,actor_id,money_original_decimal,payment_namespace "
                "FROM subathon_receipts WHERE receipt_id=X'0000000000000007'");
            require(unknowns.size() == 1 && unknowns[0].size() == 4 && unknowns[0][0].isNull() &&
                        unknowns[0][1].isNull() && unknowns[0][2].isNull() &&
                        unknowns[0][3].isNull(),
                    "Unknown source facts remain SQL NULL");
            (void)run(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                const SqliteApplicationBundleStore store(session);
                const auto cmd = data.command(name);
                const auto input = store.read_inputs(cmd.plan, cmd.subject_kind);
                require(input.disposition == SqliteBundleInputDisposition::ready && input.inputs &&
                            input.inputs->actual_plan_head == 1 &&
                            input.inputs->selected_plan.revision == 1 &&
                            input.inputs->selected_plan.captured.requested_effects ==
                                requests(data.case_data(name)["requests"]),
                        "Old reader validates exact selected full immutable request closure");
                check_state(input.inputs->current_state, data.head(name));
                const auto &event = input.inputs->selected_plan.captured.event;
                require(event.receipt_id == cmd.receipt_id &&
                            event.contribution_id == cmd.contribution_id &&
                            event.arrival.configuration_ref == "cfg" &&
                            event.arrival.modifier_context_ref == "ctx",
                        "Literal selected receipt/CID/configuration/context binding");
                if (cmd.contribution_id && cmd.contribution_id->value == 0) {
                    require(input.inputs->current_contribution &&
                                input.inputs->current_contribution->contribution.state
                                        .payment_evidence == core::PaymentEvidence::confirmed &&
                                input.inputs->current_contribution->support_anchor.observation
                                        .evidence == core::PaymentEvidence::not_applicable &&
                                input.inputs->current_contribution->support_anchor.observation
                                        .receipt.id == core::ReceiptId{7},
                            "Current confirmation evidence does not overwrite immutable original "
                            "support");
                }
                if (data.case_data(name).contains("prior_record")) {
                    const auto prior = record(data.case_data(name)["prior_record"]);
                    const auto actual = store.lookup(prior.intent.operation);
                    require(actual.has_value(),
                            "Stale preview has a complete genuine earlier operation");
                    check_record(*actual, prior);
                    require(prior.persisted_head.timer.main_remaining_ms == 3300000 &&
                                prior.after.timer.main_remaining_ms == 3480000,
                            "AS01/02 preview3300000→current3480000 is actual immutable history");
                }
                session.commit();
                return 1;
            });
            const auto c = data.case_data(name);
            if (c.contains("expected_record")) {
                const auto before = data.snapshot(observer);
                QString problem;
                const auto result = capture(writer, [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    try {
                        const SqliteApplicationBundleStore store(session);
                        const auto expected = record(c["expected_record"]);
                        require(store.stage(prepared(c["expected_record"])).disposition ==
                                    SqliteBundleStageDisposition::staged,
                                "Existing writer accepts independently literal complete fixture "
                                "bundle");
                        const auto found = store.lookup(expected.intent.operation);
                        require(found.has_value(),
                                "Existing full lookup sees provisional fixture record");
                        check_record(*found, expected);
                        session.rollback();
                        return 1;
                    } catch (const std::exception &e) {
                        problem = QString::fromUtf8(e.what());
                        throw;
                    }
                });
                require(problem.isEmpty(), "Literal old-stage fixture diagnostic: " + problem);
                require(result.transaction == TransactionOutcome::rolled_back &&
                            result.error == WorkError::not_committed && !result.value &&
                            data.snapshot(observer) == before,
                        "Old-store fixture diagnostic rolls back all provisional data without "
                        "service calls");
            }
            if (name == "add10") {
                for (const auto *fault :
                     {"unknown_state_format", "known_state_bad_magic", "known_state_trailing_byte",
                      "known_state_typed_contradiction"}) {
                    const auto before = data.snapshot(observer);
                    bool observed = false;
                    const auto result = capture(writer, [&](auto context) -> QVariant {
                        const auto session = context.begin();
                        data.apply_fault(session, fault);
                        // OLD reader treats the envelope as opaque DATA, never new service
                        // semantics.
                        const auto current = SqliteApplicationBundleStore(session).read_state();
                        require(current.has_value(),
                                "Literal mutation selects actual retained head");
                        if (QString(fault) == "unknown_state_format")
                            observed = current->envelope.format_ref == "synthetic.future.state.v9";
                        else if (QString(fault) == "known_state_bad_magic")
                            observed = current->envelope.document == "BAD1";
                        else if (QString(fault) == "known_state_trailing_byte")
                            observed = current->envelope.document.endsWith(char(0));
                        else
                            observed = current->timer.main_remaining_ms == 57;
                        session.rollback();
                        return 1;
                    });
                    require(observed && result.transaction == TransactionOutcome::rolled_back &&
                                result.error == WorkError::not_committed &&
                                data.snapshot(observer) == before,
                            "Actual Qt SQL fault DATA is valid and reversibly isolated");
                }
            }
        });
    }
    void completeHistoricalRecordsRetainLiteralServiceDocumentsAndGlobalClaims() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable history branch");
            const auto path = directory.filePath("history.sqlite");
            SqliteWriter writer;
            open_sources(writer, path, data);
            arrangement(writer, data, "history");
            Observer observer(path);
            catalog(observer, data);
            require(observer.rows("SELECT COUNT(*) FROM subathon_app_operations") ==
                            SqlRows{{qint64(3)}} &&
                        observer.rows("SELECT hex(token) FROM subathon_app_head") ==
                            SqlRows{{QString("67")}},
                    "Actual three-record chain a→b→c→d→e→f→g with terminal headg");
            const auto records =
                data.frozen["arrangements"].toObject()["history"].toObject()["records"].toObject();
            (void)run(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                const SqliteApplicationBundleStore store(session);
                for (auto it = records.begin(); it != records.end(); ++it) {
                    const auto expected = record(it.value());
                    const auto actual = store.lookup(expected.intent.operation);
                    require(actual.has_value(),
                            "Old full lookup owns every selected historical record");
                    check_record(*actual, expected);
                    require(actual->jobs.size() == 1 &&
                                actual->jobs[0].envelope.document.contains(
                                    QByteArray::fromHex("efbbbf00ff41")) &&
                                actual->jobs[0].envelope.format_ref ==
                                    "subathon.internal.application-job-synthetic.v1",
                            "Synthetic non-deliverable job retains actual raw payload bytes");
                }
                const auto by_subject = store.lookup_original_claim(
                    {ApplicationSubjectKind::contribution, {8}, core::ContributionId{0}});
                const auto by_source = store.lookup_original_claim(
                    {ApplicationSubjectKind::contribution, {8}, core::ContributionId{42}});
                require(
                    by_subject && by_subject->kind == SqliteOriginalClaimKind::subject &&
                        by_subject->existing.intent.operation.operation_scope == "history-alpha" &&
                        by_source && by_source->kind == SqliteOriginalClaimKind::source &&
                        by_source->existing.intent.operation.operation_scope == "history-gamma" &&
                        by_source->existing.effects[0].requested_units == -3,
                    "Global subjectCID0 wins actual competing signed ORIGINAL receipt8 source");
                const auto confirmation = store.lookup_original_claim(
                    {ApplicationSubjectKind::contribution, {9}, core::ContributionId{0}});
                require(confirmation && confirmation->kind == SqliteOriginalClaimKind::subject &&
                            confirmation->existing.intent.receipt_id == core::ReceiptId{7},
                        "Genuine later linked confirmation does not create another subject award");
                session.commit();
                return 1;
            });
            for (const auto *fault :
                 {"unknown_historical_result_format", "known_historical_result_trailing_byte"}) {
                const auto before = data.snapshot(observer);
                bool observed = false;
                const auto result = capture(writer, [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    data.apply_fault(session, fault);
                    const auto selected =
                        SqliteApplicationBundleStore(session).lookup({"history-alpha", "A"});
                    require(selected.has_value(),
                            "Literal result fault selects full historical record");
                    observed = QString(fault) == "unknown_historical_result_format"
                                   ? selected->result.format_ref == "synthetic.future.result.v9"
                                   : selected->result.document.endsWith(char(0));
                    session.rollback();
                    return 1;
                });
                require(observed && result.transaction == TransactionOutcome::rolled_back &&
                            result.error == WorkError::not_committed &&
                            data.snapshot(observer) == before,
                        "Actual Qt SQL historical fault DATA preserves other literal facts");
            }
        });
    }
    void independentHoldsAndResolvedRetryRetainLiteralHistory() {
        scenario([] {
            const Data data;
            const auto expected = data.holds();
            for (const auto *name : {"active_holds", "resolved_retry"}) {
                QTemporaryDir directory;
                require(directory.isValid(), "Disposable independent hold branch");
                const auto path = directory.filePath("holds.sqlite");
                SqliteWriter writer;
                open_sources(writer, path, data);
                arrangement(writer, data, name);
                Observer observer(path);
                catalog(observer, data);
                require(observer.rows("SELECT COUNT(*) FROM subathon_app_operations") ==
                                SqlRows{{qint64(0)}} &&
                            observer.rows("SELECT hex(token) FROM subathon_app_head") ==
                                SqlRows{{QString("61")}},
                        "No applied prefix, phantom token or head mutation from hold "
                        "recording/resolution");
                (void)run(writer, [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    const SqliteApplicationHoldStore store(session);
                    const auto active = store.active({core::ContributionId{0}});
                    if (QString(name) == "active_holds") {
                        require(active.holds.size() == 2, "Complete two independent global causes");
                        check_hold(active.holds[0], expected.view("h1"));
                        check_hold(active.holds[1], expected.view("h2"));
                        const auto &numeric = active.holds[0];
                        original_claim_fixture::same_envelope(
                            numeric.cause.first_observation.annotation,
                            numeric.first_detection.observation.annotation);
                        original_claim_fixture::same_envelope(numeric.cause.failure->annotation,
                                                              numeric.first_detection.audit);
                        original_claim_fixture::same_envelope(
                            numeric.cause.failure->annotation,
                            numeric.cause.first_observation.annotation);
                        require(
                            !active.holds[1].cause.first_observation.observed_utc_ms &&
                                !active.holds[1].cause.first_observation.elapsed_ms &&
                                !active.holds[1].cause.first_observation.policy_revision_ref &&
                                active.holds[1].cause.first_observation.permission_revision_ref ==
                                    std::optional<std::string>{"grant"},
                            "Known access and absent policy/clock times coexist honestly");
                    } else {
                        require(active.holds.empty(),
                                "Resolved historical detection is not an active blocker");
                        const auto view = store.find("hRetry");
                        require(view.has_value(), "Closed immutable cause remains findable");
                        check_hold(*view, expected.view("hRetry"));
                        const auto history = store.history({"hRetry", 0, "v2", 64});
                        require(
                            history.disposition == SqliteHoldHistoryDisposition::ready &&
                                history.page && history.page->entries.size() == 2 &&
                                !history.page->has_more && !history.page->next_position &&
                                std::holds_alternative<ApplicationHoldDetection>(
                                    history.page->entries[0].event) &&
                                std::holds_alternative<ApplicationHoldRecordedResolution>(
                                    history.page->entries[1].event),
                            "Finite actual detection→resolution history with truthful currentv2");
                        const auto retry = store.record_detection(expected.detection("dRetry"));
                        require(
                            retry.disposition == SqliteHoldMutationDisposition::already_recorded &&
                                retry.outcome && retry.outcome->produced_versions.size() == 1 &&
                                retry.outcome->produced_versions[0].state ==
                                    ApplicationHoldState::active &&
                                retry.outcome->produced_versions[0].locator.version_ref == "v1" &&
                                store.find("hRetry")->current_version.state ==
                                    ApplicationHoldState::resolved,
                            "CS16 exact immutable retry owns historical activev1 without claiming "
                            "current durableHeld");
                    }
                    session.commit();
                    return 1;
                });
                catalog(observer, data);
            }
            for (const auto &[case_name, detection] :
                 {std::pair{"score_overflow", "dNew"}, std::pair{"clock_overflow", "dClock"}}) {
                QTemporaryDir directory;
                require(directory.isValid(), "Disposable literal hold-proposal diagnostic");
                const auto path = directory.filePath("fresh-hold.sqlite");
                SqliteWriter writer;
                open_sources(writer, path, data);
                retain_case(writer, data, case_name);
                Observer observer(path);
                const auto before = data.snapshot(observer);
                bool checked = false;
                const auto result = capture(writer, [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    const SqliteApplicationHoldStore store(session);
                    const auto d = expected.detection(detection);
                    const auto staged = store.record_detection(d);
                    require(staged.disposition == SqliteHoldMutationDisposition::recorded &&
                                staged.outcome,
                            "Old hold store accepts independently frozen complete SAH1 creation");
                    const auto view = store.find(d.cause.hold_ref);
                    require(view.has_value(), "Old reader owns provisional literal hold closure");
                    check_hold(*view, expected.view(d.cause.hold_ref.c_str()));
                    checked = true;
                    session.rollback();
                    return 1;
                });
                require(checked && result.transaction == TransactionOutcome::rolled_back &&
                            result.error == WorkError::not_committed &&
                            data.snapshot(observer) == before,
                        "Old primitive validates proposal DATA; rollback retains no "
                        "hold/application/head changes");
            }
        });
    }
    void scriptedPortsReturnOnlyOwnedSyntheticData() {
        scenario([] {
            const Data data;
            ScriptedPorts ports(data, "add10");
            const ApplicationReferenceReadView inert_view; // Never invoke its missing read method.
            const auto cmd = data.command("add10");
            ApplicationBundleInputs inputs;
            inputs.subject_kind = cmd.subject_kind;
            inputs.plan = cmd.plan;
            inputs.actual_plan_head = 1;
            inputs.current_state = data.head("add10");
            const auto access = ports.check(inert_view, cmd);
            auto p =
                std::get<BoundApplicationPolicy>(ports.resolve(inert_view, cmd, inputs, access));
            auto c =
                std::get<ApplicationClockObservation>(ports.observe(inert_view, cmd, inputs, p));
            const auto numeric = proposal(data.case_data("add10"));
            auto a = std::get<PreparedApplicationActions>(
                ports.prepare(inert_view, cmd, inputs, p, numeric));
            require(ports.calls ==
                            std::vector<std::string>{"access", "policy", "clock", "actions"} &&
                        ports.received_proposal &&
                        ports.received_proposal->timer_after.main_remaining_ms == 60 &&
                        a.jobs.size() == 1 && a.explanation == "synthetic actions",
                    "Direct fake DATA calls log order and retain complete owned numeric proposal");
            p.policy_revision_ref = "changed-local-copy";
            c.segments[0].elapsed_ms = 9;
            a.jobs[0].envelope.document[0] = 'X';
            require(std::get<BoundApplicationPolicy>(ports.bound_policy).policy_revision_ref ==
                            "policy" &&
                        std::get<ApplicationClockObservation>(ports.observed_clock)
                                .segments[0]
                                .elapsed_ms == 0 &&
                        std::get<PreparedApplicationActions>(ports.prepared_actions)
                            .jobs[0]
                            .envelope.document.startsWith("SAJ1"),
                    "Returned fake DATA owns copies and does not mutate the scripted source");
            const auto binding = ports.issued[0].binding;
            const auto issued = ports.issue(ApplicationReferencePurpose::advanced_state, binding);
            require(std::get<IssuedApplicationReference>(issued).reference == "b",
                    "Explicit independently bound reference DATA");
            auto wrong = binding;
            wrong.persisted_head_token = "other";
            require(std::holds_alternative<ApplicationUnavailable>(
                        ports.issue(ApplicationReferencePurpose::advanced_state, wrong)),
                    "Fake does not echo an unbound caller request into a trusted result");
        });
    }
    void actualOldWriterCompletionDistinguishesCommitRollbackAndTailFailure() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable completion foundation");
            const auto path = directory.filePath("completion.sqlite");
            SqliteWriter writer;
            open_sources(writer, path, data);
            Observer observer(path);
            const auto put = [](const char *ref) {
                return SqlStatement{
                    "INSERT INTO subathon_retained_configurations(reference,document) VALUES(?,?)",
                    {blob(ref), QByteArray("", 0)}};
            };
            int bodies = 0;
            const auto success = capture(writer, [&](auto context) -> QVariant {
                ++bodies;
                const auto session = context.begin();
                (void)session.execute(put("normal"));
                session.commit();
                return QByteArray("normal-owned-value");
            });
            require(success.transaction == TransactionOutcome::committed &&
                        success.error == WorkError::none && success.value &&
                        success.value->toByteArray() == "normal-owned-value" && bodies == 1,
                    "Actual one body/BEGIN/COMMIT and NORMAL completion retains typed publication "
                    "input");
            const auto rolled = capture(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                (void)session.execute(put("rolled"));
                session.rollback();
                return QByteArray("must-not-publish");
            });
            require(rolled.transaction == TransactionOutcome::rolled_back &&
                        rolled.error == WorkError::not_committed && !rolled.value,
                    "Known rollback completion carries no publishable body value");
            const auto tail = capture(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                (void)session.execute(put("tail"));
                session.commit();
                throw std::runtime_error("synthetic body tail after actual commit");
            });
            require(tail.transaction == TransactionOutcome::committed &&
                        tail.error == WorkError::work_exception && !tail.value,
                    "Actual committed body-tail exception is distinct from normal committed "
                    "completion");
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Old acceptance seam schedules loss AFTER real control SQL");
            const auto unknown = capture(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                (void)session.execute(put("unknown"));
                session.commit();
                return QByteArray("must-not-publish");
            });
            require(unknown.transaction == TransactionOutcome::outcome_unknown &&
                        unknown.error == WorkError::uncertain_outcome && !unknown.value,
                    "Actual lost commit acknowledgment is unknown and has no publishable value");
            require(observer.rows("SELECT reference,typeof(document),length(document) FROM "
                                  "subathon_retained_configurations WHERE reference<>X'636667' "
                                  "ORDER BY reference") ==
                        SqlRows{{blob("normal"), QString("blob"), qint64(0)},
                                {blob("tail"), QString("blob"), qint64(0)},
                                {blob("unknown"), QString("blob"), qint64(0)}},
                    "Independent actual persistence distinguishes rollback, tail and lost-ack "
                    "classification");
            catalog(observer, data);
        });
    }
};
QTEST_GUILESS_MAIN(ApplicationServiceEnvironment)
#include "application_service_environment.moc"
