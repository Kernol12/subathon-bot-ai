// SPDX-License-Identifier: GPL-3.0-or-later
// H01–H16: frozen SQL foundation; feature bodies await separate review/first red.
#include "support/sqlite_application_hold_fixture.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <array>
#include <functional>
#include <limits>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>
#include <tuple>

using namespace application_hold_fixture;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;
namespace {
using IE = SqliteHoldInputError;
using SR = SqliteHoldStorageReason;
using MD = SqliteHoldMutationDisposition;
using HD = SqliteHoldHistoryDisposition;
constexpr auto maximum_id = std::numeric_limits<std::uint64_t>::max();
ApplicationHoldSubject cid(std::uint64_t n = 0) { return {{core::ContributionId{n}}}; }
ApplicationHoldSubject receipt(std::uint64_t n) { return {{core::ReceiptId{n}}}; }

// Assertion-only structural projection, NOT a storage codec or byte oracle.
// Lengths/tags distinguish optional absence, empty DATA and nested owning copies.
#define FIELDS(Type, ...)                                                                          \
    auto fields(const Type &x) { return std::tie(__VA_ARGS__); }
FIELDS(ApplicationOperationKey, x.operation_scope, x.operation_ref)
FIELDS(ApplicationPlanKey, x.plan_ref, x.revision)
FIELDS(ApplicationEnvelope, x.format_ref, x.document)
FIELDS(ApplicationHoldSubject, x.id)
FIELDS(ApplicationHoldSource, x.subject, x.receipt_id, x.event_ref, x.parent_event_ref,
       x.root_event_ref, x.configuration_ref, x.arrival_context_ref)
FIELDS(ApplicationHoldCommand, x.operation, x.intent)
FIELDS(ApplicationHoldObservation, x.persisted_state_token, x.annotation, x.policy_revision_ref,
       x.permission_revision_ref, x.actor_ref, x.decision_ref, x.observed_utc_ms, x.elapsed_ms)
FIELDS(ApplicationHoldProposedOccurrence, x.ordinal, x.action_ref, x.action_version_ref, x.rule_ref,
       x.target_ref, x.unit, x.attempted_units, x.finalized_units, x.explanation)
FIELDS(ApplicationHoldPlanEvidence, x.kind, x.plan, x.extent, x.stopped_for, x.failed_capture,
       x.occurrences)
FIELDS(ApplicationHoldFailureEvidence, x.stage, x.failing_ordinal, x.failing_target_ref,
       x.attempted_units, x.annotation)
FIELDS(ApplicationHoldAssociatedApplication, x.operation, x.request_ordinal)
FIELDS(ApplicationHoldTimerEvidence, x.request_ordinal, x.component_kind, x.request_units,
       x.deficit_policy, x.deficit_disposition, x.unapplied_units, x.composition,
       x.composition_decision, x.associated)
FIELDS(ApplicationHoldCause, x.hold_ref, x.source, x.command, x.first_observation, x.plan_evidence,
       x.kind, x.failure, x.timer, x.missing_policy_reason, x.supersedes_hold_ref)
FIELDS(ApplicationHoldDetectionData, x.mutation_ref, x.expected_version_ref, x.proposed_version_ref,
       x.observation, x.audit)
FIELDS(ApplicationHoldDetection, x.cause, x.request)
FIELDS(ApplicationHoldVersionLocator, x.hold_ref, x.version_ref)
FIELDS(ApplicationHoldReplacementLocator, x.hold_ref, x.detection_ref)
FIELDS(ApplicationHoldVersion, x.locator, x.position, x.parent_version_ref, x.state,
       x.producer_mutation_ref, x.replacement)
FIELDS(ApplicationHoldHeldOrigin, x.version, x.request_ordinal, x.component_kind, x.remaining_units)
FIELDS(ApplicationHoldApplicationLink, x.kind, x.operation, x.receipt_id, x.plan, x.held_origin)
FIELDS(ApplicationHoldResolutionEntry, x.hold_ref, x.expected_version_ref, x.proposed_version_ref,
       x.decision, x.replacement)
FIELDS(ApplicationHoldResolutionBatch, x.mutation_ref, x.subject, x.expected_active, x.entries,
       x.actor_ref, x.decision_ref, x.reason, x.options, x.application)
FIELDS(ApplicationHoldRecordedResolution, x.request, x.produced_versions)
FIELDS(ApplicationHoldView, x.cause, x.first_detection, x.first_version, x.current_version,
       x.current_event)
FIELDS(ApplicationHoldActiveSet, x.subject, x.holds)
FIELDS(ApplicationHoldHistoryEntry, x.version, x.event)
FIELDS(ApplicationHoldHistoryPage, x.hold_ref, x.checked_version_ref, x.entries, x.next_position,
       x.has_more)
FIELDS(ApplicationHoldHistoryResult, x.disposition, x.page)
FIELDS(ApplicationHoldMutationOutcome, x.mutation_ref, x.kind, x.produced_versions)
FIELDS(ApplicationHoldMutationResult, x.disposition, x.outcome)
#undef FIELDS

template <class T> QByteArray projection(const T &x);
QByteArray framed(const QByteArray &x) { return QByteArray::number(x.size()) + ':' + x; }
template <class T> QByteArray projection(const T &x) {
    if constexpr (std::is_same_v<T, QByteArray>)
        return QByteArray(x.isNull() ? "N" : "B") + framed(x);
    else if constexpr (std::is_same_v<T, std::string>)
        return framed(QByteArray(x.data(), qsizetype(x.size())));
    else if constexpr (std::is_enum_v<T> || std::is_integral_v<T>)
        return framed(QByteArray::number(qint64(x)));
    else if constexpr (std::is_same_v<T, core::ReceiptId> ||
                       std::is_same_v<T, core::ContributionId>)
        return framed(QByteArray::number(quint64(x.value)));
    else if constexpr (std::is_same_v<T, core::Exact>)
        return projection(boost::multiprecision::numerator(x).str()) +
               projection(boost::multiprecision::denominator(x).str());
    else if constexpr (requires { fields(x); })
        return std::apply([](const auto &...v) { return (QByteArray{} + ... + projection(v)); },
                          fields(x));
    else if constexpr (requires { x.has_value(); })
        return x ? QByteArray("+") + projection(*x) : QByteArray("-");
    else if constexpr (requires { x.index(); })
        return projection(x.index()) + std::visit([](const auto &v) { return projection(v); }, x);
    else {
        QByteArray result = projection(x.size());
        for (const auto &v : x)
            result += projection(v);
        return result;
    }
}
template <class T> void same(const T &actual, const T &expected) {
    require(projection(actual) == projection(expected), "Complete independent owned DTO equality");
}

struct Outcome {
    bool done{false}, missing{false};
    std::optional<IE> input;
    std::optional<SR> storage;
    std::optional<WorkError> synchronous;
    QString problem;
    SqliteWorkResult completion;
};
Outcome capture(SqliteWriter &writer, SqliteWriter::Work body) {
    auto state = std::make_shared<Outcome>();
    require(writer.enqueue_work(
                [state, body = std::move(body)](auto c) -> QVariant {
                    try {
                        return body(c);
                    } catch (const SqliteApplicationHoldNotImplemented &) {
                        state->missing = true;
                        throw;
                    } catch (const SqliteHoldInputFailure &e) {
                        state->input = e.error();
                        throw;
                    } catch (const SqliteHoldStorageFailure &e) {
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
                [state](auto r) {
                    state->completion = std::move(r);
                    state->done = true;
                }) == StorageError::none,
            "Real writer admits feature unit");
    require(QTest::qWaitFor([state] { return state->done; }, 20000), "Real completion observed");
    return *state;
}
void no_missing(const Outcome &r) {
    if (r.missing)
        throw SqliteApplicationHoldNotImplemented();
}
void committed(const Outcome &r) {
    no_missing(r);
    require(r.done && r.completion.transaction == TransactionOutcome::committed &&
                r.completion.error == WorkError::none && r.completion.value && !r.input &&
                !r.storage && !r.synchronous && r.problem.isEmpty(),
            "Normal acknowledged committed completion: " + r.problem);
}
void rolled_back(const Outcome &r, WorkError e = WorkError::not_committed) {
    no_missing(r);
    require(r.done && r.completion.transaction == TransactionOutcome::rolled_back &&
                r.completion.error == e && !r.completion.value && !r.input && !r.storage &&
                !r.synchronous && r.problem.isEmpty(),
            "Actual known caller rollback");
}
void known(const Outcome &r, SR reason, WorkError e = WorkError::work_exception) {
    no_missing(r);
    require(r.storage == reason && !r.input && !r.synchronous && r.problem.isEmpty() &&
                r.completion.transaction == TransactionOutcome::rolled_back &&
                r.completion.error == e && !r.completion.value,
            "Exact synchronous hold reason after actual whole-unit rollback");
}
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const SqliteApplicationHoldNotImplemented &e) {
        const auto message =
            QString("HOLD-MISSING[%1/%2]: %3")
                .arg(QTest::currentTestFunction(), QTest::currentDataTag(), e.what());
        QTest::qFail(qPrintable(message), __FILE__, __LINE__);
    } catch (const std::exception &e) {
        const auto message =
            QString("HOLD-ASSERT[%1/%2]: %3")
                .arg(QTest::currentTestFunction(), QTest::currentDataTag(), e.what());
        QTest::qFail(qPrintable(message), __FILE__, __LINE__);
    }
}
template <class Body> void input_failure(Body body, IE error) {
    try {
        body();
    } catch (const SqliteHoldInputFailure &e) {
        require(e.error() == error, "Exact pure input category");
        return;
    }
    throw std::runtime_error("Required pure input failure missing");
}
template <class Body> void work_failure(Body body, WorkError error) {
    try {
        body();
    } catch (const SqliteWorkFailure &e) {
        require(e.error() == error, "Original capability/protocol error");
        return;
    }
    throw std::runtime_error("Required original WorkFailure missing");
}
QString quoted(QString s) {
    s.replace('"', "\"\"");
    return '"' + s + '"';
}
QByteArray snapshot(Observer &observer) {
    QByteArray result;
    const auto tables =
        observer.rows("SELECT name FROM sqlite_schema WHERE type='table' ORDER BY name");
    for (const auto &table : tables) {
        const auto name = table.front().toString();
        result += framed(name.toUtf8());
        QStringList columns;
        for (const auto &col : observer.rows("PRAGMA table_info(" + quoted(name) + ")")) {
            const auto key = quoted(col[1].toString());
            columns << "typeof(" + key + ")" << key;
        }
        const auto rows = observer.rows("SELECT " + columns.join(',') + " FROM " + quoted(name) +
                                        " ORDER BY rowid");
        result += framed(QByteArray::number(rows.size()));
        for (const auto &row : rows)
            for (qsizetype i = 0; i < row.size(); i += 2) {
                const auto type = row[i].toString();
                result += framed(type.toUtf8());
                result += framed(type == "null"      ? QByteArray{}
                                 : type == "blob"    ? row[i + 1].toByteArray()
                                 : type == "integer" ? QByteArray::number(row[i + 1].toLongLong())
                                                     : row[i + 1].toString().toUtf8());
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
    explicit Database(const Data &d, const char *arrangement = "pending",
                      bool sources_only = false) {
        require(directory.isValid(), "Private synthetic database");
        path = directory.filePath("holds.sqlite");
        open();
        (void)application_hold_fixture::run(*writer, [&](auto c) -> QVariant {
            const auto s = c.begin();
            if (!sources_only)
                d.populate(s, arrangement);
            else {
                d.schema(s);
                const auto steps =
                    d.frozen["arrangements"].toObject()["pending"].toObject()["setup"].toArray();
                require(
                    steps.size() == 402 &&
                        steps[28].toObject()["sql"].toString().startsWith(
                            "INSERT INTO main.subathon_app_head") &&
                        steps[29].toObject()["sql"].toString().startsWith(
                            "INSERT INTO main.subathon_hold_mutations"),
                    "Approved literal source-only boundary; never populate/delete hold history");
                for (qsizetype i = 0; i < 29; ++i)
                    (void)s.execute(original_claim_fixture::statement(steps[i]));
                require(s.execute({"SELECT (SELECT count(*) FROM subathon_app_operations),"
                                   "(SELECT count(*) FROM subathon_hold_causes),hex(token) FROM "
                                   "subathon_app_head",
                                   {}}) == SqlRows{{qint64(0), qint64(0), QString("70")}},
                        "Never-applied source namespace p, no manufactured state ABA");
            }
            s.commit();
            return 1;
        });
        observer = std::make_unique<Observer>(path);
    }
    void open(StorageLimits l = limits()) {
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, l) == StorageError::none, "Actual old writer open");
    }
    void reopen(StorageLimits l = limits()) {
        writer.reset();
        open(l);
    }
    void modify(const QString &sql) { (void)observer->rows(sql); }
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
                       {}}) == SqlRows{{qint64(1)}},
            "Caller marker remains actually staged");
}
template <class Body> Outcome refusal(Database &db, Body body, SR reason) {
    const auto before = db.saved();
    bool caught = false;
    auto r = capture(*db.writer, [&](auto c) -> QVariant {
        const auto s = c.begin();
        marker(s);
        try {
            body(s);
        } catch (const SqliteHoldStorageFailure &e) {
            require(e.reason() == reason, "Exact selected/incoming storage category");
            db.released();
            require(db.saved() == before,
                    "Whole caller unit rolled back INSIDE catch, before abandonment");
            caught = true;
            throw;
        }
        throw std::runtime_error("Required storage failure missing");
    });
    no_missing(r);
    require(caught && db.saved() == before, "Known refusal preserved all previous durable bytes");
    return r;
}
ApplicationHoldVersion produced(std::string hold, std::string version, std::int64_t position,
                                std::optional<std::string> parent, ApplicationHoldState state,
                                std::string mutation,
                                std::optional<ApplicationHoldReplacementLocator> replacement = {}) {
    return {{std::move(hold), std::move(version)},
            position,
            std::move(parent),
            state,
            std::move(mutation),
            std::move(replacement)};
}
ApplicationHoldMutationResult mutation(MD disposition, const std::string &ref,
                                       ApplicationHoldMutationKind kind,
                                       std::vector<ApplicationHoldVersion> versions) {
    return {disposition, ApplicationHoldMutationOutcome{ref, kind, std::move(versions)}};
}
void disposition(const ApplicationHoldMutationResult &r, MD expected) {
    require(r.disposition == expected && !r.outcome,
            "Healthy refusal has exact disposition/no historical payload");
}
ApplicationHoldDetection append(const Data &d, const char *cause, std::string ref,
                                std::optional<std::string> expected, std::string next) {
    auto r = d.detection(cause);
    r.request.mutation_ref = std::move(ref);
    r.request.expected_version_ref = std::move(expected);
    r.request.proposed_version_ref = std::move(next);
    return r;
}
ApplicationHoldResolutionBatch close_zero() {
    return {"rClose",
            cid(),
            {{"hOverflow", "v1"}, {"hPause", "v65"}},
            {{"hOverflow", "v1", "v2", {"decision", QByteArray("", 0)}, {}}},
            "reviewer",
            "decision",
            "",
            {"options", QByteArray("", 0)},
            {}};
}
template <class Body> Outcome read(Database &db, Body body) {
    return capture(*db.writer, [&](auto c) -> QVariant {
        const auto s = c.begin();
        body(s);
        s.commit();
        return 1;
    });
}
} // namespace

class SqliteApplicationHoldTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void manifestPreservesFivePrefixesAndExactFrozenStepSix() {
        scenario([] {
            const Data d;
            const auto manifest = application_hold_repository_manifest();
            require(manifest.catalog_format == 1 &&
                        manifest.store_identity == "subathon.sqlite-store" &&
                        manifest.supported_starting_versions ==
                            QList<qint32>{0, 1, 2, 3, 4, 5, 6} &&
                        manifest.steps.size() == 6,
                    "Pure factory returns complete exact catalog domain");
            for (qsizetype i = 0; i < 6; ++i) {
                const auto expected = i < 5 ? d.old.manifests[i] : d.frozen["manifest"].toObject();
                const auto &step = manifest.steps[i];
                require(step.version == i + 1 && step.id == expected["id"].toString() &&
                            step.sql.size() == expected["sql"].toArray().size() &&
                            step.expected_schema.size() ==
                                expected["expected_schema"].toArray().size(),
                        "Frozen step identity and independent SQL/object cardinalities");
                for (qsizetype j = 0; j < step.sql.size(); ++j)
                    require(step.sql[j] == expected["sql"].toArray()[j].toString(),
                            "Every exact literal DDL statement/order");
                for (qsizetype j = 0; j < step.expected_schema.size(); ++j) {
                    const auto o = expected["expected_schema"].toArray()[j].toObject();
                    require(step.expected_schema[j] ==
                                CatalogObject{o["type"].toString(), o["name"].toString(),
                                              o["table_name"].toString(),
                                              o["sql"].isNull()
                                                  ? std::nullopt
                                                  : std::optional{o["sql"].toString()}},
                            "Exact SQL/NULL object tuple, no UUT-produced expected catalog");
                }
                require(canonical_migration_payload(step).toHex() ==
                                expected["payload_hex"].toString().toLatin1() &&
                            migration_content_sha256(step).toHex() ==
                                expected["sha256_hex"].toString().toLatin1(),
                        "Frozen framed payload and digest including unchanged prefix1–5");
            }
            require(manifest.steps[5].sql.size() == 12 &&
                        manifest.steps[5].expected_schema.size() == 89 &&
                        canonical_migration_payload(manifest.steps[5]).size() == 89890,
                    "Exact approved step6/full89, not an empty manifest stub");
        });
    }
    void explicitMigrationPreservesEachSupportedPrefix_data() {
        QTest::addColumn<int>("prefix");
        for (int i = 0; i <= 6; ++i)
            QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void explicitMigrationPreservesEachSupportedPrefix() {
        QFETCH(int, prefix);
        scenario([&] {
            const Data d;
            // A separate private empty file avoids deleting a committed catalog.
            QTemporaryDir directory;
            SqliteWriter writer;
            const auto path = directory.filePath("migrate.sqlite");
            require(directory.isValid() && writer.open(path, limits()) == StorageError::none,
                    "Migration private writer");
            if (prefix)
                (void)application_hold_fixture::run(writer, [&](auto c) -> QVariant {
                    const auto s = c.begin();
                    if (prefix == 6)
                        d.schema(s);
                    else {
                        for (const auto &value :
                             d.old.manifests.front()["expected_schema"].toArray()) {
                            const auto o = value.toObject();
                            if (o["name"] == "subathon_catalog" ||
                                o["name"] == "subathon_migration_history")
                                (void)s.execute({o["sql"].toString(), {}});
                        }
                        for (int i = 0; i < prefix; ++i) {
                            const auto m = d.old.manifests[i];
                            for (const auto &sql : m["sql"].toArray())
                                (void)s.execute({sql.toString(), {}});
                            (void)s.execute(
                                {"INSERT INTO subathon_migration_history VALUES(?,?,?)",
                                 {i + 1, m["id"].toString(),
                                  QByteArray::fromHex(m["sha256_hex"].toString().toLatin1())}});
                        }
                        (void)s.execute(
                            {"INSERT INTO subathon_catalog VALUES(1,1,'subathon.sqlite-store')",
                             {}});
                    }
                    s.commit();
                    return 1;
                });
            Observer observer(path);
            const auto old_history =
                prefix ? observer.rows("SELECT version,migration_id,content_sha256 FROM "
                                       "subathon_migration_history ORDER BY version")
                       : SqlRows{};
            const auto manifest = application_hold_repository_manifest();
            const auto request = [&](MigrationMode mode) {
                bool done = false;
                MigrationResult result;
                const auto admission =
                    SqliteMigrationRunner::enqueue(writer, {mode, manifest, {}, {}}, [&](auto r) {
                        result = std::move(r);
                        done = true;
                    });
                require(admission.error == StorageError::none &&
                            admission.reason == MigrationReason::none &&
                            QTest::qWaitFor([&] { return done; }, 20000),
                        "Real explicit migration admitted/completed");
                require(result.reason == MigrationReason::none &&
                            result.work.transaction == TransactionOutcome::committed &&
                            result.work.error == WorkError::none && result.report,
                        "Known acknowledged migration result");
                return *result.report;
            };
            const auto inspected = request(MigrationMode::inspect);
            require(inspected.version == prefix && inspected.target_version == 6 &&
                        inspected.applied == 0 &&
                        inspected.state == (prefix == 0   ? CatalogState::fresh
                                            : prefix == 6 ? CatalogState::current
                                                          : CatalogState::upgrade_required),
                    "Inspection never implicitly upgrades");
            const auto advanced = request(MigrationMode::advance);
            require(advanced.version == 6 && advanced.target_version == 6 &&
                        advanced.applied == 6 - prefix && advanced.state == CatalogState::current,
                    "Only explicit advance applies missing steps");
            const auto history = observer.rows("SELECT version,migration_id,content_sha256 FROM "
                                               "subathon_migration_history ORDER BY version");
            require(history.size() == 6 &&
                        observer.rows("SELECT count(*) FROM sqlite_schema") ==
                            SqlRows{{qint64(89)}} &&
                        observer.rows("PRAGMA foreign_key_check").empty(),
                    "Exact migrated inventory and foreign keys");
            for (qsizetype i = 0; i < old_history.size(); ++i)
                require(history[i] == old_history[i], "Existing migration history unchanged");
            require(request(MigrationMode::advance).applied == 0,
                    "Current catalog repeat is no-op");
        });
    }
    void globalTaggedSubjectsReturnCompleteOwnedViews_data() {
        QTest::addColumn<int>("which");
        const char *names[]{"CID0-two-independent-causes", "CIDMAX-no-signed-narrowing",
                            "Receipt8-non-support",        "ReceiptMAX-distinct-from-CIDMAX",
                            "Receipt7-distinct-from-CID0", "CID7-distinct-from-Receipt7"};
        for (int i = 0; i < 6; ++i)
            QTest::newRow(names[i]) << i;
    }
    void globalTaggedSubjectsReturnCompleteOwnedViews() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            std::optional<ApplicationHoldActiveSet> result;
            {
                Database db(d);
                const auto s = which == 0   ? cid()
                               : which == 1 ? cid(maximum_id)
                               : which == 2 ? receipt(8)
                               : which == 3 ? receipt(maximum_id)
                               : which == 4 ? receipt(7)
                                            : cid(7);
                committed(read(db, [&](auto ticket) {
                    result = SqliteApplicationHoldStore(ticket).active(s);
                }));
                ApplicationHoldActiveSet expected{s, {}};
                if (which == 0)
                    expected.holds = {d.view("hOverflow"), d.view("hPause")};
                if (which == 1)
                    expected.holds = {d.view("hFail"), d.view("hNew")};
                if (which == 2)
                    expected.holds = {d.view("hReceipt")};
                same(*result, expected);
                require(db.observer->rows("SELECT count(*) FROM subathon_app_operations") ==
                            SqlRows{{qint64(0)}},
                        "Whole-input holds created no successful original");
            }
            require(result.has_value(), "Owned set survives facade/writer/file destruction");
            if (which == 0) {
                same(result->holds[0], d.view("hOverflow"));
                require(result->holds[0].cause.command.operation.operation_ref !=
                                result->holds[1].cause.command.operation.operation_ref &&
                            result->holds[0].cause.plan_evidence.plan->plan_ref !=
                                result->holds[1].cause.plan_evidence.plan->plan_ref,
                        "Complete same-subject gate includes distinct commands and captured plans");
                auto copy = *result;
                copy.holds[0].cause.command.intent.document.fill('x');
                same(result->holds[0], d.view("hOverflow"));
            }
        });
    }
    void currentViewAndFiniteHistoricalResolutionAreDifferentValues() {
        scenario([] {
            const Data d;
            Database db(d);
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                same(store.find("hPause"), std::optional{d.view("hPause")});
                same(store.find("hOld"), std::optional{d.view("hOld")});
                same(store.find("hNew"), std::optional{d.view("hNew")});
                require(!store.find("missing"), "Only absent anchor gives absence");
                same(store.history({"hOld", 0, {}, 64}),
                     ApplicationHoldHistoryResult{HD::ready, d.history_page("old_resolution")});
                same(store.history({"hPause", 0, {}, 2}),
                     ApplicationHoldHistoryResult{HD::ready, d.history_page("pause_first2")});
            }));
        });
    }
    void historyChecksHeadBeforeCursorAndReturnsExactContinuation_data() {
        QTest::addColumn<int>("which");
        const char *names[]{"missing",         "stale-before-nonexistent-cursor",
                            "cursor-conflict", "empty-tail",
                            "first64",         "sixty-fifth"};
        for (int i = 0; i < 6; ++i)
            QTest::newRow(names[i]) << i;
    }
    void historyChecksHeadBeforeCursorAndReturnsExactContinuation() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            committed(read(db, [&](auto s) {
                ApplicationHoldHistoryQuery query{"hPause", 0, {}, 64};
                if (which == 0)
                    query.hold_ref = "missing";
                if (which == 1) {
                    query.after_position = 999;
                    query.checked_version_ref = "v64";
                }
                if (which == 2) {
                    query.after_position = 999;
                    query.checked_version_ref = "v65";
                }
                if (which == 3) {
                    query.after_position = 65;
                    query.checked_version_ref = "v65";
                }
                if (which == 5) {
                    query.after_position = 64;
                    query.checked_version_ref = "v65";
                }
                const auto r = SqliteApplicationHoldStore(s).history(query);
                if (which < 3)
                    require(r.disposition == (which == 0   ? HD::missing_hold
                                              : which == 1 ? HD::stale_history
                                                           : HD::cursor_conflict) &&
                                !r.page,
                            "Exact healthy nonpage result");
                else {
                    require(r.disposition == HD::ready && r.page && r.page->hold_ref == "hPause" &&
                                r.page->checked_version_ref == "v65",
                            "Actual checked current head");
                    const auto &p = *r.page;
                    require(p.entries.size() == (which == 3   ? 0
                                                 : which == 4 ? 64
                                                              : 1) &&
                                p.has_more == (which == 4) &&
                                p.next_position ==
                                    (which == 4 ? std::optional<std::int64_t>{64} : std::nullopt),
                            "Exact finite page/tail, no phantom cursor");
                    for (size_t i = 0; i < p.entries.size(); ++i) {
                        const auto n = which == 5 ? 65 : std::int64_t(i + 1);
                        const auto ref = "dPause" + std::to_string(n), v = "v" + std::to_string(n);
                        same(p.entries[i].version,
                             produced("hPause", v, n,
                                      n == 1 ? std::nullopt
                                             : std::optional{"v" + std::to_string(n - 1)},
                                      ApplicationHoldState::active, ref));
                        same(p.entries[i].event,
                             ApplicationHoldHistoryEvent{d.detection(ref.c_str())});
                    }
                }
            }));
        });
    }
    void detectionStagingIsProvisionalAndNeverAppliesWholeOverflow_data() {
        QTest::addColumn<QString>("name");
        for (const auto *n : {"dOverflow", "dFail", "dReceipt"})
            QTest::newRow(n) << QString(n);
    }
    void detectionStagingIsProvisionalAndNeverAppliesWholeOverflow() {
        QFETCH(QString, name);
        scenario([&] {
            const Data d;
            Database db(d, "pending", true);
            const auto detector = d.detection(qPrintable(name));
            const auto expected = d.view(detector.cause.hold_ref.c_str());
            const auto before = db.saved();
            const auto attempt = [&](bool commit) {
                return capture(*db.writer, [&](auto c) -> QVariant {
                    const auto s = c.begin();
                    marker(s);
                    {
                        SqliteApplicationHoldStore store(s);
                        same(store.record_detection(detector),
                             mutation(MD::recorded, detector.request.mutation_ref,
                                      ApplicationHoldMutationKind::detection,
                                      {expected.first_version}));
                        same(store.find(detector.cause.hold_ref), std::optional{expected});
                        same(store.active(detector.cause.source.subject),
                             ApplicationHoldActiveSet{detector.cause.source.subject, {expected}});
                    }
                    marker_active(s);
                    require(db.saved() == before, "Independent reader sees only earlier committed "
                                                  "intake, no own commit/destructor cleanup");
                    require(
                        s.execute({"SELECT (SELECT count(*) FROM subathon_app_operations),(SELECT "
                                   "count(*) FROM subathon_app_effects),(SELECT count(*) FROM "
                                   "subathon_app_jobs),(SELECT count(*) FROM "
                                   "subathon_app_claims),hex(token) FROM subathon_app_head",
                                   {}}) ==
                            SqlRows{{qint64(0), qint64(0), qint64(0), qint64(0), QString("70")}},
                        "Hold never claims successful mixed input/time/job/head/result");
                    require(s.execute({"SELECT main_remaining_ms,time_debt_ms FROM "
                                       "subathon_app_states WHERE token=X'70'",
                                       {}}) == SqlRows{{qint64(55), qint64(0)}},
                            "Whole input preserves Main55/debt0");
                    if (commit)
                        s.commit();
                    else
                        s.rollback();
                    return 1;
                });
            };
            rolled_back(attempt(false));
            require(db.saved() == before,
                    "Caller rollback removes ALL speculative hold/versions/marker");
            committed(attempt(true));
            db.reopen();
            committed(read(db, [&](auto s) {
                same(SqliteApplicationHoldStore(s).find(detector.cause.hold_ref),
                     std::optional{expected});
            }));
            require(db.observer->rows("SELECT count(*) FROM subathon_hold_causes") ==
                            SqlRows{{qint64(1)}} &&
                        db.observer->rows("SELECT application FROM subathon_contributions ORDER BY "
                                          "contribution_id") == SqlRows{{qint64(0)}, {qint64(0)}},
                    "Exactly one durable hold, contribution dimensions remain pending");
        });
    }
    void exactMutationReplayReturnsHistoricalVersionsBeforeCurrentFreshness() {
        scenario([] {
            const Data d;
            Database db(d);
            const auto before = db.saved();
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                marker(s);
                same(store.record_detection(d.detection("dOld")),
                     mutation(MD::already_recorded, "dOld", ApplicationHoldMutationKind::detection,
                              {d.view("hOld").first_version}));
                same(store.record_detection(d.detection("dPause1")),
                     mutation(MD::already_recorded, "dPause1",
                              ApplicationHoldMutationKind::detection,
                              {d.view("hPause").first_version}));
                const auto r = d.resolution("rReplace");
                same(store.stage_resolution(r.request),
                     mutation(MD::already_recorded, "rReplace",
                              ApplicationHoldMutationKind::resolution, r.produced_versions));
                marker_active(s);
            }));
            db.modify("DELETE FROM subathon_retained_configurations WHERE "
                      "reference=X'71756572792D7072696F72'");
            require(db.saved() == before, "Exact retries made no own writes, no reactivation");
        });
    }
    void sharedMutationIdentityAndImmutablePayloadCannotBeReinterpreted_data() {
        QTest::addColumn<int>("which");
        const char *names[]{"changed-cause",
                            "changed-attempt-observation",
                            "changed-expected-version",
                            "changed-proposed-version",
                            "stored-resolution-new-detection",
                            "stored-detection-new-resolution"};
        for (int i = 0; i < 6; ++i)
            QTest::newRow(names[i]) << i;
    }
    void sharedMutationIdentityAndImmutablePayloadCannotBeReinterpreted() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            const auto before = db.saved();
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                marker(s);
                if (which == 5) {
                    auto batch = close_zero();
                    batch.mutation_ref = "dOld";
                    disposition(store.stage_resolution(batch), MD::mutation_conflict);
                } else {
                    auto x = d.detection("dOld");
                    if (which == 0)
                        x.cause.command.intent.document.append('x');
                    if (which == 1)
                        x.request.observation.actor_ref = "different";
                    if (which == 2)
                        x.request.expected_version_ref = "v2";
                    if (which == 3)
                        x.request.proposed_version_ref = "fresh";
                    if (which == 4)
                        x.request.mutation_ref = "rReplace";
                    disposition(store.record_detection(x), MD::mutation_conflict);
                }
                marker_active(s);
            }));
            db.modify("DELETE FROM subathon_retained_configurations WHERE "
                      "reference=X'71756572792D7072696F72'");
            require(db.saved() == before, "Healthy identity conflict stages no own changes");
        });
    }
    void detectionClosedCauseExpectedAndLifetimeVersionPriority_data() {
        QTest::addColumn<int>("which");
        const char *names[]{"active-cause-before-stale",      "active-stale-before-token",
                            "active-lifetime-token-conflict", "closed-before-cause-stale-token",
                            "missing-hold-with-expected",     "standalone-cannot-replace-active"};
        for (int i = 0; i < 6; ++i)
            QTest::newRow(names[i]) << i;
    }
    void detectionClosedCauseExpectedAndLifetimeVersionPriority() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            const auto before = db.saved();
            auto x = append(d, "dPause1", "fresh", std::optional{"v65"}, "v1");
            MD expected = MD::version_conflict;
            if (which == 0) {
                x.cause.command.intent.document.append('x');
                x.request.expected_version_ref = "stale";
                expected = MD::hold_conflict;
            }
            if (which == 1) {
                x.request.expected_version_ref = "stale";
                expected = MD::stale_version;
            }
            if (which == 3) {
                x = append(d, "dOld", "fresh", std::optional{"stale"}, "v1");
                x.cause.command.intent.document.append('x');
                expected = MD::closed_hold;
            }
            if (which == 4) {
                x.cause.hold_ref = "new";
                expected = MD::stale_version;
            }
            if (which == 5) {
                x.cause.hold_ref = "new";
                x.cause.supersedes_hold_ref = "hPause";
                x.request.expected_version_ref.reset();
                x.request.proposed_version_ref = "newv";
                expected = MD::hold_conflict;
            }
            committed(read(db, [&](auto s) {
                marker(s);
                disposition(SqliteApplicationHoldStore(s).record_detection(x), expected);
                marker_active(s);
            }));
            db.modify("DELETE FROM subathon_retained_configurations WHERE "
                      "reference=X'71756572792D7072696F72'");
            require(db.saved() == before, "Healthy refusal changes no own history");
        });
    }
    void appendedDetectionPreservesFirstCauseAndSeparatesCurrentFromReplay() {
        scenario([] {
            const Data d;
            Database db(d);
            auto x = append(d, "dPause1", "dPause66", std::optional{"v65"}, "v66");
            x.request.observation.actor_ref = "second-reviewer";
            auto expected = d.view("hPause");
            expected.current_version =
                produced("hPause", "v66", 66, "v65", ApplicationHoldState::active, "dPause66");
            expected.current_event = x.request;
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                same(store.record_detection(x),
                     mutation(MD::recorded, "dPause66", ApplicationHoldMutationKind::detection,
                              {expected.current_version}));
                same(store.find("hPause"), std::optional{expected});
                same(store.record_detection(d.detection("dPause1")),
                     mutation(MD::already_recorded, "dPause1",
                              ApplicationHoldMutationKind::detection, {expected.first_version}));
            }));
            db.reopen();
            committed(read(db, [&](auto s) {
                same(SqliteApplicationHoldStore(s).find("hPause"), std::optional{expected});
            }));
        });
    }
    void subsetResolutionLeavesIndependentBlockerAndHistoricalRetry() {
        scenario([] {
            const Data d;
            Database db(d);
            const auto batch = close_zero();
            const auto closed =
                produced("hOverflow", "v2", 2, "v1", ApplicationHoldState::resolved, "rClose");
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                same(store.stage_resolution(batch),
                     mutation(MD::recorded, "rClose", ApplicationHoldMutationKind::resolution,
                              {closed}));
                same(store.active(cid()), ApplicationHoldActiveSet{cid(), {d.view("hPause")}});
                auto view = d.view("hOverflow");
                view.current_version = closed;
                view.current_event = ApplicationHoldRecordedResolution{batch, {closed}};
                same(store.find("hOverflow"), std::optional{view});
                same(store.record_detection(d.detection("dOverflow")),
                     mutation(MD::already_recorded, "dOverflow",
                              ApplicationHoldMutationKind::detection, {view.first_version}));
            }));
            db.reopen();
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                same(store.stage_resolution(batch),
                     mutation(MD::already_recorded, "rClose",
                              ApplicationHoldMutationKind::resolution, {closed}));
                same(store.active(cid()), ApplicationHoldActiveSet{cid(), {d.view("hPause")}});
            }));
            require(
                db.observer->rows(
                    "SELECT application FROM subathon_contributions ORDER BY contribution_id") ==
                    SqlRows{{qint64(0)}, {qint64(0)}},
                "Resolution decision does not overwrite other contribution dimensions");
        });
    }
    void resolutionChecksCompleteSetThenEntryAndReplacementIdentity_data() {
        QTest::addColumn<int>("which");
        const char *names[]{"omitted-independent-active",
                            "changed-head",
                            "matching-set-stale-entry",
                            "lifetime-version",
                            "missing-entry",
                            "closed-entry",
                            "replacement-hold-collision",
                            "replacement-detection-collision",
                            "mutation-before-set"};
        for (int i = 0; i < 9; ++i)
            QTest::newRow(names[i]) << i;
    }
    void resolutionChecksCompleteSetThenEntryAndReplacementIdentity() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            auto b = close_zero();
            MD expected = MD::stale_active_set;
            if (which == 0)
                b.expected_active.pop_back();
            if (which == 1)
                b.expected_active[1].version_ref = "v64";
            if (which == 2) {
                b.entries[0].expected_version_ref = "stale";
                expected = MD::stale_version;
            }
            if (which == 3) {
                b.entries[0].proposed_version_ref = "v1";
                expected = MD::version_conflict;
            }
            if (which == 4) {
                b.entries[0].hold_ref = "missing";
                expected = MD::missing_hold;
            }
            if (which == 5) {
                b.subject = cid(maximum_id);
                b.expected_active = {{"hFail", "v1"}, {"hNew", "v1"}};
                b.entries = {{"hOld", "v1", "v1", {"decision", QByteArray("", 0)}, {}}};
                expected = MD::closed_hold;
            }
            if (which == 6 || which == 7) {
                auto r = append(d, "dPause1", which == 7 ? "dPause1" : "replacement", {}, "fresh");
                r.cause.hold_ref = which == 6 ? "hPause" : "hFresh";
                r.cause.supersedes_hold_ref = "hOverflow";
                b.entries[0].replacement = r;
                expected = MD::replacement_conflict;
            }
            if (which == 8) {
                b.mutation_ref = "rReplace";
                b.expected_active.clear();
                expected = MD::mutation_conflict;
            }
            const auto before = db.saved();
            committed(read(db, [&](auto s) {
                marker(s);
                disposition(SqliteApplicationHoldStore(s).stage_resolution(b), expected);
                marker_active(s);
            }));
            db.modify("DELETE FROM subathon_retained_configurations WHERE "
                      "reference=X'71756572792D7072696F72'");
            require(db.saved() == before,
                    "All normal refusals preserve durable history and caller unit");
        });
    }
    void atomicReplacementAndLaterReoverflowPreserveOldCauseWithoutGap() {
        scenario([] {
            const Data d;
            Database db(d);
            auto b = close_zero();
            b.mutation_ref = "rFresh";
            auto replacement = append(d, "dPause1", "dFresh", {}, "u1");
            replacement.cause.hold_ref = "hFresh";
            replacement.cause.supersedes_hold_ref = "hOverflow";
            b.entries[0].replacement = replacement;
            const auto closed =
                produced("hOverflow", "v2", 2, "v1", ApplicationHoldState::resolved, "rFresh",
                         ApplicationHoldReplacementLocator{"hFresh", "dFresh"});
            const auto created =
                produced("hFresh", "u1", 1, {}, ApplicationHoldState::active, "dFresh");
            ApplicationHoldView newview{
                replacement.cause, replacement.request, created, created, {}};
            const auto before = db.saved();
            const auto attempt = [&](bool commit) {
                return capture(*db.writer, [&](auto c) -> QVariant {
                    const auto s = c.begin();
                    SqliteApplicationHoldStore store(s);
                    same(store.stage_resolution(b),
                         mutation(MD::recorded, "rFresh", ApplicationHoldMutationKind::resolution,
                                  {closed, created}));
                    same(store.active(cid()),
                         ApplicationHoldActiveSet{cid(), {newview, d.view("hPause")}});
                    require(
                        db.saved() == before,
                        "Independent reader never observes a clear/apply gap or half replacement");
                    if (commit)
                        s.commit();
                    else
                        s.rollback();
                    return 1;
                });
            };
            rolled_back(attempt(false));
            require(db.saved() == before,
                    "Real rollback removes close/new cause/head/history together");
            committed(attempt(true));
            db.reopen();
            auto later = append(d, "dPause1", "dReoverflow", {}, "z1");
            later.cause.hold_ref = "hLater";
            later.cause.supersedes_hold_ref = "hOverflow";
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                same(store.stage_resolution(b),
                     mutation(MD::already_recorded, "rFresh",
                              ApplicationHoldMutationKind::resolution, {closed, created}));
                same(store.record_detection(later),
                     mutation(MD::recorded, "dReoverflow", ApplicationHoldMutationKind::detection,
                              {produced("hLater", "z1", 1, {}, ApplicationHoldState::active,
                                        "dReoverflow")}));
                const auto old = store.find("hOverflow");
                require(old.has_value(), "Old immutable cause remains");
                same(old->cause, d.cause("hOverflow"));
                same(old->current_version, closed);
                require(store.active(cid()).holds.size() == 3,
                        "New independent reoverflow does not erase existing causes");
            }));
        });
    }
    void timerApplicationLinksAreDataAndCannotReplayBesideOriginal_data() {
        QTest::addColumn<int>("which");
        QTest::newRow("kind0-old-Beside-alias") << 0;
        QTest::newRow("kind1-distinct-correction-data") << 1;
        QTest::newRow("kind1-known-signed-MIN-data") << 2;
        QTest::newRow("kind1-unknown-remaining-is-NULL") << 3;
    }
    void timerApplicationLinksAreDataAndCannotReplayBesideOriginal() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d, "timer");
            auto b = d.proposal(which == 0 ? "forbidden_old_beside_alias" : "timer_correction");
            if (which == 2)
                b.application->held_origin->remaining_units =
                    std::numeric_limits<std::int64_t>::min();
            if (which == 3)
                b.application->held_origin->remaining_units.reset();
            if (which == 0)
                known(refusal(
                          db,
                          [&](auto s) { (void)SqliteApplicationHoldStore(s).stage_resolution(b); },
                          SR::source_mismatch),
                      SR::source_mismatch);
            else {
                const auto original = db.observer->rows(
                    "SELECT hex(operation_scope),hex(operation_ref),hex(receipt_id),effect_count "
                    "FROM subathon_app_operations ORDER BY operation_ref");
                const auto expected =
                    produced("hTimer", "v2", 2, "v1", ApplicationHoldState::resolved, "rTimer");
                committed(read(db, [&](auto s) {
                    SqliteApplicationHoldStore store(s);
                    same(store.stage_resolution(b),
                         mutation(MD::recorded, "rTimer", ApplicationHoldMutationKind::resolution,
                                  {expected}));
                    same(store.active(cid()), ApplicationHoldActiveSet{cid(), {}});
                    auto view = d.view("hTimer");
                    view.current_version = expected;
                    view.current_event = ApplicationHoldRecordedResolution{b, {expected}};
                    same(store.find("hTimer"), std::optional{view});
                }));
                require(db.observer->rows(
                            "SELECT "
                            "hex(operation_scope),hex(operation_ref),hex(receipt_id),effect_count "
                            "FROM subathon_app_operations ORDER BY operation_ref") == original,
                        "Link did not replay or rewrite successful originals/effects");
                require(b.application->operation.operation_ref == "correction",
                        "Explicit remaining DATA is retained; no -3 versus -150 entitlement policy "
                        "asserted");
            }
        });
    }
    void incomingReferenceAbsenceAndMismatchActuallyRollback_data() {
        QTest::addColumn<int>("which");
        QTest::addColumn<int>("reason");
        const char *names[]{"missing-receipt",
                            "missing-plan",
                            "missing-state",
                            "missing-retained-config",
                            "wrong-retained-context",
                            "wrong-plan-source",
                            "captured-request-copy-mismatch",
                            "known-event-mismatch",
                            "failed-optional-prior-plan-mismatch",
                            "support-source-receipt-not-linked"};
        for (int i = 0; i < 10; ++i)
            QTest::newRow(names[i])
                << i << int(i < 4 ? SR::missing_reference : SR::source_mismatch);
    }
    void incomingReferenceAbsenceAndMismatchActuallyRollback() {
        QFETCH(int, which);
        QFETCH(int, reason);
        scenario([&] {
            const Data d;
            Database db(d, "pending", true);
            auto x = d.detection("dOverflow");
            if (which == 0)
                x.cause.source.receipt_id = {999};
            if (which == 1)
                x.cause.plan_evidence.plan->plan_ref = "missing";
            if (which == 2) {
                x.cause.first_observation.persisted_state_token = "missing";
                x.request.observation.persisted_state_token = "missing";
            }
            if (which == 3)
                x.cause.source.configuration_ref = "missing";
            if (which == 4)
                x.cause.source.arrival_context_ref = "other";
            if (which == 4) {
                db.modify("INSERT INTO subathon_retained_configurations VALUES(X'6F74686572',X'')");
                db.modify("INSERT INTO subathon_retained_contexts "
                          "VALUES(X'6F74686572',X'6F74686572',X'')");
            }
            if (which == 5)
                x.cause.plan_evidence.plan = ApplicationPlanKey{"PB", 1};
            if (which == 6)
                x.cause.plan_evidence.occurrences[0].explanation = "different";
            if (which == 7)
                x.cause.source.event_ref = "different";
            if (which == 8) {
                x = d.detection("dFail");
                x.cause.plan_evidence.plan = ApplicationPlanKey{"PA", 1};
            }
            if (which == 9)
                x.cause.source.receipt_id = {8};
            known(refusal(
                      db, [&](auto s) { (void)SqliteApplicationHoldStore(s).record_detection(x); },
                      SR(reason)),
                  SR(reason));
        });
    }
    void failedCaptureAbsenceAndHistoricalStateAreNotCurrentHeadPolicy() {
        scenario([] {
            const Data d;
            Database db(d, "pending", true);
            // An unrelated new current head is healthy. A known historical p
            // still resolves fully; absent FailedCapture observations stay absent.
            db.modify("INSERT INTO subathon_app_states SELECT "
                      "X'6E6577',format_ref,document,configuration_ref,lifecycle,phase,main_"
                      "remaining_ms,grace_remaining_ms,awaiting_confirmation,time_debt_ms,balance_"
                      "count FROM subathon_app_states WHERE token=X'70'");
            db.modify(
                "INSERT INTO subathon_app_balances SELECT X'6E6577',balance_kind,target_ref,units "
                "FROM subathon_app_balances WHERE token=X'70'");
            db.modify("UPDATE subathon_app_head SET token=X'6E6577'");
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                for (const auto *name : {"dOverflow", "dFail"}) {
                    const auto x = d.detection(name);
                    same(store.record_detection(x),
                         mutation(MD::recorded, name, ApplicationHoldMutationKind::detection,
                                  {d.view(x.cause.hold_ref.c_str()).first_version}));
                    same(store.find(x.cause.hold_ref),
                         std::optional{d.view(x.cause.hold_ref.c_str())});
                }
                const auto failed = store.find("hFail");
                require(
                    failed && !failed->cause.first_observation.persisted_state_token &&
                        !failed->cause.plan_evidence.plan &&
                        !failed->cause.plan_evidence.occurrences[1].finalized_units &&
                        failed->cause.plan_evidence.occurrences[0].finalized_units == 0,
                    "Unknown failed amount/token/plan not fabricated; known zero remains known");
            }));
            require(db.observer->rows("SELECT hex(token) FROM subathon_app_head") ==
                        SqlRows{{QString("6E6577")}},
                    "Read of historical/absent token does not adopt/rewrite current head");
        });
    }
    void selectedCorruptionCannotBecomeAbsenceConflictOrPartialSuccess_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<int>("scope");
        const auto row = [](const char *name, const char *sql, int scope = 0) {
            QTest::newRow(name) << QString(sql) << scope;
        };
        row("missing-first-observation",
            "DELETE FROM subathon_hold_cause_observations WHERE hold_ref=X'684F766572666C6F77'");
        row("missing-current-detection",
            "DELETE FROM subathon_hold_detections WHERE mutation_ref=X'644F766572666C6F77'");
        row("wrong-subject-projection",
            "UPDATE subathon_hold_causes SET contribution_id=X'ffffffffffffffff' WHERE "
            "hold_ref=X'684F766572666C6F77'");
        row("selected-receipt-wrong-width",
            "UPDATE subathon_hold_causes SET source_receipt_id=X'07' WHERE "
            "hold_ref=X'684F766572666C6F77'");
        row("selected-source-event-mismatch",
            "UPDATE subathon_hold_causes SET event_ref=X'78' WHERE hold_ref=X'684F766572666C6F77'");
        row("required-command-document-TEXT",
            "UPDATE subathon_hold_causes SET command_document='text' WHERE "
            "hold_ref=X'684F766572666C6F77'");
        row("wrong-cause-kind",
            "UPDATE subathon_hold_causes SET cause_kind=9 WHERE hold_ref=X'684F766572666C6F77'");
        row("partial-failure-group", "UPDATE subathon_hold_causes SET failure_denominator=NULL "
                                     "WHERE hold_ref=X'684F766572666C6F77'");
        row("captured-missing-token",
            "UPDATE subathon_hold_cause_observations SET persisted_state_token=NULL WHERE "
            "hold_ref=X'684F766572666C6F77'");
        row("negative-known-elapsed", "UPDATE subathon_hold_detection_observations SET "
                                      "elapsed_ms=-1 WHERE mutation_ref=X'644F766572666C6F77'");
        row("understated-occurrences-before-reserve",
            "UPDATE subathon_hold_causes SET occurrence_count=0 WHERE "
            "hold_ref=X'684F766572666C6F77'");
        row("extra-ordinal-zero-not-positive-keyset",
            "INSERT INTO subathon_hold_occurrences(hold_ref,ordinal) "
            "VALUES(X'684F766572666C6F77',0)");
        row("missing-middle-occurrence", "DELETE FROM subathon_hold_occurrences WHERE "
                                         "hold_ref=X'684F766572666C6F77' AND ordinal=1");
        row("noncanonical-negative-zero",
            "UPDATE subathon_hold_occurrences SET attempted_numerator=X'2D30' WHERE "
            "hold_ref=X'684F766572666C6F77' AND ordinal=1");
        row("nonreduced-exact", "UPDATE subathon_hold_occurrences SET "
                                "attempted_numerator=X'32',attempted_denominator=X'32' WHERE "
                                "hold_ref=X'684F766572666C6F77' AND ordinal=1");
        row("failed-final-amount-fabricated-zero",
            "UPDATE subathon_hold_occurrences SET finalized_units=0 WHERE hold_ref=X'684661696C' "
            "AND ordinal=2",
            3);
        row("missing-selected-historical-state",
            "DELETE FROM subathon_app_states WHERE token=X'70'");
        row("historical-state-hidden-illegal-balance-kind",
            "INSERT INTO subathon_app_balances VALUES(X'70',1,X'78',0)");
        row("captured-plan-request-corrupt", "UPDATE subathon_plan_requests SET requested_units=6 "
                                             "WHERE plan_ref=X'504F' AND ordinal=1");
        row("plan-nonpositive-revision-metadata",
            "UPDATE subathon_plan_revisions SET revision=X'0000000000000000' WHERE "
            "plan_ref=X'504F'");
        row("retained-source-document-TEXT",
            "UPDATE subathon_retained_configurations SET document='bad' WHERE reference=X'636667'");
        row("shared-registry-wrong-kind-before-new-kind-conflict",
            "UPDATE subathon_hold_mutations SET mutation_kind=1 WHERE "
            "mutation_ref=X'644F766572666C6F77'",
            4);
        row("understated-selected-resolution-entries",
            "UPDATE subathon_hold_mutations SET entry_count=0 WHERE "
            "mutation_ref=X'725265706C616365'",
            5);
        row("missing-resolution-produced-version",
            "DELETE FROM subathon_hold_versions WHERE hold_ref=X'684F6C64' AND version_ref=X'7632'",
            5);
        row("replacement-backlink-disagrees",
            "UPDATE subathon_hold_causes SET supersedes_hold_ref=X'684661696C' WHERE "
            "hold_ref=X'684E6577'",
            5);
        row("supersedes-cycle",
            "UPDATE subathon_hold_causes SET supersedes_hold_ref=X'684E6577' WHERE "
            "hold_ref=X'684F6C64'",
            3);
        row("wrong-full-linked-result",
            "DELETE FROM subathon_app_results WHERE operation_scope=X'74696D6572' AND "
            "operation_ref=X'6F726967696E616C'",
            6);
        row("old-linked-component-not-narrow-basis",
            "DELETE FROM subathon_app_components WHERE operation_scope=X'74696D6572' AND "
            "operation_ref=X'6F726967696E616C' AND request_ordinal=2",
            6);
    }
    void selectedCorruptionCannotBecomeAbsenceConflictOrPartialSuccess() {
        QFETCH(QString, sql);
        QFETCH(int, scope);
        scenario([&] {
            const Data d;
            Database db(d, scope == 6 ? "timer" : "pending");
            db.modify("PRAGMA foreign_keys=OFF");
            db.modify("PRAGMA ignore_check_constraints=ON");
            db.modify(sql);
            require(db.observer->rows("SELECT changes()").front().front().toInt() > 0,
                    "Literal corruption really changed existing selected rows");
            db.modify("PRAGMA ignore_check_constraints=OFF");
            known(refusal(
                      db,
                      [&](auto s) {
                          SqliteApplicationHoldStore store(s);
                          if (scope == 3)
                              (void)store.active(cid(maximum_id));
                          else if (scope == 4) {
                              auto changed = d.detection("dOverflow");
                              changed.request.audit.document.append('x');
                              (void)store.record_detection(changed);
                          } else if (scope == 5)
                              (void)store.find("hOld");
                          else if (scope == 6)
                              (void)store.find("hTimer");
                          else
                              (void)store.find("hOverflow");
                      },
                      SR::invalid_stored_data),
                  SR::invalid_stored_data);
        });
    }
    void activeAuditsAllAnchorsInactiveHeadsAndSixtyFifthMetadata_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<bool>("maximum");
        const auto row = [](const char *n, const char *s, bool max = false) {
            QTest::newRow(n) << QString(s) << max;
        };
        row("missing-active-head",
            "DELETE FROM subathon_hold_heads WHERE hold_ref=X'685061757365'");
        row("empty-selected-anchor-key",
            "UPDATE subathon_hold_causes SET hold_ref=X'' WHERE hold_ref=X'685061757365'");
        row("TEXT-selected-anchor-key",
            "UPDATE subathon_hold_causes SET hold_ref='hPause' WHERE hold_ref=X'685061757365'");
        row("missing-inactive-head-before-status-filter",
            "DELETE FROM subathon_hold_heads WHERE hold_ref=X'684F6C64'", true);
        row("missing-inactive-terminal-version",
            "DELETE FROM subathon_hold_versions WHERE hold_ref=X'684F6C64' AND version_ref=X'7632'",
            true);
        row("bad-inactive-state",
            "UPDATE subathon_hold_versions SET state=9 WHERE hold_ref=X'684F6C64' AND "
            "version_ref=X'7632'",
            true);
        row("sixty-fifth-parent-not-immediate",
            "UPDATE subathon_hold_versions SET parent_version_ref=X'7631' WHERE "
            "hold_ref=X'685061757365' AND version_ref=X'763635'");
        row("sixty-fifth-actual-producer-subject",
            "UPDATE subathon_hold_mutations SET subject_id=X'ffffffffffffffff' WHERE "
            "mutation_ref=X'6450617573653635'");
        row("extra-position-zero-not-positive-keyset",
            "INSERT INTO subathon_hold_versions "
            "VALUES(X'685061757365',X'7A65726F',0,NULL,0,X'64506175736531',NULL,NULL)");
        row("head-count-not-history-count",
            "UPDATE subathon_hold_heads SET version_count=64 WHERE hold_ref=X'685061757365'");
        row("sparse-MAX-is-corruption-not-exhaustion",
            "UPDATE subathon_hold_versions SET position=9223372036854775807 WHERE "
            "hold_ref=X'685061757365' AND version_ref=X'763635'");
        row("terminal-no-version-after-resolved",
            "UPDATE subathon_hold_versions SET state=1 WHERE hold_ref=X'685061757365' AND "
            "version_ref=X'763634'");
    }
    void activeAuditsAllAnchorsInactiveHeadsAndSixtyFifthMetadata() {
        QFETCH(QString, sql);
        QFETCH(bool, maximum);
        scenario([&] {
            const Data d;
            Database db(d);
            db.modify("PRAGMA foreign_keys=OFF");
            db.modify("PRAGMA ignore_check_constraints=ON");
            db.modify(sql);
            require(db.observer->rows("SELECT changes()") == SqlRows{{qint64(1)}},
                    "Exactly one actual metadata corruption");
            db.modify("PRAGMA ignore_check_constraints=OFF");
            SqliteHoldLimits bounds;
            bounds.active_holds_per_result = 1;
            known(refusal(
                      db,
                      [&](auto s) {
                          (void)SqliteApplicationHoldStore(s, bounds).active(
                              maximum ? cid(maximum_id) : cid());
                      },
                      SR::invalid_stored_data),
                  SR::invalid_stored_data);
        });
    }
    void narrowActiveProjectionDoesNotCertifyUnselectedDocumentsOrOtherSubjects() {
        scenario([] {
            const Data d;
            Database db(d);
            db.modify("PRAGMA ignore_check_constraints=ON");
            db.modify("UPDATE subathon_hold_detections SET audit_document='bad' WHERE "
                      "mutation_ref=X'644F6C64'");
            db.modify("PRAGMA ignore_check_constraints=OFF");
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s);
                same(store.active(cid(maximum_id)),
                     ApplicationHoldActiveSet{cid(maximum_id), {d.view("hFail"), d.view("hNew")}});
                same(store.active(cid()),
                     ApplicationHoldActiveSet{cid(), {d.view("hOverflow"), d.view("hPause")}});
            }));
            known(refusal(
                      db,
                      [&](auto s) {
                          (void)SqliteApplicationHoldStore(s).history({"hOld", 0, {}, 1});
                      },
                      SR::invalid_stored_data),
                  SR::invalid_stored_data);
            db.modify("DELETE FROM subathon_hold_heads WHERE hold_ref=X'684F6C64'");
            committed(read(db, [&](auto s) {
                same(SqliteApplicationHoldStore(s).active(cid()),
                     ApplicationHoldActiveSet{cid(), {d.view("hOverflow"), d.view("hPause")}});
            }));
            known(refusal(
                      db,
                      [&](auto s) { (void)SqliteApplicationHoldStore(s).active(cid(maximum_id)); },
                      SR::invalid_stored_data),
                  SR::invalid_stored_data);
        });
    }
    void activeAnchorLookaheadIsCompleteBeforeAnyPayloadOrLimit_data() {
        QTest::addColumn<int>("which");
        QTest::newRow("all65-active-views-owned") << 0;
        QTest::newRow("65-under-ceiling64-never-truncated") << 1;
        QTest::newRow("65th-anchor-metadata-before-result-overage") << 2;
        QTest::newRow("257-under-default256-never-clear") << 3;
    }
    void activeAnchorLookaheadIsCompleteBeforeAnyPayloadOrLimit() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            const int added = which == 3 ? 255 : 63;
            const auto steps =
                d.frozen["arrangements"].toObject()["pending"].toObject()["setup"].toArray();
            require(steps.size() == 402 &&
                        original_claim_fixture::statement(steps[38]).bindings[0].toByteArray() ==
                            QByteArray("dPause1") &&
                        original_claim_fixture::statement(steps[39]).bindings[0].toByteArray() ==
                            QByteArray("hPause"),
                    "Literal seven-row initial policy-hold slice, not current65 history");
            ApplicationHoldActiveSet expected{cid(), {}};
            (void)application_hold_fixture::run(*db.writer, [&](auto c) -> QVariant {
                const auto s = c.begin();
                for (int n = 1; n <= added; ++n) {
                    const auto hold = QString("a%1").arg(n, 3, 10, QChar('0')).toStdString();
                    const auto mut = QString("dAnchor%1").arg(n, 3, 10, QChar('0')).toStdString();
                    // Clone independently frozen INITIAL rows with explicit new
                    // identities. No hold API generates data or expected values.
                    for (int i = 38; i <= 44; ++i) {
                        auto q = original_claim_fixture::statement(steps[i]);
                        if (i == 38 || i == 41 || i == 42)
                            q.bindings[0] = blob(mut);
                        else
                            q.bindings[0] = blob(hold);
                        if (i == 39)
                            q.bindings[16] = blob(mut);
                        if (i == 41)
                            q.bindings[1] = blob(hold);
                        if (i == 43)
                            q.bindings[5] = blob(mut);
                        (void)s.execute(q);
                    }
                    auto cause = d.cause("hPause");
                    cause.hold_ref = hold;
                    auto first = d.detection_data("dPause1");
                    first.mutation_ref = mut;
                    const auto v = produced(hold, "v1", 1, {}, ApplicationHoldState::active, mut);
                    expected.holds.push_back({cause, first, v, v, {}});
                }
                s.commit();
                return 1;
            });
            require(db.observer->rows("PRAGMA foreign_key_check").empty() &&
                        db.observer->rows("SELECT count(*) FROM subathon_hold_causes WHERE "
                                          "subject_kind=0 AND subject_id=X'0000000000000000'") ==
                            SqlRows{{qint64(added + 2)}},
                    "Actual coherent complete selected-anchor count before any new query");
            expected.holds.push_back(d.view("hOverflow"));
            expected.holds.push_back(d.view("hPause"));
            SqliteHoldLimits l;
            l.input_items = 1;
            l.references.input_items = 1;
            if (which == 0) {
                l.active_holds_per_result = 65;
                l.result_items = 67;
                committed(read(db, [&](auto s) {
                    same(SqliteApplicationHoldStore(s, l).active(cid()), expected);
                }));
            } else {
                if (which != 3)
                    l.active_holds_per_result = 64;
                if (which == 2)
                    db.modify("DELETE FROM subathon_hold_heads WHERE hold_ref=X'685061757365'");
                const auto reason = which == 2 ? SR::invalid_stored_data : SR::result_limit;
                known(refusal(
                          db, [&](auto s) { (void)SqliteApplicationHoldStore(s, l).active(cid()); },
                          reason),
                      reason);
            }
        });
    }
    void metadataHasNoLifetimeMaterializationQuotaAndFullPagesOwnOnlySelectedBodies() {
        scenario([] {
            const Data d;
            Database db(d);
            SqliteHoldLimits l;
            l.history_versions_per_page = 1;
            l.resolution_entries = 1;
            l.references.input_items = 1;
            l.references.plan_items_per_vector = 2;
            l.input_items = 1;
            l.result_items = 1;
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s, l);
                same(store.find("hPause"), std::optional{d.view("hPause")});
                const auto page = store.history({"hPause", 64, std::string("v65"), 1});
                require(page.disposition == HD::ready && page.page &&
                            page.page->entries.size() == 1 && !page.page->has_more,
                        "Metadata audited65; selected full page1, no invented lifetime quota");
                same(page.page->entries[0].event,
                     ApplicationHoldHistoryEvent{d.detection("dPause65")});
            }));
            // v3's OLD document is not current/first or selected by the tail page.
            db.modify("PRAGMA ignore_check_constraints=ON");
            db.modify("UPDATE subathon_hold_detections SET audit_document='bad' WHERE "
                      "mutation_ref=X'64506175736533'");
            db.modify("PRAGMA ignore_check_constraints=OFF");
            committed(read(db, [&](auto s) {
                SqliteApplicationHoldStore store(s, l);
                same(store.find("hPause"), std::optional{d.view("hPause")});
                require(store.history({"hPause", 64, {}, 1}).disposition == HD::ready,
                        "Unselected old document not spuriously certified/loaded");
            }));
            known(refusal(
                      db,
                      [&](auto s) {
                          (void)SqliteApplicationHoldStore(s, l).history({"hPause", 2, {}, 1});
                      },
                      SR::invalid_stored_data),
                  SR::invalid_stored_data);
        });
    }
    void allComposedOptionsPrecedeInputsAndStaleTickets_data() {
        QTest::addColumn<int>("which");
        QTest::addColumn<bool>("extreme");
        const char *names[]{"reference-bytes",
                            "value-bytes",
                            "exact-component-bytes",
                            "old-balances",
                            "old-plan-items",
                            "old-effects",
                            "old-jobs",
                            "old-transitions",
                            "old-links",
                            "dormant-old-input-items",
                            "old-result-items",
                            "old-owned-bytes",
                            "occurrences",
                            "active-views-and-locators",
                            "entries-and-produced-double",
                            "history-entries",
                            "hold-input-items",
                            "hold-result-items",
                            "hold-owned-bytes"};
        for (int i = 0; i < 19; ++i) {
            QTest::newRow(qPrintable(QString(names[i]) + "-zero")) << i << false;
            if (i != 0 && i != 1 && i != 2 && i != 11 && i != 18)
                QTest::newRow(qPrintable(QString(names[i]) + "-MAX")) << i << true;
        }
        for (int i = 19; i < 24; ++i)
            QTest::newRow(qPrintable(QString("actual-vector-max-size-%1").arg(i))) << i << true;
    }
    void allComposedOptionsPrecedeInputsAndStaleTickets() {
        QFETCH(int, which);
        QFETCH(bool, extreme);
        scenario([&] {
            SqliteHoldLimits l;
            std::array<qsizetype *, 19> fields{&l.references.reference_bytes,
                                               &l.references.value_bytes,
                                               &l.references.component_bytes,
                                               &l.references.balances_per_state,
                                               &l.references.plan_items_per_vector,
                                               &l.references.effects_per_bundle,
                                               &l.references.jobs_per_bundle,
                                               &l.references.transitions_per_bundle,
                                               &l.references.links_per_bundle,
                                               &l.references.input_items,
                                               &l.references.result_items,
                                               &l.references.owned_bytes,
                                               &l.known_occurrences_per_cause,
                                               &l.active_holds_per_result,
                                               &l.resolution_entries,
                                               &l.history_versions_per_page,
                                               &l.input_items,
                                               &l.result_items,
                                               &l.owned_bytes};
            if (which < 19)
                *fields[size_t(which)] = extreme ? std::numeric_limits<qsizetype>::max() : 0;
            // n+1 must fit the ACTUAL owning container, including both active
            // representations and 2*entries+1 produced versions, not a guessed DTO.
            if (which == 19)
                l.known_occurrences_per_cause =
                    qsizetype(std::vector<ApplicationHoldProposedOccurrence>{}.max_size());
            if (which == 20)
                l.active_holds_per_result =
                    qsizetype(std::vector<ApplicationHoldView>{}.max_size());
            if (which == 21)
                l.resolution_entries =
                    qsizetype(std::vector<ApplicationHoldResolutionEntry>{}.max_size());
            if (which == 22)
                l.history_versions_per_page =
                    qsizetype(std::vector<ApplicationHoldHistoryEntry>{}.max_size());
            if (which == 23)
                l.resolution_entries =
                    qsizetype(std::vector<ApplicationHoldVersion>{}.max_size() / 2 + 1);
            const SqliteApplicationHoldStore inert({}, l);
            const auto copy = inert;
            input_failure([&] { (void)copy.find(""); }, IE::invalid_options);
        });
    }
    void pureInputShapeCountsMatrixExactAndOrderingPreserveCallerStaging_data() {
        QTest::addColumn<int>("which");
        QTest::addColumn<int>("error");
        const std::array<std::pair<const char *, IE>, 34> rows{
            {{"empty-hold-key", IE::invalid_key},
             {"zero-receipt-subject", IE::invalid_subject},
             {"zero-source-receipt", IE::invalid_subject},
             {"empty-known-plan-key", IE::invalid_key},
             {"empty-known-state-key", IE::invalid_key},
             {"counts-before-null-body", IE::collection_limit},
             {"null-command-document", IE::invalid_cause},
             {"null-attempt-audit", IE::invalid_detection},
             {"negative-observed-elapsed", IE::invalid_observation},
             {"unknown-cause-enum", IE::invalid_cause},
             {"missing-numeric-failure", IE::invalid_failure},
             {"captured-without-plan", IE::invalid_plan_evidence},
             {"captured-known-prefix", IE::invalid_plan_evidence},
             {"failed-without-evidence", IE::invalid_plan_evidence},
             {"failed-wrong-stopped-reason", IE::invalid_plan_evidence},
             {"failed-final-amount-invented-zero", IE::invalid_failure},
             {"aggregate-wrong-failure-stage", IE::invalid_failure},
             {"failing-ordinal-outside-known-list", IE::invalid_failure},
             {"timer-without-explicit-composition", IE::invalid_cause},
             {"timer-unfinalized-plan", IE::invalid_plan_evidence},
             {"timer-not-negative", IE::invalid_cause},
             {"policy-missing-reason", IE::invalid_cause},
             {"unknown-occurrence-unit", IE::invalid_plan_evidence},
             {"noncontiguous-occurrences", IE::invalid_plan_evidence},
             {"both-exact-component-bounds", IE::exact_component_limit},
             {"resolution-empty-entries", IE::invalid_resolution},
             {"duplicate-expected-hold", IE::invalid_resolution},
             {"unsigned-expected-order", IE::invalid_resolution},
             {"duplicate-entry", IE::invalid_resolution},
             {"replacement-count-before-child-body", IE::collection_limit},
             {"negative-history-cursor", IE::invalid_cursor},
             {"page-count-over-ceiling", IE::collection_limit},
             {"keys-before-counts", IE::invalid_key},
             {"input-items-before-owned-bytes", IE::collection_limit}}};
        for (int i = 0; i < int(rows.size()); ++i)
            QTest::newRow(rows[size_t(i)].first) << i << int(rows[size_t(i)].second);
    }
    void pureInputShapeCountsMatrixExactAndOrderingPreserveCallerStaging() {
        QFETCH(int, which);
        QFETCH(int, error);
        scenario([&] {
            const Data d;
            Database db(d);
            auto x = d.detection("dOverflow");
            auto b = close_zero();
            SqliteHoldLimits l;
            if (which == 0)
                x.cause.hold_ref.clear();
            if (which == 1)
                x.cause.source.subject = receipt(0);
            if (which == 2)
                x.cause.source.receipt_id = {0};
            if (which == 3)
                x.cause.plan_evidence.plan->plan_ref.clear();
            if (which == 4)
                x.request.observation.persisted_state_token = "";
            if (which == 5) {
                l.known_occurrences_per_cause = 1;
                x.request.audit.document = {};
            }
            if (which == 6)
                x.cause.command.intent.document = {};
            if (which == 7)
                x.request.audit.document = {};
            if (which == 8)
                x.request.observation.elapsed_ms = -1;
            if (which == 9)
                x.cause.kind = ApplicationHoldCauseKind(9);
            if (which == 10)
                x.cause.failure.reset();
            if (which == 11)
                x.cause.plan_evidence.plan.reset();
            if (which == 12)
                x.cause.plan_evidence.extent = ApplicationHoldExtent::known_prefix;
            if (which == 13) {
                x = d.detection("dFail");
                x.cause.plan_evidence.failed_capture.reset();
            }
            if (which == 14) {
                x = d.detection("dFail");
                x.cause.plan_evidence.stopped_for =
                    ApplicationHoldStoppedFor::unresolved_capture_policy;
            }
            if (which == 15) {
                x = d.detection("dFail");
                x.cause.plan_evidence.occurrences[1].finalized_units = 0;
            }
            if (which == 16)
                x.cause.failure->stage = ApplicationHoldFailureStage::final_conversion;
            if (which == 17)
                x.cause.failure->failing_ordinal = 3;
            if (which == 18) {
                x = d.detection("dTimer");
                x.cause.timer->composition.reset();
            }
            if (which == 19) {
                x = d.detection("dTimer");
                x.cause.plan_evidence = d.cause("hFail").plan_evidence;
            }
            if (which == 20) {
                x = d.detection("dTimer");
                x.cause.timer->request_units = 0;
            }
            if (which == 21) {
                x = d.detection("dPause1");
                x.cause.missing_policy_reason.reset();
            }
            if (which == 22)
                x.cause.plan_evidence.occurrences[0].unit = core::RequestedUnit(9);
            if (which == 23) {
                x.cause.plan_evidence.occurrences[1].ordinal = 3;
                x.cause.failure->failing_ordinal = 3;
            }
            if (which == 24) {
                l.references.component_bytes = 2;
                x.cause.plan_evidence.occurrences[0].attempted_units =
                    core::Exact(100) / core::Exact(101);
            }
            if (which == 25)
                b.entries.clear();
            if (which == 26)
                b.expected_active[1] = b.expected_active[0];
            if (which == 27) {
                b.expected_active[0].hold_ref = std::string(1, char(0xFF));
                b.expected_active[1].hold_ref = std::string(1, char(0x7F));
                b.entries[0].hold_ref = b.expected_active[0].hold_ref;
            }
            if (which == 28)
                b.entries.push_back(b.entries[0]);
            if (which == 29) {
                auto replacement = d.detection("dFail");
                replacement.cause.hold_ref = "hFresh";
                replacement.cause.supersedes_hold_ref = "hOverflow";
                replacement.cause.command.intent.document = {};
                b.entries[0].replacement = replacement;
                l.known_occurrences_per_cause = 1;
            }
            if (which == 32) {
                x.cause.hold_ref.clear();
                l.input_items = 1;
            }
            if (which == 33) {
                l.input_items = 1;
                l.owned_bytes = 1;
            }
            committed(read(db, [&](auto s) {
                marker(s);
                SqliteApplicationHoldStore store(s, l);
                input_failure(
                    [&] {
                        if (which >= 25 && which <= 29)
                            (void)store.stage_resolution(b);
                        else if (which == 30)
                            (void)store.history({"hPause", -1, {}, 1});
                        else if (which == 31)
                            (void)store.history({"hPause", 0, {}, 65});
                        else
                            (void)store.record_detection(x);
                    },
                    IE(error));
                marker_active(s);
            }));
            require(db.observer->rows("SELECT count(*) FROM subathon_retained_configurations WHERE "
                                      "reference=X'71756572792D7072696F72'") ==
                        SqlRows{{qint64(1)}},
                    "Pure failure performed no rollback/cleanup and caller could commit earlier "
                    "staging");
        });
    }
    void inputOwnedBytesUseFrozenCompleteRequestAccounting_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("bytes");
        QTest::addColumn<bool>("under");
        const std::array<std::pair<const char *, int>, 7> rows{{{"dOverflow", 309},
                                                                {"dFail", 211},
                                                                {"dPause1", 165},
                                                                {"dPause65", 170},
                                                                {"dNew", 169},
                                                                {"dTimer", 327},
                                                                {"rReplace", 244}}};
        for (const auto &[name, n] : rows)
            for (bool under : {false, true})
                QTest::newRow(qPrintable(QString(name) + (under ? "-one-byte-over" : "-exact-fit")))
                    << QString(name) << n << under;
    }
    void everyPureEntryPreservesTheAlreadyExhaustedSqlBudget_data() {
        QTest::addColumn<int>("which");
        for (int i = 0; i < 5; ++i)
            QTest::newRow(i == 0   ? "find"
                          : i == 1 ? "active"
                          : i == 2 ? "history"
                          : i == 3 ? "record-detection"
                                   : "stage-resolution")
                << i;
    }
    void everyPureEntryPreservesTheAlreadyExhaustedSqlBudget() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            auto l = limits();
            l.statements_per_batch = 1;
            db.reopen(l);
            committed(read(db, [&](auto s) {
                marker(s);
                SqliteApplicationHoldStore store(s);
                if (which == 0)
                    input_failure([&] { (void)store.find(""); }, IE::invalid_key);
                if (which == 1)
                    input_failure([&] { (void)store.active(receipt(0)); }, IE::invalid_subject);
                if (which == 2)
                    input_failure([&] { (void)store.history({"hPause", -1, {}, 1}); },
                                  IE::invalid_cursor);
                if (which == 3) {
                    auto x = d.detection("dOverflow");
                    x.request.audit.document = {};
                    input_failure([&] { (void)store.record_detection(x); }, IE::invalid_detection);
                }
                if (which == 4) {
                    auto b = close_zero();
                    b.entries.clear();
                    input_failure([&] { (void)store.stage_resolution(b); }, IE::invalid_resolution);
                }
                // COMMIT control has no SQL statement charge. Any SQL in pure
                // preflight would have aborted this real caller unit instead.
            }));
            require(db.observer->rows("SELECT count(*) FROM subathon_retained_configurations WHERE "
                                      "reference=X'71756572792D7072696F72'") ==
                        SqlRows{{qint64(1)}},
                    "All five pure entries avoided SQL/cleanup after the caller spent its ONE "
                    "statement");
        });
    }
    void inputOwnedBytesUseFrozenCompleteRequestAccounting() {
        QFETCH(QString, name);
        QFETCH(int, bytes);
        QFETCH(bool, under);
        scenario([&] {
            const Data d;
            Database db(d, name == "dTimer" ? "timer" : "pending");
            SqliteHoldLimits l;
            l.owned_bytes = bytes - int(under);
            const auto is_resolution = name == "rReplace";
            const auto detector =
                is_resolution ? ApplicationHoldDetection{} : d.detection(qPrintable(name));
            const auto resolution =
                is_resolution ? d.resolution("rReplace") : ApplicationHoldRecordedResolution{};
            require(d.oracle(qPrintable(name + (is_resolution ? ".batch" : ".detection")))["bytes"]
                            .toInt() == bytes,
                    "Literal independently reviewed B(input) pin");
            committed(read(db, [&](auto s) {
                marker(s);
                SqliteApplicationHoldStore store(s, l);
                const auto call = [&] {
                    return is_resolution ? store.stage_resolution(resolution.request)
                                         : store.record_detection(detector);
                };
                if (under)
                    input_failure([&] { (void)call(); }, IE::owned_byte_limit);
                else
                    same(call(),
                         mutation(
                             MD::already_recorded, name.toStdString(),
                             is_resolution ? ApplicationHoldMutationKind::resolution
                                           : ApplicationHoldMutationKind::detection,
                             is_resolution
                                 ? resolution.produced_versions
                                 : std::vector{d.view(detector.cause.hold_ref.c_str())
                                                               .first_version.locator.hold_ref ==
                                                           "hPause" &&
                                                       name == "dPause65"
                                                   ? d.view("hPause").current_version
                                                   : d.view(detector.cause.hold_ref.c_str())
                                                         .first_version}));
                marker_active(s);
            }));
        });
    }
    void returnedOwnedBytesAndNestedItemsMatchIndependentFrozenBoundaries_data() {
        QTest::addColumn<int>("which");
        QTest::addColumn<int>("bytes");
        QTest::addColumn<int>("items");
        QTest::addColumn<int>("mode");
        const std::array<std::tuple<const char *, int, int>, 12> rows{
            {{"hOverflow", 367, 2},
             {"hFail", 253, 2},
             {"hReceipt", 286, 1},
             {"hOld", 490, 5},
             {"hNew", 207, 0},
             {"hPause", 264, 0},
             {"hTimer", 373, 2},
             {"CID0", 640, 4},
             {"CIDMAX", 469, 4},
             {"history-first2", 403, 2},
             {"history-resolution", 338, 6},
             {"empty", 9, 0}}};
        for (int i = 0; i < int(rows.size()); ++i) {
            const auto &[name, b, n] = rows[size_t(i)];
            for (int mode = 0; mode < (n > 1 ? 3 : 2); ++mode)
                QTest::newRow(qPrintable(QString(name) + (mode == 0   ? "-exact-fit"
                                                          : mode == 1 ? "-byte-over"
                                                                      : "-nested-item-over")))
                    << i << b << n << mode;
        }
    }
    void returnedOwnedBytesAndNestedItemsMatchIndependentFrozenBoundaries() {
        QFETCH(int, which);
        QFETCH(int, bytes);
        QFETCH(int, items);
        QFETCH(int, mode);
        scenario([&] {
            const Data d;
            Database db(d, which == 6 ? "timer" : "pending");
            SqliteHoldLimits l;
            l.references.input_items = 1;
            l.input_items = 1;
            l.owned_bytes = bytes - (mode == 1);
            if (items)
                l.result_items = items - (mode == 2);
            const auto call = [&](auto s) {
                SqliteApplicationHoldStore store(s, l);
                const char *holds[]{"hOverflow", "hFail",  "hReceipt", "hOld",
                                    "hNew",      "hPause", "hTimer"};
                if (which < 7)
                    same(store.find(holds[which]), std::optional{d.view(holds[which])});
                else if (which < 9)
                    same(store.active(which == 7 ? cid() : cid(maximum_id)),
                         which == 7
                             ? ApplicationHoldActiveSet{cid(),
                                                        {d.view("hOverflow"), d.view("hPause")}}
                             : ApplicationHoldActiveSet{cid(maximum_id),
                                                        {d.view("hFail"), d.view("hNew")}});
                else if (which == 9)
                    same(store.history({"hPause", 0, {}, 2}),
                         ApplicationHoldHistoryResult{HD::ready, d.history_page("pause_first2")});
                else if (which == 10)
                    same(store.history({"hOld", 0, {}, 64}),
                         ApplicationHoldHistoryResult{HD::ready, d.history_page("old_resolution")});
                else
                    same(store.active(receipt(7)), ApplicationHoldActiveSet{receipt(7), {}});
            };
            if (mode == 0)
                committed(read(db, call));
            else if (which == 11 && mode == 1)
                committed(read(db, [&](auto s) {
                    marker(s);
                    input_failure([&] { call(s); }, IE::owned_byte_limit);
                    marker_active(s);
                }));
            else
                known(refusal(db, call, SR::result_limit), SR::result_limit);
        });
    }
    void selectedCountsAndFieldOveragesAreDistinctFromCorruption_data() {
        QTest::addColumn<int>("which");
        const char *names[]{"actual-active-count",
                            "actual-occurrence-count",
                            "actual-selected-expected-set-count",
                            "selected-reference-field",
                            "selected-document-field",
                            "selected-exact-components",
                            "understated-count-not-overage",
                            "exact-class-before-both-sizes",
                            "exact-both-sizes-before-grammar"};
        for (int i = 0; i < 9; ++i)
            QTest::newRow(names[i]) << i;
    }
    void selectedCountsAndFieldOveragesAreDistinctFromCorruption() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            SqliteHoldLimits l;
            SR reason = SR::result_limit;
            if (which == 0)
                l.active_holds_per_result = 1;
            if (which == 1)
                l.known_occurrences_per_cause = 1;
            if (which == 2)
                l.active_holds_per_result = 1;
            if (which == 3) {
                l.references.reference_bytes = 9;
                db.modify("UPDATE subathon_hold_cause_observations SET "
                          "policy_revision_ref=zeroblob(100) WHERE hold_ref=X'684F766572666C6F77'");
            }
            if (which == 4)
                l.references.value_bytes = 5;
            if (which == 5)
                l.references.component_bytes = 18;
            if (which >= 6) {
                db.modify("PRAGMA ignore_check_constraints=ON");
                if (which == 6) {
                    db.modify("UPDATE subathon_hold_causes SET occurrence_count=1 WHERE "
                              "hold_ref=X'684F766572666C6F77'");
                    l.known_occurrences_per_cause = 1;
                    reason = SR::invalid_stored_data;
                }
                if (which == 7) {
                    db.modify("UPDATE subathon_hold_occurrences SET "
                              "attempted_numerator=CAST(zeroblob(100) AS "
                              "TEXT),attempted_denominator=zeroblob(100) WHERE "
                              "hold_ref=X'684F766572666C6F77' AND ordinal=1");
                    l.references.component_bytes = 20;
                    reason = SR::invalid_stored_data;
                }
                if (which == 8) {
                    db.modify("UPDATE subathon_hold_occurrences SET "
                              "attempted_numerator=X'3031',attempted_denominator=zeroblob(100) "
                              "WHERE hold_ref=X'684F766572666C6F77' AND ordinal=1");
                    l.references.component_bytes = 20;
                }
                db.modify("PRAGMA ignore_check_constraints=OFF");
            }
            known(refusal(
                      db,
                      [&](auto s) {
                          SqliteApplicationHoldStore store(s, l);
                          if (which == 0)
                              (void)store.active(cid());
                          else
                              (void)store.find(which == 2 ? "hOld" : "hOverflow");
                      },
                      reason),
                  reason);
        });
    }
    void mutationInputItemsCountEveryNestedReplacementBeforeBodies_data() {
        QTest::addColumn<int>("which");
        QTest::addColumn<int>("mode");
        for (int i = 0; i < 3; ++i)
            for (int mode = 0; mode < (i == 0 ? 2 : 3); ++mode)
                QTest::newRow(qPrintable(QString(i == 0   ? "detection-2"
                                                 : i == 1 ? "recorded-batch-3"
                                                          : "replacement-batch-5") +
                                         (mode == 0   ? "-exact-input-fit"
                                          : mode == 1 ? "-input-one-over"
                                                      : "-two-produced-result-items-under-one")))
                    << i << mode;
    }
    void mutationInputItemsCountEveryNestedReplacementBeforeBodies() {
        QFETCH(int, which);
        QFETCH(int, mode);
        scenario([&] {
            const Data d;
            Database db(d);
            SqliteHoldLimits l;
            l.input_items = (which == 0 ? 2 : which == 1 ? 3 : 5) - int(mode == 1);
            if (which > 0)
                l.result_items = mode == 2 ? 1 : 2;
            auto b = which == 1 ? d.resolution("rReplace").request : close_zero();
            if (which == 2) {
                b.mutation_ref = "rNested";
                auto r = append(d, "dOverflow", "dNested", {}, "n1");
                r.cause.hold_ref = "hNested";
                r.cause.supersedes_hold_ref = "hOverflow";
                b.entries[0].replacement = r;
            }
            if (mode == 2) {
                l.result_items = 1;
                known(
                    refusal(
                        db,
                        [&](auto s) { (void)SqliteApplicationHoldStore(s, l).stage_resolution(b); },
                        SR::result_limit),
                    SR::result_limit);
                return;
            }
            committed(read(db, [&](auto s) {
                marker(s);
                SqliteApplicationHoldStore store(s, l);
                const auto call = [&] {
                    return which == 0 ? store.record_detection(d.detection("dOverflow"))
                                      : store.stage_resolution(b);
                };
                if (mode == 1)
                    input_failure([&] { (void)call(); }, IE::collection_limit);
                else if (which == 0)
                    same(call(), mutation(MD::already_recorded, "dOverflow",
                                          ApplicationHoldMutationKind::detection,
                                          {d.view("hOverflow").first_version}));
                else if (which == 1)
                    same(call(), mutation(MD::already_recorded, "rReplace",
                                          ApplicationHoldMutationKind::resolution,
                                          d.resolution("rReplace").produced_versions));
                else
                    same(
                        call(),
                        mutation(MD::recorded, "rNested", ApplicationHoldMutationKind::resolution,
                                 {produced("hOverflow", "v2", 2, "v1",
                                           ApplicationHoldState::resolved, "rNested",
                                           ApplicationHoldReplacementLocator{"hNested", "dNested"}),
                                  produced("hNested", "n1", 1, {}, ApplicationHoldState::active,
                                           "dNested")}));
                marker_active(s);
            }));
        });
    }
    void fixedQueryInputsHaveTheirOwnOwnedBytePreflight_data() {
        QTest::addColumn<int>("which");
        QTest::addColumn<bool>("under");
        for (int i = 0; i < 3; ++i)
            for (bool under : {false, true})
                QTest::newRow(qPrintable(QString(i == 0   ? "missing-find-7"
                                                 : i == 1 ? "active-tag-plus-ID-9"
                                                          : "missing-history-23") +
                                         (under ? "-one-over" : "-exact-fit")))
                    << i << under;
    }
    void fixedQueryInputsHaveTheirOwnOwnedBytePreflight() {
        QFETCH(int, which);
        QFETCH(bool, under);
        scenario([&] {
            const Data d;
            Database db(d);
            SqliteHoldLimits l;
            l.owned_bytes = (which == 0 ? 7 : which == 1 ? 9 : 23) - int(under);
            committed(read(db, [&](auto s) {
                marker(s);
                SqliteApplicationHoldStore store(s, l);
                const auto call = [&] {
                    if (which == 0)
                        require(!store.find("missing"), "Missing find has no owned payload");
                    else if (which == 1)
                        same(store.active(receipt(7)), ApplicationHoldActiveSet{receipt(7), {}});
                    else
                        same(store.history({"missing", 0, {}, 1}),
                             ApplicationHoldHistoryResult{HD::missing_hold, {}});
                };
                if (under)
                    input_failure(call, IE::owned_byte_limit);
                else
                    call();
                marker_active(s);
            }));
        });
    }
    void lateSqlFailureRollsBackResolutionReplacementAndEarlierCallerStaging() {
        scenario([] {
            const Data d;
            Database db(d);
            auto b = close_zero();
            b.mutation_ref = "rLate";
            auto r = append(d, "dPause1", "dLate", {}, "u1");
            r.cause.hold_ref = "hLate";
            r.cause.supersedes_hold_ref = "hOverflow";
            b.entries[0].replacement = r;
            db.modify("CREATE TRIGGER hold_late_failure BEFORE UPDATE ON subathon_hold_heads WHEN "
                      "OLD.hold_ref=X'684F766572666C6F77' BEGIN SELECT RAISE(ABORT,'synthetic late "
                      "head failure'); END");
            known(refusal(
                      db, [&](auto s) { (void)SqliteApplicationHoldStore(s).stage_resolution(b); },
                      SR::sql_failure),
                  SR::sql_failure, WorkError::sql_failure);
            require(db.observer->rows(
                        "SELECT count(*) FROM subathon_hold_causes WHERE hold_ref=X'684C617465'") ==
                            SqlRows{{qint64(0)}} &&
                        db.observer->rows("SELECT hex(version_ref) FROM subathon_hold_heads WHERE "
                                          "hold_ref=X'684F766572666C6F77'") ==
                            SqlRows{{QString("7631")}},
                    "Late failure left neither close nor replacement nor head advance");
        });
    }
    void callerAndAllBorrowedPortsShareActualCumulativeWriterBudgets_data() {
        QTest::addColumn<int>("which");
        for (int i = 0; i < 4; ++i)
            QTest::newRow(i == 0   ? "statement-count"
                          : i == 1 ? "binding-bytes"
                          : i == 2 ? "result-rows"
                                   : "result-bytes")
                << i;
    }
    void callerAndAllBorrowedPortsShareActualCumulativeWriterBudgets() {
        QFETCH(int, which);
        scenario([&] {
            const Data d;
            Database db(d);
            auto l = limits();
            if (which == 0)
                l.statements_per_batch = 1;
            if (which == 1)
                l.bound_value_bytes = 264;
            if (which == 2)
                l.result_rows = 1;
            if (which == 3)
                l.result_value_bytes = 264;
            db.reopen(l);
            const auto before = db.saved();
            bool caught = false;
            const auto reason = which < 2 ? SR::resource_limit : SR::result_limit;
            const auto error = which < 2 ? WorkError::resource_limit : WorkError::result_limit;
            const auto out = capture(*db.writer, [&](auto c) -> QVariant {
                const auto s = c.begin();
                marker(s);
                if (which == 1 || which == 3)
                    require(
                        s.execute({"SELECT ?", {QByteArray(256, 'x')}}) ==
                            SqlRows{{QByteArray(256, 'x')}},
                        "Real caller SELECT consumes 8-byte value overhead plus256 actual bytes");
                if (which == 2)
                    marker_active(s);
                try {
                    (void)SqliteApplicationHoldStore(s).find("hOverflow");
                } catch (const SqliteHoldStorageFailure &e) {
                    require(e.reason() == reason, "Exact shared actual budget reason");
                    db.released();
                    require(db.saved() == before, "Writer already rolled back, no duplicate "
                                                  "terminal cleanup masks original error");
                    caught = true;
                    throw;
                }
                throw std::runtime_error("Required cumulative budget failure missing");
            });
            known(out, reason, error);
            require(caught, "Failure reached inside new port rather than setup/abandonment");
        });
    }
    void foreignConstructionAndRevokedTicketsKeepCapabilityTruthAndPurePriority() {
        scenario([] {
            const Data d;
            Database db(d);
            std::optional<SqliteApplicationHoldStore> saved;
            std::optional<WorkError> foreign;
            std::optional<IE> pure;
            bool missing = false;
            QString problem;
            committed(read(db, [&](auto s) {
                marker(s);
                std::thread worker([&] {
                    try {
                        saved.emplace(s);
                        (void)saved->active(cid());
                    } catch (const SqliteApplicationHoldNotImplemented &) {
                        missing = true;
                    } catch (const SqliteWorkFailure &e) {
                        foreign = e.error();
                    } catch (const std::exception &e) {
                        problem = QString::fromUtf8(e.what());
                    }
                    try {
                        (void)SqliteApplicationHoldStore(s).active(receipt(0));
                    } catch (const SqliteApplicationHoldNotImplemented &) {
                        missing = true;
                    } catch (const SqliteHoldInputFailure &e) {
                        pure = e.error();
                    } catch (const std::exception &e) {
                        problem = QString::fromUtf8(e.what());
                    }
                });
                worker.join();
                if (missing)
                    throw SqliteApplicationHoldNotImplemented();
                require(
                    problem.isEmpty() && foreign == WorkError::wrong_thread &&
                        pure == IE::invalid_subject,
                    "Live foreign ticket rejected before SQL, pure bad input before capability");
                require(saved.has_value(),
                        "Foreign construction is inert and transfers no ownership");
                same(saved->find("hPause"), std::optional{d.view("hPause")});
                marker_active(s);
            }));
            input_failure([&] { (void)saved->find(""); }, IE::invalid_key);
            work_failure([&] { (void)saved->find("hPause"); }, WorkError::stale_session);
            const SqliteApplicationHoldStore empty({});
            work_failure([&] { (void)empty.active(cid()); }, WorkError::stale_session);
        });
    }
    void closedSessionMisuseStaysSynchronousAndSticky() {
        scenario([] {
            const Data d;
            Database db(d);
            const auto before = db.saved();
            const auto out = capture(*db.writer, [&](auto c) -> QVariant {
                const auto s = c.begin();
                SqliteApplicationHoldStore store(s);
                same(store.find("hPause"), std::optional{d.view("hPause")});
                s.rollback();
                input_failure([&] { (void)store.history({"hPause", -1, {}, 1}); },
                              IE::invalid_cursor);
                work_failure([&] { (void)store.active(cid()); }, WorkError::session_closed);
                return 1;
            });
            rolled_back(out, WorkError::session_closed);
            require(db.saved() == before, "No ownership adoption or cleanup from inert facade");
        });
    }
    void stagedDetectionCannotTurnLostCommitAcknowledgementIntoDurableHeld() {
        scenario([] {
            const Data d;
            Database db(d, "pending", true);
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *db.writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "Actual old COMMIT acknowledgement-loss seam installed");
            bool staged = false;
            const auto out = capture(*db.writer, [&](auto c) -> QVariant {
                const auto s = c.begin();
                SqliteApplicationHoldStore store(s);
                same(store.record_detection(d.detection("dFail")),
                     mutation(MD::recorded, "dFail", ApplicationHoldMutationKind::detection,
                              {d.view("hFail").first_version}));
                same(store.find("hFail"), std::optional{d.view("hFail")});
                staged = true;
                s.commit();
                return QString("must not report Held");
            });
            no_missing(out);
            require(
                staged && out.synchronous == WorkError::uncertain_outcome && !out.storage &&
                    !out.input && out.problem.isEmpty() &&
                    out.completion.transaction == TransactionOutcome::outcome_unknown &&
                    out.completion.error == WorkError::uncertain_outcome && !out.completion.value,
                "Original unknown wins despite staged/provisional local value, no normal payload");
            require(db.observer->rows(
                        "SELECT count(*) FROM subathon_hold_causes WHERE hold_ref=X'684661696C'") ==
                        SqlRows{{qint64(1)}},
                    "Physical COMMIT actually happened; this is ack loss, not invented failure");
            require(db.writer->enqueue_work([](auto) -> QVariant { return 0; }, [](auto) {}) ==
                        StorageError::shutting_down,
                    "Actual writer failed closed, no blind retry");
        });
    }
    void uncertainRollbackOutranksLocalSelectedCorruptionWithoutKnownReason() {
        scenario([] {
            const Data d;
            Database db(d);
            db.modify("DELETE FROM subathon_hold_heads WHERE hold_ref=X'685061757365'");
            const auto before = db.saved();
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *db.writer, WorkControlFault::lose_next_rollback_ack) == StorageError::none,
                    "Actual old ROLLBACK acknowledgement-loss seam installed");
            const auto out = capture(*db.writer, [&](auto c) -> QVariant {
                const auto s = c.begin();
                marker(s);
                (void)SqliteApplicationHoldStore(s).active(cid());
                throw std::runtime_error("Required selected fault missing");
            });
            no_missing(out);
            require(out.synchronous == WorkError::uncertain_outcome && !out.storage && !out.input &&
                        out.problem.isEmpty() &&
                        out.completion.transaction == TransactionOutcome::outcome_unknown &&
                        out.completion.error == WorkError::uncertain_outcome &&
                        !out.completion.value,
                    "Original uncertainty replaces local invalid_stored_data; no known rollback "
                    "exception/payload");
            require(db.saved() == before, "Physical rollback removed earlier caller marker");
            require(db.writer->enqueue_work([](auto) -> QVariant { return 0; }, [](auto) {}) ==
                        StorageError::shutting_down,
                    "Unknown rollback fails actual writer closed");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteApplicationHoldTests)
#include "sqlite_application_hold_tests.moc"
