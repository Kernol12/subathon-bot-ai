// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_milestone_edit_detail.hpp"

#include <algorithm>
#include <tuple>

namespace subathon::server::milestone_edit_sql {

// Keep both owner operations on the same exact current definition and prefix reads.
inline constexpr auto track_match = "installation_ref=? AND track_ref=?";

inline QList<QVariant> track_args(const MilestoneTrackKey &key) {
    return {blob(key.installation_ref), blob(key.track_ref)};
}

inline bool reference_fields(std::initializer_list<std::string_view> refs) {
    return std::ranges::all_of(refs, bounded_ref);
}

inline bool same_completion(const core::MilestoneCompletion &a,
                            const core::MilestoneCompletion &b) {
    return std::tie(a.milestone_ref, a.definition_revision_ref, a.target_micro_points,
                    a.score_micro_points, a.completion_context_ref) ==
           std::tie(b.milestone_ref, b.definition_revision_ref, b.target_micro_points,
                    b.score_micro_points, b.completion_context_ref);
}

inline core::MilestoneCompletion read_completion(const SqlRow &r, qsizetype first = 0) {
    return {reference(r[first]), reference(r[first + 1]), integer(r[first + 2]),
            integer(r[first + 3]), reference(r[first + 4])};
}

inline QString completion_columns(const QString &revision) {
    return ref_column("milestone_ref") + "," + ref_column(revision) +
           ",target_micro_points,score_micro_points," + ref_column("completion_context_ref");
}

inline bool application_member(const SqliteWorkSession &s, const MilestoneTrackSeed::Member &m) {
    if (!reference_fields({m.application.operation_scope, m.application.operation_ref,
                           m.occurrence.ref, m.completion.milestone_ref,
                           m.completion.definition_revision_ref,
                           m.completion.completion_context_ref}))
        return false;
    const auto rows =
        s.execute({"SELECT " + completion_columns("definition_revision_ref") +
                       ",queue_order FROM main.subathon_milestone_groups g JOIN "
                       "main.subathon_app_operations o "
                       "USING(operation_scope,operation_ref) WHERE g.occurrence_ref=? "
                       "AND g.operation_scope=? AND g.operation_ref=?",
                   {blob(m.occurrence.ref), blob(m.application.operation_scope),
                    blob(m.application.operation_ref)}});
    if (const auto *row = unique(rows)) {
        check_application_identity(s, blob(m.occurrence.ref), integer((*row)[5]));
        if (terminal_partition::selected(s))
            verify_partitioned_origin(s, m.application);
        return same_completion(m.completion, read_completion(*row)) &&
               m.completion.score_micro_points >= m.completion.target_micro_points;
    }
    return false;
}

struct Definition {
    std::string revision;
    std::string configuration;
    std::optional<std::string> predecessor;
    std::vector<core::MilestoneTarget> targets;
};

inline std::optional<Definition> definition(const SqliteWorkSession &s,
                                            const MilestoneTrackKey &track,
                                            const std::optional<std::string> &revision = {}) {
    auto args = track_args(track);
    QString where = track_match;
    if (revision) {
        where += " AND definition_revision=?";
        args.append(blob(*revision));
    }
    const auto rows =
        s.execute({"SELECT " + ref_column("definition_revision") + "," +
                       ref_column("configuration_ref") + "," + ref_column("predecessor_revision") +
                       " FROM main.subathon_milestone_definition_versions " +
                       (revision ? QString{}
                                 : "JOIN main.subathon_milestone_definition_heads "
                                   "USING(installation_ref,track_ref,definition_revision) ") +
                       "WHERE " + where,
                   args});
    const auto *row = unique(rows);
    if (!row)
        return {};
    Definition selected{reference((*row)[0]),
                        reference((*row)[1]),
                        (*row)[2].isNull() ? std::nullopt : std::optional(reference((*row)[2])),
                        {}};
    args = track_args(track);
    args.append({blob(selected.revision), collection_limit + 1});
    const auto targets = s.execute(
        {"SELECT ordinal," + ref_column("milestone_ref") +
             ",target_micro_points FROM main.subathon_milestone_definition_targets WHERE " +
             track_match + " AND definition_revision=? ORDER BY ordinal LIMIT ?",
         args});
    if (targets.size() > collection_limit)
        corrupt();
    for (const auto &target : targets) {
        if (integer(target[0]) != qint64(selected.targets.size()) + 1)
            corrupt();
        selected.targets.push_back({reference(target[1]), integer(target[2])});
    }
    return selected;
}

struct Prefix {
    std::string revision;
    std::vector<core::MilestoneCompletion> members;
    // Retain exact application/edit links when copying the old prefix.
    SqlRows origins;
};

inline std::optional<Prefix> prefix(const SqliteWorkSession &s, const MilestoneTrackKey &track,
                                    const std::optional<std::string> &revision = {}) {
    auto args = track_args(track);
    QString where = track_match;
    if (revision) {
        where += " AND prefix_revision=?";
        args.append(blob(*revision));
    }
    const auto rows =
        s.execute({"SELECT " + ref_column("prefix_revision") +
                       ",prefix_length FROM main.subathon_milestone_prefix_revisions " +
                       (revision ? QString{}
                                 : "JOIN main.subathon_milestone_prefix_heads "
                                   "USING(installation_ref,track_ref,prefix_revision) ") +
                       "WHERE " + where,
                   args});
    const auto *row = unique(rows);
    if (!row)
        return {};
    Prefix selected{reference((*row)[0]), {}, {}};
    const auto length = integer((*row)[1]);
    if (length < 0 || length > collection_limit)
        corrupt();
    args = track_args(track);
    args.append({blob(selected.revision), length + 1});
    const auto members =
        s.execute({"SELECT ordinal," + completion_columns("definition_revision") + ",origin," +
                       ref_column("operation_scope") + "," + ref_column("operation_ref") + "," +
                       ref_column("application_occurrence_ref") + "," + ref_column("edit_ref") +
                       "," + ref_column("edit_occurrence_ref") +
                       " FROM main.subathon_milestone_prefix_members WHERE " + track_match +
                       " AND prefix_revision=? ORDER BY ordinal LIMIT ?",
                   args});
    if (members.size() != length)
        corrupt();
    for (const auto &member : members) {
        if (integer(member[0]) != qint64(selected.members.size()) + 1)
            corrupt();
        auto completion = read_completion(member, 1);
        const auto origin = integer(member[6]);
        if (origin == 0) {
            if (!member[10].isNull() || !member[11].isNull() ||
                !application_member(s, {completion,
                                        {reference(member[7]), reference(member[8])},
                                        {reference(member[9])}}))
                corrupt();
        } else if (origin == 1) {
            if (!member[7].isNull() || !member[8].isNull() || !member[9].isNull())
                corrupt();
            const auto edit = occurrence(s, blob(reference(member[11])));
            if (!edit || edit->edit.track != track ||
                edit->edit.operation_ref != reference(member[10]) ||
                !same_completion(edit->group.achievement, completion))
                corrupt();
        } else {
            corrupt();
        }
        selected.members.push_back(std::move(completion));
        selected.origins.push_back(member.mid(6));
    }
    return selected;
}

} // namespace subathon::server::milestone_edit_sql
