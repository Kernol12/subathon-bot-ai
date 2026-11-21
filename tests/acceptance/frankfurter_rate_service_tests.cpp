// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <string_view>
#include <subathon/core/valuation_snapshot_codec.hpp>
#include <subathon/server/frankfurter_rates.hpp>

namespace {
using namespace std::chrono;
using namespace subathon::core;
using namespace subathon::server;

constexpr auto t =
    (sys_days{2026y / November / 16} + hours{12}).time_since_epoch() / milliseconds{1};
constexpr auto hour = hours{1} / milliseconds{1};
constexpr auto fetched = t + 96 * hour;

FrankfurterRequest admission(std::vector<std::string> quotes = {"GBP", "USD"}) {
    return {"c0", "q1", "e1", "EUR", std::move(quotes), {}};
}

FetchedRateObservationInput observation(std::string id, std::string date,
                                        std::vector<RateQuoteInput> quotes,
                                        std::optional<std::int64_t> fetch_time = fetched,
                                        std::string request = "q1", std::string evidence = "e1") {
    return {{std::move(id),
             "Frankfurter/ECB",
             "c0",
             std::move(request),
             std::move(evidence),
             {},
             fetch_time,
             {std::move(date), {}},
             {}},
            std::move(quotes)};
}

FetchedRateObservationInput old_observation() {
    return observation("o0", "2026-11-16", {{"EUR", "USD", "1.10"}, {"EUR", "GBP", "0.80"}}, t,
                       "q0", "e0");
}

EffectiveRateSnapshotInput old_input() {
    return {"S0",
            {},
            {},
            {old_observation()},
            {},
            {FetchedRateConstraint{"o0", 1}, FetchedRateConstraint{"o0", 2}},
            {{{{"EUR", "1", "1"}, {"GBP", "4", "5"}, {"USD", "11", "10"}}}}};
}

EffectiveRateSnapshot read_back(const EffectiveRateSnapshotInput &input) {
    const auto envelope = encode_valuation_snapshot(make_effective_rate_snapshot(input));
    return decode_valuation_snapshot(envelope.format_ref, envelope.document);
}

RateSourceBasis old_basis() {
    // The owner retains provider-layer edges independently of RSV1.
    return {"Frankfurter/ECB", {old_observation()}, {{0, 1}, {0, 2}}};
}

ManualRateOverrideInput manual(std::string id, RateOverrideKind kind, std::string from,
                               std::string to, std::string decimal) {
    return {id,
            kind,
            std::move(from),
            std::move(to),
            std::move(decimal),
            "decision-" + id,
            "fixture-actor",
            "Synthetic " + id,
            {}};
}

FrankfurterFetch mixed_fetch() {
    return {admission(),
            {make_fetched_rate_observation(
                 observation("new-usd", "2026-11-19", {{"EUR", "USD", "1.20"}})),
             make_fetched_rate_observation(
                 observation("older-gbp", "2026-11-18", {{"EUR", "GBP", "0.84"}}))},
            {},
            {}};
}

// F5's accepted state written independently of the proposal under test.
EffectiveRateSnapshotInput accepted_mixed_input() {
    auto input = old_input();
    input.snapshot_ref = "S1";
    input.parent_snapshot_ref = "S0";
    input.sources.push_back(observation("new-usd", "2026-11-19", {{"EUR", "USD", "1.20"}}));
    input.constraints = {FetchedRateConstraint{"new-usd", 1}, FetchedRateConstraint{"o0", 2}};
    input.components[0].members[2] = {"USD", "6", "5"};
    return input;
}

void expect_rate(const EffectiveRateSnapshotInput &candidate, std::string_view from,
                 std::string_view to, const Exact &expected) {
    const auto result = quote_effective_rate(candidate, from, to);
    EXPECT_EQ(result.data().kind, RatePairKind::available);
    EXPECT_EQ(result.data().rate, expected);
}

void expect_edges(const RateSourceBasis &basis, std::initializer_list<RateSourceEdge> edges) {
    ASSERT_EQ(basis.edges.size(), edges.size());
    EXPECT_TRUE(std::equal(basis.edges.begin(), basis.edges.end(), edges.begin(),
                           [](const auto &left, const auto &right) {
                               return left.observation_index == right.observation_index &&
                                      left.quote_ordinal == right.quote_ordinal;
                           }));
}

void expect_carried_old(const FetchedRateObservationInput &prior) {
    EXPECT_EQ(prior.context.observation_ref, "o0");
    EXPECT_EQ(prior.context.request_ref, "q0");
    EXPECT_EQ(prior.context.source_evidence_ref, "e0");
    EXPECT_EQ(prior.context.source_rate_time.original_text, "2026-11-16");
    EXPECT_EQ(prior.context.fetched_at_utc_ms, t);
    ASSERT_EQ(prior.quotes.size(), 2u);
    EXPECT_EQ(prior.quotes[1].to_currency, "GBP");
    EXPECT_EQ(prior.quotes[1].original_decimal, "0.80");
}

TEST(FrankfurterRateService, CompleteLatestProposalPreservesOrdinalAndExactCrossRate) {
    // F1. The model fixture exercises selection without relying on the HTTP tests.
    const auto current = read_back(old_input());
    const auto before = encode_valuation_snapshot(current);
    const auto request = admission();
    const FrankfurterFetch fetch{
        request,
        {make_fetched_rate_observation(
            observation("new", "2026-11-19", {{"EUR", "USD", "1.2000"}, {"EUR", "GBP", "8e-1"}}))},
        {},
        {}};
    const auto result = propose_frankfurter_latest(fetch, request, current, old_basis(), "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    EXPECT_EQ(proposal->candidate.snapshot_ref, "S1");
    EXPECT_EQ(proposal->candidate.parent_snapshot_ref, "S0");
    EXPECT_EQ(proposal->selected_observation.data().context.observation_ref, "new");
    EXPECT_TRUE(proposal->retained_quotes.empty());
    EXPECT_TRUE(proposal->missing_quotes.empty());
    expect_edges(proposal->fetched_basis, {{0, 1}, {0, 2}});
    ASSERT_EQ(proposal->fetched_basis.observations.size(), 1u);
    EXPECT_EQ(proposal->fetched_basis.observations[0].quotes.at(0).original_decimal, "1.2000");
    EXPECT_EQ(proposal->fetched_basis.observations[0].quotes.at(1).original_decimal, "8e-1");
    expect_rate(proposal->candidate, "USD", "GBP", Exact(2) / 3);
    EXPECT_EQ(encode_valuation_snapshot(current).document, before.document);
}

TEST(FrankfurterRateService, HistoricalMismatchCannotActivateLatest) {
    // F2.
    const auto current = read_back(old_input());
    const auto before = encode_valuation_snapshot(current);
    auto request = admission({"USD"});
    request.historical_date = "2026-11-19";
    const FrankfurterFetch fetch{request,
                                 {make_fetched_rate_observation(
                                     observation("hist", "2026-11-18", {{"EUR", "USD", "1.250"}}))},
                                 {},
                                 {{"USD", "2026-11-19", "2026-11-18", false}}};
    const auto result = propose_frankfurter_latest(fetch, request, current, old_basis(), "S1");
    const auto *refusal = std::get_if<FrankfurterLatestRefusal>(&result);
    ASSERT_NE(refusal, nullptr);
    EXPECT_EQ(*refusal, FrankfurterLatestRefusal::historical);
    EXPECT_EQ(encode_valuation_snapshot(current).document, before.document);
}

TEST(FrankfurterRateService, PartialAcceptanceDistinguishesPriorRateFromMissingPair) {
    // F4.
    const auto request = admission({"CAD", "GBP", "USD"});
    const FrankfurterFetch fetch{
        request,
        {make_fetched_rate_observation(observation("new", "2026-11-19", {{"EUR", "USD", "1.22"}}))},
        {"CAD", "GBP"},
        {}};
    const auto current = read_back(old_input());
    const auto before = encode_valuation_snapshot(current);
    const auto result = propose_frankfurter_latest(fetch, request, current, old_basis(), "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    EXPECT_EQ(proposal->retained_quotes, std::vector<std::string>{"GBP"});
    EXPECT_EQ(proposal->missing_quotes, std::vector<std::string>{"CAD"});
    expect_edges(proposal->fetched_basis, {{0, 1}, {1, 2}});
    ASSERT_EQ(proposal->fetched_basis.observations.size(), 2u);
    EXPECT_EQ(proposal->fetched_basis.observations[0].context.fetched_at_utc_ms, fetched);
    expect_carried_old(proposal->fetched_basis.observations[1]);
    expect_rate(proposal->candidate, "EUR", "USD", Exact(61) / 50);
    expect_rate(proposal->candidate, "EUR", "GBP", Exact(4) / 5);
    const auto cad = quote_effective_rate(proposal->candidate, "EUR", "CAD");
    EXPECT_EQ(cad.data().kind, RatePairKind::missing);
    EXPECT_FALSE(cad.data().rate);
    EXPECT_EQ(encode_valuation_snapshot(current).document, before.document);
}

TEST(FrankfurterRateService, EmptyAndIdentityOnlyResponsesCannotActivate) {
    const auto current = read_back(old_input());
    const auto request = admission({"EUR", "USD"});
    for (const auto &groups : std::vector<std::vector<FetchedRateObservation>>{
             {},
             {make_fetched_rate_observation(
                 observation("identity", "2026-11-19", {{"EUR", "EUR", "1.000"}}))}}) {
        const FrankfurterFetch fetch{
            request, groups, groups.empty() ? request.quotes : std::vector<std::string>{"USD"}, {}};
        const auto result = propose_frankfurter_latest(fetch, request, current, old_basis(), "S1");
        const auto *refusal = std::get_if<FrankfurterLatestRefusal>(&result);
        ASSERT_NE(refusal, nullptr);
        EXPECT_EQ(*refusal, FrankfurterLatestRefusal::no_usable_quotes);
    }
}

TEST(FrankfurterRateService, NewestUsableGroupAloneDeterminesTemporaryOverrideRelevance) {
    // F5. Include a still newer identity-only group.
    auto fetch = mixed_fetch();
    std::swap(fetch.observations[0], fetch.observations[1]);
    fetch.admitted = admission({"EUR", "GBP", "USD"});
    fetch.observations.insert(
        fetch.observations.begin(),
        make_fetched_rate_observation(observation("self", "2026-11-20", {{"EUR", "EUR", "1"}})));
    auto input = old_input();
    input.overrides = {manual("temp-gbp", RateOverrideKind::temporary, "EUR", "GBP", "0.80")};
    input.constraints.insert(input.constraints.begin(), ManualRateConstraint{"temp-gbp"});
    const auto current = read_back(input);
    const auto result =
        propose_frankfurter_latest(fetch, fetch.admitted, current, old_basis(), "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    EXPECT_EQ(proposal->selected_observation.data().context.observation_ref, "new-usd");
    EXPECT_EQ(proposal->retained_quotes, std::vector<std::string>{"GBP"});
    ASSERT_EQ(proposal->candidate.overrides.size(), 1u);
    EXPECT_EQ(proposal->candidate.overrides[0].override_ref, "temp-gbp");
    EXPECT_EQ(proposal->candidate.overrides[0].decision_ref, "decision-temp-gbp");
    expect_edges(proposal->fetched_basis, {{0, 1}, {1, 2}});
    const auto &basis = proposal->fetched_basis.observations;
    ASSERT_EQ(basis.size(), 2u);
    EXPECT_EQ(basis[0].context.source_rate_time.original_text, "2026-11-19");
    expect_carried_old(basis[1]);
    expect_rate(proposal->candidate, "EUR", "USD", Exact(6) / 5);
    expect_rate(proposal->candidate, "EUR", "GBP", Exact(4) / 5);
}

TEST(FrankfurterRateService, NewEdgesKeepOriginalOrdinalsAndCarriedEdgesFollowPairOrder) {
    auto input = old_input();
    input.sources[0].quotes.push_back({"EUR", "CAD", "1.50"});
    input.constraints.emplace_back(FetchedRateConstraint{"o0", 3});
    // CAD is the first currency, so all potentials are expressed relative to CAD.
    input.components[0].members = {
        {"CAD", "1", "1"}, {"EUR", "2", "3"}, {"GBP", "8", "15"}, {"USD", "11", "15"}};
    auto basis = old_basis();
    basis.observations = input.sources;
    basis.edges.push_back({0, 3});
    const auto request = admission({"CAD", "EUR", "GBP", "JPY", "USD"});
    const FrankfurterFetch fetch{
        request,
        {make_fetched_rate_observation(
            observation("new", "2026-11-19",
                        {{"EUR", "JPY", "160"}, {"EUR", "EUR", "1"}, {"EUR", "USD", "1.20"}}))},
        {"CAD", "GBP"},
        {}};
    const auto result = propose_frankfurter_latest(fetch, request, read_back(input), basis, "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    expect_edges(proposal->fetched_basis, {{0, 1}, {0, 3}, {1, 3}, {1, 2}});
    ASSERT_EQ(proposal->fetched_basis.observations.size(), 2u);
    EXPECT_EQ(proposal->fetched_basis.observations[0].context.observation_ref, "new");
    EXPECT_EQ(proposal->fetched_basis.observations[1].context.observation_ref, "o0");
    expect_rate(proposal->candidate, "EUR", "JPY", Exact(160));
    expect_rate(proposal->candidate, "EUR", "USD", Exact(6) / 5);
}

TEST(FrankfurterRateService, CrossPinKeepsLosingFetchedEdgeForLaterReplacement) {
    // F5 cross-pin control. With USD first, EUR→GBP is (6/5)/(11/8) = 48/55.
    auto input = old_input();
    input.overrides = {manual("pin", RateOverrideKind::pinned, "GBP", "USD", "1.375")};
    input.constraints.insert(input.constraints.begin(), ManualRateConstraint{"pin"});
    const auto current = read_back(input);
    const auto fetch = mixed_fetch();
    const auto result =
        propose_frankfurter_latest(fetch, fetch.admitted, current, old_basis(), "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    expect_rate(proposal->candidate, "EUR", "USD", Exact(6) / 5);
    expect_rate(proposal->candidate, "EUR", "GBP", Exact(48) / 55);
    const auto &basis = proposal->fetched_basis;
    expect_edges(basis, {{0, 1}, {1, 2}});
    ASSERT_EQ(basis.observations.size(), 2u);
    EXPECT_EQ(basis.observations[0].context.observation_ref, "new-usd");
    expect_carried_old(basis.observations[1]);

    // Fixture acceptance: effective RSV1 drops o0, while the owner keeps both fetched edges.
    const auto accepted = read_back(proposal->candidate);
    ASSERT_EQ(accepted.data().sources.size(), 1u);
    EXPECT_EQ(accepted.data().sources[0].data().context.observation_ref, "new-usd");
    std::vector<FetchedRateObservation> observations;
    observations.reserve(basis.observations.size());
    for (const auto &raw : basis.observations)
        observations.push_back(make_fetched_rate_observation(raw));
    std::vector<RateOverrideFetchedEdge> edges;
    for (const auto &edge : basis.edges)
        edges.push_back({observations.at(edge.observation_index), edge.quote_ordinal});

    const auto edit = manual("replacement", RateOverrideKind::pinned, "GBP", "USD", "1.6");
    // Preview and save's pure recomputation both use the shipped proposal with this basis.
    const auto preview = preview_rate_override(accepted, edit, edges, "S2", "pin");
    EXPECT_EQ(preview.replaced_override_ref, "pin");
    expect_rate(preview.candidate, "EUR", "USD", Exact(6) / 5);
    expect_rate(preview.candidate, "EUR", "GBP", Exact(3) / 4);
    ASSERT_EQ(preview.candidate.sources.size(), 1u);
    EXPECT_EQ(preview.candidate.sources[0].context.observation_ref, "new-usd");
    expect_edges(basis, {{0, 1}, {1, 2}});
    expect_carried_old(basis.observations[1]);
}

TEST(FrankfurterRateService, PinWinsAndNewGraphExpiresTemporaryEvenAtSameRate) {
    // F9.
    auto input = old_input();
    input.overrides = {manual("pin-gbp", RateOverrideKind::pinned, "EUR", "GBP", "0.80"),
                       manual("temp-usd", RateOverrideKind::temporary, "EUR", "USD", "1.10")};
    input.constraints.insert(input.constraints.begin(),
                             {ManualRateConstraint{"pin-gbp"}, ManualRateConstraint{"temp-usd"}});
    const auto current = read_back(input);
    const auto request = admission();
    const FrankfurterFetch fetch{
        request,
        {make_fetched_rate_observation(
            observation("new", "2026-11-19", {{"EUR", "GBP", "0.85"}, {"EUR", "USD", "1.10"}}))},
        {},
        {}};
    const auto result = propose_frankfurter_latest(fetch, request, current, old_basis(), "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    ASSERT_EQ(proposal->candidate.overrides.size(), 1u);
    EXPECT_EQ(proposal->candidate.overrides[0].override_ref, "pin-gbp");
    EXPECT_EQ(proposal->candidate.overrides[0].decision_ref, "decision-pin-gbp");
    EXPECT_EQ(current.data().overrides.size(), 2u);
    expect_rate(proposal->candidate, "EUR", "GBP", Exact(4) / 5);
    expect_rate(proposal->candidate, "EUR", "USD", Exact(11) / 10);
    expect_edges(proposal->fetched_basis, {{0, 1}, {0, 2}});
    ASSERT_EQ(proposal->candidate.sources.size(), 1u);
    EXPECT_EQ(proposal->candidate.sources[0].quotes.at(0).original_decimal, "0.85");
    EXPECT_EQ(proposal->candidate.sources[0].context.fetched_at_utc_ms, fetched);
}

TEST(FrankfurterRateService, EqualDateCanRefreshButEarlierDateRefuses) {
    // F10.
    const auto current = read_back(old_input());
    const auto before = encode_valuation_snapshot(current);
    const auto request = admission();
    for (const auto *date : {"2026-11-15", "2026-11-16"}) {
        SCOPED_TRACE(date);
        const FrankfurterFetch fetch{
            request,
            {make_fetched_rate_observation(observation(
                "new", date, {{"EUR", "USD", "1.10"}, {"EUR", "GBP", "0.80"}}, t + 12 * hour))},
            {},
            {}};
        const auto result = propose_frankfurter_latest(fetch, request, current, old_basis(), "S1");
        EXPECT_EQ(encode_valuation_snapshot(current).document, before.document);
        if (std::string_view{date} == "2026-11-15") {
            const auto *refusal = std::get_if<FrankfurterLatestRefusal>(&result);
            ASSERT_NE(refusal, nullptr);
            EXPECT_EQ(*refusal, FrankfurterLatestRefusal::regression);
            continue;
        }
        const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
        ASSERT_NE(proposal, nullptr);
        const auto accepted = read_back(proposal->candidate);
        ASSERT_EQ(accepted.data().sources.size(), 1u);
        const auto &context = accepted.data().sources[0].data().context;
        EXPECT_EQ(context.source_rate_time.original_text, date);
        EXPECT_FALSE(context.source_rate_time.utc_ms);
        EXPECT_FALSE(context.publication_time.utc_ms);
        EXPECT_EQ(context.request_ref, "q1");
        EXPECT_EQ(context.source_evidence_ref, "e1");
        EXPECT_EQ(context.fetched_at_utc_ms, t + 12 * hour);
    }
}

TEST(FrankfurterRateService, OneRegressingPairRefusesWholeGroupEvenIfAnotherAdvances) {
    // F10 C2. The owner already accepted F5, then admitted a distinct attempt.
    const auto input = accepted_mixed_input();
    const auto current = read_back(input);
    const auto before = encode_valuation_snapshot(current);
    const RateSourceBasis basis{
        "Frankfurter/ECB", {input.sources[1], input.sources[0]}, {{0, 1}, {1, 2}}};
    auto request = admission();
    request.request_ref = "q2";
    request.evidence_ref = "e2";
    const FrankfurterFetch fetch{
        request,
        {make_fetched_rate_observation(observation("o2", "2026-11-18",
                                                   {{"EUR", "USD", "1.20"}, {"EUR", "GBP", "0.84"}},
                                                   fetched + hour, "q2", "e2"))},
        {},
        {}};
    const auto result = propose_frankfurter_latest(fetch, request, current, basis, "S2");
    const auto *refusal = std::get_if<FrankfurterLatestRefusal>(&result);
    ASSERT_NE(refusal, nullptr);
    EXPECT_EQ(*refusal, FrankfurterLatestRefusal::regression);
    EXPECT_EQ(encode_valuation_snapshot(current).document, before.document);
}

TEST(FrankfurterRateService, ValidSuccessWithoutUtcCanProposeButSupersededAttemptsCannot) {
    // F7. The owner can advance either the configuration or just its admitted attempt.
    const auto current = read_back(old_input());
    const auto before = encode_valuation_snapshot(current);
    const auto request = admission();
    const FrankfurterFetch fetch{
        request,
        {make_fetched_rate_observation(observation(
            "new", "2026-11-19", {{"EUR", "GBP", "0.85"}, {"EUR", "USD", "1.25"}}, std::nullopt))},
        {},
        {}};
    const auto basis = old_basis();
    auto changed_configuration = request;
    changed_configuration.configuration_ref = "c1";
    auto newer_attempt = request;
    newer_attempt.request_ref = "q2";
    newer_attempt.evidence_ref = "e2";
    for (const auto &newer : {changed_configuration, newer_attempt}) {
        SCOPED_TRACE(newer.configuration_ref + "/" + newer.request_ref);
        const auto result = propose_frankfurter_latest(fetch, newer, current, basis, "S1");
        const auto *refusal = std::get_if<FrankfurterLatestRefusal>(&result);
        ASSERT_NE(refusal, nullptr);
        EXPECT_EQ(*refusal, FrankfurterLatestRefusal::superseded);
    }

    const auto result = propose_frankfurter_latest(fetch, request, current, basis, "S1");
    const auto *proposal = std::get_if<FrankfurterLatestProposal>(&result);
    ASSERT_NE(proposal, nullptr);
    EXPECT_FALSE(proposal->selected_observation.data().context.fetched_at_utc_ms);
    expect_rate(proposal->candidate, "EUR", "GBP", Exact(17) / 20);
    expect_rate(proposal->candidate, "EUR", "USD", Exact(5) / 4);
    // The supplied snapshot is still readable after the pure proposal call.
    EXPECT_EQ(encode_valuation_snapshot(current).document, before.document);
}

TEST(FrankfurterFreshness, FetchAgeUsesAcceptedReceiptAndWarnsAtEquality) {
    // F1 and F3 age samples come from the owner, independent of the latest attempt.
    const auto prior = read_back(old_input());
    for (const auto &[elapsed, warning] : {std::pair{24 * hour - 1, FrankfurterWarning::clear},
                                           {24 * hour, FrankfurterWarning::due}}) {
        const auto status = assess_frankfurter_freshness(
            {FrankfurterHistory::accepted, t, prior.data().sources, t + elapsed});
        EXPECT_EQ(status.history, FrankfurterHistory::accepted);
        EXPECT_EQ(status.last_successful_fetch_utc_ms, t);
        EXPECT_EQ(status.fetch_age_ms, elapsed);
        EXPECT_EQ(status.fetch_warning, warning);
        ASSERT_EQ(status.observations.size(), 1u);
        EXPECT_FALSE(status.observations[0].publication_age_ms);
    }
    const auto current = read_back(accepted_mixed_input());
    const auto status = assess_frankfurter_freshness(
        {FrankfurterHistory::accepted, fetched, current.data().sources, fetched + hour});
    EXPECT_EQ(status.last_successful_fetch_utc_ms, fetched);
    EXPECT_EQ(status.fetch_age_ms, hour);
    EXPECT_EQ(status.fetch_warning, FrankfurterWarning::clear);
}

TEST(FrankfurterFreshness, ReturnedDateWarningUsesStrictThresholdPerObservation) {
    // F11. Following midnight for USD is 20 Nov at 00:00. GBP is three days older.
    constexpr auto boundary = sys_days{2026y / November / 23}.time_since_epoch() / milliseconds{1};
    const auto current = read_back(accepted_mixed_input());
    for (const auto &now : {std::optional<std::int64_t>{boundary}, {boundary + 1}, {}}) {
        const auto status = assess_frankfurter_freshness(
            {FrankfurterHistory::accepted, fetched, current.data().sources, now});
        ASSERT_EQ(status.observations.size(), 2u);
        for (const auto &row : status.observations) {
            const bool usd = row.observation_ref == "new-usd";
            if (!usd) {
                EXPECT_EQ(row.observation_ref, "o0");
            }
            EXPECT_EQ(row.rate_date, usd ? "2026-11-19" : "2026-11-16");
            EXPECT_EQ(row.fetched_at_utc_ms, usd ? fetched : t);
            EXPECT_EQ(row.fetch_age_ms,
                      now ? std::optional<std::int64_t>{*now - (usd ? fetched : t)} : std::nullopt);
            EXPECT_EQ(row.rate_date_warning, !now                      ? FrankfurterWarning::unknown
                                             : usd && *now == boundary ? FrankfurterWarning::clear
                                                                       : FrankfurterWarning::due);
            EXPECT_FALSE(row.publication_age_ms);
        }
        EXPECT_NE(status.observations[0].observation_ref, status.observations[1].observation_ref);
    }
}

TEST(FrankfurterFreshness, MissingOrUnusableClocksDoNotFabricateAgeOrPublication) {
    // F7 and F11. No fetch instant was captured for this owned observation.
    const std::vector active{make_fetched_rate_observation(
        observation("no-clock", "2026-11-19", {{"EUR", "USD", "1.25"}}, std::nullopt))};
    for (const auto history : {FrankfurterHistory::accepted, FrankfurterHistory::never_succeeded,
                               FrankfurterHistory::unknown}) {
        const auto status =
            assess_frankfurter_freshness({history,
                                          {},
                                          history == FrankfurterHistory::accepted
                                              ? std::span<const FetchedRateObservation>{active}
                                              : std::span<const FetchedRateObservation>{},
                                          fetched + hour});
        EXPECT_EQ(status.history, history);
        EXPECT_FALSE(status.fetch_age_ms);
        EXPECT_FALSE(status.last_successful_fetch_utc_ms);
        if (history == FrankfurterHistory::accepted) {
            EXPECT_EQ(status.fetch_warning, FrankfurterWarning::unknown);
            ASSERT_EQ(status.observations.size(), 1u);
            const auto &row = status.observations[0];
            EXPECT_FALSE(row.fetched_at_utc_ms);
            EXPECT_FALSE(row.fetch_age_ms);
            EXPECT_EQ(row.rate_date, "2026-11-19");
            EXPECT_EQ(row.rate_date_warning, FrankfurterWarning::clear);
            EXPECT_FALSE(row.publication_age_ms);
        }
    }

    using Limits = std::numeric_limits<std::int64_t>;
    for (const auto &[sample, now] :
         {std::pair<std::int64_t, std::int64_t>{t, t - 1}, {Limits::min(), Limits::max()}}) {
        const auto status =
            assess_frankfurter_freshness({FrankfurterHistory::accepted, sample, {}, now});
        EXPECT_FALSE(status.fetch_age_ms);
        EXPECT_EQ(status.fetch_warning, FrankfurterWarning::unknown);
    }
}

} // namespace
