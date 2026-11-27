// SPDX-License-Identifier: GPL-3.0-or-later
#include "donor_pins.hpp"
#include "fake_ops02_update_hold.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStorageInfo>
#include <QTextStream>

namespace {
using namespace storage_003_fixture;
using namespace subathon::server::detail;

class FixtureSql final {
  public:
    explicit FixtureSql(const QString &path, bool read_only = false)
        : db_(QSqlDatabase::addDatabase("QSQLITE", "storage-021-stopped-fixture")) {
        db_.setDatabaseName(path);
        if (read_only)
            db_.setConnectOptions("QSQLITE_OPEN_READONLY");
    }
    ~FixtureSql() {
        const auto name = db_.connectionName();
        db_.close();
        db_ = QSqlDatabase{};
        QSqlDatabase::removeDatabase(name);
    }

    void open() { require(db_.open(), db_.lastError().text()); }

    QSqlQuery run(const QString &statement) const {
        QSqlQuery query(db_);
        require(query.exec(statement), query.lastError().text() + " [" + statement + "]");
        return query;
    }

    QVariant scalar(const QString &statement) const {
        auto query = run(statement);
        require(query.next(), statement);
        auto result = query.value(0);
        require(!query.next() && !query.lastError().isValid(), statement);
        return result;
    }

  private:
    QSqlDatabase db_;
};

void seed_plans(const FixtureSql &sql) {
    sql.run("INSERT INTO subathon_retained_configurations VALUES(X'4b',X'01')");
    sql.run("INSERT INTO subathon_retained_contexts VALUES(X'43',X'01',X'4b')");
    sql.run("INSERT INTO subathon_receipts(receipt_id,provider,source_account,"
            "identifier_namespace,opaque_id,event_type,schema_version,server_timestamp_utc_ms,"
            "connection_ref,configuration_ref,modifier_context_ref) "
            "VALUES(X'0000000000000001',X'50',X'41',X'4e',X'49',X'45',1,0,X'53',X'4b',X'43')");
    sql.run("WITH RECURSIVE n(i) AS (VALUES(1) UNION ALL SELECT i+1 FROM n WHERE i<128) "
            "INSERT INTO subathon_plan_headers(plan_ref,event_ref,root_event_ref,receipt_id,"
            "head_revision) SELECT CAST(printf('plan-%03d',i) AS BLOB),X'45',X'45',"
            "X'0000000000000001',X'0000000000000001' FROM n ORDER BY i");
    sql.run("INSERT INTO subathon_plan_revisions(plan_ref,revision,payment_evidence,monetary,"
            "missing_rate,review,evidence_policy,policy_ref,application,reversal,settlement,"
            "valuation_state) SELECT plan_ref,head_revision,0,0,0,0,0,X'50',0,0,0,0 "
            "FROM subathon_plan_headers ORDER BY plan_ref");
}

void pad_to_pages(const FixtureSql &sql, int pages) {
    const auto insert_padding = [&](int bytes) {
        sql.run(QString("INSERT INTO subathon_retained_configurations VALUES(X'ff',zeroblob(%1))")
                    .arg(bytes));
    };
    require(sql.scalar("PRAGMA page_count").toInt() < pages, "Padding has room to grow");
    // Roll back each probe to the same allocation state. Keep only the
    // smallest SQL payload that reaches the requested extent.
    int lower = 0;
    int upper = pages * 4096;
    while (lower < upper) {
        const int middle = lower + (upper - lower) / 2;
        sql.run("SAVEPOINT padding");
        insert_padding(middle);
        const int attained = sql.scalar("PRAGMA page_count").toInt();
        sql.run("ROLLBACK TO padding");
        sql.run("RELEASE padding");
        if (attained < pages)
            lower = middle + 1;
        else
            upper = middle;
    }
    insert_padding(lower);
    require(sql.scalar("PRAGMA page_count").toInt() == pages, "Exact SQL-allocated extent");
}

void verify_extent(const FixtureSql &sql, int pages) {
    require(sql.scalar("PRAGMA page_size").toInt() == 4096 &&
                sql.scalar("PRAGMA page_count").toInt() == pages &&
                sql.scalar("PRAGMA auto_vacuum").toInt() == 0,
            "Exact donor page size, extent and auto-vacuum mode");
}

void verify_runtime_prerequisites(const DisposableFile &donor) {
    FixtureSql sql(donor.path(), true);
    sql.open();
    verify_extent(sql, 1024);
    const auto version = sql.scalar("SELECT sqlite_version()").toString();
    const auto source = sql.scalar("SELECT sqlite_source_id()").toString();
    require(sql.scalar("SELECT sqlite_compileoption_used('TEMP_STORE=1') OR "
                       "sqlite_compileoption_used('TEMP_STORE=2') OR "
                       "sqlite_compileoption_used('TEMP_STORE=3')")
                    .toInt() == 1,
            "QSQLITE runtime admits MEMORY temporary storage");
    QTextStream(stderr) << "QSQLITE version=" << version << " source_id=" << source
                        << " page_size=4096 auto_vacuum=0 TEMP_STORE admitted"
                        << Qt::endl;
}

void assemble_donor(const DisposableFile &file, int pages) {
    file.create_blank();
    const auto manifest = compiled_successor();
    const auto witness = candidate(file, {0, ClockPredecessor::blank, {}, {}});
    {
        SqliteWriter writer;
        require(writer.open(file.path(), profile(blank_advance)) == StorageError::none,
                "Open v22 migration owner");
        expect_guarded_v22_advance(writer, manifest, witness);
    }
    {
        FixtureSql sql(file.path());
        sql.open();
        require(sql.scalar("PRAGMA journal_mode=DELETE").toString() == "delete",
                "Assemble fixture with a single rollback journal");
        sql.run("PRAGMA foreign_keys=ON");
        require(sql.scalar("PRAGMA foreign_keys").toInt() == 1 &&
                    (sql.scalar("SELECT sqlite_compileoption_used('TEMP_STORE=1') OR "
                                "sqlite_compileoption_used('TEMP_STORE=2')")
                         .toInt() == 1),
                "Fixture enforces FKs and SQLite permits the owner to select MEMORY");
        sql.run("BEGIN IMMEDIATE");
        seed_plans(sql);
        pad_to_pages(sql, pages);
        sql.run("COMMIT");
        // With auto-vacuum disabled the committed pages go to the freelist.
        sql.run("DELETE FROM subathon_retained_configurations WHERE reference=X'ff'");
        verify_extent(sql, pages);
        require(sql.scalar("SELECT count(*) FROM subathon_plan_revisions").toInt() == 128 &&
                    sql.scalar("SELECT count(*) FROM subathon_plan_revisions "
                               "WHERE valuation_state=0 AND parent_revision IS NULL")
                            .toInt() == 128,
                "Exactly 128 eligible legacy rows");
        auto violations = sql.run("PRAGMA foreign_key_check");
        require(!violations.next() && !violations.lastError().isValid(), "No donor FK violations");
        require(sql.scalar("PRAGMA integrity_check").toString() == "ok", "Donor integrity");
    }
    require(QFileInfo(file.path()).size() == qint64{pages} * 4096, "Exact stopped donor size");
    const auto authenticated = inspect_v22_pre_open(file.path(), setup_profile);
    require(std::holds_alternative<V22PreOpenWitness>(authenticated),
            "Authenticate the assembled v22 donor against the compiled manifest");
    qInfo().noquote() << "v22 donor" << pages << "pages SHA-256" << file.seal().toHex();
}

struct BoundCase {
    const char *name;
    int pages;
    qint64 disk_estimate;
    qint64 journal_reserve;
    V23RebuildBudget budget;
    bool permit;
};

// SQLite 3.53.4 maps one WAL-index page on the recognition read (#609.13).
constexpr qint64 recognition_shm_bytes = 32'768;
constexpr qint64 disk_fit_budget = 33'562'624;
constexpr qint64 disk_one_less_budget = disk_fit_budget - 1;

constexpr BoundCase cases[]{
    {"disk-fit", 1024, 33'529'856, 4'268'032, {disk_fit_budget, 184'549'376}, true},
    {"disk-one-less", 1024, 33'529'856, 4'268'032, {disk_one_less_budget, 184'549'376}, false},
    {"disk-larger", 1025, 33'546'248, 4'272'136, {disk_one_less_budget, 184'549'376}, false},
    {"memory-fit", 1024, 33'529'856, 4'268'032, {83'886'080, 58'720'256}, true},
    {"memory-one-less", 1024, 33'529'856, 4'268'032, {83'886'080, 58'720'255}, false},
    {"memory-larger", 1025, 33'546'248, 4'272'136, {83'886'080, 58'720'255}, false},
};

void check_bounds(const DisposableFile &donor, const BoundCase &test) {
    DisposableFile file;
    donor.copy_to(file);
    const auto stopped_seal = file.seal();
    const auto disk_estimate = test.disk_estimate + recognition_shm_bytes;
    const QStorageInfo volume(QFileInfo(file.path()).absolutePath());
    require(volume.isValid() && volume.isReady() && volume.bytesAvailable() > disk_estimate,
            "Measured free space exceeds this case's disk estimate");
    {
        FixtureSql sql(file.path(), true);
        sql.open();
        verify_extent(sql, test.pages);
    }
    auto inspected = inspect_v22_pre_open(file.path(), setup_profile);
    require(std::holds_alternative<V22PreOpenWitness>(inspected) && file.seal() == stopped_seal,
            "Read-only v22 authentication preserves the donor bytes");
    const auto result = std::make_shared<std::optional<MigrationResult>>();
    bool inert = false;
    QByteArray before;
    {
        SqliteWriter writer;
        // Successful open includes the owner's readbacks of WAL, FULL,
        // foreign_keys=ON and busy_timeout=0 on its actual connection.
        require(writer.open(file.path(), setup_profile) == StorageError::none,
                "Open verifies ordinary owner settings on the authenticated donor");
        require(QFileInfo(file.path() + "-wal").size() == 0 &&
                    QFileInfo(file.path() + "-shm").size() == recognition_shm_bytes &&
                    !QFileInfo::exists(file.path() + "-journal"),
                "One WAL-index page, empty WAL and no rollback journal at the ordinary entry");
        // Normal open selects WAL. Seal after that transition so refusals
        // compare against the bytes at the actual admission boundary.
        before = file.seal();
        const auto admission = enqueue_v23_rebuild(
            std::get<V22PreOpenWitness>(std::move(inspected)),
            subathon::server::detail::testing::FakeOps02UpdateHoldIssuer::issue(),
            V23RebuildAccess::acquire(writer), test.budget, {},
            [result](auto completion) { *result = std::move(completion); });
        inert = admission.error == StorageError::feature_not_implemented &&
                admission.reason == MigrationReason::feature_not_implemented;
        if (!inert) {
            require(admission.error == StorageError::none &&
                        admission.reason == MigrationReason::none,
                    "Bounded request reaches the ordinary owner");
            require(QTest::qWaitFor([result] { return result->has_value(); }, 10000),
                    "Ordinary owner delivers the bounds result");
        }
    }
    if (inert) {
        require(!result->has_value() && file.seal() == before,
                "Inert entry delivers no completion and preserves the donor");
        require(false, "Authenticated ordinary v23 admission returned feature_not_implemented");
    }
    const auto &completion = result->value();
    require(!QFileInfo::exists(file.path() + "-journal"), "No rollback journal remains");
    if (test.permit) {
        expect_current(completion, 23, 1);
        require(QFileInfo(file.path()).size() <= disk_estimate - test.journal_reserve,
                "Committed main file fits the post-transaction estimate");
        FixtureSql sql(file.path(), true);
        sql.open();
        require(sql.scalar("SELECT count(*) FROM subathon_migration_history WHERE version=23")
                            .toInt() == 1 &&
                    sql.scalar("SELECT count(*) FROM subathon_valuation_snapshots").toInt() == 0 &&
                    sql.scalar("SELECT count(*) FROM subathon_plan_revisions").toInt() == 128,
                "Independent reopen observes the committed successor and retained rows");
    } else {
        // Disk and memory use the same result. Their precedence belongs to
        // Stage 2 code review. The hash establishes stopped-file preservation.
        require(completion.reason == MigrationReason::resource_limit &&
                    completion.work.transaction == TransactionOutcome::not_started &&
                    !completion.report && !completion.work.value,
                "Resource refusal before a transaction, without report or work value");
        require(file.seal() == before, "Resource refusal preserves the authenticated v22 bytes");
    }
}
} // namespace

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    try {
        const auto args = application.arguments();
        const bool export_donors = args.size() == 3 && args[1] == "--assemble";
        const bool use_donors = args.size() == 3 && args[1] == "--donors";
        require(export_donors || use_donors,
                "Usage: server_storage_021_stage1_bounds (--assemble|--donors) existing-directory");
        require(QDir(args[2]).exists(), "Donor directory already exists");
        DisposableFile smaller;
        DisposableFile larger;
        if (export_donors) {
            assemble_donor(smaller, 1024);
            assemble_donor(larger, 1025);
            const QDir directory(args[2]);
            const auto small_path = directory.filePath("v22-1024.sqlite");
            const auto large_path = directory.filePath("v22-1025.sqlite");
            require(!QFileInfo::exists(small_path) && !QFileInfo::exists(large_path),
                    "Donor export paths are unused");
            require(QFile::copy(smaller.path(), small_path) &&
                        QFile::copy(larger.path(), large_path),
                    "Export stopped donors for independent inspection");
            return 0;
        }
        const QDir directory(args[2]);
        // Full-file pins come from independently verified stopped v22 exports.
        const auto load_donor = [&](const QString &name, int pages,
                                    const DisposableFile &copy) {
            const auto path = directory.filePath(name);
            require(!QFileInfo::exists(path + "-wal") && !QFileInfo::exists(path + "-shm") &&
                        !QFileInfo::exists(path + "-journal"),
                    "Pinned donor has no sidecars");
            const auto expected = storage_021_donor_pin(pages);
            QFile source(path);
            QCryptographicHash hash(QCryptographicHash::Sha256);
            require(source.open(QIODevice::ReadOnly) && hash.addData(&source) &&
                        hash.result().toHex() == expected,
                    "Stopped donor matches its independent SHA-256 pin");
            source.close();
            require(QFile::copy(path, copy.path()) && copy.seal().toHex() == expected,
                    "Disposable copy matches the pinned donor");
        };
        load_donor("v22-1024.sqlite", 1024, smaller);
        load_donor("v22-1025.sqlite", 1025, larger);
        verify_runtime_prerequisites(smaller);
        bool passed = true;
        for (const auto &test : cases) {
            try {
                check_bounds(test.pages == 1024 ? smaller : larger, test);
            } catch (const std::exception &failure) {
                qCritical().noquote() << test.name << failure.what();
                passed = false;
            }
        }
        return passed ? 0 : 1;
    } catch (const std::exception &failure) {
        qCritical().noquote() << "Donor assembly:" << failure.what();
        return 1;
    }
}
