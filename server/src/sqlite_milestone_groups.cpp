// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_milestone_groups.hpp>

#include <QMetaType>
#include <algorithm>
#include <limits>
#include <string_view>
#include <subathon/server/sqlite_delivery_catalog.hpp>
#include <tuple>

namespace subathon::server {
namespace {

using Kind = MilestoneActionKind;
using Milliseconds = std::chrono::milliseconds;
using Bonus = MilestoneBonusDisposition;
using Release = MilestoneReleaseDisposition;

constexpr int automation = 0;
constexpr int bonus = 1;

[[noreturn]] void invalid() { throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_job); }

[[noreturn]] void corrupt() {
    throw SqliteBundleStorageFailure(SqliteBundleStorageReason::invalid_stored_data);
}

template <typename Work> auto guarded(const SqliteWorkSession &session, Work work) {
    try {
        return work();
    } catch (...) {
        // A caller catching an input or port exception must not be able to
        // commit any of the provisional originating work or bonus effect.
        const auto error = session.cleanup();
        if (error != WorkError::none)
            throw SqliteWorkFailure(error);
        throw;
    }
}

bool bounded_ref(std::string_view ref) { return !ref.empty() && ref.size() <= 4096; }

QByteArray blob(std::string_view ref) {
    if (!bounded_ref(ref))
        invalid();
    return QByteArray(ref.data(), qsizetype(ref.size()));
}

QString ref_column(const QString &column) {
    return "CASE WHEN typeof(" + column + ")='blob' AND length(" + column +
           ") BETWEEN 1 AND 4096 THEN " + column + " END";
}

QString integer_column(const QString &column) {
    return "CASE WHEN typeof(" + column + ")='integer' THEN " + column + " END";
}

QString timing_column(const QString &column) {
    return "CASE WHEN " + column + " IS NULL OR typeof(" + column + ")='integer' THEN " + column +
           " ELSE -1 END";
}

std::string reference(const QVariant &cell) {
    if (cell.isNull() || cell.metaType().id() != QMetaType::QByteArray)
        corrupt();
    return cell.toByteArray().toStdString();
}

qint64 integer(const QVariant &cell) {
    const auto type = cell.metaType().id();
    if (cell.isNull() || (type != QMetaType::Int && type != QMetaType::LongLong))
        corrupt();
    return cell.toLongLong();
}

qint64 scalar(const SqlRows &rows) {
    if (rows.size() != 1)
        corrupt();
    return integer(rows.front().front());
}

std::optional<Milliseconds> timing(const QVariant &cell, qint64 minimum) {
    if (cell.isNull())
        return std::nullopt;
    const auto value = integer(cell);
    if (value < minimum)
        corrupt();
    return Milliseconds{value};
}

QVariant bound_timing(const std::optional<Milliseconds> &value, qint64 minimum) {
    if (!value)
        return {};
    if (value->count() < minimum)
        invalid();
    return qint64(value->count());
}

constexpr auto capture_match =
    "c.operation_scope=a.operation_scope AND c.operation_ref=a.operation_ref "
    "AND c.job_ref=a.job_ref AND c.action_ref=a.action_ref AND c.action_version_ref=a.version_ref";

void validate_membership(const ApplicationOperationKey &origin,
                         const CapturedMilestoneGroup &capture) {
    const auto &actions = capture.group.actions;
    if (actions.size() > std::size_t(std::numeric_limits<qint64>::max()) ||
        capture.jobs.size() > actions.size())
        invalid();

    std::size_t automation_count = 0;
    for (const auto &action : actions) {
        if (!bounded_ref(action.action_ref) || !bounded_ref(action.version_ref))
            invalid();
        switch (action.kind) {
        case Kind::automation:
            ++automation_count;
            break;
        case Kind::bonus:
            break;
        default:
            invalid();
        }
    }
    if (automation_count != capture.jobs.size())
        invalid();

    for (const auto &member : capture.jobs) {
        if (member.action_ordinal < 1 || std::uint64_t(member.action_ordinal) > actions.size() ||
            member.job.operation.operation_scope != origin.operation_scope ||
            member.job.operation.operation_ref != origin.operation_ref ||
            !bounded_ref(member.job.job_ref) ||
            actions[std::size_t(member.action_ordinal - 1)].kind != Kind::automation)
            invalid();
    }
}

void insert_group(const SqliteWorkSession &session, const CapturedMilestoneGroup &capture,
                  const QByteArray &scope, const QByteArray &operation, qint64 order) {
    const auto &group = capture.group;
    if (!group.occurrence)
        invalid();
    const auto occurrence = blob(group.occurrence->ref);
    const auto &achievement = group.achievement;
    (void)session.execute(
        {"INSERT INTO main.subathon_milestone_groups("
         "occurrence_ref,operation_scope,operation_ref,queue_order,milestone_ref,"
         "definition_revision_ref,target_micro_points,score_micro_points,completion_context_ref,"
         "presentation_ms,gap_ms,released) VALUES(?,?,?,?,?,?,?,?,?,?,?,0)",
         {occurrence, scope, operation, order, blob(achievement.milestone_ref),
          blob(achievement.definition_revision_ref), qint64(achievement.target_micro_points),
          qint64(achievement.score_micro_points), blob(achievement.completion_context_ref),
          bound_timing(group.timing.presentation, 1), bound_timing(group.timing.gap, 0)}});

    for (std::size_t i = 0; i < group.actions.size(); ++i) {
        const auto &action = group.actions[i];
        if (action.kind == Kind::bonus)
            (void)session.execute(
                {"INSERT INTO main.subathon_milestone_actions("
                 "occurrence_ref,action_ordinal,kind,action_ref,version_ref) VALUES(?,?,1,?,?)",
                 {occurrence, qint64(i) + 1, blob(action.action_ref), blob(action.version_ref)}});
    }
    // The position PK and delivery-key uniqueness reject duplicates. Together
    // with the validated member count, this requires one job per automation.
    for (const auto &member : capture.jobs) {
        const auto &action = group.actions[std::size_t(member.action_ordinal - 1)];
        (void)session.execute(
            {"INSERT INTO main.subathon_milestone_actions("
             "occurrence_ref,action_ordinal,kind,action_ref,version_ref,"
             "operation_scope,operation_ref,job_ref) "
             "SELECT ?,?,0,c.action_ref,c.action_version_ref,c.operation_scope,"
             "c.operation_ref,c.job_ref FROM main.subathon_delivery_captures AS c "
             "WHERE c.operation_scope=? AND c.operation_ref=? AND c.job_ref=? "
             "AND c.action_ref=? AND c.action_version_ref=?",
             {occurrence, qint64(member.action_ordinal), scope, operation, blob(member.job.job_ref),
              blob(action.action_ref), blob(action.version_ref)}});
        if (scalar(session.execute({"SELECT changes()", {}})) != 1)
            invalid();
    }
}

} // namespace

MigrationManifest milestone_groups_repository_manifest() {
    auto manifest = delivery_jobs_repository_manifest();
    MigrationStep step{12, "milestone-groups-v1", {}, manifest.steps.back().expected_schema};
    step.sql = {
        "CREATE TABLE subathon_milestone_groups("
        "occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND "
        "length(occurrence_ref) "
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
        "presentation_ms INTEGER CHECK(presentation_ms IS NULL OR "
        "(typeof(presentation_ms)='integer' "
        "AND presentation_ms>0)), "
        "gap_ms INTEGER CHECK(gap_ms IS NULL OR (typeof(gap_ms)='integer' AND gap_ms>=0)), "
        "released INTEGER NOT NULL CHECK(typeof(released)='integer' AND released IN (0,1)), "
        "PRIMARY KEY(occurrence_ref), UNIQUE(queue_order), FOREIGN "
        "KEY(operation_scope,operation_ref) "
        "REFERENCES subathon_app_operations(operation_scope,operation_ref))",
        "CREATE TABLE subathon_milestone_actions("
        "occurrence_ref BLOB NOT NULL, action_ordinal INTEGER NOT NULL "
        "CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1), "
        "kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind IN (0,1)), "
        "action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob' AND length(action_ref) BETWEEN 1 "
        "AND "
        "4096), "
        "version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref) "
        "BETWEEN 1 "
        "AND 4096), "
        "operation_scope BLOB, operation_ref BLOB, job_ref BLOB, "
        "CHECK((kind=0 AND operation_scope IS NOT NULL AND operation_ref IS NOT NULL AND job_ref "
        "IS "
        "NOT NULL) OR "
        "(kind=1 AND operation_scope IS NULL AND operation_ref IS NULL AND job_ref IS NULL)), "
        "PRIMARY KEY(occurrence_ref,action_ordinal), "
        "UNIQUE(operation_scope,operation_ref,job_ref), "
        "FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref), "
        "FOREIGN KEY(operation_scope,operation_ref,job_ref) REFERENCES "
        "subathon_delivery_captures(operation_scope,operation_ref,job_ref))",
        "CREATE TABLE subathon_milestone_bonus_receipts("
        "occurrence_ref BLOB NOT NULL, action_ordinal INTEGER NOT NULL, "
        "effect_ref BLOB NOT NULL CHECK(typeof(effect_ref)='blob' AND length(effect_ref) BETWEEN 1 "
        "AND "
        "4096), "
        "PRIMARY KEY(occurrence_ref,action_ordinal), FOREIGN KEY(occurrence_ref,action_ordinal) "
        "REFERENCES subathon_milestone_actions(occurrence_ref,action_ordinal))",
        "CREATE INDEX subathon_milestone_pending_order ON subathon_milestone_groups(queue_order) "
        "WHERE "
        "released=0",
    };
    const QStringList tables{"subathon_milestone_groups", "subathon_milestone_actions",
                             "subathon_milestone_bonus_receipts"};
    for (qsizetype i = 0; i < tables.size(); ++i) {
        const auto &table = tables[i];
        const auto autoindex = "sqlite_autoindex_" + table;
        step.expected_schema.append({"table", table, table, step.sql[i]});
        step.expected_schema.append({"index", autoindex + "_1", table, std::nullopt});
        if (i < 2)
            step.expected_schema.append({"index", autoindex + "_2", table, std::nullopt});
    }
    step.expected_schema.append(
        {"index", "subathon_milestone_pending_order", tables.front(), step.sql.back()});
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &left, const auto &right) {
                  return std::tie(left.type, left.name) < std::tie(right.type, right.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(12);
    return manifest;
}

void SqliteMilestoneGroupStore::stage(const ApplicationOperationKey &origin,
                                      const std::vector<CapturedMilestoneGroup> &groups) const {
    guarded(session_, [&] {
        const auto scope = blob(origin.operation_scope);
        const auto operation = blob(origin.operation_ref);
        if (groups.empty())
            return;
        for (const auto &capture : groups)
            validate_membership(origin, capture);
        auto order = scalar(
            session_.execute({"SELECT CASE WHEN max(queue_order) IS NULL THEN 0 "
                              "WHEN typeof(max(queue_order))='integer' THEN max(queue_order) END "
                              "FROM main.subathon_milestone_groups",
                              {}}));
        if (order < 0)
            corrupt();
        if (groups.size() > std::uint64_t(std::numeric_limits<qint64>::max() - order))
            throw SqliteBundleStorageFailure(SqliteBundleStorageReason::sequence_exhausted);
        for (const auto &capture : groups)
            insert_group(session_, capture, scope, operation, ++order);
    });
}

std::vector<CapturedMilestoneGroup> SqliteMilestoneGroupStore::pending() const {
    return guarded(session_, [&] {
        // Read groups independently of action rows so empty occurrences are
        // retained and each captured group definition is materialized once.
        const auto groups = session_.execute(
            {"SELECT " + integer_column("queue_order") + "," + ref_column("occurrence_ref") + "," +
                 ref_column("milestone_ref") + "," + ref_column("definition_revision_ref") + "," +
                 integer_column("target_micro_points") + "," +
                 integer_column("score_micro_points") + "," + ref_column("completion_context_ref") +
                 "," + timing_column("presentation_ms") + "," + timing_column("gap_ms") +
                 " FROM main.subathon_milestone_groups "
                 "WHERE released=0 ORDER BY queue_order",
             {}});
        const auto actions = session_.execute(
            {"SELECT " + integer_column("g.queue_order") + "," +
                 integer_column("a.action_ordinal") + "," + integer_column("a.kind") + "," +
                 ref_column("a.action_ref") + "," + ref_column("a.version_ref") + "," +
                 ref_column("a.operation_scope") + "," + ref_column("a.operation_ref") + "," +
                 ref_column("a.job_ref") +
                 ",CASE WHEN a.kind=0 THEN "
                 "a.operation_scope=g.operation_scope AND a.operation_ref=g.operation_ref "
                 "AND EXISTS(SELECT 1 FROM main.subathon_delivery_captures AS c WHERE " +
                 capture_match +
                 ") ELSE a.operation_scope IS NULL AND a.operation_ref IS NULL "
                 "AND a.job_ref IS NULL END "
                 "FROM main.subathon_milestone_groups AS g JOIN main.subathon_milestone_actions AS "
                 "a "
                 "ON a.occurrence_ref=g.occurrence_ref WHERE g.released=0 "
                 "ORDER BY g.queue_order,a.action_ordinal",
             {}});

        std::vector<CapturedMilestoneGroup> result;
        result.reserve(std::size_t(groups.size()));
        auto action = actions.cbegin();
        qint64 previous_order = 0;
        for (const auto &row : groups) {
            const auto order = integer(row[0]);
            if (order <= previous_order)
                corrupt();
            previous_order = order;
            CapturedMilestoneGroup capture{{{reference(row[2]), reference(row[3]), integer(row[4]),
                                             integer(row[5]), reference(row[6])},
                                            {},
                                            {timing(row[7], 1), timing(row[8], 0)},
                                            MilestoneOccurrenceKey{reference(row[1])}},
                                           {}};
            for (; action != actions.cend() && integer((*action)[0]) == order; ++action) {
                const auto &cells = *action;
                const auto ordinal = integer(cells[1]);
                const auto kind = integer(cells[2]);
                if (ordinal < 1 || std::uint64_t(ordinal - 1) != capture.group.actions.size() ||
                    (kind != automation && kind != bonus) || integer(cells[8]) != 1)
                    corrupt();
                capture.group.actions.push_back(
                    {kind == automation ? Kind::automation : Kind::bonus, reference(cells[3]),
                     reference(cells[4])});
                if (kind == automation)
                    capture.jobs.push_back(
                        {ordinal,
                         {{reference(cells[5]), reference(cells[6])}, reference(cells[7])}});
            }
            result.push_back(std::move(capture));
        }
        if (action != actions.cend())
            corrupt();
        return result;
    });
}

MilestoneBonusDisposition
SqliteMilestoneGroupStore::apply_bonus(const MilestoneBonusKey &key,
                                       MilestoneBonusAuthority &authority,
                                       MilestoneBonusEffect &effect) const {
    if (!bounded_ref(key.occurrence.ref) || key.action_ordinal < 1)
        return Bonus::refused;
    return guarded(session_, [&] {
        QList<QVariant> args{blob(key.occurrence.ref), qint64(key.action_ordinal)};
        const auto rows = session_.execute(
            {"SELECT " + ref_column("a.action_ref") + "," + ref_column("a.version_ref") +
                 ",r.action_ordinal IS NOT NULL," + ref_column("r.effect_ref") +
                 " FROM main.subathon_milestone_actions AS a "
                 "JOIN main.subathon_milestone_groups AS g ON g.occurrence_ref=a.occurrence_ref "
                 "LEFT JOIN main.subathon_milestone_bonus_receipts AS r "
                 "ON r.occurrence_ref=a.occurrence_ref AND r.action_ordinal=a.action_ordinal "
                 "WHERE a.occurrence_ref=? AND a.action_ordinal=? "
                 "AND typeof(g.released)='integer' AND g.released=1 "
                 "AND typeof(a.kind)='integer' AND a.kind=1 "
                 "AND a.operation_scope IS NULL AND a.operation_ref IS NULL AND a.job_ref IS NULL",
             args});
        if (rows.empty())
            return Bonus::refused;
        if (rows.size() != 1)
            corrupt();
        const auto &row = rows.front();
        const MilestoneAction action{Kind::bonus, reference(row[0]), reference(row[1])};
        if (integer(row[2]) == 1) {
            (void)reference(row[3]);
            return Bonus::already_applied;
        }
        if (!authority.allows(key, action))
            return Bonus::refused;
        args.append(blob(effect.apply(session_, key, action)));
        (void)session_.execute({"INSERT INTO main.subathon_milestone_bonus_receipts"
                                "(occurrence_ref,action_ordinal,effect_ref) VALUES(?,?,?)",
                                std::move(args)});
        return Bonus::applied;
    });
}

StorageError release_milestone_head(SqliteWriter &writer, MilestoneOccurrenceKey key,
                                    MilestoneReleaseCallback completion) {
    if (!bounded_ref(key.ref))
        return StorageError::invalid_options;
    if (!completion)
        return StorageError::invalid_work;
    return writer.enqueue_work(
        [key = std::move(key)](SqliteWorkContext context) -> QVariant {
            const auto session = context.begin();
            const auto head =
                session.execute({"SELECT " + ref_column("occurrence_ref") +
                                     " FROM main.subathon_milestone_groups WHERE released=0 "
                                     "ORDER BY queue_order LIMIT 1",
                                 {}});
            auto disposition = Release::refused;
            if (!head.empty() && reference(head.front().front()) == key.ref) {
                (void)session.execute({"UPDATE main.subathon_milestone_groups SET released=1 "
                                       "WHERE occurrence_ref=? AND released=0",
                                       {blob(key.ref)}});
                if (scalar(session.execute({"SELECT changes()", {}})) != 1)
                    corrupt();
                disposition = Release::released;
            }
            QVariant value{int(disposition)};
            session.commit();
            return value;
        },
        [completion = std::move(completion)](SqliteWorkResult work) {
            std::optional<Release> disposition;
            if (work.transaction == TransactionOutcome::committed &&
                work.error == WorkError::none && work.value)
                disposition = Release(work.value->toInt());
            completion({std::move(work), disposition});
        });
}

} // namespace subathon::server
