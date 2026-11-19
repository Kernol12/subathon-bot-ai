// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <gtest/gtest.h>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <subathon/core/rate_override.hpp>

namespace {
using namespace subathon::core;

FetchedRateObservationInput original_observation() {
    return {
        .context = {"F1",
                    "fake-source",
                    "config-1",
                    "request-1",
                    "evidence-1",
                    std::nullopt,
                    1234,
                    {"2026-11-04", std::nullopt},
                    {}},
        .quotes = {{"USD", "EUR", "1.2000"}, {"EUR", "GBP", "0.5000"}},
    };
}

FetchedRateObservationInput accepted_update(std::initializer_list<RateQuoteInput> quotes) {
    return {
        .context = {"F2",
                    "fake-source",
                    "config-2",
                    "request-2",
                    "evidence-2",
                    "provider-v2",
                    2468,
                    {"2026-11-05", std::nullopt},
                    {"", 0}},
        .quotes = quotes,
    };
}

ManualRateOverrideInput usd_eur_override(RateOverrideKind kind = RateOverrideKind::pinned) {
    return {"M1", kind, "USD", "EUR", "1.2500", "decision-1", "actor-1", "Synthetic edit", 1100};
}

EffectiveRateSnapshotInput fetched_head() {
    return {
        .snapshot_ref = "S1",
        .parent_snapshot_ref = std::nullopt,
        .constructed_at_utc_ms = std::nullopt,
        .sources = {original_observation()},
        .overrides = {},
        .constraints = {FetchedRateConstraint{"F1", 1}, FetchedRateConstraint{"F1", 2}},
        .components = {{{{"EUR", "1", "1"}, {"GBP", "1", "2"}, {"USD", "5", "6"}}}},
    };
}

// Literal current-state fixture. It does not depend on a preview or refresh succeeding.
EffectiveRateSnapshotInput manual_head(RateOverrideKind kind) {
    auto input = fetched_head();
    input.overrides = {usd_eur_override(kind)};
    input.constraints.front() = ManualRateConstraint{"M1"};
    input.components.front().members.back() = {"USD", "4", "5"};
    return input;
}

void expect_rate(const EffectiveRateSnapshotInput &snapshot, std::string_view from,
                 std::string_view to, const Exact &expected) {
    SCOPED_TRACE(testing::Message() << snapshot.snapshot_ref << ' ' << from << " -> " << to);
    const auto quote = quote_effective_rate(snapshot, from, to);
    EXPECT_EQ(quote.data().kind, RatePairKind::available);
    ASSERT_TRUE(quote.data().rate);
    EXPECT_EQ(*quote.data().rate, expected);
    ASSERT_TRUE(quote.data().provenance);
    EXPECT_EQ(quote.data().provenance->snapshot_ref, snapshot.snapshot_ref);
    EXPECT_EQ(quote.data().provenance->from_currency, from);
    EXPECT_EQ(quote.data().provenance->to_currency, to);
}

void expect_original_observation(const FetchedRateObservationData &actual,
                                 const FetchedRateObservationInput &expected) {
    SCOPED_TRACE(expected.context.observation_ref);
    EXPECT_EQ(actual.context.observation_ref, expected.context.observation_ref);
    EXPECT_EQ(actual.context.provider_ref, expected.context.provider_ref);
    EXPECT_EQ(actual.context.configuration_ref, expected.context.configuration_ref);
    EXPECT_EQ(actual.context.request_ref, expected.context.request_ref);
    EXPECT_EQ(actual.context.source_evidence_ref, expected.context.source_evidence_ref);
    EXPECT_EQ(actual.context.provider_snapshot_ref, expected.context.provider_snapshot_ref);
    EXPECT_EQ(actual.context.fetched_at_utc_ms, expected.context.fetched_at_utc_ms);
    EXPECT_EQ(actual.context.source_rate_time.original_text,
              expected.context.source_rate_time.original_text);
    EXPECT_EQ(actual.context.source_rate_time.utc_ms, expected.context.source_rate_time.utc_ms);
    EXPECT_EQ(actual.context.publication_time.original_text,
              expected.context.publication_time.original_text);
    EXPECT_EQ(actual.context.publication_time.utc_ms, expected.context.publication_time.utc_ms);
    ASSERT_EQ(actual.quotes.size(), expected.quotes.size());
    for (std::size_t i = 0; i < expected.quotes.size(); ++i) {
        EXPECT_EQ(actual.quotes[i].from_currency, expected.quotes[i].from_currency);
        EXPECT_EQ(actual.quotes[i].to_currency, expected.quotes[i].to_currency);
        EXPECT_EQ(actual.quotes[i].rate.original(), expected.quotes[i].original_decimal);
    }
}

void expect_original_override(const EffectiveRateSnapshotData &snapshot,
                              const ManualRateOverrideInput &expected) {
    ASSERT_EQ(snapshot.overrides.size(), 1U);
    const auto &manual = snapshot.overrides.front();
    EXPECT_EQ(manual.override_ref, expected.override_ref);
    EXPECT_EQ(manual.kind, expected.kind);
    EXPECT_EQ(manual.from_currency, expected.from_currency);
    EXPECT_EQ(manual.to_currency, expected.to_currency);
    EXPECT_EQ(manual.rate.original(), expected.original_decimal);
    EXPECT_EQ(manual.rate.value(), Exact(5) / 4);
    EXPECT_EQ(manual.decision_ref, expected.decision_ref);
    EXPECT_EQ(manual.actor_ref, expected.actor_ref);
    EXPECT_EQ(manual.reason, expected.reason);
    EXPECT_EQ(manual.recorded_at_utc_ms, expected.recorded_at_utc_ms);
}

void expect_nonpositive_preview(std::string_view decimal) {
    const auto current = make_effective_rate_snapshot(fetched_head());
    auto edit = usd_eur_override();
    edit.original_decimal = decimal;
    try {
        static_cast<void>(preview_rate_override(current, edit, "S2"));
        ADD_FAILURE() << "nonpositive edit produced a preview";
    } catch (const RateSnapshotFailure &failure) {
        EXPECT_EQ(failure.code(), RateSnapshotError::nonpositive_rate);
    }
}

TEST(RateOverrideAcceptance, ManualPairDeterminesExactInverseAndCrossRates) {
    const auto current = make_effective_rate_snapshot(fetched_head());
    const auto preview = preview_rate_override(current, usd_eur_override(), "S2");

    EXPECT_EQ(preview.candidate.snapshot_ref, "S2");
    EXPECT_EQ(preview.candidate.parent_snapshot_ref, "S1");
    expect_rate(preview.candidate, "USD", "EUR", Exact(5) / 4);
    expect_rate(preview.candidate, "EUR", "USD", Exact(4) / 5);
    expect_rate(preview.candidate, "USD", "GBP", Exact(5) / 8);
    expect_rate(preview.candidate, "GBP", "USD", Exact(8) / 5);
    expect_rate(preview.candidate, "EUR", "GBP", Exact(1) / 2);
    const auto candidate = make_effective_rate_snapshot(preview.candidate);
    expect_original_override(candidate.data(), usd_eur_override());
}

TEST(RateOverrideAcceptance, PreviewShowsExactAffectedPairsAndKeepsFetchedEvidence) {
    const auto current = make_effective_rate_snapshot(fetched_head());
    const auto preview = preview_rate_override(current, usd_eur_override(), "S2");

    const RatePairChange expected[] = {
        {"USD", "EUR", Exact(6) / 5, Exact(5) / 4},
        {"EUR", "USD", Exact(5) / 6, Exact(4) / 5},
        {"USD", "GBP", Exact(3) / 5, Exact(5) / 8},
        {"GBP", "USD", Exact(5) / 3, Exact(8) / 5},
    };
    ASSERT_EQ(preview.affected_rates.size(), std::size(expected));
    for (const auto &pair : expected) {
        SCOPED_TRACE(testing::Message() << pair.from_currency << " -> " << pair.to_currency);
        const auto found = std::ranges::find_if(preview.affected_rates, [&](const auto &change) {
            return change.from_currency == pair.from_currency &&
                   change.to_currency == pair.to_currency;
        });
        ASSERT_NE(found, preview.affected_rates.end());
        EXPECT_EQ(found->before, pair.before);
        EXPECT_EQ(found->after, pair.after);
    }

    const auto proposed = make_effective_rate_snapshot(preview.candidate);
    ASSERT_EQ(proposed.data().sources.size(), 1U);
    expect_original_observation(proposed.data().sources.front().data(), original_observation());
    ASSERT_EQ(current.data().sources.size(), 1U);
    expect_original_observation(current.data().sources.front().data(), original_observation());
}

TEST(RateOverrideAcceptance, RefusesContradictoryPinCycleBeforeProducingCandidate) {
    auto basis = manual_head(RateOverrideKind::pinned);
    basis.sources.clear();
    basis.overrides.push_back({"M2", RateOverrideKind::pinned, "EUR", "GBP", "0.5000", "decision-2",
                               std::nullopt, "Second pin", std::nullopt});
    basis.constraints.back() = ManualRateConstraint{"M2"};
    const auto current = make_effective_rate_snapshot(basis);
    const ManualRateOverrideInput edit{"M3",         RateOverrideKind::pinned, "USD",
                                       "GBP",        "0.62500000000000000001", "decision-3",
                                       std::nullopt, "Conflicting pin",        std::nullopt};

    try {
        static_cast<void>(preview_rate_override(current, edit, "S2"));
        ADD_FAILURE() << "contradictory pin produced a preview";
    } catch (const RateSnapshotFailure &failure) {
        EXPECT_EQ(failure.code(), RateSnapshotError::contradictory_constraint);
    }

    expect_rate(basis, "USD", "GBP", Exact(5) / 8);
    ASSERT_EQ(current.data().overrides.size(), 2U);
    EXPECT_EQ(current.data().overrides[0].override_ref, "M1");
    EXPECT_EQ(current.data().overrides[1].override_ref, "M2");
}

TEST(RateOverrideAcceptance, RefusesZeroRate) { expect_nonpositive_preview("0"); }

TEST(RateOverrideAcceptance, RefusesNegativeRate) { expect_nonpositive_preview("-0.01"); }

TEST(RateOverrideAcceptance, DetectsStalePreviewAfterAffectedRateChanges) {
    const auto base = make_effective_rate_snapshot(fetched_head());
    auto newer_input = fetched_head();
    newer_input.snapshot_ref = "S2";
    newer_input.parent_snapshot_ref = "S1";
    newer_input.sources = {accepted_update({{"USD", "EUR", "1.4000"}, {"EUR", "GBP", "0.5000"}})};
    newer_input.constraints = {FetchedRateConstraint{"F2", 1}, FetchedRateConstraint{"F2", 2}};
    newer_input.components.front().members.back() = {"USD", "5", "7"};
    const auto newer = make_effective_rate_snapshot(newer_input);

    EXPECT_EQ(check_rate_preview_base(newer, base.data().snapshot_ref),
              RatePreviewBaseStatus::stale);
    EXPECT_EQ(check_rate_preview_base(base, base.data().snapshot_ref),
              RatePreviewBaseStatus::current);
}

TEST(RateOverrideAcceptance, TemporaryExpiresOnRelevantAcceptedUpdate) {
    const auto basis = manual_head(RateOverrideKind::temporary);
    const auto current = make_effective_rate_snapshot(basis);
    const auto observed = make_fetched_rate_observation(
        accepted_update({{"USD", "EUR", "1.4000"}, {"EUR", "GBP", "0.5000"}}));

    const auto proposed = propose_rate_update(current, &observed, "S2");
    ASSERT_TRUE(proposed);
    EXPECT_EQ(proposed->snapshot_ref, "S2");
    EXPECT_EQ(proposed->parent_snapshot_ref, "S1");
    EXPECT_TRUE(proposed->overrides.empty());
    expect_rate(*proposed, "USD", "EUR", Exact(7) / 5);
    expect_rate(*proposed, "EUR", "USD", Exact(5) / 7);
    expect_rate(*proposed, "USD", "GBP", Exact(7) / 10);
    expect_original_override(current.data(), usd_eur_override(RateOverrideKind::temporary));
    expect_rate(basis, "USD", "EUR", Exact(5) / 4);
}

TEST(RateOverrideAcceptance, TemporarySurvivesUnrelatedAcceptedUpdate) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::temporary));
    const auto observed =
        make_fetched_rate_observation(accepted_update({{"JPY", "CAD", "0.0120"}}));

    const auto proposed = propose_rate_update(current, &observed, "S2");
    ASSERT_TRUE(proposed);
    expect_rate(*proposed, "USD", "EUR", Exact(5) / 4);
    expect_rate(*proposed, "JPY", "CAD", Exact(3) / 250);
    const auto candidate = make_effective_rate_snapshot(*proposed);
    expect_original_override(candidate.data(), usd_eur_override(RateOverrideKind::temporary));
}

TEST(RateOverrideAcceptance, FailedUpdateKeepsTemporaryAndProposesNoSuccessor) {
    const auto basis = manual_head(RateOverrideKind::temporary);
    const auto current = make_effective_rate_snapshot(basis);

    EXPECT_FALSE(propose_rate_update(current, nullptr, "S2"));
    expect_original_override(current.data(), usd_eur_override(RateOverrideKind::temporary));
    expect_rate(basis, "USD", "EUR", Exact(5) / 4);
}

TEST(RateOverrideAcceptance, PinSurvivesRelevantUpdateAndBothReportingCurrencyLookups) {
    const auto basis = manual_head(RateOverrideKind::pinned);
    const auto current = make_effective_rate_snapshot(basis);
    const auto update = accepted_update({{"USD", "EUR", "1.4000"}, {"EUR", "GBP", "0.4000"}});
    const auto observed = make_fetched_rate_observation(update);

    const auto proposed = propose_rate_update(current, &observed, "S2");
    ASSERT_TRUE(proposed);
    EXPECT_EQ(proposed->snapshot_ref, "S2");
    EXPECT_EQ(proposed->parent_snapshot_ref, "S1");
    expect_rate(*proposed, "USD", "EUR", Exact(5) / 4);
    expect_rate(*proposed, "USD", "GBP", Exact(1) / 2);

    const auto candidate = make_effective_rate_snapshot(*proposed);
    expect_original_override(candidate.data(), usd_eur_override());
    const auto &sources = candidate.data().sources;
    const auto updated_source = std::ranges::find_if(
        sources, [](const auto &source) { return source.data().context.observation_ref == "F2"; });
    ASSERT_NE(updated_source, sources.end());
    expect_original_observation(updated_source->data(), update);
    expect_original_observation(observed.data(), update);
    ASSERT_EQ(current.data().sources.size(), 1U);
    expect_original_observation(current.data().sources.front().data(), original_observation());
    expect_rate(basis, "USD", "GBP", Exact(5) / 8);
}

} // namespace
