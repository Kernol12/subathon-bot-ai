// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_test_support.hpp"
// OC01–14: independent frozen fixtures; no new reader runs until full baseline review.
#include "support/sqlite_original_claim_fixture.hpp"
#include <QMap>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <array>
#include <functional>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

using namespace original_claim_fixture;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;
namespace {
using IE = SqliteBundleInputError;
using SR = SqliteBundleStorageReason;
using CK = ApplicationComponentKind;
using Match = std::optional<ApplicationOriginalClaimMatch>;
constexpr auto maximum_id = std::numeric_limits<std::uint64_t>::max();

ApplicationOriginalClaimSelector subject(std::uint64_t cid = 0,
                                         std::uint64_t receipt = maximum_id) {
    return {ApplicationSubjectKind::contribution, core::ReceiptId{receipt},
            core::ContributionId{cid}};
}
ApplicationOriginalClaimSelector receipt(std::uint64_t id = maximum_id) {
    return {ApplicationSubjectKind::receipt, core::ReceiptId{id}, {}};
}
QByteArray id_blob(std::uint64_t id) {
    QByteArray bytes(8, '\0');
    for (int i = 7; i >= 0; --i) {
        bytes[i] = char(id & 255U);
        id >>= 8;
    }
    return bytes;
}
std::string raw(const QByteArray &bytes) { return {bytes.constData(), size_t(bytes.size())}; }

struct Outcome {
    bool done{false}, missing{false};
    std::optional<IE> input;
    std::optional<SR> storage;
    std::optional<WorkError> synchronous;
    QString problem;
    SqliteWorkResult completion;
};
// The approved foundation's run() is committed-success-only. Feature failures
// need their real typed synchronous cause and actual independent completion.
Outcome capture(SqliteWriter &writer, SqliteWriter::Work body) {
    auto state = std::make_shared<Outcome>();
    state->completion = qt_test_support::run_work(
        writer,
        [state, body = std::move(body)](auto context) -> QVariant {
            try {
                return body(context);
            } catch (const SqliteOriginalClaimNotImplemented &) {
                state->missing = true;
                throw;
            } catch (const SqliteBundleInputFailure &e) {
                state->input = e.error();
                throw;
            } catch (const SqliteBundleStorageFailure &e) {
                state->storage = e.reason();
                throw;
            } catch (const SqliteWorkFailure &e) {
                state->synchronous = e.error();
                throw;
            } catch (const std::exception &e) {
                state->problem = QString::fromUtf8(e.what());
                throw;
            }
        },
        10000);
    state->done = true;
    return *state;
}
void no_missing(const Outcome &out) {
    if (out.missing)
        throw SqliteOriginalClaimNotImplemented();
}
void committed(const Outcome &out) {
    no_missing(out);
    require(out.done && out.completion.transaction == TransactionOutcome::committed &&
                out.completion.error == WorkError::none && out.completion.value && !out.input &&
                !out.storage && !out.synchronous && out.problem.isEmpty(),
            "Required committed normal feature result: " + out.problem);
}
void rolled_back(const Outcome &out, WorkError error = WorkError::not_committed) {
    no_missing(out);
    require(out.done && out.completion.transaction == TransactionOutcome::rolled_back &&
                out.completion.error == error && !out.completion.value && !out.input &&
                !out.storage && !out.synchronous && out.problem.isEmpty(),
            "Required actual deliberate rollback/sticky terminal outcome: " + out.problem);
}
void known(const Outcome &out, SR reason, WorkError error = WorkError::work_exception) {
    no_missing(out);
    require(out.storage == reason && !out.input && !out.synchronous && out.problem.isEmpty() &&
                out.completion.transaction == TransactionOutcome::rolled_back &&
                out.completion.error == error && !out.completion.value,
            "Required synchronous storage cause and known whole-unit rollback");
}
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const SqliteOriginalClaimNotImplemented &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
template <class Body> void input_failure(Body body, IE error) {
    try {
        body();
    } catch (const SqliteBundleInputFailure &e) {
        require(e.error() == error, "Exact pure input category");
        return;
    }
    throw std::runtime_error("Required input failure missing");
}
template <class Body> void work_failure(Body body, WorkError error) {
    try {
        body();
    } catch (const SqliteWorkFailure &e) {
        require(e.error() == error, "Exact original capability/protocol category");
        return;
    }
    throw std::runtime_error("Required WorkFailure missing");
}

QString quoted(QString value) {
    value.replace('"', "\"\"");
    return '"' + value + '"';
}
// Complete raw data snapshots retain SQL class plus exact bytes/scalars. NULL
// does not rely on QVariant::Invalid equality or empty-BLOB conversion.
QByteArray snapshot(Observer &observer) {
    QByteArray result;
    const auto append = [&](const QByteArray &part) {
        result.append(QByteArray::number(qint64(part.size()))).append(':').append(part);
    };
    const auto tables =
        observer.rows("SELECT name FROM sqlite_schema WHERE type='table' ORDER BY name");
    for (const auto &table : tables) {
        const auto name = table.front().toString();
        append(name.toUtf8());
        const auto info = observer.rows("PRAGMA table_info(" + quoted(name) + ")");
        QStringList fields;
        for (const auto &column : info) {
            const auto key = quoted(column[1].toString());
            fields.append("typeof(" + key + ")");
            fields.append(key);
        }
        const auto rows = observer.rows("SELECT " + fields.join(',') + " FROM " + quoted(name) +
                                        " ORDER BY rowid");
        append(QByteArray::number(qint64(rows.size())));
        for (const auto &row : rows)
            for (qsizetype i = 0; i < row.size(); i += 2) {
                const auto type = row[i].toString();
                append(type.toUtf8());
                const auto &cell = row[i + 1];
                if (type == "null")
                    append({});
                else if (type == "blob")
                    append(cell.toByteArray());
                else if (type == "integer")
                    append(QByteArray::number(cell.toLongLong()));
                else if (type == "text")
                    append(cell.toString().toUtf8());
                else
                    append(QByteArray::number(cell.toDouble(), 'g', 17));
            }
    }
    return result;
}

class Database final {
  public:
    QTemporaryDir directory;
    QString path;
    std::unique_ptr<SqliteWriter> writer;
    std::unique_ptr<Observer> observer;
    explicit Database(const Data &data, bool nonempty = false, bool source_only = false) {
        require(directory.isValid(), "Disposable feature directory");
        path = directory.filePath("claims.sqlite");
        open();
        (void)original_claim_fixture::run(*writer, [&](auto context) -> QVariant {
            const auto s = context.begin();
            if (!source_only) {
                data.populate(s, nonempty);
            } else {
                // Approved independent literal prefix includes sources/captures
                // and ONLY initial a, never a retired b/c/d or prior application.
                data.schema(s);
                const auto steps = data.minimal["setup"].toArray();
                require(steps.size() == 45, "Frozen source-only subset revision");
                for (qsizetype i = 0; i <= 22; ++i)
                    (void)s.execute(statement(steps[i]));
                (void)s.execute({"UPDATE main.subathon_contributions SET application=0 WHERE "
                                 "contribution_id IN(X'0000000000000000',X'ffffffffffffffff')",
                                 {}});
                require(s.execute({"SELECT changes()", {}}).front().front().toInt() == 2,
                        "Both current sources initially pending");
                (void)s.execute(
                    {"INSERT INTO main.subathon_app_head(singleton,token) VALUES(1,X'61')", {}});
                require(s.execute({"SELECT (SELECT count(*) FROM subathon_app_states),"
                                   "(SELECT count(*) FROM subathon_app_operations),"
                                   "(SELECT count(*) FROM subathon_app_claims),hex(token) FROM "
                                   "subathon_app_head",
                                   {}}) ==
                            SqlRows{{qint64(1), qint64(0), qint64(0), QString("61")}},
                        "Never-applied a namespace, not fake state ABA");
            }
            s.commit();
            return 1;
        });
        observer = std::make_unique<Observer>(path);
    }
    void open(StorageLimits l = limits()) {
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, l) == StorageError::none, "Actual writer opens feature file");
    }
    void reopen(StorageLimits l = limits()) {
        writer.reset();
        open(l);
    }
    void modify(const std::function<void(SqliteWorkSession)> &body) {
        (void)original_claim_fixture::run(*writer, [&](auto context) -> QVariant {
            const auto s = context.begin();
            body(s);
            s.commit();
            return 1;
        });
    }
    void released() {
        (void)observer->rows("BEGIN IMMEDIATE");
        (void)observer->rows("ROLLBACK");
    }
    QByteArray saved() { return snapshot(*observer); }
};

void marker(const SqliteWorkSession &s) {
    (void)s.execute({"INSERT INTO main.subathon_retained_configurations(reference,document) "
                     "VALUES(X'71756572792D7072696F72',X'79')",
                     {}});
}
void marker_active(const SqliteWorkSession &s) {
    require(s.execute({"SELECT count(*) FROM subathon_retained_configurations WHERE "
                       "reference=X'71756572792D7072696F72'",
                       {}})
                    .front()
                    .front()
                    .toInt() == 1,
            "Earlier caller marker actually staged");
}
void marker_absent(Observer &observer) {
    require(observer.rows("SELECT count(*) FROM subathon_retained_configurations WHERE "
                          "reference=X'71756572792D7072696F72'") == SqlRows{{qint64(0)}},
            "Caller marker not durable");
}
void match(const Match &result, SqliteOriginalClaimKind kind, char tag) {
    require(result.has_value() && result->kind == kind, "Exact typed match disposition");
    same_record(result->existing, tag);
}
Outcome read(Database &db, const ApplicationOriginalClaimSelector &selector, Match &selected,
             SqliteBundleLimits l = {}) {
    return capture(*db.writer, [&](auto context) -> QVariant {
        const auto s = context.begin();
        selected = SqliteApplicationBundleStore(s, l).lookup_original_claim(selector);
        s.commit();
        return 1;
    });
}
Outcome refused(Database &db, const ApplicationOriginalClaimSelector &selector, SR reason,
                bool &caught, SqliteBundleLimits l = {}) {
    const auto before = db.saved();
    caught = false;
    return capture(*db.writer, [&](auto context) -> QVariant {
        const auto s = context.begin();
        SqliteApplicationBundleStore store(s, l);
        marker(s);
        marker_active(s);
        try {
            (void)store.lookup_original_claim(selector);
        } catch (const SqliteBundleStorageFailure &e) {
            require(e.reason() == reason, "Exact synchronous selected failure");
            db.released();
            require(db.saved() == before, "Complete raw rollback while facade remains alive");
            marker_absent(*db.observer);
            caught = true;
            throw;
        }
        throw std::runtime_error("Required selected storage failure missing");
    });
}

void check_nonempty(const RecordedApplicationBundle &r) {
    const auto opaque = QByteArray::fromHex("efbbbf6100ff");
    require(r.intent.operation.operation_scope == "scope" &&
                r.intent.operation.operation_ref == "original" &&
                r.intent.receipt_id == core::ReceiptId{7} &&
                r.intent.contribution_id == core::ContributionId{0} &&
                r.intent.plan.plan_ref == "plan-P" && r.intent.plan.revision == 1 &&
                r.intent.expected_state_token == "start" && r.persisted_head.token == "start" &&
                r.advanced_before.token == "advanced-before" && r.after.token == "original-after",
            "Frozen nonempty original identities");
    require(r.persisted_head.balances.size() == 2 && r.advanced_before.balances.size() == 2 &&
                r.after.balances.size() == 2 && r.effects.size() == 4 && r.jobs.size() == 2 &&
                r.transitions.empty() && r.reversals.empty(),
            "Whole returned20 vector sets");
    size_t components = 0;
    const std::array<std::int64_t, 4> requested{100, 200, 100, 1};
    for (size_t i = 0; i < r.effects.size(); ++i) {
        const auto &e = r.effects[i];
        require(e.request_ordinal == std::int64_t(i + 1) && e.requested_units == requested[i] &&
                    e.exact_requested_units == requested[i] && e.rounding_residual == 0 &&
                    e.application_explanation == raw(opaque),
                "Every retained ordered effect");
        components += e.components.size();
        if (i < 2)
            require(e.arithmetic_role == ApplicationArithmeticRole::time_addition &&
                        e.components.size() == 3 && e.components[0].kind == CK::main_delta &&
                        e.components[0].units == requested[i] &&
                        e.components[1].kind == CK::debt_delta && e.components[1].units == 0 &&
                        e.components[2].kind == CK::cap_limited_time && e.components[2].units == 0,
                    "Separate Main/debt/cap components");
        else
            require(e.arithmetic_role == ApplicationArithmeticRole::direct &&
                        e.components.size() == 1 && e.components[0].units == requested[i] &&
                        e.components[0].target_ref ==
                            std::optional<std::string>(i == 2 ? "score" : "count"),
                    "Direct counter occurrence amount/target");
    }
    require(components == 8 && 2 + 2 + 2 + 4 + components + 2 == 20, "Independent20 total");
    require(r.after.timer.main_remaining_ms == 300 && r.after.time_debt_ms == 0 &&
                r.after.balances[0].units == 100 && r.after.balances[1].units == 1 &&
                r.persisted_head.envelope.document == opaque &&
                r.advanced_before.envelope.document == opaque &&
                r.after.envelope.document == opaque,
            "Actual frozen state/opaque byte preservation");
    require(r.jobs[0].job_ref == "job-1" && r.jobs[1].job_ref == "job-2" &&
                r.jobs[0].source_request_ordinal == 1 && r.jobs[1].source_request_ordinal == 2 &&
                r.jobs[0].group_ref == std::optional<std::string>("g") &&
                r.jobs[1].group_ref == std::optional<std::string>("g") &&
                r.jobs[0].group_ordinal == 1 && r.jobs[1].group_ordinal == 2 &&
                r.jobs[0].envelope.document == opaque && !r.jobs[0].envelope.document.isNull() &&
                r.jobs[1].envelope.document.isEmpty() && !r.jobs[1].envelope.document.isNull(),
            "Complete ordered binary and present-empty job documents");
    require(r.intent.choices.document.isEmpty() && !r.intent.choices.document.isNull() &&
                r.result.document.isEmpty() && !r.result.document.isNull() &&
                r.audit.actor_ref == std::optional<std::string>(std::string()) &&
                !r.audit.decision_ref &&
                r.audit.reason == std::optional<std::string>(raw(opaque)) &&
                r.audit.envelope.document.isEmpty() && !r.audit.envelope.document.isNull(),
            "Owned absent/present-empty audit, choices and result");
}

SqlStatement revised_scalar(const Data &data) {
    const auto rows = data.minimal["setup"].toArray();
    auto result = statement(rows[9]);
    require(result.sql.startsWith("INSERT INTO main.subathon_plan_revisions"),
            "Frozen PA row location");
    result.bindings[1] = id_blob(2);
    result.bindings[2] = id_blob(1);
    result.bindings[30] = 2;
    result.bindings[31] = blob("moderator");
    result.bindings[32] = blob("later revision");
    result.bindings[33] = blob("later-decision");
    return result;
}
} // namespace

class SqliteOriginalClaimTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void frozenSelectorsPreserveGlobalPriorityAndRoles_data() {
        QTest::addColumn<QJsonObject>("row");
        const Data data;
        for (const auto &value : data.minimal["selectors"].toArray()) {
            const auto row = value.toObject();
            QTest::newRow(qPrintable(row["name"].toString())) << row;
        }
    }
    void frozenSelectorsPreserveGlobalPriorityAndRoles() {
        QFETCH(QJsonObject, row);
        scenario([&] {
            const Data data;
            Database db(data);
            ApplicationOriginalClaimSelector selector;
            selector.subject_kind = ApplicationSubjectKind(row["subject_kind"].toInt());
            bool valid = false;
            selector.receipt_id = {row["receipt_decimal"].toString().toULongLong(&valid)};
            require(valid, "Frozen full-range receipt predicate");
            if (!row["cid_decimal"].isNull()) {
                const auto id = row["cid_decimal"].toString().toULongLong(&valid);
                require(valid, "Frozen full-range CID predicate");
                selector.contribution_id = core::ContributionId{id};
            }
            const auto before = db.saved();
            Match selected;
            committed(read(db, selector, selected));
            if (row["expected_record"].isNull())
                require(!selected, "Frozen absent predicate");
            else
                match(selected,
                      row["expected_kind"].toString() == "subject"
                          ? SqliteOriginalClaimKind::subject
                          : SqliteOriginalClaimKind::source,
                      row["expected_record"].toString().front().toLatin1());
            require(db.saved() == before, "Pure read did not change any stored bytes");
            if (row["check_lower_priority_corruption"].toBool()) {
                (void)db.observer->rows("DELETE FROM subathon_app_audit WHERE "
                                        "operation_scope=X'62657461' AND operation_ref=X'42'");
                const auto unselected_fault = db.saved();
                committed(read(db, selector, selected));
                match(selected, SqliteOriginalClaimKind::subject, 'A');
                require(db.saved() == unselected_fault,
                        "Lower-priority corrupt B is outside selected A closure");
                return;
            }
        });
    }
    void originalFilterExcludesFinancialReversalRows() {
        scenario([&] {
            const Data data;
            Database db(data, true);
            db.modify([&](auto s) {
                execute(s, data.nonempty["scenarios"].toObject()["reversal_one"].toArray());
            });
            require(db.observer->rows("SELECT operation_kind FROM subathon_app_operations WHERE "
                                      "receipt_id=X'0000000000000008'") == SqlRows{{qint64(1)}},
                    "Actual receipt8 is a kind1 reversal in OLD fixture, not new correction C");
            Match selected;
            committed(read(db, subject(42, 8), selected));
            require(!selected, "ORIGINAL source filter does not return a financial reversal");
        });
    }
    void absentDoesNotAdmitCandidateOrAuditUnselectedNamespace() {
        scenario([&] {
            const Data data;
            Database db(data);
            (void)db.observer->rows("DELETE FROM subathon_app_audit WHERE "
                                    "operation_scope=X'62657461' AND operation_ref=X'42'");
            (void)db.observer->rows("PRAGMA foreign_keys=OFF");
            (void)db.observer->rows(
                "UPDATE subathon_app_head SET token=X'6D697373696E67' WHERE singleton=1");
            require(db.observer->rows("SELECT count(*) FROM subathon_receipts WHERE "
                                      "receipt_id=X'0000000000000009'") == SqlRows{{qint64(0)}} &&
                        db.observer->rows("SELECT count(*) FROM subathon_contributions WHERE "
                                          "contribution_id=X'000000000000002A'") ==
                            SqlRows{{qint64(0)}},
                    "Candidate source/CID actually missing; unrelated current namespace corrupt");
            const auto before = db.saved();
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                marker(s);
                marker_active(s);
                require(!SqliteApplicationBundleStore(s).lookup_original_claim(subject(42, 9)),
                        "Absent predicates do not audit/admit candidate or unrelated records");
                marker_active(s);
                s.commit();
                return 1;
            });
            committed(out);
            require(db.saved() != before &&
                        db.observer->rows("SELECT count(*) FROM subathon_retained_configurations "
                                          "WHERE reference=X'71756572792D7072696F72'") ==
                            SqlRows{{qint64(1)}},
                    "Healthy absence preserves and commits caller marker");
        });
    }
    void legacyAppliedWithoutBundleNamespaceRemainsAbsent() {
        scenario([&] {
            const Data data;
            Database db(data);
            db.modify([](auto s) {
                for (const auto *table :
                     {"subathon_app_audit", "subathon_app_results", "subathon_app_contributions",
                      "subathon_app_claims", "subathon_app_operations"})
                    (void)s.execute(
                        {"DELETE FROM main." + QString::fromLatin1(table) +
                             " WHERE operation_scope=X'616C706861' AND operation_ref=X'41'",
                         {}});
            });
            require(db.observer->rows("SELECT application FROM subathon_contributions WHERE "
                                      "contribution_id=X'0000000000000000'") ==
                        SqlRows{{qint64(1)}},
                    "Old current contribution remains applied, no prior bundle namespace invented");
            Match selected;
            committed(read(db, subject(0, 7), selected));
            require(!selected,
                    "No selected claim/source record means absence, not fabricated success");
        });
    }
    void historicalMatchIgnoresCurrentDimensionsAndLaterHealthyPlanHead() {
        scenario([&] {
            const Data data;
            Database db(data);
            db.modify([&](auto s) {
                (void)s.execute(revised_scalar(data));
                (void)s.execute(
                    {"UPDATE main.subathon_plan_headers SET head_revision=? WHERE plan_ref=?",
                     {id_blob(2), blob("PA")}});
                (void)s.execute({"UPDATE main.subathon_contributions SET "
                                 "payment_evidence=2,review=3,application=0 WHERE "
                                 "contribution_id=X'0000000000000000'",
                                 {}});
            });
            require(db.observer->rows("SELECT hex(token) FROM subathon_app_head") ==
                            SqlRows{{QString("64")}} &&
                        db.observer->rows("SELECT count(*),hex(min(revision)),hex(max(revision)) "
                                          "FROM subathon_plan_revisions WHERE plan_ref=X'5041'") ==
                            SqlRows{{qint64(2), QString("0000000000000001"),
                                     QString("0000000000000002")}},
                    "Today's head d and complete later PA1/2 differ from original A");
            Match selected;
            committed(read(db, subject(), selected));
            match(selected, SqliteOriginalClaimKind::subject, 'A');
        });
    }
    void selectedClosureFailureRollsBackBeforeCatch_data() {
        QTest::addColumn<int>("fault");
        const std::array<const char *, 9> names{"missing-result-subject-before-healthy-source",
                                                "wrong-class-audit",
                                                "missing-contribution-snapshot",
                                                "dangling-selected-claim",
                                                "source-fallback-missing-own-claim",
                                                "missing-selected-job",
                                                "selected-group-gap",
                                                "selected-receipt-wrong-SQL-class",
                                                "selected-plan-head-exceeds-history"};
        for (int i = 0; i < int(names.size()); ++i)
            QTest::newRow(names[size_t(i)]) << i;
    }
    void selectedClosureFailureRollsBackBeforeCatch() {
        QFETCH(int, fault);
        scenario([&] {
            const Data data;
            Database db(data, fault >= 5 && fault <= 7);
            (void)db.observer->rows("PRAGMA foreign_keys=OFF");
            (void)db.observer->rows("PRAGMA ignore_check_constraints=ON");
            const std::array<QString, 9> corruptions{
                "DELETE FROM subathon_app_results WHERE operation_scope=X'616C706861' AND "
                "operation_ref=X'41'",
                "UPDATE subathon_app_audit SET document='wrong-class' WHERE "
                "operation_scope=X'616C706861' AND operation_ref=X'41'",
                "DELETE FROM subathon_app_contributions WHERE operation_scope=X'616C706861' AND "
                "operation_ref=X'41'",
                "UPDATE subathon_app_claims SET operation_ref=X'6D697373696E67' WHERE "
                "operation_scope=X'616C706861' AND operation_ref=X'41'",
                "DELETE FROM subathon_app_claims WHERE operation_scope=X'616C706861' AND "
                "operation_ref=X'41'",
                "DELETE FROM subathon_app_jobs WHERE operation_scope=X'73636F7065' AND "
                "operation_ref=X'6F726967696E616C' AND ordinal=2",
                "UPDATE subathon_app_jobs SET group_ordinal=3 WHERE operation_scope=X'73636F7065' "
                "AND operation_ref=X'6F726967696E616C' AND ordinal=2",
                "UPDATE subathon_app_operations SET receipt_id='00000007' WHERE "
                "operation_scope=X'73636F7065' AND operation_ref=X'6F726967696E616C'",
                "UPDATE subathon_plan_headers SET head_revision=X'0000000000000002' WHERE "
                "plan_ref=X'5041'"};
            (void)db.observer->rows(corruptions[size_t(fault)]);
            (void)db.observer->rows("PRAGMA ignore_check_constraints=OFF");
            if (fault == 0)
                require(db.observer->rows("SELECT operation_scope,operation_ref FROM "
                                          "subathon_app_results ORDER BY operation_scope") ==
                            SqlRows{{blob("beta"), blob("B")}, {blob("gamma"), blob("C")}},
                        "A result actually missing while lower-priority B source result remains");
            else if (fault == 1)
                require(
                    db.observer->rows("SELECT typeof(document),document FROM subathon_app_audit "
                                      "WHERE operation_scope=X'616C706861'") ==
                        SqlRows{{QString("text"), QString("wrong-class")}},
                    "Selected audit class fault reached");
            else if (fault == 2)
                require(db.observer->rows("SELECT count(*) FROM subathon_app_contributions WHERE "
                                          "operation_scope=X'616C706861'") == SqlRows{{qint64(0)}},
                        "Historical CID snapshot removed");
            else if (fault == 3)
                require(db.observer->rows(
                            "SELECT c.operation_ref,(SELECT count(*) FROM subathon_app_operations "
                            "o WHERE o.operation_scope=c.operation_scope AND "
                            "o.operation_ref=c.operation_ref) FROM subathon_app_claims c WHERE "
                            "c.subject_id=X'0000000000000000'") ==
                            SqlRows{{blob("missing"), qint64(0)}},
                        "Selected claim genuinely dangles");
            else if (fault == 4)
                require(
                    db.observer->rows("SELECT count(*) FROM subathon_app_claims WHERE "
                                      "subject_id=X'0000000000000000'") == SqlRows{{qint64(0)}} &&
                        db.observer->rows("SELECT count(*) FROM subathon_app_operations WHERE "
                                          "receipt_id=X'0000000000000007'") == SqlRows{{qint64(1)}},
                    "Source remains to select missing-own-claim closure");
            else if (fault == 5)
                require(db.observer->rows("SELECT job_count,(SELECT count(*) FROM "
                                          "subathon_app_jobs) FROM subathon_app_results") ==
                            SqlRows{{qint64(2), qint64(1)}},
                        "Actual job-count mismatch");
            else if (fault == 6)
                require(db.observer->rows(
                            "SELECT group_ordinal FROM subathon_app_jobs ORDER BY ordinal") ==
                            SqlRows{{qint64(1)}, {qint64(3)}},
                        "Actual ordered group gap");
            else if (fault == 7)
                require(db.observer->rows(
                            "SELECT typeof(receipt_id),receipt_id FROM subathon_app_operations") ==
                            SqlRows{{QString("text"), QString("00000007")}},
                        "Actual selected typed reference corruption");
            else
                require(db.observer->rows(
                            "SELECT typeof(head_revision),hex(head_revision),"
                            "(SELECT count(*) FROM subathon_plan_revisions WHERE plan_ref=X'5041'),"
                            "(SELECT hex(min(revision)) FROM subathon_plan_revisions WHERE "
                            "plan_ref=X'5041'),"
                            "(SELECT hex(max(revision)) FROM subathon_plan_revisions WHERE "
                            "plan_ref=X'5041') "
                            "FROM subathon_plan_headers WHERE plan_ref=X'5041'") ==
                            SqlRows{{QString("blob"), QString("0000000000000002"), qint64(1),
                                     QString("0000000000000001"), QString("0000000000000001")}},
                        "Selected PA actual header2/count1/min1/max1 is corrupt, not healthy later "
                        "history");
            const auto before = db.saved();
            bool caught = false;
            const auto out = refused(db, subject(0, fault == 4 ? 7 : maximum_id),
                                     SR::invalid_stored_data, caught);
            known(out, SR::invalid_stored_data);
            require(caught && db.saved() == before,
                    "No fallback, repair, marker survival or partial result");
        });
    }
    void pureInputPriorityPreservesHealthyCallerWork() {
        scenario([&] {
            SqliteWorkSession expired;
            SqliteBundleLimits bad;
            bad.owned_bytes = 0;
            SqliteApplicationBundleStore invalid_options(expired, bad);
            input_failure(
                [&] {
                    (void)invalid_options.lookup_original_claim(
                        {ApplicationSubjectKind(99), {0}, {}});
                },
                IE::invalid_options);
            SqliteApplicationBundleStore store(expired);
            for (const auto &selector : std::array<ApplicationOriginalClaimSelector, 4>{
                     ApplicationOriginalClaimSelector{ApplicationSubjectKind(99), {7}, {}},
                     ApplicationOriginalClaimSelector{
                         ApplicationSubjectKind::contribution, {0}, core::ContributionId{0}},
                     ApplicationOriginalClaimSelector{
                         ApplicationSubjectKind::contribution, {7}, {}},
                     ApplicationOriginalClaimSelector{
                         ApplicationSubjectKind::receipt, {7}, core::ContributionId{0}}})
                input_failure([&] { (void)store.lookup_original_claim(selector); },
                              IE::invalid_source);
            const Data data;
            Database db(data);
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                marker(s);
                marker_active(s);
                SqliteApplicationBundleStore active(s);
                input_failure([&] { (void)active.lookup_original_claim(receipt(0)); },
                              IE::invalid_source);
                marker_active(s);
                const auto selected = active.lookup_original_claim(subject());
                match(selected, SqliteOriginalClaimKind::subject, 'A');
                s.commit();
                return 1;
            });
            committed(out);
            require(db.observer->rows("SELECT count(*) FROM subathon_retained_configurations WHERE "
                                      "reference=X'71756572792D7072696F72'") ==
                        SqlRows{{qint64(1)}},
                    "Pure rejection did not clean caller work");
        });
    }
    void fixedSelectorDoesNotAcquireAggregateInputQuota() {
        scenario([&] {
            SqliteBundleLimits small;
            small.input_items = 1;
            small.owned_bytes = 1;
            const SqliteApplicationBundleStore expired(SqliteWorkSession{}, small);
            work_failure([&] { (void)expired.lookup_original_claim(subject()); },
                         WorkError::stale_session);
            const Data data;
            Database db(data);
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                marker(s);
                require(
                    !SqliteApplicationBundleStore(s, small).lookup_original_claim(subject(42, 9)),
                    "Absent result has zero returned items/bytes despite fixed17-byte selector");
                marker_active(s);
                s.commit();
                return 1;
            });
            committed(out);
        });
    }
    void bodyPlusOneDiscriminatorHasExactOwnedBoundary_data() {
        QTest::addColumn<int>("tag");
        QTest::addColumn<int>("body_bytes");
        QTest::addColumn<int>("match_bytes");
        QTest::addColumn<bool>("source_match");
        QTest::newRow("A-161-plus-1") << int('A') << 161 << 162 << false;
        QTest::newRow("B-160-plus-1") << int('B') << 160 << 161 << false;
        QTest::newRow("B-source-160-plus-1") << int('B') << 160 << 161 << true;
    }
    void bodyPlusOneDiscriminatorHasExactOwnedBoundary() {
        QFETCH(int, tag);
        QFETCH(int, body_bytes);
        QFETCH(int, match_bytes);
        QFETCH(bool, source_match);
        scenario([&] {
            const Data data;
            Database db(data);
            const char expected_tag = char(tag);
            const auto selector = source_match          ? subject(42, maximum_id)
                                  : expected_tag == 'A' ? subject()
                                                        : subject(maximum_id);
            const auto expected_kind =
                source_match ? SqliteOriginalClaimKind::source : SqliteOriginalClaimKind::subject;
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                SqliteBundleLimits old_bounds;
                old_bounds.owned_bytes = body_bytes;
                const auto old = SqliteApplicationBundleStore(s, old_bounds)
                                     .lookup(expected_record(expected_tag).intent.operation);
                require(old.has_value(), "Original lookup's independent body boundary unchanged");
                same_record(*old, expected_tag);
                SqliteBundleLimits new_bounds;
                new_bounds.owned_bytes = match_bytes;
                match(SqliteApplicationBundleStore(s, new_bounds).lookup_original_claim(selector),
                      expected_kind, expected_tag);
                s.commit();
                return 1;
            });
            committed(out);
            SqliteBundleLimits over;
            over.owned_bytes = body_bytes;
            bool caught = false;
            const auto rejected = refused(db, selector, SR::result_limit, caught, over);
            known(rejected, SR::result_limit);
            require(caught, "One new enum byte is actually charged after full closure");
        });
    }
    void fullTwentyItemResultUsesOutputQuotaWithInputOne() {
        scenario([&] {
            const Data data;
            Database db(data, true);
            SqliteBundleLimits fit;
            fit.result_items = 20;
            fit.input_items = 1;
            Match selected;
            committed(read(db, subject(0, 7), selected, fit));
            require(selected && selected->kind == SqliteOriginalClaimKind::subject,
                    "Old complete original selected");
            check_nonempty(selected->existing);
            SqliteBundleLimits over = fit;
            over.result_items = 19;
            bool caught = false;
            const auto rejected = refused(db, subject(0, 7), SR::result_limit, caught, over);
            known(rejected, SR::result_limit);
            require(caught, "One-over result is never truncated or confused with input_items");
        });
    }
    void nonemptyOpaqueResultOwnsBytesAfterFacadeWriterAndFixtureClose() {
        scenario([&] {
            Match selected;
            {
                const Data data;
                Database db(data, true);
                committed(read(db, subject(0, 7), selected));
                require(selected && selected->kind == SqliteOriginalClaimKind::subject,
                        "Owned complete record returned");
                check_nonempty(selected->existing);
            }
            require(selected.has_value(), "No session/facade/writer/fixture backing survives");
            check_nonempty(selected->existing);
            auto independent_copy = selected->existing;
            independent_copy.after.envelope.document.fill('x');
            independent_copy.jobs[0].envelope.document.fill('y');
            independent_copy.effects[0].application_explanation.assign(6, 'z');
            check_nonempty(selected->existing);
            require(selected->existing.after.envelope.document ==
                        QByteArray::fromHex("efbbbf6100ff"),
                    "Exact six-byte BOM/NUL/FF survives, not empty-buffer ownership theater");
        });
    }
    void ownStagedClaimIsProvisionalUntilRollbackOrCommitAndReopen() {
        scenario([&] {
            const Data data;
            Database db(data, false, true);
            const auto before = db.saved();
            const auto expected = expected_record('A');
            const PreparedApplicationBundle prepared{expected.intent,
                                                     expected.advanced_before,
                                                     expected.after,
                                                     expected.advance_explanation,
                                                     {},
                                                     {},
                                                     {},
                                                     {},
                                                     expected.result,
                                                     expected.audit};
            const auto stage_and_read = [&](bool commit) {
                return capture(*db.writer, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    {
                        SqliteApplicationBundleStore store(s);
                        require(store.stage(prepared).disposition ==
                                    SqliteBundleStageDisposition::staged,
                                "Old implemented stage prepares literal A, not an expected-value "
                                "producer");
                        require(
                            db.observer->rows("SELECT count(*) FROM subathon_app_claims") ==
                                    SqlRows{{qint64(0)}} &&
                                db.observer->rows("SELECT application FROM subathon_contributions "
                                                  "WHERE contribution_id=X'0000000000000000'") ==
                                    SqlRows{{qint64(0)}},
                            "Independent committed outside source remains pending, claim "
                            "invisible");
                        match(store.lookup_original_claim(subject()),
                              SqliteOriginalClaimKind::subject, 'A');
                    }
                    require(s.execute({"SELECT count(*) FROM main.subathon_app_claims", {}}) ==
                                SqlRows{{qint64(1)}},
                            "Facade destruction did not clean caller's still-active staging");
                    if (commit)
                        s.commit();
                    else
                        s.rollback();
                    return 1;
                });
            };
            rolled_back(stage_and_read(false));
            require(db.saved() == before,
                    "Known rollback removes all oldstage/newread speculative application rows");
            committed(stage_and_read(true));
            require(db.observer->rows("SELECT count(*) FROM subathon_app_claims") ==
                            SqlRows{{qint64(1)}} &&
                        db.observer->rows("SELECT application FROM subathon_contributions WHERE "
                                          "contribution_id=X'0000000000000000'") ==
                            SqlRows{{qint64(1)}},
                    "Exactly one real durable application");
            db.reopen();
            Match selected;
            committed(read(db, subject(), selected));
            match(selected, SqliteOriginalClaimKind::subject, 'A');
        });
    }
    void cumulativeWriterBudgetExhaustionPrecedesNewReader() {
        scenario([&] {
            const Data data;
            Database db(data);
            auto writer_bounds = limits();
            writer_bounds.result_rows = 1;
            db.reopen(writer_bounds);
            const auto before = db.saved();
            bool caught = false;
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                marker(s);
                // This actual SELECT consumes the ONE allowed result row before
                // the new method. No assumption about its number of SQL calls.
                marker_active(s);
                marker_absent(*db.observer);
                try {
                    (void)store.lookup_original_claim(subject());
                } catch (const SqliteBundleStorageFailure &e) {
                    require(e.reason() == SR::result_limit,
                            "Actual cumulative writer row budget maps truthfully");
                    db.released();
                    require(db.saved() == before,
                            "Budget failure rolled earlier marker back inside catch");
                    caught = true;
                    throw;
                }
                throw std::runtime_error("Required cumulative reader failure missing");
            });
            known(out, SR::result_limit, WorkError::result_limit);
            require(caught, "Synchronous query error, not later owner abandonment");
        });
    }
    void foreignAndExpiredTicketsKeepActualCapabilityTruth() {
        scenario([&] {
            const Data data;
            Database db(data);
            std::optional<SqliteApplicationBundleStore> saved;
            std::optional<WorkError> foreign;
            std::optional<IE> pure;
            bool missing = false;
            QString thread_problem;
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                marker(s);
                SqliteApplicationBundleStore store(s);
                saved.emplace(store);
                std::thread worker([&] {
                    try {
                        (void)store.lookup_original_claim(subject());
                    } catch (const SqliteOriginalClaimNotImplemented &) {
                        missing = true;
                    } catch (const SqliteWorkFailure &e) {
                        foreign = e.error();
                    } catch (const std::exception &e) {
                        thread_problem = QString::fromUtf8(e.what());
                    }
                    try {
                        (void)store.lookup_original_claim(receipt(0));
                    } catch (const SqliteOriginalClaimNotImplemented &) {
                        missing = true;
                    } catch (const SqliteBundleInputFailure &e) {
                        pure = e.error();
                    } catch (const std::exception &e) {
                        thread_problem = QString::fromUtf8(e.what());
                    }
                });
                worker.join();
                if (missing)
                    throw SqliteOriginalClaimNotImplemented();
                require(
                    thread_problem.isEmpty() && foreign == WorkError::wrong_thread &&
                        pure == IE::invalid_source,
                    "Foreign ticket truth and pure input priority, no uncaught thread exception");
                marker_active(s);
                match(store.lookup_original_claim(subject()), SqliteOriginalClaimKind::subject,
                      'A');
                s.commit();
                return 1;
            });
            committed(out);
            require(saved.has_value(), "Saved owned inert facade ticket");
            work_failure([&] { (void)saved->lookup_original_claim(subject()); },
                         WorkError::stale_session);
        });
    }
    void terminalSessionMisuseIsSynchronousAndSticky() {
        scenario([&] {
            const Data data;
            Database db(data);
            const auto before = db.saved();
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                match(store.lookup_original_claim(subject()), SqliteOriginalClaimKind::subject,
                      'A');
                s.rollback();
                work_failure([&] { (void)store.lookup_original_claim(subject()); },
                             WorkError::session_closed);
                return 1;
            });
            // Existing writer contract: catching terminal misuse cannot erase it
            // into not_committed or manufacture a normal payload.
            rolled_back(out, WorkError::session_closed);
            require(db.saved() == before, "Terminal misuse changed no durable data");
        });
    }
    void existingRecordCannotTurnCommitAckLossIntoSuccess() {
        scenario([&] {
            const Data data;
            Database db(data);
            const auto before = db.saved();
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *db.writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "Actual old COMMIT ack-loss seam installed while idle");
            bool read_reached = false;
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                const auto selected =
                    SqliteApplicationBundleStore(s).lookup_original_claim(subject());
                match(selected, SqliteOriginalClaimKind::subject, 'A');
                read_reached = true;
                s.commit();
                return QVariant("must not become normal success");
            });
            no_missing(out);
            require(read_reached && out.synchronous == WorkError::uncertain_outcome &&
                        !out.storage && !out.input && out.problem.isEmpty() &&
                        out.completion.transaction == TransactionOutcome::outcome_unknown &&
                        out.completion.error == WorkError::uncertain_outcome &&
                        !out.completion.value,
                    "Old known record cannot override this attempt's actual unknown/no payload");
            require(db.saved() == before, "Read-only physical COMMIT changed no accounting bytes");
        });
    }
    void rollbackAckLossOutranksSelectedCorruption() {
        scenario([&] {
            const Data data;
            Database db(data);
            (void)db.observer->rows("DELETE FROM subathon_app_results WHERE "
                                    "operation_scope=X'616C706861' AND operation_ref=X'41'");
            require(db.observer->rows("SELECT count(*) FROM subathon_app_results WHERE "
                                      "operation_scope=X'616C706861'") == SqlRows{{qint64(0)}},
                    "Actual selected result is missing, not a fixture setup exception");
            const auto before = db.saved();
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *db.writer, WorkControlFault::lose_next_rollback_ack) == StorageError::none,
                    "Actual ROLLBACK ack-loss seam installed while idle");
            bool synchronous_unknown = false;
            const auto out = capture(*db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                marker(s);
                marker_active(s);
                try {
                    (void)store.lookup_original_claim(subject());
                } catch (const SqliteWorkFailure &e) {
                    require(e.error() == WorkError::uncertain_outcome,
                            "Actual cleanup unknown stays original WorkFailure");
                    // Actual rollback happened, but lost acknowledgement is STILL
                    // unknown; physical observation does not recategorize outcome.
                    db.released();
                    require(db.saved() == before,
                            "Physical rollback inside catch while facade remains alive");
                    synchronous_unknown = true;
                    throw;
                }
                throw std::runtime_error("Required synchronous uncertainty missing");
            });
            // Explicit marker propagation prevents a stub followed by owner
            // cleanup ack-loss from accidentally passing an unknown-outcome test.
            no_missing(out);
            require(synchronous_unknown && out.synchronous == WorkError::uncertain_outcome &&
                        !out.storage && !out.input && out.problem.isEmpty() &&
                        out.completion.transaction == TransactionOutcome::outcome_unknown &&
                        out.completion.error == WorkError::uncertain_outcome &&
                        !out.completion.value,
                    "Unknown outranks local selected-data category, no normal payload");
        });
    }
    void fullSelectedExactClassSizeAndGrammarRemainStrict_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("both-classes-before-size") << 0;
        QTest::newRow("size-before-grammar") << 1;
        QTest::newRow("equivalent-noncanonical") << 2;
    }
    void fullSelectedExactClassSizeAndGrammarRemainStrict() {
        QFETCH(int, fault);
        scenario([&] {
            const Data data;
            Database db(data, true);
            const auto numerator = fault == 2 ? QByteArray("200") : QByteArray::fromHex("ff000080");
            // Fault injection uses literal raw values through the IDLE independent
            // observer. SELECTs below prove the actual class/size before UUT call.
            (void)db.observer->rows("PRAGMA ignore_check_constraints=ON");
            const auto denominator_sql = fault == 0   ? QString("'1'")
                                         : fault == 2 ? QString("X'32'")
                                                      : QString("X'31'");
            (void)db.observer->rows("UPDATE subathon_app_effects SET exact_numerator=X'" +
                                    QString::fromLatin1(numerator.toHex()) +
                                    "',exact_denominator=" + denominator_sql +
                                    " WHERE operation_scope=X'73636F7065' AND "
                                    "operation_ref=X'6F726967696E616C' AND request_ordinal=3");
            (void)db.observer->rows("PRAGMA ignore_check_constraints=OFF");
            require(db.observer->rows("SELECT "
                                      "typeof(exact_numerator),length(exact_numerator),hex(exact_"
                                      "numerator),typeof(exact_denominator),hex(exact_denominator) "
                                      "FROM subathon_app_effects WHERE request_ordinal=3") ==
                        SqlRows{{QString("blob"), qint64(fault == 2 ? 3 : 4),
                                 QString::fromLatin1(numerator.toHex().toUpper()),
                                 QString(fault == 0 ? "text" : "blob"),
                                 QString(fault == 2 ? "32" : "31")}},
                    "Independent feasible Exact fault class/size/bytes");
            SqliteBundleLimits bounds;
            bounds.component_bytes = 3;
            const auto expected = fault == 1 ? SR::result_limit : SR::invalid_stored_data;
            bool caught = false;
            const auto out = refused(db, subject(0, 7), expected, caught, bounds);
            known(out, expected);
            require(
                caught,
                "Full selected reader inherits class then size then canonical reduction priority");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteOriginalClaimTests)
#include "sqlite_original_claim_tests.moc"
