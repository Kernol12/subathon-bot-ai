// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/core/rate_snapshot.hpp>

namespace subathon::core {
// RATES-011 Stage 1, revision 1. Byte grammar, bounds and failure precedence:
// docs/contracts/exact-valuation-storage-v1.md §§4, 5 and 7 (RSV1 only).
struct ExactValuationDocument {
    std::string format_ref;
    std::string document;
};

// Shared §7 reason order. This codec uses only the reasons mapped below.
enum class ExactValuationError {
    resource_limit,
    unsupported_format,
    malformed_document,
    invalid_reference,
    invalid_money,
    invalid_basis,
    invalid_rate,
    missing_pair,
    inconsistent_rate,
    missing_target,
    identity_conflict,
    unusable_legacy_valuation,
    allocation_failure
};

class ExactValuationFailure final : public std::invalid_argument {
  public:
    explicit ExactValuationFailure(ExactValuationError code);
    [[nodiscard]] ExactValuationError code() const noexcept;

  private:
    ExactValuationError code_;
};

// Exactly "subathon.internal.valuation-snapshot.v1". The document begins RSV1.
extern const std::string_view valuation_snapshot_format_ref;

// Both directions apply the fixed bridge RateSnapshotLimits. Raw and parsed
// models each fit 24576 logical bytes and 32768 recursive items, independently
// of the 32768-byte document ceiling. Keep all §4 field, aggregate count, bit
// and scratch bounds, including checked arithmetic before growth. Reuse the
// exact rate model's lexical, duplicate, graph and parsed-size checks. Preserve
// original decimals and quote ordinals, opaque bytes and time presence. Stored
// source, override, constraint, component and member order must be canonical.

// Borrow the validated model for the call. Preflight both model projections and
// the exact framed size before owning an output. Return a complete owned envelope.
[[nodiscard]] ExactValuationDocument encode_valuation_snapshot(const EffectiveRateSnapshot &snapshot);

// Borrow both byte strings for the call. Check format_ref (256 bytes) and
// document (32768 bytes) ceilings before exact format dispatch. Scan a known
// document once in grammar order without allocating, checking each declared
// length/count and raw model totals before reading its body. Framing and limit
// faults take precedence in scan order. After a fitting scan, revalidate the
// complete model, rejecting stored order that the factory would otherwise sort.
// Return the owned snapshot with byte-exact canonical re-encoding.
// Bounds -> resource_limit; unknown fitting format -> unsupported_format;
// known framing -> malformed_document; empty required/present identity ->
// invalid_reference; numeric syntax/domain/reduction -> invalid_rate; conflicting
// edge arithmetic -> inconsistent_rate; other model structure -> invalid_basis.
// Feature-owned allocation refusal -> allocation_failure. Neither direction
// exposes a partial result or includes input bytes in diagnostics.
[[nodiscard]] EffectiveRateSnapshot decode_valuation_snapshot(std::string_view format_ref,
                                                               std::string_view document);
} // namespace subathon::core
