// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/reference_intake_fixture.hpp"
#include <subathon/server/sqlite_application_plans.hpp>
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
    explicit IntakeRig(bool head = true, ReferenceRuntimeLimits limits = {})
        : file("standard", head),
          runtime(configuration, authority, sampler, entropy,
                  head ? std::optional<ReferenceClockAnchor>{{committed(), {100, 1000}}}
                       : std::nullopt,
                  limits),
          intake(configuration, sampler, entropy, file.db.clock, connector(), &runtime, limits) {}
};
template <class T> void no_data_on_error(const T &r) {
    require(r.work.error != WorkError::none && !r.work.value,
            "Actual writer error suppresses normal tentative value");
}
template <class T> void clean_refusal(const T &r, ReferenceGate gate, const char *reason) {
    require(r.work.transaction == TransactionOutcome::rolled_back &&
                r.work.error == WorkError::not_committed && !r.work.value && r.refusal &&
                r.refusal->gate == gate && r.refusal->reason_ref == reason,
            "Exact original not_committed rollback, diagnostic-only refusal, gate and reason");
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
                        a.sample.utc_ms == 1000 && a.committed.observation.segments.empty() &&
                        a.committed.observation.observed_utc_ms == a.sample.utc_ms &&
                        a.committed.policy_revision_ref == "W" &&
                        a.committed.observation.policy_revision_ref == "W",
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
        QTest::newRow("unexpired-but-not-fresh") << 5;
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
            auto prepared = r.intake.make_initial_work(input, out.callback());
            if (kind == 5)
                r.file.db.clock.now = 301001;
            out.run(r.file.db.writer, std::move(prepared));
            clean_refusal(*out.result, ReferenceGate::denied, "reference.access-denied.v1");
            require(!out.result->anchor && out.result->refusal &&
                        out.result->refusal->gate == ReferenceGate::denied,
                    "Fresh actual stored-session authority required; no owner boolean");
            require(r.file.count("subathon_app_states") == 0,
                    "Unauthorized startup writes no state");
        });
    }
    void initialFreshAuthenticationBoundaryAtActualRead() {
        scenario([] {
            IntakeRig r(false);
            auto input = initial_request();
            Completion<ReferenceInitialCompletion> out;
            auto prepared = r.intake.make_initial_work(input, out.callback());
            r.file.db.clock.now = 301000; // age changes AFTER preparation, BEFORE session read
            out.run(r.file.db.writer, std::move(prepared));
            normal(out.result->work);
            require(out.result->anchor.has_value(),
                    "authenticated1000 age300000 inclusive is fresh");
        });
    }
    void initialUnsupportedAndMalformedTimerProfiles_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("zero-main") << 0;
        QTest::newRow("grace") << 1;
        QTest::newRow("confirmation-wait") << 2;
        QTest::newRow("negative-main-invalid") << 3;
        QTest::newRow("invalid-lifecycle-tag") << 4;
    }
    void initialUnsupportedAndMalformedTimerProfiles() {
        QFETCH(int, kind);
        scenario([&] {
            IntakeRig r(false);
            auto input = initial_request();
            input.timer.main_remaining_ms = kind == 3 ? -1 : 0;
            if (kind == 1)
                input.timer.phase = CountdownPhase::Grace;
            if (kind == 2)
                input.timer.awaiting_confirmation = true;
            if (kind == 4)
                input.timer.lifecycle = static_cast<TimerLifecycle>(255);
            if (kind >= 3) {
                input_failure([&] { (void)r.intake.make_initial_work(input, [](auto) {}); },
                              kind == 3 ? ReferenceInputError::invalid_value
                                        : ReferenceInputError::invalid_tag);
            } else {
                Completion<ReferenceInitialCompletion> out;
                out.run(r.file.db.writer, r.intake.make_initial_work(input, out.callback()));
                clean_refusal(*out.result, ReferenceGate::not_ready,
                              "reference.runtime-unavailable.v1");
                require(!out.result->anchor, "No fresh anchor for unsupported timer profile");
            }
            require(r.file.count("subathon_app_states") == 0 && r.sampler.calls == 0 &&
                        r.entropy.calls == 0,
                    "Invalid initial state is side-effect-free before clock/entropy");
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
            clean_refusal(*out.result, ReferenceGate::conflict, "reference.initial-conflict.v1");
            require(r.file.count("subathon_app_states") == 1 &&
                        r.file.observer->rows("SELECT token FROM subathon_app_head")[0][0]
                                .toByteArray() == "a",
                    "Existing head preserved byte-for-byte");
        });
    }
    void initialUnavailableLiveClockIsNotZeroAnchor() {
        scenario([] {
            IntakeRig r(false);
            r.sampler.next = ReferenceUnavailable{ReferenceGate::not_ready, "reference.clock.v1"};
            Completion<ReferenceInitialCompletion> out;
            out.run(r.file.db.writer,
                    r.intake.make_initial_work(initial_request(), out.callback()));
            require(!out.result->anchor && out.result->refusal &&
                        r.file.count("subathon_app_states") == 0,
                    "No fabricated UTC/elapsed0 readiness");
            clean_refusal(*out.result, ReferenceGate::not_ready, "reference.clock.v1");
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
            const auto attempts = r.file.observer->rows(
                "SELECT modifier_context_ref,configuration_ref FROM subathon_receipt_attempts "
                "WHERE receipt_id=X'0000000000000007' ORDER BY ordinal DESC LIMIT 1");
            require(attempts.size() == 1 && attempts[0][1].toByteArray() == "D" &&
                        attempts[0][0].toByteArray().startsWith("RCT1") &&
                        attempts[0][0].toByteArray() != "X",
                    "Retry's own new attempt retains current D, distinct from original C/X");
            const auto retained = r.file.observer->rows(
                "SELECT c.document,c.configuration_ref FROM subathon_retained_contexts c JOIN "
                "subathon_receipt_attempts a "
                "ON c.reference=a.modifier_context_ref WHERE a.receipt_id=X'0000000000000007' "
                "ORDER BY a.ordinal DESC LIMIT 1");
            require(retained.size() == 1 && retained[0][0].toByteArray().startsWith("SRA1") &&
                        retained[0][1].toByteArray() == "D",
                    "Attempt points to its actual retained new arrival");
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
            r.file.db.work([](const auto &session) {
                SqliteApplicationPlanStore store(session);
                const auto old = store.find_revision("P", 1);
                require(old.has_value(), "SETUP: old captured revision1 exists");
                auto next = old->revision;
                next.revision = 2;
                next.parent_revision = 1;
                next.override_decision = PlanOverrideDecision{OverrideAuthority::authorized, "A",
                                                              "new history", "decision2"};
                require(store.append("P", 1, next) == SqlitePlanAppendDisposition::appended,
                        "SETUP: OLD plan store advances history to2");
            });
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
                        out.result->work.error == WorkError::work_exception &&
                        r.file.count("subathon_reference_initial_captures") == 1 &&
                        r.entropy.calls == 0,
                    "Actual selected corruption rollback, never replacement/retry");
            bool original = false;
            try {
                out.witness.rethrow();
            } catch (const ReferenceCaptureStorageFailure &e) {
                original = e.reason() == ReferenceCaptureStorageReason::invalid_stored_data;
            }
            require(original, "Exact original capture-storage invalid_stored_data, no relabel");
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
            clean_refusal(*out.result, ReferenceGate::denied, "reference.access-denied.v1");
        });
    }
    void snapshotOwnerNeedsOrdinaryUseAndLiveSession_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("owner-without-snapshot-USE") << 0;
        QTest::newRow("revoked-owner-session") << 1;
        QTest::newRow("expired-owner-session") << 2;
    }
    void snapshotOwnerNeedsOrdinaryUseAndLiveSession() {
        QFETCH(int, kind);
        scenario([&] {
            IntakeRig r;
            if (kind == 0)
                r.file.db.replace_graph(0, graph(false));
            if (kind == 1)
                r.file.db.work([](const auto &session) {
                    (void)session.execute(
                        {"UPDATE subathon_account_sessions SET state=1 WHERE account_ref=X'4f'",
                         {}});
                });
            if (kind == 2)
                r.file.db.clock.now = 86401000;
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('O'), out.callback()));
            clean_refusal(*out.result, ReferenceGate::denied, "reference.access-denied.v1");
            require(!out.result->snapshot && r.sampler.calls == 0 && r.entropy.calls == 0,
                    "Owner has no implicit ordinary snapshot grant or expired-session bypass");
        });
    }
    void snapshotUnexpiredNonfreshSessionStillHasOrdinaryRead() {
        scenario([] {
            IntakeRig r;
            r.file.db.clock.now = 301001;
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('O'), out.callback()));
            normal(out.result->work);
            require(out.result->snapshot.has_value(),
                    "Snapshot require_fresh=false; initialization differs");
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
            clean_refusal(*out.result, ReferenceGate::not_ready,
                          "reference.runtime-unavailable.v1");
        });
    }
    void receiptUnsupportedRuntimeRefusesBeforeNewDurableWork_data() {
        QTest::addColumn<QString>("profile");
        for (const auto *name :
             {"debt", "grace", "wait", "balances", "ready", "finished", "deadline"})
            QTest::newRow(name) << QString(name);
    }
    void receiptUnsupportedRuntimeRefusesBeforeNewDurableWork() {
        QFETCH(QString, profile);
        scenario([&] {
            const bool deadline = profile == "deadline";
            const auto name = deadline ? QByteArray("standard") : profile.toUtf8();
            File file(name.constData());
            auto v = versions();
            if (deadline)
                std::get<ReferenceConfigurationFacts>(v[1].contents)
                    .projection.timer_config.hard_end_utc_ms = 999999;
            if (profile == "wait")
                std::get<ReferenceConfigurationFacts>(v[1].contents)
                    .projection.timer_config.end_mode = TimerEndMode::Confirmation;
            ReferenceConfigurationArchive configuration(std::move(v));
            SqliteReferenceAuthoritySource authority;
            Sampler sampler;
            Entropy entropy;
            ReferenceApplicationAdapters runtime(
                configuration, authority, sampler, entropy,
                ReferenceClockAnchor{committed(name.constData()), {100, 1000}});
            ReferenceIntake intake(configuration, sampler, entropy, file.db.clock, connector(),
                                   &runtime);
            const auto attempts = file.count("subathon_receipt_attempts");
            Completion<ReferenceReceiptCompletion> out;
            out.run(file.db.writer,
                    intake.make_receipt_work("D", {"Q", "profile", {}, QByteArray("", 0)},
                                             out.callback()));
            clean_refusal(*out.result,
                          deadline ? ReferenceGate::unsupported_configuration
                                   : ReferenceGate::not_ready,
                          deadline ? "reference.configuration-unsupported.v1"
                                   : "reference.runtime-unavailable.v1");
            require(!out.result->receipt && !out.result->original &&
                        file.count("subathon_receipts") == 1 &&
                        file.count("subathon_receipt_attempts") == attempts && sampler.calls == 0 &&
                        entropy.calls == 0,
                    "Profile refuses before receipt/attempt/sample/entropy");
        });
    }
    void receiptConfigurationUnknownUnsupportedOrHeadMismatch_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("unknown-source") << 0;
        QTest::newRow("supported-shape-unsupported-profile") << 1;
        QTest::newRow("head-config-mismatch") << 2;
    }
    void receiptConfigurationUnknownUnsupportedOrHeadMismatch() {
        QFETCH(int, kind);
        scenario([&] {
            IntakeRig r;
            auto v = versions();
            auto &f = std::get<ReferenceConfigurationFacts>(v[1].contents);
            if (kind == 0) {
                f.generated_events.state = ReferenceFactsState::unknown;
                f.projection.generated_events = ReferenceFactsState::unknown;
            }
            if (kind == 1)
                f.outbound_actions.enabled_refs = {"enabled"};
            ReferenceConfigurationArchive config(std::move(v));
            ReferenceIntake intake(config, r.sampler, r.entropy, r.file.db.clock, connector(),
                                   &r.runtime);
            const auto attempts = r.file.count("subathon_receipt_attempts");
            Completion<ReferenceReceiptCompletion> out;
            out.run(r.file.db.writer,
                    intake.make_receipt_work(kind == 2 ? "C" : "D",
                                             {"Q", "config-refusal", {}, QByteArray("", 0)},
                                             out.callback()));
            clean_refusal(*out.result,
                          kind == 1 ? ReferenceGate::unsupported_configuration
                                    : ReferenceGate::not_ready,
                          kind == 0   ? "reference.configuration-unavailable.v1"
                          : kind == 1 ? "reference.configuration-unsupported.v1"
                                      : "reference.runtime-unavailable.v1");
            require(!out.result->receipt && !out.result->original &&
                        r.file.count("subathon_receipts") == 1 &&
                        r.file.count("subathon_receipt_attempts") == attempts &&
                        r.entropy.calls == 0 && r.sampler.calls == 0,
                    "Configuration profile/binding preflight before fresh receipt work");
        });
    }
    void preparationCapacityIsLocalAndAbandonedPairReleasesSlot_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("initial") << 0;
        QTest::newRow("receipt") << 1;
        QTest::newRow("evidence") << 2;
        QTest::newRow("capture") << 3;
        QTest::newRow("snapshot") << 4;
    }
    void preparationCapacityIsLocalAndAbandonedPairReleasesSlot() {
        QFETCH(int, kind);
        scenario([&] {
            ReferenceRuntimeLimits limits;
            limits.pending_reservations = 1;
            IntakeRig r(true, limits);
            auto make = [&] {
                switch (kind) {
                case 0:
                    return r.intake.make_initial_work(initial_request(), [](auto) {});
                case 1:
                    return r.intake.make_receipt_work("D", {"Q", "slot", {}, QByteArray("", 0)},
                                                      [](auto) {});
                case 2:
                    return r.intake.make_evidence_work({7}, [](auto) {});
                case 3:
                    return r.intake.make_capture_work({7}, {0}, [](auto) {});
                default:
                    return r.intake.make_snapshot_work(actor('O'), [](auto) {});
                }
            };
            auto first = make();
            auto copy = first;
            input_failure([&] { (void)make(); }, ReferenceInputError::input_limit);
            first = {};
            input_failure([&] { (void)make(); }, ReferenceInputError::input_limit);
            copy.body = {};
            input_failure([&] { (void)make(); }, ReferenceInputError::input_limit);
            copy.completion = {};
            auto next = make();
            require(
                bool(next.body) && bool(next.completion) && r.sampler.calls == 0 &&
                    r.entropy.calls == 0,
                "Only BOTH final closures release slot; make_* never predicts actual writer queue");
        });
    }
    void receiptExactReference4096FitsMatching4097RefusalField() {
        scenario([] {
            IntakeRig r;
            Completion<ReferenceReceiptCompletion> out;
            const std::string event(4096, 'e');
            out.run(r.file.db.writer,
                    r.intake.make_receipt_work("D", {"Q", event, {}, QByteArray("", 0)},
                                               out.callback()));
            normal(out.result->work);
            require(out.result->receipt && out.result->original &&
                        out.result->original->observation.event_id == event,
                    "Exact event_id reference ceiling4096 fits same field as4097 invalid input");
        });
    }
    void snapshotWholeCompletionExact12607ThenPostBegin12606RollsBack() {
        scenario([] {
            // Initial SAS1 does NOT encode its head token. A literal 3072-byte
            // token occurs in snapshot and derived observation, then both copies
            // occur in work.value and the output: 323 + 4*(3072-1) =12607.
            // Snapshot3206 + basis1 + observation3084 + policy1 =6292;
            // cap presence1+8 =>6301. Work2+(1+6301)+(1+6301)+1 =12607.
            // The complete graph/catalog/authorization inputs remain well below
            // 12606; a tiny general profile323 would refuse before this boundary.
            const std::string token(3072, 'a');
            auto expected = committed();
            expected.snapshot.token = token;
            expected.observation.checked_head_token = token; // absent initial basis
            auto check = [&](std::uint64_t bytes, bool fits) {
                ReferenceRuntimeLimits limits;
                limits.values.value_owned_bytes = bytes;
                IntakeRig r(false, limits);
                // Independent OLD stores/literals seed the head. No new loader,
                // initializer or encoder creates the completion's own oracle.
                r.file.db.work([&](const auto &s) {
                    const auto config = literals()["configurations"]
                                            .toObject()["standard"]
                                            .toObject()["document_hex"]
                                            .toString();
                    require(SqliteRetainedContextStore(s).retain(
                                {{RetainedKind::configuration, "D"},
                                 QByteArray::fromHex(config.toLatin1()),
                                 {}}) == RetainDisposition::inserted,
                            "SETUP: independent current SRC1 D");
                    require(SqliteApplicationBundleStore(s).retain_initial_state(
                                expected.snapshot) == SqliteBundleInitialDisposition::inserted,
                            "SETUP: independent initial SAS1 and long head token");
                });
                Completion<ReferenceSnapshotCompletion> out;
                out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('O'), out.callback()));
                if (fits) {
                    normal(out.result->work);
                    require(out.result->snapshot &&
                                out.result->snapshot->committed.snapshot.token == token &&
                                out.result->snapshot->committed.observation.checked_head_token ==
                                    token &&
                                out.result->snapshot->committed.snapshot.envelope.document ==
                                    expected.snapshot.envelope.document &&
                                out.result->snapshot->maximum_main_ms == 3600000 &&
                                out.result->work.value->canConvert<ReferenceSnapshot>() &&
                                out.result->work.value->value<ReferenceSnapshot>()
                                        .committed.snapshot.token == token,
                            "Exact12607 fits the COMPLETE completion, including both owned copies");
                } else {
                    clean_refusal(*out.result, ReferenceGate::not_ready,
                                  "reference.result-limit.v1");
                    require(!out.result->snapshot, "Local completion overage preserves original "
                                                   "writer truth, no partial snapshot");
                }
                require(r.file.count("subathon_app_states") == 1 &&
                            r.file.count("subathon_app_operations") == 0 && r.sampler.calls == 0 &&
                            r.entropy.calls == 0,
                        "Authorized committed read neither mutates accounting nor fabricates an "
                        "anchor");
            };
            check(12607, true);
            check(12606, false);
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
            clean_refusal(*out.result, ReferenceGate::conflict, "reference.collision.v1");
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
            clean_refusal(*out.result, ReferenceGate::conflict, "reference.collision.v1");
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
