// SPDX-License-Identifier: GPL-3.0-or-later
#include <QStringView>
#include <QtTest>

namespace subathon::server::detail {
bool allowed_governing_sql(QStringView sql);
}

class WriterLexerTests final : public QObject {
    Q_OBJECT

  private slots:
    void whitespace_data() {
        QTest::addColumn<QString>("separator");
        QTest::addColumn<bool>("valid");
        QTest::newRow("space") << QString(u" ") << true;
        QTest::newRow("tab") << QString(u"\t") << true;
        QTest::newRow("line-feed") << QString(u"\n") << true;
        QTest::newRow("form-feed") << QString(u"\f") << true;
        QTest::newRow("carriage-return") << QString(u"\r") << true;
        QTest::newRow("vertical-tab-inside-whitespace") << QString(u" \v") << true;
        QTest::newRow("vertical-tab-at-token-start") << QString(u"\v") << false;
        QTest::newRow("nonbreaking-space-is-not-whitespace") << QString(u"\u00a0") << false;
        QTest::newRow("bom-at-token-boundary") << QString(u"\ufeff") << true;
    }

    void whitespace() {
        QFETCH(QString, separator);
        QFETCH(bool, valid);
        const auto sql =
            QStringLiteral("UPDATE \"example\"") + separator + QStringLiteral("SET value=1");
        QCOMPARE(subathon::server::detail::allowed_governing_sql(sql), valid);
    }

    void targetContinuation_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<bool>("valid");
        QTest::newRow("qualified") << QString(u"UPDATE main.example SET value=1") << true;
        QTest::newRow("alias") << QString(u"UPDATE example AS sample SET value=1") << true;
        QTest::newRow("index") << QString(u"UPDATE example INDEXED BY sample SET value=1") << true;
        QTest::newRow("terminal-delete") << QString(u"DELETE FROM example") << true;
        QTest::newRow("insert-columns")
            << QString(u"INSERT INTO example(value) VALUES (1)") << true;
        QTest::newRow("replace-values") << QString(u"REPLACE INTO example VALUES (1)") << true;
        QTest::newRow("unexpected-punctuation")
            << QString(u"UPDATE example , SET value=1") << false;
        QTest::newRow("unexpected-word") << QString(u"UPDATE example sample SET value=1") << false;
        QTest::newRow("missing-alias") << QString(u"UPDATE example AS SET value=1") << false;
        QTest::newRow("invalid-qualifier") << QString(u"DELETE FROM main.") << false;
        QTest::newRow("unexpected-after-qualified")
            << QString(u"DELETE FROM main.example ,") << false;
        QTest::newRow("unterminated-token") << QString(u"DELETE FROM example \"") << false;
    }

    void targetContinuation() {
        QFETCH(QString, sql);
        QFETCH(bool, valid);
        QCOMPARE(subathon::server::detail::allowed_governing_sql(sql), valid);
    }
};

QTEST_GUILESS_MAIN(WriterLexerTests)
#include "writer_lexer_tests.moc"
