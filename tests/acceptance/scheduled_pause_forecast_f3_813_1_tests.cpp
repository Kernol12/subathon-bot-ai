// SPDX-License-Identifier: GPL-3.0-or-later
// Unregistered F3 acceptance fixture. See oracle.md for decision 270 A and derivation.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
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

constexpr Instant utc(year_month_day date, milliseconds time = 0ms) {
    return duration_cast<milliseconds>(sys_days{date}.time_since_epoch()).count() + time.count();
}
constexpr Instant after(Instant instant, milliseconds by) { return instant + by.count(); }
constexpr auto january8 = 2026y / January / 8;
constexpr Instant cut_time = utc(january8, 12h);
constexpr Instant lower = after(cut_time, -48h);

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

// Record legacy calls and return only the explicitly supplied response.
class LegacyZone : public ScheduleZoneSource {
  public:
    std::optional<LegacyReply> scripted;
    std::vector<ScheduleZoneRequest> legacy_requests;

    LegacyReply read(const ScheduleZoneRequest &request) override {
        legacy_requests.push_back(request);
        return scripted ? *scripted : LegacyReply{ZoneError::insufficient_coverage};
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

  private:
    std::vector<ScheduledPauseEpoch> epochs_;
    ScheduledPauseLedger ledger_{"timer", "deliveries", {}};
    ScheduledPauseState state_;
};

void expect_forecast(const ScheduledPausePreview &value, const ScheduledPauseObservation &at,
                     Instant horizon, Instant frontier,
                     std::optional<ForecastFailure> incomplete = {}) {
    EXPECT_EQ(value.as_of.utc_ms, at.utc_ms);
    EXPECT_EQ(value.as_of.observation_ref, at.observation_ref);
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

struct F3Basis {
    std::array<ScheduledPauseEpoch, 1> epochs;
    ScheduledPauseLedger ledger{"timer", "deliveries", {}};
    ScheduledPauseState state;

    F3Basis(Instant anchor, ScheduledPauseConfiguration old)
        : epochs{{{std::move(old), {}, {}, {}}}},
          state{.snapshot_ref = "f3-snapshot",
                .timer_lineage = "timer",
                .clock_lineage = "f3-clock",
                .state_revision = "f3-ready",
                .configuration_revision = "f3-configuration",
                .timer_config = {.end_mode = TimerEndMode::Immediate, .hard_end_utc_ms = {}},
                .timer = {.lifecycle = TimerLifecycle::Ready, .main_remaining_ms = 900'000},
                .pause = {},
                .cursor = {observation(anchor), {}, {}},
                .epochs = epochs,
                .epoch_history_complete = true,
                .ledger = &ledger} {}

    ScheduledPauseClockInput clock(std::span<const ScheduledPauseElapsedCut> cuts) const {
        return {state.snapshot_ref, state.clock_lineage, state.cursor.last_observation, cuts};
    }
};

class F3WorkSource final : public ScheduleZoneSource {
  public:
    std::size_t legacy_errors = 0;
    bool error_on_page_64 = false;
    std::vector<ScheduleZoneRequest> legacy_requests;
    std::vector<ScheduleZoneRequest> page_requests;

    LegacyReply read(const ScheduleZoneRequest &request) override {
        legacy_requests.push_back(request);
        if (legacy_requests.size() <= legacy_errors) {
            return ZoneError::source_unavailable;
        }
        return facts(request.configured_zone, "old-r1", request.from_utc_ms,
                     request.through_utc_ms);
    }

    PageReply read_forecast_page(const ScheduleZoneRequest &request) override {
        const auto page = page_requests.size();
        page_requests.push_back(request);
        if (page >= 64 || (page == 63 && error_on_page_64)) {
            return ZoneError::source_unavailable;
        }
        constexpr auto first_change = after(cut_time, days{3});
        const auto first = after(first_change, minutes{static_cast<minutes::rep>(page) * 128});
        const auto next = after(first, 64 * 2min);
        return ForecastPage{facts("F3/Dense", "dense-r1", page == 0 ? lower : first - 1, next - 1,
                                  0, alternating(first, 2min, 64, 1)),
                            TransitionCap{}};
    }
};

void expect_f3_work_case(std::size_t legacy_errors, bool error_on_page_64, std::size_t page_reads,
                         Instant frontier, ForecastFailure failure, std::size_t rows,
                         std::optional<std::size_t> validated_pages) {
    const auto activation = cut_time - static_cast<Instant>(legacy_errors + 1);
    F3Basis basis(activation - 1,
                  configuration("F3/Old", {rule("old-a", 15h, 1ms), rule("old-b", 15h, 1ms)}));
    LegacyZone priming;
    priming.scripted = ZoneError::source_unavailable;
    const ScheduledPauseEntry entry{
        basis.state.snapshot_ref,
        observation(activation),
        {.lifecycle = TimerLifecycle::Active, .main_remaining_ms = 900'000}};
    const auto entered = enter_scheduled_pauses(basis.state, entry, priming);
    const auto *held = std::get_if<ScheduledPauseProgress>(&entered);
    ASSERT_NE(held, nullptr);
    ASSERT_TRUE(held->cursor.hold);
    ASSERT_EQ(held->cursor.hold->cause,
              ScheduledPauseCoverageFailure{ZoneError::source_unavailable});
    ASSERT_EQ(priming.legacy_requests.size(), 1U);
    ASSERT_TRUE(held->new_deliveries.empty());
    ASSERT_TRUE(held->stops.empty());
    basis.state.snapshot_ref = "f3-active-snapshot";
    basis.state.state_revision = "f3-active";
    basis.state.timer = held->timer;
    basis.state.pause = held->pause;
    basis.state.cursor = held->cursor;

    std::vector<ScheduledPauseElapsedCut> cuts;
    for (Instant now = activation + 1; now <= cut_time; ++now) {
        cuts.push_back(elapsed_cut(now, 1ms));
    }
    F3WorkSource source;
    source.legacy_errors = legacy_errors;
    source.error_on_page_64 = error_on_page_64;
    const auto proposed = configuration("F3/Dense", {rule("daily", 12h, 1ms)});
    const auto horizon = after(cut_time, days{10});
    const auto projected =
        project_scheduled_pause_edit(basis.state, basis.clock(cuts), proposed, horizon, source);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);
    expect_forecast(*preview, observation(cut_time), horizon, frontier, failure);
    ASSERT_TRUE(preview->pause_after_cutover);
    EXPECT_TRUE(preview->pause_after_cutover->reasons.empty());
    EXPECT_TRUE(preview->cutover_effects.empty());

    ASSERT_EQ(source.legacy_requests.size(), legacy_errors + 1);
    for (std::size_t i = 0; i < source.legacy_requests.size(); ++i) {
        const auto now = activation + static_cast<Instant>(i + 1);
        const auto &request = source.legacy_requests[i];
        EXPECT_EQ(request.configured_zone, "F3/Old");
        EXPECT_EQ(request.from_utc_ms, after(now - 1, -48h));
        EXPECT_EQ(request.through_utc_ms, after(now, 48h));
        EXPECT_FALSE(request.required_rules_revision);
    }
    std::vector<Instant> page_starts{lower};
    for (std::size_t i = 1; i < page_reads; ++i) {
        page_starts.push_back(
            after(cut_time, days{3} + minutes{static_cast<minutes::rep>(i) * 128}) - 1);
    }
    ASSERT_EQ(source.page_requests.size(), page_starts.size());
    for (std::size_t i = 0; i < page_starts.size(); ++i) {
        const auto &request = source.page_requests[i];
        EXPECT_EQ(request.configured_zone, "F3/Dense");
        EXPECT_EQ(request.from_utc_ms, page_starts[i]);
        EXPECT_EQ(request.through_utc_ms, after(horizon, 48h));
        EXPECT_EQ(request.required_rules_revision,
                  i == 0 ? std::nullopt : std::optional<std::string>{"dense-r1"});
    }
    ASSERT_EQ(preview->upcoming.size(), rows);
    for (std::size_t i = 0; i < rows; ++i) {
        const auto utc_start = after(cut_time, days{static_cast<days::rep>(i + 1)});
        const auto local_start = utc_start + (i >= 2 ? 1'000 : 0);
        expect_row(preview->upcoming[i], key("daily", utc_start), utc_start, utc_start + 1,
                   local_start, local_start + 1);
    }
    ASSERT_FALSE(preview->facts.empty());
    expect_fact(preview->facts.front(),
                facts("F3/Old", "old-r1", after(cut_time - 1, -48h), after(cut_time, 48h)), 0U);
    if (validated_pages) {
        ASSERT_EQ(preview->facts.size(), *validated_pages + 1);
        for (std::size_t i = 0; i < *validated_pages; ++i) {
            EXPECT_FALSE(preview->facts[i + 1].old_epoch_index);
            EXPECT_EQ(preview->facts[i + 1].facts.from_utc_ms, page_starts[i]);
            EXPECT_EQ(preview->facts[i + 1].facts.rules_revision, "dense-r1");
        }
    }
}

constexpr auto f3_63_frontier = after(cut_time, days{6} + 14h + 24min) - 1;
constexpr auto f3_64_frontier = after(cut_time, days{6} + 16h + 32min) - 1;

TEST(ScheduledPauseForecastF3, CapCertificateRequiresWorkForJoinedValidation) {
    const ForecastFailure support = SupportError::beyond_supported_timezone_transitions;
    expect_f3_work_case(4027, false, 64, f3_64_frontier, support, 6, 64);
    expect_f3_work_case(4028, false, 64, cut_time, support, 0, 64);
    expect_f3_work_case(4029, false, 64, cut_time, coverage(ZoneError::limit_exceeded), 0, {});
    expect_f3_work_case(4030, false, 63, cut_time, coverage(ZoneError::limit_exceeded), 0, {});
}

TEST(ScheduledPauseForecastF3, FailedForecastDispatchUsesTheLastExpansionQuery) {
    expect_f3_work_case(4027, true, 64, f3_63_frontier, coverage(ZoneError::source_unavailable), 6,
                        63);
    expect_f3_work_case(4028, true, 64, cut_time, coverage(ZoneError::limit_exceeded), 0, 63);
}

TEST(ScheduledPauseForecastF3, ResultBudgetEqualityAndOneByteOverKeepWholeUtcGroups) {
    using Limits = ScheduledPauseLimits;
    constexpr std::size_t rules = 128;
    constexpr std::size_t dates = 32;
    const std::string zone_name = "F3/Zero";
    const auto horizon = after(cut_time, days{dates});
    const F3Basis basis(cut_time - 1, configuration("F3/Old"));
    const std::array cuts{elapsed_cut(cut_time, 1ms)};

    // Decision 270 A applies the shipped #720 reserve formula to the widened DTO.
    constexpr auto reserve = sizeof(ScheduledPauseProgress) + sizeof(ScheduledPausePreview) +
                             (9 + 7) * Limits::max_reference_bytes +
                             6 * sizeof(ScheduledPauseTransitions) + 12 * sizeof(TimerTransition) +
                             (6 + 6) * sizeof(PauseReason);
    const auto fixed =
        reserve + rules * sizeof(ScheduledPauseRule) + 2 * zone_name.size() +
        sizeof(ScheduledPauseFactBinding) + sizeof(ScheduleZoneTransition) +
        rules * dates * (sizeof(ScheduledPausePreviewRow) + basis.state.timer_lineage.size());
    ASSERT_LT(fixed + 1, Limits::max_result_bytes);
    const auto variable = Limits::max_result_bytes - fixed - 1;
    const auto ids_total = variable / (dates + 1);
    const auto revision_length = 1 + variable % (dates + 1);
    ASSERT_GE(ids_total / rules, 3U);
    ASSERT_LE((ids_total + rules - 1) / rules, Limits::max_reference_bytes);

    auto proposed = configuration(zone_name);
    for (std::size_t i = 0; i < rules; ++i) {
        auto id = std::to_string(i);
        id.resize(ids_total / rules + (i < ids_total % rules), 'x');
        proposed.rules.push_back(rule(std::move(id), 12h, 1ms));
    }

    for (const bool one_over : {false, true}) {
        SCOPED_TRACE(one_over);
        PagedZone source;
        const std::string revision(revision_length + one_over, 'r');
        source.replies.emplace_back(ForecastPage{
            facts(zone_name, revision, lower, after(horizon, 48h), 0, {{horizon + 2, 1}}),
            ReachedRequestEnd{}});
        const auto projected =
            project_scheduled_pause_edit(basis.state, basis.clock(cuts), proposed, horizon, source);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        const auto failure =
            one_over ? std::optional{coverage(ZoneError::limit_exceeded)} : std::nullopt;
        expect_forecast(*preview, observation(cut_time), horizon, horizon - one_over, failure);
        ASSERT_TRUE(preview->pause_after_cutover);
        EXPECT_TRUE(source.legacy_requests.empty());
        expect_page_requests(source, zone_name, after(horizon, 48h), {lower}, revision);
        ASSERT_EQ(preview->facts.size(), 1U);
        expect_fact(preview->facts.front(), page_facts(source.replies.front()));
        ASSERT_EQ(preview->upcoming.size(), rules * (dates - one_over));
        for (std::size_t i = 0; i < preview->upcoming.size(); ++i) {
            const auto utc_start = after(cut_time, days{static_cast<days::rep>(i / rules + 1)});
            expect_row(preview->upcoming[i], key(proposed.rules[i % rules].schedule_id, utc_start),
                       utc_start, utc_start + 1, utc_start, utc_start + 1);
        }
    }
}

// #720 initial byte ledger for these single-open-epoch, empty-ledger,
// empty-pause fixtures. Cut observation text is the sole variable padding.
std::size_t f3_fixed_input(const ScheduledPauseState &state, const ScheduledPauseClockInput &clock,
                           const ScheduledPauseConfiguration &proposed) {
    const auto configuration_bytes = [](const ScheduledPauseConfiguration &config) {
        auto bytes =
            config.configured_zone.size() + config.rules.size() * sizeof(ScheduledPauseRule);
        for (const auto &rule : config.rules) {
            bytes += rule.schedule_id.size();
        }
        return bytes;
    };
    return sizeof(ScheduledPauseState) + sizeof(ScheduledPauseEpoch) +
           sizeof(ScheduledPauseLedger) + sizeof(ScheduledPauseClockInput) +
           sizeof(ScheduledPauseConfiguration) +
           clock.cuts.size() * sizeof(ScheduledPauseElapsedCut) + state.snapshot_ref.size() +
           state.timer_lineage.size() + state.clock_lineage.size() + state.state_revision.size() +
           state.configuration_revision.size() +
           state.cursor.last_observation.observation_ref.size() +
           state.ledger->timer_lineage.size() + state.ledger->revision.size() +
           clock.source_snapshot_ref.size() + clock.clock_lineage.size() +
           clock.anchor.observation_ref.size() +
           configuration_bytes(state.epochs.front().configuration) + configuration_bytes(proposed);
}

constexpr std::size_t f3_padding_cuts = 2304;

std::vector<ScheduledPauseElapsedCut> f3_padding_clock() {
    std::vector<ScheduledPauseElapsedCut> cuts;
    for (std::size_t i = 0; i < f3_padding_cuts; ++i) {
        const auto t = cut_time - static_cast<Instant>(f3_padding_cuts - 1 - i);
        cuts.push_back({{"cut-" + std::to_string(i) + "/", t}, 1ms, {}});
    }
    return cuts;
}

void f3_fill_cut_refs(std::vector<ScheduledPauseElapsedCut> &cuts, std::size_t total) {
    const auto each = total / cuts.size();
    const auto extra = total % cuts.size();
    std::size_t charged = 0;
    for (std::size_t i = 0; i < cuts.size(); ++i) {
        const auto length = each + (i < extra);
        auto &ref = cuts[i].observation.observation_ref;
        ASSERT_GE(length, ref.size());
        ASSERT_LE(length, ScheduledPauseLimits::max_reference_bytes);
        ref.resize(length, 'x');
        charged += ref.size();
    }
    ASSERT_EQ(charged, total);
}

// Under 270 A, charge the returned public value and its payload.
TEST(ScheduledPauseForecastF3, ForecastInputReplyEqualityAndOneByteOver) {
    using Limits = ScheduledPauseLimits;
    const F3Basis basis(cut_time - static_cast<Instant>(f3_padding_cuts), configuration("F3/Old"));
    const auto proposed = configuration("F3/Zero", {rule("daily", 12h, 1ms)});
    const auto horizon = after(cut_time, days{1});
    const auto request_bytes = sizeof(ScheduleZoneRequest) + proposed.configured_zone.size();
    for (const bool bare : {false, true}) {
        for (const bool one_over : {false, true}) {
            SCOPED_TRACE(bare);
            SCOPED_TRACE(one_over);
            auto cuts = f3_padding_clock();
            const auto fixed = f3_fixed_input(basis.state, basis.clock(cuts), proposed);
            const auto reply_bytes = bare ? sizeof(ZoneError)
                                          : sizeof(ForecastPage) + proposed.configured_zone.size() +
                                                1 + sizeof(ScheduleZoneTransition);
            ASSERT_LT(fixed + request_bytes + reply_bytes, Limits::max_input_bytes);
            ASSERT_NO_FATAL_FAILURE(f3_fill_cut_refs(
                cuts, Limits::max_input_bytes - fixed - request_bytes - reply_bytes + one_over));

            PagedZone source;
            if (bare) {
                source.replies.emplace_back(ZoneError::source_unavailable);
            } else {
                source.replies.emplace_back(
                    ForecastPage{facts(proposed.configured_zone, "r", lower, after(horizon, 48h), 0,
                                       {{horizon + 2, 1}}),
                                 ReachedRequestEnd{}});
            }
            const auto projected = project_scheduled_pause_edit(basis.state, basis.clock(cuts),
                                                                proposed, horizon, source);
            const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
            ASSERT_NE(preview, nullptr);
            ASSERT_TRUE(preview->pause_after_cutover);
            EXPECT_TRUE(source.legacy_requests.empty());
            expect_page_requests(source, proposed.configured_zone, after(horizon, 48h), {lower},
                                 "r");
            if (one_over || bare) {
                const auto cause =
                    one_over ? ZoneError::limit_exceeded : ZoneError::source_unavailable;
                expect_forecast(*preview, cuts.back().observation, horizon, cut_time,
                                coverage(cause));
                EXPECT_TRUE(preview->facts.empty());
                EXPECT_TRUE(preview->upcoming.empty());
            } else {
                expect_forecast(*preview, cuts.back().observation, horizon, horizon);
                ASSERT_EQ(preview->facts.size(), 1U);
                expect_fact(preview->facts.front(), page_facts(source.replies.front()));
                ASSERT_EQ(preview->upcoming.size(), 1U);
                expect_row(preview->upcoming.front(), key("daily", horizon), horizon, horizon + 1,
                           horizon, horizon + 1);
            }
        }
    }
}

TEST(ScheduledPauseForecastF3, LegacyTypedErrorSurvivesAtExactRequestBudget) {
    using Limits = ScheduledPauseLimits;
    const auto anchor = cut_time - static_cast<Instant>(f3_padding_cuts);
    F3WorkSource priming;
    const auto old = configuration("F3/Old", {rule("old", 15h, 1ms)});
    const EnteredBasis basis(priming, anchor + 1, old);
    const auto proposed = configuration("F3/New");
    const auto request_bytes = sizeof(ScheduleZoneRequest) + old.configured_zone.size();
    for (const bool one_over : {false, true}) {
        SCOPED_TRACE(one_over);
        auto cuts = f3_padding_clock();
        const auto fixed = f3_fixed_input(basis.state(), basis.clock(cuts), proposed);
        ASSERT_LT(fixed + request_bytes, Limits::max_input_bytes);
        ASSERT_NO_FATAL_FAILURE(
            f3_fill_cut_refs(cuts, Limits::max_input_bytes - fixed - request_bytes + one_over));

        PagedZone source;
        source.scripted = ZoneError::source_unavailable;
        const auto projected = project_scheduled_pause_edit(basis.state(), basis.clock(cuts),
                                                            proposed, cut_time, source);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        const auto cause = one_over ? ZoneError::limit_exceeded : ZoneError::source_unavailable;
        expect_forecast(*preview, cuts.back().observation, cut_time, cut_time, coverage(cause));
        EXPECT_FALSE(preview->pause_after_cutover);
        EXPECT_TRUE(preview->facts.empty());
        EXPECT_TRUE(preview->upcoming.empty());
        EXPECT_TRUE(source.page_requests.empty());
        ASSERT_EQ(source.legacy_requests.size(), one_over ? 0U : 1U);
        if (!one_over) {
            const auto &request = source.legacy_requests.front();
            EXPECT_EQ(request.configured_zone, old.configured_zone);
            EXPECT_EQ(request.from_utc_ms, after(anchor, -48h));
            EXPECT_EQ(request.through_utc_ms, after(cut_time, 48h));
            EXPECT_FALSE(request.required_rules_revision);
        }
    }
}

TEST(ScheduledPauseForecastF3, NativeRequestDispatchesAtExactInputBudget) {
    using Limits = ScheduledPauseLimits;
    const F3Basis basis(cut_time - static_cast<Instant>(f3_padding_cuts), configuration("F3/Old"));
    const auto proposed = configuration("F3/Zero", {rule("daily", 12h, 1ms)});
    const auto horizon = after(cut_time, days{1});
    const auto request_bytes = sizeof(ScheduleZoneRequest) + proposed.configured_zone.size();
    for (const bool one_over : {false, true}) {
        SCOPED_TRACE(one_over);
        auto cuts = f3_padding_clock();
        const auto fixed = f3_fixed_input(basis.state, basis.clock(cuts), proposed);
        ASSERT_LT(fixed + request_bytes, Limits::max_input_bytes);
        ASSERT_NO_FATAL_FAILURE(
            f3_fill_cut_refs(cuts, Limits::max_input_bytes - fixed - request_bytes + one_over));

        PagedZone source;
        // Any positive structural reply weight cannot fit at request equality.
        source.replies.emplace_back(ZoneError::source_unavailable);
        const auto projected =
            project_scheduled_pause_edit(basis.state, basis.clock(cuts), proposed, horizon, source);
        const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
        ASSERT_NE(preview, nullptr);
        expect_forecast(*preview, cuts.back().observation, horizon, cut_time,
                        coverage(ZoneError::limit_exceeded));
        EXPECT_TRUE(preview->pause_after_cutover);
        EXPECT_TRUE(preview->facts.empty());
        EXPECT_TRUE(preview->upcoming.empty());
        EXPECT_TRUE(source.legacy_requests.empty());
        if (one_over) {
            EXPECT_TRUE(source.page_requests.empty());
        } else {
            expect_page_requests(source, proposed.configured_zone, after(horizon, 48h), {lower},
                                 "unused");
        }
    }
}

} // namespace

namespace {

class FirstTerminalPointSource final : public ScheduleZoneSource {
  public:
    F3WorkSource legacy;
    std::vector<ScheduleZoneRequest> forecast_requests;

    LegacyReply read(const ScheduleZoneRequest &request) override { return legacy.read(request); }

    PageReply read_forecast_page(const ScheduleZoneRequest &request) override {
        forecast_requests.push_back(request);
        return ForecastPage{facts(request.configured_zone, "terminal-r1", request.from_utc_ms,
                                  request.from_utc_ms),
                            TerminalPrefix{ZoneError::source_unavailable}};
    }
};

TEST(ScheduledPauseForecastF3, FirstTerminalPointKeepsCauseOnLastQuery) {
    constexpr std::size_t failed_old_reads = 4'092;
    constexpr std::size_t elapsed_cuts = failed_old_reads + 1;
    constexpr auto activation = cut_time - static_cast<Instant>(elapsed_cuts);
    F3Basis basis(activation - 1,
                  configuration("F3/Old", {rule("old-a", 15h, 1ms), rule("old-b", 15h, 1ms)}));

    LegacyZone priming;
    priming.scripted = ZoneError::source_unavailable;
    const ScheduledPauseEntry entry{
        basis.state.snapshot_ref,
        observation(activation),
        {.lifecycle = TimerLifecycle::Active, .main_remaining_ms = 900'000}};
    const auto entered = enter_scheduled_pauses(basis.state, entry, priming);
    const auto *held = std::get_if<ScheduledPauseProgress>(&entered);
    ASSERT_NE(held, nullptr);
    ASSERT_TRUE(held->cursor.hold);
    ASSERT_EQ(held->cursor.hold->cause,
              ScheduledPauseCoverageFailure{ZoneError::source_unavailable});
    ASSERT_EQ(priming.legacy_requests.size(), 1U);
    ASSERT_TRUE(held->new_deliveries.empty());
    ASSERT_TRUE(held->stops.empty());
    basis.state.snapshot_ref = "f3-active-snapshot";
    basis.state.state_revision = "f3-active";
    basis.state.timer = held->timer;
    basis.state.pause = held->pause;
    basis.state.cursor = held->cursor;

    std::vector<ScheduledPauseElapsedCut> cuts;
    cuts.reserve(elapsed_cuts);
    for (Instant now = activation + 1; now <= cut_time; ++now) {
        cuts.push_back(elapsed_cut(now, 1ms));
    }
    ASSERT_EQ(cuts.size(), 4'093U);

    FirstTerminalPointSource source;
    source.legacy.legacy_errors = failed_old_reads;
    const auto proposed = configuration("F3/TerminalPoint", {rule("daily", 12h, 1ms)});
    const auto horizon = after(cut_time, days{10});
    const auto projected =
        project_scheduled_pause_edit(basis.state, basis.clock(cuts), proposed, horizon, source);
    const auto *preview = std::get_if<ScheduledPausePreview>(&projected);
    ASSERT_NE(preview, nullptr);

    // 4,092 failed reads, then one shared read and two expansions, leave one query.
    ASSERT_EQ(source.legacy.legacy_requests.size(), 4'093U);
    for (std::size_t i = 0; i < source.legacy.legacy_requests.size(); ++i) {
        const auto now = activation + static_cast<Instant>(i + 1);
        const auto &request = source.legacy.legacy_requests[i];
        ASSERT_EQ(request.configured_zone, "F3/Old");
        ASSERT_EQ(request.from_utc_ms, after(now - 1, -48h));
        ASSERT_EQ(request.through_utc_ms, after(now, 48h));
        ASSERT_FALSE(request.required_rules_revision);
    }
    ASSERT_EQ(source.forecast_requests.size(), 1U);
    const auto &request = source.forecast_requests.front();
    ASSERT_EQ(request.configured_zone, proposed.configured_zone);
    ASSERT_EQ(request.from_utc_ms, lower);
    ASSERT_EQ(request.through_utc_ms, after(horizon, 48h));
    ASSERT_FALSE(request.required_rules_revision);

    ASSERT_EQ(preview->as_of.utc_ms, cut_time);
    ASSERT_EQ(preview->as_of.observation_ref, observation(cut_time).observation_ref);
    ASSERT_EQ(preview->requested_through_utc_ms, horizon);
    ASSERT_EQ(preview->resolved_through_utc_ms, cut_time);
    ASSERT_TRUE(preview->pause_after_cutover);
    ASSERT_TRUE(preview->pause_after_cutover->reasons.empty());
    ASSERT_TRUE(preview->cutover_effects.empty());
    ASSERT_TRUE(preview->upcoming.empty());
    ASSERT_EQ(preview->facts.size(), 1U);
    ASSERT_NO_FATAL_FAILURE(expect_fact(
        preview->facts.front(),
        facts("F3/Old", "old-r1", after(cut_time - 1, -48h), after(cut_time, 48h)), 0U));

    ASSERT_TRUE(preview->incomplete);
    const auto *cause = std::get_if<ScheduledPauseCoverageFailure>(&*preview->incomplete);
    ASSERT_NE(cause, nullptr);
    const auto *zone_error = std::get_if<ZoneError>(cause);
    ASSERT_NE(zone_error, nullptr);
    EXPECT_EQ(*zone_error, ZoneError::source_unavailable)
        << "The first terminal point must preserve its source_unavailable cause";
}

} // namespace
