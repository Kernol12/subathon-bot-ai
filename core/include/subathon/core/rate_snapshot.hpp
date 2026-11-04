// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/core/exact_decimal.hpp>
#include <subathon/core/receipt.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace subathon::core {
// Local ceilings and ownership metric: docs/contracts/rate-snapshots-api-v1.md.
// Construction is inert; validation remains intentionally unimplemented.
struct RateSnapshotLimits {
    std::size_t currency_bytes{64};
    std::size_t reference_bytes{256};
    std::size_t raw_time_bytes{512};
    std::size_t reason_bytes{4096};
    std::size_t observations{16};
    std::size_t quotes_per_observation{4096};
    std::size_t total_quotes{16384};
    std::size_t overrides{1024};
    std::size_t constraints{4096};
    std::size_t currencies{256};
    std::size_t items{32768};
    std::size_t owned_bytes{8388608};
    std::size_t potential_component_bits{4096};
};

struct RateTimeEvidence {
    std::optional<std::string> original_text;
    std::optional<std::int64_t> utc_ms;
};
struct RateObservationContext {
    std::string observation_ref;
    std::string provider_ref;
    std::string configuration_ref;
    std::string request_ref;
    std::string source_evidence_ref;
    std::optional<std::string> provider_snapshot_ref;
    std::optional<std::int64_t> fetched_at_utc_ms;
    RateTimeEvidence source_rate_time;
    RateTimeEvidence publication_time;
};
struct RateQuoteInput {
    std::string from_currency;
    std::string to_currency;
    std::string original_decimal;
};
struct FetchedRateObservationInput {
    RateObservationContext context;
    std::vector<RateQuoteInput> quotes;
};
struct RateQuote {
    std::string from_currency;
    std::string to_currency;
    ExactDecimal rate;
};
struct FetchedRateObservationData {
    RateObservationContext context;
    std::vector<RateQuote> quotes;
};
struct RateMoneyData {
    ReceiptMoney original;
    Exact value;
};

class RateMoney final {
  public:
    RateMoney(const RateMoney &) = default;
    RateMoney(RateMoney &&) = default;
    RateMoney &operator=(const RateMoney &) = default;
    RateMoney &operator=(RateMoney &&) = default;
    [[nodiscard]] const RateMoneyData &data() const noexcept { return data_; }

  private:
    explicit RateMoney(RateMoneyData data) : data_(std::move(data)) {}
    friend RateMoney make_rate_money(const ReceiptMoney &, const RateSnapshotLimits &);
    RateMoneyData data_;
};

class FetchedRateObservation final {
  public:
    FetchedRateObservation(const FetchedRateObservation &) = default;
    FetchedRateObservation(FetchedRateObservation &&) = default;
    FetchedRateObservation &operator=(const FetchedRateObservation &) = default;
    FetchedRateObservation &operator=(FetchedRateObservation &&) = default;
    [[nodiscard]] const FetchedRateObservationData &data() const noexcept { return data_; }

  private:
    explicit FetchedRateObservation(FetchedRateObservationData data) : data_(std::move(data)) {}
    friend FetchedRateObservation make_fetched_rate_observation(const FetchedRateObservationInput &,
                                                                const RateSnapshotLimits &);
    FetchedRateObservationData data_;
};

enum class RateOverrideKind { temporary, pinned };
struct ManualRateOverrideInput {
    std::string override_ref;
    RateOverrideKind kind{};
    std::string from_currency;
    std::string to_currency;
    std::string original_decimal;
    std::string decision_ref;
    std::optional<std::string> actor_ref;
    std::string reason;
    std::optional<std::int64_t> recorded_at_utc_ms;
};
struct ManualRateOverrideData {
    std::string override_ref;
    RateOverrideKind kind{};
    std::string from_currency;
    std::string to_currency;
    ExactDecimal rate;
    std::string decision_ref;
    std::optional<std::string> actor_ref;
    std::string reason;
    std::optional<std::int64_t> recorded_at_utc_ms;
};
struct FetchedRateConstraint {
    std::string observation_ref;
    std::uint64_t quote_ordinal{};
};
struct ManualRateConstraint {
    std::string override_ref;
};
using RateConstraint = std::variant<FetchedRateConstraint, ManualRateConstraint>;
struct RatePotentialInput {
    std::string currency;
    std::string numerator;
    std::string denominator;
};
struct RateComponentInput {
    std::vector<RatePotentialInput> members;
};
struct RatePotential {
    std::string currency;
    Exact weight;
};
struct RateComponent {
    std::vector<RatePotential> members;
};
struct EffectiveRateSnapshotInput {
    std::string snapshot_ref;
    std::optional<std::string> parent_snapshot_ref;
    std::optional<std::int64_t> constructed_at_utc_ms;
    std::vector<FetchedRateObservationInput> sources;
    std::vector<ManualRateOverrideInput> overrides;
    std::vector<RateConstraint> constraints;
    std::vector<RateComponentInput> components;
};
struct EffectiveRateSnapshotData {
    std::string snapshot_ref;
    std::optional<std::string> parent_snapshot_ref;
    std::optional<std::int64_t> constructed_at_utc_ms;
    std::vector<FetchedRateObservation> sources;
    std::vector<ManualRateOverrideData> overrides;
    std::vector<RateConstraint> constraints;
    std::vector<RateComponent> components;
};

class EffectiveRateSnapshot final {
  public:
    EffectiveRateSnapshot(const EffectiveRateSnapshot &) = default;
    EffectiveRateSnapshot(EffectiveRateSnapshot &&) = default;
    EffectiveRateSnapshot &operator=(const EffectiveRateSnapshot &) = default;
    EffectiveRateSnapshot &operator=(EffectiveRateSnapshot &&) = default;
    [[nodiscard]] const EffectiveRateSnapshotData &data() const noexcept { return data_; }

  private:
    explicit EffectiveRateSnapshot(EffectiveRateSnapshotData data) : data_(std::move(data)) {}
    friend EffectiveRateSnapshot make_effective_rate_snapshot(const EffectiveRateSnapshotInput &,
                                                              const RateSnapshotLimits &);
    EffectiveRateSnapshotData data_;
};

enum class RatePairKind { identity, available, missing };
struct RatePairProvenance {
    std::string snapshot_ref;
    std::string from_currency;
    std::string to_currency;
};
struct RatePairResultData {
    RatePairKind kind{};
    std::optional<Exact> rate;
    std::optional<RatePairProvenance> provenance;
};

class RatePairResult final {
  public:
    RatePairResult(const RatePairResult &) = default;
    RatePairResult(RatePairResult &&) = default;
    RatePairResult &operator=(const RatePairResult &) = default;
    RatePairResult &operator=(RatePairResult &&) = default;
    [[nodiscard]] const RatePairResultData &data() const noexcept { return data_; }

  private:
    explicit RatePairResult(RatePairResultData data) : data_(std::move(data)) {}
    friend RatePairResult quote_effective_rate(const EffectiveRateSnapshotInput &, std::string_view,
                                               std::string_view, const RateSnapshotLimits &);
    RatePairResultData data_;
};

enum class RateSnapshotError {
    invalid_options,
    invalid_reference,
    invalid_currency,
    invalid_decimal,
    nonpositive_rate,
    invalid_override,
    invalid_potential,
    duplicate_identity,
    duplicate_quote,
    duplicate_constraint,
    unresolved_basis,
    noncanonical_component,
    unbacked_component,
    contradictory_constraint,
    resource_limit
};
class RateSnapshotFailure final : public std::invalid_argument {
  public:
    explicit RateSnapshotFailure(RateSnapshotError code);
    [[nodiscard]] RateSnapshotError code() const noexcept { return code_; }

  private:
    RateSnapshotError code_;
};

class RateSnapshotNotImplemented final : public std::logic_error {
  public:
    RateSnapshotNotImplemented();
};

// Immediate missing-feature stubs until reviewed tests establish the intended red.
[[nodiscard]] RateMoney make_rate_money(const ReceiptMoney &input,
                                        const RateSnapshotLimits &limits = {});
[[nodiscard]] FetchedRateObservation
make_fetched_rate_observation(const FetchedRateObservationInput &input,
                              const RateSnapshotLimits &limits = {});
[[nodiscard]] EffectiveRateSnapshot
make_effective_rate_snapshot(const EffectiveRateSnapshotInput &input,
                             const RateSnapshotLimits &limits = {});
[[nodiscard]] RatePairResult quote_effective_rate(const EffectiveRateSnapshotInput &input,
                                                  std::string_view from_currency,
                                                  std::string_view to_currency,
                                                  const RateSnapshotLimits &limits = {});
} // namespace subathon::core
