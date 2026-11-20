// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_milestone_groups.hpp>

#include "sqlite_milestone_edit_detail.hpp"

#include <algorithm>
#include <limits>
#include <subathon/server/sqlite_delivery_catalog.hpp>
#include <tuple>

namespace subathon::server {
namespace ed = milestone_edit_sql;
namespace {

using namespace milestone_sql;

using Kind = MilestoneActionKind;
using Milliseconds = std::chrono::milliseconds;
using Bonus = MilestoneBonusDisposition;
using Release = MilestoneReleaseDisposition;

constexpr int automation = 0;
constexpr int bonus = 1;

QString timing_column(const QString &column) {
    return "CASE WHEN " + column + " IS NULL OR typeof(" + column + ")='integer' THEN " + column +
           " ELSE -1 END";
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

namespace milestone_edit_sql {
namespace {

constexpr auto edit_skipped =
    "(EXISTS(SELECT 1 FROM main.subathon_milestone_edit_choices d WHERE "
    "d.occurrence_ref=g.occurrence_ref AND (d.choice=2 OR d.resolution=1)) OR "
    "EXISTS(SELECT 1 FROM main.subathon_milestone_edit_refund_decisions d WHERE "
    "d.occurrence_ref=g.occurrence_ref AND (d.policy=2 OR d.resolution=1)))";

} // namespace

qint64 queue_tail(const SqliteWorkSession &s) {
    const auto values = s.execute(
        {"SELECT max(coalesce((SELECT max(queue_order) FROM main.subathon_milestone_groups),0),"
         "coalesce((SELECT max(queue_order) FROM main.subathon_milestone_edit_occurrences),0))",
         {}});
    const auto tail = scalar(values);
    if (tail < 0)
        corrupt();
    return tail;
}

void check_application_identity(const SqliteWorkSession &s, const QByteArray &key, qint64 order) {
    if (order < 1 || !s.execute({"SELECT 1 FROM main.subathon_milestone_edit_occurrences "
                                 "WHERE occurrence_ref=? OR queue_order=? LIMIT 1",
                                 {key, order}})
                          .empty())
        corrupt();
}

std::optional<Occurrence> occurrence(const SqliteWorkSession &s, const QByteArray &key,
                                     bool actions, const MilestoneEditKey *expected) {
    QList<QVariant> args{key};
    QString scope;
    if (expected) {
        scope = " AND (g.installation_ref,g.track_ref,g.edit_ref)=(?,?,?)";
        args.append({blob(expected->track.installation_ref), blob(expected->track.track_ref),
                     blob(expected->operation_ref)});
    }
    const auto rows = s.execute(
        {"SELECT " + ref_column("g.installation_ref") + "," + ref_column("g.track_ref") + "," +
             ref_column("g.edit_ref") + ",g.queue_order," + ref_column("g.milestone_ref") + "," +
             ref_column("g.definition_revision") + ",g.target_micro_points,g.score_micro_points," +
             ref_column("g.completion_context_ref") + "," + timing_column("g.presentation_ms") +
             "," + timing_column("g.gap_ms") + ",g.released,c.choice," + ref_column("c.actor_ref") +
             "," + ref_column("c.decision_ref") +
             ",CASE WHEN (c.installation_ref,c.track_ref,c.edit_ref,c.actor_ref,c.decision_ref)="
             "(o.installation_ref,o.track_ref,o.edit_ref,o.actor_ref,o.decision_ref) "
             "AND g.definition_revision=o.resulting_revision AND "
             "g.score_micro_points=o.score_micro_points "
             "AND g.score_micro_points>=g.target_micro_points AND "
             "g.completion_context_ref=o.edit_ref "
             "AND (c.choice=1 OR (c.resolution IS NULL AND c.ended_by_skip_kind IS NULL)) "
             "AND t.target_micro_points=g.target_micro_points "
             "AND NOT EXISTS(SELECT 1 FROM main.subathon_milestone_groups a "
             "WHERE a.occurrence_ref=g.occurrence_ref OR a.queue_order=g.queue_order) "
             "THEN 1 ELSE 0 END,g.suffix_ordinal,"
             "CASE WHEN typeof(o.result)='blob' AND length(o.result)<=8388608 THEN o.result END "
             "FROM main.subathon_milestone_edit_occurrences g "
             "LEFT JOIN main.subathon_milestone_edit_operations o "
             "USING(installation_ref,track_ref,edit_ref) "
             "LEFT JOIN main.subathon_milestone_definition_targets t "
             "ON (t.installation_ref,t.track_ref,t.definition_revision,t.milestone_ref)="
             "(g.installation_ref,g.track_ref,g.definition_revision,g.milestone_ref) "
             "LEFT JOIN main.subathon_milestone_edit_choices c ON "
             "c.occurrence_ref=g.occurrence_ref "
             "WHERE g.occurrence_ref=?" +
             scope,
         args});
    const auto *row = unique(rows);
    if (!row)
        return {};
    const auto order = integer((*row)[3]);
    const auto released = integer((*row)[11]);
    if (order < 1 || released < 0 || released > 1 || integer((*row)[15]) != 1)
        corrupt();
    Occurrence result{{{reference((*row)[0]), reference((*row)[1])}, reference((*row)[2])},
                      {{reference((*row)[4]), reference((*row)[5]), integer((*row)[6]),
                        integer((*row)[7]), reference((*row)[8])},
                       {},
                       {timing((*row)[9], 1), timing((*row)[10], 0)},
                       MilestoneOccurrenceKey{key.toStdString()}},
                      enumeration((*row)[12], MilestoneActionChoice::skip),
                      {reference((*row)[13]), reference((*row)[14])},
                      order,
                      released == 1};
    if (result.released && result.choice == MilestoneActionChoice::skip)
        corrupt();
    Decoding audit((*row)[17]);
    (void)audit.ref(); // Prior and resulting active prefix revisions.
    (void)audit.ref();
    if (audit.ref() != result.group.achievement.definition_revision_ref)
        corrupt();
    const auto count = audit.number();
    const auto suffix = integer((*row)[16]);
    if (suffix < 1 || count < suffix || count > collection_limit)
        corrupt();
    for (qint64 i = 1; i <= suffix; ++i) {
        const auto milestone = audit.ref();
        const auto revision = audit.ref();
        const auto target = audit.number();
        const auto score = audit.number();
        const auto context = audit.ref();
        const auto ref = audit.ref();
        const auto order_in_audit = audit.number();
        const auto choice = audit.number();
        const auto &completion = result.group.achievement;
        if (i == suffix &&
            (milestone != completion.milestone_ref ||
             revision != completion.definition_revision_ref ||
             target != completion.target_micro_points || score != completion.score_micro_points ||
             context != completion.completion_context_ref || ref != key.toStdString() ||
             order_in_audit != result.order || choice != int(result.choice)))
            corrupt();
    }
    if (actions) {
        const auto positions =
            s.execute({"SELECT action_ordinal,kind," + ref_column("action_ref") + "," +
                           ref_column("version_ref") +
                           " FROM main.subathon_milestone_edit_actions WHERE occurrence_ref=? "
                           "ORDER BY action_ordinal LIMIT ?",
                       {key, collection_limit + 1}});
        if (positions.size() > collection_limit)
            throw SqliteBundleStorageFailure(SqliteBundleStorageReason::result_limit);
        for (const auto &position : positions) {
            const auto ordinal = integer(position[0]);
            if (ordinal != qint64(result.group.actions.size()) + 1)
                corrupt();
            const auto kind = enumeration(position[1], Kind::bonus);
            (void)job(s, result, ordinal, false);
            result.group.actions.push_back({kind, reference(position[2]), reference(position[3])});
        }
        const auto extras = s.execute(
            {"SELECT 1 FROM main.subathon_milestone_edit_jobs j WHERE j.occurrence_ref=? "
             "AND NOT EXISTS(SELECT 1 FROM main.subathon_milestone_edit_actions a "
             "WHERE (a.occurrence_ref,a.action_ordinal)=(j.occurrence_ref,j.action_ordinal)) LIMIT "
             "1",
             {key}});
        if (!extras.empty())
            corrupt();
    }
    return result;
}

std::optional<std::pair<QByteArray, bool>> pending_head(const SqliteWorkSession &s) {
    const auto rows = s.execute(
        {"SELECT " + ref_column("occurrence_ref") +
             ",queue_order,edit FROM (SELECT g.occurrence_ref,g.queue_order,0 AS edit "
             "FROM main.subathon_milestone_groups g WHERE " +
             pending_filter(Catalog::edits) +
             " UNION ALL SELECT g.occurrence_ref,g.queue_order,1 "
             "FROM main.subathon_milestone_edit_occurrences g WHERE g.released=0 AND NOT " +
             edit_skipped + ") ORDER BY queue_order LIMIT 1",
         {}});
    const auto *row = unique(rows);
    if (!row)
        return {};
    const auto key = blob(reference((*row)[0]));
    const auto edit = integer((*row)[2]) == 1;
    if (edit) {
        if (!occurrence(s, key, true))
            corrupt();
    } else {
        check_application_identity(s, key, integer((*row)[1]));
    }
    return std::pair{key, edit};
}

std::vector<std::pair<qint64, CapturedMilestoneGroup>> pending(const SqliteWorkSession &s) {
    const auto rows = s.execute(
        {"SELECT " + ref_column("g.occurrence_ref") +
             " FROM main.subathon_milestone_edit_occurrences g WHERE g.released=0 AND NOT " +
             edit_skipped + " ORDER BY g.queue_order LIMIT ?",
         {collection_limit + 1}});
    if (rows.size() > collection_limit)
        throw SqliteBundleStorageFailure(SqliteBundleStorageReason::result_limit);
    std::vector<std::pair<qint64, CapturedMilestoneGroup>> out;
    std::size_t action_count = 0;
    std::size_t bytes = 0;
    for (const auto &row : rows) {
        const auto key = reference(row[0]);
        auto selected = occurrence(s, blob(key), true).value();
        const auto &group = selected.group;
        const auto &achievement = group.achievement;
        action_count += group.actions.size();
        bytes += key.size() + achievement.milestone_ref.size() +
                 achievement.definition_revision_ref.size() +
                 achievement.completion_context_ref.size();
        for (const auto &action : group.actions)
            bytes += action.action_ref.size() + action.version_ref.size();
        if (action_count > std::size_t(collection_limit) || bytes > std::size_t(document_limit))
            throw SqliteBundleStorageFailure(SqliteBundleStorageReason::result_limit);
        out.emplace_back(selected.order, CapturedMilestoneGroup{std::move(selected.group), {}});
    }
    return out;
}

std::optional<qint64> current_target(const SqliteWorkSession &s, const MilestoneOccurrenceKey &key,
                                     const std::string &installation) {
    using Pending = MilestonePendingDisposition;
    const auto state = SqliteMilestoneGroupDecisions(s).pending_disposition(key);
    if (state == Pending::absent_or_released || state == Pending::skipped)
        return {};
    const auto selected = occurrence(s, blob(key.ref));
    SqlRows targets;
    if (selected) {
        if (selected->edit.track.installation_ref != installation)
            corrupt();
        targets = s.execute(
            {"SELECT t.target_micro_points FROM main.subathon_milestone_definition_heads h "
             "JOIN main.subathon_milestone_definition_targets t "
             "USING(installation_ref,track_ref,definition_revision) "
             "WHERE h.installation_ref=? AND h.track_ref=? AND t.milestone_ref=? LIMIT 2",
             {blob(selected->edit.track.installation_ref), blob(selected->edit.track.track_ref),
              blob(selected->group.achievement.milestone_ref)}});
    } else {
        targets = s.execute(
            {"SELECT t.target_micro_points FROM main.subathon_milestone_groups g "
             "JOIN main.subathon_milestone_prefix_members m "
             "ON m.origin=0 AND m.application_occurrence_ref=g.occurrence_ref "
             "AND (m.operation_scope,m.operation_ref,m.milestone_ref,m.definition_revision,"
             "m.target_micro_points,m.score_micro_points,m.completion_context_ref)="
             "(g.operation_scope,g.operation_ref,g.milestone_ref,g.definition_revision_ref,"
             "g.target_micro_points,g.score_micro_points,g.completion_context_ref) "
             "JOIN main.subathon_milestone_prefix_heads p "
             "USING(installation_ref,track_ref,prefix_revision) "
             "JOIN main.subathon_milestone_definition_heads h USING(installation_ref,track_ref) "
             "JOIN main.subathon_milestone_definition_targets t "
             "ON (t.installation_ref,t.track_ref,t.definition_revision,t.milestone_ref)="
             "(h.installation_ref,h.track_ref,h.definition_revision,g.milestone_ref) "
             "WHERE g.occurrence_ref=? AND p.installation_ref=? LIMIT 2",
             {blob(key.ref), blob(installation)}});
    }
    const auto *row = unique(targets);
    if (!row)
        corrupt();
    return integer(row->front());
}

std::int64_t total_score(const ApplicationStateSnapshot &state) {
    core::Exact total = 0;
    for (const auto &balance : state.balances)
        if (balance.kind == ApplicationBalanceKind::score)
            total += balance.units;
    if (total < std::numeric_limits<std::int64_t>::min() ||
        total > std::numeric_limits<std::int64_t>::max())
        throw std::overflow_error("Aggregate score exceeds signed 64-bit range");
    return total.convert_to<std::int64_t>();
}

} // namespace milestone_edit_sql

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
        const auto edits = catalog(session_) == Catalog::edits;
        auto order = edits
                         ? ed::queue_tail(session_)
                         : scalar(session_.execute(
                               {"SELECT CASE WHEN max(queue_order) IS NULL THEN 0 "
                                "WHEN typeof(max(queue_order))='integer' THEN max(queue_order) END "
                                "FROM main.subathon_milestone_groups",
                                {}}));
        if (order < 0)
            corrupt();
        if (groups.size() > std::uint64_t(std::numeric_limits<qint64>::max() - order))
            throw SqliteBundleStorageFailure(SqliteBundleStorageReason::sequence_exhausted);
        for (const auto &capture : groups) {
            ++order;
            if (edits && capture.group.occurrence)
                ed::check_application_identity(session_, blob(capture.group.occurrence->ref),
                                               order);
            insert_group(session_, capture, scope, operation, order);
        }
    });
}

std::vector<CapturedMilestoneGroup> SqliteMilestoneGroupStore::pending() const {
    return guarded(session_, [&] {
        const auto current = catalog(session_);
        const auto filter = pending_filter(current);
        // Read groups independently of action rows so empty occurrences are
        // retained and each captured group definition is materialized once.
        const auto groups = session_.execute(
            {"SELECT " + integer_column("queue_order") + "," + ref_column("occurrence_ref") + "," +
                 ref_column("milestone_ref") + "," + ref_column("definition_revision_ref") + "," +
                 integer_column("target_micro_points") + "," +
                 integer_column("score_micro_points") + "," + ref_column("completion_context_ref") +
                 "," + timing_column("presentation_ms") + "," + timing_column("gap_ms") +
                 " FROM main.subathon_milestone_groups AS g WHERE " + filter +
                 " ORDER BY g.queue_order",
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
                 "ON a.occurrence_ref=g.occurrence_ref WHERE " +
                 filter + " ORDER BY g.queue_order,a.action_ordinal",
             {}});

        std::vector<CapturedMilestoneGroup> result;
        result.reserve(std::size_t(groups.size()));
        auto action = actions.cbegin();
        qint64 previous_order = 0;
        auto edits = current == Catalog::edits
                         ? ed::pending(session_)
                         : std::vector<std::pair<qint64, CapturedMilestoneGroup>>{};
        auto edit = edits.begin();
        for (const auto &row : groups) {
            const auto order = integer(row[0]);
            if (order <= previous_order)
                corrupt();
            previous_order = order;
            if (current == Catalog::edits)
                ed::check_application_identity(session_, blob(reference(row[1])), order);
            for (; edit != edits.end() && edit->first < order; ++edit)
                result.push_back(std::move(edit->second));
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
        for (; edit != edits.end(); ++edit)
            result.push_back(std::move(edit->second));
        if (current == Catalog::edits && result.size() > std::size_t(ed::collection_limit))
            throw SqliteBundleStorageFailure(SqliteBundleStorageReason::result_limit);
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
        if (catalog(session_) == Catalog::edits) {
            if (auto edit = ed::occurrence(session_, args.front().toByteArray(), true)) {
                if (!edit->released ||
                    std::uint64_t(key.action_ordinal) > edit->group.actions.size())
                    return Bonus::refused;
                const auto &action = edit->group.actions[std::size_t(key.action_ordinal - 1)];
                if (action.kind != Kind::bonus)
                    return Bonus::refused;
                const auto prior =
                    session_.execute({"SELECT " + ref_column("effect_ref") +
                                          " FROM main.subathon_milestone_edit_bonus_receipts "
                                          "WHERE occurrence_ref=? AND action_ordinal=? AND kind=1",
                                      args});
                if (const auto *row = ed::unique(prior)) {
                    (void)reference(row->front());
                    return Bonus::already_applied;
                }
                if (!authority.allows(key, action))
                    return Bonus::refused;
                args.append(blob(effect.apply(session_, key, action)));
                (void)session_.execute(
                    {"INSERT INTO main.subathon_milestone_edit_bonus_receipts VALUES(?,?,1,?)",
                     args});
                return Bonus::applied;
            }
        }
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
            const auto current = catalog(session);
            if (current == Catalog::edits) {
                const auto head = ed::pending_head(session);
                auto status = Release::refused;
                if (head && !head->second && head->first == blob(key.ref) &&
                    SqliteMilestoneGroupDecisions(session).pending_disposition(key) ==
                        MilestonePendingDisposition::ready) {
                    (void)session.execute({"UPDATE main.subathon_milestone_groups SET released=1 "
                                           "WHERE occurrence_ref=? AND released=0",
                                           {head->first}});
                    if (scalar(session.execute({"SELECT changes()", {}})) != 1)
                        corrupt();
                    status = Release::released;
                }
                session.commit();
                return int(status);
            }
            const auto head =
                session.execute({"SELECT " + ref_column("g.occurrence_ref") + "," +
                                     (current == Catalog::decisions ? held() : QString("0")) +
                                     " FROM main.subathon_milestone_groups AS g WHERE " +
                                     pending_filter(current) + " ORDER BY g.queue_order LIMIT 1",
                                 {}});
            auto disposition = Release::refused;
            if (!head.empty() && reference(head.front()[0]) == key.ref &&
                integer(head.front()[1]) == 0) {
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
