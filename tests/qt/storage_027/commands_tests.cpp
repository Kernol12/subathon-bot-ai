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
    bool allows(const MilestoneEditJob &job) override {
        ++selected;
        return job.key.occurrence == edit::b && job.capture.sender_ref == "edit-sender";
    }
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override { return {}; }
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
