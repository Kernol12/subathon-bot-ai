// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/api_host_fixture.hpp"
#include <limits>

using namespace api_host_fixture;
using K = hooks::EventKind;
class ApiHostTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void ordinary_current_catalog_activation_HL06() {
        scenario([] {
            Rig r;
            const auto before = r.store.sessions();
            const auto history = r.store.history();
            const auto schema = r.store.catalog();
            require(!history.isEmpty() && history.back()[0].toInt() == 10,
                    "Literal current catalog head10");
            r.start();
            auto p = r.peer();
            success(r.submit(p), "Admit real work");
            r.completed();
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::committed &&
                        w->error == WorkError::none && w->has_value && w->ticket_revoked,
                    "Actual matched completion, ticket already revoked");
            require(r.log->position(K::inspection_completed) <
                            r.log->position(K::installation_read_completed) &&
                        r.log->position(K::installation_read_completed) <
                            r.log->position(K::activated),
                    "Actual startup chain order");
            require(r.store.facts() == SqlRows{{QString("49"), QString("45"), QString("4A")}},
                    "No startup epoch rotation");
            require(r.store.sessions() == before,
                    "All session bindings/digest/state/times unchanged");
            require(r.store.history() == history && r.store.catalog() == schema,
                    "Activation never advances or changes schema");
            require(QTest::qWaitFor([&] { return r.transport->takes == 1; }, wait_ms),
                    "One nonsecret reply after actual activation");
            r.stop();
        });
    }
    void activation_refusal_data() {
        QTest::addColumn<QString>("store");
        for (auto s : {"fresh", "old", "previous_head", "future", "wrong", "malformed_catalog",
                       "missing", "malformed", "wrong_selector"})
            QTest::newRow(s) << QString(s);
    }
    void activation_refusal() {
        QFETCH(QString, store);
        scenario([&] {
            Rig r(store.toStdString());
            account_sessions_fixture::Observer reader(r.store.path);
            const auto schema =
                reader.rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
            const auto history =
                store == "fresh"
                    ? SqlRows{}
                    : reader.rows("SELECT * FROM subathon_migration_history ORDER BY version");
            const auto facts_before =
                (store == "fresh" || store == "old") ? SqlRows{} : r.store.facts();
            const auto sessions_before =
                (store == "fresh" || store == "old") ? SqlRows{} : r.store.sessions();
            r.start(false);
            r.refused_activation();
            auto p = r.peer();
            // Diagnostic success proves listener serving despite refused activation.
            success(r.submit(p, "D", host::RouteId::diagnostic), "Diagnostic admission");
            require(QTest::qWaitFor([&] { return r.transport->takes.load() == 1; }, wait_ms),
                    "Diagnostic before activation");
            require(!r.instance->inspect().storage_active && r.dispatch->diagnostics == 1 &&
                        r.dispatch->executions == 0,
                    "Diagnostics need no source/UTC");
            const auto protected_admission = r.submit(p, "P");
            if (std::holds_alternative<host::Admitted>(protected_admission)) {
                require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                        wait_ms),
                        "Refused work settles");
            } else
                failure(protected_admission, host::Failure::unavailable);
            require(r.dispatch->executions == 0, "Protected entry refused");
            require(reader.rows(
                        "SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name") ==
                        schema,
                    "No DDL/setup/repair");
            if (store != "fresh")
                require(reader.rows("SELECT * FROM subathon_migration_history ORDER BY version") ==
                            history,
                        "No advance/truncate");
            if (store != "fresh" && store != "old") {
                require(r.store.facts() == facts_before && r.store.sessions() == sessions_before,
                        "Refusal never rotates epochs or changes sessions");
            }
            r.stop();
        });
    }
    void global_association_limit_HL01_data() {
        QTest::addColumn<int>("cap");
        QTest::newRow("reduced2") << 2;
        QTest::newRow("reference128") << 128;
    }
    void global_association_limit_HL01() {
        QFETCH(int, cap);
        scenario([&] {
            Rig r("current", [&](auto &o) {
                o.limits.associations = cap;
                o.limits.per_peer = std::min(cap, 8);
            });
            r.start();
            std::vector<host::PeerLease> peers;
            // A owns just one; B and later peers retain all other reservations.
            for (int n = 0; n < 1 + (cap - 1 + 7) / 8; ++n)
                peers.push_back(r.peer());
            auto spare = r.peer(); // no per-peer saturation can explain the global refusal
            for (int n = 0; n < cap; ++n)
                success(r.submit(peers[n == 0 ? 0 : 1 + (n - 1) / 8], std::string(1, char(n + 1))),
                        "Fill global capacity");
            const auto before = r.instance->inspect();
            failure(r.submit(spare, "overflow"), host::Failure::resource_limit);
            const auto after = r.instance->inspect();
            require(after.associations == std::uint64_t(cap) &&
                        after.input.capacity == before.input.capacity &&
                        after.input.copies == before.input.copies && r.dispatch->executions == 0,
                    "Refuse before copy/body");
            r.disconnect(peers[0]);
            r.disconnect(peers[0]);
            const auto released = r.instance->inspect();
            require(released.associations == std::uint64_t(cap - 1) &&
                        released.input.capacity + 4 == before.input.capacity &&
                        released.input.copies + 1 == before.input.copies &&
                        r.dispatch->executions == 0,
                    "Only A released once; B remains outstanding before owner dispatch");
            success(r.submit(spare, "D"), "D eligible with B still outstanding");
            require(r.instance->inspect().associations == std::uint64_t(cap),
                    "D reuses exactly A's slot");
            r.completed(cap);
            r.stop();
        });
    }
    void per_peer_limit_HL02_data() {
        QTest::addColumn<int>("cap");
        QTest::newRow("reduced2") << 2;
        QTest::newRow("reference8") << 8;
    }
    void per_peer_limit_HL02() {
        QFETCH(int, cap);
        scenario([&] {
            Rig r("current", [&](auto &o) {
                o.limits.per_peer = cap;
                o.limits.associations = cap + 2;
            });
            r.start();
            auto p = r.peer();
            auto q = r.peer();
            for (int i = 0; i < cap; ++i)
                success(r.submit(p, std::string(1, char(i + 1))), "P admitted");
            failure(r.submit(p, "overflow"), host::Failure::resource_limit);
            success(r.submit(q, "Q"), "Other peer eligible");
            require(r.instance->inspect().associations == std::uint64_t(cap + 1),
                    "Per-peer does not consume Q");
            r.disconnect(p);
            r.disconnect(p);
            r.completed();
            require(r.dispatch->executions == 1, "Only Q body enters");
            r.stop();
        });
    }
    void elapsed_queue_equality_HL03_HL15_data() {
        QTest::addColumn<qulonglong>("start");
        QTest::addColumn<qulonglong>("entry");
        QTest::addColumn<bool>("eligible");
        QTest::newRow("4999") << qulonglong(0) << qulonglong(4999000000) << true;
        QTest::newRow("5000") << qulonglong(0) << qulonglong(5000000000) << false;
        QTest::newRow("5001") << qulonglong(0) << qulonglong(5001000000) << false;
        QTest::newRow("5999") << qulonglong(1000000000) << qulonglong(5999000000) << true;
        QTest::newRow("6000") << qulonglong(1000000000) << qulonglong(6000000000) << false;
        QTest::newRow("6001") << qulonglong(1000000000) << qulonglong(6001000000) << false;
    }
    void elapsed_queue_equality_HL03_HL15() {
        QFETCH(qulonglong, start);
        QFETCH(qulonglong, entry);
        QFETCH(bool, eligible);
        scenario([&] {
            Rig r;
            r.elapsed->now = start;
            r.start();
            auto p = r.peer();
            success(r.submit(p), "Queued");
            require(r.dispatch->executions == 0, "No domain work during queue wait");
            r.elapsed->now = entry;
            if (eligible)
                r.completed();
            else {
                require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                        wait_ms),
                        "Expired wrapper settles");
                require(r.dispatch->executions == 0 && r.log->count(K::writer_completed) == 0,
                        "Equality no body/BEGIN/sample");
                require(r.log->count(K::writer_completed) == 0 &&
                            r.log->count(K::unit_entered) == 0,
                        "Expired command submits no unit or fabricated writer result");
            }
            r.stop();
        });
    }
    void disconnect_before_entry_HL04() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            success(r.submit(p), "Queued");
            r.disconnect(p);
            require(
                QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; }, wait_ms),
                "Cancelled completion");
            require(r.dispatch->executions == 0 && r.log->count(K::writer_completed) == 0,
                    "Disconnect wins");
            r.stop();
        });
    }
    void disconnect_after_commit_HL04_HL13() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.log->set_observer([&](const auto &e) {
                if (e.kind == K::writer_completed)
                    r.disconnect(p);
            });
            success(r.submit(p), "Queued");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Actual completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::committed &&
                        w->error == WorkError::none,
                    "Disconnect never undoes COMMIT");
            require(r.transport->handoffs == 0 && r.dispatch->executions == 1,
                    "No replay or response revival");
            r.stop();
        });
    }

    void commit_then_throw_HL07() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [](auto context) -> QVariant {
                const auto t = context.begin();
                t.commit();
                throw std::runtime_error("after commit");
            };
            r.dispatch->completion = [](auto &command, const auto &result) {
                require(result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::work_exception && !result.value,
                        "Supplied completion receives committed/work_exception/no-value unchanged");
                success(command.finish(), "No reply after abnormal commit");
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::committed &&
                        w->error == WorkError::work_exception && !w->has_value,
                    "Committed exception unchanged");
            require(r.transport->handoffs == 0, "No normal envelope reconstructed");
            r.stop();
        });
    }

    void lost_ack_and_fail_closed_fifo_HL07_HL14() {
        scenario([] {
            unsigned accepted = 0, completed = 0;
            Rig r;
            r.start();
            auto p = r.peer();
            success(
                hooks::HostAccess::inject_next_control_ack_loss(
                    *r.instance, subathon::server::testing::WorkControlFault::lose_next_commit_ack),
                "Actual writer fault");
            r.dispatch->execution = [&](auto &command, auto) {
                for (unsigned i = 0; i != 3; ++i) {
                    admitted(command.forward(
                        [&](auto context) {
                            require(accepted == 3, "All three admissions precede first body");
                            return read_installation(context);
                        },
                        [&](auto result) {
                            require(accepted == 3, "Execute prequeued three actual admissions");
                            const auto events = r.log->matching(K::writer_completed);
                            require(events.size() == completed + 1,
                                    "Actual observation precedes callback");
                            require(events.back().work && events.back().work->ticket_revoked &&
                                        events.back().work->transaction == result.transaction &&
                                        events.back().work->error == result.error &&
                                        events.back().work->has_value == result.value.has_value(),
                                    "Unchanged actual writer result after ticket revocation");
                            if (++completed == 3)
                                failure(r.dispatch->command->finish(reply("done")),
                                        host::Failure::invalid_completion);
                        }));
                    ++accepted;
                }
            };
            success(r.submit(p, "1"), "One command, three units");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return completed == 3;
                        },
                        wait_ms),
                    "Drain all");
            const auto results = r.log->matching(K::writer_completed);
            require(results[0].work->transaction == TransactionOutcome::outcome_unknown &&
                        results[0].work->error == WorkError::uncertain_outcome,
                    "Lost commit ack");
            for (unsigned i = 1; i != 3; ++i)
                require(results[i].work->transaction == TransactionOutcome::not_started &&
                            results[i].work->error == WorkError::writer_unavailable &&
                            !results[i].work->has_value,
                        "Actual fail-closed FIFO");
            require(r.dispatch->executions == 1 && r.transport->handoffs == 0 &&
                        !r.instance->inspect().storage_active,
                    "No replay/reply or active storage");
            r.stop();
            for (const auto &e : results)
                require(e.ordinal < r.log->last(K::writer_ack).ordinal,
                        "Completion before writer ack");
        });
    }

    void stop_fifo_truth_HL14() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.log->set_observer([&](const auto &e) {
                if (e.kind == K::writer_completed)
                    r.instance->request_stop();
            });
            success(r.submit(p, "1"), "W1");
            success(r.submit(p, "2"), "W2");
            success(r.submit(p, "3"), "W3");
            require(r.log->count(K::stop_consumed) == 0,
                    "All three actual ingress admissions precede stop");
            require(
                QTest::qWaitFor([&] { return r.instance->inspect().state == host::State::stopped; },
                                wait_ms),
                "Shutdown");
            const auto cancelled = r.log->matching(K::command_cancelled);
            const auto admissions = r.log->matching(K::command_admitted);
            require(admissions.size() == 3 && cancelled.size() == 2 &&
                        r.dispatch->executions == 1 && r.log->count(K::writer_completed) == 1 &&
                        r.log->count(K::unit_entered) == 1,
                    "Only W1 enters; waiting commands have no writer item");
            require(r.log->last(K::writer_completed).work->transaction ==
                        TransactionOutcome::committed,
                    "W1 truth");
            const auto stop = r.log->last(K::stop_consumed).ordinal;
            // Each command's owner admission observation precedes its cancellation.
            // Ingress was synchronously accepted for all three before Qt was pumped.
            // Actual W3 ingress admission was acknowledged above before stop;
            // no ordering of its queued Qt observation against A dispatch is assumed.
            for (const auto &e : cancelled)
                require(stop < e.ordinal && !e.work,
                        "Stop before cancellation, no fake SQL result");
            require(r.log->last(K::writer_completed).ordinal < r.log->last(K::writer_ack).ordinal &&
                        stop < r.log->last(K::writer_ack).ordinal,
                    "Drain and stop before writer ack");
            r.stop();
        });
    }

    void busy_begin_is_actual_failure() {
        scenario([] {
            Rig r;
            r.start();
            account_sessions_fixture::Observer lock(r.store.path);
            (void)lock.rows("BEGIN IMMEDIATE");
            auto p = r.peer();
            success(r.submit(p), "Admitted while external lock held");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Busy completion while lock stays held");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::not_started &&
                        w->error == WorkError::busy && !w->has_value,
                    "BEGIN throws SqliteWorkFailure(busy), not generic exception");
            (void)lock.rows("ROLLBACK");
            require(r.dispatch->executions == 1 && r.transport->handoffs == 0,
                    "No implicit replay");
            r.stop();
        });
    }
    void diagnostic_still_queued_at_stop_HL17_HL12() {
        scenario([] {
            Rig r("future");
            r.start(false);
            r.refused_activation();
            auto p = r.peer();
            success(r.submit(p, "D", host::RouteId::diagnostic), "Diagnostic queued");
            require(r.instance->inspect().queued_diagnostics == 1 && r.dispatch->diagnostics == 0,
                    "Observe actual queued dependency before ANY event pumping");
            r.instance->request_stop();
            r.stop();
            require(r.dispatch->diagnostics == 0 && r.transport->handoffs == 0 &&
                        r.log->count(K::diagnostic_cancelled) == 1,
                    "Stop cancels data-only callback");
            require(r.log->count(K::unit_entered) == 0 && r.log->count(K::writer_completed) == 0,
                    "Diagnostic invents no writer result");
        });
    }
    void credentials_and_accounts_refuse_absent_sources_HL17() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            for (auto route : {host::RouteId::credential, host::RouteId::account}) {
                auto result = r.submit(p, route == host::RouteId::credential ? "C" : "A", route);
                if (std::holds_alternative<host::Admitted>(result))
                    require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                            wait_ms),
                            "Unavailable source settles");
                else
                    failure(result, host::Failure::unavailable);
            }
            require(r.dispatch->executions == 0 && r.transport->handoffs == 0,
                    "No invented account/UTC/authentication");
            r.stop();
        });
    }
    void input_capacity_metadata_and_move() {
        scenario([] {
            Rig r("current", [](auto &o) {
                o.limits.envelope_bytes = 4;
                o.limits.input_capacity = 8;
                o.limits.input_slots = 2;
            });
            r.start();
            auto p = r.peer();
            auto q = r.peer();
            success(r.submit(p, "A"), "4 fits");
            success(r.submit(q, "B"), "Second4 fits");
            auto s = r.instance->inspect();
            require(s.input.capacity == 8 && s.input.copies == 2 && s.input.metadata == 256 &&
                        s.input.peak_charge == 264,
                    "Capacity+128 per copy");
            failure(r.submit(p, "C", host::RouteId::installation, "x"),
                    host::Failure::resource_limit);
            failure(r.submit(p, "E", host::RouteId::installation, "12345"),
                    host::Failure::resource_limit);
            require(r.instance->inspect().input.peak_charge == 264, "No refusal copy");
            r.disconnect(p);
            s = r.instance->inspect();
            require(s.input.capacity == 4 && s.input.copies == 1 && s.input.metadata == 128,
                    "Release A once leaves charge132");
            success(r.submit(q, "D"), "Another4 eligible after release");
            r.stop();
        });
    }
    void copy_slots_exhaust_even_with_payload_capacity() {
        scenario([] {
            Rig r("current", [](auto &o) {
                o.limits.envelope_bytes = 4;
                o.limits.input_capacity = 12;
                o.limits.input_slots = 2;
            });
            r.start();
            auto p = r.peer();
            success(r.submit(p, "A"), "First4");
            success(r.submit(p, "B"), "Second4");
            failure(r.submit(p, "C", host::RouteId::installation, "x"),
                    host::Failure::resource_limit);
            require(r.instance->inspect().input.peak_charge == 264,
                    "Two slots independent of remaining bytes");
            r.stop();
        });
    }
    void response_copy_move_release_and_expiry() {
        scenario([] {
            Rig r("current", [](auto &o) {
                o.limits.response_bytes = 4;
                o.limits.response_capacity = 8;
                o.limits.response_slots = 2;
                o.limits.handoff_ms = 5;
            });
            r.transport->retain_replies = true;
            r.start();
            auto p = r.peer();
            success(r.submit(p), "Admit");
            r.completed();
            require(QTest::qWaitFor([&] { return r.transport->handoffs == 1; }, wait_ms),
                    "IO lease retained");
            r.transport->on_io([&] {
                auto extra = success(r.transport->replies[0]->try_copy(), "Account another copy4");
                auto moved = std::move(extra); // transfer charge, no duplicate ownership
                failure(r.transport->replies[0]->try_copy(), host::Failure::resource_limit);
                moved.reset();
                moved.reset();
            });
            auto s = r.instance->inspect();
            require(s.response.capacity == 4 && s.response.copies == 1 &&
                        s.response.metadata == 128 && s.response.peak_charge == 264,
                    "Release once to132; move adds0");
            r.elapsed->now = 5000000;
            r.transport->on_io([&] {
                failure(r.transport->replies[0]->take(), host::Failure::expired);
                r.transport->replies[0].reset();
            });
            require(r.log->count(K::local_handoff) == 0,
                    "No expired lease consumed by delayed IO continuation");
            r.stop();
        });
    }
    void retained_response_disconnect_denies_late_io_take_HL04_HL13() {
        scenario([] {
            Rig r;
            r.transport->retain_replies = true;
            r.start();
            auto p = r.peer();
            success(r.submit(p), "Admit");
            r.completed();
            require(QTest::qWaitFor([&] { return r.transport->handoffs == 1; }, wait_ms),
                    "Actual response lease retained before disconnect");
            require(r.instance->inspect().response.copies == 1,
                    "Lease still outstanding, not previously reset");
            r.disconnect(p);
            r.transport->on_io([&] {
                failure(r.transport->replies[0]->take(), host::Failure::disconnected);
                r.transport->replies[0]->reset();
                r.transport->replies[0]->reset();
                r.transport->replies[0].reset();
            });
            const auto s = r.instance->inspect();
            require(r.log->count(K::local_handoff) == 0 && r.transport->takes == 0 &&
                        s.response.capacity == 0 && s.response.copies == 0 &&
                        s.response.metadata == 0 && s.associations == 0,
                    "Late IO inert; disconnect/reset releases reservations once to zero");
            const auto work = r.log->last(K::writer_completed).work;
            require(work && work->transaction == TransactionOutcome::committed &&
                        r.log->count(K::writer_completed) == 1,
                    "Disconnect preserves actual commit without replay");
            r.stop();
        });
    }
    void response_once_only_and_correlation_HL13() {
        scenario([] {
            Rig r;
            r.transport->retain_replies = true;
            r.start();
            auto p = r.peer();
            success(r.submit(p, "A"), "A");
            r.completed();
            require(QTest::qWaitFor([&] { return r.transport->handoffs == 1; }, wait_ms),
                    "Handoff once");
            r.transport->on_io([&] {
                auto copy = success(r.transport->replies[0]->try_copy(), "Retained response copy");
                const auto v = success(r.transport->replies[0]->take(), "First local handoff");
                require(v.correlation.size() == 1 && v.correlation[0] == std::byte{'A'},
                        "Original correlation");
                failure(copy.take(), host::Failure::invalid_completion);
                copy.reset();
                failure(r.transport->replies[0]->take(), host::Failure::invalid_completion);
                r.transport->replies[0].reset();
            });
            r.disconnect(p);
            require(r.dispatch->executions == 1 && r.log->count(K::writer_completed) == 1,
                    "No delivery claim/replay");
            r.stop();
        });
    }
    void invalid_configuration_data() {
        QTest::addColumn<int>("which");
        for (int n = 0; n < 5; ++n)
            QTest::newRow(qPrintable(QString::number(n))) << n;
    }
    void invalid_configuration() {
        QFETCH(int, which);
        scenario([&] {
            Rig r("current", [&](auto &o) {
                if (which == 0)
                    o.limits.queue_ms = 0;
                if (which == 1)
                    o.limits.per_peer = 129;
                if (which == 2)
                    o.limits.envelope_bytes = 8388609;
                if (which == 3)
                    o.limits.drain_ms = UINT64_MAX;
                if (which == 4)
                    o.limits.response_slots = 0;
            });
            failure(r.instance->start([](auto) {}), host::Failure::invalid_options);
            require(r.transport->ingress == nullptr, "No framework activation");
        });
    }

    void reference_bookkeeping_control_and_correlation_budget() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            require(r.instance->inspect().control_charge == 1024,
                    "One preallocated peer control reserve");
            success(r.submit(p, std::string(256, 'x')), "Exact256 correlation");
            require(r.instance->inspect().association_charge == 512,
                    "Inline256 correlation+256 metadata");
            const auto before = r.instance->inspect().input.capacity;
            failure(r.submit(p, std::string(257, 'x')), host::Failure::resource_limit);
            require(r.instance->inspect().input.capacity == before &&
                        r.instance->inspect().associations == 1,
                    "257 refuses before copy");
            r.completed();
            r.stop();
            require(r.instance->inspect().control_charge == 0 &&
                        r.instance->inspect().association_charge == 0,
                    "Control/association slots released once");
        });
    }
    void elapsed_failure_data() {
        QTest::addColumn<int>("which");
        QTest::newRow("addition_overflow") << 0;
        QTest::newRow("unavailable") << 1;
        QTest::newRow("regression100to99") << 2;
    }
    void elapsed_failure() {
        QFETCH(int, which);
        scenario([&] {
            Rig r;
            r.elapsed->now = 100;
            r.start();
            auto warm = r.peer();
            success(r.submit(warm, "warm", host::RouteId::diagnostic), "Same IO sample100");
            r.disconnect(warm);
            auto p = r.peer();
            if (which == 0)
                r.elapsed->now = UINT64_MAX;
            if (which == 1)
                r.elapsed->available = false;
            if (which == 2)
                r.elapsed->now = 99;
            failure(r.submit(p), host::Failure::elapsed_unavailable);
            require(r.dispatch->executions == 0 && r.log->count(K::writer_completed) == 0,
                    "No wrapped deadline or invented SQL");
            r.elapsed->available = true;
            r.elapsed->now = 100;
            r.stop();
        });
    }
    void pure_production_converter_data() {
        QTest::addColumn<qlonglong>("sec");
        QTest::addColumn<qlonglong>("nsec");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<qulonglong>("expected");
        QTest::newRow("zero") << qlonglong(0) << qlonglong(0) << true << qulonglong(0);
        QTest::newRow("one") << qlonglong(1) << qlonglong(1) << true << qulonglong(1000000001);
        QTest::newRow("MAX") << qlonglong(18446744073LL) << qlonglong(709551615) << true
                             << qulonglong(UINT64_MAX);
        QTest::newRow("overflow1")
            << qlonglong(18446744073LL) << qlonglong(709551616) << false << qulonglong(0);
        QTest::newRow("negative_sec") << qlonglong(-1) << qlonglong(0) << false << qulonglong(0);
        QTest::newRow("negative_nsec") << qlonglong(0) << qlonglong(-1) << false << qulonglong(0);
        QTest::newRow("nonnormalized")
            << qlonglong(0) << qlonglong(1000000000) << false << qulonglong(0);
    }
    void pure_production_converter() {
        QFETCH(qlonglong, sec);
        QFETCH(qlonglong, nsec);
        QFETCH(bool, valid);
        QFETCH(qulonglong, expected);
        scenario([&] {
            auto r = host::convert_monotonic_observation(sec, nsec);
            if (valid)
                require(success(std::move(r), "Production converter unimplemented") == expected,
                        "Independent exact literal ns");
            else
                failure(r, host::Failure::elapsed_unavailable);
        });
    }
    void shared_elapsed_identity_HL15() {
        scenario([] {
            Rig r;
            r.start();
            const auto id = hooks::HostAccess::elapsed_identity(*r.instance);
            require(id.host == r.elapsed.get() && id.profile == r.elapsed.get(),
                    "ONE shared raw source host and native profile");
            // FakeTransport retains but never calls the real profile. These
            // attributable owner/IO reads prove host gates only; N tests expiry.
            const auto owner_reads = r.elapsed->owner_reads.load();
            const auto io_reads = r.elapsed->foreign_reads.load();
            auto p = r.peer();
            success(r.submit(p), "Admit");
            r.completed();
            require(r.transport->takes == 1 && r.elapsed->owner_reads > owner_reads &&
                        r.elapsed->foreign_reads > io_reads,
                    "Actual Qt host entry and IO host lease reads wired");
            r.stop();
        });
    }
    void fixed_stop_deadline_and_actual_barriers_HL10_HL12() {
        scenario([] {
            Rig r;
            r.elapsed->now = 1000000000;
            r.transport->hold_network = true;
            r.start();
            r.instance->request_stop();
            require(QTest::qWaitFor([&] { return r.log->count(K::stop_consumed) == 1; }, wait_ms),
                    "First consumed stop");
            require(r.instance->inspect().drain_deadline == 6000000000ULL,
                    "Literal original drain6000ms");
            for (auto now : {1999000000ULL, 5999000000ULL}) {
                r.elapsed->now = now;
                r.instance->request_stop();
                QCoreApplication::processEvents();
                require(r.instance->inspect().drain_deadline == 6000000000ULL,
                        "Repeated stop never resets");
            }
            require(r.instance->inspect().state != host::State::stopped &&
                        !r.instance->inspect().network_barrier,
                    "Deadline alone no ack");
            r.transport->acknowledge();
            r.stop();
            for (auto k :
                 {K::callbacks_revoked, K::sockets_removed, K::writer_removed, K::writer_ack,
                  K::network_ack, K::io_joined, K::framework_joined, K::owner_cleanup})
                require(r.log->position(k) < r.log->position(K::stopped),
                        "Real owner barriers before stopped");
            failure(r.instance->start({}), host::Failure::already_started);
        });
    }
    void startup_stop_data() {
        QTest::addColumn<int>("phase");
        for (int n = 0; n < 4; ++n)
            QTest::newRow(qPrintable(QString::number(n))) << n;
    }
    void startup_stop() {
        QFETCH(int, phase);
        scenario([&] {
            Rig r;
            r.transport->delay_ready = true;
            r.transport->hold_network = true;
            if (phase == 0)
                r.instance->request_stop();
            if (phase == 1)
                r.log->set_observer([&](const auto &e) {
                    if (e.kind == K::inspection_enqueued)
                        r.instance->request_stop();
                });
            if (phase == 2)
                r.log->set_observer([&](const auto &e) {
                    if (e.kind == K::installation_read_enqueued)
                        r.instance->request_stop();
                });
            auto result = r.instance->start([&](auto) {
                if (phase == 3)
                    throw std::runtime_error("startup observer");
            });
            require(!std::holds_alternative<host::NotImplemented>(result),
                    "UNIMPLEMENTED production host start");
            if (phase == 3) {
                require(QTest::qWaitFor([&] { return bool(r.transport->startup); }, wait_ms),
                        "Actual transport callback installed");
                r.transport->fire_ready();
            }
            if (phase == 0 || phase == 1 || phase == 2) {
                require(
                    QTest::qWaitFor([&] { return r.log->count(K::stop_consumed) == 1; }, wait_ms),
                    "Startup stop consumed");
                if (r.transport->startup)
                    r.transport->fire_ready(); // deliberately late actual transport event
                if (r.transport->ingress)
                    r.transport->on_io([&] {
                        failure(r.transport->ingress->connected(), host::Failure::stopping);
                    });
            }
            require(QTest::qWaitFor(
                        [&] {
                            const auto s = r.instance->inspect();
                            return bool(r.transport->barrier) || s.network_barrier;
                        },
                        wait_ms),
                    "Actual network cleanup requested");
            if (r.transport->barrier && !r.transport->joined())
                r.transport->acknowledge();
            r.stop(phase == 3);
            const auto fences = r.log->count(K::stop_consumed);
            require(r.dispatch->executions == 0 && r.log->count(K::owner_cleanup) == 1 &&
                        ((phase == 3 && r.instance->inspect().state == host::State::failed)
                             ? fences <= 1
                             : fences == 1),
                    "No transient admission, one actual cleanup; failed startup need not serve");
            failure(r.instance->start({}), host::Failure::already_started);
        });
    }
    void late_ready_after_stop_cannot_admit() {
        scenario([] {
            Rig r;
            r.transport->delay_ready = true;
            r.transport->hold_network = true;
            success(r.instance->start([](auto) {}), "Start accepted, readiness pending");
            require(QTest::qWaitFor([&] { return bool(r.transport->startup); }, wait_ms),
                    "Actual transport readiness callback installed");
            r.instance->request_stop();
            require(QTest::qWaitFor([&] { return r.log->count(K::stop_consumed) == 1; }, wait_ms),
                    "Stop before readiness");
            r.transport->fire_ready();
            r.transport->on_io(
                [&] { failure(r.transport->ingress->connected(), host::Failure::stopping); });
            require(r.instance->inspect().state != host::State::serving &&
                        r.dispatch->executions == 0,
                    "Late ready creates no serving admission");
            require(QTest::qWaitFor([&] { return bool(r.transport->barrier); }, wait_ms),
                    "Actual cleanup requested");
            r.transport->acknowledge();
            r.stop();
        });
    }

    void known_rollback_preserves_source() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [](auto context) -> QVariant {
                const auto t = context.begin();
                account_sessions_fixture::Cleanup cleanup{t};
                SqliteAccountSessionUnit unit(t, {});
                const auto f = unit.read_installation("I");
                require(unit.replace_installation_epoch(std::get<StoredInstallationFacts>(f), "F",
                                                        "N") == LifecycleWriteResult::staged,
                        "Stage real mutation");
                t.rollback();
                return 1;
            };
            success(r.submit(p), "Admit rollback");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Actual rollback completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::rolled_back &&
                        w->error == WorkError::not_committed && !w->has_value,
                    "Known rollback truth");
            require(r.store.facts() == SqlRows{{QString("49"), QString("45"), QString("4A")}},
                    "Previous committed source retained");
            require(r.transport->handoffs == 0, "No rollback publication");
            r.stop();
        });
    }

    void startup_lost_commit_ack_stops_activation_chain() {
        scenario([] {
            Rig r;
            const auto facts = r.store.facts();
            const auto sessions = r.store.sessions();
            success(
                hooks::HostAccess::inject_next_control_ack_loss(
                    *r.instance, subathon::server::testing::WorkControlFault::lose_next_commit_ack),
                "Cold arm forwarded once to actual idle writer before inspection");
            r.start(false);
            r.refused_activation();
            const auto inspection = r.log->last(K::inspection_completed);
            require(inspection.work &&
                        inspection.work->transaction == TransactionOutcome::outcome_unknown &&
                        inspection.work->error == WorkError::uncertain_outcome &&
                        !inspection.work->has_value && inspection.work->ticket_revoked,
                    "Actual STARTUP inspection COMMIT acknowledgment lost, no forged result");
            require(
                !r.instance->inspect().storage_active && r.log->count(K::activated) == 0 &&
                    r.log->count(K::inspection_enqueued) == 1 &&
                    r.log->count(K::installation_read_enqueued) == 0 &&
                    r.log->count(K::installation_read_completed) == 0,
                "Unknown first step stops chain, no replay or downstream read on failed writer");
            auto p = r.peer();
            const auto result = r.submit(p, "P");
            if (std::holds_alternative<host::Admitted>(result))
                require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                        wait_ms),
                        "Protected refusal settles");
            else
                failure(result, host::Failure::unavailable);
            success(r.submit(p, "D", host::RouteId::diagnostic), "Diagnostic remains possible");
            require(QTest::qWaitFor([&] { return r.transport->takes == 1; }, wait_ms),
                    "Diagnostic consumed despite startup uncertainty");
            r.check_callbacks();
            require(
                r.dispatch->diagnostics == 1 && r.dispatch->executions == 0 &&
                    r.dispatch->completions == 0 && r.log->count(K::unit_entered) == 0 &&
                    r.log->count(K::writer_completed) == 0 &&
                    r.log->count(K::inspection_completed) == 1 &&
                    !r.instance->inspect().storage_active && r.store.facts() == facts &&
                    r.store.sessions() == sessions,
                "No protected publication/sample or invented command result; SQL facts unchanged");
            r.stop();
        });
    }
    void startup_inspection_budget_refusal_still_serves_diagnostics() {
        scenario([] {
            Rig r("current", [](auto &o) { o.writer_limits.statements_per_batch = 1; });
            r.start(false);
            r.refused_activation();
            auto p = r.peer();
            success(r.submit(p, "D", host::RouteId::diagnostic),
                    "Diagnostic independent of SQL inspection failure");
            require(QTest::qWaitFor([&] { return r.transport->takes == 1; }, wait_ms),
                    "Diagnostic handoff");
            require(!r.instance->inspect().storage_active && r.dispatch->executions == 0,
                    "Failed inspection cannot activate");
            r.stop();
        });
    }
    void completion_throw_without_replay_HL07() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->completion = [](auto &, const auto &) {
                throw std::runtime_error("completion exception");
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; }, wait_ms),
                "Exception cleanup");
            require(r.dispatch->completions == 1 && r.dispatch->executions == 1 &&
                        r.transport->handoffs == 0 &&
                        r.log->last(K::writer_completed).work->transaction ==
                            TransactionOutcome::committed,
                    "Actual committed truth, one supplied callback, no reply/replay");
            r.stop();
        });
    }

    void body_stop_then_commit_retains_truth_HL07() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [&](auto context) -> QVariant {
                const auto tx = context.begin();
                r.instance->request_stop();
                tx.commit();
                return 1;
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Truth completes");
            r.stop();
            require(r.log->last(K::writer_completed).work->transaction ==
                            TransactionOutcome::committed &&
                        r.transport->handoffs == 0,
                    "Stop never preempts commit, forbids subsequent publication");
        });
    }

    void oversized_response_uses_control_or_close() {
        scenario([] {
            Rig r("current", [](auto &o) { o.limits.response_bytes = 4; });
            r.start();
            auto p = r.peer();
            r.dispatch->completion = [](auto &command, const auto &) {
                failure(command.finish(reply("12345")), host::Failure::resource_limit);
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; }, wait_ms),
                "Oversized response settles");
            require(r.log->has_refusal(host::Failure::resource_limit) &&
                        r.instance->inspect().response.capacity == 0,
                    "Bounded control refusal or close, no payload lease");
            r.stop();
        });
    }
    void first_io99_after_owner100_is_valid() {
        scenario([] {
            Rig r;
            r.elapsed->now = 100;
            r.elapsed->foreign_now = 99;
            r.elapsed->foreign_override = true;
            r.start();
            const auto owner_reads = r.elapsed->owner_reads.load();
            const auto io_reads = r.elapsed->foreign_reads.load();
            auto p = r.peer();
            success(r.submit(p, "D", host::RouteId::diagnostic),
                    "First IO sample99 is not owner regression");
            require(QTest::qWaitFor([&] { return r.transport->takes == 1; }, wait_ms),
                    "Diagnostic actually enters Qt100 and is consumed on IO99");
            r.transport->check();
            r.check_callbacks();
            require(r.elapsed->owner_reads > owner_reads && r.elapsed->owner_last == 100 &&
                        r.elapsed->foreign_reads > io_reads && r.elapsed->foreign_last == 99 &&
                        r.dispatch->diagnostics == 1 && r.log->count(K::local_handoff) == 1,
                    "Both owner observations and actual response consumption, no global ordering");
            const auto second_reads = r.elapsed->foreign_reads.load();
            success(r.submit(p, "D2", host::RouteId::diagnostic), "Same IO99 accepted again");
            require(QTest::qWaitFor([&] { return r.transport->takes == 2; }, wait_ms),
                    "Second IO99 consumed");
            r.transport->check();
            require(r.elapsed->foreign_reads > second_reads && r.elapsed->foreign_last == 99 &&
                        r.dispatch->diagnostics == 2 && r.log->refusal_count() == 0,
                    "Equal same-owner time allowed; cross-owner100 never poisons IO99");
            r.stop();
        });
    }
    void elapsed_failure_at_entry_and_io_handoff_HL16_data() {
        QTest::addColumn<bool>("handoff");
        QTest::newRow("entry") << false;
        QTest::newRow("handoff") << true;
    }
    void elapsed_failure_at_entry_and_io_handoff_HL16() {
        QFETCH(bool, handoff);
        scenario([&] {
            Rig r;
            r.transport->retain_replies = true;
            r.start();
            auto p = r.peer();
            success(r.submit(p), "Queued while elapsed valid");
            if (handoff) {
                r.completed();
                require(QTest::qWaitFor([&] { return r.transport->handoffs == 1; }, wait_ms),
                        "Retained for IO gate");
            }
            r.elapsed->available = false;
            if (handoff)
                r.transport->on_io([&] {
                    failure(r.transport->replies[0]->take(), host::Failure::elapsed_unavailable);
                    r.transport->replies[0].reset();
                });
            else
                require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                        wait_ms),
                        "Entry refuses");
            require(r.log->count(K::writer_completed) == unsigned(handoff) &&
                        r.dispatch->executions == unsigned(handoff),
                    "Entry no SQL; handoff retains executed truth");
            r.elapsed->available = true;
            r.transport->on_io([&] {
                failure(r.transport->ingress->try_submit(p, bytes("late"),
                                                         host::RouteId::diagnostic, bytes("")),
                        host::Failure::disconnected);
            });
            r.stop();
        });
    }
    void exact_max_deadline_then_overflow_HL16() {
        scenario([] {
            Rig r("current", [](auto &o) { o.limits.queue_ms = 1; });
            r.start();
            auto p = r.peer();
            r.elapsed->now = UINT64_MAX - 1000000;
            success(r.submit(p, "exact", host::RouteId::diagnostic), "Deadline exactly MAX valid");
            r.elapsed->now = UINT64_MAX - 999999;
            failure(r.submit(p, "overflow", host::RouteId::diagnostic),
                    host::Failure::elapsed_unavailable);
            r.stop();
        });
    }
    void owner_thread_and_destruction() {
        scenario([] {
            Rig r;
            const auto owner = std::this_thread::get_id();
            r.start();
            auto p = r.peer();
            success(r.submit(p), "IO submit");
            r.completed();
            r.stop();
            for (auto k : {K::command_entered, K::unit_entered, K::writer_completed,
                           K::writer_removed, K::dispatcher_destroyed, K::owner_cleanup})
                require(r.log->last(k).thread == owner,
                        "Actual SQL/source/facade/cleanup on Qt owner");
            require(r.dispatch->body_thread == owner && r.dispatch->completion_thread == owner,
                    "Unit body and supplied completion on owner");
            require(r.log->last(K::local_handoff).thread == r.transport->io_thread,
                    "Actual handoff on IO");
        });
    }
    void prepared_units_serial_once_and_next_command() {
        scenario([] {
            unsigned bodies = 0, callbacks = 0;
            bool callback_returned = false;
            std::function<void(host::CommandHandle &)> next;
            Rig r;
            r.start();
            auto p = r.peer();
            next = [&](auto &command) {
                admitted(command.forward(
                    [&](auto context) {
                        require(r.dispatch->executions == 1 && r.dispatch->diagnostics == 0,
                                "No command or diagnostic interleaves units");
                        ++bodies;
                        return read_installation(context);
                    },
                    [&](auto result) {
                        ++callbacks;
                        require(result.transaction == TransactionOutcome::committed &&
                                    result.error == WorkError::none && result.value,
                                "Independent normal unit oracle");
                        const auto events = r.log->matching(K::writer_completed);
                        require(events.size() == callbacks && events.back().work->ticket_revoked,
                                "Actual completion observation after ticket revocation precedes "
                                "supplied callback");
                        if (callbacks < 3)
                            next(*r.dispatch->command);
                        else {
                            success(r.dispatch->command->finish(reply("done")), "Finish once");
                            failure(r.dispatch->command->finish(reply("evil")),
                                    host::Failure::invalid_completion);
                            require(r.transport->handoffs == 0 && r.dispatch->diagnostics == 0,
                                    "Handoff and next entry wait for normal completion return");
                            callback_returned = true;
                        }
                    }));
            };
            r.dispatch->execution = [&](auto &command, auto) { next(command); };
            r.dispatch->diagnostic_body = [&](auto) {
                require(callback_returned && bodies == 3 && callbacks == 3,
                        "Next command after finish/drain");
                return reply("diag");
            };
            success(r.submit(p, "A"), "Protected command");
            success(r.submit(p, "D", host::RouteId::diagnostic), "Waiting diagnostic");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.transport->takes == 2;
                        },
                        wait_ms),
                    "Two replies");
            require(r.log->count(K::unit_entered) == 3 && r.dispatch->executions == 1 &&
                        r.dispatch->diagnostics == 1 && r.log->count(K::finish_refused) == 1,
                    "Three units, once-only command reply plus next diagnostic");
            r.stop();
        });
    }
    void body_and_pending_finish_terminal_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("body_forward") << 0;
        QTest::newRow("body_finish") << 1;
        QTest::newRow("pending_finish") << 2;
    }
    void body_and_pending_finish_terminal() {
        QFETCH(int, kind);
        scenario([&] {
            unsigned completed = 0, forbidden = 0;
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &command, auto) {
                admitted(command.forward(
                    [&](auto context) {
                        const auto input =
                            success(r.dispatch->command->input(), "Accepted body input");
                        require(input.bytes.size() == 4,
                                "Input remains charged through terminal drain");
                        if (kind == 0) {
                            const auto refused = r.dispatch->command->forward(
                                [&](auto) -> QVariant {
                                    ++forbidden;
                                    return 0;
                                },
                                [&](auto) { ++forbidden; });
                            require(std::holds_alternative<host::Failure>(refused) &&
                                        std::get<host::Failure>(refused) ==
                                            host::Failure::invalid_completion,
                                    "Forward from body terminal invalid_completion");
                        } else if (kind == 1)
                            failure(r.dispatch->command->finish(reply("bad")),
                                    host::Failure::invalid_completion);
                        return read_installation(context);
                    },
                    [&](auto result) {
                        ++completed;
                        require(result.transaction == TransactionOutcome::committed &&
                                    result.error == WorkError::none && result.value,
                                "Terminal refusal does not preempt accepted unit");
                        require(success(r.dispatch->command->input(), "Accepted completion input")
                                        .bytes.size() == 4,
                                "Terminal input lives through accepted completion");
                        failure(r.dispatch->command->finish(reply("late")),
                                host::Failure::invalid_completion);
                    }));
                if (kind == 2)
                    failure(command.finish(reply("early")), host::Failure::invalid_completion);
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Terminal drain");
            require(completed == 1 && forbidden == 0 && r.log->count(K::writer_completed) == 1 &&
                        r.transport->handoffs == 0 && r.instance->inspect().input.capacity == 0,
                    "Drain once, no fabricated extra writer work, release input");
            r.stop();
        });
    }
    void reply_revocation_allows_later_unit_data() {
        QTest::addColumn<bool>("disconnect");
        QTest::newRow("disconnect") << true;
        QTest::newRow("elapsed_unavailable") << false;
    }
    void reply_revocation_allows_later_unit() {
        QFETCH(bool, disconnect);
        scenario([&] {
            unsigned completed = 0;
            const std::byte *input = nullptr;
            std::optional<host::PeerLease> peer;
            Rig r;
            r.start();
            peer.emplace(r.peer());
            auto &p = *peer;
            r.dispatch->execution = [&](auto &command, auto view) {
                input = view.bytes.data(); // borrow only; no copied payload
                admitted(command.forward(read_installation, [&](auto first) {
                    require(first.transaction == TransactionOutcome::committed,
                            "First unit commits");
                    ++completed;
                    if (disconnect)
                        r.disconnect(*peer);
                    else
                        r.elapsed->available = false;
                    admitted(r.dispatch->command->forward(
                        [&](auto context) {
                            const auto retained = success(r.dispatch->command->input(),
                                                          "Borrow after reply revocation");
                            require(retained.bytes.data() == input && retained.bytes.size() == 4 &&
                                        r.instance->inspect().input.capacity == 4 &&
                                        r.instance->inspect().input.copies == 1,
                                    "Same charged host payload survives accepted drain");
                            return read_installation(context);
                        },
                        [&](auto last) {
                            ++completed;
                            require(last.transaction == TransactionOutcome::committed,
                                    "Later unit still commits");
                            failure(r.dispatch->command->finish(reply("done")),
                                    disconnect ? host::Failure::disconnected
                                               : host::Failure::elapsed_unavailable);
                        }));
                }));
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Drain after reply revocation");
            require(completed == 2 && r.log->count(K::writer_completed) == 2 &&
                        r.transport->handoffs == 0 && r.instance->inspect().input.capacity == 0,
                    "No active-handle timer or payload pin");
            r.elapsed->available = true;
            r.stop();
        });
    }
    void foreign_handle_calls_preserve_owner_command() {
        scenario([] {
            unsigned forbidden = 0;
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &command, auto) {
                const auto before = r.instance->inspect();
                const auto reads = r.elapsed->reads.load();
                std::exception_ptr problem;
                std::thread foreign([&] {
                    try {
                        failure(command.input(), host::Failure::wrong_thread);
                        failure(command.finish(reply("evil")), host::Failure::wrong_thread);
                        const auto result = command.forward(
                            [&](auto) -> QVariant {
                                ++forbidden;
                                return 0;
                            },
                            [&](auto) { ++forbidden; });
                        require(std::holds_alternative<host::Failure>(result) &&
                                    std::get<host::Failure>(result) == host::Failure::wrong_thread,
                                "Foreign forward refuses before owner access");
                    } catch (...) {
                        problem = std::current_exception();
                    }
                });
                foreign.join(); // no owner event pump, no accepted unit yet
                if (problem)
                    std::rethrow_exception(problem);
                const auto after = r.instance->inspect();
                require(after.associations == before.associations &&
                            after.input.capacity == before.input.capacity &&
                            after.accepted_units == before.accepted_units &&
                            r.elapsed->reads == reads && forbidden == 0,
                        "Foreign calls do not consume response, mutate owner, sample time or "
                        "enqueue work");
                success(command.finish(reply("done")), "Owner response still eligible");
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.transport->takes == 1;
                        },
                        wait_ms),
                    "Owner reply");
            require(r.log->count(K::writer_completed) == 0, "Zero-unit reply has no writer result");
            r.stop();
        });
    }
    void actual_writer_refusal_drains_accepted_unit() {
        scenario([] {
            unsigned forbidden = 0, completed = 0;
            Rig r("current", [](auto &o) {
                o.limits.associations = o.limits.per_peer = 1;
                o.writer_limits.pending_batches = 1;
            });
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &command, auto) {
                admitted(command.forward(read_installation, [&](auto result) {
                    ++completed;
                    require(result.transaction == TransactionOutcome::committed,
                            "Accepted unit still commits");
                }));
                const auto result = command.forward(
                    [&](auto) -> QVariant {
                        ++forbidden;
                        return 0;
                    },
                    [&](auto) { ++forbidden; });
                require(std::holds_alternative<StorageError>(result) &&
                            std::get<StorageError>(result) == StorageError::queue_full,
                        "Actual writer admission, not host prediction");
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Refused writer drain");
            const auto refused = r.log->last(K::forward_refused);
            require(refused.admission == StorageError::queue_full && !refused.work &&
                        completed == 1 && forbidden == 0 &&
                        r.log->count(K::writer_completed) == 1 && r.transport->handoffs == 0,
                    "No completion for unaccepted unit");
            r.stop();
        });
    }
    void idle_and_abandoned_handles_release_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("idle_retained") << 0;
        QTest::newRow("owner_abandon") << 1;
        QTest::newRow("foreign_abandon") << 2;
    }
    void idle_and_abandoned_handles_release() {
        QFETCH(int, kind);
        scenario([&] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &, auto) {
                if (kind == 1)
                    r.dispatch->command.reset();
                if (kind == 2) {
                    std::thread foreign([handle = std::move(*r.dispatch->command)] {});
                    foreign.join();
                    r.dispatch->command.reset();
                }
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Idle/abandon owner cleanup");
            require(r.instance->inspect().input.capacity == 0 && r.transport->handoffs == 0 &&
                        r.log->count(K::writer_completed) == 0,
                    "No pin or fabricated writer completion");
            if (kind == 0) {
                const auto late = r.dispatch->command->forward(read_installation, [](auto) {});
                require(std::holds_alternative<host::Failure>(late) &&
                            std::get<host::Failure>(late) == host::Failure::invalid_completion,
                        "Inert handle cannot revive work");
                failure(r.dispatch->command->finish(reply("late")),
                        host::Failure::invalid_completion);
            }
            r.stop();
        });
    }
    void known_noncommits_and_zero_unit_may_reply_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("no_units") << 0;
        QTest::newRow("rollback") << 1;
        QTest::newRow("body_throw_before_commit") << 2;
        QTest::newRow("no_begin") << 3;
    }
    void known_noncommits_and_zero_unit_may_reply() {
        QFETCH(int, kind);
        scenario([&] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &command, auto) {
                if (kind == 0) {
                    success(command.finish(reply("deny")), "Zero-unit reply");
                    return;
                }
                admitted(command.forward(
                    [&](auto context) -> QVariant {
                        if (kind == 3)
                            return 0;
                        const auto session = context.begin();
                        account_sessions_fixture::Cleanup cleanup{session};
                        if (kind == 2)
                            throw std::runtime_error("before commit");
                        session.rollback();
                        return 0;
                    },
                    [&](auto result) {
                        require(!result.value && result.transaction ==
                                                     (kind == 3 ? TransactionOutcome::not_started
                                                                : TransactionOutcome::rolled_back),
                                "Actual known noncommit, never success");
                        require(result.error == (kind == 1   ? WorkError::not_committed
                                                 : kind == 2 ? WorkError::work_exception
                                                             : WorkError::missing_transaction),
                                "Independently derived actual writer error");
                        success(r.dispatch->command->finish(reply("deny")),
                                "Known noncommit may return dispatcher refusal");
                    }));
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.transport->takes == 1;
                        },
                        wait_ms),
                    "Nonsecret refusal reply");
            require(r.log->count(K::writer_completed) == unsigned(kind != 0),
                    "No invented zero-unit completion");
            r.stop();
        });
    }
    void entered_command_outlives_queue_deadline() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->completion = [&](auto &command, const auto &result) {
                require(result.transaction == TransactionOutcome::committed, "First normal unit");
                r.elapsed->now = 6000000000; // original entry budget5s has passed
                admitted(command.forward(read_installation, [&](auto final) {
                    require(final.transaction == TransactionOutcome::committed,
                            "No entered-handle timeout");
                    success(r.dispatch->command->finish(reply("done")),
                            "Reply uses current handoff anchor");
                }));
            };
            success(r.submit(p), "Admit before entry deadline");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.transport->takes == 1;
                        },
                        wait_ms),
                    "Entered work completes");
            require(r.dispatch->executions == 1 && r.log->count(K::writer_completed) == 2,
                    "No replay/expiry of entered handle");
            r.stop();
        });
    }
    void callback_exception_drains_and_withholds_data() {
        QTest::addColumn<bool>("execute_throw");
        QTest::newRow("execute_after_accept") << true;
        QTest::newRow("completion_after_finish") << false;
    }
    void callback_exception_drains_and_withholds() {
        QFETCH(bool, execute_throw);
        scenario([&] {
            unsigned completed = 0;
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &command, auto) {
                admitted(command.forward(read_installation, [&](auto result) {
                    ++completed;
                    require(result.transaction == TransactionOutcome::committed &&
                                result.error == WorkError::none && result.value,
                            "Actual accepted work drains despite callback error");
                    if (!execute_throw) {
                        success(r.dispatch->command->finish(reply("done")),
                                "Tentative once-only finish");
                        throw std::runtime_error("completion after chosen reply");
                    }
                }));
                if (execute_throw)
                    throw std::runtime_error("execute after accepted unit");
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Exception drains");
            require(completed == 1 && r.log->count(K::writer_completed) == 1 &&
                        r.transport->handoffs == 0,
                    "No reply before normal callback return, no replay");
            r.stop();
        });
    }
    void actual_ticket_revoked_before_supplied_completion() {
        scenario([] {
            SqliteWorkSession ticket;
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [&](auto context) -> QVariant {
                ticket = context.begin();
                ticket.commit();
                return 17;
            };
            r.dispatch->completion = [&](auto &command, const auto &result) {
                const auto observed = r.log->last(K::writer_completed).work;
                require(observed && observed->ticket_revoked &&
                            observed->transaction == result.transaction &&
                            observed->error == result.error && result.value &&
                            result.value->toInt() == 17,
                        "Actual result observed before supplied completion, value unmodified");
                bool stale = false;
                try {
                    (void)ticket.execute({"SELECT 1", {}});
                } catch (const SqliteWorkFailure &error) {
                    stale = error.error() == WorkError::stale_session;
                }
                require(stale, "Real ticket is already revoked, no observer boolean substitute");
                success(command.finish(reply("done")), "Finish after actual result");
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.transport->takes == 1;
                        },
                        wait_ms),
                    "Normal reply");
            r.stop();
        });
    }
    void abnormal_commit_withholding_latches_across_normal_unit() {
        scenario([] {
            unsigned completed = 0;
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->execution = [&](auto &command, auto) {
                admitted(command.forward(
                    [](auto context) -> QVariant {
                        const auto session = context.begin();
                        session.commit();
                        throw std::runtime_error("after commit");
                    },
                    [&](auto first) {
                        ++completed;
                        require(first.transaction == TransactionOutcome::committed &&
                                    first.error == WorkError::work_exception && !first.value,
                                "Actual abnormal commit");
                        admitted(r.dispatch->command->forward(read_installation, [&](auto last) {
                            ++completed;
                            require(last.transaction == TransactionOutcome::committed &&
                                        last.error == WorkError::none && last.value,
                                    "Later normal commit does not erase withholding");
                            failure(r.dispatch->command->finish(reply("done")),
                                    host::Failure::invalid_completion);
                        }));
                    }));
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor(
                        [&] {
                            r.check_callbacks();
                            return r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Withheld cleanup");
            require(completed == 2 && r.log->count(K::writer_completed) == 2 &&
                        r.transport->handoffs == 0,
                    "True results unchanged, latch suppresses reply, no replay");
            r.stop();
        });
    }
};
QTEST_GUILESS_MAIN(ApiHostTests)
#include "api_host_tests.moc"
