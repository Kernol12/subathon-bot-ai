// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "session_transport.hpp"

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace subathon::server::session_policy {

// These types are confined to the private, Qt-serialized host owner boundary.
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
    bool operator==(const Route &) const = default;
};
struct Facts {
    Identity installation{};
    std::string origin;
    std::vector<Route> routes;
    std::vector<std::string> peers;
    AccountTransportLimits limits{0, 0, 0, 0, 0, 0, 0};
    Identity foundation{}, native_abi{}, native_artifact{}, profile{};
    // 892.2's edge artifact or configuration refers to edge_artifact and edge_config.
    Identity listener{}, edge_artifact{}, edge_config{}, logging{};
    Identity isolation{}, forwarding_grammar{}, normalization{};
    std::string configuration, address;
    enum class Mode { proxied, direct_tls, direct_plain, unspecified } mode = Mode::unspecified;
    bool admitted_tls = false;
    bool forwarded_metadata = false;
    bool operator==(const Facts &) const = default;
};
// The bound readiness producer and lifecycle owner supply these records.
// Public request handling cannot submit them.
struct Attempt {
    Identity owner, value;
    bool operator==(const Attempt &) const = default;
};
struct Candidate {
    Attempt attempt;
    Identity value;
    bool operator==(const Candidate &) const = default;
};
struct Readiness {
    Candidate candidate;
    Identity value, profile, listener;
    std::string configuration, address;
    bool operator==(const Readiness &) const = default;
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
    // Only the exact active or current pending acknowledgment can be live.
    // A superseded acknowledgment stays non-live for this owner.
    virtual ReadinessState query(const Readiness &) noexcept = 0;
};

// Only releasing a pin may run outside the Qt owner thread. The producer's
// query runs on that thread and is bounded and non-blocking.
class Policy {
  public:
    static std::unique_ptr<Policy> construct(Owner, Budgets, std::weak_ptr<ReadinessProducer>);
    static std::unique_ptr<Policy> seed_for_test(Owner, Budgets, std::weak_ptr<ReadinessProducer>,
                                                 std::uint64_t, Facts, Readiness);

    std::optional<Attempt> suspend();
    std::optional<Candidate> validate(Attempt, const Facts &, std::span<const Identity> = {});
    bool abandon(Attempt);
    void invalidate_readiness(const Readiness &);
    Pin publish(Attempt, const Readiness &, std::optional<Drain> = {});
    Pin acquire();
    bool is_current(const Pin &);
    void stop();

    Policy(const Policy &) = delete;
    Policy &operator=(const Policy &) = delete;

  private:
    struct Validated {
        Candidate candidate;
        Facts facts;
        bool differs;
        std::vector<Identity> channels;
    };

    Policy(Identity, Identity, Budgets, std::weak_ptr<ReadinessProducer>);
    bool live_attempt(Attempt) const;
    bool confirm_active_readiness();
    bool readiness_is_live(const Readiness &) const;
    void consume();
    void latch_failure();
    bool fits_new_snapshot();

    Identity owner_;
    Identity installation_;
    Budgets budgets_;
    std::weak_ptr<ReadinessProducer> producer_;
    Identity last_attempt_{};
    std::optional<Attempt> attempt_;
    std::optional<Validated> validated_;
    std::optional<Readiness> invalid_pending_;
    std::optional<Readiness> active_;
    Pin snapshot_;
    std::vector<std::weak_ptr<const Snapshot>> retired_;
    bool suspended_{true};
    bool latched_{};
    bool stopped_{};
};

inline void release(Pin &pin) { pin.reset(); }

} // namespace subathon::server::session_policy
