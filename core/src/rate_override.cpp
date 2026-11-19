// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/rate_override.hpp>

namespace subathon::core {

RateOverridePreview preview_rate_override(const EffectiveRateSnapshot &,
                                          const ManualRateOverrideInput &, std::string_view) {
    throw RateOverrideNotImplemented();
}

RatePreviewBaseStatus check_rate_preview_base(const EffectiveRateSnapshot &, std::string_view) {
    throw RateOverrideNotImplemented();
}

std::optional<EffectiveRateSnapshotInput> propose_rate_update(const EffectiveRateSnapshot &,
                                                              const FetchedRateObservation *,
                                                              std::string_view) {
    throw RateOverrideNotImplemented();
}

} // namespace subathon::core
