// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_policy_candidate.hpp"

#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <utility>

namespace {
using namespace subathon::server;
using namespace subathon::server::session_policy_candidate;
constexpr Owner installation{11, 1};
constexpr Budgets budgets{4, 4, 3};

class SimulatedReadiness final : public ReadinessProducer {
  public:
    ReadinessState state{ReadinessState::live};
    std::map<Identity, ReadinessState> states;
    std::optional<Identity> last;
    ReadinessState query(const Readiness &request) noexcept override {
        last = request.value;
        const auto found = states.find(request.value);
        return found == states.end() ? state : found->second;
    }
};

Readiness evidence(Candidate candidate, Identity identity, const Facts &facts) {
    return {candidate, identity, facts.profile, facts.listener, facts.configuration, facts.address};
}

class SessionPolicyS2 : public ::testing::Test {
  protected:
    std::shared_ptr<SimulatedReadiness> readiness = std::make_shared<SimulatedReadiness>();
    std::unique_ptr<Policy> policy = Policy::construct(installation, budgets, readiness);
    Facts facts;

    Candidate validate(const Facts &next, std::span<const Identity> affected = {}) {
        const auto attempt = policy->suspend();
        if (!attempt) {
            ADD_FAILURE() << "expected a new suspended attempt";
            return {};
        }
        const auto candidate = policy->validate(*attempt, next, affected);
        if (!candidate) {
            ADD_FAILURE() << "expected validated candidate";
            return {};
        }
        return *candidate;
    }

    Pin publish(const Facts &next, Identity id) {
        const auto candidate = validate(next);
        if (!candidate.value) {
            return {};
        }
        return policy->publish(candidate.attempt, evidence(candidate, id, next));
    }
};

TEST_F(SessionPolicyS2, ColdPublicationAndIdenticalRenewalShareSnapshot) {
    ASSERT_NE(policy, nullptr);
    EXPECT_FALSE(policy->acquire());
    const auto first = publish(facts, 21);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->reference, "g1");
    EXPECT_EQ(policy->acquire(), first);
    const auto renewal = validate(facts);
    const auto ack = evidence(renewal, 22, facts);
    EXPECT_EQ(policy->publish(renewal.attempt, ack), first);
    EXPECT_EQ(policy->acquire(), first);
    EXPECT_EQ(readiness->last, ack.value);
}

TEST_F(SessionPolicyS2, SuspensionBlocksNewPinsAndPreservesCompatiblePins) {
    ASSERT_NE(policy, nullptr);
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    auto change = facts;
    change.limits.result_bytes = 1;
    const auto candidate = validate(change);
    EXPECT_FALSE(policy->acquire());
    EXPECT_TRUE(policy->is_current(old));
    const auto next = policy->publish(candidate.attempt, evidence(candidate, 22, change));
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, "g2");
    EXPECT_EQ(next->facts.limits.result_bytes, 1u);
    EXPECT_EQ(old->facts.limits.result_bytes, facts.limits.result_bytes);
    EXPECT_FALSE(policy->is_current(old));
}

TEST_F(SessionPolicyS2, ActiveUnknownReadinessClosesAndCannotResurrectGeneration) {
    ASSERT_NE(policy, nullptr);
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    readiness->state = ReadinessState::unknown;
    EXPECT_FALSE(policy->acquire());
    EXPECT_FALSE(policy->is_current(old));
    EXPECT_EQ(readiness->last, 21u);
    readiness->state = ReadinessState::live;
    EXPECT_FALSE(policy->acquire());
    EXPECT_FALSE(policy->is_current(old));
    const auto restored = publish(facts, 22);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->reference, "g2");
    EXPECT_NE(old, restored);
}

TEST_F(SessionPolicyS2, DestroyedBoundReadinessCannotBeRebound) {
    ASSERT_NE(policy, nullptr);
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    readiness.reset();
    EXPECT_FALSE(policy->acquire());
    EXPECT_FALSE(policy->is_current(old));
    if (const auto attempt = policy->suspend()) {
        if (const auto candidate = policy->validate(*attempt, facts)) {
            EXPECT_FALSE(policy->publish(*attempt, evidence(*candidate, 22, facts)));
        }
    }
    EXPECT_FALSE(policy->acquire());
}

TEST_F(SessionPolicyS2, RevokedPendingEvidenceDoesNotConsumeTheLiveAttempt) {
    ASSERT_NE(policy, nullptr);
    const auto candidate = validate(facts);
    const auto first = evidence(candidate, 21, facts);
    policy->invalidate_readiness(first);
    EXPECT_FALSE(policy->publish(candidate.attempt, first));
    readiness->state = ReadinessState::unknown;
    EXPECT_FALSE(policy->publish(candidate.attempt, evidence(candidate, 22, facts)));
    readiness->state = ReadinessState::live;
    const auto accepted = policy->publish(candidate.attempt, evidence(candidate, 23, facts));
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->reference, "g1");
    EXPECT_FALSE(policy->publish(candidate.attempt, evidence(candidate, 24, facts)));
    EXPECT_EQ(policy->acquire(), accepted);
}

TEST_F(SessionPolicyS2, InvalidationNamesExactAcceptedAckEvenForSameGeneration) {
    ASSERT_NE(policy, nullptr);
    const auto a = validate(facts);
    const auto old_ack = evidence(a, 21, facts);
    const auto old = policy->publish(a.attempt, old_ack);
    ASSERT_NE(old, nullptr);
    const auto b = validate(facts);
    const auto fresh_ack = evidence(b, 22, facts);
    EXPECT_EQ(policy->publish(b.attempt, fresh_ack), old);
    policy->invalidate_readiness(old_ack);
    EXPECT_TRUE(policy->is_current(old));
    EXPECT_EQ(policy->acquire(), old);
    policy->invalidate_readiness(fresh_ack);
    EXPECT_FALSE(policy->is_current(old));
    EXPECT_FALSE(policy->acquire());
}

TEST_F(SessionPolicyS2, EndpointFormPassesDnsAndRefusesNumericOrHexLastLabel) {
    ASSERT_NE(policy, nullptr);
    ASSERT_NE(publish(facts, 21), nullptr);
    for (const auto origin : {"https://127.1", "https://2130706433", "https://0x7f.0.0.1",
                              "https://1.2.3.4.5", "https://example.0x7f", "https://example.0x",
                              "https://timer.example:443", "https://[::1]"}) {
        SCOPED_TRACE(origin);
        auto invalid = facts;
        invalid.origin = origin;
        const auto attempt = policy->suspend();
        ASSERT_TRUE(attempt);
        EXPECT_FALSE(policy->validate(*attempt, invalid));
        EXPECT_FALSE(policy->acquire());
    }
}

TEST_F(SessionPolicyS2, DrainBindsFoundationChangeAndExactChannelSet) {
    ASSERT_NE(policy, nullptr);
    auto old_pin = publish(facts, 21);
    ASSERT_NE(old_pin, nullptr);
    auto next = facts;
    ++next.foundation;
    constexpr std::array<Identity, 2> channels{31, 32};
    const auto candidate = validate(next, channels);
    const auto ack = evidence(candidate, 22, next);
    EXPECT_FALSE(policy->publish(candidate.attempt, ack));
    EXPECT_FALSE(policy->publish(candidate.attempt, ack, Drain{candidate, {31, 33}}));
    for (const auto wrong :
         {Candidate{{candidate.attempt.owner + 1, candidate.attempt.value}, candidate.value},
          Candidate{{candidate.attempt.owner, candidate.attempt.value + 1}, candidate.value},
          Candidate{candidate.attempt, candidate.value + 1}}) {
        EXPECT_FALSE(policy->publish(candidate.attempt, ack, Drain{wrong, {31, 32}}));
    }
    EXPECT_FALSE(policy->acquire());
    const auto accepted = policy->publish(candidate.attempt, ack, Drain{candidate, {31, 32}});
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->reference, "g2");
    EXPECT_FALSE(policy->is_current(old_pin));
    release(old_pin);
    EXPECT_FALSE(old_pin);
}

TEST_F(SessionPolicyS2, ColdExtraDrainRefusesAndFirstSuccessStillUsesG1) {
    ASSERT_NE(policy, nullptr);
    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    constexpr std::array<Identity, 1> extra{31};
    EXPECT_FALSE(policy->validate(*attempt, facts, extra));
    EXPECT_FALSE(policy->validate(*attempt, facts));
    const auto good = publish(facts, 21);
    ASSERT_NE(good, nullptr);
    EXPECT_EQ(good->reference, "g1");
}

TEST_F(SessionPolicyS2, WarmExtraDrainLatchesAndIdenticalSuccessAdvances) {
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    ASSERT_EQ(old->reference, "g1");
    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    constexpr std::array<Identity, 1> extra{31};
    EXPECT_FALSE(policy->validate(*attempt, facts, extra));
    EXPECT_FALSE(policy->is_current(old));
    const auto restored = publish(facts, 22);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->reference, "g2");
}

TEST_F(SessionPolicyS2, AbandonChangedCandidateFencesOldPinAndRestorationAdvances) {
    ASSERT_NE(policy, nullptr);
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    auto changed = facts;
    changed.limits.result_bytes = 3;
    const auto pending = validate(changed);
    EXPECT_TRUE(policy->is_current(old));
    EXPECT_TRUE(policy->abandon(pending.attempt));
    EXPECT_FALSE(policy->is_current(old));
    EXPECT_FALSE(policy->publish(pending.attempt, evidence(pending, 22, changed)));
    const auto restored = publish(facts, 23);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->reference, "g2");
    EXPECT_FALSE(policy->is_current(old));
}

TEST_F(SessionPolicyS2, LiveHandleVersusConsumedAndSingleUseValidation) {
    ASSERT_NE(policy, nullptr);
    const auto handle = policy->suspend();
    ASSERT_TRUE(handle);
    EXPECT_FALSE(policy->publish(*handle, evidence({*handle, 99}, 21, facts)));
    const auto validated = policy->validate(*handle, facts);
    ASSERT_TRUE(validated);
    auto replacement = facts;
    replacement.limits.result_bytes = 7;
    EXPECT_FALSE(policy->validate(*handle, replacement));
    const auto result = policy->publish(*handle, evidence(*validated, 22, facts));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->facts.limits.result_bytes, facts.limits.result_bytes);
    EXPECT_FALSE(policy->publish(*handle, evidence(*validated, 22, facts)));
    EXPECT_EQ(policy->acquire(), result);
}

TEST_F(SessionPolicyS2, RetentionBudgetCountsDistinctGenerationsAtPublish) {
    policy = Policy::construct(installation, {4, 4, 2}, readiness);
    ASSERT_NE(policy, nullptr);
    auto g1 = publish(facts, 21);
    ASSERT_NE(g1, nullptr);
    auto g1_alias = policy->acquire();
    EXPECT_EQ(g1_alias, g1);
    auto next = facts;
    next.limits.result_bytes = 2;
    auto g2 = publish(next, 22);
    ASSERT_NE(g2, nullptr);
    EXPECT_EQ(g2->reference, "g2");
    auto third = facts;
    third.limits.result_bytes = 3;
    const auto candidate = validate(third);
    const auto ack = evidence(candidate, 23, third);
    EXPECT_FALSE(policy->publish(candidate.attempt, ack));
    EXPECT_FALSE(policy->is_current(g2));
    release(g1);
    EXPECT_NE(g1_alias, nullptr);
    release(g1_alias);
    EXPECT_FALSE(policy->publish(candidate.attempt, ack));
    const auto accepted = publish(third, 24);
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->reference, "g3");
}

TEST_F(SessionPolicyS2, InventoryCapsArePerSnapshotAndRefuseDuringValidation) {
    EXPECT_FALSE(Policy::construct(installation, {0, 1, 1}, readiness));
    EXPECT_FALSE(Policy::construct(installation, {1, 0, 1}, readiness));
    EXPECT_FALSE(Policy::construct(installation, {1, 1, 0}, readiness));
    EXPECT_FALSE(Policy::construct(installation, budgets, {}));
    policy = Policy::construct(installation, {1, 1, 2}, readiness);
    ASSERT_NE(policy, nullptr);
    const auto exact = publish(facts, 21);
    ASSERT_NE(exact, nullptr);
    for (bool routes : {true, false}) {
        auto excess = facts;
        if (routes) {
            excess.routes.push_back({"/other", "POST", AccountTransportPurpose::protected_access});
        } else {
            excess.peers.push_back("127.0.0.2");
        }
        const auto attempt = policy->suspend();
        ASSERT_TRUE(attempt);
        EXPECT_FALSE(policy->validate(*attempt, excess));
        EXPECT_FALSE(policy->validate(*attempt, facts));
    }
}

TEST_F(SessionPolicyS2, StopClosesAndReplacementCannotReauthorizeSameGeneration) {
    ASSERT_NE(policy, nullptr);
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    const auto pending = validate(facts);
    policy->stop();
    EXPECT_FALSE(policy->is_current(old));
    EXPECT_FALSE(policy->acquire());
    EXPECT_FALSE(policy->publish(pending.attempt, evidence(pending, 22, facts)));
    auto replacement_readiness = std::make_shared<SimulatedReadiness>();
    readiness = replacement_readiness;
    policy = Policy::construct(installation, budgets, replacement_readiness);
    ASSERT_NE(policy, nullptr);
    const auto next = publish(facts, 23);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, old->reference);
    EXPECT_NE(next->owner, old->owner);
    EXPECT_FALSE(policy->is_current(old));
}

TEST_F(SessionPolicyS2, EachNonLiveReadinessStateRefusesPendingAndActive) {
    for (const auto non_live :
         {ReadinessState::expired, ReadinessState::revoked, ReadinessState::contradictory,
          ReadinessState::unknown, ReadinessState::unavailable}) {
        SCOPED_TRACE(static_cast<int>(non_live));
        readiness->states.clear();
        policy = Policy::construct(installation, budgets, readiness);
        ASSERT_NE(policy, nullptr);
        const auto candidate = validate(facts);
        const auto ack = evidence(candidate, 21, facts);
        readiness->states[21] = non_live;
        EXPECT_FALSE(policy->publish(candidate.attempt, ack));
        const auto old = policy->publish(candidate.attempt, evidence(candidate, 22, facts));
        ASSERT_NE(old, nullptr);
        readiness->states[22] = non_live;
        EXPECT_FALSE(policy->acquire());
        EXPECT_FALSE(policy->is_current(old));
    }
}

TEST_F(SessionPolicyS2, LastGenerationIsRetainableButCannotWrap) {
    constexpr auto last = std::numeric_limits<std::uint64_t>::max();
    policy = Policy::seed_for_test(installation, budgets, readiness, last, facts,
                                   evidence({{installation.identity, 41}, 42}, 21, facts));
    ASSERT_NE(policy, nullptr);
    const auto pin = policy->acquire();
    ASSERT_NE(pin, nullptr);
    EXPECT_EQ(pin->reference, "g18446744073709551615");
    EXPECT_EQ(publish(facts, 22), pin);
    auto changed = facts;
    changed.limits.result_bytes = 1;
    EXPECT_FALSE(publish(changed, 23));
    EXPECT_FALSE(policy->is_current(pin));
    if (const auto attempt = policy->suspend()) {
        if (const auto candidate = policy->validate(*attempt, facts)) {
            EXPECT_FALSE(policy->publish(*attempt, evidence(*candidate, 24, facts)));
        }
    }
    EXPECT_FALSE(policy->acquire());
}

TEST_F(SessionPolicyS2, LatchedIdenticalRestoreCannotReuseLastGeneration) {
    constexpr auto last = std::numeric_limits<std::uint64_t>::max();
    policy = Policy::seed_for_test(installation, budgets, readiness, last, facts,
                                   evidence({{installation.identity, 41}, 42}, 21, facts));
    ASSERT_NE(policy, nullptr);
    const auto pin = policy->acquire();
    ASSERT_NE(pin, nullptr);
    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    ASSERT_TRUE(policy->abandon(*attempt));
    EXPECT_FALSE(publish(facts, 22));
    EXPECT_FALSE(policy->is_current(pin));
    EXPECT_FALSE(policy->acquire());
}

TEST_F(SessionPolicyS2, ReadinessMustMatchEveryCandidateBinding) {
    const auto candidate = validate(facts);
    const auto correct = evidence(candidate, 21, facts);
    using Change = void (*)(Readiness &);
    const std::array<Change, 7> changes{
        +[](Readiness &ack) { ++ack.candidate.attempt.owner; },
        +[](Readiness &ack) { ++ack.candidate.attempt.value; },
        +[](Readiness &ack) { ++ack.candidate.value; },
        +[](Readiness &ack) { ++ack.profile; },
        +[](Readiness &ack) { ack.configuration += "-other"; },
        +[](Readiness &ack) { ack.address = "127.0.0.2"; },
        +[](Readiness &ack) { ++ack.listener; },
    };
    for (const auto change : changes) {
        auto wrong = correct;
        change(wrong);
        EXPECT_FALSE(policy->publish(candidate.attempt, wrong));
        EXPECT_FALSE(policy->acquire());
    }
    const auto accepted = policy->publish(candidate.attempt, correct);
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->reference, "g1");
}

TEST_F(SessionPolicyS2, ActiveExpiryKeepsPendingAttemptAndLateExpiryCannotTouchG2) {
    const auto first = validate(facts);
    const auto old_ack = evidence(first, 21, facts);
    const auto old = policy->publish(first.attempt, old_ack);
    ASSERT_NE(old, nullptr);
    const auto pending = validate(facts);
    readiness->states[21] = ReadinessState::expired;
    EXPECT_FALSE(policy->is_current(old));
    EXPECT_FALSE(policy->acquire());
    const auto next = policy->publish(pending.attempt, evidence(pending, 22, facts));
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, "g2");
    policy->invalidate_readiness(old_ack);
    EXPECT_EQ(policy->acquire(), next);
    EXPECT_TRUE(policy->is_current(next));
    EXPECT_FALSE(policy->is_current(old));
}

TEST_F(SessionPolicyS2, EveryInvalidatingChangeRequiresAValidatedDrainSet) {
    using Change = void (*)(Facts &);
    const std::array<Change, 9> changes{
        +[](Facts &f) { f.address = "127.0.0.2"; },
        +[](Facts &f) { ++f.listener; },
        +[](Facts &f) { std::swap(f.routes[0], f.routes[1]); },
        +[](Facts &f) { ++f.native_artifact; },
        +[](Facts &f) { ++f.native_abi; },
        +[](Facts &f) { ++f.profile; },
        +[](Facts &f) { ++f.edge_artifact; },
        +[](Facts &f) { ++f.edge_config; },
        +[](Facts &f) { ++f.foundation; },
    };
    facts.routes.push_back({"/other", "POST", AccountTransportPurpose::protected_access});
    constexpr std::array<Identity, 2> channels{31, 32};
    for (std::size_t i = 0; i < changes.size(); ++i) {
        SCOPED_TRACE(i);
        policy = Policy::construct(installation, budgets, readiness);
        ASSERT_NE(policy, nullptr);
        ASSERT_NE(publish(facts, 21), nullptr);
        auto next = facts;
        changes[i](next);
        const auto missing = policy->suspend();
        ASSERT_TRUE(missing);
        EXPECT_FALSE(policy->validate(*missing, next));
        const auto candidate = validate(next, channels);
        const auto ack = evidence(candidate, 22, next);
        EXPECT_FALSE(policy->publish(candidate.attempt, ack));
        EXPECT_FALSE(policy->acquire());
        const auto accepted = policy->publish(candidate.attempt, ack, Drain{candidate, {31, 32}});
        ASSERT_NE(accepted, nullptr);
        EXPECT_EQ(accepted->reference, "g2");
    }
}

TEST_F(SessionPolicyS2, UnexpectedDrainAtPublishDoesNotConsumeAttempt) {
    const auto candidate = validate(facts);
    const auto ack = evidence(candidate, 21, facts);
    EXPECT_FALSE(policy->publish(candidate.attempt, ack, Drain{candidate, {31}}));
    EXPECT_FALSE(policy->acquire());
    const auto next = policy->publish(candidate.attempt, ack);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, "g1");
}

TEST_F(SessionPolicyS2, TrustedLimitAndFactValidationRefusesWithoutRepair) {
    using Change = void (*)(Facts &);
    const std::array<Change, 10> invalid{
        +[](Facts &f) { f.limits.result_bytes = 0; },
        +[](Facts &f) {
            f.limits.policy_reference_bytes = std::numeric_limits<std::uint64_t>::max();
        },
        +[](Facts &f) { f.origin = "https://" + std::string(1017, 'a'); },
        +[](Facts &f) { f.origin = "https://Timer.example"; },
        +[](Facts &f) { f.origin += '/'; },
        +[](Facts &f) { ++f.installation; },
        +[](Facts &f) { f.routes[0].purpose = static_cast<AccountTransportPurpose>(99); },
        +[](Facts &f) { f.mode = Facts::Mode::direct_plain; },
        +[](Facts &f) { f.mode = Facts::Mode::direct_tls; },
        +[](Facts &f) { f.limits.field_count = 0; },
    };
    for (std::size_t i = 0; i < invalid.size(); ++i) {
        SCOPED_TRACE(i);
        policy = Policy::construct(installation, budgets, readiness);
        ASSERT_NE(policy, nullptr);
        const auto old = publish(facts, 21);
        ASSERT_NE(old, nullptr);
        auto next = facts;
        invalid[i](next);
        const auto attempt = policy->suspend();
        ASSERT_TRUE(attempt);
        EXPECT_FALSE(policy->validate(*attempt, next));
        EXPECT_FALSE(policy->is_current(old));
        EXPECT_FALSE(policy->acquire());
    }
}

TEST_F(SessionPolicyS2, DirectModesAndNumericNonFinalDnsLabelsPublish) {
    for (const auto mode : {Facts::Mode::direct_plain, Facts::Mode::direct_tls}) {
        SCOPED_TRACE(static_cast<int>(mode));
        policy = Policy::construct(installation, budgets, readiness);
        ASSERT_NE(policy, nullptr);
        auto next = facts;
        next.mode = mode;
        next.forwarded_metadata = false;
        next.admitted_tls = mode == Facts::Mode::direct_tls;
        next.origin = "https://1.example";
        const auto pin = publish(next, 21);
        ASSERT_NE(pin, nullptr);
        EXPECT_EQ(pin->reference, "g1");
    }
}

TEST_F(SessionPolicyS2, CompatibleEffectiveFactChangesAdvanceWithoutDrain) {
    using Change = void (*)(Facts &);
    const std::array<Change, 5> changes{
        +[](Facts &f) { ++f.logging; },
        +[](Facts &f) { ++f.isolation; },
        +[](Facts &f) { ++f.forwarding_grammar; },
        +[](Facts &f) { ++f.normalization; },
        +[](Facts &f) { f.configuration = "next configuration"; },
    };
    for (std::size_t i = 0; i < changes.size(); ++i) {
        SCOPED_TRACE(i);
        policy = Policy::construct(installation, budgets, readiness);
        ASSERT_NE(policy, nullptr);
        const auto old = publish(facts, 21);
        ASSERT_NE(old, nullptr);
        auto next = facts;
        changes[i](next);
        const auto current = publish(next, 22);
        ASSERT_NE(current, nullptr);
        EXPECT_EQ(current->reference, "g2");
    }
}

TEST_F(SessionPolicyS2, PositiveUndersizedLimitsPublishAtG1) {
    for (bool shrink_origin : {false, true}) {
        policy = Policy::construct(installation, budgets, readiness);
        ASSERT_NE(policy, nullptr);
        auto next = facts;
        next.limits.origin_bytes = next.origin.size() - (shrink_origin ? 1u : 0u);
        next.limits.policy_reference_bytes = 1;
        next.limits.result_bytes = 1;
        const auto accepted = publish(next, 21);
        ASSERT_NE(accepted, nullptr);
        EXPECT_EQ(accepted->reference, "g1");
        EXPECT_EQ(accepted->facts.limits.origin_bytes, next.limits.origin_bytes);
        EXPECT_EQ(accepted->facts.limits.policy_reference_bytes, 1u);
        EXPECT_EQ(accepted->facts.limits.result_bytes, 1u);
    }
}

TEST_F(SessionPolicyS2, UnboundOrSupersededHandleCannotTouchPublication) {
    const auto g1 = publish(facts, 21);
    ASSERT_NE(g1, nullptr);
    const Attempt unbound{installation.identity, 999};
    EXPECT_FALSE(policy->validate(unbound, facts));
    EXPECT_FALSE(policy->publish(unbound, evidence({unbound, 1}, 22, facts)));
    EXPECT_EQ(policy->acquire(), g1);
    auto change = facts;
    change.limits.result_bytes = 7;
    const auto superseded = validate(change);
    auto newer = facts;
    newer.limits.result_bytes = 8;
    const auto candidate = validate(newer);
    const auto g2 = policy->publish(candidate.attempt, evidence(candidate, 23, newer));
    ASSERT_NE(g2, nullptr);
    EXPECT_EQ(g2->reference, "g2");
    EXPECT_FALSE(policy->publish(superseded.attempt, evidence(superseded, 24, change)));
    EXPECT_EQ(policy->acquire(), g2);
}

TEST_F(SessionPolicyS2, IdenticalAbandonAndPendingRefreshDoNotLatch) {
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    const auto abandoned = validate(facts);
    EXPECT_TRUE(policy->abandon(abandoned.attempt));
    EXPECT_TRUE(policy->is_current(old));
    const auto pending = validate(facts);
    const auto ack = evidence(pending, 22, facts);
    readiness->states[22] = ReadinessState::unavailable;
    EXPECT_FALSE(policy->publish(pending.attempt, ack));
    EXPECT_TRUE(policy->is_current(old));
    EXPECT_EQ(policy->publish(pending.attempt, evidence(pending, 23, facts)), old);
}

TEST_F(SessionPolicyS2, UnvalidatedAbandonLatchesAndOverCapRestoreConsumesHandle) {
    policy = Policy::construct(installation, {4, 4, 1}, readiness);
    ASSERT_NE(policy, nullptr);
    auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    const auto abandoned = policy->suspend();
    ASSERT_TRUE(abandoned);
    EXPECT_TRUE(policy->abandon(*abandoned));
    EXPECT_FALSE(policy->is_current(old));
    const auto restore = validate(facts);
    const auto ack = evidence(restore, 22, facts);
    EXPECT_FALSE(policy->publish(restore.attempt, ack));
    release(old);
    EXPECT_FALSE(policy->publish(restore.attempt, ack));
    const auto next = publish(facts, 23);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, "g2");
}

TEST_F(SessionPolicyS2, ExpiredReadinessOverCapConsumesRestoreAttempt) {
    policy = Policy::construct(installation, {4, 4, 1}, readiness);
    ASSERT_NE(policy, nullptr);
    auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    readiness->states[21] = ReadinessState::expired;
    EXPECT_FALSE(policy->is_current(old));
    const auto restore = validate(facts);
    const auto ack = evidence(restore, 22, facts);
    EXPECT_FALSE(policy->publish(restore.attempt, ack));
    release(old);
    EXPECT_FALSE(policy->publish(restore.attempt, ack));
    const auto next = publish(facts, 23);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, "g2");
}

TEST_F(SessionPolicyS2, DuplicateExpectedDrainChannelsRefuseAndLatch) {
    const auto old = publish(facts, 21);
    ASSERT_NE(old, nullptr);
    auto next = facts;
    ++next.foundation;
    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    constexpr std::array<Identity, 2> duplicates{31, 31};
    EXPECT_FALSE(policy->validate(*attempt, next, duplicates));
    EXPECT_FALSE(policy->is_current(old));
}

} // namespace
