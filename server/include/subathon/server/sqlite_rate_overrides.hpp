// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/rate_override.hpp>
#include <subathon/server/sqlite_permission_graph.hpp>

namespace subathon::server {
// RATES-010 revision 2. These values belong to the authoritative command owner.
struct RateSourceEdge {
    std::uint32_t observation_index;
    std::uint64_t quote_ordinal;
};
struct RateSourceBasis {
    std::string policy_ref;
    std::vector<core::FetchedRateObservationInput> observations;
    std::vector<RateSourceEdge> edges;
};
struct RateOverrideEdit {
    std::string installation, operation, base, successor;
    core::ManualRateOverrideInput override_input;
    std::optional<std::string> replacement;
};
struct RateEditPreview {
    RateOverrideEdit edit;
    RateSourceBasis basis;
    core::RateOverridePreview proposal;
};

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

struct RateAuthorityFacts {
    ResolvedAccountSession session;
    StoredPermissionGraph graph;
};
using RateAuthorityResolution =
    std::variant<RateAuthorityFacts, SessionFailure, PermissionGraphFailure>;

// Trusted composition supplies the resolved current session and whole graph from this
// work scope. The source loads the graph with the same PermissionGraphTrust passed
// to the rate unit, enforcing persisted parity for the full trust. The rate unit
// evaluates ordinary pair USE against those facts.
class RateAuthoritySource {
  public:
    virtual ~RateAuthoritySource() = default;
    virtual RateAuthorityResolution resolve(const SqliteWorkSession &) = 0;
};

enum class RateStoreRefusal {
    denied,
    stale,
    preview_changed,
    identity_conflict,
    invalid_proposal,
    resource_limit,
    invalid_retained_data,
    unsupported_retained_data,
    unavailable
};
struct RateOverrideReceipt {
    std::string decision, base, successor, introduced;
    std::optional<std::string> replaced;
    std::vector<core::RatePairChange> changes;
};
enum class RateReceiptOrigin { newly_staged, retained };
struct RateReceiptSelection {
    RateReceiptOrigin origin;
    RateOverrideReceipt receipt;
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
// For recovery only: verified absence of the entire attempted write. Never implies
// permission to resubmit and never produces a receipt.
struct RateAttemptAbsent {};
using RatePreviewResult = std::variant<RateEditPreview, RateStoreRefusal>;
using RateRevisionResult = std::variant<core::EffectiveRateSnapshot, RateStoreRefusal>;
using RateSaveResult = std::variant<RateReceiptSelection, RateStoreRefusal>;
using RateRecoveryResult = std::variant<RateReceiptSelection, RateStoreRefusal, RateAttemptAbsent>;
using RateRemovalPreviewResult = std::variant<RateRemovalPreview, RateStoreRefusal>;
using RateRemovalSaveResult = std::variant<RateRemovalReceiptSelection, RateStoreRefusal>;
using RateRemovalRecoveryResult =
    std::variant<RateRemovalReceiptSelection, RateStoreRefusal, RateAttemptAbsent>;

class RateOverridesNotImplemented final : public std::logic_error {
  public:
    RateOverridesNotImplemented()
        : std::logic_error("RATES-010 writer intentionally unimplemented") {}
};
class RateOverrideCatalogNotImplemented final : public std::logic_error {
  public:
    RateOverrideCatalogNotImplemented()
        : std::logic_error("RATES-010 catalog intentionally unimplemented") {}
};

[[nodiscard]] MigrationManifest rate_overrides_repository_manifest();

// Internal committed-state reader/typed write unit. No connection, transaction
// control, source transport or accepted-sync certification. The command owner
// supplies current source facts and samples acceptance time on the authorized new
// operation path. The preview's actor/edit fields must agree with the submission.
// Actual SQL/limit failures preserve their SqliteWorkFailure and rollback semantics.
class SqliteRateOverrideUnit final {
  public:
    explicit SqliteRateOverrideUnit(const SqliteWorkSession &session) : session_(session) {}
    [[nodiscard]] RateRevisionResult head() const;
    [[nodiscard]] RateRevisionResult revision(std::string_view ref) const;
    [[nodiscard]] RatePreviewResult preview(RateAuthoritySource &, const PermissionGraphTrust &,
                                            const RateOverrideEdit &,
                                            const RateSourceBasis &) const;
    [[nodiscard]] RateSaveResult
    save(RateAuthoritySource &, const PermissionGraphTrust &, const RateOverrideEdit &,
         const RateEditPreview &, const RateSourceBasis &current,
         const std::function<std::optional<std::int64_t>()> &sample) const;
    [[nodiscard]] RateRecoveryResult recover(RateAuthoritySource &, const PermissionGraphTrust &,
                                             const RateOverrideEdit &,
                                             const RateEditPreview &) const;

    [[nodiscard]] RateRemovalPreviewResult preview_removal(RateAuthoritySource &,
                                                           const PermissionGraphTrust &,
                                                           const RateOverrideRemovalEdit &,
                                                           const RateSourceBasis &) const;
    [[nodiscard]] RateRemovalSaveResult
    save_removal(RateAuthoritySource &, const PermissionGraphTrust &,
                 const RateOverrideRemovalEdit &, const RateRemovalPreview &,
                 const RateSourceBasis &current,
                 const std::function<std::optional<std::int64_t>()> &sample) const;
    [[nodiscard]] RateRemovalRecoveryResult recover_removal(RateAuthoritySource &,
                                                            const PermissionGraphTrust &,
                                                            const RateOverrideRemovalEdit &,
                                                            const RateRemovalPreview &) const;

  private:
    SqliteWorkSession session_;
};

// The caller retains the original typed result provisionally. There is only a
// normal value after acknowledged COMMIT and normal work completion.
[[nodiscard]] std::optional<RateReceiptSelection>
finish_rate_receipt(std::optional<RateReceiptSelection>, const SqliteWorkResult &);
[[nodiscard]] std::optional<RateRemovalReceiptSelection>
finish_rate_removal_receipt(std::optional<RateRemovalReceiptSelection>, const SqliteWorkResult &);
} // namespace subathon::server
