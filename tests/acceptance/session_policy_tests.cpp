// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_policy_candidate.hpp"

#include <array>
#include <gtest/gtest.h>
#include <limits>

namespace {
using namespace subathon::server;
using namespace subathon::server::session_policy_candidate;
constexpr Owner installation{11, 1};
constexpr Budgets budgets{4, 4, 3};

class SimulatedReadiness final : public ReadinessProducer {
  public:
    ReadinessState state{ReadinessState::live};
    std::optional<Identity> last;
    ReadinessState query(const Readiness &request) noexcept override {
        last = request.value;
        return state;
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
    EXPECT_EQ(first->owner, installation.identity);
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
    const auto candidate = validate(facts);
    EXPECT_FALSE(policy->publish(candidate.attempt, evidence(candidate, 22, facts)));
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
    EXPECT_FALSE(policy->validate(*handle, facts));
    const auto result = policy->publish(*handle, evidence(*validated, 22, facts));
    ASSERT_NE(result, nullptr);
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
    policy = Policy::construct({12, 1}, budgets, replacement_readiness);
    ASSERT_NE(policy, nullptr);
    const auto next = publish(facts, 23);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->reference, old->reference);
    EXPECT_NE(next->owner, old->owner);
    EXPECT_FALSE(policy->is_current(old));
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
    EXPECT_FALSE(policy->suspend());
}

} // namespace
