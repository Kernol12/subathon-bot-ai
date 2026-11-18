// SPDX-License-Identifier: GPL-3.0-or-later
// SOURCE ONLY. New exact consumer admission precedes even GoogleTest discovery.
#include "capture_assertions.hpp"
#include "log_observation.hpp"
#include "native_fixture.hpp"
#include <memory>
namespace {
namespace bh = subathon::server::bounded_http;
namespace ws = subathon::server::native_ws;
namespace ac = subathon::server::account_capture;
class Clock final : public ws::ElapsedSource {
    std::variant<ws::ElapsedInstant, ws::ElapsedUnavailable> sample() const noexcept override {
        return ws::ElapsedInstant{1000000000};
    }
};
// Every registered GoogleTest gets its own process: profile install is once/process.
// These existing /f and /foundation/ws literals are cold declarations only;
// no listener, account route, TLS or current-policy integration is installed.
struct ColdRegistration {
    account_capture_test::LogObservation logs;
    std::unique_ptr<ws::Profile> native;
};
static_assert(sizeof(ColdRegistration) <= 192);
ColdRegistration *cold_registration = nullptr; // once/process, SDK global callback owner
class ColdCapture : public testing::Test {
    account_capture_test::Deadline watchdog;

  protected:
    std::shared_ptr<Clock> clock = std::make_shared<Clock>();
    std::unique_ptr<bh::Foundation> foundation;
    unsigned captures = 0;
    void SetUp() override {
        ASSERT_EQ(cold_registration, nullptr);
        cold_registration = new ColdRegistration;
        account_capture_test::bind_checked_log(cold_registration->logs);
        ASSERT_EQ(ws::linked_backend(), ws::Backend::admitted_corrected);
        auto p = ws::create_profile(
            {1, 1, 64, 64, 16, 2, 128, 256, 256, 64, 256, 254, 8, 50, 50, 100, 50}, clock);
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<ws::Profile>>(p))
            << "FIXTURE: native profile";
        cold_registration->native = std::move(std::get<std::unique_ptr<ws::Profile>>(p));
        const std::array routes{bh::Route{"POST", "/f", 4, 0, 0}};
        auto f = bh::create({2, 1, 1, 1, 1, 4, 4, 50, 50, 50, 50, 1}, routes,
                            {"/foundation/ws", [](void *, void *, const ws::Connection &) {},
                             [](void *, void *, const ws::Connection &) {},
                             [](void *, void *, const ws::Connection &) {}},
                            clock,
                            {this, [](void *, bh::Peer) {}, [](void *, void *, bh::Request) {},
                             [](void *, void *) {}, [](void *, void *, bh::End) {},
                             [](void *, void *) {}, [](void *) {}, nullptr});
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<bh::Foundation>>(f))
            << "FIXTURE: real foundation";
        foundation = std::move(std::get<std::unique_ptr<bh::Foundation>>(f));
        ASSERT_NE(foundation, nullptr);
    }
    void TearDown() override {
        foundation.reset(); // actual cold discard; no fictitious network barrier
        if (cold_registration)
            EXPECT_TRUE(cold_registration->logs.clean());
        // SDK profile/logger callback targets intentionally survive until process exit.
    }
    ac::Callbacks callbacks() {
        return {this, [](void *owner, ac::CaptureLease) {
                    ++static_cast<ColdCapture *>(owner)->captures;
                }};
    }
};
TEST_F(ColdCapture, InstallsOnActualColdFoundationWithoutDeliveringARequest) {
    const auto result = ac::install(*foundation, callbacks());
    ASSERT_TRUE(std::holds_alternative<ac::Done>(result)) << "MISSING account capture producer";
    EXPECT_EQ(captures, 0U);
}
TEST_F(ColdCapture, RefusesNullOwnerWithoutConsumingInstallation) {
    auto cb = callbacks();
    cb.owner = nullptr;
    const auto bad = ac::install(*foundation, cb);
    ASSERT_TRUE(std::holds_alternative<ac::Error>(bad));
    EXPECT_EQ(std::get<ac::Error>(bad), ac::Error::invalid_config);
    EXPECT_TRUE(std::holds_alternative<ac::Done>(ac::install(*foundation, callbacks())));
    EXPECT_EQ(captures, 0U);
}
TEST_F(ColdCapture, RefusesNullCallbackWithoutConsumingInstallation) {
    auto cb = callbacks();
    cb.capture = nullptr;
    const auto bad = ac::install(*foundation, cb);
    ASSERT_TRUE(std::holds_alternative<ac::Error>(bad));
    EXPECT_EQ(std::get<ac::Error>(bad), ac::Error::invalid_config);
    EXPECT_TRUE(std::holds_alternative<ac::Done>(ac::install(*foundation, callbacks())));
}
TEST_F(ColdCapture, RefusesSecondInstallation) {
    ASSERT_TRUE(std::holds_alternative<ac::Done>(ac::install(*foundation, callbacks())));
    const auto second = ac::install(*foundation, callbacks());
    ASSERT_TRUE(std::holds_alternative<ac::Error>(second));
    EXPECT_EQ(std::get<ac::Error>(second), ac::Error::invalid_state);
}
TEST_F(ColdCapture, WrongThreadInstallRefusesWithoutConsumingCreatorInstallation) {
    std::optional<ac::Result<ac::Done>> result;
    std::thread worker([&] { result.emplace(ac::install(*foundation, callbacks())); });
    worker.join(); // exclusive cold Foundation access; Deadline bounds a stuck join
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<ac::Error>(*result));
    EXPECT_EQ(std::get<ac::Error>(*result), ac::Error::wrong_thread);
    EXPECT_EQ(captures, 0U);
    ASSERT_TRUE(std::holds_alternative<ac::Done>(ac::install(*foundation, callbacks())));
    EXPECT_EQ(captures, 0U);
}
} // namespace
