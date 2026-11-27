// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_policy_zero_candidate.hpp"

#include <gtest/gtest.h>

namespace {
using namespace subathon::server::session_policy_candidate;

class LiveReadiness final : public ReadinessProducer {
  public:
    ReadinessState query(const Readiness &) noexcept override { return ReadinessState::live; }
};

Readiness evidence(Candidate candidate, Identity id, const Facts &facts) {
    return {candidate, id, facts.profile, facts.listener, facts.configuration, facts.address};
}

class SessionPolicyZeroChannels : public ::testing::Test {
  protected:
    const std::shared_ptr<LiveReadiness> readiness = std::make_shared<LiveReadiness>();
    std::unique_ptr<Policy> policy = Policy::construct({11, 1}, {4, 4, 3}, readiness);
    Facts facts;

    Pin ordinary_publish(const Facts &next, Identity id) {
        const auto attempt = policy->suspend();
        if (!attempt)
            return {};
        const auto candidate = policy->validate(*attempt, next, std::span<const Identity>{});
        return candidate ? policy->publish(*attempt, evidence(*candidate, id, next)) : Pin{};
    }
};

TEST_F(SessionPolicyZeroChannels, EmptyCompletionPublishesAndMissingOrForeignCompletionCanRetry) {
    ASSERT_NE(policy, nullptr);
    const auto old = ordinary_publish(facts, 21);
    ASSERT_NE(old, nullptr);
    ASSERT_EQ(old->reference, "g1");
    auto next = facts;
    ++next.listener;

    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    const auto exact = validate_exact(*policy, *attempt, next, ExplicitEmptyAffectedChannels{});
    ASSERT_TRUE(exact);
    const auto ack = evidence(*exact, 22, next);
    EXPECT_FALSE(policy->publish(*attempt, ack));
    auto foreign = *exact;
    ++foreign.value;
    EXPECT_FALSE(policy->publish(*attempt, ack, Drain{foreign, {}}));
    EXPECT_FALSE(policy->publish(*attempt, ack, Drain{*exact, {31}}));
    EXPECT_FALSE(policy->acquire());

    const auto accepted = policy->publish(*attempt, ack, Drain{*exact, {}});
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->reference, "g2");
    EXPECT_EQ(policy->acquire(), accepted);
    EXPECT_FALSE(policy->is_current(old));
}

TEST_F(SessionPolicyZeroChannels, ColdExplicitEmptyIsExtraAndConsumesTheAttempt) {
    ASSERT_NE(policy, nullptr);
    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    EXPECT_FALSE(validate_exact(*policy, *attempt, facts, ExplicitEmptyAffectedChannels{}));
    EXPECT_FALSE(policy->validate(*attempt, facts));
    const auto first = ordinary_publish(facts, 21);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->reference, "g1");
}

TEST_F(SessionPolicyZeroChannels, CompatibleExplicitEmptyIsExtraAndLatches) {
    ASSERT_NE(policy, nullptr);
    const auto first = ordinary_publish(facts, 21);
    ASSERT_NE(first, nullptr);
    auto next = facts;
    ++next.logging;

    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    EXPECT_FALSE(validate_exact(*policy, *attempt, next, ExplicitEmptyAffectedChannels{}));
    EXPECT_FALSE(policy->is_current(first));
    const auto restored = ordinary_publish(facts, 22);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->reference, "g2");
}

TEST_F(SessionPolicyZeroChannels, InvalidatingLegacyEmptyIsMissingAndLatches) {
    ASSERT_NE(policy, nullptr);
    const auto old = ordinary_publish(facts, 21);
    ASSERT_NE(old, nullptr);
    auto next = facts;
    ++next.listener;

    const auto attempt = policy->suspend();
    ASSERT_TRUE(attempt);
    EXPECT_FALSE(policy->validate(*attempt, next, std::span<const Identity>{}));
    EXPECT_FALSE(policy->is_current(old));
    EXPECT_FALSE(validate_exact(*policy, *attempt, next, ExplicitEmptyAffectedChannels{}));
    const auto restored = ordinary_publish(facts, 22);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->reference, "g2");
}
} // namespace
