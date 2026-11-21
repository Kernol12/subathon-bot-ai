// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../storage_026/common_fixture.hpp"

namespace storage_027_fixture {
namespace predecessor = storage_026_fixture;
using namespace contribution_review_fixture;

// Literal proposal for the disposable successor. The relative coordinate in
// fixture_manifest() is only used in the migration runner's disposable files.
inline constexpr auto fixture_marker = "milestone-slot-interruption-storage-v1";
struct Table {
    const char *name;
    const char *ddl;
    int indexes;
};
inline constexpr Table new_tables[]{
    {"subathon_milestone_command_journal", R"SQL(CREATE TABLE subathon_milestone_command_journal(
installation BLOB NOT NULL REFERENCES subathon_account_installation(installation_ref),
track BLOB NOT NULL CHECK(typeof(track)='blob' AND length(track) BETWEEN 1 AND 4096),
operation BLOB NOT NULL CHECK(typeof(operation)='blob' AND length(operation) BETWEEN 1 AND 4096),
kind INTEGER NOT NULL CHECK(kind IN (1,2,3)),
actor BLOB NOT NULL, lineage BLOB NOT NULL, decision BLOB NOT NULL,
intent BLOB NOT NULL, result BLOB NOT NULL,
PRIMARY KEY(installation,track,operation)))SQL",
     1},
    {"subathon_milestone_stop_fences", R"SQL(CREATE TABLE subathon_milestone_stop_fences(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL,
origin_kind INTEGER NOT NULL CHECK(origin_kind IN (1,2)),
origin_scope BLOB NOT NULL, origin_ref BLOB NOT NULL, occurrence BLOB NOT NULL,
choice INTEGER NOT NULL CHECK(choice IN (1,2)),
playback_cap INTEGER NOT NULL CHECK(typeof(playback_cap)='integer' AND playback_cap>=0),
PRIMARY KEY(installation,track,stop),
UNIQUE(installation,track,origin_kind,origin_scope,origin_ref,occurrence),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_command_journal(installation,track,operation)))SQL",
     2},
    {"subathon_milestone_slot_versions", R"SQL(CREATE TABLE subathon_milestone_slot_versions(
installation BLOB NOT NULL, track BLOB NOT NULL, replacement BLOB NOT NULL,
origin_kind INTEGER NOT NULL CHECK(origin_kind IN (1,2)),
origin_scope BLOB NOT NULL, origin_ref BLOB NOT NULL, occurrence BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0),
version BLOB NOT NULL, predecessor BLOB,
kind INTEGER NOT NULL CHECK(kind IN (1,2)), action_ref BLOB NOT NULL, action_version BLOB NOT NULL,
payload BLOB NOT NULL, route BLOB, qualified_sender BLOB,
PRIMARY KEY(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,version),
UNIQUE(installation,track,version),
UNIQUE(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,predecessor),
FOREIGN KEY(installation,track,replacement) REFERENCES subathon_milestone_command_journal(installation,track,operation),
FOREIGN KEY(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,predecessor)
REFERENCES subathon_milestone_slot_versions(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,version),
CHECK((kind=1 AND route IS NOT NULL AND qualified_sender IS NOT NULL) OR
(kind=2 AND route IS NULL AND qualified_sender IS NULL))))SQL",
     3},
    {"subathon_milestone_slot_attempts", R"SQL(CREATE TABLE subathon_milestone_slot_attempts(
installation BLOB NOT NULL, track BLOB NOT NULL, version BLOB NOT NULL,
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
state INTEGER NOT NULL, evidence BLOB,
PRIMARY KEY(installation,track,version,revision),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     1},
    {"subathon_milestone_version_bonus", R"SQL(CREATE TABLE subathon_milestone_version_bonus(
installation BLOB NOT NULL, track BLOB NOT NULL, version BLOB NOT NULL,
effect BLOB NOT NULL, actual INTEGER NOT NULL CHECK(typeof(actual)='integer'),
PRIMARY KEY(installation,track,version),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     1},
    {"subathon_milestone_stop_cancellations",
     R"SQL(CREATE TABLE subathon_milestone_stop_cancellations(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), version BLOB,
PRIMARY KEY(installation,track,stop,ordinal),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_stop_fences(installation,track,stop),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     1},
    {"subathon_milestone_interruption_causes",
     R"SQL(CREATE TABLE subathon_milestone_interruption_causes(
installation BLOB NOT NULL, track BLOB NOT NULL, cause BLOB NOT NULL, stop BLOB NOT NULL,
resume BLOB,
PRIMARY KEY(installation,track,cause), UNIQUE(installation,track,stop),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_stop_fences(installation,track,stop),
FOREIGN KEY(installation,track,resume) REFERENCES subathon_milestone_command_journal(installation,track,operation)))SQL",
     2},
    {"subathon_milestone_playback_instances",
     R"SQL(CREATE TABLE subathon_milestone_playback_instances(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL,
instance BLOB NOT NULL, ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0),
version BLOB, adapter BLOB NOT NULL, attempt BLOB, activation BLOB NOT NULL, generation BLOB NOT NULL,
capability INTEGER NOT NULL, handoff BLOB,
PRIMARY KEY(installation,track,stop,instance),
UNIQUE(installation,track,stop,ordinal,adapter,activation,generation),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_stop_fences(installation,track,stop),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     2},
    {"subathon_milestone_playback_evidence",
     R"SQL(CREATE TABLE subathon_milestone_playback_evidence(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL, instance BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), outcome INTEGER NOT NULL,
evidence BLOB NOT NULL,
PRIMARY KEY(installation,track,stop,instance,ordinal),
FOREIGN KEY(installation,track,stop,instance) REFERENCES subathon_milestone_playback_instances(installation,track,stop,instance)))SQL",
     1},
};

inline constexpr qsizetype predecessor_objects = 253;
inline constexpr qsizetype predecessor_manifest_bytes = 1'594'845;
inline constexpr qsizetype predecessor_projection_bytes = 140'840;
// SHA-256 of the 20 ordered 32-byte canonical step hashes retained by
// protected #760. This binds earlier ordered SQL even when its effects cancel.
inline constexpr auto predecessor_prefix_digest =
    "200ea0acbb5fa5d9c6fde591cb04a16e72ec5a320a41e1ea67b6cc25a9032ea7";
inline constexpr auto predecessor_digest =
    "eb1758529cd052df4f8ed80ab693a6219afa519ae9db5f1b00f2f6bae05bf7e1";

inline MigrationManifest fixture_manifest() {
    auto manifest = predecessor::successor();
    const auto &protected_head = manifest.steps.back();
    QCryptographicHash prefix(QCryptographicHash::Sha256);
    for (const auto &step : manifest.steps)
        prefix.addData(digest(step));
    const QList<qint32> starting_versions{0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                                          11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    require(protected_head.version == 20 && protected_head.sql.size() == 14 &&
                protected_head.expected_schema.size() == predecessor_objects &&
                digest(protected_head).toHex() == predecessor_digest &&
                tally(manifest) == predecessor_manifest_bytes &&
                prefix.result().toHex() == predecessor_prefix_digest &&
                manifest.supported_starting_versions == starting_versions,
            "Independent protected predecessor framing and full repeated projection");
    MigrationStep next{
        protected_head.version + 1, fixture_marker, {}, protected_head.expected_schema};
    for (const auto &table : new_tables) {
        const auto name = QString::fromLatin1(table.name);
        const auto ddl = QString::fromLatin1(table.ddl);
        next.sql.append(ddl);
        next.expected_schema.append({"table", name, name, ddl});
        for (int i = 1; i <= table.indexes; ++i)
            next.expected_schema.append(
                {"index", "sqlite_autoindex_" + name + "_" + QString::number(i), name, {}});
    }
    std::ranges::sort(next.expected_schema, schema_order);
    manifest.supported_starting_versions.append(next.version);
    manifest.steps.append(std::move(next));
    return manifest;
}

inline void expect_same_manifest(const MigrationManifest &actual,
                                 const MigrationManifest &expected) {
    require(actual.catalog_format == expected.catalog_format &&
                actual.store_identity == expected.store_identity &&
                actual.supported_starting_versions == expected.supported_starting_versions &&
                actual.steps.size() == expected.steps.size(),
            "Exact supported starts and catalog frame");
    for (qsizetype i = 0; i < expected.steps.size(); ++i) {
        const auto &got = actual.steps[i];
        const auto &want = expected.steps[i];
        require(got.version == want.version && got.id == want.id && got.sql == want.sql &&
                    got.expected_schema == want.expected_schema,
                "Exact version, marker, ordered SQL and sorted physical objects");
    }
}

} // namespace storage_027_fixture
