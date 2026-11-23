// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_milestone_interruption_detail.hpp"

#include <QUuid>
#include <QtEndian>
#include <limits>
#include <set>

namespace subathon::server::milestone_interruption {
namespace {

bool acknowledged(const SqliteWorkResult &work) {
    return work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
           work.value.has_value();
}

std::optional<ReplacementJob> acknowledged_claim(const SqliteWorkResult &work,
                                                 std::optional<Disposition> disposition,
                                                 std::optional<ReplacementJob> &owned) {
    if (acknowledged(work) && disposition == Disposition::committed)
        return std::exchange(owned, {});
    return {};
}

} // namespace

void adopt_milestone_stop_completion(MilestoneTimeline &timeline, std::chrono::milliseconds now,
                                     const Command &command, MilestoneStopCompletionSource source,
                                     const SqliteWorkResult &work,
                                     const std::optional<Decision> &published) {
    const auto *stop = std::get_if<StopIntent>(&command.intent);
    if (!stop || !detail::valid(command.track, stop->target) || !detail::ref(command.operation) ||
        (stop->choice && *stop->choice != QueueChoice::hold &&
         *stop->choice != QueueChoice::continue_queue))
        return;
    const auto normal = acknowledged(work) && published.has_value();
    const auto from_readback = source == MilestoneStopCompletionSource::readback;
    if (normal && (published->disposition == Disposition::committed ||
                   (from_readback && published->disposition == Disposition::replay))) {
        std::optional<ScopedCause> hold;
        if (!published->added_causes.empty())
            hold = ScopedCause{command.track, published->added_causes.front()};
        adopt_milestone_timeline_update(
            timeline, now, AcknowledgedStop{stop->target, command.operation, hold, source});
        return;
    }
    const auto unresolved = !normal && (work.transaction == TransactionOutcome::outcome_unknown ||
                                        work.transaction == TransactionOutcome::committed);
    const auto absent = normal && from_readback && published->disposition == Disposition::refused;
    timeline.observe_stop_completion(now, detail::timeline_target(stop->target),
                                     command.operation.toStdString(), unresolved, absent);
}

void adopt_ordered_milestone_selection(MilestoneTimeline &timeline, std::chrono::milliseconds now,
                                       MilestoneQueueAdoption adoption,
                                       const OccurrenceSelection &selected,
                                       const std::vector<CommittedMilestoneGroup> &pending) {
    if (adoption == MilestoneQueueAdoption::cold)
        timeline.restore_committed(pending);
    if (selected.stop.isEmpty())
        return;
    std::vector<MilestoneTimelineCause> holds;
    holds.reserve(selected.interruption_holds.size());
    for (const auto &hold : selected.interruption_holds)
        holds.push_back({selected.target.track, hold.toStdString()});
    timeline.adopt_ordered_stop(
        now, detail::timeline_target(selected.target), selected.stop.toStdString(),
        !selected.skipped && !selected.independent_holds.empty(), std::move(holds));
}

namespace detail {

bool valid(const MilestoneTrackKey &track, const Occurrence &o) {
    using milestone_sql::bounded_ref;
    if (o.track != track || !bounded_ref(o.occurrence.ref))
        return false;
    if (auto app = std::get_if<ApplicationOperationKey>(&o.origin))
        return bounded_ref(app->operation_scope) && bounded_ref(app->operation_ref);
    const auto &edit = std::get<MilestoneEditKey>(o.origin);
    return edit.track == track && bounded_ref(edit.operation_ref);
}

Origin origin(const Occurrence &o, const AdmissionProfile *profile) {
    if (auto app = std::get_if<ApplicationOperationKey>(&o.origin))
        return {1, blob(app->operation_scope), blob(app->operation_ref), blob(o.occurrence.ref)};
    const auto &edit = std::get<MilestoneEditKey>(o.origin);
    const auto *bytes = fixture(profile);
    const auto edit_scope = bytes ? bytes->edit_origin_scope : blob(o.track.track_ref);
    if (!ref(edit_scope))
        corrupt();
    return {2, edit_scope, blob(edit.operation_ref), blob(o.occurrence.ref)};
}

bool same(const Occurrence &a, const Occurrence &b) {
    if (a.track != b.track || a.occurrence != b.occurrence || a.origin.index() != b.origin.index())
        return false;
    if (auto app = std::get_if<ApplicationOperationKey>(&a.origin)) {
        const auto &other = std::get<ApplicationOperationKey>(b.origin);
        return app->operation_scope == other.operation_scope &&
               app->operation_ref == other.operation_ref;
    }
    return std::get<MilestoneEditKey>(a.origin) == std::get<MilestoneEditKey>(b.origin);
}

void require_catalog(const SqliteWorkSession &s) {
    const auto store = s.execute({"SELECT singleton,catalog_format FROM subathon_catalog "
                                  "WHERE singleton=1 AND store_identity=? LIMIT 2",
                                  {QString("subathon.sqlite-store")}});
    const auto *row = unique(store);
    if (!row || integer((*row)[0]) != 1 || integer((*row)[1]) != 1)
        corrupt();
    const auto marker =
        s.execute({"SELECT CASE WHEN typeof(version)='integer' AND typeof(migration_id)='text' "
                   "AND version=? AND migration_id=? COLLATE BINARY THEN 1 ELSE 0 END "
                   "FROM subathon_migration_history ORDER BY version DESC LIMIT 1",
                   {21, QString("milestone-slot-interruption-storage-v1")}});
    if (marker.size() != 1 || integer(marker[0][0]) != 1)
        corrupt();
    (void)s.execute({milestone_interruption_schema::probe, {}});
}

QByteArray allocate(const QByteArray &fixture_value) {
    return fixture_value.isNull() ? QUuid::createUuid().toRfc4122() : fixture_value;
}

QByteArray numeric_payload(qint64 value) {
    const auto raw = qToBigEndian(quint64(value));
    return QByteArray(reinterpret_cast<const char *>(&raw), sizeof(raw));
}

std::optional<Target> target(const SqliteWorkSession &s, const Occurrence &o, const Origin &key,
                             bool include_terminal) {
    constexpr auto edit_sql =
        "SELECT g.released,g.queue_order,g.edit_ref,d.definition_revision,h.prefix_revision,"
        "c.choice,c.resolution,EXISTS(SELECT 1 FROM subathon_milestone_edit_refund_decisions r "
        "WHERE r.occurrence_ref=g.occurrence_ref AND (r.policy=2 OR r.resolution=1)) "
        "FROM subathon_milestone_edit_occurrences g "
        "JOIN subathon_milestone_definition_heads d USING(installation_ref,track_ref) "
        "JOIN subathon_milestone_prefix_heads h USING(installation_ref,track_ref) "
        "JOIN subathon_milestone_edit_choices c USING(occurrence_ref) "
        "WHERE g.installation_ref=? AND g.track_ref=? AND g.edit_ref=? AND g.occurrence_ref=? "
        "LIMIT 2";
    constexpr auto app_sql =
        "SELECT g.released,g.queue_order,g.operation_ref,d.definition_revision,h.prefix_revision,"
        "c.choice,c.resolution,EXISTS(SELECT 1 FROM subathon_milestone_refund_decisions r "
        "WHERE r.occurrence_ref=g.occurrence_ref AND (r.policy=2 OR r.resolution=1)) "
        "FROM subathon_milestone_groups g "
        "LEFT JOIN subathon_milestone_acceptance_decisions c USING(occurrence_ref) "
        "JOIN subathon_milestone_prefix_members m ON m.application_occurrence_ref=g.occurrence_ref "
        "AND m.operation_scope=g.operation_scope AND m.operation_ref=g.operation_ref "
        "JOIN subathon_milestone_prefix_heads h USING(installation_ref,track_ref,prefix_revision) "
        "JOIN subathon_milestone_definition_heads d USING(installation_ref,track_ref) "
        "WHERE m.installation_ref=? AND m.track_ref=? AND g.operation_scope=? "
        "AND g.operation_ref=? AND g.occurrence_ref=? LIMIT 2";
    auto args = scope(o.track);
    if (key.kind == 1)
        args.append(key.scope);
    args.append({key.operation, key.occurrence});
    const auto rows = s.execute({key.kind == 2 ? edit_sql : app_sql, args});
    const auto *row = unique(rows);
    if (!row)
        return {};
    const auto released = integer((*row)[0]);
    const auto order = integer((*row)[1]);
    if (released < 0 || released > 1 || order < 1 || verified_ref((*row)[2]) != key.operation)
        corrupt();
    const auto choice = (*row)[5].isNull() ? 0 : integer((*row)[5]);
    const auto resolution = (*row)[6].isNull() ? -1 : integer((*row)[6]);
    const auto skipped = choice == 2 || resolution == 1 || integer((*row)[7]) == 1;
    if (skipped && !include_terminal)
        return {};
    const auto fence = s.execute(
        {include_terminal
             ? "SELECT stop FROM subathon_milestone_stop_fences WHERE installation=? AND track=? "
               "AND origin_kind=? AND origin_scope=? AND origin_ref=? AND occurrence=? LIMIT 2"
             : "SELECT 1 FROM subathon_milestone_stop_fences WHERE installation=? AND track=? "
               "AND origin_kind=? AND origin_scope=? AND origin_ref=? AND occurrence=? LIMIT 2",
         key.at(o.track)});
    const auto *stopped = unique(fence);
    if (stopped && !include_terminal)
        return {};
    const auto reviews = s.execute(
        {key.kind == 2 ? "SELECT length(refund_ref) FROM subathon_milestone_edit_refund_decisions "
                         "WHERE occurrence_ref=? AND policy=1 AND resolution IS NULL AND "
                         "ended_by_skip_ref IS NULL LIMIT ?"
                       : "SELECT length(refund_ref) FROM subathon_milestone_refund_decisions "
                         "WHERE occurrence_ref=? AND policy=1 AND resolution IS NULL AND "
                         "ended_by_skip_ref IS NULL LIMIT ?",
         {key.occurrence, member_limit + 1}});
    if (reviews.size() > member_limit)
        throw SqliteWorkFailure(WorkError::result_limit);
    for (const auto &review : reviews)
        if (integer(review.front()) < 1 || integer(review.front()) > 4096)
            corrupt();
    return Target{order,
                  released == 1,
                  skipped,
                  verified_ref((*row)[3]),
                  verified_ref((*row)[4]),
                  stopped ? verified_ref(stopped->front()) : QByteArray{},
                  choice == 1 && resolution == -1,
                  reviews.size()};
}

std::optional<Slot> slot(const SqliteWorkSession &s, const Occurrence &o, const Origin &key,
                         qint64 ordinal) {
    const QList<QVariant> original{key.occurrence, ordinal};
    const auto rows = s.execute(
        {key.kind == 2
             ? "SELECT "
               "a.kind,length(a.action_ref),length(a.version_ref),length(j.action_document),"
               "length(c.route_ref),length(c.sender_ref),(SELECT max(e.revision) "
               "FROM subathon_milestone_edit_events e WHERE e.occurrence_ref=a.occurrence_ref "
               "AND e.action_ordinal=a.action_ordinal),EXISTS(SELECT 1 FROM "
               "subathon_milestone_edit_bonus_receipts b "
               "WHERE b.occurrence_ref=a.occurrence_ref AND b.action_ordinal=a.action_ordinal) "
               "FROM subathon_milestone_edit_actions a LEFT JOIN subathon_milestone_edit_jobs j "
               "USING(occurrence_ref,action_ordinal) LEFT JOIN subathon_milestone_edit_captures c "
               "USING(occurrence_ref,action_ordinal) WHERE a.occurrence_ref=? AND "
               "a.action_ordinal=? LIMIT 2"
             : "SELECT "
               "a.kind,length(a.action_ref),length(a.version_ref),length(a.job_ref),NULL,NULL,"
               "(SELECT max(e.revision) FROM subathon_delivery_events e WHERE "
               "e.operation_scope=a.operation_scope AND e.operation_ref=a.operation_ref "
               "AND e.job_ref=a.job_ref),EXISTS(SELECT 1 FROM subathon_milestone_bonus_receipts b "
               "WHERE b.occurrence_ref=a.occurrence_ref AND b.action_ordinal=a.action_ordinal) "
               "FROM subathon_milestone_actions a WHERE a.occurrence_ref=? AND a.action_ordinal=? "
               "LIMIT 2",
         original});
    const auto *row = unique(rows);
    if (!row)
        return {};
    const auto kind = milestone_edit_sql::enumeration((*row)[0], MilestoneActionKind::bonus);
    for (auto col : {1, 2})
        if (integer((*row)[col]) < 1 || integer((*row)[col]) > 4096)
            corrupt();
    auto args = key.at(o.track);
    args.append(ordinal);
    const auto selected = s.execute(
        {"SELECT length(version),length(payload),length(route),length(qualified_sender) "
         "FROM subathon_milestone_slot_versions v WHERE installation=? AND track=? AND "
         "origin_kind=? "
         "AND origin_scope=? AND origin_ref=? AND occurrence=? AND ordinal=? AND NOT EXISTS("
         "SELECT 1 FROM subathon_milestone_slot_versions n WHERE n.installation=v.installation "
         "AND n.track=v.track AND n.origin_kind=v.origin_kind AND n.origin_scope=v.origin_scope "
         "AND n.origin_ref=v.origin_ref AND n.occurrence=v.occurrence AND n.ordinal=v.ordinal "
         "AND n.predecessor=v.version) LIMIT 2",
         args});
    if (!unique(selected)) {
        if (kind == MilestoneActionKind::automation && ((*row)[3].isNull() || (*row)[6].isNull()))
            corrupt();
        return Slot{kind, {}, (*row)[6], (*row)[3], (*row)[4], (*row)[5], integer((*row)[7]) == 1};
    }

    const auto heads = s.execute(
        {"SELECT version,kind,(SELECT max(revision) FROM subathon_milestone_slot_attempts a "
         "WHERE (a.installation,a.track,a.version)=(v.installation,v.track,v.version)),"
         "EXISTS(SELECT 1 FROM subathon_milestone_version_bonus b WHERE "
         "(b.installation,b.track,b.version)=(v.installation,v.track,v.version)) "
         "FROM subathon_milestone_slot_versions v WHERE installation=? AND track=? AND "
         "origin_kind=? "
         "AND origin_scope=? AND origin_ref=? AND occurrence=? AND ordinal=? AND NOT EXISTS("
         "SELECT 1 FROM subathon_milestone_slot_versions n WHERE "
         "(n.installation,n.track,n.origin_kind,n.origin_scope,n.origin_ref,n.occurrence,n.ordinal,"
         "n.predecessor)=(v.installation,v.track,v.origin_kind,v.origin_scope,v.origin_ref,"
         "v.occurrence,v.ordinal,v.version)) LIMIT 2",
         args});
    const auto *head = unique(heads);
    if (!head || integer((*head)[1]) != int(kind) + 1 || integer((*row)[7]) != 0 ||
        (!(*row)[6].isNull() && integer((*row)[6]) != 1) ||
        (kind == MilestoneActionKind::automation && (*head)[2].isNull()))
        corrupt();
    const auto &sizes = selected.front();
    if (integer(sizes[0]) != verified_ref((*head)[0]).size())
        corrupt();
    return Slot{kind,     verified_ref((*head)[0]), (*head)[2], sizes[1], sizes[2],
                sizes[3], integer((*head)[3]) == 1};
}

bool started(const Slot &slot) {
    return slot.applied || (!slot.revision.isNull() && integer(slot.revision) > 1);
}

std::optional<qint64> attempt_state(const SqliteWorkSession &s, const Occurrence &o,
                                    const Origin &key, qint64 ordinal, const QByteArray &version,
                                    qint64 revision, bool playback, AttemptCharge *charge) {
    const auto execute = [&](const SqlStatement &statement) {
        auto result = s.execute(statement);
        if (charge) {
            ++charge->statements;
            charge->peak = std::max(charge->peak, statement.sql.toUtf8().size());
            const auto width = [](const QVariant &cell) {
                return 8 + (cell.metaType().id() == QMetaType::QByteArray
                                ? cell.toByteArray().size()
                                : qsizetype{0});
            };
            for (const auto &binding : statement.bindings)
                charge->binds += width(binding);
            charge->rows += result.size();
            for (const auto &row : result)
                for (const auto &cell : row)
                    charge->values += width(cell);
            charge->version_bytes = version.size();
        }
        return result;
    };
    SqlRows rows;
    if (!version.isEmpty()) {
        auto args = key.at(o.track);
        args.append({ordinal, version, revision});
        rows = execute(
            {"SELECT a.revision,a.state,a.revision FROM subathon_milestone_slot_attempts a "
             "JOIN subathon_milestone_slot_versions v USING(installation,track,version) "
             "WHERE v.installation=? AND v.track=? AND v.origin_kind=? AND v.origin_scope=? "
             "AND v.origin_ref=? AND v.occurrence=? AND v.ordinal=? AND v.version=? "
             "AND a.revision=? LIMIT 2",
             args});
    } else if (key.kind == 2) {
        rows = execute({"SELECT revision,state,attempt FROM subathon_milestone_edit_events "
                        "WHERE occurrence_ref=? AND action_ordinal=? AND revision=? LIMIT 2",
                        {key.occurrence, ordinal, revision}});
    } else {
        const auto jobs = execute({"SELECT job_ref FROM subathon_milestone_actions "
                                   "WHERE occurrence_ref=? AND action_ordinal=? "
                                   "AND operation_scope=? AND operation_ref=? LIMIT 2",
                                   {key.occurrence, ordinal, key.scope, key.operation}});
        const auto *job = unique(jobs);
        if (!job || (*job)[0].isNull())
            return {};
        rows = execute(
            {"SELECT revision,state,attempt FROM subathon_delivery_events "
             "WHERE operation_scope=? AND operation_ref=? AND job_ref=? AND revision=? LIMIT 2",
             {key.scope, key.operation, verified_ref((*job)[0]), revision}});
    }
    const auto *row = unique(rows);
    if (!row)
        return {};
    if (integer((*row)[0]) != revision)
        corrupt();
    const auto state = integer((*row)[1]);
    if (state < 0 || state > 4)
        corrupt();
    if (playback && (state != int(DeliveryJobState::sending) || (*row)[2].isNull() ||
                     integer((*row)[2]) != revision))
        return {};
    return state;
}

} // namespace detail

namespace {
using namespace detail;

enum class Kind : char { replace = 1, stop, resume };

class Frame {
  public:
    explicit Frame(QByteArrayView magic) : data(magic.toByteArray()) {}
    QByteArray data;

    void raw(QByteArrayView bytes) {
        if (bytes.size() > document_limit - data.size())
            throw SqliteWorkFailure(WorkError::resource_limit);
        data.append(bytes.data(), bytes.size());
    }
    void tag(char n) { raw(QByteArrayView(&n, 1)); }
    void number(qint64 n) { raw(numeric_payload(n)); }
    void count(qsizetype n) {
        const auto bytes = qToBigEndian(quint32(n));
        raw(QByteArrayView(reinterpret_cast<const char *>(&bytes), sizeof bytes));
    }
    void packet(QByteArrayView bytes) {
        count(bytes.size());
        raw(bytes);
    }
    void reference(QByteArrayView value) {
        if (!ref(value))
            corrupt();
        packet(value);
    }
    void text(const std::string &value) { reference(blob(value)); }
    void version(QByteArrayView value) {
        tag(value.isEmpty() ? 0 : 1);
        if (!value.isEmpty())
            reference(value);
    }
};

class ReadFrame {
  public:
    explicit ReadFrame(QByteArrayView data) : rest(data) {}
    QByteArrayView pop(qsizetype size) {
        if (size < 0 || size > rest.size())
            corrupt();
        const auto value = rest.first(size);
        rest = rest.sliced(size);
        return value;
    }
    char tag() { return pop(1)[0]; }
    qint64 number() { return qFromBigEndian<qint64>(pop(8).data()); }
    qsizetype count() {
        const auto value = qFromBigEndian<quint32>(pop(4).data());
        if (value > member_limit)
            corrupt();
        return value;
    }
    QByteArray packet(qsizetype limit, bool allow_empty) {
        const auto size = qFromBigEndian<quint32>(pop(4).data());
        if (size > limit || (!allow_empty && size == 0))
            corrupt();
        return pop(size).toByteArray();
    }
    QByteArray reference() { return packet(4096, false); }
    QByteArray version() {
        const auto present = tag();
        if (present == 0)
            return {};
        if (present != 1)
            corrupt();
        return reference();
    }
    void end() const {
        if (!rest.isEmpty())
            corrupt();
    }

  private:
    QByteArrayView rest;
};

Kind command_kind(const Command &cmd) {
    if (std::holds_alternative<StopIntent>(cmd.intent))
        return Kind::stop;
    return std::holds_alternative<ReplaceIntent>(cmd.intent) ? Kind::replace : Kind::resume;
}

bool valid_command(const Command &cmd) {
    using milestone_sql::bounded_ref;
    if (!bounded_ref(cmd.track.installation_ref) || !bounded_ref(cmd.track.track_ref) ||
        !ref(cmd.operation) || !bounded_ref(cmd.actor.actor_ref) ||
        !bounded_ref(cmd.actor.session_lineage))
        return false;
    if (const auto *stop = std::get_if<StopIntent>(&cmd.intent))
        return valid(cmd.track, stop->target) &&
               (!stop->choice || stop->choice == QueueChoice::hold ||
                stop->choice == QueueChoice::continue_queue);
    if (const auto *replace = std::get_if<ReplaceIntent>(&cmd.intent)) {
        if (!valid(cmd.track, replace->target) || replace->patch.empty() ||
            replace->patch.size() > member_limit)
            return false;
        std::set<qint64> ordinals;
        for (const auto &p : replace->patch)
            if (p.ordinal < 1 ||
                (p.action.kind != MilestoneActionKind::automation &&
                 p.action.kind != MilestoneActionKind::bonus) ||
                !bounded_ref(p.action.action_ref) || !bounded_ref(p.action.version_ref) ||
                !ordinals.insert(p.ordinal).second)
                return false;
        return true;
    }
    const auto &causes = std::get<ResumeIntent>(cmd.intent).causes;
    if (causes.empty() || causes.size() > member_limit)
        return false;
    std::set<QByteArrayView> refs;
    for (const auto &entry : causes)
        if (entry.track != cmd.track || !ref(entry.ref) || !refs.insert(entry.ref).second)
            return false;
    return true;
}

QList<QVariant> journal_key(const Command &cmd) {
    auto args = scope(cmd.track);
    args.append(cmd.operation);
    return args;
}

Frame intent_prefix(const Command &cmd) {
    Frame out("MSI1");
    out.tag(char(command_kind(cmd)));
    out.text(cmd.track.installation_ref);
    out.text(cmd.track.track_ref);
    out.reference(cmd.operation);
    return out;
}

void target_fields(Frame &out, const Occurrence &o) {
    if (const auto *app = std::get_if<ApplicationOperationKey>(&o.origin)) {
        out.tag(1);
        out.text(app->operation_scope);
        out.text(app->operation_ref);
    } else {
        out.tag(2);
        out.text(std::get<MilestoneEditKey>(o.origin).operation_ref);
    }
    out.text(o.occurrence.ref);
}

void references(Frame &out, std::vector<QByteArray> refs) {
    std::ranges::sort(refs);
    if (std::adjacent_find(refs.begin(), refs.end()) != refs.end())
        corrupt();
    out.count(qsizetype(refs.size()));
    for (const auto &r : refs)
        out.reference(r);
}

QByteArray simple_intent(const Command &cmd) {
    auto out = intent_prefix(cmd);
    if (const auto *stop = std::get_if<StopIntent>(&cmd.intent)) {
        target_fields(out, stop->target);
        out.tag(stop->choice == QueueChoice::continue_queue ? 2 : 1);
    } else {
        std::vector<QByteArray> refs;
        for (const auto &c : std::get<ResumeIntent>(cmd.intent).causes)
            refs.push_back(c.ref);
        references(out, std::move(refs));
    }
    return out.data;
}

void capture(Frame &out, const AuthorizedCapture &c) {
    const auto bonus = c.selection.action.kind == MilestoneActionKind::bonus;
    if (bonus != c.bonus_ms.has_value())
        corrupt();
    out.tag(bonus ? 2 : 1);
    out.text(c.selection.action.action_ref);
    out.text(c.selection.action.version_ref);
    if (bonus)
        out.number(*c.bonus_ms);
    else {
        out.packet(c.payload);
        out.text(c.route);
        out.text(c.qualified_sender);
    }
}

MilestoneAction read_capture(ReadFrame &in) {
    const auto kind = in.tag();
    auto action = in.reference().toStdString();
    auto version = in.reference().toStdString();
    if (kind == 2) {
        if (in.number() < 0)
            corrupt();
    } else if (kind == 1) {
        (void)in.packet(automation_limit, true);
        (void)in.reference();
        (void)in.reference();
    } else
        corrupt();
    return {kind == 2 ? MilestoneActionKind::bonus : MilestoneActionKind::automation,
            std::move(action), std::move(version)};
}

bool matches_replace(const Command &cmd, const QByteArray &saved) {
    const auto &replace = std::get<ReplaceIntent>(cmd.intent);
    auto prefix = intent_prefix(cmd);
    target_fields(prefix, replace.target);
    if (!saved.startsWith(prefix.data))
        return false;
    ReadFrame in(QByteArrayView(saved).sliced(prefix.data.size()));
    for (int field = 0; field < 4; ++field)
        (void)in.reference();
    if (const auto released = in.tag(); released != 0 && released != 1)
        corrupt();
    const auto holds = in.count();
    for (qsizetype i = 0; i < holds; ++i) {
        if (const auto type = in.tag(); type != 1 && type != 2)
            corrupt();
        (void)in.reference();
        (void)in.reference();
    }
    const auto count = in.count();
    if (count != std::ssize(replace.patch))
        return false;
    auto patch = replace.patch;
    std::ranges::sort(patch, {}, &Replacement::ordinal);
    for (const auto &p : patch) {
        const auto ordinal = in.number();
        (void)in.version();
        if (in.number() < 0)
            corrupt();
        const auto old = read_capture(in);
        const auto after = read_capture(in);
        if (ordinal != p.ordinal || after != p.action)
            return false;
        if (old.kind != after.kind || old == after)
            corrupt();
    }
    in.end();
    return true;
}

QByteArray result_document(Kind kind, const Decision &result) {
    Frame out("MSR1");
    out.tag(char(kind));
    if (kind == Kind::resume) {
        references(out, result.resolved_causes);
    } else {
        const auto &versions = kind == Kind::replace ? result.versions : result.cancellations;
        out.count(qsizetype(versions.size()));
        for (const auto &v : versions) {
            out.number(v.ordinal);
            out.version(v.version);
        }
        if (kind == Kind::stop) {
            references(out, result.added_causes);
            references(out, result.playback_intents);
        }
    }
    return out.data;
}

Decision read_result(Kind kind, const QByteArray &result) {
    ReadFrame in(result);
    if (in.pop(4) != "MSR1" || in.tag() != char(kind))
        corrupt();
    auto out = answer(Disposition::replay);
    auto read_refs = [&](std::vector<QByteArray> &refs) {
        const auto count = in.count();
        for (qsizetype i = 0; i < count; ++i)
            refs.push_back(in.reference());
    };
    if (kind == Kind::resume) {
        read_refs(out.resolved_causes);
        if (out.resolved_causes.empty())
            corrupt();
    } else {
        auto &versions = kind == Kind::replace ? out.versions : out.cancellations;
        const auto count = in.count();
        qint64 last = 0;
        for (qsizetype i = 0; i < count; ++i) {
            const auto ordinal = in.number();
            auto version = in.version();
            if (ordinal <= last || (kind == Kind::replace && version.isEmpty()))
                corrupt();
            versions.push_back({ordinal, std::move(version)});
            last = ordinal;
        }
        if (kind == Kind::stop) {
            read_refs(out.added_causes);
            read_refs(out.playback_intents);
            if (out.added_causes.size() > 1)
                corrupt();
        } else if (versions.empty())
            corrupt();
    }
    in.end();
    if (result_document(kind, out) != result)
        corrupt();
    return out;
}

std::optional<Decision> journal(const SqliteWorkSession &s, CommandAuthority &authority,
                                const Command &cmd, const CommandAuthority::Facts &facts,
                                bool preview_only) {
    const auto key = journal_key(cmd);
    const auto rows = s.execute({"SELECT kind,length(intent),length(result),decision FROM "
                                 "subathon_milestone_command_journal "
                                 "WHERE installation=? AND track=? AND operation=? LIMIT 2",
                                 key});
    const auto *row = unique(rows);
    if (!row)
        return {};
    if (!authority.discloses_retained(s, cmd, facts, verified_ref((*row)[3])))
        return answer(Disposition::denied);
    const auto intent_size = integer((*row)[1]);
    const auto result_size = integer((*row)[2]);
    if (intent_size < 5 || intent_size > document_limit || result_size < 5 ||
        result_size > document_limit)
        corrupt();
    const auto saved = s.execute({"SELECT intent FROM subathon_milestone_command_journal "
                                  "WHERE installation=? AND track=? AND operation=? LIMIT 2",
                                  key});
    const auto *data = unique(saved);
    if (!data || (*data)[0].metaType().id() != QMetaType::QByteArray ||
        (*data)[0].toByteArray().size() != intent_size)
        corrupt();
    const auto kind = command_kind(cmd);
    if (integer((*row)[0]) != int(kind) ||
        (kind == Kind::replace ? !matches_replace(cmd, (*data)[0].toByteArray())
                               : simple_intent(cmd) != (*data)[0].toByteArray()))
        return answer(Disposition::conflict);
    if (preview_only)
        return answer(Disposition::replay);
    const auto results = s.execute({"SELECT result FROM subathon_milestone_command_journal "
                                    "WHERE installation=? AND track=? AND operation=? LIMIT 2",
                                    key});
    const auto *result = unique(results);
    if (!result || (*result)[0].metaType().id() != QMetaType::QByteArray ||
        (*result)[0].toByteArray().size() != result_size)
        corrupt();
    return read_result(kind, (*result)[0].toByteArray());
}

void insert_journal(const SqliteWorkSession &s, const Command &cmd,
                    const CommandAuthority::Facts &facts, const QByteArray &intent,
                    const Decision &result) {
    const auto kind = command_kind(cmd);
    auto args = journal_key(cmd);
    args.append({int(kind), blob(cmd.actor.actor_ref), blob(cmd.actor.session_lineage),
                 facts.decision, intent, result_document(kind, result)});
    (void)s.execute({"INSERT INTO subathon_milestone_command_journal"
                     "(installation,track,operation,kind,actor,lineage,decision,intent,result) "
                     "VALUES(?,?,?,?,?,?,?,?,?)",
                     args});
}

std::optional<CommandAuthority::Facts> access(const SqliteWorkSession &s, CommandAuthority &owner,
                                              const Command &cmd) {
    auto facts = owner.select(s, cmd);
    if (!facts || !facts->can_command || !facts->can_disclose)
        return {};
    if (!ref(facts->grant_revision) || !ref(facts->configuration_revision) ||
        !ref(facts->capability_revision) || !ref(facts->decision))
        return {};
    return facts;
}
} // namespace

class ReplacePreview {
  public:
    struct Version {
        QByteArray predecessor;
        const AuthorizedCapture *capture;
    };

    QByteArray intent;
    QByteArray definition;
    std::vector<Version> versions;
    MilestoneEditActor actor;
};

namespace {

AuthorizedCapture old_capture(const SqliteWorkSession &s, CommandAuthority &authority,
                              const Occurrence &o, const Origin &key, qint64 ordinal,
                              const Slot &selected) {
    AuthorizedCapture result{{ordinal, {selected.kind, {}, {}}}, {}, {}, {}, {}};
    SqlRows captures;
    if (!selected.version.isEmpty()) {
        auto args = key.at(o.track);
        args.append({ordinal, selected.version});
        captures = s.execute(
            {"SELECT action_ref,action_version,payload,route,qualified_sender "
             "FROM subathon_milestone_slot_versions WHERE installation=? AND track=? AND "
             "origin_kind=? AND origin_scope=? AND origin_ref=? AND occurrence=? AND ordinal=? "
             "AND version=? LIMIT 2",
             args});
    } else if (key.kind == 2) {
        captures = s.execute(
            {"SELECT a.action_ref,a.version_ref,j.action_document,c.route_ref,c.sender_ref "
             "FROM subathon_milestone_edit_actions a LEFT JOIN subathon_milestone_edit_jobs j "
             "USING(occurrence_ref,action_ordinal) LEFT JOIN subathon_milestone_edit_captures c "
             "USING(occurrence_ref,action_ordinal) WHERE a.occurrence_ref=? AND "
             "a.action_ordinal=? LIMIT 2",
             {key.occurrence, ordinal}});
    } else if (selected.kind == MilestoneActionKind::bonus) {
        captures = s.execute(
            {"SELECT action_ref,version_ref,NULL,NULL,NULL FROM subathon_milestone_actions "
             "WHERE occurrence_ref=? AND action_ordinal=? LIMIT 2",
             {key.occurrence, ordinal}});
    } else {
        captures =
            s.execute({"SELECT a.action_ref,a.version_ref,CASE WHEN typeof(j.document)='blob' AND "
                       "length(j.document)<=262144 THEN j.document END,c.route_ref,c.sender_ref "
                       "FROM subathon_milestone_actions a JOIN subathon_delivery_captures c "
                       "USING(operation_scope,operation_ref,job_ref) JOIN subathon_app_jobs j "
                       "USING(operation_scope,operation_ref,job_ref) WHERE a.occurrence_ref=? "
                       "AND a.action_ordinal=? AND a.action_ref=c.action_ref AND "
                       "a.version_ref=c.action_version_ref LIMIT 2",
                       {key.occurrence, ordinal}});
    }
    const auto *row = unique(captures);
    if (!row)
        corrupt();
    result.selection.action.action_ref = reference((*row)[0]);
    result.selection.action.version_ref = reference((*row)[1]);
    if (selected.kind == MilestoneActionKind::bonus) {
        if (selected.version.isEmpty())
            result.bonus_ms = authority.historical_bonus_amount(s, o, result.selection);
        else {
            const auto payload = (*row)[2].toByteArray();
            if (payload.size() != 8 || (*row)[2].metaType().id() != QMetaType::QByteArray)
                corrupt();
            result.bonus_ms = qFromBigEndian<qint64>(payload.constData());
        }
    } else {
        result.payload = (*row)[2].toByteArray();
        if ((*row)[2].metaType().id() != QMetaType::QByteArray ||
            result.payload.size() > automation_limit)
            corrupt();
        result.route = reference((*row)[3]);
        result.qualified_sender = reference((*row)[4]);
    }
    return result;
}

struct VerifiedOriginalFormat {
    OriginalMilestoneAutomation key;
    std::string format;
};

std::optional<VerifiedOriginalFormat> original_format(const SqliteWorkSession &s,
                                                      const Occurrence &target, const Origin &key,
                                                      qint64 ordinal) {
    if (key.kind == 2) {
        const auto &edit = std::get<MilestoneEditKey>(target.origin);
        auto args = scope(target.track);
        args.append({key.operation, key.occurrence, ordinal});
        const auto rows = s.execute(
            {"SELECT j.job_format FROM main.subathon_milestone_edit_jobs j "
             "JOIN main.subathon_milestone_edit_captures c USING(occurrence_ref,action_ordinal) "
             "JOIN main.subathon_milestone_edit_actions a USING(occurrence_ref,action_ordinal) "
             "WHERE j.installation_ref=? AND j.track_ref=? AND j.edit_ref=? AND j.occurrence_ref=? "
             "AND j.action_ordinal=? AND a.action_ref=c.action_ref AND a.version_ref=c.version_ref "
             "AND j.job_format=? LIMIT 2",
             args + QList<QVariant>{blob(delivery_job_format)}});
        if (const auto *row = unique(rows))
            return VerifiedOriginalFormat{MilestoneEditJobKey{edit, target.occurrence, ordinal},
                                          reference(row->front())};
    } else {
        const auto rows = s.execute(
            {"SELECT a.job_ref,j.format_ref FROM main.subathon_milestone_actions a "
             "JOIN main.subathon_delivery_captures c USING(operation_scope,operation_ref,job_ref) "
             "JOIN main.subathon_app_jobs j USING(operation_scope,operation_ref,job_ref) "
             "WHERE a.operation_scope=? AND a.operation_ref=? AND a.occurrence_ref=? "
             "AND a.action_ordinal=? AND a.action_ref=c.action_ref AND "
             "a.version_ref=c.action_version_ref AND j.group_ref=a.occurrence_ref "
             "AND j.format_ref=? LIMIT 2",
             {key.scope, key.operation, key.occurrence, ordinal, blob(delivery_job_format)}});
        if (const auto *row = unique(rows))
            return VerifiedOriginalFormat{
                DeliveryJobKey{std::get<ApplicationOperationKey>(target.origin),
                               reference((*row)[0])},
                reference((*row)[1])};
    }
    return {};
}

std::optional<ReplacePreview>
select_replacement(const SqliteWorkSession &s, CommandAuthority &authority, const Command &cmd,
                   const CommandAuthority::Facts &facts, const AdmissionProfile *profile,
                   MilestoneReplacementDisclosure *disclosure = nullptr,
                   const MilestoneReplacementProfile *qualification = nullptr) {
    const auto &want = std::get<ReplaceIntent>(cmd.intent);
    const auto key = origin(want.target, profile);
    const auto current = target(s, want.target, key);
    if (!current)
        return {};
    if (disclosure)
        if (const auto *app = std::get_if<ApplicationOperationKey>(&want.target.origin))
            milestone_sql::verify_partitioned_origin(s, *app);
    auto out = intent_prefix(cmd);
    target_fields(out, want.target);
    out.reference(facts.grant_revision);
    out.reference(facts.configuration_revision);
    out.reference(facts.capability_revision);
    out.reference(current->prefix);
    out.tag(current->released ? 1 : 0);
    if (disclosure)
        disclosure->released = current->released;
    // Recover exact active cause identities only when the counted metadata
    // admits a nonempty hold closure.
    if (!current->deferred && current->reviews == 0) {
        out.count(0);
    } else {
        const auto holds = SqliteMilestoneGroupDecisions(s).active_holds(want.target.occurrence);
        if (std::ssize(holds) != current->reviews + qint64(current->deferred))
            corrupt();
        out.count(qsizetype(holds.size()));
        for (const auto &h : holds) {
            out.tag(char(h.cause) + 1);
            out.text(h.source_ref);
            // Active decisions are immutable until their one-way resolution.
            out.text(h.source_ref);
        }
        if (disclosure)
            disclosure->holds = holds;
    }
    auto patch = want.patch;
    std::ranges::sort(patch, {}, &Replacement::ordinal);
    if (facts.captures.size() > member_limit)
        return {};
    ReplacePreview result{{}, current->definition, {}, cmd.actor};
    out.count(qsizetype(patch.size()));
    for (const auto &p : patch) {
        const auto selected = slot(s, want.target, key, p.ordinal);
        if (!selected || started(*selected) || selected->kind != p.action.kind)
            return {};
        const auto matches = [&](const AuthorizedCapture &c) { return c.selection == p; };
        const auto after = std::ranges::find_if(facts.captures, matches);
        if (after == facts.captures.end() || std::ranges::count_if(facts.captures, matches) != 1)
            return {};
        if (p.action.kind == MilestoneActionKind::automation) {
            if (after->bonus_ms || after->payload.size() > automation_limit ||
                !milestone_sql::bounded_ref(after->route) ||
                !milestone_sql::bounded_ref(after->qualified_sender))
                return {};
            if (key.kind == 2 || !selected->version.isEmpty()) {
                if (integer(selected->payload_size) < 0 ||
                    integer(selected->payload_size) > automation_limit ||
                    integer(selected->route_size) < 1 || integer(selected->route_size) > 4096 ||
                    integer(selected->sender_size) < 1 || integer(selected->sender_size) > 4096)
                    corrupt();
            }
        } else if (!after->bonus_ms || *after->bonus_ms < 0) {
            return {};
        }
        auto before = old_capture(s, authority, want.target, key, p.ordinal, *selected);
        if (before.selection.action == p.action || (p.action.kind == MilestoneActionKind::bonus &&
                                                    (!before.bonus_ms || *before.bonus_ms < 0)))
            return {};
        out.number(p.ordinal);
        out.version(selected->version);
        out.number(selected->revision.isNull() ? 0 : integer(selected->revision));
        capture(out, before);
        capture(out, *after);
        if (disclosure) {
            std::string format;
            std::optional<DeliverySenderSelection> mode;
            if (p.action.kind == MilestoneActionKind::automation) {
                const auto original = original_format(s, want.target, key, p.ordinal);
                if (!original ||
                    !qualification->compatible(facts, p, original->key, original->format)
                         .value_or(false))
                    return {};
                mode = qualification->sender_selection(facts, p);
                if (mode != DeliverySenderSelection::configured_default &&
                    mode != DeliverySenderSelection::explicit_identity)
                    return {};
                format = original->format;
            }
            disclosure->ordered.push_back(
                {p.ordinal, selected->version, std::move(before), *after, std::move(format), mode});
        }
        result.versions.push_back({selected->version, &*after});
    }
    result.intent = std::move(out.data);
    return result;
}

Decision save_replacement(const SqliteWorkSession &s, const Command &cmd,
                          const CommandAuthority::Facts &facts, const ReplacePreview &preview,
                          const AdmissionProfile *profile) {
    const auto &o = std::get<ReplaceIntent>(cmd.intent).target;
    const auto key = origin(o, profile);
    const auto *bytes = fixture(profile);
    auto result = answer(Disposition::committed);
    for (const auto &v : preview.versions) {
        const auto ordinal = v.capture->selection.ordinal;
        const auto fixture_value = bytes && ordinal <= std::ssize(bytes->replacement_versions)
                                       ? bytes->replacement_versions[std::size_t(ordinal - 1)]
                                       : QByteArray{};
        auto version = allocate(fixture_value);
        if (!ref(version))
            corrupt();
        result.versions.push_back({ordinal, std::move(version)});
    }
    insert_journal(s, cmd, facts, preview.intent, result);
    for (std::size_t i = 0; i < preview.versions.size(); ++i) {
        const auto &v = preview.versions[i];
        const auto &selected = result.versions[i];
        const auto &c = *v.capture;
        const auto &action = c.selection.action;
        auto args = scope(cmd.track);
        args.append({cmd.operation, key.kind, key.scope, key.operation, key.occurrence,
                     selected.ordinal, selected.version,
                     v.predecessor.isEmpty() ? QVariant{} : QVariant(v.predecessor),
                     int(action.kind) + 1, blob(action.action_ref), blob(action.version_ref),
                     c.bonus_ms ? numeric_payload(*c.bonus_ms)
                                : (c.payload.isNull() ? QByteArray("", 0) : c.payload),
                     c.bonus_ms ? QVariant{} : QVariant(blob(c.route)),
                     c.bonus_ms ? QVariant{} : QVariant(blob(c.qualified_sender))});
        // select_replacement reselected the exact head in this serialized unit,
        // including the original head when predecessor is SQL NULL.
        (void)s.execute(
            {"INSERT INTO subathon_milestone_slot_versions VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
             args});
        if (!c.bonus_ms) {
            auto job_key = scope(cmd.track);
            job_key.append(selected.version);
            (void)s.execute(
                {"INSERT INTO subathon_milestone_slot_attempts VALUES(?,?,?,1,0,NULL)", job_key});
        }
    }
    return result;
}

Decision resume(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
                const CommandAuthority::Facts &facts) {
    auto result = answer(Disposition::committed);
    for (const auto &cause : std::get<ResumeIntent>(cmd.intent).causes) {
        auto args = scope(cmd.track);
        args.append(cause.ref);
        const auto rows = s.execute(
            {"SELECT h.stop,h.resume,f.occurrence,j.decision FROM "
             "subathon_milestone_interruption_causes h "
             "JOIN subathon_milestone_stop_fences f ON f.installation=h.installation AND "
             "f.track=h.track "
             "AND f.stop=h.stop JOIN subathon_milestone_command_journal j ON "
             "j.installation=h.installation "
             "AND j.track=h.track AND j.operation=h.stop WHERE h.installation=? AND h.track=? "
             "AND h.cause=? LIMIT 2",
             args});
        const auto *row = unique(rows);
        if (!row || !(*row)[1].isNull())
            return answer(Disposition::refused);
        (void)verified_ref((*row)[0]);
        (void)verified_ref((*row)[2]);
        if (!owner.discloses_retained(s, cmd, facts, verified_ref((*row)[3])))
            return answer(Disposition::denied);
        result.resolved_causes.push_back(cause.ref);
    }
    std::ranges::sort(result.resolved_causes);
    insert_journal(s, cmd, facts, simple_intent(cmd), result);
    for (const auto &cause : result.resolved_causes) {
        QList<QVariant> args{cmd.operation};
        args.append(scope(cmd.track));
        args.append(cause);
        const auto rows = s.execute(
            {"UPDATE subathon_milestone_interruption_causes SET resume=? "
             "WHERE installation=? AND track=? AND cause=? AND resume IS NULL RETURNING 1",
             args});
        if (rows.size() != 1 || integer(rows[0][0]) != 1)
            corrupt();
    }
    return result;
}

enum class PresentationState { inactive, active, incomplete };

PresentationState presentation_state(const CommandAuthority::Facts &facts, const Occurrence &o) {
    // A known absent presentation or one for another occurrence is negative.
    // An observation for this occurrence can lack facts needed to compare it.
    if (!facts.presentation || !same(o, facts.presentation->target))
        return PresentationState::inactive;
    const auto &p = *facts.presentation;
    if (!ref(p.presentation) || !ref(p.bound_generation) || !ref(p.current_generation) ||
        !ref(p.owner_revision) || !ref(p.current_owner_revision))
        return PresentationState::incomplete;
    const bool active = p.bound_generation == p.current_generation &&
                        p.owner_revision == p.current_owner_revision &&
                        p.started_ms <= p.observed_ms && p.observed_ms < p.expiry_ms &&
                        p.observed_ms >= p.last_accepted_observation_ms;
    return active ? PresentationState::active : PresentationState::inactive;
}

qint64 slot_count(const SqliteWorkSession &s, const Origin &key, qint64 cap = member_limit) {
    const auto count = s.execute(
        {key.kind == 2
             ? "SELECT count(*) FROM (SELECT action_ordinal FROM subathon_milestone_edit_actions "
               "WHERE occurrence_ref=? ORDER BY action_ordinal LIMIT ?)"
             : "SELECT count(*) FROM (SELECT action_ordinal FROM subathon_milestone_actions "
               "WHERE occurrence_ref=? ORDER BY action_ordinal LIMIT ?)",
         {key.occurrence, cap + 1}});
    return integer(count.front().front());
}

Decision stop(const SqliteWorkSession &s, const Command &cmd, const CommandAuthority::Facts &facts,
              const AdmissionProfile *profile) {
    const auto &want = std::get<StopIntent>(cmd.intent);
    const auto &o = want.target;
    const auto key = origin(o, profile);
    const auto head = target(s, o, key);
    if (!head)
        return answer(Disposition::refused);

    if (const auto *app = std::get_if<ApplicationOperationKey>(&o.origin))
        milestone_sql::verify_partitioned_origin(s, *app);

    // This bounded count is also an excess probe. Each selected slot must fit
    // the writer's cumulative statement, byte and row meters before publication.
    qint64 slot_cap = member_limit;
    if (profile) {
        // Every member needs the original metadata, head probe and action
        // reference. Branch-specific statements and the actual byte widths
        // are still charged by the writer as the complete selection is read.
        const auto &work = profile->work;
        const auto attempts = want.choice == QueueChoice::continue_queue ? 10 : 11;
        slot_cap = std::max(
            qint64{0},
            std::min(
                {member_limit,
                 qint64((std::max(work.statements_per_batch, qsizetype(attempts)) - attempts) / 3),
                 qint64((std::max(work.result_rows, qsizetype{4}) - 4) / 2)}));
    }
    const auto size = slot_count(s, key, slot_cap);
    if (size > slot_cap || facts.observed_playback.size() > member_limit)
        return answer(Disposition::resource_limit);
    auto result = answer(Disposition::committed);
    bool queued = false;
    std::vector<PlaybackRecord> playback;
    std::vector<AttemptCharge> attempts;
    for (qint64 ordinal = 1; ordinal <= size; ++ordinal) {
        const auto selected = slot(s, o, key, ordinal);
        if (!selected)
            corrupt();
        if (started(*selected) && selected->kind == MilestoneActionKind::automation) {
            const auto revision = integer(selected->revision);
            auto &charge = attempts.emplace_back();
            const auto state =
                attempt_state(s, o, key, ordinal, selected->version, revision, false, &charge);
            if (!state)
                corrupt();
            queued |= *state == int(DeliveryJobState::queued);
            auto observed = initial_playback(s, o, key, ordinal, *selected, *state, facts, profile);
            playback.insert(playback.end(), std::make_move_iterator(observed.begin()),
                            std::make_move_iterator(observed.end()));
        }
        const auto actions =
            s.execute({key.kind == 2 ? "SELECT action_ref FROM subathon_milestone_edit_actions "
                                       "WHERE occurrence_ref=? AND action_ordinal=? LIMIT 2"
                                     : "SELECT action_ref FROM subathon_milestone_actions "
                                       "WHERE occurrence_ref=? AND action_ordinal=? LIMIT 2",
                       {key.occurrence, ordinal}});
        const auto *action = unique(actions);
        if (!action)
            corrupt();
        (void)verified_ref((*action)[0]);
        if (!started(*selected)) {
            result.cancellations.push_back({ordinal, selected->version});
            queued = true;
        }
    }
    if (head->released && !queued && playback.empty()) {
        const auto state = presentation_state(facts, o);
        if (state != PresentationState::active)
            return answer(state == PresentationState::incomplete ? Disposition::unavailable
                                                                 : Disposition::refused);
    }

    const auto cap = playback_capacity(key, cmd.track, cmd.operation, facts.decision, playback,
                                       attempts, profile);
    if (std::ssize(playback) > cap)
        return answer(Disposition::resource_limit);
    const auto hold = want.choice != QueueChoice::continue_queue;
    if (hold) {
        auto cause =
            allocate(fixture(profile) ? fixture(profile)->interruption_cause : QByteArray{});
        if (!ref(cause))
            corrupt();
        result.added_causes.push_back(std::move(cause));
    }
    for (const auto &media : playback)
        if (media.observation.capability == StopCapability::supported)
            result.playback_intents.push_back(media.instance);
    std::ranges::sort(result.playback_intents);
    insert_journal(s, cmd, facts, simple_intent(cmd), result);
    auto stop_key = scope(cmd.track);
    stop_key.append(cmd.operation);
    auto args = stop_key;
    args.append({key.kind, key.scope, key.operation, key.occurrence, hold ? 1 : 2, cap});
    (void)s.execute({"INSERT INTO subathon_milestone_stop_fences VALUES(?,?,?,?,?,?,?,?,?)", args});
    if (hold) {
        auto cause_key = scope(cmd.track);
        cause_key.append({result.added_causes.front(), cmd.operation});
        (void)s.execute(
            {"INSERT INTO subathon_milestone_interruption_causes VALUES(?,?,?,?,NULL)", cause_key});
    }
    for (const auto &c : result.cancellations) {
        auto cancelled = stop_key;
        cancelled.append(c.ordinal);
        if (c.version.isEmpty()) {
            (void)s.execute(
                {"INSERT INTO subathon_milestone_stop_cancellations VALUES(?,?,?,?,NULL)",
                 cancelled});
        } else {
            cancelled.append(c.version);
            (void)s.execute(
                {"INSERT INTO subathon_milestone_stop_cancellations VALUES(?,?,?,?,?)", cancelled});
        }
    }
    for (const auto &media : playback)
        save_playback(s, cmd.track, cmd.operation, media);
    return result;
}
} // namespace

namespace {
Disposition admission(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
                      const std::optional<CommandAuthority::Facts> &facts,
                      bool classify_unavailable) {
    if (!facts)
        return classify_unavailable && !owner.denies_actor(s, cmd) ? Disposition::unavailable
                                                                   : Disposition::denied;
    if (!facts->can_command || !facts->can_disclose)
        return Disposition::denied;
    if (!ref(facts->grant_revision) || !ref(facts->configuration_revision) ||
        !ref(facts->capability_revision) || !ref(facts->decision))
        return classify_unavailable ? Disposition::unavailable : Disposition::denied;
    return Disposition::committed;
}

Preview preview_replacement(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
                            const AdmissionProfile *profile, bool classify_unavailable) {
    if (!valid_command(cmd) || command_kind(cmd) != Kind::replace)
        return Disposition::refused;
    return guarded(s, [&]() -> Preview {
        const auto facts = owner.select(s, cmd);
        const auto gate = admission(s, owner, cmd, facts, classify_unavailable);
        if (gate != Disposition::committed)
            return gate;
        require_catalog(s);
        if (const auto saved = journal(s, owner, cmd, *facts, true))
            return saved->disposition;
        auto selected = select_replacement(s, owner, cmd, *facts, profile);
        if (!selected)
            return Disposition::refused;
        // The preview retains only the immutable frame and revision. The
        // confirmation turn selects fresh captures before inserting anything.
        selected->versions.clear();
        return std::make_shared<const ReplacePreview>(std::move(*selected));
    });
}

Decision stage(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
               const ReplacePreview *preview, const AdmissionProfile *profile,
               bool classify_unavailable) {
    if (!valid_command(cmd))
        return answer(Disposition::refused);
    return guarded(s, [&] {
        const auto facts = owner.select(s, cmd);
        if (classify_unavailable && preview &&
            (preview->actor.actor_ref != cmd.actor.actor_ref ||
             preview->actor.session_lineage != cmd.actor.session_lineage))
            return answer(Disposition::denied);
        const auto gate = admission(s, owner, cmd, facts, classify_unavailable);
        if (gate != Disposition::committed)
            return answer(gate);
        require_catalog(s);
        if (const auto saved = journal(s, owner, cmd, *facts, false))
            return *saved;
        switch (command_kind(cmd)) {
        case Kind::resume:
            return resume(s, owner, cmd, *facts);
        case Kind::stop:
            return stop(s, cmd, *facts, profile);
        case Kind::replace:
            if (preview) {
                const auto current = select_replacement(s, owner, cmd, *facts, profile);
                if (current && current->intent == preview->intent &&
                    current->definition == preview->definition)
                    return save_replacement(s, cmd, *facts, *current, profile);
            }
            return answer(Disposition::refused);
        }
        corrupt();
    });
}
} // namespace

Preview preview_replace(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
                        const AdmissionProfile *profile) {
    return preview_replacement(s, owner, cmd, profile, false);
}

Preview preview_milestone_replacement(const SqliteWorkSession &s, CommandAuthority &owner,
                                      const Command &cmd) {
    return preview_replacement(s, owner, cmd, nullptr, true);
}

MilestoneReplacementDisclosureReply
preview_milestone_replacement_disclosure(const SqliteWorkSession &s, CommandAuthority &owner,
                                         const MilestoneReplacementProfile &profile,
                                         const Command &cmd) {
    if (!valid_command(cmd) || command_kind(cmd) != Kind::replace)
        return Disposition::refused;
    return guarded(s, [&]() -> MilestoneReplacementDisclosureReply {
        const auto facts = owner.select(s, cmd);
        const auto gate = admission(s, owner, cmd, facts, true);
        if (gate != Disposition::committed)
            return gate;
        if (!profile.compatible || !profile.sender_selection)
            return Disposition::unavailable;
        require_catalog(s);
        if (const auto saved = journal(s, owner, cmd, *facts, true))
            return saved->disposition;
        MilestoneReplacementDisclosure view{
            std::get<ReplaceIntent>(cmd.intent).target, false, {}, {}, {}};
        auto selected = select_replacement(s, owner, cmd, *facts, nullptr, &view, &profile);
        if (!selected)
            return Disposition::refused;
        selected->versions.clear();
        view.confirmation = std::make_shared<const ReplacePreview>(std::move(*selected));
        return view;
    });
}

Decision stage_command(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
                       const ReplacePreview *preview, const AdmissionProfile *profile) {
    return stage(s, owner, cmd, preview, profile, false);
}

Decision confirm_milestone_replacement(const SqliteWorkSession &s, CommandAuthority &owner,
                                       const Command &cmd, const ReplacePreview &preview) {
    if (command_kind(cmd) != Kind::replace)
        return answer(Disposition::refused);
    return stage(s, owner, cmd, &preview, nullptr, true);
}

Decision read_command(const SqliteWorkSession &s, CommandAuthority &owner, const Command &cmd,
                      const AdmissionProfile *) {
    if (!valid_command(cmd))
        return answer(Disposition::refused);
    return guarded(s, [&] {
        const auto facts = access(s, owner, cmd);
        if (!facts)
            return answer(Disposition::denied);
        require_catalog(s);
        return journal(s, owner, cmd, *facts, false).value_or(answer(Disposition::refused));
    });
}

std::optional<OccurrenceSelection> read_occurrence_selection(const SqliteWorkSession &s,
                                                             CommandAuthority &owner,
                                                             const Command &selector) {
    if (!valid_command(selector) || command_kind(selector) != Kind::stop)
        return {};
    return guarded(s, [&]() -> std::optional<OccurrenceSelection> {
        const auto facts = access(s, owner, selector);
        if (!facts)
            return {};
        require_catalog(s);
        const auto &o = std::get<StopIntent>(selector.intent).target;
        const auto key = origin(o, nullptr);
        const auto selected = target(s, o, key, true);
        if (!selected)
            return {};
        OccurrenceSelection out{
            o, selected->order, selected->released, selected->skipped, selected->stop, {}, {}, {},
            {}};
        if (!out.stop.isEmpty()) {
            auto stop = selector;
            stop.operation = out.stop;
            // Stop discovery and its projection use the journal's retained
            // disclosure gate. Its original queue choice selects the frame.
            const auto stop_rows = s.execute(
                {"SELECT choice FROM subathon_milestone_stop_fences WHERE installation=? AND "
                 "track=? AND stop=? LIMIT 2",
                 journal_key(stop)});
            const auto *choice = unique(stop_rows);
            if (!choice)
                corrupt();
            const auto normalized = integer(choice->front());
            if (normalized != 1 && normalized != 2)
                corrupt();
            std::get<StopIntent>(stop.intent).choice =
                normalized == 1 ? QueueChoice::hold : QueueChoice::continue_queue;
            const auto result = journal(s, owner, stop, *facts, false);
            if (!result || result->disposition == Disposition::denied)
                return {};
            if (result->disposition != Disposition::replay)
                corrupt();
            out.cancellations = result->cancellations;
        }

        const auto gates = SqliteMilestoneGroupDecisions(s).active_holds(o.occurrence);
        out.independent_holds = gates;
        const auto holds = s.execute(
            {"SELECT CASE WHEN typeof(h.cause)='blob' AND length(h.cause) BETWEEN 1 AND 4096 "
             "THEN h.cause END,j.decision FROM subathon_milestone_interruption_causes h "
             "JOIN subathon_milestone_command_journal j ON "
             "(j.installation,j.track,j.operation)=(h.installation,h.track,h.stop) "
             "WHERE h.installation=? AND h.track=? AND h.resume IS NULL ORDER BY h.cause LIMIT ?",
             {blob(o.track.installation_ref), blob(o.track.track_ref), member_limit + 1}});
        if (holds.size() > member_limit)
            throw SqliteWorkFailure(WorkError::result_limit);
        for (const auto &row : holds) {
            if (!owner.discloses_retained(s, selector, *facts, verified_ref(row[1])))
                return {};
            out.interruption_holds.push_back(verified_ref(row.front()));
        }
        const auto size = slot_count(s, key);
        if (size > member_limit)
            throw SqliteWorkFailure(WorkError::result_limit);
        for (qint64 ordinal = 1; ordinal <= size; ++ordinal) {
            const auto current = slot(s, o, key, ordinal);
            if (!current)
                corrupt();
            out.selected_versions.push_back({ordinal, current->version});
        }
        return out;
    });
}

namespace {

std::optional<ReplacementAttempt> select_version_attempt(const SqliteWorkSession &s,
                                                         const Occurrence &o,
                                                         const SelectedVersion &version,
                                                         std::optional<qint64> revision) {
    const auto key = origin(o, nullptr);
    auto args = key.at(o.track);
    args.append({version.ordinal, version.version});
    QString sql =
        "SELECT a.revision,a.state,CASE WHEN typeof(a.evidence)='blob' AND "
        "length(a.evidence) BETWEEN 1 AND 4096 THEN a.evidence END "
        "FROM subathon_milestone_slot_versions v "
        "JOIN subathon_milestone_slot_attempts a USING(installation,track,version) "
        "WHERE v.installation=? AND v.track=? AND v.origin_kind=? AND v.origin_scope=? "
        "AND v.origin_ref=? AND v.occurrence=? AND v.ordinal=? AND v.version=? AND v.kind=1";
    if (revision) {
        sql += " AND a.revision=? LIMIT 2";
        args.append(*revision);
    } else {
        sql += " ORDER BY a.revision DESC LIMIT 1";
    }
    const auto rows = s.execute({std::move(sql), args});
    const auto *row = unique(rows);
    if (!row)
        return {};
    const auto selected = integer((*row)[0]);
    const auto state =
        milestone_edit_sql::enumeration((*row)[1], DeliveryJobState::outcome_unknown);
    const auto needs_evidence = state != DeliveryJobState::sending && selected > 1;
    if (selected < 1 || (needs_evidence == (*row)[2].isNull()) ||
        (selected == 1 && state != DeliveryJobState::queued))
        corrupt();
    return ReplacementAttempt{selected, state, (*row)[2].toByteArray()};
}

bool valid_version(const Occurrence &o, const SelectedVersion &version, qint64 revision) {
    return milestone_sql::bounded_ref(o.track.installation_ref) &&
           milestone_sql::bounded_ref(o.track.track_ref) && valid(o.track, o) &&
           ref(version.version) && version.ordinal > 0 && revision > 0;
}

ReplacementJob replacement_job(const SqliteWorkSession &s, const Occurrence &o,
                               const SelectedVersion &version, const Slot &slot) {
    if (integer(slot.payload_size) < 0 || integer(slot.payload_size) > automation_limit ||
        integer(slot.route_size) < 1 || integer(slot.route_size) > 4096 ||
        integer(slot.sender_size) < 1 || integer(slot.sender_size) > 4096)
        corrupt();
    auto args = scope(o.track);
    args.append(version.version);
    const auto rows =
        s.execute({"SELECT payload,route,qualified_sender FROM subathon_milestone_slot_versions "
                   "WHERE installation=? AND track=? AND version=? LIMIT 2",
                   args});
    const auto *row = unique(rows);
    if (!row || (*row)[0].metaType().id() != QMetaType::QByteArray ||
        (*row)[0].toByteArray().size() != integer(slot.payload_size))
        corrupt();
    return {o, version, (*row)[0].toByteArray(), reference((*row)[1]), reference((*row)[2])};
}

void append_attempt(const SqliteWorkSession &s, const Occurrence &o, const SelectedVersion &version,
                    qint64 previous, DeliveryJobState state, const QByteArray &evidence = {}) {
    if (previous == std::numeric_limits<qint64>::max())
        corrupt();
    auto args = scope(o.track);
    args.append({version.version, previous + 1, int(state),
                 evidence.isNull() ? QVariant{} : QVariant(evidence)});
    (void)s.execute({"INSERT INTO subathon_milestone_slot_attempts VALUES(?,?,?,?,?,?)", args});
}

std::optional<Slot> eligible_job(const SqliteWorkSession &s, const Occurrence &o,
                                 const SelectedVersion &version, qint64 revision,
                                 DeliveryJobState expected) {
    const auto key = origin(o, nullptr);
    const auto current = target(s, o, key);
    if (!current || !current->released)
        return {};
    const auto selected = slot(s, o, key, version.ordinal);
    if (!selected || selected->version != version.version ||
        selected->kind != MilestoneActionKind::automation ||
        integer(selected->revision) != revision)
        return {};
    const auto state = attempt_state(s, o, key, version.ordinal, version.version, revision, false);
    return state == int(expected) ? selected : std::nullopt;
}
} // namespace

StrandedReplacementPage stranded_replacement_attempts(const SqliteWorkSession &s,
                                                      const MilestoneTrackKey &track,
                                                      const std::optional<QByteArray> &after,
                                                      std::size_t limit) {
    if (!milestone_sql::bounded_ref(track.installation_ref) ||
        !milestone_sql::bounded_ref(track.track_ref) || (after && !ref(*after)) || limit == 0 ||
        limit > stranded_delivery_page_limit)
        throw std::invalid_argument("Invalid stranded replacement page");
    return guarded(s, [&] {
        require_catalog(s);
        auto args = scope(track);
        QString page =
            "WITH page AS MATERIALIZED (SELECT origin_kind,origin_scope,origin_ref,occurrence,"
            "ordinal,version,kind,installation,track FROM main.subathon_milestone_slot_versions "
            "WHERE installation=? AND track=?";
        if (after) {
            page += " AND version>?";
            args.append(*after);
        }
        page += " ORDER BY version LIMIT ?) ";
        args.append(qint64(limit));
        // Price the complete bounded projection before fetching any BLOB
        // values. Both statements observe the same writer transaction.
        const auto lengths = s.execute(
            {page + "SELECT CASE WHEN typeof(origin_scope)='blob' THEN length(origin_scope) END,"
                    "CASE WHEN typeof(origin_ref)='blob' THEN length(origin_ref) END,"
                    "CASE WHEN typeof(occurrence)='blob' THEN length(occurrence) END,"
                    "CASE WHEN typeof(version)='blob' THEN length(version) END FROM page",
             args});
        qsizetype charge = 9 * 8 * lengths.size();
        for (const auto &row : lengths) {
            for (const auto &value : row) {
                const auto size = integer(value);
                if (size < 1 || size > 4096)
                    corrupt();
                charge += size;
            }
        }
        s.preflight_result(lengths.size(), charge);
        const auto rows = s.execute(
            {page + "SELECT p.origin_kind,p.origin_scope,p.origin_ref,p.occurrence,p.ordinal,"
                    "p.version,p.kind,a.revision,a.state FROM page p "
                    "LEFT JOIN main.subathon_milestone_slot_attempts a "
                    "ON (a.installation,a.track,a.version)=(p.installation,p.track,p.version) "
                    "AND a.revision=(SELECT max(last.revision) "
                    "FROM main.subathon_milestone_slot_attempts last "
                    "WHERE (last.installation,last.track,last.version)="
                    "(p.installation,p.track,p.version)) ORDER BY p.version",
             args});
        if (rows.size() != lengths.size())
            corrupt();
        StrandedReplacementPage result;
        for (const auto &row : rows) {
            const auto origin_kind = integer(row[0]);
            const auto origin_scope = reference(row[1]);
            const auto origin_ref = reference(row[2]);
            const MilestoneOccurrenceKey occurrence{reference(row[3])};
            const auto ordinal = integer(row[4]);
            const auto version = verified_ref(row[5]);
            const auto kind = integer(row[6]);
            if ((origin_kind != 1 && origin_kind != 2) || ordinal < 1 || (kind != 1 && kind != 2))
                corrupt();
            if (kind == 2) {
                if (!row[7].isNull() || !row[8].isNull())
                    corrupt();
                continue;
            }
            const auto revision = integer(row[7]);
            const auto state =
                milestone_edit_sql::enumeration(row[8], DeliveryJobState::outcome_unknown);
            if (revision < 1 || (revision == 1 && state != DeliveryJobState::queued))
                corrupt();
            if (state != DeliveryJobState::sending)
                continue;
            if (revision < 2 || (origin_kind == 2 && origin_scope != track.track_ref))
                corrupt();
            Occurrence target{track, occurrence, ApplicationOperationKey{origin_scope, origin_ref}};
            if (origin_kind == 2)
                target.origin = MilestoneEditKey{track, origin_ref};
            const auto original = origin(target, nullptr);
            const auto originals = s.execute(
                {origin_kind == 1
                     ? "SELECT 1 FROM main.subathon_milestone_groups WHERE operation_scope=? "
                       "AND operation_ref=? AND occurrence_ref=? LIMIT 2"
                     : "SELECT 1 FROM main.subathon_milestone_edit_occurrences WHERE track_ref=? "
                       "AND edit_ref=? AND occurrence_ref=? AND installation_ref=? LIMIT 2",
                 origin_kind == 1
                     ? QList<QVariant>{original.scope, original.operation, original.occurrence}
                     : QList<QVariant>{original.scope, original.operation, original.occurrence,
                                       blob(track.installation_ref)}});
            if (!unique(originals))
                corrupt();
            result.attempts.push_back({std::move(target), {ordinal, version}, revision});
        }
        if (std::size_t(rows.size()) == limit)
            result.next_after = verified_ref(rows.back()[5]);
        return result;
    });
}

std::optional<EffectiveMilestoneRelease>
select_effective_milestone_release(SqliteWriter &writer, const CommittedMilestoneGroup &cached,
                                   const Occurrence &occurrence) {
    const auto cached_edit =
        cached.edit_membership ? std::optional{cached.edit_membership->origin} : std::nullopt;
    const auto exact = detail::timeline_target(occurrence);
    if (!valid(occurrence.track, occurrence) || cached.occurrence != occurrence.occurrence ||
        exact.edit != cached_edit ||
        (cached.application_origin && cached.application_origin != exact.application))
        return {};
    std::optional<EffectiveMilestoneRelease> result;
    const auto work = writer.run_idle_owner_work([&](auto ctx) -> QVariant {
        const auto s = ctx.begin();
        require_catalog(s);
        const auto key = origin(occurrence, nullptr);
        const auto current = target(s, occurrence, key);
        if (current) {
            if (const auto *app = std::get_if<ApplicationOperationKey>(&occurrence.origin))
                milestone_sql::verify_partitioned_origin(s, *app);
            const auto count = slot_count(s, key);
            if (count != std::ssize(cached.actions))
                corrupt();
            result.emplace(EffectiveMilestoneRelease{cached, {}, {}});
            for (qint64 ordinal = 1; ordinal <= count; ++ordinal) {
                const auto selected = slot(s, occurrence, key, ordinal);
                auto &action = result->group.actions[std::size_t(ordinal - 1)];
                if (!selected || selected->kind != action.kind)
                    corrupt();
                const SelectedVersion version{ordinal, selected->version};
                result->versions.push_back(version);
                if (selected->version.isEmpty())
                    continue;
                auto args = key.at(occurrence.track);
                args.append({ordinal, selected->version});
                const auto rows = s.execute(
                    {"SELECT action_ref,action_version FROM main.subathon_milestone_slot_versions "
                     "WHERE installation=? AND track=? AND origin_kind=? AND origin_scope=? "
                     "AND origin_ref=? AND occurrence=? AND ordinal=? AND version=? LIMIT 2",
                     args});
                const auto *definition = unique(rows);
                if (!definition)
                    corrupt();
                action = {selected->kind, reference((*definition)[0]), reference((*definition)[1])};
                if (selected->kind == MilestoneActionKind::automation)
                    result->replacement_jobs.push_back(
                        replacement_job(s, occurrence, version, *selected));
            }
        }
        s.commit();
        return result.has_value();
    });
    return acknowledged(work) ? result : std::nullopt;
}

void publish_acknowledged_replacement_claim(const SqliteWorkResult &work, Disposition disposition,
                                            std::optional<ReplacementJob> owned,
                                            std::vector<ReplacementJob> &dispatch) {
    if (auto job = acknowledged_claim(work, disposition, owned))
        dispatch.push_back(std::move(*job));
}

std::optional<ReplacementAttempt> lookup_replacement_attempt(const SqliteWorkSession &s,
                                                             const Occurrence &o,
                                                             const SelectedVersion &version,
                                                             qint64 revision) {
    if (!valid_version(o, version, revision))
        return {};
    return guarded(s, [&] {
        require_catalog(s);
        return select_version_attempt(s, o, version, revision);
    });
}

Disposition finish_replacement(const SqliteWorkSession &s, const Occurrence &o,
                               const SelectedVersion &version, qint64 sending,
                               DeliveryEvidence outcome, const QByteArray &evidence) {
    if (!valid_version(o, version, sending) || sending < 2 || !ref(evidence))
        return Disposition::refused;
    DeliveryJobState state;
    switch (outcome) {
    case DeliveryEvidence::delivered:
        state = DeliveryJobState::delivered;
        break;
    case DeliveryEvidence::known_undelivered:
        state = DeliveryJobState::failed;
        break;
    case DeliveryEvidence::indeterminate:
        state = DeliveryJobState::outcome_unknown;
        break;
    default:
        return Disposition::refused;
    }
    return guarded(s, [&] {
        require_catalog(s);
        const auto previous = select_version_attempt(s, o, version, std::nullopt);
        if (!previous || previous->revision != sending ||
            previous->state != DeliveryJobState::sending)
            return Disposition::refused;
        append_attempt(s, o, version, sending, state, evidence);
        return Disposition::committed;
    });
}

Disposition claim_replacement(const SqliteWorkSession &s, const Occurrence &o,
                              const SelectedVersion &version, qint64 revision,
                              ReplacementDispatchAuthority &authority) {
    if (!valid_version(o, version, revision))
        return Disposition::refused;
    return guarded(s, [&] {
        require_catalog(s);
        const auto slot = eligible_job(s, o, version, revision, DeliveryJobState::queued);
        if (!slot || !authority.allows(replacement_job(s, o, version, *slot)))
            return Disposition::refused;
        append_attempt(s, o, version, revision, DeliveryJobState::sending);
        return Disposition::committed;
    });
}

StorageError dispatch_replacement_job(SqliteWriter &writer, Occurrence occurrence,
                                      SelectedVersion version, qint64 revision,
                                      std::shared_ptr<ReplacementDispatchAuthority> authority,
                                      std::shared_ptr<ReplacementDeliveryAdapter> adapter,
                                      ReplacementDispatchCallback completed) {
    if (!authority || !adapter || !completed || !valid_version(occurrence, version, revision) ||
        revision == std::numeric_limits<qint64>::max())
        return StorageError::invalid_work;
    struct Claim final : ReplacementDispatchAuthority {
        explicit Claim(std::shared_ptr<ReplacementDispatchAuthority> owner)
            : authority(std::move(owner)) {}

        bool allows(const ReplacementJob &job) override {
            if (!authority->allows(job))
                return false;
            owned = job;
            return true;
        }

        std::shared_ptr<ReplacementDispatchAuthority> authority;
        std::optional<ReplacementJob> owned;
        std::optional<Disposition> provisional;
    };
    auto claim = std::make_shared<Claim>(std::move(authority));
    return writer.enqueue_work(
        [claim, occurrence = std::move(occurrence), version = std::move(version),
         revision](auto context) -> QVariant {
            const auto session = context.begin();
            claim->provisional = claim_replacement(session, occurrence, version, revision, *claim);
            session.commit();
            return 1;
        },
        [claim, adapter = std::move(adapter), completed = std::move(completed),
         revision](SqliteWorkResult work) {
            ReplacementDispatchCompletion result{std::move(work), {}};
            if (acknowledged(result.work)) {
                result.disposition = claim->provisional;
                if (auto job = acknowledged_claim(result.work, result.disposition, claim->owned)) {
                    try {
                        adapter->start(std::move(*job), revision + 1);
                    } catch (...) {
                        result.adapter_threw = true;
                    }
                }
            }
            completed(std::move(result));
        });
}

Disposition retry_replacement(const SqliteWorkSession &s, const Occurrence &o,
                              const SelectedVersion &version, qint64 revision,
                              ReplacementDispatchAuthority &authority) {
    if (!valid_version(o, version, revision))
        return Disposition::refused;
    return guarded(s, [&] {
        require_catalog(s);
        const auto slot = eligible_job(s, o, version, revision, DeliveryJobState::failed);
        if (!slot)
            return Disposition::refused;
        const auto policy = authority.retry_decision(replacement_job(s, o, version, *slot));
        if (!policy || !ref(*policy))
            return Disposition::refused;
        append_attempt(s, o, version, revision, DeliveryJobState::queued, *policy);
        return Disposition::committed;
    });
}

Disposition apply_replacement_bonus(const SqliteWorkSession &s, CommandAuthority &owner,
                                    ReplacementBonusEffect &effect, const Command &cmd,
                                    const SelectedVersion &version) {
    if (!valid_command(cmd) || command_kind(cmd) != Kind::replace || !ref(version.version) ||
        version.ordinal < 1)
        return Disposition::refused;
    return guarded(s, [&] {
        const auto facts = access(s, owner, cmd);
        if (!facts)
            return Disposition::denied;
        require_catalog(s);
        const auto saved = journal(s, owner, cmd, *facts, false);
        if (saved && saved->disposition == Disposition::denied)
            return Disposition::denied;
        if (!saved || saved->disposition != Disposition::replay ||
            std::ranges::find(saved->versions, version) == saved->versions.end())
            return Disposition::refused;
        const auto &o = std::get<ReplaceIntent>(cmd.intent).target;
        const auto key = origin(o, nullptr);
        const auto current = target(s, o, key);
        if (!current || !current->released)
            return Disposition::refused;
        const auto selected = slot(s, o, key, version.ordinal);
        if (!selected || selected->version != version.version ||
            selected->kind != MilestoneActionKind::bonus)
            return Disposition::refused;
        if (selected->applied)
            return Disposition::replay;
        auto args = scope(o.track);
        args.append(version.version);
        const auto rows = s.execute({"SELECT payload FROM subathon_milestone_slot_versions "
                                     "WHERE installation=? AND track=? AND version=? LIMIT 2",
                                     args});
        const auto *row = unique(rows);
        if (!row || (*row)[0].metaType().id() != QMetaType::QByteArray ||
            (*row)[0].toByteArray().size() != 8)
            corrupt();
        const auto requested = qFromBigEndian<qint64>((*row)[0].toByteArray().constData());
        if (requested < 0)
            corrupt();
        const auto actual = effect.apply(s, o, version, requested);
        if (actual.milliseconds < 0 || actual.milliseconds > requested)
            corrupt();
        args.append({blob(actual.effect_ref), actual.milliseconds});
        (void)s.execute({"INSERT INTO subathon_milestone_version_bonus VALUES(?,?,?,?,?)", args});
        return Disposition::committed;
    });
}

} // namespace subathon::server::milestone_interruption
