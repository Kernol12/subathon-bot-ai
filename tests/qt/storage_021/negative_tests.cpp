// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_003/protected_predecessor_fixture.hpp"
#include "fake_ops02_update_hold.hpp"
#include "v23_negative_fixture.hpp"

#include <QCoreApplication>

namespace {
using namespace storage_003_fixture;
using namespace subathon::server::detail;
using namespace subathon::server::detail::testing;

void require_rollback(const DisposableFile &file, SqliteWriter &writer,
                      const V23NegativeSpecimen &specimen, const MigrationResult &result) {
    require(result.reason == MigrationReason::integrity_failed &&
                result.work.transaction == TransactionOutcome::rolled_back && !result.report &&
                !result.work.value,
            "Typed integrity refusal with confirmed rollback and no success result");
    const auto &sql = specimen.attempts;
    const auto rollback = sql.indexOf("ROLLBACK");
    const auto restore = sql.indexOf("PRAGMA foreign_keys=ON");
    require(sql.count("ROLLBACK") == 1 && sql.count("COMMIT") == 0 && restore > rollback &&
                sql.value(restore + 1) == "PRAGMA foreign_keys" && specimen.foreign_keys_restored,
            "One rollback precedes FK ON and typed readback on the same connection");

    const auto stopped = std::make_shared<bool>();
    require(writer.shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                QTest::qWaitFor([stopped] { return *stopped; }, 10000),
            "Close writer before the independent reopen");
    {
        SqliteReader reader;
        require(reader.open(file.path()) == StorageError::none,
                "Reopen with an independent reader");
        const auto history = reader.read({select_history, {}}, 23);
        require(history.error == StorageError::none, "Read durable migration history");
        expect_rows(history.rows, expected_history(22), "The durable history is exactly v22");
    }
    const auto v22 = inspect_v22_pre_open(file.path(), setup_profile);
    require(std::holds_alternative<V22PreOpenWitness>(v22),
            "The reopened store has authentic v22 schema and clean integrity and foreign keys");
}

void run_specimen(const QString &donor, V23NegativeSpecimen::Phase phase) {
    DisposableFile file;
    require(!QFileInfo::exists(donor + "-wal") && !QFileInfo::exists(donor + "-shm") &&
                !QFileInfo::exists(donor + "-journal") && QFile::copy(donor, file.path()),
            "Copy the stopped donor without sidecars");
    require(file.seal().toHex() ==
                "729cec12d8abada51bb92459f5bb1f8b4580a236c585be29cf6ce08669218d11",
            "Copied independently pinned v22 donor");
    const auto v22 = inspect_v22_pre_open(file.path(), setup_profile);
    const auto *witness = std::get_if<V22PreOpenWitness>(&v22);
    require(witness && witness->version == 22, "Authenticate clean v22 before injecting");

    V23NegativeSpecimen specimen(file.path(), phase);
    SqliteWriter writer;
    require(writer.open(file.path(), setup_profile) == StorageError::none, "Open the real writer");
    auto lease = V23RebuildAccess::acquire(writer);
    const auto result = collect_migration<MigrationResult>([&](auto done) {
        return enqueue_v23_rebuild(*witness, FakeOps02UpdateHoldIssuer::issue(), std::move(lease),
                                   {}, {}, std::move(done));
    });

    const auto &sql = specimen.attempts;
    const auto rename = sql.indexOf(
        "ALTER TABLE main.subathon_plan_revisions_rebuild RENAME TO subathon_plan_revisions");
    const auto injected = specimen.injection_after;
    require(specimen.orphan_verified && rename >= 0 && injected > rename,
            "Reached the post-rebuild injection and verified the real FK violation");

    QList<qsizetype> history_inserts;
    QList<qsizetype> integrity_checks;
    QList<qsizetype> fk_checks;
    for (qsizetype i = 0; i < sql.size(); ++i) {
        if (sql[i].startsWith("INSERT INTO main.subathon_migration_history("))
            history_inserts.append(i);
        if (sql[i].contains("integrity_check", Qt::CaseInsensitive))
            integrity_checks.append(i);
        if (sql[i].contains("foreign_key_check", Qt::CaseInsensitive))
            fk_checks.append(i);
    }

    // Check the durable outcome even when the pre-history specimen ran too far.
    require_rollback(file, writer, specimen, result);
    const bool before_history = phase == V23NegativeSpecimen::Phase::before_history;
    if (before_history) {
        require(history_inserts.isEmpty(),
                QString("Before-history violation must refuse before any history INSERT attempt "
                        "(observed %1)")
                    .arg(history_inserts.size()));
    } else {
        require(history_inserts.size() == 1 && history_inserts.front() + 1 == injected,
                "The after-history violation follows the one successful history INSERT");
    }

    const auto rollback = sql.indexOf("ROLLBACK");
    require(!integrity_checks.isEmpty() && !fk_checks.isEmpty() &&
                integrity_checks.back() >= injected && integrity_checks.back() < fk_checks.back() &&
                fk_checks.back() < rollback,
            "Integrity then FK checks were attempted after the violation and before rollback");
}

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 2) {
        qCritical("Usage: server_storage_021_stage1_negative pinned-v22-1024.sqlite");
        return 2;
    }

    bool passed = true;
    using Phase = V23NegativeSpecimen::Phase;
    for (const auto phase : {Phase::before_history, Phase::after_history}) {
        const char *name = phase == Phase::before_history ? "before history" : "after history";
        try {
            run_specimen(args[1], phase);
            qInfo("%s: PASS", name);
        } catch (const std::exception &failure) {
            qCritical("%s: %s", name, failure.what());
            passed = false;
        }
    }
    return passed ? 0 : 1;
}
