// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_v23_rebuild.hpp"

#include "sqlite_catalog_inspection.hpp"
#include "sqlite_clock_checkpoint.hpp"
#include "sqlite_connection.hpp"
#include "sqlite_v22_catalog.hpp"
#include "v23_executor/connection_pragmas.hpp"
#include "v23_executor/copy_comparison.hpp"
#include "v23_executor/step_data.hpp"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QSqlRecord>
#include <QStorageInfo>
#include <QThread>
#include <algorithm>
#include <array>
#include <limits>

namespace subathon::server::detail {
namespace {

QString text(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

QByteArray hex_bytes(std::string_view hex) {
    return QByteArray::fromHex(QByteArray(hex.data(), static_cast<qsizetype>(hex.size())));
}

// The exact compiled v22 manifest followed by the sealed v23 step. Each digest
// and the 23-step prefix must equal the sealed literals, or the recipe refuses.
struct CompiledV23 {
    MigrationManifest manifest;
    QList<QByteArray> hashes;
};

std::variant<CompiledV23, MigrationReason> compile_v23() {
    auto compiled = clock_checkpoint_manifest();
    if (const auto *reason = std::get_if<MigrationReason>(&compiled))
        return *reason;
    CompiledV23 result{std::move(std::get<MigrationManifest>(compiled)), {}};
    auto &manifest = result.manifest;
    const auto &v22 = sqlite_v22_catalog();
    const auto &v23 = sqlite_v23_step_data();
    if (manifest.steps.size() != static_cast<qsizetype>(v22.steps.size()) ||
        v23.version != manifest.steps.size() + 1)
        return MigrationReason::invalid_manifest;

    MigrationStep step{v23.version, text(v23.id), {}, {}};
    for (const auto statement : v23.statements)
        step.sql.append(text(statement));
    for (const auto &object : v23.projection)
        step.expected_schema.append({text(object.type), text(object.name), text(object.table_name),
                                     object.sql ? std::optional{text(*object.sql)} : std::nullopt});
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(v23.version);

    QCryptographicHash prefix(QCryptographicHash::Sha256);
    for (qsizetype i = 0; i < manifest.steps.size(); ++i) {
        auto &current = manifest.steps[i];
        auto digest = migration_content_sha256(current);
        const auto sealed = i < static_cast<qsizetype>(v22.steps.size())
                                ? v22.steps[static_cast<std::size_t>(i)].content_sha256_hex
                                : v23.step_sha256_hex;
        if (digest != hex_bytes(sealed))
            return MigrationReason::invalid_manifest;
        prefix.addData(digest);
        result.hashes.append(std::move(digest));
        // The shared inspector compares the canonical projection order.
        const auto ordered = encoded_objects(current.expected_schema);
        current.expected_schema.clear();
        for (const auto &entry : ordered)
            current.expected_schema.append(entry.object);
    }
    if (prefix.result() != hex_bytes(v23.prefix_sha256_hex))
        return MigrationReason::invalid_manifest;
    return result;
}

const std::variant<CompiledV23, MigrationReason> &compiled_v23() {
    static const auto compiled = compile_v23();
    return compiled;
}

// The stopped caller supplies a stable database. An immutable QSQLITE
// connection cannot create WAL/SHM or recover a journal, so refuse files
// that may need recovery before letting SQLite read the main file alone.
bool stopped_sidecars(const QString &path) {
    const QFileInfo journal(path + "-journal");
    const QFileInfo wal(path + "-wal");
    const QFileInfo shm(path + "-shm");
    return !journal.isSymLink() && !journal.exists() && !wal.isSymLink() &&
           (!wal.exists() || (wal.isFile() && wal.size() == 0)) && !shm.isSymLink() &&
           (!shm.exists() || shm.isFile());
}

class V23Inspection final {
  public:
    explicit V23Inspection(StorageLimits limits)
        : remaining_statements_(std::min(limits.statements_per_batch, ceilings.statements_per_batch)),
          statement_bytes_(std::min(limits.statement_utf8_bytes, ceilings.statement_utf8_bytes)),
          remaining_rows_(std::min(limits.result_rows, ceilings.result_rows)),
          remaining_bytes_(std::min(limits.result_value_bytes, ceilings.result_value_bytes)) {}

    std::variant<V23PreOpenObservation, MigrationReason> run(const QString &path,
                                                           const CompiledV23 &compiled) {
        // Use the reviewed r8 whole-store fixture extent for the full scans.
        constexpr qint64 max_store_bytes = 16 * 1024 * 1024;
        const QFileInfo file(path);
        const auto canonical = file.canonicalFilePath();
        if (!file.isFile() || canonical.isEmpty() || !stopped_sidecars(canonical))
            return MigrationReason::inspection_failed;
        if (file.size() > max_store_bytes)
            return MigrationReason::resource_limit;
        if (!connection_.open_immutable(canonical))
            return MigrationReason::inspection_failed;

        check_control("PRAGMA foreign_keys=ON");
        if (!pragma_equals(connection_, "PRAGMA foreign_keys", 1))
            return MigrationReason::inspection_failed;
        check_control("BEGIN DEFERRED");
        const auto extent = read("SELECT page_count, page_size FROM pragma_page_count(), "
                                 "pragma_page_size()");
        if (extent.size() != 1 || extent.front().size() != 2 ||
            !integer_value(extent.front()[0]) || !integer_value(extent.front()[1]))
            return MigrationReason::inspection_failed;
        const auto pages = extent.front()[0].toLongLong();
        const auto page_size = extent.front()[1].toLongLong();
        if (pages < 0 || page_size <= 0)
            return MigrationReason::inspection_failed;
        if (pages > max_store_bytes / page_size)
            return MigrationReason::resource_limit;

        const auto query = [this](ClockProbeQuery, const QString &sql) { return read(sql); };
        const auto version = inspect_store(compiled.manifest, compiled.hashes, query);
        check_control("ROLLBACK");
        connection_.close();
        if (!stopped_sidecars(canonical))
            return MigrationReason::inspection_failed;
        const auto &step = compiled.manifest.steps.back();
        if (version != step.version)
            return MigrationReason::unsupported_version;
        return V23PreOpenObservation{version, hex_bytes(sqlite_v23_step_data().prefix_sha256_hex),
                                     step.expected_schema.size()};
    }

  private:
    static constexpr StorageLimits ceilings{};
    Connection connection_;
    qsizetype remaining_statements_;
    const qsizetype statement_bytes_;
    qsizetype remaining_rows_;
    qsizetype remaining_bytes_;

    static void check_error(StorageError error) {
        if (error != StorageError::none)
            throw MigrationFailure(error == StorageError::busy ? MigrationReason::busy
                                                              : MigrationReason::inspection_failed);
    }

    void check_control(const QString &sql) { check_error(control(connection_, sql)); }

    SqlRows read(const QString &sql) {
        if (remaining_statements_ <= 0 || sql.toUtf8().size() > statement_bytes_)
            throw MigrationFailure(MigrationReason::resource_limit);
        --remaining_statements_;
        QSqlQuery query(connection_.database());
        query.setForwardOnly(true);
        if (!query.prepare(sql) || !query.exec())
            check_error(sql_error(query.lastError()));

        SqlRows rows;
        const int columns = query.record().count();
        while (query.next()) {
            if (remaining_rows_ <= 0)
                throw MigrationFailure(MigrationReason::resource_limit);
            --remaining_rows_;
            SqlRow row;
            for (int column = 0; column < columns; ++column) {
                auto value = query.value(column);
                if (!consume_value(remaining_bytes_, value))
                    throw MigrationFailure(MigrationReason::resource_limit);
                row.append(std::move(value));
            }
            rows.append(std::move(row));
        }
        if (query.lastError().isValid())
            check_error(sql_error(query.lastError()));
        return rows;
    }
};

// Copy charge per parent row under r8: rowid and the legacy tag at eight bytes,
// retained INTEGER/REAL at eight bytes, TEXT/BLOB octets and zero for NULL.
// The 34 retained columns are those copied by the sealed statement 3.
constexpr std::array<const char *, 34> retained_columns{
    "plan_ref",
    "revision",
    "parent_revision",
    "payment_evidence",
    "monetary",
    "original_decimal",
    "currency",
    "resolution_ref",
    "missing_rate",
    "review",
    "evidence_policy",
    "policy_ref",
    "decision_choice",
    "decision_ref",
    "decision_actor_ref",
    "application",
    "reversal",
    "settlement",
    "valuation_state",
    "historical_resolution_ref",
    "historical_original_decimal",
    "historical_currency",
    "comparison_currency",
    "effective_rate_decimal",
    "fetched_rate_decimal",
    "snapshot_ref",
    "rate_source_ref",
    "rate_timestamp_utc_ms",
    "fetch_timestamp_utc_ms",
    "manual_decision_ref",
    "override_authority",
    "override_actor_ref",
    "override_reason",
    "override_decision_ref",
};

// Counts at most 1,025 rows so the 1,024-row cap is decided without a full scan.
const QString &row_charge_sql() {
    static const QString sql = [] {
        QString charge = "16";
        for (const auto *column : retained_columns)
            charge += QString("+CASE typeof(%1) WHEN 'integer' THEN 8 WHEN 'real' THEN 8 "
                              "WHEN 'null' THEN 0 ELSE length(CAST(%1 AS BLOB)) END")
                          .arg(column);
        return "SELECT count(*),coalesce(max(b),0),coalesce(sum(b),0) FROM (SELECT " + charge +
               " AS b FROM main.subathon_plan_revisions LIMIT 1025)";
    }();
    return sql;
}

std::optional<std::uint64_t> integer(const QVariant &value) {
    const auto type = value.metaType().id();
    if (value.isNull() || (type != QMetaType::LongLong && type != QMetaType::Int) ||
        value.toLongLong() < 0)
        return std::nullopt;
    return static_cast<std::uint64_t>(value.toLongLong());
}

MigrationReason storage_reason(StorageError error, MigrationReason otherwise) {
    switch (error) {
    case StorageError::busy:
        return MigrationReason::busy;
    case StorageError::result_limit:
        return MigrationReason::resource_limit;
    default:
        return otherwise;
    }
}

WorkError work_error(MigrationReason reason) {
    switch (reason) {
    case MigrationReason::busy:
        return WorkError::busy;
    case MigrationReason::resource_limit:
        return WorkError::resource_limit;
    case MigrationReason::uncertain_outcome:
        return WorkError::uncertain_outcome;
    default:
        return WorkError::work_exception;
    }
}

// One fenced v23 window on the writer's owned connection. Settings are entered
// in r3 order and restored in reverse with a readback for each, per 609.9.
class V23Rebuild final {
  public:
    V23Rebuild(V23OwnerConnection &connection, const CompiledV23 &compiled, V23RebuildBudget budget,
               const V23Cancellation &cancel, MigrationResult &result)
        : connection_(connection), compiled_(compiled), budget_(budget), cancel_(cancel),
          result_(result) {}

    V23OwnerState run() {
        try {
            read_original_settings();
            admit_resources();
        } catch (const MigrationFailure &failure) {
            return refuse(failure.reason());
        }
        try {
            enter_window();
            probe(MigrationCheckpoint::before_begin, compiled_version(), 0);
        } catch (const MigrationFailure &failure) {
            return refuse(failure.reason());
        }
        const auto begun = connection_.transaction(V23OwnerConnection::Transaction::begin);
        if (begun.error != StorageError::none)
            return refuse(storage_reason(begun.error, MigrationReason::sql_failure));
        try {
            rebuild();
        } catch (const MigrationFailure &failure) {
            return roll_back(failure.reason());
        }
        const auto committed = connection_.transaction(V23OwnerConnection::Transaction::commit);
        if (!committed.acknowledged)
            return uncertain();
        if (committed.error != StorageError::none)
            return roll_back(storage_reason(committed.error, MigrationReason::sql_failure));
        if (!restore()) {
            finish(TransactionOutcome::committed, MigrationReason::committed_restore_failed);
            return V23OwnerState::retire;
        }
        const auto version = compiled_version();
        const MigrationReport report{CatalogState::current, version, version, 1};
        result_.work = {TransactionOutcome::committed, WorkError::none,
                        QVariantList{static_cast<int>(report.state), report.version,
                                     report.target_version,
                                     static_cast<qlonglong>(report.applied)}};
        result_.reason = MigrationReason::none;
        result_.report = report;
        return V23OwnerState::committed_v23;
    }

  private:
    V23OwnerConnection &connection_;
    const CompiledV23 &compiled_;
    V23RebuildBudget budget_;
    const V23Cancellation &cancel_;
    MigrationResult &result_;
    // Originals read before any change. A changed flag is set before its
    // setter runs, so a failed setter or readback is still restored.
    qint64 spill_{};
    qint64 temp_{};
    bool journal_changed_{};
    bool spill_changed_{};
    bool temp_changed_{};
    bool foreign_keys_changed_{};

    qint32 compiled_version() const { return static_cast<qint32>(compiled_.manifest.steps.size()); }

    void finish(TransactionOutcome outcome, MigrationReason reason) {
        result_.work = {outcome, work_error(reason), std::nullopt};
        result_.reason = reason;
        result_.report.reset();
    }

    V23OwnerState refuse(MigrationReason reason) {
        finish(TransactionOutcome::not_started, reason);
        return restore() ? V23OwnerState::normal : V23OwnerState::retire;
    }

    V23OwnerState roll_back(MigrationReason reason) {
        const auto rolled = connection_.transaction(V23OwnerConnection::Transaction::rollback);
        if (!rolled.acknowledged || rolled.error != StorageError::none)
            return uncertain();
        // A proved rollback keeps its primary reason even if restoration fails.
        finish(TransactionOutcome::rolled_back, reason);
        return restore() ? V23OwnerState::normal : V23OwnerState::retire;
    }

    V23OwnerState uncertain() {
        finish(TransactionOutcome::outcome_unknown, MigrationReason::uncertain_outcome);
        return V23OwnerState::retire;
    }

    SqlRows read(const QString &sql, MigrationReason reason) {
        auto result = connection_.read(sql);
        if (result.error != StorageError::none)
            throw MigrationFailure(storage_reason(result.error, reason));
        return std::move(result.rows);
    }

    std::optional<QVariant> scalar(const char *sql) {
        const auto result = connection_.read(sql);
        if (result.error != StorageError::none || result.rows.size() != 1 ||
            result.rows.front().size() != 1)
            return std::nullopt;
        return result.rows.front().front();
    }

    bool integer_equals(const char *sql, qint64 expected) {
        const auto value = scalar(sql);
        return value && integer(*value) && static_cast<qint64>(*integer(*value)) == expected;
    }

    bool text_equals(const char *sql, const char *expected) {
        const auto value = scalar(sql);
        return value && value->metaType().id() == QMetaType::QString &&
               value->toString() == QLatin1StringView(expected);
    }

    // A setter must succeed and return no rows.
    bool silent(const char *sql) {
        const auto result = connection_.read(sql);
        return result.error == StorageError::none && result.rows.isEmpty();
    }

    void read_original_settings() {
        const auto spill = scalar(v23_pragma::cache_spill);
        const auto temp = scalar(v23_pragma::temp_store);
        if (!text_equals(v23_pragma::journal_mode, "wal") || !spill || !integer(*spill) || !temp ||
            !integer(*temp) || *integer(*temp) > 2 || !integer_equals(v23_pragma::foreign_keys, 1))
            throw MigrationFailure(MigrationReason::inspection_failed);
        spill_ = static_cast<qint64>(*integer(*spill));
        temp_ = static_cast<qint64>(*integer(*temp));
        const auto attached = read("SELECT count(*) FROM pragma_database_list "
                                   "WHERE name NOT IN ('main','temp')",
                                   MigrationReason::inspection_failed);
        if (attached.size() != 1 || attached.front().size() != 1 ||
            integer(attached.front().front()) != std::optional<std::uint64_t>{0})
            throw MigrationFailure(MigrationReason::inspection_failed);
    }

    // Disk and memory are decided together before the first setting change.
    void admit_resources() {
        V23FixtureFacts facts{};
        const auto page_count = scalar("PRAGMA main.page_count");
        const auto page_size = scalar("PRAGMA main.page_size");
        const auto auto_vacuum = scalar("PRAGMA main.auto_vacuum");
        const auto rows = read(row_charge_sql(), MigrationReason::inspection_failed);
        if (!page_count || !integer(*page_count) || !page_size || !integer(*page_size) ||
            !auto_vacuum || !integer(*auto_vacuum) || rows.size() != 1 ||
            rows.front().size() != 3 || !integer(rows.front()[0]) || !integer(rows.front()[1]) ||
            !integer(rows.front()[2]))
            throw MigrationFailure(MigrationReason::resource_limit);
        facts.page_count = *integer(*page_count);
        facts.page_size = *integer(*page_size);
        facts.auto_vacuum = *integer(*auto_vacuum);
        facts.row_count = *integer(rows.front()[0]);
        facts.largest_row_bytes = *integer(rows.front()[1]);
        facts.retained_bytes = *integer(rows.front()[2]);

        // Every extant sidecar of the main file is charged, including SHM.
        const auto path = connection_.path();
        for (const auto *suffix : {"-wal", "-shm", "-journal"}) {
            const QFileInfo sidecar(path + suffix);
            if (!sidecar.exists())
                continue;
            if (sidecar.size() < 0 ||
                static_cast<std::uint64_t>(sidecar.size()) >
                    std::numeric_limits<std::uint64_t>::max() - facts.sidecar_bytes)
                throw MigrationFailure(MigrationReason::resource_limit);
            facts.sidecar_bytes += static_cast<std::uint64_t>(sidecar.size());
        }
        const QStorageInfo volume(QFileInfo(path).absolutePath());
        if (volume.isValid() && volume.isReady() && volume.bytesAvailable() >= 0)
            facts.free_bytes = static_cast<std::uint64_t>(volume.bytesAvailable());
        if (!admit_v23_fixture_bounds(facts, budget_))
            throw MigrationFailure(MigrationReason::resource_limit);
    }

    // r3 entry order: DELETE, spill OFF, TEMP MEMORY, FK OFF, each read back.
    void enter_window() {
        const auto unverified = [] { throw MigrationFailure(MigrationReason::sql_failure); };
        journal_changed_ = true;
        const auto journal = connection_.read(v23_pragma::journal_delete);
        if (journal.error != StorageError::none || journal.rows.size() != 1 ||
            journal.rows.front().size() != 1 ||
            journal.rows.front().front().toString() != QLatin1StringView("delete") ||
            !text_equals(v23_pragma::journal_mode, "delete"))
            unverified();
        if (!integer_equals("SELECT sqlite_compileoption_used('TEMP_STORE=1') OR "
                            "sqlite_compileoption_used('TEMP_STORE=2') OR "
                            "sqlite_compileoption_used('TEMP_STORE=3')",
                            1))
            unverified();
        if (spill_ != 0) {
            spill_changed_ = true;
            if (!silent(v23_pragma::spill_off) || !integer_equals(v23_pragma::cache_spill, 0))
                unverified();
        }
        if (!silent(v23_pragma::temp_schema_probe))
            unverified();
        if (temp_ != 2) {
            temp_changed_ = true;
            if (!silent(v23_pragma::temp_memory) || !integer_equals(v23_pragma::temp_store, 2))
                unverified();
        }
        foreign_keys_changed_ = true;
        if (!silent(v23_pragma::foreign_keys_off) || !integer_equals(v23_pragma::foreign_keys, 0))
            unverified();
    }

    // Reverse order, outside any transaction. Stops at the first failure.
    bool restore() {
        if (foreign_keys_changed_) {
            if (!silent(v23_pragma::foreign_keys_on) ||
                !integer_equals(v23_pragma::foreign_keys, 1))
                return false;
            foreign_keys_changed_ = false;
        }
        if (temp_changed_) {
            if (!silent(v23_pragma::temp_schema_probe) ||
                !silent(temp_ == 0 ? v23_pragma::temp_default : v23_pragma::temp_file) ||
                !integer_equals(v23_pragma::temp_store, temp_))
                return false;
            temp_changed_ = false;
        }
        if (spill_changed_) {
            if (!silent(v23_pragma::spill_on) || !integer_equals(v23_pragma::cache_spill, spill_))
                return false;
            spill_changed_ = false;
        }
        if (journal_changed_) {
            const auto journal = connection_.read(v23_pragma::journal_wal);
            if (journal.error != StorageError::none || journal.rows.size() != 1 ||
                journal.rows.front().size() != 1 ||
                journal.rows.front().front().toString() != QLatin1StringView("wal") ||
                !text_equals(v23_pragma::journal_mode, "wal"))
                return false;
            journal_changed_ = false;
        }
        return true;
    }

    void probe(MigrationCheckpoint checkpoint, qint32 version, qsizetype statement) {
        if (!cancel_)
            return;
        bool cancelled;
        try {
            cancelled = cancel_({checkpoint, version, statement});
        } catch (...) {
            throw MigrationFailure(MigrationReason::callback_failure);
        }
        if (cancelled)
            throw MigrationFailure(MigrationReason::cancelled);
    }

    qint32 inspect() {
        const auto query = [this](ClockProbeQuery, const QString &sql) {
            return read(sql, MigrationReason::inspection_failed);
        };
        return inspect_store(compiled_.manifest, compiled_.hashes, query);
    }

    // Inside BEGIN IMMEDIATE: recheck the exact v22 head under the fence, run
    // the five sealed statements with the copy comparison between copy and
    // drop, verify the v23 projection, append history and verify the head.
    void rebuild() {
        const auto version = compiled_version();
        if (inspect() != version - 1)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        const auto &step = compiled_.manifest.steps.back();
        for (qsizetype i = 0; i < step.sql.size(); ++i) {
            probe(MigrationCheckpoint::before_statement, version, i + 1);
            read(step.sql[i], MigrationReason::sql_failure);
            if (i == 2)
                require_v23_copy_match([this](std::string_view sql) {
                    return read(text(sql), MigrationReason::inspection_failed);
                });
        }
        probe(MigrationCheckpoint::before_history, version, step.sql.size());
        const auto query = [this](ClockProbeQuery, const QString &sql) {
            return read(sql, MigrationReason::inspection_failed);
        };
        verify_catalog_schema(read_catalog_schema(query), step.expected_schema);
        read(QString("INSERT INTO main.subathon_migration_history"
                     "(version,migration_id,content_sha256) VALUES (%1,'%2',X'%3')")
                 .arg(version)
                 .arg(step.id)
                 .arg(QString::fromLatin1(compiled_.hashes.back().toHex())),
             MigrationReason::sql_failure);
        if (inspect() != version)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        probe(MigrationCheckpoint::before_commit, version, 0);
    }
};

} // namespace

std::variant<V23PreOpenObservation, MigrationReason> inspect_v23_pre_open(const QString &path,
                                                                         StorageLimits limits) {
    try {
        const auto &compiled = compiled_v23();
        if (const auto *reason = std::get_if<MigrationReason>(&compiled))
            return *reason;
        return V23Inspection(limits).run(path, std::get<CompiledV23>(compiled));
    } catch (const MigrationFailure &failure) {
        // All queries unwind before the connection closes its read transaction.
        return failure.reason();
    }
}

std::variant<V23PreOpenObservation, MigrationReason>
V23InspectionAccess::inspect_with_limits(const QString &path, StorageLimits limits) {
    return inspect_v23_pre_open(path, limits);
}

MigrationAdmission enqueue_v23_rebuild(V22PreOpenWitness witness, Ops02UpdateHold hold,
                                       V23StoppedWriterLease lease, V23RebuildBudget budget,
                                       V23Cancellation cancel,
                                       SqliteMigrationRunner::Completion completion) {
    return V23RebuildAccess::enqueue(std::move(witness), std::move(hold), std::move(lease), budget,
                                     std::move(cancel), std::move(completion));
}

MigrationAdmission V23RebuildAccess::enqueue(V22PreOpenWitness witness, Ops02UpdateHold hold,
                                             V23StoppedWriterLease lease, V23RebuildBudget budget,
                                             V23Cancellation cancel,
                                             SqliteMigrationRunner::Completion completion) {
    const MigrationAdmission invalid{StorageError::invalid_options,
                                     MigrationReason::invalid_manifest};
    const auto &identity = sqlite_v22_catalog();
    // Check the owner thread before locking the generation so a foreign call
    // cannot keep the marker alive while its writer is being destroyed.
    if (!lease.reservation_ || QThread::currentThread() != lease.owner_)
        return invalid;
    const auto generation = lease.generation_.lock();
    if (!hold.fixture_hold_ || !generation || generation->writer->thread() != generation->owner ||
        witness.version != static_cast<qint32>(identity.steps.size()) ||
        witness.projection_objects != identity.projection_objects || witness.prefix.size() != 32 ||
        witness.prefix.toHex() !=
            QByteArrayView(identity.prefix_sha256_hex.data(), identity.prefix_sha256_hex.size()))
        return invalid;
    if (!completion)
        return {StorageError::invalid_work, MigrationReason::none};
    if (budget.disk_bytes.value_or(v23_fixture_disk_bytes) > v23_fixture_disk_bytes ||
        budget.memory_bytes.value_or(v23_fixture_memory_bytes) > v23_fixture_memory_bytes)
        return invalid;
    const auto *compiled = std::get_if<CompiledV23>(&compiled_v23());
    if (!compiled)
        return invalid;

    // The job writes the outcome; a skipped job reports an unavailable writer.
    // The hold stays alive until the outcome has been delivered.
    struct Shared {
        Ops02UpdateHold hold;
        MigrationResult result{{TransactionOutcome::not_started, WorkError::writer_unavailable, {}},
                               MigrationReason::writer_unavailable,
                               {}};
    };
    auto shared = std::make_shared<Shared>();
    shared->hold = std::move(hold);
    auto job = [shared, compiled, budget, cancel = std::move(cancel)](V23OwnerConnection &owner) {
        return V23Rebuild(owner, *compiled, budget, cancel, shared->result).run();
    };
    auto deliver = [shared, completion = std::move(completion)] {
        auto result = std::move(shared->result);
        completion(std::move(result));
    };
    if (submit(std::move(lease), std::move(job), std::move(deliver)) != StorageError::none)
        return invalid;
    return {};
}

} // namespace subathon::server::detail
