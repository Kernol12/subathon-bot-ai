// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "credential_internal.hpp"

#include <subathon/server/exchange_rate_api.hpp>
#include <subathon/server/open_exchange_rates.hpp>
#include <subathon/server/sqlite_writer.hpp>

#include <array>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace subathon::server::rate_credentials {
namespace era = exchange_rate_api;
namespace oer = open_exchange_rates;
using Revision = std::array<unsigned char, 24>;
using Profile = std::variant<era::Profile, oer::Profile>;
using Route = std::variant<era::Operation, oer::Operation>;

enum class KeyedRateProvider { exchange_rate_api = 8, open_exchange_rates = 9 };
struct RateCredentialScope {
    std::string installation;
    KeyedRateProvider provider;
    std::string account;
    std::string slot;
    auto operator<=>(const RateCredentialScope &) const = default;
};
struct RateCredentialSelection {
    RateCredentialScope scope;
    std::string generation;
    std::string configuration;
    std::string selection;
    Profile profile;
    std::string quota_scope;
    Revision revision;
    bool operator==(const RateCredentialSelection &) const = default;
};
struct Reservation {
    RateCredentialSelection selection;
    std::string request;
    Route route;
    std::uint64_t order;
    bool operator==(const Reservation &) const = default;
};

enum class Verb { put, erase, inspect };
struct RateCredentialChange {
    RateCredentialScope scope;
    Verb verb;
    std::string decision;
    std::optional<Revision> expected;
    bool operator==(const RateCredentialChange &) const = default;
};

// Exact owner-produced authority, supplied separately from the requested facts.
using ManagementGrant = std::optional<RateCredentialChange>;
using UseGrant = std::optional<Reservation>;

enum class Result {
    feature_not_implemented,
    admitted,
    staged_put,
    unchanged,
    staged_erase,
    committed_change,
    known_rollback,
    held,
    prepared,
    accepted,
    current_reply,
    accounting_only,
    reconciled,
    present,
    missing,
    invalid_input,
    resource_limit,
    not_authorized,
    stale,
    key_unavailable,
    invalid_record,
    authentication_failed,
    crypto_unavailable
};
struct MutationResult {
    Result status;
    std::optional<Revision> revision{};
};
struct Readback {
    Readback(Result category, std::optional<Revision> observed = {}) noexcept
        : status(category), revision(observed) {}
    Result status;
    std::optional<Revision> revision;

  private:
    struct Evidence;
    std::shared_ptr<const Evidence> evidence_;
    friend class CredentialOwner;
};

// These state handles are private and move-only. No guarded bytes are exposed by them.
struct SlotState;
struct PermitState;
struct AttemptState;
template <class State> struct Destroy {
    void operator()(State *) const noexcept;
};
using WorkSlot = std::unique_ptr<SlotState, Destroy<SlotState>>;
using Permit = std::unique_ptr<PermitState, Destroy<PermitState>>;
using Attempt = std::unique_ptr<AttemptState, Destroy<AttemptState>>;

class QualifiedTransport {
  public:
    virtual ~QualifiedTransport() = default;
    // Nonblocking private handoff in the accepting owner turn, without reentry.
    // The view is borrowed only for the call. The receiver bounds and erases any
    // wire representation it owns and reports dispatch independently.
    virtual void accept(const Reservation &, std::span<const unsigned char>) noexcept = 0;
};

enum class EchoShape { absent, invalid, single };
struct GuardedEcho {
    EchoShape shape;
    std::unique_ptr<credential_detail::GuardedBytes> value;
};
struct UsageFact {
    oer::AdmissionIdentity admission;
    RateCredentialSelection selection;
    oer::EchoMatch match;
};

class OwnerState;
// All operations and private handles belong to the serialized command/state
// owner. Writer work and its paired completions run on that same thread.
class CredentialOwner final {
  public:
    CredentialOwner();
    ~CredentialOwner();

    Result admit_change(const RateCredentialChange &, const ManagementGrant &,
                        std::span<const unsigned char> guarded_value);
    MutationResult stage_rate_credential_put(const SqliteWorkSession &, const CredentialKey *,
                                             const RateCredentialChange &,
                                             const credential_detail::GuardedBytes &);
    MutationResult stage_rate_credential_erase(const SqliteWorkSession &, const CredentialKey *,
                                               const RateCredentialChange &);
    Result finish_change(const RateCredentialChange &, const SqliteWorkResult &);
    Readback inspect(const SqliteWorkSession &, const CredentialKey *, const RateCredentialChange &,
                     const ManagementGrant &);
    Result reconcile(const RateCredentialChange &, const Readback &, const SqliteWorkResult &);
    // Trusted setup and reservation owners publish on serialized owner turns.
    // Grants attached to requests are presented facts, independent of publication.
    void publish_selection(const RateCredentialSelection &);
    void publish_reservation(const Reservation &);
    // Disable use for the scope and invalidate pending permits before recording the label.
    void revoke(const RateCredentialScope &, std::string disabled_generation);
    void expire_reservation(const Reservation &);

    // The restore owner enters Recovery explicitly. Ordinary selection publication
    // can qualify an exact read. Use requires approval from the Recovery owner.
    void begin_recovery(const RateCredentialScope &, std::string disabled_generation);
    void publish_recovery_selection(const RateCredentialSelection &);

    Result prepare_keyed_rate_use(const SqliteWorkSession &, const CredentialKey *,
                                  const Reservation &, const UseGrant &, WorkSlot &);
    Permit finish_keyed_rate_use(WorkSlot, const SqliteWorkResult &);
    Result consume_keyed_rate_use(Permit &, const UseGrant &, QualifiedTransport &, Attempt &);
    // Each attempt is consumed by a single settlement or usage-echo completion.
    Result finish_attempt(Attempt &, const UseGrant &, era::Dispatch);
    Result finish_attempt(Attempt &, const UseGrant &, oer::Dispatch);
    std::optional<UsageFact> complete_rate_usage_echo(Attempt &, const oer::AdmissionIdentity &,
                                                      GuardedEcho);

  private:
    MutationResult stage(const SqliteWorkSession &, const CredentialKey *,
                         const RateCredentialChange &, Verb, std::span<const unsigned char>);
    Result settle_attempt(Attempt &, const UseGrant &, KeyedRateProvider);
    std::unique_ptr<OwnerState> state_;
};

// The rotation model borrows already authenticated guarded payload bytes. These
// operations qualify the FX family and never return any payload bytes.
Result validate_rate_scope(std::span<const unsigned char>);
Readback inspect_rate_payload(std::span<const unsigned char> scope,
                              std::span<const unsigned char> authenticated);
} // namespace subathon::server::rate_credentials
