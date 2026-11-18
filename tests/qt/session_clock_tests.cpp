// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/api_host_fixture.hpp"
#include <limits>
#include <subathon/server/session_clock.hpp>

using namespace api_host_fixture;
namespace clock9 = subathon::server::session_clock;
using K = hooks::EventKind;
namespace {
clock9::Time ns(std::int64_t n) { return {n / 1000000000, n % 1000000000}; }
clock9::Frame frame(std::int64_t b, std::int64_t r, std::int64_t end) {
    return {ns(b), ns(b), ns(r), ns(end), ns(end), {}, {}, {}, {}};
}
struct Input final : clock9::Observations {
    clock9::Time q{0, 1};
    clock9::Frame next = frame(100, 1100, 102);
    unsigned starts = 0, reads = 0, closes = 0;
    bool unavailable = false, init_failure = false;
    std::variant<clock9::Time, SessionSourceUnavailable> initialize() override {
        ++starts;
        if (init_failure)
            return SessionSourceUnavailable{};
        return q;
    }
    std::variant<clock9::Frame, SessionSourceUnavailable> acquire() override {
        ++reads;
        if (unavailable)
            return SessionSourceUnavailable{};
        return next;
    }
    void close() noexcept override { ++closes; }
};
struct Policy {
    Input *input;
    clock9::Monitor monitor;
    Policy() : Policy(std::make_unique<Input>()) {}
    explicit Policy(std::unique_ptr<Input> i) : input(i.get()), monitor(std::move(i)) {}
};
void unavailable(SessionClockResolution r) {
    require(std::holds_alternative<SessionSourceUnavailable>(r), "Clock must refuse, never cache");
}
void sample(CurrentSessionClock &c, std::int64_t expected) {
    auto r = c.sample_current();
    require(std::holds_alternative<SessionClockSample>(r), "Complete healthy sample required");
    require(std::get<SessionClockSample>(r).now_utc_ms == expected, "Literal UTC oracle");
}
struct Probe final : CurrentSessionClock {
    std::int64_t utc = 1000;
    unsigned reads = 0;
    std::function<void()> during;
    SessionClockResolution sample_current() override {
        ++reads;
        if (during)
            during();
        return SessionClockSample{utc};
    }
};
} // namespace
class SessionClockTests final : public QObject {
    Q_OBJECT
  private Q_SLOTS:
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void intervals_SC01_SC04() {
        scenario([] {
            for (const auto realtime : {1010, 1210}) {
                Policy p;
                sample(p.monitor, 0); // K=[997,1001]ns
                p.input->next = frame(110, 1110, 112);
                sample(p.monitor, 0); // same K, no invented UTC extrapolation
                p.input->next = frame(110, realtime, 112);
                unavailable(p.monitor.sample_current()); // [897,901] or [1097,1101]
                require(p.input->reads == 3, "One acquisition per invocation");
            }
        });
    }
    void regression_and_cancellation_SC05_SC06() {
        scenario([] {
            for (bool cancellation : {false, true}) {
                Policy p;
                sample(p.monitor, 0);
                p.input->next = frame(100, cancellation ? 1100 : 1099, 102);
                if (cancellation)
                    p.input->next.d0.result = clock9::Guard::cancelled;
                unavailable(p.monitor.sample_current());
            }
        });
    }
    void failures_and_transient_recovery_SC07_SC08_SC11() {
        scenario([] {
            Policy p;
            sample(p.monitor, 0);
            p.input->next.a0.unsync = true;
            unavailable(p.monitor.sample_current());
            p.input->next = frame(110, 1110, 112);
            sample(p.monitor, 0);
            require(p.input->starts == 1, "Transient status failure retains original monitor/K");
            p.input->unavailable = true;
            unavailable(p.monitor.sample_current());
            p.input->unavailable = false;
            sample(p.monitor, 0);
        });
    }
    void duration_SC09_data() {
        QTest::addColumn<qlonglong>("duration");
        QTest::addColumn<bool>("accept");
        QTest::newRow("50ms") << qlonglong(50000000) << true;
        QTest::newRow("50ms-plus-ns") << qlonglong(50000001) << false;
    }
    void duration_SC09() {
        QFETCH(qlonglong, duration);
        QFETCH(bool, accept);
        scenario([&] {
            Policy p;
            p.input->next = frame(0, 1000000000, duration);
            if (accept)
                sample(p.monitor, 1000);
            else
                unavailable(p.monitor.sample_current());
        });
    }
    void guard_SC10_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<bool>("accept");
        QTest::newRow("EAGAIN") << int(clock9::Guard::again) << true;
        QTest::newRow("count3") << int(clock9::Guard::count) << true;
        QTest::newRow("short7") << int(clock9::Guard::short_read) << false;
        QTest::newRow("EINTR") << int(clock9::Guard::interrupted) << false;
    }
    void guard_SC10() {
        QFETCH(int, kind);
        QFETCH(bool, accept);
        scenario([&] {
            Policy p;
            p.input->next.d0 = {clock9::Guard(kind), 3};
            if (accept)
                sample(p.monitor, 0);
            else
                unavailable(p.monitor.sample_current());
        });
    }
    void reestablishment_floor_SC12_SC15() {
        scenario([] {
            Policy p;
            p.input->next = frame(100, 2000000, 102);
            sample(p.monitor, 2);
            p.input->next.d0.result = clock9::Guard::cancelled;
            unavailable(p.monitor.sample_current()); // detecting call NEVER clears hold
            const auto starts = p.input->starts;
            p.input->next = frame(200, 1999999, 202);
            unavailable(p.monitor.sample_current()); // below process floor even with fresh monitor
            require(p.input->starts == starts + 1, "Later invocation attempts fresh establishment");
            p.input->next = frame(210, 2000000, 212);
            sample(p.monitor, 2); // equality permitted; never return floor in place of R
        });
    }
    void restart_SC13_SC14() {
        scenario([] {
            // Each cold monitor is independent; no persisted process anchor is supplied.
            Policy cold;
            cold.input->next = frame(100, 2000000000, 102);
            sample(cold.monitor, 2000);
            Policy unavailable_start;
            unavailable_start.input->init_failure = true;
            unavailable(unavailable_start.monitor.sample_current());
            require(unavailable_start.input->reads == 0,
                    "Failed init never reads a frame/checkpoint");
        });
    }
    void full_domain_supplier_SC22_SC23() {
        scenario([] {
            struct Facts final : CurrentSessionSource {
                CompleteSessionFacts facts{{"I",
                                            "A",
                                            "S",
                                            "E",
                                            "F",
                                            "R",
                                            {},
                                            SessionRecordState::active,
                                            INT64_MIN,
                                            INT64_MIN,
                                            INT64_MAX},
                                           {"I", "A", "F", true},
                                           {"I", "E"},
                                           "snapshot"};
                SessionLookup load_current(const SessionLookupRequest &,
                                           const SessionLimits &) override {
                    return facts;
                }
            } source;
            Probe clock;
            clock.utc = INT64_MAX - 1;
            auto r = resolve_current_account_session({"I", {}}, false, source, clock);
            require(std::holds_alternative<ResolvedAccountSession>(r) &&
                        !std::get<ResolvedAccountSession>(r).fresh,
                    "SC22 age18446744073709551614 unexpired/not fresh");
            clock.utc = INT64_MAX;
            require(std::get<SessionFailure>(resolve_current_account_session(
                        {"I", {}}, false, source, clock)) == SessionFailure::expired,
                    "SC22 exclusive expiry at MAX");
            source.facts.session.lifetime_started_utc_ms = 1000;
            source.facts.session.authenticated_utc_ms = 1000;
            source.facts.session.expires_utc_ms = 86401000;
            clock.utc = 301000;
            require(std::holds_alternative<ResolvedAccountSession>(
                        resolve_current_account_session({"I", {}}, true, source, clock)),
                    "SC23 inclusive300000");
            clock.utc = 301001;
            require(std::get<SessionFailure>(
                        resolve_current_account_session({"I", {}}, true, source, clock)) ==
                        SessionFailure::fresh_authentication_required,
                    "SC23 age300001");
            clock.utc = 999;
            require(std::get<SessionFailure>(resolve_current_account_session(
                        {"I", {}}, false, source, clock)) == SessionFailure::unavailable,
                    "SC23 no unsigned age wrap");
        });
    }
    void signed_conversion_SC19_SC21_data() {
        QTest::addColumn<qlonglong>("sec");
        QTest::addColumn<qlonglong>("nsec");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<qlonglong>("expected");
        QTest::newRow("negative-floor")
            << qlonglong(-1) << qlonglong(999999999) << true << qlonglong(-1);
        QTest::newRow("MIN") << qlonglong(-9223372036854776LL) << qlonglong(192000000) << true
                             << qlonglong(INT64_MIN);
        QTest::newRow("MIN-minus1")
            << qlonglong(-9223372036854776LL) << qlonglong(191000000) << false << qlonglong(0);
        QTest::newRow("MAX") << qlonglong(9223372036854775LL) << qlonglong(807999999) << true
                             << qlonglong(INT64_MAX);
        QTest::newRow("MAX-plus1")
            << qlonglong(9223372036854775LL) << qlonglong(808000000) << false << qlonglong(0);
    }
    void signed_conversion_SC19_SC21() {
        QFETCH(qlonglong, sec);
        QFETCH(qlonglong, nsec);
        QFETCH(bool, accept);
        QFETCH(qlonglong, expected);
        scenario([&] {
            auto r = clock9::utc_milliseconds({sec, nsec});
            if (!accept)
                unavailable(r);
            else {
                require(std::holds_alternative<SessionClockSample>(r),
                        "Representable final integer");
                require(std::get<SessionClockSample>(r).now_utc_ms == expected, "Signed literal");
            }
        });
    }
    void revocation_thread_reentrancy_ex_HL05() {
        scenario([] {
            Rig r;
            r.start();
            auto source = std::make_shared<Probe>();
            clock9::HostClock service(*r.instance, source);
            std::shared_ptr<CurrentSessionClock> retained;
            unsigned bodies = 0;
            r.dispatch->body = [&](auto context) -> QVariant {
                std::shared_ptr<CurrentSessionClock> current;
                try {
                    current = service.bind(context);
                } catch (const clock9::NotImplemented &) {
                    throw AssertionFailure("UNIMPLEMENTED SESSION-009 real-host clock binding");
                }
                require(bool(current), "Actual host-issued clock handle");
                if (!retained)
                    retained = current;
                else
                    unavailable(
                        retained->sample_current()); // different actual unit cannot revive A
                const auto before = source->reads;
                source->during = [&] { unavailable(current->sample_current()); };
                sample(*current, 1000);
                source->during = {};
                require(source->reads == before + 1, "Exactly one underlying read, no reentrancy");
                bool refused = false;
                std::thread foreign([&] {
                    refused =
                        std::holds_alternative<SessionSourceUnavailable>(current->sample_current());
                });
                foreign.join();
                require(refused && source->reads == before + 1, "Live foreign call before read");
                ++bodies;
                return read_installation(context);
            };
            auto peer = r.peer();
            success(r.submit(peer), "A admitted");
            r.completed();
            const auto count = source->reads;
            unavailable(retained->sample_current()); // body ended, including before next work
            success(r.submit(peer, "B"), "B queued");
            unavailable(retained->sample_current());
            require(source->reads == count, "No reads outside A while B queued");
            r.completed(2);
            require(bodies == 2 && source->reads == count + 1, "Both real host units entered");
            r.stop();
            r.instance.reset();
            unavailable(retained->sample_current());
            bool refused = false;
            std::thread foreign([&] {
                refused =
                    std::holds_alternative<SessionSourceUnavailable>(retained->sample_current());
            });
            foreign.join();
            require(refused && source->reads == count + 1,
                    "Safe after host destruction, no cached value");
        });
    }
    void independent_completion_sample_SC25() {
        scenario([] {
            Rig r;
            r.start();
            auto source = std::make_shared<Probe>();
            clock9::HostClock service(*r.instance, source);
            std::shared_ptr<CurrentSessionClock> body_clock, publication;
            r.dispatch->body = [&](auto context) -> QVariant {
                try {
                    body_clock = service.bind(context);
                } catch (const clock9::NotImplemented &) {
                    throw AssertionFailure("UNIMPLEMENTED SESSION-009 execution binding");
                }
                require(bool(body_clock), "Actual body clock");
                sample(*body_clock, 1000);
                return read_installation(context);
            };
            r.dispatch->completion = [&](auto &command, auto result) {
                require(result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::none && result.value,
                        "Publication only after real normal commit");
                unavailable(body_clock->sample_current());
                source->utc = 86400999;
                try {
                    publication = service.bind_completion(command);
                } catch (const clock9::NotImplemented &) {
                    throw AssertionFailure("UNIMPLEMENTED SESSION-009 publication binding");
                }
                require(bool(publication), "Actual completion scope clock");
                sample(*publication, 86400999); // remaining1 is downstream emitter oracle
                require(source->reads == 2, "Independent sample, never execution cache");
                success(command.finish(), "No secret emitted by clock test");
            };
            auto peer = r.peer();
            success(r.submit(peer), "Real unit");
            r.completed();
            unavailable(publication->sample_current());
            require(source->reads == 2, "Completion handle revoked before later work");
            r.stop();
        });
    }
    void utc_deadline_ex_HL15_data() {
        QTest::addColumn<qlonglong>("utc");
        QTest::newRow("UTCmin") << qlonglong(INT64_MIN);
        QTest::newRow("UTCmax") << qlonglong(INT64_MAX);
    }
    void utc_deadline_ex_HL15() {
        QFETCH(qlonglong, utc);
        scenario([&] {
            Rig r;
            r.elapsed->now = 1000000000; // elapsed1000ms, queue5000ms => deadline6000ms
            r.start();
            auto source = std::make_shared<Probe>();
            clock9::HostClock service(*r.instance, source);
            r.dispatch->body = [&](auto c) -> QVariant {
                auto clock = service.bind(c);
                (void)clock->sample_current();
                return read_installation(c);
            };
            auto peer = r.peer();
            success(r.submit(peer), "Actual admission at elapsed1000ms");
            require(r.dispatch->executions == 0, "Queued before UTC jump");
            source->utc = utc;
            r.elapsed->now = 6000000000;
            require(
                QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; }, wait_ms),
                "Expired at original elapsed deadline6000ms");
            require(r.dispatch->executions == 0 && r.log->count(K::unit_entered) == 0 &&
                        r.log->count(K::writer_completed) == 0 && source->reads == 0,
                    "UTC extremes neither extend deadline nor enter body/read UTC");
            r.stop();
        });
    }
};
QTEST_GUILESS_MAIN(SessionClockTests)
#include "session_clock_tests.moc"
