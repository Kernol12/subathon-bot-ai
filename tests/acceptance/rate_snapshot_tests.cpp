// SPDX-License-Identifier: GPL-3.0-or-later
// RS01–17, specification NUM-04/FX-06, rate-snapshots-api-v1.md.
// All fixtures are raw inputs; expected fractions and budgets are independent literals.
#include "../fixtures/rate_snapshots_vectors.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <source_location>
#include <string>
#include <type_traits>
#include <vector>

namespace {
using namespace subathon::core;
namespace vectors = subathon::test::rates;
using boost::multiprecision::cpp_int;
using Error = RateSnapshotError;

// EXPECT_THROW is nonfatal on the distinct marker: every later row remains reachable.
template <typename Call>
void rejects(Error code, Call &&call,
             std::source_location where = std::source_location::current()) {
    SCOPED_TRACE(testing::Message()
                 << "feature call at " << where.file_name() << ':' << where.line());
    EXPECT_THROW(
        {
            try {
                static_cast<void>(call());
            } catch (const RateSnapshotFailure &failure) {
                EXPECT_EQ(failure.code(), code);
                EXPECT_STREQ(failure.what(), "Invalid rate snapshot input");
                throw;
            }
        },
        RateSnapshotFailure);
}
template <typename Call>
void accepts(Call &&call, std::source_location where = std::source_location::current()) {
    SCOPED_TRACE(testing::Message()
                 << "feature call at " << where.file_name() << ':' << where.line());
    EXPECT_NO_THROW(call());
}

static_assert(!std::is_default_constructible_v<RateMoney>);
static_assert(!std::is_constructible_v<RateMoney, RateMoneyData>);
static_assert(!std::is_default_constructible_v<FetchedRateObservation>);
static_assert(!std::is_constructible_v<FetchedRateObservation, FetchedRateObservationData>);
static_assert(!std::is_default_constructible_v<EffectiveRateSnapshot>);
static_assert(!std::is_constructible_v<EffectiveRateSnapshot, EffectiveRateSnapshotData>);
static_assert(!std::is_default_constructible_v<RatePairResult>);
static_assert(!std::is_constructible_v<RatePairResult, RatePairResultData>);
static_assert(std::is_same_v<decltype(std::declval<RateMoney &>().data()), const RateMoneyData &>);
static_assert(std::is_copy_constructible_v<EffectiveRateSnapshot>);
static_assert(std::is_move_assignable_v<RatePairResult>);

TEST(RateMoneyAcceptance, PreservesPrecisionSignedAmountsAndSignedZero) {
    struct Row {
        const char *text;
        Exact value;
    };
    const Row rows[] = {{"1.234500", Exact(2469) / 2000},
                        {"-1.234500", -Exact(2469) / 2000},
                        {"-0.000e+03", Exact(0)},
                        {"1.2e+00", Exact(6) / 5}};
    for (const auto &row : rows) {
        SCOPED_TRACE(row.text);
        accepts([&] {
            const auto result = make_rate_money({row.text, "USD"});
            EXPECT_EQ(result.data().original.original_decimal, row.text);
            EXPECT_EQ(result.data().original.currency, "USD");
            EXPECT_EQ(result.data().value, row.value);
        });
    }
}

TEST(RateMoneyAcceptance, OwnsInputAfterMutationDestructionAndResultCopies) {
    accepts([] {
        const auto result = [] {
            ReceiptMoney input{"1.234500", "USD"};
            auto owned = make_rate_money(input);
            EXPECT_EQ(input.original_decimal, "1.234500");
            EXPECT_EQ(input.currency, "USD");
            input.original_decimal = "9";
            input.currency.clear();
            return owned;
        }();
        auto copy = result;
        auto moved = std::move(copy);
        EXPECT_EQ(result.data().original.original_decimal, "1.234500");
        EXPECT_EQ(moved.data().original.currency, "USD");
        EXPECT_EQ(moved.data().value, Exact(2469) / 2000);
    });
}

TEST(RateMoneyAcceptance, CurrencyIsOpaqueBytesRatherThanIsoOrNormalizedText) {
    for (const std::string &currency : {std::string("usd"), std::string("U\0\xff", 3)}) {
        SCOPED_TRACE(currency);
        accepts([&] {
            EXPECT_EQ(make_rate_money({"1", currency}).data().original.currency, currency);
        });
    }
    rejects(Error::invalid_currency, [] { return make_rate_money({"1", ""}); });
}

TEST(RateMoneyAcceptance, RejectsDecimalGrammarWithoutTruncatingOrFloatingConversion) {
    for (const std::string &value :
         std::vector<std::string>{std::string(""), "+1", "01", "1/2", "NaN", "Infinity", " 1", "1 ",
                                  "1e", "1,2", std::string("1\0x", 3)}) {
        SCOPED_TRACE(value);
        rejects(Error::invalid_decimal, [&] { return make_rate_money({value, "U"}); });
    }
}

TEST(RateMoneyAcceptance, FixedDecimalLimitsAndCompleteGrammarHaveTheirRequiredPriority) {
    for (const std::string &value :
         std::vector<std::string>{std::string(513, '?'), std::string(257, '9'),
                                  std::string("1e257"), std::string("0e-257")}) {
        SCOPED_TRACE(value.size());
        rejects(Error::resource_limit, [&] { return make_rate_money({value, "U"}); });
    }
    for (const std::string &value :
         std::vector<std::string>{std::string(257, '9') + '?', std::string("1e257?")}) {
        SCOPED_TRACE(value.size());
        rejects(Error::invalid_decimal, [&] { return make_rate_money({value, "U"}); });
    }
}

TEST(RateMoneyAcceptance, AcceptsExactDecimalLexicalBoundaries) {
    const std::array<std::pair<std::string, Exact>, 3> rows = {
        {{"1e" + std::string(510, '0'), Exact(1)},
         {std::string(256, '9'), Exact(boost::multiprecision::pow(cpp_int(10), 256) - 1)},
         {"1e-256", Exact(1) / boost::multiprecision::pow(cpp_int(10), 256)}}};
    for (const auto &[text, expected] : rows) {
        SCOPED_TRACE(text.size());
        accepts([&] { EXPECT_EQ(make_rate_money({text, "U"}).data().value, expected); });
    }
}

TEST(RateMoneyAcceptance, CompleteOutputBudgetIncludesExactValueAndOriginalBytes) {
    RateSnapshotLimits limits;
    limits.owned_bytes = 4;
    accepts([&] { EXPECT_EQ(make_rate_money({"1", "U"}, limits).data().value, 1); });
    limits.owned_bytes = 3;
    rejects(Error::resource_limit, [&] { return make_rate_money({"1", "U"}, limits); });
    limits.owned_bytes = 1;
    rejects(Error::resource_limit, [&] { return make_rate_money({"?", "U"}, limits); });
    for (const auto &[spelling, bytes] :
         {std::pair{"-1", std::size_t(6)}, std::pair{"-0", std::size_t(5)}}) {
        SCOPED_TRACE(spelling);
        limits.owned_bytes = bytes;
        accepts([&] { static_cast<void>(make_rate_money({spelling, "U"}, limits)); });
        --limits.owned_bytes;
        rejects(Error::resource_limit, [&] { return make_rate_money({spelling, "U"}, limits); });
    }
}

TEST(RateObservationAcceptance, RetainsPrecisionAndQuoteOrderWithHonestUnknownMetadata) {
    accepts([] {
        const auto result = make_fetched_rate_observation(vectors::observation());
        const auto &data = result.data();
        EXPECT_EQ(data.context.provider_ref, "provider-A");
        EXPECT_EQ(data.context.configuration_ref, "config-1");
        EXPECT_EQ(data.context.request_ref, "request-1");
        EXPECT_EQ(data.context.source_evidence_ref, "evidence-1");
        EXPECT_FALSE(data.context.provider_snapshot_ref);
        EXPECT_FALSE(data.context.fetched_at_utc_ms);
        EXPECT_FALSE(data.context.source_rate_time.original_text);
        EXPECT_FALSE(data.context.source_rate_time.utc_ms);
        EXPECT_FALSE(data.context.publication_time.original_text);
        EXPECT_FALSE(data.context.publication_time.utc_ms);
        EXPECT_EQ(data.quotes.size(), 2U);
        EXPECT_EQ(data.quotes.at(0).rate.original(), "1.2000");
        EXPECT_EQ(data.quotes.at(0).rate.value(), Exact(6) / 5);
        EXPECT_EQ(data.quotes.at(1).to_currency, "GBP");
        EXPECT_EQ(data.quotes.at(1).rate.original(), "0.6000");
        EXPECT_EQ(data.quotes.at(1).rate.value(), Exact(3) / 5);
    });
}

TEST(RateObservationAcceptance, TimeEvidencePreservesEmptyDateEpochAndSignedExtremaIndependently) {
    auto input = vectors::observation();
    input.context.provider_snapshot_ref = std::string("v\0x", 3);
    input.context.fetched_at_utc_ms = 0;
    input.context.source_rate_time = {"2026-11-04", std::nullopt};
    input.context.publication_time = {"", std::numeric_limits<std::int64_t>::min()};
    accepts([&] {
        const auto result = make_fetched_rate_observation(input);
        EXPECT_EQ(result.data().context.provider_snapshot_ref, std::string("v\0x", 3));
        EXPECT_EQ(result.data().context.fetched_at_utc_ms, 0);
        EXPECT_EQ(result.data().context.source_rate_time.original_text, "2026-11-04");
        EXPECT_FALSE(result.data().context.source_rate_time.utc_ms);
        EXPECT_EQ(result.data().context.publication_time.original_text, "");
        EXPECT_EQ(result.data().context.publication_time.utc_ms,
                  std::numeric_limits<std::int64_t>::min());
    });
    input.context.source_rate_time = {std::nullopt, std::numeric_limits<std::int64_t>::max()};
    accepts([&] {
        EXPECT_EQ(make_fetched_rate_observation(input).data().context.source_rate_time.utc_ms,
                  std::numeric_limits<std::int64_t>::max());
    });
}

TEST(RateObservationAcceptance, OwnsAllContextAndQuoteBuffersAfterInputDies) {
    accepts([] {
        const auto result = [] {
            auto input = vectors::observation();
            input.context.source_rate_time.original_text = "raw-date";
            auto owned = make_fetched_rate_observation(input);
            EXPECT_EQ(input.context.observation_ref, "O1");
            EXPECT_EQ(input.quotes.at(0).original_decimal, "1.2000");
            EXPECT_EQ(input.context.source_rate_time.original_text, "raw-date");
            input.context.observation_ref.clear();
            input.quotes.clear();
            input.context.source_rate_time.original_text->clear();
            return owned;
        }();
        const auto copy = result;
        EXPECT_EQ(copy.data().context.observation_ref, "O1");
        EXPECT_EQ(copy.data().context.source_rate_time.original_text, "raw-date");
        EXPECT_EQ(result.data().quotes.at(1).rate.value(), Exact(3) / 5);
    });
}

TEST(RateObservationAcceptance, KeepsRoundedInconsistentCyclesAndPositiveSelfQuotesAsData) {
    auto input = vectors::observation();
    input.quotes.push_back({"EUR", "GBP", "0.7"});
    input.quotes.push_back({"USD", "USD", "2"});
    accepts([&] {
        const auto result = make_fetched_rate_observation(input);
        EXPECT_EQ(result.data().quotes.size(), 4U);
        EXPECT_EQ(result.data().quotes.at(2).rate.value(), Exact(7) / 10);
        EXPECT_EQ(result.data().quotes.at(3).rate.value(), 2);
    });
}

TEST(RateObservationAcceptance, RejectsDuplicateDirectedQuotesRatherThanChoosingSpelling) {
    for (const char *rate : {"1.2000", "1.2", "9"}) {
        SCOPED_TRACE(rate);
        auto input = vectors::observation();
        input.quotes.push_back({"USD", "EUR", rate});
        rejects(Error::duplicate_quote, [&] { return make_fetched_rate_observation(input); });
    }
}

TEST(RateObservationAcceptance, RatesRequireStrictlyPositiveExactValues) {
    for (const char *rate : {"1.20", "1.2000", "1.2e+00"}) {
        SCOPED_TRACE(rate);
        auto input = vectors::observation();
        input.quotes[0].original_decimal = rate;
        accepts([&] {
            const auto result = make_fetched_rate_observation(input);
            EXPECT_EQ(result.data().quotes.at(0).rate.original(), rate);
            EXPECT_EQ(result.data().quotes.at(0).rate.value(), Exact(6) / 5);
        });
    }
    for (const char *rate : {"0", "-0.00", "-1"}) {
        SCOPED_TRACE(rate);
        auto input = vectors::observation();
        input.quotes[0].original_decimal = rate;
        rejects(Error::nonpositive_rate, [&] { return make_fetched_rate_observation(input); });
    }
    for (const char *rate : {"NaN", "1/2", " 1.2", "1.2?"}) {
        SCOPED_TRACE(rate);
        auto input = vectors::observation();
        input.quotes[0].original_decimal = rate;
        rejects(Error::invalid_decimal, [&] { return make_fetched_rate_observation(input); });
    }
}

TEST(RateObservationAcceptance, RequiredAndOptionalReferencesAndCurrencyDomainsAreChecked) {
    const std::array<std::string RateObservationContext::*, 5> fields = {
        &RateObservationContext::observation_ref, &RateObservationContext::provider_ref,
        &RateObservationContext::configuration_ref, &RateObservationContext::request_ref,
        &RateObservationContext::source_evidence_ref};
    for (std::size_t index = 0; index < fields.size(); ++index) {
        SCOPED_TRACE(index);
        auto input = vectors::observation();
        input.context.*fields[index] = "";
        rejects(Error::invalid_reference, [&] { return make_fetched_rate_observation(input); });
    }
    auto input = vectors::observation();
    input.context.provider_snapshot_ref = "";
    rejects(Error::invalid_reference, [&] { return make_fetched_rate_observation(input); });
    input = vectors::observation();
    input.quotes[0].from_currency.clear();
    rejects(Error::invalid_currency, [&] { return make_fetched_rate_observation(input); });
    input = vectors::observation();
    input.quotes[0].to_currency.clear();
    rejects(Error::invalid_currency, [&] { return make_fetched_rate_observation(input); });
    input = vectors::observation();
    input.quotes.clear();
    accepts([&] { EXPECT_TRUE(make_fetched_rate_observation(input).data().quotes.empty()); });
}

TEST(RateQueryAcceptance, DirectRawQueriesGiveLiteralReciprocalAndCrossFractions) {
    struct Row {
        const char *from;
        const char *to;
        int numerator;
        int denominator;
    };
    for (const Row row : {Row{"EUR", "USD", 5, 6},
                          {"EUR", "GBP", 1, 2},
                          {"GBP", "USD", 5, 3},
                          {"USD", "GBP", 3, 5}}) {
        SCOPED_TRACE(std::string(row.from) + "->" + row.to);
        accepts([&] {
            const auto result = quote_effective_rate(vectors::snapshot(), row.from, row.to);
            EXPECT_EQ(result.data().kind, RatePairKind::available);
            EXPECT_EQ(result.data().rate, Exact(row.numerator) / row.denominator);
            EXPECT_EQ(result.data().provenance.value().snapshot_ref, "S1");
            EXPECT_EQ(result.data().provenance.value().from_currency, row.from);
            EXPECT_EQ(result.data().provenance.value().to_currency, row.to);
        });
    }
}

TEST(RateQueryAcceptance, IdentityDoesNotAssertCoverageAndMissingIsNotZero) {
    for (const char *key : {"USD", "CAD", "usd"}) {
        SCOPED_TRACE(key);
        accepts([&] {
            const auto result = quote_effective_rate(vectors::snapshot(), key, key);
            EXPECT_EQ(result.data().kind, RatePairKind::identity);
            EXPECT_EQ(result.data().rate, Exact(1));
            EXPECT_FALSE(result.data().provenance);
        });
    }
    for (const auto &[from, to] : {std::pair{"USD", "CAD"}, {"usd", "USD"}, {"CAD", "EUR"}}) {
        SCOPED_TRACE(std::string(from) + "->" + to);
        accepts([&] {
            const auto result = quote_effective_rate(vectors::snapshot(), from, to);
            EXPECT_EQ(result.data().kind, RatePairKind::missing);
            EXPECT_FALSE(result.data().rate);
            EXPECT_FALSE(result.data().provenance);
        });
    }
}

TEST(RateQueryAcceptance, SeparateComponentsCannotInventForeignConversions) {
    auto input = vectors::snapshot();
    input.components.push_back({{{"ZZZ", "1", "1"}}});
    accepts([&] {
        const auto result = quote_effective_rate(input, "USD", "ZZZ");
        EXPECT_EQ(result.data().kind, RatePairKind::missing);
        EXPECT_FALSE(result.data().rate);
    });
}

TEST(RateQueryAcceptance, RevalidatesCompleteCandidateBeforeEveryDisposition) {
    auto input = vectors::snapshot();
    input.components[0].members[2].denominator = "7";
    for (const auto &[from, to] : {std::pair{"CAD", "CAD"}, {"EUR", "USD"}, {"USD", "CAD"}}) {
        SCOPED_TRACE(std::string(from) + "->" + to);
        rejects(Error::contradictory_constraint,
                [&] { return quote_effective_rate(input, from, to); });
    }
    input = vectors::snapshot();
    input.sources[0].quotes[1].original_decimal = "NaN";
    rejects(Error::invalid_decimal, [&] { return quote_effective_rate(input, "CAD", "CAD"); });
}

TEST(RateQueryAcceptance, ChecksQueryKeyBoundsAndCandidateLexicalPriority) {
    rejects(Error::invalid_currency,
            [] { return quote_effective_rate(vectors::snapshot(), "", "USD"); });
    rejects(Error::invalid_currency,
            [] { return quote_effective_rate(vectors::snapshot(), "USD", ""); });
    auto input = vectors::snapshot();
    input.snapshot_ref.clear();
    rejects(Error::resource_limit,
            [&] { return quote_effective_rate(input, std::string(65, 'x'), ""); });
    rejects(Error::invalid_reference, [&] { return quote_effective_rate(input, "", ""); });
    const std::string key("\xff\0", 2);
    accepts([&] {
        EXPECT_EQ(quote_effective_rate(vectors::empty_snapshot(), key, key).data().rate, Exact(1));
    });
}

TEST(RateQueryAcceptance, PairResultOwnsItsRateAndAllProvenanceStrings) {
    accepts([] {
        auto input = vectors::snapshot();
        std::string from = "EUR", to = "USD";
        auto result = quote_effective_rate(input, from, to);
        EXPECT_EQ(input.snapshot_ref, "S1");
        EXPECT_EQ(input.sources.at(0).quotes.at(0).original_decimal, "1.2000");
        EXPECT_EQ(from, "EUR");
        EXPECT_EQ(to, "USD");
        input.snapshot_ref.clear();
        input.sources.clear();
        from.clear();
        to.clear();
        auto copy = result;
        result = quote_effective_rate(vectors::empty_snapshot(), "x", "x");
        EXPECT_EQ(copy.data().rate, Exact(5) / 6);
        EXPECT_EQ(copy.data().provenance.value().snapshot_ref, "S1");
        EXPECT_EQ(copy.data().provenance.value().from_currency, "EUR");
        EXPECT_EQ(copy.data().provenance.value().to_currency, "USD");
    });
}

TEST(RateQueryAcceptance, PairProjectionIsSeparateFromSnapshotAndHasNoPhantomQueryCopies) {
    RateSnapshotLimits limits;
    limits.owned_bytes = 2;
    rejects(Error::resource_limit,
            [&] { return quote_effective_rate(vectors::empty_snapshot(), "u", "u", limits); });
    limits.owned_bytes = 3;
    accepts([&] {
        EXPECT_EQ(quote_effective_rate(vectors::empty_snapshot(), std::string(64, 'u'),
                                       std::string(64, 'u'), limits)
                      .data()
                      .rate,
                  Exact(1));
    });
    limits.owned_bytes = 1;
    accepts([&] {
        EXPECT_EQ(quote_effective_rate(vectors::empty_snapshot(), "u", "v", limits).data().kind,
                  RatePairKind::missing);
    });
    limits.owned_bytes = 105;
    rejects(Error::resource_limit,
            [&] { return quote_effective_rate(vectors::snapshot(), "u", "u", limits); });
}

TEST(RateSnapshotAcceptance, OwnsCompleteS1AndPreservesHistoricalPrecisionAfterInputChanges) {
    accepts([] {
        auto input = vectors::snapshot();
        input.parent_snapshot_ref = "prior";
        input.constructed_at_utc_ms = -1;
        input.sources[0].context.provider_snapshot_ref = "version";
        input.sources[0].context.fetched_at_utc_ms = 0;
        input.sources[0].context.source_rate_time = {"date-only", std::nullopt};
        input.sources[0].context.publication_time = {"", -1};
        auto result = make_effective_rate_snapshot(input);
        EXPECT_EQ(input.snapshot_ref, "S1");
        EXPECT_EQ(input.sources.at(0).quotes.at(0).original_decimal, "1.2000");
        EXPECT_EQ(input.components.at(0).members.at(2).numerator, "5");
        EXPECT_EQ(input.constraints.size(), 2U);
        input.snapshot_ref.clear();
        input.sources[0].quotes[0].original_decimal = "99";
        input.components.clear();
        input.constraints.clear();
        auto copy = result;
        EXPECT_EQ(result.data().snapshot_ref, "S1");
        EXPECT_EQ(copy.data().parent_snapshot_ref, "prior");
        EXPECT_EQ(copy.data().constructed_at_utc_ms, -1);
        EXPECT_EQ(copy.data().sources.at(0).data().context.provider_snapshot_ref, "version");
        EXPECT_EQ(copy.data().sources.at(0).data().context.fetched_at_utc_ms, 0);
        EXPECT_EQ(copy.data().sources.at(0).data().context.source_rate_time.original_text,
                  "date-only");
        EXPECT_FALSE(copy.data().sources.at(0).data().context.source_rate_time.utc_ms);
        EXPECT_EQ(copy.data().sources.at(0).data().context.publication_time.original_text, "");
        EXPECT_EQ(copy.data().sources.at(0).data().context.publication_time.utc_ms, -1);
        EXPECT_EQ(copy.data().sources.at(0).data().quotes.at(0).rate.original(), "1.2000");
        EXPECT_EQ(copy.data().components.at(0).members.at(0).currency, "EUR");
        EXPECT_EQ(copy.data().components.at(0).members.at(0).weight, 1);
        EXPECT_EQ(copy.data().components.at(0).members.at(1).weight, Exact(1) / 2);
        EXPECT_EQ(copy.data().components.at(0).members.at(2).weight, Exact(5) / 6);
        EXPECT_EQ(std::get<FetchedRateConstraint>(copy.data().constraints.at(1)).quote_ordinal, 2U);
    });
}

TEST(RateSnapshotAcceptance, SortsSourceOverrideAndTypedConstraintKeysWithoutReorderingQuotes) {
    auto input = vectors::short_snapshot();
    auto second = vectors::short_observation();
    second.context.observation_ref = "Z";
    input.sources.insert(input.sources.begin(), second);
    auto manual = vectors::short_override();
    manual.override_ref = "Z";
    input.overrides = {manual, vectors::short_override()};
    input.constraints = {ManualRateConstraint{"Z"}, FetchedRateConstraint{"Z", 1},
                         ManualRateConstraint{"M"}, FetchedRateConstraint{"O", 1}};
    accepts([&] {
        const auto result = make_effective_rate_snapshot(input);
        EXPECT_EQ(result.data().sources.at(0).data().context.observation_ref, "O");
        EXPECT_EQ(result.data().sources.at(1).data().context.observation_ref, "Z");
        EXPECT_EQ(result.data().overrides.at(0).override_ref, "M");
        EXPECT_EQ(result.data().overrides.at(1).override_ref, "Z");
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(0)).observation_ref,
                  "O");
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(1)).observation_ref,
                  "Z");
        EXPECT_EQ(std::get<ManualRateConstraint>(result.data().constraints.at(2)).override_ref,
                  "M");
        EXPECT_EQ(std::get<ManualRateConstraint>(result.data().constraints.at(3)).override_ref,
                  "Z");
    });
    auto numeric = vectors::short_snapshot();
    numeric.sources[0].quotes = {{"A", "B", "1"}, {"A", "C", "1"}, {"A", "D", "1"}, {"B", "A", "1"},
                                 {"B", "C", "1"}, {"B", "D", "1"}, {"C", "A", "1"}, {"C", "B", "1"},
                                 {"C", "D", "1"}, {"D", "A", "1"}};
    numeric.components = {{{{"A", "1", "1"}, {"B", "1", "1"}, {"C", "1", "1"}, {"D", "1", "1"}}}};
    numeric.constraints.clear();
    for (std::uint64_t ordinal = 10; ordinal > 0; --ordinal)
        numeric.constraints.push_back(FetchedRateConstraint{"O", ordinal});
    accepts([&] {
        const auto result = make_effective_rate_snapshot(numeric);
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(1)).quote_ordinal,
                  2U);
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(9)).quote_ordinal,
                  10U);
        EXPECT_EQ(result.data().sources.at(0).data().quotes.at(9).from_currency, "D");
    });
}

TEST(RateSnapshotAcceptance, IdentityOrderingUsesUnsignedBytesAndTypedNamespaces) {
    auto input = vectors::short_snapshot();
    input.sources[0].context.observation_ref = std::string("\xff", 1);
    auto second = vectors::short_observation();
    second.context.observation_ref = std::string("\x80", 1);
    input.sources.push_back(second);
    auto manual = vectors::short_override();
    manual.override_ref = std::string("\xff", 1);
    input.overrides.push_back(manual);
    input.constraints = {FetchedRateConstraint{std::string("\xff", 1), 1},
                         ManualRateConstraint{std::string("\xff", 1)},
                         FetchedRateConstraint{std::string("\x80", 1), 1}};
    accepts([&] {
        const auto result = make_effective_rate_snapshot(input);
        EXPECT_EQ(result.data().sources.at(0).data().context.observation_ref,
                  std::string("\x80", 1));
        EXPECT_EQ(result.data().sources.at(1).data().context.observation_ref,
                  std::string("\xff", 1));
        EXPECT_EQ(result.data().overrides.at(0).override_ref, std::string("\xff", 1));
    });

    // ASCII must precede high-bit bytes; the old high-bit-only pair cannot detect signed char.
    auto mixed = vectors::short_snapshot();
    auto high = vectors::short_observation();
    high.context.observation_ref = std::string("\xff", 1);
    auto middle = vectors::short_observation();
    middle.context.observation_ref = std::string("\x80", 1);
    mixed.sources = {high, vectors::short_observation(), middle};
    mixed.constraints = {FetchedRateConstraint{std::string("\xff", 1), 1},
                         FetchedRateConstraint{"O", 1},
                         FetchedRateConstraint{std::string("\x80", 1), 1}};
    accepts([&] {
        const auto result = make_effective_rate_snapshot(mixed);
        EXPECT_EQ(result.data().sources.at(0).data().context.observation_ref, "O");
        EXPECT_EQ(result.data().sources.at(1).data().context.observation_ref,
                  std::string("\x80", 1));
        EXPECT_EQ(result.data().sources.at(2).data().context.observation_ref,
                  std::string("\xff", 1));
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(0)).observation_ref,
                  "O");
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(1)).observation_ref,
                  std::string("\x80", 1));
        EXPECT_EQ(std::get<FetchedRateConstraint>(result.data().constraints.at(2)).observation_ref,
                  std::string("\xff", 1));
    });
}

TEST(RateSnapshotAcceptance, RetainsUnselectedRoundedQuotesAndConsistentRedundantCycles) {
    for (const char *rate : {"0.7", "0.5"}) {
        SCOPED_TRACE(rate);
        auto input = vectors::snapshot();
        input.sources[0].quotes.push_back({"EUR", "GBP", rate});
        if (std::string(rate) == "0.5")
            input.constraints.push_back(FetchedRateConstraint{"O1", 3});
        accepts([&] {
            const auto result = make_effective_rate_snapshot(input);
            EXPECT_EQ(result.data().sources.at(0).data().quotes.size(), 3U);
            EXPECT_EQ(result.data().sources.at(0).data().quotes.at(2).rate.original(), rate);
            EXPECT_EQ(result.data().components.at(0).members.at(1).weight, Exact(1) / 2);
        });
    }
}

TEST(RateSnapshotAcceptance, SelectingInconsistentFetchedCycleRejectsWithoutChangingEvidence) {
    auto input = vectors::snapshot();
    input.sources[0].quotes.push_back({"EUR", "GBP", "0.7"});
    input.constraints.push_back(FetchedRateConstraint{"O1", 3});
    rejects(Error::contradictory_constraint, [&] { return make_effective_rate_snapshot(input); });
    EXPECT_EQ(input.sources[0].quotes[0].original_decimal, "1.2000");
    EXPECT_EQ(input.sources[0].quotes[2].original_decimal, "0.7");
    EXPECT_EQ(input.constraints.size(), 3U);
}

TEST(RateSnapshotAcceptance, PinOnlySnapshotPreservesManualEvidenceAndExternalFetchedOriginal) {
    const auto original = vectors::observation();
    auto input = vectors::pinned_snapshot();
    accepts([&] {
        const auto result = make_effective_rate_snapshot(input);
        EXPECT_TRUE(result.data().sources.empty());
        EXPECT_EQ(result.data().overrides.at(0).kind, RateOverrideKind::pinned);
        EXPECT_EQ(result.data().overrides.at(0).rate.original(), "1.25");
        EXPECT_EQ(result.data().overrides.at(0).decision_ref, "decision-1");
        EXPECT_FALSE(result.data().overrides.at(0).actor_ref);
        EXPECT_EQ(result.data().overrides.at(1).kind, RateOverrideKind::temporary);
        EXPECT_EQ(result.data().overrides.at(1).actor_ref, "actor-2");
        EXPECT_EQ(result.data().overrides.at(1).recorded_at_utc_ms, 0);
        EXPECT_EQ(result.data().overrides.at(1).reason, "Synthetic temporary two");
        EXPECT_EQ(result.data().components.at(0).members.at(2).weight, Exact(4) / 5);
    });
    accepts(
        [&] { EXPECT_EQ(quote_effective_rate(input, "USD", "GBP").data().rate, Exact(5) / 8); });
    EXPECT_EQ(original.quotes[0].original_decimal, "1.2000");
    EXPECT_EQ(original.quotes[1].original_decimal, "0.6000");
}

TEST(RateSnapshotAcceptance, ConflictingPinsAreRejectedRatherThanDiscardedOrRounded) {
    auto input = vectors::pinned_snapshot();
    input.overrides.push_back(
        {"P3", RateOverrideKind::pinned, "USD", "GBP", "0.7", "D", {}, "R", {}});
    input.constraints.push_back(ManualRateConstraint{"P3"});
    rejects(Error::contradictory_constraint, [&] { return make_effective_rate_snapshot(input); });
    EXPECT_EQ(input.overrides.size(), 3U);
    EXPECT_EQ(input.overrides[2].original_decimal, "0.7");
}

TEST(RateSnapshotAcceptance, EveryOverrideAndIncludedSourceMustHaveAnActiveReference) {
    auto input = vectors::pinned_snapshot();
    input.constraints.pop_back();
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.sources.push_back(vectors::short_observation());
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.sources.push_back(vectors::observation());
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, ReferencesOrdinalsAndEndpointsMustActuallyResolve) {
    for (const std::uint64_t ordinal :
         {std::uint64_t(0), std::uint64_t(3), std::numeric_limits<std::uint64_t>::max()}) {
        SCOPED_TRACE(ordinal);
        auto input = vectors::snapshot();
        std::get<FetchedRateConstraint>(input.constraints[0]).quote_ordinal = ordinal;
        rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
    }
    auto input = vectors::snapshot();
    std::get<FetchedRateConstraint>(input.constraints[0]).observation_ref = "unknown";
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    std::get<ManualRateConstraint>(input.constraints[0]).override_ref = "unknown";
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.components[0].members.pop_back();
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::short_snapshot();
    input.components = {{{{"A", "1", "1"}}}, {{{"B", "1", "1"}}}};
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, SelfQuotesCannotBecomeActiveConstraints) {
    auto input = vectors::short_snapshot();
    input.sources[0].quotes.push_back({"A", "A", "1"});
    input.constraints.push_back(FetchedRateConstraint{"O", 2});
    rejects(Error::unresolved_basis, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, ComponentCanonicalOrderAnchorAndUniqueMembershipAreRequired) {
    using Mutation = std::function<void(EffectiveRateSnapshotInput &)>;
    const Mutation rows[] = {
        [](auto &s) { s.components[0].members.clear(); },
        [](auto &s) { std::swap(s.components[0].members[0], s.components[0].members[1]); },
        [](auto &s) { s.components[0].members[0].numerator = "2"; },
        [](auto &s) { s.components[0].members[1].currency = "A"; },
        [](auto &s) { s.components.push_back({{{"A", "1", "1"}}}); },
        [](auto &s) { s.components.insert(s.components.begin(), {{{"Z", "1", "1"}}}); },
    };
    for (std::size_t row = 0; row < std::size(rows); ++row) {
        SCOPED_TRACE(row);
        auto input = vectors::short_snapshot();
        rows[row](input);
        rejects(Error::noncanonical_component, [&] { return make_effective_rate_snapshot(input); });
    }
}

TEST(RateSnapshotAcceptance, EmptyAndSingletonTablesAreValidButUnbackedForeignWeightsAreNot) {
    accepts([] {
        EXPECT_TRUE(
            make_effective_rate_snapshot(vectors::empty_snapshot()).data().components.empty());
    });
    auto input = vectors::empty_snapshot();
    input.components = {{{{"X", "1", "1"}}}};
    accepts([&] {
        EXPECT_EQ(make_effective_rate_snapshot(input).data().components.at(0).members.at(0).weight,
                  1);
    });
    input.components[0].members.push_back({"Y", "2", "1"});
    rejects(Error::unbacked_component, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, PotentialGrammarAndReductionAreStrictForBothIntegers) {
    for (const std::string &value : std::vector<std::string>{"", "0", "-1", "+1", "01", " 1", "1 ",
                                                             "1e0", "1/2", std::string("1\0", 2)}) {
        for (const bool denominator : {false, true}) {
            SCOPED_TRACE(value);
            SCOPED_TRACE(denominator);
            auto input = vectors::snapshot();
            (denominator ? input.components[0].members[2].denominator
                         : input.components[0].members[2].numerator) = value;
            rejects(Error::invalid_potential, [&] { return make_effective_rate_snapshot(input); });
        }
    }
    auto input = vectors::snapshot();
    input.components[0].members[2].numerator = "10";
    input.components[0].members[2].denominator = "12";
    rejects(Error::invalid_potential, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, FullWidthOverrideEnumsEndpointsAndAuditShapeAreChecked) {
    for (const int bad : {-256, -1, 2, 256}) {
        SCOPED_TRACE(bad);
        auto input = vectors::pinned_snapshot();
        input.overrides[0].kind = static_cast<RateOverrideKind>(bad);
        rejects(Error::invalid_override, [&] { return make_effective_rate_snapshot(input); });
    }
    auto input = vectors::pinned_snapshot();
    input.overrides[0].to_currency = "USD";
    rejects(Error::invalid_override, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.overrides[0].reason.clear();
    rejects(Error::invalid_override, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.overrides[0].decision_ref.clear();
    rejects(Error::invalid_reference, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.overrides[0].actor_ref = "";
    rejects(Error::invalid_reference, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.parent_snapshot_ref = "";
    rejects(Error::invalid_reference, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.overrides[0].override_ref.clear();
    rejects(Error::invalid_reference, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.overrides[0].from_currency.clear();
    rejects(Error::invalid_currency, [&] { return make_effective_rate_snapshot(input); });
    for (const char *rate : {"0", "-0.00", "-1"}) {
        SCOPED_TRACE(rate);
        input = vectors::pinned_snapshot();
        input.overrides[0].original_decimal = rate;
        rejects(Error::nonpositive_rate, [&] { return make_effective_rate_snapshot(input); });
    }
    input = vectors::pinned_snapshot();
    input.overrides[0].original_decimal = "1/2";
    rejects(Error::invalid_decimal, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, DuplicateCategoriesDoNotOverwriteEvidence) {
    auto input = vectors::snapshot();
    input.sources.push_back(input.sources[0]);
    rejects(Error::duplicate_identity, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.overrides.push_back(input.overrides[0]);
    rejects(Error::duplicate_identity, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.sources[0].quotes.push_back(input.sources[0].quotes[0]);
    rejects(Error::duplicate_quote, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.constraints.push_back(input.constraints[0]);
    rejects(Error::duplicate_constraint, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::pinned_snapshot();
    input.constraints.push_back(input.constraints[0]);
    rejects(Error::duplicate_constraint, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateSnapshotAcceptance, LargeComponentBoundariesAndAn8192BitPairRemainExact) {
    const auto input = vectors::large_snapshot();
    accepts([&] {
        const auto result = make_effective_rate_snapshot(input);
        EXPECT_EQ(result.data().components.at(0).members.at(5).weight,
                  Exact(boost::multiprecision::pow(cpp_int(3), 2584)));
        EXPECT_EQ(result.data().components.at(0).members.at(10).weight,
                  Exact(1) / boost::multiprecision::pow(cpp_int(5), 1764));
    });
    accepts([&] {
        const auto result = quote_effective_rate(input, "K", "F");
        EXPECT_EQ(result.data().rate, Exact(boost::multiprecision::pow(cpp_int(3), 2584)) *
                                          boost::multiprecision::pow(cpp_int(5), 1764));
    });
    RateSnapshotLimits limits;
    limits.potential_component_bits = 4095;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
}

TEST(RateSnapshotAcceptance, IntegerBitAndByteRefusalsPrecedeRationalOrShapeWork) {
    for (const bool denominator : {false, true}) {
        auto input = vectors::snapshot();
        const cpp_int over = cpp_int(1) << 4096;
        (denominator ? input.components[0].members[2].denominator
                     : input.components[0].members[2].numerator) = over.str();
        rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
    }
    auto input = vectors::snapshot();
    input.snapshot_ref.clear();
    input.components[0].members[2].numerator = std::string(1235, '?');
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateLimitsAcceptance, EveryOptionRejectsZeroAndAboveMaximumOnAllFourEntrypoints) {
    const std::array<std::pair<std::size_t RateSnapshotLimits::*, std::size_t>, 13> fields = {
        {{&RateSnapshotLimits::currency_bytes, 64},
         {&RateSnapshotLimits::reference_bytes, 256},
         {&RateSnapshotLimits::raw_time_bytes, 512},
         {&RateSnapshotLimits::reason_bytes, 4096},
         {&RateSnapshotLimits::observations, 16},
         {&RateSnapshotLimits::quotes_per_observation, 4096},
         {&RateSnapshotLimits::total_quotes, 16384},
         {&RateSnapshotLimits::overrides, 1024},
         {&RateSnapshotLimits::constraints, 4096},
         {&RateSnapshotLimits::currencies, 256},
         {&RateSnapshotLimits::items, 32768},
         {&RateSnapshotLimits::owned_bytes, 8388608},
         {&RateSnapshotLimits::potential_component_bits, 4096}}};
    for (std::size_t index = 0; index < fields.size(); ++index) {
        SCOPED_TRACE(index);
        for (const std::size_t bad : {std::size_t(0), fields[index].second + 1}) {
            SCOPED_TRACE(bad);
            RateSnapshotLimits limits;
            limits.*fields[index].first = bad;
            rejects(Error::invalid_options, [&] { return make_rate_money({"?", ""}, limits); });
            rejects(Error::invalid_options,
                    [&] { return make_fetched_rate_observation(vectors::observation(), limits); });
            rejects(Error::invalid_options,
                    [&] { return make_effective_rate_snapshot(vectors::snapshot(), limits); });
            rejects(Error::invalid_options,
                    [&] { return quote_effective_rate(vectors::snapshot(), "", "", limits); });
        }
    }
}

TEST(RateLimitsAcceptance, LiteralS1InputOutputByteAndRecursiveItemBoundariesAreDistinct) {
    RateSnapshotLimits limits;
    limits.owned_bytes = 106;
    limits.items = 9;
    accepts([&] {
        EXPECT_EQ(make_effective_rate_snapshot(vectors::snapshot(), limits).data().snapshot_ref,
                  "S1");
    });
    accepts([&] {
        const auto result = quote_effective_rate(vectors::snapshot(), "EUR", "USD", limits);
        EXPECT_EQ(result.data().rate, Exact(5) / 6);
        // Fixed pair tag1 + Exact(5/6)2 + S1/EUR/USD strings2/3/3 =11.
        EXPECT_EQ(1U + 2U + result.data().provenance.value().snapshot_ref.size() +
                      result.data().provenance.value().from_currency.size() +
                      result.data().provenance.value().to_currency.size(),
                  11U);
    });
    limits.owned_bytes = 105;
    rejects(Error::resource_limit,
            [&] { return make_effective_rate_snapshot(vectors::snapshot(), limits); });
    limits.owned_bytes = 102;
    auto contradictory = vectors::snapshot();
    contradictory.components[0].members[2].denominator = "7";
    rejects(Error::contradictory_constraint,
            [&] { return make_effective_rate_snapshot(contradictory, limits); });
    limits.owned_bytes = 101;
    rejects(Error::resource_limit,
            [&] { return make_effective_rate_snapshot(contradictory, limits); });
    limits.owned_bytes = 106;
    limits.items = 8;
    rejects(Error::resource_limit,
            [&] { return make_effective_rate_snapshot(vectors::snapshot(), limits); });
}

TEST(RateLimitsAcceptance, EveryObservationFieldContributesItsActualOwnedBytes) {
    struct Row {
        const char *name;
        std::function<void(FetchedRateObservationInput &)> change;
        std::size_t bytes;
    };
    const Row rows[] = {
        {"observation", [](auto &s) { s.context.observation_ref += 'x'; }, 11},
        {"provider", [](auto &s) { s.context.provider_ref += 'x'; }, 11},
        {"configuration", [](auto &s) { s.context.configuration_ref += 'x'; }, 11},
        {"request", [](auto &s) { s.context.request_ref += 'x'; }, 11},
        {"evidence", [](auto &s) { s.context.source_evidence_ref += 'x'; }, 11},
        {"provider version", [](auto &s) { s.context.provider_snapshot_ref = "V"; }, 11},
        {"fetch int64", [](auto &s) { s.context.fetched_at_utc_ms = 0; }, 18},
        {"source empty raw", [](auto &s) { s.context.source_rate_time.original_text = ""; }, 10},
        {"source raw", [](auto &s) { s.context.source_rate_time.original_text = "T"; }, 11},
        {"source int64", [](auto &s) { s.context.source_rate_time.utc_ms = -1; }, 18},
        {"publication raw", [](auto &s) { s.context.publication_time.original_text = "T"; }, 11},
        {"publication int64", [](auto &s) { s.context.publication_time.utc_ms = 0; }, 18},
        {"from", [](auto &s) { s.quotes[0].from_currency += 'x'; }, 11},
        {"to", [](auto &s) { s.quotes[0].to_currency += 'x'; }, 11},
        {"rate spelling", [](auto &s) { s.quotes[0].original_decimal = "1.0"; }, 12},
    };
    for (const auto &row : rows) {
        SCOPED_TRACE(row.name);
        auto input = vectors::short_observation();
        row.change(input);
        RateSnapshotLimits limits;
        limits.owned_bytes = row.bytes;
        accepts([&] { static_cast<void>(make_fetched_rate_observation(input, limits)); });
        --limits.owned_bytes;
        rejects(Error::resource_limit,
                [&] { return make_fetched_rate_observation(input, limits); });
    }
}

TEST(RateLimitsAcceptance, SnapshotAndManualFieldsAndVariantTagsHaveCompleteCharges) {
    struct Row {
        const char *name;
        std::function<void(EffectiveRateSnapshotInput &)> change;
        std::size_t bytes;
    };
    const Row rows[] = {
        {"snapshot", [](auto &s) { s.snapshot_ref += 'x'; }, 28},
        {"parent", [](auto &s) { s.parent_snapshot_ref = "P"; }, 28},
        {"construction int64", [](auto &s) { s.constructed_at_utc_ms = 0; }, 35},
        {"override and manual tag",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.constraints.push_back(ManualRateConstraint{"M"});
         },
         38},
        {"actor",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.overrides[0].actor_ref = "A";
             s.constraints.push_back(ManualRateConstraint{"M"});
         },
         39},
        {"recorded int64",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.overrides[0].recorded_at_utc_ms = 0;
             s.constraints.push_back(ManualRateConstraint{"M"});
         },
         46},
        {"reason",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.overrides[0].reason += 'x';
             s.constraints.push_back(ManualRateConstraint{"M"});
         },
         39},
        {"decision",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.overrides[0].decision_ref += 'x';
             s.constraints.push_back(ManualRateConstraint{"M"});
         },
         39},
        {"manual spelling",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.overrides[0].original_decimal = "1.0";
             s.constraints.push_back(ManualRateConstraint{"M"});
         },
         40},
        {"override plus actual reference",
         [](auto &s) {
             s.overrides.push_back(vectors::short_override());
             s.overrides[0].override_ref = "MM";
             s.constraints.push_back(ManualRateConstraint{"MM"});
         },
         40},
    };
    for (const auto &row : rows) {
        SCOPED_TRACE(row.name);
        auto input = vectors::short_snapshot();
        row.change(input);
        RateSnapshotLimits limits;
        limits.owned_bytes = row.bytes;
        accepts([&] { static_cast<void>(make_effective_rate_snapshot(input, limits)); });
        --limits.owned_bytes;
        rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
    }
    auto input = vectors::short_snapshot();
    input.overrides.push_back(vectors::short_override());
    input.constraints.push_back(ManualRateConstraint{"M"});
    RateSnapshotLimits limits;
    limits.items = 8;
    accepts([&] { static_cast<void>(make_effective_rate_snapshot(input, limits)); });
    limits.items = 7;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
}

TEST(RateLimitsAcceptance, FieldByteCeilingsAcceptBoundaryAndRefuseOneBeyond) {
    auto input = vectors::short_observation();
    input.quotes[0].from_currency = std::string(64, 'x');
    accepts([&] { static_cast<void>(make_fetched_rate_observation(input)); });
    input.quotes[0].from_currency += 'x';
    rejects(Error::resource_limit, [&] { return make_fetched_rate_observation(input); });
    input = vectors::short_observation();
    input.context.provider_ref = std::string(256, 'x');
    accepts([&] { static_cast<void>(make_fetched_rate_observation(input)); });
    input.context.provider_ref += 'x';
    rejects(Error::resource_limit, [&] { return make_fetched_rate_observation(input); });
    input = vectors::short_observation();
    input.context.publication_time.original_text = std::string(512, 'x');
    accepts([&] { static_cast<void>(make_fetched_rate_observation(input)); });
    input.context.publication_time.original_text->push_back('x');
    rejects(Error::resource_limit, [&] { return make_fetched_rate_observation(input); });
    auto snapshot = vectors::pinned_snapshot();
    snapshot.overrides[0].reason = std::string(4096, 'x');
    accepts([&] { static_cast<void>(make_effective_rate_snapshot(snapshot)); });
    snapshot.overrides[0].reason += 'x';
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(snapshot); });
}

TEST(RateLimitsAcceptance, SelectedCollectionCeilingsApplyBeforeAnyLexicalError) {
    RateSnapshotLimits limits;
    limits.quotes_per_observation = 1;
    auto source = vectors::observation();
    source.context.observation_ref.clear();
    rejects(Error::resource_limit, [&] { return make_fetched_rate_observation(source, limits); });
    limits = {};
    limits.total_quotes = 1;
    rejects(Error::resource_limit, [&] { return make_fetched_rate_observation(source, limits); });
    auto input = vectors::snapshot();
    input.snapshot_ref.clear();
    limits = {};
    limits.constraints = 1;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
    limits = {};
    limits.currencies = 2;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
    limits = {};
    limits.items = 8;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
    input = vectors::pinned_snapshot();
    input.snapshot_ref.clear();
    limits = {};
    limits.overrides = 1;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
    input = vectors::snapshot();
    input.sources.push_back(vectors::short_observation());
    input.snapshot_ref.clear();
    limits = {};
    limits.observations = 1;
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input, limits); });
}

TEST(RateLimitsAcceptance, LoweredCollectionBoundariesAcceptCompleteValues) {
    RateSnapshotLimits limits;
    limits.observations = 1;
    limits.quotes_per_observation = 2;
    limits.total_quotes = 2;
    limits.constraints = 2;
    limits.currencies = 3;
    limits.items = 9;
    accepts([&] { static_cast<void>(make_effective_rate_snapshot(vectors::snapshot(), limits)); });
    accepts(
        [&] { static_cast<void>(make_fetched_rate_observation(vectors::observation(), limits)); });
    auto pinned = vectors::pinned_snapshot();
    limits.overrides = 2;
    accepts([&] { static_cast<void>(make_effective_rate_snapshot(pinned, limits)); });
}

TEST(RatePriorityAcceptance, GlobalPreflightPrecedesEarlierReferencesAndDuplicateChecks) {
    auto input = vectors::snapshot();
    input.snapshot_ref.clear();
    input.sources[0].quotes[1].original_decimal = std::string(513, '?');
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.sources.push_back(input.sources[0]);
    input.sources[1].quotes[1].original_decimal = "NaN";
    rejects(Error::invalid_decimal, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.sources.push_back(input.sources[0]);
    input.components[0].members.clear();
    rejects(Error::duplicate_identity, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RatePriorityAcceptance, DeclaredInputAndCategoryOrderDetermineFirstSemanticError) {
    auto input = vectors::snapshot();
    input.sources[0].quotes[0].from_currency.clear();
    input.sources[0].quotes[1].original_decimal = "NaN";
    rejects(Error::invalid_currency, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.sources[0].quotes.push_back(input.sources[0].quotes[0]);
    input.constraints.push_back(input.constraints[0]);
    rejects(Error::duplicate_quote, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.components[0].members[0].numerator = "2";
    std::get<FetchedRateConstraint>(input.constraints[0]).quote_ordinal = 0;
    rejects(Error::noncanonical_component, [&] { return make_effective_rate_snapshot(input); });
    input = vectors::snapshot();
    input.components[0].members[2].denominator = "7";
    auto unused = vectors::short_observation();
    input.sources.push_back(unused);
    rejects(Error::contradictory_constraint, [&] { return make_effective_rate_snapshot(input); });
}

std::string numbered(const char *prefix, unsigned index) {
    const auto digits = std::to_string(index);
    return prefix + std::string(4 - digits.size(), '0') + digits;
}

TEST(RateLimitsAcceptance, DefaultObservationCountBoundaryIsSixteenThenRefusesSeventeen) {
    auto input = vectors::short_snapshot();
    input.sources.clear();
    input.constraints.clear();
    for (unsigned index = 0; index < 16; ++index) {
        auto source = vectors::short_observation();
        source.context.observation_ref = numbered("O", index);
        input.constraints.push_back(FetchedRateConstraint{source.context.observation_ref, 1});
        input.sources.push_back(std::move(source));
    }
    accepts([&] { EXPECT_EQ(make_effective_rate_snapshot(input).data().sources.size(), 16U); });
    input.sources.push_back(vectors::short_observation());
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateLimitsAcceptance, DefaultPerObservationQuoteBoundaryIs4096ThenRefuses4097) {
    auto input = vectors::short_observation();
    input.quotes.clear();
    for (unsigned index = 0; index < 4096; ++index)
        input.quotes.push_back({numbered("Q", index), "R", "1"});
    accepts([&] { EXPECT_EQ(make_fetched_rate_observation(input).data().quotes.size(), 4096U); });
    input.quotes.push_back({"last", "R", "1"});
    rejects(Error::resource_limit, [&] { return make_fetched_rate_observation(input); });
}

TEST(RateLimitsAcceptance, TotalQuoteCeilingCountsUnselectedRowsAcrossAllSources) {
    auto input = vectors::short_snapshot();
    input.sources.clear();
    input.constraints.clear();
    for (unsigned source_index = 0; source_index < 4; ++source_index) {
        auto source = vectors::short_observation();
        source.context.observation_ref = numbered("O", source_index);
        for (unsigned index = 1; index < 4096; ++index)
            source.quotes.push_back({numbered("Q", index), "R", "1"});
        input.constraints.push_back(FetchedRateConstraint{source.context.observation_ref, 1});
        input.sources.push_back(std::move(source));
    }
    accepts([&] {
        const auto result = make_effective_rate_snapshot(input);
        EXPECT_EQ(result.data().sources.size(), 4U);
        EXPECT_EQ(result.data().sources.at(3).data().quotes.size(), 4096U);
        EXPECT_EQ(result.data().components.at(0).members.size(), 2U);
    });
    auto extra = vectors::short_observation();
    extra.context.observation_ref = "extra";
    input.sources.push_back(std::move(extra)); // 16385 total; every individual count still fits.
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateLimitsAcceptance, DefaultOverrideBoundaryRetainsAll1024ConstraintsAndRejects1025) {
    auto input = vectors::short_snapshot();
    input.sources.clear();
    input.constraints.clear();
    for (unsigned index = 0; index < 1024; ++index) {
        auto manual = vectors::short_override();
        manual.override_ref = numbered("M", index);
        input.constraints.push_back(ManualRateConstraint{manual.override_ref});
        input.overrides.push_back(std::move(manual));
    }
    accepts([&] { EXPECT_EQ(make_effective_rate_snapshot(input).data().overrides.size(), 1024U); });
    input.overrides.push_back(vectors::short_override());
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateLimitsAcceptance, DefaultConstraintBoundaryAccepts4096DistinctConsistentEdges) {
    auto input = vectors::short_snapshot();
    input.constraints.clear();
    input.sources[0].quotes.clear();
    input.components[0].members.clear();
    for (unsigned index = 0; index < 65; ++index)
        input.components[0].members.push_back({numbered("C", index), "1", "1"});
    // All directed nonself edges from C0000..C0063 into the65 member set:4096.
    for (unsigned from = 0; from < 64; ++from) {
        for (unsigned to = 0; to < 65; ++to) {
            if (from == to)
                continue;
            input.sources[0].quotes.push_back({numbered("C", from), numbered("C", to), "1"});
            input.constraints.push_back(FetchedRateConstraint{
                "O", static_cast<std::uint64_t>(input.constraints.size() + 1)});
        }
    }
    accepts(
        [&] { EXPECT_EQ(make_effective_rate_snapshot(input).data().constraints.size(), 4096U); });
    input.constraints.push_back(FetchedRateConstraint{"O", 1});
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
}

TEST(RateLimitsAcceptance, DefaultMemberBoundaryCountsAcrossAllComponentsWithoutInventingEdges) {
    auto input = vectors::empty_snapshot();
    for (unsigned index = 0; index < 256; ++index)
        input.components.push_back({{{numbered("C", index), "1", "1"}}});
    accepts([&] { EXPECT_EQ(make_effective_rate_snapshot(input).data().components.size(), 256U); });
    input.components.push_back({{{"C0256", "1", "1"}}});
    rejects(Error::resource_limit, [&] { return make_effective_rate_snapshot(input); });
}

} // namespace
