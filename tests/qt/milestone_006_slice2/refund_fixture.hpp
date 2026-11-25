// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../storage_027/command_fixture.hpp"
#include "../support/sqlite_live_financial_refunds_fixture.hpp"

namespace milestone_006_slice2 {
using namespace storage_027_fixture;
namespace live = live_financial_refund_fixture;
using Policy = MilestoneRefundPolicy;

inline const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance, "accept-A"};
inline const MilestoneHoldKey review{MilestoneHoldCause::refund_review, "earlier-refund"};
inline const MilestoneAction bonus{MilestoneActionKind::bonus, "award", "v1"};

inline void reopen_financial(finance::File &file, StorageLimits limits = finance::limits()) {
    auto drained = std::make_shared<bool>(false);
    require(file.writer->shutdown([drained] { *drained = true; }) == StorageError::none &&
                QTest::qWaitFor([drained] { return *drained; }, 10000),
            "Financial writer drains before reopen");
    file.observer.reset();
    file.writer.reset();
    file.open(limits);
}

inline void advance_to_protected_successor(finance::File &file) {
    reopen_financial(file, predecessor::migration_profile());
    const auto manifest = fixture_manifest();
    MigrationLimits limits;
    limits.schema_objects = manifest.steps.back().expected_schema.size();
    report(migrate(*file.writer, {MigrationMode::advance, manifest, limits, {}}),
           CatalogState::current, 21, 21, 1);
    reopen_financial(file);
    report(migrate(*file.writer, {MigrationMode::inspect, manifest, limits, {}}),
           CatalogState::current, 21, 21, 0);
}

// Same v20 setup as the sealed old-store control. This fixture continues
// through the already shipped v21 catalog and live refund owner.
struct RefundFixture {
    live::Fixture fixture{
        {.amount = "400", .requested = 0, .main = 0, .cap_tail = 0, .score = 400, .count = 8000},
        {.main_before = 15, .other_score = 700, .omit_time = true},
        false};
    finance::File &file = fixture.file;

    explicit RefundFixture(bool edit_origin) {
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

    void reopen() { reopen_financial(file); }

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

    auto first_refund() {
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
        return std::pair{command, actual};
    }
};
} // namespace milestone_006_slice2
