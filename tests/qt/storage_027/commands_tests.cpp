// SPDX-License-Identifier: GPL-3.0-or-later
#include "command_charge_oracle.hpp"
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

charge_oracle::Record fixture_record(CatalogFile &db, const Command &cmd, const Authority &owner,
                                     const Occurrence *target = nullptr) {
    charge_oracle::Record r;
    r.install = cmd.track.installation_ref.size();
    r.track = cmd.track.track_ref.size();
    r.operation = cmd.operation.size();
    r.actor = cmd.actor.actor_ref.size();
    r.lineage = cmd.actor.session_lineage.size();
    r.decision = owner.decision.size();
    if (!target)
        return r;
    r.occurrence = target->occurrence.ref.size();
    if (const auto *app = std::get_if<ApplicationOperationKey>(&target->origin)) {
        r.edited = false;
        r.origin_scope = app->operation_scope.size();
        r.origin = app->operation_ref.size();
    } else {
        r.origin = std::get<MilestoneEditKey>(target->origin).operation_ref.size();
    }
    const auto heads =
        db.rows("SELECT length(d.definition_revision),length(p.prefix_revision) "
                "FROM main.subathon_milestone_definition_heads d "
                "JOIN main.subathon_milestone_prefix_heads p USING(installation_ref,track_ref) "
                "WHERE d.installation_ref=? AND d.track_ref=?",
                {QByteArray::fromStdString(cmd.track.installation_ref),
                 QByteArray::fromStdString(cmd.track.track_ref)});
    require(heads.size() == 1,
            "Exact protected fixture publishes one current definition and prefix");
    r.definition = heads.front()[0].toLongLong();
    r.prefix = heads.front()[1].toLongLong();
    return r;
}

// Sample only representation widths in a deliberately rolled-back turn. The
// SQL tape and the assertions about selected durable effects remain independent.
template <class Inspect>
Decision inspect_command_shape(CatalogFile &db, Authority &owner, const Command &cmd,
                               const ReplacePreview *preview, Inspect inspect) {
    Decision sampled{Disposition::not_implemented, {}, {}, {}, {}};
    const auto probe = run(
        db,
        [&](const auto &session) {
            sampled = stage_command(session, owner, cmd, preview);
            if (sampled.disposition == Disposition::committed)
                inspect(session, sampled);
            return sampled;
        },
        Finish::rollback);
    require(!probe.missing, "STORAGE-027 NI: provisional owner representation for charge");
    require(probe.entered && probe.work.transaction == TransactionOutcome::rolled_back &&
                !probe.published && sampled.disposition == Disposition::committed,
            "Exact provisional owner output is sampled without reserving the operation");
    return sampled;
}

qsizetype stored_origin_width(const SqlRows &rows) {
    require(rows.size() == 1 && !rows.front()[0].isNull(),
            "Exact provisional owner row identifies its stored origin scope");
    return rows.front()[0].toLongLong();
}

qsizetype stop_origin_width(const SqliteWorkSession &s, const QByteArray &operation) {
    return stored_origin_width(s.execute(
        {"SELECT length(origin_scope) FROM main.subathon_milestone_stop_fences WHERE stop=?",
         {operation}}));
}

void require_member_delta(const charge_oracle::Charge &one, const charge_oracle::Charge &two,
                          const charge_oracle::Charge &member) {
    require(two.attempts - one.attempts == member.attempts &&
                two.peak == std::max(one.peak, member.peak) &&
                two.binds - one.binds == member.binds && two.rows - one.rows == member.rows &&
                two.results - one.results == member.results,
            "The independently planned second member adds its complete additive charge");
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

    void replaceAcknowledgementAfterReopen() {
        check([] {
            Populated f;
            advance_fixture(f.db);
            Authority owner;
            const auto key = replace(f);
            const auto fresh = [&] {
                auto unused = key;
                unused.operation = "fresh-replace-without-preview";
                return unused;
            }();
            auto bound = bind_replace(f.db, owner, key);
            (void)commit(f.db, owner, fresh, Disposition::refused);
            require(journal(f.db).isEmpty() &&
                        f.db.rows("SELECT version FROM main.subathon_milestone_slot_versions")
                            .isEmpty(),
                    "Eligible unused Replace needs its own preview and reserves no key without it");

            auto competitor = key;
            auto &different_slot = std::get<ReplaceIntent>(competitor.intent).patch.front();
            different_slot.action.version_ref = "announce-v3";
            Authority competitor_owner;
            competitor_owner.captures.front().selection = different_slot;
            const auto unrelated_preview = bind_replace(f.db, competitor_owner, competitor);

            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "The next actual Replace COMMIT loses its acknowledgement");
            std::optional<Decision> staged;
            const auto uncertain = run(f.db, [&](const auto &s) {
                staged = stage_command(s, owner, key, bound.get());
                return *staged;
            });
            require(!uncertain.missing, "STORAGE-027 NI: Replace acknowledgement command seam");
            require(staged && staged->disposition == Disposition::committed &&
                        staged->versions.size() == 1 && !staged->versions[0].version.isEmpty() &&
                        uncertain.entered && !uncertain.published && !uncertain.work.value &&
                        uncertain.work.transaction == TransactionOutcome::outcome_unknown &&
                        uncertain.work.error == WorkError::uncertain_outcome,
                    "Lost acknowledgement suppresses publication of the physically committed "
                    "selection");

            bound.reset();
            f.db.reopen(profile());
            Authority current;
            current.configuration = "later-owner-configuration";
            current.captures.clear();
            const auto state = [&] {
                return std::array{journal(f.db),
                                  f.db.rows("SELECT * FROM main.subathon_milestone_slot_versions"),
                                  f.db.rows("SELECT * FROM main.subathon_milestone_slot_attempts")};
            };
            const auto saved = state();
            require(saved[0].size() == 1 && saved[1].size() == 1 && saved[2].size() == 1,
                    "Cold reconciliation has one original journal entry, version and queued event");
            exact_same_result(read(f.db, current, key), *staged);
            exact_same_result(commit(f.db, current, key, Disposition::replay), *staged);
            exact_same_result(
                commit(f.db, current, key, Disposition::replay, unrelated_preview.get()), *staged);

            auto changed_origin = key;
            std::get<ReplaceIntent>(changed_origin.intent).target.origin = f.original;
            auto changed_target = key;
            std::get<ReplaceIntent>(changed_target.intent).target = application(f);
            for (const auto &changed : {competitor, changed_origin, changed_target}) {
                require(read(f.db, current, changed).disposition == Disposition::conflict,
                        "An authorized changed target or patch conflicts on cold readback");
                (void)commit(f.db, current, changed, Disposition::conflict,
                             unrelated_preview.get());
            }
            same_redaction(f.db, current, key, competitor, *staged);
            require(read(f.db, current, competitor) == read(f.db, current, fresh),
                    "Current disclosure denial redacts changed retained and unused readback alike");
            current.disclosure = true;
            current.command = false;
            require(read(f.db, current, key).disposition == Disposition::denied,
                    "Current execution authority also precedes exact retained readback");
            require(state() == saved,
                    "Cold replay, conflicts and denials preserve the original journal and history");
        });
    }

    void staleReplacementCannotAppendFirstSuccessor() {
        check([] {
            Populated f;
            advance_fixture(f.db);
            Authority winning_owner;
            const auto winning = replace(f);
            auto competing = winning;
            competing.operation = "competing-first-successor";
            auto &other_slot = std::get<ReplaceIntent>(competing.intent).patch.front();
            other_slot.action.version_ref = "announce-v3";
            Authority competing_owner;
            competing_owner.captures.front().selection = other_slot;
            const auto first_preview = bind_replace(f.db, winning_owner, winning);
            const auto stale_preview = bind_replace(f.db, competing_owner, competing);
            require(journal(f.db).isEmpty(),
                    "Both independent previews begin at the same original unstarted slot");

            const auto first =
                commit(f.db, winning_owner, winning, Disposition::committed, first_preview.get());
            require(first.versions.size() == 1 && first.versions[0].ordinal == 1 &&
                        !first.versions[0].version.isEmpty(),
                    "The first serialized writer selects one exact successor");
            const auto versions = [&] {
                return f.db.rows("SELECT version,predecessor,replacement FROM "
                                 "main.subathon_milestone_slot_versions");
            };
            const auto selected = versions();
            const auto saved_journal = journal(f.db);
            const auto attempts = [&] {
                return f.db.rows("SELECT version,revision,state FROM "
                                 "main.subathon_milestone_slot_attempts");
            };
            require(
                selected.size() == 1 && selected[0][0].toByteArray() == first.versions[0].version &&
                    selected[0][1].isNull() && selected[0][2].toByteArray() == winning.operation &&
                    saved_journal.size() == 1 &&
                    attempts() == SqlRows{{first.versions[0].version, 1, 0}},
                "The first successor links the original NULL predecessor and its own queued event");
            (void)commit(f.db, competing_owner, competing, Disposition::refused,
                         stale_preview.get());
            require(versions() == selected && saved_journal == journal(f.db) &&
                        attempts() == SqlRows{{first.versions[0].version, 1, 0}},
                    "Stale append creates no second first successor, journal key or queued event");
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
            // The existing Defer reference is absent from the interruption locator key space.
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
            (void)commit(f.db, port, resume("mixed-active-resolved", {c.added_causes[0], h1}),
                         Disposition::refused);
            require(causes(f.db) == saved,
                    "A real already-resolved member cannot consume a fresh active cause");
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

    void latePlaybackIsSeparateFromImmutableStop_data() {
        QTest::addColumn<bool>("recovery");
        QTest::newRow("matching-finish") << false;
        QTest::newRow("cold-stranded-recovery-after-stop") << true;
    }

    void latePlaybackIsSeparateFromImmutableStop() {
        QFETCH(bool, recovery);
        check([&] {
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
                const auto page = stranded_milestone_delivery(s, {}, 1);
                require(page.attempts.size() == 1 && page.attempts[0].revision == 2 &&
                            std::get_if<MilestoneEditJobKey>(&page.attempts[0].key) &&
                            std::get<MilestoneEditJobKey>(page.attempts[0].key) == f.job,
                        "Cold stranded traversal still selects the exact stopped edit attempt");
                if (recovery) {
                    const auto recovered = recover_milestone_edit_job(s, f.job, 2);
                    require(
                        recovered.disposition == DeliveryDisposition::changed && recovered.job &&
                            recovered.job->state == DeliveryJobState::outcome_unknown &&
                            recovered.job->revision == 3 && recovered.job->attempt == 2,
                        "Matching recovery reaches the stopped edit attempt without a new claim");
                    return;
                }
                const auto finished = finish_milestone_edit_job(
                    s, f.job, 2, 2, DeliveryEvidence::delivered, "matching-delivery-evidence");
                require(finished.disposition == DeliveryDisposition::changed && finished.job &&
                            finished.job->revision == 3,
                        "Stop preserves exact matching finish for the old already-started attempt");
            });
        });
    }

    void playbackScopesAndUnknownCapability() {
        check([] {
            Populated f({.third_target = true, .audio_job = true, .edit_slots = 3});
            const auto third = f.add_third_occurrence(true);
            release_application(f);
            release_edit(f);
            struct ExactOtherRelease final : MilestoneEditReleaseAuthority {
                bool allows(const SqliteWorkSession &, const MilestoneTrackKey &t,
                            const MilestoneOccurrenceKey &key) override {
                    return t == edit::track && key == edit::c;
                }
            };
            const auto released = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                return release_milestone_head(
                    f.db.writer(), edit::c, std::make_shared<ExactOtherRelease>(), std::move(done));
            });
            finance::normal(released.work);
            require(released.disposition == MilestoneReleaseDisposition::released,
                    "Protected owner releases the distinct later audio occurrence");
            struct ExactAudioClaim final : MilestoneEditDispatchAuthority {
                bool allows(const MilestoneEditJob &job) override {
                    return job.capture.action_ref == "audio" &&
                           job.capture.sender_ref == "edit-sender";
                }
                std::optional<std::string> retry_policy(const MilestoneEditJob &) override {
                    return {};
                }
            } authority;
            f.db.write([&](const auto &s) {
                for (const auto &key : {f.job, MilestoneEditJobKey{f.edit_intent.key, edit::b, 2},
                                        MilestoneEditJobKey{f.edit_intent.key, edit::b, 3},
                                        MilestoneEditJobKey{third, edit::c, 1}}) {
                    const auto claimed = claim_milestone_edit_job(s, key, 1, authority);
                    require(claimed.disposition == DeliveryDisposition::changed && claimed.job &&
                                claimed.job->attempt == 2 &&
                                claimed.job->action_document == QByteArray::fromHex("0081"),
                            "Every candidate has a real committed attempt of the same audio asset");
                }
            });
            advance_fixture(f.db);
            Authority port;
            const auto b = edited(f);
            const Occurrence other{edit::track, edit::c, third};
            port.observed_playback = {
                {b, 1, {}, 2, "audio-adapter", "exact-B", "runtime-1", StopCapability::supported},
                {b, 2, {}, 2, "audio-adapter", "unknown-B", "runtime-1", StopCapability::unknown},
                {other,
                 1,
                 {},
                 2,
                 "audio-adapter",
                 "same-asset-C",
                 "runtime-1",
                 StopCapability::supported}};
            const auto key = stop("routing-stop", b, QueueChoice::continue_queue);
            const auto result = commit(f.db, port, key, Disposition::committed);
            require(result.playback_intents.size() == 1 && result.cancellations.empty(),
                    "Only positively supported selected B instance offers an exact intent");
            const auto retained = f.db.rows(
                "SELECT ordinal,activation,capability,typeof(handoff) "
                "FROM main.subathon_milestone_playback_instances WHERE stop=? ORDER BY ordinal",
                {key.operation});
            require(retained.size() == 2 && retained[0][0] == 1 &&
                        retained[0][1].toByteArray() == "exact-B" && retained[1][0] == 2 &&
                        retained[1][1].toByteArray() == "unknown-B" &&
                        retained[0][2] != retained[1][2] && retained[0][3].toString() == "null" &&
                        retained[1][3].toString() == "null",
                    "Unknown capability stays distinct, the sending-only slot and other occurrence "
                    "do not receive a retained intent, and Stage 1 makes no handoff");
        });
    }

    void stopRejectsRealSkipAndFullyHistorical_data() {
        QTest::addColumn<bool>("was_skipped");
        QTest::newRow("real-prior-236-skip") << true;
        QTest::newRow("released-empty-and-no-live-observation") << false;
    }

    void stopRejectsRealSkipAndFullyHistorical() {
        QFETCH(bool, was_skipped);
        check([&] {
            Populated f({.held = was_skipped});
            if (was_skipped) {
                f.db.write([&](const auto &s) {
                    SqliteMilestoneGroupDecisions store(s);
                    require(store.resolve(
                                f.app_occurrence,
                                {MilestoneHoldCause::deferred_acceptance, "milestone-decision"},
                                MilestoneHoldResolution::skip, {"operator", "earlier-real-skip"}) &&
                                store.pending_disposition(f.app_occurrence) ==
                                    MilestonePendingDisposition::skipped,
                            "Genuine exact #236 choice records the prior terminal Skip");
                });
            } else {
                release_application(f);
            }
            const auto before = f.db.rows("SELECT released FROM main.subathon_milestone_groups");
            advance_fixture(f.db);
            Authority port;
            const auto key = stop("fresh-ineligible-stop", application(f));
            (void)commit(f.db, port, key, Disposition::refused);
            require(journal(f.db).empty() && causes(f.db).empty() &&
                        cancellations(f.db, key.operation).empty() &&
                        f.db.rows("SELECT released FROM main.subathon_milestone_groups") == before,
                    "Fresh refusal does not change terminal Skip or historical release evidence");
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
            port.historical_bonuses = {{target, {1, f.action}, 30000}};
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

    void presentationOnlyStopStorageGate_data() {
        QTest::addColumn<int>("evidence");
        QTest::newRow("exact-active-window") << 0;
        QTest::newRow("another-typed-occurrence") << 1;
        QTest::newRow("stale-generation") << 2;
        QTest::newRow("exact-expiry") << 3;
        QTest::newRow("stale-owner-revision") << 4;
        QTest::newRow("decreasing-observation") << 5;
    }

    void presentationOnlyStopStorageGate() {
        QFETCH(int, evidence);
        check([&] {
            Populated f;
            release_application(f); // A is released and owns no slots.
            advance_fixture(f.db);
            Authority port;
            const auto wanted = application(f);
            PresentationObservation observed{
                wanted, "presentation", "timeline-1", "timeline-1", "owner-1",
                0,      10000,          1800,         "owner-1",    1700};
            if (evidence == 1)
                observed.target = edited(f);
            if (evidence == 2)
                observed.current_generation = "timeline-2";
            if (evidence == 3)
                observed.observed_ms = observed.expiry_ms;
            if (evidence == 4)
                observed.current_owner_revision = "owner-2";
            if (evidence == 5)
                observed.observed_ms = observed.last_accepted_observation_ms - 1;
            port.presentation = observed;
            const auto key = stop("presentation-stop", wanted, QueueChoice::continue_queue);
            if (evidence == 0) {
                const auto stopped = commit(f.db, port, key, Disposition::committed);
                require(stopped.cancellations.empty() && stopped.playback_intents.empty() &&
                            stopped.added_causes.empty() && journal(f.db).size() == 1,
                        "Exact owner observation alone qualifies the released empty occurrence");
            } else {
                const auto reply =
                    run(f.db, [&](const auto &s) { return stage_command(s, port, key); });
                require(!reply.missing, "STORAGE-027 NI: exact presentation-only eligibility");
                require(reply.published && reply.published->disposition == Disposition::refused &&
                            journal(f.db).empty(),
                        "Current authority refuses ineligible presentation evidence without "
                        "reserving the command key");
            }
            require(
                f.db.rows("SELECT released FROM main.subathon_milestone_groups") == SqlRows{{1}},
                "Storage keeps the existing release bit and never starts a timeline transition");
        });
    }

    void stopVersusProtectedRelease_data() {
        QTest::addColumn<bool>("stop_first");
        QTest::newRow("stop-before-release") << true;
        QTest::newRow("release-before-stop") << false;
    }

    void stopVersusProtectedRelease() {
        QFETCH(bool, stop_first);
        check([&] {
            Populated f;
            release_application(f);
            if (!stop_first)
                release_edit(f);
            advance_fixture(f.db);
            Authority port;
            const auto cmd = stop("stop-release-race", edited(f), QueueChoice::continue_queue);
            const auto chosen = commit(f.db, port, cmd, Disposition::committed);
            require(
                chosen.cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                    chosen.added_causes.empty(),
                "Both serial contenders preserve precisely one original unstarted cancellation");
            if (stop_first) {
                const auto losing = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    return release_milestone_head(f.db.writer(), f.edit_occurrence,
                                                  std::make_shared<EditRelease>(), std::move(done));
                });
                finance::normal(losing.work);
                require(losing.disposition == MilestoneReleaseDisposition::refused,
                        "Committed Stop permanently fences the losing release");
            }
            require(f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences") ==
                        SqlRows{{stop_first ? 0 : 1}},
                    "Stop preserves whichever release bit was durable at its turn");
        });
    }

    void replaceExactMultiSlotSelection_data() {
        QTest::addColumn<bool>("started_middle");
        QTest::newRow("two-distinct-unstarted-slots") << false;
        QTest::newRow("reject-started-member-then-confirm-other-two") << true;
    }

    void replaceExactMultiSlotSelection() {
        QFETCH(bool, started_middle);
        check([&] {
            Populated f({.edit_slots = 3, .middle_bonus = !started_middle});
            const MilestoneEditJobKey middle{f.edit_intent.key, f.edit_occurrence, 2};
            if (started_middle) {
                release_application(f);
                release_edit(f);
                Dispatch port;
                f.db.write([&](const auto &s) {
                    const auto started = claim_milestone_edit_job(s, middle, 1, port);
                    require(started.disposition == DeliveryDisposition::changed && started.job &&
                                started.job->attempt == 2,
                            "Protected real middle slot has an exact sending attempt");
                });
            }
            const auto physical = [&] {
                return f.db.rows(
                    "SELECT released,queue_order FROM main.subathon_milestone_edit_occurrences");
            };
            const auto before = physical();
            advance_fixture(f.db);
            Authority port;
            auto cmd = replace(f);
            auto &patch = std::get<ReplaceIntent>(cmd.intent).patch;
            const auto action = patch.front().action;
            for (int n : {2, 3})
                port.captures.push_back(
                    {{n, action}, QByteArray::fromHex("0082"), "local/new-route", "new-sender"});
            if (started_middle) {
                patch.push_back({2, action});
                f.db.write([&](const auto &s) {
                    const auto result = preview_replace(s, port, cmd);
                    const auto *verdict = std::get_if<Disposition>(&result);
                    require(verdict && *verdict == Disposition::refused &&
                                *verdict != Disposition::not_implemented,
                            "STORAGE-027 NI: mixed started/unstarted selection refuses atomically");
                });
                require(journal(f.db).isEmpty(), "Invalid whole set reserves no operation");
                patch.pop_back();
            }
            // Reverse input order to establish canonical member order independently.
            patch.insert(patch.begin(), {3, action});
            const auto bound = bind_replace(f.db, port, cmd);
            if (!started_middle) {
                const auto reject = [&](Command wrong) {
                    f.db.write([&](const auto &s) {
                        const auto selected = preview_replace(s, port, wrong);
                        const auto *verdict = std::get_if<Disposition>(&selected);
                        require(
                            verdict && *verdict == Disposition::refused,
                            "A valid same-kind member cannot conceal either invalid kind change");
                    });
                    require(journal(f.db).isEmpty(),
                            "Mixed invalid selection is wholly provisional");
                };
                const Replacement over_bonus{2, action};
                port.captures.push_back({over_bonus, QByteArray("new"), "route", "sender"});
                auto mixed = cmd;
                auto &mixed_patch = std::get<ReplaceIntent>(mixed.intent).patch;
                mixed_patch[0] = over_bonus;
                reject(mixed);
                const Replacement over_automation{
                    3, {MilestoneActionKind::bonus, "prior-bonus", "bonus-v2"}};
                port.captures.push_back({over_automation, {}, {}, {}, 20000});
                mixed_patch[0] = over_automation;
                reject(mixed);
            }
            const auto chosen = commit(f.db, port, cmd, Disposition::committed, bound.get());
            require(chosen.versions.size() == 2 && chosen.versions[0].ordinal == 1 &&
                        chosen.versions[1].ordinal == 3 &&
                        chosen.versions[0].version != chosen.versions[1].version &&
                        physical() == before,
                    "Both changed same-kind captures commit atomically without touching middle or "
                    "order");
            const auto captured = f.db.rows(
                "SELECT v.ordinal,a.revision,a.state FROM main.subathon_milestone_slot_versions v "
                "JOIN main.subathon_milestone_slot_attempts a USING(installation,track,version) "
                "ORDER BY v.ordinal,a.revision");
            require(captured == SqlRows{{1, 1, 0}, {3, 1, 0}},
                    "Each selected slot owns a separate immutable queued starting revision");
            if (started_middle) {
                f.db.write([&](const auto &s) {
                    const auto unchanged = lookup_milestone_edit_job(s, middle);
                    require(unchanged && unchanged->revision == 2 &&
                                unchanged->action_document == QByteArray::fromHex("0081"),
                            "Unselected automation retains exact original capture and attempt");
                });
            } else {
                require(f.db.rows("SELECT kind,action_ref,version_ref "
                                  "FROM main.subathon_milestone_edit_actions WHERE "
                                  "occurrence_ref=? AND action_ordinal=2",
                                  {QByteArray("b-edit")}) ==
                            SqlRows{{1, QByteArray("prior-bonus"), QByteArray("bonus-v1")}},
                        "Unselected original bonus capture survives exact multi-slot replacement");
            }
        });
    }

    void replacementRebindAfterFactsChange_data() {
        QTest::addColumn<int>("intervention");
        QTest::newRow("held-and-unchanged") << 0;
        QTest::newRow("resolve-defer-and-release") << 1;
        QTest::newRow("exact-claim-before-confirmation") << 2;
        QTest::newRow("stop-before-confirmation") << 3;
        QTest::newRow("later-revision-with-identical-capture") << 4;
    }

    void replacementRebindAfterFactsChange() {
        QFETCH(int, intervention);
        check([&] {
            const bool held = intervention == 0 || intervention == 1 || intervention == 3;
            Populated f({.hold_edit = held});
            if (intervention == 2) {
                release_application(f);
                release_edit(f);
            }
            advance_fixture(f.db);
            Authority owner;
            const auto cmd = replace(f);
            const auto bound = bind_replace(f.db, owner, cmd);
            const auto selected_versions = [&] {
                return f.db.rows(
                    "SELECT ordinal,version FROM main.subathon_milestone_slot_versions");
            };
            const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance,
                                         "decision-earn-B"};
            if (intervention == 1) {
                f.db.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).resolve(
                                edit::b, defer, MilestoneHoldResolution::continue_actions,
                                {"operator", "resolved-bound-defer"}),
                            "Exact protected Defer resolves separately from the unused preview");
                });
                release_application(f);
                release_edit(f);
            } else if (intervention == 2) {
                Dispatch dispatcher;
                f.db.write([&](const auto &s) {
                    const auto chosen = claim_milestone_edit_job(s, f.job, 1, dispatcher);
                    require(chosen.disposition == DeliveryDisposition::changed && chosen.job &&
                                chosen.job->attempt == 2,
                            "Real selected predecessor reaches sending revision two");
                });
            } else if (intervention == 3) {
                const auto saved =
                    commit(f.db, owner,
                           stop("stop-stales-preview", edited(f), QueueChoice::continue_queue),
                           Disposition::committed);
                require(saved.cancellations == std::vector<SelectedVersion>{{1, {}}},
                        "Exact Stop cancelled the selected unstarted slot");
            } else if (intervention == 4) {
                // Each later revision retains byte-identical candidate captures.
                owner.configuration = "cfg-10";
                (void)bind_replace(f.db, owner, cmd);
                owner.configuration = "cfg-11";
            }

            if (intervention == 0) {
                const auto result = commit(f.db, owner, cmd, Disposition::committed, bound.get());
                require(result.versions.size() == 1 && selected_versions().size() == 1,
                        "Unchanged current Defer permits selected replacement");
            } else {
                const auto before = journal(f.db);
                (void)commit(f.db, owner, cmd, Disposition::refused, bound.get());
                require(selected_versions().isEmpty() && journal(f.db) == before,
                        "Stale unused preview cannot silently rebase or reserve its key");
                if (intervention == 1 || intervention == 4) {
                    const auto fresh = bind_replace(f.db, owner, cmd);
                    const auto selected =
                        commit(f.db, owner, cmd, Disposition::committed, fresh.get());
                    require(selected.versions.size() == 1 && selected_versions().size() == 1,
                            "Fresh preview binds the now-current unstarted owner state");
                }
            }
            if (intervention == 0 || intervention == 3)
                f.db.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).active_holds(edit::b) ==
                                std::vector{defer},
                            "Replace and Stop retain the independent original exact Defer");
                });
        });
    }

    void foreignResumeSet_data() {
        QTest::addColumn<bool>("other_install");
        QTest::newRow("other-track-in-current-installation") << false;
        QTest::newRow("other-installation-with-its-own-writer") << true;
    }

    void foreignResumeSet() {
        QFETCH(bool, other_install);
        check([&] {
            Populated local;
            const MilestoneTrackKey foreign{other_install ? "another-install" : "install",
                                            "other-track"};
            const MilestoneOccurrenceKey occurrence{"foreign-B"};
            const MilestoneEditKey foreign_edit{foreign, "foreign-edit"};
            std::unique_ptr<predecessor::Populated> external;
            if (other_install) {
                external = std::make_unique<predecessor::Populated>(95, 0, 105);
                install_protected(*external);
                external->install(foreign.installation_ref);
            }
            auto &db = other_install ? external->db : local.db;
            struct Owner final : MilestoneEditAuthority {
                MilestoneTrackKey scope;
                const MilestoneOccurrenceKey selected;
                explicit Owner(MilestoneTrackKey t, MilestoneOccurrenceKey o)
                    : scope(std::move(t)), selected(std::move(o)) {}
                std::optional<MilestoneEditFacts>
                select(const SqliteWorkSession &s, const MilestoneEditActor &actor,
                       const MilestoneEditIntent &intent) override {
                    if (intent.key.track != scope || actor.actor_ref != edit::actor.actor_ref ||
                        actor.session_lineage != edit::actor.session_lineage)
                        return std::nullopt;
                    const auto current = SqliteApplicationBundleStore(s).read_state();
                    if (!current)
                        return std::nullopt;
                    auto facts = edit::Authority{}.facts;
                    facts.lifecycle.timer = current->timer;
                    facts.captures = {{"B", selected, {}, {}, {}}};
                    return facts;
                }
            } owner(foreign, occurrence);
            db.write([&](const auto &s) {
                const MilestoneTrackSeed seed{
                    foreign,          "cfg",        "foreign-definition",
                    "foreign-prefix", {{"B", 200}}, std::vector<MilestoneTrackSeed::Member>{}};
                require(initialize_milestone_track(s, seed),
                        "Second exact track is created by its own protected owner");
                const MilestoneEditIntent intent{foreign_edit,
                                                 {edit::actor.actor_ref, "foreign-evidence"},
                                                 {{"B", 103}},
                                                 {{occurrence, MilestoneActionChoice::run}}};
                const auto preview = preview_milestone_target_edit(s, owner, edit::actor, intent);
                edit::File::confirmable(preview);
                const auto saved = confirm_milestone_target_edit(s, owner, edit::actor, intent,
                                                                 *preview.confirmation);
                require(saved.result && saved.result->achievements.size() == 1,
                        "The foreign cause has a genuine eligible occurrence in its own scope");
            });
            advance_fixture(local.db);
            if (other_install)
                advance_fixture(db);
            Authority port;
            const auto h1 =
                commit(local.db, port, stop("local-hold", edited(local)), Disposition::committed);
            port.permitted = foreign;
            const auto h2 =
                commit(db, port, stop("foreign-hold", {foreign, occurrence, foreign_edit}),
                       Disposition::committed);
            require(h1.added_causes.size() == 1 && h2.added_causes.size() == 1,
                    "Both independent real Stops publish one scoped cause");
            const auto local_holds = causes(local.db);
            const auto foreign_holds = causes(db);
            const auto before = journal(local.db);
            port.permitted = edit::track;
            auto request = resume("foreign-resume", {h1.added_causes[0]});
            std::get<ResumeIntent>(request.intent).causes.push_back({foreign, h2.added_causes[0]});
            (void)commit(local.db, port, request, Disposition::refused);
            require(causes(local.db) == local_holds && causes(db) == foreign_holds &&
                        journal(local.db) == before,
                    "A mixed set never resolves the valid local or valid foreign cause");
            std::get<ResumeIntent>(request.intent).causes.pop_back();
            port.command = false;
            (void)commit(local.db, port, request, Disposition::denied);
            port.command = true;
            port.disclosure = false;
            (void)commit(local.db, port, request, Disposition::denied);
            require(causes(local.db) == local_holds && journal(local.db) == before,
                    "Loss of Resume execution or disclosure grant leaves all exact causes active");
            port.disclosure = true;
            const auto accepted = commit(local.db, port, request, Disposition::committed);
            require(accepted.resolved_causes == h1.added_causes,
                    "Same unused key accepts the fresh exact authorized local-only set");
        });
    }

    void canonicalCommandFrames() {
        check([] {
            namespace wire = frame_oracle;
            const MilestoneTrackKey track{"i", "t"};
            const Occurrence target{track, {"o"}, MilestoneEditKey{track, "e"}};
            const auto stop_omitted = stop("k", target);
            const auto stop_hold = stop("k", target, QueueChoice::hold);
            const auto stop_continue = stop("k", target, QueueChoice::continue_queue);
            const auto exact = QByteArray::fromHex(
                "4d53493102 0000000169 0000000174 000000016b 02 0000000165 000000016f 01");
            require(wire::intent(stop_omitted) == exact && wire::intent(stop_hold) == exact &&
                        wire::intent(stop_continue) == exact.chopped(1) + char(2),
                    "Stop frame binds typed scope and normalizes omitted Hold byte-for-byte");
            const Command ordered{track, "k", edit::actor,
                                  ResumeIntent{{{track, "a"}, {track, "b"}}}};
            const Command reversed{track, "k", edit::actor,
                                   ResumeIntent{{{track, "b"}, {track, "a"}}}};
            const auto expected = QByteArray::fromHex(
                "4d53493103 0000000169 0000000174 000000016b 00000002 0000000161 0000000162");
            require(wire::intent(ordered) == expected && wire::intent(reversed) == expected,
                    "Resume frame sorts exact distinct same-scope byte references");
            auto invalid = reversed;
            std::get<ResumeIntent>(invalid.intent).causes[1].ref = "b";
            bool duplicate = false;
            try {
                (void)wire::intent(invalid);
            } catch (const std::invalid_argument &) {
                duplicate = true;
            }
            require(duplicate, "The encoder rejects duplicate causes before canonicalization");

            const MilestoneAction old{MilestoneActionKind::automation, "a", "1"};
            const MilestoneAction next{MilestoneActionKind::automation, "a", "2"};
            const Command replace{track, "k", edit::actor, ReplaceIntent{target, {{1, next}}}};
            const wire::PreviewWitness from_owner{
                "g",
                "c",
                "p",
                "q",
                true,
                {},
                {{1,
                  {},
                  1,
                  {old, QByteArray(1, char(0)), "q", "s", {}},
                  {next, QByteArray(1, char(0xff)), "u", "z", {}}}}};
            const auto exact_replace = QByteArray::fromHex(
                "4d53493101 0000000169 0000000174 000000016b 02 0000000165 000000016f "
                "0000000167 0000000163 0000000170 0000000171 01 00000000 00000001 "
                "0000000000000001 00 0000000000000001 "
                "01 0000000161 0000000131 0000000100 0000000171 0000000173 "
                "01 0000000161 0000000132 00000001ff 0000000175 000000017a");
            require(
                wire::intent(replace, &from_owner) == exact_replace,
                "Replace wire binds the whole exact preview with opaque old and new payload bytes");

            const Decision saved{Disposition::committed, {}, {{1, {}}}, {"h"}, {}};
            const auto expected_result = QByteArray::fromHex(
                "4d53523102 00000001 0000000000000001 00 00000001 0000000168 00000000");
            require(wire::result(wire::Kind::stop, saved) == expected_result,
                    "Stop outcome distinguishes original version and immutable Hold membership");
            auto replay = saved;
            replay.disposition = Disposition::replay;
            require(wire::result(wire::Kind::stop, replay) == expected_result,
                    "Normal disclosure disposition adds no bytes to retained success");
        });
    }

    void commandBudgetEdges_data() {
        QTest::addColumn<int>("branch");
        QTest::addColumn<int>("edge");
        using B = charge_oracle::Branch;
        const std::pair<const char *, B> cases[]{{"invalid-shape", B::malformed},
                                                 {"current-denied", B::denied},
                                                 {"absent", B::absent},
                                                 {"conflict", B::conflict},
                                                 {"replay", B::replay},
                                                 {"readback", B::readback},
                                                 {"preview-two", B::replace_preview},
                                                 {"replace-two", B::replace_automation},
                                                 {"stop-continue", B::stop_continue},
                                                 {"stop-hold", B::stop_hold},
                                                 {"resume-two", B::resume},
                                                 {"resume-ineligible", B::resume_ineligible}};
        constexpr std::array names{"exact",           "attempts-minus-one", "peak-minus-one",
                                   "binds-minus-one", "rows-minus-one",     "values-minus-one"};
        for (const auto &[label, branch] : cases) {
            if (branch == B::replace_automation || branch == B::stop_continue ||
                branch == B::resume)
                QTest::newRow((QByteArray(label) + "/member-over-cap").constData())
                    << int(branch) << 5;
            const bool early = branch == B::malformed || branch == B::denied;
            for (int n = 0; n < (early ? 1 : int(names.size())); ++n)
                QTest::newRow((QByteArray(label) + "/" + names[n]).constData())
                    << int(branch) << (n - 1);
        }
    }

    void commandBudgetEdges() {
        QFETCH(int, branch);
        QFETCH(int, edge);
        check([&] {
            namespace wire = frame_oracle;
            namespace meter = charge_oracle;
            using B = meter::Branch;
            const auto selected = static_cast<B>(branch);
            const bool is_replace =
                selected == B::replace_preview || selected == B::replace_automation;
            const bool is_resume = selected == B::resume || selected == B::resume_ineligible;
            const bool existing =
                selected == B::conflict || selected == B::replay || selected == B::readback;
            const bool excess = edge == 5;
            Populated f({.third_target = excess && is_resume, .edit_slots = excess ? 3 : 2});
            std::optional<MilestoneEditKey> third;
            if (excess && is_resume)
                third = f.add_third_occurrence();
            advance_fixture(f.db);
            auto cmd = stop("budget", edited(f),
                            selected == B::stop_continue ? QueueChoice::continue_queue
                                                         : QueueChoice::hold);
            Authority port;
            std::vector<meter::Slot> members(2, meter::Slot{8, 11, 2, 11, 11, 8, 11, 2, 15, 10});
            std::vector<meter::Cause> cause_shapes;
            std::optional<wire::PreviewWitness> preview_wire;
            std::shared_ptr<const ReplacePreview> bound;
            Decision expected{Disposition::committed, {}, {}, {}, {}};
            auto kind = wire::Kind::stop;
            const auto target = edited(f);
            auto r = fixture_record(f.db, cmd, port, &target);
            if (selected == B::malformed) {
                std::get<StopIntent>(cmd.intent).choice = static_cast<QueueChoice>(-1);
                expected.disposition = Disposition::refused;
            }
            if (selected == B::denied) {
                port.command = false;
                expected.disposition = Disposition::denied;
            }
            if (selected == B::absent) {
                std::get<StopIntent>(cmd.intent).target.occurrence.ref = "absent";
                expected.disposition = Disposition::refused;
            }
            if (selected == B::stop_continue || selected == B::stop_hold) {
                expected.cancellations = {{1, {}}, {2, {}}};
                const auto sample = inspect_command_shape(
                    f.db, port, cmd, nullptr, [&](const auto &s, const auto &) {
                        r.origin_scope = stop_origin_width(s, cmd.operation);
                    });
                if (selected == B::stop_hold) {
                    require(sample.added_causes.size() == 1 && !sample.added_causes[0].isEmpty(),
                            "Provisional Hold returns one exact opaque reference");
                    r.cause = sample.added_causes[0].size();
                    expected.added_causes = sample.added_causes;
                }
            }
            if (existing) {
                const auto prior = commit(f.db, port, cmd, Disposition::committed);
                require(prior.cancellations == std::vector<SelectedVersion>{{1, {}}, {2, {}}} &&
                            prior.added_causes.size() == 1 && !prior.added_causes[0].isEmpty(),
                        "Prepared journal peer has the independently counted original closure");
                r.retained_intent = wire::intent(cmd).size();
                r.retained_result = wire::result(kind, prior).size();
                expected = prior;
                expected.disposition =
                    selected == B::conflict ? Disposition::conflict : Disposition::replay;
                if (selected == B::conflict)
                    std::get<StopIntent>(cmd.intent).choice = QueueChoice::continue_queue;
            }
            if (is_replace) {
                kind = wire::Kind::replace;
                cmd = replace(f);
                auto &patch = std::get<ReplaceIntent>(cmd.intent).patch;
                patch.push_back({2, patch.front().action});
                port.captures.push_back(
                    {patch.back(), QByteArray::fromHex("0082"), "local/new-route", "new-sender"});
                const auto &old = f.edit_authority.facts.captures.front();
                preview_wire = wire::PreviewWitness{port.revision,
                                                    port.configuration,
                                                    "capability-1",
                                                    "prefix-edit-earn-B",
                                                    false,
                                                    {},
                                                    {}};
                members.clear();
                for (size_t i = 0; i < patch.size(); ++i) {
                    const auto &job = old.jobs.at(i);
                    const auto &capture = port.captures.at(i);
                    const wire::Capture before{old.actions.at(i),
                                               job.envelope.document,
                                               job.capture.route_ref,
                                               job.capture.sender_ref,
                                               {}};
                    const wire::Capture after{capture.selection.action,
                                              capture.payload,
                                              capture.route,
                                              capture.qualified_sender,
                                              {}};
                    preview_wire->members.push_back({patch[i].ordinal, {}, 1, before, after});
                    members.push_back(
                        {qsizetype(before.action.action_ref.size()),
                         qsizetype(before.action.version_ref.size()), before.automation.size(),
                         qsizetype(before.route.size()), qsizetype(before.sender.size()),
                         qsizetype(after.action.action_ref.size()),
                         qsizetype(after.action.version_ref.size()), after.automation.size(),
                         qsizetype(after.route.size()), qsizetype(after.sender.size())});
                }
                const auto calibration =
                    stop("inspect-edit-origin", target, QueueChoice::continue_queue);
                if (selected == B::replace_preview) {
                    (void)inspect_command_shape(
                        f.db, port, calibration, nullptr, [&](const auto &s, const auto &) {
                            r.origin_scope = stop_origin_width(s, calibration.operation);
                        });
                } else {
                    bound = bind_replace(f.db, port, cmd);
                    const auto sample = inspect_command_shape(
                        f.db, port, cmd, bound.get(), [&](const auto &s, const auto &) {
                            r.origin_scope = stored_origin_width(s.execute(
                                {"SELECT DISTINCT length(origin_scope) FROM "
                                 "main.subathon_milestone_slot_versions WHERE replacement=?",
                                 {cmd.operation}}));
                        });
                    require(sample.versions.size() == members.size(),
                            "Provisional Replace chooses a reference for every exact member");
                    for (size_t i = 0; i < members.size(); ++i) {
                        require(sample.versions[i].ordinal == patch[i].ordinal &&
                                    !sample.versions[i].version.isEmpty(),
                                "Provisional Replace exposes the exact selected ordinal");
                        members[i].allocated_version = sample.versions[i].version.size();
                    }
                    expected.versions = sample.versions;
                }
            }
            if (is_resume) {
                kind = wire::Kind::resume;
                const auto a_stop = stop("hold-a", application(f));
                const auto b_stop = stop("hold-b", edited(f));
                const auto a = commit(f.db, port, a_stop, Disposition::committed);
                const auto b = commit(f.db, port, b_stop, Disposition::committed);
                require(a.added_causes.size() == 1 && b.added_causes.size() == 1 &&
                            !a.added_causes[0].isEmpty() && !b.added_causes[0].isEmpty() &&
                            a.added_causes != b.added_causes,
                        "Both real Stop producers return distinct scoped Hold locators");
                auto second = b.added_causes[0];
                if (selected == B::resume_ineligible) {
                    second = "absent-interruption-ref";
                    if (second == a.added_causes[0] || second == b.added_causes[0])
                        second += QByteArray(1, char(0xff));
                    require(second != a.added_causes[0] && second != b.added_causes[0],
                            "The mixed set uses an identifier absent from both retained Holds");
                    expected.disposition = Disposition::refused;
                } else {
                    expected.resolved_causes = {a.added_causes[0], b.added_causes[0]};
                }
                cmd = resume("budget", {a.added_causes[0], second});
                cause_shapes = {
                    {a.added_causes[0].size(), a_stop.operation.size(),
                     qsizetype(std::get<StopIntent>(a_stop.intent).target.occurrence.ref.size())},
                    {second.size(), b_stop.operation.size(),
                     qsizetype(std::get<StopIntent>(b_stop.intent).target.occurrence.ref.size())}};
            }
            r.operation = cmd.operation.size();
            if (selected != B::malformed)
                r.intent = wire::intent(cmd, preview_wire ? &*preview_wire : nullptr).size();
            r.result = wire::result(kind, expected).size();
            const auto charge = meter::plan(selected, r, members, cause_shapes);
            if (selected == B::stop_continue || selected == B::resume ||
                selected == B::replace_automation) {
                auto one = r;
                auto input_one = cmd;
                auto decision_one = expected;
                auto one_preview = preview_wire;
                std::vector<meter::Slot> one_slot;
                std::vector<meter::Cause> one_cause;
                meter::Charge member;
                qsizetype intent_member{};
                qsizetype result_member{};
                if (selected == B::stop_continue) {
                    one_slot.push_back(members.front());
                    decision_one.cancellations.pop_back();
                    member = meter::stop_member(r, members.back());
                    result_member = 8 + 1; // Ordinal and absent version tag.
                } else if (selected == B::resume) {
                    one_cause.push_back(cause_shapes.front());
                    std::get<ResumeIntent>(input_one.intent).causes.pop_back();
                    decision_one.resolved_causes.pop_back();
                    member = meter::cause_member(r, cause_shapes.back(), true, true);
                    intent_member = 4 + cause_shapes.back().ref;
                    result_member = intent_member;
                } else {
                    one_slot.push_back(members.front());
                    std::get<ReplaceIntent>(input_one.intent).patch.pop_back();
                    one_preview->members.pop_back();
                    decision_one.versions.pop_back();
                    member = meter::replacement_member(r, members.back(), true);
                    const auto &entry = preview_wire->members.back();
                    const auto framed_capture = [](const wire::Capture &capture) {
                        return 1 + 5 * 4 + capture.action.action_ref.size() +
                               capture.action.version_ref.size() + capture.automation.size() +
                               capture.route.size() + capture.sender.size();
                    };
                    intent_member =
                        8 + 1 + 8 + framed_capture(entry.before) + framed_capture(entry.after);
                    result_member = 8 + 1 + 4 + members.back().allocated_version;
                }
                one.intent = wire::intent(input_one, one_preview ? &*one_preview : nullptr).size();
                one.result = wire::result(kind, decision_one).size();
                require(r.intent - one.intent == intent_member &&
                            r.result - one.result == result_member,
                        "Independent frame reduction measures the removed member's journal bytes");
                member.binds += intent_member + result_member;
                const auto one_charge = meter::plan(selected, one, one_slot, one_cause);
                require_member_delta(one_charge, charge, member);
            }
            auto limits = meter::limits(charge);
            if (excess) {
                // Same two-member controlling bind allowance, distinct
                // third real input and all other dimensions loose.
                limits = {64, 1024, 65536, charge.binds, 8192, 16777216};
                if (is_replace) {
                    auto &patch = std::get<ReplaceIntent>(cmd.intent).patch;
                    patch.push_back({3, patch.front().action});
                    port.captures.push_back({patch.back(), QByteArray::fromHex("0082"),
                                             "local/new-route", "new-sender"});
                    bound = bind_replace(f.db, port, cmd);
                } else if (is_resume) {
                    const auto c =
                        commit(f.db, port, stop("hold-c", {edit::track, edit::c, *third}),
                               Disposition::committed);
                    require(c.added_causes.size() == 1 && !c.added_causes[0].isEmpty(),
                            "Third exact distinct Stop produces its own legal active cause");
                    std::get<ResumeIntent>(cmd.intent)
                        .causes.push_back({edit::track, c.added_causes[0]});
                }
            } else if (edge >= 0) {
                limits = meter::one_below(charge, edge);
            }
            const auto old_journal = journal(f.db);
            const auto old_causes = causes(f.db);
            const auto old_cancellations = cancellations(f.db, cmd.operation);
            const auto old_versions =
                f.db.rows("SELECT * FROM main.subathon_milestone_slot_versions");
            const auto op = [&] {
                f.db.reopen(limits);
                const AdmissionProfile profile{limits, {}};
                return run(f.db, [&](const auto &s) {
                    if (selected == B::replace_preview) {
                        const auto selected_preview = preview_replace(s, port, cmd, &profile);
                        if (const auto *disposition = std::get_if<Disposition>(&selected_preview))
                            return Decision{*disposition, {}, {}, {}, {}};
                        return Decision{Disposition::committed, {}, {}, {}, {}};
                    }
                    if (selected == B::readback)
                        return read_command(s, port, cmd, &profile);
                    return stage_command(s, port, cmd, bound.get(), &profile);
                });
            }();
            require(!op.missing, "STORAGE-027 NI: independently charged exact branch seam");
            if (edge >= 0) {
                const bool refused =
                    op.published && op.published->disposition == Disposition::resource_limit;
                const bool rolled = op.work.transaction == TransactionOutcome::rolled_back &&
                                    (op.work.error == WorkError::resource_limit ||
                                     op.work.error == WorkError::result_limit) &&
                                    !op.published;
                require(
                    (refused || rolled) && journal(f.db) == old_journal &&
                        causes(f.db) == old_causes &&
                        cancellations(f.db, cmd.operation) == old_cancellations &&
                        f.db.rows("SELECT * FROM main.subathon_milestone_slot_versions") ==
                            old_versions,
                    "Insufficient complete charge refuses without partial journal or child rows");
                return;
            }
            require(op.published && op.published->disposition == expected.disposition,
                    "Complete independently charged profile reaches its exact outcome");
            if (selected != B::stop_hold && selected != B::stop_continue && selected != B::resume &&
                selected != B::replace_automation) {
                require(journal(f.db) == old_journal && causes(f.db) == old_causes,
                        "Preview, replay, readback and refusals leave durable command state exact");
                return;
            }
            const auto saved =
                f.db.rows("SELECT intent,result FROM main.subathon_milestone_command_journal "
                          "WHERE installation=? AND track=? AND operation=?",
                          {QByteArray("install"), QByteArray("track"), cmd.operation});
            require(saved == SqlRows{{wire::intent(cmd, preview_wire ? &*preview_wire : nullptr),
                                      wire::result(kind, *op.published)}} &&
                        saved[0][1].toByteArray().size() == r.result,
                    "Exact budget commits the independently framed complete immutable record");
        });
    }

    void bonusPreviewRequiresHistoricalRevision_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::newRow("application") << false;
        QTest::newRow("edit") << true;
    }

    void bonusPreviewRequiresHistoricalRevision() {
        QFETCH(bool, edit_origin);
        check([&] {
            BonusProducer f(edit_origin);
            advance_fixture(f.db);
            const auto target = f.target();
            const Replacement replacement{1, {MilestoneActionKind::bonus, "award", "award-20"}};
            const Command key{edit::track, "historical-bonus", edit::actor,
                              ReplaceIntent{target, {replacement}}};
            Authority owner;
            owner.captures = {{replacement, {}, {}, {}, 20000}};
            auto other_revision = f.action;
            other_revision.version_ref = "another-immutable-award-revision";
            owner.historical_bonuses = {{target, {1, other_revision}, 45000}};
            const auto initial = f.main();
            const auto expect_no_write = [&] {
                require(
                    f.main() == initial && journal(f.db).isEmpty() &&
                        f.db.rows("SELECT version FROM main.subathon_milestone_slot_versions")
                            .isEmpty() &&
                        f.db.rows("SELECT version FROM main.subathon_milestone_version_bonus")
                            .isEmpty() &&
                        f.db.rows(edit_origin ? "SELECT effect_ref FROM main."
                                                "subathon_milestone_edit_bonus_receipts"
                                              : "SELECT effect_ref FROM main."
                                                "subathon_milestone_bonus_receipts")
                            .isEmpty(),
                    "The missing historical revision changes no journal, version or bonus effect");
            };
            f.db.write([&](const auto &s) {
                const auto selected = preview_replace(s, owner, key);
                const auto *disposition = std::get_if<Disposition>(&selected);
                require(!disposition || *disposition != Disposition::not_implemented,
                        "STORAGE-027 NI: bonus preview with unavailable historical revision");
                require(disposition && *disposition == Disposition::refused &&
                            owner.historical_bonus_requests > 0,
                        "Missing exact revision refuses despite both a new amount and a nearby "
                        "revision");
            });
            expect_no_write();
            owner.historical_bonuses.push_back({target, {1, f.action}, 30000});
            const auto prior_calls = owner.historical_bonus_requests;
            const auto preview = bind_replace(f.db, owner, key);
            require(owner.historical_bonus_requests > prior_calls && preview,
                    "Restoring only the exact original revision makes the same preview eligible");
            expect_no_write();
        });
    }

    void bonusReplacementBudgetEdges_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<int>("edge");
        constexpr std::array names{"exact",           "attempts-minus-one", "peak-minus-one",
                                   "binds-minus-one", "rows-minus-one",     "values-minus-one"};
        for (const bool edit : {false, true})
            for (int n = 0; n < int(names.size()); ++n)
                QTest::newRow((QByteArray(edit ? "edit/" : "application/") + names[n]).constData())
                    << edit << (n - 1);
    }

    void bonusReplacementBudgetEdges() {
        QFETCH(bool, edit_origin);
        QFETCH(int, edge);
        check([&] {
            namespace meter = charge_oracle;
            namespace wire = frame_oracle;
            BonusProducer f(edit_origin);
            advance_fixture(f.db);
            const auto target = f.target();
            const Replacement patch{1, {MilestoneActionKind::bonus, "award", "award-20"}};
            const Command cmd{edit::track, "budget-bonus", edit::actor,
                              ReplaceIntent{target, {patch}}};
            Authority authority;
            authority.captures = {{patch, {}, {}, {}, 20000}};
            authority.historical_bonuses = {{target, {1, f.action}, 30000}};
            const auto preview = bind_replace(f.db, authority, cmd);
            const QByteArray queue = edit_origin ? "prefix-edit-bonus-edit" : "bonus-prefix";
            const wire::PreviewWitness witness{
                "grant-1",
                "cfg",
                "capability-1",
                queue,
                true,
                {},
                {{1, {}, 0, {f.action, {}, {}, {}, 30000}, {patch.action, {}, {}, {}, 20000}}}};
            const auto intent = wire::intent(cmd, &witness);
            require(authority.historical_bonus_requests > 0 &&
                        intent.endsWith(QByteArray::fromHex(
                            "02 00000005 6177617264 00000008 61776172642d3330 0000000000007530 "
                            "02 00000005 6177617264 00000008 61776172642d3230 0000000000004e20")),
                    "Exact old 30,000 and new 20,000 amounts occupy separate eight-byte capture "
                    "fields");
            auto r = fixture_record(f.db, cmd, authority, &target);
            const auto expected = inspect_command_shape(
                f.db, authority, cmd, preview.get(), [&](const auto &s, const auto &) {
                    const auto scope = stored_origin_width(s.execute(
                        {"SELECT length(origin_scope) FROM main.subathon_milestone_slot_versions "
                         "WHERE replacement=? AND ordinal=1",
                         {cmd.operation}}));
                    if (!edit_origin)
                        require(r.origin_scope == scope,
                                "Application version keeps the protected operation scope");
                    r.origin_scope = scope;
                });
            require(expected.versions.size() == 1 && expected.versions[0].ordinal == 1 &&
                        !expected.versions[0].version.isEmpty(),
                    "Provisional bonus selection has one exact typed version");
            r.intent = intent.size();
            r.result = wire::result(wire::Kind::replace, expected).size();
            const auto shape = meter::plan(
                meter::Branch::replace_bonus, r,
                {meter::Slot{qsizetype(f.action.action_ref.size()),
                             qsizetype(f.action.version_ref.size()), 0, 0, 0,
                             qsizetype(patch.action.action_ref.size()),
                             qsizetype(patch.action.version_ref.size()), sizeof(qint64), 0, 0, true,
                             false, 0, expected.versions[0].version.size()}});
            const auto limits = edge < 0 ? meter::limits(shape) : meter::one_below(shape, edge);
            f.db.reopen(limits);
            const AdmissionProfile admitted{limits, {}};
            const auto reply = run(f.db, [&](const auto &s) {
                return stage_command(s, authority, cmd, preview.get(), &admitted);
            });
            require(!reply.missing, "STORAGE-027 NI: exact counted same-kind bonus replacement");
            if (edge >= 0) {
                const bool refused =
                    reply.published && reply.published->disposition == Disposition::resource_limit;
                const bool rolled = reply.work.transaction == TransactionOutcome::rolled_back &&
                                    (reply.work.error == WorkError::resource_limit ||
                                     reply.work.error == WorkError::result_limit);
                require(
                    (refused || rolled) && journal(f.db).empty() &&
                        f.db.rows("SELECT * FROM main.subathon_milestone_slot_versions").empty(),
                    "One-below bonus preflight never publishes a partial selected version");
                return;
            }
            require(reply.published && reply.published->disposition == Disposition::committed &&
                        reply.published->versions.size() == 1 &&
                        reply.published->versions[0].ordinal == 1 &&
                        reply.published->versions[0].version.size() ==
                            expected.versions[0].version.size(),
                    "Complete budget retains one exact typed bonus version");
            require(
                f.db.rows("SELECT intent,result FROM main.subathon_milestone_command_journal") ==
                        SqlRows{{intent, wire::result(wire::Kind::replace, *reply.published)}} &&
                    f.db.rows("SELECT * FROM main.subathon_milestone_slot_attempts").empty(),
                "Bonus uses exact frozen framing and consumes no automation-attempt rows");
        });
    }

    void playbackBudgetEdges_data() {
        QTest::addColumn<int>("branch");
        QTest::addColumn<int>("edge");
        using B = charge_oracle::Branch;
        const std::pair<const char *, B> cases[]{{"initial-playing-stop", B::stop_started},
                                                 {"late-add", B::late_add},
                                                 {"late-duplicate", B::late_duplicate},
                                                 {"late-exhausted", B::late_exhausted},
                                                 {"handoff", B::handoff},
                                                 {"handoff-denied", B::handoff_denied},
                                                 {"outcome", B::outcome},
                                                 {"outcome-duplicate", B::outcome_duplicate}};
        constexpr std::array names{"exact",           "attempts-minus-one", "peak-minus-one",
                                   "binds-minus-one", "rows-minus-one",     "values-minus-one"};
        for (const auto &[label, branch] : cases)
            for (int n = 0; n < int(names.size()); ++n)
                QTest::newRow((QByteArray(label) + "/" + names[n]).constData())
                    << int(branch) << (n - 1);
    }

    void playbackBudgetEdges() {
        QFETCH(int, branch);
        QFETCH(int, edge);
        check([&] {
            namespace meter = charge_oracle;
            namespace wire = frame_oracle;
            using B = meter::Branch;
            const auto operation = static_cast<B>(branch);
            Populated f({.audio_job = true});
            release_application(f);
            release_edit(f);
            Dispatch dispatch;
            f.db.write([&](const auto &s) {
                const auto started = claim_milestone_edit_job(s, f.job, 1, dispatch);
                require(started.disposition == DeliveryDisposition::changed && started.job &&
                            started.job->attempt == 2,
                        "Protected replay fixture has a real exact audio attempt");
            });
            advance_fixture(f.db);
            const PlaybackInstance initial{
                edited(f),       1,          {},          2,
                "audio-adapter", "active-1", "runtime-1", StopCapability::supported};
            auto later = initial;
            later.activation = "active-2";
            const auto key = stop("bound-stop", edited(f), QueueChoice::continue_queue);
            Authority owner;
            owner.observed_playback = {initial};
            struct Port final : PlaybackAuthority {
                bool allowed{true};
                bool proves_current(const SqliteWorkSession &,
                                    const LatePlayback &candidate) override {
                    return allowed && candidate.instance.adapter == "audio-adapter" &&
                           candidate.instance.started_attempt == 2 &&
                           candidate.instance.runtime_generation == "runtime-1" &&
                           candidate.instance.activation.startsWith("active-");
                }
            } port;
            const PlaybackEvidence evidence{{{edit::track, key.operation, initial}, {}},
                                            PlaybackOutcome::requested,
                                            "exact-evidence"};
            const auto target = edited(f);
            auto r = fixture_record(f.db, key, owner, &target);
            r.intent = wire::intent(key).size();
            meter::Playback initial_shape{
                key.operation.size(), initial.adapter.size(), initial.activation.size(),
                initial.runtime_generation.size(), evidence.evidence.size()};
            auto late_shape = initial_shape;
            late_shape.activation = later.activation.size();
            QByteArray first_ref;
            QByteArray second_ref;
            auto selected_evidence = evidence;
            auto later_request = LatePlayback{edit::track, key.operation, later};
            const auto playback_shape = [&](const SqliteWorkSession &s, const PlaybackInstance &i) {
                const auto row = s.execute(
                    {"SELECT instance,length(attempt),length(handoff) FROM "
                     "main.subathon_milestone_playback_instances WHERE stop=? "
                     "AND ordinal=? AND adapter=? AND activation=? AND generation=?",
                     {key.operation, i.ordinal, i.adapter, i.activation, i.runtime_generation}});
                require(row.size() == 1 && !row[0][0].toByteArray().isEmpty(),
                        "Exact owner activation selects one opaque playback representation");
                return row.front();
            };
            const auto sample = inspect_command_shape(
                f.db, owner, key, nullptr, [&](const auto &s, const auto &staged) {
                    r.origin_scope = stop_origin_width(s, key.operation);
                    require(staged.playback_intents.size() == 1,
                            "One exact active playback has one prospective intent");
                    first_ref = staged.playback_intents.front();
                    const auto first = playback_shape(s, initial);
                    require(first[0].toByteArray() == first_ref,
                            "Selected prospective intent identifies the same exact activation row");
                    require(stage_late_playback(s, port, later_request) == Disposition::committed,
                            "STORAGE-027 NI: provisional second playback representation");
                    const auto second = playback_shape(s, later);
                    second_ref = second[0].toByteArray();
                    if (operation == B::handoff) {
                        const auto handoff = PlaybackHandoff{evidence.handoff.selected, first_ref};
                        require(stage_playback_handoff(s, port, handoff) == Disposition::committed,
                                "STORAGE-027 NI: provisional handoff representation");
                        const auto consumed = playback_shape(s, initial);
                        require(!consumed[2].isNull(),
                                "Exact provisional handoff records its owned claim");
                        initial_shape.handoff = consumed[2].toLongLong();
                    }
                    initial_shape.instance = first_ref.size();
                    initial_shape.attempt = first[1].toLongLong();
                    late_shape.instance = second_ref.size();
                    late_shape.attempt = second[1].toLongLong();
                    require(first_ref != second_ref,
                            "Both exact activation identities stay independently addressable");
                });
            r.result = wire::result(wire::Kind::stop, sample).size();
            const Decision empty_result{Disposition::committed, {}, {}, {}, {}};
            const auto base_bytes = wire::result(wire::Kind::stop, empty_result).size();
            const auto member_bytes = [&](std::vector<QByteArray> refs) {
                auto result = empty_result;
                result.playback_intents = std::move(refs);
                return wire::result(wire::Kind::stop, result).size() - base_bytes;
            };
            const auto fixed = meter::playback_capacity_plan(r, initial_shape, {}, 0);
            const auto initial_member = meter::playback_capacity_member(r, initial_shape);
            const auto late_member = meter::playback_capacity_member(r, late_shape);
            const bool later_is_larger = late_member.binds > initial_member.binds;
            const auto &largest_shape = later_is_larger ? late_shape : initial_shape;
            const auto &largest_ref = later_is_larger ? second_ref : first_ref;
            const auto per = later_is_larger ? late_member : initial_member;
            const auto one = meter::playback_capacity_plan(r, initial_shape, {largest_shape},
                                                           member_bytes({largest_ref}));
            auto distinct_same_width = largest_ref;
            distinct_same_width[0] = char(distinct_same_width[0] ^ 0x01);
            const auto allowance =
                meter::playback_capacity_plan(r, initial_shape, {largest_shape, largest_shape},
                                              member_bytes({largest_ref, distinct_same_width}));
            require_member_delta(one, allowance, per);
            const auto mixed =
                meter::playback_capacity_plan(r, initial_shape, {initial_shape, late_shape},
                                              member_bytes({first_ref, second_ref}));
            require(
                meter::cap(fixed, per, allowance, meter::action_limit) == 2 &&
                    std::ranges::equal(meter::dimensions(mixed), meter::dimensions(allowance),
                                       std::less_equal<>{}),
                "Independent fixed and exact mixed playback plans admit both genuine activations");
            const auto play_limits = meter::limits(allowance);
            const auto &charged_playback = operation == B::late_add ||
                                                   operation == B::late_duplicate ||
                                                   operation == B::late_exhausted
                                               ? late_shape
                                               : initial_shape;
            if (operation != B::stop_started) {
                constexpr StorageLimits loose{64, 1024, 65536, 16777216, 8192, 16777216};
                const AdmissionProfile bounded{loose, play_limits};
                const auto staged = run(f.db, [&](const auto &s) {
                    return stage_command(s, owner, key, nullptr, &bounded);
                });
                require(!staged.missing && staged.published &&
                            staged.published->disposition == Disposition::committed &&
                            staged.published->playback_intents.size() == 1 &&
                            staged.published->playback_intents[0].size() == initial_shape.instance,
                        "STORAGE-027 NI: exact Stop produces the bounded playback prerequisite");
                selected_evidence.handoff.intent = staged.published->playback_intents[0];
                require(f.db.rows("SELECT playback_cap FROM main.subathon_milestone_stop_fences "
                                  "WHERE stop=?",
                                  {key.operation}) == SqlRows{{2}},
                        "Exact dynamic cap is retained with Stop for later turns");
                if (operation == B::late_duplicate || operation == B::late_exhausted) {
                    f.db.write([&](const auto &s) {
                        require(stage_late_playback(s, port, later_request, &bounded) ==
                                    Disposition::committed,
                                "At N-1, the exact new late activation fills the retained cap");
                    });
                    if (operation == B::late_exhausted)
                        later_request.instance.activation = "active-3";
                }
                if (operation == B::outcome || operation == B::outcome_duplicate)
                    f.db.write([&](const auto &s) {
                        require(stage_playback_handoff(s, port, selected_evidence.handoff) ==
                                    Disposition::committed,
                                "Outcome uses a real durable consuming handoff");
                        if (operation == B::outcome_duplicate)
                            require(stage_playback_outcome(s, port, selected_evidence) ==
                                        Disposition::committed,
                                    "Repeated evidence has one real prior append");
                    });
            }

            const auto charge = meter::plan(
                operation, r,
                {meter::Slot{
                    qsizetype(
                        f.edit_authority.facts.captures.front().actions.front().action_ref.size()),
                    0, 0, 0, 0, 0, 0, 0, 0, 0, false, true}},
                {}, charged_playback);
            auto limits = meter::limits(charge);
            if (edge >= 0)
                limits = meter::one_below(charge, edge);
            auto before = [&] {
                return std::array{
                    journal(f.db),
                    f.db.rows("SELECT * FROM main.subathon_milestone_stop_fences ORDER BY stop"),
                    f.db.rows("SELECT * FROM main.subathon_milestone_playback_instances ORDER BY "
                              "instance"),
                    f.db.rows("SELECT * FROM main.subathon_milestone_playback_evidence ORDER BY "
                              "instance,ordinal")};
            };
            const auto original = before();
            f.db.reopen(limits);
            const AdmissionProfile selected{limits, play_limits};
            port.allowed = operation != B::handoff_denied;
            const auto actual = run(f.db, [&](const auto &s) {
                if (operation == B::stop_started)
                    return stage_command(s, owner, key, nullptr, &selected);
                Disposition d;
                if (operation == B::late_add || operation == B::late_duplicate ||
                    operation == B::late_exhausted)
                    d = stage_late_playback(s, port, later_request, &selected);
                else if (operation == B::handoff || operation == B::handoff_denied)
                    d = stage_playback_handoff(s, port, selected_evidence.handoff, &selected);
                else
                    d = stage_playback_outcome(s, port, selected_evidence, &selected);
                return Decision{d, {}, {}, {}, {}};
            });
            require(!actual.missing, "STORAGE-027 NI: exact playback branch and charge");
            if (edge >= 0) {
                const bool limited = actual.published &&
                                     actual.published->disposition == Disposition::resource_limit;
                const bool metered = actual.work.transaction == TransactionOutcome::rolled_back &&
                                     (actual.work.error == WorkError::resource_limit ||
                                      actual.work.error == WorkError::result_limit);
                require((limited || metered) && before() == original,
                        "Playback one below leaves every retained row unchanged");
                return;
            }
            const auto expected =
                operation == B::late_duplicate || operation == B::outcome_duplicate
                    ? Disposition::replay
                : operation == B::late_exhausted ? Disposition::resource_limit
                : operation == B::handoff_denied ? Disposition::refused
                                                 : Disposition::committed;
            require(actual.published && actual.published->disposition == expected,
                    "The independently charged exact branch reaches its expected disposition");
            if (expected != Disposition::committed)
                require(before() == original,
                        "At the cap, duplicate and excess create no new evidence");
            if (operation == B::late_duplicate || operation == B::late_exhausted)
                require(
                    f.db.rows("SELECT count(*) FROM main.subathon_milestone_playback_instances") ==
                        SqlRows{{2}},
                    "Cumulative count stays N after identical replay or distinct legal N+1");
        });
    }

    void stopLargeClosure_data() {
        QTest::addColumn<int>("slot_count");
        QTest::addColumn<bool>("ordinary");
        QTest::newRow("ordinary-application-without-acceptance") << 1 << true;
        QTest::newRow("application-wide-t4") << 256 << false;
    }

    void stopLargeClosure() {
        QFETCH(int, slot_count);
        QFETCH(bool, ordinary);
        check([&] {
            namespace meter = charge_oracle;
            namespace wire = frame_oracle;
            Populated f({.large_slots = static_cast<std::size_t>(slot_count),
                         .ordinary_application = ordinary});
            require(f.db.rows("SELECT count(*),min(length(action_ref)),max(length(action_ref)),"
                              "count(DISTINCT action_ref) FROM main.subathon_milestone_actions") ==
                        SqlRows{{slot_count, 4096, 4096, slot_count}},
                    "Protected application has exact distinct original actions");
            require(
                f.db.rows("SELECT count(*) FROM main.subathon_milestone_acceptance_decisions") ==
                    SqlRows{{ordinary ? 0 : 1}},
                "Ordinary application legitimately has no paused-acceptance row");
            const auto heads = f.db.rows(
                "SELECT length(d.definition_revision),length(p.prefix_revision) "
                "FROM main.subathon_milestone_definition_heads d "
                "JOIN main.subathon_milestone_prefix_heads p USING(installation_ref,track_ref)");
            require(heads.size() == 1, "Exact application track heads are present");
            advance_fixture(f.db);
            const auto key = stop("wide-stop-A", application(f), QueueChoice::continue_queue);
            Authority port;
            meter::Record record;
            record.edited = false;
            record.install = key.track.installation_ref.size();
            record.track = key.track.track_ref.size();
            record.operation = key.operation.size();
            record.actor = key.actor.actor_ref.size();
            record.lineage = key.actor.session_lineage.size();
            record.decision = port.decision.size();
            record.origin_scope = f.original.operation_scope.size();
            record.origin = f.original.operation_ref.size();
            record.occurrence = f.app_occurrence.ref.size();
            record.definition = heads[0][0].toLongLong();
            record.prefix = heads[0][1].toLongLong();
            record.intent = wire::intent(key).size();
            Decision expected{Disposition::committed, {}, {}, {}, {}};
            for (int n = 1; n <= slot_count; ++n)
                expected.cancellations.push_back({n, {}});
            record.result = wire::result(wire::Kind::stop, expected).size();
            const meter::Slot original{4096, 2, 0, 0, 0, 0, 0, 0, 0, 0, true, false};
            const auto charge = meter::plan(meter::Branch::stop_continue, record,
                                            std::vector<meter::Slot>(slot_count, original));
            const auto invoke = [&](StorageLimits limits) {
                f.db.reopen(limits);
                const AdmissionProfile profile{limits, {}};
                const auto result = run(f.db, [&](const auto &session) {
                    return stage_command(session, port, key, nullptr, &profile);
                });
                require(!result.missing, "STORAGE-027 NI: exact bounded ordinary application Stop");
                return result;
            };
            const auto refused = [&](const Turn &reply) {
                const bool bounded = reply.published &&
                                     reply.published->disposition == Disposition::resource_limit &&
                                     reply.published->cancellations.empty();
                const bool metered = reply.work.transaction == TransactionOutcome::rolled_back &&
                                     reply.work.error == WorkError::result_limit &&
                                     !reply.published;
                require((bounded || metered) && journal(f.db).empty() &&
                            cancellations(f.db, key.operation).empty() && causes(f.db).empty() &&
                            f.db.rows("SELECT released FROM main.subathon_milestone_groups") ==
                                SqlRows{{0}},
                        "Result-byte refusal leaves no selected prefix, release, journal or "
                        "cancellation");
            };
            if (slot_count == 256) {
                constexpr qsizetype per_ref = 8 + 4096;
                static_assert(255 * per_ref == 1046520 && 256 * per_ref == 1050624);
                static_assert(1048576 - 255 * per_ref == 2056 && 256 * per_ref - 1048576 == 2048);
                const auto member = meter::stop_member(record, original);
                auto fixed_record = record;
                fixed_record.result =
                    wire::result(wire::Kind::stop, {Disposition::committed, {}, {}, {}, {}}).size();
                const auto fixed = meter::plan(meter::Branch::stop_continue, fixed_record);
                auto repeated = member;
                repeated.binds +=
                    9; // Each original ordinal and absent version in the bound result.
                require(meter::combine(fixed, meter::repeat(repeated, slot_count)) == charge,
                        "The actual application Stop envelope charges all slots and the retained "
                        "result");
                auto allowance = charge;
                allowance.results = 1048576;
                require(
                    meter::cap(fixed, repeated, allowance, meter::action_limit) == 251 &&
                        charge.results > allowance.results,
                    "With every other dimension qualified the actual Stop limit is result bytes");
                const StorageLimits default_value_budget{64, 4096, 65536, 16777216, 8192, 1048576};
                refused(invoke(default_value_budget));
                refused(invoke(meter::limits(allowance)));
            }
            const StorageLimits one_below_values{64,       4096, 65536,
                                                 16777216, 8192, charge.results - 1};
            refused(invoke(one_below_values));
            const auto exact = invoke(meter::limits(charge));
            require(exact.published && *exact.published == expected &&
                        cancellations(f.db, key.operation).size() == slot_count &&
                        f.db.rows("SELECT released FROM main.subathon_milestone_groups") ==
                            SqlRows{{0}},
                    "The same exact complete Stop fits at its independent charge and preserves "
                    "release");
            const auto cancelled = cancellations(f.db, key.operation);
            for (qsizetype i = 0; i < cancelled.size(); ++i)
                require(cancelled[i][0].toLongLong() == i + 1 && cancelled[i][1].isNull(),
                        "Every original position appears once in cancellation readback");
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
                require(stranded_milestone_delivery(s, {}, 1).attempts.empty(),
                        "Stopped revision 4 preserves history and stays outside current stranded "
                        "selection");
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
            port.command = true;
            auto forged = command;
            forged.actor.actor_ref = "caller-asserted-unowned-actor";
            const auto selected = run(f.db, [&](const auto &session) {
                const auto answer = preview_replace(session, port, forged);
                const auto *why = std::get_if<Disposition>(&answer);
                return Decision{why ? *why : Disposition::committed, {}, {}, {}, {}};
            });
            require(selected.published &&
                        (selected.published->disposition == Disposition::denied ||
                         selected.published->disposition == Disposition::unavailable),
                    "A client-asserted actor cannot establish authoritative current facts");
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
            exact_same_result(commit(f.db, port, key, Disposition::replay), first);
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
