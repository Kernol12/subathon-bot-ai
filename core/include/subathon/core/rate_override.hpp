// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/core/rate_snapshot.hpp>
#include <vector>

namespace subathon::core {

struct RatePairChange {
    std::string from_currency;
    std::string to_currency;
    std::optional<Exact> before;
    std::optional<Exact> after;
};

struct RateOverridePreview {
    // An owned proposal for the existing snapshot factory.
    EffectiveRateSnapshotInput candidate;
    std::vector<RatePairChange> affected_rates;
};

class RateOverrideNotImplemented final : public std::logic_error {
  public:
    RateOverrideNotImplemented() : std::logic_error("RATES-007 rate override is not implemented") {}
};

// Propose an owned successor with current as its parent. Report each ordered pair whose
// exact value or availability changes. Nonpositive rates and contradictory pins
// refuse with the corresponding RateSnapshotFailure before returning a candidate.
// The application owns authorization, identity allocation, and committing the change.
[[nodiscard]] RateOverridePreview preview_rate_override(const EffectiveRateSnapshot &current,
                                                        const ManualRateOverrideInput &edit,
                                                        std::string_view candidate_ref);

enum class RatePreviewBaseStatus { current, stale };

// Check the snapshot basis recorded for a preview against the current rate head.
// The owning command checks authorization and revisions and commits through the writer.
[[nodiscard]] RatePreviewBaseStatus check_rate_preview_base(const EffectiveRateSnapshot &current,
                                                            std::string_view preview_base_ref);

// Propose a successor using an accepted fetched update, preserving active pins and
// expiring temporary overrides only on relevant successful updates. Null means no
// observation was accepted, so there is no proposal. The application owns deciding
// sync success and retaining original observations separately from effective snapshots.
[[nodiscard]] std::optional<EffectiveRateSnapshotInput>
propose_rate_update(const EffectiveRateSnapshot &current,
                    const FetchedRateObservation *accepted_observation,
                    std::string_view candidate_ref);

} // namespace subathon::core
