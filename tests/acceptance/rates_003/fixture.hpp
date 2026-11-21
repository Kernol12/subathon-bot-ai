// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <gtest/gtest.h>

#include <stdexcept>
#include <string_view>
#include <subathon/server/exchange_rate_api.hpp>

namespace rates_003_fixture {
namespace fx = subathon::server::exchange_rate_api;
namespace core = subathon::core;
using namespace std::chrono;

constexpr std::int64_t minute_ms = 60'000;
constexpr std::int64_t day_ms = 86'400'000;
constexpr std::int64_t january = 1'767'225'600'000;
constexpr std::string_view open_latest = "https://open.er-api.com/v6/latest/USD";
constexpr std::string_view keyed_root = "https://v6.exchangerate-api.com/v6/synthetic%2F%2B%3F/";
constexpr fx::RenewalDay no_hint{fx::RenewalState::absent, std::nullopt};
constexpr fx::RenewalDay bad_hint{fx::RenewalState::invalid, std::nullopt};
constexpr fx::RenewalDay hint17{fx::RenewalState::valid, 17};
constexpr fx::RenewalDay hint31{fx::RenewalState::valid, 31};

inline std::int64_t utc(year_month_day date, hours hour) {
    return (sys_days{date} + hour).time_since_epoch() / milliseconds{1};
}
inline fx::Clocks clock(std::int64_t elapsed, std::optional<std::int64_t> wall = std::nullopt) {
    return {wall, fx::Elapsed{"fixture-epoch", elapsed}};
}
inline fx::Dispatch sent(std::int64_t elapsed) {
    return {fx::DispatchKind::sent,
            fx::SendTime{fx::SendTimeKind::exact, {"fixture-epoch", elapsed}}};
}

inline fx::SelectionUpdate setup(fx::Profile profile, std::string version = "A",
                                 std::string scope = "S",
                                 fx::AuthorizedChange change = fx::AuthorizedChange::unchanged) {
    const bool keyed = profile != fx::Profile::open && profile != fx::Profile::unknown;
    fx::SelectionBinding binding{
        profile,
        {"install", keyed ? fx::ScopeKind::account_quota : fx::ScopeKind::open_usage, scope},
        "config-" + version,
        "selection-" + version,
        keyed ? std::optional{"credential-" + version} : std::nullopt};
    return {binding,
            keyed ? std::optional{fx::Credential{binding, "synthetic/+?"}} : std::nullopt,
            {},
            change,
            fx::ScopeContinuity::qualified};
}

inline fx::Request latest(std::string id, std::vector<std::string> quotes = {"EUR"},
                          fx::RequestOrigin origin = fx::RequestOrigin::automatic) {
    return {fx::Operation::latest, id, "evidence-" + id, origin, "USD", std::move(quotes), {}};
}
inline fx::Request history(std::string id) {
    return {fx::Operation::history,
            id,
            "evidence-" + id,
            fx::RequestOrigin::automatic,
            "USD",
            {"EUR"},
            "2024-02-29"};
}
inline fx::Request quota(std::string id, fx::RequestOrigin origin = fx::RequestOrigin::automatic) {
    return {fx::Operation::quota, id, "evidence-" + id, origin};
}

inline fx::HttpReply frame(std::string body, fx::Clocks receipt, int status = 200,
                           fx::Headers headers = {}) {
    const auto size = body.size();
    return {status, std::move(headers), std::move(body), size, std::move(receipt)};
}
inline fx::HttpReply quota_reply(std::uint64_t total, std::uint64_t remaining, fx::Clocks receipt,
                                 std::string_view hint = {}) {
    std::string body = R"({"result":"success","plan_quota":)" + std::to_string(total) +
                       R"(,"requests_remaining":)" + std::to_string(remaining);
    if (!hint.empty())
        body += R"(,"refresh_day_of_month":)" + std::string(hint);
    return frame(body + "}", std::move(receipt));
}
inline fx::HttpReply provider_error(std::string_view code, fx::Clocks receipt, int status = 200) {
    return frame(R"({"result":"error","error-type":")" + std::string(code) + R"("})",
                 std::move(receipt), status);
}

inline constexpr std::string_view keyed_latest_body =
    R"({"result":"success","base_code":"USD","conversion_rates":{"EUR":0.9000}})";
inline constexpr std::string_view history_body =
    R"({"result":"success","base_code":"USD","year":2024,"month":2,"day":29,"conversion_rates":{"EUR":0.9225}})";

inline std::string open_body(std::string_view eol) {
    return R"({"result":"success","base_code":"USD","rates":{"USD":1,"EUR":0.9000},)"
           R"("time_last_update_unix":1767225600,"time_last_update_utc":"Thu, 01 Jan 2026 00:01:00 +0000",)"
           R"("time_next_update_unix":1767312000,"link":"https://fixture.invalid/fake-attribution",)" +
           std::string(eol) + "}";
}

class SyntheticSelection final : public fx::SelectionPort {
  public:
    fx::SelectionUpdate offered = setup(fx::Profile::pro);

    fx::SelectionUpdate selected_setup() const override { return offered; }
};

class ScriptedTransport final : public fx::HttpPort {
  public:
    std::optional<fx::Dispatch> next_dispatch;
    std::vector<fx::HttpRequest> calls;

    fx::Dispatch send(const fx::HttpRequest &request) override {
        calls.push_back(request);
        if (!next_dispatch) {
            ADD_FAILURE() << "Unexpected handoff to the fake HTTP port";
            return {fx::DispatchKind::not_sent};
        }
        return std::exchange(next_dispatch, std::nullopt).value();
    }
};

class Fixture {
  public:
    SyntheticSelection selection;
    ScriptedTransport transport;
    fx::Adapter adapter{selection, transport};

    [[nodiscard]] fx::SelectionStatus select(fx::SelectionUpdate update) {
        selection.offered = std::move(update);
        return adapter.select();
    }

    fx::Started start(const fx::Request &request, const fx::Clocks &at, fx::Dispatch dispatch) {
        transport.next_dispatch = std::move(dispatch);
        auto result = adapter.start(request, at);
        if (const auto *failure = std::get_if<fx::Failure>(&result))
            throw std::logic_error("Expected admission for " + request.request_ref + ", got " +
                                   ::testing::PrintToString(failure->kind));
        return std::get<fx::Started>(std::move(result));
    }

    fx::Completion exchange(const fx::Request &request, const fx::Clocks &at, fx::Reply reply) {
        return adapter.finish(start(request, at, sent(at.elapsed.value().tick_ms)),
                              std::move(reply));
    }

    fx::QuotaView view(const fx::Clocks &at) const {
        return adapter.quota_view(selection.offered.selection.scope, at);
    }
};

inline void expect_blocked(Fixture &f, const fx::Request &request, const fx::Clocks &at,
                           fx::FailureKind reason) {
    SCOPED_TRACE(request.request_ref);
    const auto calls = f.transport.calls.size();
    const auto result = f.adapter.start(request, at);
    EXPECT_EQ(f.transport.calls.size(), calls);
    const auto *failure = std::get_if<fx::Failure>(&result);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, reason);
}

// Pending recovery requests may coalesce or be blocked by their existing hold.
inline void expect_pending_probe_blocks(Fixture &f, const fx::Request &request,
                                        const fx::Clocks &at) {
    const auto calls = f.transport.calls.size();
    const auto result = f.adapter.start(request, at);
    EXPECT_EQ(f.transport.calls.size(), calls);
    const auto *failure = std::get_if<fx::Failure>(&result);
    ASSERT_NE(failure, nullptr);
    EXPECT_TRUE(failure->kind == fx::FailureKind::coalesced ||
                failure->kind == fx::FailureKind::quota_hold);
}

inline void expect_get(const fx::HttpRequest &request, std::string_view target,
                       const fx::SelectionBinding &selected, const fx::Request &requested,
                       bool probe = false) {
    EXPECT_EQ(request.private_url, target);
    EXPECT_EQ(request.method, "GET");
    EXPECT_FALSE(request.follow_redirects);
    EXPECT_EQ(request.max_header_bytes, 16u * 1024);
    EXPECT_EQ(request.max_wire_body_bytes, 256u * 1024);
    EXPECT_EQ(request.max_decoded_body_bytes, 256u * 1024);
    EXPECT_EQ(request.admission, (fx::Admission{selected, requested, probe}));
}

inline void expect_failure(const fx::Completion &completed, fx::FailureKind kind) {
    const auto *failure = std::get_if<fx::Failure>(&completed.outcome);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, kind);
}

inline void expect_counts(const fx::Quota &sample, std::uint64_t quota, std::uint64_t remaining,
                          fx::RenewalDay hint) {
    EXPECT_EQ(sample.plan_quota, quota);
    EXPECT_EQ(sample.requests_remaining, remaining);
    EXPECT_EQ(sample.renewal_day, hint);
}
inline void expect_report(const fx::QuotaView &view, std::uint64_t quota, std::uint64_t remaining,
                          fx::RenewalDay hint, std::uint64_t local, std::uint64_t since) {
    EXPECT_EQ(view.local_total, (fx::Usage{local, 0}));
    EXPECT_EQ(view.local_since_report, (fx::Usage{since, 0}));
    ASSERT_TRUE(view.sample);
    expect_counts(*view.sample, quota, remaining, hint);
}

// The fixture only consumes typed proposals. A real core snapshot supplies the prior rate.
class SnapshotOwner {
  public:
    SnapshotOwner()
        : prior(
              core::make_effective_rate_snapshot({"cached",
                                                  {},
                                                  {},
                                                  {{{"prior",
                                                     "fixture-rate-source",
                                                     "prior-config",
                                                     "prior-job",
                                                     "prior-evidence",
                                                     {},
                                                     {},
                                                     {},
                                                     {}},
                                                    {{"USD", "EUR", "0.91"}}}},
                                                  {},
                                                  {core::FetchedRateConstraint{"prior", 1}},
                                                  {{{{"EUR", "1", "1"}, {"USD", "100", "91"}}}}})) {
    }

    void consider(const fx::Completion &completed) {
        if (const auto *rate = std::get_if<fx::Rate>(&completed.outcome);
            rate && rate->use == fx::RateUse::latest_candidate)
            latest_proposal = rate->observation;
    }

    void expect_cached() const {
        EXPECT_FALSE(latest_proposal);
        const auto &sources = prior.data().sources;
        ASSERT_EQ(sources.size(), 1u);
        ASSERT_EQ(sources.front().data().quotes.size(), 1u);
        EXPECT_EQ(sources.front().data().quotes.front().rate.value(), core::Exact(91) / 100);
        EXPECT_EQ(prior.data().snapshot_ref, "cached");
    }

  private:
    core::EffectiveRateSnapshot prior;
    std::optional<core::FetchedRateObservation> latest_proposal;
};

inline void expect_open_disclosure(const fx::Rate &rate) {
    ASSERT_TRUE(rate.open_disclosure);
    EXPECT_EQ(rate.open_disclosure->attribution_text, "Rates By Exchange Rate API");
    EXPECT_EQ(rate.open_disclosure->attribution_url, "https://www.exchangerate-api.com");
    EXPECT_TRUE(rate.open_disclosure->redistribution_restricted);
}

} // namespace rates_003_fixture
