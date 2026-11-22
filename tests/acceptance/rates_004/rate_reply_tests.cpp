// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rates_004_fixture;

TEST(OpenExchangeRatesRates, O1LatestKeepsExactQuotesAndIndependentTimes) {
    Fixture f;
    f.select();
    const auto request = latest("latest", {"GBP", "JPY", "EUR", "CHF"});
    const auto started = f.start(request);
    expect_rate_route(started.admission, "/api/latest.json?symbols=CHF,EUR,GBP,JPY");
    EXPECT_EQ(started.admission.canonical_request.quotes,
              (std::vector<std::string>{"CHF", "EUR", "GBP", "JPY"}));
    ASSERT_EQ(f.transport.calls.size(), 1U);
    const auto &handoff = f.transport.sent(started);
    EXPECT_EQ(handoff.authorization.first, "Authorization");
    EXPECT_TRUE(handoff.authorization.second == "Token " + synthetic_app_id());

    const auto result = f.finish(
        started, frame(R"({"base":"USD","rates":{"GBP":0.8000,"USD":1,"CAD":1.25,"EUR":0.9000},)"
                       R"("timestamp":1767225600})",
                       {january + 123, {}}, 200, {{"Date", "Thu, 01 Jan 2026 00:00:02 GMT"}}));
    ASSERT_EQ(result.dispatch, fx::Dispatch::sent);
    const auto *rates = std::get_if<fx::Rates>(&result.outcome);
    ASSERT_NE(rates, nullptr);
    EXPECT_EQ(rates->operation, fx::Operation::latest);
    const auto &data = rates->observation.data();
    ASSERT_EQ(data.quotes.size(), 2U);
    expect_quote(data.quotes[0], "USD", "EUR", "0.9000", 9, 10);
    expect_quote(data.quotes[1], "USD", "GBP", "0.8000", 4, 5);
    EXPECT_EQ(rates->missing_quotes, (std::vector<std::string>{"CHF", "JPY"}));
    expect_times(data.context, "1767225600", january, january + 123);
    EXPECT_EQ(data.context.provider_ref, "open_exchange_rates");
    EXPECT_EQ(data.context.configuration_ref, "config-A");
    EXPECT_EQ(data.context.request_ref, request.request_ref);
    EXPECT_EQ(data.context.source_evidence_ref, request.evidence_ref);
    EXPECT_EQ(data.context.observation_ref, request.observation_ref);
    EXPECT_EQ(data.context.provider_snapshot_ref, std::nullopt);
    expect_counts(f.view().total, 1, 1);
}

TEST(OpenExchangeRatesRates, O2MissingTimeRemainsUnknown) {
    Fixture f;
    f.select();
    const auto result =
        f.exchange(latest("partial", {"EUR", "CHF"}),
                   frame(rate_body, clock(300), 200, {{"Date", "Thu, 01 Jan 2026 00:00:02 GMT"}}));
    const auto *rates = std::get_if<fx::Rates>(&result.outcome);
    ASSERT_NE(rates, nullptr);
    const auto &data = rates->observation.data();
    ASSERT_EQ(data.quotes.size(), 1U);
    expect_quote(data.quotes.front(), "USD", "EUR", "0.9000", 9, 10);
    EXPECT_EQ(rates->missing_quotes, (std::vector<std::string>{"CHF"}));
    expect_times(data.context, {}, {}, {});
    expect_counts(f.view().total, 1, 1);
}

TEST(OpenExchangeRatesRates, O2InvalidDocumentsChargeTheDispatchWithoutPublishing) {
    struct Case {
        std::string_view body;
        fx::FailureKind kind;
    };
    const Case cases[] = {
        {R"({"base":"USD","rates":{"EUR":0.9000,"EUR":0.91}})", fx::FailureKind::invalid_document},
        {R"({"base":"USD","rates":{"EUR":0.9000},"timestamp":9223372036854776})",
         fx::FailureKind::invalid_document},
        {R"({"base":"USD","rates":{"EUR":"0.9"}})", fx::FailureKind::invalid_document},
        {R"({"base":"USD","rates":{"EUR":0}})", fx::FailureKind::invalid_document},
        {R"({"base":"EUR","rates":{"GBP":0.9}})", fx::FailureKind::invalid_document},
        {R"({"base":"USD","rates":{"EUR":0.9000},"timestamp":1767225600.0})",
         fx::FailureKind::invalid_document},
        {R"({"base":"USD","rates":{"EUR":0.9,"USD":1.0001}})", fx::FailureKind::invalid_document},
        {R"({"base":"USD","rates":{"CHF":0.87}})", fx::FailureKind::missing_rates},
    };
    for (const auto &row : cases) {
        SCOPED_TRACE(row.body);
        Fixture f;
        f.select();
        const auto result = f.exchange(latest("bad"), frame(row.body));
        expect_refusal(result, row.kind);
        EXPECT_EQ(result.dispatch, fx::Dispatch::sent);
        EXPECT_EQ(f.view().latest, std::nullopt);
        expect_counts(f.view().total, 1, 1);
    }
}

TEST(OpenExchangeRatesRates, O3FreeHistoryFiltersLocallyAndRequiresClosedUtcDay) {
    Fixture f;
    f.select();
    f.owner.intent.qualified_utc_day = "2024-03-01";
    const auto past = f.start(history("past"));
    expect_rate_route(past.admission, "/api/historical/2024-02-29.json");
    const auto result = f.finish(past, frame(free_history, {1'709'251'200'123, {}}));
    const auto *rates = std::get_if<fx::Rates>(&result.outcome);
    ASSERT_NE(rates, nullptr);
    EXPECT_EQ(rates->operation, fx::Operation::historical_day);
    const auto &data = rates->observation.data();
    ASSERT_EQ(data.quotes.size(), 1U);
    expect_quote(data.quotes.front(), "USD", "EUR", "0.9225", 369, 400);
    EXPECT_TRUE(rates->missing_quotes.empty());
    expect_times(data.context, "1709251199", 1'709'251'199'000, 1'709'251'200'123);
    EXPECT_FALSE(f.view().latest);
    EXPECT_FALSE(f.view().next_latest_utc_ms);
    f.blocked(history("today", "USD", {"EUR"}, "2024-03-01"), fx::FailureKind::forbidden_feature);
    f.blocked(history("future", "USD", {"EUR"}, "2024-03-02"), fx::FailureKind::forbidden_feature);
    f.blocked(history("invalid-day", "USD", {"EUR"}, "2023-02-29"),
              fx::FailureKind::invalid_request);
    f.blocked(history("before-floor", "USD", {"EUR"}, "1998-12-31"),
              fx::FailureKind::forbidden_feature);
    f.blocked(history("alt-base", "EUR", {"GBP"}), fx::FailureKind::forbidden_feature);
    f.owner.intent.qualified_utc_day.reset();
    f.blocked(history("no-qualified-day"), fx::FailureKind::forbidden_feature);
    expect_counts(f.view().total, 1, 1);
}

TEST(OpenExchangeRatesRates, O4QualifiedPaidHistoryNeedsAnExactDayTimestamp) {
    Fixture f;
    f.select(fx::Profile::developer, true);
    f.owner.intent.qualified_utc_day = "2024-03-01";
    const auto past = f.start(history("qualified", "EUR", {"GBP"}));
    // Query order between distinct parameters is incidental. Symbol order is contractual.
    const auto &url = past.admission.url;
    EXPECT_TRUE(
        url == std::string(origin) + "/api/historical/2024-02-29.json?base=EUR&symbols=GBP" ||
        url == std::string(origin) + "/api/historical/2024-02-29.json?symbols=GBP&base=EUR");
    const auto result = f.finish(past, frame(paid_history, {1'709'251'200'999, {}}));
    const auto *rates = std::get_if<fx::Rates>(&result.outcome);
    ASSERT_NE(rates, nullptr);
    const auto &data = rates->observation.data();
    ASSERT_EQ(data.quotes.size(), 1U);
    expect_quote(data.quotes.front(), "EUR", "GBP", "0.85", 17, 20);
    expect_times(data.context, "1709164800", 1'709'164'800'000, 1'709'251'200'999);

    for (const auto bad_body : {R"({"base":"EUR","rates":{"GBP":0.85}})",
                                R"({"base":"EUR","rates":{"GBP":0.85},"timestamp":1709251200})"}) {
        SCOPED_TRACE(bad_body);
        Fixture branch;
        branch.select(fx::Profile::developer, true);
        branch.owner.intent.qualified_utc_day = "2024-03-01";
        expect_refusal(branch.exchange(history("unverified", "EUR", {"GBP"}),
                                       frame(bad_body, {1'709'251'200'999, {}})),
                       fx::FailureKind::history_unverified);
        expect_counts(branch.view().total, 1, 1);
        EXPECT_FALSE(branch.view().latest);
    }
}

TEST(OpenExchangeRatesRates, O4ApplicableBaseFalseNarrowsPreviouslyQualifiedHistory) {
    Fixture f;
    f.select(fx::Profile::developer, true);
    const auto report =
        f.report("base-false", usage_frame("", clock(900), R"("features":{"base":false})"));
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(report.outcome));
    EXPECT_EQ(std::get<fx::UsageSample>(report.outcome).base, false);
    f.owner.intent.qualified_utc_day = "2024-03-01";
    f.owner.intent.now = clock(901);
    f.blocked(history("base-denied", "EUR", {"GBP"}), fx::FailureKind::forbidden_feature);
    expect_counts(f.view().total, 1, 0);
}

TEST(OpenExchangeRatesRates, UsageClaimsCannotGrantUnqualifiedPaidFeaturesOrProfile) {
    Fixture paid;
    paid.select(fx::Profile::developer);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        paid.report("claims",
                    usage_frame("", clock(1), R"("features":{"base":true,"symbols":true})"))
            .outcome));
    paid.owner.intent.now = clock(2);
    paid.owner.intent.qualified_utc_day = "2024-03-01";
    paid.blocked(latest("base-unproven", {"GBP"}, "EUR"), fx::FailureKind::forbidden_feature);
    expect_rate_route(paid.start(history("symbols-unproven")).admission,
                      "/api/historical/2024-02-29.json");

    Fixture unknown;
    unknown.owner.setup.profile_qualified = false;
    unknown.select(fx::Profile::unknown);
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        unknown.report("metadata", usage_frame("", clock(1), R"("name":"Developer")")).outcome));
    unknown.blocked(latest("profile-unproven"), fx::FailureKind::forbidden_feature);
    expect_counts(unknown.view().total, 1, 0);
}

TEST(OpenExchangeRatesRates, O10RateMemberLimitAppliesBeforeProjection) {
    std::string body = R"({"base":"USD","rates":{"EUR":0.9)";
    for (int i = 0; i < 256; ++i) {
        const std::string code{static_cast<char>('A' + i / (26 * 26)),
                               static_cast<char>('A' + i / 26 % 26),
                               static_cast<char>('A' + i % 26)};
        body += ",\"" + code + "\":1";
    }
    body += "}}";
    Fixture f;
    f.select();
    expect_refusal(f.exchange(latest("over-limit"), frame(body)), fx::FailureKind::resource_limit);
    EXPECT_FALSE(f.view().latest);
    expect_counts(f.view().total, 1, 1);
}

TEST(OpenExchangeRatesRates, O11SymbolsFalseOmitsFilterButKeepsCanonicalProjection) {
    Fixture f;
    f.select();
    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("no-symbols", usage_frame("", clock(1000), R"("features":{"symbols":false})"))
            .outcome));
    f.owner.intent.now = clock(1001);
    const auto started = f.start(latest("unfiltered", {"GBP", "EUR"}));
    expect_rate_route(started.admission, "/api/latest.json");
    const auto result =
        f.finish(started, frame(R"({"base":"USD","rates":{"JPY":148.2,"GBP":0.80,"EUR":0.900},)"
                                R"("timestamp":1767225600})",
                                {january + 1000, {}}));
    const auto *rates = std::get_if<fx::Rates>(&result.outcome);
    ASSERT_NE(rates, nullptr);
    const auto &data = rates->observation.data();
    ASSERT_EQ(data.quotes.size(), 2U);
    expect_quote(data.quotes[0], "USD", "EUR", "0.900", 9, 10);
    expect_quote(data.quotes[1], "USD", "GBP", "0.80", 4, 5);
    EXPECT_TRUE(rates->missing_quotes.empty());
    expect_times(data.context, "1767225600", january, january + 1000);
    expect_counts(f.view().total, 2, 1);
}

TEST(OpenExchangeRatesRates, ReportedCadenceAndManualUnknownSourceDoNotReplaceAutomaticAnchor) {
    Fixture f;
    f.select(fx::Profile::enterprise);
    f.owner.intent.now = clock(1000, january + 1000);
    const auto automatic = f.exchange(
        latest("anchor"), frame(R"({"base":"USD","rates":{"EUR":0.9},"timestamp":1767225600})",
                                clock(1000, january + 1000)));
    ASSERT_TRUE(std::holds_alternative<fx::Rates>(automatic.outcome));
    constexpr auto next = january + 1000 + 43'200'000;
    EXPECT_EQ(f.view().next_latest_utc_ms, next);

    ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(
        f.report("cadence",
                 usage_frame("", clock(2000, january + 2000), R"("update_frequency":"30-minute")"))
            .outcome));
    f.owner.intent.kind = fx::IntentKind::manual;
    f.owner.intent.now = clock(3000, january + 3000);
    auto unknown_source =
        f.exchange(latest("manual"),
                   frame(R"({"base":"USD","rates":{"EUR":0.8}})", clock(3000, january + 3000)));
    ASSERT_TRUE(std::holds_alternative<fx::Rates>(unknown_source.outcome));
    const auto view = f.view();
    ASSERT_TRUE(view.latest);
    EXPECT_EQ(view.latest->data().context.observation_ref, "observation-anchor");
    EXPECT_EQ(view.next_latest_utc_ms, next);

    f.owner.intent.now = clock(4000, january + 4000);
    const auto newer =
        f.exchange(latest("newer-source"),
                   frame(R"({"base":"USD","rates":{"EUR":0.81},"timestamp":1767225601})",
                         clock(4000, january + 4000)));
    ASSERT_TRUE(std::holds_alternative<fx::Rates>(newer.outcome));
    const auto after_manual = f.view();
    ASSERT_TRUE(after_manual.latest);
    EXPECT_EQ(after_manual.latest->data().context.observation_ref, "observation-newer-source");
    EXPECT_EQ(after_manual.next_latest_utc_ms, next);
}

} // namespace
