// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_policy.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <utility>

namespace subathon::server::session_policy {
namespace {
constexpr auto last_identity = std::numeric_limits<Identity>::max();

Identity mint_owner_identity() {
    static std::atomic<Identity> last{};
    auto value = last.load(std::memory_order_relaxed);
    while (value != last_identity) {
        if (last.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
            return value + 1;
    }
    return 0;
}

bool valid_origin(std::string_view origin) {
    if (origin.size() > 1024 || !session_transport_detail::canonical_origin(origin))
        return false;
    const auto host = origin.substr(std::string_view("https://").size());
    const auto dot = host.rfind('.');
    const auto label = dot == host.npos ? host : host.substr(dot + 1);
    const auto digit = [](unsigned char c) { return c >= '0' && c <= '9'; };
    const auto hex = [&](unsigned char c) { return digit(c) || (c >= 'a' && c <= 'f'); };
    return !std::ranges::all_of(label, digit) &&
           !(label.starts_with("0x") && std::ranges::all_of(label.substr(2), hex));
}

bool valid_mode(const Facts &facts) {
    switch (facts.mode) {
    case Facts::Mode::proxied:
        return facts.forwarded_metadata;
    case Facts::Mode::direct_tls:
        return facts.admitted_tls && !facts.forwarded_metadata;
    case Facts::Mode::direct_plain:
        return !facts.admitted_tls && !facts.forwarded_metadata;
    }
    return false;
}

bool valid_route(const Route &route) {
    const auto method_byte = [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               std::string_view("!#$%&'*+-.^_`|~").find(static_cast<char>(c)) !=
                   std::string_view::npos;
    };
    return !route.method.empty() && route.method.size() <= 16 &&
           std::ranges::all_of(route.method, method_byte) && route.target.starts_with('/') &&
           route.target.size() <= 2048 && route.target.find_first_of("?#") == route.target.npos &&
           std::ranges::all_of(route.target, [](unsigned char c) { return c > 32 && c != 127; }) &&
           (route.purpose == AccountTransportPurpose::protected_access ||
            (route.purpose == AccountTransportPurpose::browser_sign_in_guard &&
             route.method == "POST"));
}

bool valid_facts(const Facts &facts, Identity installation, const Budgets &budgets) {
    if (facts.installation != installation || !valid_origin(facts.origin) ||
        !session_transport_detail::valid_limits(facts.limits) || !valid_mode(facts) ||
        facts.routes.empty() || facts.routes.size() > budgets.route_entries ||
        facts.peers.empty() || facts.peers.size() > budgets.peer_entries ||
        !(facts.foundation && facts.native_abi && facts.native_artifact && facts.profile &&
          facts.listener && facts.edge_artifact && facts.edge_config && facts.logging &&
          facts.isolation && facts.forwarding_grammar && facts.normalization) ||
        facts.configuration.empty() || facts.address.empty() ||
        !std::ranges::all_of(facts.routes, valid_route) ||
        std::ranges::any_of(facts.peers, &std::string::empty))
        return false;

    // The same native method and target cannot bind multiple policy entries.
    for (auto route = facts.routes.begin(); route != facts.routes.end(); ++route) {
        if (std::ranges::any_of(facts.routes.begin(), route, [&](const Route &prior) {
                return route->method == prior.method && route->target == prior.target;
            }))
            return false;
    }
    return true;
}

bool requires_drain(const Facts &old, const Facts &next) {
    return old.address != next.address || old.listener != next.listener ||
           old.routes != next.routes || old.foundation != next.foundation ||
           old.native_abi != next.native_abi || old.native_artifact != next.native_artifact ||
           old.profile != next.profile || old.edge_artifact != next.edge_artifact ||
           old.edge_config != next.edge_config || old.isolation != next.isolation ||
           old.peers != next.peers;
}

bool matches_readiness(const Readiness &ack, Candidate candidate, const Facts &facts) {
    return ack.value && ack.candidate == candidate && ack.profile == facts.profile &&
           ack.listener == facts.listener && ack.configuration == facts.configuration &&
           ack.address == facts.address;
}

Pin make_snapshot(Identity owner, std::uint64_t generation, const Facts &facts) {
    return std::make_shared<const Snapshot>(
        Snapshot{owner, generation, "g" + std::to_string(generation), facts});
}
} // namespace

Policy::Policy(Identity owner, Identity installation, Budgets budgets,
               std::weak_ptr<ReadinessProducer> producer)
    : owner_(owner), installation_(installation), budgets_(budgets),
      producer_(std::move(producer)) {}

std::unique_ptr<Policy> Policy::construct(Owner owner, Budgets budgets,
                                          std::weak_ptr<ReadinessProducer> producer) {
    if (!owner.identity || !owner.installation || !budgets.route_entries || !budgets.peer_entries ||
        !budgets.retained_snapshots || producer.expired())
        return {};
    const auto identity = mint_owner_identity();
    if (!identity)
        return {};
    return std::unique_ptr<Policy>(
        new Policy(identity, owner.installation, budgets, std::move(producer)));
}

std::unique_ptr<Policy> Policy::seed_for_test(Owner owner, Budgets budgets,
                                              std::weak_ptr<ReadinessProducer> producer,
                                              std::uint64_t generation, Facts facts,
                                              Readiness ack) {
    auto policy = construct(owner, budgets, std::move(producer));
    if (!policy || !generation || !valid_facts(facts, owner.installation, budgets) ||
        !matches_readiness(ack, ack.candidate, facts))
        return {};
    ack.candidate.attempt.owner = policy->owner_;
    if (!policy->readiness_is_live(ack))
        return {};
    policy->last_attempt_ = std::max(ack.candidate.attempt.value, ack.candidate.value);
    policy->snapshot_ = make_snapshot(policy->owner_, generation, facts);
    policy->active_ = std::move(ack);
    policy->suspended_ = false;
    return policy;
}

std::optional<Attempt> Policy::suspend() {
    if (stopped_ || producer_.expired() || last_attempt_ >= last_identity - 1)
        return {};
    consume();
    suspended_ = true;
    attempt_ = Attempt{owner_, ++last_attempt_};
    return attempt_;
}

bool Policy::live_attempt(Attempt attempt) const {
    return !stopped_ && attempt_ && *attempt_ == attempt;
}

std::optional<Candidate> Policy::validate(Attempt attempt, const Facts &facts,
                                          std::span<const Identity> affected) {
    if (!live_attempt(attempt) || validated_)
        return {};
    if (!valid_facts(facts, installation_, budgets_)) {
        latch_failure();
        return {};
    }

    const bool drain_required = snapshot_ && requires_drain(snapshot_->facts, facts);
    // The lifecycle owner supplies the inventory of affected native channels.
    // Its admission bound is the policy's peer-entry ceiling.
    if (drain_required == affected.empty() || affected.size() > budgets_.peer_entries) {
        latch_failure();
        return {};
    }
    std::vector<Identity> channels(affected.begin(), affected.end());
    std::ranges::sort(channels);
    if (!channels.empty() &&
        (!channels.front() || std::ranges::adjacent_find(channels) != channels.end())) {
        latch_failure();
        return {};
    }

    const bool differs = !snapshot_ || snapshot_->facts != facts;
    const Candidate candidate{attempt, ++last_attempt_};
    validated_.emplace(Validated{candidate, facts, differs, std::move(channels)});
    return candidate;
}

bool Policy::abandon(Attempt attempt) {
    if (!live_attempt(attempt))
        return false;
    if (!validated_ || validated_->differs)
        latch_failure();
    else
        consume();
    return true;
}

void Policy::consume() {
    attempt_.reset();
    validated_.reset();
    invalid_pending_.reset();
}

void Policy::latch_failure() {
    consume();
    latched_ = true;
    suspended_ = true;
}

bool Policy::readiness_is_live(const Readiness &ack) const {
    const auto producer = producer_.lock();
    return producer && producer->query(ack) == ReadinessState::live;
}

bool Policy::confirm_active_readiness() {
    if (active_ && !readiness_is_live(*active_)) {
        active_.reset();
        latched_ = true;
        suspended_ = true;
    }
    return active_.has_value();
}

void Policy::invalidate_readiness(const Readiness &ack) {
    if (active_ && ack == *active_) {
        active_.reset();
        latched_ = true;
        suspended_ = true;
    } else if (validated_ && matches_readiness(ack, validated_->candidate, validated_->facts)) {
        // The producer owns liveness for superseded acknowledgments. Keep an
        // invalidation until that producer can no longer report its identity live.
        if (!invalid_pending_ || !readiness_is_live(*invalid_pending_))
            invalid_pending_ = ack;
    }
}

bool Policy::fits_new_snapshot() {
    std::erase_if(retired_, [](const auto &pin) { return pin.expired(); });
    auto remaining = budgets_.retained_snapshots - 1;
    if (retired_.size() > remaining)
        return false;
    remaining -= retired_.size();
    // The owner's reference alone does not retain the previous generation.
    return snapshot_.use_count() <= 1 || remaining > 0;
}

Pin Policy::publish(Attempt attempt, const Readiness &ack, std::optional<Drain> drain) {
    if (!live_attempt(attempt) || !validated_ ||
        !matches_readiness(ack, validated_->candidate, validated_->facts) ||
        (invalid_pending_ && ack == *invalid_pending_) || !readiness_is_live(ack))
        return {};

    const auto &expected = validated_->channels;
    if (drain ? (expected.empty() || drain->candidate != validated_->candidate ||
                 !std::ranges::is_permutation(drain->channels, expected))
              : !expected.empty())
        return {};

    auto next_readiness = ack;
    const bool keep_snapshot = confirm_active_readiness() && !latched_ && !validated_->differs;
    if (!keep_snapshot) {
        if (!fits_new_snapshot()) {
            latch_failure();
            return {};
        }
        const auto generation = snapshot_ ? snapshot_->generation : 0;
        if (generation == last_identity) {
            stop();
            return {};
        }

        auto next = make_snapshot(owner_, generation + 1, validated_->facts);
        if (snapshot_.use_count() > 1)
            retired_.emplace_back(snapshot_);
        snapshot_ = std::move(next);
    }

    active_ = std::move(next_readiness);
    consume();
    suspended_ = false;
    latched_ = false;
    return snapshot_;
}

Pin Policy::acquire() {
    if (!confirm_active_readiness() || stopped_ || suspended_ || latched_)
        return {};
    return snapshot_;
}

bool Policy::is_current(const Pin &pin) {
    return pin && pin->owner == owner_ && confirm_active_readiness() && !stopped_ && !latched_ &&
           pin == snapshot_;
}

void Policy::stop() {
    stopped_ = true;
    suspended_ = true;
    consume();
    active_.reset();
    snapshot_.reset();
    retired_.clear();
}
} // namespace subathon::server::session_policy
