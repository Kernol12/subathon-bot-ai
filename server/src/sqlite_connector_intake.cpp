// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"

namespace subathon::server::connector_intake {
namespace {

constexpr qsizetype quarantine_framing = 9; // One tag and two BE32 lengths.
constexpr qint64 stream_entries = 256;
constexpr qint64 installation_entries = 1024;
constexpr qint64 stream_bytes = 8 * 1024 * 1024;
constexpr qint64 installation_bytes = 16 * 1024 * 1024;

using Status = QuarantineStatus;

struct SelectedQuarantine {
    qint64 input_bytes, provenance_bytes;
    qint64 phase, disposition;
    std::optional<qint64> charge;
};

std::optional<SelectedQuarantine> select_metadata(const SqliteWorkSession &session,
                                                  const QuarantineEntry &entry) {
    auto args = detail::scope_bindings(entry.scope);
    args.append(entry.key);
    const auto rows =
        session.execute({"SELECT length(input),length(provenance),phase,disposition,"
                         "quarantine_bytes FROM main.subathon_intake_entries WHERE installation=? "
                         "AND provider=? AND account=? AND stream=? AND intake_ref=?",
                         args});
    if (rows.isEmpty())
        return std::nullopt;
    const auto &row = rows.front();
    return SelectedQuarantine{row[0].toLongLong(), row[1].toLongLong(), row[2].toLongLong(),
                              row[3].toLongLong(),
                              row[4].isNull() ? std::nullopt : std::optional{row[4].toLongLong()}};
}

bool same_input(const SqliteWorkSession &session, const QuarantineEntry &entry,
                const SelectedQuarantine &selected) {
    if (selected.input_bytes != entry.normalized.size() ||
        selected.provenance_bytes != entry.provenance.size())
        return false;
    auto args = detail::scope_bindings(entry.scope);
    args.append(entry.key);
    args.append(entry.normalized);
    args.append(entry.provenance);
    return !session
                .execute({"SELECT 1 FROM main.subathon_intake_entries WHERE installation=? AND "
                          "provider=? AND account=? AND stream=? AND intake_ref=? AND input=? "
                          "AND provenance=?",
                          args})
                .isEmpty();
}

bool quota_available(const SqliteWorkSession &session, const Scope &scope, qint64 charge) {
    // The partial index starts with the installation and the exact source scope.
    // LIMIT applies before aggregation, including on an already overfull store.
    const auto stream = session.execute(
        {"SELECT count(*),coalesce(sum(quarantine_bytes),0) FROM "
         "(SELECT quarantine_bytes FROM main.subathon_intake_entries "
         "INDEXED BY subathon_intake_quarantine WHERE installation=? AND provider=? "
         "AND account=? AND stream=? AND quarantine_bytes IS NOT NULL LIMIT 257)",
         detail::scope_bindings(scope)});
    if (stream.front()[0].toLongLong() >= stream_entries ||
        stream.front()[1].toLongLong() > stream_bytes - charge)
        return false;
    const auto installation =
        session.execute({"SELECT count(*),coalesce(sum(quarantine_bytes),0) FROM "
                         "(SELECT quarantine_bytes FROM main.subathon_intake_entries "
                         "INDEXED BY subathon_intake_quarantine WHERE installation=? "
                         "AND quarantine_bytes IS NOT NULL LIMIT 1025)",
                         {scope.installation}});
    return installation.front()[0].toLongLong() < installation_entries &&
           installation.front()[1].toLongLong() <= installation_bytes - charge;
}

std::optional<qint64> charged_size(const QuarantineEntry &entry) {
    if (!detail::bounded(entry.normalized, detail::item_limit) ||
        !detail::bounded(entry.provenance, detail::document_limit) ||
        entry.normalized.size() > detail::item_limit - quarantine_framing - entry.provenance.size())
        return std::nullopt;
    return entry.normalized.size() + quarantine_framing + entry.provenance.size();
}

struct Admission {
    Status status;
    qint64 charge{};
    qint64 epoch{};
    QByteArray generation{};
    qint64 policy{};
};

Admission prepare(const SqliteWorkSession &session, const QuarantineEntry &entry) {
    if (!detail::valid_scope(entry.scope) ||
        !detail::local_installation(session, entry.scope.installation))
        return {Status::wrong_scope};
    if (!detail::bounded(entry.key, detail::reference_limit))
        return {Status::intake_limit};
    const auto charge = charged_size(entry);
    if (!charge)
        return {Status::intake_limit};
    const auto source = session.execute(
        {"SELECT epoch,generation,policy_revision FROM main.subathon_intake_sources "
         "WHERE installation=? AND provider=? AND account=? AND stream=?",
         detail::scope_bindings(entry.scope)});
    if (source.isEmpty())
        return {Status::wrong_scope};

    if (const auto selected = select_metadata(session, entry)) {
        const bool unresolved =
            selected->phase == 2 && selected->disposition == 1 && selected->charge == charge;
        const bool resolved =
            selected->phase == 3 && selected->disposition == 2 && !selected->charge;
        if ((unresolved || resolved) && same_input(session, entry, *selected))
            return {Status::exact_replay};
        return {Status::conflict};
    }
    if (!quota_available(session, entry.scope, *charge))
        return {Status::quarantine_full};
    return {Status::staged, *charge, source.front()[0].toLongLong(),
            source.front()[1].toByteArray(), source.front()[2].toLongLong()};
}

} // namespace

QuarantineStatus preflight_quarantine(const SqliteWorkSession &session,
                                      const QuarantineEntry &entry) {
    return prepare(session, entry).status;
}

QuarantineStatus stage_quarantine(const SqliteWorkSession &session, const QuarantineEntry &entry) {
    const auto selected = prepare(session, entry);
    if (selected.status != Status::staged)
        return selected.status;
    auto args = detail::scope_bindings(entry.scope);
    args.append({entry.key, selected.epoch, selected.generation, selected.policy, entry.normalized,
                 entry.provenance, entry.arrival_ms, selected.charge});
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_entries("
         "installation,provider,account,stream,intake_ref,epoch,generation,policy_revision,"
         "input,provenance,arrival_ms,phase,disposition,quarantine_bytes) "
         "VALUES(?,?,?,?,?,?,?,?,?,?,?,2,1,?)",
         args});
    detail::require_changed_one(session);
    return Status::staged;
}

QuarantineStatus stage_quarantine_resolution(const SqliteWorkSession &session,
                                             const QuarantineEntry &entry,
                                             const QByteArray &current_authority) {
    if (!detail::valid_scope(entry.scope) ||
        !detail::local_installation(session, entry.scope.installation))
        return Status::wrong_scope;
    if (!detail::bounded(current_authority, detail::document_limit))
        return Status::invalid_authority;
    if (!detail::bounded(entry.key, detail::reference_limit))
        return Status::intake_limit;
    const auto charge = charged_size(entry);
    if (!charge)
        return Status::intake_limit;

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
