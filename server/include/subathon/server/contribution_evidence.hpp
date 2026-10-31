// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/contribution.hpp>
#include <subathon/core/receipt.hpp>
#include <vector>

namespace subathon::server {
// A caller-established activity identity; raw receipt/payment references cannot construct proof.
struct VerifiedActivityIdentity {
    core::ReceiptOrigin origin;
    std::string identifier_namespace;
    std::string opaque_id;
    bool operator==(const VerifiedActivityIdentity &) const = default;
};

struct VerifiedActivityLink {
    VerifiedActivityIdentity identity;
    std::string verification_ref;
    bool operator==(const VerifiedActivityLink &) const = default;
};

enum class ContributionObservationKind { support, confirmation };

// Projection of an immutable committed receipt. Transport attempts remain in intake.
struct ContributionReceipt {
    core::ReceiptId id;
    core::ReceiptIdentity identity;
    core::ReceiptFacts facts;
    core::ReceiptArrival arrival;
    bool operator==(const ContributionReceipt &) const = default;
};

struct ContributionObservation {
    ContributionReceipt receipt;
    ContributionObservationKind kind;
    core::PaymentEvidence evidence;
    std::optional<VerifiedActivityLink> verified_activity;
    // Required for support, absent for confirmation; captured only for the first support.
    std::optional<core::ContributionConditions> support_conditions;
    bool operator==(const ContributionObservation &) const = default;
};

struct ContributionEvidenceRecord {
    ContributionObservation observation;
    std::optional<core::ContributionId> contribution_id;
    bool operator==(const ContributionEvidenceRecord &) const = default;
};

struct ContributionRecord {
    core::ContributionId id;
    std::optional<VerifiedActivityIdentity> activity_identity;
    core::ReceiptId original_receipt_id;
    core::ReceiptFacts original_facts;
    core::ReceiptArrival original_arrival;
    core::ContributionState state;
    bool operator==(const ContributionRecord &) const = default;
};

class ContributionStorageError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

// Primitive writes/lookups only. No deduplication, correlation or eligibility policy in storage.
// Known uncommitted errors throw ContributionStorageError; destruction otherwise rolls back.
class ContributionTransaction {
  public:
    virtual ~ContributionTransaction() = default;
    [[nodiscard]] virtual std::optional<ContributionEvidenceRecord>
    find_receipt(core::ReceiptId receipt_id) = 0;
    [[nodiscard]] virtual std::optional<ContributionRecord>
    find_activity(const VerifiedActivityIdentity &identity) = 0;
    [[nodiscard]] virtual std::vector<ContributionEvidenceRecord>
    unresolved_activity(const VerifiedActivityIdentity &identity) = 0;
    [[nodiscard]] virtual core::ContributionId
    create(const ContributionReceipt &support,
           const std::optional<VerifiedActivityIdentity> &identity,
           const core::ContributionState &state) = 0;
    virtual void append_observation(const ContributionEvidenceRecord &observation) = 0;
    virtual void link_observation(core::ReceiptId receipt_id,
                                  core::ContributionId contribution_id) = 0;
    virtual void update_state(core::ContributionId id, const core::ContributionState &state) = 0;
    virtual void commit() = 0;
};

class ContributionRepository {
  public:
    virtual ~ContributionRepository() = default;
    [[nodiscard]] virtual std::unique_ptr<ContributionTransaction> begin() = 0;
};

enum class ContributionLinkDisposition { created, linked, duplicate, unresolved };
struct ContributionLinkResult {
    std::optional<core::ContributionId> contribution_id;
    ContributionLinkDisposition disposition;
};

class ContributionEvidenceNotImplemented final : public std::logic_error {
  public:
    ContributionEvidenceNotImplemented()
        : std::logic_error(
              "Contribution evidence is intentionally unimplemented; review tests first") {}
};

class ContributionEvidenceService final {
  public:
    explicit ContributionEvidenceService(ContributionRepository &repository)
        : repository_(repository) {}
    // No effects, application candidates/jobs, permissions or provider capability inference.
    [[nodiscard]] ContributionLinkResult observe(const ContributionObservation &observation);

  private:
    ContributionRepository &repository_;
};
} // namespace subathon::server
