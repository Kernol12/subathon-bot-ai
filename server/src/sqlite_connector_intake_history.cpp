// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"

#include <QSet>
#include <QtEndian>
#include <limits>

namespace subathon::server::connector_intake {
namespace {

using Status = HistoryStatus;
using detail::bounded;
using detail::document_limit;
using detail::reference_limit;
using detail::scope_bindings;

bool can_advance(qint64 value) { return value > 0 && value < std::numeric_limits<qint64>::max(); }

QByteArray framed(const QByteArray &bytes) {
    const auto size = qToBigEndian(static_cast<quint32>(bytes.size()));
    return QByteArray(reinterpret_cast<const char *>(&size), sizeof(size)) + bytes;
}

QByteArray framed(const std::optional<QByteArray> &bytes) {
    return bytes ? QByteArray(1, '\1') + framed(*bytes) : QByteArray(1, '\0');
}

QByteArray integer_bytes(qint64 value) {
    const auto bits = qToBigEndian(static_cast<quint64>(value));
    return QByteArray(reinterpret_cast<const char *>(&bits), sizeof(bits));
}

QByteArray framed_scope(const Scope &scope) {
    return framed(scope.installation) + framed(scope.provider) + framed(scope.account) +
           framed(scope.stream);
}

QVariant sql_blob(const std::optional<QByteArray> &value) {
    return value ? QVariant{*value} : QVariant{};
}

std::optional<QByteArray> optional_blob(const QVariant &value) {
    return value.isNull() ? std::nullopt : std::optional{value.toByteArray()};
}

bool local_scope(const SqliteWorkSession &session, const Scope &scope) {
    return detail::valid_scope(scope) && detail::local_installation(session, scope.installation);
}

struct SourceHead {
    Fence fence;
    std::optional<QByteArray> cut, frontier;
    QByteArray cursor_contract{};
};

std::optional<SourceHead> select_source(const SqliteWorkSession &session, const Scope &scope) {
    const auto rows = session.execute(
        {"SELECT epoch,revision,policy_revision,generation,cut,frontier,document "
         "FROM main.subathon_intake_sources WHERE installation=? AND provider=? AND account=? "
         "AND stream=?",
         scope_bindings(scope)});
    if (rows.isEmpty())
        return std::nullopt;
    const auto &r = rows.front();
    return SourceHead{{r[0].toLongLong(), r[1].toLongLong(), r[2].toLongLong(), r[3].toByteArray()},
                      optional_blob(r[4]),
                      optional_blob(r[5])};
}

bool matches(const Fence &left, const Fence &right) {
    return left.epoch == right.epoch && left.revision == right.revision &&
           left.policy_revision == right.policy_revision && left.generation == right.generation;
}

QByteArray history_head(const Scope &scope, const SourceHead &head) {
    return framed_scope(scope) + integer_bytes(head.fence.epoch) +
           integer_bytes(head.fence.revision) + integer_bytes(head.fence.policy_revision) +
           framed(head.fence.generation) + framed(head.frontier);
}

bool valid_members(const SourceChange &change) {
    constexpr qsizetype policy_member_limit = 256;
    if (change.members.isEmpty() || change.members.size() > policy_member_limit)
        return false;

    QSet<QByteArray> lane_keys, groups, primaries;
    bool includes_source = false;
    for (const auto &member : change.members) {
        if (!detail::valid_scope(member.scope) ||
            member.scope.installation != change.scope.installation ||
            !bounded(member.award_class, reference_limit) ||
            !bounded(member.effect_scope, reference_limit) ||
            !bounded(member.overlap_set, reference_limit) ||
            !bounded(member.document, document_limit))
            return false;
        const auto lane =
            framed_scope(member.scope) + framed(member.award_class) + framed(member.effect_scope);
        const auto group = framed(member.overlap_set) + framed(member.effect_scope);
        if (lane_keys.contains(lane) || (member.primary && primaries.contains(group)))
            return false;
        lane_keys.insert(lane);
        groups.insert(group);
        if (member.primary)
            primaries.insert(group);
        includes_source |= member.scope == change.scope;
    }
    return includes_source && groups == primaries;
}

std::optional<QByteArray> operation_document(const OperationIntent &intent) {
    const auto short_field = [](const QByteArray &bytes) {
        return bounded(bytes, reference_limit);
    };
    const auto cursor = [](const std::optional<QByteArray> &bytes) {
        return !bytes || (!bytes->isNull() && bytes->size() <= reference_limit);
    };
    const auto &cut = intent.upper_cut;
    if (!short_field(intent.operation_ref) || !short_field(intent.generation) ||
        !short_field(intent.interval.membership_basis) ||
        !short_field(intent.interval.order_basis) || !short_field(intent.qualified_capability) ||
        !bounded(intent.retained_authority, document_limit) || !short_field(cut.format) ||
        !short_field(cut.position_namespace) || !short_field(cut.qualified_generation) ||
        !bounded(cut.proof, document_limit) || !cursor(cut.position) ||
        !cursor(intent.initial_continuation))
        return std::nullopt;
    if (intent.purpose == HistoryPurpose::import &&
        (!short_field(intent.local_date) || !short_field(intent.zone_id) ||
         !short_field(intent.zone_basis)))
        return std::nullopt;
    if (intent.purpose == HistoryPurpose::catch_up && (!intent.gap || !short_field(*intent.gap)))
        return std::nullopt;

    constexpr qsizetype length_bytes = 4;
    const auto field_size = [](const QByteArray &bytes) { return bytes.size() + length_bytes; };
    const auto optional_size = [&](const std::optional<QByteArray> &bytes) {
        return 1 + (bytes ? field_size(*bytes) : 0);
    };
    const auto cut_size = 5 + field_size(cut.format) + field_size(cut.position_namespace) +
                          optional_size(cut.position) + field_size(cut.qualified_generation) +
                          field_size(cut.proof);
    const auto scope_size = field_size(intent.scope.installation) +
                            field_size(intent.scope.provider) + field_size(intent.scope.account) +
                            field_size(intent.scope.stream);
    const auto local_size = intent.purpose == HistoryPurpose::import
                                ? field_size(intent.local_date) + field_size(intent.zone_id) +
                                      field_size(intent.zone_basis) + 8
                                : field_size(*intent.gap);
    const auto intent_size = 5 + scope_size + 18 + field_size(intent.interval.membership_basis) +
                             field_size(intent.interval.order_basis) +
                             field_size(intent.qualified_capability) +
                             field_size(intent.retained_authority) + local_size +
                             optional_size(intent.initial_continuation) + length_bytes + cut_size;
    if (cut_size > document_limit || intent_size > document_limit)
        return std::nullopt;

    QByteArray cut_document("CUT1");
    cut_document += static_cast<char>(intent.interval.end);
    cut_document += framed(cut.format) + framed(cut.position_namespace) + framed(cut.position) +
                    framed(cut.qualified_generation) + framed(cut.proof);
    QByteArray document("IOP1");
    document += static_cast<char>(intent.purpose);
    document += framed_scope(intent.scope) + integer_bytes(intent.epoch) +
                integer_bytes(intent.policy_revision);
    document += static_cast<char>(intent.interval.start);
    document += static_cast<char>(intent.interval.end);
    document += framed(intent.interval.membership_basis) + framed(intent.interval.order_basis) +
                framed(intent.qualified_capability) + framed(intent.retained_authority);
    if (intent.purpose == HistoryPurpose::import)
        document += framed(intent.local_date) + framed(intent.zone_id) + framed(intent.zone_basis) +
                    integer_bytes(intent.resolved_utc_start);
    else
        document += framed(*intent.gap);
    document += framed(intent.initial_continuation) + framed(cut_document);
    return document;
}

} // namespace

HistoryStatus stage_source_change(const SqliteWorkSession &session, const SourceChange &change) {
    if (!local_scope(session, change.scope))
        return Status::wrong_scope;
    if (!bounded(change.current_authority, document_limit) || change.boundary_proof.isEmpty())
        return Status::invalid_authority;
    if (!bounded(change.new_generation, reference_limit) ||
        !bounded(change.policy, document_limit) ||
        !bounded(change.authorized_cut, document_limit) ||
        !bounded(change.history_ref, reference_limit) ||
        !bounded(change.qualified_capability, reference_limit) ||
        !bounded(change.boundary_proof, document_limit) || !valid_members(change))
        return Status::intake_limit;
    if (change.purpose != SourcePurpose::baseline && change.purpose != SourcePurpose::adoption &&
        change.purpose != SourcePurpose::handoff)
        return Status::conflict;

    const auto prior = select_source(session, change.scope);
    if (!prior || !matches(prior->fence, change.expected))
        return Status::stale;
    if (!can_advance(prior->fence.epoch) || !can_advance(prior->fence.revision) ||
        !can_advance(prior->fence.policy_revision))
        return Status::intake_limit;

    const SourceHead next{{prior->fence.epoch + 1, prior->fence.revision + 1,
                           prior->fence.policy_revision + 1, change.new_generation},
                          change.authorized_cut,
                          std::nullopt};
    const auto document = QByteArray("SH1") + history_head(change.scope, *prior) +
                          history_head(change.scope, next) + framed(change.qualified_capability) +
                          framed(change.current_authority) + framed(change.boundary_proof);
    if (document.size() > document_limit)
        return Status::intake_limit;
    if (!session
             .execute({"SELECT 1 FROM main.subathon_intake_operations "
                       "WHERE installation=? AND operation_ref=?",
                       {change.scope.installation, change.history_ref}})
             .isEmpty() ||
        !session
             .execute({"SELECT 1 FROM main.subathon_intake_policies "
                       "WHERE installation=? AND revision=?",
                       {change.scope.installation, next.fence.policy_revision}})
             .isEmpty())
        return Status::conflict;

    (void)session.execute({"INSERT INTO main.subathon_intake_policies "
                           "VALUES(?,?,?)",
                           {change.scope.installation, next.fence.policy_revision, change.policy}});
    detail::require_changed_one(session);
    for (const auto &member : change.members) {
        (void)session.execute(
            {"INSERT INTO main.subathon_intake_members VALUES(?,?,?,?,?,?,?,?,?,?)",
             {change.scope.installation, next.fence.policy_revision, member.scope.provider,
              member.scope.account, member.scope.stream, member.award_class, member.effect_scope,
              member.overlap_set, member.primary ? QVariant{1} : QVariant{}, member.document}});
        detail::require_changed_one(session);
    }

    auto args = scope_bindings(change.scope);
    args.append({prior->fence.epoch, prior->fence.generation, prior->fence.policy_revision});
    (void)session.execute({"UPDATE main.subathon_intake_operations SET outcome=3 "
                           "WHERE installation=? AND provider=? AND account=? AND stream=? "
                           "AND epoch=? AND generation=? AND policy_revision=? AND outcome=0",
                           args});
    const auto retired = session.execute({"SELECT changes()", {}});
    if (retired.size() != 1 || retired.front().front().toLongLong() > 1)
        throw SqliteWorkFailure(WorkError::sql_failure);

    QList<QVariant> source{next.fence.epoch,           next.fence.revision,
                           next.fence.policy_revision, next.fence.generation,
                           change.authorized_cut,      change.qualified_capability};
    source.append(scope_bindings(change.scope));
    source.append({prior->fence.epoch, prior->fence.revision, prior->fence.policy_revision,
                   prior->fence.generation});
    (void)session.execute(
        {"UPDATE main.subathon_intake_sources SET epoch=?,revision=?,policy_revision=?,"
         "generation=?,baseline_state=1,cut=?,frontier=NULL,document=? "
         "WHERE installation=? AND provider=? AND account=? AND stream=? AND epoch=? "
         "AND revision=? AND policy_revision=? AND generation=?",
         source});
    detail::require_changed_one(session);
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_operations "
         "VALUES(?,?,?,?,?,?,?,NULL,?,?,1,?,NULL,1,0,1,?,?)",
         {change.scope.installation, change.history_ref, change.scope.provider,
          change.scope.account, change.scope.stream, next.fence.epoch, next.fence.policy_revision,
          static_cast<int>(change.purpose), next.fence.generation, document, sql_blob(prior->cut),
          change.authorized_cut}});
    detail::require_changed_one(session);
    return Status::staged;
}

HistoryStatus stage_operation(const SqliteWorkSession &session, const OperationIntent &intent) {
    if (!local_scope(session, intent.scope))
        return Status::wrong_scope;
    if (!bounded(intent.current_authority, document_limit))
        return Status::invalid_authority;
    if (intent.purpose != HistoryPurpose::catch_up && intent.purpose != HistoryPurpose::import)
        return Status::conflict;
    if (intent.interval.start != Endpoint::inclusive || intent.interval.end != Endpoint::exclusive)
        return Status::unproved_range;
    if (intent.purpose == HistoryPurpose::import && intent.gap)
        return Status::conflict;
    const auto document = operation_document(intent);
    if (!document)
        return Status::intake_limit;
    const auto source = select_source(session, intent.scope);
    if (!source || source->fence.epoch != intent.epoch ||
        source->fence.policy_revision != intent.policy_revision ||
        source->fence.generation != intent.generation)
        return Status::stale;

    const auto existing = session.execute(
        {"SELECT provider,account,stream,epoch,policy_revision,purpose,generation,intent,gap_ref "
         "FROM main.subathon_intake_operations WHERE installation=? AND operation_ref=?",
         {intent.scope.installation, intent.operation_ref}});
    if (!existing.isEmpty()) {
        const auto &row = existing.front();
        if (row[0].toByteArray() != intent.scope.provider ||
            row[1].toByteArray() != intent.scope.account ||
            row[2].toByteArray() != intent.scope.stream || row[3].toLongLong() != intent.epoch ||
            row[4].toLongLong() != intent.policy_revision ||
            row[5].toInt() != static_cast<int>(intent.purpose) ||
            row[7].toByteArray() != *document || optional_blob(row[8]) != intent.gap)
            return Status::conflict;
        return row[6].toByteArray() == intent.generation ? Status::exact_replay : Status::stale;
    }
    if (intent.upper_cut.qualified_generation != intent.generation)
        return Status::unproved_range;
    if (!session
             .execute({"SELECT 1 FROM main.subathon_intake_operations "
                       "WHERE installation=? AND provider=? AND account=? AND stream=? "
                       "AND outcome=0 AND purpose IN (0,1)",
                       scope_bindings(intent.scope)})
             .isEmpty())
        return Status::busy;
    if (intent.gap) {
        auto args = scope_bindings(intent.scope);
        args.append({intent.epoch, *intent.gap, intent.policy_revision});
        if (session
                .execute({"SELECT 1 FROM main.subathon_intake_gaps WHERE installation=? AND "
                          "provider=? AND account=? AND stream=? AND epoch=? AND gap_ref=? "
                          "AND policy_revision=? AND status IN (0,1)",
                          args})
                .isEmpty())
            return Status::stale;
    }
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_operations "
         "VALUES(?,?,?,?,?,?,?,?,?,?,1,?,?,1,0,0,NULL,NULL)",
         {intent.scope.installation, intent.operation_ref, intent.scope.provider,
          intent.scope.account, intent.scope.stream, intent.epoch, intent.policy_revision,
          sql_blob(intent.gap), static_cast<int>(intent.purpose), intent.generation, *document,
          sql_blob(intent.initial_continuation)}});
    detail::require_changed_one(session);
    return Status::staged;
}

HistoryStatus stage_gap(const SqliteWorkSession &session, const GapOpening &gap) {
    if (!local_scope(session, gap.scope))
        return Status::wrong_scope;
    if (!bounded(gap.gap_ref, reference_limit) || !bounded(gap.new_generation, reference_limit) ||
        !bounded(gap.frontier, reference_limit) || !bounded(gap.cursor_contract, document_limit) ||
        !bounded(gap.upper_cut, document_limit) || !bounded(gap.observation, document_limit) ||
        !bounded(gap.cut_request, document_limit))
        return Status::intake_limit;
    const auto prior = select_source(session, gap.scope);
    if (!prior || !matches(prior->fence, gap.expected) || prior->frontier != gap.frontier)
        return Status::stale;
    if (prior->cursor_contract != gap.cursor_contract)
        return Status::unproved_range;
    if (!can_advance(prior->fence.revision))
        return Status::intake_limit;

    auto gap_key = scope_bindings(gap.scope);
    gap_key.append({prior->fence.epoch, gap.gap_ref});
    if (!session
             .execute({"SELECT 1 FROM main.subathon_intake_gaps WHERE installation=? AND "
                       "provider=? AND account=? AND stream=? AND epoch=? AND gap_ref=?",
                       gap_key})
             .isEmpty())
        return Status::conflict;
    if (!session
             .execute({"SELECT 1 FROM main.subathon_intake_operations "
                       "WHERE installation=? AND provider=? AND account=? AND stream=? "
                       "AND outcome=0 AND purpose IN (0,1)",
                       scope_bindings(gap.scope)})
             .isEmpty())
        return Status::busy;

    auto args = gap_key;
    args.append({prior->fence.policy_revision, gap.frontier, gap.upper_cut, gap.observation});
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_gaps VALUES(?,?,?,?,?,?,1,?,0,?,?,?)", args});
    detail::require_changed_one(session);

    QList<QVariant> source{gap.new_generation};
    source.append(scope_bindings(gap.scope));
    source.append({prior->fence.epoch, prior->fence.revision, prior->fence.policy_revision,
                   prior->fence.generation, gap.frontier, gap.cursor_contract});
    (void)session.execute(
        {"UPDATE main.subathon_intake_sources SET generation=?,revision=revision+1 "
         "WHERE installation=? AND provider=? AND account=? AND stream=? AND epoch=? "
         "AND revision=? AND policy_revision=? AND generation=? AND frontier=? AND document=?",
         source});
    detail::require_changed_one(session);
    return Status::staged;
}

} // namespace subathon::server::connector_intake
