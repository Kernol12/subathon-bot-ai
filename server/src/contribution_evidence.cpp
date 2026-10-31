// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/contribution_evidence.hpp>

namespace subathon::server {
namespace {
void validate_observation(const ContributionObservation &observation) {
    switch (observation.kind) {
    case ContributionObservationKind::support:
        if (!observation.support_conditions) {
            throw std::invalid_argument("Support observation requires captured conditions");
        }
        break;
    case ContributionObservationKind::confirmation:
        if (observation.support_conditions ||
            observation.evidence != core::PaymentEvidence::confirmed) {
            throw std::invalid_argument(
                "Confirmation requires confirmed evidence and no conditions");
        }
        break;
    default:
        throw std::invalid_argument("Unknown contribution observation kind");
    }

    if (observation.verified_activity) {
        const auto &link = *observation.verified_activity;
        const auto &identity = link.identity;
        if (identity.origin != observation.receipt.identity.origin ||
            identity.origin.provider.empty() || identity.origin.source_account.empty() ||
            identity.identifier_namespace.empty() || identity.opaque_id.empty() ||
            link.verification_ref.empty()) {
            throw std::invalid_argument(
                "Verified activity requires same-source identity and proof");
        }
    }
}
} // namespace

ContributionLinkResult
ContributionEvidenceService::observe(const ContributionObservation &observation) {
    validate_observation(observation);
    auto transaction = repository_.begin();
    if (const auto previous = transaction->find_receipt(observation.receipt.id)) {
        if (previous->observation != observation) {
            throw std::invalid_argument("Receipt observation cannot replace committed history");
        }
        transaction->commit();
        return {previous->contribution_id, ContributionLinkDisposition::duplicate};
    }

    std::optional<VerifiedActivityIdentity> identity;
    if (observation.verified_activity) {
        identity = observation.verified_activity->identity;
    }
    const auto existing = identity ? transaction->find_activity(*identity) : std::nullopt;
    if (existing) {
        transaction->append_observation({observation, existing->id});
        // Only explicit confirmation promotes the projection; captured conditions stay intact.
        if (observation.evidence == core::PaymentEvidence::confirmed &&
            existing->state.payment_evidence != core::PaymentEvidence::confirmed) {
            auto state = existing->state;
            state.payment_evidence = core::PaymentEvidence::confirmed;
            transaction->update_state(existing->id, state);
        }
        transaction->commit();
        return {existing->id, ContributionLinkDisposition::linked};
    }

    if (observation.kind == ContributionObservationKind::confirmation) {
        transaction->append_observation({observation, std::nullopt});
        transaction->commit();
        return {std::nullopt, ContributionLinkDisposition::unresolved};
    }

    const auto pending = identity ? transaction->unresolved_activity(*identity)
                                  : std::vector<ContributionEvidenceRecord>{};
    core::ContributionState state{observation.evidence, *observation.support_conditions};
    for (const auto &record : pending) {
        if (record.observation.evidence == core::PaymentEvidence::confirmed) {
            state.payment_evidence = core::PaymentEvidence::confirmed;
        }
    }
    // Support supplies the original facts/context even when its confirmation arrived first.
    const auto id = transaction->create(observation.receipt, identity, state);
    transaction->append_observation({observation, id});
    for (const auto &record : pending) {
        transaction->link_observation(record.observation.receipt.id, id);
    }
    transaction->commit();
    return {id, ContributionLinkDisposition::created};
}
} // namespace subathon::server
