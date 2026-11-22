// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_027/command_fixture.hpp"
#include "../support/sqlite_live_financial_refunds_fixture.hpp"

#include <optional>

namespace {
using namespace storage_027_fixture;
namespace live = live_financial_refund_fixture;
using Policy = MilestoneRefundPolicy;

const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance, "accept-A"};
const MilestoneHoldKey review{MilestoneHoldCause::refund_review, "earlier-refund"};
const MilestoneAction bonus{MilestoneActionKind::bonus, "award", "v1"};

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void require_defer_history(const SqliteWorkSession &session, bool edit_origin,
                           std::optional<MilestoneDecisionEvidence> skip_resolution = std::nullopt,
                           std::optional<MilestoneSkipProvenance> ended_by_skip = std::nullopt) {
    const auto acceptance = SqliteMilestoneGroupDecisions(session).acceptance_audit(edit::a);
    const auto resolution =
        skip_resolution ? std::optional{MilestoneHoldResolution::skip} : std::nullopt;
    if (!edit_origin) {
        require(acceptance && acceptance->choice == MilestoneActionChoice::defer &&
                    acceptance->evidence ==
                        MilestoneDecisionEvidence{"operator", defer.source_ref} &&
                    acceptance->resolution == resolution &&
                    acceptance->resolution_evidence == skip_resolution &&
                    acceptance->ended_by_skip == ended_by_skip,
                "The application acceptance audit retains the exact Defer and its history");
        return;
    }

    require(!acceptance, "An edit choice has no paused-input acceptance audit");
    // The edit choice is written by the normal edit and resolution paths.
    // Inspect its retained history directly because the acceptance audit is application-only.
    const auto rows =
        session.execute({"SELECT choice,actor_ref,decision_ref,coalesce(resolution,-1),"
                         "coalesce(resolution_actor_ref,x''),coalesce(resolution_ref,x''),"
                         "coalesce(ended_by_skip_kind,-1),coalesce(ended_by_skip_ref,x'') "
                         "FROM main.subathon_milestone_edit_choices WHERE occurrence_ref=?",
                         {QByteArray::fromStdString(edit::a.ref)}});
    require(rows ==
                SqlRows{{int(MilestoneActionChoice::defer),
                         QByteArray::fromStdString(edit::actor.actor_ref),
                         QByteArray::fromStdString(defer.source_ref),
                         resolution ? int(*resolution) : -1,
                         skip_resolution ? QByteArray::fromStdString(skip_resolution->actor_ref)
                                         : QByteArray(""),
                         skip_resolution ? QByteArray::fromStdString(skip_resolution->decision_ref)
                                         : QByteArray(""),
                         ended_by_skip ? int(ended_by_skip->origin) : -1,
                         ended_by_skip ? QByteArray::fromStdString(ended_by_skip->source_ref)
                                       : QByteArray("")}},
            "The retained edit choice preserves the exact Defer and its history");
}

struct RefundControl {
    live::Fixture fixture{
        {.amount = "400", .requested = 0, .main = 0, .cap_tail = 0, .score = 400, .count = 8000},
        {.main_before = 15, .other_score = 700, .omit_time = true},
        false};
    finance::File &file = fixture.file;

    explicit RefundControl(bool edit_origin) {
        const auto manifest = connector_intake_repository_manifest();
        report(migrate(*file.writer, {MigrationMode::advance, manifest, {}, {}}),
               CatalogState::current, 20, 20, 1);
        reopen();
        fixture.input.selected_track = edit::track;
        file.write([&](const auto &s) {
            require(SqliteAccountSessionUnit(s, {}).insert_installation(
                        {{"install", "epoch"}, "revision"}) == LifecycleWriteResult::staged,
                    "Owning account store admits the exact fixture installation");
        });
        if (edit_origin)
            create_edit_groups();
        else
            create_application_groups();
        require_score(1100);
        fixture.move("score-before-earlier-refund", file.head().timer, {}, -80);
        require_score(1020);
    }

    void reopen() {
        auto drained = std::make_shared<bool>(false);
        require(file.writer->shutdown([drained] { *drained = true; }) == StorageError::none &&
                    QTest::qWaitFor([drained] { return *drained; }, 10000),
                "v20 financial writer drains before reopen");
        file.observer.reset();
        file.writer.reset();
        file.open(finance::limits());
        report(migrate(*file.writer,
                       {MigrationMode::inspect, connector_intake_repository_manifest(), {}, {}}),
               CatalogState::current, 20, 20, 0);
    }

    void create_edit_groups() {
        edit::Authority owner;
        owner.facts.lifecycle.timer = file.head().timer;
        owner.facts.captures = {{"A", edit::a, {bonus}, {}, {}}, {"B", edit::b, {}, {}, {}}};
        const MilestoneEditIntent intent{
            {edit::track, "adjust"},
            {edit::actor.actor_ref, defer.source_ref},
            {{"A", 1000}, {"B", 1050}},
            {{edit::a, MilestoneActionChoice::defer}, {edit::b, MilestoneActionChoice::run}}};
        file.write([&](const auto &s) {
            require(initialize_milestone_track(s, {edit::track,
                                                   "cfg",
                                                   "initial",
                                                   "prefix",
                                                   {{"A", 1200}, {"B", 1300}},
                                                   std::vector<MilestoneTrackSeed::Member>{}}),
                    "Owning track starts with an empty completion prefix");
            const auto preview = preview_milestone_target_edit(s, owner, edit::actor, intent);
            edit::File::confirmable(preview);
            const auto confirmed =
                confirm_milestone_target_edit(s, owner, edit::actor, intent, *preview.confirmation);
            require(confirmed.result && confirmed.result->achievements.size() == 2 &&
                        confirmed.result->achievements[0].order == 1 &&
                        confirmed.result->achievements[1].order == 2,
                    "Normal target edit creates exact Defer A and then ready B");
        });
    }

    void create_application_groups() {
        std::vector<MilestoneTrackSeed::Member> prefix;
        for (bool first : {true, false}) {
            const auto &key = first ? edit::a : edit::b;
            const std::string label = first ? "A" : "B";
            const auto ref = "application-" + label;
            const auto receipt = file.intake(ref, {}, "synthetic-milestone-owner");
            const core::MilestoneCompletion achievement{label, "initial", first ? 1000 : 1050, 1100,
                                                        "ctx"};
            prefix.push_back({achievement, {"test", ref}, key});
            file.write([&](const auto &s) {
                auto bundle = finance::File::receipt_application(s, receipt, ref);
                bundle.after = file.encoded(std::move(bundle.after), bundle.advanced_before.token);
                finance::File::retain_plan(s, bundle.intent.plan,
                                           finance::File::plain_capture(receipt, {}, {}));
                require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                            SqliteBundleStageDisposition::staged,
                        "Normal application anchors the exact occurrence");
                const auto actions = first ? std::vector{bonus} : std::vector<MilestoneAction>{};
                SqliteMilestoneGroupStore(s).stage(bundle.intent.operation,
                                                   {{{achievement, actions, {}, key}, {}}});
                SqliteMilestoneGroupDecisions(s).record_acceptance(
                    bundle.intent.operation,
                    {PausedAcceptanceMode::normal,
                     {"operator", defer.source_ref},
                     {{key, first ? MilestoneActionChoice::defer : MilestoneActionChoice::run}}});
            });
        }
        file.write([&](const auto &s) {
            require(
                initialize_milestone_track(
                    s,
                    {edit::track, "cfg", "initial", "prefix", {{"A", 1000}, {"B", 1050}}, prefix}),
                "Owning track retains the exact application-origin prefix");
        });
    }

    void require_score(qint64 expected) {
        require(file.rows("SELECT sum(b.units) FROM subathon_app_head h "
                          "JOIN subathon_app_balances b ON h.token=b.token WHERE balance_kind=2") ==
                    SqlRows{{expected}},
                "Physical score matches the independent refund arithmetic");
    }

    static FinancialEntitlement entitlement(qint64 prior, qint64 cumulative, qint64 position) {
        const auto fraction = finance::fraction(cumulative, 400);
        return live::quote(prior * 10, cumulative * 10, 4000, false, {},
                           {live::link(finance::CK::score, 1, 1, position, prior, cumulative,
                                       fraction, finance::fraction(cumulative), false),
                            live::link(finance::CK::whole_count, 2, 2, position, 0, 0, fraction,
                                       fraction * 8000, false)});
    }

    void first_refund() {
        fixture.input.milestone_policy = Policy::review;
        const auto command = fixture.command(review.source_ref, "25", 0);
        const auto actual = live::accepted(fixture.refund(command, entitlement(0, 25, 0)));
        require_score(995);
        require(actual.milestone_observations.size() == 1 &&
                    actual.milestone_observations.front().occurrence == edit::a,
                "Only A crosses at 1,000 and receives the first Review");
        file.write([&](const auto &s) {
            const SqliteMilestoneGroupDecisions decisions(s);
            const auto audit = decisions.refund_audit(edit::a, review.source_ref);
            require(audit && audit->policy == Policy::review &&
                        audit->current_target_micro_points == 1000 &&
                        audit->deficit_micro_points == 5 && !audit->resolution &&
                        !audit->ended_by_skip &&
                        decisions.active_holds(edit::a) == std::vector{defer, review} &&
                        decisions.pending_disposition(edit::b) ==
                            MilestonePendingDisposition::ready,
                    "Both original causes are active and B remains ready");
        });
    }

    void require_release_bits_zero(bool edit_origin) {
        const auto *table =
            edit_origin ? "subathon_milestone_edit_occurrences" : "subathon_milestone_groups";
        require(
            file.rows(QString("SELECT released FROM main.%1 ORDER BY queue_order").arg(table)) ==
                SqlRows{{0}, {0}},
            "The old-store controls never release either occurrence");
    }
};

class ExactEditRelease final : public MilestoneEditReleaseAuthority {
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == edit::b;
    }
};
} // namespace

class Milestone006StoreTests final : public QObject {
    Q_OBJECT

  private slots:
    void s6aUnstoppedCrossing_v20_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<int>("policy");
        for (bool edit : {false, true}) {
            for (const auto &[name, policy] :
                 {std::pair{"continue", Policy::continue_actions},
                  std::pair{"review", Policy::review}, std::pair{"skip", Policy::skip}}) {
                const auto row = QByteArray(edit ? "edit-" : "application-") + name;
                QTest::newRow(row.constData()) << edit << static_cast<int>(policy);
            }
        }
    }

    void s6aUnstoppedCrossing_v20() {
        QFETCH(bool, edit_origin);
        QFETCH(int, policy);
        check([&] {
            RefundControl control(edit_origin);
            control.first_refund();
            auto &f = control.fixture;
            const auto expected = static_cast<Policy>(policy);
            f.move("score-recovered-for-second-crossing", control.file.head().timer, {}, 15);
            control.require_score(1010);
            f.input.milestone_policy = expected;
            const auto command = f.command("later-refund", "20", 1);
            const auto refund = live::accepted(f.refund(command, control.entitlement(25, 45, 1)));
            control.require_score(990);
            require(refund.milestone_observations.size() == 1,
                    "One new policy decision follows the exact debit");
            const auto &observation = refund.milestone_observations.front();
            require(observation.occurrence == edit::a && observation.refund_ref == "later-refund" &&
                        observation.score_before_micro_points == 1010 &&
                        observation.score_after_micro_points == 990 &&
                        observation.current_target_micro_points == 1000 &&
                        observation.policy == expected,
                    "The real refund selects A only, with an exact deficit of ten");
            control.reopen();
            control.file.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions decisions(s);
                const auto earlier = decisions.refund_audit(edit::a, review.source_ref);
                const auto later = decisions.refund_audit(edit::a, "later-refund");
                require(
                    earlier && earlier->policy == Policy::review &&
                        earlier->current_target_micro_points == 1000 &&
                        earlier->deficit_micro_points == 5 && !earlier->resolution && later &&
                        later->policy == expected && later->current_target_micro_points == 1000 &&
                        later->deficit_micro_points == 10 && !later->resolution &&
                        !later->ended_by_skip && !decisions.refund_audit(edit::b, "later-refund") &&
                        decisions.pending_disposition(edit::b) ==
                            MilestonePendingDisposition::ready,
                    "Cold normal reads retain both exact refund decisions and B's gate");
                const auto holds = decisions.active_holds(edit::a);
                if (expected == Policy::skip) {
                    const MilestoneSkipProvenance provenance{MilestoneSkipOrigin::refund_policy,
                                                             "later-refund"};
                    require_defer_history(s, edit_origin, std::nullopt, provenance);
                    require(holds.empty() && earlier->ended_by_skip == provenance &&
                                decisions.pending_disposition(edit::a) ==
                                    MilestonePendingDisposition::skipped,
                            "The configured refund Skip ends both earlier causes with its own "
                            "provenance");
                } else {
                    require_defer_history(s, edit_origin);
                    std::vector wanted{defer, review};
                    if (expected == Policy::review)
                        wanted.push_back({MilestoneHoldCause::refund_review, "later-refund"});
                    require(
                        holds == wanted && !earlier->ended_by_skip &&
                            decisions.pending_disposition(edit::a) ==
                                MilestonePendingDisposition::held,
                        "Continue retains precisely D and R, and Review adds the new exact cause");
                }
            });
            control.require_release_bits_zero(edit_origin);
        });
    }

    void explicitDeferredSkip_v20_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::newRow("application-origin") << false;
        QTest::newRow("edit-origin") << true;
    }

    void explicitDeferredSkip_v20() {
        QFETCH(bool, edit_origin);
        check([&] {
            RefundControl control(edit_origin);
            control.first_refund();
            using Resolution = MilestoneHoldResolution;
            const MilestoneDecisionEvidence authority{"operator", "explicit-defer-skip"};
            control.file.write([&](const auto &s) {
                require(SqliteMilestoneGroupDecisions(s).resolve(edit::a, defer, Resolution::skip,
                                                                 authority),
                        "Authorized #236 Skip resolves the exact original Defer");
            });
            control.reopen();
            control.file.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions decisions(s);
                require_defer_history(s, edit_origin, authority);
                const auto earlier = decisions.refund_audit(edit::a, review.source_ref);
                const MilestoneSkipProvenance provenance{MilestoneSkipOrigin::deferred_resolution,
                                                         defer.source_ref};
                require(earlier && earlier->policy == Policy::review &&
                            earlier->current_target_micro_points == 1000 &&
                            earlier->deficit_micro_points == 5 && !earlier->resolution &&
                            earlier->ended_by_skip == provenance &&
                            decisions.active_holds(edit::a).empty() &&
                            decisions.pending_disposition(edit::a) ==
                                MilestonePendingDisposition::skipped &&
                            decisions.pending_disposition(edit::b) ==
                                MilestonePendingDisposition::ready,
                        "Explicit Skip retains its own decision and is the cause of Review's end");
            });
            control.require_score(995);
            control.require_release_bits_zero(edit_origin);
        });
    }

    void s8ReleaseRaceStore_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<bool>("stop_first");
        QTest::newRow("application-stop-wins") << false << true;
        QTest::newRow("application-release-wins") << false << false;
        QTest::newRow("edit-stop-wins") << true << true;
        QTest::newRow("edit-release-wins") << true << false;
    }

    void s8ReleaseRaceStore() {
        QFETCH(bool, edit_origin);
        QFETCH(bool, stop_first);
        check([&] {
            // The protected producer can capture one application bonus using
            // its large-slot fixture. No bonus receipt or delivery claim is made.
            Populated f({.large_slots = edit_origin ? 0U : 1U});
            advance_fixture(f.db);
            const auto release_application = [&] {
                const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    return release_milestone_head(f.db.writer(), f.app_occurrence, std::move(done));
                });
                finance::normal(result.work);
                return result.disposition;
            };
            if (edit_origin)
                require(release_application() == MilestoneReleaseDisposition::released,
                        "Real application release makes the exact edit occurrence head");
            const auto release_head = [&] {
                if (!edit_origin)
                    return release_application();
                const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    return release_milestone_head(f.db.writer(), f.edit_occurrence,
                                                  std::make_shared<ExactEditRelease>(),
                                                  std::move(done));
                });
                finance::normal(result.work);
                return result.disposition;
            };
            if (!stop_first)
                require(release_head() == MilestoneReleaseDisposition::released,
                        "The winning v21 release leaves the original slot unstarted");
            Authority owner;
            const auto target = edit_origin ? edited(f) : application(f);
            const auto command = stop("race-stop", target, QueueChoice::continue_queue);
            const auto decision = commit(f.db, owner, command, Disposition::committed);
            require(decision.cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                        decision.added_causes.empty() && decision.playback_intents.empty(),
                    "Both orderings cancel only the exact unstarted slot");
            if (stop_first)
                require(release_head() == MilestoneReleaseDisposition::refused,
                        "The losing normal release is refused after acknowledged Stop");
            const auto *table =
                edit_origin ? "subathon_milestone_edit_occurrences" : "subathon_milestone_groups";
            require(f.db.rows(QString("SELECT released FROM main.%1").arg(table)) ==
                        SqlRows{{stop_first ? 0 : 1}},
                    "Normal release changes the exact bit only if it wins");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006StoreTests)
#include "store_tests.moc"
