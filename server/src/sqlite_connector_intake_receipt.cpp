// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"
#include "sqlite_receipt_read.hpp"

#include <QtEndian>

namespace subathon::server::connector_intake {
namespace {

using Status = ManagedStatus;
using detail::bounded;
using detail::document_limit;
using detail::item_limit;
using detail::reference_limit;
using detail::scope_bindings;

QByteArray blob(const std::string &value) {
    return QByteArray(value.data(), static_cast<qsizetype>(value.size()));
}

std::optional<QByteArrayView> take_field(QByteArrayView &document) {
    if (document.size() < 4)
        return {};
    const auto size = qFromBigEndian<quint32>(document.data());
    document = document.sliced(4);
    if (size > document.size())
        return {};
    const auto field = document.first(size);
    document = document.sliced(size);
    return field;
}

bool valid_ref(const std::string &value) {
    return !value.empty() && value.size() <= static_cast<size_t>(reference_limit);
}

bool scoped_receipt_namespace(const Scope &scope, const core::ReceiptIdentity &identity) {
    if (!valid_ref(identity.identifier_namespace) || !valid_ref(identity.opaque_id) ||
        identity.origin.provider != scope.provider.toStdString() ||
        identity.origin.source_account != scope.account.toStdString())
        return false;
    QByteArrayView remaining(identity.identifier_namespace.data(),
                             static_cast<qsizetype>(identity.identifier_namespace.size()));
    const auto stream = take_field(remaining);
    const auto role = take_field(remaining);
    const auto reference = take_field(remaining);
    return stream == QByteArrayView(scope.stream) && role && !role->isEmpty() && reference &&
           !reference->isEmpty() && remaining.isEmpty();
}

bool valid_activity(const core::ReceiptOrigin &origin,
                    const std::optional<VerifiedActivityLink> &activity) {
    return !activity ||
           (activity->identity.origin == origin &&
            valid_ref(activity->identity.identifier_namespace) &&
            valid_ref(activity->identity.opaque_id) && valid_ref(activity->verification_ref));
}

bool local_scope(const SqliteWorkSession &session, const ManagedInput &input) {
    return detail::valid_scope(input.scope) &&
           detail::local_installation(session, input.scope.installation) &&
           scoped_receipt_namespace(input.scope, input.receipt.identity);
}

bool same_fence(const Fence &actual, const Fence &expected) {
    return actual.epoch == expected.epoch && actual.revision == expected.revision &&
           actual.policy_revision == expected.policy_revision &&
           actual.generation == expected.generation;
}

constexpr qint64 verified_support_code = 0;
constexpr qint64 audit_only_code = 3;
constexpr qint64 confirmation_only_code = 4;

qint64 admission_code(IntakeAdmission admission) {
    switch (admission) {
    case IntakeAdmission::creation_eligible_verified_support:
        return verified_support_code;
    case IntakeAdmission::audit_only:
        return audit_only_code;
    case IntakeAdmission::confirmation_only:
        return confirmation_only_code;
    }
    throw std::logic_error("Unexpected intake admission");
}

std::optional<Fence> source(const SqliteWorkSession &session, const Scope &scope) {
    const auto rows = session.execute({"SELECT epoch,revision,policy_revision,generation "
                                       "FROM main.subathon_intake_sources WHERE installation=? "
                                       "AND provider=? AND account=? AND stream=?",
                                       scope_bindings(scope)});
    if (rows.isEmpty())
        return {};
    const auto &r = rows.front();
    return Fence{r[0].toLongLong(), r[1].toLongLong(), r[2].toLongLong(), r[3].toByteArray()};
}

struct Entry {
    std::optional<AttemptLocator> attempt;
    QByteArray configuration, context, input, capture, linkage_namespace, activity_id, verification;
    qint64 arrival;
    std::optional<PageSlot> history;
    std::optional<core::ContributionId> contribution;
    qint64 phase, disposition;
};

// Phase records progression. An identified intake keeps its original
// classification in disposition across receipt and evidence commits.
std::optional<IntakeAdmission> retained_admission(const Entry &entry) {
    if (!entry.attempt || entry.attempt->receipt.value == 0 || entry.attempt->ordinal <= 0 ||
        entry.phase < 1 || entry.phase > 3 || (entry.phase < 3 && entry.contribution))
        return {};
    switch (entry.disposition) {
    case verified_support_code:
        if (entry.phase == 3 && !entry.contribution)
            return {};
        return IntakeAdmission::creation_eligible_verified_support;
    case audit_only_code:
        if (entry.phase == 3)
            return {};
        return IntakeAdmission::audit_only;
    case confirmation_only_code:
        return IntakeAdmission::confirmation_only;
    default:
        return {};
    }
}

std::optional<Entry> entry(const SqliteWorkSession &session, const Scope &scope,
                           const QByteArray &key) {
    auto args = scope_bindings(scope);
    args.append(key);
    const auto rows = session.execute(
        {"SELECT "
         "receipt_id,attempt_ordinal,configuration_ref,modifier_context_ref,input,provenance,"
         "activity_namespace,activity_id,verification_ref,arrival_ms,operation_ref,page_ordinal,"
         "item_ordinal,contribution_id,CASE WHEN typeof(phase)='integer' THEN phase END,"
         "CASE WHEN typeof(disposition)='integer' THEN disposition END "
         "FROM main.subathon_intake_entries WHERE installation=? "
         "AND provider=? AND account=? AND stream=? AND intake_ref=?",
         args});
    if (rows.isEmpty())
        return {};
    const auto &r = rows.front();
    const auto attempt =
        r[0].isNull() ? std::optional<AttemptLocator>{}
                      : std::optional{AttemptLocator{{decode_receipt_uint64(r[0].toByteArray())},
                                                     r[1].toLongLong()}};
    const auto history =
        r[10].isNull()
            ? std::optional<PageSlot>{}
            : std::optional{PageSlot{r[10].toByteArray(), r[11].toLongLong(), r[12].toLongLong()}};
    const auto contribution =
        r[13].isNull()
            ? std::optional<core::ContributionId>{}
            : std::optional{core::ContributionId{decode_receipt_uint64(r[13].toByteArray())}};
    return Entry{attempt,
                 r[2].toByteArray(),
                 r[3].toByteArray(),
                 r[4].toByteArray(),
                 r[5].toByteArray(),
                 r[6].toByteArray(),
                 r[7].toByteArray(),
                 r[8].toByteArray(),
                 r[9].toLongLong(),
                 history,
                 contribution,
                 r[14].toLongLong(),
                 r[15].isNull() ? -1 : r[15].toLongLong()};
}

bool activity_matches(const Entry &row, const std::optional<VerifiedActivityLink> &activity) {
    if (!activity)
        return row.linkage_namespace.isNull() && row.activity_id.isNull() &&
               row.verification.isNull();
    return row.linkage_namespace == blob(activity->identity.identifier_namespace) &&
           row.activity_id == blob(activity->identity.opaque_id) &&
           row.verification == blob(activity->verification_ref);
}

bool role_mismatch(const QByteArray &stored, RetainedKind role) {
    if (stored.startsWith("SRX1") || stored.startsWith("SRI1"))
        return true;
    return role == RetainedKind::configuration ? stored.startsWith("SRA1")
                                               : stored.startsWith("SRC1");
}

std::optional<Status> context_failure(const SqliteWorkSession &session,
                                      const core::ReceiptArrival &arrival,
                                      bool exact_configuration) {
    const SqliteRetainedContextStore store(session);
    const auto configuration = store.find({RetainedKind::configuration, arrival.configuration_ref});
    const auto context = store.find({RetainedKind::arrival_context, arrival.modifier_context_ref});
    if (!configuration)
        return Status::different_configuration;
    if (!context)
        return Status::different_context;
    if (role_mismatch(configuration->document, RetainedKind::configuration) ||
        role_mismatch(context->document, RetainedKind::arrival_context))
        return Status::wrong_document_role;
    if (exact_configuration &&
        (!context->configuration || context->configuration->reference != arrival.configuration_ref))
        return Status::different_configuration;
    return {};
}

bool stage_retained(const SqliteWorkSession &session,
                    const std::optional<RetainedDocument> &document, RetainedKind role,
                    const std::string &reference) {
    if (!document)
        return true;
    if (document->key.kind != role || document->key.reference != reference ||
        role_mismatch(document->document, role))
        return false;
    const auto selected = SqliteRetainedContextStore(session).retain(*document);
    return selected == RetainDisposition::inserted ||
           selected == RetainDisposition::already_present;
}

QList<QVariant> activity_key(const Scope &scope, const VerifiedActivityLink &activity) {
    return {scope.provider, scope.account, blob(activity.identity.identifier_namespace),
            blob(activity.identity.opaque_id)};
}

struct ExactOriginal {
    core::ContributionId cid;
    core::ReceiptId receipt;
    bool valid;
};

std::optional<ExactOriginal> original(const SqliteWorkSession &session, const Scope &scope,
                                      const VerifiedActivityLink &activity) {
    const auto rows = session.execute(
        {"SELECT c.contribution_id,c.original_receipt_id,"
         "o.contribution_id=c.contribution_id AND o.kind=0 AND "
         "o.activity_provider=c.activity_provider "
         "AND o.activity_source_account=c.activity_source_account "
         "AND o.activity_namespace=c.activity_namespace AND o.activity_id=c.activity_id "
         "AND typeof(o.verification_ref)='blob' AND length(o.verification_ref) BETWEEN 1 AND 4096 "
         "AND r.provider=c.activity_provider AND r.source_account=c.activity_source_account "
         "FROM main.subathon_contributions c LEFT JOIN main.subathon_contribution_observations o "
         "ON o.receipt_id=c.original_receipt_id "
         "LEFT JOIN main.subathon_receipts r ON r.receipt_id=c.original_receipt_id "
         "WHERE c.activity_provider=? AND c.activity_source_account=? AND c.activity_namespace=? "
         "AND c.activity_id=?",
         activity_key(scope, activity)});
    if (rows.isEmpty())
        return {};
    const auto &r = rows.front();
    return ExactOriginal{{decode_receipt_uint64(r[0].toByteArray())},
                         {decode_receipt_uint64(r[1].toByteArray())},
                         r[2].toLongLong() == 1};
}

struct Basis {
    QByteArray stream, intake, capture;
    AttemptLocator attempt;
};

std::optional<Basis> basis(const SqliteWorkSession &session, const Scope &scope,
                           const VerifiedActivityLink &activity) {
    auto args = activity_key(scope, activity);
    args.prepend(scope.installation);
    const auto rows =
        session.execute({"SELECT stream,intake_ref,capture,receipt_id,attempt_ordinal FROM "
                         "main.subathon_intake_basis "
                         "WHERE installation=? AND provider=? AND account=? AND "
                         "linkage_namespace=? AND activity_id=?",
                         args});
    if (rows.isEmpty())
        return {};
    const auto &r = rows.front();
    return Basis{r[0].toByteArray(),
                 r[1].toByteArray(),
                 r[2].toByteArray(),
                 {{decode_receipt_uint64(r[3].toByteArray())}, r[4].toLongLong()}};
}

void stage_entry(const SqliteWorkSession &session, const ManagedInput &input, const Fence &fence,
                 core::ReceiptId receipt, qint64 ordinal, core::ReceiptDisposition disposition) {
    const auto encoded = encode_receipt_uint64(receipt.value);
    auto values = scope_bindings(input.scope);
    values.append({input.intake_ref, fence.epoch, fence.generation, fence.policy_revision});
    values.append({input.history ? QVariant{input.history->operation} : QVariant{},
                   input.history ? QVariant{input.history->page} : QVariant{},
                   input.history ? QVariant{input.history->item} : QVariant{}});
    const auto &arrival = input.receipt.arrival;
    values.append({input.normalized, input.capture,
                   static_cast<qint64>(arrival.server_timestamp_utc_ms),
                   blob(arrival.configuration_ref), blob(arrival.modifier_context_ref)});
    values.append({input.activity ? QVariant{blob(input.activity->identity.identifier_namespace)}
                                  : QVariant{},
                   input.activity ? QVariant{blob(input.activity->identity.opaque_id)} : QVariant{},
                   input.activity ? QVariant{blob(input.activity->verification_ref)} : QVariant{}});
    values.append({admission_code(input.admission), encoded, ordinal});
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_entries(installation,provider,account,stream,intake_ref,"
         "epoch,generation,policy_revision,operation_ref,page_ordinal,item_ordinal,input,"
         "provenance,"
         "arrival_ms,configuration_ref,modifier_context_ref,activity_namespace,activity_id,"
         "verification_ref,phase,disposition,receipt_id,attempt_ordinal) "
         "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,1,?,?,?)",
         values});
    detail::require_changed_one(session);

    if (!input.activity || input.admission != IntakeAdmission::creation_eligible_verified_support ||
        disposition != core::ReceiptDisposition::created)
        return;
    if (original(session, input.scope, *input.activity) ||
        basis(session, input.scope, *input.activity))
        return;
    auto args = activity_key(input.scope, *input.activity);
    args.prepend(input.scope.installation);
    args.append({input.scope.stream, input.intake_ref, encoded, ordinal, input.capture});
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_basis VALUES(?,?,?,?,?,?,?,?,?,?)", args});
    detail::require_changed_one(session);
}

} // namespace

ManagedResult read_attempt(const SqliteWorkSession &session, const ManagedInput &input,
                           AttemptLocator locator) {
    if (!local_scope(session, input) || !source(session, input.scope))
        return {Status::wrong_scope};
    if (!bounded(input.intake_ref, reference_limit) ||
        !valid_activity(input.receipt.identity.origin, input.activity))
        return {Status::intake_limit};
    const auto retained = entry(session, input.scope, input.intake_ref);
    if (retained && retained->attempt != locator)
        return {Status::different_ordinal};
    if (retained) {
        if (retained_admission(*retained) != input.admission)
            return {Status::conflict};
        const auto selected = inspect(session, {input.scope, input.intake_ref, false, true});
        if (selected.status == SelectionStatus::intake_limit)
            return {Status::intake_limit};
        if (selected.status != SelectionStatus::selected)
            return {Status::conflict};
    }
    const auto closure = read_receipt_observation(session, locator.receipt, locator.ordinal);
    if (!closure)
        return {Status::different_ordinal};
    if (closure->original.identity != input.receipt.identity)
        return {Status::wrong_scope};
    // A managed attempt must close through its own intake key. The legacy
    // fixed-ordinal reader applies only when there is no durable intake binding.
    if (!retained &&
        !session
             .execute({"SELECT 1 FROM main.subathon_intake_entries WHERE receipt_id=? AND "
                       "attempt_ordinal=?",
                       {encode_receipt_uint64(locator.receipt.value), locator.ordinal}})
             .isEmpty())
        return {Status::conflict};

    const auto &actual = closure->attempt;
    if (actual.observed_facts != input.receipt.facts)
        return {Status::different_facts};
    if (retained) {
        if (retained->phase == 3 && actual.disposition != core::ReceiptDisposition::created)
            return {Status::conflict};
        if (actual.arrival.configuration_ref != input.receipt.arrival.configuration_ref)
            return {Status::different_configuration};
        if (actual.arrival.modifier_context_ref != input.receipt.arrival.modifier_context_ref)
            return {Status::different_context};
        if (actual.arrival != input.receipt.arrival ||
            !activity_matches(*retained, input.activity) || retained->history != input.history ||
            retained->capture != input.capture)
            return {Status::conflict};
    }
    if (const auto failure = context_failure(session, actual.arrival, retained.has_value()))
        return {*failure};
    return {Status::exact_replay, locator, actual};
}

ManagedResult admit_managed(const SqliteWorkSession &session, const ManagedInput &input,
                            const std::function<bool()> &before_commit) {
    const auto refuse = [&](ManagedStatus status) {
        session.rollback();
        return ManagedResult{status};
    };
    if (!local_scope(session, input))
        return refuse(Status::wrong_scope);
    if (!bounded(input.intake_ref, reference_limit) || !bounded(input.normalized, item_limit) ||
        !bounded(input.capture, document_limit) ||
        !valid_activity(input.receipt.identity.origin, input.activity))
        return refuse(Status::intake_limit);
    const auto current = source(session, input.scope);
    if (!current)
        return refuse(Status::wrong_scope);
    if (const auto retained = entry(session, input.scope, input.intake_ref)) {
        if (!retained->attempt || retained->input != input.normalized)
            return refuse(Status::conflict);
        const auto selected = read_attempt(session, input, *retained->attempt);
        session.rollback();
        return selected;
    }
    if (!same_fence(*current, input.expected_source) ||
        !detail::qualified_page_slot(session, input, *current))
        return refuse(Status::stale);

    auto stopped = Status::known_rollback;
    const auto prepare = [&](const SqliteWorkSession &owner) {
        const auto &arrival = input.receipt.arrival;
        if (!stage_retained(owner, input.configuration, RetainedKind::configuration,
                            arrival.configuration_ref) ||
            !stage_retained(owner, input.context, RetainedKind::arrival_context,
                            arrival.modifier_context_ref)) {
            stopped = Status::conflict;
            return false;
        }
        if (const auto failure = context_failure(owner, arrival, true)) {
            stopped = *failure;
            return false;
        }
        return true;
    };
    const auto stage = [&](const SqliteWorkSession &owner, core::ReceiptId receipt, qint64 ordinal,
                           core::ReceiptDisposition disposition) {
        stage_entry(owner, input, *current, receipt, ordinal, disposition);
        return !before_commit || before_commit();
    };
    const auto written = write_managed_receipt(session, input.receipt, prepare, stage);
    switch (written.status) {
    case ManagedReceiptStatus::committed:
        return {Status::committed, AttemptLocator{*written.receipt, written.ordinal}};
    case ManagedReceiptStatus::ordinal_limit:
        return {Status::intake_limit};
    case ManagedReceiptStatus::rolled_back:
        return {stopped};
    }
    throw std::logic_error("Unexpected managed receipt result");
}

EvidenceSelection select_evidence(const SqliteWorkSession &session, const ManagedInput &input) {
    using Evidence = EvidenceStatus;
    if (!local_scope(session, input) || !input.activity ||
        !bounded(input.intake_ref, reference_limit) ||
        !valid_activity(input.receipt.identity.origin, input.activity))
        return {Evidence::inconsistent_original};
    const auto applied = entry(session, input.scope, input.intake_ref);
    if (!applied ||
        retained_admission(*applied) != IntakeAdmission::creation_eligible_verified_support ||
        !activity_matches(*applied, input.activity))
        return {Evidence::inconsistent_original};
    const auto selected = read_attempt(session, input, *applied->attempt);
    if (selected.status != Status::exact_replay ||
        selected.selected->disposition != core::ReceiptDisposition::created)
        return {Evidence::inconsistent_original};

    const auto verified = original(session, input.scope, *input.activity);
    const auto bound = basis(session, input.scope, *input.activity);
    if (verified && (!verified->valid || (bound && verified->receipt != bound->attempt.receipt) ||
                     (applied->contribution && applied->contribution != verified->cid)))
        return {Evidence::inconsistent_original};
    if (!verified && applied->contribution)
        return {Evidence::inconsistent_original};
    if (!bound)
        return verified ? EvidenceSelection{Evidence::may_link, {}, verified->cid}
                        : EvidenceSelection{Evidence::inconsistent_original};

    auto scope = input.scope;
    scope.stream = bound->stream;
    const auto witness = entry(session, scope, bound->intake);
    if (!witness ||
        retained_admission(*witness) != IntakeAdmission::creation_eligible_verified_support ||
        witness->attempt != bound->attempt || witness->capture != bound->capture ||
        witness->activity_id != blob(input.activity->identity.opaque_id) ||
        witness->linkage_namespace != blob(input.activity->identity.identifier_namespace) ||
        !bounded(witness->verification, reference_limit) ||
        (witness->contribution && (!verified || witness->contribution != verified->cid)))
        return {Evidence::inconsistent_original};
    const auto bound_closure =
        read_receipt_observation(session, bound->attempt.receipt, bound->attempt.ordinal);
    if (!bound_closure || !scoped_receipt_namespace(scope, bound_closure->original.identity) ||
        bound_closure->attempt.disposition != core::ReceiptDisposition::created ||
        blob(bound_closure->attempt.arrival.configuration_ref) != witness->configuration ||
        blob(bound_closure->attempt.arrival.modifier_context_ref) != witness->context ||
        bound_closure->attempt.arrival.server_timestamp_utc_ms != witness->arrival ||
        context_failure(session, bound_closure->attempt.arrival, true))
        return {Evidence::inconsistent_original};
    if (verified)
        return {Evidence::may_link, bound->attempt, verified->cid, bound->capture};
    const bool its_basis = scope == input.scope && bound->intake == input.intake_ref &&
                           applied->attempt == bound->attempt;
    return {its_basis ? Evidence::may_create : Evidence::pending_original,
            bound->attempt,
            {},
            bound->capture};
}

HistoryStatus stage_intake_disposition(const SqliteWorkSession &session, const ManagedInput &input,
                                       AttemptLocator locator, IntakeDisposition disposition) {
    using H = HistoryStatus;
    if (!local_scope(session, input))
        return H::wrong_scope;
    if (!bounded(input.intake_ref, reference_limit))
        return H::intake_limit;
    const auto retained = entry(session, input.scope, input.intake_ref);
    if (!retained || retained->attempt != locator)
        return H::stale;
    const auto admission = retained_admission(*retained);
    if (!admission || *admission != input.admission)
        return H::conflict;
    const auto selected = read_attempt(session, input, locator);
    if (selected.status != Status::exact_replay)
        return selected.status == Status::intake_limit ? H::intake_limit : H::conflict;

    std::optional<core::ContributionId> recorded_contribution;
    switch (disposition) {
    case IntakeDisposition::observed_only:
        if (selected.selected->disposition == core::ReceiptDisposition::created &&
            *admission != IntakeAdmission::audit_only)
            return H::conflict;
        break;
    case IntakeDisposition::waiting_original:
        if (select_evidence(session, input).status != EvidenceStatus::pending_original)
            return H::conflict;
        break;
    case IntakeDisposition::evidence_committed: {
        if (!input.activity || *admission == IntakeAdmission::audit_only ||
            selected.selected->disposition != core::ReceiptDisposition::created)
            return H::conflict;
        const auto &activity = *input.activity;
        const auto expected_kind = *admission == IntakeAdmission::confirmation_only ? 1 : 0;
        auto args = activity_key(input.scope, activity);
        args.prepend(encode_receipt_uint64(locator.receipt.value));
        args.append({blob(activity.verification_ref), expected_kind});
        const auto rows = session.execute(
            {"SELECT contribution_id FROM main.subathon_contribution_observations "
             "WHERE receipt_id=? AND activity_provider=? AND activity_source_account=? "
             "AND activity_namespace=? AND activity_id=? AND verification_ref=? AND kind=?",
             args});
        if (rows.size() != 1)
            return H::stale;
        if (!rows.front().front().isNull()) {
            const auto actual = original(session, input.scope, activity);
            const auto cid =
                core::ContributionId{decode_receipt_uint64(rows.front().front().toByteArray())};
            if (!actual || !actual->valid || actual->cid != cid ||
                (expected_kind == 0 && select_evidence(session, input).original != cid))
                return H::conflict;
            recorded_contribution = cid;
        } else if (expected_kind != 1) {
            return H::conflict;
        }
        break;
    }
    }

    const qint64 next_phase = disposition == IntakeDisposition::evidence_committed ? 3 : 2;
    if (retained->phase >= next_phase) {
        if (retained->phase == next_phase && retained->contribution == recorded_contribution)
            return H::exact_replay;
        return H::conflict;
    }
    const bool advance_prefix = input.history && retained->phase == 1;
    if (advance_prefix) {
        const auto current = source(session, input.scope);
        if (!current || !same_fence(*current, input.expected_source) ||
            !detail::qualified_page_slot(session, input, *current))
            return H::stale;
    }

    QList<QVariant> values{next_phase,
                           recorded_contribution
                               ? QVariant{encode_receipt_uint64(recorded_contribution->value)}
                               : QVariant{}};
    values.append(scope_bindings(input.scope));
    values.append({input.intake_ref, encode_receipt_uint64(locator.receipt.value), locator.ordinal,
                   retained->phase, retained->disposition});
    (void)session.execute(
        {"UPDATE main.subathon_intake_entries SET phase=?,contribution_id=? WHERE installation=? "
         "AND provider=? AND account=? AND stream=? AND intake_ref=? AND receipt_id=? "
         "AND attempt_ordinal=? AND phase=? AND disposition=?",
         values});
    detail::require_changed_one(session);
    if (advance_prefix)
        detail::advance_page_prefix(session, input.scope, *input.history);
    return H::staged;
}

} // namespace subathon::server::connector_intake
