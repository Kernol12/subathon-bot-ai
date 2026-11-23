// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <subathon/server/scheduled_pause.hpp>

namespace {
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace subathon::core;
using namespace subathon::server;
using ZoneError = ScheduleZoneUnavailable;
using SupportError = ForecastSupportFailure;
using Instant = std::int64_t;
using PageReply = std::variant<ForecastPage, ZoneError>;
using LegacyReply = std::variant<ScheduleZoneFacts, ZoneError>;
using PreviewReply = std::variant<ScheduledPausePreview, ScheduledPauseRefusal>;

constexpr Instant utc(year_month_day date, milliseconds time = 0ms) {
    return duration_cast<milliseconds>(sys_days{date}.time_since_epoch()).count() + time.count();
}
constexpr Instant after(Instant instant, milliseconds by) { return instant + by.count(); }
constexpr auto january8 = 2026y / January / 8;
constexpr Instant cut_time = utc(january8, 12h);
constexpr Instant fold_at = after(cut_time, 1h);
constexpr Instant lower = after(cut_time, -48h);
constexpr std::array wednesday{false, false, true, false, false, false, false};
constexpr std::array thursday{false, false, false, true, false, false, false};

ScheduledPauseRule rule(std::string id, milliseconds time, milliseconds duration,
                        ScheduleFoldPolicy fold = ScheduleFoldPolicy::RunOnce) {
    return {std::move(id), {.local_time = time, .fold_policy = fold}, duration, true};
}

ScheduledPauseConfiguration configuration(std::string zone,
                                          std::vector<ScheduledPauseRule> rules = {}) {
    return {std::move(zone), std::move(rules)};
}

ScheduleZoneFacts facts(std::string zone, std::string revision, Instant from, Instant through,
                        std::int32_t offset = 0,
                        std::vector<ScheduleZoneTransition> transitions = {}) {
    return {std::move(zone), std::move(revision), from, through, offset, std::move(transitions)};
}

// Model facts from an explicit UTC timeline. Native forecast replies below are
// separately scripted so each request boundary can be asserted by the test.
class LegacyZone : public ScheduleZoneSource {
  public:
    std::vector<ScheduleZoneFacts> views;
    std::optional<LegacyReply> scripted;
    std::vector<ScheduleZoneRequest> legacy_requests;

    LegacyReply read(const ScheduleZoneRequest &request) override {
        legacy_requests.push_back(request);
        if (scripted) {
            return *scripted;
        }
        for (const auto &view : views) {
            if (request.configured_zone != view.configured_zone ||
                (request.required_rules_revision &&
                 request.required_rules_revision != view.rules_revision) ||
                request.from_utc_ms < view.from_utc_ms ||
                request.from_utc_ms > view.through_utc_ms) {
                continue;
            }
            auto result = facts(view.configured_zone, view.rules_revision, request.from_utc_ms,
                                std::min(request.through_utc_ms, view.through_utc_ms),
                                view.offset_at_start_seconds);
            for (const auto &change : view.transitions) {
                if (change.at_utc_ms <= result.from_utc_ms) {
                    result.offset_at_start_seconds = change.offset_after_seconds;
                } else if (change.at_utc_ms <= result.through_utc_ms) {
                    result.transitions.push_back(change);
                }
            }
            return result;
        }
        return ZoneError::insufficient_coverage;
    }
};

class PagedZone final : public LegacyZone {
  public:
    std::vector<PageReply> replies;
    std::vector<ScheduleZoneRequest> page_requests;

    PageReply read_forecast_page(const ScheduleZoneRequest &request) override {
        page_requests.push_back(request);
        const auto index = page_requests.size() - 1;
        return index < replies.size() ? replies[index] : PageReply{ZoneError::source_unavailable};
    }
};

ScheduledPauseObservation observation(Instant when) {
    return {"observation-" + std::to_string(when), when};
}

ScheduledPauseElapsedCut elapsed_cut(Instant at, milliseconds elapsed) {
    return {observation(at), elapsed, {}};
}

// Build each Active basis using the shipped entry projector. Keep its own
// clock, config, timer, cursor and ledger data when projecting the edit at C.
class EnteredBasis {
  public:
    EnteredBasis(ScheduleZoneSource &source, Instant at = cut_time,
                 ScheduledPauseConfiguration previous = configuration("Old/Zero"))
        : epochs_{{std::move(previous), {}, {}, {}}},
          state_{.snapshot_ref = "ready-snapshot",
                 .timer_lineage = "timer",
                 .clock_lineage = "clock",
                 .state_revision = "ready-state",
                 .configuration_revision = "configuration",
                 .timer_config = {.end_mode = TimerEndMode::Immediate, .hard_end_utc_ms = {}},
                 .timer = {.lifecycle = TimerLifecycle::Ready, .main_remaining_ms = 900'000},
                 .pause = {},
                 .cursor = {observation(at - 2), {}, {}},
                 .epochs = epochs_,
                 .epoch_history_complete = true,
                 .ledger = &ledger_} {
        const ScheduledPauseEntry entry{
            state_.snapshot_ref,
            observation(at - 1),
            {.lifecycle = TimerLifecycle::Active, .main_remaining_ms = 900'000}};
        auto entered = enter_scheduled_pauses(state_, entry, source);
        const auto *projection = std::get_if<ScheduledPauseProgress>(&entered);
        if (!projection || projection->notice || projection->cursor.hold ||
            projection->cursor.last_observation.utc_ms != at - 1 ||
            projection->cursor.evaluated_through_utc_ms.has_value() ||
            projection->timer.lifecycle != TimerLifecycle::Active ||
            projection->timer.main_remaining_ms != 900'000 || !projection->new_deliveries.empty() ||
            !projection->stops.empty()) {
            throw std::logic_error("the shipped Start failed the synthetic basis controls");
        }
        state_.snapshot_ref = "active-snapshot";
        state_.state_revision = "active-state";
        state_.timer = projection->timer;
        state_.pause = projection->pause;
        state_.cursor = projection->cursor;
    }

    EnteredBasis(const EnteredBasis &) = delete;
    EnteredBasis &operator=(const EnteredBasis &) = delete;

    const ScheduledPauseState &state() const { return state_; }

    ScheduledPauseClockInput clock(std::span<const ScheduledPauseElapsedCut> cuts) const {
        return {state_.snapshot_ref, state_.clock_lineage, state_.cursor.last_observation, cuts};
    }

    PreviewReply preview(ScheduleZoneSource &source, const ScheduledPauseConfiguration &proposal,
                         Instant horizon) const {
        const std::array cuts{elapsed_cut(state_.cursor.last_observation.utc_ms + 1, 1ms)};
        return project_scheduled_pause_edit(state_, clock(cuts), proposal, horizon, source);
    }

  private:
    std::vector<ScheduledPauseEpoch> epochs_;
    ScheduledPauseLedger ledger_{"timer", "deliveries", {}};
    ScheduledPauseState state_;
};

void expect_observation(const ScheduledPauseObservation &value, Instant at) {
    EXPECT_EQ(value.utc_ms, at);
    EXPECT_EQ(value.observation_ref, observation(at).observation_ref);
}

void expect_forecast(const ScheduledPausePreview &value, Instant at, Instant horizon,
                     Instant frontier, std::optional<ForecastFailure> incomplete = {}) {
    expect_observation(value.as_of, at);
    EXPECT_EQ(value.requested_through_utc_ms, horizon);
    EXPECT_EQ(value.resolved_through_utc_ms, frontier);
    EXPECT_EQ(value.incomplete, incomplete);
}

ForecastFailure coverage(ZoneError cause) { return ScheduledPauseCoverageFailure{cause}; }

ScheduledPauseKey key(std::string id, Instant nominal,
                      ScheduleFoldRun run = ScheduleFoldRun::First) {
    return {"timer", std::move(id), {ScheduleLocalTime{milliseconds{nominal}}, run}};
}

void expect_row(const ScheduledPausePreviewRow &value, const ScheduledPauseKey &identity,
                Instant start, Instant end, Instant local_start, Instant local_end) {
    EXPECT_EQ(value.key, identity);
    EXPECT_EQ(value.window,
              (ScheduledPauseOccurrence{
                  {identity.occurrence, start, ScheduleLocalTime{milliseconds{local_start}}},
                  end,
                  ScheduleLocalTime{milliseconds{local_end}}}));
}

void expect_page_requests(const PagedZone &source, std::string_view zone, Instant through,
                          const std::vector<Instant> &starts, std::string_view revision,
                          std::optional<std::string_view> initially_required = {}) {
    ASSERT_EQ(source.page_requests.size(), starts.size());
    for (std::size_t i = 0; i < starts.size(); ++i) {
        SCOPED_TRACE(i);
        const auto &request = source.page_requests[i];
        EXPECT_EQ(request.configured_zone, zone);
        EXPECT_EQ(request.from_utc_ms, starts[i]);
        EXPECT_EQ(request.through_utc_ms, through);
        EXPECT_EQ(request.required_rules_revision,
                  i == 0 ? initially_required : std::optional<std::string_view>{revision});
    }
}

void expect_fact(const ScheduledPauseFactBinding &binding, const ScheduleZoneFacts &expected,
                 std::optional<std::size_t> old_epoch = {}) {
    EXPECT_EQ(binding.old_epoch_index, old_epoch);
    EXPECT_EQ(binding.facts.configured_zone, expected.configured_zone);
    EXPECT_EQ(binding.facts.rules_revision, expected.rules_revision);
    EXPECT_EQ(binding.facts.from_utc_ms, expected.from_utc_ms);
    EXPECT_EQ(binding.facts.through_utc_ms, expected.through_utc_ms);
    EXPECT_EQ(binding.facts.offset_at_start_seconds, expected.offset_at_start_seconds);
    ASSERT_EQ(binding.facts.transitions.size(), expected.transitions.size());
    for (std::size_t i = 0; i < expected.transitions.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(binding.facts.transitions[i].at_utc_ms, expected.transitions[i].at_utc_ms);
        EXPECT_EQ(binding.facts.transitions[i].offset_after_seconds,
                  expected.transitions[i].offset_after_seconds);
    }
}

const ScheduleZoneFacts &page_facts(const PageReply &reply) {
    return std::get<ForecastPage>(reply).facts;
}

std::vector<ScheduleZoneTransition> alternating(Instant first, milliseconds step, std::size_t count,
                                                std::int32_t odd, std::int32_t even = 0) {
    std::vector<ScheduleZoneTransition> result;
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(
            {after(first, step * static_cast<Instant>(index)), index % 2 == 0 ? odd : even});
    }
    return result;
}

constexpr auto long_horizon = cut_time + 1;
constexpr auto long_right = after(long_horizon, days{366});
constexpr auto last_supported_change = after(cut_time, days{344} + 6h);
constexpr auto unsupported_change = after(cut_time, days{344} + 8h);

ScheduledPauseConfiguration long_rule(std::string zone) {
    return configuration(std::move(zone), {rule("long", 12h + 1ms, days{366})});
}

TEST(ScheduledPauseForecast, LongDenseDurationCertifiesAnExactOneMillisecondHorizon) {
    PagedZone zone;
    const auto first_change = utc(2026y / January / 11);
    const auto page_edge = utc(2026y / November / 27) - 1;
    const auto all_changes = alternating(first_change, days{5}, 73, 60);
    zone.views.push_back(facts("Dense/Five", "r1", lower - 1, long_right, 0, all_changes));
    zone.replies = {ForecastPage{facts("Dense/Five", "r1", lower, page_edge, 0,
                                       {all_changes.begin(), all_changes.begin() + 64}),
                                 TransitionCap{}},
                    ForecastPage{facts("Dense/Five", "r1", page_edge, long_right, 0,
                                       {all_changes.begin() + 64, all_changes.end()}),
                                 ReachedRequestEnd{}}};
    EnteredBasis basis(zone);
    const auto projected = basis.preview(zone, long_rule("Dense/Five"), long_horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    ASSERT_FALSE(preview->incomplete);
    expect_forecast(*preview, cut_time, long_horizon, long_horizon);
    ASSERT_EQ(preview->upcoming.size(), 1U);
    expect_row(preview->upcoming.front(), key("long", long_horizon), long_horizon, long_right,
               long_horizon, after(long_right, 1min));
    EXPECT_EQ(preview->source_snapshot_ref, "active-snapshot");
    EXPECT_EQ(preview->state_revision, "active-state");
    EXPECT_EQ(preview->configuration_revision, "configuration");
    EXPECT_EQ(preview->clock_lineage, "clock");
    EXPECT_EQ(preview->timer_lineage, "timer");
    EXPECT_EQ(preview->delivery_revision, "deliveries");
    EXPECT_TRUE(preview->cutover_effects.empty());
    ASSERT_TRUE(preview->pause_after_cutover);
    EXPECT_TRUE(preview->pause_after_cutover->reasons.empty());
    ASSERT_EQ(preview->facts.size(), 2U);
    expect_fact(preview->facts[0], page_facts(zone.replies[0]));
    expect_fact(preview->facts[1], page_facts(zone.replies[1]));
    expect_page_requests(zone, "Dense/Five", long_right, {lower, page_edge}, "r1");
    EXPECT_TRUE(zone.legacy_requests.empty());

    const std::array cuts{elapsed_cut(cut_time, 1ms)};
    const auto cutover = project_scheduled_pause_cutover(basis.state(), basis.clock(cuts),
                                                         long_rule("Dense/Five"), zone);
    const auto *accepted = std::get_if<ScheduledPauseCutover>(&cutover);
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->progress.timer.main_remaining_ms, 899'999);
    EXPECT_TRUE(accepted->progress.new_deliveries.empty());
    EXPECT_TRUE(accepted->progress.stops.empty());
    EXPECT_TRUE(accepted->progress.timer_transitions.empty());
    EXPECT_EQ(accepted->progress.cursor.evaluated_through_utc_ms, cut_time);
    EXPECT_FALSE(accepted->progress.cursor.hold);
    EXPECT_FALSE(zone.legacy_requests.empty());
    EXPECT_EQ(zone.page_requests.size(), 2U);
}

enum class FinalPage { complete, attested_cap, terminal_prefix, malformed_cap, oversized };

void dense_two(PagedZone &zone, FinalPage tail) {
    const std::string revision = tail == FinalPage::complete ? "r1" : "r2";
    const auto changes = alternating(after(cut_time, days{3}), 2h, 4097, 1);
    const auto size = tail == FinalPage::complete ? 4096U : 4097U;
    zone.views.push_back(facts("Dense/Two", revision, lower - 1, long_right, 0,
                               {changes.begin(), changes.begin() + size}));
    for (std::size_t page = 0; page < 64; ++page) {
        const auto index = page * 64;
        const auto from = index == 0 ? lower : changes[index].at_utc_ms - 1;
        const auto through = changes[index + 64].at_utc_ms - 1;
        std::vector<ScheduleZoneTransition> rows{changes.begin() + index,
                                                 changes.begin() + index + 64};
        if (page != 63) {
            zone.replies.emplace_back(ForecastPage{
                facts("Dense/Two", revision, from, through, 0, std::move(rows)), TransitionCap{}});
            continue;
        }
        switch (tail) {
        case FinalPage::complete:
            zone.replies.emplace_back(
                ForecastPage{facts("Dense/Two", revision, from, long_right, 0, std::move(rows)),
                             ReachedRequestEnd{}});
            break;
        case FinalPage::attested_cap:
            zone.replies.emplace_back(ForecastPage{
                facts("Dense/Two", revision, from, through, 0, std::move(rows)), TransitionCap{}});
            break;
        case FinalPage::terminal_prefix:
            zone.replies.emplace_back(
                ForecastPage{facts("Dense/Two", revision, from, through, 0, std::move(rows)),
                             TerminalPrefix{ZoneError::insufficient_coverage}});
            break;
        case FinalPage::malformed_cap:
            rows.pop_back();
            zone.replies.emplace_back(ForecastPage{
                facts("Dense/Two", revision, from, through, 0, std::move(rows)), TransitionCap{}});
            break;
        case FinalPage::oversized:
            rows.push_back(changes.back());
            zone.replies.emplace_back(
                ForecastPage{facts("Dense/Two", revision, from, long_right, 0, std::move(rows)),
                             ReachedRequestEnd{}});
            break;
        }
    }
}

void expect_dense_reads(const PagedZone &zone, std::string_view revision) {
    std::vector<Instant> starts{lower};
    for (std::size_t page = 1; page < 64; ++page) {
        starts.push_back(after(cut_time, days{3} + hours{128 * page}) - 1);
    }
    expect_page_requests(zone, "Dense/Two", long_right, starts, revision);
}

TEST(ScheduledPauseForecast, Exactly4096ChangesFitAndTheEndAt366DaysIsExact) {
    PagedZone zone;
    dense_two(zone, FinalPage::complete);
    EnteredBasis basis(zone);
    const auto projected = basis.preview(zone, long_rule("Dense/Two"), long_horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    ASSERT_FALSE(preview->incomplete);
    expect_forecast(*preview, cut_time, long_horizon, long_horizon);
    ASSERT_EQ(preview->upcoming.size(), 1U);
    expect_row(preview->upcoming.front(), key("long", long_horizon), long_horizon, long_right,
               long_horizon, long_right);
    expect_dense_reads(zone, "r1");
    EXPECT_TRUE(zone.legacy_requests.empty());
    ASSERT_EQ(preview->facts.size(), 64U);
    ASSERT_EQ(preview->facts.back().facts.transitions.size(), 64U);
    EXPECT_EQ(preview->facts.back().facts.transitions.back().at_utc_ms, last_supported_change);
    for (std::size_t i = 0; i < 64; ++i) {
        SCOPED_TRACE(i);
        expect_fact(preview->facts[i], page_facts(zone.replies[i]));
    }
}

TEST(ScheduledPauseForecast, OnlyAnAttested4097thChangeCertifiesComplexity) {
    struct Case {
        FinalPage input;
        ForecastFailure failure;
        std::size_t retained_pages;
    };
    const std::array cases{
        Case{FinalPage::attested_cap, SupportError::beyond_supported_timezone_transitions, 64},
        Case{FinalPage::terminal_prefix, coverage(ZoneError::insufficient_coverage), 64},
        Case{FinalPage::malformed_cap, coverage(ZoneError::unsupported_rules), 0},
        Case{FinalPage::oversized, coverage(ZoneError::limit_exceeded), 63}};
    for (const auto &test : cases) {
        SCOPED_TRACE(static_cast<int>(test.input));
        PagedZone zone;
        dense_two(zone, test.input);
        EnteredBasis basis(zone);
        const auto projected = basis.preview(zone, long_rule("Dense/Two"), long_horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, long_horizon, cut_time, test.failure);
        EXPECT_TRUE(preview->upcoming.empty());
        ASSERT_TRUE(preview->pause_after_cutover);
        EXPECT_TRUE(preview->pause_after_cutover->reasons.empty());
        expect_dense_reads(zone, "r2");
        EXPECT_TRUE(zone.legacy_requests.empty());
        ASSERT_EQ(preview->facts.size(), test.retained_pages);
        for (std::size_t i = 0; i < test.retained_pages; ++i) {
            expect_fact(preview->facts[i], page_facts(zone.replies[i]));
        }
        if (test.retained_pages == 64) {
            EXPECT_EQ(preview->facts.back().facts.through_utc_ms, unsupported_change - 1);
        }
    }
}

void fold_zone(PagedZone &zone, Instant through) {
    const auto early = alternating(after(lower, 1min), 1min, 64, 0, 60);
    const std::vector<ScheduleZoneTransition> fold{{fold_at, 0}};
    auto whole = early;
    whole.insert(whole.end(), fold.begin(), fold.end());
    zone.views.push_back(facts("Fold/Test", "f1", lower - 1, through, 60, whole));
    zone.replies = {
        ForecastPage{facts("Fold/Test", "f1", lower, fold_at - 1, 60, early), TransitionCap{}},
        ForecastPage{facts("Fold/Test", "f1", fold_at - 1, through, 60, fold),
                     ReachedRequestEnd{}}};
}

TEST(ScheduledPauseForecast, FoldRunsKeepTheirIdentitiesAcrossTheSharedPageBoundary) {
    struct Case {
        ScheduleFoldPolicy policy;
        Instant horizon;
        bool second;
    };
    constexpr std::array cases{Case{ScheduleFoldPolicy::RunTwice, fold_at, false},
                               Case{ScheduleFoldPolicy::RunTwice, after(fold_at, 30s), true},
                               Case{ScheduleFoldPolicy::RunOnce, after(fold_at, 30s), false}};
    for (const auto &test : cases) {
        SCOPED_TRACE(test.horizon);
        PagedZone zone;
        const auto right = after(test.horizon, 48h);
        fold_zone(zone, right);
        EnteredBasis basis(zone);
        const auto config =
            configuration("Fold/Test", {rule("fold", 13h + 30s, 30min, test.policy)});
        const auto projected = basis.preview(zone, config, test.horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        ASSERT_FALSE(preview->incomplete);
        expect_forecast(*preview, cut_time, test.horizon, test.horizon);
        ASSERT_EQ(preview->upcoming.size(), test.second ? 2U : 1U);
        const auto nominal = after(fold_at, 30s);
        expect_row(preview->upcoming[0], key("fold", nominal), after(fold_at, -30s),
                   after(fold_at, 29min + 30s), nominal, after(fold_at, 29min + 30s));
        if (test.second) {
            expect_row(preview->upcoming[1], key("fold", nominal, ScheduleFoldRun::Second), nominal,
                       after(fold_at, 30min + 30s), nominal, after(fold_at, 30min + 30s));
        }
        ASSERT_EQ(preview->facts.size(), 2U);
        expect_fact(preview->facts[0], page_facts(zone.replies[0]));
        expect_fact(preview->facts[1], page_facts(zone.replies[1]));
        expect_page_requests(zone, "Fold/Test", right, {lower, fold_at - 1}, "f1");
        EXPECT_TRUE(zone.legacy_requests.empty());
    }
}

TEST(ScheduledPauseForecast, EndAtChangeUsesNewOffsetAndStartAtHIsIncluded) {
    PagedZone zone;
    fold_zone(zone, after(fold_at, 48h));
    EnteredBasis basis(zone);
    const auto config =
        configuration("Fold/Test", {rule("right-end", 13h - 29min, 30min),
                                    rule("left-end", 13h - 29min - 1ms, 30min),
                                    rule("fold", 13h, 30min, ScheduleFoldPolicy::RunTwice)});
    const auto projected = basis.preview(zone, config, fold_at);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    ASSERT_FALSE(preview->incomplete);
    expect_forecast(*preview, cut_time, fold_at, fold_at);
    ASSERT_EQ(preview->upcoming.size(), 4U);
    const auto first = after(fold_at, -30min);
    expect_row(preview->upcoming[0], key("left-end", after(fold_at, -29min) - 1), first - 1,
               fold_at - 1, after(fold_at, -29min) - 1, after(fold_at, 1min) - 1);
    expect_row(preview->upcoming[1], key("right-end", after(fold_at, -29min)), first, fold_at,
               after(fold_at, -29min), fold_at);
    expect_row(preview->upcoming[2], key("fold", fold_at), after(fold_at, -1min),
               after(fold_at, 29min), fold_at, after(fold_at, 29min));
    expect_row(preview->upcoming[3], key("fold", fold_at, ScheduleFoldRun::Second), fold_at,
               after(fold_at, 30min), fold_at, after(fold_at, 30min));
}

TEST(ScheduledPauseForecast, InvalidContinuationsDiscardTheWholeProposedView) {
    enum class Defect { revision, shared_offset, early, late, first_change };
    constexpr std::array defects{Defect::revision, Defect::shared_offset, Defect::early,
                                 Defect::late, Defect::first_change};
    for (const auto defect : defects) {
        SCOPED_TRACE(static_cast<int>(defect));
        PagedZone zone;
        const auto right = after(fold_at, 48h);
        fold_zone(zone, right);
        auto &continued = std::get<ForecastPage>(zone.replies[1]).facts;
        switch (defect) {
        case Defect::revision:
            continued.rules_revision = "another";
            break;
        case Defect::shared_offset:
            continued.offset_at_start_seconds = 61;
            break;
        case Defect::early:
            --continued.from_utc_ms;
            break;
        case Defect::late:
            ++continued.from_utc_ms;
            continued.offset_at_start_seconds = 0;
            continued.transitions.clear();
            break;
        case Defect::first_change:
            ++continued.transitions.front().at_utc_ms;
            break;
        }
        EnteredBasis basis(zone);
        const auto proposed = configuration("Fold/Test", {rule("fold", 13h + 30s, 30min)});
        const auto projected = basis.preview(zone, proposed, fold_at);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, fold_at, cut_time,
                        coverage(ZoneError::unsupported_rules));
        EXPECT_TRUE(preview->upcoming.empty());
        EXPECT_TRUE(preview->facts.empty());
        EXPECT_TRUE(preview->pause_after_cutover);
        expect_page_requests(zone, "Fold/Test", right, {lower, fold_at - 1}, "f1");
        EXPECT_TRUE(zone.legacy_requests.empty());
    }
}

TEST(ScheduledPauseForecast, FirstPageSizeAndZoneChecksPrecedeContinuity) {
    struct Case {
        std::string_view zone;
        Instant from;
        std::size_t count;
        ZoneError cause;
    };
    const auto horizon = after(cut_time, days{1});
    const auto right = after(horizon, 48h);
    const std::array cases{Case{"Other/Zero", lower - 1, 65, ZoneError::limit_exceeded},
                           Case{"Other/Zero", lower - 1, 0, ZoneError::insufficient_coverage},
                           Case{"Page/Zero", lower + 1, 0, ZoneError::insufficient_coverage},
                           Case{"Page/Zero", lower - 1, 0, ZoneError::unsupported_rules}};
    for (const auto &test : cases) {
        SCOPED_TRACE(static_cast<int>(test.cause));
        PagedZone zone;
        zone.replies.emplace_back(
            ForecastPage{facts(std::string{test.zone}, "z1", test.from, right, 0,
                               alternating(after(cut_time, 1min), 1min, test.count, 1)),
                         ReachedRequestEnd{}});
        EnteredBasis basis(zone);
        const auto proposed = configuration("Page/Zero", {rule("daily", 12h, 1ms)});
        const auto projected = basis.preview(zone, proposed, horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, horizon, cut_time, coverage(test.cause));
        EXPECT_TRUE(preview->upcoming.empty());
        EXPECT_TRUE(preview->facts.empty());
        expect_page_requests(zone, "Page/Zero", right, {lower}, "z1");
        EXPECT_TRUE(zone.legacy_requests.empty());
    }
}

TEST(ScheduledPauseForecast, MissingContinuationRetainsOnlyCoherentPrefixAndTypedCause) {
    for (const auto error : {ZoneError::insufficient_coverage, ZoneError::source_unavailable}) {
        SCOPED_TRACE(static_cast<int>(error));
        PagedZone zone;
        const auto right = after(fold_at, 48h);
        fold_zone(zone, right);
        if (error == ZoneError::insufficient_coverage) {
            zone.replies[1] = ForecastPage{facts("Fold/Test", "f1", fold_at - 1, fold_at - 1, 60),
                                           TerminalPrefix{error}};
        } else {
            zone.replies[1] = error;
        }
        EnteredBasis basis(zone);
        const auto config = configuration("Fold/Test", {rule("fold", 13h + 30s, 30min)});
        const auto projected = basis.preview(zone, config, fold_at);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, fold_at, cut_time, coverage(error));
        EXPECT_TRUE(preview->upcoming.empty());
        // A terminal point or a bare error leaves only the prior accepted prefix.
        ASSERT_EQ(preview->facts.size(), 1U);
        expect_fact(preview->facts[0], page_facts(zone.replies[0]));
        expect_page_requests(zone, "Fold/Test", right, {lower, fold_at - 1}, "f1");
    }
}

TEST(ScheduledPauseForecast, JoinedViewRejectsAThirdInverseAcrossTwoIndividuallyValidPages) {
    PagedZone zone;
    const auto horizon = after(fold_at, 70s);
    const auto right = after(horizon, 48h);
    const auto early = alternating(after(lower, 1min), 1min, 64, 119, 120);
    const std::vector<ScheduleZoneTransition> late{{fold_at, 60}, {after(fold_at, 30s), 0}};
    auto whole = early;
    whole.insert(whole.end(), late.begin(), late.end());
    zone.views.push_back(facts("Fold/Test", "f3", lower - 1, right, 120, whole));
    zone.replies = {
        ForecastPage{facts("Fold/Test", "f3", lower, fold_at - 1, 120, early), TransitionCap{}},
        ForecastPage{facts("Fold/Test", "f3", fold_at - 1, right, 120, late), ReachedRequestEnd{}}};
    EnteredBasis basis(zone);
    const auto config = configuration(
        "Fold/Test", {rule("triple", 13h + 70s, 30min, ScheduleFoldPolicy::RunTwice)});
    const auto projected = basis.preview(zone, config, horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    // The three inverses of B+70s are B-50s, B+10s and B+70s.
    expect_forecast(*preview, cut_time, horizon, cut_time, coverage(ZoneError::unsupported_rules));
    EXPECT_TRUE(preview->upcoming.empty());
    EXPECT_TRUE(preview->facts.empty());
    expect_page_requests(zone, "Fold/Test", right, {lower, fold_at - 1}, "f3");
    EXPECT_TRUE(zone.legacy_requests.empty());
}

TEST(ScheduledPauseForecast, BoundRevisionMustAgreeThroughoutOverlapBeforeCapClassification) {
    const std::string view = "Overlap/Zero";
    const std::string revision = "same-r1";
    const auto horizon = after(cut_time, days{1});
    const auto right = after(horizon, 48h);
    const auto early = alternating(after(lower, 1min), 1min, 64, 1);
    for (const bool cap : {false, true}) {
        SCOPED_TRACE(cap);
        PagedZone zone;
        zone.views.push_back(facts(view, revision, lower - 2, right, 0, early));
        const auto old = configuration(view, {rule("old", 15h, 1ms)});
        EnteredBasis basis(zone, cut_time, old);
        zone.legacy_requests.clear();
        if (cap) {
            // The old view already covers T+1 at offset zero, so this same-revision
            // cap cannot prove a next actual change.
            const auto edge = after(lower, 2h);
            zone.replies = {
                ForecastPage{facts(view, revision, lower, edge, 0, early), TransitionCap{}}};
        } else {
            auto conflicting = early;
            conflicting.push_back({cut_time, 1});
            // Match both endpoint offsets while contradicting an interior ms.
            // Replace two early rows so the count remains exactly 64.
            conflicting.push_back({cut_time + 1, 0});
            conflicting.erase(conflicting.begin(), conflicting.begin() + 2);
            zone.replies = {ForecastPage{facts(view, revision, lower, right, 0, conflicting),
                                         ReachedRequestEnd{}}};
        }
        const auto proposed = configuration(view, {rule("new", 12h, 1ms)});
        const auto projected = basis.preview(zone, proposed, horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, horizon, cut_time,
                        coverage(ZoneError::unsupported_rules));
        EXPECT_TRUE(preview->upcoming.empty());
        ASSERT_TRUE(preview->pause_after_cutover);
        EXPECT_TRUE(preview->pause_after_cutover->reasons.empty());
        ASSERT_FALSE(zone.legacy_requests.empty());
        ASSERT_FALSE(preview->facts.empty());
        for (const auto &binding : preview->facts) {
            EXPECT_EQ(binding.old_epoch_index, 0U);
            EXPECT_EQ(binding.facts.rules_revision, revision);
        }
        expect_page_requests(zone, view, right, {lower}, revision, revision);
    }
}

TEST(ScheduledPauseForecast, UncertifiedOldEffectsWinOverAValidComplexityCertificate) {
    PagedZone zone;
    dense_two(zone, FinalPage::attested_cap);
    zone.views.push_back(facts("Old/Zero", "old-r1", lower - 2, after(cut_time, 72h)));
    EnteredBasis basis(zone, cut_time, configuration("Old/Zero", {rule("old", 15h, 1ms)}));
    zone.legacy_requests.clear();
    zone.scripted = ZoneError::source_unavailable;
    const auto projected = basis.preview(zone, long_rule("Dense/Two"), long_horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    expect_forecast(*preview, cut_time, long_horizon, cut_time,
                    coverage(ZoneError::source_unavailable));
    EXPECT_FALSE(preview->pause_after_cutover);
    EXPECT_TRUE(preview->upcoming.empty());
    ASSERT_FALSE(zone.legacy_requests.empty());
    expect_dense_reads(zone, "r2");
    ASSERT_EQ(preview->facts.size(), 64U);
    EXPECT_EQ(preview->facts.back().facts.through_utc_ms, unsupported_change - 1);
}

TEST(ScheduledPauseForecast, LegacyBridgeClipsOnlyToTheExactClosedRequest) {
    const auto horizon = after(cut_time, days{1});
    const auto right = after(horizon, 48h);
    const auto proposed = configuration("Bridge/Zero", {rule("bridge", 12h, 1ms)});
    for (const bool short_reply : {false, true}) {
        SCOPED_TRACE(short_reply);
        LegacyZone zone;
        EnteredBasis basis(zone);
        zone.scripted = short_reply ? facts("Bridge/Zero", "b1", lower, right - 1)
                                    : facts("Bridge/Zero", "b1", lower - 1, right + 1, 60,
                                            {{lower, 0}, {right + 1, 60}});
        const auto projected = basis.preview(zone, proposed, horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, horizon, short_reply ? horizon - 1 : horizon,
                        short_reply ? std::optional{coverage(ZoneError::insufficient_coverage)}
                                    : std::nullopt);
        ASSERT_EQ(zone.legacy_requests.size(), 1U);
        EXPECT_EQ(zone.legacy_requests.front().from_utc_ms, lower);
        EXPECT_EQ(zone.legacy_requests.front().through_utc_ms, right);
        EXPECT_FALSE(zone.legacy_requests.front().required_rules_revision);
        ASSERT_EQ(preview->facts.size(), 1U);
        expect_fact(preview->facts[0],
                    facts("Bridge/Zero", "b1", lower, short_reply ? right - 1 : right));
        ASSERT_EQ(preview->upcoming.size(), short_reply ? 0U : 1U);
        if (!short_reply) {
            expect_row(preview->upcoming.front(), key("bridge", horizon), horizon, horizon + 1,
                       horizon, horizon + 1);
        }
    }
}

TEST(ScheduledPauseForecast, OverCapLegacyReplyUsesOneReadAndNoHalving) {
    LegacyZone zone;
    const auto horizon = after(cut_time, days{4});
    zone.views.push_back(facts("Bridge/Zero", "b1", lower - 1, after(horizon, 48h), 0,
                               alternating(after(cut_time, days{5}), 1min, 65, 1)));
    EnteredBasis basis(zone);
    const auto projected =
        basis.preview(zone, configuration("Bridge/Zero", {rule("bridge", 12h, 1ms)}), horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    expect_forecast(*preview, cut_time, horizon, cut_time, coverage(ZoneError::limit_exceeded));
    EXPECT_TRUE(preview->upcoming.empty());
    EXPECT_TRUE(preview->facts.empty());
    ASSERT_EQ(zone.legacy_requests.size(), 1U);
    EXPECT_EQ(zone.legacy_requests.front().from_utc_ms, lower);
    EXPECT_EQ(zone.legacy_requests.front().through_utc_ms, after(horizon, 48h));
}

TEST(ScheduledPauseForecast, RowLimitAdmitsEqualityAndKeepsTheNextTieGroupAtomic) {
    LegacyZone zone;
    zone.views.push_back(facts("Fixed/Zero", "z1", lower - 1, after(cut_time, days{131})));
    EnteredBasis basis(zone);
    auto proposed = configuration("Fixed/Zero");
    // Descending IDs distinguish proposal order from lexical order.
    for (int number = 128; number != 0; --number) {
        proposed.rules.push_back(rule("rule-" + std::to_string(number), 12h, 1ms));
    }
    for (const auto days_after : {128, 129}) {
        SCOPED_TRACE(days_after);
        zone.legacy_requests.clear();
        const auto horizon = after(cut_time, days{days_after});
        const auto projected = basis.preview(zone, proposed, horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, horizon, horizon - (days_after == 129),
                        days_after == 129 ? std::optional{coverage(ZoneError::limit_exceeded)}
                                          : std::nullopt);
        ASSERT_EQ(preview->upcoming.size(), 16'384U);
        for (std::size_t i = 0; i < preview->upcoming.size(); ++i) {
            const auto nominal = after(cut_time, days{1 + i / 128});
            const auto id = "rule-" + std::to_string(128 - i % 128);
            expect_row(preview->upcoming[i], key(id, nominal), nominal, nominal + 1, nominal,
                       nominal + 1);
        }
        ASSERT_EQ(zone.legacy_requests.size(), 1U);
        EXPECT_EQ(zone.legacy_requests.front().through_utc_ms, after(horizon, 48h));
    }
}

TEST(ScheduledPauseForecast, Exact366DayHorizonAndPreflightRefusalOrder) {
    LegacyZone zone;
    const auto horizon = after(cut_time, days{366});
    zone.views.push_back(facts("Fixed/Zero", "z1", lower - 1, after(horizon, 48h)));
    EnteredBasis basis(zone);
    const auto proposed = configuration("Fixed/Zero", {rule("daily", 12h, 1ms)});
    const auto projected = basis.preview(zone, proposed, horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    expect_forecast(*preview, cut_time, horizon, horizon);
    ASSERT_EQ(preview->upcoming.size(), 366U);
    expect_row(preview->upcoming.back(), key("daily", horizon), horizon, horizon + 1, horizon,
               horizon + 1);
    zone.legacy_requests.clear();
    EXPECT_EQ(std::get<ScheduledPauseRefusal>(basis.preview(zone, proposed, horizon + 1)),
              ScheduledPauseRefusal::limit_exceeded);
    EXPECT_EQ(std::get<ScheduledPauseRefusal>(basis.preview(zone, proposed, cut_time - 1)),
              ScheduledPauseRefusal::invalid_input);
    const std::array cuts{elapsed_cut(cut_time, 1ms)};
    auto stale = basis.clock(cuts);
    stale.source_snapshot_ref = "another-snapshot";
    const auto preflight =
        project_scheduled_pause_edit(basis.state(), stale, proposed, horizon + 1, zone);
    EXPECT_EQ(std::get<ScheduledPauseRefusal>(preflight), ScheduledPauseRefusal::stale_observation);
    auto oversized = proposed;
    oversized.rules.resize(129, proposed.rules.front());
    const auto limit =
        project_scheduled_pause_edit(basis.state(), stale, oversized, cut_time - 1, zone);
    EXPECT_EQ(std::get<ScheduledPauseRefusal>(limit), ScheduledPauseRefusal::limit_exceeded);
    EXPECT_TRUE(zone.legacy_requests.empty());
}

TEST(ScheduledPauseForecast, EmptyFutureAndNoWeekdaysNeedNoForecastPage) {
    PagedZone zone;
    EnteredBasis basis(zone);
    auto no_weekdays = rule("none", 12h, 1ms);
    no_weekdays.recurrence.weekdays.fill(false);
    auto disabled = rule("disabled", 12h, 1ms);
    disabled.enabled = false;
    for (const bool at_cut : {false, true}) {
        SCOPED_TRACE(at_cut);
        const auto horizon = at_cut ? cut_time : after(cut_time, days{1});
        const auto proposal = at_cut ? configuration("Never/Read", {rule("active", 12h, 1ms)})
                                     : configuration("Never/Read", {no_weekdays, disabled});
        const auto projected = basis.preview(zone, proposal, horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cut_time, horizon, horizon);
        EXPECT_TRUE(preview->facts.empty());
        EXPECT_TRUE(preview->upcoming.empty());
        EXPECT_TRUE(zone.legacy_requests.empty());
        EXPECT_TRUE(zone.page_requests.empty());
    }
}

TEST(ScheduledPauseForecast, OrdinaryGapAndSkipRetainTheirExistingMeanings) {
    constexpr auto march29 = 2026y / March / 29;
    constexpr auto at = utc(march29, 30min);
    constexpr auto horizon = utc(march29, 1h + 20min);
    for (const auto policy : {ScheduleGapPolicy::ShiftForward, ScheduleGapPolicy::Skip}) {
        SCOPED_TRACE(static_cast<int>(policy));
        LegacyZone zone;
        zone.views.push_back(facts("Gap/Test", "g1", utc(2026y / March / 26),
                                   utc(2026y / April / 2), 3600, {{utc(march29, 1h), 7200}}));
        EnteredBasis basis(zone, at);
        auto daily = rule("gap", 2h + 20min, 45min);
        daily.recurrence.gap_policy = policy;
        const auto projected = basis.preview(zone, configuration("Gap/Test", {daily}), horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, at, horizon, horizon);
        ASSERT_EQ(preview->upcoming.size(), policy == ScheduleGapPolicy::Skip ? 0U : 1U);
        if (policy == ScheduleGapPolicy::ShiftForward) {
            expect_row(preview->upcoming.front(), key("gap", utc(march29, 2h + 20min)), horizon,
                       utc(march29, 2h + 5min), utc(march29, 3h + 20min), utc(march29, 4h + 5min));
        }
    }
}

TEST(ScheduledPauseForecast, LocalEndCoverageDoesNotChangeLegacyProgression) {
    constexpr auto at = utc(2026y / January / 6);
    constexpr auto full = utc(2026y / January / 7, 12h);
    constexpr auto short_end = utc(2026y / January / 10, 12h);
    LegacyZone zone;
    zone.views.push_back(facts("Fixed/Zero", "z1", utc(2026y / January / 1), short_end));
    EnteredBasis basis(zone, at);
    auto weekly = rule("weekly", 12h, 72h);
    weekly.recurrence.weekdays = wednesday;
    for (const auto horizon : {full, cut_time}) {
        SCOPED_TRACE(horizon);
        const auto projected = basis.preview(zone, configuration("Fixed/Zero", {weekly}), horizon);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, at, horizon, full,
                        horizon == full
                            ? std::nullopt
                            : std::optional{coverage(ZoneError::insufficient_coverage)});
        ASSERT_EQ(preview->upcoming.size(), 1U);
        expect_row(preview->upcoming.front(), key("weekly", full), full, short_end, full,
                   short_end);
    }

    weekly.recurrence.weekdays = thursday;
    EnteredBasis active(zone, cut_time, configuration("Fixed/Zero", {weekly}));
    const std::array cuts{elapsed_cut(cut_time, 1ms)};
    const auto projected = progress_scheduled_pauses(active.state(), active.clock(cuts), zone);
    const auto *progress = std::get_if<ScheduledPauseProgress>(&projected);
    ASSERT_NE(progress, nullptr);
    EXPECT_FALSE(progress->cursor.hold);
    EXPECT_EQ(progress->timer.main_remaining_ms, 899'999);
    ASSERT_EQ(progress->new_deliveries.size(), 1U);
    EXPECT_EQ(progress->new_deliveries.front().key, key("weekly", cut_time));
    EXPECT_EQ(progress->new_deliveries.front().window.start_utc_ms, cut_time);
    EXPECT_EQ(progress->new_deliveries.front().window.end_utc_ms, after(cut_time, 72h));
}

TEST(ScheduledPauseForecast, OldStartAtCutIsDeliveredThenDisabledWithoutForecastReads) {
    PagedZone zone;
    zone.views.push_back(facts("Fixed/Zero", "z1", after(lower, -1h), after(cut_time, 72h)));
    const auto old = configuration("Fixed/Zero", {rule("due", 12h, 20min)});
    EnteredBasis basis(zone, cut_time, old);
    auto proposed = old;
    proposed.rules.front().enabled = false;
    const auto horizon = after(cut_time, days{1});
    const auto projected = basis.preview(zone, proposed, horizon);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    expect_forecast(*preview, cut_time, horizon, horizon);
    EXPECT_TRUE(preview->upcoming.empty());
    EXPECT_TRUE(zone.page_requests.empty());
    ASSERT_TRUE(preview->pause_after_cutover);
    EXPECT_TRUE(preview->pause_after_cutover->reasons.empty());
    ASSERT_EQ(preview->cutover_effects.size(), 1U);
    const auto &effect = preview->cutover_effects.front();
    EXPECT_EQ(effect.delivery.key, key("due", cut_time));
    EXPECT_EQ(effect.delivery.window.start_utc_ms, cut_time);
    EXPECT_EQ(effect.delivery.window.end_utc_ms, after(cut_time, 20min));
    ASSERT_TRUE(effect.stop_at_cutover);
    EXPECT_EQ(effect.stop_at_cutover->cause, ScheduledPauseStopCause::disabled);
    EXPECT_EQ(effect.stop_at_cutover->effective_utc_ms, cut_time);
    expect_observation(effect.stop_at_cutover->reconciled_at, cut_time);

    const std::array cuts{elapsed_cut(cut_time, 1ms)};
    const auto cutover =
        project_scheduled_pause_cutover(basis.state(), basis.clock(cuts), proposed, zone);
    const auto *accepted = std::get_if<ScheduledPauseCutover>(&cutover);
    ASSERT_NE(accepted, nullptr);
    EXPECT_EQ(accepted->progress.timer.main_remaining_ms, 899'999);
    ASSERT_EQ(accepted->progress.new_deliveries.size(), 1U);
    EXPECT_EQ(accepted->progress.new_deliveries.front().key, effect.delivery.key);
    ASSERT_EQ(accepted->progress.stops.size(), 1U);
    EXPECT_EQ(accepted->progress.stops.front().key, effect.delivery.key);
    EXPECT_EQ(accepted->progress.stops.front().stop.cause, ScheduledPauseStopCause::disabled);
    EXPECT_TRUE(accepted->progress.pause.reasons.empty());
    EXPECT_TRUE(accepted->progress.timer_transitions.empty());
    EXPECT_TRUE(zone.page_requests.empty());
}
} // namespace
