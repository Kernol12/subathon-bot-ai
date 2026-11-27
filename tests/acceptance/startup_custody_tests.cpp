// SPDX-License-Identifier: GPL-3.0-or-later
#include "startup_custody_candidate.hpp"

#include <gtest/gtest.h>

namespace {
using namespace subathon::server::startup_custody_candidate;

constexpr Startup current = 41;
constexpr Startup former = 40;
constexpr auto app = Role::application;
constexpr auto overlay = Role::overlay;
constexpr auto address = "127.0.0.1";
constexpr auto scratch = "synthetic-scratch";
const BorrowedSocket app_socket{8, {app, address, 43100}};
const BorrowedSocket overlay_socket{9, {overlay, address, 43101}};
const Snapshot expected{{app_socket.actual, true}, {overlay_socket.actual, true}, scratch};

std::array<Binding, 2> registrations(Role fixed) {
    return fixed == app ? std::array<Binding, 2>{{{app, address, 43100}, {overlay, address, 0}}}
                        : std::array<Binding, 2>{{{overlay, address, 43101}, {app, address, 0}}};
}

class StartupCustodyTest : public testing::Test {
  protected:
    Candidate owner{current, registrations(app), scratch};

    static void roles(Candidate &subject) {
        EXPECT_EQ(subject.socket_options(current, app, app_socket), Turn::accepted);
        EXPECT_EQ(subject.socket_options(current, overlay, overlay_socket), Turn::accepted);
        EXPECT_EQ(subject.listen_return(current, app), Turn::accepted);
        EXPECT_EQ(subject.listen_return(current, overlay), Turn::accepted);
    }

    static void ready(Candidate &subject) {
        roles(subject);
        subject.offer(current);
        EXPECT_EQ(subject.publish(current), Turn::accepted);
    }
};

TEST_F(StartupCustodyTest, MapsFixedAndEphemeralPortsInBothRegistrationOrders) {
    for (const auto fixed : {app, overlay}) {
        Candidate subject{current, registrations(fixed), scratch};
        ready(subject);
        EXPECT_NE(subject.publish(current), Turn::accepted);
        EXPECT_EQ(subject.admit_observer(current), expected);
        EXPECT_FALSE(subject.admit_observer(current));
        EXPECT_EQ(subject.observations().publications, 1U);
    }
}

TEST_F(StartupCustodyTest, OneValidatedRoleCannotPublish) {
    EXPECT_EQ(owner.socket_options(current, app, app_socket), Turn::accepted);
    EXPECT_EQ(owner.listen_return(current, app), Turn::accepted);
    owner.offer(current);
    EXPECT_EQ(owner.publish(current), Turn::pending);
    EXPECT_FALSE(owner.admit_observer(current));
    EXPECT_EQ(owner.observations().publications, 0U);
}

TEST_F(StartupCustodyTest, InvalidLiveEndpointFactsAreTerminal) {
    for (const auto bad_port : {0U, 43101U}) {
        Candidate subject{current, registrations(app), scratch};
        auto bad = app_socket;
        bad.actual.port = static_cast<std::uint16_t>(bad_port);
        EXPECT_EQ(subject.socket_options(current, app, bad), Turn::terminal);
        EXPECT_EQ(subject.observations().publications, 0U);
    }

    EXPECT_EQ(owner.socket_options(current, app, app_socket), Turn::accepted);
    auto duplicate_port = overlay_socket;
    duplicate_port.actual.port = app_socket.actual.port;
    const auto role_turn = owner.socket_options(current, overlay, duplicate_port);
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
    auto wrong_address = app_socket;
    wrong_address.actual.address = "127.0.0.2";
    auto unverified = app_socket;
    unverified.options_verified = false;
    for (const auto &bad : {wrong_address, unverified}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, app, bad), Turn::terminal);
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

TEST_F(StartupCustodyTest, StaleIdentityCannotTouchReusedSocketOrRecord) {
    ready(owner);
    const auto before = owner.observations();
    EXPECT_EQ(owner.socket_options(former, app, app_socket), Turn::stale);
    EXPECT_EQ(owner.listen_return(former, app), Turn::stale);
    EXPECT_EQ(owner.publish(former), Turn::stale);
    EXPECT_FALSE(owner.admit_observer(former));
    owner.acknowledge(former, Party::io);
    owner.acknowledge(former, Party::framework);
    const auto after = owner.observations();
    EXPECT_EQ(after.socket_accesses, before.socket_accesses);
    EXPECT_EQ(after.record_reads, before.record_reads);
    EXPECT_FALSE(after.framework_retired);
    EXPECT_FALSE(after.io_retired);
    EXPECT_EQ(owner.admit_observer(current), expected);
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

TEST_F(StartupCustodyTest, RoutingUsesClosing400BeforePublicationAndAfterStop) {
    EXPECT_EQ(owner.route_status(current), 400);
    ready(owner);
    EXPECT_EQ(owner.route_status(current), 200);
    owner.stop(current);
    EXPECT_EQ(owner.route_status(current), 400);
}

TEST_F(StartupCustodyTest, UnprovedOrMismatchedRoleIsTerminalBeforeSocketAccess) {
    for (const auto claim : {std::optional<Role>{}, std::optional<Role>{overlay}}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, claim, app_socket), Turn::terminal);
        EXPECT_EQ(subject.observations().socket_accesses, 0U);
        EXPECT_EQ(subject.observations().publications, 0U);
    }
}

TEST_F(StartupCustodyTest, DuplicateOrThirdOptionTurnCannotTouchSocket) {
    for (const bool both_roles : {false, true}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, app, app_socket), Turn::accepted);
        if (both_roles) {
            EXPECT_EQ(subject.socket_options(current, overlay, overlay_socket), Turn::accepted);
        }
        const auto before = subject.observations();
        EXPECT_EQ(subject.socket_options(current, app, app_socket), Turn::terminal);
        EXPECT_EQ(subject.observations().socket_accesses, before.socket_accesses);
    }
}

TEST_F(StartupCustodyTest, BothEphemeralRoleFactsIgnoreCallbackOrdinal) {
    Candidate subject{current, {{{app, address, 0}, {overlay, address, 0}}}, scratch};
    EXPECT_EQ(subject.socket_options(current, overlay, overlay_socket), Turn::accepted);
    EXPECT_EQ(subject.socket_options(current, app, app_socket), Turn::accepted);
    EXPECT_EQ(subject.listen_return(current, overlay), Turn::accepted);
    EXPECT_EQ(subject.listen_return(current, app), Turn::accepted);
    subject.offer(current);
    EXPECT_EQ(subject.publish(current), Turn::accepted);
    EXPECT_EQ(subject.admit_observer(current), expected);
}

TEST_F(StartupCustodyTest, BothListenReturnsAndFrameworkOfferAreIndependentlyRequired) {
    for (const bool missing_offer : {false, true}) {
        Candidate subject{current, registrations(app), scratch};
        EXPECT_EQ(subject.socket_options(current, app, app_socket), Turn::accepted);
        EXPECT_EQ(subject.socket_options(current, overlay, overlay_socket), Turn::accepted);
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
    EXPECT_EQ(owner.socket_options(current, app, app_socket), Turn::accepted);
    EXPECT_EQ(owner.observations().io_outstanding, 1U);
    owner.stop(current);
    owner.acknowledge(current, Party::io);
    EXPECT_FALSE(owner.observations().io_retired);
    EXPECT_EQ(owner.listen_return(current, app), Turn::stale);
    EXPECT_EQ(owner.observations().record_reads, 0U);
    EXPECT_EQ(owner.observations().publications, 0U);
    owner.acknowledge(current, Party::io);
    EXPECT_EQ(owner.observations().io_outstanding, 0U);
    EXPECT_TRUE(owner.observations().io_retired);
}

} // namespace
