// SPDX-License-Identifier: GPL-3.0-or-later
// SOURCE ONLY: local oracle controls are not sodium/producer evidence.
#include "native_fixture.hpp"
namespace account_capture_test {
namespace {
TEST(AccountCaptureErasureOracle, RejectsAfterWithoutBefore) {
    ErasureObservation state;
    constexpr std::array<std::byte, 1> zero{};
    EXPECT_FALSE(state.visit({{nullptr, 1, bh::RawKind::receive, bh::RawUse::current},
                              1,
                              eo::Region::receive,
                              eo::Phase::after_sodium},
                             zero));
    EXPECT_FALSE(state.clean());
}
TEST(AccountCaptureErasureOracle, RefusesWrongAttributionEvenWhenBytesAreZero) {
    ErasureObservation state;
    constexpr std::array before{std::byte{0x71}};
    constexpr std::array<std::byte, 1> after{};
    auto event = eo::ErasureEvent{{nullptr, 1, bh::RawKind::receive, bh::RawUse::current},
                                  1,
                                  eo::Region::receive,
                                  eo::Phase::before_sodium};
    ASSERT_TRUE(state.visit(event, before));
    event.phase = eo::Phase::after_sodium;
    event.tag.sequence = 2;
    EXPECT_FALSE(state.visit(event, after));
    EXPECT_EQ(state.calls, 0U);
}
TEST(AccountCaptureErasureOracle, NonzeroAfterIsFailureNotAZeroizationClaim) {
    ErasureObservation state;
    constexpr std::array bytes{std::byte{0x71}};
    auto event = eo::ErasureEvent{{nullptr, 1, bh::RawKind::receive, bh::RawUse::current},
                                  1,
                                  eo::Region::receive,
                                  eo::Phase::before_sodium};
    ASSERT_TRUE(state.visit(event, bytes));
    event.phase = eo::Phase::after_sodium;
    EXPECT_FALSE(state.visit(event, bytes));
    EXPECT_FALSE(state.clean());
}
TEST(AccountCaptureErasureOracle, RecordsAndArenaCannotSubstituteForEachOther) {
    ErasureObservation state;
    constexpr std::array before{std::byte{0x71}};
    constexpr std::array<std::byte, 1> after{};
    auto event = eo::ErasureEvent{{nullptr, 1, bh::RawKind::fields, bh::RawUse::current},
                                  1,
                                  eo::Region::field_records,
                                  eo::Phase::before_sodium};
    ASSERT_TRUE(state.visit(event, before));
    event.phase = eo::Phase::after_sodium;
    event.region = eo::Region::field_arena;
    EXPECT_FALSE(state.visit(event, after));
}
TEST(AccountCaptureErasureOracle, FixedMetadataEqualityThenOverflowFails) {
    ErasureObservation state;
    constexpr std::array before{std::byte{0x71}};
    constexpr std::array<std::byte, 1> after{};
    // Local checker control: not an actual wipe and never a native/secure pass.
    for (unsigned i = 1; i <= 8192; ++i) {
        auto e = eo::ErasureEvent{{nullptr, 0, bh::RawKind::receive, bh::RawUse::pending},
                                  i,
                                  eo::Region::receive,
                                  eo::Phase::before_sodium};
        ASSERT_TRUE(state.visit(e, before));
        e.phase = eo::Phase::after_sodium;
        ASSERT_TRUE(state.visit(e, after));
    }
    EXPECT_TRUE(state.clean());
    EXPECT_EQ(state.calls, 8192U);
    EXPECT_FALSE(state.visit({{nullptr, 0, bh::RawKind::receive, bh::RawUse::pending},
                              8193,
                              eo::Region::receive,
                              eo::Phase::before_sodium},
                             before));
    EXPECT_FALSE(state.clean());
}
TEST(AccountCaptureErasure, RealAccountBodyRequiresAllCurrentOwnersAndRealTeardown) {
    constexpr auto body =
        "{\"supported_versions\":[\"reference.v1\"],\"required_capabilities\":[]}"sv;
    constexpr std::array fields{Field{"Host", "timer.example"},
                                Field{"Content-Type", "application/json"},
                                Field{"Content-Length", "66"}};
    Live f(true, {fields, "POST", "/api/reference/v1/negotiate",
                  std::as_bytes(std::span(body.data(), body.size()))});
    f.connect();
    f.client.send(
        "POST /api/reference/v1/negotiate HTTP/1.1\r\nHost: timer.example\r\nContent-Type: application/json\r\nContent-Length: 66\r\n\r\n"sv);
    f.client.send(body);
    std::array<char, 66> wire{};
    ASSERT_EQ(f.client.read(wire), 66U);
    constexpr auto expected =
        "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: keep-alive\r\n\r\ndiag"sv;
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), wire.begin()));
    SecretScanner scanner;
    scanner.feed({wire.data(), wire.size()});
    EXPECT_TRUE(scanner.clean());
    f.r.wait([&] { return f.r.captures.load() == 1 && f.r.requests.load() == 1; });
    f.cleanup();
    for (auto region : {eo::Region::receive, eo::Region::field_records, eo::Region::field_arena,
                        eo::Region::method, eo::Region::target, eo::Region::body})
        EXPECT_TRUE(f.r.erasures.observed(region, bh::RawUse::current));
    EXPECT_TRUE(f.r.erasures.clean());
}
TEST(AccountCaptureErasure, ActualReadAheadSuffixIsAttributedPendingZeroSequenceOnClose) {
    constexpr auto suffix = "GET /CNY-s11-7q9"sv;
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.r.initial_admit = false;
    f.connect();
    f.r.wait([&] { return f.r.connected.load() == 1; });
    std::array<char, 149 + suffix.size()> input{};
    std::copy(desktop.begin(), desktop.end(), input.begin());
    std::copy(suffix.begin(), suffix.end(), input.begin() + 149);
    f.client.send({input.data(), input.size()});
    f.post(bh::Intent::admit);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    ASSERT_GE(f.r.latch_count, 1U);
    if (f.r.read_latches[0].returned != static_cast<std::int64_t>(input.size())) {
        ADD_FAILURE() << "FIXTURE: pending suffix witness absent; no product credit";
        return;
    }
    f.post(bh::Intent::close);
    std::array<char, 65664> response{};
    auto count = f.client.read(response, true);
    EXPECT_EQ(count, 0U);
    f.cleanup();
    EXPECT_TRUE(f.r.erasures.observed(eo::Region::receive, bh::RawUse::pending));
    EXPECT_TRUE(f.r.erasures.clean()); // oracle requires pending sequence0 both phases
}
TEST(AccountCaptureErasure, UninstalledObservationDoesNotAlterActualCaptureReplyOrCleanup) {
    Live f(true, {fields_desktop}, false, false);
    f.connect();
    f.client.send(desktop);
    std::array<char, 66> wire{};
    ASSERT_EQ(f.client.read(wire), 66U);
    constexpr auto expected =
        "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: keep-alive\r\n\r\ndiag"sv;
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), wire.begin()));
    SecretScanner scanner;
    scanner.feed({wire.data(), wire.size()});
    EXPECT_TRUE(scanner.clean());
    f.r.wait([&] { return f.r.captures.load() == 1; });
    f.cleanup();
    EXPECT_EQ(f.r.erasures.calls, 0U); // source review must also prove disabled path adds no work
}
void actual_observer_failure(ObserverPolicy policy, bool overflow) {
    Live f(true, {fields_desktop}, false, true, policy);
    f.expect_wipe_failure = true;
    f.connect();
    f.client.send(desktop);
    std::array<char, 65664> wire{};
    const auto size = f.client.read(wire, true);
    constexpr auto allowed =
        "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: keep-alive\r\n\r\ndiag"sv;
    ASSERT_LE(size, allowed.size());
    EXPECT_TRUE(std::equal(wire.begin(), wire.begin() + size, allowed.begin()));
    SecretScanner scanner;
    scanner.feed({wire.data(), size});
    EXPECT_TRUE(scanner.clean());
    f.cleanup(); // actual barrier+joins, requires wipe_failed, retains quarantined core
    ASSERT_NE(f.r.foundation, nullptr); // deliberate quarantine remains charged to exit
    EXPECT_GE(f.r.observation.current.peers, 1U);
    EXPECT_GE(f.r.observation.current.metadata_charge, 16384U);
    EXPECT_EQ(f.r.observation.current.sdk_retained_peers, 0U);
    EXPECT_EQ(f.r.observation.current.transient_fds, 0U);
    EXPECT_GT(f.r.erasures.calls, 0U); // actual after-sodium notification, not fake test eraser
    if (overflow) {
        EXPECT_TRUE(f.r.erasures.failed);
        EXPECT_EQ(f.r.erasures.calls, 1U);
    } else {
        EXPECT_TRUE(f.r.erasures.clean());
        EXPECT_TRUE(f.r.erasures.observed(eo::Region::receive, bh::RawUse::current));
    }
}
TEST(AccountCaptureErasure, BeforeRefusalStillWipesThenQuarantinesAndRefusesCleanFinish) {
    actual_observer_failure({true, false, 8192}, false);
}
TEST(AccountCaptureErasure, AfterRefusalQuarantinesAndRefusesCleanFinish) {
    actual_observer_failure({false, true, 8192}, false);
}
TEST(AccountCaptureErasure, CallerObservationCapacityOverflowFailsClosed) {
    actual_observer_failure({false, false, 1}, true);
}
TEST(AccountCaptureLogs, ExistingTrantorCallbackPositiveAndOverflowAreRealFailures) {
    static LogObservation state; // live through global logger teardown
    bind_checked_log(state);
    EXPECT_TRUE(state.clean());
    LOG_TRACE << "CNY-s11-7q9";
    EXPECT_TRUE(state.detected());
    EXPECT_FALSE(state.clean());
    // Separate local stream-capacity control; no pointer/oversized span is read.
    static LogObservation over;
    over.feed(nullptr, 1);
    EXPECT_FALSE(over.clean());
    static LogObservation overflow;
    constexpr std::array<char, 8192> zeros{};
    for (unsigned i = 0; i < 256; ++i)
        overflow.feed(zeros.data(), zeros.size());
    EXPECT_TRUE(overflow.clean());
    overflow.feed("x", 1);
    EXPECT_FALSE(overflow.clean());
}
// Cold observer installation has its own NI reach, independent of account NI.
class ObserverCold : public testing::Test {
    Deadline watchdog;

  protected:
    Rig *r = nullptr;
    void SetUp() override {
        ASSERT_EQ(retained_rig, nullptr);
        r = retained_rig = new Rig;
        bind_checked_log(r->logs);
        auto p = ws::create_profile(
            {1, 1, 64, 64, 16, 2, 128, 256, 256, 64, 256, 254, 8, 50, 50, 100, 50}, r->clock);
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<ws::Profile>>(p));
        r->native = std::move(std::get<std::unique_ptr<ws::Profile>>(p));
        constexpr std::array routes{bh::Route{"GET", "/api/reference/v1/timer", 4096, 4096, 4096}};
        auto f = bh::create({2, 1, 1, 1, 1, 4096, 4, 5000, 5000, 5000, 5000, 10}, routes,
                            {"/foundation/ws", Rig::opened_cb, Rig::message_cb, Rig::closed_cb},
                            r->clock,
                            {r, Rig::connected_cb, Rig::request_cb, Rig::ready_cb, Rig::ended_cb,
                             Rig::removed_cb, Rig::drained_cb, Rig::read_cb});
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<bh::Foundation>>(f));
        r->foundation = std::move(std::get<std::unique_ptr<bh::Foundation>>(f));
    }
    eo::ErasureObserver observer() { return {&r->erasures, ErasureObservation::callback}; }
    void TearDown() override {
        if (r) {
            r->foundation.reset();
            EXPECT_TRUE(r->logs.clean());
        }
    }
};
TEST_F(ObserverCold, InstallIsAnOrdinaryPositiveNotExpectedNI) {
    ASSERT_TRUE(
        std::holds_alternative<ac::Done>(eo::install_erasure_observer(*r->foundation, observer())))
        << "MISSING private observer";
    EXPECT_EQ(r->erasures.calls, 0U);
}
TEST_F(ObserverCold, NullObserverDoesNotConsumeRegistration) {
    auto x = eo::install_erasure_observer(*r->foundation, {nullptr, ErasureObservation::callback});
    ASSERT_TRUE(std::holds_alternative<ac::Error>(x));
    EXPECT_EQ(std::get<ac::Error>(x), ac::Error::invalid_config);
    EXPECT_TRUE(
        std::holds_alternative<ac::Done>(eo::install_erasure_observer(*r->foundation, observer())));
}
TEST_F(ObserverCold, NullCallbackDoesNotConsumeRegistration) {
    auto x = eo::install_erasure_observer(*r->foundation, {&r->erasures, nullptr});
    ASSERT_TRUE(std::holds_alternative<ac::Error>(x));
    EXPECT_EQ(std::get<ac::Error>(x), ac::Error::invalid_config);
    EXPECT_TRUE(
        std::holds_alternative<ac::Done>(eo::install_erasure_observer(*r->foundation, observer())));
}
TEST_F(ObserverCold, RepeatedInstallIsRefused) {
    ASSERT_TRUE(
        std::holds_alternative<ac::Done>(eo::install_erasure_observer(*r->foundation, observer())));
    auto x = eo::install_erasure_observer(*r->foundation, observer());
    ASSERT_TRUE(std::holds_alternative<ac::Error>(x));
    EXPECT_EQ(std::get<ac::Error>(x), ac::Error::invalid_state);
}
TEST_F(ObserverCold, ForeignInstallIsRefusedWithoutCallback) {
    std::thread worker([&] {
        auto x = eo::install_erasure_observer(*r->foundation, observer());
        EXPECT_TRUE(std::holds_alternative<ac::Error>(x));
        if (auto e = std::get_if<ac::Error>(&x))
            EXPECT_EQ(*e, ac::Error::wrong_thread);
    });
    worker.join();
    EXPECT_EQ(r->erasures.calls, 0U);
}
} // namespace
} // namespace account_capture_test
