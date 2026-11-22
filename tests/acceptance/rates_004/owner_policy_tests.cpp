// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rates_004_fixture;

TEST(OpenExchangeRatesOwner, O7QuotaHoldExemptsOnlyOrdinarilyEligibleUsage) {
    Fixture f;
    f.select(fx::Profile::developer);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("zero",
                 usage_frame(R"("requests_quota":10000,"requests_remaining":0,"days_remaining":2)",
                             clock(1000)))
            .outcome));
    expect_hold(f.view(), fx::HoldKind::quota);
    EXPECT_FALSE(f.view().quota_reset_utc_ms);
    expect_counts(f.view().total, 1, 0);
    f.owner.intent.kind = fx::IntentKind::manual;
    f.blocked(latest("held-latest"), fx::FailureKind::quota_hold);

    f.owner.setup.display_allowance = 50'000;
    f.install();
    f.owner.intent.qualified_utc_day = "2026-01-01";
    f.blocked(history("held-history"), fx::FailureKind::quota_hold);
    EXPECT_EQ(f.view().display_allowance, 50'000);
    ASSERT_TRUE(f.view().last_usable_quota);
    expect_finite(*f.view().last_usable_quota, 10'000, 0);

    f.owner.intent.kind = fx::IntentKind::automatic;
    f.owner.intent.due = false;
    f.owner.intent.now = clock(4999);
    f.blocked(usage("early"), fx::FailureKind::not_due);
    f.owner.intent.due = true;
    f.owner.intent.now = clock(5000);
    const auto due = f.start(usage("due"));
    f.owner.intent.kind = fx::IntentKind::manual;
    f.owner.intent.now = clock(5001);
    const auto follower = f.start(usage("follow-due"), true);
    EXPECT_EQ(follower.admission.identity, due.admission.identity);
    ASSERT_EQ(f.transport.calls.size(), 2U);

    f.owner.intent.now = clock(5020);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.complete_usage(due, finite(10'000, 8, clock(5020))).outcome));
    {
        auto renewed = f.view();
        EXPECT_TRUE(renewed.holds.empty());
        EXPECT_FALSE(renewed.quota_reset_utc_ms);
        EXPECT_EQ(renewed.quota_age_ms, 0);
        ASSERT_TRUE(renewed.last_usable_quota);
        expect_finite(*renewed.last_usable_quota, 10'000, 8);
        EXPECT_EQ(renewed.last_usable_quota->receipt, clock(5020));
        expect_counts(renewed.total, 2, 0);
        expect_counts(renewed.since_sample, 0, 0);
    }

    f.owner.intent.now = clock(5030);
    const auto resumed = f.exchange(latest("resumed"), frame(rate_body));
    const auto *rates = std::get_if<fx::Rates>(&resumed.outcome);
    ASSERT_NE(rates, nullptr);
    ASSERT_EQ(rates->observation.data().quotes.size(), 1U);
    expect_quote(rates->observation.data().quotes.front(), "USD", "EUR", "0.9000", 9, 10);
    expect_times(rates->observation.data().context, {}, {}, {});
    const auto view = f.view();
    ASSERT_TRUE(view.last_usable_quota);
    expect_finite(*view.last_usable_quota, 10'000, 8);
    expect_counts(view.total, 3, 1);
    expect_counts(view.since_sample, 1, 1);
}

TEST(OpenExchangeRatesOwner, O8CredentialHoldRequiresOwnerQualifiedCorrection) {
    Fixture f;
    f.select();
    expect_refusal(f.exchange(usage("bad-credential"), error("invalid_app_id", 401)),
                   fx::FailureKind::credential_rejected, 401);
    const auto rejected = hold(f.view(), fx::HoldKind::credential);
    f.blocked(latest("held-rate"), fx::FailureKind::credential_hold);
    f.blocked(usage("held-usage"), fx::FailureKind::credential_hold);

    f.owner.setup.selection.configuration_ref = "config-B";
    f.install();
    f.owner.intent.kind = fx::IntentKind::requalification;
    f.blocked(usage("revision-only"), fx::FailureKind::credential_hold);
    expect_counts(f.view().total, 1, 0);

    const auto old_selection = f.owner.setup.selection;
    f.owner.setup.selection.credential_revision = "credential-corrected";
    f.install(corrected_app_id());
    f.owner.intent.now = clock(8000);
    f.owner.intent.authorities = {
        {rejected, old_selection, fx::ProbeCause::corrected_credential, "wrong-selection"}};
    f.blocked(usage("wrong-selection"), fx::FailureKind::credential_hold);

    f.owner.intent.authorities = {
        {rejected, f.owner.setup.selection, fx::ProbeCause::corrected_credential, "correction"}};
    const auto result = f.report("corrected", finite(1000, 10, clock(8000)), corrected_app_id());
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(result.outcome));
    EXPECT_EQ(std::get<fx::UsageSample>(result.outcome).account, fx::AccountState::active);
    const auto view = f.view();
    EXPECT_TRUE(view.holds.empty());
    ASSERT_TRUE(view.last_usable_quota);
    EXPECT_EQ(view.last_usable_quota->selection, f.owner.setup.selection);
    expect_finite(*view.last_usable_quota, 1000, 10);
    expect_counts(view.total, 2, 0);
}

TEST(OpenExchangeRatesOwner, O8RouteDenialSurvivesAnotherLanesSuccessfulUsage) {
    Fixture f;
    f.select();
    f.owner.intent.qualified_utc_day = "2024-03-01";
    expect_refusal(f.exchange(history("denied"), error("not_allowed", 429)),
                   fx::FailureKind::route_denied, 429);
    const auto denied = hold(f.view(), fx::HoldKind::route_denial);
    EXPECT_EQ(denied.denied_operation, fx::Operation::historical_day);
    f.blocked(history("still-denied"), fx::FailureKind::route_hold);

    f.owner.intent.now = clock(9000);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("separate-lane", finite(1000, 20, clock(9000))).outcome));
    const auto view = f.view();
    ASSERT_TRUE(view.last_usable_quota);
    expect_finite(*view.last_usable_quota, 1000, 20);
    EXPECT_EQ(view.holds, (std::vector{denied}));
    expect_counts(view.total, 2, 1);
    f.blocked(history("denial-persists"), fx::FailureKind::route_hold);
}

TEST(OpenExchangeRatesOwner, O9SupersededUsageCanOnlySettleItsOwnDispatch) {
    Fixture f;
    f.select(fx::Profile::developer);
    const auto s1 = f.start(usage("s1"));
    f.owner.setup.selection.configuration_ref = "config-B";
    f.install();
    f.owner.intent.now = clock(10'000);
    const auto s2 = f.start(usage("s2"));
    ASSERT_LT(s1.admission.identity.reservation_order, s2.admission.identity.reservation_order);

    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.complete_usage(s2, finite(10'000, 0, clock(10'000))).outcome));
    f.owner.intent.now = clock(11'000);
    f.complete_usage(s1, finite(10'000, 99, clock(11'000)));

    const auto view = f.view();
    expect_counts(view.total, 2, 0);
    ASSERT_TRUE(view.last_usable_quota);
    EXPECT_EQ(view.last_usable_quota->selection, s2.admission.identity.selection);
    EXPECT_EQ(view.last_usable_quota->receipt, clock(10'000));
    expect_finite(*view.last_usable_quota, 10'000, 0);
    ASSERT_TRUE(view.latest_usage_outcome);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(*view.latest_usage_outcome));
    expect_finite(std::get<fx::UsageSample>(*view.latest_usage_outcome), 10'000, 0);
    EXPECT_EQ(view.quota_age_ms, 1000);
    f.blocked(latest("quota-still-held"), fx::FailureKind::quota_hold);
}

TEST(OpenExchangeRatesOwner, O10RelativeRetryAfterOutlastsTheFirstManualRetryDelay) {
    Fixture f;
    f.select();
    const auto limited =
        f.exchange(latest("limited"), frame("", clock(10'000), 429, {{"Retry-After", "90"}}));
    expect_refusal(limited, fx::FailureKind::rate_limited, 429);
    const auto &retry = std::get<fx::Refusal>(limited.outcome).retry;
    EXPECT_EQ(retry.relative_delay_ms, 90'000);
    EXPECT_EQ(retry.receipt_elapsed, clock(10'000).elapsed);
    EXPECT_FALSE(retry.absolute_deadline_utc_ms);
    expect_counts(f.view().total, 1, 1);
    EXPECT_FALSE(f.view().last_usable_quota);
    EXPECT_TRUE(f.view().holds.empty());

    f.owner.intent.kind = fx::IntentKind::manual;
    f.owner.intent.now = clock(99'999);
    f.blocked(latest("early-manual"), fx::FailureKind::retry_wait);
    f.owner.intent.now = clock(100'000, {}, "unqualified-timeline");
    f.blocked(latest("wrong-clock"), fx::FailureKind::retry_wait);
    f.owner.intent.now = clock(100'000);
    f.start(latest("boundary-manual"));
    EXPECT_EQ(f.transport.calls.size(), 2U);
}

TEST(OpenExchangeRatesOwner, AbsoluteRetryAfterRequiresQualifiedUtcAndCannotShortenBackoff) {
    Fixture f;
    f.select();
    const auto limited =
        f.exchange(latest("absolute"), frame("", clock(1000, january), 429,
                                             {{"Retry-After", "Thu, 01 Jan 2026 00:00:30 GMT"}}));
    expect_refusal(limited, fx::FailureKind::rate_limited, 429);
    const auto &retry = std::get<fx::Refusal>(limited.outcome).retry;
    EXPECT_EQ(retry.absolute_deadline_utc_ms, january + 30'000);
    EXPECT_FALSE(retry.relative_delay_ms);
    f.owner.intent.kind = fx::IntentKind::manual;
    f.owner.intent.now = clock(31'000, january + 30'000);
    f.blocked(latest("ordinary-backoff"), fx::FailureKind::retry_wait);
    f.owner.intent.now = clock(61'000);
    f.blocked(latest("wall-clock-unknown"), fx::FailureKind::retry_wait);
    f.owner.intent.now = clock(61'000, january + 60'000);
    f.start(latest("both-qualified"));
    EXPECT_EQ(f.transport.calls.size(), 2U);
}

TEST(OpenExchangeRatesOwner, O12AccessNeedsIndependentRestorationAndMatchingActiveUsage) {
    Fixture f;
    f.select();
    expect_refusal(f.exchange(latest("access"), error("access_restricted", 429)),
                   fx::FailureKind::provider_access_restricted, 429);
    const auto restricted = hold(f.view(), fx::HoldKind::provider_access);
    EXPECT_FALSE(f.view().last_usable_quota);
    EXPECT_FALSE(f.view().quota_reset_utc_ms);
    expect_counts(f.view().total, 1, 1);
    f.blocked(latest("ordinary-latest"), fx::FailureKind::access_hold);
    f.blocked(usage("ordinary-usage"), fx::FailureKind::access_hold);
    f.owner.setup.selection.configuration_ref = "config-B";
    f.install();
    f.owner.intent.kind = fx::IntentKind::requalification;
    f.blocked(usage("revision-only"), fx::FailureKind::access_hold);

    f.owner.intent.authorities = {
        {restricted, f.owner.setup.selection, fx::ProbeCause::restored_access, "account-restored"}};
    f.owner.intent.now = clock(12'000);
    const auto restored = f.report("restored", finite(1000, 5, clock(12'000)));
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(restored.outcome));
    const auto view = f.view();
    EXPECT_TRUE(view.holds.empty());
    ASSERT_TRUE(view.last_usable_quota);
    EXPECT_EQ(view.last_usable_quota->receipt, clock(12'000));
    expect_finite(*view.last_usable_quota, 1000, 5);
    EXPECT_EQ(view.quota_age_ms, 0);
    expect_counts(view.total, 2, 1);
}

TEST(OpenExchangeRatesOwner, O13PreHoldReservationCannotQualifyQuotaRenewal) {
    Fixture f;
    f.select();
    const auto metered = f.start(latest("before-timeout"));
    const auto pre_hold = f.start(usage("before-quota-hold"));
    expect_refusal(
        f.failure(metered, fx::TransportFailureKind::timeout, fx::Dispatch::sent, clock(12'000)),
        fx::FailureKind::timeout);
    f.owner.intent.now = clock(12'500);
    f.owner.exhaustion = fx::ExhaustionEvidence{f.owner.setup.selection, "qualified-exhaustion"};
    f.adapter.observe_exhaustion();
    const auto held = hold(f.view(), fx::HoldKind::quota);

    f.owner.intent.now = clock(13'000);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.complete_usage(pre_hold, finite(1000, 5, clock(13'000))).outcome));
    {
        const auto view = f.view();
        EXPECT_EQ(view.holds, (std::vector{held}));
        ASSERT_TRUE(view.last_usable_quota);
        expect_finite(*view.last_usable_quota, 1000, 5);
        EXPECT_EQ(view.last_usable_quota->receipt, clock(13'000));
        expect_counts(view.total, 2, 1);
        EXPECT_FALSE(view.quota_reset_utc_ms);
    }

    f.owner.intent.kind = fx::IntentKind::manual;
    f.owner.intent.now = clock(14'000);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("post-hold", finite(1000, 4, clock(14'000))).outcome));
    const auto renewed = f.view();
    EXPECT_TRUE(renewed.holds.empty());
    ASSERT_TRUE(renewed.last_usable_quota);
    expect_finite(*renewed.last_usable_quota, 1000, 4);
    EXPECT_EQ(renewed.last_usable_quota->receipt, clock(14'000));
    expect_counts(renewed.total, 3, 1);
    EXPECT_FALSE(renewed.quota_reset_utc_ms);
    // Renewal clears quota only. The original metered lane is still in ordinary backoff.
    f.blocked(latest("retry-still-waiting"), fx::FailureKind::retry_wait);
}

TEST(OpenExchangeRatesOwner, ProbeAuthorityMustCoverEachHoldAndCanBeUsedOnce) {
    Fixture f;
    f.select();
    const auto access = f.start(latest("access-reply"));
    const auto credential = f.start(usage("credential-reply"));
    expect_refusal(f.finish(access, error("access_restricted", 429)),
                   fx::FailureKind::provider_access_restricted, 429);
    expect_refusal(f.finish(credential, error("invalid_app_id", 401)),
                   fx::FailureKind::credential_rejected, 401);
    const auto before = f.view();
    const auto access_hold = hold(before, fx::HoldKind::provider_access);
    const auto credential_hold = hold(before, fx::HoldKind::credential);
    f.owner.setup.selection.credential_revision = "credential-corrected";
    f.install(corrected_app_id());
    auto wrong_hold = access_hold;
    ++wrong_hold.generation;
    const auto current = f.owner.setup.selection;
    const fx::ProbeAuthority correction{
        credential_hold, current, fx::ProbeCause::corrected_credential, "actual-key-correction"};
    const fx::ProbeAuthority restoration{access_hold, current, fx::ProbeCause::restored_access,
                                         "actual-access-restoration"};
    f.owner.intent.kind = fx::IntentKind::requalification;
    f.owner.intent.authorities = {correction};
    f.blocked(usage("missing-access-authority"));
    f.owner.intent.authorities.push_back(
        {wrong_hold, current, fx::ProbeCause::restored_access, "wrong-hold"});
    f.blocked(usage("wrong-hold-authority"));
    f.owner.intent.authorities = {correction, restoration};
    f.owner.intent.route_authorized = false;
    f.blocked(usage("not-authorized"));
    f.owner.intent.route_authorized = true;
    f.owner.intent.applicable_owner_waits = fx::WaitEligibility::unknown;
    f.blocked(usage("owner-wait-unqualified"), fx::FailureKind::retry_wait);
    f.owner.intent.applicable_owner_waits = fx::WaitEligibility::eligible;

    const auto started = f.start(usage("probe"));
    f.owner.intent.authorities = {correction};
    f.blocked(usage("unauthorized-follower"));
    f.owner.intent.authorities = {correction, restoration};
    const auto follower = f.start(usage("authorized-follower"), true);
    EXPECT_EQ(follower.admission.identity, started.admission.identity);
    EXPECT_EQ(f.transport.calls.size(), 3U);
    // A complete usage response without a private matching echo cannot restore either hold.
    const auto unverified = f.finish(started, finite(1000, 8, clock(0)));
    expect_refusal(unverified, fx::FailureKind::credential_echo_unverified);
    const auto after = f.view();
    expect_hold(after, fx::HoldKind::provider_access);
    expect_hold(after, fx::HoldKind::credential);
    expect_counts(after.total, 3, 1);
    f.owner.intent.now = clock(4'000'000);
    f.blocked(usage("reuse-spent-bundle"));
    EXPECT_EQ(f.transport.calls.size(), 3U);
}

TEST(OpenExchangeRatesOwner, RestrictedPositiveUsageDoesNotClearQuotaHold) {
    Fixture f;
    f.select(fx::Profile::developer);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("zero", finite(1000, 0, clock(1000))).outcome));
    f.owner.intent.now = clock(2000);
    const auto restricted =
        f.report("restricted", finite(1000, 8, clock(2000), "access_restricted"));
    const auto *sample = std::get_if<fx::UsageSample>(&restricted.outcome);
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(sample->account, fx::AccountState::restricted);
    expect_finite(*sample, 1000, 8);
    const auto view = f.view();
    expect_hold(view, fx::HoldKind::quota);
    expect_hold(view, fx::HoldKind::provider_access);
    EXPECT_FALSE(view.quota_reset_utc_ms);
    expect_counts(view.total, 2, 0);
    f.blocked(usage("access-gates-exempt-lane"), fx::FailureKind::access_hold);
}

TEST(OpenExchangeRatesOwner, QualifiedNewQuotaScopeDoesNotInheritLedgerOrHolds) {
    Fixture f;
    f.select();
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("first-scope", finite(1000, 0, clock(1000))).outcome));
    expect_hold(f.view(), fx::HoldKind::quota);
    f.owner.setup.selection.quota_scope_ref = "qualified-other-quota";
    f.owner.setup.selection.configuration_ref = "other-config";
    f.owner.setup.selection.credential_revision = "other-credential";
    f.install(corrected_app_id());
    const auto view = f.view();
    EXPECT_TRUE(view.holds.empty());
    EXPECT_FALSE(view.last_usable_quota);
    expect_counts(view.total, 0, 0);
    ASSERT_TRUE(std::holds_alternative<fx::Rates>(
        f.exchange(latest("other-scope"), frame(rate_body)).outcome));
    expect_counts(f.view().total, 1, 1);
}

TEST(OpenExchangeRatesOwner, RequalificationDoesNotClearAnUnrelatedRouteDenial) {
    Fixture f;
    f.select();
    f.owner.intent.qualified_utc_day = "2024-03-01";
    expect_refusal(f.exchange(history("deny-history"), error("not_allowed", 429)),
                   fx::FailureKind::route_denied);
    expect_refusal(f.exchange(latest("access"), error("access_restricted", 403)),
                   fx::FailureKind::provider_access_restricted);

    const auto restriction = hold(f.view(), fx::HoldKind::provider_access);
    const auto denied_route = hold(f.view(), fx::HoldKind::route_denial);
    f.owner.intent.kind = fx::IntentKind::requalification;
    f.owner.intent.authorities = {
        {restriction, f.owner.setup.selection, fx::ProbeCause::restored_access, "access-restored"}};
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("active-again", finite(1000, 7, clock(1000))).outcome));
    EXPECT_EQ(f.view().holds, (std::vector{denied_route}));
    f.owner.intent.kind = fx::IntentKind::manual;
    f.blocked(history("history-still-denied"), fx::FailureKind::route_hold);

    f.owner.intent.kind = fx::IntentKind::requalification;
    f.owner.intent.authorities = {{denied_route, f.owner.setup.selection,
                                   fx::ProbeCause::restored_route, "history-restored"}};
    ASSERT_TRUE(std::holds_alternative<fx::Rates>(
        f.exchange(history("history-restored"), frame(free_history)).outcome));
    expect_counts(f.view().total, 4, 3);
}

TEST(OpenExchangeRatesOwner,
     ReusedRequestIdentityCannotDispatchDifferentIntentAndNotSentCostsZero) {
    Fixture f;
    f.select();
    const auto request = latest("request", {"GBP", "EUR"});
    const auto leader = f.start(request);
    f.blocked(latest("request", {"CHF"}), fx::FailureKind::request_conflict);
    f.owner.intent.kind = fx::IntentKind::manual;
    const auto follower = f.start(latest("canonical-follower", {"EUR", "GBP"}), true);
    EXPECT_EQ(follower.admission.identity, leader.admission.identity);
    expect_refusal(f.failure(leader, fx::TransportFailureKind::transport, fx::Dispatch::not_sent),
                   fx::FailureKind::transport_failure);
    expect_counts(f.view().total, 0, 0);
    EXPECT_EQ(f.transport.calls.size(), 1U);
}

} // namespace
