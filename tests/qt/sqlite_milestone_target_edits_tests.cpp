// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/milestone_target_edit_fixture.hpp"

#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace milestone_target_edit_fixture;
using Choice = MilestoneActionChoice;
using Life = subathon::core::TimerLifecycle;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

constexpr auto definitions =
    "SELECT ordinal,milestone_ref,target_micro_points FROM "
    "subathon_milestone_definition_targets WHERE definition_revision="
    "(SELECT definition_revision FROM subathon_milestone_definition_heads) "
    "ORDER BY ordinal";
constexpr auto head_members =
    "SELECT ordinal,milestone_ref,definition_revision,target_micro_points,"
    "score_micro_points,origin,application_occurrence_ref,edit_occurrence_ref "
    "FROM subathon_milestone_prefix_members WHERE prefix_revision="
    "(SELECT prefix_revision FROM subathon_milestone_prefix_heads) ORDER BY ordinal";
constexpr auto edit_orders = "SELECT occurrence_ref,queue_order,released FROM "
                             "subathon_milestone_edit_occurrences ORDER BY queue_order";
constexpr auto application_group = "SELECT * FROM subathon_milestone_groups ORDER BY queue_order";
constexpr auto initial_choices =
    "SELECT occurrence_ref,choice FROM subathon_milestone_edit_choices "
    "ORDER BY occurrence_ref";
const SqlRow prior_member{
    1, QByteArray("A"), QByteArray("earned"), 100, 120, 0, QByteArray("z-predecessor"), QVariant{}};

SqlRows read_head_members(File &f) {
    auto rows = f.db.rows(head_members);
    for (auto &row : rows) {
        // Preserve exact values and normalize only the two SQL NULL parent references.
        for (const auto column : {6, 7}) {
            if (row.at(column).isNull())
                row[column] = QVariant{};
        }
    }
    return rows;
}

void expect_prior(File &f) {
    require(
        f.db.rows(definitions) == SqlRows{{1, QByteArray("A"), 100},
                                          {2, QByteArray("B"), 200},
                                          {3, QByteArray("C"), 300},
                                          {4, QByteArray("D"), 400}} &&
            read_head_members(f) == SqlRows{prior_member} &&
            f.db.rows("SELECT count(*) FROM subathon_milestone_edit_operations") == SqlRows{{0}} &&
            f.db.rows("SELECT count(*) FROM subathon_milestone_definition_versions") ==
                SqlRows{{1}} &&
            f.db.rows("SELECT count(*) FROM subathon_milestone_prefix_revisions") == SqlRows{{1}} &&
            f.db.rows(edit_orders).empty() && f.db.rows(initial_choices).empty(),
        "Both heads, version histories, audit, edit groups and choices retain the initial state");
}

void expect_ordered(const MilestoneEditReply &reply) {
    require(reply.status == Status::eligible && reply.result,
            "Confirmed edit returns the retained typed result");
    const auto &result = *reply.result;
    require(result.definition_revision == "definition-edit-ordered" &&
                result.prefix_revision == "prefix-edit-ordered" && result.achievements.size() == 2,
            "Exactly two captures chose the stable result revisions");
    const auto &first = result.achievements[0];
    const auto &second = result.achievements[1];
    require(first.order == 8 && first.choice == Choice::defer && first.group.occurrence == b &&
                first.group.achievement.milestone_ref == "B" &&
                first.group.achievement.target_micro_points == 150 &&
                first.group.achievement.score_micro_points == 175 &&
                first.group.timing.presentation == 11ms && first.group.timing.gap == 3ms &&
                second.order == 9 && second.choice == Choice::run && second.group.occurrence == c &&
                second.group.achievement.milestone_ref == "C" &&
                second.group.achievement.target_micro_points == 175 &&
                second.group.achievement.score_micro_points == 175 && second.group.actions.empty(),
            "Configuration order matches orders 8/9 and the exact choices, captures and score");
}

void expect_ordered_rows(File &f) {
    require(f.db.rows(edit_orders) ==
                    SqlRows{{QByteArray("b-edit"), 8, 0}, {QByteArray("a-edit"), 9, 0}} &&
                f.db.rows(initial_choices) ==
                    SqlRows{{QByteArray("a-edit"), 0}, {QByteArray("b-edit"), 1}} &&
                f.db.rows(definitions) == SqlRows{{1, QByteArray("A"), 100},
                                                  {2, QByteArray("B"), 150},
                                                  {3, QByteArray("C"), 175},
                                                  {4, QByteArray("D"), 250}},
            "One committed patch and complete exact choice set");
    const QByteArray rev("definition-edit-ordered");
    require(read_head_members(f) ==
                SqlRows{prior_member,
                        {2, QByteArray("B"), rev, 150, 175, 1, {}, QByteArray("b-edit")},
                        {3, QByteArray("C"), rev, 175, 175, 1, {}, QByteArray("a-edit")}},
            "Active prefix retains typed application A and edit-origin B and C");
}

class ReleaseAuthority final : public MilestoneEditReleaseAuthority {
  public:
    explicit ReleaseAuthority(const Authority &authority) : authority_(authority) {}

    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &selected_track,
                const MilestoneOccurrenceKey &occurrence) override {
        require(selected_track == track && (occurrence == b || occurrence == c),
                "Release rechecks current gates on the exact typed edit occurrence");
        const auto &life = authority_.facts.lifecycle;
        return life.timer.lifecycle != Life::Finished || life.post_finish_automation;
    }

  private:
    const Authority &authority_;
};

auto release_group(File &f, const MilestoneOccurrenceKey &key) {
    const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return key == a ? release_milestone_head(*f.db.writer, key, std::move(done))
                        : release_milestone_head(*f.db.writer, key,
                                                 std::make_shared<ReleaseAuthority>(f.authority),
                                                 std::move(done));
    });
    finance::normal(result.work);
    require(result.disposition.has_value(), "Acknowledged separate release outcome");
    return *result.disposition;
}

void require_refusal(const MilestoneEditReply &result, Status expected) {
    require(result.status == expected && !result.result, "Exact refusal without a typed success");
}

// Fake timeline observer. Each selected disposition and release still uses
// its owning public SQLite boundary on a separate real writer unit.
class TimelinePort final : public MilestoneGroupRelease {
  public:
    explicit TimelinePort(File &file) : file_(file) {}
    bool review_held(const subathon::core::MilestoneCompletion &) const override {
        throw std::logic_error("Fixture uses only exact keyed milestone occurrences");
    }
    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &key) const override {
        MilestonePendingDisposition current{};
        file_.db.write([&](const auto &s) {
            current = SqliteMilestoneGroupDecisions(s).pending_disposition(key);
        });
        switch (current) {
        case MilestonePendingDisposition::ready:
            return MilestoneHeadDisposition::ready;
        case MilestonePendingDisposition::held:
            return MilestoneHeadDisposition::held;
        case MilestonePendingDisposition::skipped:
            return MilestoneHeadDisposition::skipped;
        case MilestonePendingDisposition::absent_or_released:
            return std::nullopt;
        }
        return std::nullopt;
    }
    void release(CommittedMilestoneGroup group) override {
        require(group.occurrence && release_group(file_, *group.occurrence) ==
                                        MilestoneReleaseDisposition::released,
                "Exact pending head acknowledges release before any fake effect");
        started.push_back(*group.occurrence);
    }
    std::vector<MilestoneOccurrenceKey> started;

  private:
    File &file_;
};

class EditDispatch final : public MilestoneEditDispatchAuthority {
  public:
    bool permit{};
    bool permit_retry{};
    int calls{};
    int retry_calls{};
    bool allows(const MilestoneEditJob &job) override {
        ++calls;
        require(job.action_document == QByteArray::fromHex("0081") &&
                    job.capture.sender_ref == "edit-sender" &&
                    job.capture.route_ref == "local/route",
                "Authority receives the exact immutable captured action");
        return permit;
    }
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override {
        ++retry_calls;
        return permit_retry ? std::optional<std::string>{"current-policy"} : std::nullopt;
    }
};

class BonusPorts final : public MilestoneBonusAuthority, public MilestoneBonusEffect {
  public:
    bool permitted{};
    unsigned effects{};

    bool allows(const MilestoneBonusKey &key, const MilestoneAction &capture) override {
        require(key.occurrence == b && key.action_ordinal == 1 &&
                    capture == MilestoneAction{MilestoneActionKind::bonus, "award", "award-v1"},
                "Bonus authority receives the exact typed capture");
        return permitted;
    }

    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &,
                      const MilestoneAction &) override {
        ++effects;
        const auto effect = QByteArray("effect-B");
        write(s, "INSERT INTO milestone_fixture_bonus VALUES(?)", {effect});
        return effect.toStdString();
    }
};

struct RefundEffects final : FinancialRefundEffectPort {
    finance::Effects delegate;

    RefundEffects() { delegate.apply(-25000, 0, {25000}); }
    FinancialEffectObservation observe(const SqliteWorkSession &s,
                                       const ApplicationStateSnapshot &snapshot) override {
        return delegate.observe(s, snapshot);
    }
    RefundTimerActual timer_effect(const SqliteWorkSession &s, const GroupedRefundEntitlement &e,
                                   const FinancialEffectObservation &o,
                                   subathon::core::RefundDeficitPolicy policy) override {
        return delegate.timer_effect(s, e, o, policy);
    }
    std::vector<FinancialMilestoneTarget> milestones(const SqliteWorkSession &) override {
        // Deliberately give the old capture's target. The v18 writer must
        // resolve the current target 185 at its own authoritative boundary.
        return {{b, 175, MilestoneRefundPolicy::review}};
    }
};

void expect_refund(const FinancialRefundRecord &record) {
    require(record.payment_position == 1 && record.entitlement.accepted_money == 1 &&
                record.milestone_observations.size() == 1,
            "One genuine EUR 1 refund and one crossing recorded");
    const auto &m = record.milestone_observations[0];
    require(m.occurrence == b && m.refund_ref == "crossing-refund" &&
                m.score_before_micro_points == 190 && m.score_after_micro_points == 180 &&
                m.current_target_micro_points == 185 && m.policy == MilestoneRefundPolicy::review,
            "Historical readback binds the exact edit occurrence and 190/180/185 operands");
}

} // namespace

class SqliteMilestoneTargetEditsTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void bootstrapNeedsAuthoritativePrefixAndExactApplicationOrigin() {
        check([] {
            File f(55, 7, targets, false);
            f.db.write([&](const auto &s) {
                auto absent = seed();
                absent.completion_prefix.reset();
                require(!initialize_milestone_track(s, absent),
                        "Absent prefix evidence cannot be reconstructed from queue or score");
                auto wrong = seed();
                wrong.completion_prefix->front().application.operation_ref = "unrelated-origin";
                require(!initialize_milestone_track(s, wrong),
                        "Refuse the wrong application parent");
                require(initialize_milestone_track(s, seed()), "Persist the exact explicit prefix");
            });
            expect_prior(f);
        });
    }

    void explicitlyEmptyPrefixIsSupportedWithoutBackfill() {
        check([] {
            File f(55, 7, targets, false);
            const auto intent = single("uninitialized", {"B", 180});
            const auto unavailable = f.preview(intent);
            require(unavailable.plan.status == Status::unavailable && !unavailable.confirmation,
                    "Uninitialized scope cannot derive heads from retained history");
            f.db.write([&](const auto &s) {
                auto input = seed();
                input.completion_prefix->clear();
                require(initialize_milestone_track(s, input),
                        "Explicitly empty prefix initializes");
            });
            require(read_head_members(f).empty() && f.db.rows(application_group).size() == 1,
                    "Queue history supplies no inferred completion");
        });
    }

    void orderedPatchBindsAllChoicesAndCommitsOneSuffix() {
        check([] {
            File f;
            const auto intent = ordered();
            auto incomplete = intent;
            incomplete.choices.clear();
            const auto draft = f.preview(incomplete);
            require(draft.plan.status == Status::eligible && draft.achievements.size() == 2 &&
                        !draft.confirmation,
                    "Draft exposes affected identities with choices unset");
            for (const auto &choices : {
                     std::vector<MilestoneAcceptanceChoice>{{b, Choice::run}},
                     std::vector<MilestoneAcceptanceChoice>{{b, Choice::run}, {b, Choice::skip}},
                     std::vector<MilestoneAcceptanceChoice>{
                         {b, Choice::run}, {c, Choice::run}, {a, Choice::skip}},
                     std::vector<MilestoneAcceptanceChoice>{{b, Choice(99)}, {c, Choice::run}},
                 }) {
                auto invalid = intent;
                invalid.choices = choices;
                const auto rejected = f.preview(invalid);
                require(rejected.plan.status == Status::invalid_choices && !rejected.confirmation,
                        "Missing, duplicate, extra and invalid choices refuse");
            }
            const auto p = f.preview(intent);
            File::confirmable(p);
            require(p.score == 175 && p.score_head == "awarded" && p.previous_targets.size() == 4 &&
                        p.previous_targets[1].target_micro_points == 200 &&
                        p.prior_definition_revision == "definition-initial" &&
                        p.next_definition_revision == "definition-edit-ordered" &&
                        p.achievements.size() == 2 && p.achievements[0].occurrence == b &&
                        p.achievements[1].occurrence == c &&
                        p.plan.after.remaining_micro_points == 75 && p.plan.after.interval &&
                        p.plan.after.interval->fraction == 0,
                    "Preview binds score, old/new revisions, configured suffix and 0/75 interval");
            expect_prior(f);
            expect_ordered(f.confirm(intent, p));
            expect_ordered_rows(f);
            require(f.db.rows("SELECT count(*) FROM subathon_app_operations") == SqlRows{{1}},
                    "No application operation or synthetic receipt is manufactured for the edit");
        });
    }

    void restoredTailRespectsWindowGapAndIndependentDefer_data() {
        QTest::addColumn<bool>("skip");
        QTest::newRow("continued-defer") << false;
        QTest::newRow("terminal-skip") << true;
    }

    void restoredTailRespectsWindowGapAndIndependentDefer() {
        QFETCH(bool, skip);
        check([&] {
            File f;
            const auto intent = ordered();
            expect_ordered(f.confirm(intent, f.preview(intent)));
            f.reopen();

            std::vector<CommittedMilestoneGroup> groups;
            f.db.write([&](const auto &s) {
                const auto pending = SqliteMilestoneGroupStore(s).pending();
                require(pending.size() == 3 && pending[0].group.occurrence == a &&
                            pending[1].group.occurrence == b && pending[2].group.occurrence == c,
                        "Cold restore retains one typed ordered suffix, including the held head");
                for (const auto &entry : pending)
                    groups.push_back(entry.group);
            });
            TimelinePort port(f);
            MilestoneTimeline timeline({20ms, 5ms, true}, port);
            timeline.restore_committed(std::move(groups));
            require(!timeline.presentation() && port.started.empty(),
                    "Cold restore starts nothing");
            timeline.advance(0ms);
            timeline.advance(10ms);
            timeline.advance(12ms);
            require(port.started == std::vector{a} && !timeline.presentation(),
                    "At and after the predecessor's gap, B is still held");
            require(release_group(f, c) == MilestoneReleaseDisposition::refused,
                    "No overtaking the held head by exact later key");
            f.db.write([&](const auto &s) {
                auto decisions = SqliteMilestoneGroupDecisions(s);
                require(decisions.active_holds(b) ==
                                std::vector{MilestoneHoldKey{
                                    MilestoneHoldCause::deferred_acceptance, "decision-ordered"}} &&
                            decisions.resolve(
                                b, {MilestoneHoldCause::deferred_acceptance, "decision-ordered"},
                                skip ? MilestoneHoldResolution::skip
                                     : MilestoneHoldResolution::continue_actions,
                                {"tester", "resolve-b"}),
                        "Resolve only the selected explicit Defer");
            });
            timeline.advance(13ms);
            if (skip) {
                require(port.started == std::vector{a, c} &&
                            f.db.rows(edit_orders) ==
                                SqlRows{{QByteArray("b-edit"), 8, 0}, {QByteArray("a-edit"), 9, 1}},
                        "Terminally skipped B consumes no window or gap, and C releases at 13 ms");
                return;
            }
            timeline.advance(24ms);
            timeline.advance(26ms);
            require(port.started == std::vector{a, b},
                    "C waits until B's 11 ms window and 3 ms gap end");
            timeline.advance(27ms);
            require(port.started == std::vector{a, b, c}, "Run starts after B's gap at elapsed 27");
        });
    }

    void mixedGrantsAndSkipPreserveHistoryWithoutTakingQueueTime() {
        check([] {
            File f(55, 7, {{"A", 100}, {"B", 200}});
            const MilestoneEditIntent intent{{track, "mixed"},
                                             {"tester", "decision-mixed"},
                                             {{"A", 110}, {"B", 175}},
                                             {{b, Choice::skip}}};
            for (const bool unfinished : {false, true}) {
                f.authority.facts.unfinished_grant = unfinished;
                f.authority.facts.correction_grant = !unfinished;
                const auto denied = f.preview(intent);
                require(denied.plan.status == Status::denied && !denied.confirmation,
                        "Either single current grant refuses the whole mixed patch");
                require(read_head_members(f) == SqlRows{prior_member} &&
                            f.db.rows(edit_orders).empty(),
                        "Denied mixed patch makes no partial correction or achievement");
            }
            f.authority.facts.unfinished_grant = true;
            f.authority.facts.correction_grant = true;
            const auto p = f.preview(intent);
            require(!p.plan.after.next && !p.plan.after.interval,
                    "The skipped completion ends the track");
            const auto confirmed = f.confirm(intent, p);
            require(confirmed.result && confirmed.result->achievements.size() == 1 &&
                        confirmed.result->achievements.front().choice == Choice::skip &&
                        confirmed.result->achievements.front().order == 8 &&
                        read_head_members(f).front() == prior_member,
                    "Skip retains exact occurrence and capture, with original A intact");
            f.db.write([&](const auto &s) {
                const auto pending = SqliteMilestoneGroupStore(s).pending();
                require(pending.size() == 1 && pending.front().group.occurrence == a &&
                            SqliteMilestoneGroupDecisions(s).pending_disposition(b) ==
                                MilestonePendingDisposition::skipped,
                        "The terminal edit remains auditable and consumes no pending presentation");
            });
        });
    }

    void correctionsAndRefusalsKeepCapturedCompletionAndWriterState() {
        check([] {
            File f(10);
            const auto baseline_timer =
                f.db.rows("SELECT * FROM subathon_app_states ORDER BY token");
            const auto baseline_group = f.db.rows(application_group);
            for (const auto &edit : {Target{"B", 100}, Target{"B", 301}}) {
                const auto denied = f.preview(single("invalid", edit));
                require(denied.plan.status == Status::invalid_patch && !denied.confirmation,
                        "Refuse tie and crossing, preserving identity order");
                expect_prior(f);
            }
            auto &authority = f.authority.facts;
            authority.unfinished_grant = false;
            authority.lifecycle.timer = finance::timer(Life::Finished, 0);
            authority.lifecycle.post_finish_progression = false;
            const auto correction = single("finished-correction", {"A", 150});
            const auto p = f.preview(correction);
            require(p.score == 130 && p.plan.after.track_deficit_micro_points == 20 &&
                        p.plan.after.remaining_micro_points == 70 && p.plan.after.interval &&
                        p.plan.after.interval->earned_micro_points == 0 &&
                        p.plan.after.interval->span_micro_points == 50,
                    "Current target 150 yields deficit 20 and exact next interval 0/50");
            const auto committed = f.confirm(correction, p);
            require(committed.result && committed.result->achievements.empty() &&
                        read_head_members(f) == SqlRows{prior_member} &&
                        f.db.rows(application_group) == baseline_group &&
                        f.db.rows("SELECT * FROM subathon_app_states ORDER BY token") ==
                            baseline_timer &&
                        f.db.rows("SELECT * FROM subathon_milestone_edit_refund_decisions").empty(),
                    "Historical target 100, earned score 120 and group remain byte-identical");
        });
    }

    void finishAndReopenInvalidateAnActiveBindingAtIdenticalScore() {
        check([] {
            File f;
            const auto intent = ordered();
            const auto active = f.preview(intent);
            auto &life = f.authority.facts.lifecycle;
            life.revision = "finished-after-preview";
            life.timer = finance::timer(Life::Finished, 0);
            life.post_finish_progression = false;
            require_refusal(f.confirm(intent, active), Status::stale);
            const auto finished = f.preview(intent);
            require(finished.plan.status == Status::finished_progression_disabled &&
                        !finished.confirmation && finished.score == 175,
                    "Fresh Finished preview refuses newly reachable identities");
            expect_prior(f);

            life.revision = "reopened-after-finish";
            life.timer = {Life::Active, subathon::core::CountdownPhase::Grace, 0, 9000, false};
            life.debt_ms = 17;
            life.deadline_utc_ms = 40000;
            life.pause_reasons = {"operator", "recovery"};
            life.reopen_pending_action_revision = "reopen-choices";
            const auto unchanged_head = f.db.rows("SELECT * FROM subathon_app_head");
            require_refusal(f.confirm(intent, active), Status::stale);
            const auto reopened = f.preview(intent);
            require(reopened.score_head == active.score_head && reopened.score == 175 &&
                        reopened.lifecycle.revision == "reopened-after-finish" &&
                        reopened.lifecycle.timer.phase == subathon::core::CountdownPhase::Grace &&
                        reopened.lifecycle.timer.grace_remaining_ms == 9000 &&
                        reopened.lifecycle.debt_ms == 17 &&
                        reopened.lifecycle.deadline_utc_ms == 40000 &&
                        reopened.lifecycle.pause_reasons == life.pause_reasons &&
                        reopened.lifecycle.reopen_pending_action_revision == "reopen-choices",
                    "Fresh preview uses exact owner-supplied reopened observations");
            expect_ordered(f.confirm(intent, reopened));
            require(f.db.rows("SELECT * FROM subathon_app_head") == unchanged_head,
                    "Edit does not replace timer/application state");
        });
    }

    void exactScoreAndWholeSuffixOrderBounds() {
        check([] {
            File edge(max);
            edge.db.write([](const auto &s) { File::insert_score(s, "third", min); });
            const auto good = edge.preview(single("sum", {"B", 180}));
            require(good.plan.status == Status::eligible && good.score == 119,
                    "120 + MAX + MIN equals 119 after full signed summation; count 700 is ignored");
            edge.db.write([](const auto &s) {
                write(s, "DELETE FROM subathon_app_balances "
                         "WHERE token=(SELECT token FROM subathon_app_head) "
                         "AND target_ref=X'7468697264'");
                write(s, "UPDATE subathon_app_states SET balance_count=balance_count-1 "
                         "WHERE token=(SELECT token FROM subathon_app_head)");
            });
            const auto overflow = edge.preview(single("overflow", {"B", 180}));
            require(overflow.plan.status != Status::eligible && !overflow.confirmation,
                    "120 + MAX is outside signed score range and issues no binding");
            expect_prior(edge);

            File tail(55, max - 1);
            tail.db.write([](const auto &s) {
                write(s, "UPDATE subathon_milestone_groups SET released=1 WHERE queue_order=?",
                      {qint64(max - 1)});
            });
            const auto rejected = tail.preview(ordered());
            require(rejected.plan.status == Status::order_exhausted && !rejected.confirmation,
                    "The released tail still leaves one order for two occurrences");
            expect_prior(tail);
        });
    }

    void oldReachableBacklogAndFinishedSuppressionRemainDistinct() {
        check([] {
            File f(70, 7, {{"A", 100}, {"B", 180}, {"C", 300}, {"D", 400}});
            const auto backlog = f.preview(single("backlog", {"B", 170}, {{b, Choice::run}}));
            require(backlog.plan.status == Status::old_reachable_backlog && !backlog.confirmation &&
                        f.db.rows(edit_orders).empty() && f.db.rows(initial_choices).empty() &&
                        read_head_members(f) == SqlRows{prior_member} &&
                        f.db.rows("SELECT count(*) FROM subathon_milestone_edit_operations") ==
                            SqlRows{{0}},
                    "Synthetic score 190 refuses old reachable B without a hold, edit or prefix "
                    "change");
            auto &life = f.authority.facts.lifecycle;
            life.revision = "finished-no-progression";
            life.timer = finance::timer(Life::Finished, 0);
            life.post_finish_progression = false;
            const auto correction = single("suppressed", {"A", 150});
            const auto p = f.preview(correction);
            require(p.plan.suppressed && p.plan.after.next &&
                        p.plan.after.next->milestone_ref == "B" &&
                        p.plan.after.remaining_micro_points == 0 && !p.plan.after.interval &&
                        p.achievements.empty(),
                    "Previously reachable B stays suppressed with zero remaining and no fraction");
            const auto reply = f.confirm(correction, p);
            require(reply.result && reply.result->achievements.empty() &&
                        read_head_members(f) == SqlRows{prior_member},
                    "Permitted correction preserves actual completion prefix");
        });
    }

    void finishedRunStaysBehindIndependentAutomationGate() {
        check([] {
            File f;
            auto &life = f.authority.facts.lifecycle;
            life.revision = "finished-progression-only";
            life.timer = finance::timer(Life::Finished, 0);
            life.post_finish_automation = false;
            f.authority.facts.captures[0] = capture_b(true);
            const auto intent = single("automation-gate", {"B", 175}, {{b, Choice::run}});
            const auto p = f.preview(intent);
            require(p.plan.automation_gated && p.lifecycle.timer.lifecycle == Life::Finished,
                    "Preview exposes the separate disabled automation gate");
            const auto saved = f.confirm(intent, p);
            require(saved.result && saved.result->achievements.size() == 1 &&
                        saved.result->achievements[0].choice == Choice::run,
                    "Run remains durable under the automation gate");
            require(release_group(f, a) == MilestoneReleaseDisposition::released &&
                        release_group(f, b) == MilestoneReleaseDisposition::refused,
                    "Released predecessor permits selecting B, but disabled automation refuses B");
            EditDispatch authority;
            authority.permit = true;
            f.db.write([&](const auto &s) {
                require(claim_milestone_edit_job(s, {intent.key, b, 1}, 1, authority).disposition ==
                            DeliveryDisposition::refused,
                        "Current dispatch authorization cannot bypass the release/automation gate");
            });
        });
    }

    void editJobUsesExactTypeReleaseClaimAndRecovery() {
        check([] {
            File f(55, 7, targets, true, true);
            auto intent = ordered();
            intent.choices = {{b, Choice::run}, {c, Choice::run}};
            auto &captures = f.authority.facts.captures;
            captures[0] = capture_b(true);
            auto second = capture_b(true);
            second.milestone_ref = "C";
            second.occurrence = c;
            captures[1] = second;
            const auto p = f.preview(intent);
            const auto first = f.confirm(intent, p);
            require(first.result && first.result->achievements.size() == 2,
                    "Both typed edit job parents committed");
            const auto retry = f.confirm(intent, p);
            require(retry.result && retry.result->achievements[0].order == 8 &&
                        retry.result->achievements[1].order == 9 &&
                        f.db.rows(edit_orders).size() == 2,
                    "Exact authorized retry has the same pair and no extra queue effect");
            require(
                f.db.rows(
                    "SELECT typeof(job_format),job_format,typeof(action_document),"
                    "action_document FROM subathon_milestone_edit_jobs ORDER BY occurrence_ref") ==
                    SqlRows(2, SqlRow{QString("blob"), QByteArray(delivery_job_format),
                                      QString("blob"), QByteArray::fromHex("0081")}),
                "Both positions own exact immutable format and payload BLOBs");
            captures[0].jobs[0].envelope.document = QByteArray::fromHex("0080");
            require_refusal(f.confirm(intent, p), Status::key_conflict);
            captures[0] = capture_b(true);

            const MilestoneEditJobKey key_b{intent.key, b, 1};
            const MilestoneEditJobKey key_c{intent.key, c, 1};
            EditDispatch authority;
            const auto claim = [&](const auto &key, std::int64_t revision,
                                   DeliveryDisposition disposition,
                                   std::optional<std::int64_t> sending) {
                f.db.write([&](const auto &s) {
                    const auto attempt = claim_milestone_edit_job(s, key, revision, authority);
                    require(attempt.disposition == disposition &&
                                (sending ? attempt.job && attempt.job->key == key &&
                                               attempt.job->attempt == sending &&
                                               attempt.job->revision == sending &&
                                               attempt.job->state == DeliveryJobState::sending
                                         : !attempt.job),
                            "Exact typed claim, revision and attempt");
                });
            };
            f.db.write([&](const auto &s) {
                const auto job = lookup_milestone_edit_job(s, key_b);
                require(job && job->key == key_b && job->revision == 1 &&
                            job->state == DeliveryJobState::queued && !job->attempt &&
                            job->action_document == QByteArray::fromHex("0081") &&
                            !lookup_milestone_edit_job(s, {key_b.operation, b, 2}),
                        "Lookup resolves only the exact typed edit position");
            });
            authority.permit = true;
            claim(key_b, 1, DeliveryDisposition::refused, {});
            require(release_group(f, a) == MilestoneReleaseDisposition::released &&
                        release_group(f, b) == MilestoneReleaseDisposition::released,
                    "Release strictly follows the committed cross-origin pending head");
            authority.permit = false;
            claim(key_b, 1, DeliveryDisposition::refused, {});
            authority.permit = true;
            claim(key_b, 1, DeliveryDisposition::changed, 2);
            f.db.write([&](const auto &s) {
                const auto stale = finish_milestone_edit_job(
                    s, key_b, 2, 1, DeliveryEvidence::known_undelivered, "stale-attempt");
                require(stale.disposition == DeliveryDisposition::stale,
                        "A foreign attempt cannot finish the selected typed job");
                const auto failed = finish_milestone_edit_job(
                    s, key_b, 2, 2, DeliveryEvidence::known_undelivered, "known-undelivered");
                require(failed.job && failed.job->state == DeliveryJobState::failed &&
                            failed.job->revision == 3 && failed.job->attempt == 2,
                        "Evidence applies at revision 3 to the exact sending attempt");
                require(retry_milestone_edit_job(s, key_b, 3, authority).disposition ==
                            DeliveryDisposition::refused,
                        "Known non-delivery without current policy remains failed");
                authority.permit_retry = true;
                const auto requeued = retry_milestone_edit_job(s, key_b, 3, authority);
                require(requeued.job && requeued.job->state == DeliveryJobState::queued &&
                            requeued.job->revision == 4 &&
                            requeued.job->action_document == QByteArray::fromHex("0081"),
                        "Current-policy retry retains the original bytes and adds revision 4");
            });
            require(release_group(f, c) == MilestoneReleaseDisposition::released,
                    "Release the later head");
            claim(key_c, 1, DeliveryDisposition::changed, 2);
            f.reopen();
            f.db.write([&](const auto &s) {
                const auto application = stranded_milestone_delivery(s, std::nullopt, 1);
                require(application.attempts.empty() && application.next_after &&
                            std::holds_alternative<DeliveryJobKey>(*application.next_after),
                        "The earlier origin's queued application job is examined first");
                const auto &legacy = std::get<DeliveryJobKey>(*application.next_after);
                require(finance::same(legacy.operation, finance::original) &&
                            legacy.job_ref == "original-job",
                        "Continuation carries the genuine application key");
                // C's BLOB ref a-edit sorts before b-edit within this edit.
                const auto page = stranded_milestone_delivery(s, application.next_after, 1);
                require(page.attempts.size() == 1 && page.attempts[0].revision == 2 &&
                            std::get<MilestoneEditJobKey>(page.attempts[0].key) == key_c &&
                            page.next_after &&
                            std::get<MilestoneEditJobKey>(*page.next_after) == key_c,
                        "Cold scan carries the exact edit discriminator and cursor");
                const auto next = stranded_milestone_delivery(s, page.next_after, 1);
                require(next.attempts.empty() && next.next_after &&
                            std::get<MilestoneEditJobKey>(*next.next_after) == key_b,
                        "Queued B is examined in BLOB order, without returning an attempt");
                require(!stranded_milestone_delivery(s, next.next_after, 1).next_after,
                        "Exact-full page exhausts on the next empty page");
                const auto recovered = recover_milestone_edit_job(s, key_c, 2);
                require(recovered.job &&
                            recovered.job->state == DeliveryJobState::outcome_unknown &&
                            recovered.job->revision == 3 && recovered.job->attempt == 2,
                        "Exclusive recovery records unknown with the original attempt");
                const auto calls = authority.retry_calls;
                require(retry_milestone_edit_job(s, key_c, 3, authority).disposition ==
                                DeliveryDisposition::refused &&
                            authority.retry_calls == calls,
                        "Unknown outcome never reaches a policy replay decision");
            });
        });
    }

    void releasedEditBonusCommitsOneExactEffectAndReceipt() {
        check([] {
            File f;
            const auto earn = single("bonus", {"B", 175}, {{b, Choice::run}});
            require(f.confirm(earn, f.preview(earn)).result.has_value(),
                    "Record the bonus-only Run");
            f.db.write([](const auto &s) {
                write(s, "CREATE TABLE milestone_fixture_bonus(effect BLOB NOT NULL PRIMARY KEY)");
            });
            BonusPorts ports;
            const auto apply = [&](MilestoneBonusDisposition expected) {
                f.db.write([&](const auto &s) {
                    require(SqliteMilestoneGroupStore(s).apply_bonus({b, 1}, ports, ports) ==
                                expected,
                            "Exact edit bonus disposition");
                });
            };
            ports.permitted = true;
            apply(MilestoneBonusDisposition::refused);
            require(ports.effects == 0, "Unreleased bonus has no effect");
            require(release_group(f, a) == MilestoneReleaseDisposition::released &&
                        release_group(f, b) == MilestoneReleaseDisposition::released,
                    "Separate committed releases select A then B");
            ports.permitted = false;
            apply(MilestoneBonusDisposition::refused);
            ports.permitted = true;
            apply(MilestoneBonusDisposition::applied);
            f.reopen();
            apply(MilestoneBonusDisposition::already_applied);
            require(ports.effects == 1 &&
                        f.db.rows("SELECT effect FROM milestone_fixture_bonus") ==
                            SqlRows{{QByteArray("effect-B")}} &&
                        f.db.rows("SELECT occurrence_ref,action_ordinal,effect_ref FROM "
                                  "subathon_milestone_edit_bonus_receipts") ==
                            SqlRows{{QByteArray("b-edit"), 1, QByteArray("effect-B")}},
                    "One real effect has its own durable typed receipt across restart");
        });
    }

    void genuineRefundUsesCurrentTargetAndKeepsImmutableTypedReadback() {
        check([] {
            File f(70);
            f.authority.facts.captures[0] = capture_b(true);
            const auto earn = single("earn", {"B", 175}, {{b, Choice::defer}});
            auto result = f.confirm(earn, f.preview(earn));
            require(result.result && result.result->achievements.size() == 1,
                    "B captures a real pending edit-origin group and job");
            const auto correction = single("correct-up", {"B", 185});
            const auto corrected = f.preview(correction);
            require(corrected.score == 190, "Use complete 120 + 70 snapshot and ignore count");
            require(f.confirm(correction, corrected).result.has_value() &&
                        f.db.rows("SELECT * FROM subathon_milestone_edit_refund_decisions").empty(),
                    "Editing the captured target does not run refund observation");

            // EUR 1 of 12, time 300000/12, score 120/12. Whole count remains
            // indivisible until a full refund, with residual 700/12.
            const auto share = finance::fraction(1, 12);
            const auto count = finance::fraction(175, 3);
            const FinancialEntitlement entitlement{
                0,
                1,
                share,
                false,
                GroupedRefundEntitlement{{finance::original, 1, finance::CK::main_delta},
                                         1,
                                         0,
                                         {},
                                         {25000, 0},
                                         25000,
                                         0,
                                         -25000,
                                         0},
                {{2,
                  {finance::original, 2, finance::CK::score},
                  0,
                  0,
                  10,
                  -10,
                  share,
                  10,
                  0,
                  false},
                 {3,
                  {finance::original, 3, finance::CK::whole_count},
                  0,
                  0,
                  0,
                  0,
                  share,
                  count,
                  count,
                  false}}};
            const auto receipt =
                f.db.refund("crossing-delivery", finance::core::ReceiptMoney{"1.00", "EUR"});
            const auto command = f.db.command("crossing-refund", receipt, 0);
            RefundEffects effects;
            const auto applied = f.db.run_reply([&](const auto &s) {
                return stage_financial_refund(s, command, entitlement, f.db.authority, effects);
            });
            require(applied.disposition == FinancialDisposition::applied,
                    "The real financial refund service applies the selected correlated payment");
            expect_refund(std::get<FinancialRefundRecord>(applied.record));

            f.db.write([&](const auto &s) {
                const auto decision =
                    SqliteMilestoneGroupDecisions(s).refund_audit(b, "crossing-refund");
                const auto holds = SqliteMilestoneGroupDecisions(s).active_holds(b);
                require(decision && decision->current_target_micro_points == 185 &&
                            decision->deficit_micro_points == 5 &&
                            std::ranges::find(
                                holds, MilestoneHoldKey{MilestoneHoldCause::deferred_acceptance,
                                                        "decision-earn"}) != holds.end() &&
                            std::ranges::find(holds,
                                              MilestoneHoldKey{MilestoneHoldCause::refund_review,
                                                               "crossing-refund"}) != holds.end() &&
                            holds.size() == 2,
                        "Review and Defer are separate causes with exact current deficit");
            });
            require(
                f.db.rows("SELECT target_micro_points FROM subathon_milestone_edit_occurrences") ==
                    SqlRows{{175}},
                "Captured target 175 survives the refund");
            const auto later = single("correct-down", {"B", 165});
            const auto post_refund = f.preview(later);
            require(post_refund.score == 180 && f.confirm(later, post_refund).result,
                    "The same exact aggregate selects the refund's committed 110+70 snapshot");
            f.reopen();
            f.db.write([&](const auto &s) {
                const auto record = SqliteFinancialRefundStore(s).lookup(command.operation);
                const auto history = SqliteFinancialRefundStore(s).history(f.db.payment, 16);
                require(record && history && history->refunds.size() == 1,
                        "Later true selected readback retains the earlier v15 document encoding");
                expect_refund(*record);
                expect_refund(history->refunds.front());
                const auto decisions = SqliteMilestoneGroupDecisions(s);
                require(decisions.resolve(b, {MilestoneHoldCause::refund_review, "crossing-refund"},
                                          MilestoneHoldResolution::continue_actions,
                                          {"tester", "reviewed"}) &&
                            decisions.active_holds(b) ==
                                std::vector{MilestoneHoldKey{
                                    MilestoneHoldCause::deferred_acceptance, "decision-earn"}},
                        "Resolving Review leaves the original Defer independently active");
            });
            require(f.db.rows(definitions)[1] == SqlRow{2, QByteArray("B"), 165},
                    "The live definition now differs from the refund's immutable target");
        });
    }

    void reachedChoiceFailureRollsBackAndLostAckReconcilesOnce() {
        check([] {
            File f;
            const auto intent = ordered();
            const auto competing = single("competing", {"B", 180});
            const auto old_competing = f.preview(competing);
            const auto p = f.preview(intent);
            File::confirmable(p);
            const auto prior_group = f.db.rows(application_group);
            bool reached = false;
            const auto failed = f.db.work(
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    expect_ordered(confirm_milestone_target_edit(s, f.authority, actor, intent,
                                                                 *p.confirmation));
                    require(
                        s.execute({definitions, {}})[1] == SqlRow{2, QByteArray("B"), 150} &&
                            s.execute({head_members, {}}).size() == 3 &&
                            s.execute({edit_orders, {}}).size() == 2 &&
                            s.execute({initial_choices, {}}).size() == 2 &&
                            s.execute({"SELECT count(*) FROM subathon_milestone_edit_operations",
                                       {}}) == SqlRows{{1}},
                        "Observe definition, audit, both completions, groups and choices before "
                        "the fault");
                    expect_prior(f);
                    reached = true;
                    // A real late duplicate insertion in the initial-choice table.
                    // SQL failure occurs after the full stage, inside the same unit.
                    write(s,
                          "INSERT INTO subathon_milestone_edit_choices "
                          "SELECT * FROM subathon_milestone_edit_choices WHERE occurrence_ref=?",
                          {finance::bytes(c.ref)});
                    s.commit();
                    return 1;
                },
                false);
            require(reached && failed.transaction == TransactionOutcome::rolled_back &&
                        failed.error == WorkError::sql_failure && !failed.value,
                    "Reached late physical choice insertion causes a known full rollback");
            expect_prior(f);
            require(f.db.rows(application_group) == prior_group,
                    "Prior tail 7 and release history stay intact");

            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *f.db.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Arm existing writer acknowledgement-loss seam on the idle writer");
            reached = false;
            const auto ambiguous = f.db.work(
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    expect_ordered(confirm_milestone_target_edit(s, f.authority, actor, intent,
                                                                 *p.confirmation));
                    reached = true;
                    s.commit();
                    return 1;
                },
                false);
            require(reached && ambiguous.transaction == TransactionOutcome::outcome_unknown &&
                        ambiguous.error == WorkError::uncertain_outcome && !ambiguous.value,
                    "Lost COMMIT acknowledgement publishes neither success nor refusal");
            f.db.observer.reset();
            f.db.writer.reset();
            f.db.open();
            f.db.write([&](const auto &s) {
                const auto recovered = read_milestone_target_edit(s, f.authority, actor, intent);
                expect_ordered({Status::eligible, recovered});
            });
            expect_ordered(f.confirm(intent, p));
            expect_ordered_rows(f);
            require_refusal(f.confirm(competing, old_competing), Status::stale);
            require(f.db.rows("SELECT count(*) FROM subathon_milestone_edit_operations") ==
                            SqlRows{{1}} &&
                        f.db.rows(application_group) == prior_group,
                    "One durable result, revisions, choices and positions with no release replay");
        });
    }
};

QTEST_GUILESS_MAIN(SqliteMilestoneTargetEditsTests)
#include "sqlite_milestone_target_edits_tests.moc"
