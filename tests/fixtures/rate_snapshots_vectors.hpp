// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic literal inputs from RS01–17 and the approved API. No feature calls.
#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <string>
#include <subathon/core/rate_snapshot.hpp>

namespace subathon::test::rates {
using namespace subathon::core;

inline FetchedRateObservationInput observation() {
    return {{"O1",
             "provider-A",
             "config-1",
             "request-1",
             "evidence-1",
             std::nullopt,
             std::nullopt,
             {},
             {}},
            {{"USD", "EUR", "1.2000"}, {"USD", "GBP", "0.6000"}}};
}

// Complete S1 raw bytes102, parsed output bytes106, recursive items9.
inline EffectiveRateSnapshotInput snapshot() {
    return {"S1",
            std::nullopt,
            std::nullopt,
            {observation()},
            {},
            {FetchedRateConstraint{"O1", 1}, FetchedRateConstraint{"O1", 2}},
            {{{{"EUR", "1", "1"}, {"GBP", "1", "2"}, {"USD", "5", "6"}}}}};
}

inline EffectiveRateSnapshotInput empty_snapshot() {
    return {"S", std::nullopt, std::nullopt, {}, {}, {}, {}};
}

inline FetchedRateObservationInput short_observation() {
    return {{"O", "P", "C", "R", "E", std::nullopt, std::nullopt, {}, {}}, {{"A", "B", "1"}}};
}

// Raw bytes25, output bytes27, items6. No hidden original/output duplicates.
inline EffectiveRateSnapshotInput short_snapshot() {
    return {"S",
            std::nullopt,
            std::nullopt,
            {short_observation()},
            {},
            {FetchedRateConstraint{"O", 1}},
            {{{{"A", "1", "1"}, {"B", "1", "1"}}}}};
}

// Adding this evidence AND its constraint to short_snapshot gives output38/items8.
inline ManualRateOverrideInput short_override() {
    return {"M", RateOverrideKind::pinned, "A", "B", "1", "D", std::nullopt, "R", std::nullopt};
}

inline EffectiveRateSnapshotInput pinned_snapshot() {
    return {"pins",
            std::nullopt,
            std::nullopt,
            {},
            {{"P1", RateOverrideKind::pinned, "USD", "EUR", "1.25", "decision-1", std::nullopt,
              "Synthetic pin one", std::nullopt},
             {"P2", RateOverrideKind::temporary, "EUR", "GBP", "0.5", "decision-2", "actor-2",
              "Synthetic temporary two", 0}},
            {ManualRateConstraint{"P1"}, ManualRateConstraint{"P2"}},
            {{{{"EUR", "1", "1"}, {"GBP", "1", "2"}, {"USD", "4", "5"}}}}};
}

// These are declared powers defining the INPUT, not rates inferred by a solver.
// A=1; B..F=3^{536,1072,1608,2144,2584}; G..K=1/5^{352,704,1056,1408,1764}.
// Each source quote is a finite <=256-digit integer; F and K have4096-bit components.
// Literal K->F is 3^2584 * 5^1764 (8192 bits), beyond the separate finalizer budget.
inline EffectiveRateSnapshotInput large_snapshot() {
    using boost::multiprecision::cpp_int;
    const auto power_text = [](unsigned base, unsigned exponent) {
        const cpp_int value = boost::multiprecision::pow(cpp_int(base), exponent);
        return value.str();
    };
    auto input = empty_snapshot();
    auto source = short_observation();
    source.quotes = {{"A", "B", power_text(3, 536)}, {"B", "C", power_text(3, 536)},
                     {"C", "D", power_text(3, 536)}, {"D", "E", power_text(3, 536)},
                     {"E", "F", power_text(3, 440)}, {"G", "A", power_text(5, 352)},
                     {"H", "G", power_text(5, 352)}, {"I", "H", power_text(5, 352)},
                     {"J", "I", power_text(5, 352)}, {"K", "J", power_text(5, 356)}};
    input.sources.push_back(std::move(source));
    input.constraints = {FetchedRateConstraint{"O", 1}, FetchedRateConstraint{"O", 2},
                         FetchedRateConstraint{"O", 3}, FetchedRateConstraint{"O", 4},
                         FetchedRateConstraint{"O", 5}, FetchedRateConstraint{"O", 6},
                         FetchedRateConstraint{"O", 7}, FetchedRateConstraint{"O", 8},
                         FetchedRateConstraint{"O", 9}, FetchedRateConstraint{"O", 10}};
    input.components = {{{{"A", "1", "1"},
                          {"B", power_text(3, 536), "1"},
                          {"C", power_text(3, 1072), "1"},
                          {"D", power_text(3, 1608), "1"},
                          {"E", power_text(3, 2144), "1"},
                          {"F", power_text(3, 2584), "1"},
                          {"G", "1", power_text(5, 352)},
                          {"H", "1", power_text(5, 704)},
                          {"I", "1", power_text(5, 1056)},
                          {"J", "1", power_text(5, 1408)},
                          {"K", "1", power_text(5, 1764)}}}};
    return input;
}
} // namespace subathon::test::rates
