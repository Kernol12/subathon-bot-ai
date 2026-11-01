// SPDX-License-Identifier: GPL-3.0-or-later
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <memory>
#include <subathon/server/sqlite_writer.hpp>

using namespace subathon::server;

// Fixture SQL observes committed bytes independently; it never snapshots writer inputs.
class OwnershipFixtureDatabase final {
  public:
    explicit OwnershipFixtureDatabase(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~OwnershipFixtureDatabase() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() { return db_.open(); }
    bool execute(const QString &sql) {
        QSqlQuery query(db_);
        return query.exec(sql);
    }
    QVariant scalar(const QString &sql) {
        QSqlQuery query(db_);
        if (!query.exec(sql) || !query.next())
            return {};
        return query.value(0);
    }

  private:
    QString name_;
    QSqlDatabase db_;
};

class SqliteWriterOwnershipTests final : public QObject {
    Q_OBJECT
  private:
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<OwnershipFixtureDatabase> fixture_;
    QString path_;

  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("ownership.sqlite");
        fixture_ = std::make_unique<OwnershipFixtureDatabase>(path_);
        QVERIFY(fixture_->open());
        QVERIFY(fixture_->execute("CREATE TABLE payloads(id INTEGER PRIMARY KEY, text_value TEXT, "
                                  "blob_value BLOB)"));
    }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    // Revision1's owned admission guarantee also covers QString's nonowning raw-data views.
    void ownsRawBackedSqlAtAdmission() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QChar raw_sql[] = {u'S', u'E', u'L', u'E', u'C', u'T', u' ', u'1'};
        bool completed = false;
        WriteResult result;
        {
            const QString sql_view = QString::fromRawData(raw_sql, 8);
            QVERIFY(sql_view.constData() == raw_sql);
            const WriteBatch caller_batch{{sql_view, {}}};
            QCOMPARE(writer.enqueue(caller_batch,
                                    [&](WriteResult value) {
                                        result = std::move(value);
                                        completed = true;
                                    }),
                     StorageError::none);
            QVERIFY(!completed);
        } // Caller view/batch are gone; raw storage stays alive through completion.
        raw_sql[7] = u'9';
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(result.rows, QList<SqlRows>({{{1}}}));
    }

    void ownsRawBackedTextBindingAtAdmission() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QChar raw_text[] = {u'a', u'b', u'c'};
        bool completed = false;
        WriteResult result;
        {
            const QString text_view = QString::fromRawData(raw_text, 3);
            QVERIFY(text_view.constData() == raw_text);
            const WriteBatch caller_batch{
                {"INSERT INTO payloads(id, text_value) VALUES (1, ?)", {text_view}}};
            QCOMPARE(writer.enqueue(caller_batch,
                                    [&](WriteResult value) {
                                        result = std::move(value);
                                        completed = true;
                                    }),
                     StorageError::none);
            QVERIFY(!completed);
        }
        raw_text[0] = u'z';
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM payloads").toInt(), 1);
        QCOMPARE(fixture_->scalar("SELECT text_value FROM payloads WHERE id=1").toString(),
                 QString("abc"));
    }

    void ownsRawBackedBlobBindingAtAdmission() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        char raw_blob[] = {'a', 'b', 'c'};
        bool completed = false;
        WriteResult result;
        {
            const QByteArray blob_view = QByteArray::fromRawData(raw_blob, 3);
            QVERIFY(blob_view.constData() == raw_blob);
            const WriteBatch caller_batch{
                {"INSERT INTO payloads(id, blob_value) VALUES (1, ?)", {blob_view}}};
            QCOMPARE(writer.enqueue(caller_batch,
                                    [&](WriteResult value) {
                                        result = std::move(value);
                                        completed = true;
                                    }),
                     StorageError::none);
            QVERIFY(!completed);
        }
        raw_blob[0] = 'z';
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM payloads").toInt(), 1);
        QCOMPARE(fixture_->scalar("SELECT typeof(blob_value) FROM payloads WHERE id=1").toString(),
                 QString("blob"));
        QCOMPARE(fixture_->scalar("SELECT hex(blob_value) FROM payloads WHERE id=1").toString(),
                 QString("616263"));
    }
};

QTEST_GUILESS_MAIN(SqliteWriterOwnershipTests)
#include "sqlite_writer_ownership_tests.moc"
