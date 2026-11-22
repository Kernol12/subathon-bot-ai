// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_milestone_interruption_detail.hpp"

#include <limits>
#include <set>

namespace subathon::server::milestone_interruption {
namespace {
using namespace detail;

constexpr auto stop_source =
    "SELECT f.occurrence,f.origin_kind,f.origin_scope,f.origin_ref,f.playback_cap,j.decision "
    "FROM "
    "subathon_milestone_stop_fences f "
    "JOIN subathon_milestone_command_journal j ON j.installation=f.installation AND "
    "j.track=f.track "
    "AND j.operation=f.stop WHERE f.installation=? AND f.track=? AND f.stop=? LIMIT 2";
constexpr auto count_instances =
    "SELECT count(*) FROM (SELECT instance FROM subathon_milestone_playback_instances "
    "WHERE installation=? AND track=? AND stop=? LIMIT ?)";
constexpr auto insert_instance =
    "INSERT INTO subathon_milestone_playback_instances VALUES(?,?,?,?,?,?,?,?,?,?,?,NULL)";
constexpr auto lookup_instance =
    " FROM subathon_milestone_playback_instances "
    "WHERE installation=? AND track=? AND stop=? AND ordinal=? AND adapter=? AND activation=? "
    "AND generation=? LIMIT 2";
constexpr auto edit_attempt_sql =
    "SELECT revision,state,attempt FROM subathon_milestone_edit_events "
    "WHERE occurrence_ref=? AND action_ordinal=? AND revision=? LIMIT 2";
constexpr auto application_attempt_sql =
    "SELECT revision,state,attempt FROM subathon_delivery_events "
    "WHERE operation_scope=? AND operation_ref=? AND job_ref=? AND revision=? LIMIT 2";

bool valid_playback(const PlaybackInstance &i) {
    return i.ordinal > 0 && i.started_attempt >= 2 && (i.version.isNull() || ref(i.version)) &&
           ref(i.adapter) && ref(i.activation) && ref(i.runtime_generation) &&
           (i.capability == StopCapability::supported ||
            i.capability == StopCapability::unsupported || i.capability == StopCapability::unknown);
}

QList<QVariant> stop_key(const MilestoneTrackKey &track, const QByteArray &stop) {
    auto args = scope(track);
    args.append(stop);
    return args;
}

QByteArray attempt_bytes(const PlaybackInstance &i, const AdmissionProfile *profile) {
    const auto *bytes = fixture(profile);
    auto attempt = bytes && i.version.isEmpty() && i.started_attempt == 2
                       ? bytes->playback_attempt
                       : numeric_payload(i.started_attempt);
    if (!ref(attempt))
        corrupt();
    return attempt;
}

PlaybackRecord prepared(const PlaybackInstance &i, const AdmissionProfile *profile) {
    const auto *bytes = fixture(profile);
    QByteArray supplied;
    if (bytes) {
        const auto selected = std::ranges::find(bytes->playback_refs, i.activation,
                                                &FixtureStorageBytes::PlaybackRef::activation);
        if (selected != bytes->playback_refs.end())
            supplied = selected->instance;
    }
    auto instance = allocate(supplied);
    if (!ref(instance))
        corrupt();
    return {i, std::move(instance), attempt_bytes(i, profile)};
}

std::optional<qint64> validate_stop(const SqliteWorkSession &s, const LatePlayback &wanted,
                                    const Origin &key) {
    const auto rows = s.execute({stop_source, stop_key(wanted.track, wanted.stop)});
    const auto *row = unique(rows);
    if (!row || verified_ref((*row)[0]) != key.occurrence || integer((*row)[1]) != key.kind ||
        verified_ref((*row)[2]) != key.scope || verified_ref((*row)[3]) != key.operation)
        return {};
    const auto cap = integer((*row)[4]);
    if (cap < 0 || cap > member_limit)
        corrupt();
    (void)verified_ref((*row)[5]);
    const auto &i = wanted.instance;
    if (!attempt_state(s, i.target, key, i.ordinal, i.version, i.started_attempt, true))
        return {};
    return cap;
}

QString blob_literal(const QByteArray &bytes) {
    return "X'" + QString::fromLatin1(bytes.toHex()) + "'";
}

struct Instance {
    QByteArray reference;
    bool exact;
    bool claimed;
};

std::optional<Instance> find_instance(const SqliteWorkSession &s, const LatePlayback &wanted,
                                      const AdmissionProfile *profile) {
    const auto &i = wanted.instance;
    auto args = stop_key(wanted.track, wanted.stop);
    args.append({i.ordinal, i.adapter, i.activation, i.runtime_generation});
    // The stored attempt/version comparison protects the exact live target.
    // The projected cell stays an integer on the eligible path.
    const auto version =
        i.version.isEmpty() ? QString("version IS NULL") : "version=" + blob_literal(i.version);
    const auto rows = s.execute({"SELECT instance,CASE WHEN " + version +
                                     " AND attempt=" + blob_literal(attempt_bytes(i, profile)) +
                                     " THEN capability END,length(handoff)" + lookup_instance,
                                 args});
    const auto *row = unique(rows);
    if (!row)
        return {};
    const bool exact = !(*row)[1].isNull() && integer((*row)[1]) == int(i.capability);
    if (!(*row)[2].isNull() && (integer((*row)[2]) < 1 || integer((*row)[2]) > 4096))
        corrupt();
    return Instance{verified_ref((*row)[0]), exact, !(*row)[2].isNull()};
}

bool valid_late(const LatePlayback &wanted) {
    return ref(wanted.stop) && milestone_sql::bounded_ref(wanted.track.track_ref) &&
           milestone_sql::bounded_ref(wanted.track.installation_ref) &&
           valid(wanted.track, wanted.instance.target) && valid_playback(wanted.instance);
}

bool handoff_matches(const std::optional<Instance> &selected, const PlaybackHandoff &wanted) {
    return selected && selected->reference == wanted.intent && selected->exact &&
           wanted.selected.instance.capability == StopCapability::supported;
}
} // namespace

namespace detail {

std::vector<PlaybackRecord> initial_playback(const SqliteWorkSession &s, const Occurrence &o,
                                             const Origin &key, qint64 ordinal, const Slot &slot,
                                             qint64 state, const CommandAuthority::Facts &facts,
                                             const AdmissionProfile *profile) {
    std::vector<PlaybackRecord> result;
    std::set<std::tuple<QByteArrayView, QByteArrayView, QByteArrayView>> instances;
    for (const auto &i : facts.observed_playback) {
        if (i.ordinal != ordinal || i.version != slot.version || !same(o, i.target) ||
            !valid_playback(i))
            continue;
        const auto proved =
            i.started_attempt == integer(slot.revision)
                ? state == int(DeliveryJobState::sending)
                : attempt_state(s, o, key, ordinal, i.version, i.started_attempt, true).has_value();
        if (proved && instances.emplace(i.adapter, i.activation, i.runtime_generation).second)
            result.push_back(prepared(i, profile));
    }
    return result;
}

qint64 playback_capacity(const Origin &key, const MilestoneTrackKey &track, const QByteArray &stop,
                         const QByteArray &decision, const std::vector<PlaybackRecord> &initial,
                         const AdmissionProfile *profile) {
    const auto limits = profile && profile->playback ? *profile->playback : StorageLimits{};
    // The cumulative allowance budgets one bounded Stop/attempt/count closure
    // and lookup/insert plus the result frame for every admitted instance.
    const auto scoped =
        qsizetype(track.installation_ref.size() + track.track_ref.size()) + stop.size() + 3 * 8;
    const auto attempt_sql = key.kind == 2 ? edit_attempt_sql : application_attempt_sql;
    const auto peak = qsizetype(std::max(
        {std::string_view(stop_source).size(), std::string_view(attempt_sql).size(),
         std::string_view(count_instances).size(), std::string_view(insert_instance).size(),
         std::string_view(lookup_instance).size() + 100}));
    const auto *encoded = fixture(profile);
    // Both statements share the scoped bindings. Account for the fixed
    // argument widths separately so their occurrences are easy to verify.
    const auto fixture_member = [&](const PlaybackRecord &entry) {
        const auto &i = entry.observation;
        return 2 * scoped + 4 * 8 + 8 * 8 + (entry.instance.size() + 4) + entry.instance.size() +
               i.version.size() + entry.attempt.size() +
               2 * (i.adapter.size() + i.activation.size() + i.runtime_generation.size());
    };
    const auto fixed_results =
        10 * 8 + key.occurrence.size() + key.scope.size() + key.operation.size() + decision.size();
    const auto attempt_binds = key.kind == 2 ? 24 + key.occurrence.size()
                                             : 32 + key.scope.size() + key.operation.size() + 4096;
    const auto fixed_binds = 2 * scoped + 8 + attempt_binds;
    qsizetype member_binds = 2 * scoped + 12 * 8 + 4 + 9 * 4096;
    if (encoded && !initial.empty()) {
        member_binds = 0;
        for (const auto &entry : initial)
            member_binds = std::max(member_binds, fixture_member(entry));
    }
    if (limits.statements_per_batch < 3 || limits.result_rows < 3 ||
        limits.statement_utf8_bytes < peak || limits.bound_value_bytes < fixed_binds ||
        limits.result_value_bytes < fixed_results)
        return 0;
    return std::min({member_limit, (limits.statements_per_batch - 3) / 2,
                     (limits.bound_value_bytes - fixed_binds) / member_binds});
}

void save_playback(const SqliteWorkSession &s, const MilestoneTrackKey &track,
                   const QByteArray &stop, const PlaybackRecord &record) {
    const auto &i = record.observation;
    auto args = stop_key(track, stop);
    args.append({record.instance, i.ordinal, i.version.isEmpty() ? QVariant{} : QVariant(i.version),
                 i.adapter, record.attempt, i.activation, i.runtime_generation, int(i.capability)});
    (void)s.execute({insert_instance, args});
}

} // namespace detail

bool PlaybackAuthority::owns_evidence(const SqliteWorkSession &s, const PlaybackEvidence &wanted) {
    return proves_current(s, wanted.handoff.selected);
}

Disposition stage_late_playback(const SqliteWorkSession &s, PlaybackAuthority &owner,
                                const LatePlayback &wanted, const AdmissionProfile *profile) {
    if (!valid_late(wanted))
        return Disposition::refused;
    return guarded(s, [&] {
        require_catalog(s);
        const auto key = origin(wanted.instance.target, profile);
        const auto cap = validate_stop(s, wanted, key);
        if (!cap || !owner.proves_current(s, wanted))
            return Disposition::refused;
        if (const auto existing = find_instance(s, wanted, profile))
            return existing->exact ? Disposition::replay : Disposition::refused;
        auto args = stop_key(wanted.track, wanted.stop);
        args.append(*cap + 1);
        const auto rows = s.execute({count_instances, args});
        if (integer(rows.front().front()) >= *cap)
            return Disposition::resource_limit;
        save_playback(s, wanted.track, wanted.stop, prepared(wanted.instance, profile));
        return Disposition::committed;
    });
}

Disposition stage_playback_handoff(const SqliteWorkSession &s, PlaybackAuthority &owner,
                                   const PlaybackHandoff &wanted, const AdmissionProfile *profile) {
    if (!valid_late(wanted.selected) || !ref(wanted.intent))
        return Disposition::refused;
    return guarded(s, [&] {
        const auto &request = wanted.selected;
        require_catalog(s);
        const auto key = origin(request.instance.target, profile);
        if (!validate_stop(s, request, key))
            return Disposition::refused;
        const auto existing = find_instance(s, request, profile);
        if (!handoff_matches(existing, wanted) || !owner.proves_current(s, request))
            return Disposition::refused;
        if (existing->claimed)
            return Disposition::replay;
        auto claim = allocate(fixture(profile) ? fixture(profile)->handoff : QByteArray{});
        if (!ref(claim))
            corrupt();
        QList<QVariant> args{claim};
        args.append(stop_key(request.track, request.stop));
        args.append(wanted.intent);
        const auto rows = s.execute(
            {"UPDATE subathon_milestone_playback_instances SET handoff=? WHERE installation=? "
             "AND track=? AND stop=? AND instance=? AND handoff IS NULL RETURNING 1",
             args});
        if (rows.size() != 1 || integer(rows[0][0]) != 1)
            corrupt();
        return Disposition::committed;
    });
}

Disposition stage_playback_outcome(const SqliteWorkSession &s, PlaybackAuthority &owner,
                                   const PlaybackEvidence &wanted,
                                   const AdmissionProfile *profile) {
    const auto &handoff = wanted.handoff;
    if (!valid_late(handoff.selected) || !ref(handoff.intent) || !ref(wanted.evidence) ||
        wanted.outcome < PlaybackOutcome::requested || wanted.outcome > PlaybackOutcome::uncertain)
        return Disposition::refused;
    return guarded(s, [&] {
        const auto &request = handoff.selected;
        require_catalog(s);
        const auto key = origin(request.instance.target, profile);
        if (!validate_stop(s, request, key))
            return Disposition::refused;
        const auto current = find_instance(s, request, profile);
        if (!handoff_matches(current, handoff) || !current->claimed ||
            !owner.owns_evidence(s, wanted))
            return Disposition::refused;
        auto args = stop_key(request.track, request.stop);
        args.append(handoff.intent);
        const auto rows = s.execute(
            {"SELECT ordinal,outcome,length(evidence) FROM subathon_milestone_playback_evidence "
             "WHERE installation=? AND track=? AND stop=? AND instance=? ORDER BY ordinal DESC "
             "LIMIT 1",
             args});
        const auto *last = unique(rows);
        const auto ordinal = last ? integer((*last)[0]) : 0;
        if (ordinal < 0 || ordinal == std::numeric_limits<qint64>::max())
            corrupt();
        if (last && integer((*last)[1]) == int(wanted.outcome) &&
            integer((*last)[2]) == wanted.evidence.size()) {
            auto exact = args;
            exact.append(ordinal);
            const auto retained = s.execute(
                {"SELECT evidence FROM subathon_milestone_playback_evidence WHERE "
                 "installation=? AND track=? AND stop=? AND instance=? AND ordinal=? LIMIT 2",
                 exact});
            const auto *evidence = unique(retained);
            if (!evidence)
                corrupt();
            if (verified_ref((*evidence)[0]) == wanted.evidence)
                return Disposition::replay;
        }
        if (ordinal > 1) {
            auto prior = args;
            prior.append({ordinal, int(wanted.outcome), wanted.evidence});
            const auto retained = s.execute(
                {"SELECT 1 FROM subathon_milestone_playback_evidence WHERE installation=? AND "
                 "track=? AND stop=? AND instance=? AND ordinal<? AND outcome=? AND evidence=? "
                 "LIMIT 1",
                 prior});
            if (!retained.empty())
                return Disposition::replay;
        }
        args.append({ordinal + 1, int(wanted.outcome), wanted.evidence});
        (void)s.execute(
            {"INSERT INTO subathon_milestone_playback_evidence VALUES(?,?,?,?,?,?,?)", args});
        return Disposition::committed;
    });
}
std::optional<PlaybackOutcomeRecord>
lookup_playback_evidence(const SqliteWorkSession &s, CommandAuthority &authority,
                         const Command &stop, const PlaybackHandoff &wanted, qint64 ordinal) {
    const auto &request = wanted.selected;
    const auto *intent = std::get_if<StopIntent>(&stop.intent);
    if (!intent || ordinal < 1 || !valid_late(request) || !ref(wanted.intent) ||
        stop.track != request.track || stop.operation != request.stop ||
        !same(intent->target, request.instance.target))
        return {};
    return guarded(s, [&]() -> std::optional<PlaybackOutcomeRecord> {
        if (read_command(s, authority, stop).disposition != Disposition::replay ||
            !validate_stop(s, request, origin(request.instance.target, nullptr)))
            return {};
        const auto selected = find_instance(s, request, nullptr);
        if (!handoff_matches(selected, wanted) || !selected->claimed)
            return {};
        auto args = stop_key(request.track, request.stop);
        args.append({wanted.intent, ordinal});
        const auto rows = s.execute(
            {"SELECT outcome,CASE WHEN typeof(evidence)='blob' AND "
             "length(evidence) BETWEEN 1 AND 4096 THEN evidence END "
             "FROM subathon_milestone_playback_evidence "
             "WHERE installation=? AND track=? AND stop=? AND instance=? AND ordinal=? LIMIT 2",
             args});
        const auto *row = unique(rows);
        if (!row)
            return {};
        return PlaybackOutcomeRecord{
            milestone_edit_sql::enumeration((*row)[0], PlaybackOutcome::uncertain),
            verified_ref((*row)[1])};
    });
}
} // namespace subathon::server::milestone_interruption
