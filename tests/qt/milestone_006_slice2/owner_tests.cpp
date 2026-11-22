// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_027/command_fixture.hpp"

namespace {
using namespace storage_027_fixture;
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

void publish_dispatch(const AcknowledgedClaim &, DispatchSink &) {
    throw OwnerEntryMissing("R2: publish only the acknowledged exact successor claim");
}

struct PendingCaptureSelection {
    CommittedMilestoneGroup cached;
    OccurrenceSelection committed;
};

std::vector<ReplacementJob> select_effective_release(SqliteWriter &,
                                                     const PendingCaptureSelection &) {
    throw OwnerEntryMissing("R1: select effective capture for the exact cached occurrence");
}

enum class CurrentAuthority { absent, denied };
Disposition preview_at_owner(const Command &, CurrentAuthority) {
    throw OwnerEntryMissing("R7: classify absent authoritative facts before Replace admission");
}

struct PlaybackPort final : PlaybackAuthority {
    std::optional<QByteArray> current_generation{"generation-X"};
    std::vector<PlaybackHandoff> calls;

    bool proves_current(const SqliteWorkSession &, const LatePlayback &selection) override {
        const auto &instance = selection.instance;
        return selection.track == edit::track && instance.target.occurrence == edit::b &&
               instance.ordinal == 1 && instance.started_attempt == 2 &&
               instance.adapter == "audio-adapter" && instance.activation == "X" &&
               current_generation && *current_generation == instance.runtime_generation;
    }
};

enum class PlaybackTurn { handoff, cold_restore, late_activation };
void process_playback(SqliteWriter &, PlaybackTurn, const PlaybackHandoff &, PlaybackPort &) {
    throw OwnerEntryMissing("P1/P2/P3: reconcile exact playback at the owner turn");
}

struct ColdQueueState {
    std::vector<ScopedCause> holds;
    std::vector<SelectedVersion> selected;
    std::optional<ActiveMilestonePresentation> presentation;
    std::optional<PlaybackOutcome> prior_handoff;
};

ColdQueueState restore_committed_queue(const OccurrenceSelection &, const OccurrenceSelection &,
                                       const PlaybackHandoff &, PlaybackPort &) {
    throw OwnerEntryMissing("T2: restore exact selected version, Hold and marked playback");
}

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

    void r2AcknowledgedSuccessorDispatch() {
        check([] {
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

    void r7AbsentAuthorityIsUnavailable() {
        check([] {
            Populated f;
            advance_fixture(f.db);
            Authority owner;
            const auto command = replace(f);
            const auto positive = bind_replace(f.db, owner, command);
            require(bool(positive), "The exact positive authority reaches a normal preview");
            owner.command = false;
            (void)commit(f.db, owner, command, Disposition::denied, positive.get());
            require(journal(f.db).empty(), "Known denial has no write");
            require(
                preview_at_owner(command, CurrentAuthority::absent) == Disposition::unavailable &&
                    preview_at_owner(command, CurrentAuthority::denied) == Disposition::denied &&
                    journal(f.db).empty(),
                "Absent authoritative facts are unavailable, known denial is denied");
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
            PlaybackPort port;
            if (stale)
                port.current_generation = "generation-replacement";
            require(f.intents() == SqlRows{{1, QByteArray("X"), QByteArray("generation-X"),
                                            QString("null")},
                                           {2, QByteArray("Y"), QByteArray("generation-Y"),
                                            QString("null")}} &&
                        f.evidence().empty(),
                    "Stored X and unknown Y retain exact identities without any handoff");
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, f.exact, port);
            require(port.calls.size() == (stale ? 0U : 1U),
                    "Owner sends only to current X and never to the successor generation");
            if (!stale)
                require(port.calls.front().intent == f.exact.intent &&
                            port.calls.front().selected.instance.activation == "X" &&
                            port.calls.front().selected.instance.runtime_generation ==
                                "generation-X",
                        "The dispatched handle is bound to the exact old generation");
            else
                require(f.evidence() == SqlRows{{int(PlaybackOutcome::obsolete)}},
                        "The stale intent is retired without a call");
        });
    }

    void p2ExactLateActivationIsScheduledOnce() {
        check([] {
            PlaybackFixture f(false);
            PlaybackPort port;
            require(f.intents() ==
                        SqlRows{{2, QByteArray("Y"), QByteArray("generation-Y"), QString("null")}},
                    "Stop initially retains only the unknown disposition, no X intent");
            auto wrong = f.exact;
            wrong.selected.instance.started_attempt = 3;
            process_playback(f.seed.db.writer(), PlaybackTurn::late_activation, f.exact, port);
            process_playback(f.seed.db.writer(), PlaybackTurn::late_activation, f.exact, port);
            process_playback(f.seed.db.writer(), PlaybackTurn::late_activation, wrong, port);
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
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, late, port);
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, late, port);
            require(port.calls.size() == 1 && port.calls.front().intent == late.intent &&
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
            PlaybackPort port;
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
            process_playback(f.seed.db.writer(), PlaybackTurn::cold_restore, f.exact, port);
            require(port.calls.empty(), "Cold restore alone makes no adapter call");
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, f.exact, port);
            process_playback(f.seed.db.writer(), PlaybackTurn::handoff, f.exact, port);
            require(port.calls.size() == (!marked && known_live ? 1U : 0U) &&
                        read(f.seed.db, f.owner, f.command) == original,
                    "Only an unmarked currently proven instance can hand off once on a later turn");
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
    void t2ColdRestoreSelectionHoldAndMarkedPlayback() {
        check([] {
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

            PlaybackPort port;
            f.seed.db.write([&](const auto &s) {
                require(stage_playback_handoff(s, port, f.exact) == Disposition::committed,
                        "One exact handoff marker commits before the owner loses normal "
                        "acknowledgement");
            });
            f.seed.db.reopen(profile());
            const auto stop_result = read(f.seed.db, f.owner, f.command);
            const auto marked = f.intents();
            require(!marked.empty() && !marked.front().empty(),
                    "Cold playback read retains the exact marker row");
            require(stop_result.disposition == Disposition::replay &&
                        stop_result.added_causes.size() == 1 &&
                        stop_result.playback_intents == std::vector{f.exact.intent} &&
                        marked.front().at(3) == QString("blob"),
                    "Cold history retains a real Hold and marked playback intent");
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
            const auto restored = restore_committed_queue(*stopped, *future, f.exact, port);
            const ScopedCause only_cause{edit::track, stop_result.added_causes.front()};
            const std::vector<ScopedCause> expected_holds{only_cause};
            require(
                restored.holds == expected_holds && restored.selected == committed.versions &&
                    !restored.presentation &&
                    restored.prior_handoff == PlaybackOutcome::uncertain && port.calls.empty(),
                "Cold restore carries exact selection and Hold with uncertain handoff and no live "
                "window");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006OwnerTests)
#include "owner_tests.moc"
