// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_milestone_interruption_detail.hpp"

#include <QScopeGuard>
#include <subathon/server/milestone_playback_stop.hpp>

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

QString instance_lookup_sql(const QByteArray &version, const QByteArray &attempt) {
    const auto selector =
        version.isEmpty() ? QString("version IS NULL") : "version=" + blob_literal(version);
    return "SELECT instance,CASE WHEN " + selector + " AND attempt=" + blob_literal(attempt) +
           " THEN capability END,length(handoff)" + lookup_instance;
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
    const auto rows = s.execute({instance_lookup_sql(i.version, attempt_bytes(i, profile)), args});
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
                         const std::vector<AttemptCharge> &attempts,
                         const AdmissionProfile *profile) {
    if (attempts.empty())
        return 0; // A Stop cannot start a new attempt for a later activation.
    const auto limits = profile && profile->playback ? *profile->playback : StorageLimits{};
    const auto scoped =
        qsizetype(track.installation_ref.size() + track.track_ref.size()) + stop.size() + 3 * 8;
    const auto *encoded = fixture(profile);
    // The common Stop and count statements return seven cells together. Each
    // selected attempt contributes its exact lane-specific read charge.
    const auto common_values =
        7 * 8 + key.occurrence.size() + key.scope.size() + key.operation.size() + decision.size();
    const auto common_binds = 2 * scoped + 8;
    const auto common_peak = qsizetype(
        std::max({std::string_view(stop_source).size(), std::string_view(count_instances).size(),
                  std::string_view(insert_instance).size()}));
    qint64 cap = member_limit;
    for (const auto &attempt : attempts) {
        const auto version_width = attempt.version_bytes;
        const auto attempt_width = encoded && version_width == 0
                                       ? std::max(qsizetype{8}, encoded->playback_attempt.size())
                                       : qsizetype{8};
        // Hex literals grow by two characters per byte. Use the same builder
        // as lookup, including the distinct SQL NULL selector.
        const auto lookup_peak =
            instance_lookup_sql(QByteArray(version_width, '\0'), QByteArray(attempt_width, '\0'))
                .size();
        const auto fixed_binds = common_binds + attempt.binds;
        const auto fixed_values = common_values + attempt.values;
        const auto fixed_statements = 2 + attempt.statements;
        const auto fixed_rows = 2 + attempt.rows;
        if (limits.statements_per_batch < fixed_statements || limits.result_rows < fixed_rows ||
            limits.statement_utf8_bytes < std::max({common_peak, attempt.peak, lookup_peak}) ||
            limits.bound_value_bytes < fixed_binds || limits.result_value_bytes < fixed_values)
            return 0;

        // Each new member binds its lookup and insert plus its framed result
        // reference. A successful absent lookup and insert return no rows.
        qsizetype member_binds =
            2 * scoped + 12 * 8 + 4 + 2 * 16 + version_width + attempt_width + 6 * 4096;
        if (encoded && !initial.empty()) {
            member_binds = 0;
            for (const auto &entry : initial) {
                const auto &i = entry.observation;
                const auto sized = [&](qsizetype activation, qsizetype instance) {
                    return 2 * scoped + 12 * 8 + 4 + 2 * instance + i.version.size() +
                           entry.attempt.size() +
                           2 * (i.adapter.size() + activation + i.runtime_generation.size());
                };
                member_binds =
                    std::max(member_binds, sized(i.activation.size(), entry.instance.size()));
                for (const auto &ref : encoded->playback_refs)
                    if (!ref.activation.isEmpty() && !ref.instance.isEmpty())
                        member_binds = std::max(member_binds,
                                                sized(ref.activation.size(), ref.instance.size()));
            }
        }
        cap = std::min({cap, (limits.statements_per_batch - fixed_statements) / 2,
                        (limits.bound_value_bytes - fixed_binds) / member_binds});
    }
    return cap;
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

namespace {

Disposition mark_playback_handoff(const SqliteWorkSession &s, PlaybackAuthority &owner,
                                  const PlaybackHandoff &wanted, const AdmissionProfile *profile,
                                  PlaybackCurrentness *observed) {
    if (!valid_late(wanted.selected) || !ref(wanted.intent))
        return Disposition::refused;
    return guarded(s, [&] {
        const auto &request = wanted.selected;
        require_catalog(s);
        const auto key = origin(request.instance.target, profile);
        if (!validate_stop(s, request, key))
            return Disposition::refused;
        const auto existing = find_instance(s, request, profile);
        if (!handoff_matches(existing, wanted))
            return Disposition::refused;
        const auto currentness = owner.currentness(s, request);
        if (observed)
            *observed = currentness;
        if (!std::holds_alternative<PlaybackCurrent>(currentness))
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
             "AND track=? AND stop=? AND instance=? AND handoff IS NULL AND NOT EXISTS("
             "SELECT 1 FROM main.subathon_milestone_playback_evidence e "
             "WHERE (e.installation,e.track,e.stop,e.instance)="
             "(subathon_milestone_playback_instances.installation,"
             "subathon_milestone_playback_instances.track,subathon_milestone_playback_instances."
             "stop,"
             "subathon_milestone_playback_instances.instance)) RETURNING 1",
             args});
        if (rows.empty())
            return Disposition::refused;
        if (rows.size() != 1 || integer(rows[0][0]) != 1)
            corrupt();
        return Disposition::committed;
    });
}

} // namespace

Disposition stage_playback_handoff(const SqliteWorkSession &s, PlaybackAuthority &owner,
                                   const PlaybackHandoff &wanted, const AdmissionProfile *profile) {
    return mark_playback_handoff(s, owner, wanted, profile, nullptr);
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
        if (!handoff_matches(selected, wanted))
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
        const auto outcome = milestone_edit_sql::enumeration((*row)[0], PlaybackOutcome::uncertain);
        if (!selected->claimed && (outcome != PlaybackOutcome::obsolete || ordinal != 1))
            corrupt();
        return PlaybackOutcomeRecord{outcome, verified_ref((*row)[1])};
    });
}

namespace {

std::optional<PlaybackOutcome> retained_handoff(const SqliteWorkSession &s,
                                                const PlaybackHandoff &wanted,
                                                const Instance &selected) {
    const auto &request = wanted.selected;
    auto args = stop_key(request.track, request.stop);
    args.append(wanted.intent);
    const auto rows = s.execute(
        {"SELECT outcome FROM main.subathon_milestone_playback_evidence "
         "WHERE installation=? AND track=? AND stop=? AND instance=? ORDER BY ordinal DESC "
         "LIMIT 1",
         args});
    if (rows.empty())
        return selected.claimed ? std::optional{PlaybackOutcome::uncertain} : std::nullopt;
    const auto outcome =
        milestone_edit_sql::enumeration(rows.front().front(), PlaybackOutcome::uncertain);
    if (!selected.claimed && outcome != PlaybackOutcome::obsolete)
        corrupt();
    return outcome;
}

void retire_obsolete(const SqliteWorkSession &s, const PlaybackHandoff &wanted,
                     const PlaybackProvenObsolete &obsolete) {
    const auto &request = wanted.selected;
    if (!ref(obsolete.observed_generation) ||
        obsolete.observed_generation == request.instance.runtime_generation)
        return;
    const auto selected = find_instance(s, request, nullptr);
    if (!handoff_matches(selected, wanted) || selected->claimed ||
        retained_handoff(s, wanted, *selected))
        return;
    auto args = stop_key(request.track, request.stop);
    args.append({wanted.intent, 1, int(PlaybackOutcome::obsolete), obsolete.observed_generation});
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_playback_evidence VALUES(?,?,?,?,?,?,?)", args});
}

bool normal(const SqliteWorkResult &work) {
    return work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
           work.value.has_value();
}

} // namespace

MilestonePlaybackCompletion process_milestone_playback_turn(SqliteWriter &writer,
                                                            MilestonePlaybackTurn turn,
                                                            const PlaybackHandoff &wanted,
                                                            PlaybackAuthority &authority,
                                                            PlaybackStopAdapter &adapter) {
    MilestonePlaybackCompletion result{};
    static thread_local bool processing = false;
    if (processing) {
        result.work.error = WorkError::busy;
        return result;
    }
    if (!valid_late(wanted.selected) ||
        (turn != MilestonePlaybackTurn::late_activation && !ref(wanted.intent))) {
        result.disposition = Disposition::refused;
        return result;
    }
    processing = true;
    const auto leave = qScopeGuard([] { processing = false; });
    auto disposition = Disposition::refused;
    std::optional<QByteArray> intent;
    result.work = writer.run_idle_owner_work([&](auto ctx) -> QVariant {
        const auto s = ctx.begin();
        switch (turn) {
        case MilestonePlaybackTurn::late_activation:
            disposition = stage_late_playback(s, authority, wanted.selected);
            if ((disposition == Disposition::committed || disposition == Disposition::replay) &&
                wanted.selected.instance.capability == StopCapability::supported) {
                const auto instance = find_instance(s, wanted.selected, nullptr);
                if (!instance || !instance->exact)
                    corrupt();
                intent = instance->reference;
            }
            break;
        case MilestonePlaybackTurn::cold_restore:
            require_catalog(s);
            if (validate_stop(s, wanted.selected,
                              origin(wanted.selected.instance.target, nullptr))) {
                const auto instance = find_instance(s, wanted.selected, nullptr);
                if (handoff_matches(instance, wanted)) {
                    disposition = Disposition::replay;
                    (void)retained_handoff(s, wanted, *instance);
                }
            }
            break;
        case MilestonePlaybackTurn::handoff: {
            PlaybackCurrentness observed;
            disposition = mark_playback_handoff(s, authority, wanted, nullptr, &observed);
            if (const auto *obsolete = std::get_if<PlaybackProvenObsolete>(&observed))
                retire_obsolete(s, wanted, *obsolete);
            break;
        }
        }
        s.commit();
        return 1;
    });
    if (!normal(result.work))
        return result;
    result.disposition = disposition;
    result.intent = std::move(intent);
    if (turn == MilestonePlaybackTurn::handoff && disposition == Disposition::committed) {
        try {
            adapter.request_stop(wanted);
        } catch (...) {
            // The marker remains uncertain until exact adapter evidence is read.
            result.adapter_threw = true;
        }
    }
    return result;
}

RestoredMilestoneQueueCompletion
restore_interrupted_milestone_queue(SqliteWriter &writer, CommandAuthority &authority,
                                    const Command &stop, const std::optional<Command> &next,
                                    const std::optional<PlaybackHandoff> &handoff) {
    RestoredMilestoneQueueCompletion completion;
    const auto *intent = std::get_if<StopIntent>(&stop.intent);
    if (!intent)
        return completion;

    completion.work = writer.run_idle_owner_work([&](auto ctx) -> QVariant {
        const auto s = ctx.begin();
        const auto stopped = read_command(s, authority, stop);
        completion.disposition = stopped.disposition;
        completion.authoritative_absence = stopped.authoritative_absence;
        if (stopped.disposition == Disposition::replay) {
            const auto rows = s.execute(
                {"SELECT CASE WHEN typeof(cause)='blob' AND length(cause) BETWEEN 1 AND 4096 "
                 "THEN cause END,resume IS NULL FROM main.subathon_milestone_interruption_causes "
                 "WHERE installation=? AND track=? AND stop=? LIMIT 2",
                 stop_key(stop.track, stop.operation)});
            const auto *cause = unique(rows);
            if (bool(cause) != !stopped.added_causes.empty() ||
                (cause && verified_ref(cause->front()) != stopped.added_causes.front()))
                corrupt();
            completion.durable_holds.emplace();
            if (cause && integer((*cause)[1]) == 1)
                completion.durable_holds->push_back({stop.track, stopped.added_causes.front()});

            std::optional<Instance> playback;
            if (handoff && valid_late(handoff->selected) && ref(handoff->intent) &&
                stop.track == handoff->selected.track && stop.operation == handoff->selected.stop &&
                same(intent->target, handoff->selected.instance.target) &&
                std::ranges::find(stopped.playback_intents, handoff->intent) !=
                    stopped.playback_intents.end() &&
                validate_stop(s, handoff->selected, origin(intent->target, nullptr))) {
                playback = find_instance(s, handoff->selected, nullptr);
            }
            if ((handoff && !handoff_matches(playback, *handoff)) ||
                (next && stop.track != next->track)) {
                completion.disposition = Disposition::refused;
            } else if (next) {
                auto read = read_occurrence_selection_with_disposition(s, authority, *next);
                if (const auto *failure = std::get_if<Disposition>(&read)) {
                    completion.disposition = *failure;
                    if (*failure == Disposition::denied || *failure == Disposition::unavailable)
                        completion.durable_holds.reset();
                } else {
                    auto &selected = std::get<OccurrenceSelection>(read);
                    auto &queue = completion.queue.emplace();
                    queue.selected = std::move(selected.selected_versions);
                    for (auto &hold : selected.interruption_holds)
                        queue.holds.push_back({stop.track, std::move(hold)});
                    if (handoff)
                        queue.prior_handoff = retained_handoff(s, *handoff, *playback);
                }
            }
        }
        s.commit();
        return completion.queue.has_value();
    });
    if (!normal(completion.work))
        return {std::move(completion.work), {}, {}, {}, false};
    return completion;
}
} // namespace subathon::server::milestone_interruption
