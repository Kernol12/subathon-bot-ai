// SPDX-License-Identifier: GPL-3.0-or-later
#include "boundary.hpp"

namespace subathon::server::rate_credentials {
// Compile-only Stage 1 seams. Each missing operation explicitly refuses.
class OwnerState {};
struct SlotState {};
struct PermitState {};
struct AttemptState {};

template <class State> void Destroy<State>::operator()(State *state) const noexcept {
    delete state;
}
template struct Destroy<SlotState>;
template struct Destroy<PermitState>;
template struct Destroy<AttemptState>;

CredentialOwner::CredentialOwner() = default;
CredentialOwner::~CredentialOwner() = default;

Result CredentialOwner::admit_change(const RateCredentialChange &, const ManagementGrant &,
                                     std::span<const unsigned char>) {
    return Result::feature_not_implemented;
}

MutationResult CredentialOwner::stage_rate_credential_put(const SqliteWorkSession &,
                                                          const CredentialKey *,
                                                          const RateCredentialChange &,
                                                          const credential_detail::GuardedBytes &) {
    return {Result::feature_not_implemented};
}

MutationResult CredentialOwner::stage_rate_credential_erase(const SqliteWorkSession &,
                                                            const CredentialKey *,
                                                            const RateCredentialChange &) {
    return {Result::feature_not_implemented};
}

Result CredentialOwner::finish_change(const RateCredentialChange &, const SqliteWorkResult &) {
    return Result::feature_not_implemented;
}

Readback CredentialOwner::inspect(const SqliteWorkSession &, const CredentialKey *,
                                  const RateCredentialChange &, const ManagementGrant &) {
    return {Result::feature_not_implemented};
}

Result CredentialOwner::reconcile(const RateCredentialChange &, const Readback &,
                                  const SqliteWorkResult &) {
    return Result::feature_not_implemented;
}

void CredentialOwner::publish_selection(const RateCredentialSelection &) {}
void CredentialOwner::publish_reservation(const Reservation &) {}
void CredentialOwner::revoke(const RateCredentialScope &, std::string) {}

Result CredentialOwner::prepare_keyed_rate_use(const SqliteWorkSession &, const CredentialKey *,
                                               const Reservation &, const UseGrant &, WorkSlot &) {
    return Result::feature_not_implemented;
}

Permit CredentialOwner::finish_keyed_rate_use(WorkSlot, const SqliteWorkResult &) { return {}; }

Result CredentialOwner::consume_keyed_rate_use(Permit &, const UseGrant &, QualifiedTransport &,
                                               Attempt &) {
    return Result::feature_not_implemented;
}

Result CredentialOwner::finish_attempt(Attempt &, const UseGrant &, era::Dispatch) {
    return Result::feature_not_implemented;
}

std::optional<UsageFact>
CredentialOwner::complete_rate_usage_echo(Attempt &, const oer::AdmissionIdentity &, GuardedEcho) {
    return std::nullopt;
}

Result validate_rate_scope(std::span<const unsigned char>) {
    return Result::feature_not_implemented;
}

Readback inspect_rate_payload(std::span<const unsigned char>, std::span<const unsigned char>) {
    return {Result::feature_not_implemented};
}
} // namespace subathon::server::rate_credentials
