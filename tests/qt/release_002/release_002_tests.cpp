// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/sqlite_test_support.hpp"
#include "stage1_boundary.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <array>
#include <subathon/server/sqlite_permission_graph.hpp>
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

namespace {
using namespace release_002_stage1;
using namespace subathon::core;
using namespace subathon::server;
using qt_test_support::require;
using Utc = std::int64_t;

constexpr auto day_file = "1970-01-01.log";
constexpr auto base_operation = "diagnostics.export.v1";
constexpr auto detail_operation = "diagnostics.events.export.v1";
constexpr auto disposition = "receipt_disposition";
constexpr auto schema_version = "receipt_schema_version";
constexpr auto host_code = "host.serving";
constexpr auto report_code = "connector.report_unavailable";
constexpr Utc first = 1'000'000;
constexpr Utc second = 1'000'001;
constexpr Utc cutoff = 1'210'600'000;
constexpr Utc before_cutoff = 1'210'599'999;
const SessionScope scope{"fixture-installation", "fixture-actor", "fixture-session"};

struct Adapter final : ConnectorAdapter {
    ConnectorReport value;
    ConnectorReport report() const override { return value; }
};

Adapter adapter(ConnectorConnection connection = ConnectorConnection::connected,
                ConnectorGap gap = ConnectorGap::none,
                ConnectorInstallation installation = ConnectorInstallation::validated) {
    Adapter result;
    result.value.connection = connection;
    result.value.gap = gap;
    result.value.installation = installation;
    return result;
}

Adapter setup_adapter(ConnectorConnection connection) {
    auto result = adapter(connection, ConnectorGap::unknown, ConnectorInstallation::missing);
    result.value.account_consent = ConnectorConsent::granted;
    result.value.capabilities.features = {
        {ConnectorFeature::application_registration, ConnectorSupport::supported},
        {ConnectorFeature::webhook_setup, ConnectorSupport::supported}};
    return result;
}

Adapter failed_adapter() {
    return adapter(static_cast<ConnectorConnection>(99), ConnectorGap::unknown);
}

QJsonObject log_row(Utc utc, const char *code = host_code) {
    QJsonObject result{{"utc_ms", QString::number(utc)}, {"code", code}};
    if (code == std::string_view(report_code))
        result.insert("provider", "twitch");
    return result;
}

QByteArray line(Utc utc, const char *code = host_code) {
    return QJsonDocument(log_row(utc, code)).toJson(QJsonDocument::Compact) + '\n';
}

QJsonArray providers(QJsonArray twitch_observed = {}) {
    QJsonArray rows;
    for (auto provider : {"twitch", "betterplace", "patreon", "streamlabs", "streamelements",
                          "kofi", "tipeeestream"}) {
        const auto observed =
            provider == std::string_view("twitch") ? twitch_observed : QJsonArray{};
        rows.append(
            QJsonObject{{"provider", provider}, {"observed", observed}, {"unavailable", true}});
    }
    return rows;
}

QJsonObject artifact(QJsonArray logs = {log_row(first)}, bool available = true,
                     bool limited = false) {
    return {{"format", "subathon.diagnostic-export.v1"},
            {"versions", QJsonObject{{"server", "0.1.0"}, {"qt_runtime", "6.12.0"}}},
            {"connections", providers()},
            {"logs", std::move(logs)},
            {"logs_available", available},
            {"logs_limited", limited}};
}

QByteArray selection(QByteArray ordinal, QByteArray fields) {
    return R"([{"receipt":"1","attempt":")" + ordinal + R"(","fields":)" + fields + "}]";
}

const QByteArray both = selection("1", R"(["receipt_disposition","receipt_schema_version"])");
const QByteArray first_disposition = selection("1", R"(["receipt_disposition"])");
const QByteArray second_disposition = selection("2", R"(["receipt_disposition"])");
const QByteArray retry_then_original =
    R"([{"receipt":"1","attempt":"2","fields":["receipt_disposition"]},)"
    R"({"receipt":"1","attempt":"1","fields":["receipt_schema_version"]}])";

using ReplyList = std::vector<Reply>;

void expect_artifact(const ReplyList &replies, const QJsonObject &expected) {
    QCOMPARE(replies.size(), std::size_t(1));
    QCOMPARE(replies.front().status, ReplyStatus::artifact);
    QVERIFY(replies.front().artifact.size() <= 128 * 1024);
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(replies.front().artifact, &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(parsed.isObject());
    QCOMPARE(parsed.object(), expected);
}

void expect_status(const ReplyList &replies, ReplyStatus status) {
    QCOMPARE(replies.size(), std::size_t(1));
    QCOMPARE(replies.front().status, status);
    QVERIFY(replies.front().artifact.isEmpty());
}

void expect_denial_or_withhold(const ReplyList &replies) {
    if (!replies.empty())
        expect_status(replies, ReplyStatus::denied);
}

void save_file(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate,
                      QFileDevice::ReadOwner | QFileDevice::WriteOwner),
            "open private fixture file");
    require(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner),
            "set private fixture file permissions");
    require(file.write(bytes) == bytes.size() && file.flush(), "write private fixture bytes");
}

QByteArray read_file(const QString &path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "read fixture file");
    return file.readAll();
}

void expect_log_file(const QString &path, const QJsonArray &expected) {
    QVERIFY(QFile::exists(path));
    const auto bytes = read_file(path);
    QVERIFY(bytes.endsWith('\n'));
    auto lines = bytes.split('\n');
    lines.removeLast();
    QCOMPARE(lines.size(), expected.size());
    for (qsizetype i = 0; i < lines.size(); ++i) {
        QVERIFY(lines[i].size() + 1 <= 256);
        QJsonParseError error;
        const auto parsed = QJsonDocument::fromJson(lines[i], &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(parsed.isObject());
        QCOMPARE(parsed.object(), expected[i].toObject());
    }
}

void acknowledged(const SqliteWorkResult &result) {
    require(result.transaction == TransactionOutcome::committed &&
                result.error == WorkError::none && result.value.has_value() &&
                result.value->isValid(),
            "normal committed SQLite producer work");
}

// Only fixture hydration uses the shipped identity-based reader. It is capped
// to two attempts and done outside the handoff turn. The synthetic writer port
// supplies only the requested facts, backed by this acknowledged source.
class Receipts {
  public:
    enum class LastAttempt { creation_only, retry, conflict };

    Receipts(const QString &path, LastAttempt last) {
        require(writer_.open(path) == StorageError::none, "open disposable receipt database");

        const auto migration = std::make_shared<std::optional<MigrationResult>>();
        MigrationRequest request;
        request.mode = MigrationMode::advance;
        request.manifest = receipt_repository_manifest();
        const auto admission =
            SqliteMigrationRunner::enqueue(writer_, std::move(request), [migration](auto result) {
                *migration = std::move(result);
            });
        require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
                "admit shipped receipt prefix on disposable database");
        require(QTest::qWaitFor([migration] { return migration->has_value(); }, 5000),
                "receive shipped receipt prefix result");
        require((*migration)->reason == MigrationReason::none && (*migration)->report &&
                    (*migration)->report->version == 2,
                "fixture uses shipped prefix 2");
        acknowledged((*migration)->work);

        ReceiptInput input{identity_,
                           {"synthetic-kind",
                            1,
                            7,
                            "fixture-actor-id",
                            "fixture-display",
                            ReceiptMoney{"12.30", "EUR"},
                            {},
                            {}},
                           {11, "fixture-connection", "fixture-config", "fixture-arrival"}};
        const auto insert = [&](ReceiptInput value, ReceiptDisposition expected, bool initial) {
            const auto result = qt_test_support::run_work(
                writer_,
                [value = std::move(value), expected, initial](auto work) -> QVariant {
                    auto active = work.begin();
                    if (initial) {
                        SqliteRetainedContextStore retained(active);
                        require(retained.retain({{RetainedKind::configuration, "fixture-config"},
                                                 QByteArray("config"),
                                                 {}}) == RetainDisposition::inserted,
                                "registered fixture configuration");
                        require(retained.retain({{RetainedKind::arrival_context, "fixture-arrival"},
                                                 QByteArray("arrival"),
                                                 ConfigurationReference{"fixture-config"}}) ==
                                    RetainDisposition::inserted,
                                "registered fixture arrival context");
                    }
                    SqliteReceiptRepository repository(adopt_active_receipt_session, active);
                    const auto outcome = ReceiptIntake(repository).ingest(value);
                    require(outcome.receipt_id == ReceiptId{1} && outcome.disposition == expected,
                            "expected normally committed receipt attempt");
                    return true;
                },
                5000);
            acknowledged(result);
        };
        insert(input, ReceiptDisposition::created, true);
        if (last != LastAttempt::creation_only) {
            input.arrival.server_timestamp_utc_ms = 13;
            const auto expected = last == LastAttempt::retry ? ReceiptDisposition::retry
                                                             : ReceiptDisposition::conflict;
            if (last == LastAttempt::conflict)
                input.facts.schema_version = 0;
            insert(input, expected, false);
        }
        completion_ = load(record_);
        require(record_.attempts.size() == (last == LastAttempt::creation_only ? 1U : 2U),
                "bounded acknowledged receipt source record");
    }

    AttemptRead read(const Selector &selector) const {
        std::optional<std::vector<AttemptValue>> values;
        if (selector.receipt == record_.id.value && selector.attempt > 0 &&
            std::uint64_t(selector.attempt) <= record_.attempts.size()) {
            const auto &attempt = record_.attempts.at(std::size_t(selector.attempt - 1));
            values.emplace();
            for (auto field : selector.fields) {
                if (field == Field::disposition)
                    values->emplace_back(attempt.disposition);
                else
                    values->emplace_back(attempt.observed_facts.schema_version);
            }
        }
        return {completion_, std::move(values)};
    }

    void expect_unchanged() {
        ReceiptRecord after;
        load(after);
        QVERIFY(after.id == record_.id);
        QVERIFY(after.original_facts == record_.original_facts);
        QVERIFY(after.original_arrival == record_.original_arrival);
        QVERIFY(after.attempts == record_.attempts);
    }

  private:
    SqliteWorkResult load(ReceiptRecord &record) {
        const auto read = std::make_shared<std::optional<ReceiptRecord>>();
        auto completion = qt_test_support::run_work(
            writer_,
            [identity = identity_, read](auto work) -> QVariant {
                SqliteReceiptRepository repository(work, {.attempts_per_find = 2});
                auto unit = repository.begin();
                *read = unit->find(identity);
                unit->commit();
                return true;
            },
            5000);
        acknowledged(completion);
        require(read->has_value(), "fixture receipt survives");
        record = std::move(**read);
        return completion;
    }

    SqliteWriter writer_;
    const ReceiptIdentity identity_{
        {"fixture-provider", "fixture-origin"}, "fixture-key", "fixture-receipt"};
    ReceiptRecord record_{};
    SqliteWorkResult completion_;
};

enum class MutationPlan { run, refuse, defer };
struct DeferredMutation {
    std::function<MutationOutcome()> work;
    std::function<void(MutationOutcome)> complete;
};

class Fixture {
  public:
    Fixture()
        : directory_(root_.filePath("logs")), audit_(root_.filePath("audit-witness")),
          catalog(reference_permission_graph_trust().catalog) {
        require(root_.isValid(), "allocate private disposable fixture directory");
        require(QDir().mkdir(directory_), "create fixture log directory");
        require(QFile::setPermissions(directory_, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                      QFileDevice::ExeOwner),
                "private service directory");
    }

    void with_history(Receipts::LastAttempt last = Receipts::LastAttempt::creation_only) {
        receipts = std::make_unique<Receipts>(root_.filePath("application.db"), last);
        save_file(audit_, "synthetic independent audit witness\n");
    }

    void expect_history_unchanged() {
        receipts->expect_unchanged();
        QCOMPARE(read_file(audit_), QByteArray("synthetic independent audit witness\n"));
    }

    void launch(bool serving = true) {
        require(!pending, "settle fixture mutation before restarting the store");
        boundary.reset();
        Ports ports{
            [&] {
                trace.append("log-clock");
                return std::optional<Utc>(now);
            },
            [&] {
                trace.append("versions");
                return versions;
            },
            [&](Gate gate, bool require_fresh) {
                require(!require_fresh, "export uses the guarded current non-fresh session scope");
                trace.append(gate == Gate::preparation ? "session-prepare" : "session-handoff");
                return std::optional<SessionObservation>({scope, session_now, expires});
            },
            [&](Gate gate, const SessionScope &current, const PermissionRequest &request) {
                return use(gate, current, request);
            },
            [&](Gate gate, const SessionScope &current, const Selector &selector) {
                require(current == scope, "read uses current installation and session scope");
                trace.append(gate == Gate::preparation ? "attempt-prepare" : "attempt-handoff");
                (gate == Gate::preparation ? prepare_reads : handoff_reads).push_back(selector);
                require(bool(receipts), "unexpected selected attempt lookup");
                auto reply = receipts->read(selector);
                if (gate == Gate::handoff && uncertain_final) {
                    reply.completion = {
                        TransactionOutcome::outcome_unknown, WorkError::uncertain_outcome, {}};
                }
                return reply;
            },
            [&](Mutation kind, auto work, auto complete) {
                mutations.push_back(kind);
                const auto plan = std::exchange(next_mutation, MutationPlan::run);
                if (plan == MutationPlan::refuse) {
                    complete(MutationOutcome::unchanged_refusal);
                } else if (plan == MutationPlan::defer) {
                    require(!pending, "one withheld mutation at a time");
                    pending.emplace(DeferredMutation{std::move(work), std::move(complete)});
                } else {
                    complete(work());
                }
            }};
        boundary = make_stage1_boundary(directory_, std::move(ports));
        boundary->startup();
        if (serving) {
            boundary->host_entered_serving();
            boundary->drain_observations();
        }
    }

    void settle(MutationOutcome outcome) {
        require(pending.has_value(), "fixture has an unsettled mutation");
        auto completion = std::move(pending->complete);
        pending.reset();
        completion(outcome);
    }

    void adapt_catalog() {
        catalog.revision = "stage1-only";
        for (auto operation : {base_operation, detail_operation})
            catalog.operations.push_back(
                {operation, PermissionOperationClass::ordinary, {"installation"}, {}, {}});
        // The evaluator expects a whole well-formed catalog, never just a name check.
        std::ranges::sort(catalog.operations, {}, &PermissionOperationDefinition::operation);
    }

    Use use(Gate gate, const SessionScope &current, const PermissionRequest &request) {
        require(current == scope && request.resources.size() == 1 &&
                    request.resources[0].slot == "installation" &&
                    request.resources[0].resource == scope.installation &&
                    request.quantities.empty(),
                "ordinary USE binds exactly this installation");
        require(request.operation == base_operation || request.operation == detail_operation,
                "unexpected permission operation");
        const bool detail = request.operation == detail_operation;
        trace.append(gate == Gate::preparation ? (detail ? "detail-prepare" : "base-prepare")
                                               : (detail ? "detail-handoff" : "base-handoff"));
        if (std::ranges::find(catalog.operations, request.operation,
                              &PermissionOperationDefinition::operation) ==
            catalog.operations.end())
            return Use::unavailable;

        PermissionPolicyFacts policy{PermissionFactsState::complete, {"fixture-role"}, {}};
        if (!detail || details_granted) {
            policy.entries.push_back(
                {PermissionOrigin::role,
                 "fixture-role-entry",
                 "fixture-role",
                 request.operation,
                 PermissionEffect::allow,
                 "fixture-use",
                 {{"installation", PermissionResourceMode::allowed_set, {scope.installation}}},
                 {}});
        }
        if (detail ? deny_details : deny_base) {
            policy.entries.push_back(
                {PermissionOrigin::individual,
                 "fixture-deny",
                 {},
                 request.operation,
                 PermissionEffect::deny,
                 {},
                 {{"installation", PermissionResourceMode::allowed_set, {scope.installation}}},
                 {}});
        }
        const auto result =
            evaluate_effective_permission({scope.actor,
                                           "fixture-graph",
                                           catalog,
                                           {PermissionOwnerState::present, "fixture-owner"},
                                           std::move(policy),
                                           request});
        return result.data().outcome == PermissionOutcome::permit ? Use::allowed : Use::denied;
    }

    ExportId prepare(std::optional<QByteArray> json = {}) {
        return boundary->prepare(std::move(json));
    }

    ReplyList handoff(ExportId id) {
        ReplyList result;
        boundary->handoff(id, [&](Reply reply) {
            trace.append("transfer");
            result.push_back(std::move(reply));
        });
        return result;
    }

    ReplyList dispatch(std::optional<QByteArray> json = {}) {
        return handoff(prepare(std::move(json)));
    }

    QString path() const { return QDir(directory_).filePath(day_file); }

    void log_failure() {
        boundary->report(1, failed_adapter());
        boundary->drain_observations();
    }

  private:
    QTemporaryDir root_;
    QString directory_;
    QString audit_;

  public:
    PermissionCatalog catalog;
    std::unique_ptr<Receipts> receipts;
    Utc now = first;
    Utc session_now = 101;
    Utc expires = 200;
    VersionObservations versions{{VersionTuple{0, 1, 0}}, {VersionTuple{6, 12, 0}}};
    bool deny_base = false;
    bool deny_details = false;
    bool details_granted = true;
    bool uncertain_final = false;
    QStringList trace;
    std::vector<Selector> prepare_reads;
    std::vector<Selector> handoff_reads;
    std::vector<Mutation> mutations;
    MutationPlan next_mutation = MutationPlan::run;
    std::optional<DeferredMutation> pending;
    std::unique_ptr<Stage1Boundary> boundary;
};

void start_a(Fixture &fixture) {
    fixture.adapt_catalog();
    fixture.launch();
    fixture.trace.clear();
}

void expect_no_reads(const Fixture &fixture) {
    QVERIFY(fixture.prepare_reads.empty());
    QVERIFY(fixture.handoff_reads.empty());
}

void expect_selected(const Fixture &fixture, const std::vector<Selector> &expected) {
    const auto same_selector = [](const Selector &left, const Selector &right) {
        return left.receipt == right.receipt && left.attempt == right.attempt &&
               std::ranges::is_permutation(left.fields, right.fields);
    };
    QVERIFY(std::ranges::is_permutation(fixture.prepare_reads, expected, same_selector));
    QVERIFY(std::ranges::is_permutation(fixture.handoff_reads, expected, same_selector));
    const auto &trace = fixture.trace;
    for (auto gate : {"prepare", "handoff"}) {
        const auto index = [&](const QString &step) { return trace.indexOf(step + '-' + gate); };
        QVERIFY(index("session") >= 0);
        QVERIFY(index("base") > index("session"));
        QVERIFY(index("detail") > index("session"));
        QVERIFY(index("attempt") > std::max(index("base"), index("detail")));
    }
    QVERIFY(trace.lastIndexOf("attempt-handoff") < trace.indexOf("transfer"));
}

void expect_prepared_attempt(const Fixture &fixture) {
    QVERIFY(!fixture.prepare_reads.empty());
    QVERIFY(fixture.handoff_reads.empty());
    QVERIFY(!fixture.trace.contains("transfer"));
}

void expect_invalid_selector(const QByteArray &json) {
    Fixture fixture;
    fixture.with_history(Receipts::LastAttempt::retry);
    start_a(fixture);
    expect_status(fixture.dispatch(json), ReplyStatus::invalid_selection);
    QCOMPARE(fixture.trace, QStringList{"transfer"});
    expect_no_reads(fixture);
}

} // namespace

class Release002Tests final : public QObject {
    Q_OBJECT

  private Q_SLOTS:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void producer_controls() {
        const auto actual = connector_providers();
        using Provider = ConnectorProvider;
        QVERIFY((actual == std::vector{Provider::twitch, Provider::betterplace, Provider::patreon,
                                       Provider::streamlabs, Provider::streamelements,
                                       Provider::kofi, Provider::tipeeestream}));
        const ConnectorSetupContext context{"https://fixture.invalid/callback"};
        const auto setup =
            read_connector_status(setup_adapter(ConnectorConnection::connected), context);
        QCOMPARE(setup.health, ConnectorHealth::setup_required);
        QCOMPARE(setup.gap, ConnectorGap::unknown);
        QCOMPARE(setup.setup.callback_url, context.installation_callback_url);
        const auto open =
            read_connector_status(adapter(ConnectorConnection::connected, ConnectorGap::open), {});
        QCOMPARE(open.health, ConnectorHealth::connected);
        QCOMPARE(open.gap, ConnectorGap::open);

        Fixture fixture;
        const PermissionRequest request{base_operation, {{"installation", scope.installation}}, {}};
        QCOMPARE(fixture.use(Gate::preparation, scope, request), Use::unavailable);
        fixture.adapt_catalog();
        QCOMPARE(fixture.use(Gate::preparation, scope, request), Use::allowed);
        fixture.deny_base = true;
        QCOMPARE(fixture.use(Gate::handoff, scope, request), Use::denied);
        QVERIFY(fixture.trace.contains("base-handoff"));

        fixture.with_history(Receipts::LastAttempt::conflict);
        const auto read =
            fixture.receipts->read({1, 2, {Field::disposition, Field::schema_version}});
        acknowledged(read.completion);
        QVERIFY(read.values.has_value());
        QVERIFY((*read.values ==
                 std::vector<AttemptValue>{ReceiptDisposition::conflict, std::uint32_t{0}}));
    }

    void A() {
        Fixture fixture;
        start_a(fixture);
        const auto id = fixture.prepare();
        QVERIFY(!fixture.trace.contains("transfer"));
        expect_artifact(fixture.handoff(id), artifact());
        QVERIFY(fixture.handoff(id).empty());
        expect_no_reads(fixture);
        const auto &trace = fixture.trace;
        QVERIFY(trace.indexOf("session-prepare") >= 0);
        QVERIFY(trace.indexOf("base-prepare") > trace.indexOf("session-prepare"));
        QVERIFY(trace.indexOf("versions") > trace.indexOf("base-prepare"));
        QVERIFY(trace.indexOf("session-handoff") > trace.indexOf("versions"));
        QVERIFY(trace.indexOf("base-handoff") > trace.indexOf("session-handoff"));
        QVERIFY(trace.indexOf("transfer") > trace.indexOf("base-handoff"));
        QVERIFY(!trace.contains("detail-prepare") && !trace.contains("detail-handoff"));
    }

    void A_unadapted() {
        Fixture fixture;
        fixture.launch();
        fixture.trace.clear();
        QVERIFY(fixture.dispatch().empty());
        const auto session = fixture.trace.indexOf("session-prepare");
        QVERIFY(session >= 0);
        QVERIFY(fixture.trace.indexOf("base-prepare") > session);
        QVERIFY(!fixture.trace.contains("versions"));
        expect_no_reads(fixture);
    }

    void E1() {
        Fixture fixture;
        start_a(fixture);
        fixture.versions.qt_runtime.reset();
        const std::array reports{adapter(),
                                 adapter(ConnectorConnection::disconnected),
                                 setup_adapter(ConnectorConnection::connected),
                                 adapter(ConnectorConnection::connected, ConnectorGap::open),
                                 adapter(ConnectorConnection::connected, ConnectorGap::open),
                                 setup_adapter(static_cast<ConnectorConnection>(99))};
        for (unsigned i = 0; i < reports.size(); ++i) {
            fixture.boundary->register_adapter(i + 1, ConnectorProvider::twitch);
            fixture.boundary->report(i + 1, reports[i], {"https://fixture.invalid/callback"});
        }
        const auto prepared = fixture.prepare();
        fixture.boundary->drain_observations();
        auto expected = artifact();
        expected["versions"] = QJsonObject{{"server", "0.1.0"}, {"qt_runtime", "unknown"}};
        expected["connections"] = providers({
            QJsonObject{{"health", "setup_required"}, {"gap", "unknown"}},
            QJsonObject{{"health", "disconnected"}, {"gap", "none"}},
            QJsonObject{{"health", "connected"}, {"gap", "open"}},
            QJsonObject{{"health", "connected"}, {"gap", "none"}},
        });
        expect_artifact(fixture.handoff(prepared), expected);
        expected["logs"] = QJsonArray{log_row(first), log_row(first, report_code)};
        expect_artifact(fixture.dispatch(), expected);
        expect_no_reads(fixture);
    }

    void E2() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::retry);
        start_a(fixture);
        auto expected = artifact();
        expected["events"] =
            QJsonArray{QJsonObject{{disposition, "retry"}}, QJsonObject{{schema_version, "1"}}};
        expect_artifact(fixture.dispatch(retry_then_original), expected);
        expect_selected(fixture, {{1, 2, {Field::disposition}}, {1, 1, {Field::schema_version}}});
    }

    void E2_empty() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::retry);
        start_a(fixture);
        expect_artifact(fixture.dispatch(QByteArray("[]")), artifact());
        expect_no_reads(fixture);
        QVERIFY(!fixture.trace.contains("detail-prepare") &&
                !fixture.trace.contains("detail-handoff"));
    }

    void E3_denied() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::retry);
        start_a(fixture);
        fixture.deny_details = true;
        expect_status(fixture.dispatch(both), ReplyStatus::denied);
        const auto session = fixture.trace.indexOf("session-prepare");
        QVERIFY(session >= 0);
        QVERIFY(fixture.trace.indexOf("base-prepare") > session);
        const auto detail = fixture.trace.indexOf("detail-prepare");
        const auto transfer = fixture.trace.indexOf("transfer");
        QVERIFY(detail > session);
        QVERIFY(transfer > fixture.trace.indexOf("base-prepare"));
        QVERIFY(transfer > detail);
        QVERIFY(!fixture.trace.contains("versions"));
        expect_no_reads(fixture);
    }

    void E3_invalid_duplicate_attempt() {
        expect_invalid_selector(
            R"([{"receipt":"1","attempt":"1","fields":["receipt_disposition"]},)"
            R"({"receipt":"1","attempt":"1","fields":["receipt_schema_version"]}])");
    }

    void E3_invalid_duplicate_key() {
        expect_invalid_selector(
            R"([{"receipt":"1","rece\u0069pt":"1","attempt":"1","fields":["receipt_disposition"]}])");
    }

    void E4_revoke() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::retry);
        start_a(fixture);
        const auto prepared = fixture.prepare(first_disposition);
        expect_prepared_attempt(fixture);
        fixture.deny_base = true;
        expect_denial_or_withhold(fixture.handoff(prepared));
        QVERIFY(fixture.trace.contains("session-handoff"));
        QVERIFY(fixture.trace.contains("base-handoff"));
        QVERIFY(fixture.handoff_reads.empty());
        fixture.deny_base = false;
        expect_artifact(fixture.dispatch(), artifact());
        QVERIFY(fixture.handoff(prepared).empty());
    }

    void E4_expiry() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::retry);
        start_a(fixture);
        fixture.session_now = fixture.expires - 1;
        const auto prepared = fixture.prepare(first_disposition);
        expect_prepared_attempt(fixture);
        fixture.session_now = fixture.expires;
        QVERIFY(fixture.handoff(prepared).empty());
        QVERIFY(fixture.trace.contains("session-handoff"));
        QVERIFY(!fixture.trace.contains("base-handoff"));
        QVERIFY(fixture.handoff_reads.empty());
    }

    void E4_after_transfer() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::retry);
        start_a(fixture);
        auto expected = artifact();
        expected["events"] = QJsonArray{QJsonObject{{disposition, "created"}}};
        auto released = fixture.dispatch(first_disposition);
        fixture.deny_base = true;
        expect_artifact(released, expected);
        expect_status(fixture.dispatch(first_disposition), ReplyStatus::denied);
    }

    void E5() {
        Fixture fixture;
        fixture.with_history();
        start_a(fixture);
        fixture.now = second;
        fixture.boundary->host_entered_serving();
        fixture.boundary->drain_observations();
        QVERIFY(QFile::exists(fixture.path()));
        const auto intact = read_file(fixture.path());
        fixture.now = cutoff;
        fixture.boundary->sweep();
        expect_artifact(fixture.dispatch(), artifact({log_row(second)}));
        expect_log_file(fixture.path(), {log_row(first), log_row(second)});
        QCOMPARE(read_file(fixture.path()), intact);
        fixture.now = cutoff + 1;
        fixture.boundary->sweep();
        expect_artifact(fixture.dispatch(), artifact({}));
        QVERIFY(!QFile::exists(fixture.path()));
        fixture.expect_history_unchanged();
        expect_no_reads(fixture);
    }

    void E5_unlink_refused() {
        Fixture fixture;
        fixture.with_history();
        fixture.adapt_catalog();
        save_file(fixture.path(), line(first) + line(second));
        fixture.now = cutoff;
        fixture.launch(false);
        const auto before = fixture.mutations.size();
        fixture.next_mutation = MutationPlan::refuse;
        fixture.now = cutoff + 1;
        fixture.boundary->sweep();
        QCOMPARE(fixture.mutations.size(), before + 1);
        QCOMPARE(fixture.mutations.back(), Mutation::unlink);
        // The controlled refusal persists through the next export even if it
        // attempts another expiry operation.
        fixture.next_mutation = MutationPlan::refuse;
        expect_artifact(fixture.dispatch(), artifact({}));
        QCOMPARE(read_file(fixture.path()), line(first) + line(second));
        fixture.expect_history_unchanged();
    }

    void E6_cutoff() {
        Fixture fixture;
        fixture.with_history();
        fixture.adapt_catalog();
        save_file(fixture.path(), line(first) + line(second));
        fixture.now = before_cutoff;
        fixture.launch(false);
        expect_artifact(fixture.dispatch(), artifact({log_row(first), log_row(second)}));
        const auto prepared = fixture.prepare();
        fixture.now = cutoff;
        fixture.trace.clear();
        expect_artifact(fixture.handoff(prepared), artifact({log_row(second)}));
        QVERIFY(fixture.trace.indexOf("log-clock") >= 0);
        QVERIFY(fixture.trace.indexOf("transfer") > fixture.trace.indexOf("log-clock"));
    }

    void E6_clock() {
        for (bool recover_before_handoff : {false, true}) {
            Fixture fixture;
            fixture.with_history();
            fixture.adapt_catalog();
            save_file(fixture.path(), line(first) + line(second));
            fixture.now = before_cutoff;
            fixture.launch(false);
            expect_artifact(fixture.dispatch(), artifact({log_row(first), log_row(second)}));
            const auto prepared = fixture.prepare();
            fixture.now = before_cutoff - 1;
            if (recover_before_handoff) {
                fixture.boundary->sweep(); // Observes the regression while staging is live.
                fixture.now = before_cutoff;
                // A coalesced health notice may be attempted on recovery. Keep
                // this schedule free of intervening stored rows.
                fixture.next_mutation = MutationPlan::refuse;
                fixture.boundary->sweep();
            }
            expect_artifact(fixture.handoff(prepared), artifact({}, false));
            fixture.now = before_cutoff;
            fixture.next_mutation = MutationPlan::refuse;
            expect_artifact(fixture.dispatch(), artifact({log_row(first), log_row(second)}));
            QCOMPARE(read_file(fixture.path()), line(first) + line(second));
        }
    }

    void E7() {
        Fixture fixture;
        start_a(fixture);
        fixture.boundary->register_adapter(1, ConnectorProvider::twitch);
        fixture.boundary->submit_connector_candidate(1,
                                                     ExtraMembers::header_identifier_and_message);
        fixture.boundary->drain_observations();
        expect_artifact(fixture.dispatch(), artifact());
        expect_log_file(fixture.path(), {log_row(first)});

        fixture.now = second;
        fixture.log_failure();
        fixture.log_failure(); // Repeated failed observation in the same registered state.
        expect_artifact(fixture.dispatch(),
                        artifact({log_row(first), log_row(second, report_code)}));

        fixture.boundary->report(1, adapter());
        fixture.boundary->drain_observations();
        fixture.now = second + 1;
        fixture.log_failure(); // A genuine valid report rearms the owned transition.
        expect_artifact(fixture.dispatch(), artifact({log_row(first), log_row(second, report_code),
                                                      log_row(second + 1, report_code)}));
        expect_log_file(fixture.path(), {log_row(first), log_row(second, report_code),
                                         log_row(second + 1, report_code)});
    }

    void E8_capacity() {
        Fixture fixture;
        fixture.with_history();
        fixture.adapt_catalog();
        // The reviewed startup store shape already holds the complete admitted
        // transitions. Replay the next owned failure through the live boundary.
        const auto stored = line(first, report_code).repeated(245'760);
        QVERIFY(stored.size() < 60 * 1024 * 1024);
        save_file(fixture.path(), stored);
        fixture.now = second;
        fixture.launch(false);
        fixture.boundary->register_adapter(1, ConnectorProvider::twitch);
        fixture.boundary->report(1, adapter());
        fixture.boundary->drain_observations();
        const auto before = fixture.mutations.size();
        fixture.log_failure();
        QCOMPARE(fixture.mutations.size(), before);
        QCOMPARE(read_file(fixture.path()), stored);
        QVERIFY((fixture.boundary->pending_notices() == std::vector{LogNotice::capacity_reached}));

        QJsonArray newest;
        for (int i = 0; i < 512; ++i)
            newest.append(log_row(first, report_code));
        expect_artifact(fixture.dispatch(), artifact(newest, true, true));
        fixture.expect_history_unchanged();
    }

    void E8_uncertain_final_read() {
        Fixture fixture;
        fixture.with_history();
        start_a(fixture);
        const auto prepared = fixture.prepare(first_disposition);
        expect_prepared_attempt(fixture);
        fixture.uncertain_final = true;
        QVERIFY(fixture.handoff(prepared).empty());
        QVERIFY((fixture.handoff_reads == std::vector<Selector>{{1, 1, {Field::disposition}}}));
        fixture.uncertain_final = false;
        expect_artifact(fixture.dispatch(), artifact());
        QVERIFY(fixture.handoff(prepared).empty());
        fixture.expect_history_unchanged();
    }

    void E9() {
        for (auto deny : {Gate::preparation, Gate::handoff}) {
            Fixture fixture;
            fixture.with_history();
            start_a(fixture);
            expect_status(fixture.dispatch(second_disposition), ReplyStatus::selection_unavailable);
            expect_selected(fixture, {{1, 2, {Field::disposition}}});
            fixture.trace.clear();
            fixture.prepare_reads.clear();
            fixture.handoff_reads.clear();
            fixture.details_granted = deny != Gate::preparation;
            const auto prepared = fixture.prepare(second_disposition);
            if (deny == Gate::preparation) {
                expect_status(fixture.handoff(prepared), ReplyStatus::denied);
                expect_no_reads(fixture);
            } else {
                expect_prepared_attempt(fixture);
                fixture.deny_details = true;
                expect_denial_or_withhold(fixture.handoff(prepared));
                QVERIFY(
                    (fixture.prepare_reads == std::vector<Selector>{{1, 2, {Field::disposition}}}));
                QVERIFY(fixture.trace.contains("detail-handoff"));
                QVERIFY(fixture.handoff_reads.empty());
            }
        }
    }

    void E10() {
        Fixture fixture;
        fixture.with_history(Receipts::LastAttempt::conflict);
        start_a(fixture);
        auto expected = artifact();
        expected["events"] =
            QJsonArray{QJsonObject{{disposition, "conflict"}, {schema_version, "0"}}};
        expect_artifact(
            fixture.dispatch(selection("2", R"(["receipt_disposition","receipt_schema_version"])")),
            expected);
        expect_selected(fixture, {{1, 2, {Field::disposition, Field::schema_version}}});
    }

    void E11() {
        Fixture fixture;
        fixture.adapt_catalog();
        QByteArray stored;
        QJsonArray newest;
        for (int i = 0; i < 514; ++i) {
            stored += line(first + i, report_code);
            if (i >= 2)
                newest.append(log_row(first + i, report_code));
        }
        // Same exact sample at both gates: index 0 expired, index 1 eligible
        // and omitted. The remaining 512 are serially distinguishable.
        fixture.now = cutoff;
        save_file(fixture.path(), stored);
        fixture.launch(false);
        expect_artifact(fixture.dispatch(), artifact(newest, true, true));
        QCOMPARE(read_file(fixture.path()), stored);
        expect_no_reads(fixture);

        // The oldest omitted eligible row expires at a later handoff. The
        // preparation omission flag persists even though it would be false
        // for a fresh preparation at this time.
        const auto prepared = fixture.prepare();
        fixture.now = cutoff + 1;
        expect_artifact(fixture.handoff(prepared), artifact(newest, true, true));
        expect_artifact(fixture.dispatch(), artifact(newest, true, false));
    }

    void E12_hold() {
        Fixture fixture;
        fixture.with_history();
        start_a(fixture);
        const auto prepared = fixture.prepare();
        fixture.boundary->register_adapter(1, ConnectorProvider::twitch);
        fixture.now = second;
        fixture.next_mutation = MutationPlan::defer;
        fixture.log_failure();
        QVERIFY(fixture.pending.has_value());
        QCOMPARE(fixture.mutations.back(), Mutation::append);
        const auto stopped_prefix = read_file(fixture.path());
        {
            // Only the fault fixture touches the disposable file while the
            // nominal mutation is suspended, to leave an interrupted tail.
            QFile interrupted(fixture.path());
            QVERIFY(interrupted.open(QIODevice::Append));
            QCOMPARE(interrupted.write("{"), 1);
            QVERIFY(interrupted.flush());
        }
        fixture.settle(MutationOutcome::uncertain);
        expect_artifact(fixture.handoff(prepared), artifact({}, false));

        fixture.boundary->report(1, adapter());
        fixture.boundary->drain_observations();
        const auto before = fixture.mutations.size();
        fixture.now = second + 1;
        fixture.log_failure(); // Good clock and rearmed producer while integrity still held.
        QCOMPARE(fixture.mutations.size(), before);
        QCOMPARE(read_file(fixture.path()), stopped_prefix + "{");
        expect_artifact(fixture.dispatch(), artifact({}, false));
        fixture.launch(false);
        expect_artifact(fixture.dispatch(), artifact({log_row(first)}));
        fixture.expect_history_unchanged();
    }

    void E12_pending() {
        Fixture fixture;
        start_a(fixture);
        const auto prepared = fixture.prepare();
        fixture.boundary->register_adapter(1, ConnectorProvider::twitch);
        fixture.now = second;
        fixture.next_mutation = MutationPlan::defer;
        fixture.log_failure();
        QVERIFY(fixture.pending.has_value());
        QCOMPARE(fixture.mutations.back(), Mutation::append);
        const auto before = fixture.mutations.size();
        expect_artifact(fixture.handoff(prepared), artifact({}, false));
        QVERIFY(fixture.pending.has_value());
        QCOMPARE(fixture.mutations.size(), before);
        expect_log_file(fixture.path(), {log_row(first)});
        const auto result = fixture.pending->work();
        QCOMPARE(result, MutationOutcome::changed);
        fixture.settle(result);
        expect_artifact(fixture.dispatch(),
                        artifact({log_row(first), log_row(second, report_code)}));
    }

    void E12_restart() {
        for (const bool complete : {false, true}) {
            Fixture fixture;
            fixture.with_history();
            fixture.adapt_catalog();
            const auto seeded = line(first) + (complete ? "{}\n" : "{");
            save_file(fixture.path(), seeded);
            fixture.now = second;
            fixture.launch(false);
            expect_artifact(fixture.dispatch(),
                            complete ? artifact({}, false) : artifact({log_row(first)}));
            if (!complete) {
                fixture.boundary->host_entered_serving();
                fixture.boundary->drain_observations();
                expect_artifact(fixture.dispatch(), artifact({log_row(first), log_row(second)}));
                expect_log_file(fixture.path(), {log_row(first), log_row(second)});
            } else {
                fixture.boundary->host_entered_serving();
                fixture.boundary->drain_observations();
                fixture.boundary->sweep();
                QCOMPARE(read_file(fixture.path()), seeded);
                fixture.boundary.reset(); // Stopped-service operator repair.
                save_file(fixture.path(), line(first));
                fixture.launch(false);
                expect_artifact(fixture.dispatch(), artifact({log_row(first)}));
            }
            fixture.expect_history_unchanged();
        }
    }

    void E13() {
        Fixture fixture;
        fixture.adapt_catalog();
        save_file(fixture.path(), line(second));
        fixture.now = second;
        fixture.launch(false);
        fixture.now = 2'000'000;
        fixture.boundary->sweep();
        fixture.now = 1'500'000;
        expect_artifact(fixture.dispatch(), artifact({}, false));

        fixture.launch(false);
        expect_artifact(fixture.dispatch(), artifact({log_row(second)}));

        fixture.now = first;
        const auto before = fixture.mutations.size();
        fixture.launch(false);
        expect_artifact(fixture.dispatch(), artifact({}, false));
        fixture.boundary->sweep();
        fixture.boundary->host_entered_serving();
        fixture.boundary->drain_observations();
        QCOMPARE(fixture.mutations.size(), before);
        QCOMPARE(read_file(fixture.path()), line(second));
    }

    void E14() {
        constexpr Utc first_cutoff = 1'209'600'000;
        constexpr Utc last_row = 86'399'999;
        constexpr Utc final_cutoff = 1'295'999'999;
        Fixture fixture;
        fixture.adapt_catalog();
        save_file(fixture.path(), line(0) + line(last_row));
        fixture.now = first_cutoff;
        fixture.launch(false);
        fixture.boundary->sweep();
        expect_artifact(fixture.dispatch(), artifact({log_row(last_row)}));
        QCOMPARE(read_file(fixture.path()), line(0) + line(last_row));

        fixture.now = final_cutoff - 1;
        fixture.boundary->sweep();
        expect_artifact(fixture.dispatch(), artifact({log_row(last_row)}));
        QCOMPARE(read_file(fixture.path()), line(0) + line(last_row));

        fixture.now = final_cutoff;
        fixture.boundary->sweep();
        expect_artifact(fixture.dispatch(), artifact({}));
        QVERIFY(!QFile::exists(fixture.path()));
    }
};

QTEST_GUILESS_MAIN(Release002Tests)
#include "release_002_tests.moc"
