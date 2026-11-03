// SPDX-License-Identifier: GPL-3.0-or-later
// Foundation only: no new hold facade/factory is constructed or called.
#include "support/sqlite_application_hold_fixture.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <subathon/server/sqlite_application_plans.hpp>

using namespace application_hold_fixture;
namespace {
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
QByteArray frame(const QByteArray &bytes) { return QByteArray::number(bytes.size()) + ':' + bytes; }
QByteArray literal_payload(const QJsonObject &m) {
    auto result = frame("migration-catalog-v1") + frame(QByteArray::number(m["version"].toInt())) +
                  frame(m["id"].toString().toUtf8());
    const auto sql = m["sql"].toArray(), objects = m["expected_schema"].toArray();
    result += frame(QByteArray::number(sql.size()));
    for (const auto &statement : sql)
        result += frame(statement.toString().toUtf8());
    result += frame(QByteArray::number(objects.size()));
    for (const auto &entry : objects) {
        const auto o = entry.toObject();
        result += frame(o["type"].toString().toUtf8()) + frame(o["name"].toString().toUtf8()) +
                  frame(o["table_name"].toString().toUtf8()) + frame(o["sql"].isNull() ? "0" : "1");
        if (!o["sql"].isNull())
            result += frame(o["sql"].toString().toUtf8());
    }
    return result;
}
void populate(SqliteWriter &writer, const QString &path, const Data &data, const char *name) {
    require(writer.open(path, limits()) == StorageError::none, "Old writer opens disposable file");
    (void)run(writer, [&](auto context) -> QVariant {
        const auto session = context.begin();
        data.populate(session, name);
        session.commit();
        return 1;
    });
}
void catalog(Observer &o, const Data &d) {
    const auto expected = d.frozen["manifest"].toObject()["expected_schema"].toArray();
    const auto actual =
        o.rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
    require(actual.size() == 89 && expected.size() == 89, "Literal full89 inventory");
    for (qsizetype n = 0; n < actual.size(); ++n) {
        const auto e = expected[n].toObject();
        require(actual[n][0].metaType().id() == QMetaType::QString &&
                    actual[n][1].metaType().id() == QMetaType::QString &&
                    actual[n][2].metaType().id() == QMetaType::QString &&
                    actual[n][0].toString() == e["type"].toString() &&
                    actual[n][1].toString() == e["name"].toString() &&
                    actual[n][2].toString() == e["table_name"].toString() &&
                    (e["sql"].isNull() ? actual[n][3].isNull()
                                       : actual[n][3].metaType().id() == QMetaType::QString &&
                                             actual[n][3].toString() == e["sql"].toString()),
                "Exact typed catalog SQL/NULL tuple");
    }
    require(o.rows("PRAGMA foreign_key_check").empty(), "All actual typed foreign keys agree");
    require(o.rows("PRAGMA integrity_check") == SqlRows{{QString("ok")}}, "Actual integrity ok");
    const auto history = o.rows("SELECT version,migration_id,content_sha256 FROM "
                                "subathon_migration_history ORDER BY version");
    require(history.size() == 6, "Six actual catalog history rows");
    for (qsizetype n = 0; n < history.size(); ++n) {
        const auto m = n == 5 ? d.frozen["manifest"].toObject() : d.old.manifests[n];
        require((history[n][0].metaType().id() == QMetaType::Int ||
                 history[n][0].metaType().id() == QMetaType::LongLong) &&
                    history[n][0].toLongLong() == n + 1 &&
                    history[n][1].metaType().id() == QMetaType::QString &&
                    history[n][1].toString() == m["id"].toString() &&
                    history[n][2].metaType().id() == QMetaType::QByteArray &&
                    history[n][2].toByteArray().toHex() == m["sha256_hex"].toString().toLatin1(),
                "Actual history keeps exact typed identity/hash");
    }
    require(o.rows("SELECT singleton,catalog_format,store_identity FROM subathon_catalog") ==
                SqlRows{{qint64(1), qint64(1), QString("subathon.sqlite-store")}},
            "Coherent catalog singleton");
}
void raw_queries(Observer &observer, const Data &data, const char *name) {
    for (const auto &entry :
         data.frozen["arrangements"].toObject()[name].toObject()["queries"].toArray()) {
        const auto q = entry.toObject();
        check_rows(observer.rows(q["sql"].toString()), q);
    }
}
void constraint_refusal(SqliteWriter &writer, const SqlStatement &statement) {
    bool done = false, synchronous = false;
    SqliteWorkResult result;
    require(writer.enqueue_work(
                [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    try {
                        (void)session.execute(statement);
                    } catch (const SqliteWorkFailure &e) {
                        synchronous = e.error() == WorkError::sql_failure;
                        throw;
                    }
                    session.rollback(); // Never retain an unexpectedly accepted diagnostic row.
                    return 0;
                },
                [&](auto r) {
                    result = std::move(r);
                    done = true;
                }) == StorageError::none,
            "Old writer admits raw DDL constraint diagnostic");
    require(QTest::qWaitFor([&] { return done; }, 10000), "Raw constraint unit completes");
    require(synchronous && result.error == WorkError::sql_failure &&
                result.transaction == TransactionOutcome::rolled_back && !result.value,
            "Actual SQLite constraint refusal establishes known whole rollback");
}
} // namespace

class SqliteApplicationHoldEnvironment final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void frozenCatalogAndConstraintFoundationIsIndependent() {
        scenario([] {
            const Data data;
            const auto pins = data.frozen["prefixes"].toArray();
            require(pins.size() == 5, "All five immutable prefixes pinned");
            for (qsizetype n = 0; n < pins.size(); ++n) {
                const auto m = data.old.manifests[n], pin = pins[n].toObject();
                const auto payload = literal_payload(m);
                require(payload.toHex() == m["payload_hex"].toString().toLatin1() &&
                            payload.toHex() == pin["payload_hex"].toString().toLatin1() &&
                            payload.size() == pin["payload_bytes"].toInt() &&
                            QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex() ==
                                pin["sha256_hex"].toString().toLatin1(),
                        "Independent F framing preserves every old full payload/hash");
            }
            const auto m = data.frozen["manifest"].toObject();
            const auto payload = literal_payload(m);
            require(m["sql"].toArray().size() == 12 && payload.size() == 89890 &&
                        payload.toHex() == m["payload_hex"].toString().toLatin1() &&
                        QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex() ==
                            "9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a",
                    "New step6 literal payload/frame is independent of missing factory");
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable foundation directory");
            SqliteWriter writer;
            const auto path = directory.filePath("catalog.sqlite");
            populate(writer, path, data, "pending");
            Observer observer(path);
            catalog(observer, data);
            // FK targets hFail/v1/occurrence1 are real and healthy. This is the
            // reviewed SQL-NULL-hole witness, not an invocation of a hold operation.
            constraint_refusal(
                writer,
                {"INSERT INTO main.subathon_hold_mutations"
                 "(mutation_ref,mutation_kind,subject_kind,subject_id,expected_count,entry_count,"
                 "actor_ref,decision_ref,reason,options_format_ref,options_document,held_hold_ref,"
                 "held_version_ref,held_request_ordinal,held_component_kind) VALUES"
                 "(X'626164',1,0,X'ffffffffffffffff',0,1,X'61',X'64',X'',X'66',X'',"
                 "X'684661696c',X'7631',1,4)",
                 {}});
            constraint_refusal(
                writer,
                {"INSERT INTO main.subathon_hold_mutations"
                 "(mutation_ref,mutation_kind,subject_kind,subject_id,expected_count,entry_count,"
                 "actor_ref,decision_ref,reason,options_format_ref,options_document) VALUES"
                 "(X'644f766572666c6f77',1,0,X'0000000000000000',0,1,X'61',X'64',X'',X'66',X'')",
                 {}});
            constraint_refusal(
                writer, {"INSERT INTO main.subathon_hold_occurrences"
                         "(hold_ref,ordinal,attempted_numerator) VALUES(X'684661696c',3,X'31')",
                         {}});
            constraint_refusal(writer, {"INSERT INTO main.subathon_hold_versions"
                                        "(hold_ref,version_ref,position,state,mutation_ref) VALUES"
                                        "(X'685061757365',X'7630',0,0,X'64506175736531')",
                                        {}});
            raw_queries(observer, data, "pending");
            catalog(observer, data);
            qInfo()
                << "Required hold foundation: full89, prefix1-5 exact, raw constraints/FK0, SQLite"
                << observer.rows("SELECT sqlite_version()")[0][0].toString();
        });
    }
    void pendingSourcesAndContiguousHistoryContainNoAppliedSubset() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable directory");
            SqliteWriter writer;
            const auto path = directory.filePath("pending.sqlite");
            populate(writer, path, data, "pending");
            Observer observer(path);
            raw_queries(observer, data, "pending");
            catalog(observer, data);
            (void)run(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                const SqliteApplicationBundleStore old(session);
                for (const auto *name : {"PO", "PA", "PB", "PC"}) {
                    const auto plan = SqliteApplicationPlanStore(session).find(name);
                    require(plan && plan->revisions.size() == 1,
                            "Actual complete initial plan/source closure");
                }
                const auto state = old.read_state();
                require(
                    state && state->token == "p" && state->timer.main_remaining_ms == 55 &&
                        state->balances.size() == 1 &&
                        state->balances[0].units == std::numeric_limits<std::int64_t>::max(),
                    "Fresh initial state retains exact MAX without applied subset or token ABA");
                session.commit();
                return 1;
            });
            const auto failed = data.cause("hFail"), captured = data.cause("hOverflow");
            require(!failed.plan_evidence.plan && !failed.first_observation.persisted_state_token &&
                        failed.plan_evidence.occurrences[0].finalized_units == 0 &&
                        !failed.plan_evidence.occurrences[1].finalized_units &&
                        failed.plan_evidence.occurrences[0].target_ref ==
                            std::optional<std::string>{""} &&
                        !failed.plan_evidence.occurrences[0].action_ref &&
                        failed.failure->attempted_units == core::Exact("9223372036854775808") &&
                        captured.plan_evidence.occurrences.size() == 2 &&
                        captured.plan_evidence.plan->plan_ref == "PO",
                    "Literal DTOs retain absent amount/head/plan versus known0/presentempty");
            const auto pause = data.view("hPause"), closed = data.view("hOld");
            require(pause.current_version.position == 65 && pause.first_version.position == 1 &&
                        pause.current_event &&
                        closed.current_version.state == ApplicationHoldState::resolved &&
                        closed.current_event &&
                        data.resolution("rReplace").produced_versions.size() == 2,
                    "Finite owned DTOs retain historical65 and replacement batch without recursive "
                    "views");
            require(data.oracle("hOverflow.view")["bytes"].toInt() == 367 &&
                        data.oracle("hPause.view")["bytes"].toInt() == 264 &&
                        9 + 367 + 264 == data.oracle("CID0.active")["bytes"].toInt() &&
                        data.oracle("CID0.active")["items"].toInt() == 4,
                    "Independent literal view367/264 plus subject9; actual repeated members count");
            const auto page = data.history_page("pause_first2");
            require(
                page.entries.size() == 2 && page.checked_version_ref == "v65" &&
                    page.next_position == 2 && page.has_more &&
                    1 + (6 + 3 + (24 + 1 + 165) + (26 + 1 + 167) + 8 + 1) ==
                        data.oracle("hPause.history_first2")["bytes"].toInt() &&
                    data.oracle("hOld.history_resolution")["bytes"].toInt() ==
                        1 + (4 + 2 + (33 + 1 + 296) + 1) &&
                    data.oracle("hOld.history_resolution")["items"].toInt() == 6 &&
                    1 + 9 + 1 + 29 == data.oracle("dOverflow.mutation")["bytes"].toInt(),
                "Literal history/mutation wrappers count repeated cause/event/version ownership");
            require(
                captured.command.intent.document == QByteArray::fromHex("efbbbf6100ff") &&
                    !captured.command.intent.document.isNull() &&
                    !data.resolution("rReplace").request.entries[0].decision.document.isNull() &&
                    data.resolution("rReplace").request.entries[0].decision.document.isEmpty(),
                "Meaningful BOM/NUL/FF and nonnull empty documents survive pure DTO construction");
        });
    }
    void processedTimerAndDistinctCorrectionHaveCompleteOldClosure() {
        scenario([] {
            const Data data;
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable directory");
            SqliteWriter writer;
            const auto path = directory.filePath("timer.sqlite");
            populate(writer, path, data, "timer");
            Observer observer(path);
            raw_queries(observer, data, "timer");
            catalog(observer, data);
            (void)run(writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                const SqliteApplicationBundleStore old(session);
                const auto original = old.lookup({"timer", "original"});
                const auto correction = old.lookup({"timer", "correction"});
                require(original && correction &&
                            original->intent.receipt_id == core::ReceiptId{7} &&
                            correction->intent.receipt_id == core::ReceiptId{8} &&
                            correction->intent.subject_kind == ApplicationSubjectKind::receipt &&
                            !correction->intent.contribution_id && original->effects.size() == 2 &&
                            original->effects[0].requested_units == -150 &&
                            original->effects[0].components[0].units == 0 &&
                            original->effects[0].components[1].units == 0 &&
                            original->effects[0].components[2].units == -150 &&
                            original->effects[1].components[0].units == 2 &&
                            original->after.timer.main_remaining_ms == 100 &&
                            original->after.time_debt_ms == 7 &&
                            correction->after.timer.main_remaining_ms == 97,
                        "Old complete lookup independently validates honest held timer+score and "
                        "distinct correction");
                session.commit();
                return 1;
            });
            const auto correction = data.proposal("timer_correction");
            const auto alias = data.proposal("forbidden_old_beside_alias");
            require(correction.application->kind ==
                            ApplicationHoldApplicationLinkKind::timer_correction &&
                        correction.application->held_origin->version.hold_ref == "hTimer" &&
                        correction.application->held_origin->remaining_units == -150 &&
                        correction.application->operation.operation_ref == "correction" &&
                        alias.application->kind ==
                            ApplicationHoldApplicationLinkKind::whole_input &&
                        !alias.application->held_origin &&
                        alias.application->operation.operation_ref == "original",
                    "H11 typed positive/forbidden link inputs frozen; no new behavior asserted or "
                    "enabled");
            require(data.cause("hTimer").command.operation.operation_scope == "timer" &&
                        data.cause("hTimer").command.operation.operation_ref == "original",
                    "Beside timer command key is its actual original application key");
            // Preserve the older independent support1/event7 source distinction too.
            SqliteWriter old_writer;
            require(old_writer.open(directory.filePath("old-source.sqlite"), limits()) ==
                        StorageError::none,
                    "Old nonempty foundation opens");
            (void)run(old_writer, [&](auto context) -> QVariant {
                const auto session = context.begin();
                data.old.populate(session, true);
                const auto prior =
                    SqliteApplicationBundleStore(session).lookup({"scope", "original"});
                require(prior && prior->intent.receipt_id == core::ReceiptId{7} &&
                            prior->intent.contribution_id == core::ContributionId{0},
                        "Unchanged old full source closure uses event7 distinct original support1");
                const auto source =
                    session.execute({"SELECT original_receipt_id FROM main.subathon_contributions "
                                     "WHERE contribution_id=X'0000000000000000'",
                                     {}});
                require(source.size() == 1 &&
                            source[0][0].toByteArray() == QByteArray::fromHex("0000000000000001"),
                        "Actual original support remains1 rather than overwritten by event7");
                session.commit();
                return 1;
            });
        });
    }
};
QTEST_GUILESS_MAIN(SqliteApplicationHoldEnvironment)
#include "sqlite_application_hold_environment.moc"
