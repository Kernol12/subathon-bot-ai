// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_credential_catalog_consumers.hpp"
#include <subathon/core/valuation_snapshot_codec.hpp>
#include <subathon/server/sqlite_rate_overrides.hpp>

namespace rate_fixture {
using namespace subathon::server;
using namespace credential_catalog_fixture;
namespace core = subathon::core;

inline constexpr qsizetype v16_manifest_bytes = 1065558;
inline constexpr qsizetype v17_manifest_bytes = 1169071;
inline constexpr qsizetype v17_projection_objects = 175;
inline constexpr qsizetype v17_framed_bytes = 105862;
inline const auto v17_digest =
    QByteArray::fromHex("b14cfdd3a3f69b84d884c4a274d9f4d14389b958726ad5b500e648fddac6783f");
inline const QString rate_marker = "rate-overrides-v1";
inline const QByteArray rsv_format = "subathon.internal.valuation-snapshot.v1";
inline const QByteArray rin_format = "subathon.internal.rate-override-intent.v1";
inline const QByteArray rdc_format = "subathon.internal.rate-override-decision.v1";
inline const QByteArray rob_format = "subathon.internal.rate-observation.v1";
inline const QByteArray root_bytes =
    QByteArray::fromHex("52535631 0000000145 00 00 00000000 00000000 00000000 00000000");

// Literal physical proposal. Immutability is enforced by the typed repository;
// disposable corruption setup below can still inject well-typed inconsistent rows.
inline const QStringList rate_ddl{
    "CREATE TABLE subathon_rate_observations("
    "observation_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(observation_ref)='blob' AND "
    "length(observation_ref) BETWEEN 1 AND 256),"
    "format BLOB NOT NULL CHECK(typeof(format)='blob' AND length(format) BETWEEN 1 AND 128),"
    "document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 4 AND "
    "32768))",
    "CREATE TABLE subathon_rate_revisions("
    "snapshot_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(snapshot_ref)='blob' AND "
    "length(snapshot_ref) BETWEEN 1 AND 256),"
    "parent_ref BLOB UNIQUE REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(parent_ref IS NULL OR (typeof(parent_ref)='blob' AND length(parent_ref) BETWEEN 1 AND "
    "256)),"
    "format BLOB NOT NULL CHECK(typeof(format)='blob' AND length(format) BETWEEN 1 AND 128),"
    "document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 4 AND "
    "32768),"
    "CHECK((snapshot_ref=X'45' AND parent_ref IS NULL) OR (snapshot_ref<>X'45' AND parent_ref IS "
    "NOT NULL AND snapshot_ref<>parent_ref)))",
    "CREATE TABLE subathon_rate_decisions("
    "decision_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(decision_ref)='blob' AND "
    "length(decision_ref) BETWEEN 1 AND 256),"
    "installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND "
    "length(installation_ref) BETWEEN 1 AND 4096),"
    "operation_key BLOB NOT NULL CHECK(typeof(operation_key)='blob' AND length(operation_key) "
    "BETWEEN 1 AND 256),"
    "base_ref BLOB NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(base_ref)='blob' AND length(base_ref) BETWEEN 1 AND 256),"
    "successor_ref BLOB UNIQUE NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(successor_ref)='blob' AND length(successor_ref) BETWEEN 1 AND 256),"
    "intent_format BLOB NOT NULL CHECK(typeof(intent_format)='blob' AND length(intent_format) "
    "BETWEEN 1 AND 128),"
    "intent BLOB NOT NULL CHECK(typeof(intent)='blob' AND length(intent) BETWEEN 4 AND 229376),"
    "decision_format BLOB NOT NULL CHECK(typeof(decision_format)='blob' AND "
    "length(decision_format) BETWEEN 1 AND 128),"
    "decision BLOB NOT NULL CHECK(typeof(decision)='blob' AND length(decision) BETWEEN 4 AND "
    "32768),"
    "UNIQUE(installation_ref,operation_key), CHECK(base_ref<>successor_ref))",
    "CREATE TABLE subathon_rate_occurrences("
    "override_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(override_ref)='blob' AND "
    "length(override_ref) BETWEEN 1 AND 256),"
    "decision_ref BLOB UNIQUE NOT NULL REFERENCES subathon_rate_decisions(decision_ref) "
    "CHECK(typeof(decision_ref)='blob' AND length(decision_ref) BETWEEN 1 AND 256),"
    "revision_ref BLOB NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 256),"
    "body BLOB NOT NULL CHECK(typeof(body)='blob' AND length(body) BETWEEN 1 AND 5535))",
    "CREATE TABLE subathon_rate_head("
    "slot BLOB PRIMARY KEY NOT NULL CHECK(typeof(slot)='blob' AND slot=X'48'),"
    "revision_ref BLOB NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 256))",
};

inline MigrationManifest literal_v17() {
    auto manifest = literal_v16();
    MigrationStep step{17, rate_marker, rate_ddl, manifest.steps.back().expected_schema};
    const QStringList names{"subathon_rate_observations", "subathon_rate_revisions",
                            "subathon_rate_decisions", "subathon_rate_occurrences",
                            "subathon_rate_head"};
    const QList<int> indexes{1, 2, 3, 2, 1};
    for (qsizetype i = 0; i < names.size(); ++i) {
        const auto &name = names[i];
        step.expected_schema.append({"table", name, name, rate_ddl[i]});
        for (int index = 1; index <= indexes[i]; ++index)
            step.expected_schema.append(
                {"index", "sqlite_autoindex_" + name + '_' + QString::number(index), name, {}});
    }
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    step.sql.append(
        "INSERT INTO subathon_rate_revisions VALUES("
        "X'45',NULL,X'"
        "7375626174686f6e2e696e7465726e616c2e76616c756174696f6e2d736e617073686f742e7631',"
        "X'525356310000000145000000000000000000000000000000000000')");
    step.sql.append("INSERT INTO subathon_rate_head VALUES(X'48',X'45')");
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(17);
    return manifest;
}

inline qsizetype manifest_bytes(const MigrationManifest &manifest) {
    qsizetype bytes = 0;
    const auto charge = [&](const QString &text) { bytes += text.toUtf8().size(); };
    for (const auto &step : manifest.steps) {
        for (const auto &statement : step.sql)
            charge(statement);
        for (const auto &object : step.expected_schema) {
            charge(object.type);
            charge(object.name);
            charge(object.table_name);
            if (object.sql)
                charge(*object.sql);
        }
    }
    return bytes;
}

inline MigrationResult run_catalog(SqliteWriter &writer, MigrationMode mode,
                                   MigrationManifest manifest, qsizetype budget,
                                   std::function<bool(const MigrationProgress &)> cancel = {}) {
    MigrationLimits limits;
    limits.manifest_utf8_bytes = budget;
    return finance::completed<MigrationResult>([&](auto done) {
        return SqliteMigrationRunner::enqueue(writer, {mode, std::move(manifest), limits, cancel},
                                              std::move(done))
            .error;
    });
}

inline QByteArray blob(std::string_view bytes) {
    return QByteArray(bytes.data(), qsizetype(bytes.size()));
}
inline QByteArray snapshot_bytes(const core::EffectiveRateSnapshotInput &input) {
    return blob(
        core::encode_valuation_snapshot(core::make_effective_rate_snapshot(input)).document);
}
inline QByteArray snapshot_bytes(const core::EffectiveRateSnapshot &snapshot) {
    return blob(core::encode_valuation_snapshot(snapshot).document);
}
inline void expect_reserved_root(const SqlRow &row) {
    require(
        row.size() == 4 && row[0].metaType().id() == QMetaType::QByteArray &&
            row[0].toByteArray() == blob("E") && row[1].isNull() &&
            row[2].metaType().id() == QMetaType::QByteArray && row[2].toByteArray() == rsv_format &&
            row[3].metaType().id() == QMetaType::QByteArray && row[3].toByteArray() == root_bytes,
        "Committed root is the exact E/NULL/RSV1-format/RSV1-body row");
}

inline constexpr auto revision_rows = "SELECT snapshot_ref,parent_ref,format,document FROM "
                                      "subathon_rate_revisions ORDER BY snapshot_ref";
inline constexpr auto observation_rows = "SELECT observation_ref,format,document FROM "
                                         "subathon_rate_observations ORDER BY observation_ref";
inline constexpr auto decision_rows =
    "SELECT "
    "decision_ref,installation_ref,operation_key,base_ref,successor_ref,intent_format,intent,"
    "decision_format,decision FROM subathon_rate_decisions ORDER BY decision_ref";
inline constexpr auto occurrence_rows = "SELECT override_ref,decision_ref,revision_ref,body FROM "
                                        "subathon_rate_occurrences ORDER BY override_ref";
inline constexpr auto head_rows = "SELECT slot,revision_ref FROM subathon_rate_head";

inline auto rate_image(const auto &file) {
    return std::array{file.rows(revision_rows), file.rows(observation_rows),
                      file.rows(decision_rows), file.rows(occurrence_rows), file.rows(head_rows)};
}
inline void receive_literal_v17(auto &file) {
    auto manifest = literal_v17();
    const auto budget = manifest_bytes(manifest);
    require(budget == v17_manifest_bytes,
            "Measured cumulative manifest including all repeated projections");
    expect_report(run_catalog(*file.writer, MigrationMode::advance, std::move(manifest), budget),
                  CatalogState::current, 17, 17, 1);
}
} // namespace rate_fixture
