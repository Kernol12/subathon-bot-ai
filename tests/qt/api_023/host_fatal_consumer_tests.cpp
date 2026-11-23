// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/api_host_fixture.hpp"
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

struct MissingHostFatalConsumer : std::runtime_error {
    MissingHostFatalConsumer()
        : std::runtime_error("API-023: private Host consumer of published fatal fact is missing") {}
};

// Replace only this stop with a forward through the fake transport's private
// Host binding when that owner exists. The call runs on the fixture IO owner.
void forward_published_fact(Rig &, PublishedFatal) { throw MissingHostFatalConsumer{}; }

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

// The frozen observer fields are additive. The protected headers can build
// the fixture before those fields exist. Only the private forward marks red.
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
            std::exception_ptr missing_consumer;
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
                try {
                    supply(rig, reply_deadline);
                } catch (const MissingHostFatalConsumer &) {
                    // Re-raise on the test stack. The Host owns callback exceptions.
                    missing_consumer = std::current_exception();
                    return;
                }
                failure(rig.submit(*peer, "C"), host::Failure::stopping);
                const auto pending = rig.instance->inspect();
                require(pending.accepted_units == 2 && !pending.writer_barrier &&
                            !pending.owner_cleanup,
                        "Published fatal grants no writer receipt while accepted units are pending");
            };
            success(rig.submit(*peer, "A"), "Admit first command before publication");
            success(rig.submit(*peer, "B"), "Admit waiting command before publication");

            require(QTest::qWaitFor([&] {
                        if (missing_consumer)
                            return true;
                        rig.check_callbacks();
                        return rig.instance->inspect().writer_barrier;
                    }, wait_ms), "Reach the private forward or drain through the real writer");
            if (missing_consumer)
                std::rethrow_exception(missing_consumer);
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
};

QTEST_GUILESS_MAIN(HostFatalConsumerTests)
#include "host_fatal_consumer_tests.moc"
