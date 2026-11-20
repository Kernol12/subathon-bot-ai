// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_rate_overrides.hpp>

namespace subathon::server {
MigrationManifest rate_overrides_repository_manifest() {
    throw RateOverrideCatalogNotImplemented();
}

RateRevisionResult SqliteRateOverrideUnit::head() const { throw RateOverridesNotImplemented(); }

RateRevisionResult SqliteRateOverrideUnit::revision(std::string_view) const {
    throw RateOverridesNotImplemented();
}

RatePreviewResult SqliteRateOverrideUnit::preview(RateAuthoritySource &,
                                                  const PermissionGraphTrust &,
                                                  const RateOverrideEdit &,
                                                  const RateSourceBasis &) const {
    throw RateOverridesNotImplemented();
}

RateSaveResult
SqliteRateOverrideUnit::save(RateAuthoritySource &, const PermissionGraphTrust &,
                             const RateOverrideEdit &, const RateEditPreview &,
                             const RateSourceBasis &,
                             const std::function<std::optional<std::int64_t>()> &) const {
    throw RateOverridesNotImplemented();
}

RateRecoveryResult SqliteRateOverrideUnit::recover(RateAuthoritySource &,
                                                   const PermissionGraphTrust &,
                                                   const RateOverrideEdit &,
                                                   const RateEditPreview &) const {
    throw RateOverridesNotImplemented();
}

std::optional<RateReceiptSelection> finish_rate_receipt(std::optional<RateReceiptSelection>,
                                                        const SqliteWorkResult &) {
    throw RateOverridesNotImplemented();
}
} // namespace subathon::server
