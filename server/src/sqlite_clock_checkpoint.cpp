// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_clock_checkpoint.hpp"

#include "sqlite_connection.hpp"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QSqlRecord>
#include <algorithm>
#include <limits>
#include <subathon/server/sqlite_milestone_interruption.hpp>

namespace subathon::server::detail {

namespace {
using Disposition = ClockPreOpenResult::Disposition;

struct ClockCatalogDdl {
    const char *type;
    const char *name;
    const char *table;
    const char *sql;
};

// The declared order and all SQL bytes are part of the migration digest.
constexpr ClockCatalogDdl clock_catalog_ddl[]{
    {"index", "subathon_app_head_checkpoint_pair", "subathon_app_head",
     "CREATE UNIQUE INDEX subathon_app_head_checkpoint_pair ON subathon_app_head(singleton COLLATE "
     "BINARY,token COLLATE BINARY)"},
    {"index", "subathon_app_state_checkpoint_pair", "subathon_app_states",
     "CREATE UNIQUE INDEX subathon_app_state_checkpoint_pair ON subathon_app_states(token COLLATE "
     "BINARY,configuration_ref COLLATE BINARY)"},
    {"table", "subathon_clock_evidence", "subathon_clock_evidence",
     "CREATE TABLE subathon_clock_evidence(revision INTEGER NOT NULL "
     "CHECK(typeof(revision)='integer' AND "
     "revision>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=0), kind "
     "INTEGER "
     "NOT NULL CHECK(typeof(kind)='integer' AND kind BETWEEN 0 AND 7), installation_ref BLOB NOT "
     "NULL "
     "CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096), "
     "timer_ref "
     "BLOB NOT NULL CHECK(typeof(timer_ref)='blob' AND length(timer_ref) BETWEEN 1 AND 4096), "
     "lineage_ref "
     "BLOB NOT NULL CHECK(typeof(lineage_ref)='blob' AND length(lineage_ref) BETWEEN 1 AND 4096), "
     "base_token BLOB NOT NULL CHECK(typeof(base_token)='blob' AND length(base_token) BETWEEN 1 "
     "AND "
     "4096), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND "
     "length(configuration_ref) BETWEEN 1 AND 4096), ledger_revision INTEGER NOT NULL "
     "CHECK(typeof(ledger_revision)='integer' AND ledger_revision>=0), operation_scope BLOB, "
     "operation_ref BLOB, episode_ref BLOB CHECK(episode_ref IS NULL OR "
     "(typeof(episode_ref)='blob' AND "
     "length(episode_ref) BETWEEN 1 AND 4096)), schedule_id BLOB, nominal_ms INTEGER, fold_run "
     "INTEGER, "
     "member_count INTEGER, link_revision INTEGER, link_ordinal INTEGER, page_no INTEGER, "
     "format_version "
     "INTEGER NOT NULL CHECK(typeof(format_version)='integer' AND format_version=1), document BLOB "
     "NOT "
     "NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 262144), PRIMARY "
     "KEY(revision,ordinal), FOREIGN KEY(base_token,configuration_ref) REFERENCES "
     "subathon_app_states(token,configuration_ref), FOREIGN KEY(link_revision,link_ordinal) "
     "REFERENCES "
     "subathon_clock_evidence(revision,ordinal), CHECK((kind=0 AND ordinal=0 AND "
     "typeof(member_count)='integer' AND member_count>0 AND typeof(operation_scope)='blob' AND "
     "length(operation_scope) BETWEEN 1 AND 4096 AND typeof(operation_ref)='blob' AND "
     "length(operation_ref) BETWEEN 1 AND 4096) OR (kind<>0 AND ordinal>0 AND member_count IS NULL "
     "AND "
     "operation_scope IS NULL AND operation_ref IS NULL)), CHECK((kind IN (1,2) AND episode_ref IS "
     "NOT "
     "NULL) OR (kind NOT IN (1,2) AND episode_ref IS NULL)), CHECK((kind=3 AND "
     "typeof(schedule_id)='blob' "
     "AND length(schedule_id) BETWEEN 1 AND 4096 AND typeof(nominal_ms)='integer' AND "
     "typeof(fold_run)='integer' AND fold_run IN (0,1)) OR (kind<>3 AND schedule_id IS NULL AND "
     "nominal_ms IS NULL AND fold_run IS NULL)), CHECK((kind IN (2,4,6,7) AND "
     "typeof(link_revision)='integer' AND typeof(link_ordinal)='integer' AND "
     "(link_revision<revision OR "
     "(link_revision=revision AND link_ordinal<ordinal))) OR (kind NOT IN (2,4,6,7) AND "
     "link_revision IS "
     "NULL AND link_ordinal IS NULL)), CHECK((kind=7 AND typeof(page_no)='integer' AND page_no>0) "
     "OR "
     "(kind<>7 AND page_no IS NULL)))"},
    {"index", "subathon_clock_operation_once", "subathon_clock_evidence",
     "CREATE UNIQUE INDEX subathon_clock_operation_once ON "
     "subathon_clock_evidence(installation_ref,operation_scope,operation_ref) WHERE kind=0"},
    {"index", "subathon_clock_episode_once", "subathon_clock_evidence",
     "CREATE UNIQUE INDEX subathon_clock_episode_once ON "
     "subathon_clock_evidence(installation_ref,episode_ref,kind) WHERE kind IN (1,2)"},
    {"index", "subathon_clock_nominal_once", "subathon_clock_evidence",
     "CREATE UNIQUE INDEX subathon_clock_nominal_once ON "
     "subathon_clock_evidence(installation_ref,timer_ref,lineage_ref,schedule_id,nominal_ms,fold_"
     "run) "
     "WHERE kind=3"},
    {"index", "subathon_clock_close_once", "subathon_clock_evidence",
     "CREATE UNIQUE INDEX subathon_clock_close_once ON "
     "subathon_clock_evidence(link_revision,link_ordinal) WHERE kind IN (4,6)"},
    {"index", "subathon_clock_page_once", "subathon_clock_evidence",
     "CREATE UNIQUE INDEX subathon_clock_page_once ON "
     "subathon_clock_evidence(link_revision,link_ordinal,page_no) WHERE kind=7"},
    {"index", "subathon_clock_ledger", "subathon_clock_evidence",
     "CREATE INDEX subathon_clock_ledger ON "
     "subathon_clock_evidence(installation_ref,timer_ref,lineage_ref,ledger_revision,revision,"
     "ordinal) "
     "WHERE kind IN (3,4,5,6)"},
    {"table", "subathon_clock_checkpoint", "subathon_clock_checkpoint",
     "CREATE TABLE subathon_clock_checkpoint(singleton INTEGER PRIMARY KEY CHECK(singleton=1), "
     "base_token "
     "BLOB NOT NULL CHECK(typeof(base_token)='blob' AND length(base_token) BETWEEN 1 AND 4096), "
     "configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND "
     "length(configuration_ref) BETWEEN 1 AND 4096), format_version INTEGER NOT NULL "
     "CHECK(typeof(format_version)='integer' AND format_version=1), generation INTEGER NOT NULL "
     "CHECK(typeof(generation)='integer' AND generation>0), revision INTEGER NOT NULL "
     "CHECK(typeof(revision)='integer' AND revision>0 AND revision<=generation), root_ordinal "
     "INTEGER NOT "
     "NULL CHECK(root_ordinal=0), pending_revision INTEGER, pending_ordinal INTEGER, main_ms "
     "INTEGER NOT "
     "NULL CHECK(typeof(main_ms)='integer' AND main_ms>=0), grace_ms INTEGER NOT NULL "
     "CHECK(typeof(grace_ms)='integer' AND grace_ms>=0), cadence_ms INTEGER NOT NULL "
     "CHECK(typeof(cadence_ms)='integer' AND cadence_ms>0), ledger_revision INTEGER NOT NULL "
     "CHECK(typeof(ledger_revision)='integer' AND ledger_revision>=0), document BLOB NOT NULL "
     "CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 65536), FOREIGN "
     "KEY(singleton,base_token) REFERENCES subathon_app_head(singleton,token) ON UPDATE NO ACTION "
     "ON "
     "DELETE NO ACTION DEFERRABLE INITIALLY DEFERRED, FOREIGN KEY(base_token,configuration_ref) "
     "REFERENCES subathon_app_states(token,configuration_ref), FOREIGN KEY(revision,root_ordinal) "
     "REFERENCES subathon_clock_evidence(revision,ordinal), FOREIGN "
     "KEY(pending_revision,pending_ordinal) "
     "REFERENCES subathon_clock_evidence(revision,ordinal), CHECK((pending_revision IS NULL AND "
     "pending_ordinal IS NULL) OR (typeof(pending_revision)='integer' AND pending_revision>0 AND "
     "pending_revision<=revision AND typeof(pending_ordinal)='integer' AND pending_ordinal>0)))"},
    {"trigger", "subathon_clock_head_no_delete", "subathon_app_head",
     "CREATE TRIGGER subathon_clock_head_no_delete BEFORE DELETE ON subathon_app_head BEGIN SELECT "
     "RAISE(ABORT,'checkpoint head delete'); END"},
    {"trigger", "subathon_clock_head_insert", "subathon_app_head",
     "CREATE TRIGGER subathon_clock_head_insert BEFORE INSERT ON subathon_app_head WHEN "
     "EXISTS(SELECT 1 "
     "FROM subathon_app_head) OR NOT EXISTS(SELECT 1 FROM subathon_clock_checkpoint WHERE "
     "singleton=NEW.singleton AND base_token=NEW.token) BEGIN SELECT RAISE(ABORT,'checkpoint head "
     "insert'); END"},
    {"trigger", "subathon_clock_head_update", "subathon_app_head",
     "CREATE TRIGGER subathon_clock_head_update BEFORE UPDATE ON subathon_app_head WHEN NOT "
     "EXISTS(SELECT "
     "1 FROM subathon_clock_checkpoint WHERE singleton=OLD.singleton) BEGIN SELECT "
     "RAISE(ABORT,'checkpoint head update without slot'); END"},
    {"trigger", "subathon_clock_slot_no_delete", "subathon_clock_checkpoint",
     "CREATE TRIGGER subathon_clock_slot_no_delete BEFORE DELETE ON subathon_clock_checkpoint "
     "BEGIN "
     "SELECT RAISE(ABORT,'checkpoint slot delete'); END"},
    {"trigger", "subathon_clock_slot_insert", "subathon_clock_checkpoint",
     "CREATE TRIGGER subathon_clock_slot_insert BEFORE INSERT ON subathon_clock_checkpoint WHEN "
     "EXISTS(SELECT 1 FROM subathon_clock_checkpoint) BEGIN SELECT RAISE(ABORT,'checkpoint slot "
     "insert'); "
     "END"},
};

[[noreturn]] void inspection_error(StorageError error) {
    throw MigrationFailure(error == StorageError::busy ? MigrationReason::busy
                                                       : MigrationReason::inspection_failed);
}

class ClockProbe final {
  public:
    explicit ClockProbe(StorageLimits limits) : limits_(limits) {}

    ClockPreOpenResult run(const QString &owned_path) {
        try {
            const QFileInfo file(owned_path);
            if (!file.isFile() || !connection_.open(file.absoluteFilePath(), true))
                inspection_error(StorageError::sql_failure);
            // These connection controls are outside the prepared inspection ledger.
            check_control("PRAGMA foreign_keys=ON");
            if (!pragma_equals(connection_, "PRAGMA foreign_keys", 1))
                inspection_error(StorageError::sql_failure);
            check_control("BEGIN DEFERRED");
            const ClockInspectionQuery execute_query =
                [this](ClockProbeQuery query, const QString &sql) { return execute(query, sql); };
            const auto version = inspect_clock_predecessor_catalog(
                milestone_slot_interruption_repository_manifest(), execute_query);
            if (version != 0 && version != clock_predecessor_version)
                throw MigrationFailure(MigrationReason::unsupported_version);
            auto witness = read_clock_predecessor(version, execute_query);
            check_control("ROLLBACK");
            const auto disposition = witness.predecessor == ClockPredecessor::active
                                         ? Disposition::active_refused
                                         : Disposition::candidate;
            return {disposition, MigrationReason::none, std::move(witness), terminal_, ledger_};
        } catch (const MigrationFailure &failure) {
            // Connection teardown ends any remaining read transaction.
            return {Disposition::unavailable, failure.reason(), std::nullopt, terminal_,
                    terminal_ ? std::optional{ledger_} : std::nullopt};
        }
    }

  private:
    Connection connection_;
    StorageLimits limits_;
    std::optional<ClockProbeQuery> terminal_;
    ClockStatementLedger ledger_{};

    void check_control(const QString &sql) {
        if (const auto error = control(connection_, sql); error != StorageError::none)
            inspection_error(error);
    }

    SqlRows execute(ClockProbeQuery kind, const QString &sql) {
        // A refusal before prepare still identifies the query it blocked.
        terminal_ = kind;
        const auto width = sql.toUtf8().size();
        if (ledger_.attempts >= limits_.statements_per_batch ||
            width > limits_.statement_utf8_bytes ||
            width > std::numeric_limits<qsizetype>::max() - ledger_.total_sql)
            throw MigrationFailure(MigrationReason::resource_limit);

        QSqlQuery query(connection_.database());
        query.setForwardOnly(true);
        ++ledger_.attempts;
        ledger_.peak_sql = std::max(ledger_.peak_sql, width);
        ledger_.total_sql += width;
        if (!query.prepare(sql) || !query.exec())
            inspection_error(sql_error(query.lastError()));

        SqlRows rows;
        while (query.next()) {
            const auto columns = query.record().count();
            if (ledger_.rows >= limits_.result_rows ||
                columns > std::numeric_limits<qsizetype>::max() - ledger_.cells ||
                limits_.result_value_bytes < ledger_.result_bytes)
                throw MigrationFailure(MigrationReason::resource_limit);

            auto remaining = limits_.result_value_bytes - ledger_.result_bytes;
            SqlRow row;
            for (int column = 0; column < columns; ++column) {
                auto value = query.value(column);
                if (!consume_value(remaining, value))
                    throw MigrationFailure(MigrationReason::resource_limit);
                row.append(std::move(value));
            }
            // Admit a row and its complete value charge together.
            ++ledger_.rows;
            ledger_.cells += columns;
            ledger_.result_bytes = limits_.result_value_bytes - remaining;
            rows.append(std::move(row));
        }
        if (query.lastError().isValid())
            inspection_error(sql_error(query.lastError()));
        return rows;
    }
};
} // namespace

std::variant<MigrationManifest, MigrationReason> clock_checkpoint_manifest() {
    auto manifest = milestone_slot_interruption_repository_manifest();
    if (manifest.steps.size() != clock_predecessor_version ||
        manifest.steps.back().expected_schema.size() != 276 ||
        manifest.supported_starting_versions.size() != clock_predecessor_version + 1 ||
        manifest.supported_starting_versions.front() != 0 || manifest.catalog_format != 1 ||
        manifest.store_identity != "subathon.sqlite-store")
        return MigrationReason::invalid_manifest;

    MigrationStep step{
        clock_predecessor_version + 1, "clock-checkpoints-v1", {}, manifest.steps.back().expected_schema};
    for (const auto &object : clock_catalog_ddl) {
        const auto sql = QString::fromUtf8(object.sql);
        step.sql.append(sql);
        step.expected_schema.append({QString::fromLatin1(object.type),
                                     QString::fromLatin1(object.name),
                                     QString::fromLatin1(object.table), sql});
    }
    step.expected_schema.append({"index", "sqlite_autoindex_subathon_clock_evidence_1",
                                 "subathon_clock_evidence", std::nullopt});

    try {
        const auto digest = migration_content_sha256(step);
        if (digest.toHex() != "2c8b634d2ed28a81f949e8a31e7d9de991bf513f4f5000194b1eaafb725d27bf")
            return MigrationReason::invalid_manifest;
        QCryptographicHash prefix(QCryptographicHash::Sha256);
        for (qsizetype i = 0; i < manifest.steps.size(); ++i) {
            if (manifest.supported_starting_versions[i + 1] != i + 1)
                return MigrationReason::invalid_manifest;
            prefix.addData(migration_content_sha256(manifest.steps[i]));
        }
        prefix.addData(digest);
        if (prefix.result().toHex() !=
            "382a5e86e3b6f4b2e2b8a1f3aa73f7e8f20f3e24413a7fb50d6f5a7ea6f8e3ff")
            return MigrationReason::invalid_manifest;
    } catch (const MigrationFailure &failure) {
        return failure.reason();
    }
    manifest.supported_starting_versions.append(step.version);
    manifest.steps.append(std::move(step));
    return manifest;
}

ClockPreOpenResult probe_clock_checkpoint_predecessor(const QString &owned_path,
                                                      StorageLimits limits) {
    return ClockProbe(limits).run(owned_path);
}

#ifdef SUBATHON_STORAGE_003_CONSTRAINT_TEST_ACCESS
std::optional<ClockFixtureWriterTrace> clock_checkpoint_fixture_trace(const SqliteWriter &) {
    return std::nullopt;
}
#endif

MigrationAdmission enqueue_clock_checkpoint_advance(SqliteWriter &writer, MigrationRequest request,
                                                    const ClockPreOpenWitness &witness,
                                                    ClockAdvanceCompletion completion) {
    const auto invalid = MigrationAdmission{StorageError::invalid_options,
                                            MigrationReason::invalid_manifest};
    if (request.mode != MigrationMode::advance)
        return invalid;
    const bool empty_basis = witness.token.isEmpty() && witness.configuration.isEmpty();
    switch (witness.predecessor) {
    case ClockPredecessor::blank:
        if (witness.version != 0 || !empty_basis)
            return invalid;
        break;
    case ClockPredecessor::no_head:
        if (witness.version != clock_predecessor_version || !empty_basis)
            return invalid;
        break;
    case ClockPredecessor::ready:
    case ClockPredecessor::finished:
        if (witness.version != clock_predecessor_version || witness.token.isEmpty() ||
            witness.token.size() > 4096 || witness.configuration.isEmpty() ||
            witness.configuration.size() > 4096)
            return invalid;
        break;
    default:
        return invalid;
    }

    auto compiled = clock_checkpoint_manifest();
    auto *manifest = std::get_if<MigrationManifest>(&compiled);
    const auto &supplied = request.manifest;
    if (!manifest || supplied.catalog_format != manifest->catalog_format ||
        supplied.store_identity != manifest->store_identity ||
        supplied.supported_starting_versions != manifest->supported_starting_versions ||
        !std::ranges::equal(supplied.steps, manifest->steps, [](const auto &left, const auto &right) {
            return left.version == right.version && left.id == right.id &&
                   left.sql == right.sql && left.expected_schema == right.expected_schema;
        }))
        return invalid;

    // Queue the compiled values and an owned snapshot of the untrusted basis.
    request.manifest = std::move(*manifest);
    return ClockAdvanceExecution::enqueue(
        writer, std::move(request),
        {witness.version, witness.predecessor,
         QByteArray(witness.token.constData(), witness.token.size()),
         QByteArray(witness.configuration.constData(), witness.configuration.size())},
        std::move(completion));
}

} // namespace subathon::server::detail
