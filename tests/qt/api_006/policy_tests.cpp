// SPDX-License-Identifier: GPL-3.0-or-later
#include "intake_fixture.hpp"

using namespace api_006_fixture;

namespace {
using S = intake::IntakeStatus;
using M = intake::CatchUpMode;

const intake::Selection unopened{source.scope, {0, 0, 0, "g3"}, "history-v1"};

void expect(const intake::IntakeResult &result, S expected, const char *message) {
    require(result.status == expected, message);
}

intake::Selection live_source(Fixture &f, const intake::Scope &scope = source.scope) {
    const auto rows = f.store.db.rows(
        "SELECT epoch,revision,policy_revision,generation FROM main.subathon_intake_sources "
        "WHERE installation=? AND provider=? AND account=? AND stream=?",
        {scope.installation, scope.provider, scope.account, scope.stream});
    require(rows.size() == 1 && rows.front().size() == 4, "Read exact committed source selection");
    const auto &row = rows.front();
    return {scope,
            {row[0].toLongLong(), row[1].toLongLong(), row[2].toLongLong(), row[3].toByteArray()},
            source.profile};
}

void configure(Fixture &f, M mode = M::review) {
    expect(f.submit(baseline(mode), unopened), S::committed,
           "Qualified first-connect cut commits on a fresh installed store");
    require(f.history.cuts == 1 &&
                f.store.db.rows("SELECT baseline_state FROM main.subathon_intake_sources") ==
                    SqlRows{{1}},
            "Adapter-supplied baseline has one committed qualified source");
}

intake::Page gap(Fixture &f, M mode, bool with_prior_award = false) {
    f.history.upper = with_prior_award ? "10" : "11";
    configure(f, mode);
    if (with_prior_award) {
        f.requested_ms = 5000;
        auto already_applied = item("11", "11");
        already_applied.activity->identity.opaque_id = "already-applied";
        expect(f.submit(already_applied, live_source(f)), S::applied,
               "Old authoritative scope has a committed award before the gap");
        f.store.head(85000, 0, 90);
    }
    expect(f.submit(intake::OpenGap{"G", 7000}, live_source(f)), S::committed,
           "Gap commits at last contiguous live point");
    f.history.upper = "14";
    expect(f.submit(
               intake::BeginHistory{
                   "O", intake::HistoryPurpose::catch_up, QByteArray("G"), {}, {}, {}},
               live_source(f)),
           S::committed, "Separately qualified history can recover an open replay gap");
    auto first_page = page();
    first_page.request.source = live_source(f);
    first_page.request.continuation.reset();
    return first_page;
}

auto disclosed(Fixture &f) {
    return f.store.db.rows(
        "SELECT original_receipt_id,payment_evidence,review,missing_rate,"
        "evidence_policy FROM main.subathon_contributions ORDER BY contribution_id");
}

void assert_time_award(Fixture &f) {
    require(f.store.db.rows("SELECT requested_units,maximum_main_ms "
                            "FROM main.subathon_app_effects WHERE unit=0") ==
                    SqlRows{{120000, 100000}} &&
                f.store.db.rows("SELECT component_kind,units FROM main.subathon_app_components "
                                "WHERE component_kind IN(0,5) ORDER BY component_kind") ==
                    SqlRows{{0, 20000}, {5, 100000}},
            "Arrival requests 120s, actual Main credit is 20s and cap reduction is 100s");
}
} // namespace

class Api006PolicyTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void firstConnectAndExplicitImport() {
        check([] {
            Fixture f;
            f.history.available = false;
            expect(
                f.submit(baseline(), unopened), S::unproved,
                "Unknown capability leaves default baseline pending, even with current authority");
            require(f.history.cuts == 1 && f.attempts().isEmpty(),
                    "An unproved cut cannot fabricate an anchor");
            f.history.available = true;
            expect(f.submit(baseline(M::automatic), unopened), S::committed,
                   "Qualified baseline commits through position 10 inclusive");
            f.store.db.reopen(work_profile);
            auto old = item("past", "9");
            old.facts.source_timestamp_utc_ms = 1795219200000LL;
            old.activity->identity.opaque_id = "past-activity";
            expect(f.submit(old, live_source(f)), S::baseline_only,
                   "Past observation stays baseline-only on the default route");
            require(f.attempts().isEmpty() && disclosed(f).isEmpty(),
                    "Baseline-only audit anchors no receipt or contribution");
            f.no_award();

            auto earliest = item("new", "11");
            earliest.facts.source_timestamp_utc_ms = 1795219200100LL;
            earliest.activity->identity.opaque_id = "new-activity";
            expect(f.submit(earliest, live_source(f)), S::committed,
                   "Earliest proved new item uses ordinary live intake after reopen");
            const auto entry = history_import({2026, 11, 21});
            f.history.upper = "11";
            constexpr qint64 start = 1795219200000LL;
            const auto opened = f.submit(entry, live_source(f));
            require(opened.status == S::committed && opened.resolved_utc_start == start,
                    "Import retains the chosen date's exact bound");
            auto untimed = item("untimed", "10");
            untimed.activity.reset();
            untimed.facts.source_timestamp_utc_ms.reset();
            untimed.source_time = intake::SourceTimeQuality::absent;
            untimed.membership_proof.clear();
            auto request = page({old, untimed});
            request.request.source = live_source(f);
            request.request.operation = entry.key;
            request.request.continuation.reset();
            request.request.resolved_start_ms = start;
            request.request.interval = {intake::Endpoint::inclusive, intake::Endpoint::exclusive,
                                        "date-membership", "ordered-key"};
            request.request.upper.position = "11";
            expect(f.submit(request, live_source(f)), S::committed,
                   "Explicit import admits qualified history and holds unknown membership");
            require(disclosed(f).size() == 2 && disclosed(f).last()[2].toInt() == 0,
                    "History enters ordinary pending Review despite automatic catch-up setting");
            const auto before = storage::protected_data(f.store.db);
            expect(f.submit(request, live_source(f)), S::exact_replay,
                   "Exact import page retry closes its existing disposition");
            require(storage::protected_data(f.store.db) == before,
                    "Import replay adds no new durable support or Review inputs");
        });
    }

    void coverageAndLossStayIndependentOfResubscription() {
        check([] {
            Fixture f;
            auto first = gap(f, M::review);
            auto a = item("12", "12");
            auto b = item("13", "13");
            a.activity->identity.opaque_id = "activity-12";
            b.activity->identity.opaque_id = "activity-13";
            first.items = {a, b};
            first.coverage = intake::RangeProof{
                intake::Coverage::contiguous, "11", "13", "qualified-contiguous-12-13", {}};
            expect(f.submit(first, live_source(f)), S::committed,
                   "Durable covered page advances contiguous frontier only through 13");
            require(f.cursors().front().mid(0, 3) ==
                        SqlRow{2, QByteArray("token-opaque"), QByteArray("13")},
                    "Opaque continuation and coverage frontier stay distinct");
            auto second = first;
            second.items.clear();
            second.request.source = live_source(f);
            second.request.ordinal = 2;
            second.request.continuation = first.next;
            second.request.frontier = "13";
            second.next.reset();
            second.coverage =
                intake::RangeProof{intake::Coverage::lost, "13", "14", "qualified-no-retention",
                                   "source retention expired"};
            expect(f.submit(second, live_source(f)), S::committed,
                   "Qualified loss settles only the proved remaining range");
            auto new_segment = item("15", "15");
            new_segment.activity->identity.opaque_id = "activity-15";
            expect(f.submit(new_segment, live_source(f)), S::committed,
                   "A proved new live segment can coexist with prior gap loss");
            require(f.store.db.rows("SELECT coverage FROM main.subathon_intake_segments "
                                    "ORDER BY revision,ordinal") == SqlRows{{0}, {1}} &&
                        f.store.db.rows("SELECT status FROM main.subathon_intake_gaps") ==
                            SqlRows{{1}},
                    "A newer live item cannot cover the unavailable old position");
        });
        check([] {
            Fixture f;
            auto exhausted = gap(f, M::review);
            exhausted.next.reset();
            expect(f.submit(exhausted, live_source(f)), S::committed,
                   "An exhausted empty page can record pagination without a coverage assertion");
            require(f.store.db.rows("SELECT status FROM main.subathon_intake_gaps") == SqlRows{{0}},
                    "Empty exhaustion without a range proof leaves the gap open");
        });
    }

    void recoveryModes_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("missingRate");
        QTest::newRow("review") << int(M::review) << false;
        QTest::newRow("automatic") << int(M::automatic) << false;
        QTest::newRow("automatic-independent-rate-hold") << int(M::automatic) << true;
        QTest::newRow("record-without-credit") << int(M::record_without_credit) << false;
    }

    void recoveryModes() {
        QFETCH(int, mode);
        QFETCH(bool, missingRate);
        check([&] {
            Fixture f;
            auto first = gap(f, static_cast<M>(mode));
            f.missing_rate = missingRate;
            f.requested_ms = 120000; // 60s rule ×2 at arrival. Source-time modifier is ×1.
            auto input = item("H");
            input.evidence = mode == int(M::record_without_credit)
                                 ? core::PaymentEvidence::unconfirmed
                                 : core::PaymentEvidence::confirmed;
            first.items = {input};
            expect(f.submit(first, live_source(f)), S::committed,
                   "Recovered support uses retained policy basis and original capture");
            const auto saved = disclosed(f);
            require(!saved.isEmpty() && saved.last()[3].toBool() == missingRate,
                    "Independent missing-rate fact is preserved");
            const auto reported = f.submit(intake::Inspect{QByteArray("H")}, live_source(f));
            require(reported.projection && reported.projection->source_utc_ms == 1000 &&
                        reported.projection->source_time == intake::SourceTimeQuality::reliable &&
                        reported.projection->arrival_utc_ms == 8000,
                    "Reporting retains reliable source time alongside captured arrival");
            if (mode == int(M::automatic) && !missingRate) {
                assert_time_award(f);
                require(reported.projection->credited_time_ms == 20000,
                        "Projection exposes actual credited time");
                return;
            }
            f.no_award();
            require(reported.projection->credited_time_ms == 0 &&
                        reported.projection->credited_score == 0 &&
                        reported.projection->credited_progression == 0,
                    "No award publishes known zero credited amounts");
            if (mode == int(M::review)) {
                require(saved.last()[2].toInt() == 0, "Review sets only ordinary pending state");
                const auto selected = *reported.projection;
                auto service = f.store.service();
                ReviewEntryIntent intent;
                intent.selector = {selected.contribution, selected.original};
                intent.milestones = std::vector<MilestoneAcceptanceChoice>{};
                const std::vector entries{intent};
                const auto request =
                    f.store.bind("review-gap", entries, f.store.preview(service, entries));
                const auto reply = f.store.execute(service, request);
                const auto &rows = f.store.reply(reply).rows;
                require(rows.size() == 1 && rows.front().outcome &&
                            rows.front().outcome->status == ReviewStatus::applied,
                        "Authorized ordinary Review applies the retained request");
                assert_time_award(f);
            }
            if (mode == int(M::record_without_credit)) {
                require(saved.last()[4].toInt() == 2 &&
                            reported.projection->evidence == core::PaymentEvidence::unconfirmed &&
                            !reported.projection->authoritative_verified_record_only,
                        "Unconfirmed record-only has zero credit and retains its evidence label");
                auto confirmed = input;
                confirmed.key = "confirm-H";
                confirmed.role = "confirmation";
                confirmed.identifier = confirmed.key;
                confirmed.kind = ContributionObservationKind::confirmation;
                confirmed.evidence = core::PaymentEvidence::confirmed;
                expect(f.submit(confirmed, live_source(f)), S::linked,
                       "Exact verified confirmation uses owning evidence path");
                expect(f.submit(first, live_source(f)), S::exact_replay,
                       "Original page retry preserves record-only state");
                const auto after = f.submit(intake::Inspect{QByteArray("H")}, live_source(f));
                require(after.projection && after.projection->authoritative_verified_record_only &&
                            after.projection->arrival_utc_ms == 8000 &&
                            after.projection->source_utc_ms == 1000 &&
                            after.projection->credited_time_ms == 0 && disclosed(f).last()[4] == 2,
                        "Confirmation changes evidence and verified classification only");
                f.no_award();
            }
        });
    }

    void exactReceiptIdentityAndUnprovedOverlap() {
        check([] {
            Fixture f;
            auto policy = baseline();
            auto mirror = source.scope;
            mirror.account = "B";
            policy.members.append({mirror, "individual", "main", "overlap", false, "mirror"});
            expect(f.submit(policy, unopened), S::committed,
                   "One exact primary and one configured mirror share an overlap set");
            auto primary = item("primary", "11");
            primary.identifier = QByteArray::fromHex("410062");
            primary.activity.reset();
            primary.disjointness_proof = "proved-partition";
            primary.facts.actor_display_name = "Same name";
            primary.facts.payment_reference = core::ReceiptLinkReference{"payments", "p"};
            expect(f.submit(primary, live_source(f)), S::committed,
                   "Proved disjoint receipt identity alone permits primary intake");
            auto same = primary;
            same.key = "repeat";
            f.arrival = 9000;
            expect(f.submit(same, live_source(f)), S::exact_replay,
                   "Distinct attempt preserves the immutable original arrival");
            same.key = "conflict";
            same.facts.money = core::ReceiptMoney{"12.35", "CHF"};
            expect(f.submit(same, live_source(f)), S::conflict,
                   "Conflict records audit and cannot replace original facts");
            auto other = primary;
            other.key = "mirror";
            expect(f.submit(other, live_source(f, mirror)), S::held,
                   "Equal name, amount and payment reference cannot promote a mirror");
            require(
                f.store.db.rows("SELECT count(*) FROM main.subathon_receipts") == SqlRows{{2}} &&
                    disclosed(f).size() == 1 &&
                    f.store.db.rows("SELECT min(server_timestamp_utc_ms),count(*) "
                                    "FROM main.subathon_receipt_attempts") == SqlRows{{8000, 4}},
                "Exact source identities remain distinct with original arrival retained");
            require(
                f.store.db.rows("SELECT identifier_namespace,opaque_id "
                                "FROM main.subathon_receipts WHERE source_account=X'41'") ==
                    SqlRows{{QByteArray::fromHex("00000001530000000864656c6976657279000000014e"),
                             QByteArray::fromHex("410062")}},
                "Binary identity uses independent scope/role/namespace length prefix bytes");
        });
        for (auto history_first : {false, true}) {
            check([=] {
                Fixture f;
                auto request = gap(f, M::review);
                auto live = item("L");
                auto history = item("H");
                live.activity.reset();
                history.activity.reset();
                request.items = {history};
                request.coverage = intake::RangeProof{
                    intake::Coverage::contiguous, "11", "14", "qualified-retrieval", {}};
                const auto before = disclosed(f);
                const auto live_path = [&] {
                    expect(f.submit(live, live_source(f)), S::held,
                           "Potential overlap is held on the first unproved live observation");
                };
                const auto history_path = [&] {
                    expect(f.submit(request, live_source(f)), S::committed,
                           "Durable history hold satisfies retrieval without admitting support");
                };
                if (history_first) {
                    history_path();
                    live_path();
                } else {
                    live_path();
                    history_path();
                }
                require(disclosed(f) == before,
                        "Neither unproved peer produces evidence or claims");
                f.no_award();
            });
        }
    }

    void aggregateLaneCannotCreateIndividualSupport() {
        check([] {
            Fixture f;
            auto policy = baseline();
            policy.members.append({source.scope, "aggregate", "progression", "aggregate-disjoint",
                                   true, "aggregate-only"});
            expect(f.submit(policy, unopened), S::committed,
                   "Explicitly disjoint aggregate lane owns its own progression path");
            auto total = item("total-100", "11");
            total.activity.reset();
            total.facts.money = core::ReceiptMoney{"100.00", "CHF"};
            total.award_class = "aggregate";
            total.aggregate_metric = "qualified-cumulative-support";
            const auto start = f.submit(total, live_source(f));
            require(start.status == S::baseline_only && !start.aggregate_delta,
                    "First qualified comparable total establishes an aggregate baseline");
            total.key = "total-112";
            total.identifier = total.key;
            total.position = "12";
            total.facts.money = core::ReceiptMoney{"112.34", "CHF"};
            const auto advanced = f.submit(total, live_source(f));
            require(advanced.status == S::committed &&
                        advanced.aggregate_delta == core::ReceiptMoney{"12.34", "CHF"} &&
                        !advanced.contribution && disclosed(f).isEmpty(),
                    "Exact same-lane delta is CHF 12.34 with zero individual contributions");
            const auto before = storage::protected_data(f.store.db);
            expect(f.submit(total, live_source(f)), S::exact_replay,
                   "Stable aggregate observation identity deduplicates the delta");
            require(storage::protected_data(f.store.db) == before,
                    "Exact aggregate replay creates no second accounting operation");
        });
    }

    void authorityConflictsAndLimits() {
        check([] {
            Fixture f(true);
            auto request = page();
            const auto before = f.cursors();
            f.authority = intake::Authorization::denied;
            expect(f.submit(request), S::denied, "Current refusal precedes protected page lookup");
            f.authority = intake::Authorization::unavailable;
            expect(f.submit(request), S::unavailable,
                   "Unknown authority does not grant disclosure");
            f.authority = intake::Authorization::allowed;
            auto stale = source;
            stale.fence.generation = "old-generation";
            request.request.source = stale;
            expect(f.submit(request, stale), S::stale,
                   "Old-generation page cannot advance new source");
            request.request.source = source;
            for (int n = 0; n != 17; ++n)
                request.items.append(item("distinct-" + QByteArray::number(n)));
            expect(f.submit(request), S::limit_exceeded, "Seventeen items exceed the page count");
            require(f.cursors() == before && f.attempts().isEmpty(),
                    "Refused commands keep exact cursor/gap and stage no receipt");
        });
    }

    void operationPageCapRejectsOrdinal257() {
        check([] {
            Fixture f(true, false);
            storage::catchup(f.store.db, 257, 0);
            storage::completed_prefix(f.store.db, 256, 0);
            const auto before = f.cursors();
            require(before == SqlRows{{257, QByteArray("T"), QByteArray("11"), 0}} &&
                        f.store.db.rows("SELECT count(*),max(ordinal),sum(item_count) "
                                        "FROM main.subathon_intake_pages") ==
                            SqlRows{{256, 256, 0}},
                    "Exact 256-page cap has a committed empty-item prefix and active gap");
            auto request = page();
            request.request.ordinal = 257;
            expect(f.submit(request), S::limit_exceeded,
                   "Otherwise valid page 257 exceeds the operation cap before item work");
            require(f.cursors() == before && f.attempts().isEmpty() &&
                        f.store.db.rows("SELECT count(*) FROM main.subathon_intake_pages") ==
                            SqlRows{{256}},
                    "Operation cap preserves exact bound cursor and old coverage");
        });
    }

    void independentReviewAndConfirmationBlockers() {
        check([] {
            Fixture f;
            auto request = gap(f, M::review);
            f.awaiting_evidence = true;
            f.missing_rate = true;
            auto original = item("H");
            original.evidence = core::PaymentEvidence::unknown;
            request.items = {original};
            expect(f.submit(request, live_source(f)), S::committed,
                   "Recovered unknown evidence retains its genuine ordinary Review closure");
            const auto projected = f.submit(intake::Inspect{QByteArray("H")}, live_source(f));
            require(projected.projection.has_value(), "Select actual admitted original");
            ReviewEntryIntent intent;
            intent.selector = {projected.projection->contribution, projected.projection->original};
            intent.milestones = std::vector<MilestoneAcceptanceChoice>{};
            const std::vector entries{intent};
            auto review = f.store.service();
            const auto preview = f.store.preview(review, entries);
            using B = core::ContributionBlocker;
            require(
                preview.size() == 1 && preview.front().facts &&
                    std::set(preview.front().facts->blockers.begin(),
                             preview.front().facts->blockers.end()) ==
                        std::set{B::payment_evidence_required, B::missing_rate, B::review_pending},
                "Real ordinary Review sees all three independently derived blockers");
            const auto command = f.store.bind("accept-held", entries, preview);
            const auto accepted = f.store.execute(review, command);
            const auto &reply = f.store.reply(accepted);
            require(reply.rows.size() == 1 && reply.rows.front().outcome &&
                        reply.rows.front().outcome->status == ReviewStatus::accepted_pending &&
                        reply.rows.front().outcome->facts &&
                        std::set(reply.rows.front().outcome->facts->blockers.begin(),
                                 reply.rows.front().outcome->facts->blockers.end()) ==
                            std::set{B::payment_evidence_required, B::missing_rate},
                    "Acceptance clears only ordinary Review");
            auto confirmation = original;
            confirmation.key = "evidence-H";
            confirmation.identifier = confirmation.key;
            confirmation.role = "confirmation";
            confirmation.kind = ContributionObservationKind::confirmation;
            confirmation.evidence = core::PaymentEvidence::confirmed;
            expect(f.submit(confirmation, live_source(f)), S::linked,
                   "Verified confirmation links to the original");
            require(disclosed(f) ==
                        SqlRows{{blob8(projected.projection->original.value), 2, 2, 1, 0}},
                    "Confirmed evidence clears its blocker while missing rate and capture remain");
            f.no_award();
        });
    }

    void handoffRetainsPendingOriginalAndFencesOldSource() {
        check([] {
            Fixture f;
            auto request = gap(f, M::review, true);
            f.requested_ms = 60000;
            request.items = {item("H")};
            expect(f.submit(request, live_source(f)), S::committed,
                   "Old source owns exact pending original");
            const auto original = f.submit(intake::Inspect{QByteArray("H")}, live_source(f));
            require(original.projection.has_value(), "Old original has the retained capture");
            const auto previous = live_source(f);
            const auto facts = storage::protected_data(f.store.db);
            auto next = source.scope;
            next.account = "B";
            const intake::Selection target{
                next, {0, 0, previous.fence.policy_revision, "g4"}, source.profile};
            intake::Baseline handoff{
                intake::CutPurpose::new_only_handoff,
                {{previous.scope, "individual", "main", "overlap", false, "old-primary"},
                 {next, "individual", "main", "overlap", true, "new-primary"}},
                M::automatic,
                previous};
            f.history.proof.clear();
            expect(f.submit(handoff, target), S::unproved,
                   "Unproved discontinuity leaves the old award configuration intact");
            f.history.proof = "proved-new-only-including-old-pending-envelope";
            f.history.upper = "14";
            f.requested_ms = 120000;
            expect(f.submit(handoff, target), S::committed,
                   "Qualified new-only proof fences both award paths in one revision");
            require(storage::protected_data(f.store.db) == facts,
                    "Handoff preserves unrelated rows and retained original without auto-claim");
            expect(f.submit(request, previous), S::stale,
                   "Old generation cannot commit bound history");
            require(f.store.db.rows("SELECT count(*) FROM main.subathon_app_claims") ==
                        SqlRows{{1}},
                    "No new claim follows the switch itself");
            f.store.head(85000, 0, 90);
            auto review = f.store.service();
            ReviewEntryIntent selected;
            selected.selector = {original.projection->contribution, original.projection->original};
            selected.milestones = std::vector<MilestoneAcceptanceChoice>{};
            const std::vector entries{selected};
            const auto accept =
                f.store.bind("accept-old", entries, f.store.preview(review, entries));
            const auto applied = f.store.execute(review, accept);
            const auto &results = f.store.reply(applied).rows;
            require(results.size() == 1 && results.front().outcome &&
                        results.front().outcome->status == ReviewStatus::applied &&
                        f.store.db.rows("SELECT requested_units FROM main.subathon_app_effects "
                                        "ORDER BY requested_units") == SqlRows{{5000}, {60000}},
                    "Eligible old Review uses the old 60s request under current caps");
        });
    }

    void legacyAdoptionKeepsUnprovedHistoryAtIntake() {
        check([] {
            Fixture f;
            const auto legacy = storage::managed("legacy", 2000, "legacy-valuation");
            const auto durable = storage::legacy_receipt(f.store.db, legacy);
            require(storage::evidence(f.store.db, legacy, durable).contribution_id ==
                        core::ContributionId{0},
                    "Independent legacy owner created actual retained original without API-006");
            auto adoption = baseline(M::automatic);
            adoption.purpose = intake::CutPurpose::legacy_adoption;
            f.history.available = false;
            expect(f.submit(adoption, unopened), S::unproved,
                   "Legacy missing cursor leaves adoption pending without new-only proof");
            const auto legacy_facts = disclosed(f);
            require(legacy_facts.size() == 1 && f.attempts().size() == 1,
                    "Legacy committed accounting stays visible during pending adoption");
            f.history.available = true;
            f.history.proof = "new-only-beyond-legacy-award-envelope";
            expect(f.submit(adoption, unopened), S::committed, "Qualified legacy baseline commits");
            auto post_cut = item("new", "11");
            post_cut.activity->identity.opaque_id = "new-only";
            const auto post = f.submit(post_cut, live_source(f));
            require(post.status == S::applied && post.contribution &&
                        post.contribution != core::ContributionId{0},
                    "Only proved post-cut support follows ordinary live application");

            const auto import = history_import({2026, 11, 21});
            const auto opened = f.submit(import, live_source(f));
            require(opened.status == S::committed && opened.resolved_utc_start,
                    "Scoped legacy import has immutable date and upper bound");
            auto unresolved = item("unknown-old", "8");
            unresolved.activity.reset();
            unresolved.facts.source_timestamp_utc_ms = *opened.resolved_utc_start;
            auto clear = item("disjoint-old", "9");
            clear.activity.reset();
            clear.disjointness_proof = "proved-beyond-legacy-possible-award-overlap";
            clear.facts.source_timestamp_utc_ms = *opened.resolved_utc_start + 1;
            auto historic = page({unresolved, clear});
            historic.request.source = live_source(f);
            historic.request.operation = import.key;
            historic.request.continuation.reset();
            historic.request.resolved_start_ms = opened.resolved_utc_start;
            historic.request.interval = {intake::Endpoint::inclusive, intake::Endpoint::exclusive,
                                         "date-membership", "ordered-key"};
            historic.request.upper.position = "10";
            expect(f.submit(historic, live_source(f)), S::committed,
                   "Legacy import separates qualified reviewable input from unproved historical "
                   "overlap");
            const auto all = disclosed(f);
            require(all.size() == 3 && all.first() == legacy_facts.first() &&
                        all.last()[2].toInt() == 0 &&
                        f.store.db.rows("SELECT count(*) FROM main.subathon_app_claims") ==
                            SqlRows{{1}},
                    "Unproved history contributes nothing and disjoint history enters Review");
        });
    }

    void quotaPreflightHoldsWholePageUntilResolution() {
        check([] {
            Fixture f(true, false);
            storage::catchup(f.store.db);
            const auto normalized = storage::normalized_item(18);
            f.store.db.write([&](const auto &s) {
                (void)s.execute(
                    {"WITH RECURSIVE e(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM e WHERE n<256) "
                     "INSERT INTO main.subathon_intake_entries("
                     "installation,provider,account,stream,intake_ref,epoch,generation,"
                     "policy_revision,input,provenance,arrival_ms,phase,disposition,"
                     "quarantine_bytes) "
                     "SELECT X'696e7374616c6c',X'50',X'41',X'53',CAST(printf('%d',n) AS BLOB),"
                     "1,X'6733',1,?,X'70',8000,2,1,28 FROM e",
                     {normalized}});
            });
            require(f.store.db.rows("SELECT count(quarantine_bytes),sum(quarantine_bytes) "
                                    "FROM main.subathon_intake_entries") == SqlRows{{256, 7168}},
                    "Only stream entry count is saturated; both byte budgets have spare room");
            auto unknown = item("new-unidentified");
            unknown.identifier.reset();
            unknown.activity.reset();
            auto request =
                page({unknown},
                     intake::RangeProof{
                         intake::Coverage::contiguous, "11", "14", "qualified-retrieval", {}});
            const auto before = storage::fenced_history(f.store.db);
            expect(f.submit(request), S::quarantine_full,
                   "New unidentified item preflights whole page against stream quota");
            require(storage::fenced_history(f.store.db) == before && f.attempts().isEmpty(),
                    "Quarantine refusal adds no row, receipt, coverage or progress");
            const intake::QuarantineEntry existing{source.scope, "1",          normalized, "p",
                                                   8000,         source.fence, {}};
            require(storage::transition(f.store.db,
                                        [&](const auto &s) {
                                            return storage::quarantine_result(
                                                intake::stage_quarantine_resolution(
                                                    s, existing, "current-authority"));
                                        }) == storage::Stop::committed,
                    "Owning retention path frees exactly one fixture slot");
            expect(f.submit(request), S::committed,
                   "Identical page can commit its unresolved item after qualified resolution");
            require(f.store.db.rows("SELECT intake_ref FROM main.subathon_intake_entries "
                                    "WHERE operation_ref=X'4f' AND quarantine_bytes IS NOT NULL") ==
                            SqlRows{{QByteArray("new-unidentified")}} &&
                        f.store.db.rows(
                            "SELECT committed_prefix,outcome FROM main.subathon_intake_pages") ==
                            SqlRows{{1, 1}} &&
                        f.attempts().isEmpty(),
                    "Coverage can commit only with a terminal durable unidentified disposition");
        });
    }

    void civilDateUsesRetainedBasis_data() {
        QTest::addColumn<int>("variant");
        QTest::newRow("midnight-gap") << 0;
        QTest::newRow("midnight-fold") << 1;
        QTest::newRow("full-day-skip") << 2;
        QTest::newRow("unsupported-date") << 3;
    }

    void civilDateUsesRetainedBasis() {
        QFETCH(int, variant);
        check([&] {
            Fixture f(true, false);
            constexpr qint64 utc_d = 1795219200000LL;
            if (variant == 0)
                f.zone = intake::ZoneBasis{0, {{utc_d, 3600}}};
            else if (variant == 1)
                f.zone = intake::ZoneBasis{3600, {{utc_d, 0}}};
            else if (variant == 2)
                f.zone = intake::ZoneBasis{0, {{utc_d, 86400}}};
            const auto input = history_import({2026, 11, 21}, "Synthetic/Frozen");
            const auto before = f.cursors();
            const auto operations =
                f.store.db.rows("SELECT count(*) FROM main.subathon_intake_operations");
            const auto result = f.submit(input);
            if (variant == 2 || variant == 3) {
                expect(result, S::invalid_import_date,
                       "Skipped or unsupported date fails before operation");
                require(
                    f.cursors() == before &&
                        f.store.db.rows("SELECT count(*) FROM main.subathon_intake_operations") ==
                            operations,
                    "Invalid import date creates no operation or progress");
                return;
            }
            const auto expected = variant == 0 ? utc_d : utc_d - 3600000;
            require(result.status == S::committed && result.resolved_utc_start == expected,
                    "Gap takes first real instant; fold takes earlier midnight");
            auto different = input;
            different.local_date = QDate(2026, 11, 22);
            expect(f.submit(different), S::conflict,
                   "Same installation operation key cannot bind a different resolved start");
            f.zone = intake::ZoneBasis{7200, {}};
            const auto exact = f.submit(input);
            require(exact.status == S::exact_replay && exact.resolved_utc_start == expected,
                    "Exact retry reuses persisted UTC and retained zone basis");
        });
    }
};

QTEST_GUILESS_MAIN(Api006PolicyTests)
#include "policy_tests.moc"
