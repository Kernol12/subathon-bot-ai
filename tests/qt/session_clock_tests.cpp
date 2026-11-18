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
    bool throw_acquire = false;
    bool unavailable = false, init_failure = false;
    std::variant<clock9::Time, SessionSourceUnavailable> initialize() override {
        ++starts;
        if (init_failure)
            return SessionSourceUnavailable{};
        return q;
    }
    std::variant<clock9::Frame, SessionSourceUnavailable> acquire() override {
        ++reads;
        if (throw_acquire)
            throw std::runtime_error("Synthetic source exception");
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
    bool available = true;
    std::function<void()> during;
    SessionClockResolution sample_current() override {
        ++reads;
        if (during)
            during();
        if (!available)
            return SessionSourceUnavailable{};
        return SessionClockSample{utc};
    }
};
// Keep explicit NI visible through the existing real-host callback failure capture.
template <class F> auto binding(F f) {
    try {
        auto handle = f();
        require(bool(handle), "Non-null clock handle");
        return handle;
    } catch (const clock9::NotImplemented &) {
        throw AssertionFailure("UNIMPLEMENTED SESSION-009 clock binding");
    }
}
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
                p.input->next = frame(110, realtime, 112);
                unavailable(p.monitor.sample_current()); // [897,901] or [1097,1101]
                require(p.input->reads == 2, "One acquisition per invocation");
                Policy continuous;
                sample(continuous.monitor, 0);
                continuous.input->next = frame(110, 1110, 112);
                sample(continuous.monitor, 0); // SC02 overlapping K, ordered boot
                require(continuous.input->reads == 2, "Independent healthy continuity control");
            }
        });
    }
    void regression_and_cancellation_SC05_SC06() {
        scenario([] {
            for (bool cancellation : {false, true}) {
                Policy p;
                sample(p.monitor, 0);
                p.input->next = frame(102, cancellation ? 1100 : 1099, 104);
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
            // Process-local floor deliberately disappears at orderly/process restart.
            // This models monitor lifetime, not a supervised crash/reboot experiment.
            {
                Policy previous;
                previous.input->next = frame(100, 7000000000LL, 102);
                sample(previous.monitor, 7000);
            }
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
    void startup_stored_session_SC13_SC14_data() {
        QTest::addColumn<bool>("healthy");
        QTest::newRow("current-kernel") << true;
        QTest::newRow("no-live-status") << false;
    }
    void startup_stored_session_SC13_SC14() {
        QFETCH(bool, healthy);
        scenario([&] {
            Rig r;
            const auto rows = r.store.sessions(), installation = r.store.facts();
            auto input = std::make_unique<Input>();
            auto raw = input.get();
            raw->next = frame(100, 2000000000, 102);
            raw->init_failure = !healthy;
            auto monitor = std::make_shared<clock9::Monitor>(std::move(input));
            clock9::HostClock service(*r.instance, monitor);
            r.start(); // real current catalog + persisted I/E/J, no setup entitlement
            bool resolved = false;
            r.dispatch->body = [&](auto context) -> QVariant {
                auto clock = binding([&] { return service.bind(context); });
                const auto session = context.begin();
                account_sessions_fixture::Cleanup cleanup{session};
                SqliteAccountSessionUnit source(session, {});
                auto result = resolve_current_account_session({"I", {}}, true, source, *clock);
                if (healthy) {
                    require(std::holds_alternative<ResolvedAccountSession>(result),
                            "Old persisted session still valid");
                    const auto &value = std::get<ResolvedAccountSession>(result);
                    require(value.fresh && value.account_ref == "A" && value.session_ref == "S" &&
                                value.authenticated_utc_ms == 1000 &&
                                value.expires_utc_ms == 86401000,
                            "SC13 unchanged session is unexpired/fresh at live2000");
                } else
                    require(std::get<SessionFailure>(result) == SessionFailure::unavailable,
                            "SC14 persisted times cannot manufacture current UTC");
                resolved = true;
                session.commit();
                return 1;
            };
            auto peer = r.peer();
            success(r.submit(peer), "Cold-start host work");
            r.completed();
            require(resolved, "Real resolver executed against actual private store");
            require(r.store.sessions() == rows && r.store.facts() == installation,
                    "Startup preserves epochs/digests/maxima, healthy or unavailable");
            require(raw->starts == 1 && raw->reads == unsigned(healthy),
                    "Only fresh monitor establishes cold UTC");
            r.stop();
        });
    }
    void invalid_frames_and_source_exception() {
        scenario([] {
            for (unsigned variant = 0; variant < 8; ++variant) {
                Policy p;
                if (variant == 0)
                    p.input->next.bs = {-1, 0};
                if (variant == 1)
                    p.input->next.b0 = {0, 1000000000};
                if (variant == 2)
                    p.input->next.realtime = {0, -1};
                if (variant == 3)
                    p.input->next.be = {0, 99};
                if (variant == 4)
                    p.input->next.b1 = {0, 99};
                if (variant == 5)
                    p.input->q = {0, 0};
                if (variant == 6)
                    p.input->q = {0, 1000001};
                if (variant == 7)
                    p.input->throw_acquire = true;
                unavailable(p.monitor.sample_current());
            }
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
            unavailable(retained->sample_current());
            require(source->reads == count + 1, "Stop revokes before underlying reads");
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
                source->during = [&] { unavailable(publication->sample_current()); };
                sample(*publication, 86400999); // remaining1 is downstream emitter oracle
                source->during = {};
                bool foreign_refused = false;
                std::thread foreign([&] {
                    foreign_refused = std::holds_alternative<SessionSourceUnavailable>(
                        publication->sample_current());
                });
                foreign.join();
                require(foreign_refused, "Publication foreign/reentrancy guards before read");
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
    void publication_refusal_SC26_data() {
        QTest::addColumn<bool>("discontinuity");
        QTest::newRow("outage") << false;
        QTest::newRow("rollback") << true;
    }
    void publication_refusal_SC26() {
        QFETCH(bool, discontinuity);
        scenario([&] {
            Rig r;
            r.start();
            const auto original = r.store.sessions();
            auto input = std::make_unique<Input>();
            auto raw = input.get();
            raw->next = frame(100, 1000000000, 102);
            auto monitor = std::make_shared<clock9::Monitor>(std::move(input));
            clock9::HostClock service(*r.instance, monitor);
            r.dispatch->body = [&](auto context) -> QVariant {
                auto clock = binding([&] { return service.bind(context); });
                sample(*clock, 1000);
                return read_installation(context);
            };
            bool refused = false;
            r.dispatch->completion = [&](auto &command, const auto &result) {
                require(result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::none && result.value,
                        "Acknowledged committed truth");
                raw->unavailable = !discontinuity;
                if (discontinuity)
                    raw->next = frame(110, 999999999, 112);
                auto publication = binding([&] { return service.bind_completion(command); });
                unavailable(publication->sample_current());
                refused = true;
                // Test dispatcher deliberately withholds its NONSECRET reply. Actual
                // bearer/metadata suppression is SESSION-002's emitter composition.
                success(command.finish(), "No-reply end after unavailable publication UTC");
            };
            auto peer = r.peer();
            success(r.submit(peer), "Execute then publication");
            r.completed();
            require(refused && raw->reads == 2,
                    "Publication performs new acquisition, no execution cache");
            r.stop();
            require(r.transport->handoffs == 0 && r.store.sessions() == original,
                    "Clock refusal neither fabricates handoff nor revives/changes digest");
        });
    }
    void abnormal_unit_has_no_publication_SC27_data() {
        QTest::addColumn<bool>("committed");
        QTest::newRow("commit-then-throw") << true;
        QTest::newRow("known-rollback") << false;
    }
    void abnormal_unit_has_no_publication_SC27() {
        QFETCH(bool, committed);
        scenario([&] {
            Rig r;
            r.start();
            const auto original = r.store.sessions();
            auto source = std::make_shared<Probe>();
            clock9::HostClock service(*r.instance, source);
            std::shared_ptr<CurrentSessionClock> retained;
            r.dispatch->body = [&](auto context) -> QVariant {
                retained = binding([&] { return service.bind(context); });
                sample(*retained, 1000);
                if (committed) {
                    (void)read_installation(context);
                    throw std::runtime_error("Synthetic after commit");
                }
                auto session = context.begin();
                session.rollback();
                return 1;
            };
            bool checked = false;
            r.dispatch->completion = [&](auto &command, const auto &result) {
                require(result.transaction == (committed ? TransactionOutcome::committed
                                                         : TransactionOutcome::rolled_back),
                        "Actual transaction truth preserved");
                if (committed)
                    require(result.error == WorkError::work_exception && !result.value,
                            "Committed throw cannot become normal completion");
                unavailable(retained->sample_current());
                auto denied = binding([&] { return service.bind_completion(command); });
                unavailable(denied->sample_current());
                require(source->reads == 1, "Abnormal outcome refuses BEFORE publication read");
                checked = true;
                success(command.finish(), "No secret reply");
            };
            auto peer = r.peer();
            success(r.submit(peer), "Abnormal real unit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return checked;
                        },
                        wait_ms),
                    "Matching actual completion");
            r.stop();
            require(r.transport->handoffs == 0 && r.store.sessions() == original,
                    "No handoff/digest revival; commit is never relabeled rollback");
        });
    }
    void completion_throw_revokes_scope_SC27() {
        scenario([] {
            Rig r;
            r.start();
            auto source = std::make_shared<Probe>();
            clock9::HostClock service(*r.instance, source);
            std::shared_ptr<CurrentSessionClock> retained;
            bool entered = false;
            r.dispatch->completion = [&](auto &command, const auto &result) {
                require(result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::none,
                        "Normal body before throwing supplied completion");
                retained = binding([&] { return service.bind_completion(command); });
                entered = true;
                throw std::runtime_error(
                    "Synthetic completion exception before publication sample");
            };
            auto peer = r.peer();
            success(r.submit(peer), "Real work");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return entered && r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Throw unwinds actual callback/command");
            unavailable(retained->sample_current());
            require(source->reads == 0, "After completion throw no publication read or cache");
            r.stop();
            require(r.transport->handoffs == 0, "Throw cannot publish host reply");
        });
    }
    void denied_bindings_have_no_reads() {
        scenario([] {
            Rig r;
            r.start();
            auto source = std::make_shared<Probe>();
            clock9::HostClock service(*r.instance, source);
            auto denied = binding([&] { return service.bind(SqliteWorkContext{}); });
            unavailable(denied->sample_current());
            r.dispatch->execution = [&](auto &command, auto) {
                auto denied_completion = binding([&] { return service.bind_completion(command); });
                unavailable(denied_completion->sample_current());
                success(command.finish(), "No-unit command ends without clock authority");
            };
            auto peer = r.peer();
            success(r.submit(peer), "Execute is not a writer unit/completion");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.dispatch->executions == 1 &&
                                   r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Real no-unit command ends");
            require(source->reads == 0, "No underlying read from default ticket/execute");
            SqliteWriter other;
            require(other.open(r.store.directory.filePath("other.sqlite")) == StorageError::none,
                    "Independent fixture writer, never the authoritative Host writer");
            (void)account_sessions_fixture::run(other, [&](auto context) -> QVariant {
                auto foreign = binding([&] { return service.bind(context); });
                unavailable(foreign->sample_current());
                auto session = context.begin();
                session.commit();
                return 1;
            });
            bool closed = false;
            require(other.shutdown([&] { closed = true; }) == StorageError::none,
                    "Other fixture writer shutdown");
            require(QTest::qWaitFor([&] { return closed; }, wait_ms),
                    "Other fixture writer barrier");
            require(source->reads == 0,
                    "Live other-writer context cannot counterfeit Host membership");
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
