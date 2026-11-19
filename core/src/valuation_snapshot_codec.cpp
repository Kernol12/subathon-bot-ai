// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/valuation_snapshot_codec.hpp>

namespace subathon::core {
namespace {
class ValuationSnapshotCodecNotImplemented final : public std::logic_error {
  public:
    ValuationSnapshotCodecNotImplemented()
        : std::logic_error("RSV1 codec intentionally unimplemented (RATES-011 Stage 1)") {}
};
} // namespace

const std::string_view valuation_snapshot_format_ref = "subathon.internal.valuation-snapshot.v1";

ExactValuationFailure::ExactValuationFailure(ExactValuationError code)
    : std::invalid_argument("Invalid exact valuation"), code_(code) {}

ExactValuationError ExactValuationFailure::code() const noexcept { return code_; }

ExactValuationDocument encode_valuation_snapshot(const EffectiveRateSnapshot &) {
    throw ValuationSnapshotCodecNotImplemented{};
}

EffectiveRateSnapshot decode_valuation_snapshot(std::string_view, std::string_view) {
    throw ValuationSnapshotCodecNotImplemented{};
}
} // namespace subathon::core
