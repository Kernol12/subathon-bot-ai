// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rates_004_fixture;

TEST(OpenExchangeRatesUsage, O5ProviderReportAndLocalLedgerRemainSeparate) {
    Fixture f;
    f.select(fx::Profile::enterprise);
    const auto report = f.report(
        "sample",
        usage_frame(R"("requests_quota":100000,"requests_remaining":45476,"requests":53000,)"
                    R"("days_elapsed":20,"days_remaining":10,"daily_average":1.8)",
                    clock(10'000, january + 123),
                    R"("name":"Enterprise","quota":"100,000 requests/month",)"
                    R"("update_frequency":"30-minute","features":{"base":false})"));
    ASSERT_EQ(report.dispatch, fx::Dispatch::sent);
    const auto *sample = std::get_if<fx::UsageSample>(&report.outcome);
    ASSERT_NE(sample, nullptr);
    expect_finite(*sample, 100'000, 45'476);
    EXPECT_EQ(sample->requests, 53'000);
    EXPECT_EQ(sample->plan_label, "Enterprise");
    EXPECT_EQ(sample->quota_banner, "100,000 requests/month");
    EXPECT_EQ(sample->refresh_interval_ms, 1'800'000);
    EXPECT_EQ(sample->account, fx::AccountState::active);
    EXPECT_EQ(sample->selection, f.owner.setup.selection);
    EXPECT_EQ(sample->base, false);
    EXPECT_EQ(sample->days_elapsed, 20);
    EXPECT_EQ(sample->days_remaining, 10);
    ASSERT_TRUE(sample->daily_average);
    EXPECT_EQ(sample->daily_average->original(), "1.8");
    EXPECT_EQ(sample->daily_average->value(), core::Exact(9) / 5);
    EXPECT_EQ(sample->receipt, clock(10'000, january + 123));
    ASSERT_EQ(f.transport.calls.size(), 1U);
    expect_rate_route(f.transport.calls.front().admission, "/api/usage.json");

    f.owner.intent.now = clock(11'000);
    const auto started = f.start(latest("leader"));
    f.owner.intent.kind = fx::IntentKind::manual;
    f.owner.intent.now = clock(12'000);
    const auto follower = f.start(latest("follower"), true);
    EXPECT_EQ(follower.admission.identity, started.admission.identity);
    ASSERT_EQ(f.transport.calls.size(), 2U);
    const auto rates_reply =
        f.finish(started, frame(R"({"base":"USD","rates":{"EUR":0.9000},"timestamp":1767225600})"));
    const auto *rates = std::get_if<fx::Rates>(&rates_reply.outcome);
    ASSERT_NE(rates, nullptr);
    const auto &data = rates->observation.data();
    ASSERT_EQ(data.quotes.size(), 1U);
    expect_quote(data.quotes.front(), "USD", "EUR", "0.9000", 9, 10);
    expect_times(data.context, "1767225600", january, {});

    f.owner.intent.now = clock(13'000);
    const auto timeout = f.failure(f.start(usage("timed-out")), fx::TransportFailureKind::timeout,
                                   fx::Dispatch::possibly_sent, clock(14'000));
    expect_refusal(timeout, fx::FailureKind::timeout);

    f.owner.intent.now = clock(130'000);
    const auto view = f.view();
    expect_counts(view.total, 3, 1, 1);
    expect_counts(view.since_sample, 2, 1, 1);
    EXPECT_EQ(f.transport.calls.size(), 3U);
    ASSERT_TRUE(view.last_usable_quota);
    expect_finite(*view.last_usable_quota, 100'000, 45'476);
    EXPECT_EQ(view.last_usable_quota->receipt, clock(10'000, january + 123));
    EXPECT_EQ(view.quota_age_ms, 120'000);
    EXPECT_FALSE(view.quota_reset_utc_ms);
    ASSERT_TRUE(view.latest_usage_outcome);
    const auto *failure = std::get_if<fx::Refusal>(&*view.latest_usage_outcome);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, fx::FailureKind::timeout);
}

TEST(OpenExchangeRatesUsage, O5OtherAndAbsentCadenceTokensAreUnavailable) {
    const std::string_view plan_fields[] = {
        "", R"("update_frequency":"3600s")", R"("update_frequency":"30-Minute")",
        R"("update_frequency":"30-minutes")", R"("update_frequency":1800000)"};
    for (auto fields : plan_fields) {
        SCOPED_TRACE(fields);
        Fixture f;
        f.select(fx::Profile::enterprise);
        const auto completed = f.report(
            "cadence-unknown", usage_frame(R"("requests_quota":100000,"requests_remaining":45476)",
                                           clock(10), fields));
        const auto *sample = std::get_if<fx::UsageSample>(&completed.outcome);
        ASSERT_NE(sample, nullptr);
        EXPECT_FALSE(sample->refresh_interval_ms);
        expect_finite(*sample, 100'000, 45'476);
        expect_counts(f.view().total, 1, 0);
    }
}

TEST(OpenExchangeRatesUsage, O6UnusableQuotaOrUnverifiedEchoPreservesUnlimitedSample) {
    enum class FollowUp { half_sentinel, negative_remaining, altered_case, missing_echo };
    for (const auto next : {FollowUp::half_sentinel, FollowUp::negative_remaining,
                            FollowUp::altered_case, FollowUp::missing_echo}) {
        SCOPED_TRACE(static_cast<int>(next));
        Fixture f;
        f.select(fx::Profile::unlimited);
        const auto initial = f.report(
            "unlimited", usage_frame(R"("requests_quota":-1,"requests_remaining":-1,"requests":25)",
                                     clock(20'000)));
        const auto *sample = std::get_if<fx::UsageSample>(&initial.outcome);
        ASSERT_NE(sample, nullptr);
        EXPECT_TRUE(std::holds_alternative<fx::UnlimitedQuota>(sample->monthly_quota));
        EXPECT_EQ(sample->requests, 25);

        f.owner.intent.now = clock(25'000);
        const auto started = f.start(usage("later"));
        if (next == FollowUp::half_sentinel || next == FollowUp::negative_remaining) {
            const auto fields = next == FollowUp::half_sentinel
                                    ? R"("requests_quota":-1)"
                                    : R"("requests_quota":10000,"requests_remaining":-2)";
            const auto completed = f.complete_usage(started, usage_frame(fields, clock(25'000)));
            const auto *unusable = std::get_if<fx::UsageSample>(&completed.outcome);
            ASSERT_NE(unusable, nullptr);
            EXPECT_TRUE(std::holds_alternative<std::monostate>(unusable->monthly_quota));
            EXPECT_NE(std::ranges::find(unusable->issues, fx::UsageIssue::invalid_quota),
                      unusable->issues.end());
            const auto view = f.view();
            ASSERT_TRUE(view.latest_usage_outcome);
            ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(*view.latest_usage_outcome));
            EXPECT_TRUE(std::holds_alternative<std::monostate>(
                std::get<fx::UsageSample>(*view.latest_usage_outcome).monthly_quota));
        } else {
            auto raw = finite(1000, 0, clock(25'000));
            if (next == FollowUp::altered_case) {
                auto altered = synthetic_app_id();
                altered.front() = 'A';
                add_echo(raw, "\"" + altered + "\"");
            }
            const auto reply = f.finish(started, std::move(raw));
            expect_refusal(reply, next == FollowUp::altered_case
                                      ? fx::FailureKind::credential_echo_mismatch
                                      : fx::FailureKind::credential_echo_unverified);
            EXPECT_EQ(reply.dispatch, fx::Dispatch::sent);
        }
        const auto view = f.view();
        ASSERT_TRUE(view.last_usable_quota);
        EXPECT_TRUE(
            std::holds_alternative<fx::UnlimitedQuota>(view.last_usable_quota->monthly_quota));
        EXPECT_EQ(view.last_usable_quota->requests, 25);
        EXPECT_EQ(view.last_usable_quota->receipt, clock(20'000));
        EXPECT_EQ(view.quota_age_ms, 5'000);
        EXPECT_FALSE(view.quota_reset_utc_ms);
        EXPECT_TRUE(view.holds.empty());
        expect_counts(view.total, 2, 0);
        expect_counts(view.since_sample, 1, 0);
    }
}

TEST(OpenExchangeRatesUsage, UsageOptionalFieldsKeepTheirIndependentValidity) {
    Fixture f;
    f.select(fx::Profile::developer);
    auto report = f.report(
        "metadata",
        usage_frame(R"("requests_quota":100,"requests_remaining":101,"requests":5,)"
                    R"("days_elapsed":32,"days_remaining":0,"daily_average":-0.01)",
                    clock(400, january), R"("features":{"base":"true","symbols":true})", "ACTIVE"));
    const auto *sample = std::get_if<fx::UsageSample>(&report.outcome);
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(sample->account, fx::AccountState::unknown);
    EXPECT_FALSE(sample->base);
    EXPECT_EQ(sample->symbols, true);
    EXPECT_FALSE(sample->days_elapsed);
    EXPECT_EQ(sample->days_remaining, 0);
    EXPECT_FALSE(sample->daily_average);
    EXPECT_FALSE(sample->refresh_interval_ms);
    EXPECT_FALSE(sample->plan_label);
    EXPECT_FALSE(sample->quota_banner);
    EXPECT_EQ(sample->requests, 5);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(sample->monthly_quota));
    EXPECT_FALSE(f.view().last_usable_quota);
    EXPECT_FALSE(f.view().quota_reset_utc_ms);
    expect_counts(f.view().total, 1, 0);
}

TEST(OpenExchangeRatesUsage, StaleSelectionReplyCannotMoveSampleOrSinceReportBoundary) {
    Fixture f;
    f.select(fx::Profile::developer);
    const auto older = f.start(usage("older"));
    // A new selection revision lets a new operation reserve without following the first.
    f.owner.setup.selection.configuration_ref = "config-B";
    f.install();
    const auto newer = f.start(usage("newer"));
    ASSERT_LT(older.admission.identity.reservation_order,
              newer.admission.identity.reservation_order);
    f.complete_usage(newer, finite(1000, 12, clock(2000, january + 2000)));
    f.owner.intent.now = clock(2100, january + 2100);
    f.exchange(latest("after-report"), frame(rate_body));
    f.complete_usage(older, finite(1000, 0, clock(2200, january + 2200)));
    f.owner.intent.now = clock(9999, january + 2600, "different-timeline");
    auto view = f.view();
    ASSERT_TRUE(view.last_usable_quota);
    expect_finite(*view.last_usable_quota, 1000, 12);
    EXPECT_EQ(view.quota_age_ms, 600);
    EXPECT_TRUE(view.holds.empty());
    expect_counts(view.total, 3, 1);
    // The one post-sample rate is metered. The old reservation cannot re-anchor the boundary.
    EXPECT_EQ(view.since_sample.metered, 1);
    EXPECT_EQ(view.last_usable_quota->receipt, clock(2000, january + 2000));
    f.owner.intent.now = clock(9999, january + 1999, "different-timeline");
    EXPECT_FALSE(f.view().quota_age_ms);
}

} // namespace
