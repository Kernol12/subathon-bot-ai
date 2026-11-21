// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../support/milestone_target_edit_fixture.hpp"
#include "catalog_fixture.hpp"

namespace storage_027_fixture {
namespace edit = milestone_target_edit_fixture;
using namespace std::chrono_literals;

inline void install_protected(predecessor::Populated &f) {
    const auto manifest = connector_intake_repository_manifest();
    expect_same_manifest(manifest, predecessor::successor());
    f.db.reopen(predecessor::migration_profile());
    report(migrate(f.db.writer(), {MigrationMode::advance, manifest, {}, {}}),
           CatalogState::current, 20, 20, 1);
    f.db.reopen(profile());
    report(migrate(f.db.writer(), {MigrationMode::inspect, manifest, {}, {}}),
           CatalogState::current, 20, 20, 0);
}

enum class SetupThrough { bundle, group, decision, both_origins };

struct ProducerOptions {
    bool held{};
    bool predecessor_v20{true};
    std::size_t large_slots{};
    bool third_target{};
    bool audio_job{};
    int edit_slots{1};
    bool hold_edit{};
    bool middle_bonus{};
    bool ordinary_application{};
    SetupThrough through{SetupThrough::both_origins};
};

struct Populated : predecessor::Populated {
    static inline const MilestoneOccurrenceKey app_occurrence{"sar2-occurrence"};
    static inline const MilestoneOccurrenceKey edit_occurrence = edit::b;
    CapturedMilestoneGroup app_group{
        {{"threshold-100", "rule-1", 100, 105, "proof"}, {}, {10000ms, 1000ms}, app_occurrence},
        {}};
    ReviewEntryIntent selected;
    ReviewCommand review_command;
    ApplicationOperationKey original;
    edit::Authority edit_authority;
    MilestoneEditIntent edit_intent =
        edit::single("earn-B", {"B", 104}, {{edit_occurrence, MilestoneActionChoice::run}});
    MilestoneEditJobKey job{edit_intent.key, edit_occurrence, 1};

    explicit Populated(ProducerOptions options = {}) : predecessor::Populated(99, 7) {
        if (options.hold_edit)
            edit_intent.choices[0].choice = MilestoneActionChoice::defer;
        if (options.predecessor_v20)
            install_protected(*this);
        else
            db.reopen(profile());
        install();
        selected = seed("terminal",
                        {finance::request(finance::Unit::milliseconds, 20, "main"),
                         finance::request(finance::Unit::micro_points, 15, "score"),
                         finance::request(finance::Unit::whole_count, 1, "count")},
                        false, true);
        facts.current->timer.hard_end_utc_ms = 50;
        facts.current->clock.paused = true;
        facts.current->clock.utc_now_ms = 50;
        facts.current->finished_siblings = std::vector{finance::Unit::micro_points};
        auto review = service();
        review_command = bind("terminal-apply", {selected}, preview(review, {selected}));
        const auto result = execute(review, review_command);
        expected_row(reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
        const auto &outcome = reply(result).rows.front().outcome;
        require(outcome && outcome->facts && outcome->facts->bundle,
                "Real Review committed a selectable application bundle");
        original = *outcome->facts->bundle;
        head(99, 7, 105);
        require(db.rows("SELECT request_ordinal,disposition "
                        "FROM main.subathon_review_terminal_requests ORDER BY request_ordinal") ==
                    SqlRows{{1, 1}, {3, 2}},
                "Protected Review committed exactly terminal ordinals 1 and 3");
        if (options.large_slots) {
            // Each legal bonus has a distinct exact 4,096-byte source reference.
            // A real group owner stages all slots; no budget row is invented in SQL.
            for (std::size_t i = 0; i < options.large_slots; ++i) {
                std::string ref = "bonus-" + std::to_string(i) + "-";
                ref.resize(4096, 'b');
                app_group.group.actions.push_back(
                    {MilestoneActionKind::bonus, std::move(ref), "v1"});
            }
            db.reopen({64, 1024, 65536, 4194304, 1024, 4194304});
        }
        db.write([&](const auto &session) {
            const auto bundle = SqliteApplicationBundleStore(session).lookup(original);
            require(
                bundle && bundle->effects.size() == 1 && bundle->effects[0].request_ordinal == 2 &&
                    bundle->result.document.endsWith(QByteArray::fromHex("0000000300000002010002")),
                "Owning read resolves the exact protected SAR2 result and physical effect");
            if (options.through == SetupThrough::bundle)
                return;
            SqliteMilestoneGroupStore(session).stage(original, {app_group});
            if (options.through == SetupThrough::group || options.ordinary_application)
                return;
            SqliteMilestoneGroupDecisions(session).record_acceptance(
                original, {PausedAcceptanceMode::normal,
                           {"operator", "milestone-decision"},
                           {{app_occurrence, options.held ? MilestoneActionChoice::defer
                                                          : MilestoneActionChoice::run}}});
        });
        if (options.through != SetupThrough::both_origins)
            return;
        db.write([&](const auto &session) {
            MilestoneTrackSeed track{edit::track,
                                     "cfg",
                                     "initial-def",
                                     "initial-prefix",
                                     {{"threshold-100", 100}, {"B", 200}},
                                     std::vector<MilestoneTrackSeed::Member>{
                                         {app_group.group.achievement, original, app_occurrence}}};
            if (options.third_target)
                track.targets.push_back({"C", 300});
            require(initialize_milestone_track(session, track),
                    "The shipped track initializer follows the protected SAR2 member");
            edit_authority.facts.lifecycle.timer =
                finance::timer(core::TimerLifecycle::Finished, 99);
            auto capture = edit::capture_b(true);
            if (options.audio_job) {
                capture.actions[0] = {MilestoneActionKind::automation, "audio", "audio-v1"};
                capture.jobs[0].capture.action_ref = "audio";
                capture.jobs[0].capture.action_version_ref = "audio-v1";
            }
            const auto first_action = capture.actions.front();
            const auto first_job = capture.jobs.front();
            for (int ordinal = 2; ordinal <= options.edit_slots; ++ordinal) {
                if (ordinal == 2 && options.middle_bonus) {
                    capture.actions.push_back(
                        {MilestoneActionKind::bonus, "prior-bonus", "bonus-v1"});
                    continue;
                }
                capture.actions.push_back(first_action);
                auto copy = first_job;
                copy.ordinal = ordinal;
                capture.jobs.push_back(std::move(copy));
            }
            edit_authority.facts.captures = {std::move(capture)};
            const auto preview =
                preview_milestone_target_edit(session, edit_authority, edit::actor, edit_intent);
            edit::File::confirmable(preview);
            const auto saved = confirm_milestone_target_edit(session, edit_authority, edit::actor,
                                                             edit_intent, *preview.confirmation);
            require(saved.result && saved.result->achievements.size() == 1 &&
                        saved.result->achievements[0].order == 2,
                    "Owning edit writer commits a second distinct occurrence");
        });
        db.write([&](const auto &session) {
            const auto captured = lookup_milestone_edit_job(session, job);
            require(captured && captured->state == DeliveryJobState::queued &&
                        captured->revision == 1 &&
                        captured->action_document == QByteArray::fromHex("0081"),
                    "Protected edit job is real, immutable and awaits its first start");
        });
    }

    MilestoneEditKey add_third_occurrence(bool audio = false) {
        const auto intent =
            edit::single("earn-C", {"C", 105}, {{edit::c, MilestoneActionChoice::run}});
        auto capture = edit::empty_c;
        if (audio) {
            capture = edit::capture_b(true);
            capture.milestone_ref = "C";
            capture.occurrence = edit::c;
            capture.actions[0] = {MilestoneActionKind::automation, "audio", "audio-v1"};
            capture.jobs[0].capture.action_ref = "audio";
            capture.jobs[0].capture.action_version_ref = "audio-v1";
        }
        edit_authority.facts.captures = {std::move(capture)};
        db.write([&](const auto &session) {
            const auto p =
                preview_milestone_target_edit(session, edit_authority, edit::actor, intent);
            edit::File::confirmable(p);
            const auto result = confirm_milestone_target_edit(session, edit_authority, edit::actor,
                                                              intent, *p.confirmation);
            require(result.result && result.result->achievements.size() == 1 &&
                        result.result->achievements[0].order == 3,
                    "Shipped edit writer retains a third eligible, unreleased occurrence");
        });
        return intent.key;
    }
};

using ProtectedRows = QList<std::pair<QString, SqlRows>>;

inline ProtectedRows snapshot(CatalogFile &db, const QList<CatalogObject> &objects) {
    ProtectedRows result;
    for (const auto &object : objects) {
        if (object.type != "table")
            continue;
        const auto columns =
            db.rows("SELECT cid FROM pragma_table_info(?) ORDER BY cid", {object.name});
        QStringList order;
        for (const auto &column : columns)
            order.append(QString::number(column.front().toInt() + 1));
        require(!order.isEmpty() && !object.name.contains('"'), "Exact protected table shape");
        result.append(std::pair{object.name, db.rows("SELECT * FROM main.\"" + object.name +
                                                     "\" ORDER BY " + order.join(','))});
    }
    return result;
}

} // namespace storage_027_fixture
