// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include <array>

namespace {
using namespace rates_003_fixture;
using enum fx::FailureKind;

TEST(ExchangeRateApi, G1_OpenLatestOwnsExactQuoteAndIndependentTimeEvidence) {
    Fixture f;
    const auto open = setup(fx::Profile::open);
    ASSERT_EQ(f.select(open), fx::SelectionStatus::selected);
    const auto request = latest("g1", {"EUR", "CHF"});
    const auto completed = f.exchange(
        request, clock(500), frame(open_body(R"("time_eol_unix":0)"), clock(600, january + 123)));

    ASSERT_EQ(f.transport.calls.size(), 1u);
    expect_get(f.transport.calls.front(), open_latest, open.selection, request);
    EXPECT_EQ(completed.admission, (fx::Admission{open.selection, request, false}));
    EXPECT_EQ(completed.dispatch.kind, fx::DispatchKind::sent);
    const auto *rate = std::get_if<fx::Rate>(&completed.outcome);
    ASSERT_NE(rate, nullptr);
    EXPECT_EQ(rate->use, fx::RateUse::latest_candidate);
    const auto &data = rate->observation.data();
    ASSERT_EQ(data.quotes.size(), 1u);
    EXPECT_EQ(data.quotes[0].from_currency, "USD");
    EXPECT_EQ(data.quotes[0].to_currency, "EUR");
    EXPECT_EQ(data.quotes[0].rate.original(), "0.9000");
    EXPECT_EQ(data.quotes[0].rate.value(), core::Exact(9) / 10);
    EXPECT_EQ(rate->missing_quotes, std::vector<std::string>{"CHF"});
    EXPECT_EQ(data.context.configuration_ref, open.selection.configuration_ref);
    EXPECT_EQ(data.context.request_ref, request.request_ref);
    EXPECT_EQ(data.context.source_evidence_ref, request.evidence_ref);
    EXPECT_EQ(data.context.source_rate_time.utc_ms, january);
    EXPECT_EQ(data.context.fetched_at_utc_ms, january + 123);
    EXPECT_FALSE(data.context.publication_time.utc_ms);
    EXPECT_FALSE(data.context.publication_time.original_text);
    EXPECT_EQ(rate->update.last.unix_seconds, 1'767'225'600);
    EXPECT_EQ(rate->update.last.utc_text, "Thu, 01 Jan 2026 00:01:00 +0000");
    EXPECT_EQ(rate->update.last.parsed_text_utc_ms, january + minute_ms);
    EXPECT_EQ(rate->update.last.qualified_utc_ms, january);
    EXPECT_TRUE(rate->update.last.disagrees);
    EXPECT_EQ(rate->update.next.unix_seconds, 1'767'312'000);
    EXPECT_EQ(rate->update.next.qualified_utc_ms, january + day_ms);
    EXPECT_FALSE(rate->update.next.disagrees);
    EXPECT_EQ(rate->update.eol, (std::vector<fx::UnqualifiedEol>{{"time_eol_unix", "0"}}));
    EXPECT_FALSE(rate->matched_history_date);
    expect_open_disclosure(*rate);
    const auto view = f.view(clock(600));
    EXPECT_EQ(view.local_total, (fx::Usage{1, 0}));
    EXPECT_FALSE(view.sample);
    EXPECT_FALSE(view.local_since_report);
}

// Count the base's self entry and unrequested codes toward the rate-map bound.
std::string open_map(int extra_entries) {
    std::string body = R"({"result":"success","base_code":"USD","rates":{"EUR":0.9,"USD":1)";
    for (int n = 0; n < extra_entries; ++n) {
        const char code[] = {'A', static_cast<char>('A' + n / 26), static_cast<char>('A' + n % 26),
                             '\0'};
        body += ",\"" + std::string(code) + "\":1";
    }
    return body + "}}";
}

TEST(ExchangeRateApi, G2_OpenRejectsWrongShapeBaseDuplicateAndFullMapLimit) {
    const std::array replies{
        std::pair{std::string(keyed_latest_body), invalid_document},
        std::pair{std::string(R"({"result":"success","base_code":"CAD","rates":{"EUR":0.9}})"),
                  invalid_document},
        std::pair{
            std::string(
                R"({"result":"success","base_code":"USD","rates":{"EUR":0.9,"\u0045UR":0.8}})"),
            invalid_document},
        std::pair{open_map(255), resource_limit},
    };
    for (std::size_t n = 0; n < replies.size(); ++n) {
        SCOPED_TRACE(n);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::open)), fx::SelectionStatus::selected);
        SnapshotOwner owner;
        const auto &[body, failure] = replies[n];
        const auto result =
            f.exchange(latest("g2", {"EUR", "CHF"}), clock(10), frame(body, clock(20)));
        expect_failure(result, failure);
        owner.consider(result);
        owner.expect_cached();
        ASSERT_EQ(f.transport.calls.size(), 1u);
        EXPECT_EQ(result.dispatch.kind, fx::DispatchKind::sent);
        EXPECT_EQ(f.view(clock(20)).local_total, (fx::Usage{1, 0}));
    }
    Fixture at_limit;
    ASSERT_EQ(at_limit.select(setup(fx::Profile::open)), fx::SelectionStatus::selected);
    const auto valid =
        at_limit.exchange(latest("g2-at-limit"), clock(10), frame(open_map(254), clock(20)));
    const auto *rates = std::get_if<fx::Rate>(&valid.outcome);
    ASSERT_NE(rates, nullptr);
    EXPECT_EQ(rates->observation.data().quotes.size(), 1u);
}

TEST(ExchangeRateApi, G3_ReferenceRefusalPrecedesFreeHistoryAndPaidHistoryIsDateOnly) {
    Fixture f;
    auto free = setup(fx::Profile::free);
    free.reference_allowance = 50'000;
    ASSERT_EQ(f.select(free), fx::SelectionStatus::reference_not_allowed);
    EXPECT_FALSE(f.adapter.current());
    EXPECT_TRUE(f.transport.calls.empty());

    free.reference_allowance.reset();
    ASSERT_EQ(f.select(free), fx::SelectionStatus::selected);
    expect_blocked(f, history("free-history"), clock(0), unsupported_operation);
    auto rejected_edit = free;
    rejected_edit.selection.configuration_ref = "refused-config";
    rejected_edit.selection.selection_revision = "refused-selection";
    rejected_edit.credential->binding = rejected_edit.selection;
    rejected_edit.reference_allowance = 50'000;
    EXPECT_EQ(f.select(rejected_edit), fx::SelectionStatus::reference_not_allowed);
    const auto selected_free = f.adapter.current();
    ASSERT_TRUE(selected_free);
    EXPECT_EQ(selected_free->binding, free.selection);
    ASSERT_TRUE(f.transport.calls.empty());

    // Pro is independently selected. The existing latest snapshot belongs to its caller.
    Fixture paid;
    const auto pro = setup(fx::Profile::pro);
    ASSERT_EQ(paid.select(pro), fx::SelectionStatus::selected);
    SnapshotOwner owner;
    const auto request = history("g3-history");
    const auto result =
        paid.exchange(request, clock(10), frame(std::string(history_body), clock(12, january)));
    ASSERT_EQ(paid.transport.calls.size(), 1u);
    expect_get(paid.transport.calls.front(), std::string(keyed_root) + "history/USD/2024/2/29",
               pro.selection, request);
    const auto *rate = std::get_if<fx::Rate>(&result.outcome);
    ASSERT_NE(rate, nullptr);
    EXPECT_EQ(rate->use, fx::RateUse::history_only);
    EXPECT_EQ(rate->matched_history_date, "2024-02-29");
    const auto &data = rate->observation.data();
    ASSERT_EQ(data.quotes.size(), 1u);
    EXPECT_EQ(data.quotes[0].from_currency, "USD");
    EXPECT_EQ(data.quotes[0].to_currency, "EUR");
    EXPECT_EQ(data.quotes[0].rate.original(), "0.9225");
    EXPECT_EQ(data.quotes[0].rate.value(), core::Exact(369) / 400);
    EXPECT_EQ(data.context.source_rate_time.original_text, "2024-02-29");
    EXPECT_FALSE(data.context.source_rate_time.utc_ms);
    EXPECT_FALSE(data.context.publication_time.utc_ms);
    EXPECT_EQ(data.context.fetched_at_utc_ms, january);
    EXPECT_FALSE(rate->open_disclosure);
    EXPECT_EQ(paid.view(clock(12)).local_total, (fx::Usage{1, 0}));
    owner.consider(result);
    owner.expect_cached();
}

TEST(ExchangeRateApi, G9_HistoricalDateMismatchRejectsOtherwiseGoodRate) {
    Fixture f;
    const auto pro = setup(fx::Profile::pro);
    ASSERT_EQ(f.select(pro), fx::SelectionStatus::selected);
    const auto request = history("g9");
    const auto reply = frame(
        R"({"result":"success","base_code":"USD","year":2024,"month":2,"day":28,"conversion_rates":{"EUR":0.9225}})",
        clock(12, january));
    const auto result = f.exchange(request, clock(10), reply);
    ASSERT_EQ(f.transport.calls.size(), 1u);
    expect_get(f.transport.calls.front(), std::string(keyed_root) + "history/USD/2024/2/29",
               pro.selection, request);
    expect_failure(result, invalid_document);
    EXPECT_EQ(result.dispatch.kind, fx::DispatchKind::sent);
    EXPECT_EQ(f.view(clock(12)).local_total, (fx::Usage{1, 0}));
}

TEST(ExchangeRateApi, G12_BothEolSpellingsRemainUnqualified) {
    for (const std::string member : {"time_eol_unix", "time_eol"}) {
        SCOPED_TRACE(member);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::open)), fx::SelectionStatus::selected);
        const auto result =
            f.exchange(latest("g12", {"EUR", "CHF"}), clock(10),
                       frame(open_body("\"" + member + "\":1767312000"), clock(20, january + 123)));
        const auto *rate = std::get_if<fx::Rate>(&result.outcome);
        ASSERT_NE(rate, nullptr);
        EXPECT_EQ(rate->update.eol, (std::vector<fx::UnqualifiedEol>{{member, "1767312000"}}));
        EXPECT_EQ(rate->update.next.qualified_utc_ms, january + day_ms);
        EXPECT_EQ(rate->use, fx::RateUse::latest_candidate);
        expect_open_disclosure(*rate);
    }
}

TEST(ExchangeRateApi, ProfilesExposeSupportedOperationsAndNominalRefresh) {
    using enum fx::Profile;
    struct Expected {
        fx::Profile profile;
        minutes refresh;
        bool history;
        bool quota;
    };
    for (const Expected expected :
         {Expected{open, 1440min, false, false}, Expected{free, 1440min, false, true},
          Expected{pro, 60min, true, true}, Expected{business, 5min, true, true},
          Expected{volume, 5min, true, true}}) {
        SCOPED_TRACE(static_cast<int>(expected.profile));
        Fixture f;
        const auto selection = setup(expected.profile);
        ASSERT_EQ(f.select(selection), fx::SelectionStatus::selected);
        const auto selected = f.adapter.current();
        ASSERT_TRUE(selected);
        EXPECT_EQ(selected->binding, selection.selection);
        EXPECT_EQ(selected->nominal_refresh, expected.refresh);
        EXPECT_TRUE(selected->latest);
        EXPECT_EQ(selected->history, expected.history);
        EXPECT_EQ(selected->quota, expected.quota);
        if (!expected.history)
            expect_blocked(f, history("blocked-history"), clock(0), unsupported_operation);
        if (!expected.quota)
            expect_blocked(f, quota("blocked-quota"), clock(0), unsupported_operation);
        if (expected.profile == open || expected.profile == free) {
            EXPECT_EQ(f.adapter.edit_reference_allowance(50'000),
                      fx::SelectionStatus::reference_not_allowed);
        }
        EXPECT_TRUE(f.transport.calls.empty());
    }
}

TEST(ExchangeRateApi, SetupAndOperationRefuseBeforeSelectorsOrDispatch) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::unknown)), fx::SelectionStatus::unknown_profile);
    EXPECT_FALSE(f.adapter.current());
    const auto free = setup(fx::Profile::free);
    ASSERT_EQ(f.select(free), fx::SelectionStatus::selected);
    for (int mismatch = 0; mismatch < 4; ++mismatch) {
        SCOPED_TRACE(mismatch);
        auto foreign = setup(fx::Profile::pro, "B");
        auto &binding = foreign.credential->binding;
        switch (mismatch) {
        case 0:
            binding.scope.installation_ref = "other-install";
            break;
        case 1:
            binding.scope.scope_ref = "other-scope";
            break;
        case 2:
            binding.selection_revision = "other-selection";
            break;
        case 3:
            binding.credential_revision = "other-credential";
            break;
        }
        EXPECT_EQ(f.select(foreign), fx::SelectionStatus::invalid_credential_binding);
    }
    const auto current = f.adapter.current();
    ASSERT_TRUE(current);
    EXPECT_EQ(current->binding, free.selection);
    auto wrong_history = history("free-malformed-history");
    wrong_history.history_date = "2023-02-29";
    const auto refused = f.adapter.start(wrong_history, clock(0));
    const auto *reason = std::get_if<fx::Failure>(&refused);
    ASSERT_NE(reason, nullptr);
    EXPECT_TRUE(reason->kind == unsupported_operation || reason->kind == invalid_selectors);
    expect_public_failure(*reason);
    EXPECT_TRUE(f.transport.calls.empty());

    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    auto bad_date = history("pro-malformed-history");
    bad_date.history_date = "2023-02-29";
    expect_blocked(f, bad_date, clock(0), invalid_selectors);
    expect_blocked(f, latest("empty", {}), clock(0), invalid_selectors);
    expect_blocked(f, latest("duplicate", {"EUR", "EUR"}), clock(0), invalid_selectors);
    expect_blocked(f, latest("lowercase", {"eur"}), clock(0), invalid_selectors);
    expect_blocked(f, latest("self", {"USD"}), clock(0), invalid_selectors);
    auto malformed_base = latest("base");
    malformed_base.base = "US/";
    expect_blocked(f, malformed_base, clock(0), invalid_selectors);
    auto latest_with_date = latest("extra-date");
    latest_with_date.history_date = "2024-02-29";
    expect_blocked(f, latest_with_date, clock(0), invalid_selectors);
    auto quota_with_base = quota("extra-base");
    quota_with_base.base = "USD";
    expect_blocked(f, quota_with_base, clock(0), invalid_selectors);
    EXPECT_TRUE(f.transport.calls.empty());
}

TEST(ExchangeRateApi, LatestPreservesSelectionOrderAndTextOnlyDoesNotQualifyAnInstant) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::free)), fx::SelectionStatus::selected);
    const auto result = f.exchange(
        latest("ordered", {"CHF", "EUR"}), clock(10),
        frame(
            R"({"result":"success","base_code":"USD","conversion_rates":{"EUR":0.90,"CHF":8e-1},"time_last_update_utc":"Thu, 01 Jan 2026 00:00:00 +0000"})",
            clock(20)));
    const auto *rate = std::get_if<fx::Rate>(&result.outcome);
    ASSERT_NE(rate, nullptr);
    const auto &data = rate->observation.data();
    ASSERT_EQ(data.quotes.size(), 2u);
    EXPECT_EQ(data.quotes[0].to_currency, "CHF");
    EXPECT_EQ(data.quotes[0].rate.original(), "8e-1");
    EXPECT_EQ(data.quotes[0].rate.value(), core::Exact(4) / 5);
    EXPECT_EQ(data.quotes[1].to_currency, "EUR");
    EXPECT_EQ(data.quotes[1].rate.original(), "0.90");
    EXPECT_EQ(rate->update.last.utc_text, "Thu, 01 Jan 2026 00:00:00 +0000");
    EXPECT_FALSE(rate->update.last.qualified_utc_ms);
    EXPECT_FALSE(data.context.source_rate_time.utc_ms);
    EXPECT_FALSE(data.context.fetched_at_utc_ms);
    EXPECT_FALSE(data.context.publication_time.utc_ms);
}

constexpr std::size_t body_limit = 256 * 1024;
constexpr std::size_t header_limit = 16 * 1024;

std::string nested_body(std::size_t arrays) {
    auto body = std::string(keyed_latest_body);
    body.pop_back();
    return body + ",\"nested\":" + std::string(arrays, '[') + "0" + std::string(arrays, ']') + "}";
}

fx::Headers sized_headers(std::size_t bytes) {
    // The owned header block uses "X-Note: " + value + CRLF, then a final CRLF.
    return {{"X-Note", std::string(bytes - 12, 'x')}};
}

TEST(ExchangeRateApi, CompleteFramesWithInvalidEvidenceOrExceededBoundsAreRefused) {
    struct Variant {
        std::string body;
        fx::FailureKind failure;
        std::optional<std::size_t> wire_size{};
        fx::Headers headers{};
        std::vector<std::string> quotes{"EUR"};
    };
    const auto valid = std::string(keyed_latest_body);
    const std::array variants{
        Variant{valid, resource_limit, body_limit + 1},
        Variant{valid + std::string(body_limit - valid.size() + 1, ' '), resource_limit, 1024},
        Variant{valid, resource_limit, {}, sized_headers(header_limit + 1)},
        Variant{nested_body(16), resource_limit},
        Variant{valid.substr(0, valid.size() - 1) + R"(,"future":1,"future":2})", invalid_document},
        Variant{valid.substr(0, valid.size() - 1) + R"(,"time_last_update_unix":9223372036854776})",
                // One second above the greatest signed count that can be scaled to milliseconds.
                invalid_document},
        Variant{R"({"result":"success","base_code":"USD","conversion_rates":{"EUR":0,"CHF":0.9}})",
                invalid_document,
                {},
                {},
                {"EUR", "CHF"}},
        Variant{R"({"result":"success","base_code":"USD","conversion_rates":{"EUR":0.9,"xYz":1}})",
                invalid_document},
        Variant{R"({"result":"success","base_code":"USD","conversion_rates":{"CHF":0.9}})",
                invalid_document},
    };
    for (std::size_t n = 0; n < variants.size(); ++n) {
        SCOPED_TRACE(n);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
        const auto &variant = variants[n];
        auto reply = frame(variant.body, clock(20), 200, variant.headers);
        if (variant.wire_size)
            reply.wire_body_bytes = *variant.wire_size;
        expect_failure(f.exchange(latest("bounded", variant.quotes), clock(10), std::move(reply)),
                       variant.failure);
        EXPECT_EQ(f.view(clock(20)).local_total, (fx::Usage{1, 0}));
    }
}

TEST(ExchangeRateApi, WireDecodedHeaderAndNestingCeilingsAreInclusive) {
    const auto valid = std::string(keyed_latest_body);
    auto wire = frame(valid, clock(20));
    wire.wire_body_bytes = body_limit;
    auto decoded = frame(valid + std::string(body_limit - valid.size(), ' '), clock(20));
    decoded.wire_body_bytes = 1024;
    auto headers = frame(valid, clock(20), 200, sized_headers(header_limit));
    auto nesting = frame(nested_body(15), clock(20)); // Root object plus 15 arrays.

    const std::array replies{std::move(wire), std::move(decoded), std::move(headers),
                             std::move(nesting)};
    for (std::size_t n = 0; n < replies.size(); ++n) {
        SCOPED_TRACE(n);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
        const auto result = f.exchange(latest("at-limit"), clock(10), replies[n]);
        const auto *rate = std::get_if<fx::Rate>(&result.outcome);
        ASSERT_NE(rate, nullptr);
        ASSERT_EQ(rate->observation.data().quotes.size(), 1u);
        EXPECT_EQ(rate->observation.data().quotes.front().rate.value(), core::Exact(9) / 10);
    }
}

TEST(ExchangeRateApi, TransportResourceRefusalRemainsTypedAndMeteredWhenSent) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    const auto result =
        f.exchange(latest("limited-reply"), clock(10),
                   fx::FailedTransport{fx::TransportFailure::resource_limit, clock(20)});
    expect_failure(result, resource_limit);
    EXPECT_EQ(result.dispatch.kind, fx::DispatchKind::sent);
    EXPECT_EQ(f.view(clock(20)).local_total, (fx::Usage{1, 0}));
    EXPECT_FALSE(f.view(clock(20)).sample);
}

TEST(ExchangeRateApi, GreatestScalableUnixSecondRetainsExactSourceTime) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    const auto body = std::string(keyed_latest_body.substr(0, keyed_latest_body.size() - 1)) +
                      R"(,"time_last_update_unix":9223372036854775})";
    const auto result = f.exchange(latest("max-source-instant"), clock(10), frame(body, clock(20)));
    const auto *rate = std::get_if<fx::Rate>(&result.outcome);
    ASSERT_NE(rate, nullptr);
    EXPECT_EQ(rate->update.last.unix_seconds, 9'223'372'036'854'775LL);
    EXPECT_EQ(rate->update.last.qualified_utc_ms, 9'223'372'036'854'775'000LL);
    EXPECT_EQ(rate->observation.data().context.source_rate_time.utc_ms,
              9'223'372'036'854'775'000LL);
}

TEST(ExchangeRateApi, SupersededLatestSettlesDispatchWithoutPublishingRates) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    SnapshotOwner owner;
    const auto old = f.start(latest("superseded-latest"), clock(10), sent(10));
    const auto current = setup(fx::Profile::pro, "B");
    ASSERT_EQ(f.select(current), fx::SelectionStatus::selected);
    const auto result = f.finish(old, frame(std::string(keyed_latest_body), clock(20, january)));
    expect_failure(result, superseded);
    EXPECT_EQ(result.admission, old.admission);
    EXPECT_EQ(result.dispatch.kind, fx::DispatchKind::sent);
    owner.consider(result);
    owner.expect_cached();
    EXPECT_EQ(f.view(clock(20)).local_total, (fx::Usage{1, 0}));
    EXPECT_FALSE(f.view(clock(20)).sample);

    const auto request = latest("current-latest");
    const auto fresh =
        f.exchange(request, clock(21), frame(std::string(keyed_latest_body), clock(22, january)));
    const auto *rate = std::get_if<fx::Rate>(&fresh.outcome);
    ASSERT_NE(rate, nullptr);
    EXPECT_EQ(rate->use, fx::RateUse::latest_candidate);
    EXPECT_EQ(fresh.admission, (fx::Admission{current.selection, request, false}));
}

} // namespace
