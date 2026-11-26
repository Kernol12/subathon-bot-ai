// SPDX-License-Identifier: GPL-3.0-or-later
#include "intake_recovery_codec.hpp"
#include "intake_recovery_writer_access.hpp"
#include "sqlite_connector_intake_private.hpp"
#include "sqlite_receipt_read.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace subathon::server::connector_intake::recovery {
namespace {

using Access = subathon::server::detail::IntakeRecoveryWriterAccess;
constexpr Reservation packet{26, 172528, 26, 6771084};

std::array<qsizetype, 4> charges(auto cost) {
    return {cost.statements, cost.bound_bytes, cost.result_rows, cost.result_bytes};
}

bool fits(auto available, Reservation base, Reservation call, qsizetype count) {
    const auto capacity = charges(available);
    const auto fixed = charges(base);
    const auto reserved = charges(call);
    for (std::size_t i = 0; i < capacity.size(); ++i)
        if (capacity[i] < fixed[i] || reserved[i] < 0 ||
            reserved[i] > (capacity[i] - fixed[i]) / count)
            return false;
    return true;
}

bool qualified_profile(const StorageLimits &limits, Reservation call) {
    return limits.statement_utf8_bytes >= 65536 &&
           fits(Reservation{limits.statements_per_batch, limits.bound_value_bytes,
                            limits.result_rows, limits.result_value_bytes},
                packet, call, 2);
}

bool bounded_ref(const QByteArray &reference) {
    return !reference.isEmpty() && reference.size() <= 4096;
}

struct Refusal {
    Status status;
};

[[noreturn]] void inconsistent() { throw Refusal{Status::inconsistent}; }

void authorize(const SqliteWorkSession &session, StartupAuthority &authority,
               const StartupBinding &binding, bool fresh, Reservation call, const Scope &scope) {
    const auto before = Access::remaining(session);
    if (!fits(before, {}, call, 1))
        throw Refusal{Status::unavailable};
    const auto decision =
        authority.resolve(session, {StartupAction::inspect, binding, scope, fresh});
    const auto initial = charges(before);
    const auto current = charges(Access::remaining(session));
    const auto allowance = charges(call);
    for (std::size_t i = 0; i < initial.size(); ++i)
        if (initial[i] - current[i] > allowance[i])
            throw Refusal{Status::unavailable};

    if (std::holds_alternative<StartupDenied>(decision))
        throw Refusal{Status::denied};
    const auto *permit = std::get_if<StartupPermit>(&decision);
    if (!permit || permit->action != StartupAction::inspect || permit->scope != scope ||
        !bounded_ref(permit->principal) || !bounded_ref(permit->account_snapshot) ||
        !bounded_ref(permit->permission_snapshot) || !bounded_ref(permit->actor) ||
        !bounded_ref(permit->decision))
        throw Refusal{Status::unavailable};
}

// Query specs take only owner-specified expressions and predicates. Variable
// keys always go through bindings. Metadata never returns document contents.
struct Projection {
    QStringList fields;
    QString from;
    QList<QVariant> bindings;

    SqlRow read(const SqliteWorkSession &session, const QStringList &select) const {
        const auto rows = session.execute({"SELECT " + select.join(',') + from, bindings});
        if (rows.size() > 1)
            inconsistent();
        return rows.isEmpty() ? SqlRow{} : rows.front();
    }
    QStringList metadata_fields(const QString &route = {}) const {
        QStringList expressions;
        for (const auto &field : fields)
            expressions.append({"typeof(" + field + ")", "length(" + field + ")"});
        if (!route.isEmpty())
            expressions.append(route);
        return expressions;
    }
    SqlRow metadata(const SqliteWorkSession &session, const QString &route = {}) const {
        return read(session, metadata_fields(route));
    }
    qsizetype metadata_width() const {
        // The generated clauses are ASCII and the metadata SELECT also bounds payload width.
        return 7 + metadata_fields().join(',').size() + from.size();
    }
    void validate_sizes(const SqlRow &row, std::span<const qsizetype> caps) const {
        for (qsizetype i = 0; i < fields.size(); ++i) {
            const auto type = row[2 * i].toString();
            const auto cap = caps[size_t(i)];
            if (type != "null" && type != (cap == 0 ? "integer" : "blob"))
                inconsistent();
            if (type == "blob" && row[2 * i + 1].toLongLong() > cap)
                throw Refusal{Status::resource_limit};
        }
    }
    SqlRow payload(const SqliteWorkSession &session) const {
        auto row = read(session, fields);
        if (row.isEmpty())
            inconsistent();
        return row;
    }
    SqlRow receipt_payload(const SqliteWorkSession &session,
                           std::span<const qsizetype> caps) const {
        const auto sizes = metadata(session);
        if (sizes.isEmpty())
            inconsistent();
        validate_sizes(sizes, caps);
        const auto values = payload(session);
        SqlRow combined;
        for (qsizetype i = 0; i < fields.size(); ++i)
            combined.append({values[i], sizes[2 * i], sizes[2 * i + 1]});
        return combined;
    }
};

AttemptLocator locator(const QVariant &id, const QVariant &ordinal) {
    const auto bytes = id.toByteArray();
    if (bytes.size() != 8 || ordinal.toLongLong() <= 0)
        inconsistent();
    const auto value = decode_receipt_uint64(bytes);
    if (value == 0)
        inconsistent();
    return {{value}, ordinal.toLongLong()};
}

Projection retained_document_query(bool config, const QByteArray &reference) {
    return {config ? QStringList{"reference", "document"}
                   : QStringList{"reference", "document", "configuration_ref"},
            config ? " FROM main.subathon_retained_configurations WHERE reference=?"
                   : " FROM main.subathon_retained_contexts WHERE reference=?",
            {reference}};
}

RetainedDocument document(const SqliteWorkSession &session, RetainedKind kind,
                          const QByteArray &reference, qsizetype limit) {
    if (!bounded_ref(reference))
        inconsistent();
    const bool config = kind == RetainedKind::configuration;
    const auto projection = retained_document_query(config, reference);
    const std::array<qsizetype, 3> caps{4096, limit, 4096};
    const auto metadata = projection.metadata(session);
    if (metadata.isEmpty() || metadata[0].toString() != "blob" ||
        metadata[2].toString() != "blob" || (!config && metadata[4].toString() != "blob"))
        inconsistent();
    projection.validate_sizes(metadata, caps);
    const auto row = projection.payload(session);
    if (row[0].toByteArray() != reference || (!config && !bounded_ref(row[2].toByteArray())))
        inconsistent();
    return {{kind, reference.toStdString()},
            metadata[3].toLongLong() == 0 ? QByteArray("", 0) : row[1].toByteArray(),
            config ? std::optional<ConfigurationReference>{}
                   : ConfigurationReference{row[2].toByteArray().toStdString()}};
}

constexpr qsizetype value = 262144;
constexpr std::array<qsizetype, 20> receipt_caps{8,    4096,  4096,  4096,  4096,  value, 0,
                                                 0,    value, value, value, value, 4096,  4096,
                                                 4096, 4096,  0,     4096,  4096,  4096};
constexpr std::array<qsizetype, 18> attempt_caps{
    8, 0, value, 0, 0, value, value, value, value, 4096, 4096, 4096, 4096, 0, 4096, 4096, 4096, 0};

Projection original_receipt_query(const QByteArray &id) {
    return {receipt_observation_columns(false),
            " FROM main.subathon_receipts r WHERE receipt_id=? "
            "AND EXISTS (SELECT 1 FROM main.subathon_retained_configurations c "
            "WHERE c.reference=r.configuration_ref) "
            "AND EXISTS (SELECT 1 FROM main.subathon_retained_contexts m "
            "WHERE m.reference=r.modifier_context_ref)",
            {id}};
}

Projection receipt_attempt_query(const QByteArray &id, qint64 ordinal) {
    return {receipt_observation_columns(true),
            " FROM main.subathon_receipt_attempts WHERE receipt_id=? AND ordinal=?",
            {id, ordinal}};
}

ReceiptObservation receipt(const SqliteWorkSession &session, AttemptLocator key) {
    const auto id = encode_receipt_uint64(key.receipt.value);
    const auto original = original_receipt_query(id);
    const auto attempt = receipt_attempt_query(id, key.ordinal);
    const auto original_row = original.receipt_payload(session, receipt_caps);
    const auto attempt_row = attempt.receipt_payload(session, attempt_caps);
    try {
        return decode_receipt_observation(key.receipt, key.ordinal, original_row, attempt_row);
    } catch (const SqliteReceiptStorageFailure &failure) {
        if (failure.reason() == SqliteReceiptStorageReason::invalid_stored_data)
            inconsistent();
        if (failure.reason() == SqliteReceiptStorageReason::result_limit)
            throw Refusal{Status::resource_limit};
        throw;
    }
}

IntakeAdmission lane(qint64 phase, qint64 disposition, bool contribution) {
    if (phase < 1 || phase > 3 || (phase < 3 && contribution))
        inconsistent();
    if (disposition == 0 && (phase != 3 || contribution))
        return IntakeAdmission::creation_eligible_verified_support;
    if (disposition == 3 && phase != 3)
        return IntakeAdmission::audit_only;
    if (disposition == 4)
        return IntakeAdmission::confirmation_only;
    inconsistent();
}

Projection admitted_entry_query(const Scope &scope, const QByteArray &key) {
    auto bindings = connector_intake::detail::scope_bindings(scope);
    bindings.append(key);
    // The operation join gives the admitted route from the same entry read.
    return {{"o.purpose", "e.generation", "e.policy_revision", "e.receipt_id", "e.attempt_ordinal",
             "e.configuration_ref", "e.modifier_context_ref", "e.input", "e.provenance",
             "e.activity_namespace", "e.activity_id", "e.verification_ref", "e.arrival_ms",
             "e.operation_ref", "e.page_ordinal", "e.item_ordinal", "e.contribution_id", "e.phase",
             "e.disposition"},
            " FROM main.subathon_intake_entries e "
            "LEFT JOIN main.subathon_intake_operations o "
            "ON (o.installation,o.operation_ref,o.provider,o.account,o.stream)="
            "(e.installation,e.operation_ref,e.provider,e.account,e.stream) "
            "WHERE e.installation=? AND e.provider=? AND e.account=? "
            "AND e.stream=? AND e.intake_ref=?",
            bindings};
}

Closure read_closure(const SqliteWorkSession &session, const Scope &scope, const QByteArray &key,
                     std::optional<AttemptLocator> expected, bool first_basis) {
    const auto entry = admitted_entry_query(scope, key);
    const std::array<qsizetype, 19> caps{
        0,    4096, 0,    8, 0, 4096, 4096, 32768, detail::capture_limit, 4096, 4096,
        4096, 0,    4096, 0, 0, 8,    0,    0};
    const auto metadata = entry.metadata(session);
    if (metadata.isEmpty())
        throw Refusal{first_basis ? Status::inconsistent : Status::missing};
    if (metadata[6].toString() == "null" && metadata[8].toString() == "null")
        throw Refusal{first_basis ? Status::inconsistent : Status::not_identified};
    entry.validate_sizes(metadata, caps);
    const auto row = entry.payload(session);
    const auto selected = locator(row[3], row[4]);
    if (expected && selected != *expected)
        throw Refusal{first_basis ? Status::inconsistent : Status::locator_mismatch};
    if (row[7].toByteArray().isEmpty() || row[12].isNull() || row[17].isNull() || row[18].isNull())
        inconsistent();
    const auto capture = row[8].toByteArray();
    if (capture == "ARC1")
        inconsistent(); // A current magic without its version is malformed framing.
    if (!detail::current_capture(capture))
        throw Refusal{Status::unsupported_legacy};
    const auto reference = detail::detail_reference(capture);
    if (!reference || *reference == row[6].toByteArray())
        inconsistent();

    auto observed = receipt(session, selected);
    auto &attempt = observed.attempt;
    const auto &original = observed.original;
    if (!detail::matches_scope(scope, original.identity) ||
        QByteArray::fromStdString(attempt.arrival.configuration_ref) != row[5].toByteArray() ||
        QByteArray::fromStdString(attempt.arrival.modifier_context_ref) != row[6].toByteArray() ||
        attempt.arrival.server_timestamp_utc_ms != row[12].toLongLong() ||
        (attempt.disposition == core::ReceiptDisposition::created &&
         (attempt.arrival != original.original_arrival ||
          attempt.observed_facts != original.original_facts)) ||
        (row[17].toLongLong() == 3 && attempt.disposition != core::ReceiptDisposition::created))
        inconsistent();

    const auto admission = lane(row[17].toLongLong(), row[18].toLongLong(), !row[16].isNull());
    std::optional<VerifiedActivityLink> activity;
    if (!row[9].isNull() || !row[10].isNull() || !row[11].isNull()) {
        if (!bounded_ref(row[9].toByteArray()) || !bounded_ref(row[10].toByteArray()) ||
            !bounded_ref(row[11].toByteArray()))
            inconsistent();
        activity = {{original.identity.origin, row[9].toByteArray().toStdString(),
                     row[10].toByteArray().toStdString()},
                    row[11].toByteArray().toStdString()};
    }
    std::optional<PageSlot> history;
    std::optional<HistoryPurpose> purpose;
    if (!row[13].isNull()) {
        const auto kind = row[0].toLongLong();
        if (row[0].isNull() || (kind != 0 && kind != 1) || !bounded_ref(row[13].toByteArray()) ||
            row[14].toLongLong() <= 0 || row[15].toLongLong() <= 0 || row[15].toLongLong() > 16)
            inconsistent();
        history = {row[13].toByteArray(), row[14].toLongLong(), row[15].toLongLong()};
        purpose = static_cast<HistoryPurpose>(kind);
    }
    const auto stored_detail =
        document(session, RetainedKind::arrival_context, *reference, detail::detail_limit);
    Closure closure{scope,
                    key,
                    selected,
                    original.identity,
                    std::move(attempt),
                    history,
                    activity,
                    admission,
                    row[7].toByteArray(),
                    capture,
                    document(session, RetainedKind::configuration, row[5].toByteArray(), value),
                    document(session, RetainedKind::arrival_context, row[6].toByteArray(), value),
                    stored_detail};
    if (!detail::matches_detail(closure, purpose))
        inconsistent();
    return closure;
}

} // namespace

Recovery detail::read_admitted_closure(const SqliteWorkSession &session, const Scope &scope,
                                       const QByteArray &key) {
    try {
        return {Status::recovered, read_closure(session, scope, key, {}, false), {}};
    } catch (const Refusal &error) {
        return {error.status, {}, {}};
    }
}

qsizetype detail::admitted_readback_statement_width() {
    return std::max({admitted_entry_query({}, {}).metadata_width(),
                     original_receipt_query({}).metadata_width(),
                     receipt_attempt_query({}, 0).metadata_width(),
                     retained_document_query(true, {}).metadata_width(),
                     retained_document_query(false, {}).metadata_width()});
}

bool qualified(const SqliteWriter &writer, Reservation call) {
    const auto profile = Access::limits(writer);
    return profile && qualified_profile(*profile, call);
}

Recovery recover_intake(const SqliteWorkSession &session, StartupAuthority &authority,
                        const StartupBinding &binding, bool fresh, Reservation call,
                        const Scope &scope, const QByteArray &intake_ref,
                        std::optional<AttemptLocator> expected) {
    try {
        if (!qualified_profile(Access::limits(session), call) ||
            !fits(Access::remaining(session), packet, call, 2))
            return {Status::unavailable, {}, {}};
        if (!bounded_ref(intake_ref) || !connector_intake::detail::valid_scope(scope))
            return {Status::inconsistent, {}, {}};
        authorize(session, authority, binding, fresh, call, scope);
        auto selected = read_closure(session, scope, intake_ref, expected, false);
        if (!selected.activity)
            return {Status::recovered, std::move(selected), {}};

        const auto &activity = selected.activity->identity;
        const Projection basis{
            {"stream", "intake_ref", "capture", "receipt_id", "attempt_ordinal"},
            " FROM main.subathon_intake_basis WHERE installation=? AND provider=? AND account=? "
            "AND linkage_namespace=? AND activity_id=?",
            {scope.installation, scope.provider, scope.account,
             QByteArray::fromStdString(activity.identifier_namespace),
             QByteArray::fromStdString(activity.opaque_id)}};
        const std::array<qsizetype, 5> caps{4096, 4096, detail::capture_limit, 8, 0};
        const auto metadata = basis.metadata(
            session, "CASE WHEN typeof(stream)='blob' AND length(stream) BETWEEN 1 AND 4096 "
                     "THEN stream END");
        if (metadata.isEmpty())
            return {Status::no_basis, std::move(selected), {}};
        auto original_scope = scope;
        original_scope.stream = metadata.back().toByteArray();
        if (!bounded_ref(original_scope.stream))
            inconsistent();
        authorize(session, authority, binding, fresh, call, original_scope);

        basis.validate_sizes(metadata, caps);
        const auto bound = basis.payload(session);
        const auto original_key = bound[1].toByteArray();
        if (bound[0].toByteArray() != original_scope.stream || !bounded_ref(original_key))
            inconsistent();
        const auto original_locator = locator(bound[3], bound[4]);
        const bool same_entry = scope == original_scope && intake_ref == original_key;
        auto original = same_entry ? selected
                                   : read_closure(session, original_scope, original_key,
                                                  original_locator, true);
        if (original.locator != original_locator || original.capture != bound[2].toByteArray() ||
            original.admission != IntakeAdmission::creation_eligible_verified_support ||
            original.attempt.disposition != core::ReceiptDisposition::created ||
            !original.activity || original.activity->identity != activity)
            inconsistent();
        return {Status::recovered, std::move(selected),
                same_entry ? std::optional<Closure>{} : std::move(original)};
    } catch (const Refusal &error) {
        return {error.status, {}, {}};
    } catch (const SqliteWorkFailure &failure) {
        if (failure.error() == WorkError::stale_session ||
            failure.error() == WorkError::session_closed ||
            failure.error() == WorkError::writer_unavailable)
            return {Status::unavailable, {}, {}};
        throw;
    }
}

} // namespace subathon::server::connector_intake::recovery
