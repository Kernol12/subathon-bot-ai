// SPDX-License-Identifier: GPL-3.0-or-later
// Foundation diagnostics only. New capture factories/codecs/store methods are never called.
#include "support/sqlite_reference_capture_fixture.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTemporaryDir>

using namespace reference_capture_fixture;
namespace {
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
QByteArray frame(const QByteArray &value) { return QByteArray::number(value.size()) + ':' + value; }
// Independent catalog grammar; neither production payload nor hash helpers derive expectations.
QByteArray payload(const QJsonObject &m) {
    auto p = frame("migration-catalog-v1") + frame(QByteArray::number(m["version"].toInt())) +
             frame(m["id"].toString().toUtf8());
    const auto sql = m["sql"].toArray(), objects = m["expected_schema"].toArray();
    p += frame(QByteArray::number(sql.size()));
    for (const auto &s : sql)
        p += frame(s.toString().toUtf8());
    p += frame(QByteArray::number(objects.size()));
    for (const auto &entry : objects) {
        const auto o = entry.toObject();
        p += frame(o["type"].toString().toUtf8()) + frame(o["name"].toString().toUtf8()) +
             frame(o["table_name"].toString().toUtf8()) + frame(o["sql"].isNull() ? "0" : "1");
        if (!o["sql"].isNull())
            p += frame(o["sql"].toString().toUtf8());
    }
    return p;
}
void open(SqliteWriter &writer, const QString &path, const Data &d) {
    require(writer.open(path, limits()) == StorageError::none, "Old writer opens disposable file");
    d.schema(writer);
}
void healthy(Observer &o) {
    require(o.rows("PRAGMA foreign_key_check").isEmpty(), "Every actual foreign key agrees");
    require(o.rows("PRAGMA integrity_check") == SqlRows{{QString("ok")}}, "SQLite integrity ok");
}
void constraint_refusal(SqliteWriter &writer, SqlStatement s) {
    const auto reached = std::make_shared<bool>(false);
    const auto r = outcome(writer, [s = std::move(s), reached](auto context) -> QVariant {
        const auto session = context.begin();
        try {
            (void)session.execute(s);
        } catch (const SqliteWorkFailure &e) {
            *reached = e.error() == WorkError::sql_failure;
            throw;
        }
        // Safe cleanup if an incorrect DDL accepted the diagnostic row.
        session.rollback();
        return 0;
    });
    require(*reached && r.error == WorkError::sql_failure &&
                r.transaction == TransactionOutcome::rolled_back && !r.value,
            "Actual synchronous SQLite constraint refusal and known whole rollback");
}
void catalog(Observer &o, const Data &d) {
    const auto expected = d.frozen["manifest"].toObject()["expected_schema"].toArray();
    const auto rows =
        o.rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name");
    require(rows.size() == 92 && expected.size() == 92, "Actual complete92 objects");
    for (qsizetype n = 0; n < rows.size(); ++n) {
        const auto e = expected[n].toObject();
        require(rows[n][0].metaType().id() == QMetaType::QString &&
                    rows[n][1].metaType().id() == QMetaType::QString &&
                    rows[n][2].metaType().id() == QMetaType::QString &&
                    rows[n][0].toString() == e["type"].toString() &&
                    rows[n][1].toString() == e["name"].toString() &&
                    rows[n][2].toString() == e["table_name"].toString() &&
                    (e["sql"].isNull() ? rows[n][3].isNull()
                                       : rows[n][3].metaType().id() == QMetaType::QString &&
                                             rows[n][3].toString() == e["sql"].toString()),
                "Exact typed SQLite schema tuple including SQLNULL autoindexes");
    }
    auto manifests = d.prefixes;
    manifests.append(d.frozen["manifest"].toObject());
    const auto history = o.rows("SELECT version,migration_id,content_sha256 FROM "
                                "main.subathon_migration_history ORDER BY version");
    require(history.size() == 7, "Actual complete seven-step history");
    for (qsizetype n = 0; n < history.size(); ++n) {
        const auto &m = manifests[n];
        require(history[n][0].toLongLong() == n + 1 &&
                    history[n][1].metaType().id() == QMetaType::QString &&
                    history[n][1].toString() == m["id"].toString() &&
                    history[n][2].metaType().id() == QMetaType::QByteArray &&
                    history[n][2].toByteArray().toHex() == m["sha256_hex"].toString().toLatin1(),
                "Old runner wrote exact literal step identity/hash");
    }
    require(o.rows("SELECT singleton,catalog_format,store_identity FROM main.subathon_catalog") ==
                SqlRows{{qint64(1), qint64(1), QString("subathon.sqlite-store")}},
            "Actual coherent catalog singleton");
    healthy(o);
}
} // namespace
class SqliteReferenceCaptureEnvironment final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void independentSchemaAndFramesUseExistingRunner() {
        scenario([] {
            const Data d;
            const auto pins = d.frozen["prefixes"].toArray();
            require(pins.size() == 6 && d.prefixes.size() == 6, "Six immutable prefixes");
            for (qsizetype n = 0; n < pins.size(); ++n) {
                const auto pin = pins[n].toObject();
                const auto p = payload(d.prefixes[n]);
                require(p.size() == pin["payload_bytes"].toInt() &&
                            QCryptographicHash::hash(p, QCryptographicHash::Sha256).toHex() ==
                                pin["sha256_hex"].toString().toLatin1() &&
                            d.prefixes[n]["sha256_hex"] == pin["sha256_hex"],
                        "Independent framing preserves every immutable prefix");
            }
            const auto m = d.frozen["manifest"].toObject();
            const auto p = payload(m);
            require(m["sql"].toArray().size() == 1 &&
                        m["sql"].toArray()[0].toString().toUtf8().size() == 1977 &&
                        p.size() == 71601 &&
                        QCryptographicHash::hash(p, QCryptographicHash::Sha256).toHex() ==
                            "efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218",
                    "Independent one-statement step7/full92 framing");
            const auto a = d.capture("A");
            require(a["key_bytes"].toInt() == 26 && a["record_bytes"].toInt() == 55,
                    "Literal scoped-key26/record55 accounting");
            const auto documents = a["documents"].toObject();
            for (const auto *tag : {"SRC1", "SRA1", "SRX1", "SRI1"}) {
                const auto e = documents[tag].toObject();
                const auto hex = e["document_hex"].toString().toLatin1();
                const auto bytes = QByteArray::fromHex(hex);
                require(bytes.toHex() == hex && bytes.startsWith(tag) &&
                            bytes.size() == e["document_bytes"].toInt() &&
                            bytes.size() + e["format"].toString().toUtf8().size() ==
                                e["envelope_bytes"].toInt(),
                        "Literal canonical document length and complete envelope count");
            }
            require(documents["SRI1"].toObject()["document_bytes"].toInt() == 99 &&
                        documents["SRI1"].toObject()["envelope_bytes"].toInt() == 136,
                    "Published SRI1 A99/envelope136 remains literal");
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable schema diagnostic directory");
            SqliteWriter writer;
            const auto path = directory.filePath("catalog.sqlite");
            open(writer, path, d);
            Observer observer(path);
            catalog(observer, d);
            require(
                observer.rows(
                    "PRAGMA "
                    "main.index_info(sqlite_autoindex_subathon_reference_initial_captures_1)") ==
                        SqlRows{{0, 0, QString("installation_ref")},
                                {1, 1, QString("provider")},
                                {2, 2, QString("source_account")},
                                {3, 3, QString("identifier_namespace")},
                                {4, 4, QString("opaque_id")}} &&
                    observer.rows("PRAGMA "
                                  "main.index_info(sqlite_autoindex_subathon_reference_initial_"
                                  "captures_2)") ==
                        SqlRows{{0, 0, QString("installation_ref")}, {1, 5, QString("receipt_id")}},
                "Actual generated indexes cover full source and installation/receipt");
        });
    }
    void completeLiteralClosuresKeepZeroAndMaximumIds() {
        scenario([] {
            const Data d;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable closure diagnostic directory");
            SqliteWriter writer;
            const auto path = directory.filePath("closures.sqlite");
            open(writer, path, d);
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                for (const auto *name : {"A", "B"}) {
                    d.intake(s, name);
                    d.stage_capture(s, name);
                    d.index(s, name);
                }
                s.commit();
                return 1;
            });
            Observer observer(path);
            d.verify(observer, "A");
            d.verify(observer, "B");
            require(
                observer.rows("SELECT hex(last_allocated) FROM main.subathon_receipt_allocator") ==
                        SqlRows{{QString("FFFFFFFFFFFFFFFF")}} &&
                    observer.rows(
                        "SELECT hex(last_allocated) FROM main.subathon_contribution_allocator") ==
                        SqlRows{{QString("FFFFFFFFFFFFFFFF")}},
                "Coherent literal allocator high-water marks include actual maximum IDs");
            require(observer.rows(
                        "SELECT typeof(contribution_id),hex(contribution_id) FROM "
                        "main.subathon_reference_initial_captures ORDER BY contribution_id") ==
                        SqlRows{{QString("blob"), QString("0000000000000000")},
                                {QString("blob"), QString("FFFFFFFFFFFFFFFF")}},
                    "CId0/MAX stay present exact BLOB8 values");
            require(observer.rows("SELECT COUNT(*) FROM main.subathon_plan_modifiers") ==
                        SqlRows{{0}},
                    "Both selected initial plans have zero modifiers");
            catalog(observer, d);
        });
    }
    void realConstraintsRefuseAliasesTypesAndMissingParents() {
        scenario([] {
            const Data d;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable constraint diagnostic directory");
            SqliteWriter writer;
            const auto path = directory.filePath("constraints.sqlite");
            open(writer, path, d);
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                d.intake(s, "A");
                d.stage_capture(s, "A");
                s.commit();
                return 1;
            });
            for (const auto &entry : d.frozen["constraint_refusals"].toArray())
                constraint_refusal(writer, statement(entry.toObject()["statement"]));
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                d.index(s, "A");
                s.commit();
                return 1;
            });
            auto same = statement(d.capture("A")["index"]);
            constraint_refusal(writer, same); // Exact complete PK duplicate.
            same.bindings[4] = QByteArray("E\0", 2);
            constraint_refusal(writer, same); // Distinct source, same I/Receipt7 unique binding.
            Observer observer(path);
            d.verify(observer, "A");
            require(
                observer.rows("SELECT COUNT(*) FROM main.subathon_reference_initial_captures") ==
                    SqlRows{{1}},
                "Every refused unit left the single initial binding unchanged");
            healthy(observer);
        });
    }
    void stagedCaptureAndIndexRollbackAsOneUnit() {
        scenario([] {
            const Data d;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable rollback diagnostic directory");
            SqliteWriter writer;
            const auto path = directory.filePath("rollback.sqlite");
            open(writer, path, d);
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                d.intake(s, "A");
                s.commit();
                return 1;
            });
            Observer observer(path);
            const auto r = outcome(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                d.stage_capture(s, "A");
                d.index(s, "A");
                require(s.execute({"SELECT COUNT(*) FROM main.subathon_reference_initial_captures",
                                   {}}) == SqlRows{{1}} &&
                            observer.rows(
                                "SELECT COUNT(*) FROM main.subathon_reference_initial_captures") ==
                                SqlRows{{0}},
                        "Same-session index is provisional and invisible outside transaction");
                s.rollback();
                return 1;
            });
            require(r.error == WorkError::not_committed &&
                        r.transaction == TransactionOutcome::rolled_back && !r.value,
                    "Actual explicit whole rollback cannot publish an owned normal value");
            require(
                observer.rows("SELECT COUNT(*) FROM main.subathon_plan_headers") == SqlRows{{0}} &&
                    observer.rows("SELECT COUNT(*) FROM main.subathon_plan_revisions") ==
                        SqlRows{{0}} &&
                    observer.rows(
                        "SELECT COUNT(*) FROM main.subathon_reference_initial_captures") ==
                        SqlRows{{0}} &&
                    observer.rows("SELECT COUNT(*) FROM main.subathon_retained_contexts WHERE "
                                  "reference=X'59'") == SqlRows{{0}} &&
                    observer.rows("SELECT COUNT(*) FROM main.subathon_receipts") == SqlRows{{1}} &&
                    observer.rows("SELECT COUNT(*) FROM main.subathon_contributions") ==
                        SqlRows{{1}} &&
                    observer.rows("SELECT COUNT(*) FROM main.subathon_retained_contexts WHERE "
                                  "reference=X'58'") == SqlRows{{1}},
                "Rollback removes all new plan/SRX/index and retains prior committed "
                "intake/support");
            // A subsequent actual BEGIN/COMMIT proves the rollback released admission and locks.
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                d.stage_capture(s, "A");
                d.index(s, "A");
                s.commit();
                return 1;
            });
            d.verify(observer, "A");
            healthy(observer);
        });
    }
};
QTEST_GUILESS_MAIN(SqliteReferenceCaptureEnvironment)
#include "sqlite_reference_capture_environment.moc"
