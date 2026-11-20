// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <subathon/server/scheduled_pause.hpp>

namespace {
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace subathon::core;
using namespace subathon::server;

constexpr auto jan8 = 2026y / January / 8;
constexpr auto march29 = 2026y / March / 29;
constexpr auto oct25 = 2026y / October / 25;

constexpr std::int64_t utc(year_month_day day, milliseconds time = 0ms) {
    return duration_cast<milliseconds>(sys_days{day}.time_since_epoch()).count() + time.count();
}

constexpr std::int64_t jan(milliseconds time) { return utc(jan8, time); }
constexpr std::int64_t noon = jan(12h);

constexpr ScheduleLocalTime local(year_month_day day, milliseconds time) {
    return local_days{day} + time;
}

constexpr std::array sunday{false, false, false, false, false, false, true};
constexpr std::array sunday_monday{true, false, false, false, false, false, true};
constexpr std::array wednesday{false, false, true, false, false, false, false};

ScheduledPauseRule rule(std::string id, milliseconds time, milliseconds duration) {
    return {std::move(id), {.local_time = time}, duration, true};
}

ScheduledPauseRule sunday_rule(milliseconds time, milliseconds duration) {
    auto result = rule("S", time, duration);
    result.recurrence.weekdays = sunday;
    return result;
}

ScheduledPauseConfiguration config(std::initializer_list<ScheduledPauseRule> rules,
                                   std::string zone = "Fixed/Zero") {
    return {std::move(zone), rules};
}

ScheduleZoneFacts fixed(std::int64_t end = utc(2026y / January / 12), std::int32_t offset = 0,
                        std::string zone = "Fixed/Zero", std::string revision = "fixed-r1") {
    return {std::move(zone), std::move(revision), utc(2026y / January / 1), end, offset, {}};
}

ScheduleZoneFacts march(std::int32_t initial,
                        std::initializer_list<ScheduleZoneTransition> transitions) {
    return {"Season/March",         "march-r1", utc(2026y / March / 26),
            utc(2026y / April / 2), initial,    transitions};
}

ScheduleZoneFacts october(std::string zone = "Season/October") {
    return {std::move(zone),           "oct-r1", utc(2026y / October / 22),
            utc(2026y / October / 29), 7'200,    {{utc(oct25, 1h), 3'600}}};
}

class SyntheticZones final : public ScheduleZoneSource {
  public:
    std::vector<ScheduleZoneFacts> views;
    std::optional<ScheduleZoneFacts> incoherent_reply;
    bool ignore_required_revision{};
    ScheduleZoneUnavailable unavailable{ScheduleZoneUnavailable::insufficient_coverage};
    std::size_t reads{};

    std::variant<ScheduleZoneFacts, ScheduleZoneUnavailable>
    read(const ScheduleZoneRequest &request) override {
        ++reads;
        if (incoherent_reply) {
            return *incoherent_reply;
        }
        // Return a closed prefix clipped to the requested lower bound. A
        // transition at that bound is represented by the initial offset.
        for (const auto &view : views) {
            if (view.configured_zone != request.configured_zone ||
                (!ignore_required_revision && request.required_rules_revision &&
                 *request.required_rules_revision != view.rules_revision) ||
                request.from_utc_ms < view.from_utc_ms ||
                request.from_utc_ms > view.through_utc_ms) {
                continue;
            }
            ScheduleZoneFacts result{
                view.configured_zone,         view.rules_revision,
                request.from_utc_ms,          std::min(request.through_utc_ms, view.through_utc_ms),
                view.offset_at_start_seconds, {}};
            for (const auto &transition : view.transitions) {
                if (transition.at_utc_ms <= result.from_utc_ms) {
                    result.offset_at_start_seconds = transition.offset_after_seconds;
                } else if (transition.at_utc_ms <= result.through_utc_ms) {
                    result.transitions.push_back(transition);
                }
            }
            return result;
        }
        return unavailable;
    }
};

ScheduledPauseObservation observation(std::int64_t when, std::string ref = {}) {
    if (ref.empty()) {
        ref = "wall-" + std::to_string(when);
    }
    return {std::move(ref), when};
}

ScheduledPauseElapsedCut cut(std::int64_t when, std::optional<milliseconds> elapsed,
                             PauseState independent = {}) {
    return {observation(when), elapsed, std::move(independent)};
}

const PauseState manual{.reasons = {PauseReason::Manual}};
const PauseState schedule{.reasons = {PauseReason::Schedule}};
const PauseState manual_schedule{.reasons = {PauseReason::Manual, PauseReason::Schedule}};

ScheduledPauseKey key(std::string id, year_month_day day, milliseconds time,
                      ScheduleFoldRun run = ScheduleFoldRun::First) {
    return {"L", std::move(id), {local(day, time), run}};
}

DeliveredScheduledPause delivery(ScheduledPauseKey identity, std::int64_t start, std::int64_t end,
                                 std::optional<ScheduledPauseStop> stop = {}) {
    return {std::move(identity), {start, end}, std::move(stop)};
}

ScheduledPauseStop stopped(ScheduledPauseStopCause cause, std::int64_t effective,
                           ScheduledPauseObservation reconciled) {
    return {cause, effective, std::move(reconciled)};
}

class Basis {
  public:
    Basis(std::int64_t at, ScheduledPauseConfiguration current,
          TimerLifecycle lifecycle = TimerLifecycle::Active)
        : epochs{{std::move(current), {}, {}, {}}},
          state{.snapshot_ref = "snapshot-1",
                .timer_lineage = "L",
                .clock_lineage = "clock-1",
                .state_revision = "state-1",
                .configuration_revision = "config-1",
                .timer_config = {.end_mode = TimerEndMode::Immediate, .hard_end_utc_ms = {}},
                .timer = {.lifecycle = lifecycle, .main_remaining_ms = 900'000},
                .pause = {},
                .cursor = {observation(at),
                           lifecycle == TimerLifecycle::Active ? std::optional{at} : std::nullopt,
                           {}},
                .epochs = {},
                .epoch_history_complete = true,
                .ledger = nullptr} {}

    Basis(const Basis &) = delete;
    Basis &operator=(const Basis &) = delete;

    std::vector<ScheduledPauseEpoch> epochs;
    ScheduledPauseLedger ledger{"L", "delivery-1", {}};
    ScheduledPauseState state;
    bool ledger_known = true;

    const ScheduledPauseState &input() {
        state.epochs = epochs;
        state.ledger = ledger_known ? &ledger : nullptr;
        return state;
    }

    auto clock(std::initializer_list<ScheduledPauseElapsedCut> cuts) const {
        return ScheduledPauseClockInput{state.snapshot_ref,
                                        state.clock_lineage,
                                        state.cursor.last_observation,
                                        {cuts.begin(), cuts.size()}};
    }

    auto preview(SyntheticZones &zone, const ScheduledPauseConfiguration &proposed,
                 std::initializer_list<ScheduledPauseElapsedCut> cuts, std::int64_t through) {
        return project_scheduled_pause_edit(input(), clock(cuts), proposed, through, zone);
    }

    auto edit(SyntheticZones &zone, const ScheduledPauseConfiguration &proposed,
              std::initializer_list<ScheduledPauseElapsedCut> cuts) {
        return project_scheduled_pause_cutover(input(), clock(cuts), proposed, zone);
    }

    auto progress(SyntheticZones &zone, std::initializer_list<ScheduledPauseElapsedCut> cuts) {
        return progress_scheduled_pauses(input(), clock(cuts), zone);
    }

    ScheduledPauseEntry entry_input(std::int64_t when) const {
        return {state.snapshot_ref,
                observation(when),
                {.lifecycle = TimerLifecycle::Active, .main_remaining_ms = 900'000}};
    }

    auto enter(SyntheticZones &zone, std::int64_t when) {
        return enter_scheduled_pauses(input(), entry_input(when), zone);
    }
};

void expect_observation(const ScheduledPauseObservation &actual,
                        const ScheduledPauseObservation &expected) {
    EXPECT_EQ(actual.utc_ms, expected.utc_ms);
    EXPECT_EQ(actual.observation_ref, expected.observation_ref);
}

void expect_stop(const ScheduledPauseStop &actual, const ScheduledPauseStop &expected) {
    EXPECT_EQ(actual.cause, expected.cause);
    EXPECT_EQ(actual.effective_utc_ms, expected.effective_utc_ms);
    expect_observation(actual.reconciled_at, expected.reconciled_at);
}

void expect_delivery(const DeliveredScheduledPause &actual,
                     const DeliveredScheduledPause &expected) {
    EXPECT_EQ(actual.key, expected.key);
    EXPECT_EQ(actual.window.start_utc_ms, expected.window.start_utc_ms);
    EXPECT_EQ(actual.window.end_utc_ms, expected.window.end_utc_ms);
    ASSERT_EQ(actual.stop.has_value(), expected.stop.has_value());
    if (expected.stop) {
        expect_stop(*actual.stop, *expected.stop);
    }
}

void expect_single_delivery(const ScheduledPauseProgress &result,
                            const DeliveredScheduledPause &expected) {
    ASSERT_EQ(result.new_deliveries.size(), 1U);
    expect_delivery(result.new_deliveries.front(), expected);
}

void expect_single_stop(const ScheduledPauseProgress &result, const ScheduledPauseKey &key,
                        const ScheduledPauseStop &expected) {
    ASSERT_EQ(result.stops.size(), 1U);
    EXPECT_EQ(result.stops.front().key, key);
    expect_stop(result.stops.front().stop, expected);
}

void expect_row(const ScheduledPausePreviewRow &row, const ScheduledPauseKey &identity,
                std::int64_t start, std::int64_t end, ScheduleLocalTime local_start,
                ScheduleLocalTime local_end) {
    EXPECT_EQ(row.key, identity);
    EXPECT_EQ(row.window, (ScheduledPauseOccurrence{
                              {identity.occurrence, start, local_start}, end, local_end}));
}

void expect_hold(const ScheduledPauseProgress &result, std::int64_t from,
                 const ScheduledPauseObservation &seen,
                 std::optional<ScheduledPauseCoverageFailure> cause = {}) {
    ASSERT_TRUE(result.cursor.hold);
    ASSERT_TRUE(result.notice);
    EXPECT_EQ(result.cursor.hold->from_utc_ms, from);
    EXPECT_EQ(result.notice->hold.from_utc_ms, from);
    EXPECT_EQ(result.notice->hold.cause, result.cursor.hold->cause);
    if (cause) {
        EXPECT_EQ(result.cursor.hold->cause, *cause);
    }
    expect_observation(result.notice->observed_at, seen);
    expect_observation(result.cursor.last_observation, seen);
}

void expect_clear(const ScheduledPauseProgress &result) {
    EXPECT_FALSE(result.cursor.hold);
    EXPECT_FALSE(result.notice);
}

void expect_finish(const ScheduledPauseProgress &result, std::int64_t main, std::int64_t at,
                   TimerTransition first = TimerTransition::HardEndReached) {
    EXPECT_EQ(result.timer.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(result.timer.main_remaining_ms, main);
    ASSERT_EQ(result.timer_transitions.size(), 1U);
    expect_observation(result.timer_transitions.front().observation, observation(at));
    EXPECT_EQ(result.timer_transitions.front().transitions,
              (std::vector{first, TimerTransition::Finished}));
}

template <typename Result>
void expect_refusal(const Result &result, ScheduledPauseRefusal expected) {
    const auto *refusal = std::get_if<ScheduledPauseRefusal>(&result);
    ASSERT_NE(refusal, nullptr);
    EXPECT_EQ(*refusal, expected);
}

void expect_complete(const ScheduledPausePreview &result, std::int64_t through) {
    EXPECT_FALSE(result.incomplete);
    EXPECT_EQ(result.resolved_through_utc_ms, through);
    ASSERT_TRUE(result.pause_after_cutover);
}

// Calendar and delivery examples, in frozen PR table order.
TEST(ScheduledPauseAcceptance, GapForwardAndSkip) {
    SyntheticZones zones;
    zones.views = {march(3'600, {{utc(march29, 1h), 7'200}})};
    const auto start = utc(march29, 1h + 20min);
    const auto end = utc(march29, 2h);
    const auto identity = key("S", march29, 2h + 20min);
    auto candidate = sunday_rule(2h + 20min, 40min);
    for (const auto policy : {ScheduleGapPolicy::ShiftForward, ScheduleGapPolicy::Skip}) {
        SCOPED_TRACE(static_cast<int>(policy));
        candidate.recurrence.gap_policy = policy;
        const auto proposed = config({candidate}, "Season/March");
        Basis basis(utc(march29), proposed);
        basis.state.timer.main_remaining_ms = 5'400'000;
        const auto preview = std::get<ScheduledPausePreview>(
            basis.preview(zones, proposed, {cut(utc(march29, 30min), 30min)}, end));
        expect_complete(preview, end);
        const auto progress =
            std::get<ScheduledPauseProgress>(basis.progress(zones, {cut(start, 80min)}));
        if (policy == ScheduleGapPolicy::Skip) {
            EXPECT_TRUE(preview.upcoming.empty());
            EXPECT_TRUE(progress.new_deliveries.empty());
            EXPECT_TRUE(progress.pause.reasons.empty());
        } else {
            ASSERT_EQ(preview.upcoming.size(), 1U);
            expect_row(preview.upcoming.front(), identity, start, end, local(march29, 3h + 20min),
                       local(march29, 4h));
            expect_single_delivery(progress, delivery(identity, start, end));
            EXPECT_EQ(progress.pause.reasons, schedule.reasons);
        }
    }
}

TEST(ScheduledPauseAcceptance, ElapsedDurationCrossesGap) {
    SyntheticZones zones;
    zones.views = {march(3'600, {{utc(march29, 1h), 7'200}})};
    const auto proposed = config({sunday_rule(1h + 50min, 30min)}, "Season/March");
    Basis basis(utc(march29), proposed, TimerLifecycle::Ready);
    const auto preview = std::get<ScheduledPausePreview>(
        basis.preview(zones, proposed, {cut(utc(march29, 30min), std::nullopt)}, utc(march29, 1h)));
    expect_complete(preview, utc(march29, 1h));
    ASSERT_EQ(preview.upcoming.size(), 1U);
    expect_row(preview.upcoming.front(), key("S", march29, 1h + 50min), utc(march29, 50min),
               utc(march29, 1h + 20min), local(march29, 1h + 50min), local(march29, 3h + 20min));
}

TEST(ScheduledPauseAcceptance, NonhourGapKeepsNominalMilliseconds) {
    SyntheticZones zones;
    zones.views = {march(3'600, {{utc(march29, 1h), 5'400}})};
    const auto proposed = config({sunday_rule(2h + 10min + 125ms, 10min)}, "Season/March");
    Basis basis(utc(march29), proposed, TimerLifecycle::Ready);
    const auto preview = std::get<ScheduledPausePreview>(
        basis.preview(zones, proposed, {cut(utc(march29, 1h), std::nullopt)}, utc(march29, 2h)));
    expect_complete(preview, utc(march29, 2h));
    ASSERT_EQ(preview.upcoming.size(), 1U);
    expect_row(preview.upcoming.front(), key("S", march29, 2h + 10min + 125ms),
               utc(march29, 1h + 10min + 125ms), utc(march29, 1h + 20min + 125ms),
               local(march29, 2h + 40min + 125ms), local(march29, 2h + 50min + 125ms));
}

TEST(ScheduledPauseAcceptance, FoldOnceAndTwiceKeepAssignedRunAcrossPreviewRanges) {
    SyntheticZones zones;
    zones.views = {october()};
    auto candidate = sunday_rule(2h + 40min, 40min);
    const auto nominal = local(oct25, 2h + 40min);
    const auto first = key("S", oct25, 2h + 40min);
    const auto second = key("S", oct25, 2h + 40min, ScheduleFoldRun::Second);
    for (const auto policy : {ScheduleFoldPolicy::RunOnce, ScheduleFoldPolicy::RunTwice}) {
        SCOPED_TRACE(static_cast<int>(policy));
        candidate.recurrence.fold_policy = policy;
        const auto proposed = config({candidate}, "Season/October");
        Basis basis(utc(oct25), proposed, TimerLifecycle::Ready);
        const auto full = std::get<ScheduledPausePreview>(
            basis.preview(zones, proposed, {cut(utc(oct25, 20min), std::nullopt)}, utc(oct25, 2h)));
        const auto late = std::get<ScheduledPausePreview>(
            basis.preview(zones, proposed, {cut(utc(oct25, 1h), std::nullopt)}, utc(oct25, 2h)));
        expect_complete(full, utc(oct25, 2h));
        expect_complete(late, utc(oct25, 2h));
        const bool twice = policy == ScheduleFoldPolicy::RunTwice;
        ASSERT_EQ(full.upcoming.size(), twice ? 2U : 1U);
        ASSERT_EQ(late.upcoming.size(), twice ? 1U : 0U);
        expect_row(full.upcoming.front(), first, utc(oct25, 40min), utc(oct25, 1h + 20min), nominal,
                   local(oct25, 2h + 20min));
        if (twice) {
            for (const auto *row : {&full.upcoming[1], &late.upcoming.front()}) {
                expect_row(*row, second, utc(oct25, 1h + 40min), utc(oct25, 2h + 20min), nominal,
                           local(oct25, 3h + 20min));
            }
        }
    }
}

TEST(ScheduledPauseAcceptance, CompoundGapUsesLastCrossingAndProjectsActualOffsets) {
    SyntheticZones zones;
    zones.views = {march(0, {{utc(march29, 1h), 7'200},
                             {utc(march29, 1h + 15min), -1'800},
                             {utc(march29, 1h + 20min), 3'600},
                             {utc(march29, 1h + 45min), 5'400}})};
    auto candidate = sunday_rule(1h + 30min, 30min);
    for (const auto policy : {ScheduleGapPolicy::ShiftForward, ScheduleGapPolicy::Skip}) {
        SCOPED_TRACE(static_cast<int>(policy));
        candidate.recurrence.gap_policy = policy;
        const auto proposed = config({candidate}, "Season/March");
        Basis basis(utc(march29), proposed, TimerLifecycle::Ready);
        const auto preview = std::get<ScheduledPausePreview>(basis.preview(
            zones, proposed, {cut(utc(march29, 1h), std::nullopt)}, utc(march29, 3h)));
        expect_complete(preview, utc(march29, 3h));
        if (policy == ScheduleGapPolicy::Skip) {
            EXPECT_TRUE(preview.upcoming.empty());
        } else {
            ASSERT_EQ(preview.upcoming.size(), 1U);
            // 01:30 - (-00:30) = 02:00Z, then +01:30 at both captured bounds.
            expect_row(preview.upcoming.front(), key("S", march29, 1h + 30min), utc(march29, 2h),
                       utc(march29, 2h + 30min), local(march29, 3h + 30min), local(march29, 4h));
        }
    }
}

TEST(ScheduledPauseAcceptance, ZoneEditPreservesActiveGapWindowAndSuppressesReplay) {
    const auto start = utc(march29, 1h + 20min);
    const auto change = utc(march29, 1h + 30min);
    const auto end = utc(march29, 2h);
    const auto original = delivery(key("S", march29, 2h + 20min), start, end);
    auto candidate = sunday_rule(2h + 20min, 40min);
    candidate.recurrence.weekdays = sunday_monday;
    const auto old = config({candidate}, "Season/March");
    const auto proposed = config({candidate}, "Fixed/West");
    SyntheticZones zones;
    zones.views = {march(3'600, {{utc(march29, 1h), 7'200}}), march(-7'200, {})};
    zones.views.back().configured_zone = "Fixed/West";
    zones.views.back().rules_revision = "west-r1";

    Basis before(start - 1, old);
    before.state.pause = manual;
    const auto delivered =
        std::get<ScheduledPauseProgress>(before.progress(zones, {cut(start, 1ms, manual)}));
    expect_single_delivery(delivered, original);

    Basis active(start, old);
    active.ledger.deliveries = {original};
    active.state.pause = manual_schedule;
    const auto monday = 2026y / March / 30;
    const auto horizon = utc(monday, 5h);
    const auto preview = std::get<ScheduledPausePreview>(
        active.preview(zones, proposed, {cut(change, 10min, manual)}, horizon));
    expect_complete(preview, horizon);
    ASSERT_EQ(preview.cutover_effects.size(), 1U);
    expect_delivery(preview.cutover_effects.front().delivery, original);
    EXPECT_FALSE(preview.cutover_effects.front().stop_at_cutover);
    ASSERT_EQ(preview.upcoming.size(), 1U);
    expect_row(preview.upcoming.front(), key("S", monday, 2h + 20min), utc(monday, 4h + 20min),
               utc(monday, 5h), local(monday, 2h + 20min), local(monday, 3h));
    const auto edit =
        std::get<ScheduledPauseCutover>(active.edit(zones, proposed, {cut(change, 10min, manual)}));
    EXPECT_EQ(edit.progress.next_known_expiry_utc_ms, end);
    EXPECT_EQ(edit.progress.pause.reasons, manual_schedule.reasons);
    EXPECT_TRUE(edit.progress.new_deliveries.empty());
    EXPECT_TRUE(edit.progress.stops.empty());

    Basis edited(change, proposed);
    edited.epochs = {{old, {}, change, "march-r1"}, {proposed, change, {}, {}}};
    edited.ledger.deliveries = {original};
    edited.state.pause = manual_schedule;
    SyntheticZones missing;
    const auto expired = std::get<ScheduledPauseProgress>(
        edited.progress(missing, {cut(end, std::nullopt, manual)}));
    expect_single_stop(expired, original.key,
                       stopped(ScheduledPauseStopCause::expired, end, observation(end)));
    EXPECT_EQ(expired.pause.reasons, manual.reasons);
    EXPECT_TRUE(expired.new_deliveries.empty());
    EXPECT_FALSE(expired.next_known_expiry_utc_ms);
}

TEST(ScheduledPauseAcceptance, ZoneEditFiltersPriorFirstAndPriorSecond) {
    auto candidate = sunday_rule(2h + 40min, 40min);
    candidate.recurrence.fold_policy = ScheduleFoldPolicy::RunTwice;
    const auto first = key("S", oct25, 2h + 40min);
    const auto second = key("S", oct25, 2h + 40min, ScheduleFoldRun::Second);
    const auto change = utc(oct25, 10min);
    const auto horizon = utc(oct25, 2h);
    const auto old = config({candidate}, "Season/Older");
    const auto fold = config({candidate}, "Season/October");
    const auto single = config({candidate}, "Season/Single");
    const auto before = config({candidate}, "Season/Before");
    SyntheticZones zones;
    auto older = october("Season/Older");
    older.rules_revision = "older-r1";
    older.offset_at_start_seconds = 10'800;
    older.transitions = {{utc(oct25), 7'200}};
    auto fixed_single = october("Season/Single");
    fixed_single.offset_at_start_seconds = 3'600;
    fixed_single.transitions.clear();
    auto fixed_before = october("Season/Before");
    fixed_before.offset_at_start_seconds = 10'800;
    fixed_before.transitions.clear();
    zones.views = {older, october(), fixed_single, fixed_before};
    const auto prior_start = utc(2026y / October / 24, 23h + 40min);
    const auto prior_end = utc(oct25, 20min);
    Basis active(utc(oct25), old);
    active.state.pause = schedule;
    active.ledger.deliveries = {delivery(first, prior_start, prior_end)};
    const auto projected =
        std::get<ScheduledPausePreview>(active.preview(zones, fold, {cut(change, 10min)}, horizon));
    expect_complete(projected, horizon);
    ASSERT_EQ(projected.upcoming.size(), 1U);
    expect_row(projected.upcoming.front(), second, utc(oct25, 1h + 40min), utc(oct25, 2h + 20min),
               local(oct25, 2h + 40min), local(oct25, 3h + 20min));

    // A separate archived fold had +04:00 then +03:00, so its Second was 23:40Z.
    auto earlier = october("Season/Earlier");
    earlier.rules_revision = "earlier-r1";
    earlier.offset_at_start_seconds = 14'400;
    earlier.transitions = {{utc(2026y / October / 24, 23h), 10'800}};
    zones.views.push_back(earlier);
    Basis second_only(utc(oct25), config({candidate}, "Season/Earlier"));
    second_only.state.pause = schedule;
    second_only.ledger.deliveries = {delivery(second, prior_start, prior_end)};
    const auto suppressed = std::get<ScheduledPausePreview>(
        second_only.preview(zones, single, {cut(change, 10min)}, horizon));
    expect_complete(suppressed, horizon);
    EXPECT_TRUE(suppressed.upcoming.empty());

    Basis fresh(utc(oct25), old, TimerLifecycle::Ready);
    for (const auto *proposed : {&single, &before}) {
        const auto preview = std::get<ScheduledPausePreview>(
            fresh.preview(zones, *proposed, {cut(change, std::nullopt)}, horizon));
        expect_complete(preview, horizon);
        if (proposed == &single) {
            ASSERT_EQ(preview.upcoming.size(), 1U);
            expect_row(preview.upcoming.front(), first, utc(oct25, 1h + 40min),
                       utc(oct25, 2h + 20min), local(oct25, 2h + 40min), local(oct25, 3h + 20min));
        } else {
            EXPECT_TRUE(preview.upcoming.empty());
        }
    }
}

TEST(ScheduledPauseAcceptance, FirstStartAndReopenUseOnlyRemainingCapturedTime) {
    SyntheticZones zones;
    zones.views = {march(3'600, {{utc(march29, 1h), 7'200}})};
    const auto proposed = config({sunday_rule(2h + 20min, 40min)}, "Season/March");
    const auto start = utc(march29, 1h + 20min);
    const auto end = utc(march29, 2h);
    const auto original = delivery(key("S", march29, 2h + 20min), start, end);
    for (const auto lifecycle : {TimerLifecycle::Ready, TimerLifecycle::Finished}) {
        for (const auto entry : {start, utc(march29, 1h + 45min), end}) {
            SCOPED_TRACE(entry);
            SCOPED_TRACE(static_cast<int>(lifecycle));
            Basis basis(utc(march29, 1h), proposed, lifecycle);
            basis.state.pause = manual;
            const auto result = std::get<ScheduledPauseProgress>(basis.enter(zones, entry));
            EXPECT_EQ(result.timer.lifecycle, TimerLifecycle::Active);
            EXPECT_EQ(result.timer.main_remaining_ms, 900'000);
            expect_observation(result.cursor.last_observation, observation(entry));
            EXPECT_FALSE(result.cursor.evaluated_through_utc_ms);
            expect_clear(result);
            if (entry == end) {
                EXPECT_TRUE(result.new_deliveries.empty());
                EXPECT_FALSE(result.next_known_expiry_utc_ms);
                EXPECT_EQ(result.pause.reasons, manual.reasons);
            } else {
                expect_single_delivery(result, original);
                EXPECT_EQ(result.next_known_expiry_utc_ms, end);
                EXPECT_EQ(result.pause.reasons, manual_schedule.reasons);
            }
        }
    }
    Basis reopened(utc(march29, 1h + 30min), proposed, TimerLifecycle::Finished);
    reopened.state.cursor.evaluated_through_utc_ms = start;
    reopened.state.pause = manual;
    reopened.ledger.deliveries = {original};
    const auto result =
        std::get<ScheduledPauseProgress>(reopened.enter(zones, utc(march29, 1h + 45min)));
    EXPECT_TRUE(result.new_deliveries.empty());
    EXPECT_EQ(result.next_known_expiry_utc_ms, end);
    EXPECT_EQ(result.pause.reasons, manual_schedule.reasons);
    EXPECT_EQ(result.cursor.evaluated_through_utc_ms, start);
}

TEST(ScheduledPauseAcceptance, SimultaneousExpiryAndStartPreserveTheScheduleUnion) {
    SyntheticZones zones;
    zones.views = {march(3'600, {{utc(march29, 1h), 7'200}})};
    auto next = sunday_rule(4h, 40min);
    next.schedule_id = "next";
    const auto current = config({sunday_rule(2h + 20min, 40min), next}, "Season/March");
    const auto start = utc(march29, 1h + 20min);
    const auto edge = utc(march29, 2h);
    const auto end = utc(march29, 2h + 40min);
    const auto first = delivery(key("S", march29, 2h + 20min), start, edge);
    Basis basis(start, current);
    basis.ledger.deliveries = {first};
    basis.state.pause = schedule;
    const auto result = std::get<ScheduledPauseProgress>(
        basis.progress(zones, {cut(edge, 40min), cut(edge + 1, 1ms)}));
    expect_single_delivery(result, delivery(key("next", march29, 4h), edge, end));
    expect_single_stop(result, first.key,
                       stopped(ScheduledPauseStopCause::expired, edge, observation(edge)));
    EXPECT_EQ(result.pause.reasons, schedule.reasons);
    EXPECT_EQ(result.timer.main_remaining_ms, 900'000);
    EXPECT_EQ(result.next_known_expiry_utc_ms, end);
}

// Edit cutover and entry examples, in frozen PR table order.
const auto a = rule("A", 11h + 45min, 30min);
const auto b = rule("B", 11h + 50min, 20min);
const auto a_window = delivery(key("A", jan8, 11h + 45min), jan(11h + 45min), jan(12h + 15min));
const auto b_window = delivery(key("B", jan8, 11h + 50min), jan(11h + 50min), jan(12h + 10min));

void overlapping(Basis &basis) {
    basis.ledger.deliveries = {a_window, b_window};
    basis.state.pause = manual_schedule;
}

void expect_effect(const ScheduledPausePreview &preview, const DeliveredScheduledPause &window,
                   std::optional<ScheduledPauseStop> stop = {}) {
    const auto effect =
        std::find_if(preview.cutover_effects.begin(), preview.cutover_effects.end(),
                     [&](const auto &row) { return row.delivery.key == window.key; });
    ASSERT_NE(effect, preview.cutover_effects.end());
    expect_delivery(effect->delivery, window);
    ASSERT_EQ(effect->stop_at_cutover.has_value(), stop.has_value());
    if (stop) {
        expect_stop(*effect->stop_at_cutover, *stop);
    }
}

TEST(ScheduledPauseAcceptance, OrdinaryEditsKeepOriginalKeyAndBounds) {
    SyntheticZones zones;
    zones.views = {fixed()};
    auto time_edit = a;
    time_edit.recurrence.local_time = 13h;
    time_edit.recurrence.weekdays = wednesday;
    const auto old = config({a, b});
    for (const auto &edited :
         {time_edit, rule("A", 11h + 45min, 5min), rule("A", 11h + 45min, 60min)}) {
        SCOPED_TRACE(edited.duration.count());
        const auto proposed = config({edited, b});
        Basis active(noon - 1, old);
        overlapping(active);
        const auto preview = std::get<ScheduledPausePreview>(
            active.preview(zones, proposed, {cut(noon, 1ms, manual)}, jan(12h + 15min)));
        expect_complete(preview, jan(12h + 15min));
        ASSERT_EQ(preview.cutover_effects.size(), 2U);
        expect_effect(preview, a_window);
        expect_effect(preview, b_window);
        const auto edit =
            std::get<ScheduledPauseCutover>(active.edit(zones, proposed, {cut(noon, 1ms, manual)}));
        EXPECT_TRUE(edit.progress.new_deliveries.empty());
        EXPECT_TRUE(edit.progress.stops.empty());
        EXPECT_EQ(edit.progress.pause.reasons, manual_schedule.reasons);
        EXPECT_EQ(edit.progress.next_known_expiry_utc_ms, b_window.window.end_utc_ms);

        const auto reopen_at = jan(12h + 11min);
        const auto b_end = b_window.window.end_utc_ms;
        Basis reopened(b_end, proposed, TimerLifecycle::Finished);
        reopened.epochs = {{old, {}, noon, "fixed-r1"}, {proposed, noon, {}, {}}};
        reopened.ledger.deliveries = {
            a_window,
            delivery(b_window.key, b_window.window.start_utc_ms, b_end,
                     stopped(ScheduledPauseStopCause::expired, b_end, observation(b_end)))};
        reopened.state.pause = manual_schedule;
        const auto entry = std::get<ScheduledPauseProgress>(reopened.enter(zones, reopen_at));
        EXPECT_TRUE(entry.new_deliveries.empty());
        EXPECT_EQ(entry.next_known_expiry_utc_ms, a_window.window.end_utc_ms);
        EXPECT_EQ(entry.pause.reasons, manual_schedule.reasons);

        Basis at_expiry(reopen_at, proposed);
        at_expiry.epochs = reopened.epochs;
        at_expiry.ledger.deliveries = reopened.ledger.deliveries;
        at_expiry.state.pause = manual_schedule;
        const auto end = a_window.window.end_utc_ms;
        const auto expiry =
            std::get<ScheduledPauseProgress>(at_expiry.progress(zones, {cut(end, 4min, manual)}));
        expect_single_stop(expiry, a_window.key,
                           stopped(ScheduledPauseStopCause::expired, end, observation(end)));
        EXPECT_EQ(expiry.pause.reasons, manual.reasons);
        EXPECT_EQ(expiry.timer.main_remaining_ms, 900'000);
    }
}

TEST(ScheduledPauseAcceptance, DeleteStopsOnlyTheDroppedContributor) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto old = config({a, b});
    const auto proposed = config({b});
    const auto stop = stopped(ScheduledPauseStopCause::deleted, noon, observation(noon));
    Basis active(noon - 1, old);
    overlapping(active);
    const auto preview = std::get<ScheduledPausePreview>(
        active.preview(zones, proposed, {cut(noon, 1ms, manual)}, jan(12h + 15min)));
    expect_complete(preview, jan(12h + 15min));
    ASSERT_EQ(preview.cutover_effects.size(), 2U);
    expect_effect(preview, a_window, stop);
    expect_effect(preview, b_window);
    ASSERT_TRUE(preview.pause_after_cutover);
    EXPECT_EQ(preview.pause_after_cutover->reasons, manual_schedule.reasons);
    const auto edit =
        std::get<ScheduledPauseCutover>(active.edit(zones, proposed, {cut(noon, 1ms, manual)}));
    expect_single_stop(edit.progress, a_window.key, stop);
    EXPECT_EQ(edit.progress.pause.reasons, manual_schedule.reasons);
    EXPECT_EQ(edit.progress.next_known_expiry_utc_ms, jan(12h + 10min));

    auto deleted = a_window;
    deleted.stop = stop;
    const auto b_end = b_window.window.end_utc_ms;
    Basis edited(noon, proposed);
    edited.epochs = {{old, {}, noon, "fixed-r1"}, {proposed, noon, {}, {}}};
    edited.ledger.deliveries = {deleted, b_window};
    edited.state.pause = manual_schedule;
    const auto expiry =
        std::get<ScheduledPauseProgress>(edited.progress(zones, {cut(b_end, 10min, manual)}));
    expect_single_stop(expiry, b_window.key,
                       stopped(ScheduledPauseStopCause::expired, b_end, observation(b_end)));
    EXPECT_EQ(expiry.pause.reasons, manual.reasons);
    EXPECT_FALSE(expiry.next_known_expiry_utc_ms);

    Basis reopening(noon, proposed, TimerLifecycle::Finished);
    reopening.epochs = edited.epochs;
    reopening.ledger.deliveries = edited.ledger.deliveries;
    reopening.state.pause = manual;
    const auto early = std::get<ScheduledPauseProgress>(reopening.enter(zones, jan(12h + 5min)));
    EXPECT_TRUE(early.new_deliveries.empty());
    EXPECT_EQ(early.next_known_expiry_utc_ms, b_end);
    EXPECT_EQ(early.pause.reasons, manual_schedule.reasons);
    const auto late = std::get<ScheduledPauseProgress>(reopening.enter(zones, jan(12h + 11min)));
    EXPECT_TRUE(late.new_deliveries.empty());
    EXPECT_FALSE(late.next_known_expiry_utc_ms);
    EXPECT_EQ(late.pause.reasons, manual.reasons);
}

TEST(ScheduledPauseAcceptance, ReenableRespectsLatestDisableBoundaryAndRetainedKey) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto old = config({a, b});
    auto disabled_a = a;
    disabled_a.enabled = false;
    const auto disabled = config({disabled_a, b});
    const auto stop = stopped(ScheduledPauseStopCause::disabled, noon, observation(noon));
    Basis active(noon - 1, old);
    overlapping(active);
    const auto preview = std::get<ScheduledPausePreview>(
        active.preview(zones, disabled, {cut(noon, 1ms, manual)}, jan(12h + 15min)));
    expect_complete(preview, jan(12h + 15min));
    ASSERT_EQ(preview.cutover_effects.size(), 2U);
    expect_effect(preview, a_window, stop);
    expect_effect(preview, b_window);
    ASSERT_TRUE(preview.pause_after_cutover);
    EXPECT_EQ(preview.pause_after_cutover->reasons, manual_schedule.reasons);
    const auto disable =
        std::get<ScheduledPauseCutover>(active.edit(zones, disabled, {cut(noon, 1ms, manual)}));
    expect_single_stop(disable.progress, a_window.key, stop);

    auto retained = a_window;
    retained.stop = stop;
    const auto reenabled_at = jan(12h + 3min);
    const auto entry_at = jan(12h + 4min);
    const auto b_end = jan(12h + 10min);
    for (const auto time : {12h + 2min, 12h + 6min, 11h + 45min}) {
        SCOPED_TRACE(time.count());
        const auto reenabled = config({rule("A", time, 30min), b});
        Basis finished(reenabled_at, reenabled, TimerLifecycle::Finished);
        finished.epochs = {{old, {}, noon, "fixed-r1"},
                           {disabled, noon, reenabled_at, "fixed-r1"},
                           {reenabled, reenabled_at, {}, {}}};
        finished.ledger.deliveries = {retained, b_window};
        finished.state.pause = manual;
        const auto entry = std::get<ScheduledPauseProgress>(finished.enter(zones, entry_at));
        EXPECT_TRUE(entry.new_deliveries.empty());
        EXPECT_EQ(entry.next_known_expiry_utc_ms, b_end);
        EXPECT_EQ(entry.pause.reasons, manual_schedule.reasons);

        Basis running(entry_at, reenabled);
        running.epochs = finished.epochs;
        running.ledger.deliveries = finished.ledger.deliveries;
        running.state.pause = manual_schedule;
        // A replayed trusted entry is stale. Entry has no caller-supplied delivery key.
        finished.state.snapshot_ref = "finished-at-entry-observation";
        finished.state.cursor.last_observation = observation(entry_at);
        expect_refusal(finished.enter(zones, entry_at), ScheduledPauseRefusal::stale_observation);

        const bool new_window = time == 12h + 6min;
        const auto result = std::get<ScheduledPauseProgress>(
            new_window ? running.progress(
                             zones, {cut(jan(12h + 6min), 2min, manual), cut(b_end, 4min, manual)})
                       : running.progress(zones, {cut(b_end, 6min, manual)}));
        expect_single_stop(result, b_window.key,
                           stopped(ScheduledPauseStopCause::expired, b_end, observation(b_end)));
        if (new_window) {
            expect_single_delivery(
                result, delivery(key("A", jan8, 12h + 6min), jan(12h + 6min), jan(12h + 36min)));
            EXPECT_EQ(result.pause.reasons, manual_schedule.reasons);
            EXPECT_EQ(result.next_known_expiry_utc_ms, jan(12h + 36min));
        } else {
            EXPECT_TRUE(result.new_deliveries.empty());
            EXPECT_EQ(result.pause.reasons, manual.reasons);
            EXPECT_FALSE(result.next_known_expiry_utc_ms);
        }
    }
}

TEST(ScheduledPauseAcceptance, EntryAfterReadyOrFinishedCutoverExcludesDroppedRules) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto old = config({a, b});
    auto disabled_a = a;
    disabled_a.enabled = false;
    const auto entry_at = jan(12h + 5min);
    for (const auto lifecycle : {TimerLifecycle::Ready, TimerLifecycle::Finished}) {
        for (const auto &proposed : {config({b}), config({disabled_a, b})}) {
            SCOPED_TRACE(static_cast<int>(lifecycle));
            SCOPED_TRACE(proposed.rules.size());
            Basis inactive(noon - 1, old, lifecycle);
            inactive.state.pause = manual;
            const auto preview = std::get<ScheduledPausePreview>(
                inactive.preview(zones, proposed, {cut(noon, std::nullopt, manual)}, entry_at));
            expect_complete(preview, entry_at);
            EXPECT_TRUE(preview.cutover_effects.empty());
            const auto edit = std::get<ScheduledPauseCutover>(
                inactive.edit(zones, proposed, {cut(noon, std::nullopt, manual)}));
            EXPECT_TRUE(edit.progress.new_deliveries.empty());
            EXPECT_TRUE(edit.progress.stops.empty());
            EXPECT_EQ(edit.progress.timer.lifecycle, lifecycle);

            Basis edited(noon, proposed, lifecycle);
            edited.epochs = {{old, {}, noon, "fixed-r1"}, {proposed, noon, {}, {}}};
            edited.state.pause = manual;
            const auto entry = std::get<ScheduledPauseProgress>(edited.enter(zones, entry_at));
            expect_single_delivery(entry, b_window);
            EXPECT_EQ(entry.next_known_expiry_utc_ms, jan(12h + 10min));
            EXPECT_EQ(entry.timer.main_remaining_ms, 900'000);
            EXPECT_EQ(entry.pause.reasons, manual_schedule.reasons);
        }
    }
}

TEST(ScheduledPauseAcceptance, ExactCutoverBelongsToOldEpochIncludingDueDisable) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto old_rule = rule("A", 12h, 20min);
    const auto old = config({old_rule});
    const auto proposed = config({rule("A", 12h, 30min)});
    const auto due = delivery(key("A", jan8, 12h), noon, jan(12h + 20min));
    Basis ready(noon - 1, old, TimerLifecycle::Ready);
    const auto edit =
        std::get<ScheduledPauseCutover>(ready.edit(zones, proposed, {cut(noon, std::nullopt)}));
    EXPECT_TRUE(edit.progress.new_deliveries.empty());
    EXPECT_EQ(edit.closed_zone_rules_revision, "fixed-r1");
    expect_observation(edit.cutover, observation(noon));

    Basis edited(noon, proposed, TimerLifecycle::Ready);
    edited.epochs = {{old, {}, noon, "fixed-r1"}, {proposed, noon, {}, {}}};
    const auto entry = std::get<ScheduledPauseProgress>(edited.enter(zones, jan(12h + 5min)));
    expect_single_delivery(entry, due);
    EXPECT_EQ(entry.next_known_expiry_utc_ms, jan(12h + 20min));

    auto disabled_rule = old_rule;
    disabled_rule.enabled = false;
    const auto disabled = config({disabled_rule});
    Basis active(noon - 1, old);
    const auto stop = stopped(ScheduledPauseStopCause::disabled, noon, observation(noon));
    const auto preview = std::get<ScheduledPausePreview>(
        active.preview(zones, disabled, {cut(noon, 1ms)}, jan(12h + 5min)));
    expect_complete(preview, jan(12h + 5min));
    ASSERT_EQ(preview.cutover_effects.size(), 1U);
    expect_effect(preview, due, stop);
    ASSERT_TRUE(preview.pause_after_cutover);
    EXPECT_TRUE(preview.pause_after_cutover->reasons.empty());
    const auto disable =
        std::get<ScheduledPauseCutover>(active.edit(zones, disabled, {cut(noon, 1ms)}));
    // The cutover reports both the captured due delivery and its effective stop.
    ASSERT_EQ(disable.progress.new_deliveries.size(), 1U);
    const auto &captured = disable.progress.new_deliveries.front();
    EXPECT_EQ(captured.key, due.key);
    EXPECT_EQ(captured.window.start_utc_ms, noon);
    EXPECT_EQ(captured.window.end_utc_ms, jan(12h + 20min));
    expect_single_stop(disable.progress, due.key, stop);
    EXPECT_TRUE(disable.progress.pause.reasons.empty());
    EXPECT_FALSE(disable.progress.next_known_expiry_utc_ms);
    EXPECT_EQ(disable.progress.timer.main_remaining_ms, 899'999);
}

TEST(ScheduledPauseAcceptance, EntryUsesClosedAndNewFactsWithSameNominalIdentity) {
    SyntheticZones zones;
    zones.views = {fixed(), fixed(utc(2026y / January / 12), -3'600, "Fixed/West", "west-r1")};
    const auto candidate = rule("A", 11h + 50min, 30min);
    const auto old = config({candidate});
    const auto proposed = config({candidate}, "Fixed/West");
    const auto old_window =
        delivery(key("A", jan8, 11h + 50min), jan(11h + 50min), jan(12h + 20min));
    const auto new_window = delivery(old_window.key, jan(12h + 50min), jan(13h + 20min));
    Basis ready(noon - 1, old, TimerLifecycle::Ready);
    const auto cutover =
        std::get<ScheduledPauseCutover>(ready.edit(zones, proposed, {cut(noon, std::nullopt)}));
    EXPECT_EQ(cutover.closed_zone_rules_revision, "fixed-r1");
    EXPECT_TRUE(cutover.progress.new_deliveries.empty());

    const auto early = jan(12h + 5min);
    const auto late = jan(12h + 55min);
    Basis entry_basis(noon, proposed, TimerLifecycle::Ready);
    entry_basis.epochs = {{old, {}, noon, "fixed-r1"}, {proposed, noon, {}, {}}};
    const auto old_entry = std::get<ScheduledPauseProgress>(entry_basis.enter(zones, early));
    expect_single_delivery(old_entry, old_window);
    EXPECT_EQ(old_entry.next_known_expiry_utc_ms, jan(12h + 20min));
    expect_clear(old_entry);
    const auto new_entry = std::get<ScheduledPauseProgress>(entry_basis.enter(zones, late));
    expect_single_delivery(new_entry, new_window);
    EXPECT_EQ(new_entry.next_known_expiry_utc_ms, jan(13h + 20min));

    Basis restored(jan(12h + 20min), proposed, TimerLifecycle::Finished);
    restored.epochs = entry_basis.epochs;
    auto expired = old_window;
    expired.stop =
        stopped(ScheduledPauseStopCause::expired, jan(12h + 20min), observation(jan(12h + 20min)));
    restored.ledger.deliveries = {expired};
    const auto duplicate = std::get<ScheduledPauseProgress>(restored.enter(zones, late));
    EXPECT_TRUE(duplicate.new_deliveries.empty());
    EXPECT_TRUE(duplicate.pause.reasons.empty());
    expect_clear(duplicate);

    zones.views.erase(zones.views.begin());
    const auto unprovable = std::get<ScheduledPauseProgress>(entry_basis.enter(zones, early));
    expect_hold(unprovable, early, observation(early));
    EXPECT_FALSE(unprovable.cursor.evaluated_through_utc_ms);
    EXPECT_TRUE(unprovable.new_deliveries.empty());
}

TEST(ScheduledPauseAcceptance, DroppingLastContributorClearsOnlySchedule) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto old = config({a});
    auto disabled_a = a;
    disabled_a.enabled = false;
    for (const auto cause : {ScheduledPauseStopCause::deleted, ScheduledPauseStopCause::disabled}) {
        SCOPED_TRACE(static_cast<int>(cause));
        const auto proposed =
            cause == ScheduledPauseStopCause::deleted ? config({}) : config({disabled_a});
        Basis active(noon - 1, old);
        active.ledger.deliveries = {a_window};
        active.state.pause = manual_schedule;
        const auto stop = stopped(cause, noon, observation(noon));
        const auto preview = std::get<ScheduledPausePreview>(
            active.preview(zones, proposed, {cut(noon, 1ms, manual)}, jan(12h + 1min)));
        expect_complete(preview, jan(12h + 1min));
        ASSERT_EQ(preview.cutover_effects.size(), 1U);
        expect_effect(preview, a_window, stop);
        ASSERT_TRUE(preview.pause_after_cutover);
        EXPECT_EQ(preview.pause_after_cutover->reasons, manual.reasons);
        const auto applied =
            std::get<ScheduledPauseCutover>(active.edit(zones, proposed, {cut(noon, 1ms, manual)}));
        expect_single_stop(applied.progress, a_window.key, stop);
        EXPECT_EQ(applied.progress.pause.reasons, manual.reasons);
        EXPECT_EQ(applied.progress.timer.main_remaining_ms, 900'000);
    }
}

TEST(ScheduledPauseAcceptance, CutoverDistinguishesOldSideFromFutureAndEntryOnlyGaps) {
    const auto old = config({a});
    const auto proposed = config({a}, "Fixed/West");
    const ScheduledPauseCoverageFailure absent = ScheduleZoneUnavailable::insufficient_coverage;
    SyntheticZones zones;
    Basis held(noon - 1, old);
    held.state.cursor.evaluated_through_utc_ms = jan(11h + 40min);
    held.state.cursor.hold = ScheduledPauseCoverageHold{absent, jan(11h + 40min)};
    const auto unresolved = held.edit(zones, config({}), {cut(noon, 1ms)});
    ASSERT_TRUE(std::holds_alternative<ScheduledPauseCoverageFailure>(unresolved));
    EXPECT_EQ(std::get<ScheduledPauseCoverageFailure>(unresolved), absent);
    const auto unknown_preview = std::get<ScheduledPausePreview>(
        held.preview(zones, config({}), {cut(noon, 1ms)}, jan(12h + 1min)));
    EXPECT_TRUE(unknown_preview.incomplete);
    EXPECT_FALSE(unknown_preview.pause_after_cutover);

    // Old starts through C are certified. The prior hold is resumed at C,
    // with no elapsed charged and the still-active delivery kept verbatim.
    zones.views = {fixed()};
    Basis known_old(noon - 1, old);
    const auto retained_endpoint = jan(11h + 50min);
    known_old.state.cursor.evaluated_through_utc_ms = retained_endpoint;
    known_old.state.cursor.hold = ScheduledPauseCoverageHold{absent, retained_endpoint};
    known_old.state.pause = manual_schedule;
    known_old.ledger.deliveries = {a_window};
    const auto future_gap =
        std::get<ScheduledPauseCutover>(known_old.edit(zones, proposed, {cut(noon, 1ms, manual)}));
    expect_observation(future_gap.cutover, observation(noon));
    EXPECT_EQ(future_gap.progress.timer.main_remaining_ms, 900'000);
    EXPECT_EQ(future_gap.progress.cursor.evaluated_through_utc_ms, retained_endpoint);
    EXPECT_EQ(future_gap.progress.next_known_expiry_utc_ms, jan(12h + 15min));
    EXPECT_EQ(future_gap.progress.pause.reasons, manual_schedule.reasons);
    EXPECT_TRUE(future_gap.progress.new_deliveries.empty());
    EXPECT_TRUE(future_gap.progress.stops.empty());
    expect_hold(future_gap.progress, noon, observation(noon));

    // Ready has no ordinary start to reconcile at C. A missing old lookback
    // becomes relevant at the later entry, even with a complete new view.
    Basis ready(noon - 1, old, TimerLifecycle::Ready);
    zones.views.front().from_utc_ms = utc(2026y / January / 6, 12h);
    zones.views.push_back(fixed(utc(2026y / January / 12), -3'600, "Fixed/West", "west-r1"));
    const auto entry_only_gap =
        std::get<ScheduledPauseCutover>(ready.edit(zones, proposed, {cut(noon, std::nullopt)}));
    expect_clear(entry_only_gap.progress);
    EXPECT_TRUE(entry_only_gap.progress.new_deliveries.empty());
    Basis edited(noon, proposed, TimerLifecycle::Ready);
    edited.epochs = {{old, {}, noon, "fixed-r1"}, {proposed, noon, {}, {}}};
    const auto entry_at = jan(12h + 5min);
    const auto late = std::get<ScheduledPauseProgress>(edited.enter(zones, entry_at));
    expect_hold(late, entry_at, observation(entry_at));
    EXPECT_FALSE(late.cursor.evaluated_through_utc_ms);
    EXPECT_TRUE(late.new_deliveries.empty());
}

// Coverage and clock examples, in frozen PR table order.
constexpr auto jan10_noon = utc(2026y / January / 10, 12h);
constexpr auto anchor = jan(11h + 50min);
constexpr auto target = jan(12h + 1min);
constexpr auto known_start = jan(11h + 55min);
constexpr auto known_end = jan(12h + 2min);
constexpr auto deadline = jan(12h + 4min);
const auto morning = config({rule("morning", 9h, 1min)});
const auto known_rule = config({rule("known", 11h + 55min, 7min)});
const auto known = delivery(key("known", jan8, 11h + 55min), known_start, known_end);
const ScheduledPauseCoverageFailure short_zone = ScheduleZoneUnavailable::insufficient_coverage;

TEST(ScheduledPauseAcceptance, CertifiedStartFrontierAndFirstUnevaluableMillisecond) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    Basis basis(anchor, morning);
    for (const bool split_after_frontier : {true, false}) {
        SCOPED_TRACE(split_after_frontier);
        const auto evaluated = split_after_frontier ? noon + 1 : noon;
        const auto result = std::get<ScheduledPauseProgress>(
            split_after_frontier
                ? basis.progress(zones, {cut(noon + 1, 600'001ms), cut(target, 59'999ms)})
                : basis.progress(zones, {cut(noon, 600'000ms), cut(target, 60'000ms)}));
        EXPECT_EQ(result.starts_resolved_through_utc_ms, noon);
        EXPECT_EQ(result.cursor.evaluated_through_utc_ms, evaluated);
        EXPECT_EQ(result.timer.main_remaining_ms, split_after_frontier ? 299'999 : 300'000);
        EXPECT_EQ(result.timer.lifecycle, TimerLifecycle::Active);
        expect_hold(result, evaluated, observation(target), short_zone);
        EXPECT_TRUE(result.pause.reasons.empty());
        EXPECT_TRUE(result.new_deliveries.empty());
        EXPECT_TRUE(result.timer_transitions.empty());
    }
}

TEST(ScheduledPauseAcceptance, HeldProgressReconcilesKnownExpiryAndLaterHardEnd) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    Basis basis(anchor, known_rule);
    basis.state.timer_config.hard_end_utc_ms = deadline;
    const auto held = std::get<ScheduledPauseProgress>(basis.progress(
        zones, {cut(known_start, 300'000ms), cut(noon + 1, 300'001ms), cut(target, 59'999ms)}));
    EXPECT_EQ(held.starts_resolved_through_utc_ms, noon);
    EXPECT_EQ(held.cursor.evaluated_through_utc_ms, noon + 1);
    expect_hold(held, noon + 1, observation(target), short_zone);
    expect_single_delivery(held, known);
    EXPECT_EQ(held.next_known_expiry_utc_ms, known_end);
    EXPECT_EQ(held.pause.reasons, schedule.reasons);
    EXPECT_EQ(held.timer.main_remaining_ms, 600'000);

    Basis resumed_wall(target, known_rule);
    resumed_wall.state.timer_config.hard_end_utc_ms = deadline;
    resumed_wall.state.timer.main_remaining_ms = 600'000;
    resumed_wall.state.cursor.evaluated_through_utc_ms = noon + 1;
    resumed_wall.state.cursor.hold = ScheduledPauseCoverageHold{short_zone, noon + 1};
    resumed_wall.state.pause = schedule;
    resumed_wall.ledger.deliveries = {known};
    const auto expired = std::get<ScheduledPauseProgress>(
        resumed_wall.progress(zones, {cut(known_end, std::nullopt)}));
    expect_single_stop(
        expired, known.key,
        stopped(ScheduledPauseStopCause::expired, known_end, observation(known_end)));
    EXPECT_FALSE(expired.next_known_expiry_utc_ms);
    EXPECT_TRUE(expired.pause.reasons.empty());
    EXPECT_EQ(expired.timer.main_remaining_ms, 600'000);
    EXPECT_EQ(expired.cursor.evaluated_through_utc_ms, noon + 1);
    expect_hold(expired, noon + 1, observation(known_end), short_zone);

    const auto finished = std::get<ScheduledPauseProgress>(
        resumed_wall.progress(zones, {cut(known_end, std::nullopt), cut(deadline, std::nullopt)}));
    expect_single_stop(
        finished, known.key,
        stopped(ScheduledPauseStopCause::expired, known_end, observation(known_end)));
    EXPECT_EQ(finished.cursor.evaluated_through_utc_ms, noon + 1);
    expect_finish(finished, 600'000, deadline);
}

TEST(ScheduledPauseAcceptance, DeadlineDoesNotConsumeItsOneMillisecondCut) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    Basis basis(anchor, morning);
    basis.state.timer_config.hard_end_utc_ms = noon + 1;
    const auto result = std::get<ScheduledPauseProgress>(
        basis.progress(zones, {cut(noon, 600'000ms), cut(noon + 1, 1ms)}));
    EXPECT_EQ(result.cursor.evaluated_through_utc_ms, noon);
    EXPECT_TRUE(result.new_deliveries.empty());
    EXPECT_TRUE(result.stops.empty());
    expect_finish(result, 300'000, noon + 1);
}

TEST(ScheduledPauseAcceptance, DeadlineWinsTiesWithStartAndExpiry) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    Basis basis(anchor, known_rule);
    basis.state.timer_config.hard_end_utc_ms = known_start;
    const auto at_start =
        std::get<ScheduledPauseProgress>(basis.progress(zones, {cut(known_start, 300'000ms)}));
    expect_finish(at_start, 900'000, known_start);
    EXPECT_TRUE(at_start.new_deliveries.empty());
    EXPECT_TRUE(at_start.stops.empty());
    EXPECT_FALSE(at_start.pause.reasons.contains(PauseReason::Schedule));

    Basis expiry_basis(anchor, known_rule);
    expiry_basis.state.timer_config.hard_end_utc_ms = known_end;
    const auto manual_at = jan(11h + 56min);
    const auto at_expiry = std::get<ScheduledPauseProgress>(expiry_basis.progress(
        zones, {cut(known_start, 300'000ms), cut(manual_at, 60'000ms, manual),
                cut(noon + 1, 240'001ms, manual), cut(known_end, 119'999ms, manual)}));
    expect_finish(at_expiry, 600'000, known_end);
    expect_single_delivery(at_expiry, known);
    EXPECT_TRUE(at_expiry.stops.empty());
    EXPECT_TRUE(at_expiry.pause.reasons.contains(PauseReason::Manual));
}

TEST(ScheduledPauseAcceptance, LateTrustedDeadlineAndEarlierNaturalFinishKeepTheirObservations) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    Basis basis(anchor, known_rule);
    basis.state.timer_config.hard_end_utc_ms = deadline;
    const auto late = jan(12h + 5min);
    const auto elapsed = {cut(known_start, 300'000ms), cut(noon + 1, 300'001ms),
                          cut(known_end, std::nullopt), cut(late, std::nullopt)};
    // An observation from a different clock lineage cannot supply the deadline.
    const auto untrusted_cut = {cut(deadline, std::nullopt)};
    auto untrusted = basis.clock(untrusted_cut);
    untrusted.clock_lineage = "other-clock";
    expect_refusal(progress_scheduled_pauses(basis.input(), untrusted, zones),
                   ScheduledPauseRefusal::stale_observation);
    const auto result = std::get<ScheduledPauseProgress>(basis.progress(zones, elapsed));
    expect_finish(result, 600'000, late);
    expect_single_stop(
        result, known.key,
        stopped(ScheduledPauseStopCause::expired, known_end, observation(known_end)));

    const auto natural_at = jan(12h + 3min);
    zones.views = {fixed()};
    Basis natural(natural_at - 1, morning);
    natural.state.timer.main_remaining_ms = 1;
    natural.state.timer_config.hard_end_utc_ms = deadline;
    const auto finished = std::get<ScheduledPauseProgress>(
        natural.progress(zones, {cut(natural_at, 1ms), cut(late, std::nullopt)}));
    expect_finish(finished, 0, natural_at, TimerTransition::MainExpired);
    EXPECT_TRUE(finished.new_deliveries.empty());
    Basis already_finished(natural_at, morning, TimerLifecycle::Finished);
    already_finished.state.timer.main_remaining_ms = 0;
    already_finished.state.timer_config.hard_end_utc_ms = deadline;
    already_finished.state.cursor.evaluated_through_utc_ms = natural_at;
    const auto after = std::get<ScheduledPauseProgress>(
        already_finished.progress(zones, {cut(late, std::nullopt)}));
    EXPECT_EQ(after.timer.lifecycle, TimerLifecycle::Finished);
    EXPECT_TRUE(after.timer_transitions.empty());
}

TEST(ScheduledPauseAcceptance, ResumeIsProspectiveAndDoesNotReplayExpiredStarts) {
    SyntheticZones zones;
    const auto current = config({rule("short", 12h + 1min, 1min), rule("long", 12h + 1min, 5min)});
    Basis held(target, current);
    held.state.timer.main_remaining_ms = 299'999;
    held.state.cursor.evaluated_through_utc_ms = noon + 1;
    held.state.cursor.hold = ScheduledPauseCoverageHold{short_zone, noon + 1};
    const auto resume = jan(12h + 3min);
    const auto end = jan(12h + 6min);
    const auto final = jan(12h + 7min);
    const auto long_window = delivery(key("long", jan8, 12h + 1min), target, end);
    zones.views = {fixed(utc(2026y / January / 10, 12h + 3min))};
    const auto resumed =
        std::get<ScheduledPauseProgress>(held.progress(zones, {cut(resume, 120'000ms)}));
    expect_clear(resumed);
    expect_observation(resumed.cursor.last_observation, observation(resume));
    EXPECT_EQ(resumed.cursor.evaluated_through_utc_ms, noon + 1);
    EXPECT_EQ(resumed.timer.main_remaining_ms, 299'999);
    expect_single_delivery(resumed, long_window);
    EXPECT_EQ(resumed.next_known_expiry_utc_ms, end);
    EXPECT_EQ(resumed.pause.reasons, schedule.reasons);

    Basis prospective(resume, current);
    prospective.state.snapshot_ref = "after-resume";
    prospective.state.timer.main_remaining_ms = 299'999;
    prospective.state.cursor.evaluated_through_utc_ms = noon + 1;
    prospective.state.pause = schedule;
    prospective.ledger.deliveries = {long_window};
    zones.views = {fixed(utc(2026y / January / 10, 12h + 7min))};
    const auto at_expiry =
        std::get<ScheduledPauseProgress>(prospective.progress(zones, {cut(end, 180'000ms)}));
    EXPECT_EQ(at_expiry.timer.main_remaining_ms, 299'999);
    EXPECT_TRUE(at_expiry.pause.reasons.empty());
    const auto advanced = std::get<ScheduledPauseProgress>(
        prospective.progress(zones, {cut(end, 180'000ms), cut(final, 59'999ms)}));
    expect_single_stop(advanced, long_window.key,
                       stopped(ScheduledPauseStopCause::expired, end, observation(end)));
    EXPECT_EQ(advanced.timer.main_remaining_ms, 240'000);
    EXPECT_EQ(advanced.cursor.evaluated_through_utc_ms, final);
    EXPECT_TRUE(advanced.new_deliveries.empty());
    expect_clear(advanced);

    // The matching snapshot and anchor cannot make a previously applied cut fresh.
    expect_refusal(prospective.progress(zones, {cut(resume, 120'000ms)}),
                   ScheduledPauseRefusal::stale_observation);
}

TEST(ScheduledPauseAcceptance, StartAndReopenAtOrAfterDeadlinePrecedeScheduleLookup) {
    const auto overlapping_deadline = config({rule("late", 12h, 10min)});
    for (const auto lifecycle : {TimerLifecycle::Ready, TimerLifecycle::Finished}) {
        for (const auto at : {deadline, jan(12h + 5min)}) {
            for (const bool covered : {false, true}) {
                SCOPED_TRACE(static_cast<int>(lifecycle));
                SCOPED_TRACE(at);
                SCOPED_TRACE(covered);
                SyntheticZones zones;
                if (covered) {
                    zones.views = {fixed()};
                }
                Basis basis(noon - 1, overlapping_deadline, lifecycle);
                basis.state.timer_config.hard_end_utc_ms = deadline;
                expect_refusal(basis.enter(zones, at), ScheduledPauseRefusal::hard_end_reached);
                EXPECT_EQ(zones.reads, 0U);
            }
        }
    }
}

TEST(ScheduledPauseAcceptance, EntryTypesMalformedStaleAndHeldInputsAndAnchorsProgression) {
    SyntheticZones zones;
    Basis active(anchor, morning);
    expect_refusal(active.enter(zones, target), ScheduledPauseRefusal::invalid_input);
    Basis fresh(anchor, morning, TimerLifecycle::Ready);
    auto entry = fresh.entry_input(target);
    entry.admitted_timer.lifecycle = TimerLifecycle::Ready;
    expect_refusal(enter_scheduled_pauses(fresh.input(), entry, zones),
                   ScheduledPauseRefusal::invalid_input);
    entry.admitted_timer.lifecycle = TimerLifecycle::Active;
    entry.admitted_timer.main_remaining_ms = -1;
    expect_refusal(enter_scheduled_pauses(fresh.input(), entry, zones),
                   ScheduledPauseRefusal::invalid_input);
    entry.admitted_timer.main_remaining_ms = 900'000;
    entry.source_snapshot_ref = "old-snapshot";
    expect_refusal(enter_scheduled_pauses(fresh.input(), entry, zones),
                   ScheduledPauseRefusal::stale_observation);
    // Same snapshot and lifecycle with a trusted observation older than the cursor.
    expect_refusal(fresh.enter(zones, anchor - 1), ScheduledPauseRefusal::stale_observation);

    entry.source_snapshot_ref = fresh.state.snapshot_ref;
    entry.observation = observation(target, "trusted-entry");
    const auto held =
        std::get<ScheduledPauseProgress>(enter_scheduled_pauses(fresh.input(), entry, zones));
    EXPECT_EQ(held.source_snapshot_ref, fresh.state.snapshot_ref);
    EXPECT_EQ(held.timer.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(held.timer.main_remaining_ms, 900'000);
    EXPECT_FALSE(held.cursor.evaluated_through_utc_ms);
    expect_hold(held, target, entry.observation, short_zone);
    EXPECT_TRUE(held.new_deliveries.empty());
    EXPECT_TRUE(held.stops.empty());

    Basis retained(anchor, morning, TimerLifecycle::Finished);
    retained.state.cursor.evaluated_through_utc_ms = anchor - 1;
    const auto reopened = std::get<ScheduledPauseProgress>(retained.enter(zones, target));
    EXPECT_EQ(reopened.cursor.evaluated_through_utc_ms, anchor - 1);
    expect_hold(reopened, target, observation(target), short_zone);

    // Use a synthetic post-entry basis with the exact admitted anchor and
    // no invented evaluated endpoint. Another missing interval preserves both.
    Basis entered(target, morning);
    entered.state.snapshot_ref = "after-entry";
    entered.state.cursor.last_observation = entry.observation;
    entered.state.cursor.evaluated_through_utc_ms.reset();
    entered.state.cursor.hold = ScheduledPauseCoverageHold{short_zone, target};
    const auto later =
        std::get<ScheduledPauseProgress>(entered.progress(zones, {cut(known_end, std::nullopt)}));
    EXPECT_EQ(later.timer.main_remaining_ms, 900'000);
    EXPECT_FALSE(later.cursor.evaluated_through_utc_ms);
    expect_hold(later, target, observation(known_end), short_zone);
}

TEST(ScheduledPauseAcceptance, WallTimeWithoutElapsedCannotEvaluateButCanFinish) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    Basis basis(anchor, morning);
    basis.state.timer_config.hard_end_utc_ms = deadline;
    const auto held =
        std::get<ScheduledPauseProgress>(basis.progress(zones, {cut(target, std::nullopt)}));
    EXPECT_EQ(held.starts_resolved_through_utc_ms, noon);
    EXPECT_EQ(held.cursor.evaluated_through_utc_ms, anchor);
    EXPECT_EQ(held.timer.main_remaining_ms, 900'000);
    expect_hold(held, anchor, observation(target));
    const auto finished = std::get<ScheduledPauseProgress>(
        basis.progress(zones, {cut(target, std::nullopt), cut(deadline, std::nullopt)}));
    EXPECT_EQ(finished.cursor.evaluated_through_utc_ms, anchor);
    expect_finish(finished, 900'000, deadline);

    // With complete start facts, the missing split is the only uncertainty.
    zones.views = {fixed()};
    const auto missing_elapsed =
        std::get<ScheduledPauseProgress>(basis.progress(zones, {cut(target, std::nullopt)}));
    expect_hold(missing_elapsed, anchor, observation(target),
                ScheduledPauseKnowledgeUnavailable::elapsed_split);
    EXPECT_EQ(missing_elapsed.cursor.evaluated_through_utc_ms, anchor);
    EXPECT_EQ(missing_elapsed.timer.main_remaining_ms, 900'000);
}

TEST(ScheduledPauseAcceptance, LocalEndCoverageIsNarrowerThanStartCoverage) {
    SyntheticZones zones;
    zones.views = {fixed(jan10_noon)};
    const auto jan6 = utc(2026y / January / 6);
    constexpr auto jan7 = 2026y / January / 7;
    const auto local_frontier = utc(jan7, 12h);
    auto long_rule = rule("long", 12h, 72h);
    long_rule.recurrence.weekdays = wednesday;
    const auto current = config({long_rule});
    Basis ready(jan6 - 1, current, TimerLifecycle::Ready);
    const auto preview = std::get<ScheduledPausePreview>(
        ready.preview(zones, current, {cut(jan6, std::nullopt)}, noon));
    EXPECT_TRUE(preview.incomplete);
    EXPECT_EQ(preview.resolved_through_utc_ms, local_frontier);
    ASSERT_EQ(preview.upcoming.size(), 1U);
    expect_row(preview.upcoming.front(), key("long", jan7, 12h), local_frontier, jan10_noon,
               local(jan7, 12h), local(2026y / January / 10, 12h));
    const auto at_frontier = std::get<ScheduledPausePreview>(
        ready.preview(zones, current, {cut(jan6, std::nullopt)}, local_frontier));
    expect_complete(at_frontier, local_frontier);
    ASSERT_EQ(at_frontier.upcoming.size(), 1U);
    EXPECT_EQ(at_frontier.upcoming.front().window.local_end, local(2026y / January / 10, 12h));

    // Thursday at noon is at the inclusive start frontier; its UTC end lies
    // beyond the supplied local facts and still must be captured on delivery.
    long_rule.recurrence.weekdays = {false, false, false, true, false, false, false};
    Basis active(noon - 1, config({long_rule}));
    const auto progress =
        std::get<ScheduledPauseProgress>(active.progress(zones, {cut(noon, 1ms)}));
    EXPECT_EQ(progress.starts_resolved_through_utc_ms, noon);
    const auto jan11_noon = utc(2026y / January / 11, 12h);
    expect_single_delivery(progress, delivery(key("long", jan8, 12h), noon, jan11_noon));
    EXPECT_EQ(progress.next_known_expiry_utc_ms, jan11_noon);
}

TEST(ScheduledPauseAcceptance, InadequateOrIncoherentFactsCannotCertifyAnEmptyResult) {
    const auto current = config({rule("long", 13h, 72h)});
    const auto required_lower = utc(2026y / January / 3, 12h);
    auto inadequate = fixed(jan10_noon);
    inadequate.from_utc_ms = required_lower + 1;
    auto wrong_zone = fixed(jan10_noon);
    wrong_zone.configured_zone = "Other/Zero";
    auto too_many = fixed(jan10_noon);
    // All 65 transitions lie inside even the smallest 48h-padded input at
    // entry. The offsets differ by a second and crossings are a minute apart.
    for (int i = 0; i < 65; ++i) {
        too_many.transitions.push_back(
            {utc(2026y / January / 7, 12h + minutes{i}), i % 2 == 0 ? 1 : 0});
    }
    Basis ready(noon - 1, current, TimerLifecycle::Ready);
    for (const auto *bad : {&inadequate, &wrong_zone, &too_many}) {
        SCOPED_TRACE(bad->configured_zone);
        SCOPED_TRACE(bad->from_utc_ms);
        SCOPED_TRACE(bad->transitions.size());
        SyntheticZones zones;
        zones.incoherent_reply = *bad;
        const auto entry = std::get<ScheduledPauseProgress>(ready.enter(zones, noon));
        expect_hold(entry, noon, observation(noon));
        EXPECT_FALSE(entry.cursor.evaluated_through_utc_ms);
    }

    // The old epoch requires r1. Current facts tagged r2 can be coherent as a
    // current view, but must not be accepted as the archived view.
    SyntheticZones mixed;
    mixed.views = {fixed(utc(2026y / January / 12), 0, "Fixed/Zero", "fixed-r2")};
    mixed.ignore_required_revision = true;
    const auto old = config({a});
    Basis archived(noon, old, TimerLifecycle::Ready);
    archived.epochs = {{old, {}, noon, "fixed-r1"}, {old, noon, {}, {}}};
    const auto at = jan(12h + 5min);
    const auto unproven = std::get<ScheduledPauseProgress>(archived.enter(mixed, at));
    expect_hold(unproven, at, observation(at));
    EXPECT_TRUE(unproven.new_deliveries.empty());

    SyntheticZones complete;
    complete.views = {fixed(jan10_noon)};
    auto future = rule("future", 13h, 72h);
    future.recurrence.weekdays = {false, false, false, true, false, false, false};
    Basis empty(noon - 1, config({future}), TimerLifecycle::Ready);
    const auto certified = std::get<ScheduledPauseProgress>(empty.enter(complete, noon));
    EXPECT_TRUE(certified.new_deliveries.empty());
    EXPECT_TRUE(certified.pause.reasons.empty());
    expect_clear(certified);
}

TEST(ScheduledPauseAcceptance, SnapshotAnchorAndObservationBindingVersusCertifiedEmptyPreview) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto current = config({a});
    Basis basis(noon - 1, current, TimerLifecycle::Ready);
    const auto cuts = {cut(noon, std::nullopt)};
    const auto through = jan(12h + 5min);
    auto wrong_snapshot = basis.clock(cuts);
    wrong_snapshot.source_snapshot_ref = "mismatched-snapshot";
    expect_refusal(
        project_scheduled_pause_edit(basis.input(), wrong_snapshot, current, through, zones),
        ScheduledPauseRefusal::stale_observation);

    auto wrong_anchor = basis.clock(cuts);
    wrong_anchor.anchor.observation_ref = "a-different-anchor-at-the-same-wall-time";
    expect_refusal(
        project_scheduled_pause_edit(basis.input(), wrong_anchor, current, through, zones),
        ScheduledPauseRefusal::stale_observation);

    // Here both snapshot and anchor match. The already applied observation is stale.
    const auto stale_cuts = {cut(noon - 1, std::nullopt)};
    expect_refusal(project_scheduled_pause_edit(basis.input(), basis.clock(stale_cuts), current,
                                                through, zones),
                   ScheduledPauseRefusal::stale_observation);

    const auto empty =
        std::get<ScheduledPausePreview>(basis.preview(zones, current, cuts, through));
    expect_complete(empty, through);
    EXPECT_TRUE(empty.upcoming.empty());
    EXPECT_TRUE(empty.cutover_effects.empty());
    ASSERT_TRUE(empty.pause_after_cutover);
    EXPECT_TRUE(empty.pause_after_cutover->reasons.empty());
}

TEST(ScheduledPauseAcceptance, CutoverEffectsMoveWithTheTrustedObservation) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto old = config({a, b});
    const auto proposed = config({b});
    Basis basis(noon - 1, old);
    overlapping(basis);
    const auto end = jan(12h + 20min);
    for (const auto at : {noon, noon + 1'000, jan(12h + 10min)}) {
        SCOPED_TRACE(at);
        // Independent Manual remains in force for the whole interval.
        const auto preview = std::get<ScheduledPausePreview>(
            basis.preview(zones, proposed, {cut(at, milliseconds{at - (noon - 1)}, manual)}, end));
        expect_complete(preview, end);
        expect_observation(preview.as_of, observation(at));
        expect_effect(preview, a_window,
                      stopped(ScheduledPauseStopCause::deleted, at, observation(at)));
        if (at < b_window.window.end_utc_ms) {
            ASSERT_EQ(preview.cutover_effects.size(), 2U);
            expect_effect(preview, b_window);
            ASSERT_TRUE(preview.pause_after_cutover);
            EXPECT_EQ(preview.pause_after_cutover->reasons, manual_schedule.reasons);
        } else {
            ASSERT_EQ(preview.cutover_effects.size(), 1U);
            ASSERT_TRUE(preview.pause_after_cutover);
            EXPECT_EQ(preview.pause_after_cutover->reasons, manual.reasons);
        }
    }
}

std::vector<DeliveredScheduledPause> prior_daily_deliveries(std::size_t count) {
    std::vector<DeliveredScheduledPause> result;
    result.reserve(count);
    // These are complete, already stopped occurrences of a midnight rule.
    // Every key is unique and even the newest stop precedes this basis.
    const auto last = sys_days{2026y / January / 7};
    for (std::size_t i = 0; i < count; ++i) {
        const auto day = year_month_day{last - days{static_cast<days::rep>(count - 1 - i)}};
        const auto start = utc(day);
        result.push_back(
            delivery(key("retained", day, 0ms), start, start + 1,
                     stopped(ScheduledPauseStopCause::expired, start + 1, observation(start + 1))));
    }
    return result;
}

TEST(ScheduledPauseAcceptance, PreviewAndRetainedLedgerRejectOverLimitInput) {
    SyntheticZones zones;
    const auto current = config({rule("retained", 0ms, 1ms)});
    Basis ready(noon - 1, current, TimerLifecycle::Ready);
    const auto beyond = noon + duration_cast<milliseconds>(days{366}).count() + 1;
    expect_refusal(ready.preview(zones, current, {cut(noon, std::nullopt)}, beyond),
                   ScheduledPauseRefusal::limit_exceeded);
    EXPECT_EQ(zones.reads, 0U);

    Basis oversized(noon - 1, current, TimerLifecycle::Ready);
    oversized.ledger.deliveries = prior_daily_deliveries(16'385);
    expect_refusal(oversized.enter(zones, noon), ScheduledPauseRefusal::limit_exceeded);
    EXPECT_EQ(zones.reads, 0U);
}

TEST(ScheduledPauseAcceptance, MissingLedgerAndBoundedSourcePreserveOnlyCertifiedWork) {
    SyntheticZones zones;
    zones.views = {fixed()};
    Basis missing(anchor, known_rule);
    missing.ledger_known = false;
    const auto held =
        std::get<ScheduledPauseProgress>(missing.progress(zones, {cut(known_start, 300'000ms)}));
    expect_hold(held, anchor, observation(known_start), ScheduledPauseKnowledgeUnavailable::ledger);
    EXPECT_EQ(held.timer.main_remaining_ms, 900'000);
    EXPECT_TRUE(held.new_deliveries.empty());
    EXPECT_TRUE(held.stops.empty());

    const auto preview = std::get<ScheduledPausePreview>(
        missing.preview(zones, known_rule, {cut(known_start, 300'000ms)}, target));
    EXPECT_TRUE(preview.incomplete);
    EXPECT_FALSE(preview.pause_after_cutover);
    Basis deadline_without_ledger(anchor, known_rule);
    deadline_without_ledger.ledger_known = false;
    deadline_without_ledger.state.timer_config.hard_end_utc_ms = known_start;
    const auto terminal = std::get<ScheduledPauseProgress>(
        deadline_without_ledger.progress(zones, {cut(known_start, std::nullopt)}));
    expect_finish(terminal, 900'000, known_start);
    EXPECT_TRUE(terminal.new_deliveries.empty());

    // The source can certify through noon and refuses work after that saved
    // prefix. This exercises bounded-source failure without pinning how the
    // scheduler batches its own source and CORE-004 queries.
    zones.views = {fixed(jan10_noon)};
    zones.unavailable = ScheduleZoneUnavailable::limit_exceeded;
    Basis prefix(anchor, known_rule);
    const auto kept = std::get<ScheduledPauseProgress>(prefix.progress(
        zones, {cut(known_start, 300'000ms), cut(noon + 1, 300'001ms), cut(target, 59'999ms)}));
    EXPECT_LE(zones.reads, ScheduledPauseLimits::max_projection_queries);
    EXPECT_EQ(kept.starts_resolved_through_utc_ms, noon);
    EXPECT_EQ(kept.cursor.evaluated_through_utc_ms, noon + 1);
    expect_hold(kept, noon + 1, observation(target));
    expect_single_delivery(kept, known);
    EXPECT_EQ(kept.next_known_expiry_utc_ms, known_end);
    EXPECT_EQ(kept.timer.main_remaining_ms, 600'000);

    // Independently restore the last certified endpoint and deny all further
    // source work. The refusal cannot undo the delivery or the prior elapsed.
    SyntheticZones exhausted;
    exhausted.unavailable = ScheduleZoneUnavailable::limit_exceeded;
    Basis at_frontier(noon + 1, known_rule);
    at_frontier.state.timer.main_remaining_ms = 600'000;
    at_frontier.state.pause = schedule;
    at_frontier.ledger.deliveries = {known};
    const auto refused =
        std::get<ScheduledPauseProgress>(at_frontier.progress(exhausted, {cut(target, 59'999ms)}));
    EXPECT_EQ(refused.cursor.evaluated_through_utc_ms, noon + 1);
    expect_hold(refused, noon + 1, observation(target), ScheduleZoneUnavailable::limit_exceeded);
    EXPECT_TRUE(refused.new_deliveries.empty());
    EXPECT_EQ(refused.next_known_expiry_utc_ms, known_end);
    EXPECT_EQ(refused.timer.main_remaining_ms, 600'000);
}

TEST(ScheduledPauseAcceptance, RetentionCapCannotSplitATieAndDeadlineWinsFirst) {
    SyntheticZones zones;
    zones.views = {fixed()};
    const auto tie =
        config({rule("retained", 0ms, 1ms), rule("Z", 12h, 10min), rule("A", 12h, 5min)});
    Basis basis(anchor, tie);
    const auto cuts = {cut(noon - 1, 599'999ms), cut(noon, 1ms)};
    const auto both = std::get<ScheduledPauseProgress>(basis.progress(zones, cuts));
    ASSERT_EQ(both.new_deliveries.size(), 2U);
    for (const auto &expected : {delivery(key("Z", jan8, 12h), noon, jan(12h + 10min)),
                                 delivery(key("A", jan8, 12h), noon, jan(12h + 5min))}) {
        const auto actual =
            std::ranges::find(both.new_deliveries, expected.key, &DeliveredScheduledPause::key);
        ASSERT_NE(actual, both.new_deliveries.end());
        expect_delivery(*actual, expected);
    }
    EXPECT_EQ(both.pause.reasons, schedule.reasons);
    EXPECT_EQ(both.timer.main_remaining_ms, 300'000);
    expect_clear(both);

    Basis full(anchor, tie);
    full.ledger.deliveries = prior_daily_deliveries(16'383);
    const auto saturated = std::get<ScheduledPauseProgress>(full.progress(zones, cuts));
    expect_hold(saturated, noon, observation(noon), ScheduledPauseKnowledgeUnavailable::ledger);
    EXPECT_TRUE(saturated.new_deliveries.empty());
    EXPECT_TRUE(saturated.stops.empty());
    EXPECT_TRUE(saturated.pause.reasons.empty());
    EXPECT_EQ(saturated.cursor.evaluated_through_utc_ms, noon);
    EXPECT_EQ(saturated.timer.main_remaining_ms, 300'000);

    Basis at_deadline(anchor, tie);
    at_deadline.ledger.deliveries = full.ledger.deliveries;
    at_deadline.state.timer_config.hard_end_utc_ms = noon;
    const auto terminal = std::get<ScheduledPauseProgress>(at_deadline.progress(zones, cuts));
    expect_finish(terminal, 300'001, noon);
    EXPECT_EQ(terminal.cursor.evaluated_through_utc_ms, noon - 1);
    EXPECT_TRUE(terminal.new_deliveries.empty());
    EXPECT_TRUE(terminal.stops.empty());
}

// Expiry observation example.
TEST(ScheduledPauseAcceptance, ExpiryKeepsEffectiveTimeAndFirstReconcilingObservation) {
    SyntheticZones zones;
    zones.views = {fixed()};
    Basis basis(target, known_rule);
    basis.state.timer.main_remaining_ms = 600'000;
    basis.state.pause = schedule;
    basis.ledger.deliveries = {known};
    auto first = cut(jan(12h + 3min), std::nullopt);
    first.observation.observation_ref = "first-trusted-wall-after-expiry";
    const auto last = cut(jan(12h + 5min), std::nullopt);
    const auto result = std::get<ScheduledPauseProgress>(basis.progress(zones, {first, last}));
    expect_single_stop(result, known.key,
                       stopped(ScheduledPauseStopCause::expired, known_end, first.observation));
    expect_observation(result.cursor.last_observation, last.observation);
    EXPECT_TRUE(result.pause.reasons.empty());
    EXPECT_FALSE(result.next_known_expiry_utc_ms);
    EXPECT_EQ(result.timer.main_remaining_ms, 600'000);
}

} // namespace
