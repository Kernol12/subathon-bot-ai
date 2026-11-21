// SPDX-License-Identifier: GPL-3.0-or-later
#include "receipt_fixture.hpp"
#include "transition_fixture.hpp"

#include <subathon/server/sqlite_reference_capture.hpp>

using namespace storage_026_fixture;

class Storage026ReceiptTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

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
            auto snapshot = [&] { return protected_data(f.db, existing > 1024); };
            const auto before = snapshot();
            const auto [result, work] = manage(f.db, input);
            if (result.stop == Stop::not_implemented)
                require(snapshot() == before,
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
                            snapshot() == before &&
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
            require(matches_sql_rows(
                        f.db.rows("SELECT intake_ref,operation_ref,page_ordinal,item_ordinal "
                                  "FROM main.subathon_intake_entries ORDER BY intake_ref"),
                        SqlRows{{QByteArray("H"), QByteArray("O"), 1, 1},
                                {QByteArray("L"), QVariant{}, QVariant{}, QVariant{}}}),
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

QTEST_GUILESS_MAIN(Storage026ReceiptTests)
#include "receipts_tests.moc"
