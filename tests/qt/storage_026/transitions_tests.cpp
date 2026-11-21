// SPDX-License-Identifier: GPL-3.0-or-later
#include "receipt_fixture.hpp"
#include "transition_fixture.hpp"

namespace {
using namespace storage_026_fixture;

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

class Storage026TransitionTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

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
                        matches_sql_rows(
                            page_before[1],
                            SqlRows{{QByteArray("O"), 2, 1, QByteArray("T"), 0, 0},
                                    {QByteArray("adoption"), 1, 1, QVariant{}, 0, 1},
                                    {QByteArray("baseline"), 1, 1, QVariant{}, 0, 1}}) &&
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
                        matches_sql_rows(selected_source_history(f.db), prior_history) &&
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
                require(matches_sql_rows(selected_source_history(f.db), expected_history) &&
                            durable() == after && protected_data(f.db) == protected_before &&
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
                matches_sql_rows(
                    selected_source_history(f.db),
                    SqlRows{{QByteArray("history"), int(purpose), 2, 2, QByteArray("g4"),
                             source_history(prior, selected, "history-v2", "current-decision",
                                            proof),
                             QVariant{}, QVariant{}, 1, 0, int(HistoryOutcome::completed),
                             QVariant{}, QByteArray("new-cut")}}) &&
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
                            matches_sql_rows(
                                f.db.rows("SELECT installation,provider,account,stream,epoch,"
                                          "policy_revision,purpose,gap_ref,generation "
                                          "FROM main.subathon_intake_operations"),
                                SqlRows{{QByteArray("install"), QByteArray("P"), QByteArray("A"),
                                         QByteArray("S"), 1, 1, int(Purpose::import), QVariant{},
                                         QByteArray("g3")}}),
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
                                                           [](auto) {}) == StorageError::shutting_down,
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
            require(matches_sql_rows(
                        f.db.rows("SELECT proof FROM main.subathon_intake_pages WHERE ordinal=1"),
                        SqlRows{{page.qualified_proof.isNull() ? QVariant{}
                                                              : QVariant{page.qualified_proof}}}),
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
};

QTEST_GUILESS_MAIN(Storage026TransitionTests)
#include "transitions_tests.moc"
