// SPDX-License-Identifier: GPL-3.0-or-later
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QVersionNumber>

class SqliteMigrationEngineWitness final : public QObject {
    Q_OBJECT

  private slots:
    void observeEngine() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString name = QStringLiteral("storagecontract-003-witness");
        const auto removeConnection = qScopeGuard([&] { QSqlDatabase::removeDatabase(name); });
        auto connection = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        connection.setDatabaseName(directory.filePath(QStringLiteral("witness.sqlite")));
        QVERIFY2(connection.open(), qPrintable(connection.lastError().text()));

        QSqlQuery query(connection);
        QVERIFY2(query.exec(QStringLiteral("SELECT sqlite_version(), sqlite_source_id()")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        const auto version = QVersionNumber::fromString(query.value(0).toString());
        QVERIFY(version >= QVersionNumber(3, 45, 1));
        QVERIFY(!query.value(1).toString().isEmpty());
        QJsonObject witness{
            {QStringLiteral("sqlite_version"), query.value(0).toString()},
            {QStringLiteral("sqlite_source_id"), query.value(1).toString()},
        };
        QVERIFY2(query.exec(QStringLiteral("PRAGMA compile_options")),
                 qPrintable(query.lastError().text()));
        QJsonArray options;
        bool memoryTempSupported = false;
        while (query.next()) {
            const auto option = query.value(0).toString();
            options.append(option);
            memoryTempSupported |= option == QStringLiteral("TEMP_STORE=1") ||
                                   option == QStringLiteral("TEMP_STORE=2") ||
                                   option == QStringLiteral("TEMP_STORE=3");
        }
        QVERIFY2(!query.lastError().isValid(), qPrintable(query.lastError().text()));
        QVERIFY(memoryTempSupported);
        witness.insert(QStringLiteral("compile_options"), options);

        for (const auto &pragma : {QStringLiteral("page_size"), QStringLiteral("cache_size"),
                                   QStringLiteral("cache_spill"), QStringLiteral("temp_store"),
                                   QStringLiteral("journal_mode"), QStringLiteral("soft_heap_limit")}) {
            QVERIFY2(query.exec(QStringLiteral("PRAGMA ") + pragma),
                     qPrintable(query.lastError().text()));
            QVERIFY2(query.next(), qPrintable(query.lastError().text()));
            witness.insert(pragma, QJsonValue::fromVariant(query.value(0)));
        }

        QVERIFY2(query.exec(QStringLiteral("PRAGMA database_list")),
                 qPrintable(query.lastError().text()));
        bool hasMain = false;
        while (query.next()) {
            const auto schema = query.value(1).toString();
            QVERIFY(schema == QStringLiteral("main") || schema == QStringLiteral("temp"));
            hasMain |= schema == QStringLiteral("main");
        }
        QVERIFY2(!query.lastError().isValid(), qPrintable(query.lastError().text()));
        QVERIFY(hasMain);

        const auto initialSpill = witness.value(QStringLiteral("cache_spill")).toInt(-1);
        const auto initialTemp = witness.value(QStringLiteral("temp_store")).toInt(-1);
        QVERIFY(initialSpill >= 0);
        QVERIFY(initialTemp >= 0 && initialTemp <= 2);

        QVERIFY2(query.exec(QStringLiteral("PRAGMA cache_spill=OFF")),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("PRAGMA cache_spill")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        query.finish();

        QVERIFY2(query.exec(QStringLiteral("SELECT count(*) FROM temp.sqlite_schema")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        query.finish();
        QVERIFY2(query.exec(QStringLiteral("PRAGMA temp_store=MEMORY")),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("PRAGMA temp_store")), qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
        query.finish();

        QVERIFY2(query.exec(QStringLiteral("SELECT count(*) FROM temp.sqlite_schema")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        query.finish();
        QVERIFY2(query.exec(QStringLiteral("PRAGMA temp_store=%1").arg(initialTemp)),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("PRAGMA temp_store")), qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), initialTemp);
        query.finish();
        QVERIFY2(query.exec(initialSpill == 0 ? QStringLiteral("PRAGMA cache_spill=OFF")
                                             : QStringLiteral("PRAGMA cache_spill=ON")),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("PRAGMA cache_spill")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), initialSpill);

        const auto line = QByteArrayLiteral("QSQLITE_ENGINE_WITNESS ") +
                          QJsonDocument(witness).toJson(QJsonDocument::Compact);
        qInfo().noquote() << line;
    }
};

QTEST_GUILESS_MAIN(SqliteMigrationEngineWitness)
#include "sqlite_migration_engine_witness.moc"
