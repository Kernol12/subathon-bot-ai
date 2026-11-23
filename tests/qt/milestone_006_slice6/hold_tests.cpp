// SPDX-License-Identifier: GPL-3.0-or-later
#include "../milestone_006/timeline_boundary.hpp"
#include "../milestone_006_slice3/application_fixture.hpp"

#include <subathon/server/milestone_playback_stop.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace milestone_006_slice3;
using namespace subathon::server::testing;
using namespace std::chrono_literals;
using Sample = std::chrono::milliseconds;
using Release = MilestoneReleaseDisposition;
using Kind = MilestoneActionKind;
using State = DeliveryJobState;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

bool same_target(const Occurrence &a, const Occurrence &b) {
    if (a.track != b.track || a.occurrence != b.occurrence || a.origin.index() != b.origin.index())
        return false;
    if (const auto *application = std::get_if<ApplicationOperationKey>(&a.origin))
        return finance::same(*application, std::get<ApplicationOperationKey>(b.origin));
    return std::get<MilestoneEditKey>(a.origin) == std::get<MilestoneEditKey>(b.origin);
}

bool same_playback(const PlaybackInstance &a, const PlaybackInstance &b) {
    return same_target(a.target, b.target) && a.ordinal == b.ordinal && a.version == b.version &&
           a.started_attempt == b.started_attempt && a.adapter == b.adapter &&
           a.activation == b.activation && a.runtime_generation == b.runtime_generation &&
           a.capability == b.capability;
}

template <class Database> struct StopAdapter final : PlaybackAuthority, PlaybackStopAdapter {
    Database &db;
    const PlaybackInstance current;
    const QByteArray operation;
    std::vector<PlaybackHandoff> calls;
    int current_checks{};
    bool committed_marker_at_call{};

    StopAdapter(Database &store, PlaybackInstance instance, QByteArray stop_operation)
        : db(store), current(std::move(instance)), operation(std::move(stop_operation)) {}

    bool proves_current(const SqliteWorkSession &, const LatePlayback &candidate) override {
        ++current_checks;
        return exact(candidate);
    }

    bool owns_evidence(const SqliteWorkSession &, const PlaybackEvidence &evidence) override {
        return exact(evidence.handoff.selected) && calls.size() == 1 &&
               evidence.handoff.intent == calls.front().intent &&
               evidence.outcome == PlaybackOutcome::confirmed_stopped &&
               evidence.evidence == "adapter-confirmed-exact-M";
    }

    void request_stop(PlaybackHandoff request) override {
        // The observer connection only sees committed state.
        const auto stored = intent_rows();
        committed_marker_at_call =
            stored == SqlRows{{request.intent, current.ordinal, QString("blob"), current.adapter,
                               current.activation, current.runtime_generation,
                               static_cast<int>(StopCapability::supported), QString("blob")}};
        calls.push_back(std::move(request));
    }

    bool exact(const LatePlayback &candidate) const {
        return candidate.track == current.target.track && candidate.stop == operation &&
               same_playback(candidate.instance, current);
    }

    SqlRows intent_rows() const {
        return db.rows("SELECT instance,ordinal,typeof(attempt),adapter,activation,generation,"
                       "capability,typeof(handoff) FROM main.subathon_milestone_playback_instances "
                       "WHERE installation=? AND track=? AND stop=? ORDER BY ordinal",
                       {QByteArray::fromStdString(current.target.track.installation_ref),
                        QByteArray::fromStdString(current.target.track.track_ref), operation});
    }

    void require_unmarked(const PlaybackHandoff &selected) const {
        require(intent_rows() ==
                        SqlRows{{selected.intent, current.ordinal, QString("blob"), current.adapter,
                                 current.activation, current.runtime_generation,
                                 static_cast<int>(StopCapability::supported), QString("null")}} &&
                    calls.empty(),
                "Only the exact playing M has an unmarked Stop intent");
    }

    void require_one_call(const PlaybackHandoff &selected) const {
        require(current_checks > 0 && calls.size() == 1 && committed_marker_at_call &&
                    calls.front().intent == selected.intent && exact(calls.front().selected),
                "One call for exact M observes its durable one-way marker before entry");
    }
};

void handoff_playback(SqliteWriter &, const PlaybackHandoff &, PlaybackAuthority &,
                      PlaybackStopAdapter &) {
    throw std::logic_error("S1/T2: hand off the exact acknowledged Hold playback intent");
}

enum class StopCompletionSource { command, readback };

void adopt_stop_completion(MilestoneTimeline &, Sample, const Command &, StopCompletionSource,
                           const SqliteWorkResult &, const std::optional<Decision> &) {
    throw std::logic_error("T1/T2: adopt the exact acknowledged Hold on the warm timeline");
}

std::vector<SelectedVersion> originals(qint64 slot_count) {
    std::vector<SelectedVersion> selected;
    for (qint64 ordinal = 1; ordinal <= slot_count; ++ordinal)
        selected.push_back({ordinal, {}});
    return selected;
}

void require_selection(const SqliteWorkSession &s, Authority &owner, const Command &cmd,
                       qint64 order, qint64 slot_count, const Decision *stopped = nullptr) {
    const auto selection = read_occurrence_selection(s, owner, cmd);
    require(selection && same_target(selection->target, std::get<StopIntent>(cmd.intent).target) &&
                selection->queue_order == order && selection->released && !selection->skipped &&
                selection->selected_versions == originals(slot_count) &&
                selection->independent_holds.empty(),
            "Exact typed occurrence retains release, original versions and independent gates");
    require(selection->stop == (stopped ? cmd.operation : QByteArray{}) &&
                selection->cancellations ==
                    (stopped ? stopped->cancellations : std::vector<SelectedVersion>{}) &&
                selection->interruption_holds ==
                    (stopped ? stopped->added_causes : std::vector<QByteArray>{}),
            "Selection exposes only the expected exact Stop, cancellations and Hold");
}

struct ExactEditRelease final : MilestoneEditReleaseAuthority {
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == edit::c;
    }
};

struct EditDispatch final : MilestoneEditDispatchAuthority {
    bool allows(const MilestoneEditJob &job) override {
        return job.key.occurrence == edit::c && job.capture.sender_ref == "edit-sender";
    }
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override { return {}; }
};

inline const MilestoneAction bonus_action{Kind::bonus, "prior-bonus", "bonus-v1"};

struct EditBonus final : MilestoneBonusAuthority, MilestoneBonusEffect {
    ContributionReceipt source;
    explicit EditBonus(ContributionReceipt receipt) : source(std::move(receipt)) {}

    bool allows(const MilestoneBonusKey &key, const MilestoneAction &action) override {
        return key.occurrence == edit::c && key.action_ordinal == 2 && action == bonus_action;
    }

    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &,
                      const MilestoneAction &) override {
        auto bundle = finance::File::receipt_application(s, source, "mixed-edit-bonus");
        require(bundle.after.timer.lifecycle == subathon::core::TimerLifecycle::Active &&
                    bundle.after.time_debt_ms == 0,
                "Real timer head is active with no debt before the supplied exact bonus");
        finance::File::retain_plan(
            s, bundle.intent.plan,
            finance::File::plain_capture(
                source, {}, {finance::request(finance::Unit::milliseconds, 30000, "main")}));
        bundle.effects.push_back(finance::addition(30000, 30000, 0, 0));
        bundle.after.timer.main_remaining_ms += 30000;
        require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                    SqliteBundleStageDisposition::staged,
                "The normal application owner stages precisely +30 seconds before the receipt");
        return "mixed-edit-bonus";
    }
};

const std::array<std::string, 6> action_refs{"audio",     "prior-bonus", "chat",
                                             "in-flight", "queued-1",    "queued-2"};

struct MixedEdit {
    predecessor::Populated protected_seed;
    CatalogFile &db = protected_seed.db;
    MilestoneEditIntent earn =
        edit::single("earn-mixed-edit", {"C", 96}, {{edit::c, MilestoneActionChoice::run}});
    Occurrence exact{edit::track, edit::c, earn.key};

    MixedEdit() {
        install_protected(protected_seed);
        protected_seed.install();
        const auto candidate = protected_seed.seed("mixed-edit-origin", Case::positive());
        auto review = protected_seed.service();
        const auto result = protected_seed.execute(
            review, protected_seed.bind("mixed-edit-origin", {candidate},
                                        protected_seed.preview(review, {candidate})));
        expected_row(protected_seed.reply(result), 1, ReviewDisposition::committed,
                     ReviewStatus::applied);
        const auto &outcome = protected_seed.reply(result).rows.front().outcome;
        require(outcome && outcome->facts && outcome->facts->bundle,
                "Protected #751 Review commits the exact preceding application");
        const auto application = *outcome->facts->bundle;
        const MilestoneOccurrenceKey preceding{"mixed-edit-preceding"};
        const subathon::core::MilestoneCompletion first{"target-95", "mixed-definition", 95, 97,
                                                        "ctx"};
        auto capture = edit::empty_c;
        const auto job_template = edit::capture_b(true).jobs.front();
        for (std::size_t i = 0; i < action_refs.size(); ++i) {
            if (i == 1) {
                capture.actions.push_back(bonus_action);
                continue;
            }
            const auto &ref = action_refs[i];
            capture.actions.push_back({Kind::automation, ref, "v1"});
            auto job = job_template;
            job.ordinal = static_cast<qint64>(i + 1);
            job.envelope.document = QByteArray::fromStdString("body-" + ref);
            job.capture.action_ref = ref;
            job.capture.action_version_ref = "v1";
            capture.jobs.push_back(std::move(job));
        }
        edit::Authority owner;
        owner.facts.captures = {std::move(capture)};
        db.write([&](const auto &s) {
            SqliteMilestoneGroupStore(s).stage(application, {{{first, {}, {}, preceding}, {}}});
            SqliteMilestoneGroupDecisions(s).record_acceptance(
                application, {PausedAcceptanceMode::normal,
                              {"operator", "mixed-edit-acceptance"},
                              {{preceding, MilestoneActionChoice::run}}});
            require(initialize_milestone_track(s, {edit::track,
                                                   "cfg",
                                                   "mixed-definition",
                                                   "mixed-prefix",
                                                   {{"target-95", 95}, {"C", 200}},
                                                   std::vector<MilestoneTrackSeed::Member>{
                                                       {first, application, preceding}}}),
                    "Real track owner enrolls the exact application prefix");
            const auto preview = preview_milestone_target_edit(s, owner, edit::actor, earn);
            edit::File::confirmable(preview);
            const auto saved =
                confirm_milestone_target_edit(s, owner, edit::actor, earn, *preview.confirmation);
            require(saved.result && saved.result->achievements.size() == 1 &&
                        saved.result->achievements.front().order == 2 &&
                        saved.result->achievements.front().group.occurrence == edit::c,
                    "Actual edit capture creates the exact six-slot occurrence at order two");
        });
        advance_fixture(db);
        const auto first_release = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
            return release_milestone_head(db.writer(), preceding, std::move(done));
        });
        finance::normal(first_release.work);
        const auto edit_release = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
            return release_milestone_head(db.writer(), edit::c,
                                          std::make_shared<ExactEditRelease>(), std::move(done));
        });
        finance::normal(edit_release.work);
        require(first_release.disposition == Release::released &&
                    edit_release.disposition == Release::released,
                "Normal v21 writer acknowledges both releases in order");
    }

    MilestoneEditJobKey job(qint64 ordinal) const { return {earn.key, edit::c, ordinal}; }

    SqlRows timer() {
        return db.rows("SELECT s.main_remaining_ms FROM main.subathon_app_head h "
                       "JOIN main.subathon_app_states s ON h.token=s.token");
    }

    SqlRows attempts() {
        return db.rows("SELECT action_ordinal,revision,state,coalesce(attempt,0),"
                       "coalesce(evidence_ref,X'') "
                       "FROM main.subathon_milestone_edit_events ORDER BY action_ordinal,revision");
    }

    auto captures_and_receipts() {
        return std::array{
            db.rows("SELECT * FROM main.subathon_milestone_edit_occurrences"),
            db.rows("SELECT * FROM main.subathon_milestone_edit_actions ORDER BY action_ordinal"),
            db.rows("SELECT * FROM main.subathon_milestone_edit_jobs ORDER BY action_ordinal"),
            db.rows("SELECT * FROM main.subathon_milestone_edit_captures ORDER BY action_ordinal"),
            db.rows("SELECT * FROM main.subathon_milestone_edit_bonus_receipts"),
            db.rows("SELECT * FROM main.subathon_milestone_slot_versions"),
            db.rows("SELECT * FROM main.subathon_milestone_slot_attempts")};
    }

    void require_job(const SqliteWorkSession &s, qint64 ordinal, State state, qint64 revision,
                     std::optional<qint64> attempt) const {
        const auto actual = lookup_milestone_edit_job(s, job(ordinal));
        const auto &ref = action_refs.at(static_cast<std::size_t>(ordinal - 1));
        require(actual && actual->key == job(ordinal) && actual->state == state &&
                    actual->revision == revision && actual->attempt == attempt &&
                    actual->capture.action_ref == ref &&
                    actual->capture.action_version_ref == "v1" &&
                    actual->capture.route_ref == "local/route" &&
                    actual->capture.sender_ref == "edit-sender" &&
                    actual->action_document == QByteArray::fromStdString("body-" + ref),
                "Normal exact job lookup preserves capture and the expected attempt state");
    }
};

struct Completion {
    SqliteWorkResult work;
    std::optional<Decision> published;
};

template <class Operation> Completion complete(SqliteWriter &writer, Operation operation) {
    std::optional<Decision> result;
    auto work = qt_test_support::run_work(
        writer,
        [&](auto ctx) -> QVariant {
            const auto s = ctx.begin();
            result = operation(s);
            s.commit();
            return 1;
        },
        10000);
    require(result.has_value(), "The exact owner operation ran in a real writer work item");
    if (work.transaction != TransactionOutcome::committed || work.error != WorkError::none ||
        !work.value)
        result.reset();
    return {work, std::move(result)};
}

struct ActualReleasePort final : MilestoneGroupRelease {
    ApplicationFixture &f;
    std::vector<MilestoneOccurrenceKey> releases;

    explicit ActualReleasePort(ApplicationFixture &fixture) : f(fixture) {}

    bool review_held(const subathon::core::MilestoneCompletion &) const override {
        throw std::logic_error("Warm Hold must use exact occurrence identity");
    }

    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &key) const override {
        if (key == first || key == later)
            return MilestoneHeadDisposition::ready;
        return {};
    }

    void release(CommittedMilestoneGroup group) override {
        require(group.occurrence == first || group.occurrence == later,
                "Only an enrolled exact application group can release");
        const auto &target = group.occurrence == first ? f.a : f.b;
        require(f.release(target) == Release::released, "Release commits once on the real writer");
        releases.push_back(target.occurrence);
    }
};

std::optional<ActiveMilestonePresentation> window(std::string ref, Sample elapsed) {
    return ActiveMilestonePresentation{std::move(ref), elapsed, 10s};
}

} // namespace

class Milestone006HoldTests final : public QObject {
    Q_OBJECT

  private slots:
    void s1MixedEditHoldPublishesOnlyExactMedia() {
        check([] {
            MixedEdit f;
            const auto before = f.timer();
            require(before.size() == 1 && before.front().size() == 1,
                    "The real timer has one head");
            const SqlRows with_bonus{{before.front().front().toLongLong() + 30000}};

            EditBonus bonus(f.protected_seed.intake("mixed-edit-bonus"));
            EditDispatch dispatch;
            f.db.write([&](const auto &s) {
                for (qint64 ordinal : {1, 3, 4}) {
                    const auto claimed = claim_milestone_edit_job(s, f.job(ordinal), 1, dispatch);
                    require(claimed.disposition == DeliveryDisposition::changed && claimed.job &&
                                claimed.job->attempt == 2 && claimed.job->revision == 2,
                            "Only audio, chat and the separate in-flight job actually start");
                }
                require(finish_milestone_edit_job(s, f.job(3), 2, 2, DeliveryEvidence::delivered,
                                                  "chat-sent-before-hold")
                                    .disposition == DeliveryDisposition::changed &&
                            SqliteMilestoneGroupStore(s).apply_bonus({edit::c, 2}, bonus, bonus) ==
                                MilestoneBonusDisposition::applied,
                        "The chat finish and actual +30-second bonus commit before Stop");
            });
            const auto cmd = stop("mixed-edit-hold", f.exact);
            StopAdapter media(f.db,
                              PlaybackInstance{f.exact,
                                               1,
                                               {},
                                               2,
                                               "audio-adapter",
                                               "M",
                                               "generation-M",
                                               StopCapability::supported},
                              cmd.operation);
            Authority owner;
            owner.observed_playback = {media.current};
            const auto retained = f.captures_and_receipts();
            const SqlRows prior_attempts{
                {1, 1, int(State::queued), 0, QByteArray("")},
                {1, 2, int(State::sending), 2, QByteArray("")},
                {3, 1, int(State::queued), 0, QByteArray("")},
                {3, 2, int(State::sending), 2, QByteArray("")},
                {3, 3, int(State::delivered), 2, QByteArray("chat-sent-before-hold")},
                {4, 1, int(State::queued), 0, QByteArray("")},
                {4, 2, int(State::sending), 2, QByteArray("")},
                {5, 1, int(State::queued), 0, QByteArray("")},
                {6, 1, int(State::queued), 0, QByteArray("")}};
            require(f.timer() == with_bonus && f.attempts() == prior_attempts &&
                        retained[2].size() == 5 && retained[3].size() == 5 &&
                        f.db.rows("SELECT occurrence_ref,action_ordinal,effect_ref FROM "
                                  "main.subathon_milestone_edit_bonus_receipts") ==
                            SqlRows{{QByteArray::fromStdString(edit::c.ref), 2,
                                     QByteArray("mixed-edit-bonus")}} &&
                        retained[5].empty() && retained[6].empty() && journal(f.db).empty() &&
                        causes(f.db).empty() && media.intent_rows().empty(),
                    "The initial history has two unstarted slots, one bonus and no Stop state");
            f.db.write([&](const auto &s) {
                require_selection(s, owner, cmd, 2, 6);
                f.require_job(s, 1, State::sending, 2, 2);
                f.require_job(s, 3, State::delivered, 3, 2);
                f.require_job(s, 4, State::sending, 2, 2);
                f.require_job(s, 5, State::queued, 1, {});
                f.require_job(s, 6, State::queued, 1, {});
            });

            const auto saved = commit(f.db, owner, cmd, Disposition::committed);
            require(saved.cancellations == std::vector<SelectedVersion>{{5, {}}, {6, {}}} &&
                        saved.added_causes.size() == 1 && !saved.added_causes.front().isEmpty() &&
                        saved.playback_intents.size() == 1 &&
                        !saved.playback_intents.front().isEmpty() && saved.versions.empty() &&
                        saved.resolved_causes.empty() && journal(f.db).size() == 1 &&
                        f.db.rows("SELECT cause,stop,typeof(resume) FROM "
                                  "main.subathon_milestone_interruption_causes") ==
                            SqlRows{{saved.added_causes.front(), cmd.operation, QString("null")}} &&
                        f.captures_and_receipts() == retained && f.attempts() == prior_attempts &&
                        f.timer() == with_bonus,
                    "Default Hold adds one Stop and one cause and cancels only original 5 and 6");
            const PlaybackHandoff exact{{edit::track, cmd.operation, media.current},
                                        saved.playback_intents.front()};
            media.require_unmarked(exact);
            const auto stopped_journal = journal(f.db);
            const auto stopped_causes = causes(f.db);
            f.db.write([&](const auto &s) {
                require_selection(s, owner, cmd, 2, 6, &saved);
                require(claim_milestone_edit_job(s, f.job(5), 1, dispatch).disposition ==
                                DeliveryDisposition::refused &&
                            claim_milestone_edit_job(s, f.job(6), 1, dispatch).disposition ==
                                DeliveryDisposition::refused,
                        "Both cancelled originals remain readable and can never start");
                const auto finished = finish_milestone_edit_job(
                    s, f.job(4), 2, 2, DeliveryEvidence::indeterminate, "exact-stranded-attempt");
                require(finished.disposition == DeliveryDisposition::changed && finished.job &&
                            finished.job->key == f.job(4) && finished.job->revision == 3 &&
                            finished.job->attempt == 2 &&
                            finished.job->state == State::outcome_unknown,
                        "Exact original attempt 4 settles after Stop with indeterminate evidence");
            });
            auto settled = prior_attempts;
            settled.insert(
                7, {4, 3, int(State::outcome_unknown), 2, QByteArray("exact-stranded-attempt")});
            auto replay = saved;
            replay.disposition = Disposition::replay;
            require(f.attempts() == settled && read(f.db, owner, cmd) == replay &&
                        f.captures_and_receipts() == retained && f.timer() == with_bonus,
                    "The separate attempt finish retains the exact Stop and all earlier history");
            media.require_unmarked(exact);

            handoff_playback(f.db.writer(), exact, media, media);
            media.require_one_call(exact);
            require(f.db.rows("SELECT evidence FROM main.subathon_milestone_playback_evidence "
                              "WHERE stop=? AND outcome=?",
                              {cmd.operation, int(PlaybackOutcome::confirmed_stopped)})
                            .empty() &&
                        read(f.db, owner, cmd) == replay,
                    "Request and commit do not prove a stopped adapter outcome");

            const QByteArray evidence("adapter-confirmed-exact-M");
            f.db.write([&](const auto &s) {
                require(stage_playback_outcome(
                            s, media, {exact, PlaybackOutcome::confirmed_stopped, evidence}) ==
                            Disposition::committed,
                        "Separate reliable evidence for the called instance commits normally");
            });
            const auto confirmation =
                f.db.rows("SELECT ordinal,evidence FROM main.subathon_milestone_playback_evidence "
                          "WHERE stop=? AND instance=? AND outcome=?",
                          {cmd.operation, exact.intent, int(PlaybackOutcome::confirmed_stopped)});
            require(confirmation.size() == 1 && confirmation.front()[0].toLongLong() >= 1 &&
                        confirmation.front()[1] == evidence,
                    "One exact independent adapter confirmation is retained");
            f.db.write([&](const auto &s) {
                const auto outcome = lookup_playback_evidence(s, owner, cmd, exact,
                                                              confirmation.front()[0].toLongLong());
                require(outcome && outcome->outcome == PlaybackOutcome::confirmed_stopped &&
                            outcome->evidence == evidence,
                        "The owning evidence reader returns exactly the adapter observation");
                require_selection(s, owner, cmd, 2, 6, &saved);
                f.require_job(s, 3, State::delivered, 3, 2);
                f.require_job(s, 4, State::outcome_unknown, 3, 2);
            });
            media.require_one_call(exact);
            require(read(f.db, owner, cmd) == replay && journal(f.db) == stopped_journal &&
                        causes(f.db) == stopped_causes && f.captures_and_receipts() == retained &&
                        f.attempts() == settled && f.timer() == with_bonus,
                    "Later media and delivery evidence cannot amend the stable Stop");
        });
    }

    void t1T2WarmHoldAndMediaReplay_data() {
        QTest::addColumn<bool>("resume_before_gap_end");
        QTest::newRow("resume-during-gap") << true;
        QTest::newRow("resume-after-gap") << false;
    }

    void t1T2WarmHoldAndMediaReplay() {
        QFETCH(bool, resume_before_gap_end);
        check([&] {
            ApplicationFixture f(false);
            ActualReleasePort release(f);
            MilestoneTimeline timeline({10s, 1s, false}, release);
            std::vector<CommittedMilestoneGroup> pending;
            f.file.write([&](const auto &s) {
                for (const auto &row : SqliteMilestoneGroupStore(s).pending())
                    pending.push_back(row.group);
            });
            require(pending.size() == 2 && pending[0].occurrence == first &&
                        pending[1].occurrence == later,
                    "Owning reader supplies exact ordered A and B");
            timeline.accept_committed(std::move(pending), 0ms);
            require(timeline.presentation() == window("A", 0ms) &&
                        release.releases == std::vector{first},
                    "A starts with a real normally acknowledged release at zero");
            const auto dispatch_port = std::make_shared<OriginalAdapter>();
            const auto key = f.job(f.a, 1);
            require(dispatch(f, key, dispatch_port) == DeliveryDisposition::changed &&
                        dispatch_port->starts.size() == 1 &&
                        dispatch_port->starts.front().key.operation.operation_ref ==
                            key.operation.operation_ref &&
                        dispatch_port->starts.front().key.job_ref == key.job_ref &&
                        dispatch_port->starts.front().attempt == 2,
                    "A's only exact audio job has a normally acknowledged sending attempt");
            timeline.advance(1800ms);
            require(timeline.presentation() == window("A", 1800ms),
                    "A survives the request sample");

            const auto cmd = stop("warm-hold-A", f.a);
            StopAdapter media(
                f.file,
                PlaybackInstance{
                    f.a, 1, {}, 2, "audio-adapter", "M", "generation-M", StopCapability::supported},
                cmd.operation);
            Authority owner;
            owner.observed_playback = {media.current};
            owner.presentation = PresentationObservation{
                f.a, "A-window", "A-generation", "A-generation", "A-revision",
                0,   10000,      2000,           "A-revision",   1800};
            const auto releases = [&] {
                return f.file.rows(
                    "SELECT released FROM main.subathon_milestone_groups ORDER BY queue_order");
            };
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *f.file.writer, WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Lose normal acknowledgement for the exact Stop COMMIT");
            const auto attempt = complete(*f.file.writer, [&](const auto &s) {
                const auto staged = stage_command(s, owner, cmd);
                require(staged.disposition == Disposition::committed &&
                            staged.added_causes.size() == 1 &&
                            staged.playback_intents.size() == 1 && staged.cancellations.empty(),
                        "The exact default Hold and one intent reach the real writer");
                return staged;
            });
            require(attempt.work.transaction == TransactionOutcome::outcome_unknown &&
                        attempt.work.error == WorkError::uncertain_outcome && !attempt.work.value &&
                        !attempt.published && media.calls.empty(),
                    "The uncertain work item publishes neither Stop nor handoff permission");
            timeline.advance(2000ms);
            require(timeline.presentation() == window("A", 2000ms) &&
                        release.releases == std::vector{first},
                    "A is still presenting at the unacknowledged sample");

            reopen_financial(f.file);
            const auto acknowledged = complete(*f.file.writer, [&](const auto &s) {
                const auto selected = read_command(s, owner, cmd);
                require_selection(s, owner, cmd, 1, 1, &selected);
                return selected;
            });
            finance::normal(acknowledged.work);
            require(acknowledged.published &&
                        acknowledged.published->disposition == Disposition::replay &&
                        acknowledged.published->added_causes.size() == 1 &&
                        !acknowledged.published->added_causes.front().isEmpty() &&
                        acknowledged.published->cancellations.empty() &&
                        acknowledged.published->playback_intents.size() == 1 &&
                        !acknowledged.published->playback_intents.front().isEmpty() &&
                        releases() == SqlRows{{1}, {0}} && f.release(f.b) == Release::refused,
                    "Normal readback establishes one exact Hold and the real writer blocks B");
            const Decision stop_result = *acknowledged.published;
            const auto &hold = stop_result.added_causes.front();
            const PlaybackHandoff exact{{edit::track, cmd.operation, media.current},
                                        stop_result.playback_intents.front()};
            media.require_unmarked(exact);

            adopt_stop_completion(timeline, 2600ms, cmd, StopCompletionSource::readback,
                                  acknowledged.work, acknowledged.published);
            require(!timeline.presentation() && release.releases == std::vector{first},
                    "Normal readback interrupts active A on the 2,600 sample without media");
            handoff_playback(*f.file.writer, exact, media, media);
            media.require_one_call(exact);
            const auto unchanged = [&] {
                return std::array{
                    f.file.rows("SELECT * FROM main.subathon_milestone_stop_fences"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_stop_cancellations"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_interruption_causes"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_playback_instances"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_playback_evidence"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_command_journal "
                                "ORDER BY operation")};
            };
            const auto saved = unchanged();
            require(saved[0].size() == 1 && saved[1].empty() && saved[2].size() == 1 &&
                        saved[3].size() == 1 && saved[5].size() == 1,
                    "One real Stop, Hold and marked intent exist before replay");
            const auto retry = [&](Sample now) {
                const auto completion = complete(
                    *f.file.writer, [&](const auto &s) { return stage_command(s, owner, cmd); });
                finance::normal(completion.work);
                require(completion.published == acknowledged.published,
                        "Authorized exact replay publishes the original durable result");
                adopt_stop_completion(timeline, now, cmd, StopCompletionSource::command,
                                      completion.work, completion.published);
            };
            retry(3100ms);
            require(unchanged() == saved && releases() == SqlRows{{1}, {0}} &&
                        release.releases == std::vector{first} && !timeline.presentation(),
                    "Warm replay adds no new journal, Hold, marker, effect or release");
            media.require_one_call(exact);
            handoff_playback(*f.file.writer, exact, media, media);
            require(unchanged() == saved, "The next playback turn cannot spend the marker again");
            media.require_one_call(exact);

            const auto resume_sample = resume_before_gap_end ? 3200ms : 3800ms;
            if (!resume_before_gap_end) {
                timeline.advance(3600ms);
                timeline.advance(3799ms);
                require(!timeline.presentation() && release.releases == std::vector{first} &&
                            releases() == SqlRows{{1}, {0}},
                        "The exact Hold stays active after the interruption gap has elapsed");
            }
            const Command resume{edit::track, "resume-warm-hold", edit::actor,
                                 ResumeIntent{{{edit::track, hold}}}};
            const auto settled = complete(
                *f.file.writer, [&](const auto &s) { return stage_command(s, owner, resume); });
            finance::normal(settled.work);
            require(settled.published && settled.published->disposition == Disposition::committed &&
                        settled.published->resolved_causes == std::vector{hold} &&
                        f.file.rows("SELECT cause,stop,resume FROM "
                                    "main.subathon_milestone_interruption_causes") ==
                            SqlRows{{hold, cmd.operation, resume.operation}},
                    "Real Queue Resume resolves only the selected Hold exactly once");
            milestone_006_stage1::apply_owner_update(
                timeline, resume_sample,
                milestone_006_stage1::CommittedResume{{{edit::track, hold}}});
            const auto earliest = resume_before_gap_end ? 3600ms : 3800ms;
            if (resume_before_gap_end) {
                timeline.advance(3599ms);
                require(!timeline.presentation() && release.releases == std::vector{first},
                        "Clearing Hold during the gap cannot shorten the 3,600 boundary");
            }
            const std::vector expected_releases{first, later};
            const auto adopted = timeline.presentation();
            require((!adopted && release.releases == std::vector{first}) ||
                        (adopted == window("B", 0ms) && release.releases == expected_releases),
                    "B may start on an eligible Resume or the next advance at that sample");
            timeline.advance(earliest);
            require(timeline.presentation() == window("B", 0ms) &&
                        release.releases == expected_releases && releases() == SqlRows{{1}, {1}},
                    "B starts exactly once at the independently derived earliest sample");

            auto after_resume = saved;
            after_resume[2] =
                f.file.rows("SELECT * FROM main.subathon_milestone_interruption_causes");
            const auto resume_rows =
                f.file.rows("SELECT * FROM main.subathon_milestone_command_journal "
                            "WHERE operation=?",
                            {resume.operation});
            require(resume_rows.size() == 1, "Normal Resume adds only its own journal row");
            after_resume[5].prepend(resume_rows.front());
            retry(earliest + 100ms);
            const auto after_replay = timeline.presentation();
            require(after_replay == window("B", 0ms) || after_replay == window("B", 100ms),
                    "Replay leaves B visible whether or not its turn advances presentation");
            timeline.advance(earliest + 100ms);
            require(timeline.presentation() == window("B", 100ms) &&
                        release.releases == expected_releases && releases() == SqlRows{{1}, {1}} &&
                        unchanged() == after_resume,
                    "Same-row replay neither interrupts B nor restarts a gap or recreates Hold");
            media.require_one_call(exact);
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006HoldTests)
#include "hold_tests.moc"
