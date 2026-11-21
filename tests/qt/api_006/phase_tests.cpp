// SPDX-License-Identifier: GPL-3.0-or-later
#include "boundary_fixture.hpp"

using namespace api_006_fixture;

namespace {
using S = intake::IntakeStatus;
using P = intake::Phase;

struct Pair {
    intake::Item first, peer;
    qint64 first_ms{8000}, peer_ms{9000};

    std::optional<intake::PageSlot> slot(const intake::Item &input) const {
        return input.key == "H" ? std::optional{intake::PageSlot{"O", 1, 1}} : std::nullopt;
    }

    void receipt(Fixture &fixture, const intake::Item &input, qint64 arrival, qint64 id) const {
        fixture.arrival = arrival;
        const auto result = fixture.phase(input.key, P::receipt, input, slot(input));
        require(Fixture::result(result, S::committed,
                                "Product receipt phase acknowledges the adopted COMMIT")
                        .attempt == intake::AttemptLocator{{static_cast<std::uint64_t>(id)}, 1},
                "Product returns the actually allocated receipt and its first attempt");
        storage::finance::normal(result.work);
    }
};

void resume_check(Fixture &f, QByteArray key, std::uint64_t receipt,
                  std::optional<core::ContributionId> cid) {
    const auto closure =
        Fixture::result(f.phase(key, P::reconcile), S::exact_replay,
                        "Product reconciles exact selected key on recovered writer");
    require(closure.attempt == intake::AttemptLocator{{receipt}, 1} && closure.contribution == cid,
            "Readback binds the exact attempt and optional committed CID");
}

void assert_original(Fixture &f, const Pair &pair, bool linked) {
    const auto rule_suffix = pair.first.key;
    require(f.store.db.rows(
                "SELECT c.contribution_id,c.original_receipt_id,r.server_timestamp_utc_ms,"
                "c.resolution_ref,c.review,c.application FROM main.subathon_contributions c "
                "JOIN main.subathon_receipts r ON r.receipt_id=c.original_receipt_id") ==
                SqlRows{{blob8(0), blob8(1), 8000,
                         rule_suffix == "L" ? QByteArray("VL") : QByteArray("VH"), 0, 0}},
            "One original preserves the first receipt, arrival, valuation and pending Review");
    require(f.store.db.rows("SELECT receipt_id,contribution_id FROM "
                            "main.subathon_contribution_observations ORDER BY receipt_id") ==
                (linked ? SqlRows{{blob8(1), blob8(0)}, {blob8(2), blob8(0)}}
                        : SqlRows{{blob8(1), blob8(0)}}),
            "Owning evidence records exactly the observations expected at this boundary");
    f.no_award();
}
} // namespace

class Api006PhaseTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void firstEligibleReceiptOwnsEvidence_data() {
        QTest::addColumn<bool>("historyFirst");
        QTest::addColumn<int>("interruption");
        for (const auto history : {false, true}) {
            const QByteArray order = history ? "history-" : "live-";
            QTest::newRow(order + "ordinary") << history << 0;
            QTest::newRow(order + "peer-first") << history << 1;
            QTest::newRow(order + "restart-before-peer") << history << 2;
            QTest::newRow(order + "lost-receipt-ack") << history << 3;
            QTest::newRow(order + "lost-evidence-ack") << history << 4;
        }
    }

    void firstEligibleReceiptOwnsEvidence() {
        QFETCH(bool, historyFirst);
        QFETCH(int, interruption);
        check([&] {
            Fixture f(true);
            const Pair pair{item(historyFirst ? "H" : "L"), item(historyFirst ? "L" : "H")};
            const auto initial = f.cursors();
            const auto prior = storage::protected_data(f.store.db);
            require(initial == SqlRows{{1, QByteArray("T"), QByteArray("11"), 0}},
                    "The healthy source and open gap have a durable checkpoint at 11");
            if (interruption == 3) {
                f.arm_loss();
                const auto response =
                    f.phase(pair.first.key, P::receipt, pair.first, pair.slot(pair.first));
                if (response.result && response.result->status == S::not_implemented)
                    require(storage::protected_data(f.store.db) == prior && f.cursors() == initial,
                            "The explicit Stage 1 stop preserves earlier owned rows and progress");
                require(!response.result, "Product receipt must reach the targeted actual COMMIT");
                f.stopped(response.work);
            } else {
                pair.receipt(f, pair.first, pair.first_ms, 1);
            }
            if (interruption == 2 || interruption == 3)
                f.store.db.reopen(work_profile);
            if (interruption == 3)
                resume_check(f, pair.first.key, 1, {});

            const auto basis = [&] {
                return f.store.db.rows("SELECT intake_ref,receipt_id,attempt_ordinal,capture "
                                       "FROM main.subathon_intake_basis");
            };
            const auto bound = basis();
            require(bound.size() == 1 &&
                        bound.front().mid(0, 3) == SqlRow{pair.first.key, blob8(1), 1} &&
                        !bound.front()[3].toByteArray().isEmpty(),
                    "First eligible receipt COMMIT atomically binds its own capture and attempt");
            if (interruption == 0) {
                const auto committed = f.phase(pair.first.key, P::evidence);
                Fixture::result(committed, S::pending_review,
                                "First evidence creates pending Review");
                storage::finance::normal(committed.work);
            }

            pair.receipt(f, pair.peer, pair.peer_ms, 2);
            require(
                f.attempts() ==
                        SqlRows{{blob8(1), 1, 8000, QByteArray("arrival-") + pair.first.key},
                                {blob8(2), 1, 9000, QByteArray("arrival-") + pair.peer.key}} &&
                    basis() == bound && f.cursors() == initial,
                "Independent observer sees both attaches and no replacement basis or cursor move");
            if (interruption != 0) {
                const auto early = f.phase(pair.peer.key, P::evidence);
                Fixture::result(early, S::pending_original,
                                "Peer evidence waits for selected first");
                require(f.store.db.rows("SELECT count(*) FROM subathon_contributions") ==
                                SqlRows{{0}} &&
                            f.store.db.rows(
                                "SELECT count(*) FROM subathon_contribution_observations") ==
                                SqlRows{{0}},
                        "Pending peer adds no ACCT-002 observation or original");
                f.no_award();
                f.arrival = 99000; // Resume must not recapture the elapsed arrival.
                if (interruption == 4) {
                    f.arm_loss();
                    const auto stopped = f.phase(pair.first.key, P::evidence);
                    require(!stopped.result, "Unknown evidence COMMIT returns no typed evidence");
                    f.stopped(stopped.work);
                    f.store.db.reopen(work_profile);
                    assert_original(f, pair, false);
                    resume_check(f, pair.first.key, 1, core::ContributionId{0});
                } else {
                    const auto evidence = f.phase(pair.first.key, P::evidence);
                    Fixture::result(evidence, S::pending_review, "Bound original retains Review");
                    storage::finance::normal(evidence.work);
                }
            }
            const auto peer = f.phase(pair.peer.key, P::evidence);
            require(
                Fixture::result(peer, S::linked, "Peer links through existing ACCT-002 original")
                        .contribution == core::ContributionId{0},
                "Both paths select CID zero allocated by real ACCT-002");
            storage::finance::normal(peer.work);
            assert_original(f, pair, true);
            for (const auto &input : {pair.first, pair.peer})
                require(Fixture::result(f.phase(input.key, P::evidence), S::exact_replay,
                                        "Exact evidence replay reuses original")
                                .contribution == core::ContributionId{0},
                        "Exact replay preserves CID");
            require(basis() == bound, "Evidence replay never alters first admitted basis");

            const auto capture = f.phase(pair.first.key, P::capture);
            Fixture::result(capture, S::pending_review,
                            "Chosen original retains one plan for Review");
            storage::finance::normal(capture.work);
            require(f.store.db.rows("SELECT requested_units FROM main.subathon_plan_requests") ==
                            SqlRows{{historyFirst ? 120000 : 60000}} &&
                        f.store.db.rows("SELECT version_ref FROM main.subathon_plan_rules") ==
                            SqlRows{{QByteArray("rule-") + pair.first.key}} &&
                        f.store.db.rows(
                            "SELECT instance_revision_ref FROM main.subathon_plan_modifiers") ==
                            SqlRows{{QByteArray("modifier-") + pair.first.key}},
                    "Captured plan uses the bound side's request, rule and modifier at original "
                    "arrival");

            auto review = f.store.service();
            ReviewEntryIntent selected;
            selected.selector = {{0}, {1}};
            selected.milestones = std::vector<MilestoneAcceptanceChoice>{};
            const std::vector rows{selected};
            const auto preview = f.store.preview(review, rows);
            const auto accept = f.store.bind("accept-selected", rows, preview);
            const auto applied = f.store.execute(review, accept);
            const auto &reply = f.store.reply(applied);
            require(!reply.stop && reply.rows.size() == 1 && reply.rows.front().outcome &&
                        reply.rows.front().outcome->status == ReviewStatus::applied,
                    "Current authorized Review uses the actual selected capture");
            (void)f.store.execute(review, accept);
            require(f.store.db.rows("SELECT count(*) FROM main.subathon_app_claims") ==
                        SqlRows{{1}},
                    "Authorized exact retry leaves one claim");
            f.store.head(100000, 0, 90);
        });
    }

    void evidenceRollbackRetainsPagePrefix() {
        check([] {
            Fixture f(true);
            const auto h = item("H");
            const Pair pair{h, item()};
            pair.receipt(f, h, 8000, 1);
            const auto before = storage::protected_data(f.store.db);
            const auto cursors = f.cursors();
            const auto page_before = f.store.db.rows(
                "SELECT item_count,committed_prefix,outcome FROM main.subathon_intake_pages");
            bool veto_reached = false;
            const auto failed = f.phase("H", P::evidence, {}, {}, [&] {
                veto_reached = true;
                return false;
            });
            require(veto_reached && !failed.work.value &&
                        failed.work.transaction == TransactionOutcome::rolled_back,
                    "Real owning evidence unit reaches the pre-COMMIT veto and rolls back");
            f.store.db.reopen(work_profile);
            require(
                storage::protected_data(f.store.db) == before && f.cursors() == cursors &&
                    page_before == SqlRows{{1, 0, 0}} &&
                    f.store.db.rows("SELECT item_count,committed_prefix,outcome FROM "
                                    "main.subathon_intake_pages") == page_before,
                "Known evidence rollback leaves receipt and page binding but no coverage advance");
            resume_check(f, "H", 1, {});
            f.arrival = 99000;
            const auto resumed = f.phase("H", P::evidence);
            Fixture::result(resumed, S::pending_review, "Resumed evidence uses original arrival");
            storage::finance::normal(resumed.work);
            assert_original(f, pair, false);
            require(f.cursors() == cursors, "Evidence COMMIT alone does not finalize the page");
        });
    }

    void mirrorCannotSupplyAwardSubject_data() {
        QTest::addColumn<bool>("application");
        QTest::newRow("capture") << false;
        QTest::newRow("application") << true;
    }

    void mirrorCannotSupplyAwardSubject() {
        QFETCH(bool, application);
        check([&] {
            Fixture f(true, false);
            seed_mirror_policy(f);
            auto input = storage::managed("mirror", 8000, "mirror-audit");
            input.admission = storage::ManagedInput::Admission::audit_only;
            const auto [receipt, committed] = storage::manage(f.store.db, input);
            storage::finance::normal(committed);
            require(
                receipt.stop == storage::Stop::committed &&
                    receipt.attempt == intake::AttemptLocator{{1}, 1},
                "Real ACCT-001 COMMIT supplies the identified mirror's own receipt and attempt");
            const auto managed = storage::managed_input(input);
            require(storage::transition(f.store.db,
                                        [&](const auto &session) {
                                            return storage::history_result(
                                                intake::stage_intake_disposition(
                                                    session, managed, *receipt.attempt,
                                                    intake::IntakeDisposition::observed_only));
                                        }) == storage::Stop::committed,
                    "The v20 owner commits the mirror's terminal audit disposition");
            require(storage::matches_sql_rows(
                        f.store.db.rows(
                            "SELECT receipt_id,attempt_ordinal,phase,disposition,contribution_id "
                            "FROM main.subathon_intake_entries WHERE intake_ref=X'6d6972726f72'"),
                        SqlRows{{blob8(1), 1, 2, 3, QVariant{}}}),
                    "Fixture retains a receipt and explicit audit classification with no "
                    "fabricated CID");

            const auto before = storage::protected_data(f.store.db);
            const auto intake_before = storage::fenced_history(f.store.db);
            const auto reply = f.phase("mirror", application ? P::application : P::capture);
            Fixture::result(
                reply, S::held,
                "Authorized award phase must stop on the retained mirror classification");
            require(
                !reply.result->contribution && !reply.result->projection &&
                    storage::protected_data(f.store.db) == before &&
                    storage::fenced_history(f.store.db) == intake_before &&
                    f.store.db
                        .rows("SELECT subject_kind,receipt_id FROM main.subathon_app_operations "
                              "WHERE receipt_id=?",
                              {blob8(1)})
                        .isEmpty() &&
                    f.store.db.rows("SELECT count(*) FROM main.subathon_plan_revisions") ==
                        SqlRows{{0}},
                "Mirror audit supplies neither a plan nor a committed command of either subject "
                "kind");
            f.no_award();
        });
    }

    void productMirrorCannotSupplyAwardSubject_data() {
        QTest::addColumn<bool>("application");
        QTest::newRow("capture") << false;
        QTest::newRow("application") << true;
    }

    void productMirrorCannotSupplyAwardSubject() {
        QFETCH(bool, application);
        check([&] {
            Fixture f;
            auto mirror = source.scope;
            mirror.account = "B";
            auto policy = baseline();
            policy.members.append({mirror, "individual", "main", "overlap", false, "mirror"});
            require(f.submit(policy, {source.scope, {0, 0, 0, "g3"}, source.profile}).status ==
                        S::committed,
                    "First connect configures the primary and mirror without a recovery gap");
            const QVariantList scope{mirror.installation, mirror.provider, mirror.account,
                                     mirror.stream};
            const auto current = f.store.db.rows(
                "SELECT epoch,revision,policy_revision,generation "
                "FROM main.subathon_intake_sources "
                "WHERE installation=? AND provider=? AND account=? AND stream=?", scope);
            require(current.size() == 1 && current.front().size() == 4 &&
                        f.store.db.rows("SELECT count(*) FROM main.subathon_intake_gaps") ==
                            SqlRows{{0}},
                    "Select the committed mirror source outside gap recovery");
            const auto &fence = current.front();
            const intake::Selection selection{
                mirror,
                {fence[0].toLongLong(), fence[1].toLongLong(), fence[2].toLongLong(),
                 fence[3].toByteArray()},
                source.profile};
            auto input = item("product-mirror", "12");
            input.activity.reset();
            input.disjointness_proof = "proved-partition";
            const auto submitted = f.submit(input, selection);
            require(submitted.status == S::held && !submitted.projection &&
                        !submitted.contribution,
                    "Normal product intake retains the mirror as identified audit only");
            const auto selected = f.store.db.rows(
                "SELECT intake_ref,receipt_id,attempt_ordinal,contribution_id "
                "FROM main.subathon_intake_entries WHERE installation=? AND provider=? "
                "AND account=? AND stream=? AND intake_ref=?",
                QVariantList(scope) << input.key);
            require(storage::matches_sql_rows(selected,
                                              SqlRows{{input.key, blob8(1), 1, QVariant{}}}),
                    "The product-retained mirror selects its exact receipt and attempt, no CID");

            const auto before = storage::protected_data(f.store.db);
            const auto history = storage::fenced_history(f.store.db);
            const auto reply = f.phase(selected.front()[0].toByteArray(),
                                       application ? P::application : P::capture,
                                       {}, {}, {}, selection);
            Fixture::result(reply, S::held,
                            "Existing award phase holds the normal product-retained mirror");
            require(!reply.result->projection && !reply.result->contribution &&
                        storage::protected_data(f.store.db) == before &&
                        storage::fenced_history(f.store.db) == history &&
                        f.store.db.rows("SELECT subject_kind,receipt_id "
                                        "FROM main.subathon_app_operations WHERE receipt_id=?",
                                        {selected.front()[1]}).isEmpty() &&
                        f.store.db.rows("SELECT count(*) FROM main.subathon_plan_revisions") ==
                            SqlRows{{0}},
                    "The product-retained audit entry creates no plan or command of either "
                    "award subject kind");
            f.no_award();
        });
    }

    void priorReadersRetainOriginalAcrossV20() {
        check([] {
            storage::Populated store;
            store.install();
            auto selected = store.seed("protected-review", {});
            const auto before = storage::protected_data(store.db);
            auto v19 = store.service();
            const auto prior = store.preview(v19, {selected});
            require(prior.size() == 1 && prior.front().facts && prior.front().facts->may_decide,
                    "Owning v19 Review selects the real original");
            storage::advance(store.db);
            const auto committed = storage::protected_data(store.db);
            auto v20 = store.service();
            const auto after = store.preview(v20, {selected});
            require(committed == before && after.size() == 1 && after.front().facts &&
                        after.front().facts->selector == prior.front().facts->selector &&
                        after.front().facts->basis == prior.front().facts->basis &&
                        after.front().facts->blockers == prior.front().facts->blockers,
                    "v20 preserves exact selected v19 review and its unrelated durable closure");
        });
    }
};

QTEST_GUILESS_MAIN(Api006PhaseTests)
#include "phase_tests.moc"
