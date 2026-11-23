// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../support/sqlite_rate_override_writer_fixture.hpp"

#if defined(SUBATHON_RATE_REMOVAL_MISSING_OWNER)
namespace subathon::core {
struct RateOverridePair {
    std::string low_currency, high_currency;
};
struct RateOverrideRemovalProposal {
    EffectiveRateSnapshotInput candidate;
    std::vector<RatePairChange> affected_rates;
    std::string removed_override_ref;
};

inline RateOverrideRemovalProposal
preview_rate_override_removal(const EffectiveRateSnapshot &, const RateOverridePair &,
                              std::string_view, std::span<const RateOverrideFetchedEdge>,
                              std::string_view) {
    throw std::logic_error("Missing owner: preview_rate_override_removal");
}
} // namespace subathon::core

namespace subathon::server {
struct RateOverrideRemovalEdit {
    std::string installation, operation, actor, decision, base, successor;
    core::RateOverridePair pair;
    std::string removed_override_ref;
};
struct RateRemovalPreview {
    RateOverrideRemovalEdit edit;
    RateSourceBasis basis;
    core::RateOverrideRemovalProposal proposal;
};
struct RateRemovalReceipt {
    std::string decision, base, successor;
    core::RateOverridePair pair;
    std::string removed_override_ref;
    std::vector<core::RatePairChange> changes;
};
struct RateRemovalReceiptSelection {
    RateReceiptOrigin origin;
    RateRemovalReceipt receipt;
};
using RateRemovalPreviewResult = std::variant<RateRemovalPreview, RateStoreRefusal>;
using RateRemovalSaveResult = std::variant<RateRemovalReceiptSelection, RateStoreRefusal>;
using RateRemovalRecoveryResult =
    std::variant<RateRemovalReceiptSelection, RateStoreRefusal, RateAttemptAbsent>;

inline std::optional<RateRemovalReceiptSelection>
finish_rate_removal_receipt(std::optional<RateRemovalReceiptSelection>, const SqliteWorkResult &) {
    return std::nullopt;
}
} // namespace subathon::server
#endif

namespace removal_test {
using namespace rate_fixture;

#if defined(SUBATHON_RATE_REMOVAL_MISSING_OWNER)
// These ports supply no acceptance, authority, time, or durable effects.
inline RateRemovalPreviewResult preview_port(const SqliteRateOverrideUnit &, RateAuthoritySource &,
                                             const PermissionGraphTrust &,
                                             const RateOverrideRemovalEdit &,
                                             const RateSourceBasis &) {
    return RateStoreRefusal::unavailable;
}
inline RateRemovalSaveResult save_port(const SqliteRateOverrideUnit &, RateAuthoritySource &,
                                       const PermissionGraphTrust &,
                                       const RateOverrideRemovalEdit &, const RateRemovalPreview &,
                                       const RateSourceBasis &,
                                       const std::function<std::optional<std::int64_t>()> &) {
    return RateStoreRefusal::unavailable;
}
inline RateRemovalRecoveryResult recover_port(const SqliteRateOverrideUnit &, RateAuthoritySource &,
                                              const PermissionGraphTrust &,
                                              const RateOverrideRemovalEdit &,
                                              const RateRemovalPreview &) {
    return RateStoreRefusal::unavailable;
}
#else
inline constexpr auto preview_port = &SqliteRateOverrideUnit::preview_removal;
inline constexpr auto save_port = &SqliteRateOverrideUnit::save_removal;
inline constexpr auto recover_port = &SqliteRateOverrideUnit::recover_removal;
#endif
} // namespace removal_test
