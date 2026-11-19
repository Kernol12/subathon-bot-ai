// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <subathon/core/rate_override.hpp>
#include <variant>

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

// The ordinary F1 and F2 fixtures have two policy candidates in quote order.
std::array<RateOverrideFetchedEdge, 2> two_fetched_edges(const FetchedRateObservation &source) {
    return {{{source, 1}, {source, 2}}};
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
                              const ManualRateOverrideInput &expected,
                              const Exact &rate = Exact(5) / 4) {
    ASSERT_EQ(snapshot.overrides.size(), 1U);
    const auto &manual = snapshot.overrides.front();
    EXPECT_EQ(manual.override_ref, expected.override_ref);
    EXPECT_EQ(manual.kind, expected.kind);
    EXPECT_EQ(manual.from_currency, expected.from_currency);
    EXPECT_EQ(manual.to_currency, expected.to_currency);
    EXPECT_EQ(manual.rate.original(), expected.original_decimal);
    EXPECT_EQ(manual.rate.value(), rate);
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
        static_cast<void>(preview_rate_override(
            current, edit, two_fetched_edges(current.data().sources.front()), "S2"));
        ADD_FAILURE() << "nonpositive edit produced a preview";
    } catch (const RateSnapshotFailure &failure) {
        EXPECT_EQ(failure.code(), RateSnapshotError::nonpositive_rate);
    }
}

TEST(RateOverrideAcceptance, ManualPairDeterminesExactInverseAndCrossRates) {
    const auto current = make_effective_rate_snapshot(fetched_head());
    const auto preview = preview_rate_override(
        current, usd_eur_override(), two_fetched_edges(current.data().sources.front()), "S2");

    EXPECT_EQ(preview.candidate.snapshot_ref, "S2");
    EXPECT_EQ(preview.candidate.parent_snapshot_ref, "S1");
    EXPECT_FALSE(preview.replaced_override_ref);
    expect_rate(preview.candidate, "USD", "EUR", Exact(5) / 4);
    expect_rate(preview.candidate, "EUR", "USD", Exact(4) / 5);
    expect_rate(preview.candidate, "USD", "GBP", Exact(5) / 8);
    expect_rate(preview.candidate, "GBP", "USD", Exact(8) / 5);
    expect_rate(preview.candidate, "EUR", "GBP", Exact(1) / 2);
    const auto candidate = make_effective_rate_snapshot(preview.candidate);
    expect_original_override(candidate.data(), usd_eur_override());
}

void expect_usd_eur_preview_changes(const RateOverridePreview &preview) {
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
}

TEST(RateOverrideAcceptance, PreviewShowsExactAffectedPairsAndKeepsFetchedEvidence) {
    const auto current = make_effective_rate_snapshot(fetched_head());
    const auto preview = preview_rate_override(
        current, usd_eur_override(), two_fetched_edges(current.data().sources.front()), "S2");

    expect_usd_eur_preview_changes(preview);

    const auto proposed = make_effective_rate_snapshot(preview.candidate);
    ASSERT_EQ(proposed.data().sources.size(), 1U);
    expect_original_observation(proposed.data().sources.front().data(), original_observation());
    ASSERT_EQ(current.data().sources.size(), 1U);
    expect_original_observation(current.data().sources.front().data(), original_observation());
}

TEST(RateOverrideAcceptance, PreviewUsesSuppliedOrderForCompetingFetchedEdges) {
    auto basis = fetched_head();
    // This cross quote agrees with the original table before the pin is proposed.
    basis.sources.front().quotes.push_back({"USD", "GBP", "0.6000"});
    basis.constraints.push_back(FetchedRateConstraint{"F1", 3});
    const auto current = make_effective_rate_snapshot(basis);
    const auto &source = current.data().sources.front();
    const RateOverrideFetchedEdge edges[] = {{source, 3}, {source, 2}, {source, 1}};

    const auto preview = preview_rate_override(current, usd_eur_override(), edges, "S2");
    expect_rate(preview.candidate, "USD", "EUR", Exact(5) / 4);
    expect_rate(preview.candidate, "USD", "GBP", Exact(3) / 5);
    // Keeping the cross quote gives (3/5) ÷ (5/4) = 12/25, which excludes 1/2.
    expect_rate(preview.candidate, "EUR", "GBP", Exact(12) / 25);
    const auto candidate = make_effective_rate_snapshot(preview.candidate);
    ASSERT_EQ(candidate.data().sources.size(), 1U);
    expect_original_observation(candidate.data().sources.front().data(), basis.sources.front());
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
        static_cast<void>(preview_rate_override(current, edit, {}, "S2"));
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

ManualRateOverrideInput replacement_edit(std::string_view from, std::string_view to,
                                         std::string_view decimal) {
    return {"M2",
            RateOverrideKind::pinned,
            std::string(from),
            std::string(to),
            std::string(decimal),
            "decision-2",
            "actor-2",
            "Synthetic replacement",
            1200};
}

TEST(RateOverrideAcceptance, PreviewsExplicitSameDirectionReplacement) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::pinned));
    const auto edit = replacement_edit("USD", "EUR", "2.0000");
    const auto preview = preview_rate_override(
        current, edit, two_fetched_edges(current.data().sources.front()), "S2", "M1");

    EXPECT_EQ(preview.replaced_override_ref, "M1");
    expect_rate(preview.candidate, "USD", "EUR", Exact(2));
    expect_rate(preview.candidate, "EUR", "USD", Exact(1) / 2);
    expect_rate(preview.candidate, "USD", "GBP", Exact(1));
    const auto candidate = make_effective_rate_snapshot(preview.candidate);
    expect_original_override(candidate.data(), edit, Exact(2));
    expect_original_override(current.data(), usd_eur_override());
}

TEST(RateOverrideAcceptance, PreviewsExplicitReverseDirectionReplacementAtSameRate) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::pinned));
    const auto edit = replacement_edit("EUR", "USD", "0.8000");
    const auto preview = preview_rate_override(
        current, edit, two_fetched_edges(current.data().sources.front()), "S2", "M1");

    EXPECT_EQ(preview.replaced_override_ref, "M1");
    EXPECT_TRUE(preview.affected_rates.empty());
    expect_rate(preview.candidate, "USD", "EUR", Exact(5) / 4);
    expect_rate(preview.candidate, "EUR", "USD", Exact(4) / 5);
    const auto candidate = make_effective_rate_snapshot(preview.candidate);
    expect_original_override(candidate.data(), edit, Exact(4) / 5);
    expect_original_override(current.data(), usd_eur_override());
}

void expect_implicit_replacement_refused(const ManualRateOverrideInput &edit) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::pinned));
    try {
        static_cast<void>(preview_rate_override(
            current, edit, two_fetched_edges(current.data().sources.front()), "S2"));
        ADD_FAILURE() << "same-pair edit without an explicit replacement produced a preview";
    } catch (const RateSnapshotFailure &failure) {
        EXPECT_EQ(failure.code(), RateSnapshotError::invalid_override);
    }
    expect_original_override(current.data(), usd_eur_override());
}

TEST(RateOverrideAcceptance, RefusesImplicitSameDirectionReplacement) {
    expect_implicit_replacement_refused(replacement_edit("USD", "EUR", "2.0000"));
}

TEST(RateOverrideAcceptance, RefusesImplicitReverseDirectionReplacementAtSameRate) {
    expect_implicit_replacement_refused(replacement_edit("EUR", "USD", "0.8000"));
}

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

TEST(RateOverrideAcceptance, RequiresFreshPreviewAfterUnrelatedHeadChange) {
    const auto base = make_effective_rate_snapshot(fetched_head());
    auto newer_input = fetched_head();
    newer_input.snapshot_ref = "S2";
    newer_input.parent_snapshot_ref = "S1";
    newer_input.sources.push_back(accepted_update({{"JPY", "CAD", "0.0120"}}));
    newer_input.constraints.push_back(FetchedRateConstraint{"F2", 1});
    newer_input.components.insert(newer_input.components.begin(),
                                  RateComponentInput{{{"CAD", "1", "1"}, {"JPY", "250", "3"}}});
    const auto newer = make_effective_rate_snapshot(newer_input);

    EXPECT_EQ(check_rate_preview_base(newer, base.data().snapshot_ref),
              RatePreviewBaseStatus::stale);
    const auto old_preview = preview_rate_override(
        base, usd_eur_override(), two_fetched_edges(base.data().sources.front()), "S3");
    const RateOverrideFetchedEdge newer_edges[] = {
        {newer.data().sources[0], 1}, {newer.data().sources[0], 2}, {newer.data().sources[1], 1}};
    const auto fresh_preview = preview_rate_override(newer, usd_eur_override(), newer_edges, "S4");
    expect_usd_eur_preview_changes(old_preview);
    expect_usd_eur_preview_changes(fresh_preview);
    ASSERT_TRUE(fresh_preview.candidate.parent_snapshot_ref);
    EXPECT_EQ(*fresh_preview.candidate.parent_snapshot_ref, "S2");
    EXPECT_EQ(check_rate_preview_base(newer, *fresh_preview.candidate.parent_snapshot_ref),
              RatePreviewBaseStatus::current);
    expect_rate(fresh_preview.candidate, "JPY", "CAD", Exact(3) / 250);
}

TEST(RateOverrideAcceptance, TemporaryExpiresOnRelevantAcceptedUpdate) {
    const auto basis = manual_head(RateOverrideKind::temporary);
    const auto current = make_effective_rate_snapshot(basis);
    const auto observed = make_fetched_rate_observation(
        accepted_update({{"USD", "EUR", "1.4000"}, {"EUR", "GBP", "0.5000"}}));

    const auto proposed =
        propose_rate_update(current, &observed, two_fetched_edges(observed), "S2");
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

TEST(RateOverrideAcceptance, TemporaryExpiresWhenFetchedAndEffectiveRatesStayUnchanged) {
    auto basis = manual_head(RateOverrideKind::temporary);
    auto &manual = basis.overrides.front();
    manual.original_decimal = "1.2000";
    basis.components.front().members.back() = {"USD", "5", "6"};
    const auto current = make_effective_rate_snapshot(basis);
    const auto observed = make_fetched_rate_observation(
        accepted_update({{"USD", "EUR", "1.20"}, {"EUR", "GBP", "0.5000"}}));

    const auto proposed =
        propose_rate_update(current, &observed, two_fetched_edges(observed), "S2");
    ASSERT_TRUE(proposed);
    EXPECT_TRUE(proposed->overrides.empty());
    expect_rate(*proposed, "USD", "EUR", Exact(6) / 5);
    expect_rate(*proposed, "EUR", "USD", Exact(5) / 6);
    expect_original_override(current.data(), manual, Exact(6) / 5);
}

TEST(RateOverrideAcceptance, TemporaryExpiresOnFetchedCrossRate) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::temporary));
    const auto observed = make_fetched_rate_observation(
        accepted_update({{"USD", "CAD", "1.5000"}, {"CAD", "EUR", "0.9000"}}));

    const auto proposed =
        propose_rate_update(current, &observed, two_fetched_edges(observed), "S2");
    ASSERT_TRUE(proposed);
    EXPECT_TRUE(proposed->overrides.empty());
    expect_rate(*proposed, "USD", "EUR", Exact(27) / 20);
    expect_rate(*proposed, "EUR", "USD", Exact(20) / 27);
    expect_rate(*proposed, "USD", "CAD", Exact(3) / 2);
    expect_rate(*proposed, "CAD", "EUR", Exact(9) / 10);
}

TEST(RateOverrideAcceptance, TemporaryExpiresOnReverseFetchedQuote) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::temporary));
    const auto observed =
        make_fetched_rate_observation(accepted_update({{"EUR", "USD", "0.7000"}}));
    const RateOverrideFetchedEdge edges[] = {{observed, 1}};

    const auto proposed = propose_rate_update(current, &observed, edges, "S2");
    ASSERT_TRUE(proposed);
    EXPECT_TRUE(proposed->overrides.empty());
    expect_rate(*proposed, "EUR", "USD", Exact(7) / 10);
    expect_rate(*proposed, "USD", "EUR", Exact(10) / 7);
}

TEST(RateOverrideAcceptance, TemporarySurvivesUnrelatedAcceptedUpdate) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::temporary));
    const auto observed = make_fetched_rate_observation(
        accepted_update({{"JPY", "CAD", "0.0120"}, {"USD", "JPY", "100.0000"}}));
    const auto &cached = current.data().sources.front();
    const RateOverrideFetchedEdge edges[] = {
        {observed, 1}, {observed, 2}, {cached, 1}, {cached, 2}};

    const auto proposed = propose_rate_update(current, &observed, edges, "S2");
    ASSERT_TRUE(proposed);
    expect_rate(*proposed, "USD", "EUR", Exact(5) / 4);
    expect_rate(*proposed, "JPY", "CAD", Exact(3) / 250);
    expect_rate(*proposed, "USD", "JPY", Exact(100));
    const auto candidate = make_effective_rate_snapshot(*proposed);
    expect_original_override(candidate.data(), usd_eur_override(RateOverrideKind::temporary));
}

TEST(RateOverrideAcceptance, FailedUpdateKeepsTemporaryAndProposesNoSuccessor) {
    const auto basis = manual_head(RateOverrideKind::temporary);
    const auto current = make_effective_rate_snapshot(basis);

    EXPECT_FALSE(propose_rate_update(current, nullptr, {}, "S2"));
    expect_original_override(current.data(), usd_eur_override(RateOverrideKind::temporary));
    expect_rate(basis, "USD", "EUR", Exact(5) / 4);
}

TEST(RateOverrideAcceptance, PinSurvivesRelevantUpdateAndBothReportingCurrencyLookups) {
    const auto basis = manual_head(RateOverrideKind::pinned);
    const auto current = make_effective_rate_snapshot(basis);
    const auto update = accepted_update({{"USD", "EUR", "1.4000"}, {"EUR", "GBP", "0.4000"}});
    const auto observed = make_fetched_rate_observation(update);

    const auto proposed =
        propose_rate_update(current, &observed, two_fetched_edges(observed), "S2");
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

void expect_competing_fetched_edges(std::uint64_t first, std::uint64_t second,
                                    const Exact &eur_to_gbp, const Exact &usd_to_gbp) {
    const auto current = make_effective_rate_snapshot(manual_head(RateOverrideKind::pinned));
    const auto update = accepted_update({{"EUR", "GBP", "0.4000"}, {"USD", "GBP", "0.7000"}});
    const auto observed = make_fetched_rate_observation(update);
    const RateOverrideFetchedEdge edges[] = {{observed, first}, {observed, second}};

    const auto proposed = propose_rate_update(current, &observed, edges, "S2");
    ASSERT_TRUE(proposed);
    expect_rate(*proposed, "USD", "EUR", Exact(5) / 4);
    expect_rate(*proposed, "EUR", "GBP", eur_to_gbp);
    expect_rate(*proposed, "USD", "GBP", usd_to_gbp);

    const auto candidate = make_effective_rate_snapshot(*proposed);
    expect_original_override(candidate.data(), usd_eur_override());
    ASSERT_EQ(candidate.data().constraints.size(), 2U);
    const auto *selected = std::get_if<FetchedRateConstraint>(&candidate.data().constraints[0]);
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->observation_ref, "F2");
    EXPECT_EQ(selected->quote_ordinal, first);
    ASSERT_EQ(candidate.data().sources.size(), 1U);
    expect_original_observation(candidate.data().sources.front().data(), update);
    expect_original_observation(observed.data(), update);
}

TEST(RateOverrideAcceptance, KeepsFirstCompatibleFetchedEdgeInQuoteOrder) {
    // 5/4 × 2/5 = 1/2, which conflicts with the later 7/10 cross quote.
    expect_competing_fetched_edges(1, 2, Exact(2) / 5, Exact(1) / 2);
}

TEST(RateOverrideAcceptance, KeepsFirstCompatibleFetchedEdgeInReverseQuoteOrder) {
    // (7/10) ÷ (5/4) = 14/25, which conflicts with the later 2/5 quote.
    expect_competing_fetched_edges(2, 1, Exact(14) / 25, Exact(7) / 10);
}

} // namespace
