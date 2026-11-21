// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "missing_storage.hpp"
#include "producer_fixture.hpp"

namespace storage_027_fixture {
inline void advance_fixture(CatalogFile &db) {
    auto manifest = fixture_manifest();
    const auto version = manifest.steps.back().version;
    MigrationLimits limits;
    limits.schema_objects = manifest.steps.back().expected_schema.size();
    db.reopen(predecessor::migration_profile());
    report(migrate(db.writer(), {MigrationMode::advance, manifest, limits, {}}),
           CatalogState::current, version, version, 1);
    db.reopen(profile());
    report(migrate(db.writer(), {MigrationMode::inspect, std::move(manifest), limits, {}}),
           CatalogState::current, version, version, 0);
}

struct Authority final : CommandAuthority {
    bool command{true};
    bool disclosure{true};
    QByteArray revision{"grant-1"};
    QByteArray configuration{"cfg"};
    std::vector<AuthorizedCapture> captures{
        {{1, {MilestoneActionKind::automation, "announce", "announce-v2"}},
         QByteArray::fromHex("0082"),
         "local/new-route",
         "new-sender"}};
    std::vector<PlaybackInstance> observed_playback;
    std::optional<Facts> select(const SqliteWorkSession &, const Command &wanted) override {
        if (wanted.track != edit::track || wanted.actor.actor_ref != edit::actor.actor_ref ||
            wanted.actor.session_lineage != edit::actor.session_lineage)
            return std::nullopt;
        return Facts{command,  disclosure,       revision, configuration, "operator-decision",
                     captures, observed_playback};
    }
};

inline Occurrence application(const Populated &fixture) {
    return {edit::track, Populated::app_occurrence, fixture.original};
}

inline Occurrence edited(const Populated &fixture) {
    return {edit::track, Populated::edit_occurrence, fixture.edit_intent.key};
}

inline Command stop(QByteArray key, Occurrence target, std::optional<QueueChoice> choice = {}) {
    return {target.track, std::move(key), edit::actor, StopIntent{std::move(target), choice}};
}

inline Command resume(QByteArray key, std::vector<QByteArray> causes) {
    return {edit::track, std::move(key), edit::actor, ResumeIntent{std::move(causes)}};
}

inline Command replace(const Populated &fixture) {
    const Replacement selection{1, {MilestoneActionKind::automation, "announce", "announce-v2"}};
    return {edit::track, "replace-edit-slot-1", edit::actor,
            ReplaceIntent{edited(fixture), {selection}}};
}

enum class Finish { commit, rollback, throw_after_commit };
struct Turn {
    SqliteWorkResult work;
    std::optional<Decision> published;
    bool entered;
    bool missing;
};

template <class Call> Turn run(CatalogFile &db, Call call, Finish finish = Finish::commit) {
    std::optional<Decision> provisional;
    const auto work = qt_test_support::run_work(
        db.writer(),
        [&](auto context) -> QVariant {
            const auto session = context.begin();
            provisional = call(session);
            if (finish == Finish::rollback ||
                provisional->disposition == Disposition::not_implemented)
                session.rollback();
            else
                session.commit();
            if (finish == Finish::throw_after_commit &&
                provisional->disposition != Disposition::not_implemented)
                throw std::runtime_error("Synthetic abnormal return after acknowledged SQL commit");
            return 1;
        },
        10000);
    const bool entered = provisional.has_value();
    const bool missing = entered && provisional->disposition == Disposition::not_implemented;
    if (work.transaction != TransactionOutcome::committed || work.error != WorkError::none ||
        !work.value)
        provisional.reset();
    return {work, std::move(provisional), entered, missing};
}

inline Decision commit(CatalogFile &db, Authority &authority, const Command &command,
                       Disposition expected, const ReplacePreview *preview = nullptr) {
    const auto result = run(db, [&](const SqliteWorkSession &session) {
        return stage_command(session, authority, command, preview);
    });
    require(!result.missing, "STORAGE-027 NI: eligible command reached the storage seam");
    require(result.entered && result.published && result.published->disposition == expected,
            "STORAGE-027 command disposition at normal acknowledged completion");
    return *result.published;
}

inline Decision read(CatalogFile &db, Authority &authority, const Command &command) {
    const auto result = run(db, [&](const SqliteWorkSession &session) {
        return read_command(session, authority, command);
    });
    require(result.entered && result.published,
            "Only a normal exact readback publishes a decision");
    return *result.published;
}

inline std::shared_ptr<const ReplacePreview> bind_replace(CatalogFile &db, Authority &authority,
                                                          const Command &command) {
    std::shared_ptr<const ReplacePreview> bound;
    db.write([&](const auto &session) {
        const auto preview = preview_replace(session, authority, command);
        auto result = std::get_if<std::shared_ptr<const ReplacePreview>>(&preview);
        require(result && *result, "STORAGE-027 NI: exact preview of the v20-produced queued slot");
        bound = *result;
    });
    return bound;
}

inline SqlRows cancellations(CatalogFile &db, const QByteArray &operation) {
    return db.rows("SELECT ordinal,version FROM main.subathon_milestone_stop_cancellations "
                   "WHERE installation=X'696e7374616c6c' AND track=X'747261636b' AND stop=? "
                   "ORDER BY ordinal",
                   {operation});
}

inline SqlRows causes(CatalogFile &db) {
    return db.rows("SELECT cause,stop,resume FROM main.subathon_milestone_interruption_causes "
                   "ORDER BY stop");
}

inline SqlRows journal(CatalogFile &db) {
    return db.rows(
        "SELECT installation,track,operation,kind,actor,lineage,decision,intent,result "
        "FROM main.subathon_milestone_command_journal ORDER BY installation,track,operation");
}

} // namespace storage_027_fixture
