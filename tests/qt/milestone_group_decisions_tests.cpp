// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/milestone_group_restart_fixture.hpp"

#include <QTest>
#include <limits>
#include <subathon/server/milestone_group_decisions.hpp>

namespace {
using namespace milestone_group_restart_fixture;
using Choice = MilestoneActionChoice;
using Mode = PausedAcceptanceMode;
using Policy = MilestoneRefundPolicy;
using Resolution = MilestoneHoldResolution;
using Release = MilestoneReleaseDisposition;

const MilestoneOccurrenceKey first{"z-first"}, second{"a-second"};
const DeliveryJobKey first_job{origin, "job"}, delivered_job{origin, "done"},
    second_job{following_origin, "job"};
const MilestoneBonusKey first_bonus{first, 2}, second_bonus{second, 1};
const MilestoneDecisionEvidence input_review{"operator-a", "accept-31"};
const MilestoneDecisionEvidence action_review{"operator-b", "review-42"};
const MilestoneHoldKey deferred{MilestoneHoldCause::deferred_acceptance, "accept-31"};
const MilestoneHoldKey refund_hold{MilestoneHoldCause::refund_review, "refund-17"};
const std::vector<std::string> ordinary{"ordinary-message", "ordinary-sound", "ordinary-visual"};

template <class Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

struct RecordedCaptures {
    SqlRows achievements, actions, jobs, catalog_history;

    bool operator==(const RecordedCaptures &) const = default;
};

RecordedCaptures recorded(const File &file) {
    return {
        file.reads("SELECT occurrence_ref,operation_scope,operation_ref,queue_order,milestone_ref,"
                   "definition_revision_ref,target_micro_points,score_micro_points,"
                   "completion_context_ref,presentation_ms,gap_ms "
                   "FROM main.subathon_milestone_groups ORDER BY queue_order"),
        file.reads("SELECT * FROM main.subathon_milestone_actions ORDER BY occurrence_ref,"
                   "action_ordinal"),
        file.reads("SELECT * FROM main.subathon_delivery_captures ORDER BY operation_scope,"
                   "operation_ref,job_ref"),
        file.reads(
            "SELECT * FROM main.subathon_migration_history WHERE version<=12 ORDER BY version"),
    };
}

SqlRows delivery_history(const File &file) {
    return file.reads("SELECT * FROM main.subathon_delivery_events ORDER BY operation_scope,"
                      "operation_ref,job_ref,revision");
}

SqlRows bonus_receipts(const File &file) {
    return file.reads("SELECT occurrence_ref,action_ordinal,effect_ref "
                      "FROM main.subathon_milestone_bonus_receipts ORDER BY occurrence_ref,"
                      "action_ordinal");
}

template <class Reply, class Submit> Reply completed(Submit submit) {
    const auto reply = std::make_shared<std::optional<Reply>>();
    require(submit([reply](Reply result) { reply->emplace(std::move(result)); }) ==
                StorageError::none,
            "Existing owner entry admits the selected work");
    require(QTest::qWaitFor([reply] { return reply->has_value(); }, timeout),
            "Existing owner entry posts a completion");
    return std::move(reply->value());
}

void release(File &file, const MilestoneOccurrenceKey &key, Release expected) {
    const auto result = completed<MilestoneReleaseCompletion>([&](auto callback) {
        return release_milestone_head(*file.writer, key, std::move(callback));
    });
    normal(result.work);
    require(result.disposition == expected,
            QString("Exact release disposition for %1").arg(key.ref));
}

class FakeDelivery final : public DeliveryDispatchAuthority, public DeliveryAdapter {
  public:
    bool allows(const DeliveryJob &) override { return true; }
    void start(DeliveryJob claim) override { starts.push_back(std::move(claim)); }

    std::vector<DeliveryJob> starts;
};

void dispatch(File &file, const DeliveryJobKey &key, std::int64_t revision,
              const std::shared_ptr<FakeDelivery> &port, DeliveryDisposition expected) {
    const auto before = port->starts.size();
    const auto result = completed<DeliveryDispatchCompletion>([&](auto callback) {
        return dispatch_delivery_job(*file.writer, key, revision, port, port, std::move(callback));
    });
    normal(result.work);
    require(result.result && result.result->disposition == expected && !result.adapter_threw,
            "Exact existing dispatch result");
    if (expected == DeliveryDisposition::changed) {
        require(result.result->job && port->starts.size() == before + 1,
                "One committed delivery claim reaches the fake adapter");
        const auto &started = port->starts.back();
        require(started.key.operation.operation_scope == key.operation.operation_scope &&
                    started.key.operation.operation_ref == key.operation.operation_ref &&
                    started.key.job_ref == key.job_ref &&
                    started.state == DeliveryJobState::sending,
                "Adapter sees the exact selected sending job");
    } else {
        require(!result.result->job && port->starts.size() == before,
                "Refusal carries no claim and starts no adapter");
    }
}

class BonusEffect final : public MilestoneBonusAuthority, public MilestoneBonusEffect {
  public:
    bool allows(const MilestoneBonusKey &, const MilestoneAction &) override { return true; }
    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &key,
                      const MilestoneAction &action) override {
        require(key.occurrence == first && key.action_ordinal == 2 &&
                    action == MilestoneAction{MilestoneActionKind::bonus, "award", "v4"},
                "Effect port sees the exact selected capture");
        ++calls;
        (void)s.execute(
            {"INSERT INTO synthetic_bonus_effects VALUES (?,37)", {QByteArray("performed-37")}});
        return "performed-37";
    }

    int calls{0};
};

MilestoneBonusDisposition bonus(File &file, const MilestoneBonusKey &key, BonusEffect &effect) {
    return committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupStore(s).apply_bonus(key, effect, effect);
    });
}

void refused_actions(File &file, const DeliveryJobKey &job, const MilestoneBonusKey &bonus_key) {
    auto adapter = std::make_shared<FakeDelivery>();
    dispatch(file, job, 1, adapter, DeliveryDisposition::refused);
    BonusEffect effect;
    require(bonus(file, bonus_key, effect) == MilestoneBonusDisposition::refused &&
                effect.calls == 0,
            "Ineligible exact bonus starts no effect");
}

PausedMilestoneAcceptance acceptance(Mode mode, const MilestoneOccurrenceKey &key, Choice choice) {
    return {mode, input_review, {{key, choice}}};
}

void record_acceptance(const SqliteWorkSession &s, const ApplicationOperationKey &operation,
                       const MilestoneOccurrenceKey &key, Choice choice) {
    // Supply literal accepted choices so storage cases can reach their own
    // boundary independently of the paused-input planner.
    SqliteMilestoneGroupDecisions(s).record_acceptance(operation,
                                                       acceptance(Mode::silent, key, choice));
}

void require_acceptance(File &file, const MilestoneOccurrenceKey &key, Choice expected) {
    const auto result = committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupDecisions(s).acceptance_audit(key);
    });
    require(result && result->mode == Mode::silent && result->choice == expected &&
                result->evidence == input_review,
            "Acceptance audit retains silent mode, separate choice, actor and decision");
}

std::vector<MilestoneHoldKey> holds(File &file, const MilestoneOccurrenceKey &key) {
    return committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupDecisions(s).active_holds(key);
    });
}

std::optional<MilestoneRefundAudit> refund_audit(File &file, const MilestoneOccurrenceKey &key) {
    return committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupDecisions(s).refund_audit(key, "refund-17");
    });
}

void expect_refund(File &file, const MilestoneOccurrenceKey &key, Policy policy,
                   std::uint64_t deficit, std::optional<Resolution> resolution = std::nullopt) {
    const auto audit = refund_audit(file, key);
    require(audit && audit->policy == policy && audit->deficit_micro_points == deficit &&
                audit->resolution == resolution &&
                audit->resolution_evidence ==
                    (resolution ? std::optional{action_review} : std::nullopt),
            "Exact refund deficit, selected policy and any authorized resolution are audited");
}

void resolve(File &file, const MilestoneOccurrenceKey &key, const MilestoneHoldKey &hold,
             Resolution resolution) {
    require(committed(*file.writer,
                      [&](const auto &s) {
                          return SqliteMilestoneGroupDecisions(s).resolve(key, hold, resolution,
                                                                          action_review);
                      }),
            "One identified hold is resolved by the authorized decision");
}

void require_order(File &file, const std::vector<MilestoneOccurrenceKey> &expected) {
    const auto pending = committed(
        *file.writer, [](const auto &s) { return SqliteMilestoneGroupStore(s).pending(); });
    std::vector<MilestoneOccurrenceKey> actual;
    for (const auto &capture : pending) {
        require(capture.group.occurrence.has_value(), "Every recovered group has exact identity");
        actual.push_back(*capture.group.occurrence);
    }
    require(actual == expected, "Recovered groups retain durable queue order");
}
} // namespace

class MilestoneGroupDecisionsTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void pausedAcceptancePlanning_data() {
        QTest::addColumn<bool>("silent");
        QTest::addColumn<int>("choiceValue");
        QTest::newRow("normal-run") << false << int(Choice::run);
        QTest::newRow("silent-run") << true << int(Choice::run);
        QTest::newRow("silent-defer") << true << int(Choice::defer);
        QTest::newRow("silent-skip") << true << int(Choice::skip);
    }

    void pausedAcceptancePlanning() {
        QFETCH(bool, silent);
        QFETCH(int, choiceValue);
        check([&] {
            const auto mode = silent ? Mode::silent : Mode::normal;
            const auto choice = static_cast<Choice>(choiceValue);
            const auto expected = acceptance(mode, first, choice);
            const auto result = plan_paused_milestone_actions(expected, {first}, ordinary);
            require(result && result->acceptance.mode == mode &&
                        result->acceptance.evidence == input_review &&
                        result->acceptance.choices == expected.choices,
                    "Normal and silent acceptance preserve the explicit milestone choice");
            require(result->ordinary_alert_refs == (silent ? std::vector<std::string>{} : ordinary),
                    "Silent acceptance suppresses ordinary message, sound and visual intents");
        });
    }

    void silentAcceptanceStillRequiresChoiceForEachAffectedGroup() {
        check([] {
            require(!plan_paused_milestone_actions(acceptance(Mode::silent, first, Choice::run),
                                                   {first, second}, ordinary),
                    "A partially specified affected set cannot produce an acceptance plan");
        });
    }

    void silentAcceptanceRunOrSkip_data() {
        QTest::addColumn<bool>("skip");
        QTest::newRow("run") << false;
        QTest::newRow("skip") << true;
    }

    void silentAcceptanceRunOrSkip() {
        QFETCH(bool, skip);
        check([&] {
            File file;
            const auto choice = skip ? Choice::skip : Choice::run;
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_origin(s);
                seed_head(s, 0);
                record_acceptance(s, origin, first, choice);
                return true;
            });
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_following(s);
                seed_suffix(s);
                return true;
            });
            file.reopen();
            require_acceptance(file, first, choice);
            const auto before = recorded(file);
            require(before.achievements.size() == 4 && before.actions.size() == 7 &&
                        before.jobs.size() == 5 && before.catalog_history.size() == 12 &&
                        before.achievements.front().at(0) == QByteArray("z-first") &&
                        before.achievements.front().at(8) == QByteArray("crossing-9"),
                    "All literal achievements and action captures survive either choice");
            require(holds(file, first).empty(), "Run and Skip do not create a deferred hold");
            release(file, first, skip ? Release::refused : Release::released);
            if (skip) {
                refused_actions(file, first_job, first_bonus);
            } else {
                auto adapter = std::make_shared<FakeDelivery>();
                dispatch(file, first_job, 1, adapter, DeliveryDisposition::changed);
            }
            release(file, second, Release::released);
            require(recorded(file) == before, "Choice and release retain immutable captured data");
        });
    }

    void defaultRefundReviewBlocksExactHeadsIndependently() {
        check([] {
            File file;
            file.record();
            const auto before = recorded(file);
            const auto queued = delivery_history(file);
            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions decisions(s);
                decisions.observe_refund({first, "refund-17", 350, 99, 100});
                decisions.observe_refund({second, "refund-17", 350, 99, 200});
                return true;
            });
            file.reopen();
            expect_refund(file, first, Policy::review, 1);
            expect_refund(file, second, Policy::review, 101);
            require(holds(file, first) == std::vector{refund_hold} &&
                        holds(file, second) == std::vector{refund_hold},
                    "One refund creates independently addressed holds on both occurrences");
            release(file, first, Release::refused);
            release(file, second, Release::refused);
            release(file, MilestoneOccurrenceKey{"m-third"}, Release::refused);
            refused_actions(file, first_job, first_bonus);
            refused_actions(file, second_job, second_bonus);

            resolve(file, first, refund_hold, Resolution::continue_actions);
            require(holds(file, first).empty() && holds(file, second) == std::vector{refund_hold},
                    "Continuing one occurrence preserves the other occurrence's hold");
            expect_refund(file, first, Policy::review, 1, Resolution::continue_actions);
            release(file, first, Release::released);
            release(file, second, Release::refused);
            release(file, MilestoneOccurrenceKey{"m-third"}, Release::refused);

            resolve(file, second, refund_hold, Resolution::skip);
            file.reopen();
            expect_refund(file, first, Policy::review, 1, Resolution::continue_actions);
            expect_refund(file, second, Policy::review, 101, Resolution::skip);
            require(holds(file, second).empty(), "Skip resolves the selected refund review");
            release(file, second, Release::refused);
            refused_actions(file, second_job, second_bonus);
            release(file, MilestoneOccurrenceKey{"m-third"}, Release::released);
            require(recorded(file) == before && delivery_history(file) == queued &&
                        bonus_receipts(file).empty(),
                    "Review decisions and release never erase captures or claim jobs or bonuses");
        });
    }

    void configuredRefundPolicy_data() {
        QTest::addColumn<bool>("skip");
        QTest::newRow("continue") << false;
        QTest::newRow("skip") << true;
    }

    void configuredRefundPolicy() {
        QFETCH(bool, skip);
        check([&] {
            File file;
            file.record();
            const auto before = recorded(file);
            const auto policy = skip ? Policy::skip : Policy::continue_actions;
            // 100 - INT64_MIN is 9223372036854775908 micro-points.
            (void)committed(*file.writer, [policy](const auto &s) {
                SqliteMilestoneGroupDecisions(s).observe_refund(
                    {first, "refund-17", 150, std::numeric_limits<std::int64_t>::min(), 100,
                     policy});
                return true;
            });
            file.reopen();
            expect_refund(file, first, policy, 9223372036854775908ULL);
            require(holds(file, first).empty(), "Configured Continue and Skip need no review hold");
            release(file, first, skip ? Release::refused : Release::released);
            if (skip)
                refused_actions(file, first_job, first_bonus);
            release(file, second, Release::released);
            require(recorded(file) == before, "Refund policy preserves achievements and captures");
        });
    }

    void refundToExactTargetLeavesHeadEligible() {
        check([] {
            File file;
            file.record();
            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions(s).observe_refund(
                    {first, "refund-17", 150, 100, 100});
                return true;
            });
            require(holds(file, first).empty(), "Equal score and target yield no refund hold");
            release(file, first, Release::released);
        });
    }

    void refundAfterActionsStartedPreservesSendingAndPerformedEffects() {
        check([] {
            File file;
            file.record();
            release(file, first, Release::released);
            auto adapter = std::make_shared<FakeDelivery>();
            dispatch(file, first_job, 1, adapter, DeliveryDisposition::changed);
            dispatch(file, delivered_job, 1, adapter, DeliveryDisposition::changed);
            (void)committed(*file.writer, [](const auto &s) {
                require(
                    SqliteDeliveryJobStore(s)
                            .finish(delivered_job, 2, 2, DeliveryEvidence::delivered, "delivered-1")
                            .disposition == DeliveryDisposition::changed,
                    "Second action has a real delivered record");
                (void)s.execute({"CREATE TABLE synthetic_bonus_effects(ref BLOB PRIMARY KEY,"
                                 "units INTEGER)",
                                 {}});
                return true;
            });
            BonusEffect effect;
            require(bonus(file, first_bonus, effect) == MilestoneBonusDisposition::applied,
                    "Bonus effect and receipt commit before the refund");
            const auto before = recorded(file);
            const auto events = delivery_history(file);
            require(bonus_receipts(file) ==
                        SqlRows{{QByteArray("z-first"), 2, QByteArray("performed-37")}},
                    "One exact bonus receipt precedes the refund");

            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions(s).observe_refund({first, "refund-17", 150, 99, 100});
                return true;
            });
            file.reopen();
            require(holds(file, first).empty(), "Started group gains no pre-start review hold");
            release(file, second, Release::released);
            dispatch(file, first_job, 2, adapter, DeliveryDisposition::refused);
            dispatch(file, delivered_job, 3, adapter, DeliveryDisposition::refused);
            require(bonus(file, first_bonus, effect) ==
                            MilestoneBonusDisposition::already_applied &&
                        effect.calls == 1,
                    "Existing exact bonus receipt prevents reapplication");
            require(recorded(file) == before && delivery_history(file) == events &&
                        bonus_receipts(file) ==
                            SqlRows{{QByteArray("z-first"), 2, QByteArray("performed-37")}} &&
                        file.reads("SELECT ref,units FROM synthetic_bonus_effects") ==
                            SqlRows{{QByteArray("performed-37"), 37}},
                    "Sending, delivered, captured and applied records survive the refund");
        });
    }

    void deferredGroupReopensInOrderWithoutReplayingActions() {
        check([] {
            File file;
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_origin(s);
                seed_head(s, 0);
                return true;
            });
            release(file, first, Release::released);
            auto adapter = std::make_shared<FakeDelivery>();
            dispatch(file, first_job, 1, adapter, DeliveryDisposition::changed);
            (void)committed(*file.writer, [](const auto &s) {
                require(SqliteDeliveryJobStore(s)
                                .finish(first_job, 2, 2, DeliveryEvidence::delivered,
                                        "sent-before-close")
                                .disposition == DeliveryDisposition::changed,
                        "Preceding action is durably performed");
                return true;
            });
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_following(s);
                seed_suffix(s);
                // The following operation crosses three thresholds.
                SqliteMilestoneGroupDecisions(s).record_acceptance(following_origin,
                                                                   {Mode::silent,
                                                                    input_review,
                                                                    {{second, Choice::defer},
                                                                     {{"m-third"}, Choice::run},
                                                                     {{"b-empty"}, Choice::run}}});
                return true;
            });
            const auto before = recorded(file);
            const auto events = delivery_history(file);
            file.reopen();
            require_order(file, {second, {"m-third"}, {"b-empty"}});
            require_acceptance(file, second, Choice::defer);
            require(holds(file, second) == std::vector{deferred}, "Defer survives a cold reopen");
            release(file, second, Release::refused);
            release(file, MilestoneOccurrenceKey{"m-third"}, Release::refused);
            refused_actions(file, second_job, second_bonus);
            dispatch(file, first_job, 3, adapter, DeliveryDisposition::refused);
            require(recorded(file) == before && delivery_history(file) == events &&
                        bonus_receipts(file).empty(),
                    "Reopen retains captures, performed history and untouched deferred actions");

            resolve(file, second, deferred, Resolution::continue_actions);
            file.reopen();
            require_acceptance(file, second, Choice::defer);
            require(holds(file, second).empty(), "Explicit Continue durably clears defer");
            release(file, second, Release::released);
            dispatch(file, second_job, 1, adapter, DeliveryDisposition::changed);
            require_order(file, {{"m-third"}, {"b-empty"}});
            require(recorded(file) == before, "Resuming a deferred group retains its capture");
        });
    }

    void rolledBackRefundLeavesNoHoldOrAudit() {
        check([] {
            File file;
            file.record();
            const auto before = recorded(file);
            const auto result = work(*file.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                SqliteMilestoneGroupDecisions(s).observe_refund({first, "refund-17", 150, 99, 100});
                s.rollback();
                return 1;
            });
            require(result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::not_committed && !result.value,
                    "Known rollback publishes no provisional result");
            file.reopen();
            require(holds(file, first).empty() && !refund_audit(file, first),
                    "Rolled-back decision leaves neither an active hold nor an audit record");
            release(file, first, Release::released);
            require(recorded(file) == before, "Rollback retains previously committed captures");
        });
    }

    void rolledBackAcceptanceLeavesNoChoiceOrOriginatingCapture() {
        check([] {
            File file;
            const auto before = recorded(file);
            const auto result = work(*file.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                file.seed_origin(s);
                seed_head(s, 0);
                record_acceptance(s, origin, first, Choice::defer);
                s.rollback();
                return 1;
            });
            require(result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::not_committed && !result.value,
                    "Known rollback publishes no provisional acceptance");
            file.reopen();
            const auto acceptance_exists = committed(*file.writer, [](const auto &s) {
                return SqliteMilestoneGroupDecisions(s).acceptance_audit(first).has_value();
            });
            require(!acceptance_exists && recorded(file) == before &&
                        delivery_history(file).empty(),
                    "Rollback leaves no decision, achievement, capture or delivery history");
            require(file.reads("SELECT operation_scope,operation_ref FROM "
                               "main.subathon_app_operations")
                        .empty(),
                    "The originating application bundle rolls back with the decision");
        });
    }
};

QTEST_GUILESS_MAIN(MilestoneGroupDecisionsTests)
#include "milestone_group_decisions_tests.moc"
