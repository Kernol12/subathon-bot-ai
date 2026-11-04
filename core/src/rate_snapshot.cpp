// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/rate_snapshot.hpp>

namespace subathon::core {
RateSnapshotFailure::RateSnapshotFailure(RateSnapshotError code)
    : std::invalid_argument("Invalid rate snapshot input"), code_(code) {}

RateSnapshotNotImplemented::RateSnapshotNotImplemented()
    : std::logic_error("Rate snapshots are intentionally unimplemented; review tests first") {}

RateMoney make_rate_money(const ReceiptMoney &, const RateSnapshotLimits &) {
    throw RateSnapshotNotImplemented{};
}

FetchedRateObservation make_fetched_rate_observation(const FetchedRateObservationInput &,
                                                     const RateSnapshotLimits &) {
    throw RateSnapshotNotImplemented{};
}

EffectiveRateSnapshot make_effective_rate_snapshot(const EffectiveRateSnapshotInput &,
                                                   const RateSnapshotLimits &) {
    throw RateSnapshotNotImplemented{};
}

RatePairResult quote_effective_rate(const EffectiveRateSnapshotInput &, std::string_view,
                                    std::string_view, const RateSnapshotLimits &) {
    throw RateSnapshotNotImplemented{};
}
} // namespace subathon::core
