// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_027/command_fixture.hpp"

#include <subathon/server/milestone_playback_stop.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

#include <thread>

namespace {
using namespace storage_027_fixture;
using namespace subathon::server::testing;
using Release = MilestoneReleaseDisposition;

class OwnerEntryMissing final : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

struct EditRelease final : MilestoneEditReleaseAuthority {
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == edit::b;
    }
};

void release(Populated &f) {
    const auto a = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return release_milestone_head(f.db.writer(), f.app_occurrence, std::move(done));
    });
    finance::normal(a.work);
    const auto b = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return release_milestone_head(f.db.writer(), f.edit_occurrence,
                                      std::make_shared<EditRelease>(), std::move(done));
    });
    finance::normal(b.work);
    require(a.disposition == Release::released && b.disposition == Release::released,
            "Both exact groups release normally before the owner boundary");
}

struct Dispatch final : MilestoneEditDispatchAuthority {
    bool allows(const MilestoneEditJob &job) override {
        return job.key.occurrence == edit::b && job.capture.sender_ref == "edit-sender";
    }
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override { return {}; }
};

using DispatchSink = std::vector<ReplacementJob>;

struct CaptureDispatch final : ReplacementDispatchAuthority {
    std::optional<ReplacementJob> owned;

    bool allows(const ReplacementJob &job) override {
        owned = job;
        return job.target.track == edit::track && job.target.occurrence == edit::b &&
               job.version.ordinal == 1 && job.payload == QByteArray::fromHex("0082") &&
               job.route == "local/new-route" && job.qualified_sender == "new-sender";
    }
};

struct AcknowledgedClaim {
    SqliteWorkResult work;
    Decision result;
    std::optional<ReplacementJob> selected_job;
};

void publish_dispatch(const AcknowledgedClaim &claim, DispatchSink &sink) {
    publish_acknowledged_replacement_claim(claim.work, claim.result.disposition, claim.selected_job,
                                           sink);
}

struct PendingCaptureSelection {
    CommittedMilestoneGroup cached;
    OccurrenceSelection committed;
};

std::vector<ReplacementJob> select_effective_release(SqliteWriter &writer,
                                                     const PendingCaptureSelection &selection) {
    auto completed =
        select_effective_milestone_release(writer, selection.cached, selection.committed.target);
    finance::normal(completed.work);
    return std::move(completed.selection.value().replacement_jobs);
}

struct PreviewAuthority final : CommandAuthority {
    Authority base;
    bool facts_available{true};
    int selections{};

    std::optional<Facts> select(const SqliteWorkSession &session, const Command &command) override {
        ++selections;
        return facts_available ? base.select(session, command) : std::nullopt;
    }

    std::optional<qint64> historical_bonus_amount(const SqliteWorkSession &, const Occurrence &,
                                                  const Replacement &) override {
        return {};
    }
};

Preview preview_at_owner(const SqliteWorkSession &session, CommandAuthority &owner,
                         const Command &command) {
    return preview_milestone_replacement(session, owner, command);
}

struct PlaybackPort final : PlaybackAuthority, PlaybackStopAdapter {
    CatalogFile &db;
    const LatePlayback &bound;
    std::optional<QByteArray> current_generation{"generation-X"};
    std::vector<PlaybackHandoff> calls;
    int current_checks{};
    bool marker_observed_at_call{false};

    PlaybackPort(CatalogFile &file, const LatePlayback &instance) : db(file), bound(instance) {}

    bool proves_current(const SqliteWorkSession &s, const LatePlayback &selection) override {
        return std::holds_alternative<PlaybackCurrent>(currentness(s, selection));
    }

    PlaybackCurrentness currentness(const SqliteWorkSession &,
                                    const LatePlayback &selection) override {
        ++current_checks;
        const auto &instance = selection.instance;
        const auto &expected = bound.instance;
        const auto *origin = std::get_if<MilestoneEditKey>(&instance.target.origin);
        if (selection.track != bound.track || selection.stop != bound.stop ||
            instance.target.track != expected.target.track ||
            instance.target.occurrence != expected.target.occurrence || !origin ||
            *origin != std::get<MilestoneEditKey>(expected.target.origin) ||
            instance.ordinal != expected.ordinal || instance.version != expected.version ||
            instance.started_attempt != expected.started_attempt ||
            instance.adapter != expected.adapter || instance.activation != expected.activation ||
            instance.runtime_generation != expected.runtime_generation ||
            instance.capability != expected.capability || !current_generation ||
            current_generation->isEmpty())
            return PlaybackCurrentnessUnknown{};
        if (*current_generation == expected.runtime_generation)
            return PlaybackCurrent{};
        return PlaybackProvenObsolete{*current_generation};
    }

    void request_stop(PlaybackHandoff handoff) override {
        // Independent read connection. An uncommitted marker is invisible here.
        const auto &selected = handoff.selected;
        const auto marker = db.rows("SELECT typeof(handoff),activation,generation FROM "
                                    "main.subathon_milestone_playback_instances "
                                    "WHERE installation=? AND track=? AND stop=? AND instance=?",
                                    {QByteArray::fromStdString(selected.track.installation_ref),
                                     QByteArray::fromStdString(selected.track.track_ref),
                                     selected.stop, handoff.intent});
        const SqlRows expected{
            {QString("blob"), selected.instance.activation, selected.instance.runtime_generation}};
        marker_observed_at_call = marker == expected;
        calls.push_back(std::move(handoff));
    }
};

enum class PlaybackTurn { handoff, cold_restore, late_activation };
void process_playback(SqliteWriter &writer, PlaybackTurn turn, const PlaybackHandoff &handoff,
                      PlaybackAuthority &authority, PlaybackStopAdapter &adapter) {
    process_milestone_playback_turn(writer, static_cast<MilestonePlaybackTurn>(turn), handoff,
                                    authority, adapter);
}

struct ColdQueueState {
    std::vector<ScopedCause> holds;
    std::vector<SelectedVersion> selected;
    std::optional<ActiveMilestonePresentation> presentation;
    std::optional<PlaybackOutcome> prior_handoff;
};

ColdQueueState restore_committed_queue(SqliteWriter &writer, CommandAuthority &authority,
                                       const Command &stop, const Command &next,
                                       const PlaybackHandoff &handoff, PlaybackStopAdapter &) {
    auto completed = restore_interrupted_milestone_queue(writer, authority, stop, next, handoff);
    finance::normal(completed.work);
    auto &restored = completed.queue.value();
    return {std::move(restored.holds), std::move(restored.selected), restored.presentation,
            restored.prior_handoff};
}

// The pinned implementation exposes only an optional projection. Compare its
// observable result until the owner returns a typed readback disposition.
template <class Completion>
bool same_restore_disposition(const Completion &left, const Completion &right) {
    if constexpr (requires { left.disposition; })
        return left.disposition == right.disposition;
    else
        return left.queue.has_value() == right.queue.has_value();
}

struct RestoreAuthority final : CommandAuthority {
    Authority base;
    QByteArray restricted_operation;
    bool facts_available{true};
    std::vector<QByteArray> reads;

    std::optional<Facts> select(const SqliteWorkSession &session, const Command &command) override {
        reads.push_back(command.operation);
        auto facts = base.select(session, command);
        if (facts && command.operation == restricted_operation) {
            if (!facts_available)
                return std::nullopt;
            facts->can_disclose = false;
        }
        return facts;
    }

    std::optional<qint64> historical_bonus_amount(const SqliteWorkSession &session,
                                                  const Occurrence &target,
                                                  const Replacement &replacement) override {
        return base.historical_bonus_amount(session, target, replacement);
    }
};

struct PlaybackFixture {
    Populated seed{{.third_target = true, .audio_job = true, .edit_slots = 3}};
    Authority owner;
    Command command = stop("stop-audio", edited(seed), QueueChoice::continue_queue);
    PlaybackInstance x{
        edited(seed), 1, {}, 2, "audio-adapter", "X", "generation-X", StopCapability::supported};
    PlaybackHandoff exact;

    explicit PlaybackFixture(bool acknowledged_x = true, bool hold = false) {
        if (hold)
            std::get<StopIntent>(command.intent).choice.reset();
        release(seed);
        Dispatch dispatch;
        seed.db.write([&](const auto &s) {
            for (qint64 ordinal : {1, 2, 3}) {
                const MilestoneEditJobKey key{seed.edit_intent.key, edit::b, ordinal};
                const auto started = claim_milestone_edit_job(s, key, 1, dispatch);
                require(started.disposition == DeliveryDisposition::changed && started.job &&
                            started.job->attempt == 2 &&
                            started.job->action_document == QByteArray::fromHex("0081"),
                        "Every playback candidate has a real exact attempt of the same asset");
            }
        });
        advance_fixture(seed.db);
        owner.observed_playback = {{edited(seed),
                                    2,
                                    {},
                                    2,
                                    "audio-adapter",
                                    "Y",
                                    "generation-Y",
                                    StopCapability::unknown}};
        if (acknowledged_x)
            owner.observed_playback.push_back(x);
        const auto decision = commit(seed.db, owner, command, Disposition::committed);
        require(decision.playback_intents.size() == (acknowledged_x ? 1U : 0U) &&
                    decision.cancellations.empty(),
                "Only exact supported X supplies a Stop intent");
        if (acknowledged_x)
            exact = {{edit::track, command.operation, x}, decision.playback_intents.front()};
        else
            exact = {{edit::track, command.operation, x}, {}};
    }

    SqlRows intents() {
        return seed.db.rows("SELECT ordinal,activation,generation,typeof(handoff) "
                            "FROM main.subathon_milestone_playback_instances ORDER BY ordinal");
    }

    SqlRows evidence() {
        return seed.db.rows(
            "SELECT outcome FROM main.subathon_milestone_playback_evidence ORDER BY ordinal");
    }
};
} // namespace

class Milestone006OwnerTests final : public QObject {
    Q_OBJECT

  private slots:
    void idleOwnerWorkRefusesWhileQueued() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        bool ran = false;
        bool completed = false;
        SqliteWriter writer;
        QCOMPARE(writer.open(dir.filePath("owner.sqlite")), StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [](auto context) {
                         context.begin().commit();
                         return QVariant(1);
                     },
                     [&](auto) { completed = true; }),
                 StorageError::none);
        const auto result = writer.run_idle_owner_work([&](auto) {
            ran = true;
            return QVariant{};
        });
        QCOMPARE(result.transaction, TransactionOutcome::not_started);
        QCOMPARE(result.error, WorkError::busy);
        QVERIFY(!ran && !completed);
        QTRY_VERIFY(completed);
    }

    void idleOwnerWorkRefusesOnForeignThreadAndDuringShutdown() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        bool ran = false;
        bool stopped = false;
        SqliteWriter writer;
        QCOMPARE(writer.open(dir.filePath("owner.sqlite")), StorageError::none);
        const auto work = [&](auto) {
            ran = true;
            return QVariant{};
        };
        SqliteWorkResult foreign{};
        std::thread caller([&] { foreign = writer.run_idle_owner_work(work); });
        caller.join();
        QCOMPARE(foreign.transaction, TransactionOutcome::not_started);
        QCOMPARE(foreign.error, WorkError::wrong_thread);
        QVERIFY(!ran);

        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        const auto stopping = writer.run_idle_owner_work(work);
        QCOMPARE(stopping.transaction, TransactionOutcome::not_started);
        QCOMPARE(stopping.error, WorkError::writer_unavailable);
        QVERIFY(!ran);
        QTRY_VERIFY(stopped);
    }

    void idleOwnerWorkReschedulesAfterFinalization() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        SqliteWorkSession session;
        bool ran_after_revocation = false;
        std::optional<SqliteWorkResult> queued;
        SqliteWriter writer;
        QCOMPARE(writer.open(dir.filePath("owner.sqlite")), StorageError::none);
        StorageError admitted{};
        const auto immediate = writer.run_idle_owner_work([&](auto context) {
            session = context.begin();
            admitted = writer.enqueue_work(
                [&](auto next_context) {
                    try {
                        (void)session.execute({"SELECT 1", {}});
                    } catch (const SqliteWorkFailure &error) {
                        ran_after_revocation = error.error() == WorkError::stale_session;
                    }
                    next_context.begin().commit();
                    return QVariant(2);
                },
                [&](auto result) { queued = std::move(result); });
            session.commit();
            return QVariant(1);
        });
        QCOMPARE(admitted, StorageError::none);
        QCOMPARE(immediate.transaction, TransactionOutcome::committed);
        QCOMPARE(immediate.error, WorkError::none);
        QVERIFY(!queued);
        QTRY_VERIFY(queued.has_value());
        QCOMPARE(queued->transaction, TransactionOutcome::committed);
        QCOMPARE(queued->error, WorkError::none);
        QVERIFY(ran_after_revocation);
    }

    void r1CachedReleaseSelectsEffectiveCapture() {
        check([] {
            Populated f({.edit_slots = 2, .middle_bonus = true});
            advance_fixture(f.db);
            Authority owner;
            const auto command = replace(f);
            std::optional<CommittedMilestoneGroup> original;
            f.db.write([&](const auto &s) {
                for (const auto &entry : SqliteMilestoneGroupStore(s).pending()) {
                    if (entry.group.occurrence == edit::b)
                        original = entry.group;
                }
                require(original && original->actions.size() == 2 &&
                            original->actions[0].version_ref == "announce-v1" &&
                            original->actions[1].kind == MilestoneActionKind::bonus,
                        "Warm owner holds the exact cached original and bonus");
            });
            owner.captures.front().payload = QByteArray::fromHex("0081");
            owner.captures.front().qualified_sender = "S";
            const auto preview = bind_replace(f.db, owner, command);
            const auto confirmed =
                commit(f.db, owner, command, Disposition::committed, preview.get());
            require(confirmed.versions.size() == 1, "Exactly one successor was selected");
            std::optional<OccurrenceSelection> retained;
            f.db.write([&](const auto &s) {
                retained = read_occurrence_selection(s, owner, stop("select-exact-B", edited(f)));
            });
            require(retained && !retained->released &&
                        retained->selected_versions ==
                            std::vector<SelectedVersion>{confirmed.versions.front(), {2, {}}},
                    "Normal readback selects the successor and preserves the original bonus");
            release(f);
            const auto selected = select_effective_release(f.db.writer(), {*original, *retained});
            require(selected.size() == 1 && selected.front().target.occurrence == edit::b &&
                        selected.front().version == confirmed.versions.front() &&
                        selected.front().payload == QByteArray::fromHex("0081") &&
                        selected.front().qualified_sender == "S",
                    "Owner selects the exact successor from storage despite the cached original");
        });
    }

    void r2AcknowledgedSuccessorDispatch_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("acknowledged-and-duplicate") << 0;
        QTest::newRow("lost-commit-ack") << 1;
        QTest::newRow("known-rollback") << 2;
    }

    void r2AcknowledgedSuccessorDispatch() {
        QFETCH(int, fault);
        check([&] {
            Populated f;
            release(f);
            advance_fixture(f.db);
            Authority owner;
            const auto command = replace(f);
            const auto preview = bind_replace(f.db, owner, command);
            const auto selected =
                commit(f.db, owner, command, Disposition::committed, preview.get());
            require(selected.versions.size() == 1, "Normal confirmation selects one successor");
            const auto version = selected.versions.front();
            CaptureDispatch authority;
            const auto claim = [&](qint64 revision) {
                authority.owned.reset();
                return run(f.db, [&](const auto &s) {
                    return Decision{claim_replacement(s, edited(f), version, revision, authority),
                                    {},
                                    {},
                                    {},
                                    {}};
                });
            };
            if (fault != 0) {
                if (fault == 1)
                    require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                                f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                                StorageError::none,
                            "Real successor COMMIT will lose acknowledgement");
                std::optional<Decision> provisional;
                const auto stopped = run(
                    f.db,
                    [&](const auto &s) {
                        provisional = Decision{
                            claim_replacement(s, edited(f), version, 1, authority), {}, {}, {}, {}};
                        return *provisional;
                    },
                    fault == 1 ? Finish::commit : Finish::rollback);
                require(provisional && provisional->disposition == Disposition::committed &&
                            authority.owned && authority.owned->version == version &&
                            !stopped.published && !stopped.work.value &&
                            stopped.work.transaction == (fault == 1
                                                             ? TransactionOutcome::outcome_unknown
                                                             : TransactionOutcome::rolled_back) &&
                            stopped.work.error == (fault == 1 ? WorkError::uncertain_outcome
                                                              : WorkError::not_committed),
                        "The exact selected claim was staged but never normally acknowledged");
                f.db.reopen(profile());
                f.db.write([&](const auto &s) {
                    const auto sending = lookup_replacement_attempt(s, edited(f), version, 2);
                    require(fault == 1 ? sending && sending->state == DeliveryJobState::sending
                                       : !sending,
                            "Cold readback distinguishes durable sending from known rollback");
                });
                DispatchSink sink;
                // Keep the tempting provisional result and body in both lanes.
                publish_dispatch({stopped.work, *provisional, authority.owned}, sink);
                require(sink.empty(), "An unacknowledged claim hands off no selected job");
                return;
            }
            const auto acknowledged = claim(1);
            require(acknowledged.published &&
                        acknowledged.published->disposition == Disposition::committed &&
                        authority.owned && authority.owned->version == version,
                    "Real committed sending transition owns one exact dispatch snapshot");
            const AcknowledgedClaim first{acknowledged.work, *acknowledged.published,
                                          authority.owned};
            const auto duplicate = claim(1);
            require(duplicate.published &&
                        duplicate.published->disposition == Disposition::refused &&
                        !authority.owned,
                    "Stale duplicate grants no snapshot or sending transition");
            DispatchSink sink;
            publish_dispatch(first, sink);
            publish_dispatch({duplicate.work, *duplicate.published, {}}, sink);
            require(sink.size() == 1 && sink.front().version == version &&
                        sink.front().target.occurrence == edit::b &&
                        sink.front().payload == QByteArray::fromHex("0082") &&
                        sink.front().qualified_sender == "new-sender",
                    "Only one exact acknowledged successor reaches the sink");
        });
    }

    void knownReplacementAttemptCanSettleAfterReopen() {
        check([] {
            Populated f;
            release(f);
            advance_fixture(f.db);
            Authority owner;
            const auto command = replace(f);
            const auto preview = bind_replace(f.db, owner, command);
            const auto selected =
                commit(f.db, owner, command, Disposition::committed, preview.get());
            require(selected.versions.size() == 1, "Replacement selects one exact successor");
            const auto version = selected.versions.front();
            CaptureDispatch authority;
            f.db.write([&](const auto &s) {
                require(claim_replacement(s, edited(f), version, 1, authority) ==
                            Disposition::committed,
                        "Real successor claim establishes sending revision two");
            });
            f.db.reopen(profile());
            f.db.write([&](const auto &s) {
                const auto sending = lookup_replacement_attempt(s, edited(f), version, 2);
                require(sending && sending->state == DeliveryJobState::sending &&
                            finish_replacement(s, edited(f), version, 2,
                                               DeliveryEvidence::indeterminate,
                                               "exact-stranded-attempt") == Disposition::committed,
                        "Known exact sending attempt can settle independently of discovery");
                const auto retained = lookup_replacement_attempt(s, edited(f), version, 3);
                require(retained && retained->evidence == "exact-stranded-attempt" &&
                            retained->state == DeliveryJobState::outcome_unknown,
                        "An independent event retains indeterminate outcome evidence");
            });
        });
    }

    void r7AbsentAuthorityIsUnavailable_data() {
        QTest::addColumn<bool>("facts_available");
        QTest::newRow("absent-facts") << false;
        QTest::newRow("known-denial") << true;
    }

    void r7AbsentAuthorityIsUnavailable() {
        QFETCH(bool, facts_available);
        check([&] {
            Populated f;
            advance_fixture(f.db);
            PreviewAuthority owner;
            const auto command = replace(f);
            const auto positive = bind_replace(f.db, owner.base, command);
            require(bool(positive), "The exact positive authority reaches a normal preview");
            owner.base.command = false;
            (void)commit(f.db, owner.base, command, Disposition::denied, positive.get());
            const auto no_writes = [&] {
                return journal(f.db).empty() &&
                       f.db.rows("SELECT version FROM main.subathon_milestone_slot_versions")
                           .empty() &&
                       f.db.rows("SELECT version FROM main.subathon_milestone_slot_attempts")
                           .empty();
            };
            require(no_writes(), "Known protected denial reaches the writer with no mutation");

            owner.facts_available = facts_available;
            f.db.write([&](const auto &session) {
                const auto selected = owner.select(session, command);
                require(facts_available ? selected && !selected->can_command : !selected,
                        "Exact authority produces absent facts or known denial on this session");
            });
            owner.selections = 0;
            f.db.write([&](const auto &session) {
                const auto reply = preview_at_owner(session, owner, command);
                const auto *denial = std::get_if<Disposition>(&reply);
                require(denial &&
                            *denial == (facts_available ? Disposition::denied
                                                        : Disposition::unavailable) &&
                            owner.selections > 0,
                        "Owner distinguishes absence from denial using actual authority selection");
            });
            require(no_writes(),
                    "Owner refusal leaves the journal, versions and attempts unchanged");
        });
    }

    void p1ExactPlaybackHandoff_data() {
        QTest::addColumn<bool>("stale");
        QTest::newRow("exact-current-X") << false;
        QTest::newRow("same-asset-new-generation") << true;
    }

    void p1ExactPlaybackHandoff() {
        QFETCH(bool, stale);
        check([&] {
            PlaybackFixture f;
            PlaybackPort port(f.seed.db, f.exact.selected);
            if (stale)
                port.current_generation = "generation-replacement";
            require(f.intents() == SqlRows{{1, QByteArray("X"), QByteArray("generation-X"),
                                            QString("null")},
                                           {2, QByteArray("Y"), QByteArray("generation-Y"),
                                            QString("null")}} &&
                        f.evidence().empty(),
                    "Stored X and unknown Y retain exact identities without any handoff");
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, f.exact, port, port);
            require(port.current_checks > 0 && port.calls.size() == (stale ? 0U : 1U),
                    "Owner sends only to current X and never to the successor generation");
            if (!stale)
                require(
                    port.marker_observed_at_call && port.calls.front().intent == f.exact.intent &&
                        port.calls.front().selected.instance.activation == "X" &&
                        port.calls.front().selected.instance.runtime_generation == "generation-X",
                    "The dispatched handle is bound to the exact old generation");
            if (stale) {
                require(f.evidence() == SqlRows{{int(PlaybackOutcome::obsolete)}},
                        "The stale intent is retired without a call");
            } else {
                const auto outcomes = f.evidence();
                require(std::ranges::find(
                            outcomes, QList<QVariant>{int(PlaybackOutcome::confirmed_stopped)}) ==
                            outcomes.end(),
                        "A request alone produces no confirmed-stop evidence");
            }
        });
    }

    void p2ExactLateActivationIsScheduledOnce() {
        check([] {
            PlaybackFixture f(false);
            PlaybackPort port(f.seed.db, f.exact.selected);
            require(f.intents() ==
                        SqlRows{{2, QByteArray("Y"), QByteArray("generation-Y"), QString("null")}},
                    "Stop initially retains only the unknown disposition, no X intent");
            auto wrong = f.exact;
            wrong.selected.instance.started_attempt = 3;
            process_playback(f.seed.db.writer(), PlaybackTurn::late_activation, f.exact, port,
                             port);
            process_playback(f.seed.db.writer(), PlaybackTurn::late_activation, f.exact, port,
                             port);
            process_playback(f.seed.db.writer(), PlaybackTurn::late_activation, wrong, port, port);
            require(port.calls.empty(), "Late evidence never calls media on its registration turn");
            const auto rows = f.intents();
            require(rows.size() == 2 &&
                        rows[0] == QList<QVariant>{1, QByteArray("X"), QByteArray("generation-X"),
                                                   QString("null")},
                    "Only the matching started attempt gets one new exact intent");
            const auto ids = f.seed.db.rows(
                "SELECT instance FROM main.subathon_milestone_playback_instances WHERE ordinal=1");
            require(ids.size() == 1 && ids.front().size() == 1,
                    "Exact late intent has one durable instance key");
            auto late = f.exact;
            late.intent = ids.front().front().toByteArray();
            require(!late.intent.isEmpty(), "Late handoff retains a real exact key");
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, late, port, port);
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, late, port, port);
            require(port.calls.size() == 1 && port.marker_observed_at_call &&
                        port.calls.front().intent == late.intent &&
                        port.calls.front().selected.instance.started_attempt == 2,
                    "A separate owner turn hands off the exact late attempt once");
        });
    }

    void p3ColdHandoffNeedsPositiveProof_data() {
        QTest::addColumn<bool>("marked");
        QTest::addColumn<bool>("known_live");
        QTest::newRow("marked-current") << true << true;
        QTest::newRow("marked-without-proof") << true << false;
        QTest::newRow("unmarked-with-proof") << false << true;
        QTest::newRow("unmarked-without-proof") << false << false;
    }

    void p3ColdHandoffNeedsPositiveProof() {
        QFETCH(bool, marked);
        QFETCH(bool, known_live);
        check([&] {
            PlaybackFixture f;
            PlaybackPort port(f.seed.db, f.exact.selected);
            if (marked) {
                f.seed.db.write([&](const auto &s) {
                    require(stage_playback_handoff(s, port, f.exact) == Disposition::committed,
                            "An exact one-way handoff marker commits before acknowledgement loss");
                });
            }
            f.seed.db.reopen(profile());
            if (!known_live)
                port.current_generation.reset();
            const auto original = read(f.seed.db, f.owner, f.command);
            require(original.disposition == Disposition::replay &&
                        original.playback_intents == std::vector{f.exact.intent},
                    "Authoritative cold readback selects the exact immutable Stop result");
            process_playback(f.seed.db.writer(), PlaybackTurn::cold_restore, f.exact, port, port);
            require(port.calls.empty(), "Cold restore alone makes no adapter call");
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, f.exact, port, port);
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, f.exact, port, port);
            require(port.calls.size() == (!marked && known_live ? 1U : 0U) &&
                        read(f.seed.db, f.owner, f.command) == original,
                    "Only an unmarked currently proven instance can hand off once on a later turn");
            if (!marked && known_live)
                require(
                    port.marker_observed_at_call,
                    "The marker is visible on an independent connection before the adapter call");
            if (!marked && !known_live)
                require(f.intents().front().at(3) == QString("null") && f.evidence().empty(),
                        "Without positive proof the unmarked intent remains pending without "
                        "outcome evidence");
            if (marked && known_live) {
                f.seed.db.write([&](const auto &s) {
                    require(stage_playback_outcome(s, port,
                                                   {f.exact, PlaybackOutcome::confirmed_stopped,
                                                    "observed-X"}) == Disposition::committed,
                            "An owning exact observation can settle the marked attempt");
                });
                const auto evidence = f.evidence();
                require(!evidence.empty() &&
                            evidence.back() ==
                                QList<QVariant>{int(PlaybackOutcome::confirmed_stopped)} &&
                            read(f.seed.db, f.owner, f.command) == original && port.calls.empty(),
                        "Later confirmed stop evidence leaves the immutable Stop result unchanged");
            }
        });
    }
    void coldRestoreDistinguishesAuthorityFromAbsence_data() {
        QTest::addColumn<bool>("at_successor");
        QTest::newRow("stop-readback") << false;
        QTest::newRow("successor-selection") << true;
    }

    void coldRestoreDistinguishesAuthorityFromAbsence() {
        QFETCH(bool, at_successor);
        check([&] {
            PlaybackFixture f(true, true);
            const Occurrence successor{edit::track, edit::c, f.seed.add_third_occurrence(true)};
            f.seed.db.reopen(profile());
            RestoreAuthority authority;
            const auto next = stop("select-existing-next", successor);
            const auto restore = [&](const Command &wanted_stop, const Command &wanted_next,
                                     const PlaybackHandoff &handoff) {
                authority.reads.clear();
                auto result = restore_interrupted_milestone_queue(
                    f.seed.db.writer(), authority, wanted_stop, wanted_next, handoff);
                finance::normal(result.work);
                return result;
            };
            const auto positive = restore(f.command, next, f.exact);
            require(positive.queue && positive.queue->holds.size() == 1 &&
                        !positive.queue->selected.empty() && !positive.queue->presentation,
                    "Normally acknowledged restore publishes the durable Hold and selection");

            auto missing_stop = f.command;
            auto missing_handoff = f.exact;
            auto missing_next = next;
            if (at_successor) {
                auto target = successor;
                target.occurrence = MilestoneOccurrenceKey{"unrecorded-successor"};
                missing_next = stop("select-missing-next", std::move(target));
            } else {
                missing_stop.operation = "no-recorded-stop";
                missing_handoff.selected.stop = missing_stop.operation;
            }
            const auto absent = restore(missing_stop, missing_next, missing_handoff);
            const auto absent_operation =
                at_successor ? missing_next.operation : missing_stop.operation;
            require(!absent.queue && std::ranges::find(authority.reads, absent_operation) !=
                                         authority.reads.end(),
                    "Authoritative lookup reaches the exact missing record without a projection");

            authority.restricted_operation = at_successor ? next.operation : f.command.operation;
            authority.facts_available = false;
            const auto unavailable = restore(f.command, next, f.exact);
            require(!unavailable.queue &&
                        std::ranges::find(authority.reads, authority.restricted_operation) !=
                            authority.reads.end(),
                    "Missing authority is reached without publishing an empty queue");
            authority.facts_available = true;
            const auto denied = restore(f.command, next, f.exact);
            require(!denied.queue &&
                        std::ranges::find(authority.reads, authority.restricted_operation) !=
                            authority.reads.end(),
                    "Known disclosure denial is reached without publishing an empty queue");

            require(!same_restore_disposition(absent, unavailable),
                    "Missing authority is distinguishable from acknowledged absence");
            require(!same_restore_disposition(absent, denied),
                    "Known denial is distinguishable from acknowledged absence");
            require(!same_restore_disposition(unavailable, denied),
                    "Missing authority is distinguishable from known denial");
        });
    }

    void coldRestoreDistinguishesWriterUncertaintyFromAbsence() {
        check([] {
            PlaybackFixture f(true, true);
            const Occurrence successor{edit::track, edit::c, f.seed.add_third_occurrence(true)};
            f.seed.db.reopen(profile());
            RestoreAuthority authority;
            const auto next = stop("select-existing-next", successor);
            const auto restore = [&](const Command &wanted_stop, const PlaybackHandoff &handoff) {
                authority.reads.clear();
                return restore_interrupted_milestone_queue(f.seed.db.writer(), authority,
                                                           wanted_stop, next, handoff);
            };
            auto missing = f.command;
            missing.operation = "no-recorded-stop";
            auto missing_handoff = f.exact;
            missing_handoff.selected.stop = missing.operation;
            const auto absent = restore(missing, missing_handoff);
            finance::normal(absent.work);
            require(!absent.queue && authority.reads == std::vector{missing.operation},
                    "Acknowledged absence was looked up with authority");

            bool drained = false;
            auto &writer = f.seed.db.writer();
            require(writer.enqueue_work(
                        [](auto context) {
                            context.begin().commit();
                            return QVariant(1);
                        },
                        [&](auto) { drained = true; }) == StorageError::none,
                    "A queued writer unit prevents inline restore");
            const auto busy = restore(f.command, f.exact);
            QTRY_VERIFY(drained);
            require(busy.work.transaction == TransactionOutcome::not_started &&
                        busy.work.error == WorkError::busy && !busy.queue &&
                        authority.reads.empty(),
                    "Busy writer begins no read and publishes no projection");

            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "The exact restore transaction will lose acknowledgement");
            const auto unknown = restore(f.command, f.exact);
            require(unknown.work.transaction == TransactionOutcome::outcome_unknown &&
                        unknown.work.error == WorkError::uncertain_outcome && !unknown.queue &&
                        std::ranges::find(authority.reads, next.operation) != authority.reads.end(),
                    "Lost acknowledgement suppresses the projection after successor selection");
            f.seed.db.reopen(profile());
            const auto recovered = restore(f.command, f.exact);
            finance::normal(recovered.work);
            require(recovered.queue && recovered.queue->holds.size() == 1 &&
                        !recovered.queue->selected.empty() && !recovered.queue->presentation,
                    "Fresh acknowledged readback recovers the durable Hold and selection");

            require(!same_restore_disposition(absent, busy),
                    "Busy writer is distinguishable from acknowledged absence");
            require(!same_restore_disposition(absent, unknown),
                    "Unknown writer result is distinguishable from acknowledged absence");
            require(!same_restore_disposition(busy, unknown),
                    "Busy writer is distinguishable from an unknown transaction outcome");
        });
    }

    void t2ColdRestoreSelectionHoldAndMarkedPlayback_data() {
        QTest::addColumn<bool>("has_marker");
        QTest::newRow("marked") << true;
        QTest::newRow("unmarked") << false;
    }

    void t2ColdRestoreSelectionHoldAndMarkedPlayback() {
        QFETCH(bool, has_marker);
        check([&] {
            PlaybackFixture f(true, true);
            // Keep C independent of the stopped B and leave it unreleased and
            // unstarted. Its selected replacement is proved by normal readback.
            const auto origin = f.seed.add_third_occurrence(true);
            const Occurrence next{edit::track, edit::c, origin};
            const Replacement patch{1, {MilestoneActionKind::automation, "audio", "audio-v2"}};
            const Command command{edit::track, "replace-C", edit::actor,
                                  ReplaceIntent{next, {patch}}};
            f.owner.captures = {{patch, QByteArray::fromHex("00ff"), "local/route", "new-sender"}};
            const auto preview = bind_replace(f.seed.db, f.owner, command);
            const auto committed =
                commit(f.seed.db, f.owner, command, Disposition::committed, preview.get());
            require(committed.versions.size() == 1, "C has one unstarted successor.");

            PlaybackPort port(f.seed.db, f.exact.selected);
            if (has_marker) {
                f.seed.db.write([&](const auto &s) {
                    require(stage_playback_handoff(s, port, f.exact) == Disposition::committed,
                            "A real one-way marker commits before loss of adapter acknowledgement");
                });
            }
            f.seed.db.reopen(profile());
            const auto stop_result = read(f.seed.db, f.owner, f.command);
            const auto marked = f.intents();
            require(!marked.empty() && !marked.front().empty(),
                    "Cold playback read retains the exact marker row");
            require(stop_result.disposition == Disposition::replay &&
                        stop_result.added_causes.size() == 1 &&
                        stop_result.playback_intents == std::vector{f.exact.intent} &&
                        marked.front().at(3) == QString(has_marker ? "blob" : "null"),
                    "Cold history retains a real Hold and the exact marked or unmarked playback "
                    "intent");
            std::optional<OccurrenceSelection> stopped;
            std::optional<OccurrenceSelection> future;
            f.seed.db.write([&](const auto &s) {
                stopped = read_occurrence_selection(s, f.owner, f.command);
                future = read_occurrence_selection(s, f.owner, stop("select-C", next));
            });
            require(stopped && future && stopped->stop == f.command.operation &&
                        future->target.occurrence == edit::c && !future->released &&
                        future->selected_versions == committed.versions &&
                        future->interruption_holds == stop_result.added_causes &&
                        port.calls.empty(),
                    "Normal v21 readback reaches the distinct stopped and selected occurrences");
            const auto restored = restore_committed_queue(f.seed.db.writer(), f.owner, f.command,
                                                          stop("select-C", next), f.exact, port);
            const ScopedCause only_cause{edit::track, stop_result.added_causes.front()};
            const std::vector<ScopedCause> expected_holds{only_cause};
            require(
                restored.holds == expected_holds && restored.selected == committed.versions &&
                    !restored.presentation &&
                    restored.prior_handoff ==
                        (has_marker ? std::optional{PlaybackOutcome::uncertain} : std::nullopt) &&
                    f.intents() == marked && port.calls.empty(),
                "Cold restore selects exact Hold and version, distinguishes marker states, and "
                "starts no window or media");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006OwnerTests)
#include "owner_tests.moc"
