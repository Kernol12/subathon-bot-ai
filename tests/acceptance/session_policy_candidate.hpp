// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "session_transport.hpp"

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace subathon::server::session_policy_candidate {

// Synthetic identities and evidence for private, Qt-serialized owner operations.
using Identity = std::uint64_t;

struct Owner {
    Identity identity;
    Identity installation;
};
struct Budgets {
    std::uint64_t route_entries, peer_entries, retained_snapshots;
};
struct Route {
    std::string target, method;
    AccountTransportPurpose purpose;
};
struct Facts {
    Identity installation = 1;
    std::string origin = "https://timer.example";
    std::vector<Route> routes{{"/account", "POST", AccountTransportPurpose::protected_access}};
    std::vector<std::string> peers{"127.0.0.1"};
    AccountTransportLimits limits;
    Identity foundation = 1, native_abi = 1, native_artifact = 1, profile = 1;
    // 892.2's edge artifact or configuration refers to edge_artifact and edge_config.
    Identity listener = 1, edge_artifact = 1, edge_config = 1, logging = 1;
    Identity isolation = 1, forwarding_grammar = 1, normalization = 1;
    std::string configuration = "configuration", address = "127.0.0.1";
    enum class Mode { proxied, direct_tls, direct_plain } mode = Mode::proxied;
    bool admitted_tls = false;
    bool forwarded_metadata = true;
};
// Test-only aggregate evidence enables negative binding tests. The owner
// issues production evidence privately after validation. The lifecycle
// owner supplies the finite affected-channel bound.
struct Attempt {
    Identity owner, value;
};
struct Candidate {
    Attempt attempt;
    Identity value;
};
struct Readiness {
    Candidate candidate;
    Identity value, profile, listener;
    std::string configuration, address;
};
struct Drain {
    Candidate candidate;
    std::vector<Identity> channels;
};
struct Snapshot {
    Identity owner;
    std::uint64_t generation;
    std::string reference;
    Facts facts;
};
using Pin = std::shared_ptr<const Snapshot>;
enum class ReadinessState { live, expired, revoked, contradictory, unknown, unavailable };
class ReadinessProducer {
  public:
    virtual ~ReadinessProducer() = default;
    virtual ReadinessState query(const Readiness &) noexcept = 0;
};

class Policy {
  public:
    static std::unique_ptr<Policy> construct(Owner, Budgets, std::weak_ptr<ReadinessProducer>) {
        return std::make_unique<Policy>();
    }
    static std::unique_ptr<Policy> seed_for_test(Owner, Budgets, std::weak_ptr<ReadinessProducer>,
                                                 std::uint64_t, Facts, Readiness) {
        return {};
    }
    std::optional<Attempt> suspend() { return {}; }
    std::optional<Candidate> validate(Attempt, const Facts &, std::span<const Identity> = {}) {
        return {};
    }
    bool abandon(Attempt) { return false; }
    void invalidate_readiness(const Readiness &) {}
    Pin publish(Attempt, const Readiness &, std::optional<Drain> = {}) { return {}; }

    // Owner thread only. Query the bound producer without blocking, then acquire.
    Pin acquire() { return {}; }
    // Compare owner, authoritative readiness, then generation on the owner thread.
    bool is_current(const Pin &) { return false; }
    void stop() {}
};

// Dropping the reference is the only any-thread operation; it calls no owner.
inline void release(Pin &pin) { pin.reset(); }

} // namespace subathon::server::session_policy_candidate
