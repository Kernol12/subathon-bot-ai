// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"

#include <QtEndian>
#include <limits>

namespace subathon::server::connector_intake {
namespace {

using Status = HistoryStatus;
using detail::bounded;
using detail::document_limit;
using detail::item_limit;
using detail::page_limit;
using detail::reference_limit;
using detail::scope_bindings;

struct Operation {
    qint64 epoch, source_revision, policy, purpose, revision, ordinal, reserved, outcome;
    QByteArray generation, current_generation, continuation, frontier, gap;
    qint64 source_epoch, source_policy;
};

std::optional<Operation> select_operation(const SqliteWorkSession &session, const Scope &scope,
                                          const QByteArray &reference) {
    auto args = scope_bindings(scope);
    args.append(reference);
    const auto rows = session.execute(
        {"SELECT o.epoch,s.revision,o.policy_revision,o.purpose,o.revision,o.next_ordinal,"
         "o.slots,o.outcome,o.generation,s.generation,o.continuation,s.frontier,o.gap_ref,"
         "s.epoch,s.policy_revision FROM main.subathon_intake_operations o "
         "JOIN main.subathon_intake_sources s ON s.installation=o.installation "
         "AND s.provider=o.provider AND s.account=o.account AND s.stream=o.stream "
         "WHERE o.installation=? AND o.provider=? AND o.account=? AND o.stream=? "
         "AND o.operation_ref=?",
         args});
    if (rows.isEmpty())
        return std::nullopt;
    const auto &r = rows.front();
    return Operation{r[0].toLongLong(),   r[1].toLongLong(),   r[2].toLongLong(),
                     r[3].toLongLong(),   r[4].toLongLong(),   r[5].toLongLong(),
                     r[6].toLongLong(),   r[7].toLongLong(),   r[8].toByteArray(),
                     r[9].toByteArray(),  r[10].toByteArray(), r[11].toByteArray(),
                     r[12].toByteArray(), r[13].toLongLong(),  r[14].toLongLong()};
}

bool same_fence(const Operation &operation, const PageBinding &request) {
    return operation.epoch == request.source.epoch &&
           operation.source_epoch == request.source.epoch &&
           operation.policy == request.source.policy_revision &&
           operation.source_policy == request.source.policy_revision &&
           operation.generation == request.source.generation &&
           operation.current_generation == request.source.generation;
}

bool same_blob(const QByteArray &a, const QByteArray &b) {
    return a.isNull() == b.isNull() && a == b;
}

QVariant sql_blob(const QByteArray &bytes) { return bytes.isNull() ? QVariant{} : QVariant{bytes}; }

bool page_bounds(const PageBinding &page) {
    return bounded(page.operation_ref, reference_limit) &&
           bounded(page.source.generation, reference_limit) && bounded(page.page, page_limit) &&
           page.expected_continuation.size() <= reference_limit &&
           page.next_continuation.size() <= reference_limit &&
           page.frontier_before.size() <= reference_limit &&
           page.frontier_after.size() <= reference_limit &&
           page.upper_bound.size() <= reference_limit &&
           page.qualified_proof.size() <= document_limit && page.ordinal >= 1 &&
           page.ordinal <= 256 && page.items >= 0 && page.items <= 16;
}

struct GapHead {
    qint64 revision, status;
    QByteArray upper;
};

std::optional<GapHead> select_gap(const SqliteWorkSession &session, const Scope &scope,
                                  const Operation &operation) {
    auto args = scope_bindings(scope);
    args.append({operation.epoch, operation.gap, operation.policy});
    const auto rows = session.execute(
        {"SELECT revision,status,upper_cut FROM main.subathon_intake_gaps WHERE installation=? "
         "AND provider=? AND account=? AND stream=? AND epoch=? AND gap_ref=? AND "
         "policy_revision=?",
         args});
    if (rows.isEmpty())
        return std::nullopt;
    const auto &r = rows.front();
    return GapHead{r[0].toLongLong(), r[1].toLongLong(), r[2].toByteArray()};
}

struct RetainedPage {
    QByteArray page, token, proof;
    qint64 items, prefix, outcome, purpose;
};

std::optional<RetainedPage> select_page(const SqliteWorkSession &session, const PageBinding &page) {
    const auto rows = session.execute(
        {"SELECT page,expected_continuation,proof,item_count,committed_prefix,outcome,purpose "
         "FROM main.subathon_intake_pages WHERE installation=? AND operation_ref=? AND ordinal=?",
         {page.scope.installation, page.operation_ref, page.ordinal}});
    if (rows.isEmpty())
        return std::nullopt;
    const auto &r = rows.front();
    return RetainedPage{r[0].toByteArray(), r[1].toByteArray(), r[2].toByteArray(),
                        r[3].toLongLong(),  r[4].toLongLong(),  r[5].toLongLong(),
                        r[6].toLongLong()};
}

bool same_page(const RetainedPage &retained, const PageBinding &page, const Operation &operation) {
    return retained.page == page.page && same_blob(retained.token, page.expected_continuation) &&
           retained.items == page.items && retained.purpose == operation.purpose;
}

class FrameReader {
  public:
    explicit FrameReader(QByteArrayView bytes) : bytes_(bytes) {}

    bool consume(char value) {
        if (bytes_.isEmpty() || bytes_.front() != value)
            return false;
        bytes_.slice(1);
        return true;
    }

    std::optional<QByteArrayView> field() {
        const auto size = integer();
        if (!size || *size > bytes_.size())
            return std::nullopt;
        const auto field = bytes_.first(*size);
        bytes_.slice(*size);
        return field;
    }

    std::optional<quint32> integer() {
        if (bytes_.size() < 4)
            return std::nullopt;
        const auto value = qFromBigEndian<quint32>(bytes_.data());
        bytes_.slice(4);
        return value;
    }

    bool finished() const { return bytes_.isEmpty(); }

  private:
    QByteArrayView bytes_;
};

bool page_matches_items(const PageAdmission &request) {
    FrameReader page(request.binding.page);
    if (!page.consume('P') || page.integer() != request.normalized_items.size())
        return false;
    for (const auto &item : request.normalized_items) {
        if (page.field() != QByteArrayView(item))
            return false;
    }
    return page.finished() ||
           (page.field() == QByteArrayView(request.binding.next_continuation) &&
            page.field() == QByteArrayView(request.binding.qualified_proof) && page.finished());
}

bool proved_prefix(const SqliteWorkSession &session, const PageBinding &page, qint64 prefix,
                   const QList<QByteArray> *items = nullptr) {
    auto args = scope_bindings(page.scope);
    args.append({page.operation_ref, page.ordinal});
    const auto rows = session.execute(
        {"SELECT item_ordinal,phase,disposition,epoch,generation,policy_revision "
         "FROM main.subathon_intake_entries WHERE installation=? AND provider=? AND account=? "
         "AND stream=? AND operation_ref=? AND page_ordinal=? ORDER BY item_ordinal LIMIT 17",
         args});
    if (rows.size() != prefix || prefix > page.items)
        return false;
    for (qsizetype i = 0; i < rows.size(); ++i) {
        const auto &row = rows[i];
        if (row[0].toLongLong() != i + 1 || row[1].toInt() < 2 || row[2].toInt() == 5 ||
            row[3].toLongLong() != page.source.epoch ||
            row[4].toByteArray() != page.source.generation ||
            row[5].toLongLong() != page.source.policy_revision)
            return false;
        if (items) {
            auto exact = args;
            exact.append({i + 1, (*items)[i]});
            if (session
                    .execute(
                        {"SELECT 1 FROM main.subathon_intake_entries WHERE "
                         "installation=? AND provider=? AND account=? AND stream=? "
                         "AND operation_ref=? AND page_ordinal=? AND item_ordinal=? AND input=?",
                         exact})
                    .isEmpty())
                return false;
        }
    }
    return true;
}

bool current_scope(const SqliteWorkSession &session, const Scope &scope) {
    return detail::valid_scope(scope) && detail::local_installation(session, scope.installation);
}

bool incrementable(qint64 value) { return value > 0 && value < std::numeric_limits<qint64>::max(); }

bool matches_committed_successor(const SqliteWorkSession &session, const PageBinding &page,
                                 const Operation &operation) {
    if (operation.ordinal <= page.ordinal || !incrementable(page.operation_revision) ||
        operation.revision < page.operation_revision + 1)
        return false;

    if (operation.ordinal == page.ordinal + 1)
        return same_blob(operation.continuation, page.next_continuation);
    // Once another page finishes, its immutable bound token is the prior
    // page's output. The current operation token may now point farther ahead.
    const auto successor = session.execute(
        {"SELECT expected_continuation,purpose,outcome FROM main.subathon_intake_pages "
         "WHERE installation=? AND operation_ref=? AND ordinal=?",
         {page.scope.installation, page.operation_ref, page.ordinal + 1}});
    return successor.size() == 1 && successor.front()[1].toLongLong() == operation.purpose &&
           successor.front()[2].toLongLong() == 1 &&
           same_blob(successor.front()[0].toByteArray(), page.next_continuation);
}

struct CoverageChange {
    int segment;
    QByteArray frontier;
    std::optional<QByteArray> baseline_cut{};
};

std::optional<CoverageChange> qualify(const PageBinding &page) {
    const bool same_frontier = same_blob(page.frontier_before, page.frontier_after);
    switch (page.coverage) {
    case Coverage::unproved:
        if (!page.qualified_proof.isNull() || !same_frontier)
            return std::nullopt;
        return CoverageChange{-1, page.frontier_before};
    case Coverage::lost:
        if (page.qualified_proof.isEmpty())
            return std::nullopt;
        return CoverageChange{1, page.frontier_before};
    case Coverage::contiguous:
    case Coverage::recovered:
        if (page.qualified_proof.isEmpty() || page.frontier_after.isEmpty())
            return std::nullopt;
        return CoverageChange{0, page.frontier_after};
    case Coverage::new_baseline: {
        if (page.qualified_proof.isEmpty() || page.frontier_after.isEmpty() ||
            page.upper_bound != page.frontier_after || same_frontier)
            return std::nullopt;
        const QByteArrayView body(page.qualified_proof);
        if (!body.startsWith("CV1"))
            return std::nullopt;
        FrameReader coverage(body.sliced(3));
        if (!coverage.consume('\0') || !coverage.consume('\2') || !coverage.consume('\1'))
            return std::nullopt;
        const auto format = coverage.field();
        const auto position_namespace = coverage.field();
        const auto lower = coverage.field();
        const auto upper = coverage.field();
        const auto reason = coverage.field();
        const auto cut = coverage.field();
        if (!format || format->isEmpty() || !position_namespace || position_namespace->isEmpty() ||
            !lower || lower->isEmpty() || upper != QByteArrayView(page.frontier_after) || !reason ||
            reason->isEmpty() || !cut || !cut->startsWith("CUT1") || !coverage.finished())
            return std::nullopt;
        FrameReader qualified(cut->sliced(4));
        if (!qualified.consume('\1') || qualified.field() != format ||
            qualified.field() != position_namespace || !qualified.consume('\1') ||
            qualified.field() != upper ||
            qualified.field() != QByteArrayView(page.source.generation))
            return std::nullopt;
        const auto proof = qualified.field();
        if (!proof || proof->isEmpty() || !qualified.finished())
            return std::nullopt;
        return CoverageChange{0, page.frontier_after, cut->toByteArray()};
    }
    }
    return std::nullopt;
}

} // namespace

BoundPage stage_page(const SqliteWorkSession &session, const PageAdmission &request) {
    const auto &page = request.binding;
    if (!current_scope(session, page.scope))
        return {Status::wrong_scope};
    if (!page_bounds(page) || request.normalized_items.size() > 16)
        return {Status::intake_limit};
    qsizetype bytes = 5;
    for (const auto &item : request.normalized_items) {
        if (!bounded(item, item_limit) || item.size() + 4 > page_limit - bytes)
            return {Status::intake_limit};
        bytes += item.size() + 4;
    }
    if (request.normalized_items.size() != page.items || !page_matches_items(request))
        return {Status::conflict};

    const auto operation = select_operation(session, page.scope, page.operation_ref);
    if (!operation || !same_fence(*operation, page) || operation->outcome != 0 ||
        operation->source_revision != page.source.revision ||
        operation->revision != page.operation_revision || operation->ordinal != page.ordinal)
        return {Status::stale};

    if (operation->purpose == 0) {
        const auto gap = select_gap(session, page.scope, *operation);
        if (!gap || gap->revision != page.gap_revision)
            return {Status::stale};
    }
    if (const auto retained = select_page(session, page)) {
        if (!same_page(*retained, page, *operation) || retained->outcome != 0 ||
            !same_blob(operation->continuation, page.expected_continuation))
            return {Status::conflict};
        if (!proved_prefix(session, page, retained->prefix, &request.normalized_items))
            return {Status::incomplete_prefix};
        return {Status::exact_replay, retained->prefix};
    }
    if (!same_blob(operation->continuation, page.expected_continuation))
        return {Status::conflict};
    if (operation->reserved > 4096 - page.items)
        return {Status::intake_limit};
    if (request.quarantine_preflight) {
        const auto &entry = *request.quarantine_preflight;
        if (entry.scope != page.scope || !request.normalized_items.contains(entry.normalized))
            return {Status::conflict};
        const auto accepted = preflight_quarantine(session, entry);
        switch (accepted) {
        case QuarantineStatus::staged:
        case QuarantineStatus::exact_replay:
            break;
        case QuarantineStatus::quarantine_full:
            return {Status::quarantine_full};
        case QuarantineStatus::intake_limit:
            return {Status::intake_limit};
        case QuarantineStatus::wrong_scope:
        case QuarantineStatus::invalid_authority:
            return {Status::wrong_scope};
        case QuarantineStatus::conflict:
            return {Status::conflict};
        case QuarantineStatus::stale:
            return {Status::stale};
        }
    }
    (void)session.execute({"INSERT INTO main.subathon_intake_pages VALUES(?,?,?,?,?,?,0,0,NULL,?)",
                           {page.scope.installation, page.operation_ref, page.ordinal, page.page,
                            sql_blob(page.expected_continuation), page.items, operation->purpose}});
    detail::require_changed_one(session);
    if (page.items != 0) {
        (void)session.execute(
            {"UPDATE main.subathon_intake_operations SET slots=slots+? "
             "WHERE installation=? AND operation_ref=? AND epoch=? "
             "AND generation=? AND revision=? AND next_ordinal=? AND outcome=0",
             {page.items, page.scope.installation, page.operation_ref, page.source.epoch,
              page.source.generation, page.operation_revision, page.ordinal}});
        detail::require_changed_one(session);
    }
    return {Status::staged, 0};
}

HistoryStatus stage_page_finalization(const SqliteWorkSession &session, const PageBinding &page) {
    if (!current_scope(session, page.scope))
        return Status::wrong_scope;
    if (!page_bounds(page))
        return Status::intake_limit;

    const auto operation = select_operation(session, page.scope, page.operation_ref);
    if (!operation || !same_fence(*operation, page))
        return Status::stale;
    const auto retained = select_page(session, page);
    if (!retained || !same_page(*retained, page, *operation))
        return Status::conflict;
    if (retained->prefix != page.items || !proved_prefix(session, page, retained->prefix))
        return Status::incomplete_prefix;

    if (retained->outcome == 1) {
        return same_blob(retained->proof, page.qualified_proof) &&
                       matches_committed_successor(session, page, *operation)
                   ? Status::exact_replay
                   : Status::stale;
    }
    if (retained->outcome != 0 || operation->outcome != 0 ||
        operation->source_revision != page.source.revision ||
        operation->revision != page.operation_revision || operation->ordinal != page.ordinal)
        return Status::stale;
    if (!same_blob(operation->continuation, page.expected_continuation) ||
        !same_blob(operation->frontier, page.frontier_before))
        return Status::conflict;
    if (!incrementable(operation->revision))
        return Status::intake_limit;

    const auto coverage = qualify(page);
    if (!coverage)
        return Status::unproved_range;
    if (!page.next_continuation.isNull() &&
        same_blob(page.next_continuation, page.expected_continuation) && page.items == 0 &&
        same_blob(coverage->frontier, page.frontier_before))
        return Status::unproved_range;
    const auto gap =
        operation->purpose == 0 ? select_gap(session, page.scope, *operation) : std::nullopt;
    if (operation->purpose == 0 && (!gap || gap->revision != page.gap_revision))
        return Status::stale;

    const bool new_frontier = !same_blob(coverage->frontier, page.frontier_before);
    if (new_frontier && !incrementable(operation->source_revision))
        return Status::intake_limit;
    if (gap && coverage->segment >= 0 && !incrementable(gap->revision))
        return Status::intake_limit;
    if (coverage->baseline_cut &&
        (!gap || gap->status != 1 || gap->upper.isEmpty() || gap->upper == page.frontier_after))
        return Status::unproved_range;

    if (new_frontier || coverage->baseline_cut) {
        QList<QVariant> source{sql_blob(coverage->frontier), coverage->baseline_cut
                                                                 ? sql_blob(*coverage->baseline_cut)
                                                                 : QVariant{}};
        source.append(scope_bindings(page.scope));
        source.append({page.source.epoch, page.source.generation, page.source.revision,
                       page.source.policy_revision});
        (void)session.execute(
            {"UPDATE main.subathon_intake_sources SET revision=revision+1,frontier=?,"
             "cut=coalesce(?,cut) WHERE installation=? AND provider=? AND account=? AND stream=? "
             "AND epoch=? AND generation=? AND revision=? AND policy_revision=?",
             source});
        detail::require_changed_one(session);
    }
    if (gap && coverage->segment >= 0) {
        auto segment = scope_bindings(page.scope);
        segment.append({page.source.epoch, operation->gap, page.operation_ref, gap->revision + 1,
                        coverage->segment, page.qualified_proof});
        (void)session.execute(
            {"INSERT INTO main.subathon_intake_segments VALUES(?,?,?,?,?,?,?,?,1,?,?)", segment});
        detail::require_changed_one(session);

        QList<QVariant> key{coverage->segment == 1                 ? 1
                            : page.coverage == Coverage::recovered ? 2
                                                                   : gap->status};
        key.append(scope_bindings(page.scope));
        key.append({page.source.epoch, operation->gap, gap->revision, page.source.policy_revision});
        (void)session.execute(
            {"UPDATE main.subathon_intake_gaps SET revision=revision+1,status=? "
             "WHERE installation=? AND provider=? AND account=? AND stream=? AND epoch=? "
             "AND gap_ref=? AND revision=? AND policy_revision=?",
             key});
        detail::require_changed_one(session);
    }
    (void)session.execute(
        {"UPDATE main.subathon_intake_operations SET "
         "revision=revision+1,next_ordinal=next_ordinal+1,"
         "continuation=?,outcome=? WHERE installation=? AND operation_ref=? AND epoch=? "
         "AND generation=? "
         "AND policy_revision=? AND revision=? AND next_ordinal=? AND outcome=0",
         {sql_blob(page.next_continuation), page.next_continuation.isNull() ? 1 : 0,
          page.scope.installation, page.operation_ref, page.source.epoch, page.source.generation,
          page.source.policy_revision, page.operation_revision, page.ordinal}});
    detail::require_changed_one(session);
    (void)session.execute(
        {"UPDATE main.subathon_intake_pages SET outcome=1,proof=? WHERE installation=? AND "
         "operation_ref=? AND ordinal=? AND committed_prefix=item_count AND outcome=0",
         {sql_blob(page.qualified_proof), page.scope.installation, page.operation_ref,
          page.ordinal}});
    detail::require_changed_one(session);
    return Status::staged;
}

HistoryStatus stage_rollover(const SqliteWorkSession &session, const Rollover &input) {
    if (!current_scope(session, input.scope))
        return Status::wrong_scope;
    if (!bounded(input.current_authority, document_limit))
        return Status::invalid_authority;
    if (!bounded(input.operation, reference_limit) ||
        !bounded(input.replacement, reference_limit) ||
        !bounded(input.immutable_intent, document_limit) ||
        input.frontier.size() > reference_limit || input.continuation.size() > reference_limit)
        return Status::intake_limit;
    const auto operation = select_operation(session, input.scope, input.operation);
    if (!operation || operation->source_revision != input.source_revision ||
        operation->revision != input.operation_revision || operation->outcome != 0 ||
        operation->epoch != operation->source_epoch ||
        operation->policy != operation->source_policy ||
        operation->generation != operation->current_generation)
        return Status::stale;
    if (!same_blob(operation->continuation, input.continuation) ||
        !same_blob(operation->frontier, input.frontier))
        return Status::conflict;
    if (operation->ordinal != 257 && operation->reserved != 4096)
        return Status::conflict;
    if (operation->purpose == 0 && !select_gap(session, input.scope, *operation))
        return Status::stale;

    const auto bound = session.execute({"SELECT intent FROM main.subathon_intake_operations WHERE "
                                        "installation=? AND operation_ref=?",
                                        {input.scope.installation, input.operation}});
    if (bound.front().front().toByteArray() != input.immutable_intent ||
        !session
             .execute({"SELECT 1 FROM main.subathon_intake_operations WHERE installation=? "
                       "AND operation_ref=?",
                       {input.scope.installation, input.replacement}})
             .isEmpty())
        return Status::conflict;

    const auto prefix = session.execute(
        {"SELECT ordinal,item_count,committed_prefix,outcome FROM main.subathon_intake_pages "
         "WHERE installation=? AND operation_ref=? ORDER BY ordinal LIMIT 257",
         {input.scope.installation, input.operation}});
    if (prefix.size() != operation->ordinal - 1)
        return Status::incomplete_prefix;
    qint64 used = 0;
    for (qsizetype i = 0; i < prefix.size(); ++i) {
        const auto &row = prefix[i];
        if (row[0].toLongLong() != i + 1 || row[1].toLongLong() != row[2].toLongLong() ||
            row[3].toInt() != 1)
            return Status::incomplete_prefix;
        used += row[1].toLongLong();
    }
    if (used != operation->reserved)
        return Status::incomplete_prefix;
    (void)session.execute(
        {"UPDATE main.subathon_intake_operations SET outcome=3 WHERE installation=? AND "
         "operation_ref=? AND revision=? AND outcome=0",
         {input.scope.installation, input.operation, input.operation_revision}});
    detail::require_changed_one(session);
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_operations "
         "SELECT installation,?,provider,account,stream,epoch,policy_revision,gap_ref,purpose,"
         "generation,1,intent,continuation,1,0,0,NULL,NULL FROM main.subathon_intake_operations "
         "WHERE installation=? AND operation_ref=? AND revision=? AND outcome=3",
         {input.replacement, input.scope.installation, input.operation, input.operation_revision}});
    detail::require_changed_one(session);
    return Status::staged;
}

} // namespace subathon::server::connector_intake
