// SPDX-License-Identifier: GPL-3.0-or-later
// Independent literal oracles: specification v0.45 §8/MILE-01/NUM-04; model MP01–22.
// Standard-library data only. No project header, calculator or generated expectation.
#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <span>
#include <string_view>

namespace milestone_vectors {
inline constexpr std::string_view track = "track";
inline constexpr std::string_view revision = "definition-current";
inline constexpr std::string_view context = "application-current";
inline constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
inline constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
inline constexpr auto distance_maximum = std::numeric_limits<std::uint64_t>::max();

struct Target {
    std::string_view id;
    std::int64_t value;
};
struct Completion {
    std::string_view id;
    std::string_view revision;
    std::int64_t target;
    std::int64_t score;
    std::string_view context;
};
struct Deficit {
    Target current;
    std::uint64_t amount;
};
struct Interval {
    std::uint64_t earned;
    std::uint64_t span;
    std::uint64_t numerator;
    std::uint64_t denominator;
};
enum class State { empty, before_first, between_targets, all_completed };
struct Projection {
    State state;
    std::span<const Deficit> deficits;
    std::optional<Target> previous;
    std::optional<Target> next;
    std::uint64_t track_deficit;
    std::optional<std::uint64_t> remaining;
    std::optional<Interval> interval;
};
struct Row {
    std::string_view label;
    std::int64_t score;
    std::span<const Target> targets;
    std::span<const Completion> before;
    std::span<const Completion> after;
    std::uint64_t begin;
    std::uint64_t count;
    Projection projection;
};
// Stable discovery/failure metadata, without pointer dumps or feature execution.
inline void PrintTo(const Row &row, std::ostream *output) { *output << row.label; }

inline constexpr std::array two{Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000}};
inline constexpr std::array three{Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000},
                                  Target{"C", 3'000'000'000}};
inline constexpr std::array old_a{
    Completion{"A", "definition-old", 1'000'000'000, 1'100'000'000, "application-old-A"}};
inline constexpr std::array old_ab{
    Completion{"A", "definition-old", 1'000'000'000, 1'100'000'000, "application-old-A"},
    Completion{"B", "definition-old", 2'000'000'000, 2'500'000'000, "application-old-B"}};
inline constexpr std::array earned_a{
    Completion{"A", revision, 1'000'000'000, 1'000'000'000, context}};
inline constexpr std::array jump_ab{
    Completion{"A", revision, 1'000'000'000, 2'500'000'000, context},
    Completion{"B", revision, 2'000'000'000, 2'500'000'000, context}};
inline constexpr std::array jump_abc{
    Completion{"A", "definition-old", 1'000'000'000, 1'100'000'000, "application-old-A"},
    Completion{"B", revision, 2'000'000'000, 3'500'000'000, context},
    Completion{"C", revision, 3'000'000'000, 3'500'000'000, context}};
inline constexpr std::array zero_a{Deficit{{"A", 1'000'000'000}, 0}};
inline constexpr std::array zero_ab{Deficit{{"A", 1'000'000'000}, 0},
                                    Deficit{{"B", 2'000'000'000}, 0}};
inline constexpr std::array zero_abc{Deficit{{"A", 1'000'000'000}, 0},
                                     Deficit{{"B", 2'000'000'000}, 0},
                                     Deficit{{"C", 3'000'000'000}, 0}};
inline constexpr std::array deficit_a{Deficit{{"A", 1'000'000'000}, 200'000'000}};
inline constexpr std::array deficit_ab{Deficit{{"A", 1'000'000'000}, 200'000'000},
                                       Deficit{{"B", 2'000'000'000}, 1'200'000'000}};
inline constexpr std::array manual_a{
    Completion{"A", "manual-definition", 1'000'000'000, -50'000'000, "manual-command"}};
inline constexpr std::array manual_deficit{Deficit{{"A", 1'000'000'000}, 1'050'000'000}};
inline constexpr std::array signed_targets{Target{"A", -100'000'000}, Target{"B", 0},
                                           Target{"C", 100'000'000}};
inline constexpr std::array signed_a{Completion{"A", revision, -100'000'000, -50'000'000, context}};
inline constexpr std::array signed_zero{Deficit{{"A", -100'000'000}, 0}};
inline constexpr std::array zero_targets{Target{"A", 0}, Target{"B", 100'000'000}};
inline constexpr std::array zero_earned{Completion{"A", revision, 0, 0, context}};
inline constexpr std::array zero_micro{Deficit{{"A", 0}, 0}};
inline constexpr std::array extremes{Target{"A", minimum}, Target{"B", maximum}};
inline constexpr std::array minimum_earned{Completion{"A", revision, minimum, 0, context}};
inline constexpr std::array minimum_zero{Deficit{{"A", minimum}, 0}};
inline constexpr std::array highest_target{Target{"A", maximum}};
inline constexpr std::array highest_completed{
    Completion{"A", "definition-old", maximum, maximum, "application-old-A"}};
inline constexpr std::array maximum_deficit{Deficit{{"A", maximum}, distance_maximum}};
inline constexpr std::array corrected{Target{"A", 1'200'000'000}, Target{"B", 2'000'000'000}};
inline constexpr std::array corrected_deficit{Deficit{{"A", 1'200'000'000}, 100'000'000}};
inline constexpr std::array thirds{Target{"A", 0}, Target{"B", 3}};
inline constexpr std::array third_before{Completion{"A", "old", 0, 0, "old-A"}};
inline constexpr std::array sole_zero{Target{"A", 0}};

// All amounts are explicit micro-points; no points-to-units calculator supplies an oracle.
inline constexpr std::array rows{
    Row{"MP01_Empty", -5'000'000, {}, {}, {}, 0, 0, {State::empty, {}, {}, {}, 0, {}, {}}},
    Row{"MP02_BeforeFirst",
        800'000'000,
        two,
        {},
        {},
        0,
        0,
        {State::before_first, {}, {}, Target{"A", 1'000'000'000}, 0, 200'000'000, {}}},
    Row{"MP03_ExactThreshold",
        1'000'000'000,
        two,
        {},
        earned_a,
        0,
        1,
        {State::between_targets, zero_a, Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000}, 0,
         1'000'000'000, Interval{0, 1'000'000'000, 0, 1}}},
    Row{"MP04_IntervalHalf",
        1'500'000'000,
        two,
        old_a,
        old_a,
        1,
        0,
        {State::between_targets, zero_a, Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000}, 0,
         500'000'000, Interval{500'000'000, 1'000'000'000, 1, 2}}},
    Row{"MP05_RefundDeficit",
        800'000'000,
        two,
        old_a,
        old_a,
        1,
        0,
        {State::between_targets, deficit_a, Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000},
         200'000'000, 1'200'000'000, Interval{0, 1'000'000'000, 0, 1}}},
    Row{"MP06_RecoveryAtPrevious",
        1'000'000'000,
        two,
        old_a,
        old_a,
        1,
        0,
        {State::between_targets, zero_a, Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000}, 0,
         1'000'000'000, Interval{0, 1'000'000'000, 0, 1}}},
    Row{"MP07_RecoveryAbovePrevious",
        1'100'000'000,
        two,
        old_a,
        old_a,
        1,
        0,
        {State::between_targets, zero_a, Target{"A", 1'000'000'000}, Target{"B", 2'000'000'000}, 0,
         900'000'000, Interval{100'000'000, 1'000'000'000, 1, 10}}},
    Row{"MP08_OrderedJump",
        2'500'000'000,
        three,
        {},
        jump_ab,
        0,
        2,
        {State::between_targets, zero_ab, Target{"B", 2'000'000'000}, Target{"C", 3'000'000'000}, 0,
         500'000'000, Interval{500'000'000, 1'000'000'000, 1, 2}}},
    Row{"MP09_JumpPreservesOldPrefix",
        3'500'000'000,
        three,
        old_a,
        jump_abc,
        1,
        2,
        {State::all_completed, zero_abc, Target{"C", 3'000'000'000}, {}, 0, {}, {}}},
    Row{"MP10_MultipleDeficitsAreNotSummed",
        800'000'000,
        three,
        old_ab,
        old_ab,
        2,
        0,
        {State::between_targets, deficit_ab, Target{"B", 2'000'000'000}, Target{"C", 3'000'000'000},
         1'200'000'000, 2'200'000'000, Interval{0, 1'000'000'000, 0, 1}}},
    Row{"MP11_ManualCompletionWithDeficit",
        -50'000'000,
        two,
        manual_a,
        manual_a,
        1,
        0,
        {State::between_targets, manual_deficit, Target{"A", 1'000'000'000},
         Target{"B", 2'000'000'000}, 1'050'000'000, 2'050'000'000,
         Interval{0, 1'000'000'000, 0, 1}}},
    Row{"MP12_NegativeTargets",
        -50'000'000,
        signed_targets,
        {},
        signed_a,
        0,
        1,
        {State::between_targets, signed_zero, Target{"A", -100'000'000}, Target{"B", 0}, 0,
         50'000'000, Interval{50'000'000, 100'000'000, 1, 2}}},
    Row{"MP13_ZeroTarget",
        0,
        zero_targets,
        {},
        zero_earned,
        0,
        1,
        {State::between_targets, zero_micro, Target{"A", 0}, Target{"B", 100'000'000}, 0,
         100'000'000, Interval{0, 100'000'000, 0, 1}}},
    Row{"MP14_FullWidthInterval",
        0,
        extremes,
        {},
        minimum_earned,
        0,
        1,
        {State::between_targets, minimum_zero, Target{"A", minimum}, Target{"B", maximum}, 0,
         9'223'372'036'854'775'807ULL,
         Interval{9'223'372'036'854'775'808ULL, 18'446'744'073'709'551'615ULL,
                  9'223'372'036'854'775'808ULL, 18'446'744'073'709'551'615ULL}}},
    Row{"MP15_FullWidthCompletedDeficit",
        minimum,
        highest_target,
        highest_completed,
        highest_completed,
        1,
        0,
        {State::all_completed,
         maximum_deficit,
         Target{"A", maximum},
         {},
         18'446'744'073'709'551'615ULL,
         {},
         {}}},
    Row{"MP16_FullWidthFirstRemaining",
        minimum,
        highest_target,
        {},
        {},
        0,
        0,
        {State::before_first, {}, {}, Target{"A", maximum}, 0, 18'446'744'073'709'551'615ULL, {}}},
    Row{"MP17_CurrentVersusCapturedTarget",
        1'100'000'000,
        corrected,
        old_a,
        old_a,
        1,
        0,
        {State::between_targets, corrected_deficit, Target{"A", 1'200'000'000},
         Target{"B", 2'000'000'000}, 100'000'000, 900'000'000, Interval{0, 800'000'000, 0, 1}}},
    Row{"MP18_ExactThird",
        1,
        thirds,
        third_before,
        third_before,
        1,
        0,
        {State::between_targets, zero_micro, Target{"A", 0}, Target{"B", 3}, 0, 2,
         Interval{1, 3, 1, 3}}},
    Row{"MP21_BelowNegativeFirstTarget",
        -150'000'000,
        signed_targets,
        {},
        {},
        0,
        0,
        {State::before_first, {}, {}, Target{"A", -100'000'000}, 0, 50'000'000, {}}},
    Row{"MP22_SoleZeroTarget",
        0,
        sole_zero,
        {},
        zero_earned,
        0,
        1,
        {State::all_completed, zero_micro, Target{"A", 0}, {}, 0, {}, {}}},
};
} // namespace milestone_vectors
