// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/milestone_group_decisions.hpp>

#include "sqlite_milestone_detail.hpp"

#include <algorithm>
#include <set>
#include <tuple>

namespace subathon::server {
namespace {

using namespace milestone_sql;
using Choice = MilestoneActionChoice;
using Mode = PausedAcceptanceMode;
using Policy = MilestoneRefundPolicy;
using Resolution = MilestoneHoldResolution;
using Cause = MilestoneHoldCause;
using Disposition = MilestonePendingDisposition;
using SkipOrigin = MilestoneSkipOrigin;
using Occurrences = std::set<std::string_view>;

constexpr std::size_t list_limit = 4096;

bool valid_evidence(const MilestoneDecisionEvidence &evidence) {
    return bounded_ref(evidence.actor_ref) && bounded_ref(evidence.decision_ref);
}

bool valid_acceptance(const PausedMilestoneAcceptance &acceptance) {
    return (acceptance.mode == Mode::normal || acceptance.mode == Mode::silent) &&
           valid_evidence(acceptance.evidence) && acceptance.choices.size() <= list_limit;
}

bool exact_choices(const PausedMilestoneAcceptance &acceptance, Occurrences affected) {
    if (acceptance.choices.size() != affected.size())
        return false;
    return std::ranges::all_of(acceptance.choices, [&](const auto &entry) {
        return (entry.choice == Choice::run || entry.choice == Choice::defer ||
                entry.choice == Choice::skip) &&
               affected.erase(entry.occurrence.ref) == 1;
    });
}

const SqlRow *unique_row(const SqlRows &rows) {
    if (rows.size() > 1)
        corrupt();
    return rows.empty() ? nullptr : &rows.front();
}

// Use exact stored bytes and a validated enum range when decoding an audit.
template <typename Enum> Enum read_enum(const QVariant &value, Enum upper) {
    const auto raw = integer(value);
    if (raw < 0 || raw > qint64(upper))
        corrupt();
    return Enum(raw);
}

Disposition disposition(const SqliteWorkSession &session, const QByteArray &occurrence) {
    const auto rows =
        session.execute({"SELECT g.released," + QString(skipped) + "," + held() +
                             " FROM main.subathon_milestone_groups AS g WHERE g.occurrence_ref=?",
                         {occurrence}});
    const auto *row = unique_row(rows);
    if (!row)
        return Disposition::absent_or_released;
    const auto released = integer((*row)[0]);
    if (released == 1)
        return Disposition::absent_or_released;
    if (released != 0)
        corrupt();
    if (integer((*row)[1]) == 1)
        return Disposition::skipped;
    return integer((*row)[2]) == 1 ? Disposition::held : Disposition::ready;
}

void end_other_holds(const SqliteWorkSession &session, const QByteArray &occurrence,
                     SkipOrigin origin, const QByteArray &source) {
    // The caller has just recorded the actual Skip. The active predicate
    // excludes its selected resolution and every earlier ended/resolved hold.
    const QList<QVariant> args{int(origin), source, occurrence};
    (void)session.execute(
        {QString("UPDATE main.subathon_milestone_acceptance_decisions "
                 "SET ended_by_skip_kind=?,ended_by_skip_ref=? WHERE occurrence_ref=? "
                 "AND choice=1 AND ") +
             active,
         args});
    (void)session.execute(
        {QString("UPDATE main.subathon_milestone_refund_decisions "
                 "SET ended_by_skip_kind=?,ended_by_skip_ref=? WHERE occurrence_ref=? "
                 "AND policy=1 AND ") +
             active,
         args});
}

struct HoldHistory {
    std::optional<Resolution> resolution;
    std::optional<MilestoneDecisionEvidence> evidence;
    std::optional<MilestoneSkipProvenance> ended;
};

QString history_columns() {
    return "resolution," + ref_column("resolution_actor_ref") + "," + ref_column("resolution_ref") +
           ",ended_by_skip_kind," + ref_column("ended_by_skip_ref");
}

HoldHistory hold_history(const SqlRow &row, qsizetype start, bool can_hold) {
    HoldHistory result;
    if (!row[start].isNull()) {
        if (!can_hold || !row[start + 3].isNull() || !row[start + 4].isNull())
            corrupt();
        result.resolution = read_enum(row[start], Resolution::skip);
        result.evidence =
            MilestoneDecisionEvidence{reference(row[start + 1]), reference(row[start + 2])};
    } else if (!row[start + 1].isNull() || !row[start + 2].isNull()) {
        corrupt();
    }
    if (!row[start + 3].isNull()) {
        if (!can_hold)
            corrupt();
        result.ended = MilestoneSkipProvenance{read_enum(row[start + 3], SkipOrigin::refund_policy),
                                               reference(row[start + 4])};
    } else if (!row[start + 4].isNull()) {
        corrupt();
    }
    return result;
}

SqlRows refund_rows(const SqliteWorkSession &session, const QByteArray &occurrence,
                    const QByteArray &refund) {
    return session.execute(
        {"SELECT score_before_micro_points,score_after_micro_points,current_target_micro_points,"
         "policy," +
             history_columns() +
             " FROM main.subathon_milestone_refund_decisions WHERE occurrence_ref=? AND "
             "refund_ref=?",
         {occurrence, refund}});
}

} // namespace

MigrationManifest milestone_group_decisions_repository_manifest() {
    auto manifest = milestone_groups_repository_manifest();
    MigrationStep step{
        13, "milestone-group-decisions-v1", {}, manifest.steps.back().expected_schema};
    step.sql = {
        "CREATE TABLE subathon_milestone_acceptance_decisions(occurrence_ref BLOB NOT NULL, "
        "mode INTEGER NOT NULL CHECK(typeof(mode)='integer' AND mode IN (0,1)), choice "
        "INTEGER NOT NULL CHECK(typeof(choice)='integer' AND choice IN (0,1,2)), actor_ref "
        "BLOB NOT NULL CHECK(typeof(actor_ref)='blob' AND length(actor_ref) BETWEEN 1 AND "
        "4096), decision_ref BLOB NOT NULL CHECK(typeof(decision_ref)='blob' AND "
        "length(decision_ref) BETWEEN 1 AND 4096), resolution INTEGER, resolution_actor_ref "
        "BLOB, resolution_ref BLOB, ended_by_skip_kind INTEGER, ended_by_skip_ref BLOB, "
        "CHECK((resolution IS NULL AND resolution_actor_ref IS NULL AND resolution_ref IS "
        "NULL) OR (choice=1 AND typeof(resolution)='integer' AND resolution IN (0,1) AND "
        "typeof(resolution_actor_ref)='blob' AND length(resolution_actor_ref) BETWEEN 1 AND "
        "4096 AND typeof(resolution_ref)='blob' AND length(resolution_ref) BETWEEN 1 AND "
        "4096)), CHECK((ended_by_skip_kind IS NULL AND ended_by_skip_ref IS NULL) OR "
        "(choice=1 AND resolution IS NULL AND typeof(ended_by_skip_kind)='integer' AND "
        "ended_by_skip_kind IN (0,1,2) AND typeof(ended_by_skip_ref)='blob' AND "
        "length(ended_by_skip_ref) BETWEEN 1 AND 4096)), PRIMARY KEY(occurrence_ref), "
        "FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref))",
        "CREATE TABLE subathon_milestone_refund_decisions(occurrence_ref BLOB NOT NULL, "
        "refund_ref BLOB NOT NULL CHECK(typeof(refund_ref)='blob' AND length(refund_ref) "
        "BETWEEN 1 AND 4096), score_before_micro_points INTEGER NOT NULL "
        "CHECK(typeof(score_before_micro_points)='integer'), score_after_micro_points "
        "INTEGER NOT NULL CHECK(typeof(score_after_micro_points)='integer'), "
        "current_target_micro_points INTEGER NOT NULL "
        "CHECK(typeof(current_target_micro_points)='integer'), policy INTEGER NOT NULL "
        "CHECK(typeof(policy)='integer' AND policy IN (0,1,2)), resolution INTEGER, "
        "resolution_actor_ref BLOB, resolution_ref BLOB, ended_by_skip_kind INTEGER, "
        "ended_by_skip_ref BLOB, CHECK((resolution IS NULL AND resolution_actor_ref IS NULL "
        "AND resolution_ref IS NULL) OR (policy=1 AND typeof(resolution)='integer' AND "
        "resolution IN (0,1) AND typeof(resolution_actor_ref)='blob' AND "
        "length(resolution_actor_ref) BETWEEN 1 AND 4096 AND typeof(resolution_ref)='blob' "
        "AND length(resolution_ref) BETWEEN 1 AND 4096)), CHECK((ended_by_skip_kind IS NULL "
        "AND ended_by_skip_ref IS NULL) OR (policy=1 AND resolution IS NULL AND "
        "typeof(ended_by_skip_kind)='integer' AND ended_by_skip_kind IN (0,1,2) AND "
        "typeof(ended_by_skip_ref)='blob' AND length(ended_by_skip_ref) BETWEEN 1 AND "
        "4096)), PRIMARY KEY(occurrence_ref,refund_ref), FOREIGN KEY(occurrence_ref) "
        "REFERENCES subathon_milestone_groups(occurrence_ref))",
    };
    const QStringList tables{"subathon_milestone_acceptance_decisions",
                             "subathon_milestone_refund_decisions"};
    for (qsizetype i = 0; i < tables.size(); ++i) {
        const auto &table = tables[i];
        step.expected_schema.append({"table", table, table, step.sql[i]});
        step.expected_schema.append({"index", "sqlite_autoindex_" + table + "_1", table, {}});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &left, const auto &right) {
                  return std::tie(left.type, left.name) < std::tie(right.type, right.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(13);
    return manifest;
}

std::optional<PausedMilestoneActionPlan>
plan_paused_milestone_actions(const PausedMilestoneAcceptance &acceptance,
                              const std::vector<MilestoneOccurrenceKey> &affected,
                              const std::vector<std::string> &ordinary) {
    if (!valid_acceptance(acceptance) || affected.size() > list_limit ||
        ordinary.size() > list_limit || !std::ranges::all_of(ordinary, bounded_ref))
        return std::nullopt;
    Occurrences keys;
    for (const auto &key : affected) {
        if (!bounded_ref(key.ref))
            return std::nullopt;
        keys.insert(key.ref);
    }
    if (!exact_choices(acceptance, std::move(keys)))
        return std::nullopt;
    return PausedMilestoneActionPlan{
        acceptance, acceptance.mode == Mode::normal ? ordinary : std::vector<std::string>{}};
}

void SqliteMilestoneGroupDecisions::record_acceptance(
    const ApplicationOperationKey &origin, const PausedMilestoneAcceptance &acceptance) const {
    guarded(session_, [&] {
        if (!valid_acceptance(acceptance))
            invalid();
        require_decision_catalog(session_);
        const auto groups = session_.execute(
            {"SELECT g.occurrence_ref IS NOT NULL," + ref_column("g.occurrence_ref") +
                 ",g.released,EXISTS(SELECT 1 FROM main.subathon_milestone_refund_decisions AS d "
                 "WHERE d.occurrence_ref=g.occurrence_ref) "
                 "FROM main.subathon_app_operations AS o "
                 "LEFT JOIN main.subathon_milestone_groups AS g "
                 "ON g.operation_scope=o.operation_scope AND g.operation_ref=o.operation_ref "
                 "WHERE o.operation_scope=? AND o.operation_ref=?",
             {blob(origin.operation_scope), blob(origin.operation_ref)}});
        if (groups.empty() || groups.size() > qsizetype(list_limit))
            invalid();

        std::vector<std::string> refs;
        refs.reserve(std::size_t(groups.size()));
        for (const auto &group : groups) {
            if (integer(group[0]) == 0)
                continue; // The originating operation has no groups.
            // Paused acceptance is initial enrollment, before release/refund.
            if (integer(group[2]) != 0 || integer(group[3]) != 0)
                invalid();
            refs.push_back(reference(group[1]));
        }
        if (!exact_choices(acceptance, Occurrences(refs.begin(), refs.end())))
            invalid();

        const auto actor = blob(acceptance.evidence.actor_ref);
        const auto decision = blob(acceptance.evidence.decision_ref);
        for (const auto &entry : acceptance.choices) {
            (void)session_.execute(
                {"INSERT INTO main.subathon_milestone_acceptance_decisions"
                 "(occurrence_ref,mode,choice,actor_ref,decision_ref) VALUES(?,?,?,?,?)",
                 {blob(entry.occurrence.ref), int(acceptance.mode), int(entry.choice), actor,
                  decision}});
        }
    });
}

void SqliteMilestoneGroupDecisions::observe_refund(const MilestoneRefundObservation &input) const {
    guarded(session_, [&] {
        if (input.policy != Policy::continue_actions && input.policy != Policy::review &&
            input.policy != Policy::skip)
            invalid();

        require_decision_catalog(session_);
        const auto occurrence = blob(input.occurrence.ref);
        const auto refund = blob(input.refund_ref);
        const auto existing = refund_rows(session_, occurrence, refund);
        if (const auto *row = unique_row(existing)) {
            if (integer((*row)[0]) != input.score_before_micro_points ||
                integer((*row)[1]) != input.score_after_micro_points ||
                integer((*row)[2]) != input.current_target_micro_points ||
                read_enum((*row)[3], Policy::skip) != input.policy)
                invalid();
            return;
        }

        if (input.score_before_micro_points < input.current_target_micro_points ||
            input.score_after_micro_points >= input.current_target_micro_points)
            return;
        const auto state = disposition(session_, occurrence);
        if (state == Disposition::absent_or_released || state == Disposition::skipped)
            return;

        (void)session_.execute(
            {"INSERT INTO main.subathon_milestone_refund_decisions"
             "(occurrence_ref,refund_ref,score_before_micro_points,score_after_micro_points,"
             "current_target_micro_points,policy) VALUES(?,?,?,?,?,?)",
             {occurrence, refund, qint64(input.score_before_micro_points),
              qint64(input.score_after_micro_points), qint64(input.current_target_micro_points),
              int(input.policy)}});
        if (input.policy == Policy::skip)
            end_other_holds(session_, occurrence, SkipOrigin::refund_policy, refund);
    });
}

bool SqliteMilestoneGroupDecisions::resolve(const MilestoneOccurrenceKey &key,
                                            const MilestoneHoldKey &hold, Resolution resolution,
                                            const MilestoneDecisionEvidence &evidence) const {
    return guarded(session_, [&] {
        if ((hold.cause != Cause::deferred_acceptance && hold.cause != Cause::refund_review) ||
            (resolution != Resolution::continue_actions && resolution != Resolution::skip) ||
            !valid_evidence(evidence))
            invalid();
        require_decision_catalog(session_);
        const auto occurrence = blob(key.ref);
        const auto source = blob(hold.source_ref);
        if (disposition(session_, occurrence) != Disposition::held)
            return false;

        const auto deferred = hold.cause == Cause::deferred_acceptance;
        const QString table = deferred ? "subathon_milestone_acceptance_decisions"
                                       : "subathon_milestone_refund_decisions";
        const QString match =
            deferred ? "choice=1 AND decision_ref=?" : "policy=1 AND refund_ref=?";
        (void)session_.execute({"UPDATE main." + table +
                                    " SET resolution=?,resolution_actor_ref=?,resolution_ref=? "
                                    "WHERE occurrence_ref=? AND " +
                                    match + " AND " + active,
                                {int(resolution), blob(evidence.actor_ref),
                                 blob(evidence.decision_ref), occurrence, source}});
        const auto changed = session_.execute({"SELECT changes()", {}});
        const auto *row = unique_row(changed);
        if (!row)
            corrupt();
        const auto count = integer(row->front());
        if (count == 0)
            return false;
        if (count != 1)
            corrupt();

        if (resolution == Resolution::skip)
            end_other_holds(
                session_, occurrence,
                deferred ? SkipOrigin::deferred_resolution : SkipOrigin::refund_resolution, source);
        return true;
    });
}

std::vector<MilestoneHoldKey>
SqliteMilestoneGroupDecisions::active_holds(const MilestoneOccurrenceKey &key) const {
    return guarded(session_, [&] {
        require_decision_catalog(session_);
        const auto occurrence = blob(key.ref);
        std::vector<MilestoneHoldKey> result;
        if (disposition(session_, occurrence) != Disposition::held)
            return result;

        const auto rows = session_.execute(
            {"SELECT cause," + ref_column("source") +
                 " FROM (SELECT 0 AS cause,decision_ref AS source "
                 "FROM main.subathon_milestone_acceptance_decisions "
                 "WHERE occurrence_ref=? AND choice=1 AND " +
                 active +
                 " UNION ALL SELECT 1,refund_ref FROM main.subathon_milestone_refund_decisions "
                 "WHERE occurrence_ref=? AND policy=1 AND " +
                 active + ") ORDER BY cause,source",
             {occurrence, occurrence}});
        result.reserve(std::size_t(rows.size()));
        for (const auto &row : rows)
            result.push_back({read_enum(row[0], Cause::refund_review), reference(row[1])});
        return result;
    });
}

MilestonePendingDisposition
SqliteMilestoneGroupDecisions::pending_disposition(const MilestoneOccurrenceKey &key) const {
    return guarded(session_, [&] {
        require_decision_catalog(session_);
        return disposition(session_, blob(key.ref));
    });
}

std::optional<MilestoneAcceptanceAudit>
SqliteMilestoneGroupDecisions::acceptance_audit(const MilestoneOccurrenceKey &key) const {
    return guarded(session_, [&]() -> std::optional<MilestoneAcceptanceAudit> {
        require_decision_catalog(session_);
        const auto rows = session_.execute(
            {"SELECT mode,choice," + ref_column("actor_ref") + "," + ref_column("decision_ref") +
                 "," + history_columns() +
                 " FROM main.subathon_milestone_acceptance_decisions WHERE occurrence_ref=?",
             {blob(key.ref)}});
        const auto *row = unique_row(rows);
        if (!row)
            return std::nullopt;
        const auto choice = read_enum((*row)[1], Choice::skip);
        auto history = hold_history(*row, 4, choice == Choice::defer);
        return MilestoneAcceptanceAudit{read_enum((*row)[0], Mode::silent),
                                        choice,
                                        {reference((*row)[2]), reference((*row)[3])},
                                        history.resolution,
                                        std::move(history.evidence),
                                        std::move(history.ended)};
    });
}

std::optional<MilestoneRefundAudit>
SqliteMilestoneGroupDecisions::refund_audit(const MilestoneOccurrenceKey &key,
                                            const std::string &refund) const {
    return guarded(session_, [&]() -> std::optional<MilestoneRefundAudit> {
        require_decision_catalog(session_);
        const auto rows = refund_rows(session_, blob(key.ref), blob(refund));
        const auto *row = unique_row(rows);
        if (!row)
            return std::nullopt;
        const auto before = integer((*row)[0]);
        const auto after = integer((*row)[1]);
        const auto target = integer((*row)[2]);
        if (before < target || after >= target)
            corrupt();
        // For ordered signed operands this unsigned difference is exact,
        // including INT64_MAX - INT64_MIN.
        const auto deficit = std::uint64_t(target) - std::uint64_t(after);
        const auto policy = read_enum((*row)[3], Policy::skip);
        auto history = hold_history(*row, 4, policy == Policy::review);
        return MilestoneRefundAudit{policy,
                                    target,
                                    deficit,
                                    history.resolution,
                                    std::move(history.evidence),
                                    std::move(history.ended)};
    });
}

} // namespace subathon::server
