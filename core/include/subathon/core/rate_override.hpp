// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
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
    // A successful substitution names the removed override. The candidate owns the new one.
    std::optional<std::string> replaced_override_ref;
};

struct RateOverridePair {
    std::string low_currency, high_currency;
};
struct RateOverrideRemovalProposal {
    EffectiveRateSnapshotInput candidate;
    std::vector<RatePairChange> affected_rates;
    std::string removed_override_ref;
};

// A policy-supplied fetched edge refers to the original one-based quote ordinal.
struct RateOverrideFetchedEdge {
    std::reference_wrapper<const FetchedRateObservation> observation;
    std::uint64_t quote_ordinal;
};

class RateOverrideNotImplemented final : public std::logic_error {
  public:
    RateOverrideNotImplemented() : std::logic_error("RATES-007 rate override is not implemented") {}
};

// Both proposals apply the retained manual constraints first, then keep compatible
// fetched edges in the order supplied by the cache/source policy. Keep full source
// observations for selected edges, including their unselected quote rows. The caller
// retains observations without selected edges. Snapshot bounds and validation apply
// to the candidate. Inputs are borrowed for the call and left intact.
//
// Preview an owned successor with current as its parent, reporting every ordered
// pair whose exact value or availability changes. Editing an active unordered pair
// requires replace_override_ref to name that pair's active override, in either
// orientation. A missing or mismatched target refuses with invalid_override.
// A replacement removes that override, adds the proposed edit and names the removal
// in the preview. Nonpositive rates and contradictory manual constraints refuse
// with the corresponding RateSnapshotFailure before returning a candidate.
[[nodiscard]] RateOverridePreview
preview_rate_override(const EffectiveRateSnapshot &current, const ManualRateOverrideInput &edit,
                      std::span<const RateOverrideFetchedEdge> fetched_edges,
                      std::string_view candidate_ref,
                      std::optional<std::string_view> replace_override_ref = std::nullopt);

// Select the exact active occurrence on the byte-ordered pair and rebuild the
// successor from the surviving manuals and the supplied fetched edge order.
[[nodiscard]] RateOverrideRemovalProposal
preview_rate_override_removal(const EffectiveRateSnapshot &current, const RateOverridePair &pair,
                              std::string_view removed_override_ref,
                              std::span<const RateOverrideFetchedEdge> fetched_edges,
                              std::string_view candidate_ref);

enum class RatePreviewBaseStatus { current, stale };

// The preview base must equal the current rate-head identity. Any change requires
// a fresh preview, even if recomputing would give the same affected rates.
// RATES-010 (#727) owns the authoritative save, permission check and durable revisions.
[[nodiscard]] RatePreviewBaseStatus check_rate_preview_base(const EffectiveRateSnapshot &current,
                                                            std::string_view preview_base_ref);

// Propose a successor using an accepted fetched update, preserving active pins.
// Expire a temporary override when the update supplies a rate for its unordered
// pair through its fetched quotes, directly or via a fetched path, even if the
// value is unchanged. Determine relevance from the update's fetched quotes before
// applying manual constraints. An update with no rate for the pair preserves the
// override. Null means no observation was accepted and yields no proposal. The
// application owns sync acceptance and supplies the fetched edge order.
[[nodiscard]] std::optional<EffectiveRateSnapshotInput> propose_rate_update(
    const EffectiveRateSnapshot &current, const FetchedRateObservation *accepted_observation,
    std::span<const RateOverrideFetchedEdge> fetched_edges, std::string_view candidate_ref);

} // namespace subathon::core
