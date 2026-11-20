// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_milestone_target_edits.hpp>

#include "sqlite_milestone_edit_detail.hpp"
#include "sqlite_milestone_edit_schema.hpp"

#include <QDataStream>
#include <QIODevice>
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <subathon/server/sqlite_rate_overrides.hpp>
#include <tuple>

namespace subathon::server {

class MilestoneEditBinding final {
  public:
    QByteArray intent;
    QByteArray sources;
    QByteArray captures;
};

namespace {
using namespace milestone_edit_sql;
using Status = core::MilestoneEditStatus;
using Choice = MilestoneActionChoice;
using Kind = MilestoneActionKind;
using Life = core::TimerLifecycle;
using State = DeliveryJobState;
using Disposition = DeliveryDisposition;
using Release = MilestoneReleaseDisposition;

constexpr auto track_match = "installation_ref=? AND track_ref=?";
constexpr auto edit_match = "installation_ref=? AND track_ref=? AND edit_ref=?";

QList<QVariant> track_args(const MilestoneTrackKey &key) {
    return {blob(key.installation_ref), blob(key.track_ref)};
}

QList<QVariant> edit_args(const MilestoneEditKey &key) {
    auto args = track_args(key.track);
    args.append(blob(key.operation_ref));
    return args;
}

void execute(const SqliteWorkSession &s, QString sql, QList<QVariant> args) {
    (void)s.execute({std::move(sql), std::move(args)});
}

void exactly_one(const SqliteWorkSession &s) {
    const auto rows = s.execute({"SELECT changes()", {}});
    if (!unique(rows) || integer(rows.front().front()) != 1)
        corrupt();
}

bool reference_fields(std::initializer_list<std::string_view> refs) {
    return std::ranges::all_of(refs, bounded_ref);
}

bool same_completion(const core::MilestoneCompletion &a, const core::MilestoneCompletion &b) {
    return std::tie(a.milestone_ref, a.definition_revision_ref, a.target_micro_points,
                    a.score_micro_points, a.completion_context_ref) ==
           std::tie(b.milestone_ref, b.definition_revision_ref, b.target_micro_points,
                    b.score_micro_points, b.completion_context_ref);
}

QList<QVariant> completion_args(const core::MilestoneCompletion &c) {
    return {blob(c.milestone_ref), blob(c.definition_revision_ref), qint64(c.target_micro_points),
            qint64(c.score_micro_points), blob(c.completion_context_ref)};
}

core::MilestoneCompletion read_completion(const SqlRow &r, qsizetype first = 0) {
    return {reference(r[first]), reference(r[first + 1]), integer(r[first + 2]),
            integer(r[first + 3]), reference(r[first + 4])};
}

QString completion_columns(const QString &revision) {
    return ref_column("milestone_ref") + "," + ref_column(revision) +
           ",target_micro_points,score_micro_points," + ref_column("completion_context_ref");
}

bool application_member(const SqliteWorkSession &s, const MilestoneTrackSeed::Member &m) {
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

std::optional<Definition> definition(const SqliteWorkSession &s, const MilestoneTrackKey &track,
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

std::optional<Prefix> prefix(const SqliteWorkSession &s, const MilestoneTrackKey &track,
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

void insert_definition(const SqliteWorkSession &s, const MilestoneTrackKey &track,
                       const Definition &version) {
    auto args = track_args(track);
    args.append(blob(version.revision));
    auto version_args = args;
    version_args.append({blob(version.configuration),
                         version.predecessor ? QVariant(blob(*version.predecessor)) : QVariant{}});
    execute(s, "INSERT INTO main.subathon_milestone_definition_versions VALUES(?,?,?,?,?)",
            version_args);
    qint64 ordinal = 0;
    for (const auto &target : version.targets) {
        auto values = args;
        values.append({++ordinal, blob(target.milestone_ref), qint64(target.target_micro_points)});
        execute(s, "INSERT INTO main.subathon_milestone_definition_targets VALUES(?,?,?,?,?,?)",
                values);
    }
}

void insert_prefix(const SqliteWorkSession &s, const MilestoneTrackKey &track,
                   const Prefix &version) {
    auto args = track_args(track);
    args.append(blob(version.revision));
    auto values = args;
    values.append(qint64(version.members.size()));
    execute(s, "INSERT INTO main.subathon_milestone_prefix_revisions VALUES(?,?,?,?)", values);
    for (std::size_t i = 0; i < version.members.size(); ++i) {
        values = args;
        values.append(qint64(i) + 1);
        values.append(completion_args(version.members[i]));
        values.append(version.origins[qsizetype(i)]);
        execute(s,
                "INSERT INTO main.subathon_milestone_prefix_members "
                "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                values);
    }
}

void advance_head(const SqliteWorkSession &s, const MilestoneTrackKey &track, bool definition_head,
                  const std::string &next, const std::optional<std::string> &prior = {}) {
    const QString table = definition_head ? "main.subathon_milestone_definition_heads"
                                          : "main.subathon_milestone_prefix_heads";
    auto args = track_args(track);
    if (prior) {
        const QString revision = definition_head ? "definition_revision" : "prefix_revision";
        args.prepend(blob(next));
        args.append(blob(*prior));
        execute(s,
                "UPDATE " + table + " SET " + revision + "=? WHERE " + track_match + " AND " +
                    revision + "=?",
                args);
    } else {
        args.append(blob(next));
        execute(s, "INSERT INTO " + table + " VALUES(?,?,?)", args);
    }
    exactly_one(s);
}

// Fixed-width numbers and length-prefixed bytes, with a bound checked before
// every append. These records are private to v18 and are compared byte for byte.
class Encoding {
  public:
    Encoding() { number(1); }

    void number(qint64 n) {
        if (data_.size() > document_limit - 8)
            limit();
        stream_ << n;
        if (stream_.status() != QDataStream::Ok)
            corrupt();
    }
    void bytes(const QByteArray &b) {
        number(b.size());
        if (b.size() > document_limit - data_.size())
            limit();
        if (stream_.writeRawData(b.constData(), b.size()) != b.size())
            corrupt();
    }
    void string(std::string_view ref) {
        if (ref.size() > 4096)
            limit();
        bytes(QByteArray::fromRawData(ref.data(), qsizetype(ref.size())));
    }
    const QByteArray &data() const { return data_; }

  private:
    [[noreturn]] static void limit() {
        throw SqliteBundleStorageFailure(SqliteBundleStorageReason::result_limit);
    }
    QByteArray data_;
    QDataStream stream_{&data_, QIODevice::WriteOnly};
};

void targets_wire(Encoding &out, const std::vector<core::MilestoneTarget> &targets) {
    out.number(qint64(targets.size()));
    for (const auto &target : targets) {
        out.string(target.milestone_ref);
        out.number(target.target_micro_points);
    }
}

QByteArray identity(const MilestoneEditActor &actor, const MilestoneEditIntent &intent) {
    Encoding out;
    out.string(intent.key.track.installation_ref);
    out.string(intent.key.track.track_ref);
    out.string(intent.key.operation_ref);
    out.string(actor.actor_ref);
    out.string(actor.session_lineage);
    out.string(intent.evidence.decision_ref);
    targets_wire(out, intent.patch);
    out.number(qint64(intent.choices.size()));
    for (const auto &choice : intent.choices) {
        out.string(choice.occurrence.ref);
        out.number(int(choice.choice));
    }
    return out.data();
}

bool valid_intent(const MilestoneEditActor &actor, const MilestoneEditIntent &intent) {
    if (intent.evidence.actor_ref != actor.actor_ref ||
        !reference_fields({actor.actor_ref, actor.session_lineage, intent.evidence.decision_ref,
                           intent.key.track.installation_ref, intent.key.track.track_ref,
                           intent.key.operation_ref}) ||
        intent.patch.size() > std::size_t(collection_limit) ||
        intent.choices.size() > std::size_t(collection_limit))
        return false;
    return std::ranges::all_of(
               intent.patch,
               [](const auto &target) { return bounded_ref(target.milestone_ref); }) &&
           std::ranges::all_of(intent.choices, [](const auto &choice) {
               return bounded_ref(choice.occurrence.ref);
           });
}

void lifecycle_wire(Encoding &out, const MilestoneEditLifecycle &life) {
    out.string(life.revision);
    out.number(int(life.timer.lifecycle));
    out.number(int(life.timer.phase));
    out.number(life.timer.main_remaining_ms);
    out.number(life.timer.grace_remaining_ms);
    out.number(life.timer.awaiting_confirmation);
    out.number(life.debt_ms);
    out.number(life.deadline_utc_ms.has_value());
    if (life.deadline_utc_ms)
        out.number(*life.deadline_utc_ms);
    out.number(qint64(life.pause_reasons.size()));
    for (const auto &reason : life.pause_reasons)
        out.string(reason);
    out.string(life.reopen_pending_action_revision);
    out.number(life.post_finish_progression);
    out.number(life.post_finish_automation);
}

void completion_wire(Encoding &out, const core::MilestoneCompletion &c) {
    out.string(c.milestone_ref);
    out.string(c.definition_revision_ref);
    out.number(c.target_micro_points);
    out.number(c.score_micro_points);
    out.string(c.completion_context_ref);
}

QByteArray sources_wire(const MilestoneEditFacts &facts, const Definition &current,
                        const Prefix &active, const MilestoneEditPreview &preview, qint64 tail) {
    Encoding out;
    out.string(facts.configuration_ref);
    out.string(facts.capture_revision);
    out.string(facts.grant_revision);
    out.number(facts.unfinished_grant);
    out.number(facts.correction_grant);
    out.number(facts.disclosure_grant);
    out.string(current.revision);
    out.string(facts.candidate_definition_revision);
    out.string(active.revision);
    out.string(facts.candidate_prefix_revision);
    out.string(preview.score_head);
    out.number(preview.score);
    out.number(tail);
    lifecycle_wire(out, facts.lifecycle);
    targets_wire(out, current.targets);
    out.number(qint64(active.members.size()));
    for (std::size_t i = 0; i < active.members.size(); ++i) {
        completion_wire(out, active.members[i]);
        const auto &origin = active.origins[qsizetype(i)];
        out.number(integer(origin[0]));
        for (qsizetype j = 1; j < origin.size(); ++j)
            out.string(origin[j].isNull() ? "" : reference(origin[j]));
    }
    return out.data();
}

bool valid_facts(const MilestoneEditFacts &facts) {
    const auto &life = facts.lifecycle;
    return reference_fields({facts.configuration_ref, facts.capture_revision, facts.grant_revision,
                             facts.candidate_definition_revision, facts.candidate_prefix_revision,
                             life.revision}) &&
           (life.reopen_pending_action_revision.empty() ||
            bounded_ref(life.reopen_pending_action_revision)) &&
           life.timer.lifecycle >= Life::Ready && life.timer.lifecycle <= Life::Finished &&
           life.timer.phase >= core::CountdownPhase::Main &&
           life.timer.phase <= core::CountdownPhase::Grace && life.timer.main_remaining_ms >= 0 &&
           life.timer.grace_remaining_ms >= 0 && life.debt_ms >= 0 &&
           (life.timer.phase != core::CountdownPhase::Grace ||
            (life.timer.main_remaining_ms == 0 && !life.timer.awaiting_confirmation)) &&
           (!life.timer.awaiting_confirmation ||
            (life.timer.lifecycle == Life::Active && life.timer.main_remaining_ms == 0)) &&
           life.pause_reasons.size() <= std::size_t(collection_limit) &&
           std::ranges::all_of(life.pause_reasons, bounded_ref) &&
           facts.captures.size() <= std::size_t(collection_limit);
}

void timing_wire(Encoding &out, const MilestoneTimingOverride &timing) {
    out.number(timing.presentation ? timing.presentation->count() : -1);
    out.number(timing.gap ? timing.gap->count() : -1);
}

bool capture_wire(Encoding &out, const MilestoneEditCapture &capture) {
    if (!reference_fields({capture.milestone_ref, capture.occurrence.ref}) ||
        capture.actions.size() > std::size_t(collection_limit) ||
        capture.jobs.size() > capture.actions.size() ||
        (capture.timing.presentation && capture.timing.presentation->count() < 1) ||
        (capture.timing.gap && capture.timing.gap->count() < 0))
        return false;
    out.string(capture.milestone_ref);
    out.string(capture.occurrence.ref);
    timing_wire(out, capture.timing);
    out.number(qint64(capture.actions.size()));
    std::map<qint64, const MilestoneEditCapture::Job *> jobs;
    for (const auto &entry : capture.jobs)
        if (!jobs.emplace(entry.ordinal, &entry).second)
            return false;
    for (std::size_t i = 0; i < capture.actions.size(); ++i) {
        const auto &action = capture.actions[i];
        if (!reference_fields({action.action_ref, action.version_ref}) ||
            (action.kind != Kind::automation && action.kind != Kind::bonus))
            return false;
        out.number(int(action.kind));
        out.string(action.action_ref);
        out.string(action.version_ref);
        if (action.kind == Kind::bonus)
            continue;
        const auto entry = jobs.find(qint64(i) + 1);
        if (entry == jobs.end())
            return false;
        const auto &job = *entry->second;
        const auto &cap = job.capture;
        if (job.envelope.format_ref != delivery_job_format || job.envelope.document.isNull() ||
            job.envelope.document.size() > 262144 ||
            !reference_fields({cap.route_ref, cap.sender_ref}) ||
            cap.action_ref != action.action_ref || cap.action_version_ref != action.version_ref ||
            (cap.selection != DeliverySenderSelection::explicit_identity &&
             cap.selection != DeliverySenderSelection::configured_default))
            return false;
        out.string(job.envelope.format_ref);
        out.bytes(job.envelope.document);
        out.string(cap.route_ref);
        out.number(int(cap.selection));
        out.string(cap.sender_ref);
        jobs.erase(entry);
    }
    return jobs.empty();
}

const MilestoneEditCapture *find_capture(const MilestoneEditFacts &facts, std::string_view target) {
    const MilestoneEditCapture *selected = nullptr;
    for (const auto &capture : facts.captures) {
        if (capture.milestone_ref != target)
            continue;
        if (selected)
            return nullptr;
        selected = &capture;
    }
    return selected;
}

struct Prepared {
    MilestoneEditPreview preview{};
    std::optional<Prefix> active;
    qint64 tail{};
    QByteArray sources;
};

MilestoneEditPreview refusal(Status status) {
    MilestoneEditPreview preview{};
    preview.plan.status = status;
    return preview;
}

Prepared prepare(const SqliteWorkSession &s, const MilestoneEditActor &actor,
                 const MilestoneEditIntent &intent, const MilestoneEditFacts &facts,
                 const QByteArray *bound_sources = nullptr) {
    Prepared p;
    p.preview = refusal(Status::unavailable);
    const auto current = definition(s, intent.key.track);
    p.active = prefix(s, intent.key.track);
    if (!current || !p.active || current->configuration != facts.configuration_ref)
        return p;
    const auto state = SqliteApplicationBundleStore(s).read_state();
    if (!state)
        return p;

    auto &preview = p.preview;
    try {
        preview.score = total_score(*state);
    } catch (const std::overflow_error &) {
        return p;
    }
    p.tail = queue_tail(s);
    preview.score_head = state->token;
    preview.prior_definition_revision = current->revision;
    preview.next_definition_revision = facts.candidate_definition_revision;
    p.sources = sources_wire(facts, *current, *p.active, preview, p.tail);
    if (bound_sources && p.sources != *bound_sources) {
        preview.plan.status = Status::stale;
        return p;
    }
    preview.previous_targets = current->targets;
    preview.lifecycle = facts.lifecycle;
    const bool finished = facts.lifecycle.timer.lifecycle == Life::Finished;
    preview.plan = core::plan_milestone_target_edits(
        {intent.key.track.track_ref, current->revision, intent.key.operation_ref, preview.score,
         current->targets, p.active->members},
        intent.patch,
        {facts.unfinished_grant, facts.correction_grant,
         !finished || facts.lifecycle.post_finish_progression,
         !finished || facts.lifecycle.post_finish_automation, p.tail});
    if (preview.plan.status != Status::eligible)
        return p;

    // Candidate revisions are never recycled, even if the values return.
    const auto track = track_args(intent.key.track);
    auto candidates = track;
    candidates.append(blob(facts.candidate_definition_revision));
    candidates.append(track);
    candidates.append(blob(facts.candidate_prefix_revision));
    const auto revision_reuse = s.execute(
        {"SELECT 1 FROM main.subathon_milestone_definition_versions WHERE installation_ref=? "
         "AND track_ref=? AND definition_revision=? UNION ALL SELECT 1 "
         "FROM main.subathon_milestone_prefix_revisions WHERE installation_ref=? "
         "AND track_ref=? AND prefix_revision=?",
         candidates});
    if (!revision_reuse.empty()) {
        preview.plan.status = Status::key_conflict;
        return p;
    }

    Encoding captures;
    captures.number(qint64(preview.plan.newly_completed.size()));
    std::set<std::string_view> affected;
    for (const auto &target : preview.plan.newly_completed) {
        const auto *capture = find_capture(facts, target.milestone_ref);
        if (!capture || !capture_wire(captures, *capture)) {
            preview.plan.status = Status::unavailable;
            return p;
        }
        if (!affected.insert(capture->occurrence.ref).second ||
            !s.execute({"SELECT 1 FROM main.subathon_milestone_groups WHERE occurrence_ref=? "
                        "UNION ALL SELECT 1 FROM main.subathon_milestone_edit_occurrences "
                        "WHERE occurrence_ref=?",
                        {blob(capture->occurrence.ref), blob(capture->occurrence.ref)}})
                 .empty()) {
            preview.plan.status = Status::key_conflict;
            return p;
        }
    }
    for (const auto &target : preview.plan.newly_completed)
        preview.achievements.push_back(*find_capture(facts, target.milestone_ref));
    if (intent.choices.empty() && !affected.empty())
        return p;
    for (const auto &choice : intent.choices) {
        if (choice.choice < Choice::run || choice.choice > Choice::skip ||
            affected.erase(choice.occurrence.ref) != 1) {
            preview.plan.status = Status::invalid_choices;
            return p;
        }
    }
    if (!affected.empty()) {
        preview.plan.status = Status::invalid_choices;
        return p;
    }

    preview.confirmation = std::make_shared<MilestoneEditBinding>(
        MilestoneEditBinding{identity(actor, intent), p.sources, captures.data()});
    return p;
}

struct Classification {
    bool unfinished{};
    bool completed{};
};

Classification classify(const std::vector<core::MilestoneTarget> &before,
                        const std::vector<core::MilestoneTarget> &after, std::size_t prefix_size) {
    if (before.size() != after.size() || prefix_size > before.size())
        corrupt();
    Classification result;
    for (std::size_t i = 0; i < before.size(); ++i) {
        if (before[i].milestone_ref != after[i].milestone_ref)
            corrupt();
        if (before[i].target_micro_points != after[i].target_micro_points)
            (i < prefix_size ? result.completed : result.unfinished) = true;
    }
    if (!result.completed && !result.unfinished)
        corrupt();
    return result;
}

QByteArray result_wire(const MilestoneEditResult &result, const std::string &prior_prefix) {
    Encoding out;
    out.string(prior_prefix);
    out.string(result.prefix_revision);
    out.string(result.definition_revision);
    out.number(qint64(result.achievements.size()));
    for (const auto &achievement : result.achievements) {
        completion_wire(out, achievement.group.achievement);
        out.string(achievement.group.occurrence->ref);
        out.number(achievement.order);
        out.number(int(achievement.choice));
    }
    return out.data();
}

void projection_wire(Encoding &out, const core::MilestoneProjection &projection) {
    const auto target = [&](const core::MilestoneTarget &v) {
        out.string(v.milestone_ref);
        out.number(v.target_micro_points);
    };
    const auto optional_target = [&](const std::optional<core::MilestoneTarget> &v) {
        out.number(v.has_value());
        if (v)
            target(*v);
    };
    const auto distance = [&](std::uint64_t v) { out.string(std::to_string(v)); };
    out.number(int(projection.state));
    out.number(qint64(projection.completed_deficits.size()));
    for (const auto &deficit : projection.completed_deficits) {
        target(deficit.current_target);
        distance(deficit.deficit_micro_points);
    }
    optional_target(projection.previous);
    optional_target(projection.next);
    distance(projection.track_deficit_micro_points);
    out.number(projection.remaining_micro_points.has_value());
    if (projection.remaining_micro_points)
        distance(*projection.remaining_micro_points);
    out.number(projection.interval.has_value());
    if (projection.interval) {
        distance(projection.interval->earned_micro_points);
        distance(projection.interval->span_micro_points);
    }
}

struct Stored {
    QByteArray identity;
    QByteArray sources;
    QByteArray captures;
    QVariant result;
    std::string prior_revision;
    std::string revision;
    MilestoneDecisionEvidence evidence;
};

std::optional<Stored> operation(const SqliteWorkSession &s, const MilestoneEditKey &key) {
    const auto rows = s.execute(
        {"SELECT CASE WHEN typeof(intent)='blob' AND length(intent)<=8388608 THEN intent END,"
         "CASE WHEN typeof(result)='blob' AND length(result)<=8388608 THEN result END," +
             ref_column("prior_revision") + "," + ref_column("resulting_revision") + "," +
             ref_column("actor_ref") + "," + ref_column("decision_ref") +
             " FROM main.subathon_milestone_edit_operations WHERE " + edit_match,
         edit_args(key)});
    const auto *row = unique(rows);
    if (!row)
        return {};
    Decoding wire((*row)[0]);
    Stored stored{wire.bytes(),
                  wire.bytes(),
                  wire.bytes(),
                  (*row)[1],
                  reference((*row)[2]),
                  reference((*row)[3]),
                  {reference((*row)[4]), reference((*row)[5])}};
    if (wire.bytes().isEmpty())
        corrupt();
    wire.end();
    return stored;
}

MilestoneEditReply reconcile(const SqliteWorkSession &s, const MilestoneEditIntent &intent,
                             const MilestoneEditFacts &facts, const QByteArray &intent_wire,
                             const Stored &stored, const MilestoneEditBinding *binding) {
    if (stored.evidence.actor_ref != intent.evidence.actor_ref)
        return {Status::denied, {}};
    if (stored.identity != intent_wire || stored.evidence != intent.evidence ||
        (binding && (binding->sources != stored.sources || binding->captures != stored.captures)))
        return {Status::key_conflict, {}};

    Decoding result_doc(stored.result);
    const auto previous_prefix = result_doc.ref();
    const auto resulting_prefix = result_doc.ref();
    auto old = definition(s, intent.key.track, stored.prior_revision);
    auto next = definition(s, intent.key.track, stored.revision);
    auto previous = prefix(s, intent.key.track, previous_prefix);
    auto completed = resulting_prefix == previous_prefix
                         ? std::optional<Prefix>{}
                         : prefix(s, intent.key.track, resulting_prefix);
    if (!old || !next || next->predecessor != old->revision || !previous ||
        (resulting_prefix != previous_prefix && !completed))
        corrupt();
    if (binding && (facts.configuration_ref != next->configuration ||
                    facts.candidate_definition_revision != next->revision ||
                    (completed && facts.candidate_prefix_revision != completed->revision)))
        return {Status::key_conflict, {}};
    const auto classification = classify(old->targets, next->targets, previous->members.size());
    std::map<std::string_view, std::int64_t> patch;
    for (const auto &target : intent.patch)
        if (!patch.emplace(target.milestone_ref, target.target_micro_points).second)
            corrupt();
    for (std::size_t i = 0; i < old->targets.size(); ++i) {
        const auto &old_target = old->targets[i];
        const auto change = patch.find(old_target.milestone_ref);
        if (next->targets[i].target_micro_points !=
            (change == patch.end() ? old_target.target_micro_points : change->second))
            corrupt();
        if (change != patch.end())
            patch.erase(change);
    }
    if (!patch.empty())
        corrupt();
    if ((classification.completed && !facts.correction_grant) ||
        (classification.unfinished && !facts.unfinished_grant))
        return {Status::denied, {}};

    const auto rows = s.execute({"SELECT suffix_ordinal," + ref_column("occurrence_ref") +
                                     " FROM main.subathon_milestone_edit_occurrences WHERE " +
                                     edit_match + " ORDER BY suffix_ordinal LIMIT 4097",
                                 edit_args(intent.key)});
    if (rows.size() > collection_limit ||
        (completed &&
         completed->members.size() != previous->members.size() + std::size_t(rows.size())) ||
        (rows.empty() != !completed) || std::size_t(rows.size()) != intent.choices.size())
        corrupt();
    if (completed)
        for (std::size_t i = 0; i < previous->members.size(); ++i)
            if (!same_completion(previous->members[i], completed->members[i]) ||
                previous->origins[qsizetype(i)] != completed->origins[qsizetype(i)])
                corrupt();

    MilestoneEditResult result{stored.revision, resulting_prefix, {}};
    Encoding actual_captures;
    Encoding selected_captures;
    actual_captures.number(rows.size());
    selected_captures.number(rows.size());
    GroupResultSize result_size;
    for (const auto &row : rows) {
        const auto index = result.achievements.size();
        if (integer(row[0]) != qint64(index) + 1)
            corrupt();
        const auto ref = reference(row[1]);
        auto selected = occurrence(s, blob(ref), true);
        if (!selected || selected->edit != intent.key ||
            previous->members.size() + index >= next->targets.size() ||
            selected->group.achievement.milestone_ref !=
                next->targets[previous->members.size() + index].milestone_ref ||
            !same_completion(selected->group.achievement,
                             completed->members[previous->members.size() + index]) ||
            reference(completed->origins[qsizetype(previous->members.size() + index)][5]) != ref)
            corrupt();
        const auto choice = std::ranges::find(intent.choices, *selected->group.occurrence,
                                              &MilestoneAcceptanceChoice::occurrence);
        if (choice == intent.choices.end() || choice->choice != selected->choice)
            corrupt();
        const auto &g = selected->group;
        result_size.include(group_result_size(selected->edit, *g.occurrence, g.achievement,
                                              g.actions, g.edit_membership->jobs.size()));
        MilestoneEditCapture retained{
            g.achievement.milestone_ref, *g.occurrence, g.actions, g.timing, {}};
        for (std::size_t i = 0; i < g.actions.size(); ++i) {
            if (g.actions[i].kind != Kind::automation)
                continue;
            const auto ordinal = qint64(i) + 1;
            auto captured_job = job(s, *selected, ordinal, true).value();
            retained.jobs.push_back({ordinal,
                                     {delivery_job_format, std::move(captured_job.action_document)},
                                     std::move(captured_job.capture)});
        }
        if (!capture_wire(actual_captures, retained))
            corrupt();
        if (binding) {
            const auto *current = find_capture(facts, retained.milestone_ref);
            if (!current || !capture_wire(selected_captures, *current))
                return {Status::key_conflict, {}};
        }
        result.achievements.push_back(
            {std::move(selected->group), selected->order, selected->choice});
    }
    if (actual_captures.data() != stored.captures ||
        result_wire(result, previous_prefix) != stored.result.toByteArray())
        corrupt();
    if (binding && selected_captures.data() != stored.captures)
        return {Status::key_conflict, {}};
    return {Status::eligible, std::move(result)};
}

QVariant timing_arg(const std::optional<std::chrono::milliseconds> &value) {
    return value ? QVariant(qint64(value->count())) : QVariant{};
}

void stage_occurrence(const SqliteWorkSession &s, const MilestoneEditIntent &intent,
                      const MilestoneEditAchievement &achievement,
                      const MilestoneEditCapture &capture, qint64 suffix_ordinal) {
    const auto key = blob(capture.occurrence.ref);
    const auto edit = edit_args(intent.key);
    QList<QVariant> values{key};
    values.append(edit);
    values.append({suffix_ordinal, qint64(achievement.order)});
    values.append(completion_args(achievement.group.achievement));
    values.append({timing_arg(capture.timing.presentation), timing_arg(capture.timing.gap)});
    execute(s,
            "INSERT INTO main.subathon_milestone_edit_occurrences "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,0)",
            values);
    qint64 ordinal = 0;
    for (const auto &action : capture.actions)
        execute(
            s, "INSERT INTO main.subathon_milestone_edit_actions VALUES(?,?,?,?,?)",
            {key, ++ordinal, int(action.kind), blob(action.action_ref), blob(action.version_ref)});
    for (const auto &job : capture.jobs) {
        values = edit;
        values.append(
            {key, qint64(job.ordinal), blob(job.envelope.format_ref), job.envelope.document});
        execute(s, "INSERT INTO main.subathon_milestone_edit_jobs VALUES(?,?,?,?,?,0,?,?)", values);
        const auto &c = job.capture;
        execute(s, "INSERT INTO main.subathon_milestone_edit_captures VALUES(?,?,?,?,?,?,?)",
                {key, qint64(job.ordinal), blob(c.action_ref), blob(c.action_version_ref),
                 blob(c.route_ref), int(c.selection), blob(c.sender_ref)});
        execute(s, "INSERT INTO main.subathon_milestone_edit_events VALUES(?,?,1,0,NULL,NULL)",
                {key, qint64(job.ordinal)});
    }
    values = edit;
    values.append({key, int(achievement.choice), blob(intent.evidence.actor_ref),
                   blob(intent.evidence.decision_ref)});
    execute(s,
            "INSERT INTO main.subathon_milestone_edit_choices "
            "VALUES(?,?,?,?,?,?,?,NULL,NULL,NULL,NULL,NULL)",
            values);
}

MilestoneEditResult stage(const SqliteWorkSession &s, const MilestoneEditIntent &intent,
                          const MilestoneEditFacts &facts, Prepared prepared) {
    const auto &p = prepared.preview;
    auto &prefix = *prepared.active;
    const auto prior_prefix = prefix.revision;
    MilestoneEditResult result{p.next_definition_revision,
                               p.achievements.empty() ? prior_prefix
                                                      : facts.candidate_prefix_revision,
                               {}};
    GroupResultSize result_size;
    GroupResultSize pending_growth;
    for (std::size_t i = 0; i < p.achievements.size(); ++i) {
        const auto &capture = p.achievements[i];
        const auto choice = std::ranges::find(intent.choices, capture.occurrence,
                                              &MilestoneAcceptanceChoice::occurrence)
                                ->choice;
        const core::MilestoneCompletion completion{
            capture.milestone_ref, result.definition_revision,
            p.plan.newly_completed[i].target_micro_points, p.score, intent.key.operation_ref};
        const auto group_size = group_result_size(intent.key, capture.occurrence, completion,
                                                  capture.actions, capture.jobs.size());
        result_size.include(group_size);
        if (choice != MilestoneActionChoice::skip)
            pending_growth.include(group_size);
        MilestoneEditMembership membership{intent.key, {}};
        membership.jobs.reserve(capture.jobs.size());
        for (std::size_t action = 0; action < capture.actions.size(); ++action)
            if (capture.actions[action].kind == Kind::automation)
                membership.jobs.push_back({intent.key, capture.occurrence, qint64(action) + 1});
        result.achievements.push_back({{completion, capture.actions, capture.timing,
                                        capture.occurrence, std::move(membership)},
                                       ++prepared.tail,
                                       choice});
        prefix.members.push_back(completion);
        prefix.origins.push_back(
            {1, {}, {}, {}, blob(intent.key.operation_ref), blob(capture.occurrence.ref)});
    }

    if (pending_growth.groups != 0)
        pending_result_size(s).include(pending_growth);

    Encoding audit;
    audit.bytes(p.confirmation->intent);
    audit.bytes(p.confirmation->sources);
    audit.bytes(p.confirmation->captures);
    Encoding projection;
    const auto classification =
        classify(p.previous_targets, p.plan.targets, prefix.members.size() - p.achievements.size());
    projection.number(classification.unfinished);
    projection.number(classification.completed);
    targets_wire(projection, p.plan.targets);
    projection_wire(projection, p.plan.before);
    projection_wire(projection, p.plan.after);
    projection.number(p.plan.suppressed);
    projection.number(p.plan.automation_gated);
    audit.bytes(projection.data());
    const auto result_doc = result_wire(result, prior_prefix);

    insert_definition(s, intent.key.track,
                      {p.next_definition_revision, facts.configuration_ref,
                       p.prior_definition_revision, p.plan.targets});
    auto args = edit_args(intent.key);
    args.append({blob(p.prior_definition_revision), blob(p.next_definition_revision),
                 blob(intent.evidence.actor_ref), blob(intent.evidence.decision_ref),
                 blob(p.score_head), qint64(p.score), audit.data(), result_doc});
    execute(s, "INSERT INTO main.subathon_milestone_edit_operations VALUES(?,?,?,?,?,?,?,?,?,?,?)",
            args);
    for (std::size_t i = 0; i < p.achievements.size(); ++i)
        stage_occurrence(s, intent, result.achievements[i], p.achievements[i], qint64(i) + 1);
    advance_head(s, intent.key.track, true, p.next_definition_revision,
                 p.prior_definition_revision);
    if (!result.achievements.empty()) {
        prefix.revision = result.prefix_revision;
        insert_prefix(s, intent.key.track, prefix);
        advance_head(s, intent.key.track, false, prefix.revision, prior_prefix);
    }
    return result;
}

} // namespace

MigrationManifest milestone_target_edits_repository_manifest() {
    auto manifest = rate_overrides_repository_manifest();
    MigrationStep step{18, "milestone-target-edits-v1", {}, manifest.steps.back().expected_schema};
    for (const auto &table : edit_tables) {
        const auto name = QString::fromLatin1(table.name);
        const auto ddl = QString::fromLatin1(table.sql);
        step.sql.append(ddl);
        step.expected_schema.append({"table", name, name, ddl});
        for (int i = 1; i <= table.indexes; ++i)
            step.expected_schema.append(
                {"index", "sqlite_autoindex_" + name + "_" + QString::number(i), name, {}});
    }
    step.sql.append(QString::fromLatin1(edit_pending_sql));
    step.expected_schema.append(
        {"index", edit_pending_index, "subathon_milestone_edit_occurrences", edit_pending_sql});
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(18);
    return manifest;
}

bool initialize_milestone_track(const SqliteWorkSession &s, const MilestoneTrackSeed &seed) {
    return guarded(s, [&] {
        if (catalog(s) != Catalog::edits || !seed.completion_prefix ||
            !reference_fields({seed.track.installation_ref, seed.track.track_ref,
                               seed.configuration_ref, seed.definition_revision,
                               seed.prefix_revision}) ||
            seed.targets.size() > std::size_t(collection_limit) ||
            seed.completion_prefix->size() > seed.targets.size())
            return false;
        // Bootstrap accepts only explicit configuration and completion facts supplied
        // by the owning composition, with independently checked occurrence provenance.
        std::set<std::string_view> ids;
        for (std::size_t i = 0; i < seed.targets.size(); ++i) {
            const auto &target = seed.targets[i];
            if (target.milestone_ref.size() > core::MilestoneProgressionLimits{}.reference_bytes ||
                !bounded_ref(target.milestone_ref) || !ids.insert(target.milestone_ref).second ||
                (i && seed.targets[i - 1].target_micro_points >= target.target_micro_points))
                return false;
        }
        auto args = track_args(seed.track);
        args.append(track_args(seed.track));
        if (!s.execute({"SELECT 1 FROM main.subathon_milestone_definition_versions WHERE "
                        "installation_ref=? AND track_ref=? UNION ALL SELECT 1 "
                        "FROM main.subathon_milestone_prefix_revisions WHERE installation_ref=? "
                        "AND track_ref=? LIMIT 1",
                        args})
                 .empty())
            return false;
        Prefix initial{seed.prefix_revision, {}, {}};
        for (const auto &member : *seed.completion_prefix) {
            if (seed.targets[initial.members.size()].milestone_ref !=
                    member.completion.milestone_ref ||
                !application_member(s, member))
                return false;
            initial.members.push_back(member.completion);
            initial.origins.push_back({0,
                                       blob(member.application.operation_scope),
                                       blob(member.application.operation_ref),
                                       blob(member.occurrence.ref),
                                       {},
                                       {}});
        }
        insert_definition(s, seed.track,
                          {seed.definition_revision, seed.configuration_ref, {}, seed.targets});
        insert_prefix(s, seed.track, initial);
        advance_head(s, seed.track, true, seed.definition_revision);
        advance_head(s, seed.track, false, seed.prefix_revision);
        return true;
    });
}

MilestoneEditPreview preview_milestone_target_edit(const SqliteWorkSession &s,
                                                   MilestoneEditAuthority &authority,
                                                   const MilestoneEditActor &actor,
                                                   const MilestoneEditIntent &intent) {
    return guarded(s, [&] {
        if (!valid_intent(actor, intent))
            return refusal(Status::invalid_patch);
        if (catalog(s) != Catalog::edits)
            return refusal(Status::unavailable);
        const auto facts = authority.select(s, actor, intent);
        if (!facts || !facts->disclosure_grant)
            return refusal(Status::denied);
        if (!valid_facts(*facts))
            return refusal(Status::unavailable);
        if (operation(s, intent.key))
            return refusal(Status::key_conflict);
        return prepare(s, actor, intent, *facts).preview;
    });
}

MilestoneEditReply confirm_milestone_target_edit(const SqliteWorkSession &s,
                                                 MilestoneEditAuthority &authority,
                                                 const MilestoneEditActor &actor,
                                                 const MilestoneEditIntent &intent,
                                                 const MilestoneEditBinding &binding) {
    return guarded(s, [&]() -> MilestoneEditReply {
        if (!valid_intent(actor, intent))
            return {Status::invalid_patch, {}};
        if (catalog(s) != Catalog::edits)
            return {Status::unavailable, {}};
        const auto facts = authority.select(s, actor, intent);
        if (!facts || !facts->disclosure_grant)
            return {Status::denied, {}};
        if (!valid_facts(*facts))
            return {Status::unavailable, {}};
        const auto intent_wire = identity(actor, intent);
        if (const auto old = operation(s, intent.key))
            return reconcile(s, intent, *facts, intent_wire, *old, &binding);
        if (intent_wire != binding.intent)
            return {Status::stale, {}};

        auto prepared = prepare(s, actor, intent, *facts, &binding.sources);
        if (prepared.preview.plan.status == Status::denied)
            return {Status::denied, {}};
        if (prepared.sources != binding.sources || !prepared.preview.confirmation ||
            prepared.preview.confirmation->captures != binding.captures)
            return {Status::stale, {}};
        return {Status::eligible, stage(s, intent, *facts, std::move(prepared))};
    });
}

std::optional<MilestoneEditResult> read_milestone_target_edit(const SqliteWorkSession &s,
                                                              MilestoneEditAuthority &authority,
                                                              const MilestoneEditActor &actor,
                                                              const MilestoneEditIntent &intent) {
    return guarded(s, [&]() -> std::optional<MilestoneEditResult> {
        if (!valid_intent(actor, intent) || catalog(s) != Catalog::edits)
            return {};
        const auto facts = authority.select(s, actor, intent);
        if (!facts || !facts->disclosure_grant || !valid_facts(*facts))
            return {};
        const auto old = operation(s, intent.key);
        if (!old)
            return {};
        return reconcile(s, intent, *facts, identity(actor, intent), *old, nullptr).result;
    });
}

StorageError release_milestone_head(SqliteWriter &writer, MilestoneOccurrenceKey key,
                                    std::shared_ptr<MilestoneEditReleaseAuthority> authority,
                                    MilestoneReleaseCallback completion) {
    if (!bounded_ref(key.ref))
        return StorageError::invalid_options;
    if (!authority || !completion)
        return StorageError::invalid_work;
    return writer.enqueue_work(
        [key = std::move(key),
         authority = std::move(authority)](SqliteWorkContext context) -> QVariant {
            const auto session = context.begin();
            auto status = Release::refused;
            if (catalog(session) == Catalog::edits) {
                const auto head = pending_head(session);
                const auto ref = blob(key.ref);
                if (head && head->second && head->first == ref &&
                    SqliteMilestoneGroupDecisions(session).pending_disposition(key) ==
                        MilestonePendingDisposition::ready) {
                    const auto selected = occurrence(session, ref);
                    if (!selected)
                        corrupt();
                    if (authority->allows(session, selected->edit.track, key)) {
                        execute(session,
                                "UPDATE main.subathon_milestone_edit_occurrences SET released=1 "
                                "WHERE occurrence_ref=? AND released=0",
                                {ref});
                        exactly_one(session);
                        status = Release::released;
                    }
                }
            }
            session.commit();
            return int(status);
        },
        [completion = std::move(completion)](SqliteWorkResult work) {
            std::optional<Release> disposition;
            if (work.transaction == TransactionOutcome::committed &&
                work.error == WorkError::none && work.value)
                disposition = Release(work.value->toInt());
            completion({std::move(work), disposition});
        });
}

namespace {

void validate_job_key(const MilestoneEditJobKey &key) {
    if (key.action_ordinal < 1 ||
        !reference_fields({key.operation.track.installation_ref, key.operation.track.track_ref,
                           key.operation.operation_ref, key.occurrence.ref}))
        throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_key);
}

std::optional<MilestoneEditJob> select_job(const SqliteWorkSession &s,
                                           const MilestoneEditJobKey &key, bool body) {
    if (catalog(s) != Catalog::edits)
        return {};
    const auto selected = occurrence(s, blob(key.occurrence.ref), false, &key.operation);
    if (!selected)
        return {};
    return job(s, *selected, key.action_ordinal, body);
}

MilestoneEditJobResult transition(const SqliteWorkSession &s, MilestoneEditJob job, State next,
                                  const std::optional<std::string> &evidence = {}) {
    if (job.revision == std::numeric_limits<std::int64_t>::max())
        throw SqliteBundleStorageFailure(SqliteBundleStorageReason::sequence_exhausted);
    ++job.revision;
    job.state = next;
    if (next == State::sending)
        job.attempt = job.revision;
    else if (next == State::queued)
        job.attempt.reset();
    execute(s, "INSERT INTO main.subathon_milestone_edit_events VALUES(?,?,?,?,?,?)",
            {blob(job.key.occurrence.ref), qint64(job.key.action_ordinal), qint64(job.revision),
             int(next), job.attempt ? QVariant(qint64(*job.attempt)) : QVariant{},
             evidence ? QVariant(blob(*evidence)) : QVariant{}});
    return {Disposition::changed, std::move(job)};
}

template <class Change>
MilestoneEditJobResult change(const SqliteWorkSession &s, const MilestoneEditJobKey &key,
                              std::int64_t revision, State expected, Change change_job) {
    validate_job_key(key);
    if (revision < 1)
        invalid();
    return guarded(s, [&]() -> MilestoneEditJobResult {
        auto selected = select_job(s, key, true);
        if (!selected)
            return {Disposition::missing, {}};
        if (selected->revision != revision)
            return {Disposition::stale, {}};
        if (selected->state != expected)
            return {Disposition::refused, {}};
        return change_job(std::move(*selected));
    });
}

} // namespace

std::optional<MilestoneEditJob> lookup_milestone_edit_job(const SqliteWorkSession &s,
                                                          const MilestoneEditJobKey &key) {
    validate_job_key(key);
    return guarded(s, [&] { return select_job(s, key, true); });
}

MilestoneEditJobResult claim_milestone_edit_job(const SqliteWorkSession &s,
                                                const MilestoneEditJobKey &key,
                                                std::int64_t revision,
                                                MilestoneEditDispatchAuthority &authority) {
    return change(s, key, revision, State::queued, [&](MilestoneEditJob job) {
        const auto selected = occurrence(s, blob(key.occurrence.ref));
        if (!selected || !selected->released || !authority.allows(job))
            return MilestoneEditJobResult{Disposition::refused, {}};
        return transition(s, std::move(job), State::sending);
    });
}

MilestoneEditJobResult finish_milestone_edit_job(const SqliteWorkSession &s,
                                                 const MilestoneEditJobKey &key,
                                                 std::int64_t revision, std::int64_t attempt,
                                                 DeliveryEvidence evidence,
                                                 const std::string &evidence_ref) {
    const auto next = [&] {
        switch (evidence) {
        case DeliveryEvidence::delivered:
            return State::delivered;
        case DeliveryEvidence::known_undelivered:
            return State::failed;
        case DeliveryEvidence::indeterminate:
            return State::outcome_unknown;
        }
        invalid();
    }();
    if (attempt < 1 || !bounded_ref(evidence_ref))
        invalid();
    return change(s, key, revision, State::sending, [&](MilestoneEditJob job) {
        if (job.attempt != attempt)
            return MilestoneEditJobResult{Disposition::stale, {}};
        return transition(s, std::move(job), next, evidence_ref);
    });
}

MilestoneEditJobResult retry_milestone_edit_job(const SqliteWorkSession &s,
                                                const MilestoneEditJobKey &key,
                                                std::int64_t revision,
                                                MilestoneEditDispatchAuthority &authority) {
    return change(s, key, revision, State::failed, [&](MilestoneEditJob job) {
        auto policy = authority.retry_policy(job);
        if (!policy)
            return MilestoneEditJobResult{Disposition::refused, {}};
        return transition(s, std::move(job), State::queued, policy);
    });
}

MilestoneStrandedPage stranded_milestone_delivery(const SqliteWorkSession &s,
                                                  const std::optional<MilestoneDeliveryKey> &after,
                                                  std::size_t limit) {
    if (limit == 0 || limit > stranded_delivery_page_limit)
        throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_options);
    if (after)
        if (const auto *key = std::get_if<MilestoneEditJobKey>(&*after))
            validate_job_key(*key);

    return guarded(s, [&] {
        const auto current = catalog(s);
        const auto *edit_after = after ? std::get_if<MilestoneEditJobKey>(&*after) : nullptr;
        MilestoneStrandedPage page;
        std::size_t examined = 0;
        if (!edit_after) {
            std::optional<DeliveryJobKey> cursor;
            if (after)
                cursor = std::get<DeliveryJobKey>(*after);
            const SqliteDeliveryJobStore store(s);
            for (; examined < limit; ++examined) {
                auto application = store.stranded(cursor, 1);
                for (auto &attempt : application.attempts)
                    page.attempts.push_back({std::move(attempt.key), attempt.sending_revision});
                if (!application.next_after)
                    break;
                cursor = std::move(application.next_after);
                if (examined + 1 == limit) {
                    page.next_after = *cursor;
                    return page;
                }
            }
        }
        if (current != Catalog::edits)
            return page;
        QString sql = "SELECT " + ref_column("installation_ref") + "," + ref_column("track_ref") +
                      "," + ref_column("edit_ref") + "," + ref_column("occurrence_ref") +
                      ",action_ordinal FROM main.subathon_milestone_edit_jobs";
        QList<QVariant> args;
        if (edit_after) {
            sql += " WHERE (installation_ref,track_ref,edit_ref,occurrence_ref,action_ordinal)"
                   ">(?,?,?,?,?)";
            args = edit_args(edit_after->operation);
            args.append({blob(edit_after->occurrence.ref), qint64(edit_after->action_ordinal)});
        }
        sql +=
            " ORDER BY installation_ref,track_ref,edit_ref,occurrence_ref,action_ordinal LIMIT ?";
        args.append(qint64(limit - examined));
        const auto rows = s.execute({sql, args});
        for (const auto &row : rows) {
            MilestoneEditJobKey key{{{reference(row[0]), reference(row[1])}, reference(row[2])},
                                    {reference(row[3])},
                                    integer(row[4])};
            if (key.action_ordinal < 1)
                corrupt();
            const auto selected = select_job(s, key, false);
            if (!selected)
                corrupt();
            if (selected->state == State::sending)
                page.attempts.push_back({key, selected->revision});
            if (++examined == limit)
                page.next_after = std::move(key);
        }
        return page;
    });
}

MilestoneEditJobResult recover_milestone_edit_job(const SqliteWorkSession &s,
                                                  const MilestoneEditJobKey &key,
                                                  std::int64_t revision) {
    return change(s, key, revision, State::sending, [&](MilestoneEditJob job) {
        return transition(s, std::move(job), State::outcome_unknown, "interrupted-sending");
    });
}

} // namespace subathon::server
