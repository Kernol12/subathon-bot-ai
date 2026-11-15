// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/reference_intake_fixture.hpp"
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
using namespace reference_intake_fixture;
namespace {
struct IntakeRig {
    File file;
    ReferenceConfigurationArchive configuration{versions()};
    Sampler sampler;
    Entropy entropy;
    SqliteReferenceAuthoritySource authority;
    ReferenceApplicationAdapters runtime;
    ReferenceIntake intake;
    explicit IntakeRig(bool head = true)
        : file("standard", head),
          runtime(configuration, authority, sampler, entropy,
                  head ? std::optional<ReferenceClockAnchor>{{committed(), {100, 1000}}}
                       : std::nullopt),
          intake(configuration, sampler, entropy, file.db.clock, connector(), &runtime) {}
};
template <class T> void no_data_on_error(const T &r) {
    require(r.work.error != WorkError::none && !r.work.value,
            "Actual writer error suppresses normal tentative value");
}
void no_application(File &f) {
    for (const auto *t : {"subathon_app_operations", "subathon_app_claims", "subathon_app_effects",
                          "subathon_app_results", "subathon_app_audit", "subathon_app_jobs"})
        require(f.count(t) == 0, "Intake units never write application/timer effects");
}
} // namespace
class ReferenceIntakeTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void initialOwnerUsesActualSessionAndExactFreshSas1() {
        scenario([] {
            IntakeRig r(false);
            Completion<ReferenceInitialCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_initial_work(initial_request(), out.callback()));
            normal(out.result->work);
            require(out.result->anchor && !out.result->refusal, "Fresh owner-only initialization");
            const auto &a = *out.result->anchor;
            require(a.committed.snapshot.envelope.document ==
                            committed("initial").snapshot.envelope.document &&
                        !a.committed.basis && a.sample.monotonic_ms == 100 &&
                        a.sample.utc_ms == 1000,
                    "Exact independent 68-byte SAS1 and separately sampled live anchor");
            const auto rows = r.file.observer->rows(
                "SELECT token,configuration_ref,main_remaining_ms,grace_remaining_ms,document "
                "FROM subathon_app_states");
            require(rows.size() == 1 && rows[0][0].toByteArray() == "a" &&
                        rows[0][1].toByteArray() == "C" && rows[0][2].toLongLong() == 3480000 &&
                        rows[0][3].toLongLong() == 120000 &&
                        rows[0][4].toByteArray() == committed("initial").snapshot.envelope.document,
                    "Physical state bytes independently observed after actual completion");
            no_application(r.file);
        });
    }
    void initialRejectsNonOwnerStaleRevokedDisabledSession_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("nonowner-real-session") << 0;
        QTest::newRow("revoked-owner-session") << 1;
        QTest::newRow("disabled-owner-account") << 2;
        QTest::newRow("expired-owner-session") << 3;
        QTest::newRow("unknown-digest") << 4;
    }
    void initialRejectsNonOwnerStaleRevokedDisabledSession() {
        QFETCH(int, kind);
        scenario([&] {
            IntakeRig r(false);
            auto input = initial_request();
            if (kind == 0)
                input.owner_session = actor('A');
            if (kind == 1)
                r.file.db.work([](const auto &s) {
                    (void)s.execute(
                        {"UPDATE subathon_account_sessions SET state=1 WHERE account_ref=X'4f'",
                         {}});
                });
            if (kind == 2)
                r.file.db.work([](const auto &s) {
                    (void)s.execute({"UPDATE subathon_accounts SET application_access_enabled=0 "
                                     "WHERE account_ref=X'4f'",
                                     {}});
                });
            if (kind == 3)
                r.file.db.clock.now = 86401000;
            if (kind == 4)
                input.owner_session.digest.fill(0xff);
            Completion<ReferenceInitialCompletion> out;
            out.run(r.file.db.writer, r.intake.make_initial_work(input, out.callback()));
            require(!out.result->anchor && out.result->refusal &&
                        out.result->refusal->gate == ReferenceGate::denied,
                    "Fresh actual stored-session authority required; no owner boolean");
            require(r.file.count("subathon_app_states") == 0,
                    "Unauthorized startup writes no state");
        });
    }
    void initialNeverResetsExistingHead() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceInitialCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_initial_work(initial_request(), out.callback()));
            require(!out.result->anchor && out.result->refusal &&
                        out.result->refusal->gate == ReferenceGate::conflict,
                    "Even owner cannot reset/reanchor existing head");
            require(r.file.count("subathon_app_states") == 1 &&
                        r.file.observer->rows("SELECT token FROM subathon_app_head")[0][0]
                                .toByteArray() == "a",
                    "Existing head preserved byte-for-byte");
        });
    }
    void initialUnavailableLiveClockIsNotZeroAnchor() {
        scenario([] {
            IntakeRig r(false);
            r.sampler.next = ReferenceUnavailable{ReferenceGate::not_ready, "test.clock-missing"};
            Completion<ReferenceInitialCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_initial_work(initial_request(), out.callback()));
            require(!out.result->anchor && out.result->refusal &&
                        r.file.count("subathon_app_states") == 0,
                    "No fabricated UTC/elapsed0 readiness");
        });
    }
    void receiptRetainsNormalizedSyntheticOriginalAndOwnsOneCommit() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_receipt_work(
                        "D", {"Q", "new-event", -2000, QByteArray("{\"award\":99999999}")},
                        out.callback()));
            normal(out.result->work);
            require(out.result->receipt && out.result->original && !out.result->refusal,
                    "Actual adopted receipt commit returns original");
            const auto rows = r.file.observer->rows(
                "SELECT event_type,schema_version,source_timestamp_utc_ms,server_timestamp_utc_ms,"
                "actor_id,money_original_decimal,payment_opaque_id,activity_opaque_id,"
                "configuration_ref "
                "FROM subathon_receipts WHERE opaque_id=X'6e65772d6576656e74'");
            require(
                rows.size() == 1 && rows[0][0].toByteArray() == "synthetic.reference.support.v1" &&
                    rows[0][1].toInt() == 1 && rows[0][2].toLongLong() == -2000 &&
                    rows[0][3].toLongLong() == 1000 && rows[0][4].isNull() && rows[0][5].isNull() &&
                    rows[0][6].isNull() && rows[0][7].isNull() && rows[0][8].toByteArray() == "D",
                "Exact synthetic facts, no provider proof/amount inference");
            require(out.result->original->configuration.rule.add_ms == 600000 &&
                        out.result->original->observation.original_payload ==
                            "{\"award\":99999999}",
                    "Audit payload cannot choose request");
            no_application(r.file);
        });
    }
    void receiptRetryUsesOriginalConfigAndNewAttemptOnly() {
        scenario([] {
            IntakeRig r;
            const auto before = r.file.count("subathon_receipt_attempts");
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_receipt_work(
                        "D", {"Q", "E", {}, QByteArray("changed audit bytes")}, out.callback()));
            normal(out.result->work);
            require(out.result->receipt && out.result->receipt->receipt_id.value == 7 &&
                        out.result->original &&
                        out.result->original->configuration.configuration_ref == "C" &&
                        out.result->original->arrival_context_ref == "X" &&
                        out.result->original->event_ref == "V" &&
                        r.file.count("subathon_receipts") == 1 &&
                        r.file.count("subathon_receipt_attempts") == before + 1,
                    "Retry retained attempt does not replace original arrival/r1");
            no_application(r.file);
        });
    }
    void receiptConflictingNormalizedFactsCannotMintContribution() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer, r.intake.make_receipt_work(
                                          "D", {"Q", "E", 123, QByteArray("", 0)}, out.callback()));
            normal(out.result->work);
            require(out.result->receipt &&
                        out.result->receipt->disposition == ReceiptDisposition::conflict &&
                        r.file.count("subathon_receipts") == 1 &&
                        r.file.count("subathon_contributions") == 1,
                    "Same source/different normalized UTC is original conflict, no fresh award");
            no_application(r.file);
        });
    }
    void receiptInvalidInputPrecedence_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("error");
        QTest::newRow("null-payload") << 0 << int(ReferenceInputError::invalid_shape);
        QTest::newRow("payload-65537") << 1 << int(ReferenceInputError::input_limit);
        QTest::newRow("event-ref-4097") << 2 << int(ReferenceInputError::input_limit);
        QTest::newRow("empty-config") << 3 << int(ReferenceInputError::invalid_reference);
        QTest::newRow("unregistered-connection") << 4 << int(ReferenceInputError::invalid_binding);
    }
    void receiptInvalidInputPrecedence() {
        QFETCH(int, kind);
        QFETCH(int, error);
        scenario([&] {
            IntakeRig r;
            ReferenceSyntheticInput in{"Q", "E", {}, QByteArray("", 0)};
            std::string config = "C";
            if (kind == 0)
                in.original_payload = {};
            if (kind == 1)
                in.original_payload = QByteArray(65537, 'x');
            if (kind == 2)
                in.event_id.assign(4097, 'e');
            if (kind == 3)
                config.clear();
            if (kind == 4)
                in.connection_ref = "foreign";
            input_failure([&] { (void)r.intake.make_receipt_work(config, in, [](auto) {}); },
                          static_cast<ReferenceInputError>(error));
            require(r.sampler.calls == 0 && r.entropy.calls == 0,
                    "Pure whole preflight before clocks/entropy/admission");
        });
    }
    void receiptExactPayloadLimitFits() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_receipt_work("D", {"Q", "bounded", {}, QByteArray(65536, 'p')},
                                               out.callback()));
            normal(out.result->work);
            require(out.result->original &&
                        out.result->original->observation.original_payload.size() == 65536,
                    "Exact payload ceiling, independent positive control");
        });
    }
    void evidenceRereadsOriginalReceiptAndPreservesRealCidZero() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceEvidenceCompletion> out;
            out.run(r.file.db.writer, r.intake.make_evidence_work({7}, out.callback()));
            normal(out.result->work);
            require(out.result->evidence && out.result->evidence->contribution_id &&
                        out.result->evidence->contribution_id->value == 0 &&
                        out.result->evidence->disposition ==
                            ContributionLinkDisposition::duplicate &&
                        r.file.count("subathon_contributions") == 1,
                    "Original support duplicated once, present CID0, not absent");
            no_application(r.file);
        });
    }
    void evidenceInvalidReceiptWinsBeforeWork() {
        scenario([] {
            IntakeRig r;
            input_failure([&] { (void)r.intake.make_evidence_work({0}, [](auto) {}); },
                          ReferenceInputError::invalid_value);
        });
    }
    void captureDiscoversOriginalBeforeNewRefsAndCurrentConfig() {
        scenario([] {
            IntakeRig r;
            const auto plans = r.file.count("subathon_plan_headers");
            Completion<ReferenceCaptureCompletion> out;
            out.run(r.file.db.writer, r.intake.make_capture_work({7}, {0}, out.callback()));
            normal(out.result->work);
            require(out.result->capture && out.result->capture->plan.plan_ref == "P" &&
                        out.result->capture->plan.revision == 1 &&
                        out.result->capture->execution_context_ref == "Y" &&
                        out.result->capture->configuration_ref == "C" && r.entropy.calls == 0 &&
                        r.file.count("subathon_plan_headers") == plans,
                    "Original indexed plan/r1, no new refs/orphan/operation-key dependence");
        });
    }
    void captureCorruptionNeverBecomesAbsence() {
        scenario([] {
            IntakeRig r;
            // Same key, physically well-typed known document with wrong frame.
            r.file.db.work([](const auto &s) {
                (void)s.execute(
                    {"UPDATE subathon_retained_contexts SET document=X'00' WHERE reference=X'59'",
                     {}});
            });
            Completion<ReferenceCaptureCompletion> out;
            out.run(r.file.db.writer, r.intake.make_capture_work({7}, {0}, out.callback()));
            no_data_on_error(*out.result);
            require(!out.result->capture &&
                        out.result->work.transaction == TransactionOutcome::rolled_back &&
                        r.file.count("subathon_reference_initial_captures") == 1 &&
                        r.entropy.calls == 0,
                    "Actual selected corruption rollback, never replacement/retry");
        });
    }
    void captureAbsentIndexStagesOneAtomicPlanSrxAndIndex() {
        scenario([] {
            IntakeRig r;
            r.file.db.work([](const auto &s) {
                for (const auto *sql :
                     {"DELETE FROM subathon_reference_initial_captures",
                      "DELETE FROM subathon_plan_requests", "DELETE FROM subathon_plan_rules",
                      "DELETE FROM subathon_plan_revisions", "DELETE FROM subathon_plan_headers",
                      "DELETE FROM subathon_retained_contexts WHERE reference=X'59'"})
                    (void)s.execute({sql, {}});
            });
            Completion<ReferenceCaptureCompletion> out;
            out.run(r.file.db.writer, r.intake.make_capture_work({7}, {0}, out.callback()));
            normal(out.result->work);
            require(out.result->capture && out.result->capture->contribution_id.value == 0 &&
                        out.result->capture->plan.revision == 1 &&
                        r.file.count("subathon_plan_headers") == 1 &&
                        r.file.count("subathon_reference_initial_captures") == 1,
                    "One committed initial plan/SRX/index unit, real CID0");
            no_application(r.file);
        });
    }
    void snapshotReadsCommittedCoherentConfigCapAndObservedUtc() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('O'), out.callback()));
            normal(out.result->work);
            require(out.result->snapshot && out.result->snapshot->maximum_main_ms == 3600000 &&
                        out.result->snapshot->committed.snapshot.token == "a" &&
                        out.result->snapshot->committed.snapshot.configuration_ref == "D" &&
                        out.result->snapshot->committed.observation.observed_utc_ms == 1000 &&
                        r.sampler.calls == 0,
                    "Read actual coherent head/config/SAS1, never substitute request UTC");
        });
    }
    void snapshotDoesNotDiscloseWithoutCurrentSnapshotUse() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('A'), out.callback()));
            require(!out.result->snapshot && out.result->refusal &&
                        out.result->refusal->gate == ReferenceGate::denied,
                    "Fresh actual snapshot authorization before protected lookup");
        });
    }
    void snapshotCommitAckLossCannotPublishTentativeRead() {
        scenario([] {
            IntakeRig r;
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        r.file.db.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Existing real COMMIT acknowledgement fault");
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('O'), out.callback()));
            require(out.result->work.transaction == TransactionOutcome::outcome_unknown &&
                        out.result->work.error == WorkError::uncertain_outcome &&
                        !out.result->snapshot,
                    "Unknown read completion suppresses tentative snapshot");
        });
    }
    void snapshotAcknowledgedCommitBodyThrowCannotPublish() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceSnapshotCompletion> out;
            auto p = r.intake.make_snapshot_work(actor('O'), out.callback());
            auto body = std::move(p.body);
            bool reached = false;
            p.body = [&, body](auto context) -> QVariant {
                (void)body(context);
                reached = true;
                throw std::runtime_error("after acknowledged COMMIT before normal return");
            };
            out.run(r.file.db.writer, std::move(p));
            require(reached && out.result->work.transaction == TransactionOutcome::committed &&
                        out.result->work.error == WorkError::work_exception &&
                        !out.result->snapshot,
                    "Committed body failure is not a normally published snapshot");
        });
    }
    void missingLiveAnchorOnReopenedBytesRefusesFreshReceipt() {
        scenario([] {
            IntakeRig r;
            ReferenceIntake unanchored(r.configuration, r.sampler, r.entropy, r.file.db.clock,
                                       connector());
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer,
                    unanchored.make_receipt_work("D", {"Q", "reopened", {}, QByteArray("", 0)},
                                                 out.callback()));
            require(!out.result->receipt && out.result->refusal &&
                        out.result->refusal->gate == ReferenceGate::not_ready &&
                        r.file.count("subathon_receipts") == 1 && r.entropy.calls == 0,
                    "Raw SAS1/current config cannot recreate this process's live clock evidence");
        });
    }
    void captureLateIndexSqlFaultRollsBackAllStagingButPreservesIntake() {
        scenario([] {
            IntakeRig r;
            r.file.db.work([](const auto &s) {
                for (const auto *sql :
                     {"DELETE FROM subathon_reference_initial_captures",
                      "DELETE FROM subathon_plan_requests", "DELETE FROM subathon_plan_rules",
                      "DELETE FROM subathon_plan_revisions", "DELETE FROM subathon_plan_headers",
                      "DELETE FROM subathon_retained_contexts WHERE reference=X'59'"})
                    (void)s.execute({sql, {}});
                (void)s.execute(
                    {"CREATE TRIGGER api011_capture_fault BEFORE INSERT ON "
                     "subathon_reference_initial_captures "
                     "WHEN EXISTS(SELECT 1 FROM subathon_plan_requests) "
                     "AND EXISTS(SELECT 1 FROM subathon_retained_contexts WHERE "
                     "substr(document,1,4)=X'53525831') "
                     "BEGIN SELECT RAISE(ABORT,'late capture index staging fault'); END",
                     {}});
            });
            Completion<ReferenceCaptureCompletion> out;
            out.run(r.file.db.writer, r.intake.make_capture_work({7}, {0}, out.callback()));
            require(!out.result->capture &&
                        out.result->work.transaction == TransactionOutcome::rolled_back &&
                        out.result->work.error == WorkError::sql_failure &&
                        r.file.count("subathon_plan_headers") == 0 &&
                        r.file.count("subathon_plan_requests") == 0 &&
                        r.file.count("subathon_reference_initial_captures") == 0 &&
                        r.file.count("subathon_receipts") == 1 &&
                        r.file.count("subathon_contributions") == 1 &&
                        r.file.observer
                                ->rows("SELECT count(*) FROM subathon_retained_contexts WHERE "
                                       "substr(document,1,4)=X'53525831'")[0][0]
                                .toInt() == 0,
                    "IC01 original SQL failure, ALL plan/SRX/index rolled back, earlier intake "
                    "durable");
        });
    }
    void existingCaptureMismatchedCidIsConflictBeforeAllocation() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceCaptureCompletion> out;
            out.run(r.file.db.writer, r.intake.make_capture_work({7}, {1}, out.callback()));
            require(
                !out.result->capture && out.result->refusal &&
                    out.result->refusal->gate == ReferenceGate::conflict && r.entropy.calls == 0 &&
                    r.file.count("subathon_reference_initial_captures") == 1 &&
                    r.file.count("subathon_plan_headers") == 1,
                "IC02 first source identity wins, no competing committed orphan/new capability");
        });
    }
    void actualContextCollisionRefusesAndCleansWithoutRetry() {
        scenario([] {
            IntakeRig r;
            r.entropy.advance = false;
            r.entropy.next.bytes.fill(0x01);
            r.file.db.work([](const auto &session) {
                const auto key = QByteArray("RCT1", 4) + QByteArray(32, char(1));
                require(SqliteRetainedContextStore(session).retain(
                            {{RetainedKind::arrival_context, key.toStdString()},
                             QByteArray("occupied context"),
                             ConfigurationReference{"D"}}) == RetainDisposition::inserted,
                        "SETUP: actual retained-context namespace conflict target");
            });
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_receipt_work("D", {"Q", "collision", {}, QByteArray("", 0)},
                                               out.callback()));
            require(!out.result->receipt && out.result->refusal &&
                        out.result->refusal->gate == ReferenceGate::conflict &&
                        r.entropy.calls == 1 && r.file.count("subathon_receipts") == 1,
                    "Actual occupied context key, whole cleanup/no fallback or extra draw");
        });
    }
    void snapshotAuthorizedNonOwnerUsesActualOrdinaryTimerReadPolicy() {
        scenario([] {
            IntakeRig r;
            auto g = graph(false);
            for (auto &a : g.accounts)
                if (a.account == "A") {
                    PermissionPolicyEntry e;
                    e.source = "snapshot-fixture";
                    e.grant = "snapshot-use";
                    e.operation = "timer.snapshot.read";
                    e.resources = {{"timer", PermissionResourceMode::allowed_set, {"T"}}};
                    a.individual_entries.push_back(e);
                }
            r.file.db.replace_graph(0, g);
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('A'), out.callback()));
            normal(out.result->work);
            require(out.result->snapshot && out.result->snapshot->committed.snapshot.token == "a",
                    "Actual snapshot USE works independently of ownership or execute permit");
        });
    }
};
QTEST_GUILESS_MAIN(ReferenceIntakeTests)
#include "reference_intake_tests.moc"
