// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "command_frame_oracle.hpp"

#include <array>

namespace storage_027_fixture::charge_oracle {

// The proposed statements are not invoked by the tests. Each node describes
// the whole attempted SQL text, bound payload lengths and returned cells for
// the exact fixture branch. Numeric values and SQL NULL have no payload.
struct Charge {
    qsizetype attempts{}, peak{}, binds{}, rows{}, results{};
    bool operator==(const Charge &) const = default;
};
using Cells = std::vector<qsizetype>;

inline constexpr qsizetype sum(qsizetype left, qsizetype right) {
    if (left < 0 || right < 0 || left > std::numeric_limits<qsizetype>::max() - right)
        throw std::overflow_error("Fixture SQL charge overflow");
    return left + right;
}
inline constexpr qsizetype cells(const Cells &fields) {
    qsizetype bytes = 0;
    for (const auto width : fields)
        bytes = sum(bytes, sum(8, width));
    return bytes;
}
inline constexpr Charge combine(const Charge &a, const Charge &b) {
    return {sum(a.attempts, b.attempts), std::max(a.peak, b.peak), sum(a.binds, b.binds),
            sum(a.rows, b.rows), sum(a.results, b.results)};
}
inline constexpr Charge statement(std::string_view sql, const Cells &bind = {},
                                  std::initializer_list<Cells> rows = {}) {
    Charge shape{1, qsizetype(sql.size()), cells(bind), qsizetype(rows.size()), 0};
    if (std::ranges::count(sql, '?') != std::ssize(bind))
        throw std::invalid_argument("Fixture SQL bind inventory differs from its shape");
    for (const auto &row : rows)
        shape.results = sum(shape.results, cells(row));
    return shape;
}
inline constexpr Charge repeat(Charge member, qsizetype count) {
    Charge shape;
    for (qsizetype i = 0; i < count; ++i)
        shape = combine(shape, member);
    return shape;
}
inline std::array<qsizetype, 5> dimensions(Charge q) {
    return {q.attempts, q.peak, q.binds, q.rows, q.results};
}
inline StorageLimits limits(Charge q) {
    // SQLite writer requires positive maxima even for a branch with zero use.
    return {64,
            std::max(q.attempts, qsizetype(1)),
            std::max(q.peak, qsizetype(1)),
            std::max(q.binds, qsizetype(1)),
            std::max(q.rows, qsizetype(1)),
            std::max(q.results, qsizetype(1))};
}
inline StorageLimits one_below(Charge q, int axis) {
    constexpr std::array fields{&StorageLimits::statements_per_batch,
                                &StorageLimits::statement_utf8_bytes,
                                &StorageLimits::bound_value_bytes, &StorageLimits::result_rows,
                                &StorageLimits::result_value_bytes};
    const auto value = dimensions(q).at(axis);
    if (value <= 1)
        throw std::invalid_argument("One-below writer limit has no positive representation");
    auto result = StorageLimits{64, 1024, 65536, 16777216, 8192, 16777216};
    result.*fields.at(axis) = value - 1;
    return result;
}

inline qsizetype cap(Charge fixed, Charge member, Charge allowance, qsizetype owner) {
    const auto f = dimensions(fixed), m = dimensions(member), l = dimensions(allowance);
    for (size_t i = 0; i < f.size(); ++i) {
        if (f[i] > l[i] || (i == 1 && m[i] > l[i]))
            return 0;
        if (i != 1 && m[i] > 0)
            owner = std::min(owner, (l[i] - f[i]) / m[i]);
    }
    return owner;
}

enum class Branch {
    malformed,
    denied,
    absent,
    conflict,
    replay,
    readback,
    replace_preview,
    replace_automation,
    replace_bonus,
    stop_continue,
    stop_hold,
    stop_started,
    resume,
    resume_ineligible,
    late_add,
    late_duplicate,
    late_exhausted,
    handoff,
    handoff_denied,
    outcome,
    outcome_duplicate
};

inline constexpr qsizetype action_limit = 4096;

struct Record {
    qsizetype install = 7;
    qsizetype track = 5;
    qsizetype operation{};
    qsizetype actor{};
    qsizetype lineage{};
    qsizetype decision{};
    qsizetype origin_scope{};
    qsizetype origin{};
    qsizetype occurrence{};
    qsizetype intent{};
    qsizetype result{};
    qsizetype retained_intent{};
    qsizetype retained_result{};
    qsizetype definition{22};
    qsizetype prefix{18};
    bool edited{true};
    qsizetype cause{};
};

inline constexpr Charge catalog() {
    // Full prefix readiness is separately verified by SqliteMigrationRunner.
    // These attempts recheck the exact operative tail and new structure before
    // any selected reads. A zero-limit query resolves columns and constraints'
    // owning tables without copying payload.
    const auto tail = combine(
        statement("SELECT CASE WHEN typeof(version)='integer' AND typeof(migration_id)='text' "
                  "AND version=? AND migration_id=? COLLATE BINARY THEN 1 ELSE 0 END "
                  "FROM subathon_migration_history ORDER BY version DESC LIMIT 1",
                  {0, 38}, {{0}}),
        statement("SELECT j.kind,f.origin_kind,v.predecessor,a.revision,b.actual,c.ordinal,"
                  "h.resume,p.handoff,e.outcome FROM subathon_milestone_command_journal j "
                  "CROSS JOIN subathon_milestone_stop_fences f "
                  "CROSS JOIN subathon_milestone_slot_versions v "
                  "CROSS JOIN subathon_milestone_slot_attempts a "
                  "CROSS JOIN subathon_milestone_version_bonus b "
                  "CROSS JOIN subathon_milestone_stop_cancellations c "
                  "CROSS JOIN subathon_milestone_interruption_causes h "
                  "CROSS JOIN subathon_milestone_playback_instances p "
                  "CROSS JOIN subathon_milestone_playback_evidence e LIMIT 0"));
    return combine(statement("SELECT singleton,catalog_format FROM subathon_catalog "
                             "WHERE singleton=1 AND store_identity=? LIMIT 2",
                             {21}, {{0, 0}}),
                   tail);
}

inline constexpr Charge journal_meta(const Record &r, bool present) {
    // Metadata returns one discriminator, exact intent/result lengths and
    // the retained decision reference. Exact disclosure is checked by the
    // supplied owner port before pulling the documents.
    constexpr auto sql = "SELECT kind,length(intent),length(result),decision FROM "
                         "subathon_milestone_command_journal "
                         "WHERE installation=? AND track=? AND operation=? LIMIT 2";
    const Cells key{r.install, r.track, r.operation};
    return present ? statement(sql, key, {{0, 0, 0, r.decision}}) : statement(sql, key);
}
inline constexpr Charge journal_documents(const Record &r, bool include_result) {
    if (include_result)
        return statement("SELECT intent,result FROM subathon_milestone_command_journal "
                         "WHERE installation=? AND track=? AND operation=? LIMIT 2",
                         {r.install, r.track, r.operation},
                         {{r.retained_intent, r.retained_result}});
    return statement("SELECT intent FROM subathon_milestone_command_journal "
                     "WHERE installation=? AND track=? AND operation=? LIMIT 2",
                     {r.install, r.track, r.operation}, {{r.retained_intent}});
}
inline constexpr Charge journal_write(const Record &r) {
    return statement(
        "INSERT INTO subathon_milestone_command_journal"
        "(installation,track,operation,kind,actor,lineage,decision,intent,result) "
        "VALUES(?,?,?,?,?,?,?,?,?)",
        {r.install, r.track, r.operation, 0, r.actor, r.lineage, r.decision, r.intent, r.result});
}

inline constexpr Charge target(const Record &r, bool edit, bool present) {
    constexpr auto edit_sql =
        "SELECT g.released,g.queue_order,g.edit_ref,d.definition_revision,h.prefix_revision,"
        "c.choice,c.resolution FROM subathon_milestone_edit_occurrences g "
        "JOIN subathon_milestone_definition_heads d USING(installation_ref,track_ref) "
        "JOIN subathon_milestone_prefix_heads h USING(installation_ref,track_ref) "
        "JOIN subathon_milestone_edit_choices c USING(occurrence_ref) "
        "WHERE g.installation_ref=? AND g.track_ref=? AND g.edit_ref=? AND g.occurrence_ref=? "
        "LIMIT 2";
    constexpr auto app_sql =
        "SELECT g.released,g.queue_order,g.operation_ref,d.definition_revision,h.prefix_revision,"
        "c.choice,c.resolution FROM subathon_milestone_groups g "
        "LEFT JOIN subathon_milestone_acceptance_decisions c USING(occurrence_ref) "
        "JOIN subathon_milestone_prefix_members m ON m.application_occurrence_ref=g.occurrence_ref "
        "AND m.operation_scope=g.operation_scope AND m.operation_ref=g.operation_ref "
        "JOIN subathon_milestone_prefix_heads h USING(installation_ref,track_ref,prefix_revision) "
        "JOIN subathon_milestone_definition_heads d USING(installation_ref,track_ref) "
        "WHERE m.installation_ref=? AND m.track_ref=? AND g.operation_scope=? "
        "AND g.operation_ref=? AND g.occurrence_ref=? LIMIT 2";
    const Cells edit_key{r.install, r.track, r.origin, r.occurrence};
    const Cells app_key{r.install, r.track, r.origin_scope, r.origin, r.occurrence};
    // An ordinary application has no paused-acceptance row. The two
    // projected NULLs still charge the same cell overhead as enum integers.
    const Cells closure{0, 0, r.origin, r.definition, r.prefix, 0, 0};
    const auto check =
        present ? statement(edit ? edit_sql : app_sql, edit ? edit_key : app_key, {closure})
                : statement(edit ? edit_sql : app_sql, edit ? edit_key : app_key);
    if (!present)
        return check;
    const auto fence =
        statement("SELECT 1 FROM subathon_milestone_stop_fences WHERE installation=? AND track=? "
                  "AND origin_kind=? AND origin_scope=? AND origin_ref=? AND occurrence=? LIMIT 2",
                  {r.install, r.track, 0, r.origin_scope, r.origin, r.occurrence});
    // Protected test targets here have no refund Review. The explicit excess
    // probe is independent of the exact selected decision row above.
    const auto holds =
        statement(edit ? "SELECT length(refund_ref) FROM subathon_milestone_edit_refund_decisions "
                         "WHERE occurrence_ref=? AND policy=1 AND resolution IS NULL AND "
                         "ended_by_skip_ref IS NULL LIMIT ?"
                       : "SELECT length(refund_ref) FROM subathon_milestone_refund_decisions "
                         "WHERE occurrence_ref=? AND policy=1 AND resolution IS NULL AND "
                         "ended_by_skip_ref IS NULL LIMIT ?",
                  {r.occurrence, 0});
    return combine(check, combine(fence, holds));
}

struct Slot {
    qsizetype original_action, original_version, original_payload, original_route, original_sender;
    qsizetype action, version, payload, route, sender;
    bool bonus{}, started{};
    qsizetype job_ref{};
    qsizetype allocated_version{};
};

inline constexpr Charge original_attempt(const Record &r, qsizetype job_ref) {
    if (r.edited)
        return statement("SELECT revision,state,attempt FROM subathon_milestone_edit_events "
                         "WHERE occurrence_ref=? AND action_ordinal=? AND revision=? LIMIT 2",
                         {r.occurrence, 0, 0}, {{0, 0, 0}});
    return statement(
        "SELECT revision,state,attempt FROM subathon_delivery_events "
        "WHERE operation_scope=? AND operation_ref=? AND job_ref=? AND revision=? LIMIT 2",
        {r.origin_scope, r.origin, job_ref, 0}, {{0, 0, 0}});
}

inline constexpr Charge original_slot(const Record &r, const Slot &m, bool edit = true) {
    const auto metadata = statement(
        edit ? "SELECT "
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
        {r.occurrence, 0}, {{0, 0, 0, 0, 0, 0, 0, 0}});
    const auto selected = statement(
        "SELECT length(version),length(payload),length(route),length(qualified_sender) "
        "FROM subathon_milestone_slot_versions v WHERE installation=? AND track=? AND "
        "origin_kind=? "
        "AND origin_scope=? AND origin_ref=? AND occurrence=? AND ordinal=? AND NOT EXISTS("
        "SELECT 1 FROM subathon_milestone_slot_versions n WHERE n.installation=v.installation "
        "AND n.track=v.track AND n.origin_kind=v.origin_kind AND n.origin_scope=v.origin_scope "
        "AND n.origin_ref=v.origin_ref AND n.occurrence=v.occurrence AND n.ordinal=v.ordinal "
        "AND n.predecessor=v.version) LIMIT 2",
        {r.install, r.track, 0, r.origin_scope, r.origin, r.occurrence, 0});
    auto planned = combine(metadata, selected);
    if (!m.started)
        return planned;
    return combine(planned, original_attempt(r, m.job_ref));
}

inline constexpr Charge replacement_member(const Record &r, const Slot &m, bool write) {
    auto q = original_slot(r, m, r.edited);
    const auto payload = statement(
        r.edited
            ? "SELECT a.action_ref,a.version_ref,j.action_document,c.route_ref,c.sender_ref "
              "FROM subathon_milestone_edit_actions a LEFT JOIN subathon_milestone_edit_jobs j "
              "USING(occurrence_ref,action_ordinal) LEFT JOIN subathon_milestone_edit_captures c "
              "USING(occurrence_ref,action_ordinal) WHERE a.occurrence_ref=? AND "
              "a.action_ordinal=? LIMIT 2"
            : "SELECT action_ref,version_ref,NULL,NULL,NULL FROM subathon_milestone_actions "
              "WHERE occurrence_ref=? AND action_ordinal=? LIMIT 2",
        {r.occurrence, 0},
        {{m.original_action, m.original_version, m.original_payload, m.original_route,
          m.original_sender}});
    q = combine(q, payload);
    if (!write)
        return q;
    q = combine(
        q,
        statement(
            "INSERT INTO subathon_milestone_slot_versions VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            {r.install, r.track, r.operation, 0, r.origin_scope, r.origin, r.occurrence, 0,
             m.allocated_version, 0, 0, m.action, m.version, m.payload, m.route, m.sender}));
    if (!m.bonus)
        q = combine(q,
                    statement("INSERT INTO subathon_milestone_slot_attempts VALUES(?,?,?,1,0,NULL)",
                              {r.install, r.track, m.allocated_version}));
    return q;
}

inline constexpr Charge stop_fence(const Record &r, bool hold) {
    auto q = statement(
        "INSERT INTO subathon_milestone_stop_fences VALUES(?,?,?,?,?,?,?,?,?)",
        {r.install, r.track, r.operation, 0, r.origin_scope, r.origin, r.occurrence, 0, 0});
    if (hold)
        q = combine(
            q, statement("INSERT INTO subathon_milestone_interruption_causes VALUES(?,?,?,?,NULL)",
                         {r.install, r.track, r.cause, r.operation}));
    return q;
}
inline constexpr Charge stop_probe(const Record &r) {
    // A bounded cap+1 probe returns count only, but still incurs one returned cell.
    // Per-slot metadata and bodies are charged separately.
    return statement(
        r.edited
            ? "SELECT count(*) FROM (SELECT action_ordinal FROM subathon_milestone_edit_actions "
              "WHERE occurrence_ref=? ORDER BY action_ordinal LIMIT ?)"
            : "SELECT count(*) FROM (SELECT action_ordinal FROM subathon_milestone_actions "
              "WHERE occurrence_ref=? ORDER BY action_ordinal LIMIT ?)",
        {r.occurrence, 0}, {{0}});
}
inline constexpr Charge stop_member(const Record &r, const Slot &m) {
    // Metadata preflights this exact payload before any packet is copied.
    // The separately bounded pull validates the original action reference
    // rather than treating a length alone as identifying the selected slot.
    auto q = combine(original_slot(r, m, r.edited),
                     statement(r.edited ? "SELECT action_ref FROM subathon_milestone_edit_actions "
                                          "WHERE occurrence_ref=? AND action_ordinal=? LIMIT 2"
                                        : "SELECT action_ref FROM subathon_milestone_actions "
                                          "WHERE occurrence_ref=? AND action_ordinal=? LIMIT 2",
                               {r.occurrence, 0}, {{m.original_action}}));
    if (!m.started)
        q = combine(
            q, statement("INSERT INTO subathon_milestone_stop_cancellations VALUES(?,?,?,?,NULL)",
                         {r.install, r.track, r.operation, 0}));
    return q;
}

struct Cause {
    qsizetype ref{}, stop{}, occurrence{};
};
inline constexpr Charge cause_member(const Record &r, const Cause &m, bool present, bool write) {
    constexpr auto sql =
        "SELECT h.stop,h.resume,f.occurrence,j.decision FROM "
        "subathon_milestone_interruption_causes h "
        "JOIN subathon_milestone_stop_fences f ON f.installation=h.installation AND "
        "f.track=h.track "
        "AND f.stop=h.stop JOIN subathon_milestone_command_journal j ON "
        "j.installation=h.installation "
        "AND j.track=h.track AND j.operation=h.stop WHERE h.installation=? AND h.track=? "
        "AND h.cause=? LIMIT 2";
    const Cells bind{r.install, r.track, m.ref};
    auto q = present ? statement(sql, bind, {{m.stop, 0, m.occurrence, r.decision}})
                     : statement(sql, bind);
    if (write)
        q = combine(
            q,
            statement("UPDATE subathon_milestone_interruption_causes SET resume=? "
                      "WHERE installation=? AND track=? AND cause=? AND resume IS NULL RETURNING 1",
                      {r.operation, r.install, r.track, m.ref}, {{0}}));
    return q;
}

struct Playback {
    qsizetype stop{}, adapter{}, activation{}, generation{}, evidence{}, job_ref{};
    qsizetype instance{}, handoff{}, attempt{};
};

inline constexpr Charge playback_fixed(const Record &r, const Playback &p) {
    auto q = statement(
        "SELECT f.occurrence,f.origin_kind,f.origin_scope,f.origin_ref,f.playback_cap,j.decision "
        "FROM "
        "subathon_milestone_stop_fences f "
        "JOIN subathon_milestone_command_journal j ON j.installation=f.installation AND "
        "j.track=f.track "
        "AND j.operation=f.stop WHERE f.installation=? AND f.track=? AND f.stop=? LIMIT 2",
        {r.install, r.track, p.stop}, {{r.occurrence, 0, r.origin_scope, r.origin, 0, r.decision}});
    return combine(q, original_attempt(r, p.job_ref));
}

inline constexpr Charge playback_lookup(const Record &r, const Playback &p, bool present) {
    constexpr auto sql =
        "SELECT instance,capability,length(handoff) FROM subathon_milestone_playback_instances "
        "WHERE installation=? AND track=? AND stop=? AND ordinal=? AND adapter=? AND activation=? "
        "AND generation=? LIMIT 2";
    const Cells bind{r.install, r.track, p.stop, 0, p.adapter, p.activation, p.generation};
    return present ? statement(sql, bind, {{p.instance, 0, 0}}) : statement(sql, bind);
}
inline constexpr Charge playback_count(const Record &r, const Playback &p) {
    return statement(
        "SELECT count(*) FROM (SELECT instance FROM subathon_milestone_playback_instances "
        "WHERE installation=? AND track=? AND stop=? LIMIT ?)",
        {r.install, r.track, p.stop, 0}, {{0}});
}
inline constexpr Charge playback_insert(const Record &r, const Playback &p) {
    return statement(
        "INSERT INTO subathon_milestone_playback_instances VALUES(?,?,?,?,?,?,?,?,?,?,?,NULL)",
        {r.install, r.track, p.stop, p.instance, 0, 0, p.adapter, p.attempt, p.activation,
         p.generation, 0});
}
inline constexpr Charge playback_owner_update(const Record &r, const Playback &p) {
    return statement(
        "UPDATE subathon_milestone_playback_instances SET handoff=? WHERE installation=? "
        "AND track=? AND stop=? AND instance=? AND handoff IS NULL RETURNING 1",
        {p.handoff, r.install, r.track, p.stop, p.instance}, {{0}});
}
inline constexpr Charge outcome_head(const Record &r, const Playback &p, bool existing) {
    constexpr auto sql =
        "SELECT ordinal,outcome,length(evidence) FROM subathon_milestone_playback_evidence "
        "WHERE installation=? AND track=? AND stop=? AND instance=? ORDER BY ordinal DESC LIMIT 1";
    const Cells key{r.install, r.track, p.stop, p.instance};
    return existing ? statement(sql, key, {{0, 0, 0}}) : statement(sql, key);
}
inline constexpr Charge outcome_write(const Record &r, const Playback &p) {
    return statement("INSERT INTO subathon_milestone_playback_evidence VALUES(?,?,?,?,?,?,?)",
                     {r.install, r.track, p.stop, p.instance, 0, 0, p.evidence});
}

inline constexpr Charge playback_capacity_fixed(const Record &r, const Playback &p) {
    return combine(playback_fixed(r, p), playback_count(r, p));
}
inline constexpr Charge playback_capacity_member(const Record &r, const Playback &p) {
    auto q = combine(playback_lookup(r, p, false), playback_insert(r, p));
    q.binds = sum(q.binds, 4 + p.instance);
    return q;
}

inline constexpr Charge playback_capacity_plan(const Record &r, const Playback &p,
                                               const std::vector<Playback> &members,
                                               qsizetype framed_member_bytes) {
    auto q = playback_capacity_fixed(r, p);
    for (const auto &member : members) {
        q = combine(q, playback_lookup(r, member, false));
        q = combine(q, playback_insert(r, member));
    }
    q.binds = sum(q.binds, framed_member_bytes);
    return q;
}

inline constexpr Charge plan(Branch branch, const Record &r, const std::vector<Slot> &members = {},
                             const std::vector<Cause> &causes = {}, Playback p = {}) {
    if (branch == Branch::malformed || branch == Branch::denied)
        return {}; // Shape/authority port refuses before opening the SQL path.
    auto q = catalog();
    const auto add = [&](Charge next) { q = combine(q, next); };
    switch (branch) {
    case Branch::malformed:
    case Branch::denied:
        break;
    case Branch::conflict:
    case Branch::replay:
    case Branch::readback:
        add(journal_meta(r, true));
        add(journal_documents(r, branch != Branch::conflict));
        break;
    case Branch::absent:
        add(journal_meta(r, false));
        add(target(r, r.edited, false));
        break;
    case Branch::replace_preview:
    case Branch::replace_automation:
    case Branch::replace_bonus:
        add(journal_meta(r, false));
        add(target(r, r.edited, true));
        for (const auto &slot : members)
            add(replacement_member(r, slot, branch != Branch::replace_preview));
        if (branch != Branch::replace_preview)
            add(journal_write(r)); // Staged before its children; order doesn't change totals.
        break;
    case Branch::stop_continue:
    case Branch::stop_hold:
    case Branch::stop_started:
        add(journal_meta(r, false));
        add(target(r, r.edited, true));
        add(stop_probe(r));
        add(journal_write(r));
        add(stop_fence(r, branch == Branch::stop_hold));
        for (const auto &slot : members)
            add(stop_member(r, slot));
        if (branch == Branch::stop_started)
            add(playback_insert(r, p));
        break;
    case Branch::resume:
    case Branch::resume_ineligible:
        add(journal_meta(r, false));
        for (size_t i = 0; i < causes.size(); ++i)
            add(cause_member(r, causes[i], branch == Branch::resume || i != causes.size() - 1,
                             branch == Branch::resume));
        if (branch == Branch::resume)
            add(journal_write(r));
        break;
    case Branch::late_add:
    case Branch::late_duplicate:
    case Branch::late_exhausted:
        add(playback_fixed(r, p));
        add(playback_lookup(r, p, branch == Branch::late_duplicate));
        if (branch != Branch::late_duplicate)
            add(playback_count(r, p));
        if (branch == Branch::late_add)
            add(playback_insert(r, p));
        break;
    case Branch::handoff:
    case Branch::handoff_denied:
        add(playback_fixed(r, p));
        add(playback_lookup(r, p, true));
        if (branch == Branch::handoff)
            add(playback_owner_update(r, p));
        break;
    case Branch::outcome:
    case Branch::outcome_duplicate:
        add(playback_fixed(r, p));
        add(playback_lookup(r, p, true));
        add(outcome_head(r, p, branch == Branch::outcome_duplicate));
        if (branch == Branch::outcome) {
            add(outcome_write(r, p));
        } else {
            add(statement(
                "SELECT evidence FROM subathon_milestone_playback_evidence WHERE "
                "installation=? AND track=? AND stop=? AND instance=? AND ordinal=? LIMIT 2",
                {r.install, r.track, p.stop, p.instance, 0}, {{p.evidence}}));
        }
        break;
    }
    return q;
}

// Offline byte count of the literals plus the exact fixture cell widths.
// These anchors catch an accidental change in the tape before any admission.
static_assert([] {
    Record r;
    r.operation = 6;
    r.actor = 6;
    r.lineage = 15;
    r.decision = 17;
    r.origin_scope = 5;
    r.origin = 6;
    r.occurrence = 6;
    r.intent = 57;
    r.result = 35;
    const std::vector<Slot> members(2, {8, 11, 2, 11, 11, 8, 11, 2, 15, 10});
    if (plan(Branch::stop_continue, r, members) != Charge{18, 660, 987, 8, 294})
        return false;
    r.result = 71;
    r.cause = 32; // Synthetic arithmetic anchor only.
    if (plan(Branch::stop_hold, r, members) != Charge{19, 660, 1105, 8, 294})
        return false;
    r.intent = 111;
    r.result = 81;
    return plan(Branch::resume, r, {}, {{32, 6, 15}, {32, 6, 6}}) == Charge{9, 536, 745, 6, 171};
}());

static_assert([] {
    // Compile-time exercise of every literal placeholder inventory. The
    // branch tests separately select real owner-produced values at runtime.
    const Record r{7, 5, 6, 6, 15, 17, 5, 6, 6, 121, 71, 57, 71};
    const std::vector<Slot> members{{8, 11, 2, 11, 11, 8, 11, 2, 15, 10}};
    const std::vector<Cause> causes{{32, 6, 6}};
    const Playback media{6, 13, 8, 9, 14};
    for (int current = int(Branch::absent); current <= int(Branch::outcome_duplicate); ++current)
        if (plan(static_cast<Branch>(current), r, members, causes, media).attempts < 3)
            return false;
    return true;
}());

} // namespace storage_027_fixture::charge_oracle
