// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_fixture.hpp"

#include <subathon/server/milestone_playback_stop.hpp>

namespace {
using namespace milestone_006_slice3;
using Release = MilestoneReleaseDisposition;
using Kind = MilestoneActionKind;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

const Replacement audio{1, {Kind::automation, "audio", "replacement-v2"}};
const Replacement chat{3, {Kind::automation, "chat", "replacement-v2"}};

struct BonusEffect final : MilestoneBonusAuthority, MilestoneBonusEffect {
    ContributionReceipt source;
    explicit BonusEffect(ContributionReceipt receipt) : source(std::move(receipt)) {}

    bool allows(const MilestoneBonusKey &key, const MilestoneAction &action) override {
        return key.occurrence == first && key.action_ordinal == bonus_ordinal &&
               action == MilestoneAction{Kind::bonus, "award", "bonus-v1"};
    }

    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &,
                      const MilestoneAction &) override {
        auto bundle = finance::File::receipt_application(s, source, "application-bonus");
        require(bundle.after.timer.lifecycle == subathon::core::TimerLifecycle::Active &&
                    bundle.after.time_debt_ms == 0,
                "Exact bonus fixture has headroom and zero debt");
        finance::File::retain_plan(
            s, bundle.intent.plan,
            finance::File::plain_capture(
                source, {}, {finance::request(finance::Unit::milliseconds, 30000, "main")}));
        bundle.effects.push_back(finance::addition(30000, 30000, 0, 0));
        bundle.after.timer.main_remaining_ms += 30000;
        require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                    SqliteBundleStageDisposition::staged,
                "Real application store stages the supplied +30-second effect");
        return "application-bonus";
    }
};

bool same_instance(const PlaybackInstance &left, const PlaybackInstance &right) {
    const auto *actual = std::get_if<ApplicationOperationKey>(&left.target.origin);
    const auto &expected = std::get<ApplicationOperationKey>(right.target.origin);
    return actual && finance::same(*actual, expected) && left.target.track == right.target.track &&
           left.target.occurrence == right.target.occurrence && left.ordinal == right.ordinal &&
           left.version == right.version && left.started_attempt == right.started_attempt &&
           left.adapter == right.adapter && left.activation == right.activation &&
           left.runtime_generation == right.runtime_generation &&
           left.capability == right.capability;
}

struct StopAdapter final : PlaybackStopAdapter, PlaybackAuthority {
    finance::File &file;
    const PlaybackInstance &expected;
    std::vector<PlaybackHandoff> calls;
    bool marker_visible{};
    int checked{};

    StopAdapter(finance::File &db, const PlaybackInstance &instance)
        : file(db), expected(instance) {}

    bool proves_current(const SqliteWorkSession &, const LatePlayback &candidate) override {
        ++checked;
        return candidate.track == expected.target.track &&
               same_instance(candidate.instance, expected);
    }

    void request_stop(PlaybackHandoff request) override {
        const auto stored = file.rows(
            "SELECT activation,generation,typeof(handoff) FROM "
            "main.subathon_milestone_playback_instances WHERE installation=? AND track=? "
            "AND stop=? AND instance=?",
            {QByteArray("install"), QByteArray("track"), request.selected.stop, request.intent});
        marker_visible =
            stored == SqlRows{{expected.activation, expected.runtime_generation, QString("blob")}};
        calls.push_back(std::move(request));
    }
};

void handoff_playback(SqliteWriter &writer, const PlaybackHandoff &handoff,
                      PlaybackAuthority &authority, PlaybackStopAdapter &adapter) {
    process_milestone_playback_turn(writer, MilestonePlaybackTurn::handoff, handoff, authority,
                                    adapter);
}

bool refused_preview(ApplicationFixture &f, Authority &owner, const Command &cmd) {
    bool refused = false;
    f.file.write([&](const auto &s) {
        const auto value = preview_replace(s, owner, cmd);
        const auto *disposition = std::get_if<Disposition>(&value);
        refused = disposition && *disposition == Disposition::refused;
    });
    return refused;
}

SqlRows releases(ApplicationFixture &f) {
    return f.file.rows("SELECT released FROM main.subathon_milestone_groups ORDER BY queue_order");
}

} // namespace

class Milestone006ApplicationTests final : public QObject {
    Q_OBJECT

  private slots:
    void r2R4ApplicationClaimInvalidatesMixedPreview() {
        check([] {
            ApplicationFixture f;
            require(f.release(f.a) == Release::released,
                    "A releases through its exact normal entry");
            Authority owner;
            const auto mixed = replacement(owner, f.a, "mixed-preview", {audio, chat});
            const auto old = f.preview(owner, mixed);
            const auto original = std::make_shared<OriginalAdapter>();
            require(dispatch(f, f.job(f.a, 3), original) == DeliveryDisposition::changed &&
                        original->starts.size() == 1 && original->starts.front().attempt == 2,
                    "A3 actually starts after the preview");
            const auto versions = [&] {
                return f.file.rows("SELECT version FROM main.subathon_milestone_slot_versions");
            };
            require(
                f.command(owner, mixed, old.get()).disposition == Disposition::refused &&
                    refused_preview(f, owner, mixed) && versions().empty() &&
                    f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal")
                        .empty(),
                "Started A3 stales the old preview and refuses a fresh mixed preview atomically");
            const auto exact = replacement(owner, f.a, "only-one", {audio});
            const auto bound = f.preview(owner, exact);
            const auto decided = f.command(owner, exact, bound.get());
            require(decided.disposition == Disposition::committed && decided.versions.size() == 1 &&
                        decided.versions.front().ordinal == 1 && versions().size() == 1,
                    "Fresh exact A1 preview remains eligible");
            const auto version = decided.versions.front();
            require(!version.version.isEmpty() &&
                        dispatch(f, f.job(f.a, 1), original) == DeliveryDisposition::refused &&
                        original->selections == 1,
                    "Replacement-first fences the original before authority");
            SuccessorAuthority dispatch_authority;
            f.file.write([&](const auto &s) {
                require(claim_replacement(s, f.a, version, 1, dispatch_authority) ==
                                Disposition::committed &&
                            claim_replacement(s, f.a, version, 1, dispatch_authority) ==
                                Disposition::refused &&
                            dispatch_authority.calls == 1,
                        "Only the first exact application successor claim enters authority");
                const auto state = lookup_replacement_attempt(s, f.a, version, 2);
                require(state && state->state == DeliveryJobState::sending,
                        "The selected application's sole r2 transition is sending");
            });
        });
    }

    void r4R5ApplicationPreviewRebindsCurrentGates_data() {
        QTest::addColumn<int>("intervening");
        QTest::newRow("unchanged-defer") << 0;
        QTest::newRow("resolve-and-release") << 1;
        QTest::newRow("stop-with-defer-active") << 2;
        QTest::newRow("equal-bytes-new-config-revision") << 3;
    }

    void r4R5ApplicationPreviewRebindsCurrentGates() {
        QFETCH(int, intervening);
        check([&] {
            ApplicationFixture f(false, true);
            Authority owner;
            owner.configuration = "C9";
            const auto request = replacement(owner, f.a, "bound-application", {audio});
            const auto old = f.preview(owner, request);
            const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance,
                                         "accept-" + first.ref};
            if (intervening == 1) {
                f.file.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).resolve(
                                first, defer, MilestoneHoldResolution::continue_actions,
                                {"operator", "continue-application-defer"}),
                            "Exact stored Defer resolves through #236");
                });
                require(f.release(f.a) == Release::released,
                        "A releases normally, with no sending claim");
            } else if (intervening == 2) {
                const auto stopped =
                    f.command(owner, stop("held-A", f.a, QueueChoice::continue_queue));
                require(stopped.disposition == Disposition::committed &&
                            stopped.cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                            stopped.added_causes.empty() && releases(f) == SqlRows{{0}, {0}},
                        "Stop cancels A1 while retaining unreleased held A");
            } else if (intervening == 3) {
                owner.configuration = "C10";
                (void)f.preview(owner, request);
                owner.configuration = "C11";
            }
            const auto versions = [&] {
                return f.file.rows("SELECT version FROM main.subathon_milestone_slot_versions");
            };
            if (intervening != 0) {
                const auto before =
                    f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal");
                require(f.command(owner, request, old.get()).disposition == Disposition::refused &&
                            versions().empty() &&
                            f.file.rows(
                                "SELECT operation FROM main.subathon_milestone_command_journal") ==
                                before,
                        "Changing committed release, exact Stop or current config revision stales "
                        "the old preview");
            }
            if (intervening != 2) {
                const auto bound = intervening == 0 ? old : f.preview(owner, request);
                const auto selected = f.command(owner, request, bound.get());
                require(selected.disposition == Disposition::committed &&
                            selected.versions.size() == 1 && versions().size() == 1,
                        "Unchanged or freshly bound prospects select precisely one unstarted slot");
            }
            if (intervening != 1)
                f.file.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).active_holds(first) ==
                                std::vector{defer},
                            "Replace and Stop preserve the independently retained Defer");
                });
        });
    }

    void r7ApplicationKindChangesAreAtomic() {
        check([] {
            ApplicationFixture f;
            Authority owner;
            const auto before = releases(f);
            const Replacement to_bonus{1, {Kind::bonus, "award", "bonus-v2"}};
            const Replacement to_automation{2, {Kind::automation, "chat", "v2"}};
            for (const auto &bad : {to_bonus, to_automation}) {
                const auto incompatible = replacement(owner, f.a, "mixed-kind", {chat, bad});
                if (bad.ordinal == 1)
                    owner.captures.back() = {bad, {}, {}, {}, 20000};
                require(
                    refused_preview(f, owner, incompatible) && releases(f) == before &&
                        f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal")
                            .empty() &&
                        f.file.rows("SELECT version FROM main.subathon_milestone_slot_versions")
                            .empty(),
                    "Either kind change rejects the otherwise-valid patch without writes");
            }
            const auto control = replacement(owner, f.b, "confirmable-B", {audio});
            require(bool(f.preview(owner, control)),
                    "B can independently preview the same slot patch");
            const auto command = replacement(owner, f.a, "real-application-patch", {audio});
            const auto preview = f.preview(owner, command);
            auto mismatched = command;
            std::get<ReplaceIntent>(mismatched.intent).target = f.b;
            require(f.command(owner, mismatched, preview.get()).disposition ==
                            Disposition::refused &&
                        f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal")
                            .empty(),
                    "A's retained preview cannot select independently eligible B");
            const auto good = f.command(owner, command, preview.get());
            require(good.disposition == Disposition::committed && good.versions.size() == 1 &&
                        good.versions.front().ordinal == 1 &&
                        f.file.rows("SELECT version FROM main.subathon_milestone_slot_versions")
                                .size() == 1 &&
                        releases(f) == before,
                    "Positive same-kind confirmation changes precisely A1, without release");
            f.file.write([&](const auto &s) {
                const auto retained = read_occurrence_selection(s, owner, stop("read-B", f.b));
                require(retained &&
                            retained->selected_versions == std::vector<SelectedVersion>{{1, {}}},
                        "Exact B retains its original selection");
            });
        });
    }

    void s5ApplicationHeadUsesExactTrackHold() {
        check([] {
            ApplicationFixture f(false);
            Authority owner;
            const auto stopped = f.command(owner, stop("hold-A", f.a));
            require(stopped.disposition == Disposition::committed &&
                        stopped.added_causes.size() == 1 &&
                        stopped.cancellations == std::vector<SelectedVersion>{{1, {}}},
                    "Exact A adds one Hold and cancels only its unstarted slot");
            require(f.release(f.b) == Release::refused && releases(f) == SqlRows{{0}, {0}},
                    "The later application head is blocked by the existing same-track cause");
            const auto resumed = f.command(owner, resume("resume-A", stopped.added_causes));
            require(resumed.disposition == Disposition::committed &&
                        resumed.resolved_causes == stopped.added_causes &&
                        f.release(f.b) == Release::released && releases(f) == SqlRows{{0}, {1}},
                    "Exact Resume unblocks only B; A remains fenced without release");
        });
    }

    void s1ApplicationStopRetainsFullHistory_data() {
        QTest::addColumn<bool>("recover");
        QTest::newRow("matching-finish") << false;
        QTest::newRow("stranded-recovery") << true;
    }

    void s1ApplicationStopRetainsFullHistory() {
        QFETCH(bool, recover);
        check([&] {
            ApplicationFixture f;
            require(f.release(f.a) == Release::released, "A releases via its application key");
            auto original = std::make_shared<OriginalAdapter>();
            for (qint64 ordinal : {1, 3, 4})
                require(dispatch(f, f.job(f.a, ordinal), original) == DeliveryDisposition::changed,
                        "Audio, chat and in-flight jobs each acquire sending r2");
            require(original->starts.size() == 3, "Only three originals start");
            const auto main = f.file.head().timer.main_remaining_ms;
            BonusEffect bonus(f.file.intake("application-bonus", {}, "synthetic-owner-command"));
            f.file.write([&](const auto &s) {
                require(SqliteDeliveryJobStore(s)
                                    .finish(f.job(f.a, 3), 2, 2, DeliveryEvidence::delivered,
                                            "chat-delivered")
                                    .disposition == DeliveryDisposition::changed &&
                            SqliteMilestoneGroupStore(s).apply_bonus({first, bonus_ordinal}, bonus,
                                                                     bonus) ==
                                MilestoneBonusDisposition::applied,
                        "Chat settles and normal application writes the bonus and its receipt");
                require(SqliteMilestoneGroupStore(s).apply_bonus({first, bonus_ordinal}, bonus,
                                                                 bonus) ==
                            MilestoneBonusDisposition::already_applied,
                        "Exact bonus replay applies no second effect");
            });
            const auto history = [&] {
                return std::array{
                    f.file.rows("SELECT * FROM main.subathon_milestone_actions "
                                "ORDER BY occurrence_ref,action_ordinal"),
                    f.file.rows("SELECT * FROM main.subathon_delivery_events "
                                "ORDER BY operation_scope,operation_ref,job_ref,revision"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_bonus_receipts")};
            };
            const auto saved = history();
            require(saved[2].size() == 1 && f.file.head().timer.main_remaining_ms == main + 30000,
                    "Supplied bonus adds exactly 30,000 once with one receipt");
            Authority owner;
            const auto command = stop("application-S1", f.a);
            const PlaybackInstance instance{
                f.a, 1, {}, 2, "audio-adapter", "M", "generation-M", StopCapability::supported};
            owner.observed_playback = {instance};
            const auto stopped = f.command(owner, command);
            require(stopped.disposition == Disposition::committed &&
                        stopped.cancellations == std::vector<SelectedVersion>{{5, {}}, {6, {}}} &&
                        stopped.added_causes.size() == 1 && stopped.playback_intents.size() == 1 &&
                        history() == saved && f.file.head().timer.main_remaining_ms == main + 30000,
                    "Stop preserves all history and cancels only the two exact unstarted slots");
            reopen_financial(f.file);
            f.file.write([&](const auto &s) {
                const auto jobs = SqliteDeliveryJobStore(s);
                const auto key = f.job(f.a, 4);
                if (recover) {
                    const auto page = jobs.stranded({}, stranded_delivery_page_limit);
                    require(std::ranges::any_of(page.attempts,
                                                [&](const auto &attempt) {
                                                    return finance::same(attempt.key.operation,
                                                                         key.operation) &&
                                                           attempt.key.job_ref == key.job_ref &&
                                                           attempt.sending_revision == 2;
                                                }),
                            "Real stranded pager locates the stopped in-flight attempt");
                }
                const auto result =
                    recover ? jobs.recover(key, 2)
                            : jobs.finish(key, 2, 2, DeliveryEvidence::delivered, "exact-finish");
                require(result.disposition == DeliveryDisposition::changed && result.job &&
                            result.job->revision == 3 && result.job->attempt == 2 &&
                            result.job->state == (recover ? DeliveryJobState::outcome_unknown
                                                          : DeliveryJobState::delivered),
                        "Exact already-started attempt may finish or recover after Stop");
                const auto chat_record = jobs.lookup(f.job(f.a, 3));
                require(chat_record && chat_record->state == DeliveryJobState::delivered &&
                            chat_record->revision == 3,
                        "Already delivered chat history survives unchanged");
            });
        });
    }

    void s1FullHistoryAcknowledgesBeforePlayback() {
        check([] {
            ApplicationFixture f;
            require(f.release(f.a) == Release::released, "Real application A has released");
            auto original = std::make_shared<OriginalAdapter>();
            for (qint64 ordinal : {1, 3, 4})
                require(dispatch(f, f.job(f.a, ordinal), original) == DeliveryDisposition::changed,
                        "Only audio, chat and in-flight jobs start");
            const auto main = f.file.head().timer.main_remaining_ms;
            BonusEffect bonus(f.file.intake("application-bonus", {}, "synthetic-owner-command"));
            f.file.write([&](const auto &s) {
                require(SqliteDeliveryJobStore(s)
                                    .finish(f.job(f.a, 3), 2, 2, DeliveryEvidence::delivered,
                                            "chat-delivered")
                                    .disposition == DeliveryDisposition::changed &&
                            SqliteMilestoneGroupStore(s).apply_bonus({first, bonus_ordinal}, bonus,
                                                                     bonus) ==
                                MilestoneBonusDisposition::applied,
                        "Real chat and bonus receipts precede the interruption");
            });
            Authority owner;
            const auto stop_command = stop("application-S1-media", f.a);
            const PlaybackInstance instance{
                f.a, 1, {}, 2, "audio-adapter", "M", "generation-M", StopCapability::supported};
            owner.observed_playback = {instance};
            const auto stopped = f.command(owner, stop_command);
            require(stopped.disposition == Disposition::committed &&
                        stopped.cancellations == std::vector<SelectedVersion>{{5, {}}, {6, {}}} &&
                        stopped.added_causes.size() == 1 && stopped.playback_intents.size() == 1 &&
                        f.file.head().timer.main_remaining_ms == main + 30000,
                    "Normally acknowledged Stop produces exactly one owned media intent");
            const PlaybackHandoff handoff{{edit::track, stop_command.operation, instance},
                                          stopped.playback_intents.front()};
            StopAdapter adapter(f.file, instance);
            require(f.file.rows(
                        "SELECT typeof(handoff) FROM main.subathon_milestone_playback_instances") ==
                        SqlRows{{QString("null")}},
                    "Exact acknowledged intent is independently known to be unmarked");
            handoff_playback(*f.file.writer, handoff, adapter, adapter);
            handoff_playback(*f.file.writer, handoff, adapter, adapter);
            require(
                adapter.checked > 0 && adapter.calls.size() == 1 && adapter.marker_visible &&
                    adapter.calls.front().intent == handoff.intent &&
                    same_instance(adapter.calls.front().selected.instance, instance) &&
                    f.file.head().timer.main_remaining_ms == main + 30000 &&
                    f.file.rows("SELECT effect_ref FROM main.subathon_milestone_bonus_receipts") ==
                        SqlRows{{QByteArray("application-bonus")}},
                "Only M stops once after its marker; the captured effect is retained");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006ApplicationTests)
#include "application_tests.moc"
