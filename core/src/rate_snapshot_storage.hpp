// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/rate_snapshot.hpp>

namespace subathon::core::detail {
inline constexpr RateSnapshotLimits stored_snapshot_limits{
    .currency_bytes = 64,
    .reference_bytes = 256,
    .raw_time_bytes = 512,
    .reason_bytes = 4096,
    .observations = 16,
    .quotes_per_observation = 4096,
    .total_quotes = 16384,
    .overrides = 1024,
    .constraints = 4096,
    .currencies = 256,
    .items = 32768,
    .owned_bytes = 24576,
    .potential_component_bits = 4096,
};
inline constexpr std::size_t stored_potential_digits = 1234;
inline constexpr std::size_t snapshot_scratch_items = 8192;

// Validate stored order and apply the storage-specific arithmetic and scratch bounds.
EffectiveRateSnapshot make_stored_rate_snapshot(const EffectiveRateSnapshotInput &input);
} // namespace subathon::core::detail
