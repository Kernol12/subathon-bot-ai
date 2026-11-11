// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/api_host_fixture.hpp"
#include <limits>

using namespace api_host_fixture;
using K = hooks::EventKind;
class ApiHostTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void ordinary_schema8_activation_and_fresh_publication_HL06() {
        scenario([] {
            Rig r;
            const auto before = r.store.sessions();
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
            require(r.log->count(K::precommit_read) == 1 &&
                        r.log->position(K::precommit_read) < r.log->position(K::actual_commit),
                    "Same transaction final read before COMMIT");
            require(r.store.facts() == SqlRows{{QString("49"), QString("45"), QString("4A")}},
                    "No startup epoch rotation");
            require(r.store.sessions() == before,
                    "All session bindings/digest/state/times unchanged");
            require(std::holds_alternative<SessionSourceUnavailable>(
                        r.dispatch->execution_clock->sample_current()),
                    "Retained execution facade revoked at completion");
            require(std::holds_alternative<SessionSourceUnavailable>(
                        r.dispatch->publication_clock->sample_current()),
                    "Fresh publication facade revoked after callback");
            r.stop();
        });
    }
    void activation_refusal_data() {
        QTest::addColumn<QString>("store");
        for (auto s :
             {"fresh", "old", "future", "wrong", "malformed_catalog", "missing", "malformed"})
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
            auto p = r.peer();
            // Diagnostic success proves listener serving despite refused activation.
            success(r.submit(p, "D", host::RouteId::diagnostic), "Diagnostic admission");
            require(QTest::qWaitFor([&] { return r.transport->takes.load() == 1; }, wait_ms),
                    "Diagnostic before activation");
            require(!r.instance->inspect().storage_active && r.dispatch->diagnostics == 1 &&
                        r.dispatch->executions == 0 && r.sampler->calls == 0,
                    "Diagnostics need no source/UTC");
            const auto protected_admission = r.submit(p, "P");
            if (std::holds_alternative<host::Admitted>(protected_admission)) {
                require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                        wait_ms),
                        "Refused work settles");
            } else
                failure(protected_admission, host::Failure::unavailable);
            require(r.dispatch->executions == 0 && r.sampler->calls == 0,
                    "Protected entry refused");
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
            Rig r("current", [&](auto &o) { o.limits.associations = cap; });
            r.start();
            std::vector<host::PeerLease> peers;
            for (int n = 0; n < (cap + 7) / 8; ++n)
                peers.push_back(r.peer());
            for (int n = 0; n < cap; ++n)
                success(r.submit(peers[n / 8], std::string(1, char(n + 1))),
                        "Fill global capacity");
            const auto before = r.instance->inspect();
            failure(r.submit(peers[0], "overflow"), host::Failure::resource_limit);
            const auto after = r.instance->inspect();
            require(after.associations == std::uint64_t(cap) &&
                        after.input.capacity == before.input.capacity &&
                        after.input.copies == before.input.copies && r.dispatch->executions == 0,
                    "Refuse before copy/body");
            r.completed(cap);
            auto p = r.peer();
            success(r.submit(p, "D"), "Released capacity reusable");
            r.completed(cap + 1);
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
                require(r.dispatch->executions == 0 && r.log->count(K::begin_attempted) == 0 &&
                            r.sampler->calls == 0,
                        "Equality no body/BEGIN/sample");
                const auto w = r.log->last(K::writer_completed).work;
                require(w && w->transaction == TransactionOutcome::not_started &&
                            w->error == WorkError::missing_transaction && !w->has_value,
                        "Real no-BEGIN truth");
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
            require(r.dispatch->executions == 0 && r.log->count(K::begin_attempted) == 0,
                    "Disconnect wins");
            r.stop();
        });
    }
    void disconnect_after_commit_HL04_HL13() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.log->observe = [&](const auto &e) {
                if (e.kind == K::execution_revoked)
                    r.disconnect(p);
            };
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
    void clock_revocation_thread_and_reentrancy_HL05() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            std::shared_ptr<CurrentSessionClock> retained;
            r.dispatch->body = [&](auto &e, auto) -> host::OwnedDomainResult {
                retained = e.clock();
                const auto calls = r.sampler->calls.load();
                r.sampler->during_sample = [&] {
                    require(std::holds_alternative<SessionSourceUnavailable>(
                                retained->sample_current()),
                            "Reentrant denied");
                };
                require(std::holds_alternative<SessionClockSample>(retained->sample_current()),
                        "Live guarded synthetic probe");
                require(r.sampler->calls == calls + 1, "One underlying sample, no recursion");

                auto &t = e.begin_transaction();
                t.commit();
                return reply("done");
            };
            success(r.submit(p), "A");
            r.completed();
            auto calls = r.sampler->calls.load();
            success(r.submit(p, "B"), "B queued");
            require(std::holds_alternative<SessionSourceUnavailable>(retained->sample_current()) &&
                        r.sampler->calls == calls,
                    "No queue-time/cached authority");
            r.completed(2);
            bool foreign_unavailable = false;
            std::thread foreign([clock = retained, &foreign_unavailable] {
                foreign_unavailable =
                    std::holds_alternative<SessionSourceUnavailable>(clock->sample_current());
            });
            foreign.join();
            require(foreign_unavailable && r.sampler->calls == calls + 1,
                    "Foreign facade no extra raw reads");
            r.stop();
            const auto after = r.sampler->calls.load();
            r.instance.reset();
            require(std::holds_alternative<SessionSourceUnavailable>(retained->sample_current()) &&
                        r.sampler->calls == after,
                    "Ownable facade safe after host destruction");
        });
    }
    void commit_then_throw_HL07() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [](auto &e, auto) -> host::OwnedDomainResult {
                auto &t = e.begin_transaction();
                t.commit();
                throw std::runtime_error("after commit");
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::committed &&
                        w->error == WorkError::work_exception && !w->has_value,
                    "Committed exception unchanged");
            require(r.dispatch->completions == 0 && r.transport->handoffs == 0,
                    "No normal envelope reconstructed");
            r.stop();
        });
    }
    void invalid_domain_result_HL07() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [](auto &e, auto) -> host::OwnedDomainResult {
                auto &t = e.begin_transaction();
                t.commit();
                return host::Failure::invalid_completion;
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Completion");
            require(r.log->last(K::writer_completed).work->transaction ==
                            TransactionOutcome::committed &&
                        r.transport->handoffs == 0 && r.dispatch->completions == 0,
                    "Invalid result never a successful private envelope");
            r.stop();
        });
    }
    void lost_ack_and_fail_closed_fifo_HL07_HL14() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            success(
                hooks::HostAccess::inject_next_control_ack_loss(
                    *r.instance, subathon::server::testing::WorkControlFault::lose_next_commit_ack),
                "Forward real writer fault seam");
            success(r.submit(p, "1"), "W1");
            success(r.submit(p, "2"), "W2");
            success(r.submit(p, "3"), "W3");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 3; }, wait_ms),
                "All actual completions");
            std::vector<hooks::WorkObservation> outcomes;
            for (std::size_t i = 0; i < r.log->used; ++i)
                if (r.log->events[i].kind == K::writer_completed)
                    outcomes.push_back(*r.log->events[i].work);
            require(outcomes[0].transaction == TransactionOutcome::outcome_unknown &&
                        outcomes[0].error == WorkError::uncertain_outcome,
                    "Actual lost ack, no rollback claim");
            for (unsigned i = 1; i < 3; ++i)
                require(outcomes[i].transaction == TransactionOutcome::not_started &&
                            outcomes[i].error == WorkError::writer_unavailable &&
                            !outcomes[i].has_value,
                        "Actual fail-closed FIFO");
            require(r.dispatch->executions == 1 && r.transport->handoffs == 0,
                    "No queued domain/replay/publication");
            r.stop();
        });
    }
    void actual_epoch_mutations_reach_generation7_then8_HL09() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            std::string target, revision;
            r.dispatch->body = [&](auto &e, auto) -> host::OwnedDomainResult {
                auto &t = e.begin_transaction();
                auto facts = t.accounts().read_installation("I");
                require(std::holds_alternative<StoredInstallationFacts>(facts),
                        "Real current facts");
                require(t.accounts().replace_installation_epoch(
                            std::get<StoredInstallationFacts>(facts), target, revision) ==
                            LifecycleWriteResult::staged,
                        "Real expected-facts CAS");
                t.commit();
                return reply("done");
            };
            r.dispatch->completion = [](auto &p, const auto &) {
                success(p.current_installation(), "Current changed facts or suppress");
                return reply("done");
            };
            // Actual writes, distinct epochs/revisions; never a generation setter.
            for (int i = 0; i < 6; ++i) {
                target = i == 5 ? "E" : "epoch" + std::to_string(i);
                revision = "revision" + std::to_string(i);
                success(r.submit(p, std::string(1, char(i + 1))), "Actual rebind");
                require(
                    QTest::qWaitFor(
                        [&] { return r.instance->inspect().generation == std::uint64_t(i + 2); },
                        wait_ms),
                    "Automatic generation advance");
            }
            require(r.instance->inspect().generation == 7, "Generation7 from six actual mutations");
            target = "F";
            revision = "N";
            success(r.submit(p, "B"), "B first");
            success(r.submit(p, "A", host::RouteId::installation, "I/F"), "A captured7");
            require(QTest::qWaitFor(
                        [&] {
                            return r.instance->inspect().generation == 8 &&
                                   r.instance->inspect().associations == 0;
                        },
                        wait_ms),
                    "Rebind8 before A entry");
            require(r.dispatch->executions == 7 && r.log->count(K::begin_attempted) == 7,
                    "A text cannot relabel captured generation");
            require(r.store.facts() == SqlRows{{QString("49"), QString("46"), QString("4E")}},
                    "Actual persisted I/F/N");
            r.dispatch->body = [](auto &e, auto) -> host::OwnedDomainResult {
                const auto f = success(e.current_installation(), "Fresh execution source");
                require(f.epoch == "F", "Fresh sees F");
                auto &t = e.begin_transaction();
                t.commit();
                return reply("done");
            };
            success(r.submit(p, "fresh"), "New generation work");
            require(QTest::qWaitFor([&] { return r.dispatch->executions == 8; }, wait_ms),
                    "Fresh body");
            r.stop();
        });
    }
    void stop_fifo_truth_HL14() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.log->observe = [&](const auto &e) {
                if (e.kind == K::writer_completed && r.log->count(K::writer_completed) == 1)
                    r.instance->request_stop();
            };
            success(r.submit(p, "1"), "W1");
            success(r.submit(p, "2"), "W2");
            success(r.submit(p, "3"), "W3");
            // Actual wrapper acceptance must be observable BEFORE stop, never inferred.
            require(QTest::qWaitFor(
                        [&] {
                            return r.log->count(K::writer_completed) == 3 &&
                                   r.instance->inspect().state == host::State::stopped;
                        },
                        wait_ms),
                    "FIFO then shutdown");
            std::vector<hooks::WorkObservation> results;
            for (std::size_t i = 0; i < r.log->used; ++i)
                if (r.log->events[i].kind == K::writer_completed)
                    results.push_back(*r.log->events[i].work);
            require(r.log->count(K::wrapper_accepted) == 3 &&
                        r.log->count(K::wrapper_dispatched) == 3 && r.dispatch->executions == 1,
                    "Every accepted wrapper dispatches once");
            require(results[0].transaction == TransactionOutcome::committed, "W1 truth");
            for (unsigned i = 1; i < 3; ++i)
                require(results[i].transaction == TransactionOutcome::not_started &&
                            results[i].error == WorkError::missing_transaction &&
                            !results[i].has_value,
                        "W2/W3 real missing transaction, not rollback");
            require(r.log->position(K::writer_ack) > r.log->position(K::stop_consumed) &&
                        r.log->count(K::begin_attempted) == 1,
                    "One BEGIN; shutdown last");
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
            require(r.dispatch->executions == 1 && r.dispatch->completions == 0,
                    "No implicit replay");
            r.stop();
        });
    }
    void diagnostic_still_queued_at_stop_HL17_HL12() {
        scenario([] {
            Rig r("future");
            r.start(false);
            auto p = r.peer();
            success(r.submit(p, "D", host::RouteId::diagnostic), "Diagnostic queued");
            require(r.instance->inspect().queued_diagnostics == 1 && r.dispatch->diagnostics == 0,
                    "Observe actual queued dependency before ANY event pumping");
            r.instance->request_stop();
            r.stop();
            require(r.dispatch->diagnostics == 0 && r.transport->handoffs == 0 &&
                        r.log->count(K::diagnostic_cancelled) == 1,
                    "Stop cancels data-only callback");
            require(r.log->count(K::begin_attempted) == 0 && r.log->count(K::writer_completed) == 0,
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
            require(r.dispatch->executions == 0 && r.sampler->calls == 0 &&
                        r.transport->handoffs == 0,
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
            success(r.submit(p, "A"), "4 fits");
            success(r.submit(p, "B"), "Second4 fits");
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
            r.stop();
        });
    }
    void copy_slots_exhaust_even_with_payload_capacity() {
        scenario([] {
            Rig r("current", [](auto &o) {
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
            require(r.transport->takes == 0, "Delayed IO continuation cannot refresh expiry");
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
            require(r.dispatch->executions == 1 && r.log->count(K::actual_commit) == 1,
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
    void wall_utc_jumps_do_not_move_elapsed_deadline_HL15() {
        scenario([] {
            for (auto utc : {INT64_MIN, INT64_MAX}) {
                Rig r;
                r.elapsed->now = 1000000000;
                r.start();
                auto p = r.peer();
                success(r.submit(p), "Queue at1000ms");
                r.sampler->utc = utc;
                r.elapsed->now = 6000000000;
                require(QTest::qWaitFor([&] { return r.instance->inspect().associations == 0; },
                                        wait_ms),
                        "Original deadline expires at6000");
                require(r.dispatch->executions == 0 && r.log->count(K::begin_attempted) == 0,
                        "UTC jump cannot extend entry");
                r.stop();
            }
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
            auto p = r.peer();
            if (which == 0)
                r.elapsed->now = UINT64_MAX;
            if (which == 1)
                r.elapsed->available = false;
            if (which == 2)
                r.elapsed->now = 99;
            failure(r.submit(p), host::Failure::elapsed_unavailable);
            require(r.dispatch->executions == 0 && r.log->count(K::begin_attempted) == 0,
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
            const auto reads = r.elapsed->reads.load();
            auto p = r.peer();
            success(r.submit(p), "Admit");
            r.completed();
            require(r.elapsed->reads > reads, "Actual host deadline reads wired");
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
            if (phase == 0)
                r.instance->request_stop();
            if (phase == 1)
                r.log->observe = [&](const auto &e) {
                    if (e.kind == K::inspection_enqueued)
                        r.instance->request_stop();
                };
            if (phase == 2)
                r.log->observe = [&](const auto &e) {
                    if (e.kind == K::installation_read_enqueued)
                        r.instance->request_stop();
                };
            auto result = r.instance->start([&](auto) {
                if (phase == 3)
                    throw std::runtime_error("startup observer");
            });
            require(!std::holds_alternative<host::NotImplemented>(result),
                    "UNIMPLEMENTED production host start");
            if (phase == 3 && r.transport->startup)
                r.transport->startup(host::StartAccepted{});
            if (phase == 0 || phase == 1 || phase == 2) {
                require(
                    QTest::qWaitFor([&] { return r.log->count(K::stop_consumed) == 1; }, wait_ms),
                    "Startup stop consumed");
                if (r.transport->startup)
                    r.transport->startup(
                        host::StartAccepted{}); // deliberately late actual transport event
            }
            r.stop();
            require(r.dispatch->executions == 0 && r.log->count(K::stop_consumed) == 1,
                    "No transient admission, one cleanup");
            failure(r.instance->start({}), host::Failure::already_started);
        });
    }
    void precommit_selected_read_failure_rolls_back_actual_mutation() {
        scenario([] {
            Rig r("current", [](auto &o) { o.store_limits.selected_bytes = 4; });
            r.start();
            auto p = r.peer();
            r.dispatch->body = [](auto &e, auto) -> host::OwnedDomainResult {
                auto &t = e.begin_transaction();
                const auto f = t.accounts().read_installation("I");
                require(std::holds_alternative<StoredInstallationFacts>(f),
                        "Old I/E/J fits selected read");
                require(t.accounts().replace_installation_epoch(
                            std::get<StoredInstallationFacts>(f), "12345", "N") ==
                            LifecycleWriteResult::staged,
                        "Real staged mutation within independent mutation budget");
                t.commit();
                return reply("done");
            };
            success(r.submit(p), "Admit");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Actual precommit refusal completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::rolled_back &&
                        w->error != WorkError::none && !w->has_value,
                    "Final read cannot be bypassed by COMMIT");
            require(r.log->count(K::actual_commit) == 0 && r.dispatch->completions == 0 &&
                        r.instance->inspect().generation == 1,
                    "No candidate bound/publication");
            require(r.store.facts() == SqlRows{{QString("49"), QString("45"), QString("4A")}},
                    "Actual known rollback preserves original facts");
            r.stop();
        });
    }
    void known_rollback_preserves_source_and_generation() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            const auto g = r.instance->inspect().generation;
            r.dispatch->body = [](auto &e, auto) -> host::OwnedDomainResult {
                auto &t = e.begin_transaction();
                const auto f = t.accounts().read_installation("I");
                require(t.accounts().replace_installation_epoch(
                            std::get<StoredInstallationFacts>(f), "F", "N") ==
                            LifecycleWriteResult::staged,
                        "Stage real mutation");
                t.rollback();
                return reply("done");
            };
            success(r.submit(p), "Admit rollback");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Actual rollback completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::rolled_back &&
                        w->error == WorkError::not_committed && !w->has_value,
                    "Known rollback truth");
            require(r.instance->inspect().generation == g &&
                        r.store.facts() == SqlRows{{QString("49"), QString("45"), QString("4A")}},
                    "Previous committed source retained");
            require(r.dispatch->completions == 0 && r.transport->handoffs == 0,
                    "No rollback publication");
            r.stop();
        });
    }
    void changed_commit_then_throw_rebinds_but_never_publishes() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            r.dispatch->body = [](auto &e, auto) -> host::OwnedDomainResult {
                auto &t = e.begin_transaction();
                const auto f = t.accounts().read_installation("I");
                require(t.accounts().replace_installation_epoch(
                            std::get<StoredInstallationFacts>(f), "F", "N") ==
                            LifecycleWriteResult::staged,
                        "Actual mutation");
                t.commit();
                throw std::runtime_error("after actual commit");
            };
            success(r.submit(p), "Admitted changed work");
            require(
                QTest::qWaitFor([&] { return r.log->count(K::writer_completed) == 1; }, wait_ms),
                "Real completion");
            const auto w = r.log->last(K::writer_completed).work;
            require(w && w->transaction == TransactionOutcome::committed &&
                        w->error == WorkError::work_exception && !w->has_value,
                    "Committed truth despite missing normal envelope");
            require(r.instance->inspect().generation == 2 &&
                        r.store.facts() == SqlRows{{QString("49"), QString("46"), QString("4E")}},
                    "Acknowledged source candidate bound before next entry");
            require(r.dispatch->completions == 0 && r.transport->handoffs == 0,
                    "No normal publication reconstructed");
            r.stop();
        });
    }
    void startup_inspection_budget_refusal_still_serves_diagnostics() {
        scenario([] {
            Rig r("current", [](auto &o) { o.writer_limits.statements_per_batch = 1; });
            r.start(false);
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
    void publication_throw_revokes_facade_without_replay_HL07() {
        scenario([] {
            Rig r;
            r.start();
            auto p = r.peer();
            std::shared_ptr<CurrentSessionClock> clock;
            r.dispatch->completion = [&](auto &p, const auto &) -> host::OwnedReply {
                clock = p.clock();
                throw std::runtime_error("emitter refused");
            };
            success(r.submit(p), "Admit");
            require(QTest::qWaitFor([&] { return r.dispatch->completions == 1; }, wait_ms),
                    "Matched completion attempted once");
            require(std::holds_alternative<SessionSourceUnavailable>(clock->sample_current()),
                    "Exception revokes fresh publication facade");
            require(r.transport->handoffs == 0 && r.dispatch->executions == 1 &&
                        r.log->last(K::writer_completed).work->transaction ==
                            TransactionOutcome::committed,
                    "No rollback or replay");
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
            for (auto k :
                 {K::execution_entered, K::begin_attempted, K::precommit_read, K::writer_completed,
                  K::publication_entered, K::writer_removed, K::owner_cleanup})
                require(r.log->last(k).thread == owner,
                        "Actual SQL/source/facade/cleanup on Qt owner");
            require(r.log->last(K::local_handoff).thread == r.transport->io_thread,
                    "Actual handoff on IO");
        });
    }
};
QTEST_GUILESS_MAIN(ApiHostTests)
#include "api_host_tests.moc"
