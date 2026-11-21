// SPDX-License-Identifier: GPL-3.0-or-later
#include "command_fixture.hpp"

#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace storage_027_fixture;
using namespace subathon::server::testing;
using predecessor::check;

void exact_same_result(Decision retained, const Decision &committed) {
    require(retained.disposition == Disposition::replay, "Exact authorized intent replays");
    retained.disposition = Disposition::committed;
    require(retained == committed, "Replay returns exact immutable selected members and causes");
}

void same_redaction(CatalogFile &db, Authority &port, const Command &exact, Command changed,
                    const Decision &before) {
    const auto prior = journal(db);
    port.disclosure = false;
    auto unused = exact;
    unused.operation = "unused-operation";
    const auto unknown = commit(db, port, unused, Disposition::denied);
    const auto hidden = commit(db, port, exact, Disposition::denied);
    const auto conflict_hidden = commit(db, port, changed, Disposition::denied);
    require(
        hidden == unknown && conflict_hidden == unknown && read(db, port, exact) == unknown &&
            before.disposition == Disposition::committed,
        "Revoked disclosure has the same redacted shape on unused, replay, conflict and lookup");
    require(prior == journal(db), "Denied calls add no operations");
}

void release_application(Populated &f) {
    const auto release = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return release_milestone_head(f.db.writer(), f.app_occurrence, std::move(done));
    });
    finance::normal(release.work);
    require(release.disposition == MilestoneReleaseDisposition::released,
            "Application releases through its owning overload");
}

struct EditRelease final : MilestoneEditReleaseAuthority {
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == edit::b;
    }
};

void release_edit(Populated &f) {
    const auto release = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return release_milestone_head(f.db.writer(), f.edit_occurrence,
                                      std::make_shared<EditRelease>(), std::move(done));
    });
    finance::normal(release.work);
    require(release.disposition == MilestoneReleaseDisposition::released,
            "Edit releases through its owning exact-authority overload");
}

struct Dispatch final : MilestoneEditDispatchAuthority {
    int selected{};
    bool allow_retry{};
    bool allows(const MilestoneEditJob &job) override {
        ++selected;
        return job.key.occurrence == edit::b && job.capture.sender_ref == "edit-sender";
    }
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override {
        return allow_retry ? std::optional<std::string>{"exact-retry-grant"} : std::nullopt;
    }
};
struct SuccessorDispatch final : ReplacementDispatchAuthority {
    int calls{};
    bool allows(const ReplacementJob &job) override {
        ++calls;
        return job.target.track == edit::track && job.target.occurrence == edit::b &&
               job.version.ordinal == 1 && job.payload == QByteArray::fromHex("0082") &&
               job.route == "local/new-route" && job.qualified_sender == "new-sender";
    }
};

struct PlaybackOwner final : PlaybackAuthority {
    bool proves_current(const SqliteWorkSession &, const LatePlayback &requested) override {
        return requested.track == edit::track && requested.instance.target.track == edit::track &&
               requested.instance.target.occurrence == edit::b &&
               requested.instance.adapter == "audio-adapter" &&
               requested.instance.runtime_generation == "runtime-1" &&
               requested.instance.capability == StopCapability::supported;
    }
};

struct BonusProducer : predecessor::Populated {
    const MilestoneOccurrenceKey app_occurrence{"active-bonus-origin"};
    const MilestoneOccurrenceKey occurrence;
    const MilestoneAction action{MilestoneActionKind::bonus, "award", "award-30"};
    ApplicationOperationKey original;
    std::optional<MilestoneEditKey> edit_operation;
    ContributionReceipt original_effect, replacement_effect;

    explicit BonusProducer(bool use_edit) : occurrence(use_edit ? edit::b : app_occurrence) {
        install_protected(*this);
        install();
        const auto entry = seed("dense-bonus-origin", Case::positive());
        auto review = service();
        const auto cmd = bind("dense-bonus-origin", {entry}, preview(review, {entry}));
        const auto result = execute(review, cmd);
        expected_row(reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
        original = *reply(result).rows.front().outcome->facts->bundle;
        const core::MilestoneCompletion achieved{"target-95", "bonus-track-v1", 95, 97, "ctx"};
        db.write([&](const auto &s) {
            const auto actions = use_edit ? std::vector<MilestoneAction>{} : std::vector{action};
            SqliteMilestoneGroupStore(s).stage(original,
                                               {{{achieved, actions, {}, app_occurrence}, {}}});
            SqliteMilestoneGroupDecisions(s).record_acceptance(
                original, {PausedAcceptanceMode::normal,
                           {"operator", "actual-award-decision"},
                           {{app_occurrence, MilestoneActionChoice::run}}});
            MilestoneTrackSeed track{
                edit::track,
                "cfg",
                "bonus-definition",
                "bonus-prefix",
                {{"target-95", 95}},
                std::vector<MilestoneTrackSeed::Member>{{achieved, original, app_occurrence}}};
            if (use_edit)
                track.targets.push_back({"B", 200});
            require(initialize_milestone_track(s, track),
                    "Real authority track anchors the exact credited group");
            if (!use_edit)
                return;
            edit::Authority owner;
            owner.facts.captures = {{"B", occurrence, {action}, {}, {}}};
            const auto edit = milestone_target_edit_fixture::single(
                "bonus-edit", {"B", 96}, {{occurrence, MilestoneActionChoice::run}});
            const auto preview =
                preview_milestone_target_edit(s, owner, milestone_target_edit_fixture::actor, edit);
            milestone_target_edit_fixture::File::confirmable(preview);
            const auto saved = confirm_milestone_target_edit(
                s, owner, milestone_target_edit_fixture::actor, edit, *preview.confirmation);
            require(saved.result && saved.result->achievements.size() == 1 &&
                        saved.result->achievements[0].order == 2,
                    "Actual target edit commits the exact immutable bonus successor lane");
            edit_operation = edit.key;
        });
        const auto released = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
            return release_milestone_head(db.writer(), app_occurrence, std::move(done));
        });
        finance::normal(released.work);
        require(released.disposition == MilestoneReleaseDisposition::released,
                "Actual protected application release precedes either bonus contender");
        if (use_edit) {
            const auto second = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                return release_milestone_head(db.writer(), occurrence,
                                              std::make_shared<EditRelease>(), std::move(done));
            });
            finance::normal(second.work);
            require(second.disposition == MilestoneReleaseDisposition::released,
                    "The second protected release selects the exact edit bonus");
        }
        original_effect = intake("bonus-original-effect");
        replacement_effect = intake("bonus-replacement-effect");
    }

    Occurrence target() const {
        using Origin = decltype(Occurrence::origin);
        return {edit::track, occurrence,
                edit_operation ? Origin{*edit_operation} : Origin{original}};
    }

    qint64 main() {
        return db
            .rows("SELECT s.main_remaining_ms FROM subathon_app_head h "
                  "JOIN subathon_app_states s ON h.token=s.token")[0][0]
            .toLongLong();
    }
};

struct BonusPorts final : MilestoneBonusAuthority, MilestoneBonusEffect, ReplacementBonusEffect {
    BonusProducer &f;
    bool cap{};
    explicit BonusPorts(BonusProducer &fixture, bool capped) : f(fixture), cap(capped) {}

    bool allows(const MilestoneBonusKey &key, const MilestoneAction &selected) override {
        return key.occurrence == f.occurrence && key.action_ordinal == 1 && selected == f.action;
    }

    static Actual record(const SqliteWorkSession &s, const ContributionReceipt &receipt,
                         const char *id, qint64 requested, bool capped) {
        auto bundle = finance::File::receipt_application(s, receipt, id);
        const auto prior = bundle.after.timer.main_remaining_ms;
        require(bundle.after.timer.lifecycle == core::TimerLifecycle::Active &&
                    bundle.after.time_debt_ms == 0,
                "Bonus budget witness uses a real Active head with zero debt");
        const auto actual = capped ? qint64(0) : requested;
        finance::File::retain_plan(
            s, bundle.intent.plan,
            finance::File::plain_capture(
                receipt, {}, {finance::request(finance::Unit::milliseconds, requested, "main")}));
        auto effect = finance::addition(requested, actual, 0, requested - actual);
        if (capped)
            effect.maximum_main_ms = prior;
        bundle.effects.push_back(std::move(effect));
        bundle.after.timer.main_remaining_ms += actual;
        require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                    SqliteBundleStageDisposition::staged,
                "Owning bundle writes exactly the actual effect and retained provenance");
        return {id, actual};
    }

    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &,
                      const MilestoneAction &) override {
        return record(s, f.original_effect, "bonus-original", 30000, cap).effect_ref;
    }

    Actual apply(const SqliteWorkSession &s, const Occurrence &target,
                 const SelectedVersion &version, qint64 requested_ms) override {
        require(target.occurrence == f.occurrence && version.ordinal == 1 && requested_ms == 20000,
                "The effect owner receives only the exact successor's retained 20 seconds");
        return record(s, f.replacement_effect, "bonus-replacement", requested_ms, false);
    }
};

} // namespace

class Storage027CommandsTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void stopReplayAndStopFence() {
        check([] {
            Populated f;
            advance_fixture(f.db);
            Authority port;
            const auto key = stop("stop-B", edited(f));
            const auto first = commit(f.db, port, key, Disposition::committed);
            require(first.cancellations.size() == 1 && first.cancellations[0].ordinal == 1 &&
                        first.added_causes.size() == 1 && !first.added_causes[0].isEmpty() &&
                        cancellations(f.db, key.operation).size() == 1,
                    "Omitted choice retains one Hold and cancels exactly queued original slot 1");
            const auto original = journal(f.db);
            const auto hold = causes(f.db);
            auto normalized = key;
            std::get<StopIntent>(normalized.intent).choice = QueueChoice::hold;
            exact_same_result(commit(f.db, port, normalized, Disposition::replay), first);
            auto changed = key;
            std::get<StopIntent>(changed.intent).choice = QueueChoice::continue_queue;
            (void)commit(f.db, port, changed, Disposition::conflict);
            (void)commit(f.db, port, stop("second-B", edited(f)), Disposition::refused);
            require(
                original.size() == 1 && original == journal(f.db) && hold == causes(f.db),
                "Exact retry, fresh Stop fence and conflict retain the original journal and Hold");
            require(f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences "
                              "WHERE occurrence_ref=?",
                              {QByteArray("b-edit")}) == SqlRows{{0}},
                    "Stop never writes the release bit of a pending occurrence");
            same_redaction(f.db, port, key, changed, first);
        });
    }

    void stoppedHeadKeepsTheIndependentDefer() {
        check([] {
            Populated f({.held = true});
            advance_fixture(f.db);
            Authority port;
            const auto stop_a =
                stop("stop-A-continue", application(f), QueueChoice::continue_queue);
            const auto result = commit(f.db, port, stop_a, Disposition::committed);
            require(result.added_causes.empty() && result.cancellations.empty(),
                    "Continue fences empty A without synthesizing a Hold or cancellation");
            f.db.reopen(profile());
            const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance,
                                         "milestone-decision"};
            f.db.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions gates(s);
                require(gates.active_holds(f.app_occurrence) == std::vector{defer} &&
                            gates.pending_disposition(f.app_occurrence) ==
                                MilestonePendingDisposition::held,
                        "Cold selected A keeps the exact prior Defer despite terminal Stop");
            });
            const auto blocked = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                return release_milestone_head(f.db.writer(), f.edit_occurrence,
                                              std::make_shared<EditRelease>(), std::move(done));
            });
            finance::normal(blocked.work);
            require(blocked.disposition == MilestoneReleaseDisposition::refused,
                    "The otherwise ready later edit cannot pass stopped A while Defer is active");
            f.db.write([&](const auto &s) {
                require(SqliteMilestoneGroupDecisions(s).resolve(
                            f.app_occurrence, defer, MilestoneHoldResolution::continue_actions,
                            {"operator", "resolve-exact-defer"}),
                        "Authorized independent #236 resolution clears its own exact cause");
            });
            release_edit(f);
            require(
                f.db.rows("SELECT released FROM main.subathon_milestone_groups") == SqlRows{{0}} &&
                    f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences") ==
                        SqlRows{{1}},
                "Once passable, stopped A stays unreleased and only B releases");
        });
    }

    void stopAcknowledgement_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("confirmed-rollback") << 0;
        QTest::newRow("lost-commit-ack") << 1;
        QTest::newRow("abnormal-work-return") << 2;
    }

    void stopAcknowledgement() {
        QFETCH(int, fault);
        check([&] {
            Populated f;
            advance_fixture(f.db);
            Authority port;
            const auto key = stop("stop-B", edited(f));
            if (fault == 1)
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Only the real successful Stop COMMIT loses acknowledgement");
            bool staged = false;
            const auto turn = run(
                f.db,
                [&](const auto &session) {
                    const auto answer = stage_command(session, port, key);
                    staged = answer.disposition == Disposition::committed;
                    return answer;
                },
                fault == 0   ? Finish::rollback
                : fault == 2 ? Finish::throw_after_commit
                             : Finish::commit);
            require(!turn.missing, "STORAGE-027 NI: Stop acknowledgement command seam");
            require(staged, "The eligible pending Stop stages before the control fault");
            require(turn.entered && !turn.published && !turn.work.value,
                    "Rollback, uncertainty and abnormal exit have no published Stop");
            const auto expected = fault == 0   ? TransactionOutcome::rolled_back
                                  : fault == 1 ? TransactionOutcome::outcome_unknown
                                               : TransactionOutcome::committed;
            require(turn.work.transaction == expected,
                    "Acknowledgement failure preserves the exact physical transaction outcome");
            const auto error = fault == 0   ? WorkError::not_committed
                               : fault == 1 ? WorkError::uncertain_outcome
                                            : WorkError::work_exception;
            require(turn.work.error == error, "Writer retains the exact first work error");
            f.db.reopen(profile());
            if (fault == 0) {
                require(journal(f.db).isEmpty() && causes(f.db).isEmpty() &&
                            cancellations(f.db, key.operation).isEmpty(),
                        "Acknowledged rollback persists no command, cancellation or Hold");
                (void)commit(f.db, port, key, Disposition::committed);
            } else {
                const auto recovered = read(f.db, port, key);
                require(recovered.disposition == Disposition::replay &&
                            recovered.added_causes.size() == 1 &&
                            recovered.cancellations.size() == 1 && journal(f.db).size() == 1,
                        "Normal exact readback recovers the single committed result");
                require(commit(f.db, port, key, Disposition::replay) == recovered,
                        "Later exact retry does not publish a changed result or new Hold");
            }
        });
    }

    void resumeSetReplay() {
        check([] {
            Populated f({.held = true, .third_target = true});
            const auto third = f.add_third_occurrence();
            advance_fixture(f.db);
            Authority port;
            const auto a =
                commit(f.db, port, stop("hold-A", application(f)), Disposition::committed);
            const auto b = commit(f.db, port, stop("hold-B", edited(f)), Disposition::committed);
            require(a.added_causes.size() == 1 && b.added_causes.size() == 1 &&
                        a.added_causes != b.added_causes,
                    "Two exact typed origins create two distinct interruption causes");
            const auto &h1 = a.added_causes[0];
            const auto &h2 = b.added_causes[0];
            const auto active = causes(f.db);
            for (const auto &invalid : std::vector<std::vector<QByteArray>>{
                     {}, {h1, h1}, {h1, "missing-cause"}, {h1, "milestone-decision"}})
                (void)commit(f.db, port, resume("invalid", invalid), Disposition::refused);
            require(active.size() == 2 && active == causes(f.db),
                    "Malformed or mixed missing set never resolves a legal member");
            const auto key = resume("resume-both", {h1, h2});
            const auto first = commit(f.db, port, key, Disposition::committed);
            require(first.resolved_causes.size() == 2 &&
                        std::ranges::find(first.resolved_causes, h1) !=
                            first.resolved_causes.end() &&
                        std::ranges::find(first.resolved_causes, h2) != first.resolved_causes.end(),
                    "One Resume returns both exact independent resolutions");
            const auto c = commit(f.db, port, stop("hold-C", {edit::track, edit::c, third}),
                                  Disposition::committed);
            require(c.added_causes.size() == 1 && c.added_causes[0] != h1 &&
                        c.added_causes[0] != h2,
                    "A later Stop adds a third independent Hold");
            const auto saved = causes(f.db);
            require(saved.size() == 3 && saved[0][2].toByteArray() == key.operation &&
                        saved[1][2].toByteArray() == key.operation &&
                        saved[2][0].toByteArray() == c.added_causes[0] && saved[2][2].isNull(),
                    "Only the two original Hold rows link the one Resume operation");
            f.db.write([&](const auto &s) {
                const auto remaining =
                    SqliteMilestoneGroupDecisions(s).active_holds(f.app_occurrence);
                require(remaining ==
                            std::vector{MilestoneHoldKey{MilestoneHoldCause::deferred_acceptance,
                                                         "milestone-decision"}},
                        "Explicit Resume leaves the independent protected Defer intact");
            });
            auto reordered = resume(key.operation, {h2, h1});
            exact_same_result(commit(f.db, port, reordered, Disposition::replay), first);
            auto changed = resume(key.operation, {h1});
            (void)commit(f.db, port, changed, Disposition::conflict);
            (void)commit(f.db, port, resume("fresh-same-causes", {h1, h2}), Disposition::refused);
            (void)commit(f.db, port, stop(key.operation, edited(f)), Disposition::conflict);
            require(saved == causes(f.db) && journal(f.db).size() == 4,
                    "Fresh refusal and authorized retry preserve the later independent Hold");
            same_redaction(f.db, port, key, changed, first);
            port.disclosure = true;
            const auto subset = resume("resume-only-H3", {c.added_causes[0]});
            const auto last = commit(f.db, port, subset, Disposition::committed);
            const auto closed = causes(f.db);
            require(last.resolved_causes == c.added_causes && closed.size() == 3 &&
                        closed.mid(0, 2) == saved.mid(0, 2) &&
                        closed[2][2].toByteArray() == subset.operation,
                    "Fresh authorized subset resolves only the one remaining interruption cause");
        });
    }

    void latePlaybackIsSeparateFromImmutableStop() {
        check([] {
            Populated f({.audio_job = true});
            release_application(f);
            release_edit(f);
            Dispatch dispatch;
            f.db.write([&](const auto &s) {
                const auto started = claim_milestone_edit_job(s, f.job, 1, dispatch);
                require(started.disposition == DeliveryDisposition::changed && started.job &&
                            started.job->attempt == 2 && started.job->capture.action_ref == "audio",
                        "Exact active audio attempt is committed by the protected edit owner");
            });
            advance_fixture(f.db);
            Authority port;
            PlaybackInstance playing{edited(f),
                                     1,
                                     {},
                                     2,
                                     "audio-adapter",
                                     "desktop-audio-1",
                                     "runtime-1",
                                     StopCapability::supported};
            port.observed_playback = {playing};
            const auto cmd = stop("stop-playing", edited(f), QueueChoice::continue_queue);
            const auto first = commit(f.db, port, cmd, Disposition::committed);
            require(
                first.cancellations.empty() && first.playback_intents.size() == 1 &&
                    !first.playback_intents[0].isEmpty(),
                "Exact supported activation yields one intent and never cancels the started slot");

            PlaybackOwner owner;
            auto late = playing;
            late.activation = "desktop-audio-late";
            const auto stage = [&](const PlaybackInstance &instance, Disposition expected) {
                f.db.write([&](const auto &s) {
                    require(stage_late_playback(s, owner, {edit::track, cmd.operation, instance}) ==
                                expected,
                            "Exact late activation is selected against the same committed attempt");
                });
            };
            auto wrong = late;
            wrong.started_attempt = 3;
            stage(wrong, Disposition::refused);
            wrong = late;
            wrong.runtime_generation = "stale-generation";
            stage(wrong, Disposition::refused);
            stage(late, Disposition::committed);
            stage(late, Disposition::replay);
            const auto instances = f.db.rows("SELECT activation,generation,handoff "
                                             "FROM main.subathon_milestone_playback_instances "
                                             "ORDER BY activation");
            require(instances.size() == 2 && instances[0][0].toByteArray() == playing.activation &&
                        instances[1][0].toByteArray() == late.activation &&
                        instances[0][1].toByteArray() == playing.runtime_generation &&
                        instances[1][1].toByteArray() == playing.runtime_generation &&
                        instances[0][2].isNull() && instances[1][2].isNull(),
                    "Only two exact activation identities persist, both without a handoff claim");
            f.db.reopen(profile());
            exact_same_result(read(f.db, port, cmd), first);
            f.db.write([&](const auto &s) {
                const auto finished = finish_milestone_edit_job(
                    s, f.job, 2, 2, DeliveryEvidence::delivered, "matching-delivery-evidence");
                require(finished.disposition == DeliveryDisposition::changed && finished.job &&
                            finished.job->revision == 3,
                        "Stop preserves exact matching finish for the old already-started attempt");
            });
        });
    }

    void playbackHandoffAndOutcome_data() {
        QTest::addColumn<bool>("lost_claim_ack");
        QTest::newRow("acknowledged-claim") << false;
        QTest::newRow("lost-claim-ack") << true;
    }

    void playbackHandoffAndOutcome() {
        QFETCH(bool, lost_claim_ack);
        check([&] {
            Populated f({.audio_job = true});
            release_application(f);
            release_edit(f);
            Dispatch dispatch;
            f.db.write([&](const auto &s) {
                const auto claimed = claim_milestone_edit_job(s, f.job, 1, dispatch);
                require(claimed.disposition == DeliveryDisposition::changed && claimed.job &&
                            claimed.job->attempt == 2,
                        "Protected owner commits the exact audio attempt before interruption");
            });
            advance_fixture(f.db);
            Authority port;
            const PlaybackInstance live{edited(f),
                                        1,
                                        {},
                                        2,
                                        "audio-adapter",
                                        "owned-live-activation",
                                        "runtime-1",
                                        StopCapability::supported};
            port.observed_playback = {live};
            const auto cmd = stop("stop-audio", edited(f), QueueChoice::continue_queue);
            const auto saved = commit(f.db, port, cmd, Disposition::committed);
            require(saved.playback_intents.size() == 1 && !saved.playback_intents[0].isEmpty(),
                    "Normal Stop owns one exact adapter-specific intent");
            const PlaybackHandoff handoff{{edit::track, cmd.operation, live},
                                          saved.playback_intents[0]};
            PlaybackOwner owner;
            const auto claim = [&](const PlaybackHandoff &selected) {
                return run(f.db, [&](const auto &s) {
                    return Decision{stage_playback_handoff(s, owner, selected), {}, {}, {}, {}};
                });
            };
            const auto claimed_rows = [&] {
                return f.db.rows(
                    "SELECT typeof(handoff) FROM main.subathon_milestone_playback_instances"
                    " WHERE stop=? AND instance=?",
                    {cmd.operation, handoff.intent});
            };
            auto stale = handoff;
            stale.selected.instance.runtime_generation = "runtime-after-reconnect";
            const auto rejected = claim(stale);
            require(rejected.published && rejected.published->disposition == Disposition::refused &&
                        claimed_rows() == SqlRows{{QString("null")}},
                    "Current-generation mismatch never claims a retained intent");
            if (lost_claim_ack)
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Exact handoff SQL COMMIT can lose its normal acknowledgement");
            const auto acquired = claim(handoff);
            int adapter_calls = 0;
            if (acquired.published && acquired.published->disposition == Disposition::committed) {
                require(claimed_rows() == SqlRows{{QString("blob")}},
                        "Fake adapter sees the consuming handoff claim before entry");
                ++adapter_calls;
            }
            require(!acquired.missing, "STORAGE-027 NI: durable exact playback handoff");
            if (lost_claim_ack) {
                require(acquired.work.transaction == TransactionOutcome::outcome_unknown &&
                            acquired.work.error == WorkError::uncertain_outcome &&
                            !acquired.published && adapter_calls == 0,
                        "Lost handoff acknowledgement publishes no invocation permission");
            } else {
                finance::normal(acquired.work);
                require(adapter_calls == 1, "Normal claim permits precisely one fake adapter call");
            }

            f.db.reopen(profile());
            require(claimed_rows() == SqlRows{{QString("blob")}},
                    "Cold reopened intent retains its consuming handoff claim");
            const auto repeat = claim(handoff);
            require(repeat.published && (repeat.published->disposition == Disposition::replay ||
                                         repeat.published->disposition == Disposition::refused),
                    "A durable handoff claim never grants another adapter invocation");
            exact_same_result(read(f.db, port, cmd), saved);

            const auto signal = [&](PlaybackOutcome outcome, const QByteArray &evidence) {
                f.db.write([&](const auto &s) {
                    require(stage_playback_outcome(s, owner, {handoff, outcome, evidence}) ==
                                Disposition::committed,
                            "Exact owned outcome appends independent playback evidence");
                });
            };
            const auto uncertain = QByteArray("owner-did-not-observe-adapter-ack");
            signal(PlaybackOutcome::uncertain, uncertain);
            const auto observed = QByteArray("authoritative-exact-activation-stopped");
            signal(PlaybackOutcome::confirmed_stopped, observed);
            require(f.db.rows("SELECT evidence FROM main.subathon_milestone_playback_evidence "
                              "WHERE stop=? AND instance=? ORDER BY ordinal",
                              {cmd.operation, handoff.intent}) == SqlRows{{uncertain}, {observed}},
                    "Uncertain and later exact confirmed outcome retain their append order");
            require(adapter_calls == (lost_claim_ack ? 0 : 1),
                    "Cold selection, replay and later evidence cannot trigger new delivery");
            exact_same_result(commit(f.db, port, cmd, Disposition::replay), saved);
        });
    }

    void bonusSelectionRace_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<int>("winner");
        for (bool edit : {false, true})
            for (int outcome = 0; outcome != 3; ++outcome) {
                const auto name =
                    QByteArray(edit ? "edit-" : "application-") +
                    std::array{"original-before-confirm", "zero-actual-original-is-started",
                               "replacement-after-original-rollback"}[outcome];
                QTest::newRow(name.constData()) << edit << outcome;
            }
    }

    void bonusSelectionRace() {
        QFETCH(bool, edit_origin);
        QFETCH(int, winner);
        check([&] {
            BonusProducer f(edit_origin);
            const auto initial = f.main();
            advance_fixture(f.db);
            const auto target = f.target();
            const Replacement patch{1, {MilestoneActionKind::bonus, "award", "award-20"}};
            const Command cmd{edit::track, "replace-bonus", edit::actor,
                              ReplaceIntent{target, {patch}}};
            Authority port;
            port.captures = {{patch, {}, {}, {}, 20000}};
            const auto preview = bind_replace(f.db, port, cmd);
            BonusPorts bonus(f, winner == 1);
            const MilestoneBonusKey original{f.occurrence, 1};
            const auto receipt = [&] {
                return f.db.rows(edit_origin ? "SELECT action_ordinal,effect_ref FROM "
                                               "main.subathon_milestone_edit_bonus_receipts"
                                             : "SELECT action_ordinal,effect_ref FROM "
                                               "main.subathon_milestone_bonus_receipts");
            };
            if (winner < 2) {
                f.db.write([&](const auto &s) {
                    SqliteMilestoneGroupStore store(s);
                    require(
                        store.apply_bonus(original, bonus, bonus) ==
                                MilestoneBonusDisposition::applied &&
                            store.apply_bonus(original, bonus, bonus) ==
                                MilestoneBonusDisposition::already_applied,
                        "Original bonus effect and receipt commit once in the same writer unit");
                });
                require(f.main() == initial + (winner == 0 ? 30000 : 0) &&
                            receipt() == SqlRows{{1, QByteArray("bonus-original")}},
                        "A committed exact receipt owns the actual +30 seconds or capped zero");
                (void)commit(f.db, port, cmd, Disposition::refused, preview.get());
                require(journal(f.db).isEmpty(),
                        "Even a zero-effect receipt makes the unused preview stale");
                return;
            }
            const auto rolled = qt_test_support::run_work(
                f.db.writer(),
                [&](auto context) {
                    const auto s = context.begin();
                    require(SqliteMilestoneGroupStore(s).apply_bonus(original, bonus, bonus) ==
                                MilestoneBonusDisposition::applied,
                            "Original bonus is staged before a controlled rollback");
                    s.rollback();
                    return 1;
                },
                10000);
            require(rolled.transaction == TransactionOutcome::rolled_back &&
                        rolled.error == WorkError::not_committed && !rolled.value &&
                        f.main() == initial && receipt().isEmpty(),
                    "Rolled-back original persists neither the effect nor its receipt");
            const auto selected = commit(f.db, port, cmd, Disposition::committed, preview.get());
            require(selected.versions.size() == 1, "Only one exact replacement is selected");
            f.db.write([&](const auto &s) {
                require(SqliteMilestoneGroupStore(s).apply_bonus(original, bonus, bonus) ==
                                MilestoneBonusDisposition::refused &&
                            apply_replacement_bonus(s, port, bonus, cmd, selected.versions[0]) ==
                                Disposition::committed &&
                            apply_replacement_bonus(s, port, bonus, cmd, selected.versions[0]) ==
                                Disposition::replay,
                        "Superseded original cannot apply; exact successor applies once");
            });
            require(f.main() == initial + 20000 && receipt().isEmpty() &&
                        f.db.rows("SELECT version,effect,actual "
                                  "FROM main.subathon_milestone_version_bonus") ==
                            SqlRows{{selected.versions[0].version, QByteArray("bonus-replacement"),
                                     20000}},
                    "Durable successor provenance keys the single actual +20-second effect");
        });
    }

    void stopLargeClosure() {
        check([] {
            Populated f({.large_slots = 256});
            const auto selected =
                f.db.rows("SELECT count(*),min(length(action_ref)),max(length(action_ref)),"
                          "count(DISTINCT action_ref) FROM main.subathon_milestone_actions");
            require(selected == SqlRows{{256, 4096, 4096, 256}},
                    "Real protected owner staged 256 distinct maximum-sized slot references");
            constexpr qsizetype per_ref = 8 + 4096;
            static_assert(255 * per_ref == 1046520 && 256 * per_ref == 1050624);
            static_assert(1048576 - 255 * per_ref == 2056 && 256 * per_ref - 1048576 == 2048);
            advance_fixture(f.db);
            const auto key = stop("wide-stop-A", application(f), QueueChoice::continue_queue);
            Authority port;
            auto limits = StorageLimits{64, 4096, 65536, 16777216, 8192, 1048576};
            f.db.reopen(limits);
            const auto attempt =
                run(f.db, [&](const auto &session) { return stage_command(session, port, key); });
            require(!attempt.missing, "STORAGE-027 NI: bounded Stop selection seam");
            const bool preflight = attempt.published &&
                                   attempt.published->disposition == Disposition::resource_limit &&
                                   attempt.published->cancellations.empty();
            const bool metered = attempt.work.transaction == TransactionOutcome::rolled_back &&
                                 attempt.work.error == WorkError::result_limit &&
                                 !attempt.published;
            require(
                preflight || metered,
                "STORAGE-027 T4: complete selected closure refuses at the default value budget");
            require(journal(f.db).isEmpty() && cancellations(f.db, key.operation).isEmpty() &&
                        causes(f.db).isEmpty() &&
                        f.db.rows("SELECT released FROM main.subathon_milestone_groups") ==
                            SqlRows{{0}},
                    "No retained prefix, cancellation, release or Hold escapes the failed unit");
            limits.result_value_bytes = 16777216;
            f.db.reopen(limits);
            const auto saved = commit(f.db, port, key, Disposition::committed);
            const auto cancelled = cancellations(f.db, key.operation);
            require(saved.cancellations.size() == 256 && cancelled.size() == 256 &&
                        saved.added_causes.empty(),
                    "The same legal full selection commits under a loose result-value budget");
            for (qsizetype i = 0; i < cancelled.size(); ++i)
                require(cancelled[i][0].toLongLong() == i + 1 &&
                            saved.cancellations[static_cast<std::size_t>(i)].ordinal == i + 1,
                        "Every exact position appears once in cancellation readback");
        });
    }

    void attemptedRequeueCannotBeReplaced() {
        check([] {
            Populated f;
            release_application(f);
            release_edit(f);
            Dispatch dispatch;
            dispatch.allow_retry = true;
            f.db.write([&](const auto &s) {
                const auto claimed = claim_milestone_edit_job(s, f.job, 1, dispatch);
                require(claimed.disposition == DeliveryDisposition::changed && claimed.job &&
                            claimed.job->attempt == 2,
                        "Protected edit enters exact sending attempt at revision 2");
                const auto failed = finish_milestone_edit_job(
                    s, f.job, 2, 2, DeliveryEvidence::known_undelivered, "confirmed-not-sent");
                require(failed.disposition == DeliveryDisposition::changed && failed.job &&
                            failed.job->state == DeliveryJobState::failed &&
                            failed.job->revision == 3,
                        "Known-undelivered evidence is durably linked to attempt 2");
                const auto queued = retry_milestone_edit_job(s, f.job, 3, dispatch);
                require(queued.disposition == DeliveryDisposition::changed && queued.job &&
                            queued.job->state == DeliveryJobState::queued &&
                            queued.job->revision == 4,
                        "Protected retry requeues the same immutable original at revision 4");
            });
            const auto history = [&] {
                return f.db.rows(
                    "SELECT revision,state,attempt FROM main.subathon_milestone_edit_events "
                    "WHERE occurrence_ref=? ORDER BY revision",
                    {QByteArray("b-edit")});
            };
            const auto before = history();
            require(before.size() == 4 && before[0][0] == 1 && before[1][2] == 2 &&
                        before[2][2] == 2 && before[3][0] == 4 && before[3][2].isNull(),
                    "The newest queue state retains all earlier exact attempt evidence");
            advance_fixture(f.db);
            Authority owner;
            const auto replace_key = replace(f);
            f.db.write([&](const auto &s) {
                const auto selected = preview_replace(s, owner, replace_key);
                const auto *refused = std::get_if<Disposition>(&selected);
                require(refused && *refused == Disposition::refused,
                        "STORAGE-027 NI: the selected requeue at revision 4 is already started");
            });
            const auto request = stop("stop-requeued", edited(f), QueueChoice::continue_queue);
            const auto stopped = commit(f.db, owner, request, Disposition::committed);
            require(stopped.cancellations.empty() && stopped.added_causes.empty(),
                    "Stop fences requeued work without treating it as an unstarted cancellation");
            const auto calls = dispatch.selected;
            f.db.write([&](const auto &s) {
                require(claim_milestone_edit_job(s, f.job, 4, dispatch).disposition ==
                                DeliveryDisposition::refused &&
                            dispatch.selected == calls,
                        "Exact historical requeue cannot reach dispatch after the occurrence Stop");
            });
            require(
                history() == before &&
                    f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences") ==
                        SqlRows{{1}},
                "Stop preserves every attempt and the unchanged protected release bit");
        });
    }

    void boundedReferenceProjectionCountsFixedCells() {
        check([] {
            Populated f({.large_slots = 256});
            // The projection oracle is deliberately independent of command SQL:
            // three fixed BLOB cells, then ordinal + reference for each slot
            // across two reads. None of these rows is synthesised.
            // Original Review addresses its produced application as
            // review-application / review-<64 lowercase hex digits>.
            constexpr qsizetype fixed = (8 + 15) + (8 + 18) + (8 + 71);
            constexpr qsizetype member = 8 + 8 + 4096;
            constexpr qsizetype default_budget = 1048576;
            constexpr qsizetype cap = (default_budget - fixed) / member;
            static_assert(cap == 254 && fixed + 255 * member == 1048688);
            const auto projection = [&](qsizetype count, qsizetype value_bytes) {
                auto limits = StorageLimits{64, 64, 65536, 1048576, 1024, value_bytes};
                f.db.reopen(limits);
                return qt_test_support::run_work(
                    f.db.writer(),
                    [&](auto context) -> QVariant {
                        const auto s = context.begin();
                        const QByteArray key("sar2-occurrence");
                        const auto whole =
                            s.execute({"SELECT occurrence_ref,operation_scope,operation_ref "
                                       "FROM main.subathon_milestone_groups WHERE occurrence_ref=?",
                                       {key}});
                        const auto app_scope =
                            QByteArray::fromStdString(f.original.operation_scope);
                        const auto app_ref = QByteArray::fromStdString(f.original.operation_ref);
                        require(app_scope == "review-application" &&
                                    app_ref.startsWith("review-") && app_ref.size() == 71 &&
                                    whole == SqlRows{{key, app_scope, app_ref}},
                                "Exact retained Review application and occurrence charge the fixed "
                                "fields");
                        const auto chunk = [&](qsizetype start, qsizetype last) {
                            return s.execute({"SELECT action_ordinal,action_ref "
                                              "FROM main.subathon_milestone_actions "
                                              "WHERE occurrence_ref=? AND action_ordinal>? AND "
                                              "action_ordinal<=? "
                                              "ORDER BY action_ordinal",
                                              {key, qint64(start), qint64(last)}});
                        };
                        const auto left = chunk(0, 128);
                        const auto right = chunk(128, count);
                        require(
                            left.size() == 128 && right.size() == count - 128,
                            "Both complete non-overlapping reads return their actual slot counts");
                        s.commit();
                        return qint64(left.size() + right.size());
                    },
                    10000);
            };
            const auto fit = projection(cap, default_budget);
            finance::normal(fit);
            require(fit.value->toLongLong() == cap,
                    "Exact fixed and member counts admit 254 complete members at 1 MiB");
            const auto rejected = projection(cap + 1, default_budget);
            require(rejected.transaction == TransactionOutcome::rolled_back &&
                        rejected.error == WorkError::result_limit && !rejected.value,
                    "The 255th legal member exceeds the cumulative projection, not its local cap");
            const auto exact = projection(cap + 1, fixed + (cap + 1) * member);
            finance::normal(exact);
            require(exact.value->toLongLong() == cap + 1,
                    "Independently derived exact value budget admits the same complete selection");
            const auto short_by_one = projection(cap + 1, fixed + (cap + 1) * member - 1);
            require(short_by_one.transaction == TransactionOutcome::rolled_back &&
                        short_by_one.error == WorkError::result_limit && !short_by_one.value,
                    "Exact-minus-one returns no partial selected prefix");
        });
    }

    void invalidReplacementShapeAndTypedOrigin() {
        check([] {
            Populated f;
            advance_fixture(f.db);
            Authority port;
            const auto command = replace(f);
            (void)bind_replace(f.db, port, command);
            const auto evaluate = [&](Command candidate, Disposition expected) {
                f.db.write([&](const auto &s) {
                    const auto selected = preview_replace(s, port, candidate);
                    const auto *reason = std::get_if<Disposition>(&selected);
                    require(reason && *reason == expected,
                            "Unused preview refuses the complete invalid shape or authorization");
                });
            };
            auto empty = command;
            std::get<ReplaceIntent>(empty.intent).patch.clear();
            evaluate(empty, Disposition::refused);
            auto duplicate = command;
            auto &choices = std::get<ReplaceIntent>(duplicate.intent).patch;
            choices.push_back(choices.front());
            evaluate(duplicate, Disposition::refused);
            auto unknown = command;
            std::get<ReplaceIntent>(unknown.intent).patch[0].ordinal = 2;
            evaluate(unknown, Disposition::refused);
            auto wrong_origin = command;
            std::get<ReplaceIntent>(wrong_origin.intent).target.origin = f.original;
            evaluate(wrong_origin, Disposition::refused);
            auto converted = command;
            auto &conversion = std::get<ReplaceIntent>(converted.intent).patch[0];
            conversion.action = {MilestoneActionKind::bonus, "award", "award-20"};
            port.captures.push_back({conversion, {}, {}, {}, 20000});
            evaluate(converted, Disposition::refused);
            auto no_change = command;
            auto &unchanged = std::get<ReplaceIntent>(no_change.intent).patch[0];
            unchanged.action.version_ref = "announce-v1";
            port.captures.push_back(
                {unchanged, QByteArray::fromHex("0081"), "local/route", "edit-sender"});
            evaluate(no_change, Disposition::refused);
            port.command = false;
            evaluate(command, Disposition::denied);
            require(journal(f.db).isEmpty() &&
                        f.db.rows("SELECT count(*) FROM main.subathon_milestone_slot_versions") ==
                            SqlRows{{0}},
                    "No invalid shape reserves a key or persists any successor");
        });
    }

    void replaceRetainsOriginalAndFencesStart() {
        check([] {
            Populated f;
            release_application(f);
            release_edit(f);
            advance_fixture(f.db);
            Authority port;
            const auto key = replace(f);
            const auto preview = bind_replace(f.db, port, key);
            require(journal(f.db).isEmpty(), "Read-only preview reserves no operation");
            bool accepted = false;
            const auto staged = run(
                f.db,
                [&](const auto &session) {
                    const auto decision = stage_command(session, port, key, preview.get());
                    accepted = decision.disposition == Disposition::committed &&
                               decision.versions.size() == 1;
                    return decision;
                },
                Finish::rollback);
            require(accepted && staged.entered &&
                        staged.work.transaction == TransactionOutcome::rolled_back &&
                        !staged.published && journal(f.db).isEmpty(),
                    "Confirmed rollback has no selected successor or journal result");
            port.configuration = "later-config";
            (void)commit(f.db, port, key, Disposition::refused, preview.get());
            require(journal(f.db).isEmpty(), "Changed configuration does not rebase the preview");
            const auto fresh_preview = bind_replace(f.db, port, key);
            const auto first = commit(f.db, port, key, Disposition::committed, fresh_preview.get());
            require(first.versions.size() == 1 && first.versions[0].ordinal == 1 &&
                        !first.versions[0].version.isEmpty(),
                    "Exact bound confirmation selects one fresh scoped version");
            const auto selected = f.db.rows("SELECT action_version,payload,route,qualified_sender "
                                            "FROM main.subathon_milestone_slot_versions");
            require(selected == SqlRows{{QByteArray("announce-v2"), QByteArray::fromHex("0082"),
                                         QByteArray("local/new-route"), QByteArray("new-sender")}},
                    "Only the successor stores the new bytes, route and qualified sender");
            require(f.db.rows("SELECT revision,state FROM main.subathon_milestone_slot_attempts") ==
                        SqlRows{{1, 0}},
                    "Replacement automation has its own queued revision 1");
            Dispatch dispatch;
            f.db.write([&](const auto &session) {
                const auto captured = lookup_milestone_edit_job(session, f.job);
                require(captured && captured->revision == 1 &&
                            captured->state == DeliveryJobState::queued &&
                            captured->action_document == QByteArray::fromHex("0081"),
                        "Historical lookup still selects the immutable original");
                const auto attempt = claim_milestone_edit_job(session, f.job, 1, dispatch);
                require(attempt.disposition == DeliveryDisposition::refused &&
                            dispatch.selected == 0,
                        "Superseded original cannot reach its dispatch authority");
            });
            SuccessorDispatch new_dispatch;
            f.db.write([&](const auto &session) {
                require(claim_replacement(session, edited(f), first.versions[0], 1, new_dispatch) ==
                                Disposition::committed &&
                            new_dispatch.calls == 1,
                        "Exact current successor can enter sending under its own route and sender");
            });
            exact_same_result(commit(f.db, port, key, Disposition::replay, fresh_preview.get()),
                              first);
            require(f.db.rows("SELECT revision,state FROM main.subathon_milestone_slot_attempts "
                              "ORDER BY revision") == SqlRows{{1, 0}, {2, 1}},
                    "Replay after the successor starts retains exactly two revisions");
            require(journal(f.db).size() == 1 &&
                        f.db.rows("SELECT count(*) FROM main.subathon_milestone_slot_versions") ==
                            SqlRows{{1}},
                    "Replay leaves exactly one selected version and result");
        });
    }
};

QTEST_GUILESS_MAIN(Storage027CommandsTests)
#include "commands_tests.moc"
