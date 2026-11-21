// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"

#include <QHash>
#include <QMap>
#include <QSet>
#include <QtEndian>
#include <algorithm>
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
                      optional_blob(r[5]),
                      r[6].toByteArray()};
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

constexpr qsizetype policy_member_limit = 256;

QByteArray member_key(const PolicyMember &member) {
    return framed_scope(member.scope) + framed(member.award_class) + framed(member.effect_scope);
}

QByteArray overlap_key(const PolicyMember &member) {
    return framed(member.overlap_set) + framed(member.effect_scope);
}

bool valid_members(const SourceChange &change) {
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
        const auto lane = member_key(member);
        const auto group = overlap_key(member);
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

// The limits apply to selected cells before SQLite returns the BLOB bodies.
QString membership_columns() {
    QStringList fields;
    for (const auto *name : {"provider", "account", "stream", "award_class", "effect_scope",
                             "overlap_set", "document"}) {
        const QString column = QString("m.") + name;
        fields.append("CASE WHEN typeof(" + column + ")='blob' AND length(" + column +
                      ") BETWEEN 1 AND " +
                      QString::number(column == "m.document" ? document_limit : reference_limit) +
                      " THEN " + column + " END");
    }
    fields.append("m.primary_slot");
    return fields.join(',');
}

std::optional<QList<PolicyMember>> decode_members(const QByteArray &installation,
                                                  const SqlRows &rows) {
    if (rows.size() > policy_member_limit)
        return {};
    QList<PolicyMember> result;
    for (const auto &row : rows) {
        if (std::any_of(row.cbegin(), row.cbegin() + 7,
                        [](const QVariant &cell) { return cell.isNull(); }) ||
            (!row[7].isNull() && row[7].toLongLong() != 1))
            return {};
        result.append(
            {{installation, row[0].toByteArray(), row[1].toByteArray(), row[2].toByteArray()},
             row[3].toByteArray(),
             row[4].toByteArray(),
             row[5].toByteArray(),
             !row[7].isNull(),
             row[6].toByteArray()});
    }
    return result;
}

std::optional<QSet<QByteArray>> affected_sources(const SqliteWorkSession &session,
                                                 const SourceChange &change, qint64 policy_head) {
    const auto old = decode_members(
        change.scope.installation,
        session.execute({"SELECT " + membership_columns() +
                             " FROM main.subathon_intake_members m WHERE m.installation=? "
                             "AND m.policy_revision=? LIMIT 257",
                         {change.scope.installation, policy_head}}));
    // Active sources can still use a revision older than the installation head.
    const auto active = decode_members(
        change.scope.installation,
        session.execute({"SELECT " + membership_columns() +
                             " FROM main.subathon_intake_sources s "
                             "JOIN main.subathon_intake_members m ON m.installation=s.installation "
                             "AND m.policy_revision=s.policy_revision AND m.provider=s.provider "
                             "AND m.account=s.account AND m.stream=s.stream "
                             "WHERE s.installation=? LIMIT 257",
                         {change.scope.installation}}));
    if (!old || !active)
        return {};

    QMap<QByteArray, PolicyMember> old_members, next_members;
    QHash<QByteArray, QSet<QByteArray>> scopes_in_group, groups_in_scope;
    const auto index = [&](const PolicyMember &member) {
        const auto scope = framed_scope(member.scope);
        const auto group = overlap_key(member);
        scopes_in_group[group].insert(scope);
        groups_in_scope[scope].insert(group);
    };
    for (const auto &member : *old) {
        old_members.insert(member_key(member), member);
        index(member);
    }
    for (const auto &member : change.members) {
        next_members.insert(member_key(member), member);
        index(member);
    }
    for (const auto &member : *active)
        index(member);

    QSet<QByteArray> affected;
    QList<QByteArray> pending;
    const auto require_scope = [&](const QByteArray &scope) {
        if (!affected.contains(scope)) {
            affected.insert(scope);
            pending.append(scope);
        }
    };
    require_scope(framed_scope(change.scope));
    for (auto i = old_members.cbegin(); i != old_members.cend(); ++i) {
        const auto next = next_members.constFind(i.key());
        if (next == next_members.cend() || *next != *i)
            require_scope(framed_scope(i->scope));
    }
    for (auto i = next_members.cbegin(); i != next_members.cend(); ++i) {
        if (!old_members.contains(i.key()))
            require_scope(framed_scope(i->scope));
    }

    // Moving one source changes its whole fence. Close over every overlap it
    // connects to, including the roles still on retained older revisions.
    QSet<QByteArray> visited_groups;
    for (qsizetype i = 0; i < pending.size(); ++i) {
        for (const auto &group : groups_in_scope.value(pending[i])) {
            if (visited_groups.contains(group))
                continue;
            visited_groups.insert(group);
            for (const auto &scope : scopes_in_group.value(group))
                require_scope(scope);
        }
    }
    return affected;
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
    if (!bounded(change.policy, document_limit) || !valid_members(change) ||
        change.affected_sources.size() >= policy_member_limit)
        return Status::intake_limit;

    const auto head =
        session.execute({"SELECT revision FROM main.subathon_intake_policies WHERE installation=? "
                         "ORDER BY revision DESC LIMIT 1",
                         {change.scope.installation}});
    if (head.size() != 1 ||
        head.front().front().toLongLong() != change.expected_installation_policy_revision)
        return Status::stale;
    const auto policy_head = head.front().front().toLongLong();
    if (!can_advance(policy_head))
        return Status::intake_limit;
    const auto next_policy = policy_head + 1;

    QList<SourceTransition> transitions{{change.scope, change.expected, change.new_generation,
                                         change.authorized_cut, change.history_ref,
                                         change.current_authority, change.qualified_capability,
                                         change.boundary_proof, change.purpose}};
    transitions.append(change.affected_sources);
    QSet<QByteArray> supplied;
    QSet<QByteArray> history_refs;
    for (const auto &transition : transitions) {
        if (!detail::valid_scope(transition.scope) ||
            transition.scope.installation != change.scope.installation)
            return Status::wrong_scope;
        if (transition.current_authority.isEmpty() || transition.boundary_proof.isEmpty())
            return Status::invalid_authority;
        if (!bounded(transition.new_generation, reference_limit) ||
            !bounded(transition.authorized_cut, document_limit) ||
            !bounded(transition.history_ref, reference_limit) ||
            !bounded(transition.qualified_capability, reference_limit) ||
            !bounded(transition.current_authority, document_limit) ||
            !bounded(transition.boundary_proof, document_limit))
            return Status::intake_limit;
        if (transition.purpose != SourcePurpose::baseline &&
            transition.purpose != SourcePurpose::adoption &&
            transition.purpose != SourcePurpose::handoff)
            return Status::conflict;
        const auto scope = framed_scope(transition.scope);
        if (supplied.contains(scope) || history_refs.contains(transition.history_ref))
            return Status::conflict;
        supplied.insert(scope);
        history_refs.insert(transition.history_ref);
    }

    const auto affected = affected_sources(session, change, policy_head);
    if (!affected || affected->size() > policy_member_limit)
        return Status::intake_limit;
    if (*affected != supplied)
        return Status::conflict;

    struct Plan {
        SourceTransition request;
        SourceHead old, next;
        QByteArray document;
    };
    QList<Plan> plans;
    for (const auto &transition : transitions) {
        const auto prior = select_source(session, transition.scope);
        if (!prior || !matches(prior->fence, transition.expected))
            return Status::stale;
        if (!can_advance(prior->fence.epoch) || !can_advance(prior->fence.revision))
            return Status::intake_limit;
        const auto active = session.execute(
            {"SELECT epoch,generation,policy_revision FROM main.subathon_intake_operations "
             "WHERE installation=? AND provider=? AND account=? AND stream=? "
             "AND outcome=0 AND purpose IN (0,1) LIMIT 2",
             scope_bindings(transition.scope)});
        if (active.size() > 1 ||
            (!active.isEmpty() && (active.front()[0].toLongLong() != prior->fence.epoch ||
                                   active.front()[1].toByteArray() != prior->fence.generation ||
                                   active.front()[2].toLongLong() != prior->fence.policy_revision)))
            return Status::stale;
        const SourceHead next{{prior->fence.epoch + 1, prior->fence.revision + 1, next_policy,
                               transition.new_generation},
                              transition.authorized_cut,
                              std::nullopt};
        const auto document =
            QByteArray("SH1") + history_head(transition.scope, *prior) +
            history_head(transition.scope, next) + framed(transition.qualified_capability) +
            framed(transition.current_authority) + framed(transition.boundary_proof);
        if (document.size() > document_limit)
            return Status::intake_limit;
        if (!session
                 .execute({"SELECT 1 FROM main.subathon_intake_operations "
                           "WHERE installation=? AND operation_ref=?",
                           {transition.scope.installation, transition.history_ref}})
                 .isEmpty())
            return Status::conflict;
        plans.append({transition, *prior, next, document});
    }

    (void)session.execute({"INSERT INTO main.subathon_intake_policies VALUES(?,?,?)",
                           {change.scope.installation, next_policy, change.policy}});
    detail::require_changed_one(session);
    for (const auto &member : change.members) {
        (void)session.execute(
            {"INSERT INTO main.subathon_intake_members VALUES(?,?,?,?,?,?,?,?,?,?)",
             {change.scope.installation, next_policy, member.scope.provider, member.scope.account,
              member.scope.stream, member.award_class, member.effect_scope, member.overlap_set,
              member.primary ? QVariant{1} : QVariant{}, member.document}});
        detail::require_changed_one(session);
    }

    for (const auto &plan : plans) {
        const auto &request = plan.request;
        const auto &old = plan.old.fence;
        const auto &next = plan.next.fence;
        auto args = scope_bindings(request.scope);
        args.append({old.epoch, old.generation, old.policy_revision});
        (void)session.execute({"UPDATE main.subathon_intake_operations SET outcome=3 "
                               "WHERE installation=? AND provider=? AND account=? AND stream=? "
                               "AND epoch=? AND generation=? AND policy_revision=? AND outcome=0",
                               args});
        const auto retired = session.execute({"SELECT changes()", {}});
        if (retired.size() != 1 || retired.front().front().toLongLong() > 1)
            throw SqliteWorkFailure(WorkError::sql_failure);

        QList<QVariant> source{
            next.epoch,      next.revision,          next.policy_revision,
            next.generation, request.authorized_cut, request.qualified_capability};
        source.append(scope_bindings(request.scope));
        source.append({old.epoch, old.revision, old.policy_revision, old.generation});
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
             {request.scope.installation, request.history_ref, request.scope.provider,
              request.scope.account, request.scope.stream, next.epoch, next.policy_revision,
              static_cast<int>(request.purpose), next.generation, plan.document,
              sql_blob(plan.old.cut), request.authorized_cut}});
        detail::require_changed_one(session);
    }
    return Status::staged;
}

HistoryStatus stage_operation(const SqliteWorkSession &session, const OperationIntent &intent) {
    if (!local_scope(session, intent.scope))
        return Status::wrong_scope;
    if (!bounded(intent.current_authority, document_limit))
        return Status::invalid_authority;
    if (intent.purpose != HistoryPurpose::catch_up && intent.purpose != HistoryPurpose::import)
        return Status::conflict;
    const bool importing = intent.purpose == HistoryPurpose::import;
    if (intent.interval.start != (importing ? Endpoint::inclusive : Endpoint::exclusive) ||
        intent.interval.end != (importing ? Endpoint::exclusive : Endpoint::inclusive))
        return Status::unproved_range;
    if (intent.purpose == HistoryPurpose::import && intent.gap)
        return Status::conflict;
    const auto document = operation_document(intent);
    if (!document)
        return Status::intake_limit;
    const auto source = select_source(session, intent.scope);
    if (!source || source->fence != Fence{intent.epoch, intent.source_revision,
                                          intent.policy_revision, intent.generation})
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

HistoryStatus stage_reconnect(const SqliteWorkSession &session, const Reconnect &request) {
    if (!local_scope(session, request.scope))
        return Status::wrong_scope;
    if (request.current_authority.isEmpty() || request.observation.isEmpty())
        return Status::invalid_authority;
    if (!bounded(request.new_generation, reference_limit) ||
        !bounded(request.history_ref, reference_limit) ||
        !bounded(request.cursor_contract, document_limit) ||
        !bounded(request.current_authority, document_limit) ||
        !bounded(request.observation, document_limit))
        return Status::intake_limit;

    const auto prior = select_source(session, request.scope);
    if (!prior || !matches(prior->fence, request.expected))
        return Status::stale;
    if (prior->cursor_contract != request.cursor_contract || !prior->cut ||
        request.new_generation == prior->fence.generation)
        return Status::unproved_range;
    if (!can_advance(prior->fence.revision))
        return Status::intake_limit;
    if (!session
             .execute({"SELECT 1 FROM main.subathon_intake_operations "
                       "WHERE installation=? AND operation_ref=?",
                       {request.scope.installation, request.history_ref}})
             .isEmpty())
        return Status::conflict;

    const auto active = session.execute(
        {"SELECT o.operation_ref,o.epoch,o.policy_revision,o.generation,o.revision,"
         "EXISTS(SELECT 1 FROM main.subathon_intake_pages p WHERE p.installation=o.installation "
         "AND p.operation_ref=o.operation_ref AND p.outcome=0) "
         "FROM main.subathon_intake_operations o WHERE o.installation=? AND o.provider=? "
         "AND o.account=? AND o.stream=? AND o.outcome=0 AND o.purpose IN (0,1)",
         scope_bindings(request.scope)});
    if (!active.isEmpty()) {
        const auto &row = active.front();
        if (row[1].toLongLong() != prior->fence.epoch ||
            row[2].toLongLong() != prior->fence.policy_revision ||
            row[3].toByteArray() != prior->fence.generation)
            return Status::stale;
        if (!can_advance(row[4].toLongLong()))
            return Status::intake_limit;
    }

    auto next = *prior;
    ++next.fence.revision;
    next.fence.generation = request.new_generation;
    auto document = QByteArray("RC1") + history_head(request.scope, *prior) +
                    history_head(request.scope, next) + framed(request.cursor_contract) +
                    framed(request.current_authority) + framed(request.observation);
    if (document.size() > document_limit)
        return Status::intake_limit;
    if (!active.isEmpty() && active.front()[5].toBool()) {
        const auto witness = detail::retain_page_reconnect(
            session, request, active.front()[0].toByteArray(), document);
        if (!witness)
            return Status::incomplete_prefix;
        if (witness->size() > document_limit)
            return Status::intake_limit;
        document = *witness;
    }
    auto args = scope_bindings(request.scope);
    args.append({prior->fence.epoch, prior->fence.generation, prior->fence.revision,
                 prior->fence.policy_revision});
    args.prepend(request.new_generation);
    (void)session.execute(
        {"UPDATE main.subathon_intake_sources SET generation=?,revision=revision+1 "
         "WHERE installation=? AND provider=? AND account=? AND stream=? "
         "AND epoch=? AND generation=? AND revision=? AND policy_revision=?",
         args});
    detail::require_changed_one(session);
    if (!active.isEmpty()) {
        const auto &row = active.front();
        (void)session.execute(
            {"UPDATE main.subathon_intake_operations SET generation=?,revision=revision+1 "
             "WHERE installation=? AND operation_ref=? AND generation=? "
             "AND revision=? AND outcome=0",
             {request.new_generation, request.scope.installation, row[0], prior->fence.generation,
              row[4]}});
        detail::require_changed_one(session);
    }
    (void)session.execute({"INSERT INTO main.subathon_intake_operations "
                           "VALUES(?,?,?,?,?,?,?,NULL,4,?,1,?,NULL,1,0,1,?,?)",
                           {request.scope.installation, request.history_ref, request.scope.provider,
                            request.scope.account, request.scope.stream, prior->fence.epoch,
                            prior->fence.policy_revision, request.new_generation, document,
                            *prior->cut, *prior->cut}});
    detail::require_changed_one(session);
    return Status::staged;
}

} // namespace subathon::server::connector_intake
