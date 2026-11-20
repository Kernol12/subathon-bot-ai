// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_rate_override_writer_fixture.hpp"

namespace rate_fixture {
// Fixture rendering, independent of the runtime codecs. Numeric output is ASCII.
inline std::string hex_token(char prefix, unsigned value, int digits = 2) {
    constexpr std::string_view hex = "0123456789ABCDEF";
    std::string token(1 + digits, prefix);
    for (int i = digits; i > 0; --i) {
        token[std::size_t(i)] = hex[value & 15];
        value >>= 4;
    }
    require(value == 0, "Fixture token fits its fixed width");
    return token;
}

inline core::EffectiveRateSnapshotInput encoded_pressure(unsigned quotes,
                                                         std::size_t evidence_bytes) {
    core::EffectiveRateSnapshotInput input{"S", "E", {}, {}, {}, {}, {core::RateComponentInput{}}};
    auto &members = input.components.front().members;
    for (const std::string_view base : {"CHF", "EUR", "GBP", "USD"}) {
        const auto ref = std::string(base.substr(0, 1));
        input.sources.push_back({{ref,
                                  "p",
                                  "c",
                                  "q",
                                  std::string(base == "CHF" ? evidence_bytes : 1, 'e'),
                                  {},
                                  {},
                                  {},
                                  {}},
                                 {}});
        members.push_back({std::string(base), "1", "1"});
        auto &source = input.sources.back();
        for (unsigned i = 0; i < quotes; ++i) {
            source.quotes.push_back({std::string(base), hex_token('x', i), "1.20"});
            input.constraints.push_back(core::FetchedRateConstraint{ref, i + 1});
        }
    }
    for (unsigned i = 0; i < quotes; ++i)
        members.push_back({hex_token('x', i), "6", "5"});
    return input;
}

inline core::EffectiveRateSnapshotInput parsed_pressure(unsigned quotes, std::size_t time_bytes) {
    core::EffectiveRateSnapshotInput input{"S", "E", {}, {}, {}, {}, {core::RateComponentInput{}}};
    input.sources.push_back({{"F", "p", "c", "q", "e", {}, {}, {}, {}}, {}});
    auto &source = input.sources.front();
    if (time_bytes)
        source.context.source_rate_time.original_text = std::string(time_bytes, 't');
    auto &members = input.components.front().members;
    members.push_back({"USD", "1", "1"});
    for (unsigned i = 0; i < quotes; ++i) {
        const auto target = hex_token('x', i);
        source.quotes.push_back({"USD", target, "1.234567890123456789"});
        input.constraints.push_back(core::FetchedRateConstraint{"F", i + 1});
        members.push_back({target, "1234567890123456789", "1000000000000000000"});
    }
    return input;
}

// Exact standalone ROB1 fixture generated from the reviewed grammar. It has
// five one-byte refs, no timestamp and one present source-rate original text.
inline auto standalone_pressure(bool parsed_ceiling, bool over) {
    const auto count = parsed_ceiling ? 377u : 1722u;
    const auto text_length = (parsed_ceiling ? 66u : 7u) + unsigned(over);
    core::FetchedRateObservationInput input{
        {"J", "p", "c", "q", "e", {}, {}, {std::string(text_length, 't'), {}}, {}}, {}};
    auto document = QByteArray::fromHex(
        "524f4231 000000014a 0000000170 0000000163 0000000171 0000000165 00 00 01");
    document += QByteArray(3, '\0') + char(text_length) + QByteArray(text_length, 't');
    document += QByteArray::fromHex(parsed_ceiling ? "00000000000179" : "000000000006ba");
    for (unsigned i = 0; i < count; ++i) {
        auto from = hex_token('a', i / 64);
        auto to = hex_token('b', i % 64, parsed_ceiling ? 3 : 2);
        std::string decimal = parsed_ceiling ? "1.234567890123456789" : "2";
        document += QByteArray::fromHex("00000003") + blob(from);
        document += QByteArray(3, '\0') + char(to.size()) + blob(to);
        document += QByteArray(3, '\0') + char(decimal.size()) + blob(decimal);
        input.quotes.push_back({std::move(from), std::move(to), std::move(decimal)});
    }
    return std::pair{std::move(input), std::move(document)};
}
} // namespace rate_fixture
