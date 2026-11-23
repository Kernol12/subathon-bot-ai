// SPDX-License-Identifier: GPL-3.0-or-later
#include "refund_fixture.hpp"

#include "../milestone_006/timeline_boundary.hpp"

namespace {
using namespace milestone_006_slice2;
using namespace std::chrono_literals;
using Resolution = MilestoneHoldResolution;
using Gate = MilestonePendingDisposition;

struct ExactRelease final : MilestoneEditReleaseAuthority {
    MilestoneOccurrenceKey key;
    explicit ExactRelease(MilestoneOccurrenceKey occurrence) : key(std::move(occurrence)) {}

    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == key;
    }
};

MilestoneReleaseDisposition release(SqliteWriter &writer, const MilestoneOccurrenceKey &key,
                                    bool edit_origin) {
    const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        if (edit_origin)
            return release_milestone_head(writer, key, std::make_shared<ExactRelease>(key),
                                          std::move(done));
        return release_milestone_head(writer, key, std::move(done));
    });
    finance::normal(result.work);
    require(result.disposition.has_value(), "Acknowledged release returns a disposition");
    return *result.disposition;
}

Occurrence target_a(bool edit_origin) {
    using Origin = decltype(Occurrence::origin);
    return {edit::track, edit::a,
            edit_origin ? Origin{MilestoneEditKey{edit::track, "adjust"}}
                        : Origin{ApplicationOperationKey{"test", "application-A"}}};
}

Decision stop_a(RefundFixture &f, bool edit_origin, Authority &owner) {
    const auto cmd = stop("stop-A", target_a(edit_origin), QueueChoice::continue_queue);
    std::optional<Decision> committed;
    f.file.write([&](const auto &s) {
        committed = stage_command(s, owner, cmd);
        require(committed->disposition == Disposition::committed &&
                    committed->cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                    committed->added_causes.empty() && committed->playback_intents.empty(),
                "Exact Stop cancels one unstarted bonus without adding a Hold or playback intent");
    });
    return *committed;
}

void require_review(const SqliteWorkSession &s, std::optional<Resolution> resolution = {},
                    std::optional<MilestoneDecisionEvidence> evidence = {},
                    std::optional<MilestoneSkipProvenance> ended = {}) {
    const auto old = SqliteMilestoneGroupDecisions(s).refund_audit(edit::a, review.source_ref);
    require(old && old->policy == Policy::review && old->current_target_micro_points == 1000 &&
                old->deficit_micro_points == 5 && old->resolution == resolution &&
                old->resolution_evidence == evidence && old->ended_by_skip == ended,
            "The original refund retains its saved target, deficit and exact decision history");
}

SqlRows release_bits(RefundFixture &f, bool edit_origin) {
    const auto *table =
        edit_origin ? "subathon_milestone_edit_occurrences" : "subathon_milestone_groups";
    return f.file.rows(QString("SELECT released FROM main.%1 ORDER BY queue_order").arg(table));
}

void require_defer_resolution(const SqliteWorkSession &s, bool edit_origin,
                              std::optional<Resolution> resolution = {},
                              std::optional<MilestoneDecisionEvidence> evidence = {}) {
    if (!edit_origin) {
        const auto row = SqliteMilestoneGroupDecisions(s).acceptance_audit(edit::a);
        require(row && row->choice == MilestoneActionChoice::defer &&
                    row->evidence.decision_ref == defer.source_ref &&
                    row->resolution == resolution && row->resolution_evidence == evidence &&
                    !row->ended_by_skip,
                "Original application Defer retains its own exact resolution and provenance");
        return;
    }
    const auto rows = s.execute(
        {"SELECT choice,decision_ref,coalesce(resolution,-1),coalesce(resolution_actor_ref,x''),"
         "coalesce(resolution_ref,x''),coalesce(ended_by_skip_kind,-1) "
         "FROM main.subathon_milestone_edit_choices WHERE occurrence_ref=?",
         {QByteArray::fromStdString(edit::a.ref)}});
    require(
        rows ==
            SqlRows{{int(MilestoneActionChoice::defer), QByteArray("accept-A"),
                     resolution ? int(*resolution) : -1,
                     evidence ? QByteArray::fromStdString(evidence->actor_ref) : QByteArray(""),
                     evidence ? QByteArray::fromStdString(evidence->decision_ref) : QByteArray(""),
                     -1}},
        "Original edit Defer retains its own exact resolution and provenance");
}

void require_held_head(RefundFixture &f, bool edit_origin, Authority &owner,
                       const std::vector<MilestoneHoldKey> &holds) {
    const auto cmd = stop("stop-A", target_a(edit_origin), QueueChoice::continue_queue);
    f.file.write([&](const auto &s) {
        const auto projection = read_occurrence_selection(s, owner, cmd);
        const auto pending = SqliteMilestoneGroupStore(s).pending();
        require(projection && projection->target.occurrence == edit::a &&
                    projection->stop == cmd.operation && !projection->released &&
                    !projection->skipped && projection->independent_holds == holds &&
                    projection->interruption_holds.empty() &&
                    projection->cancellations == std::vector<SelectedVersion>{{1, {}}} &&
                    pending.size() == 2 && pending[0].group.occurrence == edit::a &&
                    pending[1].group.occurrence == edit::b,
                "Exact stopped A remains the ordered blocker while any independent cause remains");
    });
    require(release(*f.file.writer, edit::b, edit_origin) == MilestoneReleaseDisposition::refused &&
                release_bits(f, edit_origin) == SqlRows{{0}, {0}},
            "The normal B release cannot pass the held stopped head");
}

void resolve(RefundFixture &f, const MilestoneHoldKey &cause, Resolution resolution,
             const MilestoneDecisionEvidence &evidence) {
    f.file.write([&](const auto &s) {
        require(SqliteMilestoneGroupDecisions(s).resolve(edit::a, cause, resolution, evidence),
                "Normal #236 resolve consumes the exact active cause");
    });
}

void check_pass_and_release_b(RefundFixture &f, bool edit_origin) {
    f.file.write([&](const auto &s) {
        const auto pending = SqliteMilestoneGroupStore(s).pending();
        require(pending.size() == 1 && pending.front().group.occurrence == edit::b,
                "Cold traversal omits the passable stopped A and retains exact B");
    });
    require(release(*f.file.writer, edit::a, edit_origin) == MilestoneReleaseDisposition::refused &&
                release(*f.file.writer, edit::b, edit_origin) ==
                    MilestoneReleaseDisposition::released &&
                release_bits(f, edit_origin) == SqlRows{{0}, {1}},
            "B alone releases, and A retains release zero");
}

struct ExactQueuePort final : MilestoneGroupRelease {
    std::vector<MilestoneHoldKey> a_holds{defer, review};
    std::vector<MilestoneOccurrenceKey> released;

    bool review_held(const subathon::core::MilestoneCompletion &) const override {
        throw std::logic_error("S6 requires an exact occurrence");
    }

    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &key) const override {
        return key == edit::a && !a_holds.empty() ? MilestoneHeadDisposition::held
                                                  : MilestoneHeadDisposition::ready;
    }

    void release(CommittedMilestoneGroup group) override {
        require(group.occurrence.has_value(), "Owner release retains the exact key");
        released.push_back(*group.occurrence);
    }
};

struct OrderedSelection {
    OccurrenceSelection stopped;
    std::vector<CommittedMilestoneGroup> pending;
};

OrderedSelection read_ordered(RefundFixture &f, bool edit_origin, Authority &owner) {
    const auto selector = stop("stop-A", target_a(edit_origin), QueueChoice::continue_queue);
    std::optional<OrderedSelection> committed;
    f.file.write([&](const auto &s) {
        auto selected = read_occurrence_selection(s, owner, selector);
        require(selected && selected->target.occurrence == edit::a && selected->stop == "stop-A" &&
                    !selected->released && !selected->skipped &&
                    selected->interruption_holds.empty(),
                "Normal readback selects the exact stopped occurrence and its independent gates");
        std::vector<CommittedMilestoneGroup> pending;
        for (const auto &row : SqliteMilestoneGroupStore(s).pending())
            pending.push_back(row.group);
        committed = OrderedSelection{std::move(*selected), std::move(pending)};
    });
    return *committed;
}

enum class QueueAdoption { warm, cold };
void apply_ordered_selection(MilestoneTimeline &timeline, std::chrono::milliseconds now,
                             QueueAdoption adoption, const OrderedSelection &selection) {
    adopt_ordered_milestone_selection(timeline, now,
                                     static_cast<MilestoneQueueAdoption>(adoption),
                                     selection.stopped, selection.pending);
}

struct OriginalDispatch final : DeliveryDispatchAuthority, DeliveryAdapter {
    std::vector<DeliveryJob> started;
    std::size_t selection_calls{};

    bool allows(const DeliveryJob &job) override {
        ++selection_calls;
        return job.key.job_ref == "original-job" && job.capture.route_ref == "local-fixture" &&
               job.action_document == "synthetic-action";
    }

    void start(DeliveryJob job) override { started.push_back(std::move(job)); }
};

struct RetryOnce final : DeliveryRetryPolicy {
    std::optional<std::string> allows(const DeliveryJob &job) override {
        if (job.key.job_ref == "original-job" && job.revision == 3)
            return "known-not-delivered";
        return {};
    }
};

struct ApplicationAutomation {
    finance::File file{finance::OriginalCredit{.milestone = true}};
    DeliveryJobKey key{finance::original, "original-job"};
    Occurrence target{edit::track, finance::achievement, finance::original};

    ApplicationAutomation() {
        report(migrate(*file.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
               CatalogState::current, 19, 19, 4);
        report(migrate(*file.writer,
                       {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
               CatalogState::current, 20, 20, 1);
        reopen_financial(file);
        file.write([&](const auto &s) {
            require(SqliteAccountSessionUnit(s, {}).insert_installation(
                        {{"install", "epoch"}, "revision"}) == LifecycleWriteResult::staged,
                    "Account owner retains exact application command installation");
            const subathon::core::MilestoneCompletion completed{"target-35", "milestone-v1",
                                                                35000000, 40000000, "ctx"};
            require(initialize_milestone_track(
                        s, {edit::track,
                            "cfg",
                            "milestone-v1",
                            "application-prefix",
                            {{"target-35", 35000000}},
                            std::vector<MilestoneTrackSeed::Member>{
                                {completed, finance::original, finance::achievement}}}),
                    "Protected track owner enrolls the real grouped application job");
        });
    }

    DeliveryDisposition dispatch(qint64 revision, const std::shared_ptr<OriginalDispatch> &port) {
        const auto completed = finance::completed<DeliveryDispatchCompletion>([&](auto done) {
            return dispatch_delivery_job(*file.writer, key, revision, port, port, std::move(done));
        });
        finance::normal(completed.work);
        require(completed.result && !completed.adapter_threw,
                "Normal original dispatch returns an acknowledged disposition");
        return completed.result->disposition;
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

class Milestone006SettlementTests final : public QObject {
    Q_OBJECT

  private slots:
    void r1ApplicationAutomationSelectionFencesOriginal() {
        check([] {
            ApplicationAutomation f;
            advance_to_protected_successor(f.file);
            Authority owner;
            const Replacement patch{1, {MilestoneActionKind::automation, "original-action", "v2"}};
            const Command command{edit::track, "replace-application", edit::actor,
                                  ReplaceIntent{f.target, {patch}}};
            owner.captures = {{patch, QByteArray::fromHex("0081"), "local/next", "S"}};
            const auto version_rows = [&] {
                return f.file.rows("SELECT version,payload,qualified_sender "
                                   "FROM main.subathon_milestone_slot_versions");
            };
            const auto state = [&] {
                return f.file.rows("SELECT released FROM main.subathon_milestone_groups");
            };
            std::shared_ptr<const ReplacePreview> preview;
            f.file.write([&](const auto &s) {
                const auto result = preview_replace(s, owner, command);
                const auto *bound = std::get_if<std::shared_ptr<const ReplacePreview>>(&result);
                require(bound && *bound, "Real pending application job enters exact preview");
                preview = *bound;
            });
            require(version_rows().empty() && state() == SqlRows{{0}} &&
                        f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal")
                            .empty(),
                    "Preview does not reserve operations, change selection or release");
            std::optional<Decision> selected;
            f.file.write([&](const auto &s) {
                selected = stage_command(s, owner, command, preview.get());
                require(selected->disposition == Disposition::committed &&
                            selected->versions.size() == 1 &&
                            selected->versions.front().ordinal == 1,
                        "Confirm selects a real new application automation version");
            });
            const auto version = selected->versions.front().version;
            require(!version.isEmpty() &&
                        version_rows() ==
                            SqlRows{{version, QByteArray::fromHex("0081"), QByteArray("S")}} &&
                        state() == SqlRows{{0}},
                    "Exact captured bytes and S are durable, while release remains zero");
            require(release(*f.file.writer, f.target.occurrence, false) ==
                        MilestoneReleaseDisposition::released,
                    "The normal exact application release still succeeds");
            auto port = std::make_shared<OriginalDispatch>();
            require(f.dispatch(1, port) == DeliveryDisposition::refused && port->started.empty() &&
                        port->selection_calls == 0,
                    "Superseded application predecessor cannot hand off");
            f.file.write([&](const auto &s) {
                const auto retained = SqliteDeliveryJobStore(s).lookup(f.key);
                const auto queued =
                    lookup_replacement_attempt(s, f.target, selected->versions.front(), 1);
                require(
                    retained && retained->revision == 1 &&
                        retained->action_document == "synthetic-action" &&
                        retained->capture.sender_ref == "fixture-sender" && queued &&
                        queued->state == DeliveryJobState::queued,
                    "Original bytes remain readable, while only successor is currently selected");
            });
        });
    }

    void r3ApplicationAutomationRequeueRemainsStarted() {
        check([] {
            ApplicationAutomation f;
            require(release(*f.file.writer, f.target.occurrence, false) ==
                        MilestoneReleaseDisposition::released,
                    "Exact application starts through the normal release owner");
            advance_to_protected_successor(f.file);
            Authority owner;
            const Replacement patch{1, {MilestoneActionKind::automation, "original-action", "v2"}};
            const Command replace_command{edit::track, "requeue-replace", edit::actor,
                                          ReplaceIntent{f.target, {patch}}};
            owner.captures = {{patch, QByteArray::fromHex("0081"), "local/next", "S"}};
            f.file.write([&](const auto &s) {
                const auto preview = preview_replace(s, owner, replace_command);
                const auto *bound = std::get_if<std::shared_ptr<const ReplacePreview>>(&preview);
                require(bound && *bound, "Identical request is confirmable before its first claim");
            });
            auto port = std::make_shared<OriginalDispatch>();
            require(f.dispatch(1, port) == DeliveryDisposition::changed &&
                        port->started.size() == 1 && port->started.front().attempt == 2,
                    "One acknowledged claim hands off the original sending attempt");
            RetryOnce retry;
            f.file.write([&](const auto &s) {
                const SqliteDeliveryJobStore jobs(s);
                const auto failed = jobs.finish(f.key, 2, 2, DeliveryEvidence::known_undelivered,
                                                "confirmed-undelivered");
                const auto queued = jobs.retry(f.key, 3, retry);
                require(failed.disposition == DeliveryDisposition::changed && failed.job &&
                            failed.job->revision == 3 &&
                            failed.job->state == DeliveryJobState::failed &&
                            queued.disposition == DeliveryDisposition::changed && queued.job &&
                            queued.job->revision == 4 &&
                            queued.job->state == DeliveryJobState::queued,
                        "Original attempt fails at r3 and returns to queue at r4");
            });
            const auto history = [&] {
                return f.file.rows("SELECT revision,state,coalesce(attempt,0) "
                                   "FROM main.subathon_delivery_events ORDER BY revision");
            };
            using State = DeliveryJobState;
            const SqlRows expected{{1, int(State::queued), 0},
                                   {2, int(State::sending), 2},
                                   {3, int(State::failed), 2},
                                   {4, int(State::queued), 0}};
            require(history() == expected, "All four independent revisions precede Stop");
            const auto normalized = stop("stop-requeued", f.target, QueueChoice::continue_queue);
            f.file.write([&](const auto &s) {
                const auto preview = preview_replace(s, owner, replace_command);
                const auto *refusal = std::get_if<Disposition>(&preview);
                require(refusal && *refusal == Disposition::refused,
                        "Requeued original remains started for replacement eligibility");
                const auto result = stage_command(s, owner, normalized);
                require(result.disposition == Disposition::committed &&
                            result.cancellations.empty() && result.added_causes.empty(),
                        "Stop fences the attempted original without cancelling an unstarted slot");
            });
            require(f.dispatch(4, port) == DeliveryDisposition::refused &&
                        port->started.size() == 1 && port->selection_calls == 1 &&
                        history() == expected,
                    "The second dispatch attempt stops before authority or handoff");
        });
    }

    void s6StoppedRefundRetainsExactCauses_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<int>("policy");
        for (bool edit_lane : {false, true}) {
            for (const auto &[name, policy] :
                 {std::pair{"continue", Policy::continue_actions},
                  std::pair{"review", Policy::review}, std::pair{"skip", Policy::skip}}) {
                const auto row = QByteArray(edit_lane ? "edit-" : "application-") + name;
                QTest::newRow(row.constData()) << edit_lane << int(policy);
            }
        }
    }

    void s6StoppedRefundRetainsExactCauses() {
        QFETCH(bool, edit_origin);
        QFETCH(int, policy);
        check([&] {
            RefundFixture f(edit_origin);
            const auto [earlier, original] = f.first_refund();
            advance_to_protected_successor(f.file);
            Authority owner;
            (void)stop_a(f, edit_origin, owner);
            require_held_head(f, edit_origin, owner, {defer, review});
            f.reopen();
            require_held_head(f, edit_origin, owner, {defer, review});

            f.fixture.move("score-recovered-for-second-crossing", f.file.head().timer, {}, 15);
            f.require_score(1010);
            f.fixture.input.milestone_policy = static_cast<Policy>(policy);
            const auto later = f.fixture.command("later-refund", "20", 1);
            const auto accepted = live::accepted(f.fixture.refund(later, f.entitlement(25, 45, 1)));
            f.require_score(990);
            require(accepted.milestone_observations.empty(),
                    "The ordinary debit leaves deficit ten without a stopped-A policy result");
            f.file.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions gates(s);
                require(!gates.refund_audit(edit::a, "later-refund") &&
                            !gates.refund_audit(edit::b, "later-refund") &&
                            gates.active_holds(edit::a) == std::vector{defer, review},
                        "Neither A nor B gets a new decision, resolution or ended-by-Skip history");
                require_defer_resolution(s, edit_origin);
                require_review(s);
            });
            const auto assert_original_replay = [&] {
                const auto old = live::accepted(f.fixture.refund(earlier, f.entitlement(0, 25, 0)),
                                                finance::D::existing);
                require(old.payment_position == 1 &&
                            old.application.after.token == original.application.after.token &&
                            old.milestone_observations.size() == 1 &&
                            old.milestone_observations[0].occurrence == edit::a &&
                            old.milestone_observations[0].policy == Policy::review,
                        "Authorized replay retains the exact earlier result");
                f.require_score(990);
            };
            assert_original_replay();
            const MilestoneDecisionEvidence d_evidence{"operator", "continue-D"};
            const MilestoneDecisionEvidence r_evidence{"operator", "continue-R"};
            resolve(f, defer, Resolution::continue_actions, d_evidence);
            require_held_head(f, edit_origin, owner, {review});
            resolve(f, review, Resolution::continue_actions, r_evidence);
            f.reopen();
            check_pass_and_release_b(f, edit_origin);
            assert_original_replay();
            f.file.write([&](const auto &s) {
                require_defer_resolution(s, edit_origin, Resolution::continue_actions, d_evidence);
                require_review(s, Resolution::continue_actions, r_evidence);
            });
        });
    }

    void s6OwnerOrderedReconciliation_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<bool>("cold");
        QTest::newRow("application-warm") << false << false;
        QTest::newRow("application-cold") << false << true;
        QTest::newRow("edit-warm") << true << false;
        QTest::newRow("edit-cold") << true << true;
    }

    void s6OwnerOrderedReconciliation() {
        QFETCH(bool, edit_origin);
        QFETCH(bool, cold);
        check([&] {
            RefundFixture f(edit_origin);
            (void)f.first_refund();
            advance_to_protected_successor(f.file);
            ExactQueuePort port;
            MilestoneTimeline timeline({}, port);
            if (!cold) {
                std::vector<CommittedMilestoneGroup> cached;
                f.file.write([&](const auto &s) {
                    for (const auto &row : SqliteMilestoneGroupStore(s).pending())
                        cached.push_back(row.group);
                });
                require(cached.size() == 2 && cached[0].occurrence == edit::a &&
                            cached[1].occurrence == edit::b,
                        "Warm owner caches A then B before the Stop transaction");
                timeline.restore_committed(std::move(cached));
                timeline.advance(0ms);
            }
            require(!timeline.presentation() && port.released.empty(),
                    "Owner reaches the seam with no presentation and no release");
            Authority owner;
            (void)stop_a(f, edit_origin, owner);
            if (cold)
                f.reopen();
            const auto both = read_ordered(f, edit_origin, owner);
            require(both.stopped.independent_holds == std::vector{defer, review} &&
                        both.pending.size() == 2 && both.pending[0].occurrence == edit::a &&
                        both.pending[1].occurrence == edit::b,
                    "Both exact causes and the ordered suffix survive the Stop");
            const auto adoption = cold ? QueueAdoption::cold : QueueAdoption::warm;
            apply_ordered_selection(timeline, 100ms, adoption, both);
            require(!timeline.presentation() && port.released.empty(),
                    "Neither restore nor reconciliation starts a window");
            timeline.advance(100ms);
            require(!timeline.presentation() && port.released.empty(), "D and R together block B");

            resolve(f, defer, Resolution::continue_actions, {"operator", "owner-continue-D"});
            const auto only_review = read_ordered(f, edit_origin, owner);
            require(only_review.stopped.independent_holds == std::vector{review},
                    "Ordinary D resolution still leaves exact R active");
            port.a_holds = only_review.stopped.independent_holds;
            apply_ordered_selection(timeline, 200ms, QueueAdoption::warm, only_review);
            timeline.advance(200ms);
            require(!timeline.presentation() && port.released.empty(), "R alone still blocks B");

            resolve(f, review, Resolution::continue_actions, {"operator", "owner-continue-R"});
            const auto clear = read_ordered(f, edit_origin, owner);
            require(clear.stopped.independent_holds.empty() && clear.pending.size() == 1 &&
                        clear.pending[0].occurrence == edit::b,
                    "Normal readers report that stopped A is now passable");
            port.a_holds.clear();
            apply_ordered_selection(timeline, 300ms, QueueAdoption::warm, clear);
            require(!timeline.presentation() && port.released.empty(), "Adoption awaits advance");
            timeline.advance(300ms);
            const auto shown = timeline.presentation();
            require(shown && shown->milestone_ref == "B" && shown->elapsed == 0ms &&
                        port.released == std::vector{edit::b},
                    "B starts at elapsed zero with no release or gap for A");
        });
    }

    void s7OnlyExplicitSkipEndsReview_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::newRow("application") << false;
        QTest::newRow("edit") << true;
    }

    void s7OnlyExplicitSkipEndsReview() {
        QFETCH(bool, edit_origin);
        check([&] {
            RefundFixture f(edit_origin);
            (void)f.first_refund();
            advance_to_protected_successor(f.file);
            Authority owner;
            (void)stop_a(f, edit_origin, owner);
            require_held_head(f, edit_origin, owner, {defer, review});
            f.file.write([&](const auto &s) {
                require_defer_resolution(s, edit_origin);
                require_review(s);
            });
            const MilestoneDecisionEvidence evidence{"operator", "explicit-Skip-D"};
            resolve(f, defer, Resolution::skip, evidence);
            f.reopen();
            f.file.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions gates(s);
                require_defer_resolution(s, edit_origin, Resolution::skip, evidence);
                require_review(s, {}, {},
                               MilestoneSkipProvenance{MilestoneSkipOrigin::deferred_resolution,
                                                       defer.source_ref});
                require(gates.active_holds(edit::a).empty() &&
                            gates.pending_disposition(edit::a) == Gate::skipped,
                        "The real terminal Skip ends the remaining Review");
            });
            check_pass_and_release_b(f, edit_origin);
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006SettlementTests)
#include "settlement_tests.moc"
