// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <subathon/core/recurring_schedule.hpp>
#include <vector>

namespace {
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace subathon::core;

constexpr std::int64_t utc_ms(year_month_day date, milliseconds time = 0ms) {
    return duration_cast<milliseconds>(sys_days{date}.time_since_epoch()).count() + time.count();
}

constexpr ScheduleLocalTime local_ms(year_month_day date, milliseconds time) {
    return local_days{date} + time;
}

std::vector<std::int64_t> start_times(const std::vector<ScheduleOccurrence> &occurrences) {
    std::vector<std::int64_t> starts;
    for (const auto &occurrence : occurrences) {
        starts.push_back(occurrence.start_utc_ms);
    }
    return starts;
}

constexpr auto year_start = utc_ms(2024y / January / 1);
constexpr auto year_end = utc_ms(2025y / January / 1);
constexpr auto spring_day = 2024y / March / 31;
constexpr auto autumn_day = 2024y / October / 27;
constexpr std::array seasonal_transitions{
    ScheduleZoneTransition{utc_ms(spring_day, 1h), 7'200},
    ScheduleZoneTransition{utc_ms(autumn_day, 1h), 3'600},
};
constexpr ScheduleZoneRules seasonal_zone{year_start, year_end, 3'600, seasonal_transitions};
constexpr ScheduleZoneRules utc_zone{year_start, year_end, 0};

TEST(RecurringScheduleAcceptance, DefaultGapShiftsByAnHourWithoutChangingNextDay) {
    const RecurringSchedule schedule{.local_time = 2h + 20min};
    const auto next_day = 2024y / April / 1;
    const auto preview = preview_schedule_occurrences(schedule, seasonal_zone,
                                                      {utc_ms(spring_day), utc_ms(next_day, 3h)});

    // 02:20 shifts to 03:20 at +02:00. The following day's 02:20 is ordinary.
    const std::vector<ScheduleOccurrence> expected{
        {{local_ms(spring_day, 2h + 20min), ScheduleFoldRun::First},
         utc_ms(spring_day, 1h + 20min),
         local_ms(spring_day, 3h + 20min)},
        {{local_ms(next_day, 2h + 20min), ScheduleFoldRun::First},
         utc_ms(next_day, 20min),
         local_ms(next_day, 2h + 20min)},
    };
    EXPECT_EQ(preview, expected);
}

TEST(RecurringScheduleAcceptance, GapShiftUsesActualOffsetDelta) {
    constexpr std::array transitions{ScheduleZoneTransition{utc_ms(spring_day, 2h), 1'800}};
    const ScheduleZoneRules half_hour_zone{year_start, year_end, 0, transitions};
    const RecurringSchedule schedule{.local_time = 2h + 10min + 125ms};
    const auto preview = preview_schedule_occurrences(
        schedule, half_hour_zone, {utc_ms(spring_day, 2h), utc_ms(spring_day, 4h)});

    // 02:10:00.125 becomes 02:40:00.125 at +00:30.
    const std::vector<ScheduleOccurrence> expected{
        {{local_ms(spring_day, 2h + 10min + 125ms), ScheduleFoldRun::First},
         utc_ms(spring_day, 2h + 10min + 125ms),
         local_ms(spring_day, 2h + 40min + 125ms)},
    };
    EXPECT_EQ(preview, expected);
}

TEST(RecurringScheduleAcceptance, SkipOmitsOnlyTheMissingLocalOccurrence) {
    const RecurringSchedule schedule{.local_time = 2h + 20min,
                                     .gap_policy = ScheduleGapPolicy::Skip};
    const auto day_before = 2024y / March / 30;
    const auto day_after = 2024y / April / 1;
    const auto preview = preview_schedule_occurrences(schedule, seasonal_zone,
                                                      {utc_ms(day_before), utc_ms(day_after, 3h)});

    const std::vector expected{utc_ms(day_before, 1h + 20min), utc_ms(day_after, 20min)};
    EXPECT_EQ(start_times(preview), expected);
}

TEST(RecurringScheduleAcceptance, DefaultFoldRunsOnceAcrossSeparatePreviewRanges) {
    const RecurringSchedule schedule{.local_time = 2h + 20min};
    const auto start = utc_ms(autumn_day);
    const auto split = utc_ms(autumn_day, 1h);
    const auto end = utc_ms(autumn_day, 3h);
    const auto whole_day = preview_schedule_occurrences(schedule, seasonal_zone, {start, end});

    // 02:20 is 00:20Z at +02:00 or 01:20Z at +01:00. RunOnce selects 00:20Z.
    const auto nominal = local_ms(autumn_day, 2h + 20min);
    const std::vector<ScheduleOccurrence> expected{
        {{nominal, ScheduleFoldRun::First}, utc_ms(autumn_day, 20min), nominal},
    };
    EXPECT_EQ(whole_day, expected);

    EXPECT_EQ(preview_schedule_occurrences(schedule, seasonal_zone, {start, split}), expected);
    EXPECT_TRUE(preview_schedule_occurrences(schedule, seasonal_zone, {split, end}).empty());
}

TEST(RecurringScheduleAcceptance, RunTwiceReturnsBothInstantsInUtcOrder) {
    const RecurringSchedule schedule{.local_time = 2h + 20min,
                                     .fold_policy = ScheduleFoldPolicy::RunTwice};
    const auto early = utc_ms(autumn_day, 20min);
    const auto late = utc_ms(autumn_day, 1h + 20min);
    const auto nominal = local_ms(autumn_day, 2h + 20min);
    const ScheduleOccurrence first{{nominal, ScheduleFoldRun::First}, early, nominal};
    const ScheduleOccurrence second{{nominal, ScheduleFoldRun::Second}, late, nominal};
    const std::vector expected{first, second};
    EXPECT_EQ(preview_schedule_occurrences(schedule, seasonal_zone, {early - 1, late}), expected);

    // The lower bound excludes an instant that is already due; the upper bound includes it.
    EXPECT_EQ(preview_schedule_occurrences(schedule, seasonal_zone, {early, late}),
              std::vector{second});
}

TEST(RecurringScheduleAcceptance, WeeklyRecurrenceUsesTheLocalWeekdayAcrossUtcMidnight) {
    const RecurringSchedule mondays{.local_time = 15min,
                                    .weekdays = {true, false, false, false, false, false, false}};
    const ScheduleZoneRules east{year_start, year_end, 7'200};
    const auto first_sunday = 2024y / January / 7;
    const auto next_sunday = 2024y / January / 14;
    const auto preview = preview_schedule_occurrences(
        mondays, east, {utc_ms(first_sunday), utc_ms(2024y / January / 16)});

    const std::vector expected{utc_ms(first_sunday, 22h + 15min), utc_ms(next_sunday, 22h + 15min)};
    EXPECT_EQ(start_times(preview), expected);
    ASSERT_EQ(preview.size(), 2U);
    EXPECT_EQ(preview[0].local_start, local_ms(2024y / January / 8, 15min));
    EXPECT_EQ(preview[1].local_start, local_ms(2024y / January / 15, 15min));
}

TEST(RecurringScheduleAcceptance, TimezoneEditPreviewsFutureInstantsWithoutReplayingPastUtc) {
    const RecurringSchedule schedule{.local_time = 11h};
    const ScheduleZoneRules old_zone{year_start, year_end, -7'200};
    const ScheduleZoneRules proposed_zone{year_start, year_end, 7'200};
    const auto today = 2024y / January / 8;
    const auto tomorrow = 2024y / January / 9;
    const SchedulePreviewRange range{utc_ms(today, 12h), utc_ms(tomorrow, 12h)};

    EXPECT_EQ(start_times(preview_schedule_occurrences(schedule, old_zone, range)),
              std::vector{utc_ms(today, 13h)});
    // The edit would move today's slot to 09:00Z, before the 12:00Z observation.
    EXPECT_EQ(start_times(preview_schedule_occurrences(schedule, proposed_zone, range)),
              std::vector{utc_ms(tomorrow, 9h)});
    EXPECT_EQ(start_times(preview_schedule_occurrences(schedule, old_zone, range)),
              std::vector{utc_ms(today, 13h)});
}

TEST(RecurringScheduleAcceptance, TimezoneEditSuppressesDeliveredSlotShiftedByGap) {
    const RecurringSchedule schedule{.local_time = 2h + 20min};
    const ScheduleZoneRules proposed_zone{year_start, year_end, -7'200};
    const auto next_day = 2024y / April / 1;
    const auto nominal = local_ms(spring_day, 2h + 20min);
    const auto observation = utc_ms(spring_day, 2h);
    const auto old_preview =
        preview_schedule_occurrences(schedule, seasonal_zone, {utc_ms(spring_day), observation});

    // The previously delivered 03:20 local start still belongs to the nominal 02:20 slot.
    const std::vector<ScheduleOccurrence> delivered_occurrences{
        {{nominal, ScheduleFoldRun::First},
         utc_ms(spring_day, 1h + 20min),
         local_ms(spring_day, 3h + 20min)},
    };
    ASSERT_EQ(old_preview, delivered_occurrences);

    const SchedulePreviewRange range{observation, utc_ms(next_day, 5h)};
    const ScheduleOccurrence same_slot{
        {nominal, ScheduleFoldRun::First}, utc_ms(spring_day, 4h + 20min), nominal};
    const ScheduleOccurrence next_slot{{local_ms(next_day, 2h + 20min), ScheduleFoldRun::First},
                                       utc_ms(next_day, 4h + 20min),
                                       local_ms(next_day, 2h + 20min)};
    const std::vector both_slots{same_slot, next_slot};
    EXPECT_EQ(preview_schedule_occurrences(schedule, proposed_zone, range), both_slots);

    const std::array delivered{old_preview.front().id};
    EXPECT_EQ(preview_schedule_occurrences(schedule, proposed_zone, range, delivered),
              std::vector{next_slot});
}

TEST(RecurringScheduleAcceptance, TimezoneEditSuppressesOnlyTheDeliveredFoldRunWhenTwoRemain) {
    const RecurringSchedule schedule{.local_time = 2h + 20min,
                                     .fold_policy = ScheduleFoldPolicy::RunTwice};
    constexpr std::array transitions{
        ScheduleZoneTransition{utc_ms(autumn_day, 4h), -7'200},
    };
    const ScheduleZoneRules proposed_zone{year_start, year_end, -3'600, transitions};
    const SchedulePreviewRange range{utc_ms(autumn_day, 2h), utc_ms(autumn_day, 5h)};
    const auto nominal = local_ms(autumn_day, 2h + 20min);
    const ScheduleOccurrenceId first_id{nominal, ScheduleFoldRun::First};
    const ScheduleOccurrenceId second_id{nominal, ScheduleFoldRun::Second};
    const ScheduleOccurrence first{first_id, utc_ms(autumn_day, 3h + 20min), nominal};
    const ScheduleOccurrence second{second_id, utc_ms(autumn_day, 4h + 20min), nominal};

    // Delivery records from the earlier fold at 00:20Z and 01:20Z use the same keys.
    const std::vector both_runs{first, second};
    EXPECT_EQ(preview_schedule_occurrences(schedule, proposed_zone, range), both_runs);
    const std::array first_delivered{first_id};
    EXPECT_EQ(preview_schedule_occurrences(schedule, proposed_zone, range, first_delivered),
              std::vector{second});
    const std::array second_delivered{second_id};
    EXPECT_EQ(preview_schedule_occurrences(schedule, proposed_zone, range, second_delivered),
              std::vector{first});
    const std::array both_delivered{first_id, second_id};
    EXPECT_TRUE(
        preview_schedule_occurrences(schedule, proposed_zone, range, both_delivered).empty());
}

TEST(RecurringScheduleAcceptance, TimezoneEditDoesNotReplayASecondFoldRunAsASingleRun) {
    const RecurringSchedule schedule{.local_time = 2h + 20min,
                                     .fold_policy = ScheduleFoldPolicy::RunTwice};
    const ScheduleZoneRules proposed_zone{year_start, year_end, -7'200};
    const SchedulePreviewRange range{utc_ms(autumn_day, 2h), utc_ms(autumn_day, 5h)};
    const auto nominal = local_ms(autumn_day, 2h + 20min);
    const std::vector<ScheduleOccurrence> expected{
        {{nominal, ScheduleFoldRun::First}, utc_ms(autumn_day, 4h + 20min), nominal},
    };
    EXPECT_EQ(preview_schedule_occurrences(schedule, proposed_zone, range), expected);

    const std::array delivered{ScheduleOccurrenceId{nominal, ScheduleFoldRun::Second}};
    EXPECT_TRUE(preview_schedule_occurrences(schedule, proposed_zone, range, delivered).empty());
}

TEST(RecurringScheduleAcceptance, RejectsIncompleteZoneFactsAtPreviewBoundary) {
    const auto day = 2024y / January / 8;
    const ScheduleZoneRules incomplete{utc_ms(day), year_end, 0};
    const SchedulePreviewRange range{utc_ms(day, 1h), utc_ms(day, 3h)};

    EXPECT_THROW(static_cast<void>(preview_schedule_occurrences({}, incomplete, range)),
                 std::invalid_argument);
}

TEST(RecurringScheduleAcceptance, RejectsPreviewBeyondBoundedHorizon) {
    // The timezone covers the full padded range, so only the horizon is invalid.
    const ScheduleZoneRules wide_zone{utc_ms(2023y / December / 1), utc_ms(2025y / February / 1),
                                      0};
    const SchedulePreviewRange range{year_start, year_end + 1}; // 366 days + 1 ms.
    EXPECT_THROW(static_cast<void>(preview_schedule_occurrences({}, wide_zone, range)),
                 std::invalid_argument);
}

TEST(RecurringScheduleAcceptance, RejectsOversizedTransitionSnapshot) {
    std::array<ScheduleZoneTransition, 65> transitions{};
    auto at = year_start;
    std::int32_t offset = 0;
    for (auto &transition : transitions) {
        at += milliseconds{1h}.count();
        transition = {at, ++offset};
    }
    const ScheduleZoneRules oversized{year_start, year_end, 0, transitions};
    const auto day = 2024y / January / 8;
    EXPECT_THROW(static_cast<void>(
                     preview_schedule_occurrences({}, oversized, {utc_ms(day), utc_ms(day, 1h)})),
                 std::invalid_argument);
}

TEST(RecurringScheduleAcceptance, ElapsedPauseCrossingGapPreviewsActualLocalEnd) {
    const RecurringSchedule schedule{.local_time = 1h + 50min};
    const SchedulePreviewRange range{utc_ms(spring_day), utc_ms(spring_day, 1h)};
    const auto nominal = local_ms(spring_day, 1h + 50min);
    // 01:50 at +01:00 starts at 00:50Z. Thirty minutes later, 01:20Z is 03:20 at +02:00.
    const std::vector<ScheduledPauseOccurrence> expected{
        {{{nominal, ScheduleFoldRun::First}, utc_ms(spring_day, 50min), nominal},
         utc_ms(spring_day, 1h + 20min),
         local_ms(spring_day, 3h + 20min)},
    };
    EXPECT_EQ(preview_scheduled_pauses(schedule, seasonal_zone, range, 30min), expected);
}

TEST(RecurringScheduleAcceptance, PauseStartingInGapUsesResolvedStartOrSkips) {
    const RecurringSchedule schedule{.local_time = 2h + 20min};
    const RecurringSchedule skip{.local_time = 2h + 20min, .gap_policy = ScheduleGapPolicy::Skip};
    const SchedulePreviewRange range{utc_ms(spring_day), utc_ms(spring_day, 3h)};
    const auto nominal = local_ms(spring_day, 2h + 20min);
    // 02:20 moves to 03:20 at +02:00. After 30 minutes, the clock reads 03:50.
    const std::vector<ScheduledPauseOccurrence> expected{
        {{{nominal, ScheduleFoldRun::First},
          utc_ms(spring_day, 1h + 20min),
          local_ms(spring_day, 3h + 20min)},
         utc_ms(spring_day, 1h + 50min),
         local_ms(spring_day, 3h + 50min)},
    };
    EXPECT_EQ(preview_scheduled_pauses(schedule, seasonal_zone, range, 30min), expected);
    EXPECT_TRUE(preview_scheduled_pauses(skip, seasonal_zone, range, 30min).empty());
}

TEST(RecurringScheduleAcceptance, ElapsedPausesCrossingFoldPreviewEachActualLocalEnd) {
    const RecurringSchedule schedule{.local_time = 2h + 40min,
                                     .fold_policy = ScheduleFoldPolicy::RunTwice};
    const SchedulePreviewRange range{utc_ms(autumn_day), utc_ms(autumn_day, 2h)};
    const auto nominal = local_ms(autumn_day, 2h + 40min);
    const ScheduleOccurrenceId first_id{nominal, ScheduleFoldRun::First};
    // Both ends are at +01:00. The first pause ends at an earlier displayed local time.
    const ScheduledPauseOccurrence first{{first_id, utc_ms(autumn_day, 40min), nominal},
                                         utc_ms(autumn_day, 1h + 10min),
                                         local_ms(autumn_day, 2h + 10min)};
    const ScheduledPauseOccurrence second{
        {{nominal, ScheduleFoldRun::Second}, utc_ms(autumn_day, 1h + 40min), nominal},
        utc_ms(autumn_day, 2h + 10min),
        local_ms(autumn_day, 3h + 10min)};
    const std::vector both{first, second};
    EXPECT_EQ(preview_scheduled_pauses(schedule, seasonal_zone, range, 30min), both);
    const std::array delivered{first_id};
    EXPECT_EQ(preview_scheduled_pauses(schedule, seasonal_zone, range, 30min, delivered),
              std::vector{second});
}

TEST(RecurringScheduleAcceptance, RejectsNonpositiveElapsedPauseDuration) {
    const auto day = 2024y / January / 8;
    const SchedulePreviewRange range{utc_ms(day, 8h), utc_ms(day, 10h)};
    const RecurringSchedule schedule{.local_time = 9h};
    EXPECT_THROW(static_cast<void>(preview_scheduled_pauses(schedule, utc_zone, range, 0ms)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(preview_scheduled_pauses(schedule, utc_zone, range, -1ms)),
                 std::invalid_argument);
}

TEST(RecurringScheduleAcceptance, RejectsUnrepresentableElapsedPauseEnd) {
    const auto day = 2024y / January / 8;
    const SchedulePreviewRange range{utc_ms(day, 8h), utc_ms(day, 10h)};
    const RecurringSchedule schedule{.local_time = 9h};
    EXPECT_THROW(
        static_cast<void>(preview_scheduled_pauses(schedule, utc_zone, range, milliseconds::max())),
        std::invalid_argument);
}

TEST(RecurringScheduleAcceptance, PausePreviewIncludesEndAtZoneCoverageBoundary) {
    const auto day = 2024y / January / 8;
    const auto end_day = 2024y / January / 11;
    const SchedulePreviewRange range{utc_ms(day, 8h), utc_ms(day, 9h)};
    const RecurringSchedule schedule{.local_time = 9h};
    const ScheduleZoneRules complete{year_start, utc_ms(end_day, 9h), 0};
    const auto nominal = local_ms(day, 9h);
    const std::vector<ScheduledPauseOccurrence> expected{
        {{{nominal, ScheduleFoldRun::First}, utc_ms(day, 9h), nominal},
         utc_ms(end_day, 9h),
         local_ms(end_day, 9h)},
    };

    EXPECT_EQ(preview_scheduled_pauses(schedule, complete, range, 72h), expected);
}

TEST(RecurringScheduleAcceptance, RejectsZoneCoverageEndingBeforePauseEnd) {
    const auto day = 2024y / January / 8;
    const auto end_day = 2024y / January / 11;
    const SchedulePreviewRange range{utc_ms(day, 8h), utc_ms(day, 9h)};
    const RecurringSchedule schedule{.local_time = 9h};
    // The snapshot covers the 48-hour start padding but misses the last millisecond.
    const ScheduleZoneRules incomplete{year_start, utc_ms(end_day, 9h) - 1, 0};
    EXPECT_THROW(static_cast<void>(preview_scheduled_pauses(schedule, incomplete, range, 72h)),
                 std::invalid_argument);
}

TEST(RecurringScheduleAcceptance, PreviewedStartCanPauseAcrossUtcMidnight) {
    const RecurringSchedule schedule{.local_time = 23h + 45min};
    const auto day = 2024y / January / 8;
    const auto next_day = 2024y / January / 9;
    const auto start = utc_ms(day, 23h + 45min);
    const auto end = utc_ms(next_day, 15min);
    const auto preview = preview_scheduled_pauses(schedule, utc_zone,
                                                  {utc_ms(day, 23h), utc_ms(next_day, 1h)}, 30min);
    const auto nominal = local_ms(day, 23h + 45min);
    const std::vector<ScheduledPauseOccurrence> expected{
        {{{nominal, ScheduleFoldRun::First}, start, nominal}, end, local_ms(next_day, 15min)},
    };
    ASSERT_EQ(preview, expected);

    const std::array windows{
        ScheduledPauseWindow{preview.front().start.start_utc_ms, preview.front().end_utc_ms}};
    const auto before = reconcile_scheduled_pause({}, windows, start - 1);
    EXPECT_TRUE(before.reasons.empty());
    const auto at_start = reconcile_scheduled_pause(before, windows, start);
    EXPECT_EQ(at_start.reasons, PauseReasons{PauseReason::Schedule});
    const auto midnight = reconcile_scheduled_pause(at_start, windows, utc_ms(next_day));
    EXPECT_EQ(midnight.reasons, PauseReasons{PauseReason::Schedule});
    const auto at_end = reconcile_scheduled_pause(midnight, windows, end);
    EXPECT_TRUE(at_end.reasons.empty());
}

TEST(RecurringScheduleAcceptance, EndingSchedulePreservesIndependentReasonsAndSignalState) {
    const auto start = utc_ms(2024y / January / 8, 9h);
    const auto end = utc_ms(2024y / January / 8, 10h);
    const std::array windows{ScheduledPauseWindow{start, end}};
    const PauseState initial{
        .reasons = {PauseReason::Manual, PauseReason::StreamOffline, PauseReason::RecoveryReview},
        .client_disconnect_pending_ms = 4'000,
        .stream_unknown_pending_ms = 2'000,
        .stream_unknown_notice = true,
    };

    const auto paused = reconcile_scheduled_pause(initial, windows, start);
    const PauseReasons expected{PauseReason::Manual, PauseReason::StreamOffline,
                                PauseReason::RecoveryReview, PauseReason::Schedule};
    EXPECT_EQ(paused.reasons, expected);
    EXPECT_EQ(paused.stream_offline_pending_ms, initial.stream_offline_pending_ms);
    EXPECT_EQ(paused.client_disconnect_pending_ms, 4'000);
    EXPECT_EQ(paused.stream_unknown_pending_ms, 2'000);
    EXPECT_TRUE(paused.stream_unknown_notice);

    const auto ended = reconcile_scheduled_pause(paused, windows, end);
    EXPECT_EQ(ended.reasons, initial.reasons);
    EXPECT_EQ(ended.stream_offline_pending_ms, initial.stream_offline_pending_ms);
    EXPECT_EQ(ended.client_disconnect_pending_ms, 4'000);
    EXPECT_EQ(ended.stream_unknown_pending_ms, 2'000);
    EXPECT_TRUE(ended.stream_unknown_notice);
}

TEST(RecurringScheduleAcceptance, DuplicateDeliveryDoesNotExtendOrReactivateThePause) {
    const ScheduledPauseWindow occurrence{utc_ms(2024y / January / 8, 9h),
                                          utc_ms(2024y / January / 8, 10h)};
    const std::array one{occurrence};
    const std::array duplicates{occurrence, occurrence};
    const auto first = reconcile_scheduled_pause({}, one, occurrence.start_utc_ms);
    EXPECT_EQ(first.reasons, PauseReasons{PauseReason::Schedule});
    const auto duplicate = reconcile_scheduled_pause(first, duplicates, occurrence.start_utc_ms);
    EXPECT_EQ(duplicate.reasons, PauseReasons{PauseReason::Schedule});
    const auto ended = reconcile_scheduled_pause(duplicate, duplicates, occurrence.end_utc_ms);
    EXPECT_TRUE(ended.reasons.empty());
    const auto late_duplicate = reconcile_scheduled_pause(ended, one, occurrence.end_utc_ms + 1);
    EXPECT_TRUE(late_duplicate.reasons.empty());
}

TEST(RecurringScheduleAcceptance, OneWindowEndingCannotClearAnotherActiveWindow) {
    const auto day = 2024y / January / 8;
    const std::array windows{
        ScheduledPauseWindow{utc_ms(day, 9h), utc_ms(day, 10h)},
        ScheduledPauseWindow{utc_ms(day, 9h + 30min), utc_ms(day, 11h)},
    };
    const auto first = reconcile_scheduled_pause({}, windows, utc_ms(day, 9h));
    EXPECT_EQ(first.reasons, PauseReasons{PauseReason::Schedule});
    const auto still_paused = reconcile_scheduled_pause(first, windows, utc_ms(day, 10h));
    EXPECT_EQ(still_paused.reasons, PauseReasons{PauseReason::Schedule});
    EXPECT_TRUE(reconcile_scheduled_pause(still_paused, windows, utc_ms(day, 11h)).reasons.empty());
}

TEST(RecurringScheduleAcceptance, InvalidWindowCannotClearAnExistingSchedulePause) {
    const auto instant = utc_ms(2024y / January / 8, 10h);
    const std::array windows{ScheduledPauseWindow{instant, instant}};
    const PauseState paused{.reasons = {PauseReason::Schedule}};
    EXPECT_THROW(static_cast<void>(reconcile_scheduled_pause(paused, windows, instant)),
                 std::invalid_argument);
}
} // namespace
