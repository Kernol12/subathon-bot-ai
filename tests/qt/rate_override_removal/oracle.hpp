// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "boundary.hpp"

namespace removal_test {
enum class Start { pin_f_first, pin_w_first, temporary, equal_rates };

inline const core::FetchedRateObservationInput source_w{{"W", "p", "c", "q", "e", {}, {}, {}, {}},
                                                        {{"USD", "GBP", "0.70"}}};
inline constexpr auto pair_cad_jpy = "00000003434144000000034a5059";
inline constexpr auto pair_chf_sek = "000000034348460000000353454b";

inline RateAuthorityFacts permitted() {
    auto facts = authorized();
    facts.graph.use.accounts.front().individual_entries = {
        grant({pair_eur_usd, pair_eur_gbp, pair_cad_jpy, pair_chf_sek})};
    return facts;
}

inline RateOverrideEdit pin(std::string key, std::string base, std::string next,
                            std::string occurrence, std::string decision, std::string from,
                            std::string to, std::string decimal) {
    auto input = edit(std::move(key), std::move(base), std::move(next), std::move(occurrence),
                      std::move(decision));
    input.override_input.from_currency = std::move(from);
    input.override_input.to_currency = std::move(to);
    input.override_input.original_decimal = std::move(decimal);
    return input;
}

struct Example {
    RateSourceBasis basis;
    std::vector<RateOverrideEdit> introductions;
    core::EffectiveRateSnapshotInput before;
    core::EffectiveRateSnapshotInput after;
    std::vector<core::RatePairChange> changes;

    RateRemovalPreview expected_preview() const {
        return {{"I", "Z", "A", "M", before.snapshot_ref, "U", {"EUR", "USD"}, "O"},
                basis,
                {after, changes, "O"}};
    }
};

// The weights are literal normalized values from the contract examples. No
// solver, override preview, or actual receipt takes part in building this oracle.
inline Example example(Start start) {
    auto first = edit();
    if (start == Start::equal_rates) {
        const auto f = selected_f("1.20");
        return {{"B", {f}, {{0, 2}}},
                {first},
                {"S",
                 "E",
                 {},
                 {f},
                 {first.override_input},
                 {core::FetchedRateConstraint{"F", 2}, core::ManualRateConstraint{"O"}},
                 {{{{"EUR", "1", "1"}, {"USD", "6", "5"}}}}},
                {"U",
                 "S",
                 {},
                 {f},
                 {},
                 {core::FetchedRateConstraint{"F", 2}},
                 {{{{"EUR", "1", "1"}, {"USD", "6", "5"}}}}},
                {}};
    }
    if (start == Start::temporary) {
        first.override_input.kind = core::RateOverrideKind::temporary;
        first.override_input.from_currency = "USD";
        first.override_input.to_currency = "EUR";
        first.override_input.original_decimal = "1.25";
        const auto q = pin("L", "S", "T", "Q", "J", "EUR", "GBP", "0.50");
        return {empty_basis,
                {first, q},
                {"T",
                 "S",
                 {},
                 {},
                 {first.override_input, q.override_input},
                 {core::ManualRateConstraint{"O"}, core::ManualRateConstraint{"Q"}},
                 {{{{"EUR", "1", "1"}, {"GBP", "1", "2"}, {"USD", "4", "5"}}}}},
                {"U",
                 "T",
                 {},
                 {},
                 {q.override_input},
                 {core::ManualRateConstraint{"Q"}},
                 {{{{"EUR", "1", "1"}, {"GBP", "1", "2"}}}}},
                {{"EUR", "USD", ratio(4, 5), {}},
                 {"GBP", "USD", ratio(8, 5), {}},
                 {"USD", "EUR", ratio(5, 4), {}},
                 {"USD", "GBP", ratio(5, 8), {}}}};
    }

    const auto f = selected_f();
    const auto q = pin("L", "S", "T", "Q", "J", "CAD", "JPY", "3.00");
    Example value{
        {"B", {f, source_w, unused_g}, {{0, 1}, {0, 2}, {1, 1}}},
        {first, q},
        {"T",
         "S",
         {},
         {f},
         {first.override_input, q.override_input},
         {core::FetchedRateConstraint{"F", 1}, core::ManualRateConstraint{"O"},
          core::ManualRateConstraint{"Q"}},
         {{{{"CAD", "1", "1"}, {"JPY", "3", "1"}}},
          {{{"EUR", "1", "1"}, {"GBP", "9", "10"}, {"USD", "6", "5"}}}}},
        {"U",
         "T",
         {},
         {f},
         {q.override_input},
         {core::FetchedRateConstraint{"F", 1}, core::FetchedRateConstraint{"F", 2},
          core::ManualRateConstraint{"Q"}},
         {{{{"CAD", "1", "1"}, {"JPY", "3", "1"}}},
          {{{"EUR", "1", "1"}, {"GBP", "9", "10"}, {"USD", "13", "10"}}}}},
        {change("EUR", "USD", 13, 10, ratio(6, 5)), change("GBP", "USD", 13, 9, ratio(4, 3)),
         change("USD", "EUR", 10, 13, ratio(5, 6)), change("USD", "GBP", 9, 13, ratio(3, 4))}};
    if (start == Start::pin_w_first) {
        std::swap(value.basis.edges[1], value.basis.edges[2]);
        value.after.sources.push_back(source_w);
        value.after.constraints[1] = core::FetchedRateConstraint{"W", 1};
        value.after.components[1].members[2] = {"USD", "9", "7"};
        value.changes = {
            change("EUR", "USD", 9, 7, ratio(6, 5)), change("GBP", "USD", 10, 7, ratio(4, 3)),
            change("USD", "EUR", 7, 9, ratio(5, 6)), change("USD", "GBP", 7, 10, ratio(3, 4))};
    }
    return value;
}

inline QByteArray expected_intent(Start start, bool new_source = false) {
    // Common RRI1 fields: I, Z, M, base, U, EUR, USD, O, A, B.
    auto bytes = QByteArray::fromHex("52524931 0000000149 000000015a 000000014d 00000001");
    bytes += start == Start::equal_rates ? 'S' : 'T';
    bytes += QByteArray::fromHex(
        "0000000155 00000003455552 00000003555344 000000014f 0000000141 0000000142");
    switch (start) {
    case Start::temporary:
        return bytes + QByteArray::fromHex("00000000 00000000");
    case Start::equal_rates:
        return bytes +
               QByteArray::fromHex("00000001 0000000146 00000001 00000000 0000000000000002");
    case Start::pin_f_first:
    case Start::pin_w_first:
        bytes += QByteArray::fromHex(new_source ? "00000004" : "00000003");
        bytes += QByteArray::fromHex("0000000146 0000000157 0000000147");
        if (new_source)
            bytes += QByteArray::fromHex("0000000158");
        bytes += QByteArray::fromHex("00000003 00000000 0000000000000001");
        return bytes +
               QByteArray::fromHex(start == Start::pin_f_first
                                       ? "00000000 0000000000000002 00000001 0000000000000001"
                                       : "00000001 0000000000000001 00000000 0000000000000002");
    }
    throw std::logic_error("Missing intent oracle");
}

inline QByteArray expected_audit(Start start, bool known_time, bool role_witness = false) {
    // Initial fixture account/session N/R/P, catalog permission-rates-v1,
    // graph revision one, individual witness U/G. Known time is UTC0.
    auto bytes = QByteArray::fromHex("52524431 0000000149 000000015a 000000014d 00000001");
    bytes += start == Start::equal_rates ? 'S' : 'T';
    bytes += QByteArray::fromHex("0000000155 00000003455552 00000003555344 000000014f "
                                 "0000000141 000000014e 0000000152 0000000150 "
                                 "000000137065726d697373696f6e2d72617465732d7631 0000000000000001");
    // Role witness stores source RoleAllow, grant G and role RAllow.
    bytes += QByteArray::fromHex(
        role_witness ? "01 00000009526f6c65416c6c6f77 0000000147 0000000652416c6c6f77"
                     : "00 0000000155 0000000147");
    return bytes + QByteArray::fromHex(known_time ? "01 0000000000000000" : "00");
}

inline const QByteArray w_rob = QByteArray::fromHex(
    "524f4231 0000000157 0000000170 0000000163 0000000171 0000000165 "
    "00 00 00 00 00 00 00000001 00000003555344 00000003474250 00000004302e3730");
inline const QByteArray x_rob =
    QByteArray::fromHex("524f4231 0000000158 0000000170 0000000163 0000000171 0000000165 "
                        "00 00 00 00 00 00 00000001 00000003434144 000000034a5059 00000003322e30");
inline const QByteArray equal_f_rob =
    QByteArray::fromHex("524f4231 0000000146 0000000170 0000000163 0000000171 0000000165 "
                        "00 01 0000000000000000 01 0000000154 00 01 00000000 00 "
                        "00000002 00000003455552 00000003474250 00000004302e3930 "
                        "00000003455552 00000003555344 00000004312e3230");
} // namespace removal_test
