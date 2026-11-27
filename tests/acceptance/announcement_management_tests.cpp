// SPDX-License-Identifier: GPL-3.0-or-later
#include "announcement_management_candidate.hpp"

#include <array>
#include <gtest/gtest.h>

namespace {
using namespace subathon::server;
using namespace subathon::server::event_announcement_owner;
using namespace std::chrono_literals;
using Outcome = CheckedManagementOutcome;
using Grant = CommandAuthority;

EventAnnouncementAction action(std::string event) {
    return {
        {event, std::nullopt, event, std::nullopt}, {"rule", "v2", "A", "2", 0}, "presentation"};
}

const auto first = action("first");
const auto second = action("second");
const auto third = action("third");
constexpr auto owner_ref = "fixture owner observation";
constexpr auto lifetime = "fixture producer lifetime";

class AnnouncementManagementTest : public testing::Test {
  protected:
    Producer owner{lifetime};
    AnnouncementManagementCandidate binding{owner, owner_ref};

    void admit(const EventAnnouncementAction &source) {
        const Admission arrival{source, AuthoredVersion{source.presentation_version_ref, 100ms},
                                Cause::event_announcement, ApplicationCompletion::committed,
                                ActionAuthority::current};
        owner.admit(std::span{&arrival, 1});
    }

    static CheckedManagementTurn turn(std::span<const StartFact> facts = {},
                                      std::optional<std::chrono::milliseconds> elapsed = {}) {
        return {true, Grant::current, Grant::current, {{lifetime, elapsed}, facts}};
    }

    void expect_undisclosed() const {
        for (const auto grant : {Grant::withdrawn, Grant::unavailable}) {
            const auto observation = binding.read(grant);
            EXPECT_TRUE(std::holds_alternative<EventAnnouncementUndisclosed>(observation.view));
            EXPECT_TRUE(observation.owner_reference.empty());
        }
    }

    void expect_hold(std::string_view expected) const {
        const auto view = owner.management(Disclosure::allowed);
        const auto *hold = std::get_if<HeldEventAnnouncementQueue>(&view);
        ASSERT_NE(hold, nullptr);
        EXPECT_EQ(hold->hold_ref, expected);
    }

    std::string create_hold() {
        admit(first);
        const std::array facts{StartFact{first, ActionAuthority::grant_withdrawn}};
        owner.cancel({first}, {Grant::current, ApplicationCompletion::committed, turn(facts).turn});
        const auto view = owner.management(Disclosure::allowed);
        const auto *hold = std::get_if<HeldEventAnnouncementQueue>(&view);
        EXPECT_NE(hold, nullptr);
        return hold ? hold->hold_ref : std::string{};
    }
};

TEST_F(AnnouncementManagementTest, DisclosurePrecedesObservationAndNeverEchoesOwnerOnDenial) {
    admit(first);
    const auto denied = binding.read(Grant::withdrawn);
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUndisclosed>(denied.view));
    EXPECT_TRUE(denied.owner_reference.empty());
    const auto unknown = binding.read(Grant::unavailable);
    EXPECT_TRUE(std::holds_alternative<EventAnnouncementUndisclosed>(unknown.view));
    EXPECT_TRUE(unknown.owner_reference.empty());
}

TEST_F(AnnouncementManagementTest, DisclosureConcealsCapturedEmptyStuckAndHoldStates) {
    binding.advance_and_capture(turn({}, 10ms).turn);
    EXPECT_TRUE(
        std::holds_alternative<NoStuckEventAnnouncement>(binding.read(Grant::current).view));
    expect_undisclosed();

    admit(first);
    const std::array facts{StartFact{first, ActionAuthority::grant_withdrawn}};
    binding.advance_and_capture(turn(facts, 20ms).turn);
    EXPECT_TRUE(std::holds_alternative<StuckEventAnnouncement>(binding.read(Grant::current).view));
    expect_undisclosed();

    owner.cancel({first}, {Grant::current, ApplicationCompletion::committed, turn(facts).turn});
    binding.advance_and_capture(turn({}, 30ms).turn);
    EXPECT_TRUE(
        std::holds_alternative<HeldEventAnnouncementQueue>(binding.read(Grant::current).view));
    expect_undisclosed();
}

TEST_F(AnnouncementManagementTest, AuthorizedCaptureUsesExactOwnerAndInvalidates) {
    admit(first);
    EXPECT_TRUE(
        std::holds_alternative<EventAnnouncementUnavailable>(binding.read(Grant::current).view));
    const std::array facts{StartFact{first, ActionAuthority::grant_withdrawn}};
    binding.advance_and_capture(turn(facts, 30ms).turn);
    const auto captured = binding.read(Grant::current);
    EXPECT_EQ(captured.owner_reference, owner_ref);
    const auto *stuck = std::get_if<StuckEventAnnouncement>(&captured.view);
    ASSERT_NE(stuck, nullptr);
    EXPECT_EQ(stuck->head, first);
    binding.invalidate_capture();
    EXPECT_TRUE(
        std::holds_alternative<EventAnnouncementUnavailable>(binding.read(Grant::current).view));
}

TEST_F(AnnouncementManagementTest, EligibleHeadIsNotReportedStuckAfterProgression) {
    admit(first);
    const std::array facts{StartFact{first, ActionAuthority::current}};
    binding.advance_and_capture(turn(facts, 30ms).turn);
    admit(second);
    const std::array waiting{StartFact{second, ActionAuthority::current}};
    binding.advance_and_capture(turn(waiting, 40ms).turn);
    const auto capture = binding.read(Grant::current);
    EXPECT_EQ(capture.owner_reference, owner_ref);
    EXPECT_TRUE(std::holds_alternative<NoStuckEventAnnouncement>(capture.view));
}

TEST_F(AnnouncementManagementTest, CancelDisclosureGrantGatesMalformedShape) {
    admit(first);
    const CancelStuckEventAnnouncement malformed{first,
                                                 static_cast<EventAnnouncementAfterCancel>(99)};
    auto denied = turn();
    denied.disclosure = Grant::withdrawn;
    EXPECT_EQ(binding.cancel(owner_ref, malformed, denied), Outcome::refused);
    denied.disclosure = Grant::unavailable;
    EXPECT_EQ(binding.cancel(owner_ref, malformed, denied), Outcome::unavailable);
    denied.command = Grant::withdrawn;
    EXPECT_EQ(binding.cancel(owner_ref, malformed, denied), Outcome::unavailable);
    denied.owner_admitted = false;
    denied.disclosure = Grant::current;
    EXPECT_EQ(binding.cancel("former owner", malformed, denied), Outcome::unavailable);
    const auto state = owner.management(Disclosure::allowed);
    const auto *stuck = std::get_if<StuckEventAnnouncement>(&state);
    ASSERT_NE(stuck, nullptr);
    EXPECT_EQ(stuck->head, first);
}

TEST_F(AnnouncementManagementTest, CancelRequiresItsOwnGrantAndValidChoice) {
    admit(first);
    const std::array facts{StartFact{first, ActionAuthority::grant_withdrawn}};
    auto denied = turn(facts);
    denied.command = Grant::withdrawn;
    EXPECT_EQ(binding.cancel(owner_ref, {first}, denied), Outcome::refused);
    const CancelStuckEventAnnouncement malformed{first,
                                                 static_cast<EventAnnouncementAfterCancel>(99)};
    EXPECT_EQ(binding.cancel(owner_ref, malformed, turn(facts)), Outcome::refused);
    const auto view = owner.management(Disclosure::allowed);
    const auto *stuck = std::get_if<StuckEventAnnouncement>(&view);
    ASSERT_NE(stuck, nullptr);
    EXPECT_EQ(stuck->head, first);
}

TEST_F(AnnouncementManagementTest, ReplacedOwnerReferenceAndChangedHeadAreStaleBeforeFacts) {
    admit(first);
    EXPECT_EQ(binding.cancel("former owner", {first}, turn()), Outcome::stale);
    EXPECT_EQ(binding.cancel(owner_ref, {second}, turn()), Outcome::stale);
    EXPECT_EQ(binding.cancel(owner_ref, {first}, turn()), Outcome::unavailable);
    const auto state = owner.management(Disclosure::allowed);
    const auto *stuck = std::get_if<StuckEventAnnouncement>(&state);
    ASSERT_NE(stuck, nullptr);
    EXPECT_EQ(stuck->head, first);
}

TEST_F(AnnouncementManagementTest, CurrentEligibleHeadIsStaleForCancel) {
    admit(first);
    const std::array facts{StartFact{first, ActionAuthority::current}};
    EXPECT_EQ(binding.cancel(owner_ref, {first, EventAnnouncementAfterCancel::continue_queue},
                             turn(facts, 30ms)),
              Outcome::stale);
    EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
        owner.current({lifetime, 30ms}, Disclosure::allowed)));
    owner.advance(turn(facts, 30ms).turn);
    const auto active = owner.current({lifetime, 30ms}, Disclosure::allowed);
    const auto *window = std::get_if<ActiveEventAnnouncement>(&active);
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->occurrence.source, first);
}

TEST_F(AnnouncementManagementTest, PresentNonCurrentActionFactAllowsDefaultHold) {
    for (const auto authority :
         {ActionAuthority::use_withdrawn, ActionAuthority::grant_withdrawn,
          ActionAuthority::execution_gate_unavailable, ActionAuthority::stale_checked_snapshot,
          ActionAuthority::definition_missing}) {
        SCOPED_TRACE(static_cast<int>(authority));
        Producer subject{lifetime};
        AnnouncementManagementCandidate checked{subject, owner_ref};
        const Admission arrival{first, AuthoredVersion{first.presentation_version_ref, 100ms},
                                Cause::event_announcement, ApplicationCompletion::committed,
                                ActionAuthority::current};
        subject.admit(std::span{&arrival, 1});
        const std::array facts{StartFact{first, authority}};
        EXPECT_EQ(checked.cancel(owner_ref, {first}, turn(facts)), Outcome::applied);
        const auto state = subject.management(Disclosure::allowed);
        const auto *hold = std::get_if<HeldEventAnnouncementQueue>(&state);
        ASSERT_NE(hold, nullptr);
        EXPECT_FALSE(hold->hold_ref.empty());
        EXPECT_FALSE(hold->next_head);
    }
}

TEST_F(AnnouncementManagementTest, ContinueStartsNextHeadAtSameQualifiedCut) {
    admit(first);
    admit(second);
    const std::array facts{StartFact{first, ActionAuthority::grant_withdrawn},
                           StartFact{second, ActionAuthority::current}};
    EXPECT_EQ(binding.cancel(owner_ref, {first, EventAnnouncementAfterCancel::continue_queue},
                             turn(facts, 30ms)),
              Outcome::applied);
    const auto active = owner.current({lifetime, 30ms}, Disclosure::allowed);
    const auto *window = std::get_if<ActiveEventAnnouncement>(&active);
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->occurrence.source, second);
    EXPECT_EQ(window->elapsed, 0ms);
}

TEST_F(AnnouncementManagementTest, ContinueWithoutElapsedDoesNotStartNext) {
    const auto hold = create_hold();
    ASSERT_FALSE(hold.empty());
    admit(second);
    admit(third);
    const std::array facts{StartFact{second, ActionAuthority::grant_withdrawn},
                           StartFact{third, ActionAuthority::current}};
    EXPECT_EQ(binding.cancel(owner_ref, {second, EventAnnouncementAfterCancel::continue_queue},
                             turn(facts)),
              Outcome::applied);
    EXPECT_FALSE(
        std::holds_alternative<HeldEventAnnouncementQueue>(owner.management(Disclosure::allowed)));
    EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
        owner.current({lifetime, 30ms}, Disclosure::allowed)));
    owner.advance(turn(facts, 30ms).turn);
    const auto active = owner.current({lifetime, 30ms}, Disclosure::allowed);
    const auto *window = std::get_if<ActiveEventAnnouncement>(&active);
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->occurrence.source, third);
}

TEST_F(AnnouncementManagementTest, ResumeChecksBothGrantsBeforeObservationBasis) {
    const auto hold = create_hold();
    ASSERT_FALSE(hold.empty());
    auto denied = turn();
    denied.disclosure = Grant::withdrawn;
    EXPECT_EQ(binding.resume("former owner", {"guess", std::nullopt}, denied), Outcome::refused);
    denied.disclosure = Grant::unavailable;
    EXPECT_EQ(binding.resume(owner_ref, {hold, std::nullopt}, denied), Outcome::unavailable);
    EXPECT_TRUE(
        std::holds_alternative<HeldEventAnnouncementQueue>(owner.management(Disclosure::allowed)));
}

TEST_F(AnnouncementManagementTest, ResumeExactEmptyHoldRequiresCommandGrantAndExactReference) {
    const auto hold = create_hold();
    ASSERT_FALSE(hold.empty());
    auto denied = turn();
    denied.command = Grant::withdrawn;
    EXPECT_EQ(binding.resume(owner_ref, {hold, std::nullopt}, denied), Outcome::refused);
    expect_hold(hold);
    EXPECT_EQ(binding.resume(owner_ref, {"other Hold", std::nullopt}, turn()), Outcome::stale);
    expect_hold(hold);
    EXPECT_EQ(binding.resume(owner_ref, {hold, std::nullopt}, turn()), Outcome::applied);
    EXPECT_TRUE(
        std::holds_alternative<NoStuckEventAnnouncement>(owner.management(Disclosure::allowed)));
    EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
        owner.current({lifetime, 30ms}, Disclosure::allowed)));
}

TEST_F(AnnouncementManagementTest, CurrentFactDoesNotResolveUncertainRetainedDisposition) {
    const std::array retained{RetainedAnnouncement{first, 100ms, RecoveryDisposition::uncertain}};
    owner.restart("new producer lifetime", retained);
    const std::array facts{StartFact{first, ActionAuthority::current}};
    auto current = turn(facts);
    current.turn.clock.lifetime = "new producer lifetime";
    EXPECT_EQ(binding.cancel(owner_ref, {first}, current), Outcome::applied);
    const auto view = owner.management(Disclosure::allowed);
    const auto *held = std::get_if<HeldEventAnnouncementQueue>(&view);
    ASSERT_NE(held, nullptr);
    EXPECT_FALSE(held->next_head);
}

TEST_F(AnnouncementManagementTest, RetainedHeadAfterInPlaceRestartKeepsOwnerReference) {
    const auto hold = create_hold();
    ASSERT_FALSE(hold.empty());
    const std::array retained{
        RetainedAnnouncement{second, 100ms, RecoveryDisposition::never_shown}};
    owner.restart("new producer lifetime", retained);
    binding.invalidate_capture();
    auto current = turn();
    current.turn.clock.lifetime = "new producer lifetime";
    EXPECT_TRUE(
        std::holds_alternative<EventAnnouncementUnavailable>(binding.read(Grant::current).view));
    auto withdrawn = current;
    withdrawn.command = Grant::withdrawn;
    EXPECT_EQ(binding.resume(owner_ref, {hold, second}, withdrawn), Outcome::refused);
    expect_hold(hold);
    EXPECT_EQ(binding.resume(owner_ref, {hold, third}, current), Outcome::stale);
    expect_hold(hold);
    EXPECT_EQ(binding.resume(owner_ref, {hold, second}, current), Outcome::applied);
    EXPECT_FALSE(
        std::holds_alternative<HeldEventAnnouncementQueue>(owner.management(Disclosure::allowed)));
    EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
        owner.current({current.turn.clock.lifetime, 30ms}, Disclosure::allowed)));
}

TEST_F(AnnouncementManagementTest, CancelUsesCurrentStartFactsAndInvalidatesEarlierCapture) {
    admit(first);
    const std::array withdrawn{StartFact{first, ActionAuthority::grant_withdrawn}};
    binding.advance_and_capture(turn(withdrawn, 30ms).turn);
    ASSERT_TRUE(std::holds_alternative<StuckEventAnnouncement>(binding.read(Grant::current).view));
    const std::array eligible{StartFact{first, ActionAuthority::current}};
    EXPECT_EQ(binding.cancel(owner_ref, {first}, turn(eligible)), Outcome::stale);
    EXPECT_TRUE(
        std::holds_alternative<EventAnnouncementUnavailable>(binding.read(Grant::current).view));
}

TEST_F(AnnouncementManagementTest, ResumeExactHeadIgnoresSuffixAndDoesNotNeedElapsed) {
    const auto hold = create_hold();
    ASSERT_FALSE(hold.empty());
    admit(second);
    admit(third);
    EXPECT_EQ(binding.resume("former owner", {hold, second}, turn()), Outcome::stale);
    EXPECT_EQ(binding.resume(owner_ref, {hold, std::nullopt}, turn()), Outcome::stale);
    expect_hold(hold);
    EXPECT_EQ(binding.resume(owner_ref, {hold, second}, turn()), Outcome::applied);
    EXPECT_FALSE(
        std::holds_alternative<HeldEventAnnouncementQueue>(owner.management(Disclosure::allowed)));
    EXPECT_TRUE(std::holds_alternative<NoCurrentEventAnnouncement>(
        owner.current({lifetime, 30ms}, Disclosure::allowed)));
}

} // namespace
