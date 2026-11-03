// SPDX-License-Identifier: GPL-3.0-or-later
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
} // namespace subathon::server
