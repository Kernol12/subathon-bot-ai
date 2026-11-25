// SPDX-License-Identifier: GPL-3.0-or-later
#include "contribution_review_bundle_history.hpp"
#include "sqlite_application_bundle_access.hpp"
#include "sqlite_clock_checkpoint.hpp"
#include <QtEndian>
namespace subathon::server {
namespace {

bool checkpoint_ref(std::string_view ref) {
    return !ref.empty() && ref.size() <= 4096;
}

bool take_checkpoint_ref(QByteArrayView &document, QByteArrayView &ref) {
    if (document.size() < 4)
        return false;
    const auto length = qFromBigEndian<quint32>(document.data());
    document = document.sliced(4);
    if (length == 0 || length > 4096 || length > document.size())
        return false;
    ref = document.first(length);
    document = document.sliced(length);
    return true;
}

bool valid_initial_slot(const detail::ClockInitialCheckpoint &checkpoint) {
    QByteArrayView document(checkpoint.slot_document);
    if (document.empty() || document.size() > 65536 || document.front() != 0)
        return false;
    document = document.sliced(1);
    for (const QByteArrayView expected : {QByteArrayView(checkpoint.installation),
                                         QByteArrayView(checkpoint.timer),
                                         QByteArrayView(checkpoint.lineage)}) {
        QByteArrayView ref;
        if (!take_checkpoint_ref(document, ref) || ref != expected)
            return false;
    }
    for (int i = 0; i < 4; ++i) {
        QByteArrayView ref;
        if (!take_checkpoint_ref(document, ref))
            return false;
    }
    // Form 0: present zero UTC, present zero elapsed, qualified, then the
    // frozen absent/disabled fields.
    static const auto suffix = QByteArray::fromHex(
        "0100000000000000000100000000000000000100000000000000000000");
    return document == QByteArrayView(suffix);
}

template <typename Integer> void append_big_endian(QByteArray &out, Integer value) {
    char encoded[sizeof(Integer)];
    qToBigEndian(value, encoded);
    out.append(encoded, sizeof(encoded));
}

QByteArray initial_root(const detail::ClockInitialCheckpoint &checkpoint,
                        const QByteArray &token) {
    const auto scope = bytes(checkpoint.operation.operation_scope);
    const auto operation = bytes(checkpoint.operation.operation_ref);
    QByteArray body(1, '\1');
    for (const QByteArrayView ref : {QByteArrayView(scope),
                                    QByteArrayView(operation),
                                    QByteArrayView(token), QByteArrayView(token)}) {
        append_big_endian(body, static_cast<quint32>(ref.size()));
        body.append(ref.data(), ref.size());
    }
    for (qint64 counter : {0, 0, 1, 1})
        append_big_endian(body, counter);
    QByteArray root(1, '\0');
    append_big_endian(root, static_cast<quint32>(body.size()));
    root.append(body);
    return root;
}

bool same_checkpoint_row(const SqlRows &selected, const SqlRow &expected) {
    if (selected.size() != 1 || selected.front().size() != expected.size())
        return false;
    for (qsizetype i = 0; i < expected.size(); ++i) {
        const auto &actual = selected.front()[i];
        const auto &want = expected[i];
        if (want.isNull()) {
            if (!actual.isNull())
                return false;
        } else if (want.metaType().id() == QMetaType::LongLong) {
            if (actual.isNull() || !detail::integer_value(actual) ||
                actual.toLongLong() != want.toLongLong())
                return false;
        } else if (actual.metaType() != want.metaType() || actual != want) {
            return false;
        }
    }
    return true;
}

} // namespace

std::variant<SqliteBundleInitialDisposition, WorkError>
detail::stage_clock_checkpoint_initial(const SqliteWorkSession &owner,
                                       const RetainedDocument &configuration,
                                       const ApplicationStateSnapshot &snapshot,
                                       const ClockInitialCheckpoint &checkpoint) {
    using Disposition = SqliteBundleInitialDisposition;
    const SqliteBundleLimits limits;
    try {
        bundle_detail::validate_initial(snapshot, limits);
    } catch (const SqliteBundleInputFailure &) {
        return WorkError::invalid_statement;
    }
    const auto token = bytes(snapshot.token);
    const auto config_ref = bytes(snapshot.configuration_ref);
    if (configuration.key.kind != RetainedKind::configuration ||
        configuration.key.reference != snapshot.configuration_ref || configuration.configuration ||
        configuration.document.isNull() || configuration.document.size() > limits.value_bytes ||
        !checkpoint_ref(checkpoint.operation.operation_scope) ||
        !checkpoint_ref(checkpoint.operation.operation_ref) || checkpoint.cadence_ms <= 0 ||
        checkpoint.ledger_revision < 0 || !valid_initial_slot(checkpoint) ||
        checkpoint.root_document.size() > 262144 ||
        initial_root(checkpoint, token) != checkpoint.root_document)
        return WorkError::invalid_statement;
    const auto scope = bytes(checkpoint.operation.operation_scope);
    const auto operation = bytes(checkpoint.operation.operation_ref);

    try {
        if (!CatalogWriterAccess::verified_v22(owner))
            return WorkError::invalid_statement;
        const QVariant null(QMetaType::fromType<qint64>());
        const SqlRow slot{qint64(1), token, config_ref, qint64(1), qint64(1), qint64(1),
                          qint64(0), null, null, qint64(snapshot.timer.main_remaining_ms),
                          qint64(snapshot.timer.grace_remaining_ms), checkpoint.cadence_ms,
                          checkpoint.ledger_revision, checkpoint.slot_document};
        const SqlRow root{qint64(1), qint64(0), qint64(0), checkpoint.installation,
                          checkpoint.timer, checkpoint.lineage, token, config_ref,
                          checkpoint.ledger_revision, scope, operation, null, null, null, null,
                          qint64(1), null, null, null, qint64(1), checkpoint.root_document};
        const SqlStatement select_slot{
            "SELECT singleton,base_token,configuration_ref,format_version,generation,revision,"
            "root_ordinal,pending_revision,pending_ordinal,main_ms,grace_ms,cadence_ms,"
            "ledger_revision,document FROM main.subathon_clock_checkpoint LIMIT 2", {}};
        const SqlStatement select_root{
            "SELECT revision,ordinal,kind,installation_ref,timer_ref,lineage_ref,base_token,"
            "configuration_ref,ledger_revision,operation_scope,operation_ref,episode_ref,"
            "schedule_id,nominal_ms,fold_run,member_count,link_revision,link_ordinal,page_no,"
            "format_version,document FROM main.subathon_clock_evidence "
            "WHERE (revision=1 AND ordinal=0) OR (operation_scope=? AND operation_ref=?) LIMIT 2",
            {scope, operation}};
        const SqliteRetainedContextStore retained(
            owner, {limits.reference_bytes, limits.value_bytes});
        Access access(owner, limits);
        return access.guarded([&] {
            const auto head = access.current();
            const auto current_slots = owner.execute(select_slot);
            const auto roots = owner.execute(select_root);
            const auto matches = [&](const auto &state, const auto &selected_slots,
                                     const auto &selected_roots) {
                return state && bundle_detail::same_state(*state, snapshot) &&
                       same_checkpoint_row(selected_slots, slot) &&
                       same_checkpoint_row(selected_roots, root) &&
                       retained.find(configuration.key) == configuration;
            };
            if (head) {
                if (current_slots.isEmpty())
                    return Disposition::conflict;
                const auto &occupied = current_slots.front();
                if (current_slots.size() != 1 || occupied.size() != slot.size() ||
                    occupied[1] != bytes(head->token) ||
                    occupied[2] != bytes(head->configuration_ref) ||
                    !integer_value(occupied[9]) ||
                    occupied[9].toLongLong() != head->timer.main_remaining_ms ||
                    !integer_value(occupied[10]) ||
                    occupied[10].toLongLong() != head->timer.grace_remaining_ms)
                    access.fail(SqliteBundleStorageReason::invalid_stored_data);
                return matches(head, current_slots, roots) ? Disposition::already_present
                                                           : Disposition::conflict;
            }
            if (!current_slots.isEmpty())
                access.fail(SqliteBundleStorageReason::invalid_stored_data);
            if (!roots.isEmpty())
                return Disposition::conflict;
            const auto retained_result = retained.retain(configuration);
            if (retained_result == RetainDisposition::conflict)
                return Disposition::conflict;
            if (retained_result == RetainDisposition::missing_reference)
                access.fail(SqliteBundleStorageReason::missing_reference);
            access.wrote_ = retained_result == RetainDisposition::inserted;

            std::vector<SqlStatement> writes;
            access.prepare_state(writes, snapshot);
            writes.push_back({"INSERT INTO main.subathon_clock_evidence "
                              "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", root});
            writes.push_back({"INSERT INTO main.subathon_clock_checkpoint "
                              "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)", slot});
            writes.push_back(insertion("subathon_app_head", app_head_columns, {1, token}));
            access.write(std::move(writes));
            const auto stored_head = access.current();
            const auto stored_slots = owner.execute(select_slot);
            const auto stored_roots = owner.execute(select_root);
            if (!matches(stored_head, stored_slots, stored_roots))
                access.fail(SqliteBundleStorageReason::invalid_stored_data);
            return Disposition::inserted;
        });
    } catch (const SqliteWorkFailure &error) {
        return error.error();
    } catch (const SqliteBundleStorageFailure &error) {
        if (error.reason() == SqliteBundleStorageReason::result_limit)
            return WorkError::result_limit;
        if (error.reason() == SqliteBundleStorageReason::resource_limit)
            return WorkError::resource_limit;
        if (error.reason() == SqliteBundleStorageReason::busy)
            return WorkError::busy;
        return WorkError::sql_failure;
    } catch (const RetentionFailure &error) {
        return error.error() == RetentionError::result_limit ? WorkError::result_limit
                                                            : WorkError::sql_failure;
    }
}
MigrationManifest application_bundle_repository_manifest() {
    return bundle_detail::make_manifest();
}
SqliteApplicationBundleStore::SqliteApplicationBundleStore(SqliteWorkSession session,
                                                           SqliteBundleLimits limits)
    : session_(std::move(session)), limits_(limits) {}
SqliteBundleInitialDisposition
SqliteApplicationBundleStore::retain_initial_state(const ApplicationStateSnapshot &s) const {
    bundle_detail::validate_initial(s, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.initial(s); });
}
std::optional<ApplicationStateSnapshot> SqliteApplicationBundleStore::read_state() const {
    bundle_detail::validate_options(limits_);
    Access a(session_, limits_);
    return a.guarded([&] {
        auto s = a.current();
        if (s)
            a.result_size(s->balances.size(), bundle_detail::logical_bytes(*s));
        return s;
    });
}
ApplicationBundleInputResult
SqliteApplicationBundleStore::read_inputs(const ApplicationPlanKey &p,
                                          ApplicationSubjectKind k) const {
    bundle_detail::validate_options(limits_);
    bundle_detail::validate_plan_key(p, limits_);
    if (int(k) < 0 || int(k) > 1)
        throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_source);
    Access a(session_, limits_);
    a.owning_plan_read_ = true;
    return a.guarded([&] { return a.inputs(p, k); });
}
std::optional<RecordedApplicationBundle>
SqliteApplicationBundleStore::lookup(const ApplicationOperationKey &k) const {
    bundle_detail::validate_options(limits_);
    bundle_detail::validate_key(k, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.lookup(k); });
}
std::optional<ApplicationOriginalClaimMatch> SqliteApplicationBundleStore::lookup_original_claim(
    const ApplicationOriginalClaimSelector &selector) const {
    bundle_detail::validate_options(limits_);
    if (int(selector.subject_kind) < 0 || int(selector.subject_kind) > 1 ||
        selector.receipt_id.value == 0 ||
        (selector.subject_kind == ApplicationSubjectKind::contribution) !=
            selector.contribution_id.has_value())
        throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_source);
    Access a(session_, limits_);
    return a.guarded([&] { return a.original_claim(selector); });
}
ApplicationReversalBasisResult
SqliteApplicationBundleStore::read_reversal_basis(const ApplicationOriginalComponent &o) const {
    bundle_detail::validate_options(limits_);
    bundle_detail::validate_original(o, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.basis(o); });
}
ApplicationBundleStageResult
SqliteApplicationBundleStore::stage(const PreparedApplicationBundle &b) const {
    bundle_detail::validate_bundle(b, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.stage(b); });
}

review_detail::PreviewInputs review_detail::preview_inputs(const SqliteWorkSession &session) {
    auto read = std::make_shared<Access>(session, SqliteBundleLimits{});
    read->immutable_reads_ = true;
    return [read = std::move(read), current = std::optional<ApplicationStateSnapshot>{}](
               const ApplicationPlanKey &key, detail::ReviewReadCost *plan_cost) mutable {
        bundle_detail::validate_plan_key(key, read->limits_);
        return read->guarded([&] {
            if (!current)
                current = read->current();
            if (!current)
                read->fail(Reason::missing_state);
            return read->inputs(key, ApplicationSubjectKind::contribution, &*current, plan_cost);
        });
    };
}

ApplicationBundleInputResult review_detail::live_review_inputs(const SqliteWorkSession &session,
                                                               const ApplicationPlanKey &key,
                                                               detail::ReviewReadCost &plan_cost) {
    Access access(session, SqliteBundleLimits{});
    bundle_detail::validate_plan_key(key, access.limits_);
    return access.guarded([&] {
        return access.inputs(key, ApplicationSubjectKind::contribution, nullptr, &plan_cost);
    });
}

review_detail::BundleHistory review_detail::bundle_history(const SqliteWorkSession &session) {
    auto read = std::make_shared<Access>(session, SqliteBundleLimits{});
    read->review_history_ = true;
    return [read = std::move(read)](const ApplicationOperationKey &key) {
        bundle_detail::validate_key(key, read->limits_);
        return read->guarded([&] {
            auto record = read->lookup(key);
            if (record)
                read->preceding_review_state_ = record->after;
            return record;
        });
    };
}
} // namespace subathon::server

#include "sqlite_selected_metadata.hpp"
namespace subathon::server::selected_record_read {
ApplicationResult read_application(const SqliteWorkSession &session,
                                   const ApplicationOperationKey &operation,
                                   const SqliteBundleLimits &limits) {
    bundle_detail::validate_options(limits);
    bundle_detail::validate_key(operation, limits);
    try {
        SelectedMetadata metadata(session, limits);
        if (!metadata.application(operation))
            return Failure::missing;
        Access decoder(session, limits, Access::ReadPolicy::selected);
        auto record = decoder.lookup(operation);
        if (!record)
            return Failure::missing;
        return std::move(*record);
    } catch (Failure f) {
        return f;
    }
}
} // namespace subathon::server::selected_record_read
