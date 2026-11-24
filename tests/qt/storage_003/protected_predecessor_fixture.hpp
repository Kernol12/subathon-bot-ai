// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "catalog_oracle.hpp"
#include "structural_fixture.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTemporaryDir>
#include <memory>
#include <subathon/server/sqlite_milestone_interruption.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

namespace storage_003_fixture {

class DisposableFile final {
  public:
    DisposableFile() { require(directory_.isValid(), "Fixture owns a disposable directory"); }
    QString path() const { return directory_.filePath("store.sqlite"); }

    void create_blank() const {
        QFile file(path());
        require(file.open(QIODevice::WriteOnly | QIODevice::NewOnly), "Create empty stopped file");
    }

    QByteArray seal() const {
        require(QFileInfo(path() + "-wal").size() == 0,
                "Stopped file has no outstanding WAL bytes");
        QFile file(path());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        require(file.open(QIODevice::ReadOnly) && hash.addData(&file), "Seal the stopped file");
        return hash.result();
    }

    void copy_to(const DisposableFile &copy) const {
        const auto original = seal();
        require(QFile::copy(path(), copy.path()) && copy.seal() == original && seal() == original,
                "Detached copy matches the stopped owner-produced file byte for byte");
    }

  private:
    QTemporaryDir directory_;
};

inline constexpr auto select_state =
    "SELECT token,format_ref,document,configuration_ref,lifecycle,phase,main_remaining_ms,"
    "grace_remaining_ms,awaiting_confirmation,time_debt_ms,balance_count "
    "FROM subathon_app_states ORDER BY token";
inline constexpr auto select_head = "SELECT singleton,token FROM subathon_app_head";
inline constexpr auto select_configuration =
    "SELECT reference,document FROM subathon_retained_configurations";
inline constexpr auto select_history =
    "SELECT version,migration_id,content_sha256 FROM subathon_migration_history ORDER BY version";

inline const SqlRows initial_head{{integer(1), blob("A")}};
inline const SqlRows initial_configuration{{blob("K"), blob("configuration")}};

inline SqlRows expected_history(qint32 count) {
    SqlRows rows;
    for (qint32 i = 0; i < count; ++i)
        rows.append(SqlRow{integer(i + 1), QString::fromLatin1(step_ids[i]),
                           blob(QByteArray::fromHex(step_digests[i]))});
    return rows;
}

inline detail::ClockPreOpenResult probe_stopped(const DisposableFile &file, StorageLimits limits) {
    const auto before = file.seal();
    const auto probe = detail::probe_clock_checkpoint_predecessor(file.path(), limits);
    require(file.seal() == before, "Read-only pre-open leaves the stopped main file unchanged");
    return probe;
}

inline void expect_probe_ledger(const detail::ClockPreOpenResult &probe, const Charge &expected,
                                detail::ClockProbeQuery terminal_query) {
    require(probe.terminal_query == terminal_query && probe.attained == expected,
            "Exact terminal probe query and independently fixed eight-field attained ledger");
}

inline detail::ClockPreOpenWitness candidate(const DisposableFile &file,
                                             const detail::ClockPreOpenWitness &expected) {
    using Query = detail::ClockProbeQuery;
    using Disposition = detail::ClockPreOpenResult::Disposition;
    const bool blank = expected.predecessor == detail::ClockPredecessor::blank;
    const auto &charge = blank ? blank_probe : ready_probe;
    const auto result = probe_stopped(file, profile(charge));
    require(result.disposition == Disposition::candidate &&
                result.reason == MigrationReason::none && result.witness == expected,
            QString("Read-only pre-open candidate and exact basis, reason %1")
                .arg(static_cast<int>(result.reason)));
    expect_probe_ledger(result, charge, blank ? Query::foreign_keys : Query::legacy_guard);
    return *result.witness;
}

inline void verify_manifest(const MigrationManifest &manifest, qint32 end) {
    require(manifest.catalog_format == 1 && manifest.store_identity == "subathon.sqlite-store" &&
                manifest.steps.size() == end &&
                manifest.supported_starting_versions.size() == end + 1,
            "Exact format, identity and counts");
    QCryptographicHash prefix(QCryptographicHash::Sha256);
    require(manifest.supported_starting_versions.front() == 0, "Start zero is explicitly admitted");
    for (qint32 version = 1; version <= end; ++version) {
        const auto &step = manifest.steps[version - 1];
        require(step.version == version && manifest.supported_starting_versions[version] == version,
                "Every supported start and step is ordered and contiguous");
        const auto digest = migration_content_sha256(step);
        require(digest.toHex() == step_digests[version - 1],
                "Every ordered statement and full typed projection hashes to the independent seal");
        prefix.addData(digest);
    }
    const auto &v21 = manifest.steps[20];
    require(v21.expected_schema.size() == 276 && v21.sql.size() == 9 &&
                prefix.result().toHex() == (end == 21 ? prefix21 : prefix22),
            "Protected v21 object count and exact raw-digest prefix");
    if (end == 22) {
        const auto &v22 = manifest.steps[21];
        require(v22.id == "clock-checkpoints-v1" && v22.sql.size() == 15 &&
                    v22.expected_schema.size() == 292 &&
                    canonical_migration_payload(v22).size() == 165839,
                "Exact v22 identity, statement and object counts and canonical byte size");
        for (qsizetype i = 0; i < v22.sql.size(); ++i)
            require(v22.sql[i].toUtf8() == successor_sql[i], "Exact ordered v22 statement bytes");
        require(v22.expected_schema.contains(
                    {QStringLiteral("index"),
                     QStringLiteral("sqlite_autoindex_subathon_clock_evidence_1"),
                     QStringLiteral("subathon_clock_evidence"), std::nullopt}),
                "Implicit evidence index projects genuine SQL NULL");
    }
}

inline MigrationManifest compiled_successor() {
    auto outcome = detail::clock_checkpoint_manifest();
    if (auto reason = std::get_if<MigrationReason>(&outcome))
        require(false, QString("First stop: private compiled v22 catalog, migration reason %1")
                           .arg(static_cast<int>(*reason)));
    auto manifest = std::get<MigrationManifest>(std::move(outcome));
    verify_manifest(manifest, 22);
    return manifest;
}

inline MigrationRequest request(MigrationMode mode, MigrationManifest manifest) {
    MigrationLimits limits;
    limits.schema_objects = manifest.steps.back().expected_schema.size();
    return {mode, std::move(manifest), limits, {}};
}

template <typename Result, typename Enqueue> Result collect_migration(Enqueue enqueue) {
    const auto result = std::make_shared<std::optional<Result>>();
    const auto admission = enqueue([result](auto value) { *result = std::move(value); });
    require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
            QString("Migration admission, storage error %1, migration reason %2")
                .arg(static_cast<int>(admission.error))
                .arg(static_cast<int>(admission.reason)));
    require(QTest::qWaitFor([result] { return result->has_value(); }, 10000),
            "Real migration runner completes");
    return std::move(result->value());
}

inline MigrationResult migrate(SqliteWriter &writer, MigrationRequest request) {
    return collect_migration<MigrationResult>([&](auto done) {
        return SqliteMigrationRunner::enqueue(writer, std::move(request), std::move(done));
    });
}

inline detail::ClockAdvanceResult advance(SqliteWriter &writer, MigrationRequest request,
                                          const detail::ClockPreOpenWitness &witness) {
    return collect_migration<detail::ClockAdvanceResult>([&](auto done) {
        return detail::enqueue_clock_checkpoint_advance(writer, std::move(request), witness,
                                                        std::move(done));
    });
}

inline void expect_current(const MigrationResult &r, qint32 version, qsizetype applied) {
    expect_commit(r.work);
    require(r.reason == MigrationReason::none && r.report &&
                r.report->state == CatalogState::current && r.report->version == version &&
                r.report->target_version == version && r.report->applied == applied,
            "Normal committed exact catalog report");
}

inline void expect_migration_rollback(const MigrationResult &r, MigrationReason reason,
                                      const QString &diagnostic = {}) {
    require(r.reason == reason && !r.report && !r.work.value &&
                r.work.transaction == TransactionOutcome::rolled_back,
            QString("Exact refusal with acknowledged rollback and no report or value") +
                diagnostic);
}

inline const QList<qsizetype> v22_statement_order{1, 2,  3,  4,  5,  6,  7, 8,
                                                  9, 10, 11, 12, 13, 14, 15};

struct V22Progress {
    QList<std::pair<MigrationCheckpoint, qsizetype>> events;

    QList<qsizetype> statements() const {
        QList<qsizetype> result;
        for (const auto &[checkpoint, ordinal] : events)
            if (checkpoint == MigrationCheckpoint::before_statement)
                result.append(ordinal);
        return result;
    }

    bool reached_history() const {
        return events.contains({MigrationCheckpoint::before_history, 15});
    }

    bool completed_in_order() const {
        QList<std::pair<MigrationCheckpoint, qsizetype>> expected;
        for (const auto ordinal : v22_statement_order)
            expected.append({MigrationCheckpoint::before_statement, ordinal});
        expected.append({MigrationCheckpoint::before_history, 15});
        expected.append({MigrationCheckpoint::before_commit, 0});
        return events == expected;
    }

    QString diagnose(const MigrationResult &result) const {
        const auto seen = statements();
        QStringList ordinals;
        for (const auto ordinal : seen)
            ordinals.append(QString::number(ordinal));
        const auto reason = result.reason == MigrationReason::sql_failure
                                ? QString("sql_failure")
                                : QString::number(static_cast<int>(result.reason));
        const auto error = result.work.error == WorkError::invalid_statement
                               ? QString("invalid_statement")
                               : QString::number(static_cast<int>(result.work.error));
        const auto settlement = result.work.transaction == TransactionOutcome::rolled_back
                                    ? QString("rolled_back")
                                    : QString::number(static_cast<int>(result.work.transaction));
        return QString(" [v22 statements=%1; last=%2; before_history(22,15)=%3; "
                       "before_commit(22,0)=%4; reason=%5; work=%6; settlement=%7; "
                       "report=%8; value=%9]")
            .arg(ordinals.join(','))
            .arg(seen.isEmpty() ? 0 : seen.back())
            .arg(reached_history() ? "yes" : "no")
            .arg(events.contains({MigrationCheckpoint::before_commit, 0}) ? "yes" : "no")
            .arg(reason, error, settlement)
            .arg(result.report ? "yes" : "no")
            .arg(result.work.value ? "yes" : "no");
    }
};

inline auto observe_v22(MigrationRequest &request, bool cancel_at_history = false) {
    const auto observed = std::make_shared<V22Progress>();
    request.cancel = [observed, cancel_at_history](const MigrationProgress &progress) {
        if (progress.version != 22)
            return false;
        observed->events.append({progress.checkpoint, progress.statement});
        return cancel_at_history && progress.checkpoint == MigrationCheckpoint::before_history &&
               progress.statement == 15;
    };
    return observed;
}

inline void expect_guarded_v22_advance(SqliteWriter &writer, const MigrationManifest &manifest,
                                       const detail::ClockPreOpenWitness &witness) {
    auto command = request(MigrationMode::advance, manifest);
    const auto observed = observe_v22(command);
    const auto result = advance(writer, std::move(command), witness);
    const auto &migration = result.migration;
    require(
        observed->completed_in_order() && result.refusal == detail::ClockAdvanceRefusal::none &&
            migration.reason == MigrationReason::none &&
            migration.work.transaction == TransactionOutcome::committed &&
            migration.work.error == WorkError::none && migration.work.value && migration.report &&
            migration.report->state == CatalogState::current && migration.report->version == 22 &&
            migration.report->target_version == 22 &&
            migration.report->applied == 22 - witness.version,
        "Expected v22 statements 1..15, before_history and before_commit with committed report" +
            observed->diagnose(migration));
    expect_current(migration, 22, 22 - witness.version);
}

inline void expect_state(const ApplicationStateSnapshot &got, bool is_ready) {
    using namespace subathon::core;
    require(got.token == "A" && got.configuration_ref == "K" && got.envelope.format_ref == "E" &&
                got.envelope.document == (is_ready ? "ready" : "active") &&
                got.timer.lifecycle ==
                    (is_ready ? TimerLifecycle::Ready : TimerLifecycle::Active) &&
                got.timer.phase == CountdownPhase::Main &&
                got.timer.main_remaining_ms == (is_ready ? 0 : 10000) &&
                got.timer.grace_remaining_ms == 0 && !got.timer.awaiting_confirmation &&
                got.time_debt_ms == 0 && got.balances.empty(),
            "Protected typed initial state and exact envelope");
}

inline void verify_k_children(const SqliteWorkSession &session) {
    const auto counts = session.execute(
        {"SELECT "
         "(SELECT count(*) FROM subathon_app_states WHERE configuration_ref=X'4b'),"
         "(SELECT count(*) FROM subathon_hold_causes WHERE configuration_ref=X'4b'),"
         "(SELECT count(*) FROM subathon_intake_entries WHERE configuration_ref=X'4b'),"
         "(SELECT count(*) FROM subathon_receipt_attempts WHERE configuration_ref=X'4b'),"
         "(SELECT count(*) FROM subathon_receipts WHERE configuration_ref=X'4b'),"
         "(SELECT count(*) FROM subathon_reference_initial_captures WHERE configuration_ref=X'4b'),"
         "(SELECT count(*) FROM subathon_retained_contexts WHERE configuration_ref=X'4b')",
         {}});
    expect_rows(
        counts,
        {{integer(1), integer(0), integer(0), integer(0), integer(0), integer(0), integer(0)}},
        "C2: every protected v21 child table of configuration K");
    expect_rows(session.execute({"SELECT rowid,token FROM subathon_app_states "
                                 "WHERE configuration_ref=X'4b'",
                                 {}}),
                {{integer(1), blob("A")}}, "A is the sole child of K, at rowid 1");
}

class ProtectedPredecessor final {
  public:
    explicit ProtectedPredecessor(bool is_ready) {
        using namespace subathon::core;
        const auto manifest = milestone_slot_interruption_repository_manifest();
        verify_manifest(manifest, 21);
        SqliteWriter writer;
        require(writer.open(file.path(), setup_profile) == StorageError::none,
                "Protected predecessor writer opens its own file");
        expect_current(migrate(writer, request(MigrationMode::advance, manifest)), 21, 21);
        expect_current(migrate(writer, request(MigrationMode::inspect, manifest)), 21, 0);
        expect_commit(qt_test_support::run_work(
            writer,
            [is_ready](auto context) {
                const auto session = context.begin();
                SqliteRetainedContextStore retained(session);
                require(
                    retained.retain({{RetainedKind::configuration, "K"}, "configuration", {}}) ==
                        RetainDisposition::inserted,
                    "Protected configuration owner retains K");
                const ApplicationStateSnapshot state{
                    "A",
                    {"E", is_ready ? "ready" : "active"},
                    "K",
                    {is_ready ? TimerLifecycle::Ready : TimerLifecycle::Active,
                     CountdownPhase::Main, is_ready ? 0 : 10000, 0, false},
                    0,
                    {}};
                const SqliteApplicationBundleStore bundles(session);
                require(bundles.retain_initial_state(state) ==
                            SqliteBundleInitialDisposition::inserted,
                        "Protected bundle owner retains A as the initial head");
                session.commit();
                return 1;
            },
            10000));
        expect_current(migrate(writer, request(MigrationMode::inspect, manifest)), 21, 0);
        verify(writer, is_ready);
    }

    static void verify(SqliteWriter &writer, bool is_ready) {
        expect_commit(qt_test_support::run_work(
            writer,
            [is_ready](auto context) {
                const auto session = context.begin();
                const auto live = SqliteApplicationBundleStore(session).read_state();
                require(live.has_value(), "Protected owner reads its committed head");
                expect_state(*live, is_ready);
                expect_rows(session.execute({select_state, {}}),
                            {is_ready ? ready_state : active_state}, "Only A exists");
                expect_rows(session.execute({select_head, {}}), initial_head, "Actual A head");
                expect_rows(session.execute({select_configuration, {}}), initial_configuration,
                            "Exact retained K");
                expect_rows(session.execute({select_history, {}}), expected_history(21),
                            "Exact ordered protected v21 INTEGER/TEXT/BLOB history");
                verify_k_children(session);
                session.commit();
                return 1;
            },
            10000));
    }

    DisposableFile file;
};

} // namespace storage_003_fixture
