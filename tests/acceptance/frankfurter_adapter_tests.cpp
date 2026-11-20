// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <chrono>
#include <limits>
#include <string_view>
#include <subathon/server/frankfurter_rates.hpp>

namespace {
using namespace std::chrono;
using namespace subathon::server;
using subathon::core::Exact;

constexpr auto receipt_utc =
    (sys_days{2026y / November / 20} + hours{12}).time_since_epoch() / milliseconds{1};
constexpr std::int64_t receipt_tick = 300'000;

FrankfurterRequest latest(std::vector<std::string> quotes = {"GBP", "USD"}) {
    return {"c0", "q1", "e1", "EUR", std::move(quotes), std::nullopt};
}

FrankfurterHttpFrame frame(std::string body, int status = 200) {
    const auto bytes = body.size();
    return {status,
            {{"Content-Type", "application/json"},
             {"Date", "Fri, 20 Nov 2026 11:50:00 GMT"},
             {"Last-Modified", "Thu, 19 Nov 2026 16:00:00 GMT"},
             {"Age", "600"}},
            std::move(body),
            bytes,
            receipt_utc,
            receipt_tick};
}

// A single owned, already-framed response. Admission failure releases the frame;
// this fake never performs IO and cannot return a second response.
class OneResponsePort final : public FrankfurterHttpPort {
  public:
    explicit OneResponsePort(FrankfurterHttpResult response) : response_(std::move(response)) {}

    FrankfurterHttpResult exchange(const FrankfurterHttpRequest &request) override {
        ++dispatches;
        if (dispatches != 1) {
            ADD_FAILURE() << "one admitted attempt may dispatch only once";
            return FrankfurterTransportFailure::framing;
        }
        sent = request;
        if (const auto *body = std::get_if<FrankfurterHttpFrame>(&response_)) {
            std::size_t header_bytes = 2; // Final CRLF.
            for (const auto &[name, value] : body->headers)
                header_bytes += name.size() + value.size() + 4;
            if (body->decoded_body.size() > request.max_decoded_body_bytes ||
                body->wire_body_bytes > request.max_wire_body_bytes ||
                header_bytes > request.max_header_bytes)
                return FrankfurterTransportFailure::resource_limit;
        }
        return std::move(response_);
    }

    unsigned dispatches{};
    std::optional<FrankfurterHttpRequest> sent;

  private:
    FrankfurterHttpResult response_;
};

void expect_provenance(const FrankfurterFetch &fetch, std::size_t index, std::string_view date,
                       std::optional<std::int64_t> utc = receipt_utc) {
    const auto &context = fetch.observations.at(index).data().context;
    EXPECT_FALSE(context.observation_ref.empty());
    EXPECT_EQ(context.provider_ref, "Frankfurter/ECB");
    EXPECT_EQ(context.configuration_ref, "c0");
    EXPECT_EQ(context.request_ref, "q1");
    EXPECT_EQ(context.source_evidence_ref, "e1");
    EXPECT_EQ(context.fetched_at_utc_ms, utc);
    EXPECT_EQ(context.source_rate_time.original_text, std::string{date});
    EXPECT_FALSE(context.source_rate_time.utc_ms);
    EXPECT_FALSE(context.provider_snapshot_ref);
    EXPECT_FALSE(context.publication_time.original_text);
    EXPECT_FALSE(context.publication_time.utc_ms);
}

TEST(FrankfurterAdapter, FilteredEurGetOwnsExactDigitsInRowOrder) {
    // F1. Inspect the admitted transport request and destroy the fake before reading the result.
    const auto result = [] {
        OneResponsePort port{frame(R"([
            {"date":"2026-11-19","base":"EUR","quote":"USD","rate":1.2000},
            {"date":"2026-11-19","base":"EUR","quote":"GBP","rate":8e-1,"extra":[42]}
        ])")};
        auto fetch = FrankfurterAdapter{port}.fetch(latest({"USD", "GBP", "USD"}));
        EXPECT_EQ(port.dispatches, 1u);
        if (port.sent) {
            EXPECT_EQ(port.sent->method, "GET");
            EXPECT_EQ(port.sent->url,
                      "https://api.frankfurter.dev/v2/rates?providers=ecb&base=EUR&quotes=GBP,USD");
            const FrankfurterHeaders headers{{"Accept", "application/json"}};
            EXPECT_EQ(port.sent->headers, headers);
            EXPECT_EQ(port.sent->max_wire_body_bytes, 256u * 1024);
            EXPECT_EQ(port.sent->max_decoded_body_bytes, 256u * 1024);
            EXPECT_EQ(port.sent->max_header_bytes, 16u * 1024);
            EXPECT_EQ(port.sent->deadline, seconds{30});
        } else {
            ADD_FAILURE() << "missing dispatched HTTP request";
        }
        return fetch;
    }();

    const auto *fetch = std::get_if<FrankfurterFetch>(&result);
    ASSERT_NE(fetch, nullptr);
    EXPECT_EQ(fetch->admitted.quotes, (std::vector<std::string>{"GBP", "USD"}));
    EXPECT_FALSE(fetch->admitted.historical_date);
    EXPECT_TRUE(fetch->omitted_quotes.empty());
    EXPECT_TRUE(fetch->historical_matches.empty());
    ASSERT_EQ(fetch->observations.size(), 1u);
    expect_provenance(*fetch, 0, "2026-11-19");
    const auto &quotes = fetch->observations.front().data().quotes;
    ASSERT_EQ(quotes.size(), 2u);
    EXPECT_EQ(quotes[0].from_currency, "EUR");
    EXPECT_EQ(quotes[0].to_currency, "USD");
    EXPECT_EQ(quotes[0].rate.original(), "1.2000");
    EXPECT_EQ(quotes[0].rate.value(), Exact(6) / 5);
    EXPECT_EQ(quotes[1].from_currency, "EUR");
    EXPECT_EQ(quotes[1].to_currency, "GBP");
    EXPECT_EQ(quotes[1].rate.original(), "8e-1");
    EXPECT_EQ(quotes[1].rate.value(), Exact(4) / 5);
}

TEST(FrankfurterAdapter, CurrencyCeilingCountsDistinctCodesIncludingBase) {
    auto request = latest({});
    for (unsigned i = 0; i < 256; ++i)
        request.quotes.push_back(
            {'X', static_cast<char>('A' + i / 26), static_cast<char>('A' + i % 26)});
    const auto rejected = make_frankfurter_request(request);
    const auto *failure = std::get_if<FrankfurterFailure>(&rejected);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, FrankfurterFailureKind::resource_limit);

    request.quotes.pop_back();
    request.quotes.push_back(request.quotes.front());
    const auto admitted = make_frankfurter_request(request);
    const auto *http = std::get_if<FrankfurterHttpRequest>(&admitted);
    ASSERT_NE(http, nullptr);
    EXPECT_EQ(http->admitted.quotes.size(), 255u);
    EXPECT_EQ(http->admitted.quotes.front(), "XAA");
    EXPECT_EQ(http->admitted.quotes.back(), "XJU");
}

TEST(FrankfurterAdapter, InvalidSelectorsNeverDispatch) {
    struct Case {
        std::string_view base;
        std::vector<std::string> quotes;
        std::optional<std::string> date;
    };
    for (const auto &row : std::vector<Case>{{"USD", {"GBP"}, {}},
                                             {"EUR", {"usd"}, {}},
                                             {"EUR", {"EUR"}, {}},
                                             {"EUR", {}, {}},
                                             {"EUR", {"USD"}, "2026-02-29"},
                                             {"EUR", {"USD&base=GBP"}, {}},
                                             {"EUR", {"USD"}, "0000-11-19"}}) {
        SCOPED_TRACE(row.base);
        OneResponsePort port{frame("[]")};
        auto query = latest(row.quotes);
        query.base = row.base;
        query.historical_date = row.date;
        const auto result = FrankfurterAdapter{port}.fetch(query);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, FrankfurterFailureKind::invalid_request);
        EXPECT_EQ(port.dispatches, 0u);
    }
}

TEST(FrankfurterAdapter, HistoricalSelectorRetainsDifferentReturnedDate) {
    // F2. Eligibility is independent of matching.
    OneResponsePort port{
        frame(R"([{"date":"2026-11-18","base":"EUR","quote":"USD","rate":1.250}])")};
    auto request = latest({"USD"});
    request.historical_date = "2026-11-19";
    const auto result = FrankfurterAdapter{port}.fetch(request);
    ASSERT_EQ(port.dispatches, 1u);
    ASSERT_TRUE(port.sent);
    EXPECT_EQ(port.sent->url, "https://api.frankfurter.dev/v2/rates?providers=ecb&base=EUR"
                              "&quotes=USD&date=2026-11-19");
    const auto *fetch = std::get_if<FrankfurterFetch>(&result);
    ASSERT_NE(fetch, nullptr);
    ASSERT_EQ(fetch->observations.size(), 1u);
    expect_provenance(*fetch, 0, "2026-11-18");
    const auto &quote = fetch->observations.front().data().quotes.at(0);
    EXPECT_EQ(quote.rate.original(), "1.250");
    EXPECT_EQ(quote.rate.value(), Exact(5) / 4);
    ASSERT_EQ(fetch->historical_matches.size(), 1u);
    const auto &match = fetch->historical_matches.front();
    EXPECT_EQ(match.quote, "USD");
    EXPECT_EQ(match.requested_date, "2026-11-19");
    EXPECT_EQ(match.returned_date, "2026-11-18");
    EXPECT_FALSE(match.matches);
}

TEST(FrankfurterAdapter, RejectsWholeResponseWhenAnyRowFailsOrDocumentIsIncomplete) {
    // F3. Every mutation follows a valid USD row.
    constexpr std::string_view first =
        R"([{"date":"2026-11-19","base":"EUR","quote":"USD","rate":1.21},)";
    using Failure = FrankfurterFailureKind;
    struct Case {
        std::string_view tail;
        Failure expected;
    };
    for (const auto &row :
         {Case{R"({"date":"2026-11-19","base":"EUR","quote":"GBP","rate":"0.81"}])",
               Failure::invalid_row},
          Case{R"({"date":"2026-11-19","base":"EUR","quote":"GBP","rate":0}])",
               Failure::invalid_row},
          Case{R"({"date":"2026-11-19","base":"EUR","quote":"GBP"}])", Failure::invalid_row},
          Case{R"({"date":"2026-11-19","base":"GBP","quote":"GBP","rate":1}])",
               Failure::invalid_row},
          Case{R"({"date":"2026-11-18","base":"EUR","quote":"USD","rate":1.21}])",
               Failure::invalid_row},
          Case{R"({"date":"2026-11-19","base":"EUR","quote":"CAD","rate":1.21}])",
               Failure::invalid_row},
          Case{R"({"date":"2026-02-29","base":"EUR","quote":"GBP","rate":0.81}])",
               Failure::invalid_row},
          Case{R"({"date":"2026-11-19","base":"EUR","quote":"GBP","rate":0.81,"r\u0061te":0.81}])",
               Failure::invalid_row},
          Case{"{", Failure::invalid_document}}) {
        SCOPED_TRACE(row.tail);
        OneResponsePort port{frame(std::string{first} + std::string{row.tail})};
        const auto result = FrankfurterAdapter{port}.fetch(latest());
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, row.expected);
    }
}

TEST(FrankfurterAdapter, ReceiveBudgetsTakePrecedenceOverHttpStatus) {
    for (const auto field : {"decoded body", "wire body", "header"}) {
        SCOPED_TRACE(field);
        auto response = frame("[]", 429);
        if (std::string_view{field} == "decoded body")
            response.decoded_body.resize(256u * 1024 + 1, ' ');
        else if (std::string_view{field} == "wire body")
            response.wire_body_bytes = 256u * 1024 + 1;
        else
            response.headers.emplace_back("X-Fixture", std::string(16u * 1024, 'x'));
        OneResponsePort port{std::move(response)};
        const auto result = FrankfurterAdapter{port}.fetch(latest());
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, FrankfurterFailureKind::resource_limit);
        EXPECT_FALSE(failure->status);
        EXPECT_EQ(failure->wait.state, FrankfurterWaitState::unknown);
    }
}

TEST(FrankfurterAdapter, ParsedObservationBatchHasItsOwnLogicalBudget) {
    // 96 quotes split over two dates. Raw tokens are six bytes each. The parsed
    // denominators alone total 96 × 257 = 24,672 bytes across the batch.
    auto request = latest({});
    std::string body = "[";
    for (unsigned i = 0; i < 96; ++i) {
        std::string code{'X', static_cast<char>('A' + i / 26), static_cast<char>('A' + i % 26)};
        request.quotes.push_back(code);
        if (i != 0)
            body += ',';
        body += R"({"base":"EUR","quote":")" + code + R"(","date":"2026-11-)";
        body += i < 48 ? "19" : "18";
        body += R"(","rate":1e-256})";
    }
    body += ']';
    OneResponsePort port{frame(std::move(body))};
    const auto result = FrankfurterAdapter{port}.fetch(request);
    ASSERT_EQ(port.dispatches, 1u);
    const auto *failure = std::get_if<FrankfurterFailure>(&result);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, FrankfurterFailureKind::resource_limit);
}

TEST(FrankfurterAdapter, PartialAndMixedRowsRetainOmissionAndOrderedDateEvidence) {
    auto request = latest({"GBP", "USD", "CAD", "EUR"});
    OneResponsePort port{frame(R"([
        {"date":"2026-11-18","base":"EUR","quote":"GBP","rate":0.84},
        {"date":"2026-11-19","base":"EUR","quote":"EUR","rate":1.000},
        {"date":"2026-11-19","base":"EUR","quote":"USD","rate":1.20}
    ])")};
    const auto result = FrankfurterAdapter{port}.fetch(request);
    ASSERT_EQ(port.dispatches, 1u);
    const auto *fetch = std::get_if<FrankfurterFetch>(&result);
    ASSERT_NE(fetch, nullptr);
    EXPECT_EQ(fetch->omitted_quotes, std::vector<std::string>{"CAD"});
    ASSERT_EQ(fetch->observations.size(), 2u);
    expect_provenance(*fetch, 0, "2026-11-18");
    expect_provenance(*fetch, 1, "2026-11-19");
    const auto &older = fetch->observations[0].data();
    const auto &newer = fetch->observations[1].data();
    EXPECT_NE(older.context.observation_ref, newer.context.observation_ref);
    ASSERT_EQ(older.quotes.size(), 1u);
    EXPECT_EQ(older.quotes[0].to_currency, "GBP");
    EXPECT_EQ(older.quotes[0].rate.value(), Exact(21) / 25);
    ASSERT_EQ(newer.quotes.size(), 2u);
    EXPECT_EQ(newer.quotes[0].to_currency, "EUR");
    EXPECT_EQ(newer.quotes[0].rate.original(), "1.000");
    EXPECT_EQ(newer.quotes[1].to_currency, "USD");
    EXPECT_EQ(newer.quotes[1].rate.value(), Exact(6) / 5);
}

TEST(FrankfurterAdapter, AbsentUtcRemainsAbsentEvenWithHttpDates) {
    // F7.
    auto response = frame(R"([
        {"date":"2026-11-19","base":"EUR","quote":"GBP","rate":0.85},
        {"date":"2026-11-19","base":"EUR","quote":"USD","rate":1.25}
    ])");
    response.receipt_utc_ms.reset();
    OneResponsePort port{std::move(response)};
    const auto result = FrankfurterAdapter{port}.fetch(latest());
    ASSERT_EQ(port.dispatches, 1u);
    const auto *fetch = std::get_if<FrankfurterFetch>(&result);
    ASSERT_NE(fetch, nullptr);
    ASSERT_EQ(fetch->observations.size(), 1u);
    expect_provenance(*fetch, 0, "2026-11-19", std::nullopt);
}

TEST(FrankfurterAdapter, HttpFailurePrecedesBodyDecodeAndPreservesSelector) {
    using Failure = FrankfurterFailureKind;
    for (const auto &[status, expected] : {std::pair{400, Failure::rejected_request},
                                           {422, Failure::rejected_request},
                                           {404, Failure::missing_rates},
                                           {302, Failure::http_failure},
                                           {304, Failure::http_failure},
                                           {401, Failure::http_failure},
                                           {403, Failure::http_failure}}) {
        SCOPED_TRACE(status);
        auto response = frame("{invalid JSON", status);
        response.headers.emplace_back("Retry-After", "120");
        OneResponsePort port{std::move(response)};
        auto request = latest({"USD", "GBP", "USD"});
        request.historical_date = "2026-11-19";
        const auto result = FrankfurterAdapter{port}.fetch(request);
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, expected);
        EXPECT_EQ(failure->status, status);
        ASSERT_TRUE(failure->admitted);
        EXPECT_EQ(failure->admitted->configuration_ref, "c0");
        EXPECT_EQ(failure->admitted->request_ref, "q1");
        EXPECT_EQ(failure->admitted->evidence_ref, "e1");
        EXPECT_EQ(failure->admitted->base, "EUR");
        EXPECT_EQ(failure->admitted->quotes, (std::vector<std::string>{"GBP", "USD"}));
        EXPECT_EQ(failure->admitted->historical_date, "2026-11-19");
        EXPECT_EQ(failure->wait.state, FrankfurterWaitState::unknown);
    }
}

TEST(FrankfurterAdapter, TimeoutCancellationTransportAndFramingStayDistinct) {
    using Transport = FrankfurterTransportFailure;
    using Failure = FrankfurterFailureKind;
    for (const auto &[signal, expected] : {std::pair{Transport::timeout, Failure::timeout},
                                           {Transport::cancelled, Failure::cancelled},
                                           {Transport::transport, Failure::transport},
                                           {Transport::framing, Failure::framing}}) {
        OneResponsePort port{signal};
        const auto result = FrankfurterAdapter{port}.fetch(latest());
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, expected);
        EXPECT_FALSE(failure->status);
        EXPECT_EQ(failure->wait.state, FrankfurterWaitState::unknown);
    }
}

TEST(FrankfurterAdapter, DelayWaitRetainsElapsedBasisAcrossLeadingZerosAndTickOverflow) {
    // F6. The clock is intentionally too close to int64 max for receipt + delay.
    constexpr auto near_end = std::numeric_limits<std::int64_t>::max() - 100;
    for (const auto &[hint, delay] : {std::pair{std::string{"120"}, 120'000},
                                      {std::string(254, '0') + "120", 120'000},
                                      {std::string(257, '0'), 0}}) {
        SCOPED_TRACE(hint);
        auto response = frame("bad JSON", 429);
        response.receipt_monotonic_ms = near_end;
        response.receipt_utc_ms.reset();
        response.headers.emplace_back("Retry-After", hint);
        OneResponsePort port{std::move(response)};
        const auto result = FrankfurterAdapter{port}.fetch(latest());
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->status, 429);
        EXPECT_EQ(failure->kind, FrankfurterFailureKind::limited);
        EXPECT_EQ(failure->wait.state, FrankfurterWaitState::known);
        EXPECT_EQ(failure->wait.delay_ms, delay);
        EXPECT_EQ(failure->wait.receipt_monotonic_ms, near_end);
        EXPECT_FALSE(failure->wait.deadline_utc_ms);
    }
}

TEST(FrankfurterAdapter, MalformedOrAmbiguousHintLeavesUnknownWait) {
    for (const auto &hints :
         std::vector<FrankfurterHeaders>{{},
                                         {{"Retry-After", "-1"}},
                                         {{"Retry-After", "120"}, {"retry-after", "120"}},
                                         {{"Retry-After", std::string(257, '1') + "x"}}}) {
        auto response = frame("[]", 429);
        response.headers.insert(response.headers.end(), hints.begin(), hints.end());
        OneResponsePort port{std::move(response)};
        const auto result = FrankfurterAdapter{port}.fetch(latest());
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, FrankfurterFailureKind::limited);
        EXPECT_EQ(failure->wait.state, FrankfurterWaitState::unknown);
        EXPECT_FALSE(failure->wait.delay_ms);
        EXPECT_FALSE(failure->wait.deadline_utc_ms);
    }
}

TEST(FrankfurterAdapter, UnrepresentableValidHintHoldsIndeterminateEmbargo) {
    auto response = frame("[]", 429);
    response.headers.emplace_back("Retry-After", std::string(257, '9'));
    OneResponsePort port{std::move(response)};
    const auto result = FrankfurterAdapter{port}.fetch(latest());
    ASSERT_EQ(port.dispatches, 1u);
    const auto *failure = std::get_if<FrankfurterFailure>(&result);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, FrankfurterFailureKind::limited);
    EXPECT_EQ(failure->wait.state, FrankfurterWaitState::indeterminate);
    EXPECT_FALSE(failure->wait.delay_ms);
    EXPECT_FALSE(failure->wait.deadline_utc_ms);
    ASSERT_TRUE(failure->wait.retained_hint);
    EXPECT_FALSE(failure->wait.retained_hint->empty());
    EXPECT_LE(failure->wait.retained_hint->size(), 256u);
}

TEST(FrankfurterAdapter, ServiceUnavailableDateWaitUsesOnlyTrustedUtc) {
    for (const bool have_utc : {true, false}) {
        SCOPED_TRACE(have_utc);
        auto response = frame("[]", 503);
        if (!have_utc)
            response.receipt_utc_ms.reset();
        response.headers.emplace_back("Retry-After", "Fri, 20 Nov 2026 12:02:00 GMT");
        OneResponsePort port{std::move(response)};
        const auto result = FrankfurterAdapter{port}.fetch(latest());
        ASSERT_EQ(port.dispatches, 1u);
        const auto *failure = std::get_if<FrankfurterFailure>(&result);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, FrankfurterFailureKind::unavailable);
        EXPECT_EQ(failure->status, 503);
        EXPECT_EQ(failure->wait.state,
                  have_utc ? FrankfurterWaitState::known : FrankfurterWaitState::indeterminate);
        EXPECT_EQ(failure->wait.deadline_utc_ms, receipt_utc + 120'000);
        EXPECT_FALSE(failure->wait.delay_ms);
    }
}

TEST(FrankfurterAdapter, NormalizerRetainsKnownPastDeadlineAndUnanchoredDelay) {
    // This direct seam isolates normalization from request creation and dispatch.
    const FrankfurterHttpRequest request{
        latest(),
        "GET",
        "https://api.frankfurter.dev/v2/rates?providers=ecb&base=EUR&quotes=GBP,USD",
        {{"Accept", "application/json"}},
        256 * 1024,
        256 * 1024,
        16 * 1024,
        seconds{30}};
    auto past = frame("[]", 503);
    past.headers.emplace_back("Retry-After", "Fri, 20 Nov 2026 11:59:00 GMT");
    const auto past_result = normalize_frankfurter_response(request, past);
    const auto *known = std::get_if<FrankfurterFailure>(&past_result);
    ASSERT_NE(known, nullptr);
    EXPECT_EQ(known->wait.state, FrankfurterWaitState::known);
    EXPECT_EQ(known->wait.deadline_utc_ms, receipt_utc - 60'000);

    auto unanchored = frame("[]", 429);
    unanchored.receipt_monotonic_ms.reset();
    unanchored.headers.emplace_back("Retry-After", "120");
    const auto unanchored_result = normalize_frankfurter_response(request, unanchored);
    const auto *held = std::get_if<FrankfurterFailure>(&unanchored_result);
    ASSERT_NE(held, nullptr);
    EXPECT_EQ(held->wait.state, FrankfurterWaitState::indeterminate);
    EXPECT_EQ(held->wait.delay_ms, 120'000);
    EXPECT_FALSE(held->wait.receipt_monotonic_ms);
    EXPECT_FALSE(held->wait.deadline_utc_ms);
}

void expect_public_status(const ConnectorStatus &status) {
    EXPECT_EQ(status.setup.installation, ConnectorInstallation::not_required);
    EXPECT_EQ(status.setup.account_consent, ConnectorConsent::not_required);
    EXPECT_EQ(status.setup.application_registration, ConnectorSupport::unsupported);
    EXPECT_EQ(status.setup.api_key_setup, ConnectorSupport::unsupported);
    EXPECT_EQ(status.setup.webhook_setup, ConnectorSupport::unsupported);
    EXPECT_EQ(status.setup.browser_link, ConnectorLink::unsupported);
    EXPECT_FALSE(status.setup.callback_url);
    EXPECT_EQ(status.health, ConnectorHealth::unknown);
    EXPECT_EQ(status.gap, ConnectorGap::unknown);
}

TEST(FrankfurterAdapter, PublicStatusReadIsInert) {
    OneResponsePort port{frame("[]")};
    const FrankfurterAdapter adapter{port};
    expect_public_status(read_connector_status(adapter, {}));
    EXPECT_EQ(port.dispatches, 0u);
}

TEST(FrankfurterAdapter, HttpLimitingLeavesSetupAndStreamStatusUnknownWhereInapplicable) {
    // F8. Observed limiting is a rates fact, independent of connector health.
    OneResponsePort port{frame("[]", 429)};
    const FrankfurterAdapter adapter{port};
    const auto result = adapter.fetch(latest());
    const auto *failure = std::get_if<FrankfurterFailure>(&result);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, FrankfurterFailureKind::limited);
    expect_public_status(read_connector_status(adapter, {}));
    EXPECT_EQ(port.dispatches, 1u);
}

} // namespace
