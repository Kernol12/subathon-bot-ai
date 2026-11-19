// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application_service_fixture.hpp"
#include "sqlite_permission_graph_fixture.hpp"

#include <QTemporaryDir>
#include <subathon/server/sqlite_delivery_catalog.hpp>
#include <subathon/server/sqlite_milestone_groups.hpp>
#include <tuple>

namespace milestone_group_restart_fixture {
using namespace std::chrono_literals;
using namespace subathon::server;
using qt_test_support::require;
namespace app = application_service_fixture;

constexpr int timeout = 10000;
const ApplicationOperationKey origin{"svc", "add10"};
const ApplicationOperationKey following_origin{"svc", "negative_stop"};
constexpr auto marker = "subathon.internal.delivery-job.v1";
const QByteArray action_document = QByteArray::fromHex("6561726e656400ff");

// Revision 1 SQL literals. Recovery, release and bonus setup must be able to
// reach their own public boundaries without running the product v12 migration
// or group enrollment.
inline const QStringList ddl{
    "CREATE TABLE subathon_milestone_groups("
    "occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) "
    "BETWEEN 1 AND 4096), "
    "operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND "
    "length(operation_scope) BETWEEN 1 AND 4096), "
    "operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref) "
    "BETWEEN 1 AND 4096), "
    "queue_order INTEGER NOT NULL CHECK(typeof(queue_order)='integer' AND queue_order>=1), "
    "milestone_ref BLOB NOT NULL CHECK(typeof(milestone_ref)='blob' AND length(milestone_ref) "
    "BETWEEN 1 AND 4096), "
    "definition_revision_ref BLOB NOT NULL CHECK(typeof(definition_revision_ref)='blob' AND "
    "length(definition_revision_ref) BETWEEN 1 AND 4096), "
    "target_micro_points INTEGER NOT NULL CHECK(typeof(target_micro_points)='integer'), "
    "score_micro_points INTEGER NOT NULL CHECK(typeof(score_micro_points)='integer'), "
    "completion_context_ref BLOB NOT NULL CHECK(typeof(completion_context_ref)='blob' AND "
    "length(completion_context_ref) BETWEEN 1 AND 4096), "
    "presentation_ms INTEGER CHECK(presentation_ms IS NULL OR (typeof(presentation_ms)='integer' "
    "AND presentation_ms>0)), "
    "gap_ms INTEGER CHECK(gap_ms IS NULL OR (typeof(gap_ms)='integer' AND gap_ms>=0)), "
    "released INTEGER NOT NULL CHECK(typeof(released)='integer' AND released IN (0,1)), "
    "PRIMARY KEY(occurrence_ref), UNIQUE(queue_order), FOREIGN KEY(operation_scope,operation_ref) "
    "REFERENCES subathon_app_operations(operation_scope,operation_ref))",
    "CREATE TABLE subathon_milestone_actions("
    "occurrence_ref BLOB NOT NULL, action_ordinal INTEGER NOT NULL "
    "CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1), "
    "kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind IN (0,1)), "
    "action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob' AND length(action_ref) BETWEEN 1 AND "
    "4096), "
    "version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref) BETWEEN 1 "
    "AND 4096), "
    "operation_scope BLOB, operation_ref BLOB, job_ref BLOB, "
    "CHECK((kind=0 AND operation_scope IS NOT NULL AND operation_ref IS NOT NULL AND job_ref IS "
    "NOT NULL) OR "
    "(kind=1 AND operation_scope IS NULL AND operation_ref IS NULL AND job_ref IS NULL)), "
    "PRIMARY KEY(occurrence_ref,action_ordinal), UNIQUE(operation_scope,operation_ref,job_ref), "
    "FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref), "
    "FOREIGN KEY(operation_scope,operation_ref,job_ref) REFERENCES "
    "subathon_delivery_captures(operation_scope,operation_ref,job_ref))",
    "CREATE TABLE subathon_milestone_bonus_receipts("
    "occurrence_ref BLOB NOT NULL, action_ordinal INTEGER NOT NULL, "
    "effect_ref BLOB NOT NULL CHECK(typeof(effect_ref)='blob' AND length(effect_ref) BETWEEN 1 AND "
    "4096), "
    "PRIMARY KEY(occurrence_ref,action_ordinal), FOREIGN KEY(occurrence_ref,action_ordinal) "
    "REFERENCES subathon_milestone_actions(occurrence_ref,action_ordinal))",
    "CREATE INDEX subathon_milestone_pending_order ON subathon_milestone_groups(queue_order) WHERE "
    "released=0",
};

inline MigrationStep literal_step(const MigrationManifest &prefix, QStringList statements = ddl) {
    MigrationStep step{12, "milestone-groups-v1", std::move(statements),
                       prefix.steps.back().expected_schema};
    const QStringList tables{"subathon_milestone_groups", "subathon_milestone_actions",
                             "subathon_milestone_bonus_receipts"};
    for (qsizetype i = 0; i < tables.size(); ++i) {
        const auto &name = tables[i];
        step.expected_schema.append({"table", name, name, step.sql[i]});
        step.expected_schema.append({"index", "sqlite_autoindex_" + name + "_1", name, {}});
        if (i != 2)
            step.expected_schema.append({"index", "sqlite_autoindex_" + name + "_2", name, {}});
    }
    step.expected_schema.append(
        {"index", "subathon_milestone_pending_order", tables[0], step.sql[3]});
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    return step;
}

inline void normal(const SqliteWorkResult &result) {
    require(result.transaction == TransactionOutcome::committed &&
                result.error == WorkError::none && result.value.has_value(),
            QString("Expected acknowledged normal commit. Transaction %1, error %2")
                .arg(int(result.transaction))
                .arg(int(result.error)));
}

inline void migrated(const MigrationResult &result, int version, int applied) {
    normal(result.work);
    require(result.reason == MigrationReason::none && result.report &&
                result.report->state == CatalogState::current &&
                result.report->version == version && result.report->target_version == version &&
                result.report->applied == applied,
            "Explicit migration returns the exact committed catalog report");
}

inline SqliteWorkResult work(SqliteWriter &writer, SqliteWriter::Work body) {
    const auto failure = std::make_shared<std::exception_ptr>();
    auto result = qt_test_support::run_work(
        writer,
        [failure, body = std::move(body)](auto context) {
            try {
                return body(context);
            } catch (...) {
                *failure = std::current_exception();
                throw;
            }
        },
        timeout);
    if (*failure && result.error == WorkError::work_exception)
        std::rethrow_exception(*failure);
    return result;
}

template <typename Call> auto committed(SqliteWriter &writer, Call call) {
    using Result = decltype(call(std::declval<SqliteWorkSession>()));
    const auto value = std::make_shared<std::optional<Result>>();
    normal(work(writer, [value, call = std::move(call)](auto context) -> QVariant {
        const auto s = context.begin();
        value->emplace(call(s));
        s.commit();
        return 1;
    }));
    return std::move(value->value());
}

inline std::vector<CapturedMilestoneGroup> captures() {
    using Kind = MilestoneActionKind;
    return {
        {{{"peak-z", "track-v3", 100, 150, "crossing-9"},
          {{Kind::automation, "announce", "v7"},
           {Kind::bonus, "award", "v4"},
           {Kind::automation, "celebrate", "v9"},
           {Kind::automation, "post", "v2"}},
          {2500ms, 400ms},
          MilestoneOccurrenceKey{"z-first"}},
         {{1, {origin, "job"}}, {3, {origin, "done"}}, {4, {origin, "stranded"}}}},
        {{{"peak-a", "track-v4", 200, 350, "crossing-10"},
          {{Kind::bonus, "award", "v4"}, {Kind::automation, "announce", "v8"}},
          {{}, 0ms},
          MilestoneOccurrenceKey{"a-second"}},
         {{2, {following_origin, "job"}}}},
        {{{"peak-m", "track-v4", 300, 350, "crossing-10"},
          {{Kind::bonus, "award", "v4"}},
          {},
          MilestoneOccurrenceKey{"m-third"}},
         {}},
        {{{"peak-b", "track-v4", 350, 350, "crossing-10"},
          {},
          {},
          MilestoneOccurrenceKey{"b-empty"}},
         {}},
    };
}

inline void seed_head(const SqliteWorkSession &s, int released) {
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_groups VALUES "
         "(?,?,?,?,?,?,?,?,?,2500,400,?)",
         {QByteArray("z-first"), QByteArray("svc"), QByteArray("add10"), 1, QByteArray("peak-z"),
          QByteArray("track-v3"), 100, 150, QByteArray("crossing-9"), released}});
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_actions VALUES "
         "(?,1,0,?,?,X'737663',X'6164643130',?),"
         "(?,2,1,?,?,NULL,NULL,NULL),"
         "(?,3,0,?,?,X'737663',X'6164643130',?),"
         "(?,4,0,?,?,X'737663',X'6164643130',?)",
         {QByteArray("z-first"), QByteArray("announce"), QByteArray("v7"), QByteArray("job"),
          QByteArray("z-first"), QByteArray("award"), QByteArray("v4"), QByteArray("z-first"),
          QByteArray("celebrate"), QByteArray("v9"), QByteArray("done"), QByteArray("z-first"),
          QByteArray("post"), QByteArray("v2"), QByteArray("stranded")}});
}

inline void seed_suffix(const SqliteWorkSession &s) {
    (void)s.execute({"INSERT INTO main.subathon_milestone_groups VALUES "
                     "(?,X'737663',?,2,?,?,200,350,?,NULL,0,0),"
                     "(?,X'737663',?,3,?,?,300,350,?,NULL,NULL,0),"
                     "(?,X'737663',?,4,?,?,350,350,?,NULL,NULL,0)",
                     {QByteArray("a-second"), QByteArray("negative_stop"), QByteArray("peak-a"),
                      QByteArray("track-v4"), QByteArray("crossing-10"), QByteArray("m-third"),
                      QByteArray("negative_stop"), QByteArray("peak-m"), QByteArray("track-v4"),
                      QByteArray("crossing-10"), QByteArray("b-empty"),
                      QByteArray("negative_stop"), QByteArray("peak-b"), QByteArray("track-v4"),
                      QByteArray("crossing-10")}});
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_actions VALUES "
         "(?,1,1,?,?,NULL,NULL,NULL),"
         "(?,2,0,?,?,X'737663',?,X'6a6f62'),"
         "(?,1,1,?,?,NULL,NULL,NULL)",
         {QByteArray("a-second"), QByteArray("award"), QByteArray("v4"), QByteArray("a-second"),
          QByteArray("announce"), QByteArray("v8"), QByteArray("negative_stop"),
          QByteArray("m-third"), QByteArray("award"), QByteArray("v4")}});
}

inline void stage_origin(const SqliteWorkSession &s, PreparedApplicationBundle bundle) {
    bundle.result = {"subathon.storage.synthetic-result.v1", QByteArray("milestone-example")};
    auto job = bundle.jobs.at(0);
    job.envelope = {marker, action_document};
    // Deliberate collision with a typed occurrence when considered as generic
    // data. All four jobs share the generic grouping, only three are members.
    job.group_ref = "a-second";
    bundle.jobs.clear();
    std::int64_t ordinal = 0;
    for (const auto *ref : {"job", "done", "stranded", "legacy"}) {
        job.job_ref = ref;
        job.group_ordinal = ++ordinal;
        bundle.jobs.push_back(job);
    }
    const std::vector<DeliveryCapture> delivery{
        {"job", "synthetic/channel", DeliverySenderSelection::explicit_identity, "sender",
         "announce", "v7"},
        {"done", "synthetic/channel", DeliverySenderSelection::explicit_identity, "sender",
         "celebrate", "v9"},
        {"stranded", "synthetic/channel", DeliverySenderSelection::explicit_identity, "sender",
         "post", "v2"},
        {"legacy", "synthetic/channel", DeliverySenderSelection::explicit_identity, "sender",
         "announce", "v7"},
    };
    require(SqliteDeliveryJobStore(s).stage(bundle, delivery).disposition ==
                SqliteBundleStageDisposition::staged,
            "Existing application and delivery ports stage the fixture parents");
}

inline PreparedApplicationBundle following_bundle(const app::Data &data,
                                                  const ApplicationStateSnapshot &head) {
    auto bundle = app::prepared(data.case_data("negative_stop")["expected_record"]);
    // On the preceding committed head, Main=60 and debt=0. Stop at zero
    // applies -60 of the retained -150 request, leaving -90 unapplied.
    bundle.intent.expected_state_token = "c";
    bundle.advanced_before = head;
    bundle.advance_explanation.clear();
    bundle.after.token = "d";
    bundle.after.time_debt_ms = 0;
    // This fixture uses the generic application bundle store, whose state
    // document is opaque. The numeric projection is specified above.
    bundle.after.envelope.document = QByteArray("synthetic-following-state");
    bundle.effects.at(0).components.at(0).units = -60;
    bundle.effects.at(0).components.at(2).units = -90;
    bundle.jobs.at(0).envelope = {marker, action_document};
    bundle.result = {"subathon.storage.synthetic-result.v1", QByteArray("milestone-following")};
    return bundle;
}

class File {
  public:
    enum class Schema { v11, literal_v12, empty };
    enum class Fault { none, refuse_release_write, refuse_receipt_write };

    explicit File(Schema schema = Schema::literal_v12, Fault fault = Fault::none) {
        require(dir_.isValid(), "Disposable SQLite store directory");
        reopen();
        if (schema == Schema::empty)
            return;
        auto catalog = delivery_jobs_repository_manifest();
        migrated(permission_graph_fixture::migrate(*writer, catalog), 11, 11);
        if (schema == Schema::literal_v12) {
            auto statements = ddl;
            // Fault cases reject a real write with a SQLite constraint.
            if (fault == Fault::refuse_release_write)
                statements[0].replace("released IN (0,1)", "released=0");
            if (fault == Fault::refuse_receipt_write)
                statements[2].replace("length(effect_ref) BETWEEN 1 AND 4096",
                                      "length(effect_ref)=99");
            catalog.steps.append(literal_step(catalog, std::move(statements)));
            catalog.supported_starting_versions.append(12);
            migrated(permission_graph_fixture::migrate(*writer, std::move(catalog)), 12, 1);
        }
        const app::Data data;
        bundle = app::prepared(data.case_data("add10")["expected_record"]);
        following = following_bundle(data, bundle.after);
        (void)committed(*writer, [&](const auto &s) {
            data.retain_sources(s);
            data.populate_case(s, "add10");
            // Retain the independent source rule and request, leaving the
            // first application's state and singleton head in place.
            const auto plan_setup = data.case_data("negative_stop")["setup"].toArray();
            app::execute(s, QJsonArray{plan_setup.at(0), plan_setup.at(1)});
            return true;
        });
    }

    void reopen() {
        writer.reset();
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, app::limits()) == StorageError::none,
                "Fresh writer opens the same disposable SQLite file");
    }

    void seed_origin(const SqliteWorkSession &s) const { stage_origin(s, bundle); }

    void seed_following(const SqliteWorkSession &s, std::string action_version = "v8") const {
        const std::vector<DeliveryCapture> delivery{
            {"job", "synthetic/channel", DeliverySenderSelection::explicit_identity, "sender",
             "announce", std::move(action_version)},
        };
        require(SqliteDeliveryJobStore(s).stage(following, delivery).disposition ==
                    SqliteBundleStageDisposition::staged,
                "Second originating application stages the same job ref under its own operation");
    }

    void record(int first_released = 0) {
        (void)committed(*writer, [&](const auto &s) {
            seed_origin(s);
            seed_head(s, first_released);
            return true;
        });
        (void)committed(*writer, [&](const auto &s) {
            seed_following(s);
            seed_suffix(s);
            return true;
        });
    }

    SqlRows reads(const QString &sql, QList<QVariant> bindings = {}) const {
        SqliteReader reader;
        require(reader.open(path) == StorageError::none, "Open independent read-only observer");
        auto result = reader.read({sql, std::move(bindings)});
        require(result.error == StorageError::none, "Read durable rows: " + sql);
        return std::move(result.rows);
    }

    PreparedApplicationBundle bundle;
    PreparedApplicationBundle following;

  private:
    QTemporaryDir dir_;

  public:
    const QString path{dir_.filePath("milestones.sqlite")};
    std::unique_ptr<SqliteWriter> writer;
};

} // namespace milestone_group_restart_fixture
