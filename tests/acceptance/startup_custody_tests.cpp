// SPDX-License-Identifier: GPL-3.0-or-later
#include "startup_custody_candidate.hpp"

#include <gtest/gtest.h>

// Stale entry point      Cut where current could take effect                      All observations
// unchanged socket_options         StaleSocketCannotClaimTheMissingCurrentRole             yes
// listen_return, offer   StaleOfferAndLastListenReturnCannotEnablePublication     yes
// publish                StalePublishCannotConsumeEligibleCurrentHandoff         yes
// admit_observer         StaleIdentityCannotTouchReusedSocketOrRecord            yes
// release_observer, stop StaleStopAndReleaseCannotRetireCurrentReader             yes
// route                  StaleIdentityCannotTouchReusedSocketOrRecord            yes
// acknowledge            StaleAcknowledgmentsCannotRetireClosedCurrentRecord     yes
// cancel_cold            StaleColdCancelCannotReleaseFreshCurrentRecord          yes
// framework_exception    StaleIdentityCannotTouchReusedSocketOrRecord            yes

namespace {
using namespace subathon::server::startup_custody_candidate;

constexpr Startup current = 41;
constexpr Startup former = 40;
constexpr auto app = Role::application;
constexpr auto overlay = Role::overlay;
constexpr auto address = "127.0.0.1";
constexpr auto scratch = "synthetic-scratch";
const Snapshot expected{{{app, address, 43100}, true}, {{overlay, address, 43101}, true}, scratch};

std::array<Registration, 2> registrations(Role fixed) {
    const std::array<Registration, 2> app_fixed{{
        {Acceptor::first, {app, address, 43100}},
        {Acceptor::second, {overlay, address, 0}},
    }};
    const std::array<Registration, 2> overlay_fixed{{
        {Acceptor::first, {overlay, address, 43101}},
        {Acceptor::second, {app, address, 0}},
    }};
    return fixed == app ? app_fixed : overlay_fixed;
}

class StartupCustodyTest : public testing::Test {
  protected:
    Candidate owner{current, registrations(app), scratch};
    const BorrowedSocket app_socket{8, {address, 43100}};
    const BorrowedSocket overlay_socket{9, {address, 43101}};

    template <typename Call>
    void expect_stale_effect_free(Candidate &subject, Call call,
                                  const BorrowedSocket *borrowed = nullptr) {
        const auto before = subject.observations();
        const auto app_accesses = app_socket.accesses();
        const auto overlay_accesses = overlay_socket.accesses();
        const auto extra_accesses = borrowed ? borrowed->accesses() : 0U;
        call();
        EXPECT_EQ(subject.observations(), before);
        EXPECT_EQ(app_socket.accesses(), app_accesses);
        EXPECT_EQ(overlay_socket.accesses(), overlay_accesses);
        if (borrowed) {
            EXPECT_EQ(borrowed->accesses(), extra_accesses);
        }
    }

    std::array<unsigned, 2> roles(Candidate &subject, Role fixed = app) {
        const auto ephemeral = fixed == app ? overlay : app;
        const auto &fixed_socket = fixed == app ? app_socket : overlay_socket;
        const auto &ephemeral_socket = fixed == app ? overlay_socket : app_socket;
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, fixed, fixed_socket),
                  Turn::accepted);
        const auto fixed_accesses = fixed_socket.accesses();
        EXPECT_EQ(subject.socket_options(current, Acceptor::second, ephemeral, ephemeral_socket),
                  Turn::accepted);
        const auto ephemeral_accesses = ephemeral_socket.accesses();
        EXPECT_EQ(subject.listen_return(current, fixed), Turn::accepted);
        EXPECT_EQ(subject.listen_return(current, ephemeral), Turn::accepted);
        return fixed == app ? std::array{fixed_accesses, ephemeral_accesses}
                            : std::array{ephemeral_accesses, fixed_accesses};
    }

    std::array<unsigned, 2> ready(Candidate &subject, Role fixed = app) {
        const auto accesses = roles(subject, fixed);
        subject.offer(current);
        EXPECT_EQ(subject.publish(current), Turn::accepted);
        return accesses;
    }
};

TEST_F(StartupCustodyTest, MapsFixedAndEphemeralPortsInBothRegistrationOrders) {
    for (const auto fixed : {app, overlay}) {
        Candidate subject{current, registrations(fixed), scratch};
        const std::array before{app_socket.accesses(), overlay_socket.accesses()};
        const auto accesses = ready(subject, fixed);
        EXPECT_GT(accesses[0], before[0]);
        EXPECT_GT(accesses[1], before[1]);
        EXPECT_NE(subject.publish(current), Turn::accepted);
        const auto reads = subject.observations().record_reads;
        EXPECT_EQ(subject.admit_observer(current), expected);
        EXPECT_GT(subject.observations().record_reads, reads);
        EXPECT_FALSE(subject.admit_observer(current));
        EXPECT_EQ(subject.observations().publications, 1U);
        EXPECT_EQ(subject.route(current, app, 43100), Route::use_existing_predicates);
        EXPECT_EQ(subject.route(current, overlay, 43101), Route::use_existing_predicates);
        EXPECT_EQ(app_socket.accesses(), accesses[0]);
        EXPECT_EQ(overlay_socket.accesses(), accesses[1]);
    }
}

TEST_F(StartupCustodyTest, OneValidatedRoleCannotPublish) {
    const BorrowedSocket socket{8, {address, 43100}};
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, socket), Turn::accepted);
    EXPECT_GT(socket.accesses(), 0U);
    EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
    owner.offer(current);
    EXPECT_EQ(owner.publish(current), Turn::pending);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_EQ(owner.observations().publications, 0U);
}

TEST_F(StartupCustodyTest, InvalidLiveEndpointFactsAreTerminal) {
    for (const auto bad_port : {0U, 43101U}) {
        Candidate subject{current, registrations(app), scratch};
        const BorrowedSocket bad{8, {address, static_cast<std::uint16_t>(bad_port)}};
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, app, bad), Turn::terminal);
        EXPECT_EQ(subject.observations().publications, 0U);
    }

    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    const BorrowedSocket duplicate_port{9, {address, 43100}};
    const auto role_turn = owner.socket_options(current, Acceptor::second, overlay, duplicate_port);
    if (role_turn == Turn::accepted) {
        EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
        EXPECT_EQ(owner.listen_return(current, overlay), Turn::accepted);
        owner.offer(current);
        EXPECT_EQ(owner.publish(current), Turn::terminal);
    } else {
        EXPECT_EQ(role_turn, Turn::terminal);
    }
    EXPECT_EQ(owner.observations().publications, 0U);
}

TEST_F(StartupCustodyTest, WrongAddressAndUnverifiedOptionsAreTerminal) {
    const BorrowedSocket wrong_address{8, {"127.0.0.2", 43100}};
    const BorrowedSocket unverified{8, {address, 43100}, false};
    for (const auto *bad : {&wrong_address, &unverified}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, app, *bad), Turn::terminal);
        EXPECT_EQ(subject.observations().publications, 0U);
    }
}

TEST_F(StartupCustodyTest, StopClosesPublicationBeforeAndAfterFrameworkOffer) {
    for (const bool offered : {false, true}) {
        Candidate subject{current, registrations(app), scratch};
        roles(subject);
        if (offered)
            subject.offer(current);
        subject.stop(current);
        if (!offered)
            subject.offer(current);
        EXPECT_EQ(subject.publish(current), Turn::stale);
        EXPECT_FALSE(subject.admit_observer(current));
        EXPECT_EQ(subject.observations().publications, 0U);
    }
}

TEST_F(StartupCustodyTest, ClosedFenceRefusesCurrentSocketTurnBeforeDescriptorAccess) {
    owner.stop(current);
    EXPECT_FALSE(owner.observations().admission_open);
    const BorrowedSocket unused{8, {address, 43100}};
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, unused), Turn::stale);
    EXPECT_EQ(unused.accesses(), 0U);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_EQ(owner.observations().publications, 0U);
}

TEST_F(StartupCustodyTest, StopDropsQueuedQtObserverBeforeRecordRead) {
    ready(owner);
    owner.stop(current);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_EQ(owner.observations().record_reads, 0U);
}

TEST_F(StartupCustodyTest, AdmittedSnapshotSurvivesStopAndDelaysRecordRelease) {
    ready(owner);
    const auto reader = owner.admit_observer(current);
    ASSERT_EQ(reader, expected);
    owner.stop(current);
    owner.acknowledge(current, Party::io);
    owner.acknowledge(current, Party::framework);
    EXPECT_EQ(reader, expected);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_FALSE(owner.observations().record_released);
    owner.release_observer(current);
    EXPECT_EQ(owner.observations().admitted_readers, 0U);
    EXPECT_TRUE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, EachRetirementOrderRequiresBothAcknowledgments) {
    for (const auto first : {Party::framework, Party::io}) {
        Candidate subject{current, registrations(app), scratch};
        ready(subject);
        subject.stop(current);
        subject.acknowledge(current, first);
        subject.acknowledge(current, first);
        const auto partial = subject.observations();
        EXPECT_EQ(partial.framework_retired, first == Party::framework);
        EXPECT_EQ(partial.io_retired, first == Party::io);
        EXPECT_FALSE(partial.record_released);
        subject.acknowledge(current, first == Party::io ? Party::framework : Party::io);
        EXPECT_TRUE(subject.observations().record_released);
    }
}

TEST_F(StartupCustodyTest, ProvenColdCancellationNeedsNoParticipantTurns) {
    owner.cancel_cold(current);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_EQ(owner.observations().publications, 0U);
    EXPECT_TRUE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, AcceptedIoWorkCannotBeTreatedAsCold) {
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    owner.cancel_cold(current);
    EXPECT_FALSE(owner.observations().io_retired);
    EXPECT_FALSE(owner.observations().record_released);
    EXPECT_EQ(owner.observations().io_outstanding, 1U);
}

TEST_F(StartupCustodyTest, StaleColdCancelCannotReleaseFreshCurrentRecord) {
    EXPECT_TRUE(owner.observations().admission_open);
    expect_stale_effect_free(owner, [&] { owner.cancel_cold(former); });
    EXPECT_FALSE(owner.observations().record_released);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    owner.cancel_cold(current);
    EXPECT_TRUE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, StaleSocketCannotClaimTheMissingCurrentRole) {
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
    const BorrowedSocket reused{9, {address, 43101}};
    expect_stale_effect_free(
        owner,
        [&] {
            EXPECT_EQ(owner.socket_options(former, Acceptor::second, overlay, reused), Turn::stale);
        },
        &reused);
    EXPECT_EQ(reused.accesses(), 0U);
    owner.offer(current);
    EXPECT_EQ(owner.publish(current), Turn::pending);
    EXPECT_EQ(owner.socket_options(current, Acceptor::second, overlay, reused), Turn::accepted);
    EXPECT_GT(reused.accesses(), 0U);
    EXPECT_EQ(owner.listen_return(current, overlay), Turn::accepted);
    EXPECT_EQ(owner.publish(current), Turn::accepted);
    EXPECT_EQ(owner.admit_observer(current), expected);
}

TEST_F(StartupCustodyTest, StalePublishCannotConsumeEligibleCurrentHandoff) {
    roles(owner);
    owner.offer(current);
    EXPECT_EQ(owner.observations().io_outstanding, 1U);
    expect_stale_effect_free(owner, [&] { EXPECT_EQ(owner.publish(former), Turn::stale); });
    EXPECT_EQ(owner.observations().publications, 0U);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_EQ(owner.publish(current), Turn::accepted);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_EQ(owner.admit_observer(current), expected);
}

TEST_F(StartupCustodyTest, StaleIdentityCannotTouchReusedSocketOrRecord) {
    ready(owner);
    const auto before = owner.observations();
    const BorrowedSocket reused{8, {address, 43100}};
    expect_stale_effect_free(
        owner,
        [&] { EXPECT_EQ(owner.socket_options(former, Acceptor::first, app, reused), Turn::stale); },
        &reused);
    expect_stale_effect_free(owner,
                             [&] { EXPECT_EQ(owner.listen_return(former, app), Turn::stale); });
    expect_stale_effect_free(owner, [&] { EXPECT_EQ(owner.publish(former), Turn::stale); });
    expect_stale_effect_free(owner, [&] { EXPECT_FALSE(owner.admit_observer(former)); });
    expect_stale_effect_free(
        owner, [&] { EXPECT_EQ(owner.route(former, app, 43100), Route::closing_400); });
    expect_stale_effect_free(owner, [&] { owner.framework_exception(former); });
    expect_stale_effect_free(owner, [&] { owner.acknowledge(former, Party::io); });
    expect_stale_effect_free(owner, [&] { owner.acknowledge(former, Party::framework); });
    const auto after = owner.observations();
    EXPECT_EQ(reused.accesses(), 0U);
    EXPECT_EQ(after.record_reads, before.record_reads);
    EXPECT_FALSE(after.framework_retired);
    EXPECT_FALSE(after.io_retired);
    EXPECT_FALSE(after.unavailable);
    EXPECT_EQ(owner.admit_observer(current), expected);
}

TEST_F(StartupCustodyTest, StaleAcknowledgmentsCannotRetireClosedCurrentRecord) {
    for (const auto missing : {Party::framework, Party::io}) {
        Candidate subject{current, registrations(app), scratch};
        ready(subject);
        subject.stop(current);
        subject.acknowledge(current, missing == Party::io ? Party::framework : Party::io);
        expect_stale_effect_free(subject, [&] { subject.acknowledge(former, missing); });
        const auto partial = subject.observations();
        EXPECT_EQ(partial.framework_retired, missing == Party::io);
        EXPECT_EQ(partial.io_retired, missing == Party::framework);
        EXPECT_FALSE(partial.record_released);
        subject.acknowledge(current, missing);
        EXPECT_TRUE(subject.observations().record_released);
    }
}

TEST_F(StartupCustodyTest, StaleOfferAndLastListenReturnCannotEnablePublication) {
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    EXPECT_EQ(owner.socket_options(current, Acceptor::second, overlay, overlay_socket),
              Turn::accepted);
    EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
    owner.offer(current);
    EXPECT_EQ(owner.observations().io_outstanding, 2U);
    expect_stale_effect_free(owner,
                             [&] { EXPECT_EQ(owner.listen_return(former, overlay), Turn::stale); });
    EXPECT_EQ(owner.publish(current), Turn::pending);
    EXPECT_EQ(owner.observations().io_outstanding, 1U);
    EXPECT_EQ(owner.listen_return(current, overlay), Turn::accepted);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_EQ(owner.publish(current), Turn::accepted);

    Candidate without_offer{current, registrations(app), scratch};
    roles(without_offer);
    expect_stale_effect_free(without_offer, [&] { without_offer.offer(former); });
    EXPECT_EQ(without_offer.publish(current), Turn::pending);
    without_offer.offer(current);
    EXPECT_EQ(without_offer.publish(current), Turn::accepted);
}

TEST_F(StartupCustodyTest, StaleStopAndReleaseCannotRetireCurrentReader) {
    ready(owner);
    expect_stale_effect_free(owner, [&] { owner.stop(former); });
    expect_stale_effect_free(owner, [&] { owner.cancel_cold(former); });
    ASSERT_EQ(owner.admit_observer(current), expected);
    EXPECT_EQ(owner.observations().admitted_readers, 1U);
    owner.stop(current);
    expect_stale_effect_free(owner, [&] { owner.release_observer(former); });
    owner.acknowledge(current, Party::framework);
    owner.acknowledge(current, Party::io);
    EXPECT_FALSE(owner.observations().record_released);
    owner.release_observer(current);
    EXPECT_EQ(owner.observations().admitted_readers, 0U);
    EXPECT_TRUE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, EarlyExceptionNeedsProofOfIoAbsence) {
    for (const bool queued : {false, true}) {
        Candidate subject{current, registrations(app), scratch};
        if (queued)
            subject.offer(current);
        subject.framework_exception(current);
        const auto state = subject.observations();
        EXPECT_TRUE(state.unavailable);
        EXPECT_TRUE(state.framework_retired);
        EXPECT_EQ(state.record_released, !queued);
        EXPECT_FALSE(subject.admit_observer(current));
    }
}

TEST_F(StartupCustodyTest, DrainedIoWorkStillRequiresRetirementAfterEarlyException) {
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    EXPECT_EQ(owner.observations().io_outstanding, 1U);
    EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_EQ(owner.observations().publications, 0U);

    owner.framework_exception(current);
    const auto pending = owner.observations();
    EXPECT_TRUE(pending.unavailable);
    EXPECT_TRUE(pending.framework_retired);
    EXPECT_FALSE(pending.io_retired);
    EXPECT_FALSE(pending.record_released);
    owner.acknowledge(current, Party::io);
    EXPECT_TRUE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, LateExceptionRefusesFirstObserverWithoutReadingRecord) {
    ready(owner);
    EXPECT_TRUE(owner.observations().admission_open);
    EXPECT_EQ(owner.observations().admitted_readers, 0U);
    const auto reads = owner.observations().record_reads;

    owner.framework_exception(current);
    EXPECT_TRUE(owner.observations().unavailable);
    EXPECT_FALSE(owner.observations().admission_open);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_EQ(owner.observations().record_reads, reads);
    EXPECT_EQ(owner.observations().admitted_readers, 0U);
    EXPECT_FALSE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, LateExceptionKeepsAdmittedReaderAndRequiresIoRetirement) {
    ready(owner);
    const auto reader = owner.admit_observer(current);
    ASSERT_EQ(reader, expected);
    owner.framework_exception(current);
    EXPECT_TRUE(owner.observations().unavailable);
    EXPECT_TRUE(owner.observations().framework_retired);
    EXPECT_FALSE(owner.observations().io_retired);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_EQ(reader, expected);
    owner.release_observer(current);
    EXPECT_FALSE(owner.observations().record_released);
    owner.acknowledge(current, Party::io);
    EXPECT_TRUE(owner.observations().record_released);
}

TEST_F(StartupCustodyTest, RoutingUsesClosing400BeforePublicationAndAfterRetirement) {
    EXPECT_EQ(owner.route(current, app, 43100), Route::closing_400);
    ready(owner);
    EXPECT_EQ(owner.route(current, app, 43100), Route::use_existing_predicates);
    EXPECT_EQ(owner.route(current, overlay, 43101), Route::use_existing_predicates);
    owner.stop(current);
    EXPECT_EQ(owner.route(current, app, 43100), Route::closing_400);
    owner.acknowledge(current, Party::framework);
    owner.acknowledge(current, Party::io);
    EXPECT_EQ(owner.route(current, app, 43100), Route::closing_400);
}

TEST_F(StartupCustodyTest, UnprovedOrMismatchedRoleIsTerminalBeforeSocketAccess) {
    for (const auto claim : {std::optional<Role>{}, std::optional<Role>{overlay}}) {
        Candidate subject{current, registrations(app), scratch};
        const BorrowedSocket socket{8, {address, 43100}};
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, claim, socket), Turn::terminal);
        EXPECT_EQ(socket.accesses(), 0U);
        EXPECT_EQ(subject.observations().publications, 0U);
    }
}

TEST_F(StartupCustodyTest, DuplicateOrThirdOptionTurnCannotTouchSocket) {
    for (const bool both_roles : {false, true}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, app, app_socket),
                  Turn::accepted);
        if (both_roles) {
            EXPECT_EQ(subject.socket_options(current, Acceptor::second, overlay, overlay_socket),
                      Turn::accepted);
        }
        const BorrowedSocket replacement{8, {address, 43102}};
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, app, replacement),
                  Turn::terminal);
        EXPECT_EQ(replacement.accesses(), 0U);
    }
}

TEST_F(StartupCustodyTest, BothEphemeralRoleFactsIgnoreCallbackOrdinal) {
    auto registered = registrations(app);
    registered.front().requested.port = 0;
    Candidate subject{current, registered, scratch};
    EXPECT_EQ(subject.socket_options(current, Acceptor::second, overlay, overlay_socket),
              Turn::accepted);
    EXPECT_EQ(subject.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    EXPECT_EQ(subject.listen_return(current, overlay), Turn::accepted);
    EXPECT_EQ(subject.listen_return(current, app), Turn::accepted);
    subject.offer(current);
    EXPECT_EQ(subject.publish(current), Turn::accepted);
    EXPECT_EQ(subject.admit_observer(current), expected);
}

TEST_F(StartupCustodyTest, BothListenReturnsAndFrameworkOfferAreIndependentlyRequired) {
    for (const bool missing_offer : {false, true}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, Acceptor::first, app, app_socket),
                  Turn::accepted);
        EXPECT_EQ(subject.socket_options(current, Acceptor::second, overlay, overlay_socket),
                  Turn::accepted);
        EXPECT_EQ(subject.listen_return(current, app), Turn::accepted);
        if (missing_offer)
            EXPECT_EQ(subject.listen_return(current, overlay), Turn::accepted);
        else
            subject.offer(current);
        EXPECT_EQ(subject.publish(current), Turn::pending);
        EXPECT_EQ(subject.observations().publications, 0U);
        if (missing_offer)
            subject.offer(current);
        else
            EXPECT_EQ(subject.listen_return(current, overlay), Turn::accepted);
        EXPECT_EQ(subject.publish(current), Turn::accepted);
    }
}

TEST_F(StartupCustodyTest, StopBetweenRoleTurnAndListenReturnAccountsForQueuedWork) {
    EXPECT_EQ(owner.socket_options(current, Acceptor::first, app, app_socket), Turn::accepted);
    const auto app_accesses = app_socket.accesses();
    EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
    EXPECT_EQ(owner.socket_options(current, Acceptor::second, overlay, overlay_socket),
              Turn::accepted);
    const auto overlay_accesses = overlay_socket.accesses();
    owner.offer(current);
    EXPECT_EQ(owner.publish(current), Turn::pending);
    EXPECT_EQ(owner.observations().io_outstanding, 1U);
    owner.stop(current);
    owner.acknowledge(current, Party::io);
    EXPECT_FALSE(owner.observations().io_retired);
    EXPECT_EQ(owner.listen_return(current, overlay), Turn::stale);
    EXPECT_EQ(owner.observations().record_reads, 0U);
    EXPECT_EQ(owner.observations().publications, 0U);
    owner.acknowledge(current, Party::io);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_TRUE(owner.observations().io_retired);
    EXPECT_EQ(app_socket.accesses(), app_accesses);
    EXPECT_EQ(overlay_socket.accesses(), overlay_accesses);
}

} // namespace
