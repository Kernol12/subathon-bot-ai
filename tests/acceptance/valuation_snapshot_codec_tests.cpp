// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/valuation_snapshot_codec_vectors.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <subathon/core/valuation_snapshot_codec.hpp>
#include <utility>

namespace {
using namespace subathon::core;
using Error = ExactValuationError;
namespace wire = subathon::test::valuation_snapshot;

EffectiveRateSnapshotInput empty_input() { return {"S", {}, {}, {}, {}, {}, {}}; }

EffectiveRateSnapshotInput populated_input() {
    return {std::string("S\0\xff", 3),
            "P",
            -1,
            {{{"o",
               std::string("p\0\x80", 3),
               "c",
               "r",
               "e",
               "v",
               0,
               {std::string("t\0", 2), {}},
               {std::string{}, -2}},
              {{"EUR", "USD", "9.99"}, {"EUR", "GBP", "9.0e-1"}}}},
            {{"m", RateOverrideKind::pinned, "EUR", "USD", "1.2000", "d", "a",
              std::string("R\0\xff", 3), -3}},
            {FetchedRateConstraint{"o", 2}, ManualRateConstraint{"m"}},
            {{{{"EUR", "1", "1"}, {"GBP", "9", "10"}, {"USD", "6", "5"}}}}};
}

void same_encoding(const EffectiveRateSnapshot &value, std::string_view document) {
    const auto encoded = encode_valuation_snapshot(value);
    EXPECT_EQ(encoded.format_ref, wire::format);
    EXPECT_EQ(encoded.document, document);
}

template <typename Call>
void refuses(Error expected, Call call,
             std::source_location where = std::source_location::current()) {
    SCOPED_TRACE(testing::Message() << where.file_name() << ':' << where.line());
    // The Stage 1 sentinel is a different exception and leaves later rows reachable.
    EXPECT_THROW(
        {
            try {
                static_cast<void>(call());
            } catch (const ExactValuationFailure &failure) {
                EXPECT_EQ(failure.code(), expected);
                EXPECT_EQ(std::string_view(failure.what()).find("rsv1-private-fixture-marker"),
                          std::string_view::npos);
                throw;
            }
        },
        ExactValuationFailure);
}

void refuses_document(std::string_view document, Error expected,
                      std::source_location where = std::source_location::current()) {
    refuses(expected, [&] { return decode_valuation_snapshot(wire::format, document); }, where);
}

void check_time(const RateTimeEvidence &actual, const RateTimeEvidence &expected) {
    EXPECT_EQ(actual.original_text, expected.original_text);
    EXPECT_EQ(actual.utc_ms, expected.utc_ms);
}

void check_populated(const EffectiveRateSnapshotData &data,
                     const EffectiveRateSnapshotInput &expected) {
    EXPECT_EQ(data.snapshot_ref, expected.snapshot_ref);
    EXPECT_EQ(data.parent_snapshot_ref, expected.parent_snapshot_ref);
    EXPECT_EQ(data.constructed_at_utc_ms, expected.constructed_at_utc_ms);
    ASSERT_EQ(data.sources.size(), 1U);
    const auto &source = data.sources.front().data();
    const auto &context = expected.sources.front().context;
    EXPECT_EQ(source.context.observation_ref, context.observation_ref);
    EXPECT_EQ(source.context.provider_ref, context.provider_ref);
    EXPECT_EQ(source.context.configuration_ref, context.configuration_ref);
    EXPECT_EQ(source.context.request_ref, context.request_ref);
    EXPECT_EQ(source.context.source_evidence_ref, context.source_evidence_ref);
    EXPECT_EQ(source.context.provider_snapshot_ref, context.provider_snapshot_ref);
    EXPECT_EQ(source.context.fetched_at_utc_ms, context.fetched_at_utc_ms);
    check_time(source.context.source_rate_time, context.source_rate_time);
    check_time(source.context.publication_time, context.publication_time);
    ASSERT_EQ(source.quotes.size(), 2U);
    EXPECT_EQ(source.quotes[0].from_currency, "EUR");
    EXPECT_EQ(source.quotes[0].to_currency, "USD");
    EXPECT_EQ(source.quotes[0].rate.original(), "9.99");
    EXPECT_EQ(source.quotes[0].rate.value(), Exact(999) / 100);
    EXPECT_EQ(source.quotes[1].from_currency, "EUR");
    EXPECT_EQ(source.quotes[1].to_currency, "GBP");
    EXPECT_EQ(source.quotes[1].rate.original(), "9.0e-1");
    EXPECT_EQ(source.quotes[1].rate.value(), Exact(9) / 10);

    ASSERT_EQ(data.overrides.size(), 1U);
    const auto &replacement = data.overrides.front();
    EXPECT_EQ(replacement.override_ref, "m");
    EXPECT_EQ(replacement.kind, RateOverrideKind::pinned);
    EXPECT_EQ(replacement.from_currency, "EUR");
    EXPECT_EQ(replacement.to_currency, "USD");
    EXPECT_EQ(replacement.rate.original(), "1.2000");
    EXPECT_EQ(replacement.rate.value(), Exact(6) / 5);
    EXPECT_EQ(replacement.decision_ref, "d");
    EXPECT_EQ(replacement.actor_ref, std::optional<std::string>("a"));
    EXPECT_EQ(replacement.reason, expected.overrides.front().reason);
    EXPECT_EQ(replacement.recorded_at_utc_ms, expected.overrides.front().recorded_at_utc_ms);
    ASSERT_EQ(data.constraints.size(), 2U);
    const auto *fetched = std::get_if<FetchedRateConstraint>(&data.constraints[0]);
    const auto *manual = std::get_if<ManualRateConstraint>(&data.constraints[1]);
    ASSERT_NE(fetched, nullptr);
    ASSERT_NE(manual, nullptr);
    EXPECT_EQ(fetched->observation_ref, "o");
    EXPECT_EQ(fetched->quote_ordinal, 2U);
    EXPECT_EQ(manual->override_ref, "m");

    ASSERT_EQ(data.components.size(), 1U);
    const auto &members = data.components.front().members;
    ASSERT_EQ(members.size(), 3U);
    EXPECT_EQ(members[0].currency, "EUR");
    EXPECT_EQ(members[0].weight, Exact(1));
    EXPECT_EQ(members[1].currency, "GBP");
    EXPECT_EQ(members[1].weight, Exact(9) / 10);
    EXPECT_EQ(members[2].currency, "USD");
    EXPECT_EQ(members[2].weight, Exact(6) / 5);
}

// The shipped query accepts a DTO. Reconstruct its inputs from decoded data so
// the exact pair and provenance assertions consume the decoder's result.
EffectiveRateSnapshotInput query_input(const EffectiveRateSnapshotData &data) {
    EffectiveRateSnapshotInput input{data.snapshot_ref,
                                     data.parent_snapshot_ref,
                                     data.constructed_at_utc_ms,
                                     {},
                                     {},
                                     data.constraints,
                                     {}};
    for (const auto &entry : data.sources) {
        FetchedRateObservationInput source{entry.data().context, {}};
        for (const auto &quote : entry.data().quotes)
            source.quotes.push_back(
                {quote.from_currency, quote.to_currency, quote.rate.original()});
        input.sources.push_back(std::move(source));
    }
    for (const auto &entry : data.overrides) {
        input.overrides.push_back({entry.override_ref, entry.kind, entry.from_currency,
                                   entry.to_currency, entry.rate.original(), entry.decision_ref,
                                   entry.actor_ref, entry.reason, entry.recorded_at_utc_ms});
    }
    for (const auto &entry : data.components) {
        RateComponentInput component;
        for (const auto &member : entry.members) {
            component.members.push_back({member.currency,
                                         boost::multiprecision::numerator(member.weight).str(),
                                         boost::multiprecision::denominator(member.weight).str()});
        }
        input.components.push_back(std::move(component));
    }
    return input;
}

void check_pairs(const EffectiveRateSnapshotInput &input) {
    const std::array<std::pair<std::string_view, Exact>, 2> pairs{{
        {"EUR", Exact(5) / 6},
        {"GBP", Exact(3) / 4},
    }};
    for (const auto &[to, expected] : pairs) {
        SCOPED_TRACE(to);
        const auto pair = quote_effective_rate(input, "USD", to);
        EXPECT_EQ(pair.data().kind, RatePairKind::available);
        ASSERT_TRUE(pair.data().rate);
        EXPECT_EQ(*pair.data().rate, expected);
        ASSERT_TRUE(pair.data().provenance);
        EXPECT_EQ(pair.data().provenance->snapshot_ref, std::string("S\0\xff", 3));
        EXPECT_EQ(pair.data().provenance->from_currency, "USD");
        EXPECT_EQ(pair.data().provenance->to_currency, to);
    }
}

TEST(ValuationSnapshotCodec, FrozenFormatIdentifier) {
    EXPECT_EQ(valuation_snapshot_format_ref, wire::format);
}

TEST(ValuationSnapshotCodec, EncodesEmptyLiteral) {
    const auto value = make_effective_rate_snapshot(empty_input());
    EXPECT_NO_THROW(same_encoding(value, wire::empty_document));
}

TEST(ValuationSnapshotCodec, DecodesEmptyLiteral) {
    EXPECT_NO_THROW({
        const auto value = decode_valuation_snapshot(wire::format, wire::empty_document);
        const auto &data = value.data();
        EXPECT_EQ(data.snapshot_ref, "S");
        EXPECT_FALSE(data.parent_snapshot_ref);
        EXPECT_FALSE(data.constructed_at_utc_ms);
        EXPECT_TRUE(data.sources.empty());
        EXPECT_TRUE(data.overrides.empty());
        EXPECT_TRUE(data.constraints.empty());
        EXPECT_TRUE(data.components.empty());
        same_encoding(value, wire::empty_document);
    });
}

TEST(ValuationSnapshotCodec, EncodesPopulatedLiteral) {
    const auto value = make_effective_rate_snapshot(populated_input());
    EXPECT_NO_THROW(same_encoding(value, wire::populated_document));
}

TEST(ValuationSnapshotCodec, DecodesPopulatedLiteralAndOwnsOpaqueEvidence) {
    std::string format(wire::format);
    std::string bytes(wire::populated_document);
    EXPECT_NO_THROW({
        const auto value = decode_valuation_snapshot(format, bytes);
        std::ranges::fill(format, 'x');
        std::ranges::fill(bytes, 'x');
        check_populated(value.data(), populated_input());
        same_encoding(value, wire::populated_document);
    });
}

TEST(ValuationSnapshotCodec, DecodedSnapshotRetainsExactInverseAndCrossRates) {
    EXPECT_NO_THROW({
        const auto value = decode_valuation_snapshot(wire::format, wire::populated_document);
        check_pairs(query_input(value.data()));
    });
}

TEST(ValuationSnapshotCodec, ShippedLiteralModelSetupControl) {
    const auto input = populated_input();
    const auto value = make_effective_rate_snapshot(input);
    check_populated(value.data(), input);
    check_pairs(input);
}

TEST(ValuationSnapshotCodec, TimePresenceAndSignedInstantsAreIndependent) {
    struct Row {
        const char *name;
        std::size_t offset;
        std::size_t removed;
        std::string replacement;
        std::size_t expected_size;
        void (*edit)(EffectiveRateSnapshotInput &);
    };
    const Row rows[] = {
        {"fetch absent", 63, 9, wire::byte(0), 281,
         [](auto &i) { i.sources[0].context.fetched_at_utc_ms.reset(); }},
        {"source text absent, UTC zero", 72, 8, wire::byte(0) + wire::byte(1) + wire::u64(0), 291,
         [](auto &i) { i.sources[0].context.source_rate_time = {{}, 0}; }},
        {"publication text absent", 80, 5, wire::byte(0), 285,
         [](auto &i) { i.sources[0].context.publication_time.original_text.reset(); }},
        {"override time absent", 196, 9, wire::byte(0), 281,
         [](auto &i) { i.overrides[0].recorded_at_utc_ms.reset(); }},
        {"constructed UTC minimum", 18, 8, wire::u64(0x8000000000000000ULL), 289,
         [](auto &i) { i.constructed_at_utc_ms = std::numeric_limits<std::int64_t>::min(); }},
        {"constructed UTC maximum", 18, 8, wire::u64(0x7fffffffffffffffULL), 289,
         [](auto &i) { i.constructed_at_utc_ms = std::numeric_limits<std::int64_t>::max(); }},
    };
    for (const auto &row : rows) {
        SCOPED_TRACE(row.name);
        auto input = populated_input();
        row.edit(input);
        auto document = std::string(wire::populated_document);
        document.replace(row.offset, row.removed, row.replacement);
        ASSERT_EQ(document.size(), row.expected_size);
        const auto model = make_effective_rate_snapshot(input);
        EXPECT_NO_THROW(same_encoding(model, document));
        EXPECT_NO_THROW({
            const auto decoded = decode_valuation_snapshot(wire::format, document);
            check_populated(decoded.data(), input);
            same_encoding(decoded, document);
        });
    }
}

struct Fixture {
    EffectiveRateSnapshotInput input = empty_input();
    std::string document = wire::base_prefix();
};

Fixture framed_fixture(std::size_t request_bytes) {
    Fixture fixture;
    const std::string request(request_bytes, 'r');
    FetchedRateObservationInput source{{"o", "p", "c", request, "e", {}, {}, {}, {}},
                                       {{"A", "B", "1"}}};
    fixture.document += wire::u32(1) + wire::context("o", request) + wire::u32(2177);
    fixture.document += wire::quote("A", "B", "1");
    for (unsigned key = 0; source.quotes.size() < 2177; ++key) {
        const auto from = wire::byte(static_cast<unsigned char>(key >> 8));
        const auto to = wire::byte(static_cast<unsigned char>(key & 0xff));
        if (from == "A" && to == "B")
            continue;
        source.quotes.push_back({from, to, "1"});
        fixture.document += wire::quote(from, to, "1");
    }
    fixture.input.sources.push_back(std::move(source));
    fixture.input.constraints = {FetchedRateConstraint{"o", 1}};
    fixture.input.components = {{{{"A", "1", "1"}, {"B", "1", "1"}}}};
    fixture.document +=
        wire::u32(0) + wire::u32(1) + wire::fetched("o", 1) + wire::u32(1) + wire::unit_component();
    return fixture;
}

// This is a deliberately wide fixture: 2176 unselected pairs, including self
// quotes and NUL/high-bit currency identifiers, outside the two effective members.
TEST(ValuationSnapshotCodec, FramedCeilingIsIndependentOfLogicalBytes) {
    auto fitting = framed_fixture(4);
    ASSERT_EQ(fitting.document.size(), 32768U);
    const auto model = make_effective_rate_snapshot(fitting.input);
    EXPECT_NO_THROW(same_encoding(model, fitting.document));
    EXPECT_NO_THROW({
        const auto decoded = decode_valuation_snapshot(wire::format, fitting.document);
        ASSERT_EQ(decoded.data().sources.size(), 1U);
        const auto &quotes = decoded.data().sources.front().data().quotes;
        ASSERT_EQ(quotes.size(), 2177U);
        EXPECT_EQ(quotes[0].from_currency, "A");
        EXPECT_EQ(quotes[0].to_currency, "B");
        EXPECT_EQ(quotes[1].from_currency, wire::byte(0));
        EXPECT_EQ(quotes[1].to_currency, wire::byte(0));
        EXPECT_EQ(quotes[256].to_currency, wire::byte(255));
        EXPECT_EQ(quotes.back().from_currency, wire::byte(8));
        EXPECT_EQ(quotes.back().to_currency, wire::byte(127));
        ASSERT_EQ(decoded.data().components.size(), 1U);
        EXPECT_EQ(decoded.data().components.front().members.size(), 2U);
        same_encoding(decoded, fitting.document);
    });

    auto oversized = framed_fixture(5);
    ASSERT_EQ(oversized.document.size(), 32769U);
    const auto oversized_model = make_effective_rate_snapshot(oversized.input);
    refuses(Error::resource_limit, [&] { return encode_valuation_snapshot(oversized_model); });
    refuses_document(oversized.document, Error::resource_limit);
}

// Raw B = 55 + 5*4096 + L. Parsed B adds 6 pairs of one-digit integers.
// Framing adds 234 bytes, independently of L.
Fixture reason_fixture(std::size_t last_reason_size, std::string_view first_rate = "1") {
    Fixture fixture;
    fixture.document += wire::u32(0) + wire::u32(6);
    for (char key : std::string_view("abcdef")) {
        const std::string ref(1, key);
        const std::string reason(key == 'f' ? last_reason_size : 4096, 'x');
        const auto spelling = key == 'a' ? first_rate : "1";
        fixture.input.overrides.push_back({ref,
                                           RateOverrideKind::temporary,
                                           "A",
                                           "B",
                                           std::string(spelling),
                                           "d",
                                           {},
                                           reason,
                                           {}});
        fixture.input.constraints.push_back(ManualRateConstraint{ref});
        fixture.document += wire::temporary_override(ref, spelling, reason);
    }
    fixture.document += wire::u32(6);
    for (const auto &constraint : fixture.input.constraints)
        fixture.document += wire::manual(std::get<ManualRateConstraint>(constraint).override_ref);
    fixture.input.components = {{{{"A", "1", "1"}, {"B", "1", "1"}}}};
    fixture.document += wire::u32(1) + wire::unit_component();
    return fixture;
}

TEST(ValuationSnapshotCodec, ParsedCeilingAllowsEqualityAndRefusesOneOver) {
    const auto fitting = reason_fixture(4029);
    ASSERT_EQ(fitting.document.size(), 24798U);
    const auto model = make_effective_rate_snapshot(fitting.input);
    EXPECT_NO_THROW(same_encoding(model, fitting.document));
    EXPECT_NO_THROW({
        const auto decoded = decode_valuation_snapshot(wire::format, fitting.document);
        ASSERT_EQ(decoded.data().overrides.size(), 6U);
        EXPECT_EQ(decoded.data().overrides.back().reason, std::string(4029, 'x'));
        EXPECT_EQ(decoded.data().overrides.back().rate.value(), Exact(1));
        same_encoding(decoded, fitting.document);
    });

    const auto oversized = reason_fixture(4030);
    ASSERT_EQ(oversized.document.size(), 24799U);
    const auto oversized_model = make_effective_rate_snapshot(oversized.input);
    refuses(Error::resource_limit, [&] { return encode_valuation_snapshot(oversized_model); });
    refuses_document(oversized.document, Error::resource_limit);
}

TEST(ValuationSnapshotCodec, RawCeilingStopsBeforeSemanticsOnlyWhenExceeded) {
    const auto exact_raw = reason_fixture(4041);
    ASSERT_EQ(exact_raw.document.size(), 24810U);
    refuses_document(exact_raw.document, Error::resource_limit); // Parsed B is 24588.
    refuses_document(reason_fixture(4041, "0").document, Error::invalid_rate);
    const auto raw_over = reason_fixture(4042);
    ASSERT_EQ(raw_over.document.size(), 24811U);
    refuses_document(reason_fixture(4042, "0").document, Error::resource_limit);
    const auto model = make_effective_rate_snapshot(raw_over.input);
    refuses(Error::resource_limit, [&] { return encode_valuation_snapshot(model); });
}

std::string overrides_through_last_reason(std::size_t last_reason_size) {
    auto prefix = wire::base_prefix() + wire::u32(0) + wire::u32(6);
    for (char key : std::string_view("abcdef")) {
        const std::string reason(key == 'f' ? last_reason_size : 4096, 'x');
        prefix += wire::temporary_override(std::string(1, key), "1", reason);
    }
    return prefix;
}

TEST(ValuationSnapshotCodec, FixedWidthChargesOccurAtCompletedTokens) {
    // At the last optional instant, prior B = 1+6*6+5*4096+L.
    auto instant = overrides_through_last_reason(4052);
    instant.back() = '\x01';
    refuses_document(instant + std::string(7, '\0'), Error::malformed_document);
    refuses_document(instant + wire::u64(0), Error::resource_limit);
    auto fits = overrides_through_last_reason(4051);
    fits.back() = '\x01';
    refuses_document(fits + wire::u64(0), Error::malformed_document); // Missing next count.

    // At B=24576 the manual variant cannot charge another byte; an unknown
    // variant is a framing fault before that charge.
    const auto constraint = overrides_through_last_reason(4059) + wire::u32(1);
    refuses_document(constraint + wire::byte(2), Error::malformed_document);
    refuses_document(constraint + wire::byte(1), Error::resource_limit);
}

Fixture potential_fixture(bool denominator, bool one_bit_over) {
    Fixture fixture;
    FetchedRateObservationInput source{{"o", "p", "c", "r", "e", {}, {}, {}, {}}, {}};
    RateComponentInput component;
    fixture.document += wire::u32(1) + wire::context("o") + wire::u32(5);
    constexpr std::string_view keys = "ABCDEF";
    for (std::size_t i = 1; i < keys.size(); ++i) {
        const auto from = keys.substr(i - 1, 1);
        const auto to = keys.substr(i, 1);
        std::string_view spelling = denominator ? "1e-256" : "1e256";
        if (i == 5)
            spelling = denominator ? (one_bit_over ? "5e-210" : "1e-209")
                                   : (one_bit_over ? "2e209" : "1e209");
        source.quotes.push_back({std::string(from), std::string(to), std::string(spelling)});
        fixture.document += wire::quote(from, to, spelling);
        fixture.input.constraints.push_back(FetchedRateConstraint{"o", i});
    }
    fixture.input.sources.push_back(std::move(source));
    fixture.document += wire::u32(0) + wire::u32(5);
    for (std::uint64_t i = 1; i <= 5; ++i)
        fixture.document += wire::fetched("o", i);
    fixture.document += wire::u32(1) + wire::u32(6);
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto currency = keys.substr(i, 1);
        const std::string power =
            (i == 5 && one_bit_over ? "2" : "1") + std::string(i == 5 ? 1233 : 256 * i, '0');
        const auto n = denominator ? "1" : power;
        const auto d = denominator ? power : "1";
        fixture.document += wire::member(currency, n, d);
        component.members.push_back({std::string(currency), n, d});
    }
    fixture.input.components.push_back(std::move(component));
    return fixture;
}

TEST(ValuationSnapshotCodec, FourThousandNinetySixBitPotentialComponentsFit) {
    for (bool denominator : {false, true}) {
        SCOPED_TRACE(denominator ? "denominator" : "numerator");
        const auto fitting = potential_fixture(denominator, false);
        const auto model = make_effective_rate_snapshot(fitting.input);
        EXPECT_NO_THROW(same_encoding(model, fitting.document));
        EXPECT_NO_THROW({
            const auto decoded = decode_valuation_snapshot(wire::format, fitting.document);
            ASSERT_EQ(decoded.data().components.size(), 1U);
            const auto &members = decoded.data().components.front().members;
            ASSERT_EQ(members.size(), 6U);
            const auto &weight = members.back().weight;
            const auto &large = denominator ? boost::multiprecision::denominator(weight)
                                            : boost::multiprecision::numerator(weight);
            EXPECT_EQ(large.str(), "1" + std::string(1233, '0'));
            EXPECT_EQ(boost::multiprecision::msb(large), 4095U);
            same_encoding(decoded, fitting.document);
        });
        const auto one_over = potential_fixture(denominator, true);
        ASSERT_EQ(one_over.document.size(), fitting.document.size());
        refuses_document(one_over.document, Error::resource_limit);
    }
}

TEST(ValuationSnapshotCodec, DispatchChecksBothOuterBoundsBeforeInspectingUnknownBody) {
    const auto unknown = std::string(256, 'u');
    const std::string internal_over = "RSV1" + wire::u32(257);
    for (const auto body : {std::string_view{}, std::string_view("wrong magic"),
                            std::string_view(internal_over), wire::empty_document}) {
        SCOPED_TRACE(body.size());
        refuses(Error::unsupported_format,
                [&] { return decode_valuation_snapshot(unknown, body); });
    }
    const auto full_body = std::string(32768, 'x');
    const auto over_body = full_body + "x";
    refuses(Error::unsupported_format,
            [&] { return decode_valuation_snapshot(unknown, full_body); });
    refuses(Error::resource_limit, [&] { return decode_valuation_snapshot(unknown + "u", ""); });
    refuses(Error::resource_limit, [&] { return decode_valuation_snapshot(unknown, over_body); });
    refuses_document(over_body, Error::resource_limit);
    const auto suffixed = std::string(wire::format) + '\0';
    refuses(Error::unsupported_format,
            [&] { return decode_valuation_snapshot(suffixed, wire::empty_document); });
}

std::string replaced(std::size_t offset, std::size_t length, std::string_view replacement) {
    return std::string(wire::populated_document).replace(offset, length, replacement);
}

TEST(ValuationSnapshotCodec, FramingScansTheWholeDocumentBeforeLexicalSemantics) {
    const auto empty_ref = "RSV1" + wire::field("") + std::string(18, '\0');
    refuses_document(empty_ref, Error::invalid_reference);
    refuses_document(empty_ref + "rsv1-private-fixture-marker", Error::malformed_document);
    refuses_document(empty_ref.substr(0, empty_ref.size() - 1), Error::malformed_document);
    refuses_document("RSV2" + wire::u32(257), Error::malformed_document);
    refuses_document("RSV1" + wire::u32(257), Error::resource_limit);
    refuses_document("RSV1" + wire::u32(257).substr(0, 3), Error::malformed_document);

    // The earlier optional tag wins over an observation overcount.
    const auto prefix = "RSV1" + wire::field("S");
    refuses_document(prefix + wire::byte(2) + wire::byte(0) + wire::u32(17),
                     Error::malformed_document);
    // Conversely, framing still sees a late member length even after an
    // earlier currency is semantically invalid.
    auto lexical_fault = replaced(98, 7, wire::field(""));
    lexical_fault.resize(lexical_fault.size() - 5);
    lexical_fault += wire::u32(1235);
    refuses_document(lexical_fault, Error::resource_limit);
}

TEST(ValuationSnapshotCodec, TruncatedTokensAndUnknownTagsAreMalformed) {
    for (std::size_t length : {3U, 7U, 10U, 25U, 28U, 71U, 78U, 92U, 217U, 288U}) {
        SCOPED_TRACE(length);
        refuses_document(wire::populated_document.substr(0, length), Error::malformed_document);
    }
    // Parent, construction, provider, fetch, source time (both), publication
    // (both), override kind, actor, recorded, and constraint variant.
    for (std::size_t offset : {11U, 17U, 57U, 63U, 72U, 79U, 80U, 85U, 153U, 183U, 196U, 209U}) {
        SCOPED_TRACE(offset);
        refuses_document(replaced(offset, 1, wire::byte(2)), Error::malformed_document);
    }
}

TEST(ValuationSnapshotCodec, CompleteDeclaredCeilingsAreCheckedBeforeMissingBodies) {
    const auto base = wire::base_prefix();
    const auto source = base + wire::u32(1) + wire::context("o");
    const auto override = base + wire::u32(0) + wire::u32(1);
    const auto components = base + std::string(12, '\0') + wire::u32(2);
    struct Row {
        const char *name;
        std::string prefix;
        std::uint32_t bound;
    };
    const Row rows[] = {
        {"observations", base, 16},
        {"quotes in one observation", source, 4096},
        {"overrides", base + wire::u32(0), 1024},
        {"constraints", base + std::string(8, '\0'), 4096},
        {"recursive items, one observation already counted", source + std::string(12, '\0'), 32767},
        {"cumulative members", components + wire::u32(1) + wire::member("A"), 255},
        {"snapshot ref", "RSV1", 256},
        {"currency", source + wire::u32(1), 64},
        {"source time text", base + wire::u32(1) + wire::context("o").substr(0, 27) + wire::byte(1),
         512},
        {"decimal", source + wire::u32(1) + wire::field("A") + wire::field("B"), 512},
        {"reason",
         override + wire::field("m") + wire::byte(0) + wire::quote("A", "B", "1") +
             wire::field("d") + wire::byte(0),
         4096},
        {"numerator", components + wire::u32(1) + wire::field("A"), 1234},
        {"denominator", components + wire::u32(1) + wire::field("A") + wire::field("1"), 1234},
    };
    for (const auto &row : rows) {
        SCOPED_TRACE(row.name);
        refuses_document(row.prefix + wire::u32(row.bound), Error::malformed_document);
        refuses_document(row.prefix + wire::u32(row.bound + 1), Error::resource_limit);
    }
}

TEST(ValuationSnapshotCodec, LexicalReasonsFollowDeclarationOrder) {
    refuses_document(replaced(184, 5, wire::field("")), Error::invalid_reference);
    refuses_document(replaced(58, 5, wire::field("")), Error::invalid_reference);
    refuses_document(replaced(189, 7, wire::field("")), Error::invalid_basis);
    refuses_document(replaced(158, 3, "USD"), Error::invalid_basis); // Manual self edge.
    auto mixed = replaced(134, 10, wire::field("not-a-rate"));
    mixed.replace(98, 7, wire::field(""));
    refuses_document(mixed, Error::invalid_basis);
    // Decimal grammar is checked completely before numeric mantissa/exponent refusal.
    const std::string digits(257, '9');
    refuses_document(replaced(112, 8, wire::field(digits + "x")), Error::invalid_rate);
    refuses_document(replaced(112, 8, wire::field(digits)), Error::resource_limit);
    refuses_document(replaced(112, 8, wire::field("1e257x")), Error::invalid_rate);
    refuses_document(replaced(112, 8, wire::field("1e257")), Error::resource_limit);
    refuses_document(replaced(112, 8, wire::field("-1")), Error::invalid_rate);
}

TEST(ValuationSnapshotCodec, PotentialSpellingAndReductionAreSemanticFaults) {
    for (const auto &[n, d] : std::array{std::pair{"05", "6"}, std::pair{"0", "1"},
                                         std::pair{"10", "12"}, std::pair{"6", "0"}}) {
        SCOPED_TRACE(testing::Message() << n << '/' << d);
        refuses_document(replaced(279, 10, wire::field(n) + wire::field(d)), Error::invalid_rate);
    }
}

TEST(ValuationSnapshotCodec, LexicalBeforeStoredOrderAndStoredOrderBeforeGraph) {
    const auto reversed = wire::manual("m") + wire::fetched("o", 2);
    refuses_document(replaced(209, 20, reversed), Error::invalid_basis);
    auto bad_rate = replaced(209, 20, reversed);
    bad_rate.replace(134, 10, wire::field("?"));
    refuses_document(bad_rate, Error::invalid_rate);
    // If sorted silently this reaches the contradictory 9.99 edge.
    refuses_document(replaced(209, 20, wire::manual("m") + wire::fetched("o", 1)),
                     Error::invalid_basis);
    const auto swapped_members = std::string(wire::populated_document.substr(272, 17)) +
                                 std::string(wire::populated_document.substr(254, 18));
    refuses_document(replaced(254, 35, swapped_members), Error::invalid_basis);
}

TEST(ValuationSnapshotCodec, ResolvesEveryActiveReferenceBeforeCheckingExactEdges) {
    for (std::uint64_t ordinal :
         std::array<std::uint64_t, 3>{0, 3, std::numeric_limits<std::uint64_t>::max()}) {
        SCOPED_TRACE(ordinal);
        refuses_document(replaced(215, 8, wire::u64(ordinal)), Error::invalid_basis);
    }
    refuses_document(replaced(215, 8, wire::u64(1)), Error::inconsistent_rate);
    auto mixed = replaced(215, 8, wire::u64(1));
    mixed[228] = '?';
    refuses_document(mixed, Error::invalid_basis);
    // The contradictory active edge precedes retained-evidence coverage.
    refuses_document(replaced(205, 24, wire::u32(1) + wire::fetched("o", 1)),
                     Error::inconsistent_rate);
    refuses_document(replaced(205, 24, wire::u32(1) + wire::fetched("o", 2)),
                     Error::invalid_basis); // Retained override unused.
    refuses_document(replaced(205, 24, wire::u32(1) + wire::manual("m")),
                     Error::invalid_basis); // Retained source unused.
    refuses_document(wire::base_prefix() + std::string(12, '\0') + wire::u32(1) +
                         wire::unit_component(),
                     Error::invalid_basis); // No connecting edge.
}

struct OrderedSections {
    std::array<std::string, 2> sources;
    std::array<std::string, 2> overrides;
    std::array<std::string, 5> constraints;
    std::array<std::string, 2> components;

    std::string document() const {
        return wire::base_prefix() + wire::u32(2) + sources[0] + sources[1] + wire::u32(2) +
               overrides[0] + overrides[1] + wire::u32(5) + constraints[0] + constraints[1] +
               constraints[2] + constraints[3] + constraints[4] + wire::u32(2) + components[0] +
               components[1];
    }
};

OrderedSections ordered_sections() {
    const auto low = wire::byte(0x7f);
    const auto high = wire::byte(0x80);
    auto first = wire::context(low) + wire::u32(10) + wire::quote("C", "C", "1") +
                 wire::quote("B", "A", "1");
    for (char key : std::string_view("DEFGHIJ")) {
        const std::string currency(1, key);
        first += wire::quote(currency, currency, "1");
    }
    first += wire::quote("A", "B", "1");
    return {{first, wire::context(high) + wire::u32(1) + wire::quote("A", "B", "1")},
            {wire::temporary_override(low, "1", "x"), wire::temporary_override(high, "1", "x")},
            {wire::fetched(low, 2), wire::fetched(low, 10), wire::fetched(high, 1),
             wire::manual(low), wire::manual(high)},
            {wire::unit_component(), wire::u32(1) + wire::member("Z")}};
}

TEST(ValuationSnapshotCodec, CanonicalStoredOrderUsesUnsignedRefsAndNumericOrdinals) {
    const auto sections = ordered_sections();
    const auto document = sections.document();
    EXPECT_NO_THROW({
        const auto decoded = decode_valuation_snapshot(wire::format, document);
        ASSERT_EQ(decoded.data().sources.size(), 2U);
        EXPECT_EQ(decoded.data().sources[0].data().context.observation_ref, wire::byte(0x7f));
        EXPECT_EQ(decoded.data().sources[1].data().context.observation_ref, wire::byte(0x80));
        ASSERT_EQ(decoded.data().constraints.size(), 5U);
        const auto *first = std::get_if<FetchedRateConstraint>(&decoded.data().constraints[0]);
        const auto *second = std::get_if<FetchedRateConstraint>(&decoded.data().constraints[1]);
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
        EXPECT_EQ(first->quote_ordinal, 2U);
        EXPECT_EQ(second->quote_ordinal, 10U);
        same_encoding(decoded, document);
    });

    for (const auto &[name, edit] :
         std::array<std::pair<const char *, void (*)(OrderedSections &)>, 7>{{
             {"sources", [](auto &s) { std::swap(s.sources[0], s.sources[1]); }},
             {"overrides", [](auto &s) { std::swap(s.overrides[0], s.overrides[1]); }},
             {"fetched ordinals", [](auto &s) { std::swap(s.constraints[0], s.constraints[1]); }},
             {"fetched refs", [](auto &s) { std::swap(s.constraints[1], s.constraints[2]); }},
             {"manual refs", [](auto &s) { std::swap(s.constraints[3], s.constraints[4]); }},
             {"component anchors", [](auto &s) { std::swap(s.components[0], s.components[1]); }},
             {"duplicate constraint", [](auto &s) { s.constraints[1] = s.constraints[0]; }},
         }}) {
        SCOPED_TRACE(name);
        auto changed = sections;
        edit(changed);
        refuses_document(changed.document(), Error::invalid_basis);
    }
    auto duplicate_source = sections;
    duplicate_source.sources[1] = sections.sources[0];
    refuses_document(duplicate_source.document(), Error::invalid_basis);
    auto duplicate_quote = sections;
    // The second quote is selected and would become the same pair as ordinal 1.
    duplicate_quote.sources[0].replace(31 + 4 + 15, 15, wire::quote("C", "C", "1"));
    refuses_document(duplicate_quote.document(), Error::invalid_basis);
}

TEST(ValuationSnapshotCodec, MoreThan256UnselectedCurrencyKeysAreRetained) {
    const auto a = std::string("A\0", 2);
    const auto b = std::string("B\xff", 2);
    auto document = wire::base_prefix() + wire::u32(1) + wire::context("o") + wire::u32(258);
    document += wire::quote(a, b, "1");
    for (unsigned key = 0; key < 257; ++key) {
        const auto unselected = wire::big_endian(key, 2);
        document += wire::quote(a, unselected, "2");
    }
    document += wire::u32(0) + wire::u32(1) + wire::fetched("o", 1) + wire::u32(1) + wire::u32(2) +
                wire::member(a) + wire::member(b);
    EXPECT_NO_THROW({
        const auto decoded = decode_valuation_snapshot(wire::format, document);
        ASSERT_EQ(decoded.data().sources.size(), 1U);
        const auto &quotes = decoded.data().sources.front().data().quotes;
        ASSERT_EQ(quotes.size(), 258U);
        EXPECT_EQ(quotes.front().from_currency, a);
        EXPECT_EQ(quotes.front().to_currency, b);
        EXPECT_EQ(quotes[1].to_currency, std::string(2, '\0'));
        EXPECT_EQ(quotes.back().to_currency, std::string("\x01\0", 2));
        EXPECT_EQ(quotes.back().rate.value(), Exact(2));
        same_encoding(decoded, document);
    });
}

TEST(ValuationSnapshotCodec, Fitting4096ConstraintDocumentReachesUnresolvedBasis) {
    auto document = wire::base_prefix() + wire::u32(0) + wire::u32(0) + wire::u32(4096);
    for (std::uint64_t key = 0; key < 4096; ++key)
        document += wire::manual(wire::big_endian(key, 2));
    document += wire::u32(0);
    // 27 fixed bytes + 4096*(1 tag + 4 length + 2 ref). Raw B is 1+4096*3.
    // This asserts the observable refusal. The scratch ceiling on this path
    // requires independent Stage 2 source review.
    ASSERT_EQ(document.size(), 28699U);
    refuses_document(document, Error::invalid_basis);
}

TEST(ValuationSnapshotCodec, ShippedBoundaryModelSetupControl) {
    const auto empty = make_effective_rate_snapshot(empty_input());
    EXPECT_EQ(empty.data().snapshot_ref, "S");
    for (std::size_t length : {4U, 5U}) {
        const auto model = make_effective_rate_snapshot(framed_fixture(length).input);
        ASSERT_EQ(model.data().sources.size(), 1U);
        EXPECT_EQ(model.data().sources.front().data().quotes.size(), 2177U);
    }
    for (std::size_t length : {4029U, 4030U, 4042U}) {
        const auto model = make_effective_rate_snapshot(reason_fixture(length).input);
        ASSERT_EQ(model.data().overrides.size(), 6U);
        EXPECT_EQ(model.data().overrides.back().reason.size(), length);
    }
    for (bool denominator : {false, true}) {
        const auto model =
            make_effective_rate_snapshot(potential_fixture(denominator, false).input);
        ASSERT_EQ(model.data().components.size(), 1U);
        EXPECT_EQ(model.data().components.front().members.size(), 6U);
    }
}
} // namespace
