// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"

#include <QSet>

namespace subathon::server::connector_intake {
namespace {

constexpr qsizetype quarantine_framing = 9; // One tag and two BE32 lengths.
constexpr qint64 stream_entries = 256;
constexpr qint64 installation_entries = 1024;
constexpr qint64 stream_bytes = 8 * 1024 * 1024;
constexpr qint64 installation_bytes = 16 * 1024 * 1024;

using Status = QuarantineStatus;

struct SelectedQuarantine {
    qint64 input_bytes, provenance_bytes, arrival_ms;
    qint64 phase, disposition;
    std::optional<qint64> charge;
    qint64 epoch, policy;
    QByteArray generation;
    std::optional<PageSlot> history;
};

std::optional<SelectedQuarantine> select_metadata(const SqliteWorkSession &session,
                                                  const QuarantineEntry &entry) {
    auto args = detail::scope_bindings(entry.scope);
    args.append(entry.key);
    const auto rows = session.execute(
        {"SELECT length(input),length(provenance),arrival_ms,phase,disposition,quarantine_bytes,"
         "epoch,policy_revision,generation,operation_ref,page_ordinal,item_ordinal "
         "FROM main.subathon_intake_entries WHERE installation=? "
         "AND provider=? AND account=? AND stream=? AND intake_ref=? "
         "AND receipt_id IS NULL AND attempt_ordinal IS NULL AND contribution_id IS NULL "
         "AND activity_namespace IS NULL AND activity_id IS NULL AND verification_ref IS NULL",
         args});
    if (rows.isEmpty())
        return {};
    const auto &r = rows.front();
    return SelectedQuarantine{
        r[0].toLongLong(),
        r[1].toLongLong(),
        r[2].toLongLong(),
        r[3].toLongLong(),
        r[4].toLongLong(),
        r[5].isNull() ? std::nullopt : std::optional{r[5].toLongLong()},
        r[6].toLongLong(),
        r[7].toLongLong(),
        r[8].toByteArray(),
        r[9].isNull()
            ? std::nullopt
            : std::optional{PageSlot{r[9].toByteArray(), r[10].toLongLong(), r[11].toLongLong()}}};
}

bool same_input(const SqliteWorkSession &session, const QuarantineEntry &entry,
                const SelectedQuarantine &selected) {
    const auto slot = entry.history ? std::optional{entry.history->slot} : std::nullopt;
    if (selected.input_bytes != entry.normalized.size() ||
        selected.provenance_bytes != entry.provenance.size() ||
        selected.arrival_ms != entry.arrival_ms || selected.history != slot)
        return false;
    auto args = detail::scope_bindings(entry.scope);
    args.append({entry.key, entry.normalized, entry.provenance});
    return !session
                .execute({"SELECT 1 FROM main.subathon_intake_entries WHERE installation=? AND "
                          "provider=? AND account=? AND stream=? AND intake_ref=? AND input=? "
                          "AND provenance=?",
                          args})
                .isEmpty();
}

bool occupied(const SqliteWorkSession &session, const QuarantineEntry &entry) {
    auto args = detail::scope_bindings(entry.scope);
    args.append(entry.key);
    return !session
                .execute({"SELECT 1 FROM main.subathon_intake_entries WHERE installation=? "
                          "AND provider=? AND account=? AND stream=? AND intake_ref=?",
                          args})
                .isEmpty();
}

bool quota_available(const SqliteWorkSession &session, const Scope &scope, qint64 count,
                     qint64 bytes) {
    if (count == 0)
        return true;
    // Check the full additional cohort without fetching item documents. Each
    // individual stage repeats both checks under the same writer owner.
    const auto stream = session.execute(
        {"SELECT count(*),coalesce(sum(quarantine_bytes),0) FROM "
         "(SELECT quarantine_bytes FROM main.subathon_intake_entries "
         "INDEXED BY subathon_intake_quarantine WHERE installation=? AND provider=? "
         "AND account=? AND stream=? AND quarantine_bytes IS NOT NULL LIMIT 257)",
         detail::scope_bindings(scope)});
    if (stream.front()[0].toLongLong() > stream_entries - count ||
        stream.front()[1].toLongLong() > stream_bytes - bytes)
        return false;
    const auto installation =
        session.execute({"SELECT count(*),coalesce(sum(quarantine_bytes),0) FROM "
                         "(SELECT quarantine_bytes FROM main.subathon_intake_entries "
                         "INDEXED BY subathon_intake_quarantine WHERE installation=? "
                         "AND quarantine_bytes IS NOT NULL LIMIT 1025)",
                         {scope.installation}});
    return installation.front()[0].toLongLong() <= installation_entries - count &&
           installation.front()[1].toLongLong() <= installation_bytes - bytes;
}

std::optional<qint64> charged_size(const QuarantineEntry &entry) {
    if (!detail::bounded(entry.normalized, detail::item_limit) ||
        !detail::bounded(entry.provenance, detail::document_limit) ||
        entry.normalized.size() > detail::item_limit - quarantine_framing - entry.provenance.size())
        return {};
    return entry.normalized.size() + quarantine_framing + entry.provenance.size();
}

bool authorized_scope(const SqliteWorkSession &session, const Scope &scope) {
    return detail::valid_scope(scope) && detail::local_installation(session, scope.installation);
}

Status source_status(const SqliteWorkSession &session, const QuarantineEntry &entry) {
    const auto rows = session.execute(
        {"SELECT epoch,revision,policy_revision,generation FROM main.subathon_intake_sources "
         "WHERE installation=? AND provider=? AND account=? AND stream=?",
         detail::scope_bindings(entry.scope)});
    if (rows.isEmpty())
        return Status::wrong_scope;
    const auto &r = rows.front();
    return Fence{r[0].toLongLong(), r[1].toLongLong(), r[2].toLongLong(), r[3].toByteArray()} ==
                   entry.expected_source
               ? Status::staged
               : Status::stale;
}

struct Admission {
    Status status;
    qint64 charge{};
};

Admission prepare(const SqliteWorkSession &session, const QuarantineEntry &entry,
                  bool planning = false) {
    if (!authorized_scope(session, entry.scope))
        return {Status::wrong_scope};
    if (!detail::bounded(entry.key, detail::reference_limit))
        return {Status::intake_limit};
    const auto charge = charged_size(entry);
    if (!charge)
        return {Status::intake_limit};
    const auto scope = source_status(session, entry);
    if (scope != Status::staged)
        return {scope};

    if (const auto selected = select_metadata(session, entry)) {
        const bool unresolved =
            selected->phase == 2 && selected->disposition == 1 && selected->charge == charge;
        const bool resolved =
            selected->phase == 3 && selected->disposition == 2 && !selected->charge;
        if (!(unresolved || resolved) || !same_input(session, entry, *selected))
            return {Status::conflict};
        if (entry.history) {
            if (!detail::qualified_quarantine_slot(session, entry, true))
                return {Status::stale};
        } else if (selected->epoch != entry.expected_source.epoch ||
                   selected->policy != entry.expected_source.policy_revision ||
                   selected->generation != entry.expected_source.generation) {
            return {Status::stale};
        }
        return {Status::exact_replay};
    }
    if (occupied(session, entry))
        return {Status::conflict};
    if (!planning && entry.history && !detail::qualified_quarantine_slot(session, entry, false))
        return {Status::stale};
    return {Status::staged, *charge};
}

} // namespace

QuarantineStatus preflight_quarantine(const SqliteWorkSession &session,
                                      const QuarantineEntry &entry) {
    const auto selected = prepare(session, entry);
    if (selected.status == Status::staged &&
        !quota_available(session, entry.scope, 1, selected.charge))
        return Status::quarantine_full;
    return selected.status;
}

QuarantineStatus detail::preflight_page_quarantines(const SqliteWorkSession &session,
                                                    const PageAdmission &request) {
    const auto &page = request.binding;
    if (request.quarantine_preflight.size() > page.items)
        return Status::intake_limit;
    QSet<QByteArray> keys;
    QSet<qint64> ordinals;
    qint64 count = 0, bytes = 0;
    for (const auto &entry : request.quarantine_preflight) {
        if (entry.scope != page.scope || entry.expected_source != page.source || !entry.history)
            return Status::conflict;
        const auto &historical = *entry.history;
        const auto &slot = historical.slot;
        if (slot.operation != page.operation_ref || slot.page != page.ordinal || slot.item < 1 ||
            slot.item > page.items || historical.operation_revision != page.operation_revision ||
            historical.gap_revision != page.gap_revision ||
            historical.expected_continuation != page.expected_continuation ||
            historical.expected_continuation.isNull() != page.expected_continuation.isNull() ||
            historical.page != page.page || historical.page_reconnect != page.page_reconnect ||
            historical.page_reconnect.isNull() != page.page_reconnect.isNull() ||
            entry.normalized != request.normalized_items[slot.item - 1] ||
            keys.contains(entry.key) || ordinals.contains(slot.item))
            return Status::conflict;
        keys.insert(entry.key);
        ordinals.insert(slot.item);
        const auto selected = prepare(session, entry, true);
        if (selected.status == Status::exact_replay)
            continue;
        if (selected.status != Status::staged)
            return selected.status;
        ++count;
        bytes += selected.charge;
    }
    return quota_available(session, page.scope, count, bytes) ? Status::staged
                                                              : Status::quarantine_full;
}

QuarantineStatus stage_quarantine(const SqliteWorkSession &session, const QuarantineEntry &entry) {
    const auto selected = prepare(session, entry);
    if (selected.status != Status::staged)
        return selected.status;
    if (!quota_available(session, entry.scope, 1, selected.charge))
        return Status::quarantine_full;
    auto args = detail::scope_bindings(entry.scope);
    const auto &fence = entry.expected_source;
    args.append({entry.key, fence.epoch, fence.generation, fence.policy_revision});
    const auto slot = entry.history ? std::optional{entry.history->slot} : std::nullopt;
    args.append({slot ? QVariant{slot->operation} : QVariant{},
                 slot ? QVariant{slot->page} : QVariant{}, slot ? QVariant{slot->item} : QVariant{},
                 entry.normalized, entry.provenance, entry.arrival_ms, selected.charge});
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_entries("
         "installation,provider,account,stream,intake_ref,epoch,generation,policy_revision,"
         "operation_ref,page_ordinal,item_ordinal,input,provenance,arrival_ms,phase,disposition,"
         "quarantine_bytes) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,2,1,?)",
         args});
    detail::require_changed_one(session);
    if (slot)
        detail::advance_page_prefix(session, entry.scope, *slot);
    return Status::staged;
}

QuarantineStatus stage_quarantine_resolution(const SqliteWorkSession &session,
                                             const QuarantineEntry &entry,
                                             const QByteArray &current_authority) {
    if (!authorized_scope(session, entry.scope))
        return Status::wrong_scope;
    if (!detail::bounded(current_authority, detail::document_limit))
        return Status::invalid_authority;
    if (!detail::bounded(entry.key, detail::reference_limit))
        return Status::intake_limit;
    const auto charge = charged_size(entry);
    if (!charge)
        return Status::intake_limit;
    const auto scope = source_status(session, entry);
    if (scope != Status::staged)
        return scope;

    const auto selected = select_metadata(session, entry);
    if (!selected)
        return Status::stale;
    if (!same_input(session, entry, *selected))
        return Status::conflict;
    if (selected->phase == 3 && selected->disposition == 2 && !selected->charge)
        return Status::exact_replay;
    if (selected->phase != 2 || selected->disposition != 1 || selected->charge != charge)
        return Status::conflict;

    auto args = detail::scope_bindings(entry.scope);
    args.append({entry.key, *charge});
    (void)session.execute(
        {"UPDATE main.subathon_intake_entries SET phase=3,disposition=2,quarantine_bytes=NULL "
         "WHERE installation=? AND provider=? AND account=? AND stream=? AND intake_ref=? "
         "AND phase=2 AND disposition=1 AND quarantine_bytes=?",
         args});
    detail::require_changed_one(session);
    return Status::staged;
}

} // namespace subathon::server::connector_intake
