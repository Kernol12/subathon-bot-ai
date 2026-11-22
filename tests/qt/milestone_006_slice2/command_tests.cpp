// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_027/command_fixture.hpp"

#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace storage_027_fixture;
using namespace subathon::server::testing;
using Release = MilestoneReleaseDisposition;

struct ReleaseAuthority final : MilestoneEditReleaseAuthority {
    MilestoneOccurrenceKey key;
    explicit ReleaseAuthority(MilestoneOccurrenceKey occurrence) : key(std::move(occurrence)) {}

    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == key;
    }
};

Release release(CatalogFile &db, const MilestoneOccurrenceKey &key, bool edit_origin) {
    const auto reply = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        if (edit_origin)
            return release_milestone_head(db.writer(), key, std::make_shared<ReleaseAuthority>(key),
                                          std::move(done));
        return release_milestone_head(db.writer(), key, std::move(done));
    });
    finance::normal(reply.work);
    require(reply.disposition.has_value(), "Acknowledged release returns a disposition");
    return *reply.disposition;
}

struct ResumeAuthority final : CommandAuthority {
    Authority base;
    QByteArray undisclosed;

    std::optional<Facts> select(const SqliteWorkSession &s, const Command &command) override {
        return base.select(s, command);
    }

    bool discloses_retained(const SqliteWorkSession &, const Command &, const Facts &facts,
                            const QByteArray &decision) override {
        return facts.can_disclose && decision != undisclosed;
    }

    std::optional<qint64> historical_bonus_amount(const SqliteWorkSession &, const Occurrence &,
                                                  const Replacement &) override {
        return {};
    }
};

struct Dispatch final : MilestoneEditDispatchAuthority {
    MilestoneOccurrenceKey occurrence{edit::b};
    int calls{};
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override { return {}; }

    bool allows(const MilestoneEditJob &job) override {
        ++calls;
        return job.key.occurrence == occurrence && job.capture.sender_ref == "edit-sender";
    }
};

struct ReplacementDispatch final : ReplacementDispatchAuthority {
    int calls{};
    bool allows(const ReplacementJob &job) override {
        ++calls;
        return job.target.occurrence == edit::b && job.version.ordinal == 1 &&
               job.payload == QByteArray::fromHex("0081") && job.route == "local/next" &&
               job.qualified_sender == "S";
    }
};

struct BonusEffect final : MilestoneBonusAuthority, MilestoneBonusEffect {
    ContributionReceipt source;
    explicit BonusEffect(ContributionReceipt receipt) : source(std::move(receipt)) {}

    bool allows(const MilestoneBonusKey &key, const MilestoneAction &action) override {
        return key.occurrence == edit::c && key.action_ordinal == 2 &&
               action == MilestoneAction{MilestoneActionKind::bonus, "prior-bonus", "bonus-v1"};
    }

    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &,
                      const MilestoneAction &) override {
        auto bundle = finance::File::receipt_application(s, source, "S1-applied-bonus");
        require(bundle.after.timer.lifecycle == subathon::core::TimerLifecycle::Active &&
                    bundle.after.time_debt_ms == 0,
                "Bonus runs on an Active head with no debt");
        finance::File::retain_plan(
            s, bundle.intent.plan,
            finance::File::plain_capture(
                source, {}, {finance::request(finance::Unit::milliseconds, 30000, "main")}));
        bundle.effects.push_back(finance::addition(30000, 30000, 0, 0));
        bundle.after.timer.main_remaining_ms += 30000;
        require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                    SqliteBundleStageDisposition::staged,
                "Normal application stages the actual +30 seconds");
        return "S1-applied-bonus";
    }
};

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}
} // namespace

class Milestone006CommandTests final : public QObject {
    Q_OBJECT

  private slots:
    void r1PendingReplacementRetainsOriginalAndBonus() {
        check([] {
            Populated f({.edit_slots = 2, .middle_bonus = true});
            require(release(f.db, f.app_occurrence, false) == Release::released,
                    "Normal application release leaves the exact edit head next and unreleased");
            advance_fixture(f.db);
            Authority owner;
            const auto command = replace(f);
            owner.captures.front().payload = QByteArray::fromHex("0081");
            owner.captures.front().route = "local/next";
            owner.captures.front().qualified_sender = "S";
            const auto history = [&] {
                return std::array{
                    f.db.rows("SELECT * FROM main.subathon_milestone_edit_occurrences"),
                    f.db.rows("SELECT * FROM main.subathon_milestone_edit_actions ORDER BY "
                              "action_ordinal"),
                    f.db.rows(
                        "SELECT * FROM main.subathon_milestone_edit_events ORDER BY revision")};
            };
            const auto before = history();
            require(before[0].size() == 1 && before[1].size() == 2 && before[2].size() == 1,
                    "Two original positions and one queued original job are retained");
            const auto preview = bind_replace(f.db, owner, command);
            require(
                history() == before && journal(f.db).empty() &&
                    f.db.rows("SELECT version FROM main.subathon_milestone_slot_versions").empty(),
                "Preview creates no reservation or selection");
            const auto chosen = commit(f.db, owner, command, Disposition::committed, preview.get());
            require(chosen.versions.size() == 1 && chosen.versions.front().ordinal == 1 &&
                        !chosen.versions.front().version.isEmpty() && history() == before,
                    "Confirm changes only the selected first slot version");
            const auto &successor = chosen.versions.front();
            require(f.db.rows("SELECT version,action_version,payload,route,qualified_sender "
                              "FROM main.subathon_milestone_slot_versions") ==
                            SqlRows{{successor.version, QByteArray("announce-v2"),
                                     QByteArray::fromHex("0081"), QByteArray("local/next"),
                                     QByteArray("S")}} &&
                        f.db.rows("SELECT version,revision,state FROM "
                                  "main.subathon_milestone_slot_attempts") ==
                            SqlRows{{successor.version, 1, int(DeliveryJobState::queued)}},
                    "Exactly one successor begins queued at revision one with independently fixed "
                    "bytes");
            require(release(f.db, f.edit_occurrence, true) == Release::released,
                    "The real owner releases the exact occurrence normally");
            Dispatch original;
            ReplacementDispatch dispatch;
            f.db.write([&](const auto &s) {
                const auto retained = lookup_milestone_edit_job(s, f.job);
                require(retained && retained->revision == 1 &&
                            retained->action_document == QByteArray::fromHex("0081") &&
                            retained->capture.sender_ref == "edit-sender" &&
                            claim_milestone_edit_job(s, f.job, 1, original).disposition ==
                                DeliveryDisposition::refused &&
                            original.calls == 0,
                        "Original is still readable and fenced before authority");
                require(claim_replacement(s, edited(f), successor, 1, dispatch) ==
                            Disposition::committed,
                        "First successor claim may enter sending");
                require(claim_replacement(s, edited(f), successor, 1, dispatch) ==
                                Disposition::refused &&
                            dispatch.calls == 1,
                        "Duplicate old-revision claim cannot enter authority again");
                const auto attempt = lookup_replacement_attempt(s, edited(f), successor, 2);
                require(attempt && attempt->revision == 2 &&
                            attempt->state == DeliveryJobState::sending,
                        "Exact successor has one sending event at revision two");
            });
        });
    }

    void s1StopRetainsStartedHistory() {
        check([] {
            predecessor::Populated f;
            install_protected(f);
            f.install();
            const auto candidate = f.seed("S1-origin", Case::positive());
            auto review_service = f.service();
            const auto accepted =
                f.execute(review_service,
                          f.bind("S1-origin", {candidate}, f.preview(review_service, {candidate})));
            expected_row(f.reply(accepted), 1, ReviewDisposition::committed, ReviewStatus::applied);
            const auto &outcome = f.reply(accepted).rows.front().outcome;
            require(outcome && outcome->facts && outcome->facts->bundle,
                    "Protected Review establishes the original committed application");
            const auto application = *outcome->facts->bundle;
            const MilestoneOccurrenceKey preceding{"S1-preceding"};
            const subathon::core::MilestoneCompletion achievement{"target-95", "S1-definition", 95,
                                                                  97, "ctx"};
            const auto intent =
                edit::single("earn-S1", {"C", 96}, {{edit::c, MilestoneActionChoice::run}});
            auto capture = edit::empty_c;
            const auto template_job = edit::capture_b(true).jobs.front();
            const std::array<const char *, 6> names{"audio",     "prior-bonus", "chat",
                                                    "in-flight", "queued-1",    "queued-2"};
            for (qint64 ordinal = 1; ordinal <= 6; ++ordinal) {
                const auto ref = names.at(static_cast<std::size_t>(ordinal - 1));
                if (ordinal == 2) {
                    capture.actions.push_back({MilestoneActionKind::bonus, ref, "bonus-v1"});
                    continue;
                }
                capture.actions.push_back({MilestoneActionKind::automation, ref, "v1"});
                auto job = template_job;
                job.ordinal = ordinal;
                job.capture.action_ref = ref;
                job.capture.action_version_ref = "v1";
                capture.jobs.push_back(std::move(job));
            }
            edit::Authority edit_owner;
            edit_owner.facts.captures = {std::move(capture)};
            f.db.write([&](const auto &s) {
                SqliteMilestoneGroupStore(s).stage(application,
                                                   {{{achievement, {}, {}, preceding}, {}}});
                SqliteMilestoneGroupDecisions(s).record_acceptance(
                    application, {PausedAcceptanceMode::normal,
                                  {"operator", "S1-accepted"},
                                  {{preceding, MilestoneActionChoice::run}}});
                const MilestoneTrackSeed track{
                    edit::track,
                    "cfg",
                    "S1-definition",
                    "S1-prefix",
                    {{"target-95", 95}, {"C", 200}},
                    std::vector<MilestoneTrackSeed::Member>{{achievement, application, preceding}}};
                require(initialize_milestone_track(s, track),
                        "Normal track owner admits the preceding exact application");
                const auto preview =
                    preview_milestone_target_edit(s, edit_owner, edit::actor, intent);
                edit::File::confirmable(preview);
                const auto result = confirm_milestone_target_edit(s, edit_owner, edit::actor,
                                                                  intent, *preview.confirmation);
                require(result.result && result.result->achievements.size() == 1 &&
                            result.result->achievements.front().order == 2,
                        "Actual edit capture retains all six positions on C");
            });
            require(release(f.db, preceding, false) == Release::released &&
                        release(f.db, edit::c, true) == Release::released,
                    "Real writer releases the two exact heads in order");
            const auto balance = [&] {
                return f.db.rows("SELECT s.main_remaining_ms FROM main.subathon_app_head h "
                                 "JOIN main.subathon_app_states s ON h.token=s.token");
            };
            const auto before = balance();
            require(before.size() == 1 && before.front().size() == 1, "Exact current timer head");
            const auto main_before = before.front().front().toLongLong();
            BonusEffect bonus(f.intake("S1-applied-bonus"));
            const auto key = [&](qint64 ordinal) {
                return MilestoneEditJobKey{intent.key, edit::c, ordinal};
            };
            Dispatch dispatch;
            dispatch.occurrence = edit::c;
            f.db.write([&](const auto &s) {
                for (qint64 ordinal : {1, 3, 4}) {
                    const auto job = claim_milestone_edit_job(s, key(ordinal), 1, dispatch);
                    require(job.disposition == DeliveryDisposition::changed && job.job &&
                                job.job->attempt == 2,
                            "Only specified live, sent and in-flight jobs actually start");
                }
                require(finish_milestone_edit_job(s, key(3), 2, 2, DeliveryEvidence::delivered,
                                                  "sent-before-Stop")
                                    .disposition == DeliveryDisposition::changed &&
                            SqliteMilestoneGroupStore(s).apply_bonus({edit::c, 2}, bonus, bonus) ==
                                MilestoneBonusDisposition::applied,
                        "Retained delivery and +30-second effect precede Stop");
            });
            require(balance() == SqlRows{{main_before + 30000}}, "Bonus applied exactly once");
            advance_fixture(f.db);
            Authority owner;
            const Occurrence target{edit::track, edit::c, intent.key};
            const auto cmd = stop("S1-stop", target);
            owner.observed_playback = {{target,
                                        1,
                                        {},
                                        2,
                                        "audio-adapter",
                                        "M",
                                        "generation-M",
                                        StopCapability::supported}};
            const auto history = [&] {
                return std::array{
                    f.db.rows("SELECT * FROM main.subathon_milestone_edit_actions ORDER BY "
                              "action_ordinal"),
                    f.db.rows("SELECT * FROM main.subathon_milestone_edit_events ORDER BY "
                              "action_ordinal,revision"),
                    f.db.rows("SELECT * FROM main.subathon_milestone_edit_bonus_receipts")};
            };
            const auto retained = history();
            const auto stopped = commit(f.db, owner, cmd, Disposition::committed);
            require(
                stopped.cancellations == std::vector<SelectedVersion>{{5, {}}, {6, {}}} &&
                    stopped.added_causes.size() == 1 && stopped.playback_intents.size() == 1 &&
                    journal(f.db).size() == 1 && history() == retained &&
                    balance() == SqlRows{{main_before + 30000}},
                "Stop cancels precisely two unstarted slots, adds one Hold and retains history");
            require(f.db.rows("SELECT ordinal,activation,generation,typeof(handoff) FROM "
                              "main.subathon_milestone_playback_instances") ==
                        SqlRows{{1, QByteArray("M"), QByteArray("generation-M"), QString("null")}},
                    "Only positively identified M has an intent and no handoff");
            f.db.write([&](const auto &s) {
                const auto finished = finish_milestone_edit_job(
                    s, key(4), 2, 2, DeliveryEvidence::delivered, "matching-after-Stop");
                require(finished.disposition == DeliveryDisposition::changed && finished.job &&
                            finished.job->revision == 3,
                        "Captured in-flight attempt can still record exact finish");
            });
        });
    }

    void s5DurableSubsetGatesRelease_data() {
        QTest::addColumn<bool>("stop_a_with_hold");
        QTest::newRow("continue-retains-H1") << false;
        QTest::newRow("resolve-H2-then-H1") << true;
    }

    void s5DurableSubsetGatesRelease() {
        QFETCH(bool, stop_a_with_hold);
        check([&] {
            Populated f({.large_slots = 1, .third_target = true});
            (void)f.add_third_occurrence();
            advance_fixture(f.db);
            Authority owner;
            const auto first =
                commit(f.db, owner, stop("hold-B", edited(f)), Disposition::committed);
            const auto choice =
                stop_a_with_hold ? std::nullopt : std::optional{QueueChoice::continue_queue};
            const auto second =
                commit(f.db, owner, stop("stop-A", application(f), choice), Disposition::committed);
            require(first.added_causes.size() == 1 &&
                        second.added_causes.size() == (stop_a_with_hold ? 1U : 0U),
                    "Omission adds one independent Hold and Continue adds none");
            require(release(f.db, edit::c, true) == Release::refused,
                    "An existing interruption Hold blocks the later ready head");
            if (stop_a_with_hold) {
                const auto command = resume("resolve-H2", second.added_causes);
                require(commit(f.db, owner, command, Disposition::committed).resolved_causes ==
                            second.added_causes,
                        "Resume consumes only H2");
                require(release(f.db, edit::c, true) == Release::refused,
                        "H1 still gates release after exact H2 resolution");
            }
            const auto last = resume("resolve-H1", first.added_causes);
            require(commit(f.db, owner, last, Disposition::committed).resolved_causes ==
                            first.added_causes &&
                        release(f.db, edit::c, true) == Release::released,
                    "Normal release succeeds only when the last independent Hold resolves");
            require(f.db.rows("SELECT released FROM main.subathon_milestone_groups") ==
                            SqlRows{{0}} &&
                        f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences "
                                  "ORDER BY queue_order") == SqlRows{{0}, {1}},
                    "Neither stopped occurrence acquired a release");
        });
    }

    void s7FreshStopRefusalsHaveNoWrites_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::newRow("application") << false;
        QTest::newRow("edit") << true;
    }

    void s7FreshStopRefusalsHaveNoWrites() {
        QFETCH(bool, edit_origin);
        check([&] {
            Populated f({.large_slots = edit_origin ? 0U : 1U});
            if (edit_origin)
                require(release(f.db, f.app_occurrence, false) == Release::released,
                        "Normal preceding application releases before edit head selection");
            advance_fixture(f.db);
            Authority owner;
            const auto target = edit_origin ? edited(f) : application(f);
            auto absent = target;
            absent.occurrence = {"absent"};
            const auto snapshot = [&] {
                return std::array{
                    journal(f.db),
                    causes(f.db),
                    f.db.rows("SELECT * FROM main.subathon_milestone_stop_fences"),
                    f.db.rows("SELECT * FROM main.subathon_milestone_stop_cancellations"),
                    f.db.rows("SELECT released FROM main.subathon_milestone_groups"),
                    f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences")};
            };
            const auto refuse = [&](const Command &command, Disposition expected) {
                const auto before = snapshot();
                (void)commit(f.db, owner, command, expected);
                require(snapshot() == before, "Fresh refusal preserves complete Stop state");
            };
            refuse(stop("absent-stop", absent), Disposition::refused);
            refuse(stop("invalid-choice", target, static_cast<QueueChoice>(9)),
                   Disposition::refused);
            owner.command = false;
            refuse(stop("ungranted-stop", target), Disposition::denied);
            owner.command = true;
            const auto original = stop("only-successful-stop", target);
            const auto saved = commit(f.db, owner, original, Disposition::committed);
            require(saved.added_causes.size() == 1 && saved.cancellations.size() == 1,
                    "Positive control on the same target commits one Stop and Hold");
            refuse(stop("new-key-already-stopped", target), Disposition::refused);
            auto replay = saved;
            replay.disposition = Disposition::replay;
            const auto before = snapshot();
            require(commit(f.db, owner, original, Disposition::replay) == replay &&
                        snapshot() == before,
                    "The exact retry returns the original result without a second write");
        });
    }

    void t1T2StopAcknowledgementAndReadback_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<int>("fault");
        for (bool edit_lane : {false, true}) {
            for (const auto &[name, fault] :
                 {std::pair{"rollback", 0}, std::pair{"lost-commit-ack", 1},
                  std::pair{"abnormal-return", 2}}) {
                const auto row = QByteArray(edit_lane ? "edit-" : "application-") + name;
                QTest::newRow(row.constData()) << edit_lane << fault;
            }
        }
    }

    void t1T2StopAcknowledgementAndReadback() {
        QFETCH(bool, edit_origin);
        QFETCH(int, fault);
        check([&] {
            Populated f({.large_slots = edit_origin ? 0U : 1U});
            if (edit_origin)
                require(release(f.db, f.app_occurrence, false) == Release::released,
                        "Application predecessor releases before the edit head");
            const auto target = edit_origin ? edited(f) : application(f);
            require(release(f.db, target.occurrence, edit_origin) == Release::released,
                    "A has a normal release before the writer result is lost");
            advance_fixture(f.db);
            Authority owner;
            owner.presentation = PresentationObservation{target,         "presentation-A",
                                                         "generation-A", "generation-A",
                                                         "revision-A",   0,
                                                         10000,          2000,
                                                         "revision-A",   1800};
            const auto command = stop("T1-stop", target);
            if (fault == 1)
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            f.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Only the real successful COMMIT loses acknowledgement");
            bool staged = false;
            const auto turn = run(
                f.db,
                [&](const auto &s) {
                    const auto result = stage_command(s, owner, command);
                    staged = result.disposition == Disposition::committed &&
                             result.cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                             result.added_causes.size() == 1 && result.playback_intents.empty();
                    return result;
                },
                fault == 0   ? Finish::rollback
                : fault == 1 ? Finish::commit
                             : Finish::throw_after_commit);
            const auto outcome =
                std::array{TransactionOutcome::rolled_back, TransactionOutcome::outcome_unknown,
                           TransactionOutcome::committed}
                    .at(static_cast<std::size_t>(fault));
            const auto error = std::array{WorkError::not_committed, WorkError::uncertain_outcome,
                                          WorkError::work_exception}
                                   .at(static_cast<std::size_t>(fault));
            require(staged && turn.entered && !turn.published && !turn.work.value &&
                        turn.work.transaction == outcome && turn.work.error == error,
                    "Provisional Stop reaches the real writer fault, with no owner publication");
            f.db.reopen(profile());
            if (fault == 0) {
                require(journal(f.db).empty() && causes(f.db).empty() &&
                            cancellations(f.db, command.operation).empty(),
                        "Known rollback persists neither Stop, Hold nor cancellation");
                const auto retry = commit(f.db, owner, command, Disposition::committed);
                require(retry.cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                            retry.added_causes.size() == 1,
                        "Same operation can first commit after known rollback");
                return;
            }

            const auto saved = read(f.db, owner, command);
            require(saved.disposition == Disposition::replay && saved.added_causes.size() == 1 &&
                        saved.cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                        saved.playback_intents.empty(),
                    "Only a normal exact readback establishes the single committed Stop");
            const auto logical = [&] {
                return std::array{journal(f.db), causes(f.db),
                                  cancellations(f.db, command.operation)};
            };
            const auto before = logical();
            require(before[0].size() == 1 && before[1].size() == 1 && before[2].size() == 1 &&
                        commit(f.db, owner, command, Disposition::replay) == saved,
                    "Exact retry preserves the acknowledged readback result");
            const auto changed = stop(command.operation, target, QueueChoice::continue_queue);
            (void)commit(f.db, owner, changed, Disposition::conflict);
            owner.command = false;
            (void)commit(f.db, owner, command, Disposition::denied);
            require(read(f.db, owner, command).disposition == Disposition::denied &&
                        logical() == before,
                    "Change and revocation preserve the only committed result and its Hold");
        });
    }

    void s9ResumeAtomicRefusalAndDisclosure() {
        check([] {
            Populated f({.held = true, .third_target = true});
            const auto third = f.add_third_occurrence();
            const MilestoneHoldKey d{MilestoneHoldCause::deferred_acceptance, "milestone-decision"};
            const MilestoneHoldKey r{MilestoneHoldCause::refund_review, "prior-review"};
            f.db.write([&](const auto &s) {
                SqliteMilestoneGroupDecisions(s).observe_refund(
                    {f.app_occurrence, r.source_ref, 105, 99, 100, MilestoneRefundPolicy::review});
            });
            advance_fixture(f.db);
            Authority owner;
            const auto hold = [&](const char *operation, const Occurrence &target) {
                owner.decision = operation;
                const auto result =
                    commit(f.db, owner, stop(operation, target), Disposition::committed);
                require(result.added_causes.size() == 1, "Each Stop creates one distinct Hold");
                return result.added_causes.front();
            };
            const auto h0 = hold("stop-zero", {edit::track, edit::c, third});
            require(commit(f.db, owner, resume("resolve-zero", {h0}), Disposition::committed)
                            .resolved_causes == std::vector{h0},
                    "H0 is durably resolved before the mixed-set trials");
            const auto h1 = hold("stop-one", application(f));
            const auto h2 = hold("stop-two", edited(f));
            require(h0 != h1 && h1 != h2 && h0 != h2, "Every Hold has its own identity");

            const auto snapshot = [&] {
                return std::array{
                    journal(f.db), causes(f.db),
                    f.db.rows("SELECT * FROM main.subathon_milestone_stop_fences"),
                    f.db.rows("SELECT released FROM main.subathon_milestone_groups"),
                    f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences "
                              "ORDER BY queue_order")};
            };
            const auto assert_independent = [&] {
                f.db.write([&](const auto &s) {
                    const SqliteMilestoneGroupDecisions decisions(s);
                    require(decisions.active_holds(f.app_occurrence) == std::vector{d, r} &&
                                decisions.pending_disposition(f.app_occurrence) ==
                                    MilestonePendingDisposition::held,
                            "Independent Defer and refund Review still gate exact A");
                });
            };
            assert_independent();
            const auto before = snapshot();
            const auto attempt = [&](ResumeAuthority &authority, const Command &command,
                                     Disposition expected) {
                const auto turn =
                    run(f.db, [&](const auto &s) { return stage_command(s, authority, command); });
                require(turn.published && turn.published->disposition == expected,
                        "Normal writer acknowledgement returns the exact Resume disposition");
                if (expected != Disposition::committed) {
                    require(snapshot() == before, "Refusal writes neither result nor resolution");
                    assert_independent();
                }
                return *turn.published;
            };
            ResumeAuthority authority;
            qsizetype ordinal = 0;
            for (const auto &members :
                 std::vector<std::vector<QByteArray>>{{},
                                                      {h1, h1},
                                                      {h1, h0},
                                                      {h1, "missing"},
                                                      {h1, "milestone-decision"},
                                                      {h1, "prior-review"}}) {
                (void)attempt(authority,
                              resume("invalid-" + QByteArray::number(++ordinal), members),
                              Disposition::refused);
            }
            const auto eligible = resume("selected-H1-H2", {h1, h2});
            authority.base.command = false;
            (void)attempt(authority, eligible, Disposition::denied);
            authority.base.command = true;
            f.db.write([&](const auto &s) {
                const auto selected = authority.select(s, eligible);
                require(selected && selected->can_command && selected->can_disclose,
                        "The exact set was selected with current grants");
            });
            authority.base.command = false;
            authority.base.revision = "revoked-after-selection";
            auto revoked = eligible;
            revoked.operation = "revoked-after-selection";
            (void)attempt(authority, revoked, Disposition::denied);
            authority.base.command = true;
            authority.base.revision = "fresh-grant";
            authority.undisclosed = "stop-two";
            (void)attempt(authority, eligible, Disposition::denied);
            authority.undisclosed.clear();
            const auto committed = attempt(authority, eligible, Disposition::committed);
            auto wanted = std::vector{h1, h2};
            std::ranges::sort(wanted);
            require(committed.resolved_causes == wanted &&
                        journal(f.db).size() == before[0].size() + 1 &&
                        causes(f.db) ==
                            SqlRows{{h1, QByteArray("stop-one"), eligible.operation},
                                    {h2, QByteArray("stop-two"), eligible.operation},
                                    {h0, QByteArray("stop-zero"), QByteArray("resolve-zero")}},
                    "One authorized Resume writes exactly its two selected resolutions");
            assert_independent();
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006CommandTests)
#include "command_tests.moc"
