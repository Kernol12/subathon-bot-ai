// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <gtest/gtest.h>
#include <string_view>
#include <subathon/server/open_exchange_rates.hpp>

namespace rates_004_fixture {
namespace fx = subathon::server::open_exchange_rates;
namespace wire = fx::private_transport;
namespace core = subathon::core;
using namespace std::chrono_literals;

constexpr std::string_view origin = "https://openexchangerates.org";
constexpr std::int64_t january = 1'767'225'600'000;
constexpr std::string_view rate_body = R"({"base":"USD","rates":{"EUR":0.9000}})";
constexpr std::string_view free_history =
    R"({"base":"USD","rates":{"EUR":0.9225,"CHF":0.87},"timestamp":1709251199})";
constexpr std::string_view paid_history =
    R"({"base":"EUR","rates":{"GBP":0.85},"timestamp":1709164800})";

inline std::string currency_code(int index) {
    return {static_cast<char>('A' + index / (26 * 26)), static_cast<char>('A' + index / 26 % 26),
            static_cast<char>('A' + index % 26)};
}

inline std::string synthetic_app_id() { return std::string(16, 'a') + std::string(16, '1'); }
inline std::string corrected_app_id() { return std::string(16, 'b') + std::string(16, '2'); }

inline fx::Clocks clock(std::int64_t tick, std::optional<std::int64_t> utc = std::nullopt,
                        std::string timeline = "fixture-timeline") {
    return {utc, fx::Elapsed{std::move(timeline), tick}};
}

inline fx::Request latest(std::string ref, std::vector<std::string> quotes = {"EUR"},
                          std::string base = "USD") {
    return {fx::Operation::latest, ref, "evidence-" + ref, "observation-" + ref, std::move(base),
            std::move(quotes),     {}};
}
inline fx::Request history(std::string ref, std::string base = "USD",
                           std::vector<std::string> quotes = {"EUR"},
                           std::string date = "2024-02-29") {
    return {fx::Operation::historical_day,
            ref,
            "evidence-" + ref,
            "observation-" + ref,
            std::move(base),
            std::move(quotes),
            std::move(date)};
}
inline fx::Request usage(std::string ref) {
    return {fx::Operation::usage, ref, "evidence-" + ref, {}, {}, {}, {}};
}

inline wire::RawFrame frame(std::string_view body, fx::Clocks receipt = {}, int status = 200,
                            fx::Headers headers = {}) {
    return {status,      std::move(headers), std::string(body),
            body.size(), body.size(),        std::move(receipt)};
}
inline wire::RawFrame usage_frame(std::string_view usage_fields, fx::Clocks receipt = {},
                                  std::string_view plan_fields = {},
                                  std::string_view account = "active",
                                  std::string_view extra_data = {}) {
    return frame(R"({"status":200,"data":{"status":")" + std::string(account) + R"(","plan":{)" +
                     std::string(plan_fields) + R"(},"usage":{)" + std::string(usage_fields) + "}" +
                     std::string(extra_data) + "}}",
                 std::move(receipt));
}
inline wire::RawFrame finite(std::int64_t quota, std::int64_t remaining, fx::Clocks receipt,
                             std::string_view account = "active") {
    return usage_frame(R"("requests_quota":)" + std::to_string(quota) +
                           R"(,"requests_remaining":)" + std::to_string(remaining),
                       std::move(receipt), {}, account);
}
inline void add_echo(wire::RawFrame &raw, std::string_view json_value) {
    // All fixture usage envelopes end with the data and envelope closing braces.
    const std::string field = R"(,"app_id":)" + std::string(json_value);
    raw.body.insert(raw.body.size() - 2, field);
    raw.decoded_bytes += field.size();
    raw.wire_bytes += field.size();
}

inline wire::RawFrame error(std::string_view code, int status, fx::Clocks receipt = {}) {
    return frame(R"({"error":true,"status":)" + std::to_string(status) + R"(,"message":")" +
                     std::string(code) + R"(","description":"private-fixture-error-detail"})",
                 std::move(receipt), status);
}

class SyntheticOwner final : public fx::OwnerPort {
  public:
    fx::SelectedSetup setup{
        {"fixture-installation", "fixture-quota", "config-A", "credential-A", fx::Profile::free},
        true,
        {},
        {},
        true};
    fx::OwnerIntent intent{setup.selection,
                           fx::IntentKind::automatic,
                           true,
                           true,
                           clock(0),
                           {},
                           4s,
                           {},
                           fx::WaitEligibility::eligible};
    std::optional<fx::ExhaustionEvidence> exhaustion;

    fx::SelectedSetup selected_setup() const override { return setup; }
    fx::OwnerIntent intent_for(const fx::Request &) const override { return intent; }
    std::optional<fx::ExhaustionEvidence> exhaustion_evidence() const override {
        return exhaustion;
    }
};

class ScriptedPrivateTransport final : public fx::PrivateTransportPort {
  public:
    std::optional<wire::Credential> credential;
    std::vector<wire::Handoff> calls;

    bool has_qualified_credential(const fx::Selection &selection) const override {
        return credential && credential->selection == selection;
    }

    void send(const fx::Admission &admission) override {
        if (!credential)
            throw std::logic_error("Unexpected transport handoff without fixture credential");
        auto call = wire::make_handoff(admission, *credential);
        if (!call)
            throw std::logic_error("Private transport rejected fixture handoff");
        calls.push_back(std::move(*call));
    }

    const wire::Handoff &sent(const fx::Started &started) const {
        const auto call = std::ranges::find_if(calls, [&](const auto &candidate) {
            return candidate.admission.identity == started.admission.identity;
        });
        if (call == calls.end())
            throw std::logic_error("No private handoff for the admitted fixture request");
        return *call;
    }
};

class Fixture {
  public:
    SyntheticOwner owner;
    ScriptedPrivateTransport transport;
    fx::Adapter adapter{owner, transport};

    void select(fx::Profile profile = fx::Profile::free, bool paid_features = false) {
        owner.setup.selection.profile = profile;
        if (paid_features)
            owner.setup.features = fx::FeatureQualification{owner.setup.selection, true, true};
        install();
    }

    void install(std::string app_id = synthetic_app_id()) {
        owner.intent.selection = owner.setup.selection;
        transport.credential = wire::Credential{owner.setup.selection, std::move(app_id)};
        adapter.select();
    }

    fx::Started start(const fx::Request &request, bool follower = false) {
        auto result = adapter.start(request);
        if (const auto *refusal = std::get_if<fx::Refusal>(&result))
            throw std::logic_error("Expected admission for " + request.request_ref + ", got " +
                                   ::testing::PrintToString(refusal->kind));
        auto started = std::get<fx::Started>(std::move(result));
        EXPECT_EQ(started.follower, follower);
        EXPECT_EQ(started.admission.identity.selection, owner.setup.selection);
        if (!follower) {
            EXPECT_EQ(started.admission.identity.request_ref, request.request_ref);
        }
        return started;
    }

    void blocked(const fx::Request &request, fx::FailureKind expected) {
        const auto calls = transport.calls.size();
        const auto result = adapter.start(request);
        const auto *refusal = std::get_if<fx::Refusal>(&result);
        ASSERT_NE(refusal, nullptr) << request.request_ref;
        EXPECT_EQ(refusal->kind, expected) << request.request_ref;
        EXPECT_EQ(transport.calls.size(), calls);
    }

    fx::Completion finish(const fx::Started &started, wire::RawFrame raw) {
        auto reply = wire::sanitize_reply(transport.sent(started), std::move(raw));
        auto complete = adapter.finish(std::move(reply));
        EXPECT_EQ(complete.identity, started.admission.identity);
        return complete;
    }

    fx::Completion exchange(const fx::Request &request, wire::RawFrame raw) {
        return finish(start(request), std::move(raw));
    }

    fx::Completion complete_usage(const fx::Started &started, wire::RawFrame raw,
                                  std::string_view app_id = {}) {
        const auto echo = app_id.empty() ? synthetic_app_id() : std::string(app_id);
        add_echo(raw, "\"" + echo + "\"");
        return finish(started, std::move(raw));
    }

    fx::Completion report(std::string ref, wire::RawFrame raw, std::string_view app_id = {}) {
        return complete_usage(start(usage(std::move(ref))), std::move(raw), app_id);
    }

    fx::Completion failure(const fx::Started &started, fx::TransportFailureKind kind,
                           fx::Dispatch dispatch, fx::Clocks receipt = {}) {
        auto complete = adapter.finish(
            {started.admission.identity, fx::TransportFailure{kind, std::move(receipt)}, dispatch});
        EXPECT_EQ(complete.identity, started.admission.identity);
        EXPECT_EQ(complete.dispatch, dispatch);
        return complete;
    }

    fx::OwnerView view() const { return adapter.view(owner.intent.now); }
};

inline void expect_counts(const fx::Counts &counts, std::int64_t http, std::int64_t metered,
                          std::int64_t uncertain = 0) {
    EXPECT_EQ(counts, (fx::Counts{http, metered, uncertain}));
}

inline void expect_refusal(const fx::Completion &result, fx::FailureKind kind,
                           std::optional<int> status = std::nullopt) {
    const auto *refusal = std::get_if<fx::Refusal>(&result.outcome);
    ASSERT_NE(refusal, nullptr);
    EXPECT_EQ(refusal->kind, kind);
    if (status) {
        EXPECT_EQ(refusal->http_status, status);
    }
}

inline void expect_hold(const fx::OwnerView &view, fx::HoldKind kind) {
    EXPECT_TRUE(
        std::ranges::any_of(view.holds, [kind](const auto &hold) { return hold.kind == kind; }));
}

inline fx::HoldIdentity hold(const fx::OwnerView &view, fx::HoldKind kind) {
    auto match = std::ranges::find(view.holds, kind, &fx::HoldIdentity::kind);
    if (match == view.holds.end())
        throw std::logic_error("Expected owner hold is missing");
    return *match;
}

inline void expect_finite(const fx::UsageSample &sample, std::int64_t total,
                          std::int64_t remaining) {
    const auto *quota = std::get_if<fx::FiniteQuota>(&sample.monthly_quota);
    ASSERT_NE(quota, nullptr);
    EXPECT_EQ(*quota, (fx::FiniteQuota{total, remaining}));
}

inline void expect_times(const core::RateObservationContext &context,
                         std::optional<std::string_view> seconds,
                         std::optional<std::int64_t> dataset_ms,
                         std::optional<std::int64_t> fetched_ms) {
    EXPECT_EQ(context.source_rate_time.original_text, seconds);
    EXPECT_EQ(context.publication_time.original_text, seconds);
    EXPECT_EQ(context.source_rate_time.utc_ms, dataset_ms);
    EXPECT_EQ(context.publication_time.utc_ms, dataset_ms);
    EXPECT_EQ(context.fetched_at_utc_ms, fetched_ms);
}

inline void expect_quote(const core::RateQuote &quote, std::string_view base,
                         std::string_view currency, std::string_view lexeme, int numerator,
                         int denominator) {
    EXPECT_EQ(quote.from_currency, base);
    EXPECT_EQ(quote.to_currency, currency);
    EXPECT_EQ(quote.rate.original(), lexeme);
    EXPECT_EQ(quote.rate.value(), core::Exact(numerator) / denominator);
}

inline void expect_rate_route(const fx::Admission &admitted, std::string_view suffix) {
    EXPECT_EQ(admitted.method, "GET");
    EXPECT_EQ(admitted.url, std::string(origin) + std::string(suffix));
    EXPECT_EQ(admitted.credential_header_name, "Authorization");
    EXPECT_FALSE(admitted.follow_redirects);
    EXPECT_EQ(admitted.max_header_bytes, 16 * 1024);
    EXPECT_EQ(admitted.max_wire_bytes, 256 * 1024);
    EXPECT_EQ(admitted.max_decoded_bytes, 256 * 1024);
    EXPECT_EQ(admitted.deadline, 4s);
}

} // namespace rates_004_fixture
