// SPDX-License-Identifier: GPL-3.0-or-later
// Fixture foundation only: old writer/lookup and raw observation; no new claim-reader call.
#include "support/sqlite_original_claim_fixture.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTemporaryDir>

using namespace original_claim_fixture;
namespace {
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
void populate(SqliteWriter &writer, const QString &path, const Data &data, bool nonempty = false) {
    require(writer.open(path, limits()) == StorageError::none,
            "Old writer opens disposable fixture");
    (void)run(writer, [&](auto context) -> QVariant {
        const auto session = context.begin();
        data.populate(session, nonempty);
        session.commit();
        return 1;
    });
}
void check_catalog(Observer &observer, const Data &data) {
    const auto objects =
        observer.rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
    const auto expected = data.manifests.back()["expected_schema"].toArray();
    require(objects.size() == 66 && expected.size() == 66,
            "Complete literal prefix5/full66 inventory");
    for (qsizetype i = 0; i < objects.size(); ++i) {
        const auto object = expected[i].toObject();
        require(objects[i].size() == 4 && objects[i][0].toString() == object["type"].toString() &&
                    objects[i][1].toString() == object["name"].toString() &&
                    objects[i][2].toString() == object["table_name"].toString() &&
                    (object["sql"].isNull()
                         ? objects[i][3].isNull()
                         : !objects[i][3].isNull() &&
                               objects[i][3].toString() == object["sql"].toString()),
                "Every actual catalog tuple equals unchanged frozen prefix data");
    }
    require(observer.rows("SELECT singleton,catalog_format,store_identity FROM subathon_catalog") ==
                SqlRows{{qint64(1), qint64(1), QString("subathon.sqlite-store")}},
            "Coherent catalog singleton");
    const auto history = observer.rows("SELECT version,migration_id,content_sha256 FROM "
                                       "subathon_migration_history ORDER BY version");
    require(history.size() == 5, "All five actual migration history entries");
    const auto pins = data.minimal["prefixes"].toArray();
    for (qsizetype i = 0; i < history.size(); ++i) {
        const auto m = data.manifests[i];
        const auto sha = m["sha256_hex"].toString().toLatin1();
        require(pins[i].toObject()["sha256_hex"].toString().toLatin1() == sha &&
                    QCryptographicHash::hash(
                        QByteArray::fromHex(m["payload_hex"].toString().toLatin1()),
                        QCryptographicHash::Sha256)
                            .toHex() == sha &&
                    history[i][0].toLongLong() == i + 1 &&
                    history[i][1].metaType().id() == QMetaType::QString &&
                    history[i][1].toString() == m["id"].toString() &&
                    history[i][2].metaType().id() == QMetaType::QByteArray &&
                    history[i][2].toByteArray() == QByteArray::fromHex(sha),
                "Frozen payload hash/pin and actual history retain exact metadata");
    }
}
} // namespace

class SqliteOriginalClaimEnvironment final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void minimalFixtureHasCoherentCatalogAndCompetingClaims() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable directory");
            const auto path = directory.filePath("minimal.sqlite");
            SqliteWriter writer;
            populate(writer, path, data);
            Observer observer(path);
            check_catalog(observer, data);
            for (const auto &entry : data.minimal["queries"].toArray()) {
                const auto query = entry.toObject();
                check_rows(observer.rows(query["sql"].toString()), query);
            }
            qInfo() << "Literal foundation:3 competing ORIGINAL records, MAX allocators, "
                       "full66/FK0, SQLite"
                    << observer.rows("SELECT sqlite_version()")[0][0].toString();
        });
    }
    void oldLookupValidatesCompleteMinimalRecordsAndLiteralByteBounds() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable directory");
            SqliteWriter writer;
            populate(writer, directory.filePath("old-lookup.sqlite"), data);
            (void)run(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                for (char tag : {'A', 'B', 'C'}) {
                    SqliteBundleLimits bounds;
                    if (tag != 'C')
                        bounds.owned_bytes = tag == 'A' ? 161 : 160;
                    const SqliteApplicationBundleStore old_store(session, bounds);
                    const auto expected = expected_record(tag);
                    const auto selected = old_store.lookup(expected.intent.operation);
                    require(selected.has_value(),
                            "Old exact-key lookup returns whole literal record");
                    same_record(*selected, tag);
                }
                session.commit();
                return 1;
            });
            // Independent literal arithmetic: no production byte counter or new query creates this
            // oracle.
            const auto a = data.minimal["logical_byte_oracles"].toObject()["A"].toObject();
            const auto b = data.minimal["logical_byte_oracles"].toObject()["B"].toObject();
            require(45 + 32 + 32 + 32 + 18 + 1 + 1 == 161 && a["body"].toInt() == 161 &&
                        a["matched_wrapper"].toInt() == 162 &&
                        a["returned_vector_items"].toInt() == 0 &&
                        44 + 32 + 32 + 32 + 18 + 1 + 1 == 160 && b["body"].toInt() == 160 &&
                        b["matched_wrapper"].toInt() == 161 &&
                        b["returned_vector_items"].toInt() == 0,
                    "Body161/160 plus exactly one discriminator byte, zero new vector items");
        });
    }
    void oldNonemptyFixtureRetainsTwentyItemFoundation() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable directory");
            const auto path = directory.filePath("nonempty.sqlite");
            SqliteWriter writer;
            populate(writer, path, data, true);
            Observer observer(path);
            check_catalog(observer, data);
            for (const auto &entry : data.nonempty["queries"].toObject()["original"].toArray()) {
                const auto query = entry.toObject();
                check_rows(observer.rows(query["sql"].toString()), query);
            }
            (void)run(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                SqliteBundleLimits bounds;
                bounds.input_items = 1;
                bounds.result_items = 20;
                const SqliteApplicationBundleStore old_store(session, bounds);
                const auto selected = old_store.lookup({"scope", "original"});
                require(selected && selected->intent.receipt_id == core::ReceiptId{7} &&
                            selected->intent.contribution_id == core::ContributionId{0} &&
                            selected->persisted_head.balances.size() == 2 &&
                            selected->advanced_before.balances.size() == 2 &&
                            selected->after.balances.size() == 2 && selected->effects.size() == 4 &&
                            selected->jobs.size() == 2 && selected->transitions.empty() &&
                            selected->reversals.empty(),
                        "Frozen nonempty original vectors retain2+2+2+4+components+2+0+0");
                size_t components = 0;
                for (const auto &effect : selected->effects)
                    components += effect.components.size();
                require(components == 8 && 2 + 2 + 2 + 4 + components + 2 == 20,
                        "Independent nonempty20, input_items is not a returned quota");
                session.commit();
                return 1;
            });
        });
    }
};
QTEST_GUILESS_MAIN(SqliteOriginalClaimEnvironment)
#include "sqlite_original_claim_environment.moc"
