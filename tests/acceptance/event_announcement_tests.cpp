// SPDX-License-Identifier: GPL-3.0-or-later
// #792 · owner decisions 232–234 and 237, advisor decision 244 A.
#include "event_announcement_producer.hpp"

#include <array>
#include <gtest/gtest.h>
#include <initializer_list>
#include <limits>
#include <utility>

namespace {
using namespace std::chrono_literals;
using namespace subathon::server;
using namespace subathon::server::event_announcement_owner;
using Ms = std::chrono::milliseconds;
using Authority = ActionAuthority;
using Completion = ApplicationCompletion;
using Disposition = RecoveryDisposition;

EventAnnouncementAction action(std::string event, std::string version = "V1") {
    return {{event, std::nullopt, event, std::nullopt},
            {"rule", "rule-v2", "A", "2", 0},
            std::move(version)};
}

const auto E1 = action("E1");
const auto E2 = action("E2", "V2");
const auto E3 = action("E3", "V3");
const auto E4 = action("E4", "V4");

Admission admission(const EventAnnouncementAction &source,
                    decltype(AuthoredVersion::duration) duration) {
    return {source, AuthoredVersion{source.presentation_version_ref, duration},
            Cause::event_announcement, Completion::committed, Authority::current};
}

StartFact allowed(const EventAnnouncementAction &source) { return {source, Authority::current}; }

constexpr auto first_lifetime = "first owner lifetime";
constexpr auto next_lifetime = "next owner lifetime";

class EventAnnouncementTest : public testing::Test {
  protected:
    Producer owner{first_lifetime};
    std::string_view lifetime = first_lifetime;

    void admit(std::initializer_list<Admission> arrivals) {
        owner.admit({arrivals.begin(), arrivals.size()});
    }

    ClockCut cut(std::optional<Ms> sample) const { return {lifetime, sample}; }

    void advance(std::optional<Ms> sample, std::initializer_list<StartFact> facts) {
        owner.advance({cut(sample), {facts.begin(), facts.size()}});
    }

    void restart(std::initializer_list<RetainedAnnouncement> retained) {
        lifetime = next_lifetime;
        owner.restart(lifetime, {retained.begin(), retained.size()});
    }

    void cancel(CancelStuckEventAnnouncement request, std::initializer_list<StartFact> facts,
                CommandAuthority authority = CommandAuthority::current,
                Completion completion = Completion::committed) {
        owner.cancel(std::move(request),
                     {authority, completion, {cut(std::nullopt), {facts.begin(), facts.size()}}});
    }

    void resume(ResumeEventAnnouncementQueue request, std::optional<Ms> sample,
                std::initializer_list<StartFact> facts,
                CommandAuthority authority = CommandAuthority::current) {
        owner.resume(
            std::move(request),
            {authority, Completion::committed, {cut(sample), {facts.begin(), facts.size()}}});
    }

    CurrentEventAnnouncement current(Ms sample) {
        return owner.current(cut(sample), Disclosure::allowed);
    }

    void absent(Ms sample) {
        EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(current(sample)))
            << "expected known absence at sample " << sample.count();
    }

    std::optional<std::string> active(Ms sample, const EventAnnouncementAction &source, Ms elapsed,
                                      Ms duration) {
        SCOPED_TRACE(sample.count());
        const auto view = current(sample);
        const auto *window = std::get_if<ActiveEventAnnouncement>(&view);
        if (!window) {
            ADD_FAILURE() << "expected active " << source.event.event_ref;
            return std::nullopt;
        }
        EXPECT_EQ(window->occurrence.source, source);
        EXPECT_FALSE(window->occurrence.occurrence_ref.empty());
        EXPECT_EQ(window->elapsed, elapsed);
        EXPECT_EQ(window->duration, duration);
        return window->occurrence.occurrence_ref;
    }

    void stuck(const EventAnnouncementAction &head) {
        const auto view = owner.management(Disclosure::allowed);
        const auto *item = std::get_if<StuckEventAnnouncement>(&view);
        ASSERT_NE(item, nullptr) << "expected stuck " << head.event.event_ref;
        EXPECT_EQ(item->head, head);
    }

    std::optional<std::string> held(std::optional<EventAnnouncementQueueHead> next) {
        const auto view = owner.management(Disclosure::allowed);
        const auto *hold = std::get_if<HeldEventAnnouncementQueue>(&view);
        if (!hold) {
            ADD_FAILURE() << "expected operator Hold";
            return std::nullopt;
        }
        EXPECT_FALSE(hold->hold_ref.empty());
        EXPECT_EQ(hold->next_head, next);
        return hold->hold_ref;
    }

    void no_stuck() {
        EXPECT_TRUE(std::holds_alternative<NoStuckEventAnnouncement>(
            owner.management(Disclosure::allowed)));
    }
};

TEST_F(EventAnnouncementTest, PlanDeliveryAndWaitingCannotPublishCurrent) {
    for (const auto cause :
         {Cause::plan, Cause::simulation, Cause::queued_job, Cause::delivered_job}) {
        SCOPED_TRACE(static_cast<int>(cause));
        auto proposal = admission(E1, 600ms);
        proposal.cause = cause;
        admit({proposal});
        advance(1000ms, {allowed(E1)});
        absent(1000ms);
    }

    admit({admission(E1, 600ms)});
    absent(1000ms);
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUnavailable>(
        owner.current(cut(1000ms), Disclosure::allowed, SelectionEvidence::unavailable)));
    advance(1500ms, {allowed(E1)});
    active(1500ms, E1, 0ms, 600ms);
}

TEST_F(EventAnnouncementTest, ActualActivationReconnectAndExclusiveEnd) {
    admit({admission(E1, 600ms)});
    advance(1500ms, {allowed(E1)});
    const auto original = active(1500ms, E1, 0ms, 600ms);
    EXPECT_EQ(active(1800ms, E1, 300ms, 600ms), original);
    EXPECT_EQ(active(2099ms, E1, 599ms, 600ms), original);
    absent(2100ms);
    absent(2200ms);
}

TEST_F(EventAnnouncementTest, FifoWaitsForActualOwnerTurnsWithoutCutoffOrBackdating) {
    admit({admission(E1, 600ms)});
    advance(1500ms, {allowed(E1)});
    admit({admission(E2, 300ms)});
    advance(1750ms, {allowed(E1), allowed(E2)});
    admit({admission(E3, 400ms)});
    advance(1760ms, {allowed(E1), allowed(E2), allowed(E3)});
    active(1800ms, E1, 300ms, 600ms);
    no_stuck();
    absent(2100ms);

    advance(2120ms, {allowed(E2), allowed(E3)});
    active(2120ms, E2, 0ms, 300ms);
    absent(2420ms);
    absent(2439ms);
    advance(2440ms, {allowed(E3)});
    active(2440ms, E3, 0ms, 400ms);
}

TEST_F(EventAnnouncementTest, OneCommittedPassPreservesSelectedActionOrder) {
    auto first = action("E1", "V2");
    const auto second = E1;
    first.action.rule_ref = "rule-z";
    admit({admission(first, 300ms), admission(second, 600ms)});
    advance(1000ms, {allowed(second), allowed(first)});
    active(1000ms, first, 0ms, 300ms);
    advance(1300ms, {allowed(second), allowed(first)});
    active(1300ms, second, 0ms, 600ms);
}

TEST_F(EventAnnouncementTest, RetriesAndConflictsCannotCreateNewOccurrence) {
    const auto changed_version = action("E1", "V2");
    auto changed_lineage = E1;
    changed_lineage.event.parent_ref = "parent";
    changed_lineage.event.root_ref = "root";
    changed_lineage.event.cause = E2.action;

    admit({admission(E1, 600ms)});
    advance(1500ms, {allowed(E1)});
    const auto original = active(1500ms, E1, 0ms, 600ms);
    admit({admission(E1, 600ms)});
    advance(1750ms, {allowed(E1)});
    EXPECT_EQ(active(1750ms, E1, 250ms, 600ms), original);

    admit({admission(changed_version, 900ms), admission(changed_lineage, 600ms)});
    advance(1800ms, {allowed(changed_version), allowed(changed_lineage), allowed(E1)});
    EXPECT_EQ(active(1800ms, E1, 300ms, 600ms), original);
    admit({admission(E1, 600ms)});
    advance(2250ms, {allowed(changed_version), allowed(changed_lineage), allowed(E1)});
    absent(2250ms);

    const auto same_version = action("E2");
    admit({admission(same_version, 600ms)});
    advance(2260ms, {allowed(same_version)});
    const auto distinct = active(2260ms, same_version, 0ms, 600ms);
    EXPECT_NE(distinct, original);
}

TEST_F(EventAnnouncementTest, OmittedDurationUsesTenSeconds) {
    admit({admission(E1, DurationDeclaration::omitted)});
    advance(1500ms, {allowed(E1)});
    active(1500ms, E1, 0ms, 10000ms);
    active(11499ms, E1, 9999ms, 10000ms);
    absent(11500ms);
}

TEST_F(EventAnnouncementTest, InvalidOrMissingVersionHasNoDurationFallback) {
    auto missing = admission(E1, 600ms);
    missing.version.reset();
    auto mismatched = admission(E1, 600ms);
    mismatched.version->version_ref = "V2";

    const std::array invalid{
        admission(E1, DurationDeclaration::explicit_null),
        admission(E1, 0ms),
        admission(E1, -1ms),
        admission(E1, DurationDeclaration::out_of_range),
        missing,
        mismatched,
    };
    for (std::size_t i = 0; i < invalid.size(); ++i) {
        SCOPED_TRACE(i);
        Producer fresh{lifetime};
        const std::array arrivals{invalid[i]};
        const std::array facts{allowed(E1)};
        fresh.admit(arrivals);
        fresh.advance({cut(1500ms), facts});
        EXPECT_FALSE(std::holds_alternative<ActiveEventAnnouncement>(
            fresh.current(cut(1500ms), Disclosure::allowed)));
    }
    admit({admission(E1, 600ms)});
    advance(1500ms, {allowed(E1)});
    active(2099ms, E1, 599ms, 600ms);
    absent(2100ms);
}

TEST_F(EventAnnouncementTest, CurrentDisclosureConcealsPresenceAndUnknownClock) {
    admit({admission(E1, 600ms)});
    advance(1500ms, {allowed(E1)});
    active(1800ms, E1, 300ms, 600ms);

    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUndisclosed>(
        owner.current(cut(1800ms), Disclosure::denied)));
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUndisclosed>(
        owner.current(cut(std::nullopt), Disclosure::denied, SelectionEvidence::unavailable)));
    absent(2100ms);
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUndisclosed>(
        owner.current(cut(2100ms), Disclosure::denied)));
}

TEST_F(EventAnnouncementTest, RevocationBlocksWaitingHeadWithoutRetimingCurrent) {
    admit({admission(E1, 600ms), admission(E2, 300ms), admission(E3, 400ms)});
    advance(1500ms, {allowed(E1), allowed(E2), allowed(E3)});
    const StartFact revoked_current{E1, Authority::grant_withdrawn};
    const StartFact blocked_head{E2, Authority::use_withdrawn};
    advance(1750ms, {revoked_current, blocked_head, allowed(E3)});
    active(1750ms, E1, 250ms, 600ms);
    stuck(E2);

    advance(2100ms, {blocked_head, allowed(E3)});
    absent(2100ms);
    stuck(E2);
    advance(2120ms, {allowed(E2), allowed(E3)});
    active(2120ms, E2, 0ms, 300ms);
}

TEST_F(EventAnnouncementTest, RestartDropsCurrentAndStartsProvedWaitingFromZero) {
    admit({admission(E1, 600ms), admission(E2, 300ms)});
    advance(1500ms, {allowed(E1), allowed(E2)});
    const auto old = active(1700ms, E1, 200ms, 600ms);

    restart({{E1, 600ms, Disposition::shown}, {E2, 300ms, Disposition::never_shown}});
    absent(20ms);
    admit({admission(E1, 600ms)});
    advance(80ms, {allowed(E1), allowed(E2)});
    const auto next = active(80ms, E2, 0ms, 300ms);
    EXPECT_NE(next, old);
    absent(380ms);
    advance(390ms, {allowed(E1), allowed(E2)});
    absent(390ms);
}

TEST_F(EventAnnouncementTest, UnknownRestartHeadRequiresPositiveDisposition) {
    for (const auto proof : {Disposition::shown, Disposition::never_shown}) {
        SCOPED_TRACE(static_cast<int>(proof));
        Producer recovered{first_lifetime};
        const std::array retained{
            RetainedAnnouncement{E2, 300ms, Disposition::uncertain},
            RetainedAnnouncement{E3, 400ms, Disposition::never_shown},
        };
        const std::array facts{allowed(E2), allowed(E3)};
        recovered.restart(next_lifetime, retained);
        recovered.admit(std::array{admission(E2, 300ms)});
        recovered.advance({{next_lifetime, 80ms}, facts});
        const auto waiting = recovered.management(Disclosure::allowed);
        const auto *head = std::get_if<StuckEventAnnouncement>(&waiting);
        EXPECT_NE(head, nullptr);
        if (head) {
            EXPECT_EQ(head->head, E2);
        }
        EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
            recovered.current({next_lifetime, 80ms}, Disclosure::allowed)));

        recovered.resolve(E2, proof);
        recovered.advance({{next_lifetime, 100ms}, facts});
        const auto view = recovered.current({next_lifetime, 100ms}, Disclosure::allowed);
        const auto *window = std::get_if<ActiveEventAnnouncement>(&view);
        EXPECT_NE(window, nullptr);
        if (window) {
            EXPECT_EQ(window->occurrence.source, proof == Disposition::shown ? E3 : E2);
            EXPECT_EQ(window->elapsed, 0ms);
            EXPECT_EQ(window->duration, proof == Disposition::shown ? 400ms : 300ms);
        }
    }
}

TEST_F(EventAnnouncementTest, SilentAcceptanceDoesNotStartOrdinaryAnnouncement) {
    auto silent = admission(E1, 600ms);
    silent.cause = Cause::silent_acceptance;
    admit({silent, admission(E2, 300ms)});
    advance(50ms, {allowed(E1), allowed(E2)});
    const auto original = active(50ms, E2, 0ms, 300ms);
    EXPECT_EQ(active(100ms, E2, 50ms, 300ms), original);
    EXPECT_EQ(active(170ms, E2, 120ms, 300ms), original);
}

TEST_F(EventAnnouncementTest, ExactElapsedSurvivesUnqualifiedOrRegressedCuts) {
    constexpr auto largest = Ms::max();
    const auto start = largest - 1ms;
    admit({admission(E1, 2ms)});
    advance(start, {allowed(E1)});
    const auto original = active(start, E1, 0ms, 2ms);

    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUnavailable>(
        owner.current(cut(start - 1ms), Disclosure::allowed)));
    advance(std::nullopt, {allowed(E1)});
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUnavailable>(
        owner.current(cut(std::nullopt), Disclosure::allowed)));
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUnavailable>(
        owner.current({next_lifetime, largest}, Disclosure::allowed)));
    EXPECT_EQ(active(largest, E1, 1ms, 2ms), original);
}

TEST_F(EventAnnouncementTest, OnlyNormallyCommittedCurrentlyAuthorizedInputIsAdmitted) {
    for (const auto completion :
         {Completion::staged, Completion::rolled_back, Completion::unknown, Completion::abnormal}) {
        auto candidate = admission(E1, 600ms);
        candidate.completion = completion;
        admit({candidate});
    }
    for (const auto denied : {Authority::use_withdrawn, Authority::grant_withdrawn,
                              Authority::execution_gate_unavailable,
                              Authority::stale_checked_snapshot, Authority::definition_missing}) {
        auto candidate = admission(E1, 600ms);
        candidate.authority = denied;
        admit({candidate});
    }
    admit({admission(E2, 300ms)});
    advance(1500ms, {allowed(E1), allowed(E2)});
    active(1500ms, E2, 0ms, 300ms);
    advance(1800ms, {allowed(E1), allowed(E2)});
    absent(1800ms);
}

TEST_F(EventAnnouncementTest, StartRequiresCurrentFactsForExactHead) {
    const auto different_version = action("E1", "V2");
    admit({admission(E1, 600ms), admission(E2, 300ms)});
    advance(1500ms, {allowed(different_version), allowed(E2)});
    stuck(E1);
    absent(1500ms);
    for (const auto denied : {Authority::grant_withdrawn, Authority::execution_gate_unavailable,
                              Authority::stale_checked_snapshot, Authority::definition_missing}) {
        SCOPED_TRACE(static_cast<int>(denied));
        advance(1500ms, {{E1, denied}, allowed(E2)});
        stuck(E1);
        absent(1500ms);
    }
    advance(1600ms, {allowed(E1), allowed(E2)});
    active(1600ms, E1, 0ms, 600ms);
}

TEST_F(EventAnnouncementTest, LaterVersionDoesNotRetimeTheCapturedVersion) {
    admit({admission(E1, 600ms)});
    admit({admission(E2, 900ms)});
    advance(1500ms, {allowed(E1), allowed(E2)});
    active(2099ms, E1, 599ms, 600ms);
    absent(2100ms);
    advance(2100ms, {allowed(E2)});
    active(2100ms, E2, 0ms, 900ms);
}

TEST_F(EventAnnouncementTest, StaleOrRefusedCancellationPreservesTheHead) {
    const StartFact blocked{E2, Authority::grant_withdrawn};
    admit({admission(E2, 300ms), admission(E3, 400ms)});
    advance(2170ms, {blocked, allowed(E3)});
    stuck(E2);

    cancel({E2}, {blocked, allowed(E3)}, CommandAuthority::withdrawn);
    stuck(E2);
    cancel({E2, EventAnnouncementAfterCancel::continue_queue}, {blocked, allowed(E3)},
           CommandAuthority::withdrawn);
    stuck(E2);
    cancel({E2}, {blocked, allowed(E3)}, CommandAuthority::current, Completion::unknown);
    stuck(E2);
    cancel({E2, EventAnnouncementAfterCancel::continue_queue}, {blocked, allowed(E3)},
           CommandAuthority::current, Completion::unknown);
    stuck(E2);

    // The same head became eligible before the command committed.
    cancel({E2}, {allowed(E2), allowed(E3)});
    no_stuck();
    cancel({E2, EventAnnouncementAfterCancel::continue_queue}, {allowed(E2), allowed(E3)});
    no_stuck();
    advance(2200ms, {allowed(E2), allowed(E3)});
    active(2200ms, E2, 0ms, 300ms);
}

TEST_F(EventAnnouncementTest, ContinueCancelsExactHeadAndStaleRetryCannotCancelSuccessor) {
    const StartFact blocked_e2{E2, Authority::grant_withdrawn};
    const StartFact blocked_e3{E3, Authority::execution_gate_unavailable};
    admit({admission(E2, 300ms), admission(E3, 400ms)});
    advance(2170ms, {blocked_e2, allowed(E3)});
    stuck(E2);

    const CancelStuckEventAnnouncement request{E2, EventAnnouncementAfterCancel::continue_queue};
    cancel(request, {blocked_e2, allowed(E3)});
    no_stuck();
    absent(2190ms);

    // This time E3 is the exact stuck head. E2's request cannot remove it or
    // create a Hold, even though cancellation of the current head is possible.
    advance(2195ms, {blocked_e3});
    stuck(E3);
    cancel(request, {blocked_e3});
    stuck(E3);
    cancel({E2}, {blocked_e3});
    stuck(E3);
    advance(2200ms, {allowed(E2), allowed(E3)});
    active(2200ms, E3, 0ms, 400ms);
    admit({admission(E2, 300ms)});
    advance(2600ms, {allowed(E2), allowed(E3)});
    absent(2600ms);
}

TEST_F(EventAnnouncementTest, ManagementDenialConcealsStuckAndHold) {
    const StartFact blocked{E2, Authority::grant_withdrawn};
    admit({admission(E2, 300ms), admission(E3, 400ms)});
    advance(2170ms, {blocked, allowed(E3)});
    stuck(E2);
    EXPECT_TRUE(
        std::holds_alternative<EventAnnouncementUndisclosed>(owner.management(Disclosure::denied)));
    absent(2170ms);

    cancel({E2}, {blocked, allowed(E3)});
    const auto original = held(EventAnnouncementQueueHead{E3, false});
    EXPECT_TRUE(
        std::holds_alternative<EventAnnouncementUndisclosed>(owner.management(Disclosure::denied)));
    advance(2200ms, {allowed(E2), allowed(E3)});
    EXPECT_EQ(held(EventAnnouncementQueueHead{E3, false}), original);
    absent(2200ms);
}

TEST_F(EventAnnouncementTest, ResumeMatchesHoldAndHeadEvenWithSuffixArrival) {
    const StartFact blocked{E2, Authority::grant_withdrawn};
    const auto check = [&](bool suffix_arrives) {
        SCOPED_TRACE(suffix_arrives);
        const std::array arrivals{admission(E2, 300ms), admission(E3, 400ms)};
        const std::array blocked_facts{blocked, allowed(E3)};
        const std::array start_facts{allowed(E3), allowed(E4)};
        Producer fresh{lifetime};
        fresh.admit(arrivals);
        fresh.advance({cut(2170ms), blocked_facts});
        fresh.cancel(
            {E2},
            {CommandAuthority::current, Completion::committed, {cut(std::nullopt), blocked_facts}});

        const auto view = fresh.management(Disclosure::allowed);
        const auto *hold = std::get_if<HeldEventAnnouncementQueue>(&view);
        ASSERT_NE(hold, nullptr);
        ASSERT_FALSE(hold->hold_ref.empty());
        EXPECT_EQ(hold->next_head, (EventAnnouncementQueueHead{E3, false}));
        const ResumeEventAnnouncementQueue request{hold->hold_ref, E3};

        if (suffix_arrives) {
            fresh.admit(std::array{admission(E4, 500ms)});
        }

        fresh.resume(
            request,
            {CommandAuthority::current, Completion::committed, {cut(2200ms), start_facts}});
        EXPECT_TRUE(std::holds_alternative<NoStuckEventAnnouncement>(
            fresh.management(Disclosure::allowed)));
        const auto started = fresh.current(cut(2200ms), Disclosure::allowed);
        const auto *window = std::get_if<ActiveEventAnnouncement>(&started);
        ASSERT_NE(window, nullptr);
        EXPECT_EQ(window->occurrence.source, E3);
        EXPECT_EQ(window->elapsed, 0ms);
        EXPECT_EQ(window->duration, 400ms);
        EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
            fresh.current(cut(2600ms), Disclosure::allowed)));
        fresh.advance({cut(2601ms), start_facts});
        const auto next = fresh.current(cut(2601ms), Disclosure::allowed);
        if (suffix_arrives) {
            const auto *successor = std::get_if<ActiveEventAnnouncement>(&next);
            ASSERT_NE(successor, nullptr);
            EXPECT_EQ(successor->occurrence.source, E4);
            EXPECT_EQ(successor->elapsed, 0ms);
        } else {
            EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(next));
        }
    };
    for (const bool suffix_arrives : {false, true}) {
        check(suffix_arrives);
    }
}

TEST_F(EventAnnouncementTest, ResumeRechecksCommandAuthorityAndNeedsQualifiedStartTurn) {
    const StartFact blocked{E2, Authority::grant_withdrawn};
    admit({admission(E2, 300ms), admission(E3, 400ms)});
    advance(2170ms, {blocked, allowed(E3)});
    cancel({E2}, {blocked, allowed(E3)});
    const auto hold = held(EventAnnouncementQueueHead{E3, false});
    ASSERT_TRUE(hold);

    const ResumeEventAnnouncementQueue request{*hold, E3};
    resume(request, 2200ms, {allowed(E3)}, CommandAuthority::withdrawn);
    EXPECT_EQ(held(EventAnnouncementQueueHead{E3, false}), hold);
    absent(2200ms);

    resume(request, std::nullopt, {allowed(E3)});
    no_stuck();
    absent(2201ms);
    advance(2210ms, {allowed(E3)});
    active(2210ms, E3, 0ms, 400ms);
}

TEST_F(EventAnnouncementTest, ResumeRejectsChangedNextHeadInSameHold) {
    const StartFact blocked_e2{E2, Authority::grant_withdrawn};
    const StartFact blocked_e3{E3, Authority::execution_gate_unavailable};
    admit({admission(E2, 300ms), admission(E3, 400ms), admission(E4, 500ms)});
    advance(2170ms, {blocked_e2, allowed(E3), allowed(E4)});
    cancel({E2}, {blocked_e2, allowed(E3), allowed(E4)});
    const auto hold = held(EventAnnouncementQueueHead{E3, false});
    ASSERT_TRUE(hold);
    const ResumeEventAnnouncementQueue original{*hold, E3};

    advance(2180ms, {blocked_e3, allowed(E4)});
    EXPECT_EQ(held(EventAnnouncementQueueHead{E3, true}), hold);
    cancel({E3}, {blocked_e3, allowed(E4)});
    ASSERT_EQ(held(EventAnnouncementQueueHead{E4, false}), hold);

    resume(original, 2200ms, {allowed(E4)});
    EXPECT_EQ(held(EventAnnouncementQueueHead{E4, false}), hold);
    absent(2200ms);
    resume({*hold, E4}, 2210ms, {allowed(E4)});
    active(2210ms, E4, 0ms, 500ms);
}

TEST_F(EventAnnouncementTest, ResumeRejectsDifferentHoldEvenWithMatchingHead) {
    const StartFact blocked_e2{E2, Authority::grant_withdrawn};
    const StartFact blocked_e3{E3, Authority::execution_gate_unavailable};
    admit({admission(E2, 300ms), admission(E3, 400ms)});
    advance(2170ms, {blocked_e2, allowed(E3)});
    cancel({E2}, {blocked_e2, allowed(E3)});
    const auto first = held(EventAnnouncementQueueHead{E3, false});
    ASSERT_TRUE(first);
    const ResumeEventAnnouncementQueue earlier{*first, E3};
    resume(earlier, std::nullopt, {allowed(E3)});
    no_stuck();

    advance(2180ms, {blocked_e3});
    admit({admission(E4, 500ms)});
    cancel({E3, EventAnnouncementAfterCancel::hold}, {blocked_e3, allowed(E4)});
    const auto second = held(EventAnnouncementQueueHead{E4, false});
    ASSERT_TRUE(second);
    EXPECT_NE(first, second);

    // Isolate the Hold comparison with the correct current head.
    resume({*first, E4}, 2200ms, {allowed(E4)});
    EXPECT_EQ(held(EventAnnouncementQueueHead{E4, false}), second);
    absent(2200ms);
    resume({*second, E4}, 2210ms, {allowed(E4)});
    active(2210ms, E4, 0ms, 500ms);
}

TEST_F(EventAnnouncementTest, EmptyHoldCanResumeButArrivalInvalidatesEmptyBasis) {
    const StartFact blocked_e2{E2, Authority::grant_withdrawn};
    const StartFact blocked_e3{E3, Authority::execution_gate_unavailable};
    admit({admission(E2, 300ms)});
    advance(2150ms, {blocked_e2});
    cancel({E2}, {blocked_e2});
    const auto first = held(std::nullopt);
    ASSERT_TRUE(first);
    resume({*first, std::nullopt}, 2170ms, {});
    no_stuck();
    absent(2170ms);

    admit({admission(E3, 400ms)});
    advance(2180ms, {blocked_e3});
    cancel({E3}, {blocked_e3});
    const auto second = held(std::nullopt);
    ASSERT_TRUE(second);
    EXPECT_NE(first, second);
    resume({*first, std::nullopt}, 2185ms, {});
    EXPECT_EQ(held(std::nullopt), second);
    const ResumeEventAnnouncementQueue empty_basis{*second, std::nullopt};
    admit({admission(E4, 500ms)});
    advance(2190ms, {allowed(E4)});
    EXPECT_EQ(held(EventAnnouncementQueueHead{E4, false}), second);
    resume(empty_basis, 2200ms, {allowed(E4)});
    EXPECT_EQ(held(EventAnnouncementQueueHead{E4, false}), second);
    absent(2200ms);
    resume({*second, E4}, 2210ms, {allowed(E4)});
    active(2210ms, E4, 0ms, 500ms);
}

TEST_F(EventAnnouncementTest, CancellationDuringHoldKeepsOrClearsItWithoutBypassingGates) {
    struct Scenario {
        EventAnnouncementAfterCancel choice;
        Authority successor_authority;
    };
    constexpr auto hold = EventAnnouncementAfterCancel::hold;
    constexpr auto continue_queue = EventAnnouncementAfterCancel::continue_queue;
    const std::array scenarios{
        Scenario{hold, Authority::current},
        Scenario{continue_queue, Authority::current},
        Scenario{continue_queue, Authority::grant_withdrawn},
    };

    const std::array initial{admission(E1, 600ms)};
    const std::array blocked_e1{StartFact{E1, Authority::use_withdrawn}};
    const auto check = [&](const Scenario &scenario) {
        SCOPED_TRACE(static_cast<int>(scenario.choice));
        SCOPED_TRACE(static_cast<int>(scenario.successor_authority));
        Producer fresh{lifetime};
        fresh.admit(initial);
        fresh.advance({cut(2150ms), blocked_e1});
        fresh.cancel(
            {E1},
            {CommandAuthority::current, Completion::committed, {cut(std::nullopt), blocked_e1}});
        const auto before = fresh.management(Disclosure::allowed);
        const auto *original_hold = std::get_if<HeldEventAnnouncementQueue>(&before);
        ASSERT_NE(original_hold, nullptr);
        EXPECT_FALSE(original_hold->hold_ref.empty());
        EXPECT_FALSE(original_hold->next_head);

        const std::array arrivals{admission(E2, 300ms), admission(E3, 400ms)};
        const std::array facts{StartFact{E2, Authority::grant_withdrawn},
                               StartFact{E3, scenario.successor_authority}};
        fresh.admit(arrivals);
        fresh.advance({cut(2180ms), facts});
        const auto waiting = fresh.management(Disclosure::allowed);
        const auto *held_head = std::get_if<HeldEventAnnouncementQueue>(&waiting);
        ASSERT_NE(held_head, nullptr);
        EXPECT_EQ(held_head->hold_ref, original_hold->hold_ref);
        EXPECT_EQ(held_head->next_head, (EventAnnouncementQueueHead{E2, true}));

        const auto request = scenario.choice == hold
                                 ? CancelStuckEventAnnouncement{E2}
                                 : CancelStuckEventAnnouncement{E2, continue_queue};
        fresh.cancel(
            request,
            {CommandAuthority::current, Completion::committed, {cut(std::nullopt), facts}});
        fresh.advance({cut(2200ms), facts});
        const auto management = fresh.management(Disclosure::allowed);
        const auto current = fresh.current(cut(2200ms), Disclosure::allowed);

        if (scenario.choice == hold) {
            const auto *still_held = std::get_if<HeldEventAnnouncementQueue>(&management);
            ASSERT_NE(still_held, nullptr);
            EXPECT_EQ(still_held->hold_ref, original_hold->hold_ref);
            EXPECT_EQ(still_held->next_head, (EventAnnouncementQueueHead{E3, false}));
            EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(current));
        } else if (scenario.successor_authority == Authority::current) {
            EXPECT_TRUE(std::holds_alternative<NoStuckEventAnnouncement>(management));
            const auto *window = std::get_if<ActiveEventAnnouncement>(&current);
            ASSERT_NE(window, nullptr);
            EXPECT_EQ(window->occurrence.source, E3);
            EXPECT_EQ(window->elapsed, 0ms);
            EXPECT_EQ(window->duration, 400ms);
        } else {
            const auto *head = std::get_if<StuckEventAnnouncement>(&management);
            ASSERT_NE(head, nullptr);
            EXPECT_EQ(head->head, E3);
            EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(current));
        }
    };
    for (const auto &scenario : scenarios) {
        check(scenario);
    }
}

} // namespace
