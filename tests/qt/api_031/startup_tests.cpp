// SPDX-License-Identifier: GPL-3.0-or-later

#include "fixture.hpp"

namespace {
using namespace api_031_fixture;

void scenario(const auto &body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

const QString policies = "subathon_intake_policies";
const QString members = "subathon_intake_members";
const QString sources = "subathon_intake_sources";
const QString history = "subathon_intake_operations";

void expect_status(const StartupOutcome &outcome, IntakeStatus expected, const char *step) {
    require(outcome.admission == StorageError::none && outcome.work && outcome.result,
            QString("%1 completes with its real work and acknowledged result").arg(step));
    catalog::finance::normal(*outcome.work);
    require(outcome.result->status == expected, QString("%1 expected IntakeStatus %2, got %3")
                                                    .arg(step)
                                                    .arg(int(expected))
                                                    .arg(int(outcome.result->status)));
}

void expect_projection(const StartupOutcome &outcome, IntakeStatus status,
                       const StartupProjection &want, const char *step,
                       std::optional<StartupReadback> readback = {}) {
    expect_status(outcome, status, step);
    require(outcome.result->projection == want && outcome.result->readback == readback,
            QString("%1 publishes the exact owned and scoped durable closure").arg(step));
}

void expect_refusal(const StartupOutcome &outcome, IntakeStatus status,
                    std::optional<StartupReadback> readback = {}) {
    require(outcome.admission == StorageError::none && outcome.result,
            "Accepted refusal reports an explicit status");
    if (outcome.work)
        catalog::finance::normal(*outcome.work);
    require(outcome.result->status == status && !outcome.result->projection &&
                outcome.result->readback == readback,
            QString("Refusal projects no protected closure, expected %1 got %2")
                .arg(int(status))
                .arg(int(outcome.result->status)));
}

QVariant blob(const std::optional<QByteArray> &value) {
    return value ? QVariant(*value) : QVariant{};
}

void apply_policy(Image &image, const StartupProjection &revision) {
    image[policies].append(SqlRow{a.installation, revision.policy_head, revision.policy});
    for (const auto &m : revision.members)
        image[members].append(SqlRow{m.scope.installation, revision.policy_head, m.scope.provider,
                                     m.scope.account, m.scope.stream, m.award_class, m.effect_scope,
                                     m.overlap_set, m.primary ? QVariant(1) : QVariant{},
                                     m.document});
}
SqlRow source_row(const StartupSource &row) {
    return {row.scope.installation, row.scope.provider,        row.scope.account,
            row.scope.stream,       row.fence.epoch,           row.fence.generation,
            row.fence.revision,     row.fence.policy_revision, row.state,
            blob(row.cut),          blob(row.frontier),        row.cursor_contract};
}
SqlRow history_row(const StartupOperation &row) {
    return {row.scope.installation,
            row.key,
            row.scope.provider,
            row.scope.account,
            row.scope.stream,
            row.epoch,
            row.policy_revision,
            QVariant{},
            row.purpose,
            row.generation,
            row.revision,
            row.intent,
            QVariant{},
            1,
            0,
            row.outcome,
            blob(row.prior_cut),
            blob(row.cut)};
}

Image registered(Image before, bool two = false, bool disjoint = false) {
    const auto want = registration(two, disjoint);
    apply_policy(before, want);
    for (const auto &source : want.sources)
        before[sources].append(source_row(source));
    return before;
}
Image activated(Image before, bool two = false, bool disjoint = false) {
    const auto want = activation(two, disjoint);
    apply_policy(before, want);
    before[sources].clear();
    for (const auto &source : want.sources)
        before[sources].append(source_row(source));
    for (const auto &operation : want.operations)
        before[history].append(history_row(operation));
    return before;
}
Image connected_again(Image before) {
    before[sources][0] = source_row({a, reconnected, 1, k10, {}, cursor});
    before[history].append(history_row(r0));
    return before;
}
Image checkpointed(Image before, bool reconnect = false) {
    const auto want = checkpoint(reconnect);
    before[sources][0] = source_row(want.sources.front());
    // K0 sorts before R0 in the observer's operation key order.
    const auto offset = before[history].size() - (reconnect ? 1 : 0);
    before[history].insert(offset, history_row(want.operations.back()));
    return before;
}

Image register_ok(Fixture &fixture, Image before) {
    const auto outcome = fixture.submit(fixture.register_request());
    // This is the first intended red after successful migration, identity and
    // permission setup. The baseline stub acknowledges a real read-only turn.
    expect_projection(outcome, IntakeStatus::committed, registration(fixture.two, fixture.disjoint),
                      "register_first");
    auto after = registered(std::move(before), fixture.two, fixture.disjoint);
    same_image(fixture.store.image(), after);
    require(fixture.registry.first.cuts == 0 && fixture.registry.first.lives == 0 &&
                fixture.registry.second.cuts == 0 && fixture.registry.second.lives == 0,
            "Registration does not consume adapter proof");
    return after;
}
Image activate_ok(Fixture &fixture, Image before) {
    expect_projection(fixture.submit(fixture.activate_request()), IntakeStatus::committed,
                      activation(fixture.two, fixture.disjoint), "activate_first");
    auto after = activated(std::move(before), fixture.two, fixture.disjoint);
    same_image(fixture.store.image(), after);
    require(fixture.registry.first.cuts == 1 &&
                fixture.registry.second.cuts == (fixture.two ? 1 : 0) &&
                fixture.registry.first.lives == 0 && fixture.registry.first.pages == 0,
            "Activation observes exactly one cut per source, with no progress or page fetch");
    return after;
}
void expect_unchanged(Fixture &fixture, const Image &before, qint64 changes) {
    same_image(fixture.store.image(), before);
    require(fixture.store.changes() == changes, "Writer ran no row mutations");
}
void verify_absent(Fixture &fixture, StartupSelector selector, const Image &before) {
    const auto changes = fixture.store.changes();
    expect_refusal(fixture.inspect(std::move(selector)), IntakeStatus::stale,
                   StartupReadback::absent_at_before);
    expect_unchanged(fixture, before, changes);
}

enum class Fault { veto, lost_ack, post_commit };

template <class Request>
void verify_fault(Fixture &fixture, Request request, Fault fault, const Image &before,
                  const Image &after, const StartupProjection &projection) {
    StartupFaults hooks;
    const auto marker = std::make_shared<int>(0);
    switch (fault) {
    case Fault::veto:
        hooks.before_commit = [marker] {
            ++*marker;
            return false;
        };
        break;
    case Fault::lost_ack:
        hooks.lose_commit_ack = true;
        break;
    case Fault::post_commit:
        hooks.after_commit = [marker] {
            ++*marker;
            throw std::runtime_error("Synthetic exception after acknowledged commit");
        };
        break;
    }
    const auto outcome = fixture.submit(request, hooks);
    require(outcome.admission == StorageError::none && outcome.work && !outcome.work->value &&
                !outcome.result && *marker == (fault == Fault::lost_ack ? 0 : 1),
            "Fault exposes only the real decisive writer result, once");
    if (fault == Fault::veto) {
        require(outcome.work->transaction == TransactionOutcome::rolled_back &&
                    outcome.work->error == WorkError::not_committed,
                "Pre-COMMIT veto gives acknowledged rollback and no staged projection");
        same_image(fixture.store.image(), before);
        fixture.reopen();
        verify_absent(fixture, {request}, before);
        expect_projection(fixture.submit(request), IntakeStatus::committed, projection,
                          "retry after readback proved exact before-image");
    } else {
        if (fault == Fault::lost_ack) {
            require(outcome.work->transaction == TransactionOutcome::outcome_unknown &&
                        outcome.work->error == WorkError::uncertain_outcome,
                    "Lost commit acknowledgement carries the real uncertain outcome");
            const auto suffix_calls = std::make_shared<int>(0);
            require(fixture.store.writer().enqueue_work(
                        [](auto) -> QVariant { return {}; },
                        [suffix_calls](auto) { ++*suffix_calls; }) == StorageError::shutting_down &&
                        *suffix_calls == 0,
                    "Uncertain old writer refuses a suffix synchronously");
        } else {
            require(outcome.work->transaction == TransactionOutcome::committed &&
                        outcome.work->error == WorkError::work_exception,
                    "Post-COMMIT throw retains the acknowledged commit without a result");
        }
        fixture.reopen();
        same_image(fixture.store.image(), after);
        const auto changes = fixture.store.changes();
        fixture.authority.decision_suffix = "/reopened";
        expect_projection(fixture.inspect({request}), IntakeStatus::exact_replay, projection,
                          "authorized readback", StartupReadback::applied);
        expect_projection(fixture.submit(request), IntakeStatus::exact_replay, projection,
                          "exact retry preserves retained decisions");
        expect_unchanged(fixture, after, changes);
    }
    same_image(fixture.store.image(), after);
}

} // namespace

class Api031StartupTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void registerFreshThenActivateAndProveFirstLiveFrontier() {
        scenario([] {
            Fixture fixture;
            const auto empty = fixture.store.image();
            const auto enrolled = register_ok(fixture, empty);
            auto replay = [&](auto request, const StartupProjection &selected,
                              const Image &before) {
                const auto changes = fixture.store.changes();
                fixture.authority.decision_suffix = "/fresh";
                expect_projection(fixture.submit(request), IntakeStatus::exact_replay, selected,
                                  "exact retry retains the original decision");
                expect_unchanged(fixture, before, changes);
                fixture.authority.decision_suffix.clear();
            };
            replay(fixture.register_request(), registration(), enrolled);
            const auto started = activate_ok(fixture, enrolled);
            replay(fixture.activate_request(), activation(), started);
            expect_projection(fixture.submit(fixture.checkpoint_request()), IntakeStatus::committed,
                              checkpoint(), "checkpoint_first");
            require(fixture.registry.first.lives == 1,
                    "The first frontier uses the registered live transcript");
            const auto complete = checkpointed(started);
            same_image(fixture.store.image(), complete);
            replay(fixture.checkpoint_request(), checkpoint(), complete);
            const auto changes = fixture.store.changes();
            expect_refusal(fixture.submit(fixture.activate_request()), IntakeStatus::stale);
            expect_refusal(fixture.inspect({fixture.activate_request()}), IntakeStatus::stale,
                           StartupReadback::superseded);
            expect_unchanged(fixture, complete, changes);
        });
    }

    void fullTrustedCohortWinsOverOmittedScope() {
        scenario([] {
            for (bool allow_b : {false, true}) {
                Fixture fixture(true, false, allow_b);
                auto incomplete = fixture.register_request();
                incomplete.cohort.removeLast();
                incomplete.members.removeLast();
                incomplete.enrollment.removeLast();
                const auto before = fixture.store.image();
                const auto changes = fixture.store.changes();
                expect_refusal(fixture.submit(incomplete),
                               allow_b ? IntakeStatus::invalid_input : IntakeStatus::denied);
                expect_unchanged(fixture, before, changes);
                require(fixture.registry.first.cuts == 0 && fixture.registry.second.cuts == 0,
                        "Omitted-scope refusal consumes no first-cut proof");
            }
        });
    }

    void currentSessionGatesBeforeEnrollmentEcho() {
        scenario([] {
            Fixture fixture;
            auto bad = fixture.register_request();
            bad.enrollment.front().generation = "caller-invented";
            const auto before = fixture.store.image();
            const auto changes = fixture.store.changes();
            fixture.authority.clock.now = 301001;
            expect_refusal(fixture.submit(bad), IntakeStatus::denied);
            fixture.authority.clock.now.reset();
            expect_refusal(fixture.submit(bad), IntakeStatus::unavailable);
            fixture.authority.clock.now = 301000;
            expect_refusal(fixture.submit(bad), IntakeStatus::invalid_input);
            expect_unchanged(fixture, before, changes);
        });
    }

    void disjointCohortActivatesTogether() {
        scenario([] {
            Fixture fixture(true, true);
            const auto enrolled = register_ok(fixture, fixture.store.image());
            fixture.store.work([&](const auto &s) -> QVariant {
                auto dec = [&](const Scope &scope) {
                    return std::get<StartupPermit>(
                               fixture.authority.resolve(
                                   s, {StartupAction::activate_first, session_lookup, scope, true}))
                        .decision;
                };
                const SourceChange incompatible{a,
                                                pending,
                                                "g2",
                                                policy("operator/D1"),
                                                registration(true, true).members,
                                                k10,
                                                "B0",
                                                dec(a),
                                                cursor,
                                                "cut10",
                                                SourcePurpose::baseline,
                                                1,
                                                {{b, pending_b, "gB", kb10, "BB0", dec(b), cursor,
                                                  "cutB10", SourcePurpose::baseline}}};
                require(stage_source_change(s, incompatible) == HistoryStatus::conflict,
                        "Protected source change still uses its narrower affected closure");
                return true;
            });
            same_image(fixture.store.image(), enrolled);
            fixture.registry.second.cut_available = false;
            const auto changes = fixture.store.changes();
            expect_refusal(fixture.submit(fixture.activate_request()), IntakeStatus::unproved);
            require(fixture.registry.second.cuts == 1, "Failure reaches B's installed cut adapter");
            expect_unchanged(fixture, enrolled, changes);

            fixture.registry.second.cut_available = true;
            expect_projection(fixture.submit(fixture.activate_request()), IntakeStatus::committed,
                              activation(true, true), "activate full disjoint cohort");
            same_image(fixture.store.image(), activated(enrolled, true, true));
        });
    }

    void contradictoryOrUnwitnessedProgressDoesNotSetFrontier() {
        scenario([] {
            Fixture fixture;
            auto before = register_ok(fixture, fixture.store.image());
            before = activate_ok(fixture, before);
            const auto changes = fixture.store.changes();
            auto request = fixture.checkpoint_request();
            request.candidate = live_witness(false, "12");
            expect_refusal(fixture.submit(request), IntakeStatus::invalid_input);
            require(fixture.registry.first.lives == 1,
                    "Contradictory range is tested against the actual 11 observation");
            expect_unchanged(fixture, before, changes);
            fixture.registry.first.live_fact.reset();
            request.candidate = live_witness();
            expect_refusal(fixture.submit(request), IntakeStatus::unproved);
            expect_unchanged(fixture, before, changes);
        });
    }

    void reconnectNeedsCurrentFenceExactChainAndFreshLiveProof() {
        scenario([] {
            Fixture fixture;
            auto before = register_ok(fixture, fixture.store.image());
            before = activate_ok(fixture, before);
            fixture.stage_protected_reconnect();
            before = connected_again(before);
            same_image(fixture.store.image(), before);
            const auto changes = fixture.store.changes();

            auto request = fixture.checkpoint_request(true);
            auto stale = request;
            stale.expected = active;
            expect_refusal(fixture.submit(stale), IntakeStatus::stale);
            auto missing = request;
            missing.reconnect_refs.clear();
            expect_refusal(fixture.submit(missing), IntakeStatus::unproved);
            require(fixture.registry.reconnect.lives == 0,
                    "Old fence and invalid chain stop before live evidence");

            fixture.registry.reconnect.live_fact.reset();
            expect_refusal(fixture.submit(request), IntakeStatus::unproved);
            expect_unchanged(fixture, before, changes);

            fixture.registry.reconnect.live_fact = live_witness(true);
            expect_projection(fixture.submit(request), IntakeStatus::committed, checkpoint(true),
                              "g3 live prefix joins retained g2 CUT1 through RC1");
            same_image(fixture.store.image(), checkpointed(before, true));
        });
    }

    void inspectIsRequiredForMutatingReplay() {
        scenario([] {
            Fixture fixture;
            auto before = register_ok(fixture, fixture.store.image());
            before = activate_ok(fixture, before);
            expect_projection(fixture.submit(fixture.checkpoint_request()), IntakeStatus::committed,
                              checkpoint(), "checkpoint_first");
            fixture.remove_inspect_grant();
            const auto without_inspect = fixture.store.image();
            const auto changes = fixture.store.changes();
            expect_refusal(fixture.submit(fixture.checkpoint_request()), IntakeStatus::denied);
            expect_refusal(fixture.inspect({fixture.checkpoint_request()}), IntakeStatus::denied);
            expect_unchanged(fixture, without_inspect, changes);
        });
    }

    void mutationFaults_data() {
        QTest::addColumn<int>("operation");
        QTest::addColumn<int>("fault");
        for (int action = 0; action < 3; ++action)
            for (int fault = 0; fault < 3; ++fault) {
                const QByteArray row =
                    QList<QByteArray>{"register", "activate", "checkpoint"}[action] + "-" +
                    QList<QByteArray>{"veto", "lost-commit-ack", "post-commit-throw"}[fault];
                QTest::newRow(row.constData()) << action << fault;
            }
    }
    void mutationFaults() {
        QFETCH(int, operation);
        QFETCH(int, fault);
        scenario([&] {
            // The activation fault cases protect both writes in a disjoint cohort.
            Fixture fixture(operation == 1, operation == 1);
            const auto empty = fixture.store.image();
            const auto kind = static_cast<Fault>(fault);
            if (operation == 0) {
                verify_fault(fixture, fixture.register_request(), kind, empty, registered(empty),
                             registration());
                return;
            }
            const auto enrolled = register_ok(fixture, empty);
            if (operation == 1) {
                verify_fault(fixture, fixture.activate_request(), kind, enrolled,
                             activated(enrolled, true, true), activation(true, true));
                return;
            }
            const auto activated_image = activate_ok(fixture, enrolled);
            verify_fault(fixture, fixture.checkpoint_request(), kind, activated_image,
                         checkpointed(activated_image), checkpoint());
        });
    }
};

QTEST_GUILESS_MAIN(Api031StartupTests)
#include "startup_tests.moc"
