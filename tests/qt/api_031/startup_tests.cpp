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
    const auto &got = outcome.result->projection;
    require(got && got->policy_head == want.policy_head && got->policy == want.policy &&
                std::ranges::is_permutation(got->members, want.members) &&
                std::ranges::is_permutation(got->sources, want.sources) &&
                std::ranges::is_permutation(got->operations, want.operations) &&
                outcome.result->readback == readback,
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

const QString entries = "subathon_intake_entries";

SqlRow hold_row(const QuarantineEntry &entry, std::optional<qint64> charge) {
    return {entry.scope.installation,
            entry.scope.provider,
            entry.scope.account,
            entry.scope.stream,
            entry.key,
            entry.expected_source.epoch,
            entry.expected_source.generation,
            entry.expected_source.policy_revision,
            {},
            {},
            {},
            entry.normalized,
            entry.provenance,
            entry.arrival_ms,
            {},
            {},
            {},
            {},
            {},
            charge ? 2 : 3,
            charge ? 1 : 2,
            charge ? QVariant::fromValue(*charge) : QVariant{},
            {},
            {},
            {}};
}

LiveDispositionClaim claim(const QuarantineEntry &entry, qint64 charge) {
    return {entry.scope,
            entry.expected_source.generation,
            entry.key,
            entry.normalized,
            entry.provenance,
            entry.arrival_ms,
            {},
            2,
            1,
            charge,
            {}};
}

void stage_holds(Fixture &fixture, const QList<QuarantineEntry> &holds) {
    for (const auto &hold : holds) {
        fixture.store.work([&](const auto &session) -> QVariant {
            const StartupExecutionAccount actor{"install", "operator"};
            require(std::holds_alternative<StartupPermit>(fixture.authority.resolve(
                        session, {StartupAction::checkpoint_first, actor, hold.scope, false})),
                    "The protected fixture writer uses a current scoped execution grant");
            require(stage_quarantine(session, hold) == QuarantineStatus::staged,
                    "Protected quarantine stages the independently observed input");
            return true;
        });
    }
}

QuarantineEntry observe_q11() {
    ConnectorEventObservation observation{};
    observation.facts.event_type = "individual-support";
    observation.facts.schema_version = 1;
    const auto event = describe_connector_event({"P", "A"}, {8000, "g2", {}, {}}, observation);
    const LiveItemEnvelope envelope{{a, active, profile}, "11", event, "u11"};
    require(envelope.position == "11" && envelope.event.identifiers.empty() &&
                envelope.event.evidence == core::PaymentEvidence::unknown,
            "The independent live envelope contains the unidentified in-range u11 item");
    return {envelope.selection.scope,
            "Q11",
            envelope.normalized,
            "unidentified",
            envelope.event.arrival.server_timestamp_utc_ms,
            envelope.selection.fence};
}

StartupProjection progress_from(StartupProjection projection, const Fence &before,
                                QByteArray evidence, QByteArray upper = "11") {
    auto after = before;
    ++after.revision;
    projection.sources.front().fence = after;
    projection.sources.front().frontier = upper;
    projection.operations.append(operation(a, before, after, "K0", "operator/D2",
                                           covered(evidence, upper), k10, k10, 2, upper));
    return projection;
}

Image exact_progress_image(Image before, const StartupProjection &projection) {
    before[sources][0] = source_row(projection.sources.front());
    const auto &op = projection.operations.back();
    auto &rows = before[history];
    rows.insert(std::ranges::find_if(
                    rows, [&](const SqlRow &row) { return row[1].toByteArray() > op.key; }),
                history_row(op));
    return before;
}

template <class Request>
StartupOutcome between_preparation_and_write(Fixture &fixture, const Request &request,
                                             const auto &between) {
    std::exception_ptr failure;
    int pauses = 0;
    StartupFaults hooks;
    hooks.on_prepared = [&](StartupFaults::Resume resume) {
        ++pauses;
        try {
            require(QThread::currentThread() == QCoreApplication::instance()->thread(),
                    "Preparation barrier runs on the Qt owner thread");
            between();
        } catch (...) {
            failure = std::current_exception();
        }
        resume();
    };
    auto result = fixture.submit(request, hooks);
    if (failure)
        std::rethrow_exception(failure);
    require(pauses == 1, "The prepared attempt reaches the owner barrier exactly once");
    return result;
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
        const int live_observations = fixture.registry.first.lives;
        if constexpr (std::is_same_v<Request, CheckpointRequest>)
            require(!request.candidate && !request.qualified,
                    "Checkpoint recovery starts with only the original key and before-image");
        expect_projection(fixture.inspect({request}), IntakeStatus::exact_replay, projection,
                          "authorized readback", StartupReadback::applied);
        if constexpr (std::is_same_v<Request, CheckpointRequest>)
            require(fixture.registry.first.lives == live_observations,
                    "Readback verifies retained evidence without a fresh live observation");
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
            const auto calls = fixture.registry.calls();
            expect_refusal(fixture.submit(bad), IntakeStatus::unavailable);
            require(fixture.registry.calls() == calls,
                    "Clock unavailability stops before adapter observation or attestation");
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
            same_image(fixture.store.image(), checkpointed(before));
            fixture.remove_inspect_grant();
            fixture.store.work([&](const auto &session) -> QVariant {
                for (const auto &scope : {a, b}) {
                    require(
                        std::holds_alternative<StartupPermit>(
                            fixture.authority.resolve(session, {StartupAction::checkpoint_first,
                                                                session_lookup, scope, true})) &&
                            std::holds_alternative<StartupDenied>(fixture.authority.resolve(
                                session, {StartupAction::inspect, session_lookup, scope, true})),
                        "Revoking Inspect keeps the original mutating grants for A and B");
                }
                return true;
            });
            const auto calls = fixture.registry.calls();
            const auto without_inspect = fixture.store.image();
            const auto changes = fixture.store.changes();
            expect_refusal(fixture.submit(fixture.checkpoint_request()), IntakeStatus::denied);
            expect_refusal(fixture.inspect({fixture.checkpoint_request()}), IntakeStatus::denied);
            expect_unchanged(fixture, without_inspect, changes);
            require(fixture.registry.calls() == calls,
                    "Inspect denial prevents observation and retained evidence comparison");
        });
    }

    void missingPrimaryAndChangedApprovedRegistrationConfiguration() {
        scenario([] {
            Fixture fixture;
            auto &primary = fixture.configuration.lanes.front().member.primary;
            primary = false;
            const auto empty = fixture.store.image();
            const auto empty_changes = fixture.store.changes();
            const auto calls = fixture.registry.calls();
            expect_refusal(fixture.submit(fixture.register_request()), IntakeStatus::invalid_input);
            expect_unchanged(fixture, empty, empty_changes);
            require(fixture.registry.calls() == calls, "Missing primary observes no adapter");

            primary = true;
            const auto enrolled = register_ok(fixture, empty);
            const auto changes = fixture.store.changes();
            fixture.configuration.revision = "cfg-2";
            expect_refusal(fixture.submit(fixture.register_request()), IntakeStatus::conflict);
            expect_unchanged(fixture, enrolled, changes);
            require(fixture.registry.calls() == calls,
                    "Registration conflicts without observation");
        });
    }

    void activationModeAndChangedCheckpointConfiguration() {
        scenario([] {
            Fixture fixture;
            const auto enrolled = register_ok(fixture, fixture.store.image());
            auto request = fixture.activate_request();
            request.mode = CatchUpMode::automatic;
            auto calls = fixture.registry.calls();
            const auto enrolled_changes = fixture.store.changes();
            expect_refusal(fixture.submit(request), IntakeStatus::invalid_input);
            expect_unchanged(fixture, enrolled, enrolled_changes);
            require(fixture.registry.calls() == calls, "Conflicting mode echo observes no cut");

            const auto started = activate_ok(fixture, enrolled);
            const auto changes = fixture.store.changes();
            fixture.configuration.mode = CatchUpMode::automatic;
            expect_refusal(fixture.submit(request), IntakeStatus::conflict);
            expect_unchanged(fixture, started, changes);

            fixture.configuration.mode = CatchUpMode::review;
            fixture.configuration.revision = "cfg-2";
            auto checkpoint = fixture.checkpoint_request();
            checkpoint.configuration_revision = "cfg-2";
            calls = fixture.registry.calls();
            expect_refusal(fixture.submit(checkpoint), IntakeStatus::stale);
            require(
                fixture.registry.calls() == calls,
                "Currently allowed action with changed configuration consumes no live evidence");
            expect_unchanged(fixture, started, changes);
        });
    }

    void activationAuthorityIsRecheckedAfterObservation() {
        scenario([] {
            Fixture fixture(true, false);
            const auto enrolled = register_ok(fixture, fixture.store.image());
            Image revoked;
            qint64 changes{};
            const auto outcome =
                between_preparation_and_write(fixture, fixture.activate_request(), [&] {
                    require(fixture.registry.first.cuts == 1 && fixture.registry.second.cuts == 1,
                            "Both cuts have been observed before the independent revocation");
                    same_image(fixture.store.image(), enrolled);
                    fixture.remove_activation_grant();
                    changes = fixture.store.changes();
                    revoked = fixture.store.image();
                    for (const auto &object : frozen::delta_objects)
                        if (object.type == "table")
                            require(revoked[object.name] == enrolled[object.name],
                                    "Revocation alone leaves all nine intake tables unchanged");
                });
            expect_refusal(outcome, IntakeStatus::denied);
            expect_unchanged(fixture, revoked, changes);
        });
    }

    void cutEchoUnsupportedAndRetiredHandle() {
        scenario([] {
            Fixture fixture;
            const auto before = register_ok(fixture, fixture.store.image());
            const auto changes = fixture.store.changes();
            auto request = fixture.activate_request();
            request.sources.front().candidate = cut_witness();
            request.sources.front().candidate->cut.position = QByteArray("12");
            expect_refusal(fixture.submit(request), IntakeStatus::invalid_input);
            require(fixture.registry.first.cuts == 1, "Echo is compared with an actual cut at 10");
            expect_unchanged(fixture, before, changes);

            fixture.registry.first.cut_failure = HistoryFailure::unsupported;
            request.sources.front().candidate.reset();
            expect_refusal(fixture.submit(request), IntakeStatus::unsupported);
            require(fixture.registry.first.cuts == 2, "Unsupported comes from the registered port");
            expect_unchanged(fixture, before, changes);

            fixture.registry.first.cut_failure.reset();
            expect_refusal(between_preparation_and_write(
                               fixture, request,
                               [&] {
                                   require(fixture.registry.first.cuts == 3,
                                           "The cut was observed under the prior registry binding");
                                   fixture.registry.retire_first();
                               }),
                           IntakeStatus::stale);
            expect_unchanged(fixture, before, changes);
        });
    }

    void liveRefusalsKeepFrontierNull() {
        scenario([] {
            Fixture fixture;
            const auto before = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            const auto changes = fixture.store.changes();
            auto request = fixture.checkpoint_request();
            auto &port = fixture.registry.first;
            auto refuse = [&](IntakeStatus status) {
                const auto observations = port.lives;
                expect_refusal(fixture.submit(request), status);
                require(port.lives == observations + 1 && port.pages == 0,
                        "Refusal consumes one live observation and no history pages");
                expect_unchanged(fixture, before, changes);
            };

            auto bare = live_witness();
            bare.proof.clear();
            port.live_fact.reset();
            request.candidate = bare;
            refuse(IntakeStatus::unproved);

            request.candidate.reset();
            auto lost = live_witness();
            lost.coverage = Coverage::lost;
            lost.proof = "lost-11";
            port.offer_live(lost);
            refuse(IntakeStatus::unproved);

            for (const auto &upper : {std::optional<QByteArray>{}, std::optional{QByteArray("")}}) {
                auto witness = live_witness();
                witness.upper.position = upper;
                witness.proof = upper ? "present-empty-upper" : "absent-upper";
                port.offer_live(witness);
                refuse(IntakeStatus::unproved);
            }

            // A real observed u11 has no committed disposition at this point.
            auto unsettled = live_witness();
            unsettled.ordered_inputs.append(observe_q11().normalized);
            unsettled.proof = "live-unsettled-11";
            port.offer_live(unsettled);
            refuse(IntakeStatus::unproved);

            port.live_failure = HistoryFailure::unsupported;
            refuse(IntakeStatus::unsupported);
            port.live_failure = HistoryFailure::temporarily_unavailable;
            refuse(IntakeStatus::unavailable);
        });
    }

    void absentAndPresentEmptyCutPositionsCannotSetLiveFrontier() {
        scenario([] {
            for (const auto &lower : {std::optional<QByteArray>{}, std::optional{QByteArray("")}}) {
                Fixture fixture;
                const auto enrolled = register_ok(fixture, fixture.store.image());
                fixture.registry.first.cut_fact.cut.position = lower;
                const auto cut_bytes = cut("g2", "cut10", lower);
                auto selected = activation();
                selected.sources.front().cut = cut_bytes;
                selected.operations.front().cut = cut_bytes;
                expect_projection(fixture.submit(fixture.activate_request()),
                                  IntakeStatus::committed, selected,
                                  "activate with an optional boundary position");
                auto started = activated(enrolled);
                started[sources][0] = source_row(selected.sources.front());
                started[history][0] = history_row(selected.operations.front());
                same_image(fixture.store.image(), started);

                auto witness = live_witness();
                witness.request.activation = selected.operations.front();
                witness.lower.position = lower;
                witness.proof = lower ? "present-empty-lower" : "absent-lower";
                fixture.registry.first.offer_live(witness);
                const auto changes = fixture.store.changes();
                expect_refusal(fixture.submit(fixture.checkpoint_request()),
                               IntakeStatus::unproved);
                expect_unchanged(fixture, started, changes);
            }
        });
    }

    void mirrorCohortPreservesSecondSourceAcrossCheckpoint() {
        scenario([] {
            Fixture fixture(true);
            const auto started = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            const auto selected = checkpoint();
            expect_projection(fixture.submit(fixture.checkpoint_request()), IntakeStatus::committed,
                              selected, "checkpoint selects only A in the mirror cohort");
            same_image(fixture.store.image(), exact_progress_image(started, selected));
            require(fixture.registry.second.lives == 0 && fixture.registry.second.pages == 0,
                    "Only the A port participates in checkpoint");
        });
    }

    void interveningHoldMustBeCoveredAndResolutionPreservesReplay() {
        scenario([] {
            Fixture fixture;
            const auto started = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            const auto hold = observe_q11();
            constexpr qint64 q11_charge = 9 + 3 + 12;
            auto held = started;
            held[entries].append(hold_row(hold, q11_charge));
            qint64 held_changes{};
            const auto request = fixture.checkpoint_request();
            auto &port = fixture.registry.first;
            auto unsettled = live_witness();
            unsettled.ordered_inputs.append(hold.normalized);
            unsettled.proof = "live-unsettled-11";
            port.offer_live(unsettled);
            const auto initial_changes = fixture.store.changes();
            expect_refusal(fixture.submit(request), IntakeStatus::unproved);
            expect_unchanged(fixture, started, initial_changes);

            port.offer_live(live_witness());
            expect_refusal(between_preparation_and_write(
                               fixture, request,
                               [&] {
                                   require(port.lives == 2,
                                           "Old candidate witnessed an empty prefix");
                                   same_image(fixture.store.image(), started);
                                   stage_holds(fixture, {hold});
                                   same_image(fixture.store.image(), held);
                                   held_changes = fixture.store.changes();
                               }),
                           IntakeStatus::unproved);
            expect_unchanged(fixture, held, held_changes);

            auto witness = live_witness();
            witness.ordered_inputs.append(hold.normalized);
            witness.dispositions.append(claim(hold, q11_charge));
            witness.proof = "live-held-11";
            port.offer_live(witness);
            const auto selected = progress_from(activation(), active, witness.proof);
            require(covered(witness.proof).size() == 61 &&
                        selected.operations.back().intent.size() == 222,
                    "Exact held CV1 and SH1 lengths");
            expect_projection(fixture.submit(request), IntakeStatus::committed, selected,
                              "fresh witness accounts for the exact intervening hold");
            auto after = exact_progress_image(held, selected);
            same_image(fixture.store.image(), after);

            auto resolution = hold;
            resolution.expected_source = progressed;
            fixture.store.work([&](const auto &session) -> QVariant {
                const auto decision = fixture.authority.resolve(
                    session, {StartupAction::checkpoint_first, session_lookup, a, true});
                const auto *permit = std::get_if<StartupPermit>(&decision);
                require(permit &&
                            stage_quarantine_resolution(session, resolution, permit->decision) ==
                                QuarantineStatus::staged,
                        "Protected writer resolves the exact original Q11 under fresh authority");
                return true;
            });
            after[entries][0] = hold_row(hold, {});
            same_image(fixture.store.image(), after);
            const auto changes = fixture.store.changes();
            fixture.authority.decision_suffix = "/after-resolution";
            expect_projection(fixture.inspect({request}), IntakeStatus::exact_replay, selected,
                              "inspect retains the original held K0", StartupReadback::applied);
            expect_projection(fixture.submit(request), IntakeStatus::exact_replay, selected,
                              "retry retains the original held K0");
            expect_unchanged(fixture, after, changes);
        });
    }

    void reconnectRejectsOldCallbackAndRequiresItsEarlierGenerationHold() {
        scenario([] {
            Fixture fixture;
            const auto started = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            const auto hold = observe_q11();
            stage_holds(fixture, {hold});
            auto held = started;
            constexpr qint64 q11_charge = 9 + 3 + 12;
            held[entries].append(hold_row(hold, q11_charge));
            same_image(fixture.store.image(), held);

            // Use an exact g2 witness so preparation reaches the owner barrier.
            auto g2 = live_witness();
            g2.ordered_inputs.append(hold.normalized);
            g2.dispositions.append(claim(hold, q11_charge));
            g2.proof = "live-held-11";
            fixture.registry.first.offer_live(g2);
            auto reconnected_image = connected_again(held);
            qint64 changes{};
            expect_refusal(between_preparation_and_write(
                               fixture, fixture.checkpoint_request(),
                               [&] {
                                   require(fixture.registry.first.lives == 1,
                                           "g2 produced the paused callback");
                                   fixture.stage_protected_reconnect();
                                   same_image(fixture.store.image(), reconnected_image);
                                   changes = fixture.store.changes();
                               }),
                           IntakeStatus::stale);
            expect_unchanged(fixture, reconnected_image, changes);
            require(fixture.registry.reconnect.lives == 0,
                    "The retired g2 callback does not invoke g3 live observation");

            const auto request = fixture.checkpoint_request(true);
            expect_refusal(fixture.submit(request), IntakeStatus::unproved);
            expect_unchanged(fixture, reconnected_image, changes);
            auto g3 = live_witness(true);
            g3.ordered_inputs.append(hold.normalized);
            g3.dispositions.append(claim(hold, q11_charge));
            g3.proof = "live-held-r1-11";
            fixture.registry.reconnect.offer_live(g3);
            auto predecessor = activation();
            predecessor.operations.append(r0);
            const auto selected = progress_from(predecessor, reconnected, g3.proof);
            require(covered(g3.proof).size() == 64 &&
                        selected.operations.back().intent.size() == 225,
                    "Exact reconnect held CV1 and SH1 lengths");
            expect_projection(fixture.submit(request), IntakeStatus::committed, selected,
                              "current registry binds exact g2 hold through R0");
            same_image(fixture.store.image(), exact_progress_image(reconnected_image, selected));
        });
    }

    void sameBeforeHeadQualifiedIntentsConflictAndOldFenceNewKeyIsStale() {
        scenario([] {
            Fixture fixture;
            const auto before = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            const auto after = checkpointed(before);
            const auto request = fixture.checkpoint_request();
            auto &port = fixture.registry.first;
            port.offer_live(live_witness(false, "12"));
            qint64 changes{};
            expect_refusal(
                between_preparation_and_write(
                    fixture, request,
                    [&] {
                        require(port.lives == 1, "The first prepared intent observed 12");
                        same_image(fixture.store.image(), before);
                        port.offer_live(live_witness());
                        expect_projection(fixture.submit(request), IntakeStatus::committed,
                                          checkpoint(), "same before-head commits 11");
                        require(port.lives == 2, "Both intents came from actual invocations");
                        same_image(fixture.store.image(), after);
                        changes = fixture.store.changes();
                    }),
                IntakeStatus::conflict);
            expect_unchanged(fixture, after, changes);

            auto stale = request;
            stale.checkpoint_ref = "K-new";
            const auto calls = fixture.registry.calls();
            expect_refusal(fixture.submit(stale), IntakeStatus::stale);
            require(fixture.registry.calls() == calls,
                    "A stale fence stops before proof comparison");
            expect_unchanged(fixture, after, changes);
        });
    }

    void preliminaryExceptionCompletesWithoutConsumingDecisiveFault() {
        scenario([] {
            Fixture fixture;
            const auto before = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            const auto changes = fixture.store.changes();
            const auto calls = fixture.registry.calls();
            int attempts{}, decisive_hooks{}, barriers{};
            fixture.authority.before_resolve = [&](const SqliteWorkSession &session,
                                                   StartupAction action) {
                require(action == StartupAction::checkpoint_first &&
                            session.execute({"SELECT 1", {}}).front().front().toInt() == 1,
                        "Throw from the preliminary authorization inside an active transaction");
                ++attempts;
                throw std::runtime_error("Synthetic preliminary work exception");
            };
            StartupFaults hooks;
            hooks.lose_commit_ack = true;
            hooks.before_commit = [&] {
                ++decisive_hooks;
                return true;
            };
            hooks.after_commit = [&] { ++decisive_hooks; };
            hooks.on_prepared = [&](auto resume) {
                ++barriers;
                resume();
            };
            const auto outcome = fixture.submit(fixture.checkpoint_request(), hooks);
            fixture.authority.before_resolve = {};
            require(outcome.admission == StorageError::none && outcome.work &&
                        outcome.work->transaction == TransactionOutcome::rolled_back &&
                        outcome.work->error == WorkError::work_exception && !outcome.work->value &&
                        !outcome.result && attempts == 1 && decisive_hooks == 0 && barriers == 0,
                    "Only the failed preliminary turn completes, without a value or decisive turn");
            require(fixture.registry.calls() == calls, "Preliminary exception consumes no proof");
            // changes() itself commits a real owner turn, so a stray armed fault would fail here.
            expect_unchanged(fixture, before, changes);
        });
    }

    void completeCheckpointDocumentLimitIsInclusive() {
        scenario([] {
            Fixture fixture;
            const auto before = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
            auto witness = live_witness();
            witness.proof = QByteArray(16'175, 'v');
            auto &port = fixture.registry.first;
            port.offer_live(witness);
            const auto excess = progress_from(activation(), active, witness.proof);
            require(covered(witness.proof).size() == 16'224 &&
                        excess.operations.back().intent.size() == 16'385,
                    "One extra evidence byte exceeds only the enclosing SH1 bound");
            const auto changes = fixture.store.changes();
            expect_refusal(fixture.submit(fixture.checkpoint_request()),
                           IntakeStatus::limit_exceeded);
            expect_unchanged(fixture, before, changes);

            witness.proof.chop(1);
            port.offer_live(witness);
            const auto selected = progress_from(activation(), active, witness.proof);
            require(covered(witness.proof).size() == 16'223 &&
                        selected.operations.back().intent.size() == 16'384,
                    "Exact inclusive CV1 and SH1 lengths");
            expect_projection(fixture.submit(fixture.checkpoint_request()), IntakeStatus::committed,
                              selected, "16,384-byte SH1 is admissible");
            same_image(fixture.store.image(), exact_progress_image(before, selected));
        });
    }

    void registrationCohortLimitIsInclusive() {
        scenario([] {
            Fixture fixture(false, false, true, true);
            // First install and verify with the exact #761 limits. Reopen under a
            // bounded acceptance profile sized for 257 scoped checks and 513 writes.
            auto cohort_limits = work_limits;
            cohort_limits.statements_per_batch = 32'768;
            cohort_limits.bound_value_bytes = 4'194'304;
            cohort_limits.result_value_bytes = 67'108'864;
            fixture.store.reopen(cohort_limits);
            auto selected = registration();
            for (int i = 1; i < 257; ++i) {
                const auto ref = QByteArray::number(1000 + i);
                const Scope scope{"install", "P", "source-" + ref, "S"};
                const auto lane_ref = "lane-" + ref;
                const PolicyMember compact{scope,
                                           "individual",
                                           "effect-" + ref,
                                           "overlap-" + ref,
                                           true,
                                           f(lane_ref) + f("operator") + f("history-v1") +
                                               f("cursor-contract")};
                fixture.configuration.lanes.append(
                    {compact, lane_ref, "g0", "g2", profile, cursor, true});
                if (i < 256) {
                    selected.members.append(compact);
                    selected.sources.append({scope, pending, 0, {}, {}, cursor});
                }
            }
            const auto before = fixture.store.image();
            const auto changes = fixture.store.changes();
            const auto calls = fixture.registry.calls();
            expect_refusal(fixture.submit(fixture.register_request()),
                           IntakeStatus::limit_exceeded);
            expect_unchanged(fixture, before, changes);
            fixture.configuration.lanes.removeLast();
            expect_projection(fixture.submit(fixture.register_request()), IntakeStatus::committed,
                              selected, "256 compact members fit the bounded writer profile");
            auto after = before;
            apply_policy(after, selected);
            for (const auto &source : selected.sources)
                after[sources].append(source_row(source));
            same_image(fixture.store.image(), after);
            require(fixture.registry.calls() == calls,
                    "Cohort bound requires no proof observation");
        });
    }

    void reconnectKeyLimitIsInclusive() {
        scenario([] {
            for (const int keys : {16, 17}) {
                Fixture fixture;
                auto before = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
                auto predecessor = activation();
                auto fence = active;
                auto request = fixture.checkpoint_request();
                for (int i = 0; i < keys; ++i) {
                    const auto suffix = QByteArray::number(10 + i);
                    const auto key = "R" + suffix;
                    const auto generation = "g" + suffix;
                    fixture.stage_protected_reconnect(fence, generation, key);
                    const Fence after{2, fence.revision + 1, 2, generation};
                    const auto link =
                        operation(a, fence, after, key, "operator/D3", "connected", k10, k10, 4);
                    predecessor.operations.append(link);
                    before[history].append(history_row(link));
                    request.reconnect_refs.append(key);
                    fence = after;
                }
                predecessor.sources.front().fence = fence;
                before[sources][0] = source_row(predecessor.sources.front());
                same_image(fixture.store.image(), before);

                request.expected = fence;
                fixture.registry.use_reconnect(fence.generation);
                auto witness = live_witness(true);
                witness.request.fence = fence;
                witness.request.reconnects = predecessor.operations.mid(1);
                witness.proof = "bounded-live-chain";
                fixture.registry.reconnect.offer_live(witness);
                if (keys == 17) {
                    const auto changes = fixture.store.changes();
                    const auto calls = fixture.registry.calls();
                    expect_refusal(fixture.submit(request), IntakeStatus::limit_exceeded);
                    expect_unchanged(fixture, before, changes);
                    require(fixture.registry.calls() == calls, "17 keys stop before live evidence");
                } else {
                    const auto selected = progress_from(predecessor, fence, witness.proof);
                    expect_projection(fixture.submit(request), IntakeStatus::committed, selected,
                                      "16 connected RC1 keys qualify the live prefix");
                    same_image(fixture.store.image(), exact_progress_image(before, selected));
                }
            }
        });
    }

    void selectedIntakeRowsAndPayloadUseCapPlusOne() {
        scenario([] {
            // The 17-row case stays well below the byte bound. The byte case
            // has just 16 rows, with independently sized input/provenance.
            struct BudgetCase {
                int rows;
                int payload_per_row;
                int extra_byte;
                bool fits;
            };
            for (const auto [rows, payload_per_row, extra_byte, fits] :
                 {BudgetCase{16, 32, 0, true}, BudgetCase{17, 32, 0, false},
                  BudgetCase{16, 16'384, 1, false}}) {
                Fixture fixture;
                auto before = activate_ok(fixture, register_ok(fixture, fixture.store.image()));
                QList<QuarantineEntry> holds;
                auto witness = live_witness();
                witness.proof = "bounded-live-intake";
                for (int i = 0; i < rows; ++i) {
                    const auto suffix = QByteArray::number(10 + i);
                    const auto payload = payload_per_row + (i == 0 ? extra_byte : 0);
                    const auto input = suffix + QByteArray(payload - 12 - suffix.size(), 'i');
                    const QuarantineEntry hold{a,        "Q" + suffix, input, "unidentified",
                                               8000 + i, active};
                    const qint64 charge = payload + 12;
                    holds.append(hold);
                    witness.ordered_inputs.append(hold.normalized);
                    witness.dispositions.append(claim(hold, charge));
                    before[entries].append(hold_row(hold, charge));
                }
                stage_holds(fixture, holds);
                same_image(fixture.store.image(), before);
                fixture.registry.first.offer_live(witness);
                const auto changes = fixture.store.changes();
                if (fits) {
                    const auto selected = progress_from(activation(), active, witness.proof);
                    expect_projection(fixture.submit(fixture.checkpoint_request()),
                                      IntakeStatus::committed, selected,
                                      "16 small intake rows fit");
                    same_image(fixture.store.image(), exact_progress_image(before, selected));
                } else {
                    expect_refusal(fixture.submit(fixture.checkpoint_request()),
                                   IntakeStatus::limit_exceeded);
                    expect_unchanged(fixture, before, changes);
                }
            }
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
