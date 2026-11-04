// SPDX-License-Identifier: GPL-3.0-or-later
// Specification v0.45 §4/§18/TIMER-01; logical f4a4c3ce / API a2ac5a2c.
// Raw synthetic aggregates and independent literals. No reducer builds an oracle.
#include <subathon/core/timer_editorial_commands.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace subathon::core;
using Values = TimerEditorialValues;
using Input = TimerEditorialInput;
using Status = TimerEditorialDisposition;
using Required = TimerEditorialPrerequisite;
using Error = TimerEditorialError;
using Adjustment = TimerEditorialAdjustment;
using Warning = TimerCapWarning;
using Transition = TimerTransition;
using Trace = std::vector<Transition>;
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
constexpr auto minimum = std::numeric_limits<std::int64_t>::min();

Values values(std::int64_t main = 100, std::int64_t debt = 0) {
    return {{TimerLifecycle::Active, CountdownPhase::Main, main, 73, false},
            debt,
            {},
            {TimerEndMode::Immediate, 120, GraceReuse::Fresh, {}}};
}
Input input(TimerEditorialPayload payload, Values current = values()) {
    return {{"r", std::move(current)},
            {"o", "r", {}, {}, std::move(payload)},
            {TimerEditorialAccessState::allowed, "p"},
            {{0, 500, false}},
            500};
}
struct Expected {
    Status disposition;
    Required prerequisite;
    std::optional<Values> observed;
    std::optional<Values> accepted;
    std::vector<Trace> observations;
    Trace editorial;
    std::optional<Adjustment> adjustment;
    std::optional<Warning> warning;
};
Expected accepted(Values before, Values after, std::optional<Adjustment> adjustment = {},
                  Trace editorial = {}, std::optional<Warning> warning = {}) {
    return {Status::accepted, Required::none, std::move(before),
            std::move(after), {{}},           std::move(editorial),
            adjustment,       warning};
}
Expected observed(Status status, Values before, std::optional<Adjustment> adjustment = {},
                  Required prerequisite = Required::none, std::optional<Warning> warning = {}) {
    return {status, prerequisite, std::move(before), {}, {{}}, {}, adjustment, warning};
}
Expected early(Status status) { return {status, Required::none, {}, {}, {}, {}, {}, {}}; }
struct Case {
    const char *id;
    Input raw;
    Expected expected;
    TimerEditorialLimits limits{};
};

void check_values(const Values &actual, const Values &expected) {
    EXPECT_EQ(actual.timer.lifecycle, expected.timer.lifecycle);
    EXPECT_EQ(actual.timer.phase, expected.timer.phase);
    EXPECT_EQ(actual.timer.main_remaining_ms, expected.timer.main_remaining_ms);
    EXPECT_EQ(actual.timer.grace_remaining_ms, expected.timer.grace_remaining_ms);
    EXPECT_EQ(actual.timer.awaiting_confirmation, expected.timer.awaiting_confirmation);
    EXPECT_EQ(actual.time_debt_ms, expected.time_debt_ms);
    EXPECT_EQ(actual.maximum_remaining_ms, expected.maximum_remaining_ms);
    EXPECT_EQ(actual.config.end_mode, expected.config.end_mode);
    EXPECT_EQ(actual.config.grace_duration_ms, expected.config.grace_duration_ms);
    EXPECT_EQ(actual.config.grace_reuse, expected.config.grace_reuse);
    EXPECT_EQ(actual.config.hard_end_utc_ms, expected.config.hard_end_utc_ms);
}
void check_preview(const TimerCapWarningPreview &actual, const TimerCapWarningPreview &expected) {
    EXPECT_EQ(actual.revision, expected.revision);
    EXPECT_EQ(actual.main_remaining_ms, expected.main_remaining_ms);
    EXPECT_EQ(actual.old_cap_ms, expected.old_cap_ms);
    EXPECT_EQ(actual.proposed_cap_ms, expected.proposed_cap_ms);
}
template <class T, class Check>
void check_optional(const std::optional<T> &actual, const std::optional<T> &expected, Check check) {
    EXPECT_EQ(actual.has_value(), expected.has_value());
    if (actual && expected)
        check(*actual, *expected);
}
void check_input(const Input &actual, const Input &expected) {
    EXPECT_EQ(actual.current.revision, expected.current.revision);
    check_values(actual.current.values, expected.current.values);
    EXPECT_EQ(actual.command.operation, expected.command.operation);
    EXPECT_EQ(actual.command.expected_revision, expected.command.expected_revision);
    EXPECT_EQ(actual.command.actor, expected.command.actor);
    EXPECT_EQ(actual.command.reason, expected.command.reason);
    EXPECT_EQ(actual.access.state, expected.access.state);
    EXPECT_EQ(actual.access.permission_revision, expected.access.permission_revision);
    EXPECT_EQ(actual.final_utc_ms, expected.final_utc_ms);
    EXPECT_EQ(actual.observations.size(), expected.observations.size());
    for (std::size_t i = 0; i < std::min(actual.observations.size(), expected.observations.size());
         ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(actual.observations[i].elapsed_ms, expected.observations[i].elapsed_ms);
        EXPECT_EQ(actual.observations[i].utc_now_ms, expected.observations[i].utc_now_ms);
        EXPECT_EQ(actual.observations[i].paused, expected.observations[i].paused);
    }
    EXPECT_EQ(actual.command.payload.index(), expected.command.payload.index());
    if (actual.command.payload.index() != expected.command.payload.index())
        return;
    if (const auto *delta = std::get_if<TimerSignedDelta>(&expected.command.payload)) {
        const auto &got = std::get<TimerSignedDelta>(actual.command.payload);
        EXPECT_EQ(got.requested_ms, delta->requested_ms);
        EXPECT_EQ(got.negative_policy, delta->negative_policy);
    } else if (const auto *cap = std::get_if<TimerCapReplacement>(&expected.command.payload)) {
        const auto &got = std::get<TimerCapReplacement>(actual.command.payload);
        EXPECT_EQ(got.maximum_remaining_ms, cap->maximum_remaining_ms);
        EXPECT_EQ(got.choice, cap->choice);
        check_optional(got.warning_preview, cap->warning_preview, check_preview);
    } else if (const auto *deadline =
                   std::get_if<TimerDeadlineReplacement>(&expected.command.payload)) {
        EXPECT_EQ(std::get<TimerDeadlineReplacement>(actual.command.payload).hard_end_utc_ms,
                  deadline->hard_end_utc_ms);
    }
}
void check_batch(const TimerEditorialTransitionBatch &actual, const Trace &expected) {
    EXPECT_EQ(actual.count, expected.size());
    EXPECT_LE(actual.count, 4);
    EXPECT_LE(expected.size(), 4U);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(actual.entries[i], i < expected.size() ? expected[i] : Transition::MainExpired);
    }
}
void check_adjustment(const Adjustment &actual, const Adjustment &expected) {
    EXPECT_EQ(actual.requested_ms, expected.requested_ms);
    EXPECT_EQ(actual.main_adjustment_ms, expected.main_adjustment_ms);
    EXPECT_EQ(actual.debt_repaid_ms, expected.debt_repaid_ms);
    EXPECT_EQ(actual.debt_added_ms, expected.debt_added_ms);
    EXPECT_EQ(actual.cap_limited_ms, expected.cap_limited_ms);
    EXPECT_EQ(actual.unapplied_ms, expected.unapplied_ms);
}
void check_warning(const Warning &actual, const Warning &expected) {
    EXPECT_EQ(actual.main_remaining_ms, expected.main_remaining_ms);
    EXPECT_EQ(actual.old_cap_ms, expected.old_cap_ms);
    EXPECT_EQ(actual.proposed_cap_ms, expected.proposed_cap_ms);
    EXPECT_EQ(actual.reduction_ms, expected.reduction_ms);
}
void check_result(const TimerEditorialProposalData &actual, const Case &row) {
    check_input(actual.input, row.raw);
    const auto &expected = row.expected;
    EXPECT_EQ(actual.disposition, expected.disposition);
    EXPECT_EQ(actual.prerequisite, expected.prerequisite);
    check_optional(actual.observed_before, expected.observed, check_values);
    check_optional(actual.accepted_after, expected.accepted, check_values);
    EXPECT_EQ(actual.observation_transitions.size(), expected.observations.size());
    for (std::size_t i = 0;
         i < std::min(actual.observation_transitions.size(), expected.observations.size()); ++i) {
        SCOPED_TRACE(i);
        check_batch(actual.observation_transitions[i], expected.observations[i]);
    }
    check_batch(actual.editorial_transitions, expected.editorial);
    check_optional(actual.adjustment, expected.adjustment, check_adjustment);
    check_optional(actual.warning, expected.warning, check_warning);
}
void cases(const std::vector<Case> &rows) {
    for (const auto &row : rows) {
        SCOPED_TRACE(row.id);
        try {
            const auto proposal = project_timer_editorial_command(row.raw, row.limits);
            check_result(proposal.data(), row);
        } catch (const std::exception &error) {
            ADD_FAILURE() << "Expected proposal, got exception: " << error.what();
        }
    }
}
void expect_error(const Input &raw, Error code, const TimerEditorialLimits &limits = {}) {
    try {
        (void)project_timer_editorial_command(raw, limits);
        ADD_FAILURE() << "Expected typed error " << static_cast<unsigned>(code);
    } catch (const TimerEditorialFailure &error) {
        EXPECT_EQ(error.code(), code);
        EXPECT_STREQ(error.what(), "Timer editorial command projection failed");
    } catch (const std::exception &error) {
        ADD_FAILURE() << "Wrong exception, including a module marker: " << error.what();
    }
}
using Mutation = std::function<void(Input &)>;
struct InvalidCase {
    const char *id;
    Mutation change;
    Error code;
};
void invalid_cases(const Input &base, const std::vector<InvalidCase> &rows) {
    for (const auto &row : rows) {
        SCOPED_TRACE(row.id);
        auto raw = base;
        row.change(raw);
        expect_error(raw, row.code);
    }
}

TEST(TimerEditorial, AdditionsRepayDebtBeforeCurrentCapacity) {
    auto before = values(55, 2);
    before.maximum_remaining_ms = 60;
    auto after = values(60);
    after.maximum_remaining_ms = 60;
    auto kept = values(100, 5);
    kept.maximum_remaining_ms = 60;
    auto paid = values(100);
    paid.maximum_remaining_ms = 60;
    auto partial = values(100, 2);
    partial.maximum_remaining_ms = 60;
    auto elapsed = input(TimerSignedDelta{12, {}}, kept);
    elapsed.observations = {{45, 545, false}};
    elapsed.final_utc_ms = 545;
    auto observed55 = values(55, 5);
    observed55.maximum_remaining_ms = 60;
    auto final60 = values(60);
    final60.maximum_remaining_ms = 60;
    cases({{"ED01 partition10=2+5+3", input(TimerSignedDelta{10, {}}, before),
            accepted(before, after, Adjustment{10, 5, 2, 0, 3, 0})},
           {"ED02/42 legitimate Main100 above cap60", input(TimerSignedDelta{8, {}}, kept),
            accepted(kept, paid, Adjustment{8, 0, 5, 0, 3, 0})},
           {"ED43 partial debt repayment", input(TimerSignedDelta{3, {}}, kept),
            accepted(kept, partial, Adjustment{3, 0, 3, 0, 0, 0})},
           {"ED44 countdown creates real capacity", elapsed,
            accepted(observed55, final60, Adjustment{12, 5, 5, 0, 2, 0})}});
}

TEST(TimerEditorial, OnlyActualMainCreditRescuesGraceOrConfirmation) {
    auto grace = values(0, 5);
    grace.timer.phase = CountdownPhase::Grace;
    grace.timer.grace_remaining_ms = 90;
    grace.config.end_mode = TimerEndMode::Grace;
    auto paid = grace;
    paid.time_debt_ms = 0;
    auto wait = values(0, 5);
    wait.timer.awaiting_confirmation = true;
    wait.config.end_mode = TimerEndMode::Confirmation;
    auto resumed = values(2);
    resumed.config.end_mode = TimerEndMode::Confirmation;
    auto a = input(TimerSignedDelta{5, {}}, grace);
    a.observations[0].paused = true;
    auto b = input(TimerSignedDelta{7, {}}, wait);
    b.observations[0].paused = true;
    cases(
        {{"ED03 debt-only leaves Grace90", a, accepted(grace, paid, Adjustment{5, 0, 5, 0, 0, 0})},
         {"ED04 actual2 resumes wait, pause retained", b,
          accepted(wait, resumed, Adjustment{7, 2, 5, 0, 0, 0})}});
}

TEST(TimerEditorial, NegativePoliciesPreserveFullRequestAndEndingTrace) {
    auto before = values(100, 7);
    auto stopped = values(0, 7);
    stopped.timer.lifecycle = TimerLifecycle::Finished;
    auto debt = values(0, 57);
    debt.timer.lifecycle = TimerLifecycle::Finished;
    auto min_before = values(1);
    auto min_after = values(0, maximum);
    min_after.timer.lifecycle = TimerLifecycle::Finished;
    auto waiting = values(0);
    waiting.config.end_mode = TimerEndMode::Confirmation;
    waiting.timer.awaiting_confirmation = true;
    const Trace finish{Transition::MainExpired, Transition::Finished};
    cases({{"ED05 stop retains unapplied -50",
            input(TimerSignedDelta{-150, RefundDeficitPolicy::StopAtZero}, before),
            accepted(before, stopped, Adjustment{-150, -100, 0, 0, 0, -50}, finish)},
           {"ED06 debt carries50",
            input(TimerSignedDelta{-150, RefundDeficitPolicy::CarryTimeDebt}, before),
            accepted(before, debt, Adjustment{-150, -100, 0, 50, 0, 0}, finish)},
           {"ED07 review is whole timer request",
            input(TimerSignedDelta{-150, RefundDeficitPolicy::RequireReview}, before),
            observed(Status::timer_review_required, before, Adjustment{-150, 0, 0, 0, 0, -150})},
           {"ED08 minimum request no signed negation",
            input(TimerSignedDelta{minimum, RefundDeficitPolicy::CarryTimeDebt}, min_before),
            accepted(min_before, min_after, Adjustment{minimum, -1, 0, maximum, 0, 0}, finish)},
           {"ED09 overflow no partial debt",
            input(TimerSignedDelta{minimum, RefundDeficitPolicy::CarryTimeDebt}, waiting),
            observed(Status::numeric_overflow_review, waiting,
                     Adjustment{minimum, 0, 0, 0, 0, minimum})},
           {"non-exhausting negative does not finish or hold",
            input(TimerSignedDelta{-1, RefundDeficitPolicy::RequireReview}, before),
            accepted(before, values(99, 7), Adjustment{-1, -1, 0, 0, 0, 0})},
           {"overflow also rolls back hypothetical debt repayment",
            input(TimerSignedDelta{6, {}}, values(maximum, 5)),
            observed(Status::numeric_overflow_review, values(maximum, 5),
                     Adjustment{6, 0, 0, 0, 0, 6})},
           {"ED10 overflow no partial Main", input(TimerSignedDelta{1, {}}, values(maximum)),
            observed(Status::numeric_overflow_review, values(maximum),
                     Adjustment{1, 0, 0, 0, 0, 1})}});
}

TEST(TimerEditorial, ClockObservationPrecedesDeltaAndCannotReopen) {
    auto raw = input(TimerSignedDelta{5, {}}, values(100));
    raw.observations[0].elapsed_ms = 10;
    auto grace = values(0, 5);
    grace.timer.phase = CountdownPhase::Grace;
    grace.timer.grace_remaining_ms = 1;
    grace.config.end_mode = TimerEndMode::Grace;
    auto expired = grace;
    expired.timer.lifecycle = TimerLifecycle::Finished;
    expired.timer.grace_remaining_ms = 0;
    auto finished = input(TimerSignedDelta{10, {}}, grace);
    finished.observations[0].elapsed_ms = 1;
    auto expected = observed(Status::prerequisite_not_ready, expired,
                             Adjustment{10, 0, 0, 0, 0, 10}, Required::finished_policy);
    expected.observations = {{Transition::GraceExpired, Transition::Finished}};
    cases({{"ED11 elapsed10 is not editorial credit", raw,
            accepted(values(90), values(95), Adjustment{5, 5, 0, 0, 0, 0})},
           {"ED37 expiry before ordinary credit", finished, expected}});
}

TEST(TimerEditorial, CapKeepAndUnaffectedSettingsRetainMainAndDebt) {
    auto before = values(55, 2);
    before.maximum_remaining_ms = 60;
    auto raised = before;
    raised.maximum_remaining_ms = 100;
    auto old = values(100, 5);
    old.maximum_remaining_ms = 120;
    auto kept = old;
    kept.maximum_remaining_ms = 60;
    auto equal = old;
    equal.maximum_remaining_ms = 100;
    auto removed = old;
    removed.maximum_remaining_ms.reset();
    cases({{"ED12 raise cap", input(TimerCapReplacement{100, {}, {}}, before),
            accepted(before, raised)},
           {"ED17 Keep accepts above-cap Main",
            input(TimerCapReplacement{60, TimerCapChoice::keep, {}}, old),
            accepted(old, kept, {}, {}, Warning{100, 120, 60, 40})},
           {"ED25 equality needs no warning", input(TimerCapReplacement{100, {}, {}}, old),
            accepted(old, equal)},
           {"ED25 cap removal", input(TimerCapReplacement{{}, {}, {}}, old),
            accepted(old, removed)}});
}

TEST(TimerEditorial, CapReductionRequiresPerAttemptChoiceAndAbortCancelsBoth) {
    auto before = values(100, 5);
    before.maximum_remaining_ms = 120;
    auto reduced = values(60, 5);
    reduced.maximum_remaining_ms = 60;
    auto elapsed = input(TimerCapReplacement{60, TimerCapChoice::reduce, {}}, before);
    elapsed.observations[0].elapsed_ms = 10;
    auto observed90 = values(90, 5);
    observed90.maximum_remaining_ms = 120;
    cases({{"ED18 explicit audited reduction40",
            input(TimerCapReplacement{60, TimerCapChoice::reduce, {}}, before),
            accepted(before, reduced, Adjustment{-40, -40, 0, 0, 0, 0}, {},
                     Warning{100, 120, 60, 40})},
           {"ED19 Abort no accepted edit",
            input(TimerCapReplacement{60, TimerCapChoice::abort, {}}, before),
            observed(Status::aborted, before, {}, Required::none, Warning{100, 120, 60, 40})},
           {"ED20 no hidden global default", input(TimerCapReplacement{60, {}, {}}, before),
            observed(Status::cap_choice_required, before, {}, Required::none,
                     Warning{100, 120, 60, 40})},
           {"ED21 elapsed10 changes actual correction to30", elapsed,
            accepted(observed90, reduced, Adjustment{-30, -30, 0, 0, 0, 0}, {},
                     Warning{90, 120, 60, 30})}});
}

TEST(TimerEditorial, CapExhaustionUsesImmediateConfirmationAndFreshSharedGrace) {
    auto before = values(100, 5);
    auto done = values(0, 5);
    done.maximum_remaining_ms = 0;
    done.timer.lifecycle = TimerLifecycle::Finished;
    auto immediate = input(TimerCapReplacement{0, TimerCapChoice::reduce, {}}, before);
    immediate.observations[0].paused = true;
    auto confirm = immediate;
    confirm.current.values.config.end_mode = TimerEndMode::Confirmation;
    auto wait = values(0, 5);
    wait.maximum_remaining_ms = 0;
    wait.config.end_mode = TimerEndMode::Confirmation;
    wait.timer.awaiting_confirmation = true;
    auto fresh = immediate;
    fresh.current.values.config.end_mode = TimerEndMode::Grace;
    auto fresh_after = values(0, 5);
    fresh_after.maximum_remaining_ms = 0;
    fresh_after.config.end_mode = TimerEndMode::Grace;
    fresh_after.timer.phase = CountdownPhase::Grace;
    fresh_after.timer.grace_remaining_ms = 120;
    auto shared = fresh;
    shared.current.values.config.grace_reuse = GraceReuse::Shared;
    auto shared_after = fresh_after;
    shared_after.config.grace_reuse = GraceReuse::Shared;
    shared_after.timer.grace_remaining_ms = 73;
    auto zero_grace = fresh;
    zero_grace.current.values.config.grace_duration_ms = 0;
    auto zero_after = fresh_after;
    zero_after.config.grace_duration_ms = 0;
    zero_after.timer.grace_remaining_ms = 0;
    zero_after.timer.lifecycle = TimerLifecycle::Finished;
    auto limit = input(TimerCapReplacement{0, TimerCapChoice::reduce, {}}, values(maximum, 5));
    auto extreme_done = done;
    cases(
        {{"ED22 immediate even paused", immediate,
          accepted(before, done, Adjustment{-100, -100, 0, 0, 0, 0},
                   {Transition::MainExpired, Transition::Finished}, Warning{100, {}, 0, 100})},
         {"ED23 confirmation wait", confirm,
          accepted(confirm.current.values, wait, Adjustment{-100, -100, 0, 0, 0, 0},
                   {Transition::MainExpired, Transition::ConfirmationRequired},
                   Warning{100, {}, 0, 100})},
         {"ED24 Fresh budget120", fresh,
          accepted(fresh.current.values, fresh_after, Adjustment{-100, -100, 0, 0, 0, 0},
                   {Transition::MainExpired, Transition::GraceStarted}, Warning{100, {}, 0, 100})},
         {"ED24 Shared retained73", shared,
          accepted(shared.current.values, shared_after, Adjustment{-100, -100, 0, 0, 0, 0},
                   {Transition::MainExpired, Transition::GraceStarted}, Warning{100, {}, 0, 100})},
         {"full four-event batch for zero Grace", zero_grace,
          accepted(zero_grace.current.values, zero_after, Adjustment{-100, -100, 0, 0, 0, 0},
                   {Transition::MainExpired, Transition::GraceStarted, Transition::GraceExpired,
                    Transition::Finished},
                   Warning{100, {}, 0, 100})},
         {"ED40 -M is representable", limit,
          accepted(values(maximum, 5), extreme_done, Adjustment{-maximum, -maximum, 0, 0, 0, 0},
                   {Transition::MainExpired, Transition::Finished},
                   Warning{maximum, {}, 0, maximum})}});
}

TEST(TimerEditorial, CapEditsPreserveReadyFinishedGraceAndWait) {
    auto ready = values(100, 5);
    ready.maximum_remaining_ms = 120;
    ready.timer.lifecycle = TimerLifecycle::Ready;
    auto ready_after = ready;
    ready_after.timer.main_remaining_ms = 60;
    ready_after.maximum_remaining_ms = 60;
    auto finished = ready;
    finished.timer.lifecycle = TimerLifecycle::Finished;
    auto finished_after = ready_after;
    finished_after.timer.lifecycle = TimerLifecycle::Finished;
    auto a = input(TimerCapReplacement{60, TimerCapChoice::reduce, {}}, ready);
    a.observations[0].elapsed_ms = 10;
    auto b = input(TimerCapReplacement{60, TimerCapChoice::reduce, {}}, finished);
    b.observations[0].elapsed_ms = 10;
    auto grace = values(0, 5);
    grace.maximum_remaining_ms = 120;
    grace.config.end_mode = TimerEndMode::Grace;
    grace.timer.phase = CountdownPhase::Grace;
    grace.timer.grace_remaining_ms = 90;
    auto grace_after = grace;
    grace_after.maximum_remaining_ms = 0;
    auto wait = values(0, 5);
    wait.maximum_remaining_ms = 120;
    wait.config.end_mode = TimerEndMode::Confirmation;
    wait.timer.awaiting_confirmation = true;
    auto wait_after = wait;
    wait_after.maximum_remaining_ms = 0;
    auto c = input(TimerCapReplacement{0, {}, {}}, grace);
    c.observations[0].paused = true;
    auto d = input(TimerCapReplacement{0, {}, {}}, wait);
    d.observations[0].paused = true;
    cases({{"ED26 Ready settings do not start countdown", a,
            accepted(ready, ready_after, Adjustment{-40, -40, 0, 0, 0, 0}, {},
                     Warning{100, 120, 60, 40})},
           {"ED27 Finished settings do not reopen", b,
            accepted(finished, finished_after, Adjustment{-40, -40, 0, 0, 0, 0}, {},
                     Warning{100, 120, 60, 40})},
           {"ED41 Grace cap0 no fabricated warning", c, accepted(grace, grace_after)},
           {"ED41 waiting cap0 retains wait", d, accepted(wait, wait_after)}});
}

TEST(TimerEditorial, FirstStartVariantChecksDeadlineOnly) {
    std::vector<Case> rows;
    for (const auto now : {1000, 1001}) {
        auto ready = values();
        ready.timer.lifecycle = TimerLifecycle::Ready;
        ready.config.hard_end_utc_ms = 1000;
        auto raw = input(TimerFirstStartDeadlineCheck{}, ready);
        raw.observations = {{0, now, false}};
        raw.final_utc_ms = now;
        rows.push_back({"ED28 equality/past blocks but stays Ready", raw,
                        observed(Status::start_deadline_blocked, ready, {},
                                 Required::first_start_absolute_main)});
    }
    for (const auto deadline : {std::optional<std::int64_t>{1001}, std::optional<std::int64_t>{}}) {
        auto ready = values();
        ready.timer.lifecycle = TimerLifecycle::Ready;
        ready.config.hard_end_utc_ms = deadline;
        auto raw = input(TimerFirstStartDeadlineCheck{}, ready);
        raw.observations = {{0, 1000, false}};
        raw.final_utc_ms = 1000;
        rows.push_back({"ED29 clear is not a complete Start", raw,
                        observed(Status::start_deadline_clear, ready, {},
                                 Required::first_start_absolute_main)});
    }
    auto active = values();
    auto finished = active;
    finished.timer.lifecycle = TimerLifecycle::Finished;
    rows.push_back({"check on Active is not Start", input(TimerFirstStartDeadlineCheck{}, active),
                    observed(Status::prerequisite_not_ready, active, {},
                             Required::first_start_absolute_main)});
    rows.push_back(
        {"check on Finished is not Reopen", input(TimerFirstStartDeadlineCheck{}, finished),
         observed(Status::prerequisite_not_ready, finished, {}, Required::finished_policy)});
    cases(rows);
}

TEST(TimerEditorial, NewReachedDeadlineFinishesOnlyObservedActiveState) {
    std::vector<Case> rows;
    for (const auto new_deadline : {999, 1000}) {
        auto ready = values();
        ready.timer.lifecycle = TimerLifecycle::Ready;
        auto ready_after = ready;
        ready_after.config.hard_end_utc_ms = new_deadline;
        auto raw = input(TimerDeadlineReplacement{new_deadline}, ready);
        raw.observations = {{10, 1000, false}};
        raw.final_utc_ms = 1000;
        rows.push_back(
            {"ED30 Ready deadline does not auto-finish", raw, accepted(ready, ready_after)});
        for (const auto old_deadline :
             {std::optional<std::int64_t>{}, std::optional<std::int64_t>{2000}}) {
            auto main = values(50, 5);
            main.config.hard_end_utc_ms = old_deadline;
            auto after = main;
            after.timer.lifecycle = TimerLifecycle::Finished;
            after.config.hard_end_utc_ms = new_deadline;
            auto edit = input(TimerDeadlineReplacement{new_deadline}, main);
            edit.observations = {{0, 1000, true}};
            edit.final_utc_ms = 1000;
            rows.push_back(
                {"ED31 edit-now/past retains pre-call Main50", edit,
                 accepted(main, after, {}, {Transition::HardEndReached, Transition::Finished})});
        }
    }
    auto grace = values(0, 5);
    grace.config.end_mode = TimerEndMode::Grace;
    grace.timer.phase = CountdownPhase::Grace;
    grace.timer.grace_remaining_ms = 90;
    auto after_grace = grace;
    after_grace.timer.lifecycle = TimerLifecycle::Finished;
    after_grace.config.hard_end_utc_ms = 1000;
    auto edit_grace = input(TimerDeadlineReplacement{1000}, grace);
    edit_grace.observations = {{0, 1000, true}};
    edit_grace.final_utc_ms = 1000;
    rows.push_back(
        {"ED32 paused Grace90 retained", edit_grace,
         accepted(grace, after_grace, {}, {Transition::HardEndReached, Transition::Finished})});
    auto wait = values(0, 5);
    wait.config.end_mode = TimerEndMode::Confirmation;
    wait.timer.awaiting_confirmation = true;
    auto after_wait = wait;
    after_wait.timer.lifecycle = TimerLifecycle::Finished;
    after_wait.timer.awaiting_confirmation = false;
    after_wait.config.hard_end_utc_ms = 999;
    auto edit_wait = input(TimerDeadlineReplacement{999}, wait);
    edit_wait.observations = {{0, 1000, true}};
    edit_wait.final_utc_ms = 1000;
    rows.push_back(
        {"ED33 deadline clears wait without confirm", edit_wait,
         accepted(wait, after_wait, {}, {Transition::HardEndReached, Transition::Finished})});
    auto active = values(50, 5);
    auto future = active;
    future.config.hard_end_utc_ms = 2000;
    auto edit_future = input(TimerDeadlineReplacement{2000}, active);
    edit_future.observations = {{0, 1000, true}};
    edit_future.final_utc_ms = 1000;
    rows.push_back({"future replacement does not finish", edit_future, accepted(active, future)});
    cases(rows);
}

TEST(TimerEditorial, OldDeadlineObservationCannotBeErasedOrReplayed) {
    auto main = values(50, 5);
    main.config.hard_end_utc_ms = 1000;
    auto old_finished = main;
    old_finished.timer.lifecycle = TimerLifecycle::Finished;
    auto disabled = old_finished;
    disabled.config.hard_end_utc_ms.reset();
    auto raw = input(TimerDeadlineReplacement{{}}, main);
    raw.observations = {{10, 1000, false}};
    raw.final_utc_ms = 1000;
    auto full = accepted(old_finished, disabled);
    full.observations = {{Transition::HardEndReached, Transition::Finished}};
    auto grace = values(0, 5);
    grace.config.end_mode = TimerEndMode::Grace;
    grace.timer.phase = CountdownPhase::Grace;
    grace.timer.grace_remaining_ms = 90;
    grace.config.hard_end_utc_ms = 1000;
    auto grace_finished = grace;
    grace_finished.timer.lifecycle = TimerLifecycle::Finished;
    auto future = grace_finished;
    future.config.hard_end_utc_ms = 2000;
    auto grace_raw = input(TimerDeadlineReplacement{2000}, grace);
    grace_raw.observations = {{5, 1000, false}};
    grace_raw.final_utc_ms = 1000;
    auto full_grace = accepted(grace_finished, future);
    full_grace.observations = {{Transition::HardEndReached, Transition::Finished}};
    auto before_future = values(50);
    before_future.config.hard_end_utc_ms = 1000;
    auto early_disabled = before_future;
    early_disabled.config.hard_end_utc_ms.reset();
    auto early_edit = input(TimerDeadlineReplacement{{}}, before_future);
    early_edit.observations = {{0, 999, true}};
    early_edit.final_utc_ms = 999;
    auto already = input(TimerDeadlineReplacement{{}}, old_finished);
    already.observations = {{0, 1001, false}};
    already.final_utc_ms = 1001;
    auto same = already;
    same.command.payload = TimerDeadlineReplacement{1000};
    auto abort = input(TimerCapReplacement{20, TimerCapChoice::abort, {}}, main);
    abort.observations = {{0, 1000, false}};
    abort.final_utc_ms = 1000;
    auto aborted =
        observed(Status::aborted, old_finished, {}, Required::none, Warning{50, {}, 20, 30});
    aborted.observations = {{Transition::HardEndReached, Transition::Finished}};
    cases({{"ED13 future deadline may be disabled", early_edit,
            accepted(before_future, early_disabled)},
           {"ED14/34 old Main50 retained, not40", raw, full},
           {"ED14/35 old Grace90 retained, not85", grace_raw, full_grace},
           {"ED36 disabled Finished does not reopen", already, accepted(old_finished, disabled)},
           {"identical Finished setting does not repeat events", same,
            accepted(old_finished, old_finished)},
           {"ED38 Abort still retains old deadline trace", abort, aborted}});
}

TEST(TimerEditorial, AccessAndRevisionRefusalsDoNotInventClockResults) {
    auto old = values(50);
    old.config.hard_end_utc_ms = 1000;
    auto stale = input(TimerSignedDelta{5, {}}, old);
    stale.current.revision = "r2";
    stale.observations = {{0, 1000, true}};
    stale.final_utc_ms = 1000;
    auto denied = stale;
    denied.access = {TimerEditorialAccessState::denied, {}};
    auto unknown = stale;
    unknown.access = {};
    auto cap = input(
        TimerCapReplacement{40, TimerCapChoice::reduce, TimerCapWarningPreview{"r", 100, 120, 40}},
        values(100));
    cap.current.revision = "r2";
    cap.current.values.maximum_remaining_ms = 60;
    cases({{"ED15 stale has no old proposed observation", stale, early(Status::revision_conflict)},
           {"ED16 deny wins over revision check", denied, early(Status::access_denied)},
           {"ED16 unknown supplies no grant", unknown, early(Status::access_unknown)},
           {"ED39 stale warning is not reused against cap60", cap,
            early(Status::revision_conflict)}});
}

TEST(TimerEditorial, ReadyFinishedAndMissingPolicyRemainExplicitPrerequisites) {
    auto ready = values();
    ready.timer.lifecycle = TimerLifecycle::Ready;
    auto finished = values();
    finished.timer.lifecycle = TimerLifecycle::Finished;
    auto exact = values(100, 7);
    auto done = values(0, 7);
    done.timer.lifecycle = TimerLifecycle::Finished;
    cases({{"Ready zero delta does not start", input(TimerSignedDelta{0, {}}, ready),
            observed(Status::prerequisite_not_ready, ready, Adjustment{0, 0, 0, 0, 0, 0},
                     Required::first_start_absolute_main)},
           {"Finished correction requires its owning policy",
            input(TimerSignedDelta{-5, RefundDeficitPolicy::StopAtZero}, finished),
            observed(Status::prerequisite_not_ready, finished, Adjustment{-5, 0, 0, 0, 0, -5},
                     Required::finished_policy)},
           {"missing negative policy is not a zero success",
            input(TimerSignedDelta{-5, {}}, values()),
            observed(Status::policy_not_ready, values(), Adjustment{-5, 0, 0, 0, 0, -5})},
           {"RequireReview equality is not deficit",
            input(TimerSignedDelta{-100, RefundDeficitPolicy::RequireReview}, exact),
            accepted(exact, done, Adjustment{-100, -100, 0, 0, 0, 0},
                     {Transition::MainExpired, Transition::Finished})}});
}

TEST(TimerEditorial, EverySegmentRetainsOrderedTransitionsAndPauseMeaning) {
    auto initial = values(10, 1);
    initial.config.end_mode = TimerEndMode::Grace;
    initial.config.grace_duration_ms = 3;
    initial.timer.grace_remaining_ms = 7;
    auto raw = input(TimerSignedDelta{2, {}}, initial);
    raw.observations = {{10, 100, false}, {1, 101, true}, {1, 102, false}};
    raw.final_utc_ms = 102;
    auto before = values(0, 1);
    before.timer.phase = CountdownPhase::Grace;
    before.timer.grace_remaining_ms = 2;
    before.config.end_mode = TimerEndMode::Grace;
    before.config.grace_duration_ms = 3;
    auto after = values(1);
    after.timer.grace_remaining_ms = 2;
    after.config.end_mode = TimerEndMode::Grace;
    after.config.grace_duration_ms = 3;
    auto result = accepted(before, after, Adjustment{2, 1, 1, 0, 0, 0});
    result.observations = {{Transition::MainExpired, Transition::GraceStarted}, {}, {}};
    cases({{"explicit paused segment consumes no Grace", raw, result}});
}

TEST(TimerEditorial, PrefixSurvivesPolicyReviewAndNumericOverflow) {
    auto before = values(1, 7);
    before.config.end_mode = TimerEndMode::Grace;
    before.config.grace_duration_ms = 5;
    auto at_grace = values(0, 7);
    at_grace.config.end_mode = TimerEndMode::Grace;
    at_grace.config.grace_duration_ms = 5;
    at_grace.timer.phase = CountdownPhase::Grace;
    at_grace.timer.grace_remaining_ms = 5;
    auto review = input(TimerSignedDelta{-2, RefundDeficitPolicy::RequireReview}, before);
    review.observations[0].elapsed_ms = 1;
    auto held = observed(Status::timer_review_required, at_grace, Adjustment{-2, 0, 0, 0, 0, -2});
    held.observations = {{Transition::MainExpired, Transition::GraceStarted}};
    auto missing = review;
    missing.command.payload = TimerSignedDelta{-2, {}};
    auto not_ready = held;
    not_ready.disposition = Status::policy_not_ready;
    auto overflow = review;
    overflow.current.values.time_debt_ms = maximum;
    overflow.command.payload = TimerSignedDelta{-2, RefundDeficitPolicy::CarryTimeDebt};
    auto overflow_before = at_grace;
    overflow_before.time_debt_ms = maximum;
    auto failed =
        observed(Status::numeric_overflow_review, overflow_before, Adjustment{-2, 0, 0, 0, 0, -2});
    failed.observations = {{Transition::MainExpired, Transition::GraceStarted}};
    cases({{"timer review retains prior Main expiration", review, held},
           {"unknown policy retains same complete prefix", missing, not_ready},
           {"debt overflow retains prefix without partial balance", overflow, failed}});
}

TEST(TimerEditorial, WarningPreviewIsHistoryAndCannotFreezeOrIncreaseMain) {
    auto initial = values(100, 5);
    initial.maximum_remaining_ms = 120;
    auto raw = input(
        TimerCapReplacement{60, TimerCapChoice::reduce, TimerCapWarningPreview{"r", 100, 120, 60}},
        initial);
    raw.observations[0].elapsed_ms = 10;
    auto at90 = values(90, 5);
    at90.maximum_remaining_ms = 120;
    auto reduced = values(60, 5);
    reduced.maximum_remaining_ms = 60;
    auto later = raw;
    later.observations[0].elapsed_ms = 50;
    auto at50 = values(50, 5);
    at50.maximum_remaining_ms = 120;
    auto cap_only = at50;
    cap_only.maximum_remaining_ms = 60;
    auto aborted = later;
    std::get<TimerCapReplacement>(aborted.command.payload).choice = TimerCapChoice::abort;
    cases(
        {{"ED21 preview100 but actual90 yields -30", raw,
          accepted(at90, reduced, Adjustment{-30, -30, 0, 0, 0, 0}, {}, Warning{90, 120, 60, 30})},
         {"Reduce below target cannot add10", later,
          accepted(at50, cap_only, Adjustment{0, 0, 0, 0, 0, 0})},
         {"Abort still cancels when warning no longer needed", aborted,
          observed(Status::aborted, at50)}});
}

TEST(TimerEditorial, OpaqueIdentitiesAndOptionalEmptyDataRemainExact) {
    auto raw = input(TimerSignedDelta{0, {}}, values(1));
    raw.current.revision = std::string(1, '\0');
    raw.command.expected_revision = std::string(1, '\0');
    raw.command.operation = std::string(1, static_cast<char>(0x80));
    raw.access.permission_revision = std::string("p\0x", 3);
    raw.command.actor = "";
    raw.command.reason = std::string("why\0?", 5);
    auto absent = raw;
    absent.command.actor.reset();
    absent.command.reason.reset();
    cases({{"NUL/high-byte refs and present-empty actor", raw,
            accepted(values(1), values(1), Adjustment{})},
           {"absence stays distinct from empty", absent,
            accepted(values(1), values(1), Adjustment{})}});
}

TEST(TimerEditorial, ReturnedProposalDeeplyOwnsCompleteAuditAndBothTraces) {
    auto initial = values(100, 5);
    initial.maximum_remaining_ms = 120;
    auto raw =
        input(TimerCapReplacement{60, TimerCapChoice::reduce,
                                  TimerCapWarningPreview{std::string(128, 'r'), 100, 120, 60}},
              initial);
    raw.current.revision = std::string(128, 'r');
    raw.command.expected_revision = std::string(128, 'r');
    raw.command.operation = std::string(128, 'o');
    raw.command.actor = std::string(128, 'a');
    raw.command.reason = std::string("why\0?", 5);
    raw.access.permission_revision = std::string(128, 'p');
    raw.observations = {{5, 500, false}, {0, 501, true}};
    raw.final_utc_ms = 501;
    auto at95 = values(95, 5);
    at95.maximum_remaining_ms = 120;
    auto after = values(60, 5);
    after.maximum_remaining_ms = 60;
    auto expected =
        accepted(at95, after, Adjustment{-35, -35, 0, 0, 0, 0}, {}, Warning{95, 120, 60, 35});
    expected.observations = {{}, {}};
    const Case row{"deep ownership", raw, expected};
    std::optional<TimerEditorialProposal> retained;
    try {
        auto supplied = raw;
        retained.emplace(project_timer_editorial_command(supplied));
        check_input(supplied, raw);
        supplied = Input{};
    } catch (const std::exception &error) {
        ADD_FAILURE() << error.what();
    }
    // Gate in the caller before invoking getters/copying after failed evaluation.
    ASSERT_TRUE(retained.has_value());
    const auto copy = *retained;
    auto moved = std::move(*retained);
    retained.reset();
    check_result(copy.data(), row);
    check_result(moved.data(), row);
}

TEST(TimerEditorial, SnapshotValidationPreservesExistingDomains) {
    invalid_cases(
        input(TimerSignedDelta{}),
        {{"empty current revision", [](auto &x) { x.current.revision.clear(); },
          Error::invalid_snapshot},
         {"negative Main", [](auto &x) { x.current.values.timer.main_remaining_ms = -1; },
          Error::invalid_snapshot},
         {"negative Grace", [](auto &x) { x.current.values.timer.grace_remaining_ms = -1; },
          Error::invalid_snapshot},
         {"negative debt", [](auto &x) { x.current.values.time_debt_ms = -1; },
          Error::invalid_snapshot},
         {"negative current cap", [](auto &x) { x.current.values.maximum_remaining_ms = -1; },
          Error::invalid_snapshot},
         {"negative configured Grace",
          [](auto &x) { x.current.values.config.grace_duration_ms = -1; }, Error::invalid_snapshot},
         {"unknown lifecycle full enum range",
          [](auto &x) { x.current.values.timer.lifecycle = static_cast<TimerLifecycle>(256); },
          Error::invalid_snapshot},
         {"negative phase enum",
          [](auto &x) { x.current.values.timer.phase = static_cast<CountdownPhase>(-1); },
          Error::invalid_snapshot},
         {"unknown end mode full enum range",
          [](auto &x) { x.current.values.config.end_mode = static_cast<TimerEndMode>(256); },
          Error::invalid_snapshot},
         {"unknown Grace reuse",
          [](auto &x) { x.current.values.config.grace_reuse = static_cast<GraceReuse>(-1); },
          Error::invalid_snapshot},
         {"Grace cannot have positive Main",
          [](auto &x) { x.current.values.timer.phase = CountdownPhase::Grace; },
          Error::invalid_snapshot},
         {"wait cannot have positive Main",
          [](auto &x) {
              x.current.values.timer.awaiting_confirmation = true;
              x.current.values.config.end_mode = TimerEndMode::Confirmation;
          },
          Error::invalid_snapshot},
         {"wait requires Confirmation mode",
          [](auto &x) {
              x.current.values.timer.main_remaining_ms = 0;
              x.current.values.timer.awaiting_confirmation = true;
          },
          Error::invalid_snapshot},
         {"Finished cannot wait",
          [](auto &x) {
              x.current.values.timer = {TimerLifecycle::Finished, CountdownPhase::Main, 0, 73,
                                        true};
              x.current.values.config.end_mode = TimerEndMode::Confirmation;
          },
          Error::invalid_snapshot},
         {"Ready cannot be Grace",
          [](auto &x) {
              x.current.values.timer = {TimerLifecycle::Ready, CountdownPhase::Grace, 0, 73, false};
          },
          Error::invalid_snapshot},
         {"Ready cannot await confirmation",
          [](auto &x) {
              x.current.values.timer = {TimerLifecycle::Ready, CountdownPhase::Main, 0, 73, true};
              x.current.values.config.end_mode = TimerEndMode::Confirmation;
          },
          Error::invalid_snapshot}});
}

TEST(TimerEditorial, CommandAndAccessShapesAreExplicit) {
    invalid_cases(
        input(TimerSignedDelta{}),
        {{"empty operation", [](auto &x) { x.command.operation.clear(); }, Error::invalid_command},
         {"empty expected revision", [](auto &x) { x.command.expected_revision.clear(); },
          Error::invalid_command},
         {"negative command policy enum",
          [](auto &x) {
              x.command.payload = TimerSignedDelta{-1, static_cast<RefundDeficitPolicy>(-1)};
          },
          Error::invalid_command},
         {"policy enum is not narrowed",
          [](auto &x) {
              x.command.payload = TimerSignedDelta{-1, static_cast<RefundDeficitPolicy>(256)};
          },
          Error::invalid_command},
         {"zero does not take negative-only policy",
          [](auto &x) { x.command.payload = TimerSignedDelta{0, RefundDeficitPolicy::StopAtZero}; },
          Error::invalid_command},
         {"positive does not take negative-only policy",
          [](auto &x) { x.command.payload = TimerSignedDelta{1, RefundDeficitPolicy::StopAtZero}; },
          Error::invalid_command},
         {"negative proposed cap",
          [](auto &x) { x.command.payload = TimerCapReplacement{-1, {}, {}}; },
          Error::invalid_command},
         {"unknown cap choice",
          [](auto &x) {
              x.command.payload = TimerCapReplacement{10, static_cast<TimerCapChoice>(255), {}};
          },
          Error::invalid_command},
         {"Keep cannot remove a cap",
          [](auto &x) { x.command.payload = TimerCapReplacement{{}, TimerCapChoice::keep, {}}; },
          Error::invalid_command},
         {"Reduce cannot remove a cap",
          [](auto &x) { x.command.payload = TimerCapReplacement{{}, TimerCapChoice::reduce, {}}; },
          Error::invalid_command},
         {"unknown access state",
          [](auto &x) { x.access.state = static_cast<TimerEditorialAccessState>(255); },
          Error::invalid_access},
         {"allowed needs permission revision",
          [](auto &x) { x.access.permission_revision.reset(); }, Error::invalid_access},
         {"allowed revision nonempty", [](auto &x) { x.access.permission_revision = ""; },
          Error::invalid_access},
         {"denied has no grant",
          [](auto &x) { x.access.state = TimerEditorialAccessState::denied; },
          Error::invalid_access},
         {"unknown has no grant",
          [](auto &x) { x.access.state = TimerEditorialAccessState::unknown; },
          Error::invalid_access}});
}

TEST(TimerEditorial, PreviewBindingsCannotBecomeAuthorityOrStaleEffects) {
    auto v = values(100, 5);
    v.maximum_remaining_ms = 120;
    auto base = input(
        TimerCapReplacement{60, TimerCapChoice::reduce, TimerCapWarningPreview{"r", 100, 120, 60}},
        v);
    invalid_cases(
        base,
        {{"preview revision nonempty",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->revision.clear();
          },
          Error::invalid_command},
         {"preview tied to expected revision",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->revision = "other";
          },
          Error::invalid_command},
         {"preview Main nonnegative",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->main_remaining_ms =
                  -1;
          },
          Error::invalid_command},
         {"preview old cap nonnegative",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->old_cap_ms = -1;
          },
          Error::invalid_command},
         {"preview proposed cap nonnegative",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->proposed_cap_ms =
                  -1;
          },
          Error::invalid_command},
         {"preview target matches command",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->proposed_cap_ms =
                  50;
          },
          Error::invalid_command},
         {"preview warning needs actual excess",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->main_remaining_ms =
                  60;
          },
          Error::invalid_command},
         {"matching revision must have matching old cap",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).warning_preview->old_cap_ms = 110;
          },
          Error::invalid_command},
         {"removed cap cannot carry warning preview",
          [](auto &x) {
              std::get<TimerCapReplacement>(x.command.payload).maximum_remaining_ms.reset();
              std::get<TimerCapReplacement>(x.command.payload).choice = TimerCapChoice::abort;
          },
          Error::invalid_command}});
    auto abort_remove = input(TimerCapReplacement{{}, TimerCapChoice::abort, {}}, v);
    cases({{"explicit Abort can cancel cap removal", abort_remove, observed(Status::aborted, v)}});
}

TEST(TimerEditorial, ObservationsAreCompleteAndNeverInvented) {
    invalid_cases(input(TimerSignedDelta{}),
                  {{"at least one explicit observation", [](auto &x) { x.observations.clear(); },
                    Error::invalid_observation},
                   {"negative elapsed rejected", [](auto &x) { x.observations[0].elapsed_ms = -1; },
                    Error::invalid_observation},
                   {"final UTC must match last segment", [](auto &x) { x.final_utc_ms = 501; },
                    Error::invalid_observation},
                   {"later malformed segment not ignored",
                    [](auto &x) { x.observations.push_back({-1, 500, false}); },
                    Error::invalid_observation}});
}

TEST(TimerEditorial, UtcExtremaAreNotDurationsOrAnElapsedSum) {
    auto raw = input(TimerDeadlineReplacement{{}}, values(1000));
    raw.observations = {{maximum, minimum, false}, {maximum, maximum, false}};
    raw.final_utc_ms = maximum;
    auto done = values(0);
    done.timer.lifecycle = TimerLifecycle::Finished;
    auto exp = accepted(done, done);
    exp.observations = {{Transition::MainExpired, Transition::Finished}, {}};
    auto wall = input(TimerSignedDelta{0, {}}, values(1000));
    wall.observations = {{100, maximum, false}};
    wall.final_utc_ms = maximum;
    auto negative_utc = input(TimerDeadlineReplacement{minimum}, values(50));
    negative_utc.observations = {{0, minimum, true}};
    negative_utc.final_utc_ms = minimum;
    auto hard = values(50);
    hard.timer.lifecycle = TimerLifecycle::Finished;
    hard.config.hard_end_utc_ms = minimum;
    cases({{"two valid huge elapsed segments need no signed sum", raw, exp},
           {"disabled deadline UTCmax is not elapsed", wall,
            accepted(values(900), values(900), Adjustment{})},
           {"negative UTC deadline equality is valid", negative_utc,
            accepted(values(50), hard, {}, {Transition::HardEndReached, Transition::Finished})}});
}

TEST(TimerEditorial, StagePrioritySeparatesValidationAccessRevisionAndPreview) {
    auto raw = input(TimerSignedDelta{});
    raw.current.values.time_debt_ms = -1;
    raw.command.operation.clear();
    raw.access.permission_revision.reset();
    raw.observations.clear();
    expect_error(raw, Error::invalid_snapshot);
    raw.current.values.time_debt_ms = 0;
    expect_error(raw, Error::invalid_command);
    raw.command.operation = "o";
    expect_error(raw, Error::invalid_access);
    raw.access.permission_revision = "p";
    expect_error(raw, Error::invalid_observation);
    auto bad_deny = input(TimerSignedDelta{});
    bad_deny.access = {TimerEditorialAccessState::denied, {}};
    bad_deny.current.values.timer.main_remaining_ms = -1;
    expect_error(bad_deny, Error::invalid_snapshot);
    auto v = values(100);
    v.maximum_remaining_ms = 60;
    auto preview = input(
        TimerCapReplacement{40, TimerCapChoice::reduce, TimerCapWarningPreview{"r", 100, 120, 40}},
        v);
    preview.current.revision = "r2";
    auto denied = preview;
    denied.access = {TimerEditorialAccessState::denied, {}};
    cases({{"revision conflict precedes dependent old-cap mismatch", preview,
            early(Status::revision_conflict)},
           {"access refusal precedes revision/dependent preview", denied,
            early(Status::access_denied)}});
    preview.current.revision = "r";
    expect_error(preview, Error::invalid_command);
    auto malformed = denied;
    std::get<TimerCapReplacement>(malformed.command.payload).warning_preview->proposed_cap_ms = 1;
    expect_error(malformed, Error::invalid_command);
}

TEST(TimerEditorial, ReferenceAndStepProfilesHaveExactBoundaries) {
    auto v = values(1);
    auto raw = input(TimerSignedDelta{}, v);
    raw.command.operation = std::string(4096, 'o');
    cases({{"exact reference4096", raw, accepted(v, v, Adjustment{})}});
    raw.command.operation.push_back('o');
    expect_error(raw, Error::input_limit);
    auto wider = TimerEditorialLimits{};
    wider.reference_bytes = 4097;
    cases({{"larger explicit reference profile", raw, accepted(v, v, Adjustment{}), wider}});
    auto optional = input(TimerSignedDelta{}, v);
    optional.command.reason = std::string(4097, 'x');
    expect_error(optional, Error::input_limit);
    auto steps = input(TimerSignedDelta{}, v);
    steps.observations.assign(4096, TimerClockStep{0, 500, false});
    auto all = accepted(v, v, Adjustment{});
    all.observations.assign(4096, Trace{});
    cases({{"all4096 explicit steps and batches retained", steps, all}});
    steps.observations.push_back({0, 500, false});
    expect_error(steps, Error::input_limit);
    wider = {};
    wider.observation_steps = 4097;
    all.observations.push_back({});
    cases({{"larger explicit step profile", steps, all, wider}});
    auto tight = TimerEditorialLimits{};
    tight.observation_steps = 1;
    auto two = input(TimerSignedDelta{}, v);
    two.observations.push_back({0, 500, false});
    expect_error(two, Error::input_limit, tight);
    cases({{"one-step selected profile fits exactly", input(TimerSignedDelta{}, v),
            accepted(v, v, Adjustment{}), tight}});
}

TEST(TimerEditorial, CompleteInputAndResultBytesIncludeWholeAudit) {
    auto v = values(1);
    auto zero = input(TimerSignedDelta{}, v);
    auto exact = TimerEditorialLimits{};
    exact.input_owned_bytes = 82;
    exact.result_owned_bytes = 224;
    cases({{"API literal82/224 includes six components and fixed batches", zero,
            accepted(v, v, Adjustment{}), exact}});
    auto short_input = exact;
    short_input.input_owned_bytes = 81;
    expect_error(zero, Error::input_limit, short_input);
    auto short_result = exact;
    short_result.result_owned_bytes = 223;
    expect_error(zero, Error::result_limit, short_result);
    auto denied = zero;
    denied.access = {TimerEditorialAccessState::denied, {}};
    exact.input_owned_bytes = 81;
    exact.result_owned_bytes = 92;
    cases({{"API denied literal81/92 still owns all input", denied, early(Status::access_denied),
            exact}});
    short_result = exact;
    short_result.result_owned_bytes = 91;
    expect_error(denied, Error::result_limit, short_result);
    auto cap_before = values(100, 5);
    cap_before.maximum_remaining_ms = 120;
    auto reduce = input(TimerCapReplacement{60, TimerCapChoice::reduce, {}}, cap_before);
    auto cap_after = values(60, 5);
    cap_after.maximum_remaining_ms = 60;
    exact.input_owned_bytes = 93;
    exact.result_owned_bytes = 284;
    cases({{"API Reduce literal93/284 includes warning and optional values", reduce,
            accepted(cap_before, cap_after, Adjustment{-40, -40, 0, 0, 0, 0}, {},
                     Warning{100, 120, 60, 40}),
            exact}});
    short_input = exact;
    short_input.input_owned_bytes = 92;
    expect_error(reduce, Error::input_limit, short_input);
    short_result = exact;
    short_result.result_owned_bytes = 283;
    expect_error(reduce, Error::result_limit, short_result);
    auto review_before = values(100, 7);
    auto review = input(TimerSignedDelta{-150, RefundDeficitPolicy::RequireReview}, review_before);
    exact = {};
    exact.input_owned_bytes = 83;
    exact.result_owned_bytes = 186;
    cases({{"held request still owns complete audit83/186", review,
            observed(Status::timer_review_required, review_before,
                     Adjustment{-150, 0, 0, 0, 0, -150}),
            exact}});
    short_result = exact;
    short_result.result_owned_bytes = 185;
    expect_error(review, Error::result_limit, short_result);
    auto two = zero;
    two.observations.push_back({0, 500, false});
    auto exp = accepted(v, v, Adjustment{});
    exp.observations = {{}, {}};
    exact = {};
    exact.input_owned_bytes = 99;
    exact.result_owned_bytes = 246;
    cases({{"second step17 and batch5 counted99/246", two, exp, exact}});
    short_input = exact;
    short_input.input_owned_bytes = 98;
    expect_error(two, Error::input_limit, short_input);
    short_result = exact;
    short_result.result_owned_bytes = 245;
    expect_error(two, Error::result_limit, short_result);
}

TEST(TimerEditorial, AllProfilesRejectInvalidAndOverflowingOptionsBeforeWork) {
    using Member = std::uint64_t TimerEditorialLimits::*;
    const std::array<Member, 4> members{
        &TimerEditorialLimits::reference_bytes, &TimerEditorialLimits::observation_steps,
        &TimerEditorialLimits::input_owned_bytes, &TimerEditorialLimits::result_owned_bytes};
    auto invalid = input(TimerSignedDelta{});
    invalid.command.operation.clear();
    for (std::size_t i = 0; i < members.size(); ++i) {
        SCOPED_TRACE(i);
        auto limit = TimerEditorialLimits{};
        limit.*members[i] = 0;
        expect_error(invalid, Error::invalid_limits, limit);
        limit.*members[i] = std::numeric_limits<std::uint64_t>::max();
        expect_error(invalid, Error::invalid_limits, limit);
    }
    auto refs = TimerEditorialLimits{};
    refs.reference_bytes = UINT64_C(2305843009213693952);
    expect_error(invalid, Error::invalid_limits, refs);
    for (const auto count : {UINT64_C(3074457345618258603), UINT64_C(838488366986797801)}) {
        auto steps = TimerEditorialLimits{};
        steps.observation_steps = count;
        expect_error(invalid, Error::invalid_limits, steps);
    }
    auto malformed = input(TimerSignedDelta{});
    malformed.current.values.time_debt_ms = -1;
    malformed.command.reason = std::string(4097, 'x');
    expect_error(malformed, Error::input_limit);
    auto result_small = TimerEditorialLimits{};
    result_small.result_owned_bytes = 1;
    auto bad = input(TimerSignedDelta{});
    bad.command.operation.clear();
    expect_error(bad, Error::invalid_command, result_small);
}
} // namespace
