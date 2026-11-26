// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/api_host_fixture.hpp"
#include <QAbstractEventDispatcher>
#include <QEventLoop>
#include <chrono>
#include <thread>
#include <type_traits>

namespace {
using namespace api_host_fixture;
using K = hooks::EventKind;

// These two immutable examples are supplied facts from the fixture owner.
// No producer logic, input to Foundation, or elapsed failure lives here.
enum class PublishedFatal { unavailable_at_scan, overflow_at_reply_deadline };
constexpr auto scan = PublishedFatal::unavailable_at_scan;
constexpr auto reply_deadline = PublishedFatal::overflow_at_reply_deadline;
constexpr std::uint64_t ordinary_deadline = 7'000'000'000;

// Typed forward of the supplied fact to the Host's private fatal relay binding,
// the one TransportComposition receives. The call runs on the fixture IO owner.
// Rule for this body: see Forward seam in tests/qt/api_023/README.md.
void forward_published_fact(Rig &rig, PublishedFatal fact) {
    using Reason = bounded_http::FatalElapsedReason;
    using Site = bounded_http::FatalElapsedSite;
    hooks::HostAccess::publish_fatal(
        *rig.instance,
        fact == scan ? bounded_http::FatalElapsed{Reason::sample_unavailable, Site::scan}
                     : bounded_http::FatalElapsed{Reason::deadline_overflow, Site::reply_deadline});
}

void supply(Rig &rig, PublishedFatal fact) {
    rig.transport->on_io([&] { forward_published_fact(rig, fact); });
}

// A borrowing sink with bounded inline capture, no Host calls, and no output.
struct SinkCapture {
    std::array<char, 96> first{};
    std::size_t length = 0;
    std::atomic<unsigned> calls{0};
    std::atomic<bool> captured{false};
    bool accept = true;
    std::thread::id thread;

    static bool write(void *context, std::span<const char> line) noexcept {
        auto &sink = *static_cast<SinkCapture *>(context);
        if (sink.calls.fetch_add(1, std::memory_order_relaxed) == 0) {
            sink.thread = std::this_thread::get_id();
            sink.length = line.size();
            if (line.size() <= sink.first.size())
                std::copy(line.begin(), line.end(), sink.first.begin());
            sink.captured.store(true, std::memory_order_release);
        }
        return sink.accept;
    }

    void expect(std::string_view expected) const {
        require(captured.load(std::memory_order_acquire) && calls == 1 &&
                    thread == std::this_thread::get_id(),
                "Exactly one diagnostic on the Qt owner");
        require(length == expected.size() &&
                    std::equal(expected.begin(), expected.end(), first.begin()),
                "Exact redacted ASCII line, including LF and excluding NUL");
    }
};

// The observer fields are additive Host members. These probes stay passive;
// the fatal selectors demand real Host observation after the forward.
template <class Options> void bind_sink(Options &options, SinkCapture &sink) {
    if constexpr (requires { options.fatal_diagnostic; }) {
        using Sink = typename std::remove_cvref_t<decltype(options.fatal_diagnostic)>::value_type;
        options.fatal_diagnostic = Sink{&sink, &SinkCapture::write};
    }
}

template <class Inspection> void expect_no_fatal(const Inspection &inspection) {
    if constexpr (requires { inspection.fatal_elapsed; })
        require(!inspection.fatal_elapsed, "No recorded Foundation fatal before publication");
}

template <class Inspection> void expect_fatal(const Inspection &inspection, PublishedFatal fact) {
    if constexpr (requires { inspection.fatal_elapsed; }) {
        const auto &record = inspection.fatal_elapsed;
        require(record.has_value(), "Host inspection retains the typed fatal");
        using Reason = decltype(record->reason);
        using Site = decltype(record->site);
        if (fact == scan)
            require(record->reason == Reason::sample_unavailable && record->site == Site::scan,
                    "Inspection retains the supplied unavailable/scan pair");
        else
            require(record->reason == Reason::deadline_overflow &&
                        record->site == Site::reply_deadline,
                    "Inspection retains the supplied overflow/reply_deadline pair");
    } else {
        require(false, "Missing typed Host fatal inspection");
    }
}

// The control's no-fatal checks pass vacuously while a member is absent. Both
// frozen members must exist or neither, so one missing or misspelled member
// cannot silently disable those checks once the other one lands.
template <class Inspection, class Options>
constexpr bool fatal_members_consistent =
    (requires(const Inspection &inspection) { inspection.fatal_elapsed; }) ==
    (requires(const Options &options) { options.fatal_diagnostic; });
static_assert(fatal_members_consistent<host::Inspection, host::Options>);

void await_writer_only(Rig &rig) {
    require(QTest::qWaitFor([&] { return rig.instance->inspect().writer_barrier; }, wait_ms),
            "Actual writer shutdown while fake network receipt is held");
    const auto state = rig.instance->inspect();
    require(state.state == host::State::stopping && !state.storage_active &&
                !state.network_barrier && !state.owner_cleanup && state.stop_fences == 1,
            "One stop fence with storage off and cleanup blocked by network custody");
    require(rig.transport->barrier && !rig.transport->joined() &&
                rig.log->count(K::writer_ack) == 1 && rig.log->count(K::network_ack) == 0 &&
                rig.log->count(K::owner_cleanup) == 0,
            "Writer receipt exists and network and cleanup receipts remain absent");
}

void complete_network(Rig &rig, host::State terminal) {
    rig.transport->acknowledge();
    require(QTest::qWaitFor([&] { return rig.instance->inspect().owner_cleanup; }, wait_ms),
            "Host cleanup after fake network receipt");
    const auto state = rig.instance->inspect();
    require(state.state == terminal && state.stop_fences == 1 && state.writer_barrier &&
                state.network_barrier && rig.transport->joined() && !state.storage_active,
            "Expected terminal disposition after both owner barriers");
    require(state.associations == 0 && state.association_charge == 0 && state.control_charge == 0 &&
                state.accepted_units == 0 && state.queued_diagnostics == 0 &&
                state.input.copies == 0 && state.input.capacity == 0 &&
                state.response.copies == 0 && state.response.capacity == 0,
            "No accepted work, associations, or retained input or output at owner cleanup");
    const auto &log = *rig.log;
    require(log.count(K::stop_consumed) == 1 && log.count(K::network_ack) == 1 &&
                log.count(K::writer_ack) == 1 && log.count(K::owner_cleanup) == 1 && !log.full(),
            "Exactly one observed fence and each receipt");
    require(log.position(K::writer_removed) < log.position(K::writer_ack) &&
                log.position(K::writer_ack) < log.position(K::owner_cleanup) &&
                log.position(K::network_ack) < log.position(K::owner_cleanup),
            "Writer removal and both receipts precede owner cleanup");
    rig.transport->check();
    rig.check_callbacks();
}
// Stage 3 (r4 "Delivery and wake", Correction 3). Test-only HostAccess seams
// inject the post fault or timer refusal; production never sets them.
using hooks::PostFault;
enum Site { stop_site, disconnect_site, lease_site };
enum Bootstrap { stop_latched, arms_once, start_before_bootstrap };
constexpr std::string_view scan_line =
    "api_host.foundation_elapsed_fatal/sample_unavailable/scan\n";

// A nonowner thread latches ordinary stop; its own wake post meets the fault.
void foreign_stop(host::Host &instance) {
    std::thread foreign([&] { instance.request_stop(); });
    foreign.join();
}

// One committed command whose reply lease the fake IO owner keeps in slot 0.
// The short settle lets the Host's own queued turns drain, so the next
// owner-turn post belongs to the stimulus under test.
host::PeerLease retained_reply(Rig &rig) {
    rig.transport->retain_replies = true;
    rig.start();
    auto peer = rig.peer();
    success(rig.submit(peer), "Admit one command");
    rig.completed();
    require(QTest::qWaitFor([&] { return rig.transport->handoffs == 1; }, wait_ms),
            "IO owner retains one reply lease");
    QTest::qWait(50);
    return peer;
}

// Reports the first assertion before Rig teardown, whose own watchdog ends a
// process whose stop never completes and would otherwise hide it.
template <class F> void report_first(F body) {
    try {
        body();
    } catch (const std::exception &error) {
        QTest::qFail(error.what(), __FILE__, __LINE__);
        throw;
    }
}

void expect_cold_stopped(Rig &rig, const SinkCapture &sink) {
    require(QTest::qWaitFor([&] { return rig.instance->inspect().owner_cleanup; }, wait_ms),
            "Owner turn consumes the latched stop without a delivered post");
    const auto s = rig.instance->inspect();
    require(s.state == host::State::stopped && s.stop_fences == 1 && !s.storage_active &&
                rig.log->count(K::stop_consumed) == 1 && rig.log->count(K::owner_cleanup) == 1,
            "One fence and real owner cleanup to stopped");
    require(!rig.transport->startup && rig.log->count(K::inspection_enqueued) == 0 &&
                rig.log->count(K::writer_ack) == 0 && rig.log->count(K::network_ack) == 0,
            "No writer open, transport start or fabricated receipt");
    expect_no_fatal(s);
    require(sink.calls == 0, "No fatal report");
}

// Runs on a thread whose event dispatcher does not exist when the Host is built.
void bootstrap_owner(Bootstrap mode, const std::string &path) {
    require(!QAbstractEventDispatcher::instance(), "Owner thread starts without a dispatcher");
    auto transport = std::make_shared<FakeTransport>();
    auto dispatch = std::make_shared<Dispatcher>();
    auto log = std::make_shared<Log>();
    host::Options options;
    options.store_path = path;
    options.writer_limits = account_sessions_fixture::limits();
    options.writer_limits.pending_batches = 128;
    auto instance = hooks::HostAccess::create(
        std::move(options), std::make_unique<TransportOwner>(transport),
        std::make_unique<DispatcherOwner>(dispatch),
        {std::make_shared<Elapsed>(), [log](const auto &e) { log->add(e); }});
    require(!hooks::HostAccess::owner_timer(*instance).active,
            "No registration without dispatcher");
    if (mode == stop_latched) {
        hooks::HostAccess::inject_post_faults(*instance, PostFault::refuse, 1);
        foreign_stop(*instance);
    }
    QEventLoop loop; // installs this thread's dispatcher; nothing is pumped yet
    require(QAbstractEventDispatcher::instance(), "Owner dispatcher installed");
    const auto pump = [&](auto done) {
        const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(wait_ms);
        while (!done()) {
            if (std::chrono::steady_clock::now() > until)
                return false;
            loop.processEvents(QEventLoop::AllEvents, 10);
        }
        return true;
    };
    const auto cleaned = [&] {
        const auto s = instance->inspect();
        return s.owner_cleanup && s.state == host::State::stopped;
    };
    if (mode == stop_latched) {
        require(pump(cleaned), "Deferred bootstrap consumes the latched stop");
        require(instance->inspect().stop_fences == 1 && log->count(K::stop_consumed) == 1 &&
                    !transport->startup && log->count(K::inspection_enqueued) == 0 &&
                    log->count(K::writer_ack) == 0 && log->count(K::network_ack) == 0,
                "One fence, no startup work and no fabricated receipt");
        require(hooks::HostAccess::owner_timer(*instance).registrations == 0,
                "Stop is consumed first; the timer never registers");
        failure(instance->start([](auto) {}), host::Failure::already_started);
    } else if (mode == arms_once) {
        require(pump([&] { return hooks::HostAccess::owner_timer(*instance).active; }),
                "Deferred bootstrap arms the owner timer");
        instance->request_stop();
        require(pump(cleaned), "Ordinary stop after bootstrap");
        require(pump([&] { return !hooks::HostAccess::owner_timer(*instance).active; }),
                "Owner disarms the timer after cleanup");
    } else {
        success(instance->start([](auto) {}), "Live start reaches prelaunch first");
        require(hooks::HostAccess::owner_timer(*instance).active, "Prelaunch completes setup");
        require(pump([&] { return instance->inspect().storage_active; }), "Actual startup chain");
        instance->request_stop();
        require(pump([&] { return cleaned() && instance->inspect().network_barrier; }),
                "Real barriers and cleanup");
    }
    require(hooks::HostAccess::owner_timer(*instance).registrations ==
                (mode == stop_latched ? 0U : 1U),
            "A late bootstrap never registers twice");
    dispatch->failures.check();
    log->failures.check();
}
} // namespace

class HostFatalConsumerTests : public QObject {
    Q_OBJECT

  private Q_SLOTS:
    void ordinary_stop_control() {
        scenario([] {
            SinkCapture sink;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            rig.elapsed->now = 2'000'000'000;
            rig.transport->hold_network = true;
            rig.start();
            expect_no_fatal(rig.instance->inspect());

            rig.instance->request_stop();
            await_writer_only(rig);
            const auto stopped = rig.instance->inspect();
            require(stopped.drain_deadline == ordinary_deadline &&
                        rig.transport->first_deadline &&
                        rig.transport->first_deadline->nanoseconds == ordinary_deadline,
                    "Ordinary stop keeps the original absolute deadline");
            complete_network(rig, host::State::stopped);
            expect_no_fatal(rig.instance->inspect());
            require(sink.calls == 0, "Ordinary stop emits no fatal diagnostic");
        });
    }

    void fatal_stop_order_data() {
        QTest::addColumn<bool>("ordinary_first");
        QTest::newRow("fatal_first") << false;
        QTest::newRow("ordinary_first_sink_refuses") << true;
    }

    void fatal_stop_order() {
        QFETCH(bool, ordinary_first);
        scenario([&] {
            SinkCapture sink;
            sink.accept = !ordinary_first;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            rig.elapsed->now = 2'000'000'000;
            rig.transport->hold_network = true;
            rig.start();
            if (ordinary_first) {
                rig.instance->request_stop();
                await_writer_only(rig);
                require(rig.instance->inspect().drain_deadline == ordinary_deadline &&
                            rig.transport->first_deadline &&
                            rig.transport->first_deadline->nanoseconds == ordinary_deadline,
                        "Ordinary fence and deadline already consumed before the supplied fact");
            }
            expect_no_fatal(rig.instance->inspect());
            require(sink.calls == 0, "No report before the supplied fact");
            const auto owner_reads = rig.elapsed->owner_reads.load();

            supply(rig, scan);
            require(QTest::qWaitFor([&] { return sink.captured.load(); }, wait_ms),
                    "Owner records and reports fatal without a test stop request");
            await_writer_only(rig);
            const auto stopping = rig.instance->inspect();
            const auto expected_deadline =
                ordinary_first ? std::optional{ordinary_deadline} : std::nullopt;
            require(stopping.drain_deadline == expected_deadline &&
                        rig.transport->first_deadline &&
                        rig.transport->first_deadline->nanoseconds == expected_deadline.value_or(0),
                    "Fatal first passes zero and records no deadline; ordinary first keeps D");
            require(rig.elapsed->owner_reads == owner_reads,
                    "Fatal consumption does not sample elapsed to decide a deadline");
            expect_fatal(stopping, scan);
            sink.expect("api_host.foundation_elapsed_fatal/sample_unavailable/scan\n");

            rig.instance->request_stop();
            complete_network(rig, host::State::failed);
            const auto failed = rig.instance->inspect();
            require(failed.drain_deadline == expected_deadline,
                    "Later stop and cleanup keep the original deadline decision");
            expect_fatal(failed, scan);
            sink.expect("api_host.foundation_elapsed_fatal/sample_unavailable/scan\n");
        });
    }

    void published_before_queued_host_readiness() {
        scenario([] {
            SinkCapture sink;
            unsigned startup_successes = 0;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            rig.transport->delay_ready = true;
            rig.transport->hold_network = true;
            success(rig.instance->start([&](auto result) {
                if (std::holds_alternative<host::StartAccepted>(result))
                    ++startup_successes;
            }), "Host started with fake readiness pending");
            require(rig.transport->startup && rig.instance->inspect().state == host::State::starting &&
                        rig.log->count(K::inspection_enqueued) == 1 &&
                        rig.log->count(K::installation_read_enqueued) == 0,
                    "Transport startup callback installed and catalog inspection already accepted");

            rig.transport->fire_ready();
            rig.transport->check();
            // Same fake owner FIFO: readiness has returned from the Host's startup
            // callback before this handoff. The Qt owner has not pumped either turn.
            supply(rig, scan);
            await_writer_only(rig);
            expect_fatal(rig.instance->inspect(), scan);
            complete_network(rig, host::State::failed);
            require(startup_successes == 0 && rig.log->count(K::listener_ready) == 0 &&
                        rig.log->count(K::installation_read_enqueued) == 0 &&
                        rig.log->count(K::activated) == 0,
                    "Published fatal suppresses Host readiness and further startup work");
            const auto inspection = rig.log->matching(K::inspection_completed);
            require(inspection.size() == 1 && !inspection.front().refusal &&
                        inspection.front().work &&
                        inspection.front().work->transaction == TransactionOutcome::committed &&
                        inspection.front().work->error == WorkError::none &&
                        inspection.front().work->has_value,
                    "Already accepted startup inspection completes with its actual result");
            sink.expect("api_host.foundation_elapsed_fatal/sample_unavailable/scan\n");
        });
    }

    void accepted_units_drain_and_later_command_stays_out() {
        scenario([] {
            SinkCapture sink;
            unsigned completed = 0;
            std::optional<host::PeerLease> peer;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            rig.transport->hold_network = true;
            rig.start();
            peer.emplace(rig.peer());
            rig.dispatch->execution = [&](auto &command, auto view) {
                require(std::ranges::equal(view.correlation, bytes("A")),
                        "Only the first wrapper may own writer work");
                admitted(command.forward(read_installation, [&](const auto &result) {
                    rig.dispatch->failures.call([&] {
                        require(completed++ == 0 &&
                                    result.transaction == TransactionOutcome::committed &&
                                    result.error == WorkError::none && result.value &&
                                    result.value->toInt() == 1,
                                "First supplied completion retains actual committed result");
                    });
                }));
                admitted(command.forward(
                    [](auto context) -> QVariant {
                        const auto session = context.begin();
                        account_sessions_fixture::Cleanup cleanup{session};
                        session.rollback();
                        return 2;
                    },
                    [&, handle = &command](const auto &result) {
                        rig.dispatch->failures.call([&] {
                            require(completed++ == 1 &&
                                        result.transaction == TransactionOutcome::rolled_back &&
                                        result.error == WorkError::not_committed && !result.value,
                                    "Second supplied completion retains actual known noncommit");
                            failure(handle->finish(reply("reply")), host::Failure::stopping);
                        });
                    }));
                require(rig.instance->inspect().accepted_units == 2 && completed == 0,
                        "Both actual writer admissions precede the supplied fact");
                supply(rig, reply_deadline);
                failure(rig.submit(*peer, "C"), host::Failure::stopping);
                const auto pending = rig.instance->inspect();
                require(pending.accepted_units == 2 && !pending.writer_barrier &&
                            !pending.owner_cleanup,
                        "Published fatal grants no writer receipt while accepted units are pending");
            };
            success(rig.submit(*peer, "A"), "Admit first command before publication");
            success(rig.submit(*peer, "B"), "Admit waiting command before publication");

            require(QTest::qWaitFor([&] {
                        rig.check_callbacks();
                        return rig.instance->inspect().writer_barrier;
                    }, wait_ms), "Reach the private forward or drain through the real writer");
            await_writer_only(rig);
            complete_network(rig, host::State::failed);
            expect_fatal(rig.instance->inspect(), reply_deadline);
            sink.expect("api_host.foundation_elapsed_fatal/deadline_overflow/reply_deadline\n");

            const auto cancelled = rig.log->matching(K::command_cancelled);
            const auto admissions = rig.log->matching(K::command_admitted);
            const auto results = rig.log->matching(K::writer_completed);
            const auto command = rig.log->last(K::command_entered).command;
            require(admissions.size() == 2 && cancelled.size() == 1 &&
                        cancelled.front().command != command &&
                        std::ranges::any_of(admissions, [&](const auto &e) {
                            return e.command == cancelled.front().command;
                        }) &&
                        cancelled.front().refusal == host::Failure::stopping &&
                        !cancelled.front().work && rig.dispatch->executions == 1 &&
                        rig.log->count(K::command_entered) == 1 &&
                        rig.log->count(K::unit_entered) == 2 && completed == 2 &&
                        results.size() == 2,
                    "Queued wrapper retires before entry; only accepted units complete");
            require(results[0].command == command && results[1].command == command &&
                        results[0].unit == 1 && results[1].unit == 2 &&
                        results[0].ordinal < results[1].ordinal &&
                        results[1].ordinal < rig.log->last(K::writer_ack).ordinal,
                    "Both real unit completions precede the writer receipt in FIFO order");
            require(rig.transport->handoffs == 0 && rig.log->count(K::local_handoff) == 0,
                    "Fatal suppresses reply publication");
        });
    }
    // Correction 3 (advisor 684.6): a published fatal refuses lease take and
    // copy with stopping and changes nothing; ordinary stop keeps the window.
    void lease_take_under_fatal_data() {
        QTest::addColumn<bool>("fatal");
        QTest::newRow("fatal") << true;
        QTest::newRow("ordinary") << false;
    }
    void lease_take_under_fatal() {
        QFETCH(bool, fatal);
        scenario([&] {
            SinkCapture sink;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            report_first([&] {
                rig.transport->hold_network = true;
                [[maybe_unused]] const auto peer = retained_reply(rig);
                const auto held = rig.instance->inspect();
                require(held.associations == 1 && held.response.copies == 1, "One retained lease");
                if (fatal) {
                    rig.transport->on_io([&] {
                        forward_published_fact(rig, scan);
                        failure(rig.transport->replies[0]->take(), host::Failure::stopping);
                        failure(rig.transport->replies[0]->try_copy(), host::Failure::stopping);
                    });
                    const auto refused = rig.instance->inspect();
                    require(refused.associations == 1 && refused.response.copies == 1 &&
                                refused.response.capacity == held.response.capacity &&
                                rig.transport->takes == 0 && rig.log->count(K::local_handoff) == 0,
                            "Refusal changes no lease, charge or association");
                    await_writer_only(rig);
                    rig.transport->on_io([&] {
                        failure(rig.transport->replies[0]->take(), host::Failure::stopping);
                        rig.transport->replies[0].reset();
                    });
                    require(rig.log->count(K::local_handoff) == 0,
                            "No handoff under fatal closure");
                } else {
                    rig.instance->request_stop();
                    await_writer_only(rig);
                    rig.transport->on_io([&] {
                        success(rig.transport->replies[0]->take(),
                                "Ordinary stop keeps the window");
                        rig.transport->replies[0].reset();
                    });
                    require(rig.log->count(K::local_handoff) == 1,
                            "One handoff after ordinary stop");
                }
                require(rig.instance->inspect().associations == 0,
                        "The normal lease owner releases the association once");
                complete_network(rig, fatal ? host::State::failed : host::State::stopped);
                if (fatal) {
                    expect_fatal(rig.instance->inspect(), scan);
                    sink.expect(scan_line);
                } else {
                    expect_no_fatal(rig.instance->inspect());
                    require(sink.calls == 0, "Ordinary stop emits no fatal diagnostic");
                }
            });
        });
    }

    // r4 "Delivery and wake": a refused or throwing owner-turn post from
    // disconnect, request_stop or the last lease release stays inside the post
    // boundary and keeps its fact; the owner timer consumes stop and fatal.
    void post_failure_is_contained_data() {
        QTest::addColumn<int>("site");
        QTest::addColumn<bool>("fatal");
        QTest::addColumn<bool>("raise");
        QTest::newRow("stop_refuse_fatal") << int(stop_site) << true << false;
        QTest::newRow("stop_raise_fatal") << int(stop_site) << true << true;
        QTest::newRow("stop_refuse_ordinary") << int(stop_site) << false << false;
        QTest::newRow("disconnect_raise_fatal") << int(disconnect_site) << true << true;
        QTest::newRow("disconnect_refuse_ordinary") << int(disconnect_site) << false << false;
        QTest::newRow("lease_raise_fatal") << int(lease_site) << true << true;
        QTest::newRow("lease_refuse_ordinary") << int(lease_site) << false << false;
    }
    void post_failure_is_contained() {
        QFETCH(int, site);
        QFETCH(bool, fatal);
        QFETCH(bool, raise);
        scenario([&] {
            SinkCapture sink;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            report_first([&] {
                rig.transport->hold_network = true;
                const auto peer = retained_reply(rig);
                const auto control = rig.instance->inspect().control_charge;
                rig.transport->on_io([&] {
                    if (fatal)
                        forward_published_fact(rig, scan);
                    hooks::HostAccess::inject_post_faults(
                        *rig.instance, raise ? PostFault::raise : PostFault::refuse, 1);
                    if (site == stop_site)
                        rig.instance->request_stop();
                    else if (site == disconnect_site)
                        rig.transport->ingress->disconnected(peer);
                    else
                        rig.transport->replies[0].reset();
                });
                const auto after = rig.instance->inspect();
                require(site != disconnect_site || after.control_charge + 1024 == control,
                        "Disconnect retires its peer charge before returning");
                require(site != lease_site ||
                            (after.associations == 0 && after.response.copies == 0),
                        "The last lease release retires the association once");
                if (site != stop_site)
                    rig.instance->request_stop();
                if (site != lease_site)
                    rig.transport->on_io([&] { rig.transport->replies[0].reset(); });
                await_writer_only(rig);
                complete_network(rig, fatal ? host::State::failed : host::State::stopped);
                if (fatal) {
                    expect_fatal(rig.instance->inspect(), scan);
                    sink.expect(scan_line);
                } else {
                    expect_no_fatal(rig.instance->inspect());
                    require(sink.calls == 0, "Ordinary wake failure emits no fatal diagnostic");
                }
            });
        });
    }

    // Cold Host with a running dispatcher and no start(): the nonowner's stop
    // post fails, yet the owner drives one fence and real cleanup.
    void cold_stop_without_start_data() {
        QTest::addColumn<bool>("raise");
        QTest::newRow("refuse") << false;
        QTest::newRow("raise") << true;
    }
    void cold_stop_without_start() {
        QFETCH(bool, raise);
        scenario([&] {
            SinkCapture sink;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            report_first([&] {
                hooks::HostAccess::inject_post_faults(
                    *rig.instance, raise ? PostFault::raise : PostFault::refuse, 1);
                foreign_stop(*rig.instance);
                expect_cold_stopped(rig, sink);
                rig.instance->request_stop();
                foreign_stop(*rig.instance);
                QTest::qWait(50);
                require(rig.instance->inspect().stop_fences == 1 &&
                            rig.log->count(K::stop_consumed) == 1,
                        "Duplicate stops and wakes are inert");
                failure(rig.instance->start([](auto) {}), host::Failure::already_started);
            });
        });
    }

    // Stop latched between startup preparation and writer open; the initial
    // post and the early-branch re-wake both fail.
    void stop_before_writer_open_data() {
        QTest::addColumn<bool>("raise");
        QTest::newRow("refuse") << false;
        QTest::newRow("raise") << true;
    }
    void stop_before_writer_open() {
        QFETCH(bool, raise);
        scenario([&] {
            SinkCapture sink;
            unsigned startup_calls = 0;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            report_first([&] {
                const auto latch = [&] {
                    hooks::HostAccess::inject_post_faults(
                        *rig.instance, raise ? PostFault::raise : PostFault::refuse, 2);
                    foreign_stop(*rig.instance);
                };
                success(hooks::HostAccess::before_writer_open(*rig.instance, latch),
                        "Hook after startup preparation");
                success(rig.instance->start([&](auto) { ++startup_calls; }),
                        "Early-stop branch keeps StartAccepted");
                expect_cold_stopped(rig, sink);
                require(startup_calls == 0, "No startup report");
            });
        });
    }

    // Abstract owner-timer registration failure keeps the existing prelaunch
    // refusal and cleanup.
    void timer_registration_failure_refuses_start() {
        scenario([] {
            SinkCapture sink;
            unsigned startup_calls = 0;
            Rig rig("current", [&](auto &options) { bind_sink(options, sink); });
            report_first([&] {
                require(hooks::HostAccess::owner_timer(*rig.instance).active,
                        "Cold setup armed the owner timer");
                success(hooks::HostAccess::refuse_timer_registration(*rig.instance),
                        "Abstract registration failure");
                require(!hooks::HostAccess::owner_timer(*rig.instance).active, "Timer inactive");
                failure(rig.instance->start([&](auto) { ++startup_calls; }),
                        host::Failure::unavailable);
                require(
                    QTest::qWaitFor([&] { return rig.instance->inspect().owner_cleanup; }, wait_ms),
                    "Existing prelaunch cleanup");
                const auto s = rig.instance->inspect();
                require((s.state == host::State::stopped || s.state == host::State::failed) &&
                            !rig.transport->startup &&
                            rig.log->count(K::inspection_enqueued) == 0 && startup_calls == 0,
                        "No framework launch, writer work or startup report");
                expect_no_fatal(s);
                require(sink.calls == 0, "No fatal report");
            });
        });
    }

    // Host built before its thread has a dispatcher: one receiver-owned
    // bootstrap event completes cold setup or consumes a latched stop.
    void deferred_bootstrap_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("stop_latched") << int(stop_latched);
        QTest::newRow("arms_once") << int(arms_once);
        QTest::newRow("start_before_bootstrap") << int(start_before_bootstrap);
    }
    void deferred_bootstrap() {
        QFETCH(int, mode);
        scenario([&] {
            std::optional<Store> store;
            if (mode == start_before_bootstrap)
                store.emplace("current"); // disposable v10 store, built on the test owner
            const auto path = store ? store->path.toStdString() : std::string("unused-cold");
            std::exception_ptr error;
            std::thread owner([&] {
                try {
                    bootstrap_owner(Bootstrap(mode), path);
                } catch (...) {
                    error = std::current_exception();
                }
            });
            owner.join();
            if (error)
                std::rethrow_exception(error);
        });
    }
};

QTEST_GUILESS_MAIN(HostFatalConsumerTests)
#include "host_fatal_consumer_tests.moc"
