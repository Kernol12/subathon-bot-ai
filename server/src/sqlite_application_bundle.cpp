// SPDX-License-Identifier: GPL-3.0-or-later
#include "contribution_review_bundle_history.hpp"
#include "sqlite_application_bundle_access.hpp"
namespace subathon::server {
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
