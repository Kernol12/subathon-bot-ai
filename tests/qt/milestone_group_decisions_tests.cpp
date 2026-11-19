// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/milestone_group_decisions_fixture.hpp"

#include <QTest>
#include <limits>
#include <map>
#include <subathon/server/milestone_group_decisions.hpp>

namespace {
using namespace milestone_group_decisions_fixture;
using Choice = MilestoneActionChoice;
using Mode = PausedAcceptanceMode;
using Policy = MilestoneRefundPolicy;
using Resolution = MilestoneHoldResolution;
using Release = MilestoneReleaseDisposition;
using Gate = MilestonePendingDisposition;
using Head = MilestoneHeadDisposition;
using SkipOrigin = MilestoneSkipOrigin;

const MilestoneOccurrenceKey first{"z-first"}, second{"a-second"}, third{"m-third"};
const DeliveryJobKey first_job{origin, "job"}, delivered_job{origin, "done"},
    second_job{following_origin, "job"};
const MilestoneBonusKey first_bonus{first, 2}, second_bonus{second, 1};
const MilestoneDecisionEvidence input_review{"operator-a", "accept-31"};
const MilestoneDecisionEvidence action_review{"operator-b", "review-42"};
const MilestoneHoldKey deferred{MilestoneHoldCause::deferred_acceptance, "accept-31"};
const MilestoneHoldKey refund_hold{MilestoneHoldCause::refund_review, "refund-17"};
const std::vector<std::string> ordinary{"ordinary-message", "ordinary-sound", "ordinary-visual"};
constexpr auto release_rows = "SELECT occurrence_ref,released FROM main.subathon_milestone_groups "
                              "ORDER BY queue_order";
constexpr auto acceptance_rows =
    "SELECT * FROM main.subathon_milestone_acceptance_decisions ORDER BY occurrence_ref";
constexpr auto refund_rows =
    "SELECT * FROM main.subathon_milestone_refund_decisions ORDER BY occurrence_ref,refund_ref";

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

void require_acceptance(File &file, const MilestoneOccurrenceKey &key, Choice expected,
                        std::optional<Resolution> resolution = std::nullopt,
                        std::optional<MilestoneSkipProvenance> ended_by = std::nullopt) {
    const auto result = committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupDecisions(s).acceptance_audit(key);
    });
    require(result && result->mode == Mode::silent && result->choice == expected &&
                result->evidence == input_review && result->resolution == resolution &&
                result->resolution_evidence ==
                    (resolution ? std::optional{action_review} : std::nullopt) &&
                result->ended_by_skip == ended_by,
            "Acceptance audit retains the original choice and any separate resolution");
}

std::vector<MilestoneHoldKey> holds(File &file, const MilestoneOccurrenceKey &key) {
    return committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupDecisions(s).active_holds(key);
    });
}

std::optional<MilestoneRefundAudit> refund_audit(File &file, const MilestoneOccurrenceKey &key,
                                                 const std::string &ref = "refund-17") {
    return committed(*file.writer, [&](const auto &s) {
        return SqliteMilestoneGroupDecisions(s).refund_audit(key, ref);
    });
}

void expect_refund(File &file, const MilestoneOccurrenceKey &key, Policy policy,
                   std::uint64_t deficit, std::int64_t current_target,
                   std::optional<Resolution> resolution = std::nullopt,
                   const std::string &ref = "refund-17",
                   std::optional<MilestoneSkipProvenance> ended_by = std::nullopt) {
    const auto audit = refund_audit(file, key, ref);
    require(audit && audit->policy == policy && audit->deficit_micro_points == deficit &&
                audit->current_target_micro_points == current_target &&
                audit->resolution == resolution &&
                audit->resolution_evidence ==
                    (resolution ? std::optional{action_review} : std::nullopt) &&
                audit->ended_by_skip == ended_by,
            "Exact refund deficit, selected policy and any authorized resolution are audited");
}

void require_gate(File &file, const MilestoneOccurrenceKey &key, Gate expected) {
    require(committed(*file.writer,
                      [&](const auto &s) {
                          return SqliteMilestoneGroupDecisions(s).pending_disposition(key);
                      }) == expected,
            "Decision gate reports the status of the exact occurrence");
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

class PresentationProbe : public MilestoneGroupRelease {
  public:
    bool review_held(const subathon::core::MilestoneCompletion &) const override { return false; }

    std::optional<Head> occurrence_disposition(const MilestoneOccurrenceKey &) const override {
        // These tests supply the ready v12 group or exercise cold queue selection
        // independently of the warm gate.
        return Head::ready;
    }

    void release(CommittedMilestoneGroup group) override {
        require(group.occurrence.has_value(), "Each offered presentation carries an occurrence");
        offers.push_back(*group.occurrence);
    }

    std::vector<MilestoneOccurrenceKey> offers;
};

class WarmPresentationProbe final : public PresentationProbe {
  public:
    bool review_held(const subathon::core::MilestoneCompletion &) const override {
        ++legacy_lookups;
        return false;
    }

    std::optional<Head> occurrence_disposition(const MilestoneOccurrenceKey &key) const override {
        lookups.push_back(key);
        const auto found = decisions.find(key.ref);
        if (found == decisions.end())
            return std::nullopt;
        return found->second;
    }

    std::map<std::string, Head> decisions;
    mutable std::vector<MilestoneOccurrenceKey> lookups;
    mutable int legacy_lookups{0};
};

std::vector<CommittedMilestoneGroup> recovered_groups(File &file) {
    auto pending = committed(*file.writer,
                             [](const auto &s) { return SqliteMilestoneGroupStore(s).pending(); });
    std::vector<CommittedMilestoneGroup> groups;
    for (auto &capture : pending)
        groups.push_back(std::move(capture.group));
    return groups;
}

void seed_first_acceptance(const SqliteWorkSession &s, int choice) {
    // Literal v13 storage lets the gate, release and recovery cases reach the
    // consumer without first passing through the decision writer.
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_acceptance_decisions "
         "(occurrence_ref,mode,choice,actor_ref,decision_ref) VALUES (?,1,?,?,?)",
         {QByteArray("z-first"), choice, QByteArray("operator-a"), QByteArray("accept-31")}});
}

void seed_overlapping_refund_reviews(const SqliteWorkSession &s) {
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_refund_decisions "
         "(occurrence_ref,refund_ref,score_before_micro_points,score_after_micro_points,"
         "current_target_micro_points,policy) VALUES (?, ?, 150, 99, 100, 1),"
         "(?, ?, 120, 117, 120, 1)",
         {QByteArray("z-first"), QByteArray("refund-17"), QByteArray("z-first"),
          QByteArray("refund-25")}});
    (void)s.execute(
        {"INSERT INTO main.subathon_milestone_refund_decisions "
         "(occurrence_ref,refund_ref,score_before_micro_points,score_after_micro_points,"
         "current_target_micro_points,policy,resolution,resolution_actor_ref,resolution_ref) "
         "VALUES (?, ?, 100, 98, 100, 1, 0, ?, ?)",
         {QByteArray("z-first"), QByteArray("refund-09"), QByteArray("operator-c"),
          QByteArray("review-12")}});
}
} // namespace

class MilestoneGroupDecisionsTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void additiveCatalogPreservesPopulatedV12AndExactHistory() {
        check([] {
            File existing;
            existing.record(1);
            const auto before = recorded(existing);
            const auto releases = existing.reads(release_rows);
            const auto events = delivery_history(existing);
            require(before.catalog_history.size() == 12 && releases.size() == 4 &&
                        releases.front() == SqlRow{QByteArray("z-first"), 1} && events.size() == 5,
                    "Populated v12 includes a committed release with queued jobs");

            const auto expected = literal_decision_manifest();
            const auto actual = milestone_group_decisions_repository_manifest();
            require(actual.catalog_format == expected.catalog_format &&
                        actual.store_identity == expected.store_identity &&
                        actual.supported_starting_versions ==
                            expected.supported_starting_versions &&
                        actual.steps.size() == 13,
                    "v13 adds exactly one supported catalog step");
            SqlRows history;
            for (qsizetype i = 0; i < actual.steps.size(); ++i) {
                const auto &step = actual.steps[i];
                const auto &literal = expected.steps[i];
                const auto digest = literal_digest(literal);
                require(step.version == literal.version && step.id == literal.id &&
                            step.sql == literal.sql &&
                            step.expected_schema == literal.expected_schema &&
                            migration_content_sha256(step) == digest,
                        "Each version retains its exact ordered SQL, schema and canonical digest");
                history.append({qint64(literal.version), literal.id, digest});
            }
            migrated(permission_graph_fixture::migrate(*existing.writer, actual), 13, 1);
            existing.reopen();
            require(recorded(existing) == before && existing.reads(release_rows) == releases &&
                        delivery_history(existing) == events && bonus_receipts(existing).empty(),
                    "Upgrade preserves releases, captures and delivery history without effects");

            File fresh(File::Schema::empty);
            migrated(permission_graph_fixture::migrate(*fresh.writer, actual), 13, 13);
            fresh.reopen();
            for (const auto *file : {&fresh, &existing}) {
                require(file->reads("SELECT version,migration_id,content_sha256 "
                                    "FROM main.subathon_migration_history ORDER BY version") ==
                                history &&
                            file->reads(acceptance_rows).empty() &&
                            file->reads(refund_rows).empty(),
                        "Both installation paths have exact history and empty decision tables");
            }
        });
    }

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

    void staleOrDuplicateChoicesRefuseACompleteLookingPlan() {
        check([] {
            require(!plan_paused_milestone_actions(
                        {Mode::normal, input_review, {{first, Choice::run}, {first, Choice::skip}}},
                        {first, second}, ordinary),
                    "Duplicate choices cannot stand in for the second occurrence");
            require(
                !plan_paused_milestone_actions(
                    {Mode::normal, input_review, {{first, Choice::run}, {second, Choice::skip}}},
                    {first}, ordinary),
                "Stale choices for an unaffected occurrence refuse the whole plan");
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
            DecisionFile file;
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
            require_gate(file, first, skip ? Gate::skipped : Gate::ready);
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
            DecisionFile file;
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
            expect_refund(file, first, Policy::review, 1, 100);
            expect_refund(file, second, Policy::review, 101, 200);
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
            expect_refund(file, first, Policy::review, 1, 100, Resolution::continue_actions);
            release(file, first, Release::released);
            release(file, second, Release::refused);
            release(file, MilestoneOccurrenceKey{"m-third"}, Release::refused);

            resolve(file, second, refund_hold, Resolution::skip);
            file.reopen();
            expect_refund(file, first, Policy::review, 1, 100, Resolution::continue_actions);
            expect_refund(file, second, Policy::review, 101, 200, Resolution::skip);
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
            DecisionFile file;
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
            expect_refund(file, first, policy, 9223372036854775908ULL, 100);
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
            DecisionFile file;
            file.record();
            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions(s).observe_refund(
                    {first, "refund-17", 150, 100, 100});
                return true;
            });
            require(holds(file, first).empty() && !refund_audit(file, first),
                    "Equal score and target yield no refund policy or hold");
            release(file, first, Release::released);
        });
    }

    void correctedTargetControlsRefundAndCapturedTargetStaysHistorical_data() {
        QTest::addColumn<bool>("raised");
        QTest::newRow("raised-crosses-current-only") << true;
        QTest::newRow("lowered-ends-at-current") << false;
    }

    void correctedTargetControlsRefundAndCapturedTargetStaysHistorical() {
        QFETCH(bool, raised);
        check([&] {
            DecisionFile file;
            file.record();
            const auto before = recorded(file);
            const auto score_after = raised ? 110 : 80;
            const auto current_target = raised ? 120 : 80;
            (void)committed(*file.writer, [&](const auto &s) {
                // With the same captured 100, 110 crosses current 120 and
                // 80 stops exactly at current 80.
                SqliteMilestoneGroupDecisions(s).observe_refund(
                    {first, "refund-17", 350, score_after, current_target});
                return true;
            });
            file.reopen();
            SqlRows expected;
            if (raised) {
                expect_refund(file, first, Policy::review, 10, 120);
                require(holds(file, first) == std::vector{refund_hold},
                        "Crossing the raised target triggers Review");
                expected = {
                    {QByteArray("z-first"), QByteArray("refund-17"), 350, 110, 120, 1, 1, 1, 1}};
            } else {
                require(holds(file, first).empty() && !refund_audit(file, first),
                        "Reaching the lowered target exactly triggers no policy");
            }
            require(recorded(file) == before &&
                        file.reads("SELECT target_micro_points FROM main.subathon_milestone_groups "
                                   "WHERE queue_order IN (1,2) ORDER BY queue_order") ==
                            SqlRows{{100}, {200}} &&
                        file.reads("SELECT occurrence_ref,refund_ref,score_before_micro_points,"
                                   "score_after_micro_points,current_target_micro_points,policy,"
                                   "resolution IS NULL,resolution_actor_ref IS NULL,"
                                   "resolution_ref IS NULL FROM "
                                   "main.subathon_milestone_refund_decisions") == expected,
                    "The corrected audit operands and immutable captured targets coexist");
        });
    }

    void furtherDecreaseDoesNotRepeatPolicyButFreshCrossingDoes() {
        check([] {
            DecisionFile file;
            file.record();
            const auto before = recorded(file);
            (void)committed(*file.writer, [](const auto &s) {
                seed_first_acceptance(s, 1);
                return true;
            });
            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions decisions(s);
                // The current target is 120 while the captured target is 100.
                decisions.observe_refund({first, "refund-17", 120, 110, 120});
                decisions.observe_refund({first, "refund-20", 110, 99, 120, Policy::skip});
                // Score later recovered to 120 without releasing the deferred head.
                decisions.observe_refund({first, "refund-25", 120, 117, 120});
                return true;
            });
            file.reopen();
            expect_refund(file, first, Policy::review, 10, 120);
            require(!refund_audit(file, first, "refund-20"),
                    "A further decrease does not apply even configured Skip");
            expect_refund(file, first, Policy::review, 3, 120, std::nullopt, "refund-25");
            require(
                holds(file, first) ==
                    std::vector{deferred, refund_hold,
                                MilestoneHoldKey{MilestoneHoldCause::refund_review, "refund-25"}},
                "Defer and both distinct crossings remain open across a cold reopen");
            require_gate(file, first, Gate::held);
            require_order(file, {first, second, third, {"b-empty"}});
            release(file, first, Release::refused);
            release(file, second, Release::refused);
            require(recorded(file) == before, "Refund decisions preserve the captured target");
        });
    }

    void releasedButUnclaimedGroupRetainsEligibilityAndPresentation() {
        check([] {
            File file;
            file.record();
            release(file, first, Release::released);
            PresentationProbe port;
            MilestoneTimeline timeline({}, port);
            timeline.accept_committed({captures().front().group}, 10ms);
            const auto events = delivery_history(file);
            require(events.size() == 5 && bonus_receipts(file).empty(),
                    "At refund time the release has made no claim or bonus receipt");
            advance_to_literal_decisions(file);
            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions decisions(s);
                decisions.observe_refund({first, "refund-17", 350, 99, 100});
                decisions.observe_refund({second, "refund-17", 350, 99, 200});
                (void)s.execute({"CREATE TABLE synthetic_bonus_effects(ref BLOB PRIMARY KEY,"
                                 "units INTEGER)",
                                 {}});
                return true;
            });
            file.reopen();
            require(holds(file, first).empty() && !refund_audit(file, first) &&
                        holds(file, second) == std::vector{refund_hold},
                    "Only the unreleased occurrence acquires a refund Review hold");
            require_gate(file, first, Gate::absent_or_released);
            require_gate(file, second, Gate::held);
            require(delivery_history(file) == events && bonus_receipts(file).empty(),
                    "Release and refund leave all claims and bonuses untouched");
            release(file, second, Release::refused);
            auto adapter = std::make_shared<FakeDelivery>();
            dispatch(file, first_job, 1, adapter, DeliveryDisposition::changed);
            BonusEffect effect;
            require(bonus(file, first_bonus, effect) == MilestoneBonusDisposition::applied &&
                        effect.calls == 1,
                    "Previously unclaimed actions are still eligible after the refund");
            timeline.advance(2509ms);
            require(timeline.presentation() ==
                        ActiveMilestonePresentation{"peak-z", 2499ms, 2500ms},
                    "The started presentation is still visible at its last millisecond");
            timeline.advance(2510ms);
            require(!timeline.presentation() && port.offers == std::vector{first},
                    "The started presentation finishes at the original boundary");
        });
    }

    void deferralAndRefundReviewResolveIndependently_data() {
        QTest::addColumn<bool>("refundFirst");
        QTest::newRow("refund-first") << true;
        QTest::newRow("deferral-first") << false;
    }

    void deferralAndRefundReviewResolveIndependently() {
        QFETCH(bool, refundFirst);
        check([&] {
            DecisionFile file;
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_origin(s);
                seed_head(s, 0);
                record_acceptance(s, origin, first, Choice::defer);
                return true;
            });
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_following(s);
                seed_suffix(s);
                SqliteMilestoneGroupDecisions(s).observe_refund({first, "refund-17", 350, 99, 100});
                return true;
            });
            file.reopen();
            require(holds(file, first) == std::vector{deferred, refund_hold},
                    "The deferred head retains both causes across reopen");
            const auto &selected = refundFirst ? refund_hold : deferred;
            const auto &remaining = refundFirst ? deferred : refund_hold;
            resolve(file, first, selected, Resolution::continue_actions);
            file.reopen();
            require(holds(file, first) == std::vector{remaining},
                    "Either resolution leaves the other cause active");
            release(file, first, Release::refused);
            release(file, second, Release::refused);
            require(!committed(*file.writer,
                               [&](const auto &s) {
                                   return SqliteMilestoneGroupDecisions(s).resolve(
                                       first, selected, Resolution::continue_actions,
                                       action_review);
                               }),
                    "Retry cannot resolve a different outstanding cause");
            resolve(file, first, remaining, Resolution::continue_actions);
            file.reopen();
            require(holds(file, first).empty(), "Both decisions are durable");
            require_acceptance(file, first, Choice::defer, Resolution::continue_actions);
            expect_refund(file, first, Policy::review, 1, 100, Resolution::continue_actions);
            release(file, second, Release::refused);
            release(file, first, Release::released);
            release(file, second, Release::released);
        });
    }

    void terminalSkipEndsAllRemainingHolds_data() {
        QTest::addColumn<int>("skipSource");
        QTest::newRow("deferral-resolution") << 0;
        QTest::newRow("refund-resolution") << 1;
        QTest::newRow("configured-refund") << 2;
    }

    void terminalSkipEndsAllRemainingHolds() {
        QFETCH(int, skipSource);
        check([&] {
            DecisionFile file;
            file.record();
            (void)committed(*file.writer, [](const auto &s) {
                seed_first_acceptance(s, 1);
                seed_overlapping_refund_reviews(s);
                return true;
            });
            const auto before = recorded(file);
            const auto events = delivery_history(file);
            const auto release_before = file.reads(release_rows);

            MilestoneSkipProvenance terminal{SkipOrigin::deferred_resolution, "accept-31"};
            if (skipSource == 0) {
                resolve(file, first, deferred, Resolution::skip);
            } else if (skipSource == 1) {
                resolve(file, first, refund_hold, Resolution::skip);
                terminal = {SkipOrigin::refund_resolution, "refund-17"};
            } else {
                (void)committed(*file.writer, [](const auto &s) {
                    SqliteMilestoneGroupDecisions(s).observe_refund(
                        {first, "refund-31", 125, 119, 120, Policy::skip});
                    return true;
                });
                terminal = {SkipOrigin::refund_policy, "refund-31"};
            }

            file.reopen();
            if (skipSource == 0)
                require_acceptance(file, first, Choice::defer, Resolution::skip);
            else
                require_acceptance(file, first, Choice::defer, std::nullopt, terminal);
            if (skipSource == 1)
                expect_refund(file, first, Policy::review, 1, 100, Resolution::skip);
            else
                expect_refund(file, first, Policy::review, 1, 100, std::nullopt, "refund-17",
                              terminal);
            expect_refund(file, first, Policy::review, 3, 120, std::nullopt, "refund-25", terminal);
            if (skipSource == 2)
                expect_refund(file, first, Policy::skip, 1, 120, std::nullopt, "refund-31");
            const auto earlier = refund_audit(file, first, "refund-09");
            require(earlier && earlier->policy == Policy::review &&
                        earlier->deficit_micro_points == 2 &&
                        earlier->current_target_micro_points == 100 &&
                        earlier->resolution == Resolution::continue_actions &&
                        earlier->resolution_evidence ==
                            MilestoneDecisionEvidence{"operator-c", "review-12"} &&
                        !earlier->ended_by_skip,
                    "A previously resolved review keeps its own committed decision");
            require(holds(file, first).empty(), "No hold on the skipped occurrence stays open");
            require_gate(file, first, Gate::skipped);
            const auto acceptance_before_retry = file.reads(acceptance_rows);
            const auto refunds_before_retry = file.reads(refund_rows);
            const MilestoneHoldKey other_review{MilestoneHoldCause::refund_review, "refund-25"};
            require(!committed(*file.writer,
                               [&](const auto &s) {
                                   return SqliteMilestoneGroupDecisions(s).resolve(
                                       first, other_review, Resolution::continue_actions,
                                       action_review);
                               }),
                    "An ended hold cannot be resolved after terminal Skip");
            require(file.reads(acceptance_rows) == acceptance_before_retry &&
                        file.reads(refund_rows) == refunds_before_retry,
                    "Refusing an ended hold leaves both decision tables intact");
            require_order(file, {second, third, {"b-empty"}});
            release(file, first, Release::refused);
            refused_actions(file, first_job, first_bonus);
            require(file.reads(release_rows) == release_before,
                    "Skip omits the head without marking it released");
            release(file, second, Release::released);
            require(recorded(file) == before && delivery_history(file) == events &&
                        bonus_receipts(file).empty(),
                    "Terminal Skip preserves captures and delivery history");
        });
    }

    void warmExactHeadStopsUntilReady_data() {
        QTest::addColumn<bool>("unavailable");
        QTest::newRow("held") << false;
        QTest::newRow("unavailable") << true;
    }

    void warmExactHeadStopsUntilReady() {
        QFETCH(bool, unavailable);
        check([&] {
            const auto groups = captures();
            auto following = groups[1].group;
            following.achievement = groups[0].group.achievement;
            WarmPresentationProbe port;
            if (!unavailable)
                port.decisions.emplace(first.ref, Head::held);
            port.decisions.emplace(second.ref, Head::ready);
            MilestoneTimeline timeline({}, port);
            timeline.accept_committed({groups[0].group, following}, 10ms);
            require(port.offers.empty() && !timeline.presentation(),
                    "Held or unavailable exact head stops a ready alias of the same achievement");
            require(port.lookups == std::vector{first} && port.legacy_lookups == 0,
                    "Only the exact keyed head is consulted");

            port.decisions[first.ref] = Head::ready;
            timeline.advance(11ms);
            require(port.lookups == std::vector{first, first} &&
                        port.offers == std::vector{first} && port.legacy_lookups == 0 &&
                        timeline.presentation() ==
                            ActiveMilestonePresentation{"peak-z", 0ms, 2500ms},
                    "Ready starts the exact head on the next eligible turn");
        });
    }

    void warmSkipWaitsForCurrentWindowAndGap_data() {
        QTest::addColumn<bool>("duringGap");
        QTest::newRow("during-presentation") << false;
        QTest::newRow("during-gap") << true;
    }

    void warmSkipWaitsForCurrentWindowAndGap() {
        QFETCH(bool, duringGap);
        check([&] {
            const auto groups = captures();
            auto alias = groups[2].group;
            alias.achievement = groups[1].group.achievement;
            auto skipped = groups[1].group;
            skipped.timing = {3000ms, 500ms};
            WarmPresentationProbe port;
            port.decisions = {
                {first.ref, Head::ready}, {second.ref, Head::held}, {third.ref, Head::ready}};
            MilestoneTimeline timeline({}, port);
            timeline.accept_committed({groups[0].group, skipped, alias}, 10ms);

            if (!duringGap)
                port.decisions[second.ref] = Head::skipped;
            timeline.advance(2509ms);
            require(port.offers == std::vector{first} &&
                        timeline.presentation() ==
                            ActiveMilestonePresentation{"peak-z", 2499ms, 2500ms},
                    "A pending Skip leaves the active presentation at its original last sample");
            timeline.advance(2510ms);
            require(!timeline.presentation() && port.offers == std::vector{first},
                    "The original presentation expires into its captured gap");
            if (duringGap)
                port.decisions[second.ref] = Head::skipped;
            timeline.advance(2909ms);
            require(!timeline.presentation() && port.offers == std::vector{first},
                    "The original gap still gates the next start");
            timeline.advance(2910ms);
            require(port.offers == std::vector{first, third} &&
                        timeline.presentation() ==
                            ActiveMilestonePresentation{"peak-a", 0ms, 10000ms},
                    "Skip advances to the ready alias on the same turn without its own window");
            require(port.lookups == std::vector{first, second, third} && port.legacy_lookups == 0,
                    "The window and gap defer consultation, then only exact keys are used");
            timeline.advance(13910ms);
            require(!timeline.presentation() && port.offers == std::vector{first, third} &&
                        port.lookups == std::vector{first, second, third},
                    "The skipped head was removed and never offered again");
        });
    }

    void gateUsesOccurrenceWhenTwoGroupsShareTheSameMilestone() {
        check([] {
            DecisionFile file;
            file.record();
            (void)committed(*file.writer, [](const auto &s) {
                // Synthetic historical captures of two occurrences of the same definition.
                (void)s.execute({"UPDATE main.subathon_milestone_groups SET milestone_ref=? "
                                 "WHERE queue_order=2",
                                 {QByteArray("peak-z")}});
                seed_first_acceptance(s, 2);
                return true;
            });
            file.reopen();
            require_gate(file, first, Gate::skipped);
            require_gate(file, second, Gate::ready);
            require_gate(file, {"peak-z"}, Gate::absent_or_released);
        });
    }

    void recoveredTimelineOmitsSkippedOccurrenceAndItsGap() {
        check([] {
            DecisionFile file;
            file.record();
            (void)committed(*file.writer, [](const auto &s) {
                seed_first_acceptance(s, 2);
                return true;
            });
            const auto before = recorded(file);
            file.reopen();
            PresentationProbe port;
            MilestoneTimeline timeline({}, port);
            timeline.restore_committed(recovered_groups(file));
            require(!timeline.presentation() && port.offers.empty(),
                    "Recovery does not start a presentation");
            timeline.advance(50ms);
            require(port.offers == std::vector{second} &&
                        timeline.presentation() ==
                            ActiveMilestonePresentation{"peak-a", 0ms, 10000ms},
                    "The first turn presents the next group without a skipped window or gap");
            require(recorded(file) == before &&
                        file.reads(release_rows).front() == SqlRow{QByteArray("z-first"), 0},
                    "Omission preserves the skipped capture and never marks it released");
        });
    }

    void directReleaseRechecksPersistedHoldInWriter() {
        check([] {
            DecisionFile file;
            file.record();
            (void)committed(*file.writer, [](const auto &s) {
                seed_first_acceptance(s, 1);
                return true;
            });
            const auto before = file.reads(release_rows);
            release(file, first, Release::refused);
            release(file, second, Release::refused);
            require(file.reads(release_rows) == before,
                    "Neither a held head nor its suffix is marked released");
        });
    }

    void dispatchRecognizesV13AndKeepsExactV12Membership() {
        check([] {
            DecisionFile file;
            file.record(1);
            auto adapter = std::make_shared<FakeDelivery>();
            const auto before = delivery_history(file);
            dispatch(file, second_job, 1, adapter, DeliveryDisposition::refused);
            require(delivery_history(file) == before, "Refused exact job has no new event");
            dispatch(file, first_job, 1, adapter, DeliveryDisposition::changed);
            dispatch(file, {origin, "legacy"}, 1, adapter, DeliveryDisposition::changed);
            require(adapter->starts.size() == 2,
                    "The released job and the independent legacy job each start once");
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

            advance_to_literal_decisions(file);
            (void)committed(*file.writer, [](const auto &s) {
                SqliteMilestoneGroupDecisions(s).observe_refund({first, "refund-17", 150, 99, 100});
                return true;
            });
            file.reopen();
            require(holds(file, first).empty() && !refund_audit(file, first),
                    "Started group gains no pre-start policy or hold");
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
            advance_to_literal_decisions(file);
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
            require_acceptance(file, second, Choice::defer, Resolution::continue_actions);
            require(holds(file, second).empty(), "Explicit Continue durably clears defer");
            release(file, second, Release::released);
            dispatch(file, second_job, 1, adapter, DeliveryDisposition::changed);
            require_order(file, {{"m-third"}, {"b-empty"}});
            require(recorded(file) == before, "Resuming a deferred group retains its capture");
        });
    }

    void rolledBackRefundLeavesNoHoldOrAudit() {
        check([] {
            DecisionFile file;
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
            DecisionFile file;
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
