// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_terminal_partition.hpp"
#include "sqlite_milestone_interruption_schema.hpp"

#include <QMetaType>
#include <string_view>
#include <subathon/server/sqlite_application_bundle.hpp>

namespace subathon::server::milestone_sql {

[[noreturn]] inline void invalid() {
    throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_job);
}

[[noreturn]] inline void corrupt() {
    throw SqliteBundleStorageFailure(SqliteBundleStorageReason::invalid_stored_data);
}

// A terminal partition must be verified before its origin can authorize an
// achievement or pending-action operation. Historical result formats follow
// the existing milestone guards.
inline void verify_partitioned_origin(const SqliteWorkSession &session,
                                      const ApplicationOperationKey &key) {
    const auto rows = session.execute(
        {"SELECT typeof(format_ref)='blob' AND format_ref=? FROM main.subathon_app_results "
         "WHERE operation_scope=? AND operation_ref=?",
         {QByteArray(terminal_partition::result_format),
          QByteArray::fromStdString(key.operation_scope),
          QByteArray::fromStdString(key.operation_ref)}});
    if (rows.size() != 1 ||
        (rows.front().front().toBool() && !SqliteApplicationBundleStore(session).lookup(key)))
        corrupt();
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

inline bool bounded_ref(std::string_view ref) { return !ref.empty() && ref.size() <= 4096; }

inline QByteArray blob(std::string_view ref) {
    if (!bounded_ref(ref))
        invalid();
    return QByteArray(ref.data(), qsizetype(ref.size()));
}

inline QString ref_column(const QString &column) {
    return "CASE WHEN typeof(" + column + ")='blob' AND length(" + column +
           ") BETWEEN 1 AND 4096 THEN " + column + " END";
}

inline QString integer_column(const QString &column) {
    return "CASE WHEN typeof(" + column + ")='integer' THEN " + column + " END";
}

inline std::string reference(const QVariant &cell) {
    if (cell.isNull() || cell.metaType().id() != QMetaType::QByteArray)
        corrupt();
    const auto bytes = cell.toByteArray();
    if (!bounded_ref({bytes.constData(), std::size_t(bytes.size())}))
        corrupt();
    return bytes.toStdString();
}

inline qint64 integer(const QVariant &cell) {
    const auto type = cell.metaType().id();
    if (cell.isNull() || (type != QMetaType::Int && type != QMetaType::LongLong))
        corrupt();
    return cell.toLongLong();
}

enum class Catalog { independent, groups, decisions, edits };

// Reuse the catalog marker at the dispatch and queue boundaries. A newer or
// mismatched marker cannot fall through to independent delivery or v12 order.
inline void require_edit_schema(const SqliteWorkSession &session) {
    // Resolve each required table and key even when the selected row is absent.
    // Startup inspection owns the full SQL and index projection comparison.
    (void)session.execute(
        {"SELECT v.definition_revision,t.ordinal,d.definition_revision,p.prefix_length,"
         "h.prefix_revision,m.origin,m.application_occurrence_ref,m.edit_occurrence_ref,"
         "o.intent,o.result,g.suffix_ordinal,g.queue_order,g.released,a.action_ordinal,a.kind,"
         "j.job_format,j.action_document,c.route_ref,c.sender_selection,c.sender_ref,"
         "e.revision,e.state,e.attempt,e.evidence_ref,b.effect_ref,i.choice,i.actor_ref,"
         "i.decision_ref,i.resolution,i.ended_by_skip_ref,r.refund_ref,r.score_before_micro_points,"
         "r.score_after_micro_points,r.current_target_micro_points,r.policy,r.resolution,"
         "r.ended_by_skip_ref "
         "FROM main.subathon_milestone_definition_versions v "
         "CROSS JOIN main.subathon_milestone_definition_targets t "
         "CROSS JOIN main.subathon_milestone_definition_heads d "
         "CROSS JOIN main.subathon_milestone_prefix_revisions p "
         "CROSS JOIN main.subathon_milestone_prefix_heads h "
         "CROSS JOIN main.subathon_milestone_prefix_members m "
         "CROSS JOIN main.subathon_milestone_edit_operations o "
         "CROSS JOIN main.subathon_milestone_edit_occurrences g "
         "INDEXED BY subathon_milestone_edit_pending_order "
         "CROSS JOIN main.subathon_milestone_edit_actions a "
         "CROSS JOIN main.subathon_milestone_edit_jobs j "
         "CROSS JOIN main.subathon_milestone_edit_captures c "
         "CROSS JOIN main.subathon_milestone_edit_events e "
         "CROSS JOIN main.subathon_milestone_edit_bonus_receipts b "
         "CROSS JOIN main.subathon_milestone_edit_choices i "
         "CROSS JOIN main.subathon_milestone_edit_refund_decisions r "
         "WHERE g.released=0 LIMIT 0",
         {}});
}

inline Catalog catalog(const SqliteWorkSession &session, bool *interruptions = nullptr) {
    const auto rows = session.execute(
        {"SELECT CASE "
         "WHEN typeof(version)!='integer' THEN NULL "
         "WHEN version BETWEEN 1 AND 10 OR "
         "(version=11 AND migration_id='delivery-jobs-v1') THEN 0 "
         "WHEN version=12 AND migration_id='milestone-groups-v1' THEN 1 "
         "WHEN (version=13 AND migration_id='milestone-group-decisions-v1') OR "
         "(version=14 AND migration_id COLLATE BINARY='modifier-activations-v1') OR "
         "(version=15 AND migration_id COLLATE BINARY='financial-refunds-v1') OR "
         "(version=16 AND migration_id COLLATE BINARY='provider-credentials-v1') OR "
         "(version=17 AND migration_id COLLATE BINARY='rate-overrides-v1') THEN 2 "
         "WHEN version=18 AND migration_id COLLATE BINARY='milestone-target-edits-v1' THEN 3 "
         "WHEN version=19 AND migration_id COLLATE BINARY='contribution-review-v1' THEN 3 "
         "WHEN version=20 AND migration_id COLLATE BINARY='connector-intake-storage-v1' THEN 3 "
         "WHEN version=21 AND migration_id COLLATE BINARY="
         "'milestone-slot-interruption-storage-v1' THEN 4 END "
         "FROM main.subathon_migration_history "
         "WHERE EXISTS(SELECT 1 FROM main.subathon_catalog WHERE singleton=1 "
         "AND typeof(catalog_format)='integer' AND catalog_format=1 "
         "AND store_identity='subathon.sqlite-store') ORDER BY version DESC LIMIT 1",
         {}});
    if (rows.size() != 1)
        corrupt();
    const auto code = integer(rows.front().front());
    if (interruptions)
        *interruptions = code == 4;
    if (code == 4)
        (void)session.execute({milestone_interruption_schema::probe, {}});
    const auto result = code == 4 ? Catalog::edits : Catalog(code);
    if (result >= Catalog::decisions) {
        // Resolve the v13 columns even on paths with no matching occurrence or
        // membership. A missing table/column is a writer SQL failure.
        (void)session.execute(
            {"SELECT a.occurrence_ref,a.mode,a.choice,a.actor_ref,a.decision_ref,"
             "a.resolution,a.resolution_actor_ref,a.resolution_ref,"
             "a.ended_by_skip_kind,a.ended_by_skip_ref,"
             "r.occurrence_ref,r.refund_ref,r.score_before_micro_points,r.score_after_micro_points,"
             "r.current_target_micro_points,r.policy,r.resolution,r.resolution_actor_ref,"
             "r.resolution_ref,r.ended_by_skip_kind,r.ended_by_skip_ref "
             "FROM main.subathon_milestone_acceptance_decisions AS a "
             "CROSS JOIN main.subathon_milestone_refund_decisions AS r LIMIT 0",
             {}});
    }
    if (result == Catalog::edits)
        require_edit_schema(session);
    return result;
}

constexpr auto active = "resolution IS NULL AND ended_by_skip_kind IS NULL";

// Correlated against the exact group alias g. Terminal Skip is recoverable
// from the originating audit row and never changes the v12 release flag.
constexpr auto skipped =
    "(EXISTS(SELECT 1 FROM main.subathon_milestone_acceptance_decisions AS d "
    "WHERE d.occurrence_ref=g.occurrence_ref AND (d.choice=2 OR d.resolution=1)) "
    "OR EXISTS(SELECT 1 FROM main.subathon_milestone_refund_decisions AS d "
    "WHERE d.occurrence_ref=g.occurrence_ref AND (d.policy=2 OR d.resolution=1)))";

inline QString held() {
    return QString("(EXISTS(SELECT 1 FROM main.subathon_milestone_acceptance_decisions AS d "
                   "WHERE d.occurrence_ref=g.occurrence_ref AND d.choice=1 AND ") +
           active +
           ") OR EXISTS(SELECT 1 FROM main.subathon_milestone_refund_decisions AS d "
           "WHERE d.occurrence_ref=g.occurrence_ref AND d.policy=1 AND " +
           active + "))";
}

// The original keys are global within each lane. Correlate the complete
// typed membership before consulting a fence or selected successor.
inline QString interruption_origin(bool edit) {
    if (edit)
        return "f.origin_kind=2 AND f.occurrence=g.occurrence_ref AND "
               "(f.installation,f.track,f.origin_scope,f.origin_ref)="
               "(g.installation_ref,g.track_ref,g.track_ref,g.edit_ref)";
    return "f.origin_kind=1 AND f.occurrence=g.occurrence_ref AND "
           "(f.origin_scope,f.origin_ref)=(g.operation_scope,g.operation_ref) "
           "AND EXISTS(SELECT 1 FROM main.subathon_milestone_prefix_members m "
           "JOIN main.subathon_milestone_prefix_heads p USING(installation_ref,track_ref,"
           "prefix_revision) WHERE (m.installation_ref,m.track_ref,m.application_occurrence_ref,"
           "m.operation_scope,m.operation_ref)=(f.installation,f.track,f.occurrence,"
           "f.origin_scope,f.origin_ref))";
}

inline QString stopped(bool edit) {
    return "EXISTS(SELECT 1 FROM main.subathon_milestone_stop_fences f WHERE " +
           interruption_origin(edit) + ")";
}

inline QString release_blocked(bool edit) {
    const QString scope = edit
        ? "(h.installation,h.track)=(g.installation_ref,g.track_ref)"
        : "EXISTS(SELECT 1 FROM main.subathon_milestone_prefix_members m "
          "JOIN main.subathon_milestone_prefix_heads p USING(installation_ref,track_ref,"
          "prefix_revision) WHERE (m.installation_ref,m.track_ref,m.application_occurrence_ref,"
          "m.operation_scope,m.operation_ref)=(h.installation,h.track,g.occurrence_ref,"
          "g.operation_scope,g.operation_ref))";
    return "(" + stopped(edit) +
           " OR EXISTS(SELECT 1 FROM main.subathon_milestone_interruption_causes h "
           "WHERE h.resume IS NULL AND " + scope + "))";
}

inline QString original_start_fenced(bool edit, QString ordinal) {
    return "(" + stopped(edit) +
           " OR EXISTS(SELECT 1 FROM main.subathon_milestone_slot_versions f WHERE " +
           interruption_origin(edit) + " AND f.ordinal=" + ordinal + "))";
}

inline QString occurrence_table(bool edit) {
    return edit ? "main.subathon_milestone_edit_occurrences" : "main.subathon_milestone_groups";
}

inline bool fenced(const SqliteWorkSession &s, bool edit, const QByteArray &occurrence,
                   const QString &predicate, QList<QVariant> args = {}) {
    args.prepend(occurrence);
    const auto rows = s.execute({"SELECT 1 FROM " + occurrence_table(edit) +
                                 " g WHERE g.occurrence_ref=? AND " + predicate + " LIMIT 1",
                                 std::move(args)});
    return !rows.empty();
}

// Release advances one occurrence. Check the stored successor closure in
// that same turn, before changing the release bit or asking the owner to act.
inline void verify_release_selection(const SqliteWorkSession &s, bool edit,
                                      const QByteArray &occurrence) {
    const QString actions = edit ? "main.subathon_milestone_edit_actions"
                                 : "main.subathon_milestone_actions";
    const auto rows = s.execute(
        {"WITH versions AS (SELECT f.* FROM main.subathon_milestone_slot_versions f JOIN " +
             occurrence_table(edit) + " g ON g.occurrence_ref=? WHERE " +
             interruption_origin(edit) + ") SELECT 1 FROM versions v "
             "LEFT JOIN " + actions + " a ON (a.occurrence_ref,a.action_ordinal)="
             "(v.occurrence,v.ordinal) WHERE a.action_ordinal IS NULL OR v.kind<>a.kind+1 OR "
             "typeof(v.version)<>'blob' OR length(v.version) NOT BETWEEN 1 AND 4096 OR "
             "(v.predecessor IS NULL AND EXISTS(SELECT 1 FROM versions n WHERE "
             "n.ordinal=v.ordinal AND n.predecessor IS NULL AND n.version<>v.version)) LIMIT 1",
         {occurrence}});
    if (!rows.empty())
        corrupt();
}

inline QString pending_filter(Catalog current) {
    return current >= Catalog::decisions ? QString("g.released=0 AND NOT ") + skipped
                                         : QString("g.released=0");
}

} // namespace subathon::server::milestone_sql
