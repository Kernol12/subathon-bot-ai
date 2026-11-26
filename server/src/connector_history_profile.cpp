// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/connector_history_profile.hpp>

#include "sqlite_connector_intake_private.hpp"

#include <QDateTime>
#include <QTimeZone>
#include <QtEndian>
#include <algorithm>
#include <limits>

namespace subathon::server::connector_intake {

class QualifiedPage final {
  public:
    StartupProfileRegistry &registry;
    std::shared_ptr<const StartupProfile> profile;
    AdmittedHistoryRequest binding;
    PageAdmission page;
};

namespace {
using Status = IntakeStatus;
using detail::bounded;
using detail::document_limit;
using detail::item_limit;
using detail::page_limit;
using detail::reference_limit;
using detail::scope_bindings;

Status authorize(const OwnerInputs &owner, Action action, const Scope &scope) {
    if (!owner.current_authority)
        return Status::unavailable;
    switch (owner.current_authority(action, scope)) {
    case Authorization::allowed:
        return Status::committed;
    case Authorization::denied:
        return Status::denied;
    case Authorization::unavailable:
        return Status::unavailable;
    }
    return Status::unavailable;
}

Status outcome(HistoryStatus status) {
    switch (status) {
    case HistoryStatus::staged:
        return Status::committed;
    case HistoryStatus::exact_replay:
        return Status::exact_replay;
    case HistoryStatus::wrong_scope:
        return Status::conflict;
    case HistoryStatus::invalid_authority:
        return Status::denied;
    case HistoryStatus::intake_limit:
        return Status::limit_exceeded;
    case HistoryStatus::stale:
        return Status::stale;
    case HistoryStatus::conflict:
        return Status::conflict;
    case HistoryStatus::busy:
        return Status::busy;
    case HistoryStatus::incomplete_prefix:
    case HistoryStatus::unproved_range:
        return Status::unproved;
    case HistoryStatus::quarantine_full:
        return Status::quarantine_full;
    }
    return Status::unavailable;
}

Status outcome(HistoryFailure status) {
    switch (status) {
    case HistoryFailure::unknown_capability:
    case HistoryFailure::unproved:
        return Status::unproved;
    case HistoryFailure::unsupported:
        return Status::unsupported;
    case HistoryFailure::temporarily_unavailable:
        return Status::unavailable;
    }
    return Status::unavailable;
}

bool accepted(Status status) {
    return status == Status::committed || status == Status::exact_replay;
}

std::optional<quint64> next_invocation(quint64 &count) {
    if (count == std::numeric_limits<quint64>::max())
        return {};
    return ++count;
}

template <class Step>
SqliteWorkResult run(SqliteWriter &writer, Status &status, Step step, bool write = false) {
    const auto result =
        writer.run_idle_owner_work([&](const SqliteWorkContext &context) -> QVariant {
            const auto session = context.begin();
            status = step(session);
            if (write && status == Status::committed)
                session.commit();
            else
                session.rollback();
            return true;
        });
    if (result.error == WorkError::busy)
        status = Status::busy;
    else if (result.error == WorkError::resource_limit || result.error == WorkError::result_limit)
        status = Status::limit_exceeded;
    else if ((result.error != WorkError::none &&
              !(result.error == WorkError::not_committed &&
                result.transaction == TransactionOutcome::rolled_back)) ||
             (write && status == Status::committed &&
              result.transaction != TransactionOutcome::committed))
        status = Status::unavailable;
    return result;
}

bool current_profile(const HistoryProfile &profile, const Selection &source,
                     const QByteArray &cursor) {
    const auto &snapshot = profile.snapshot;
    return snapshot && snapshot->scope == source.scope &&
           snapshot->profile_revision == source.profile && snapshot->cursor_contract == cursor &&
           snapshot->generation == source.fence.generation &&
           profile.registry.select(source.scope, source.profile, cursor) == snapshot;
}

bool single_profile(const HistoryProfile &profile, const OwnerInputs &owner,
                    const Selection &source, const QByteArray &cursor) {
    return current_profile(profile, source, cursor) && &profile.snapshot->history == &owner.history;
}

bool same(const Selection &a, const Selection &b) {
    return a.scope == b.scope && a.fence == b.fence && a.profile == b.profile;
}
bool same(const HistoryInterval &a, const HistoryInterval &b) {
    return a.start == b.start && a.end == b.end && a.membership_basis == b.membership_basis &&
           a.order_basis == b.order_basis;
}
bool same(const QualifiedCut &a, const QualifiedCut &b) {
    return a.format == b.format && a.position_namespace == b.position_namespace &&
           a.position == b.position && a.qualified_generation == b.qualified_generation &&
           a.proof == b.proof;
}
bool same(const HistoryRequest &a, const HistoryRequest &b) {
    return same(a.source, b.source) && a.operation == b.operation && a.ordinal == b.ordinal &&
           a.continuation == b.continuation && a.frontier == b.frontier &&
           same(a.interval, b.interval) && same(a.upper, b.upper) &&
           a.resolved_start_ms == b.resolved_start_ms;
}
bool same(const CutRequest &a, const CutRequest &b) {
    return same(a.source, b.source) && a.purpose == b.purpose &&
           a.prior_frontier == b.prior_frontier;
}
bool same(const CutObservation &a, const CutObservation &b) {
    return same(a.request, b.request) && same(a.cut, b.cut) && a.boundary == b.boundary;
}
bool same(const Item &a, const Item &b) {
    return a.key == b.key && a.role == b.role && a.identifier_namespace == b.identifier_namespace &&
           a.identifier == b.identifier && a.facts == b.facts && a.position == b.position &&
           a.membership_proof == b.membership_proof &&
           a.disjointness_proof == b.disjointness_proof && a.activity == b.activity &&
           a.source_time == b.source_time && a.evidence == b.evidence && a.kind == b.kind &&
           a.award_class == b.award_class && a.aggregate_metric == b.aggregate_metric &&
           a.normalized == b.normalized;
}
bool same(const std::optional<RangeProof> &a, const std::optional<RangeProof> &b) {
    return a && b ? a->meaning == b->meaning && a->lower == b->lower && a->upper == b->upper &&
                        a->proof == b->proof && a->loss_reason == b->loss_reason
                  : a.has_value() == b.has_value();
}

QByteArray number(quint32 n) {
    const auto bytes = qToBigEndian(n);
    return QByteArray(reinterpret_cast<const char *>(&bytes), sizeof(bytes));
}

QByteArray frame(const QByteArray &bytes) {
    return number(static_cast<quint32>(bytes.size())) + bytes;
}

QByteArray cut_document(const CutObservation &observation) {
    const auto &cut = observation.cut;
    return QByteArray("CUT1") + char(observation.boundary) + frame(cut.format) +
           frame(cut.position_namespace) +
           (cut.position ? QByteArray(1, '\1') + frame(*cut.position) : QByteArray(1, '\0')) +
           frame(cut.qualified_generation) + frame(cut.proof);
}

QByteArray blob(const std::optional<QByteArray> &value) {
    return value ? (value->isNull() ? QByteArray("", 0) : *value) : QByteArray{};
}

struct Head {
    Fence fence;
    QByteArray cursor, frontier;
};

std::optional<Head> read_head(const SqliteWorkSession &session, const Scope &scope) {
    const auto rows = session.execute(
        {"SELECT epoch,revision,policy_revision,generation,document,frontier,typeof(frontier) FROM "
         "main.subathon_intake_sources "
         "WHERE installation=? AND provider=? AND account=? AND stream=?",
         scope_bindings(scope)});
    if (rows.size() != 1)
        return {};
    const auto &r = rows.front();
    return Head{{r[0].toLongLong(), r[1].toLongLong(), r[2].toLongLong(), r[3].toByteArray()},
                r[4].toByteArray(),
                r[6].toString() == "blob" ? blob(r[5].toByteArray()) : QByteArray{}};
}

SqlRows read_operation(const SqliteWorkSession &session, const HistoryRequest &request) {
    auto args = scope_bindings(request.source.scope);
    args.append(request.operation);
    return session.execute(
        {"SELECT "
         "revision,intent,purpose,gap_ref,generation,epoch,policy_revision,next_ordinal,outcome,"
         "continuation,typeof(continuation) "
         "FROM main.subathon_intake_operations WHERE installation=? AND provider=? AND account=? "
         "AND stream=? AND operation_ref=?",
         args});
}

bool pinned_operation(const SqliteWorkSession &session, const AdmittedHistoryRequest &binding,
                      bool terminal) {
    const auto rows = read_operation(session, binding.request);
    if (rows.size() != 1)
        return false;
    const auto &r = rows.front();
    const auto &fence = binding.request.source.fence;
    return r[1].toByteArray() == binding.intent && r[2].toInt() == int(binding.purpose) &&
           r[3].isNull() == !binding.gap && r[3].toByteArray() == blob(binding.gap) &&
           r[4].toByteArray() == fence.generation && r[5].toLongLong() == fence.epoch &&
           r[6].toLongLong() == fence.policy_revision &&
           (terminal
                ? r[0].toLongLong() > binding.operation_revision &&
                      r[7].toLongLong() > binding.request.ordinal
                : r[0].toLongLong() == binding.operation_revision &&
                      r[7].toLongLong() == binding.request.ordinal && r[8].toInt() == 0 &&
                      (r[10].toString() == "blob") == binding.request.continuation.has_value() &&
                      r[9].toByteArray() == blob(binding.request.continuation));
}

Status verify_binding(const SqliteWorkSession &session, const AdmittedHistoryRequest &binding,
                      bool terminal = false) {
    const auto &source = binding.request.source;
    const auto head = read_head(session, source.scope);
    if (!head || head->cursor != binding.cursor_contract ||
        (terminal ? head->fence.generation != source.fence.generation ||
                        head->fence.epoch != source.fence.epoch ||
                        head->fence.policy_revision != source.fence.policy_revision
                  : head->fence != source.fence ||
                        head->frontier.isNull() != !binding.request.frontier ||
                        head->frontier != blob(binding.request.frontier)) ||
        !pinned_operation(session, binding, terminal))
        return Status::stale;
    if (!terminal && binding.gap) {
        auto args = scope_bindings(source.scope);
        args.append({source.fence.epoch, *binding.gap, source.fence.policy_revision});
        const auto gap = session.execute(
            {"SELECT revision,upper_cut FROM main.subathon_intake_gaps WHERE installation=? AND "
             "provider=? AND account=? AND stream=? AND epoch=? AND gap_ref=? AND "
             "policy_revision=?",
             args});
        if (gap.size() != 1 || gap.front().front().toLongLong() != binding.gap_revision ||
            gap.front()[1].toByteArray() != blob(binding.request.upper.position))
            return Status::stale;
    }
    return Status::committed;
}

bool has_page(const SqliteWorkSession &session, const HistoryRequest &request) {
    auto args = scope_bindings(request.source.scope);
    args.append({request.operation, request.ordinal});
    return !session
                .execute({"SELECT 1 FROM main.subathon_intake_operations o JOIN "
                          "main.subathon_intake_pages p "
                          "USING(installation,operation_ref) WHERE o.installation=? AND "
                          "o.provider=? AND "
                          "o.account=? AND o.stream=? AND o.operation_ref=? AND p.ordinal=?",
                          args})
                .isEmpty();
}

PageQualification refusal(Status status, std::optional<HistoryFailure> adapter = {},
                          std::optional<HistoryStatus> storage = {}) {
    return {status, {}, adapter, storage, {}};
}

PageQualification project(const AdmittedHistoryRequest &binding, const Page &page,
                          const HistoryEvidence &evidence, const HistoryProfile &profile,
                          const QByteArray &reconnect = {}) {
    if (!evidence.order_proved || evidence.slot_evidence.size() != page.items.size())
        return refusal(Status::unproved);
    const auto &request = binding.request;
    if (request.ordinal <= 0 || request.ordinal > 256 || page.items.size() > 16 ||
        (page.next && page.next->size() > reference_limit))
        return refusal(Status::limit_exceeded);

    const auto before = blob(request.frontier);
    auto after = before;
    QByteArray trailer, proof;
    auto coverage = Coverage::unproved;
    if (evidence.range) {
        const auto &range = *evidence.range;
        if (!same(evidence.range, page.coverage))
            return refusal(Status::invalid_input);
        if (!bounded(range.proof, document_limit))
            return refusal(range.proof.isEmpty() ? Status::unproved : Status::limit_exceeded);
        if (binding.purpose == HistoryPurpose::import) {
            trailer = range.proof;
        } else {
            const bool lost = range.meaning == Coverage::lost;
            if ((range.meaning != Coverage::contiguous && range.meaning != Coverage::recovered &&
                 !lost) ||
                range.lower != before || range.upper.isEmpty() ||
                (lost ? range.loss_reason.isEmpty() : !range.loss_reason.isEmpty()))
                return refusal(Status::invalid_input);
            trailer = QByteArray("CV1") + char(lost ? 1 : 0) + char(request.interval.start) +
                      char(request.interval.end) + frame(request.upper.format) +
                      frame(request.upper.position_namespace) + frame(range.lower) +
                      frame(range.upper) + frame(range.loss_reason) + frame(range.proof);
            proof = trailer;
            if (range.meaning == Coverage::recovered && range.upper != request.upper.position)
                return refusal(Status::invalid_input);
            coverage = range.meaning;
            if (!lost)
                after = range.upper;
        }
    } else if (page.coverage) {
        return refusal(Status::invalid_input);
    }
    if (trailer.size() > document_limit)
        return refusal(Status::limit_exceeded);
    if (page.next && page.next == request.continuation && page.items.isEmpty() && after == before)
        return refusal(Status::unproved);

    QByteArray document = QByteArray("P") + number(static_cast<quint32>(page.items.size()));
    QList<QByteArray> items;
    for (const auto &item : page.items) {
        if (!bounded(item.normalized, item_limit))
            return refusal(Status::limit_exceeded);
        items.append(item.normalized);
        document += frame(item.normalized);
    }
    document += frame(blob(page.next)) + frame(trailer);
    if (document.size() > page_limit)
        return refusal(Status::limit_exceeded);

    PageBinding stored{request.source.scope,
                       request.source.fence,
                       request.operation,
                       binding.operation_revision,
                       binding.gap_revision,
                       blob(request.continuation),
                       document,
                       request.ordinal,
                       page.items.size(),
                       blob(page.next),
                       before,
                       after,
                       binding.purpose == HistoryPurpose::import ? trailer : proof,
                       coverage,
                       blob(request.upper.position),
                       reconnect};
    auto handle = std::make_shared<const QualifiedPage>(
        QualifiedPage{profile.registry,
                      profile.snapshot,
                      binding,
                      {std::move(stored), std::move(items), evidence.quarantine}});
    return {Status::committed, std::move(handle), {}, {}, evidence.slot_evidence};
}

PageBinding finalization(const QualifiedPage &handle) {
    auto page = handle.page.binding;
    if (handle.binding.purpose == HistoryPurpose::import)
        page.qualified_proof = {};
    return page;
}
} // namespace

PageQualification qualify_history_page(SqliteWriter &writer, const OwnerInputs &owner,
                                       const HistoryProfile &profile,
                                       const AdmittedHistoryRequest &binding,
                                       const HistoryPageCandidate &candidate) {
    const auto &request = binding.request;
    auto status = authorize(owner, Action::page, request.source.scope);
    if (status != Status::committed)
        return refusal(status);
    if (!single_profile(profile, owner, request.source, binding.cursor_contract) ||
        profile.snapshot->registry_revision != binding.registry_revision)
        return refusal(Status::stale);
    run(writer, status, [&](const SqliteWorkSession &session) {
        if (const auto current = authorize(owner, Action::page, request.source.scope);
            current != Status::committed)
            return current;
        if (has_page(session, request)) {
            const auto inspect = authorize(owner, Action::inspect, request.source.scope);
            if (inspect != Status::committed)
                return inspect;
        }
        return verify_binding(session, binding);
    });
    if (status != Status::committed)
        return refusal(status);
    const auto invocation = next_invocation(profile.page_invocations);
    if (!invocation)
        return refusal(Status::unavailable);
    const auto fetched = owner.history.fetch_page(request);
    if (const auto *failure = std::get_if<HistoryFailure>(&fetched))
        return refusal(outcome(*failure), *failure);
    const auto &observed = std::get<Page>(fetched);
    auto item_equal = [](const Item &a, const Item &b) { return same(a, b); };
    if (!same(observed.request, request) || !same(candidate.page.request, request) ||
        candidate.purpose != binding.purpose || candidate.gap != binding.gap ||
        candidate.page.next != observed.next || !same(candidate.page.coverage, observed.coverage) ||
        !std::ranges::equal(candidate.page.items, observed.items, item_equal))
        return refusal(Status::invalid_input);
    const auto evidence =
        profile.validate
            ? profile.validate(PageInvocation{*invocation, owner.history, binding, observed})
            : std::nullopt;
    if (!evidence)
        return refusal(Status::unproved);
    if (!single_profile(profile, owner, request.source, binding.cursor_contract) ||
        profile.snapshot->registry_revision != binding.registry_revision)
        return refusal(Status::stale);
    return project(binding, observed, *evidence, profile);
}

PageOutcome bind_history_page(SqliteWriter &writer, const OwnerInputs &owner,
                              const QualifiedPage &handle) {
    const auto &request = handle.binding.request;
    auto status = authorize(owner, Action::page, request.source.scope);
    if (status != Status::committed)
        return {status, {}, {}};
    if (&handle.profile->history != &owner.history ||
        handle.registry.select(request.source.scope, request.source.profile,
                               handle.binding.cursor_contract) != handle.profile)
        return {Status::stale, {}, {}};
    std::optional<BoundPage> result;
    run(
        writer, status,
        [&](const SqliteWorkSession &session) {
            if (const auto current = authorize(owner, Action::page, request.source.scope);
                current != Status::committed)
                return current;
            if (has_page(session, request)) {
                const auto inspect = authorize(owner, Action::inspect, request.source.scope);
                if (inspect != Status::committed)
                    return inspect;
            }
            if (const auto current = verify_binding(session, handle.binding);
                current != Status::committed)
                return current;
            result = stage_page(session, handle.page);
            return outcome(result->status);
        },
        true);
    return {status, result ? std::optional{result->status} : std::nullopt,
            result && accepted(status) ? result->committed_prefix : std::nullopt};
}

PageOutcome finalize_history_page(SqliteWriter &writer, const OwnerInputs &owner,
                                  const QualifiedPage &handle) {
    const auto &request = handle.binding.request;
    auto status = authorize(owner, Action::page, request.source.scope);
    if (status != Status::committed)
        return {status, {}, {}};
    if (&handle.profile->history != &owner.history ||
        handle.registry.select(request.source.scope, request.source.profile,
                               handle.binding.cursor_contract) != handle.profile)
        return {Status::stale, {}, {}};
    std::optional<HistoryStatus> result;
    run(
        writer, status,
        [&](const SqliteWorkSession &session) {
            if (const auto current = authorize(owner, Action::page, request.source.scope);
                current != Status::committed)
                return current;
            if (verify_binding(session, handle.binding) != Status::committed) {
                const auto inspect = authorize(owner, Action::inspect, request.source.scope);
                if (inspect != Status::committed)
                    return inspect;
                if (verify_binding(session, handle.binding, true) != Status::committed)
                    return Status::stale;
            }
            result = stage_page_finalization(session, finalization(handle));
            return outcome(*result);
        },
        true);
    return {status, result, {}};
}

PageQualification reconstruct_history_page(SqliteWriter &writer, const OwnerInputs &owner,
                                           const HistoryProfile &profile,
                                           const RetainedPageRequest &request) {
    const auto &binding = request.current;
    const auto &source = binding.request.source;
    auto status = authorize(owner, Action::page, source.scope);
    if (status != Status::committed)
        return refusal(status);
    status = authorize(owner, Action::inspect, source.scope);
    if (status != Status::committed)
        return refusal(status);
    if (!single_profile(profile, owner, source, binding.cursor_contract) ||
        profile.snapshot->registry_revision != binding.registry_revision)
        return refusal(Status::stale);
    if (request.page.size() > page_limit)
        return refusal(Status::limit_exceeded);
    const auto &original = request.original;
    auto same_source = original.request;
    same_source.source.fence = source.fence;
    if (!same(same_source, binding.request) || original.intent != binding.intent ||
        original.gap != binding.gap || original.gap_revision != binding.gap_revision ||
        original.purpose != binding.purpose ||
        original.cursor_contract != binding.cursor_contract || !bounded(request.page, page_limit))
        return refusal(Status::unproved);
    if (!profile.reconstruct)
        return refusal(Status::unproved);
    const auto recovered = profile.reconstruct(request);
    if (!recovered || !same(recovered->page.request, original.request))
        return refusal(Status::unproved);
    auto projected =
        project(binding, recovered->page, recovered->evidence, profile, request.reconnect);
    if (!projected.qualified || projected.qualified->page.binding.page != request.page)
        return refusal(Status::unproved);
    std::optional<HistoryStatus> result;
    run(writer, status, [&](const SqliteWorkSession &session) {
        for (const auto action : {Action::page, Action::inspect}) {
            const auto access = authorize(owner, action, source.scope);
            if (access != Status::committed)
                return access;
        }
        if (!single_profile(profile, owner, source, binding.cursor_contract))
            return Status::stale;
        if (!has_page(session, binding.request))
            return Status::unproved;
        if (verify_binding(session, binding) == Status::committed) {
            result = stage_page(session, projected.qualified->page).status;
        } else if (verify_binding(session, binding, true) == Status::committed) {
            // Only verify terminal readback when the operation has passed this page.
            result = stage_page_finalization(session, finalization(*projected.qualified));
        } else {
            return Status::stale;
        }
        if (*result == HistoryStatus::staged)
            return Status::unproved;
        return *result == HistoryStatus::exact_replay ? Status::committed : outcome(*result);
    });
    if (status != Status::committed)
        return refusal(status, {}, result);
    return projected;
}

namespace {
Status check_head(const SqliteWorkSession &session, const Selection &selection,
                  const QByteArray &cursor) {
    const auto head = read_head(session, selection.scope);
    return head && head->fence == selection.fence && head->cursor == cursor ? Status::committed
                                                                            : Status::stale;
}

Status verify_cut(const CutRequest &selected, const CutObservation &echo,
                  const CutObservation &actual, Endpoint endpoint) {
    if (!same(selected, actual.request) || !same(echo, actual))
        return Status::invalid_input;
    if (actual.boundary != endpoint ||
        actual.cut.qualified_generation != selected.source.fence.generation)
        return Status::unproved;
    return Status::committed;
}

Status qualify_cut(const HistoryProfile &profile, const CutRequest &selected,
                   const CutObservation &echo, Endpoint endpoint, const CutWitness &witness) {
    const auto invocation = next_invocation(profile.cut_invocations);
    if (!invocation)
        return Status::unavailable;
    const auto cut = profile.snapshot->history.observe_cut(selected);
    if (const auto *failure = std::get_if<HistoryFailure>(&cut))
        return outcome(*failure);
    const auto &actual = std::get<CutObservation>(cut);
    const auto status = verify_cut(selected, echo, actual, endpoint);
    if (status != Status::committed)
        return status;
    return witness && witness(*invocation, actual) ? Status::committed : Status::unproved;
}

std::optional<qint64> local_start(const OwnerInputs &owner, const OperationIntent &intent) {
    const auto date = QDate::fromString(QString::fromUtf8(intent.local_date), Qt::ISODate);
    if (!date.isValid() || !owner.zone)
        return {};
    const auto zone = owner.zone(intent.zone_id, intent.zone_basis, date);
    if (!zone || zone->transitions.size() > 1024)
        return {};
    const auto day = QDateTime(date, QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
    constexpr qint64 day_length = 86400000;
    auto offset = zone->initial_offset_seconds;
    qint64 lower = std::numeric_limits<qint64>::min();
    std::optional<qint64> first;
    const auto consider = [&](qint64 upper) {
        constexpr int offset_limit = 86400;
        if (offset < -offset_limit || offset > offset_limit)
            return false;
        const auto candidate = std::max(lower, day - qint64(offset) * 1000);
        if (candidate < upper && candidate < day + day_length - qint64(offset) * 1000 &&
            (!first || candidate < *first))
            first = candidate;
        return true;
    };
    for (const auto &transition : zone->transitions) {
        if (transition.utc_ms <= lower || !consider(transition.utc_ms))
            return {};
        lower = transition.utc_ms;
        offset = transition.offset_after_seconds;
    }
    return consider(std::numeric_limits<qint64>::max()) ? first : std::nullopt;
}

std::optional<qint64> retained_import_start(QByteArrayView bytes) {
    if (!bytes.startsWith(QByteArrayView("IOP1\1", 5)) || bytes.size() > document_limit)
        return {};
    bytes = bytes.sliced(5);
    const auto skip_field = [&]() {
        if (bytes.size() < 4)
            return false;
        const auto n = qFromBigEndian<quint32>(bytes.data());
        if (n > bytes.size() - 4)
            return false;
        bytes = bytes.sliced(4 + n);
        return true;
    };
    for (int field = 0; field < 4; ++field)
        if (!skip_field())
            return {};
    if (bytes.size() < 18)
        return {};
    bytes = bytes.sliced(18);
    for (int field = 0; field < 7; ++field)
        if (!skip_field())
            return {};
    if (bytes.size() < 8)
        return {};
    return qFromBigEndian<qint64>(bytes.data());
}
} // namespace

IntakeStatus open_history_gap(SqliteWriter &writer, const OwnerInputs &owner,
                              const HistoryProfile &profile, const GapOpening &binding,
                              const OpenGap &candidate, const GapProducer &producer) {
    auto status = authorize(owner, Action::gap, binding.scope);
    if (status != Status::committed)
        return status;
    const Selection selected{binding.scope, binding.expected,
                             profile.snapshot ? profile.snapshot->profile_revision : QByteArray{}};
    if (!single_profile(profile, owner, selected, binding.cursor_contract))
        return Status::stale;
    run(writer, status, [&](const auto &session) {
        return check_head(session, selected, binding.cursor_contract);
    });
    if (status != Status::committed)
        return status;
    if (candidate.key != binding.gap_ref)
        return Status::invalid_input;
    const auto invocation = next_invocation(profile.gap_invocations);
    if (!invocation)
        return Status::unavailable;
    const auto evidence =
        producer ? producer(GapInvocation{*invocation, *profile.snapshot, binding}) : std::nullopt;
    if (!evidence)
        return Status::unproved;
    if (candidate.observed_utc_ms && candidate.observed_utc_ms != evidence->observed_utc_ms)
        return Status::invalid_input;
    auto gap = binding;
    gap.observation = evidence->observation;
    run(
        writer, status,
        [&](const auto &session) {
            const auto current = authorize(owner, Action::gap, binding.scope);
            if (current != Status::committed)
                return current;
            if (!single_profile(profile, owner, selected, binding.cursor_contract))
                return Status::stale;
            return outcome(stage_gap(session, gap));
        },
        true);
    return status;
}

namespace {
IntakeStatus open_operation(SqliteWriter &writer, const OwnerInputs &owner,
                            const HistoryProfile &profile, const AdmittedHistoryCut &selected,
                            const HistoryRequest *candidate, const CutObservation &echo,
                            const CutWitness &witness, Action action) {
    const auto &source = selected.request.source;
    const auto &intent = selected.intent;
    const bool importing = action == Action::import;
    auto status = authorize(owner, action, source.scope);
    if (status != Status::committed)
        return status;
    if (!single_profile(profile, owner, source, selected.cursor_contract))
        return Status::stale;
    if (intent.scope != source.scope || intent.epoch != source.fence.epoch ||
        intent.generation != source.fence.generation ||
        intent.policy_revision != source.fence.policy_revision ||
        intent.source_revision != source.fence.revision ||
        intent.purpose != (importing ? HistoryPurpose::import : HistoryPurpose::catch_up) ||
        selected.request.purpose != (importing ? CutPurpose::import_upper : CutPurpose::gap_upper))
        return Status::invalid_input;
    if ((importing && selected.predecessor != source.fence) ||
        (!importing && (selected.predecessor.epoch != source.fence.epoch ||
                        selected.predecessor.policy_revision != source.fence.policy_revision ||
                        selected.predecessor.revision >= source.fence.revision ||
                        source.fence.revision - 1 != selected.predecessor.revision || !intent.gap)))
        return Status::invalid_input;
    const HistoryRequest request{source,
                                 intent.operation_ref,
                                 1,
                                 intent.initial_continuation,
                                 selected.request.prior_frontier,
                                 intent.interval,
                                 intent.upper_cut,
                                 importing ? std::optional{intent.resolved_utc_start}
                                           : std::nullopt};
    const auto check_operation_head = [&](const SqliteWorkSession &session) {
        const auto head = read_head(session, source.scope);
        if (!head || head->fence != source.fence || head->cursor != selected.cursor_contract)
            return Status::stale;
        if (importing && (head->frontier.isNull() != !request.frontier ||
                          head->frontier != blob(request.frontier)))
            return Status::stale;
        return Status::committed;
    };
    bool retained = false;
    std::optional<qint64> start;
    run(writer, status, [&](const auto &session) {
        const auto current = check_operation_head(session);
        if (current != Status::committed)
            return current;
        if (!importing) {
            auto args = scope_bindings(source.scope);
            args.append({source.fence.epoch, *intent.gap, source.fence.policy_revision});
            const auto rows = session.execute(
                {"SELECT upper_cut,start_frontier FROM main.subathon_intake_gaps WHERE "
                 "installation=? "
                 "AND provider=? AND account=? AND stream=? AND epoch=? AND gap_ref=? "
                 "AND policy_revision=? AND status IN (0,1)",
                 args});
            if (rows.size() != 1 ||
                rows.front()[0].toByteArray() != blob(intent.upper_cut.position) ||
                rows.front()[1].toByteArray() != blob(selected.request.prior_frontier))
                return Status::stale;
        }
        if (importing) {
            const auto rows = read_operation(session, request);
            if (!rows.isEmpty()) {
                const auto access = authorize(owner, Action::inspect, source.scope);
                if (access != Status::committed)
                    return access;
                if (rows.front()[2].toInt() != static_cast<int>(HistoryPurpose::import))
                    return Status::conflict;
                retained = true;
                start = retained_import_start(rows.front()[1].toByteArray());
                if (!start)
                    return Status::unavailable;
            }
        }
        return Status::committed;
    });
    if (status != Status::committed)
        return status;
    if (candidate && !same(request, *candidate))
        return Status::invalid_input;
    if (importing) {
        if (!retained)
            start = local_start(owner, intent);
        if (!start)
            return Status::invalid_import_date;
        if (*start != intent.resolved_utc_start)
            return Status::invalid_input;
    }
    if (!same(intent.upper_cut, echo.cut))
        return Status::invalid_input;
    status = qualify_cut(profile, selected.request, echo, intent.interval.end, witness);
    if (status != Status::committed)
        return status;
    run(
        writer, status,
        [&](const auto &session) {
            const auto current = authorize(owner, action, source.scope);
            if (current != Status::committed)
                return current;
            if (!single_profile(profile, owner, source, selected.cursor_contract))
                return Status::stale;
            if (const auto head_status = check_operation_head(session);
                head_status != Status::committed)
                return head_status;
            return outcome(stage_operation(session, intent));
        },
        true);
    return status;
}
} // namespace

IntakeStatus begin_history_catch_up(SqliteWriter &writer, const OwnerInputs &owner,
                                    const HistoryProfile &profile,
                                    const AdmittedHistoryCut &selected, const CutObservation &echo,
                                    const CutWitness &witness) {
    return open_operation(writer, owner, profile, selected, nullptr, echo, witness,
                          Action::catch_up);
}

IntakeStatus open_history_import(SqliteWriter &writer, const OwnerInputs &owner,
                                 const HistoryProfile &profile, const AdmittedHistoryCut &selected,
                                 const HistoryRequest &candidate, const CutObservation &echo,
                                 const CutWitness &witness) {
    return open_operation(writer, owner, profile, selected, &candidate, echo, witness,
                          Action::import);
}

HandoffOutcome commit_history_handoff(SqliteWriter &writer, const OwnerInputs &owner,
                                      const QList<const HistoryProfile *> &profiles,
                                      const AdmittedHandoff &admitted,
                                      const QList<CutObservation> &echoes,
                                      const HandoffValidators &validators,
                                      const std::function<bool()> &before_commit) {
    auto change = admitted.change;
    const auto authorize_all = [&]() {
        if (const auto primary = authorize(owner, Action::baseline, change.scope);
            primary != Status::committed)
            return primary;
        for (const auto &side : change.affected_sources)
            if (const auto access = authorize(owner, Action::baseline, side.scope);
                access != Status::committed)
                return access;
        for (const auto &member : change.members)
            if (const auto access = authorize(owner, Action::baseline, member.scope);
                access != Status::committed)
                return access;
        return Status::committed;
    };
    auto status = authorize_all();
    if (status != Status::committed)
        return {status, {}};
    const bool legacy = change.purpose == SourcePurpose::adoption;
    const auto sides = legacy ? 1 : 2;
    if ((!legacy && change.purpose != SourcePurpose::handoff) || profiles.size() != sides ||
        admitted.selected.size() != sides || admitted.prior_cuts.size() != sides ||
        echoes.size() != sides || change.affected_sources.size() != sides - 1)
        return {Status::invalid_input, {}};

    const auto check = [&](const SqliteWorkSession &session) {
        const auto current = authorize_all();
        if (current != Status::committed)
            return current;
        const auto policies = session.execute(
            {"SELECT revision FROM main.subathon_intake_policies WHERE installation=? "
             "ORDER BY revision DESC LIMIT 1",
             {change.scope.installation}});
        if (policies.size() != 1 ||
            policies.front().front().toLongLong() != change.expected_installation_policy_revision)
            return Status::stale;
        for (qsizetype i = 0; i < sides; ++i) {
            const auto &scope = i == 0 ? change.scope : change.affected_sources[i - 1].scope;
            const auto &expected =
                i == 0 ? change.expected : change.affected_sources[i - 1].expected;
            const auto &generation =
                i == 0 ? change.new_generation : change.affected_sources[i - 1].new_generation;
            const auto &cursor = i == 0 ? change.qualified_capability
                                        : change.affected_sources[i - 1].qualified_capability;
            const auto &request = admitted.selected[i];
            if (request.source.scope != scope || request.source.fence.generation != generation ||
                request.purpose !=
                    (legacy ? CutPurpose::legacy_adoption : CutPurpose::new_only_handoff))
                return Status::invalid_input;
            const auto head = read_head(session, scope);
            if (!head || head->fence != expected || !profiles[i] ||
                !current_profile(*profiles[i], request.source, cursor))
                return Status::stale;
            const auto rows = session.execute(
                {"SELECT cut,frontier FROM main.subathon_intake_sources WHERE installation=? "
                 "AND provider=? AND account=? AND stream=?",
                 scope_bindings(scope)});
            if (rows.size() != 1 || rows.front()[0].toByteArray() != admitted.prior_cuts[i] ||
                rows.front()[1].isNull() != !request.prior_frontier ||
                rows.front()[1].toByteArray() != blob(request.prior_frontier))
                return Status::stale;
            if (expected.epoch == std::numeric_limits<qint64>::max() ||
                expected.revision == std::numeric_limits<qint64>::max() ||
                change.expected_installation_policy_revision == std::numeric_limits<qint64>::max())
                return Status::limit_exceeded;
            if (request.source.fence != Fence{expected.epoch + 1, expected.revision + 1,
                                              change.expected_installation_policy_revision + 1,
                                              generation})
                return Status::invalid_input;
        }
        return Status::committed;
    };
    run(writer, status, check);
    if (status != Status::committed)
        return {status, {}};
    if (sides == 2 && (&profiles[0]->snapshot->history == &profiles[1]->snapshot->history ||
                       admitted.selected[0].source.scope == admitted.selected[1].source.scope))
        return {Status::invalid_input, {}};
    QList<ObservedCut> observed;
    for (qsizetype i = 0; i < sides; ++i) {
        auto &adapter = profiles[i]->snapshot->history;
        const auto invocation = next_invocation(profiles[i]->cut_invocations);
        if (!invocation)
            return {Status::unavailable, {}};
        auto actual = adapter.observe_cut(admitted.selected[i]);
        if (const auto *failure = std::get_if<HistoryFailure>(&actual))
            return {outcome(*failure), {}};
        auto &cut = std::get<CutObservation>(actual);
        status = verify_cut(admitted.selected[i], echoes[i], cut, Endpoint::inclusive);
        if (status != Status::committed)
            return {status, {}};
        observed.append(ObservedCut{*invocation, &adapter, std::move(cut)});
    }
    const auto same_observation = [](const ObservedCut &a, const ObservedCut &b) {
        return a.invocation == b.invocation && a.adapter == b.adapter &&
               same(a.observation, b.observation);
    };
    const auto old = validators.old_side ? validators.old_side(observed.front()) : std::nullopt;
    if (!old || !same_observation(old->source, observed.front()) ||
        !old->includes_pending_and_outstanding || old->through != change.expected ||
        !old->attempt || old->eligible_upper != observed.front().observation.cut.position)
        return {Status::unproved, {}};
    std::optional<NewSide> new_side;
    if (sides == 2) {
        new_side = validators.new_side ? validators.new_side(observed[1]) : std::nullopt;
        if (!new_side || !same_observation(new_side->source, observed[1]) ||
            new_side->attempt != old->attempt || new_side->exclusive_lower != old->eligible_upper ||
            new_side->first.isEmpty())
            return {Status::unproved, {}};
    } else if (!old->legacy_eligible_first || old->legacy_eligible_first->isEmpty()) {
        return {Status::unproved, {}};
    }
    const auto proof = validators.relation ? validators.relation(*old, new_side) : std::nullopt;
    if (!proof || proof->isEmpty())
        return {Status::unproved, {}};
    const auto cut = cut_document(observed.front().observation);
    if (cut != change.authorized_cut || *proof != change.boundary_proof)
        return {Status::invalid_input, {}};
    if (sides == 2) {
        auto &next = change.affected_sources.front();
        if (cut_document(observed[1].observation) != next.authorized_cut ||
            *proof != next.boundary_proof)
            return {Status::invalid_input, {}};
    }
    const auto work = run(
        writer, status,
        [&](const SqliteWorkSession &session) {
            const auto current = check(session);
            if (current != Status::committed)
                return current;
            const auto staged = outcome(stage_source_change(session, change));
            if (staged == Status::committed && before_commit && !before_commit())
                return Status::unavailable;
            return staged;
        },
        true);
    return {status, work};
}
} // namespace subathon::server::connector_intake
