// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "milestone_group_decisions_fixture.hpp"

#include <array>
#include <subathon/server/milestone_group_decisions.hpp>
#include <subathon/server/sqlite_modifier_activations.hpp>

namespace modifier_activation_fixture {
using namespace milestone_group_decisions_fixture;
namespace core = subathon::core;

// Reviewed v14 DDL. This fixture never calls the v14 manifest under test.
inline const QStringList modifier_ddl{
    R"sql(CREATE TABLE subathon_modifier_definition_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), duration_ms INTEGER NOT NULL CHECK(typeof(duration_ms)='integer' AND duration_ms>0), reactivation INTEGER NOT NULL CHECK(typeof(reactivation)='integer' AND reactivation BETWEEN 0 AND 3), target_count INTEGER NOT NULL CHECK(typeof(target_count)='integer' AND target_count BETWEEN 0 AND 1024), PRIMARY KEY(revision_ref), UNIQUE(definition_ref,position)))sql",
    R"sql(CREATE TABLE subathon_modifier_targets(definition_revision_ref BLOB NOT NULL, ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 1024), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind BETWEEN 0 AND 2), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob' AND length(target_ref) BETWEEN 1 AND 4096), qualifier_ref BLOB CHECK(qualifier_ref IS NULL OR (typeof(qualifier_ref)='blob' AND length(qualifier_ref) BETWEEN 1 AND 4096)), factor_decimal BLOB NOT NULL CHECK(typeof(factor_decimal)='blob' AND length(factor_decimal) BETWEEN 1 AND 512), PRIMARY KEY(definition_revision_ref,ordinal), FOREIGN KEY(definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref)))sql",
    R"sql(CREATE TABLE subathon_modifier_instances(instance_ref BLOB NOT NULL CHECK(typeof(instance_ref)='blob' AND length(instance_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), lifetime INTEGER NOT NULL CHECK(typeof(lifetime)='integer' AND lifetime BETWEEN 0 AND 2), successor_instance_ref BLOB, CHECK((lifetime=1 AND typeof(successor_instance_ref)='blob' AND length(successor_instance_ref) BETWEEN 1 AND 4096 AND successor_instance_ref<>instance_ref) OR (lifetime IN (0,2) AND successor_instance_ref IS NULL)), PRIMARY KEY(instance_ref), FOREIGN KEY(successor_instance_ref) REFERENCES subathon_modifier_instances(instance_ref) DEFERRABLE INITIALLY DEFERRED))sql",
    R"sql(CREATE TABLE subathon_modifier_instance_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), instance_ref BLOB NOT NULL, position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), parent_revision_ref BLOB, captured_definition_revision_ref BLOB NOT NULL, applied_definition_revision_ref BLOB, remaining_ms INTEGER NOT NULL CHECK(typeof(remaining_ms)='integer' AND remaining_ms>=0), operation INTEGER NOT NULL CHECK(typeof(operation)='integer' AND operation BETWEEN 0 AND 1), source_kind INTEGER NOT NULL CHECK(typeof(source_kind)='integer' AND source_kind BETWEEN 0 AND 4), source_ref BLOB NOT NULL CHECK(typeof(source_ref)='blob' AND length(source_ref) BETWEEN 1 AND 4096), trigger_ref BLOB, trigger_timing INTEGER, CHECK((position=1 AND parent_revision_ref IS NULL AND operation=0) OR (position>1 AND typeof(parent_revision_ref)='blob')), CHECK((operation=0 AND typeof(applied_definition_revision_ref)='blob' AND remaining_ms>0) OR (operation=1 AND applied_definition_revision_ref IS NULL)), CHECK((trigger_ref IS NULL AND trigger_timing IS NULL) OR (typeof(trigger_ref)='blob' AND length(trigger_ref) BETWEEN 1 AND 4096 AND typeof(trigger_timing)='integer' AND trigger_timing IN (1,2))), PRIMARY KEY(revision_ref), UNIQUE(instance_ref,position), FOREIGN KEY(instance_ref) REFERENCES subathon_modifier_instances(instance_ref), FOREIGN KEY(parent_revision_ref) REFERENCES subathon_modifier_instance_revisions(revision_ref), FOREIGN KEY(captured_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref), FOREIGN KEY(applied_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref)))sql",
    R"sql(CREATE UNIQUE INDEX subathon_modifier_one_live ON subathon_modifier_instances(definition_ref) WHERE lifetime=0)sql",
};

inline MigrationManifest literal_modifier_manifest() {
    auto manifest = literal_decision_manifest();
    MigrationStep step{14, "modifier-activations-v1", modifier_ddl,
                       manifest.steps.back().expected_schema};
    step.expected_schema.append({
        {"table", "subathon_modifier_definition_revisions",
         "subathon_modifier_definition_revisions", modifier_ddl[0]},
        {"table", "subathon_modifier_targets", "subathon_modifier_targets", modifier_ddl[1]},
        {"table", "subathon_modifier_instances", "subathon_modifier_instances", modifier_ddl[2]},
        {"table", "subathon_modifier_instance_revisions", "subathon_modifier_instance_revisions",
         modifier_ddl[3]},
        {"index", "subathon_modifier_one_live", "subathon_modifier_instances", modifier_ddl[4]},
        {"index",
         "sqlite_autoindex_subathon_modifier_definition_revisions_1",
         "subathon_modifier_definition_revisions",
         {}},
        {"index",
         "sqlite_autoindex_subathon_modifier_definition_revisions_2",
         "subathon_modifier_definition_revisions",
         {}},
        {"index", "sqlite_autoindex_subathon_modifier_targets_1", "subathon_modifier_targets", {}},
        {"index",
         "sqlite_autoindex_subathon_modifier_instances_1",
         "subathon_modifier_instances",
         {}},
        {"index",
         "sqlite_autoindex_subathon_modifier_instance_revisions_1",
         "subathon_modifier_instance_revisions",
         {}},
        {"index",
         "sqlite_autoindex_subathon_modifier_instance_revisions_2",
         "subathon_modifier_instance_revisions",
         {}},
    });
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(14);
    return manifest;
}

// Prefix 1–8 digests are pinned by the frozen account-session vectors.
// Steps 9–14 were independently framed from the frozen additive projections
// and reviewed v14 DDL, with the catalog v1 UTF-8 length framing.
struct CatalogPin {
    const char *id;
    const char *digest;
};
inline constexpr std::array<CatalogPin, 14> catalog_pins{{
    {"retain-config-context-v1",
     "0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e"},
    {"identified-receipts-v1", "24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2"},
    {"contribution-evidence-v1",
     "b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d"},
    {"captured-application-plans-v1",
     "bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50"},
    {"atomic-application-bundles-v1",
     "c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb"},
    {"durable-application-holds-v1",
     "9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a"},
    {"reference-initial-captures-v1",
     "efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218"},
    {"account-sessions-v1", "ec41d4b3bf7d04a3e846b401060cc3ed9430020db3dfce0e2650ed76f6a46d5b"},
    {"permission-graph-v1", "a818324e2ff37a638fa00dcb89e3c4b89140fe7e09e1c6eb06d20bcc38be7f68"},
    {"reference-authority-v1", "d0c8f1348abbc3521293ee2d3af3fa75fbeb65c40e5a5589c3c1e9953e172315"},
    {"delivery-jobs-v1", "342ab385bd4b70d75ec5bbf16ef0b34d6173d5a4f41da539a85d1c6412cdfbc7"},
    {"milestone-groups-v1", "590881656952aa89350784c4de3ea50a7d002c3d1b00baf21f1e04aede36f18e"},
    {"milestone-group-decisions-v1",
     "06479f8418119285626522c9f4fa84096b28a594c617a017c8b842ddbd37f7de"},
    {"modifier-activations-v1", "a6ca951e91632dd7a2f5bb0f4028a95eb6b0397545cf942a8e34d690e65ea1c8"},
}};

inline QByteArray pinned_digest(qint32 version) {
    return QByteArray::fromHex(catalog_pins.at(static_cast<std::size_t>(version - 1)).digest);
}

inline constexpr auto history_sql =
    "SELECT version,migration_id,content_sha256 FROM main.subathon_migration_history "
    "ORDER BY version";
inline constexpr auto modifiers_schema_sql =
    "SELECT type,name,tbl_name,sql FROM main.sqlite_schema "
    "WHERE tbl_name GLOB 'subathon_modifier_*' ORDER BY type,name";

inline void reopen(File &file) {
    auto stopped = std::make_shared<bool>(false);
    require(file.writer->shutdown([stopped] { *stopped = true; }) == StorageError::none,
            "Writer accepts drain and close");
    require(QTest::qWaitFor([stopped] { return *stopped; }, timeout), "Writer closes after drain");
    file.reopen();
}

class ModifierFile final : public File {
  public:
    ModifierFile() : File(Schema::empty) {
        migrated(permission_graph_fixture::migrate(*writer, literal_modifier_manifest()), 14, 14);
    }
};

inline const core::ModifierDefinition old_definition{
    "D",
    "z-old",
    480000,
    {{core::ModifierTargetKind::added_time, "timer.main", "payment", "2.00"},
     {core::ModifierTargetKind::counter, "counter.synthetic", {}, "1.250"}},
    core::ModifierReactivation::extend};
inline const core::ModifierDefinition new_definition{
    "D",
    "a-new",
    600000,
    {{core::ModifierTargetKind::added_time, "timer.main", "payment", "0.50"}},
    core::ModifierReactivation::replace};
inline const core::ModifierDefinition independent_definition{
    "E", "e0", 1, {}, core::ModifierReactivation::extend};
inline const core::ModifierActivationSource moderator{core::ModifierActivationKind::moderator,
                                                      "operator-A"};
inline const core::ModifierActivationSource rule{core::ModifierActivationKind::rule, "rule-B"};
inline const core::ModifierTriggeringContribution subsequent{
    "event-B", core::ModifierTriggerTiming::subsequent_only};
inline const core::ModifierInstance initial_instance{"I",    "j0",      {}, old_definition,
                                                     480000, moderator, {}};
inline const core::ModifierInstance checkpointed_instance{"I",    "j1",      "j0", old_definition,
                                                          180000, moderator, {}};
inline const core::ModifierInstance k0{"J", "k0", {}, new_definition, 600000, rule, subsequent};
inline const core::ModifierInstance k1{"J", "k1", "k0", new_definition, 0, rule, subsequent};

inline constexpr auto instance_rows =
    "SELECT instance_ref,definition_ref,lifetime,successor_instance_ref "
    "FROM main.subathon_modifier_instances ORDER BY instance_ref";
inline constexpr auto revision_rows =
    "SELECT "
    "revision_ref,instance_ref,position,parent_revision_ref,captured_definition_revision_ref,"
    "applied_definition_revision_ref,remaining_ms,operation,source_kind,source_ref,trigger_ref,"
    "trigger_timing FROM main.subathon_modifier_instance_revisions ORDER BY instance_ref,position";

// Independent setup for cases that should reach activate/checkpoint without first
// depending on append_definition, or reach exact reads without any modifier writes.
inline void seed_old_live(const SqliteWorkSession &s) {
    (void)s.execute({"INSERT INTO main.subathon_modifier_definition_revisions "
                     "VALUES(X'7a2d6f6c64',X'44',1,480000,0,2)",
                     {}});
    (void)s.execute({"INSERT INTO main.subathon_modifier_targets VALUES "
                     "(X'7a2d6f6c64',1,0,?,?,?),(X'7a2d6f6c64',2,2,?,NULL,?)",
                     {QByteArray("timer.main"), QByteArray("payment"), QByteArray("2.00"),
                      QByteArray("counter.synthetic"), QByteArray("1.250")}});
    (void)s.execute(
        {"INSERT INTO main.subathon_modifier_instances VALUES(X'49',X'44',0,NULL)", {}});
    (void)s.execute({"INSERT INTO main.subathon_modifier_instance_revisions VALUES "
                     "(X'6a30',X'49',1,NULL,X'7a2d6f6c64',X'7a2d6f6c64',480000,0,0,?,NULL,NULL),"
                     "(X'6a31',X'49',2,X'6a30',X'7a2d6f6c64',NULL,180000,1,0,?,NULL,NULL)",
                     {QByteArray("operator-A"), QByteArray("operator-A")}});
}

inline void seed_definition_edit(const SqliteWorkSession &s) {
    (void)s.execute({"INSERT INTO main.subathon_modifier_definition_revisions "
                     "VALUES(X'612d6e6577',X'44',2,600000,2,1)",
                     {}});
    (void)s.execute({"INSERT INTO main.subathon_modifier_targets VALUES (X'612d6e6577',1,0,?,?,?)",
                     {QByteArray("timer.main"), QByteArray("payment"), QByteArray("0.50")}});
}

inline void seed_replacement(const SqliteWorkSession &s) {
    (void)s.execute({"UPDATE main.subathon_modifier_instances "
                     "SET lifetime=1,successor_instance_ref=X'4a' WHERE instance_ref=X'49'",
                     {}});
    (void)s.execute(
        {"INSERT INTO main.subathon_modifier_instances VALUES(X'4a',X'44',0,NULL)", {}});
    (void)s.execute({"INSERT INTO main.subathon_modifier_instance_revisions VALUES "
                     "(X'6b30',X'4a',1,NULL,X'612d6e6577',X'612d6e6577',600000,0,3,?,?,2)",
                     {QByteArray("rule-B"), QByteArray("event-B")}});
}

inline void seed_replaced(const SqliteWorkSession &s) {
    seed_old_live(s);
    seed_definition_edit(s);
    seed_replacement(s);
}
} // namespace modifier_activation_fixture
