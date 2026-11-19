// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/milestone_group_restart_fixture.hpp"

#include <QTest>
#include <exception>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace milestone_group_restart_fixture;
using testing::SqliteWriterWorkAccess;
using testing::WorkControlFault;
using Release = MilestoneReleaseDisposition;
using Bonus = MilestoneBonusDisposition;

const MilestoneOccurrenceKey first{"z-first"}, second{"a-second"}, third{"m-third"},
    empty{"b-empty"};
const MilestoneBonusKey first_bonus{first, 2}, second_bonus{second, 1};
constexpr auto releases_query =
    "SELECT CAST(occurrence_ref AS TEXT),queue_order,released FROM main.subathon_milestone_groups "
    "ORDER BY queue_order";
constexpr auto receipts_query =
    "SELECT CAST(occurrence_ref AS TEXT),action_ordinal,CAST(effect_ref AS TEXT) "
    "FROM main.subathon_milestone_bonus_receipts ORDER BY occurrence_ref,action_ordinal";
constexpr auto effects_query =
    "SELECT CAST(ref AS TEXT),units FROM main.synthetic_bonus_effects ORDER BY ref";
constexpr auto prefix_history =
    "SELECT * FROM main.subathon_migration_history WHERE version<=11 ORDER BY version";
constexpr auto delivery_captures_query =
    "SELECT * FROM main.subathon_delivery_captures ORDER BY operation_scope,operation_ref,job_ref";
constexpr auto delivery_events_query = "SELECT * FROM main.subathon_delivery_events "
                                       "ORDER BY operation_scope,operation_ref,job_ref,revision";

const SqlRows pending_rows{{QString("z-first"), 1, 0},
                           {QString("a-second"), 2, 0},
                           {QString("m-third"), 3, 0},
                           {QString("b-empty"), 4, 0}};
const SqlRows first_released_rows{{QString("z-first"), 1, 1},
                                  {QString("a-second"), 2, 0},
                                  {QString("m-third"), 3, 0},
                                  {QString("b-empty"), 4, 0}};
const SqlRows first_bonus_receipt{{QString("z-first"), 2, QString("effect-first")}};
const SqlRows first_effect{{QString("effect-first"), 37}};

template <class Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void check_captured(const CommittedMilestoneGroup &actual, const CommittedMilestoneGroup &literal) {
    require(actual.occurrence == literal.occurrence &&
                actual.achievement.milestone_ref == literal.achievement.milestone_ref &&
                actual.achievement.definition_revision_ref ==
                    literal.achievement.definition_revision_ref &&
                actual.achievement.target_micro_points == literal.achievement.target_micro_points &&
                actual.achievement.score_micro_points == literal.achievement.score_micro_points &&
                actual.achievement.completion_context_ref ==
                    literal.achievement.completion_context_ref &&
                actual.actions == literal.actions &&
                actual.timing.presentation == literal.timing.presentation &&
                actual.timing.gap == literal.timing.gap,
            "Exact occurrence, captured achievement, ordered action versions and timing");
}

bool same_key(const DeliveryJobKey &a, const DeliveryJobKey &b) {
    return a.operation.operation_scope == b.operation.operation_scope &&
           a.operation.operation_ref == b.operation.operation_ref && a.job_ref == b.job_ref;
}

std::vector<CapturedMilestoneGroup> pending(File &file) {
    return committed(*file.writer,
                     [](const auto &s) { return SqliteMilestoneGroupStore(s).pending(); });
}

std::vector<CommittedMilestoneGroup>
timeline_groups(const std::vector<CapturedMilestoneGroup> &captures) {
    std::vector<CommittedMilestoneGroup> groups;
    groups.reserve(captures.size());
    for (const auto &capture : captures)
        groups.push_back(capture.group);
    return groups;
}

void check_pending(const std::vector<CapturedMilestoneGroup> &actual, std::size_t offset) {
    const auto literal = captures();
    require(actual.size() == literal.size() - offset, "Exact unreleased suffix size");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto &expected = literal[i + offset];
        check_captured(actual[i].group, expected.group);
        require(actual[i].jobs.size() == expected.jobs.size(), "Exact job membership count");
        for (std::size_t j = 0; j < expected.jobs.size(); ++j)
            require(actual[i].jobs[j].action_ordinal == expected.jobs[j].action_ordinal &&
                        same_key(actual[i].jobs[j].job, expected.jobs[j].job),
                    "Exact captured action position and delivery job key");
    }
}

template <class Reply, class Submit> Reply completed(Submit submit) {
    struct Witness {
        std::optional<Reply> reply;
        int calls{0};
    };
    const auto witness = std::make_shared<Witness>();
    require(submit([witness](Reply result) {
                ++witness->calls;
                witness->reply = std::move(result);
            }) == StorageError::none,
            "Owner entry admits writer work");
    require(QTest::qWaitFor([witness] { return witness->reply.has_value(); }, timeout),
            "Owner entry completes writer work");
    require(witness->calls == 1, "One completion");
    return std::move(*witness->reply);
}

MilestoneReleaseCompletion release(File &file, const MilestoneOccurrenceKey &key) {
    return completed<MilestoneReleaseCompletion>([&](auto callback) {
        return release_milestone_head(*file.writer, key, std::move(callback));
    });
}

void acknowledged(const MilestoneReleaseCompletion &result, Release expected) {
    normal(result.work);
    require(result.disposition == expected, "Exact normal committed release result");
}

void unpublished(const SqliteWorkResult &work, TransactionOutcome outcome, WorkError error) {
    require(work.transaction == outcome && work.error == error && !work.value,
            "Actual writer failure or ambiguous outcome has no work value");
}

void check_events(File &file, const DeliveryJobKey &key, const SqlRows &expected) {
    require(
        file.reads("SELECT revision,state,coalesce(attempt,0),"
                   "coalesce(CAST(evidence_ref AS TEXT),'') FROM main.subathon_delivery_events "
                   "WHERE operation_scope=? AND operation_ref=? AND job_ref=? ORDER BY revision",
                   {app::blob(key.operation.operation_scope),
                    app::blob(key.operation.operation_ref), app::blob(key.job_ref)}) == expected,
        QString("Exact delivery history for %1/%2").arg(key.operation.operation_ref, key.job_ref));
}

const DeliveryJobKey first_job{origin, "job"};
const DeliveryJobKey second_job{following_origin, "job"};

const SqlRows queued{{1, 0, 0, QString("")}};
const SqlRows sending{{1, 0, 0, QString("")}, {2, 1, 2, QString("")}};
const SqlRows delivered{{1, 0, 0, QString("")}, {2, 1, 2, QString("")}, {3, 2, 2, QString("sent")}};
const SqlRows uncertain{
    {1, 0, 0, QString("")}, {2, 1, 2, QString("")}, {3, 4, 2, QString("interrupted-sending")}};

class AllowDispatch final : public DeliveryDispatchAuthority {
  public:
    bool allows(const DeliveryJob &job) override {
        checked.push_back(job.key);
        return permitted;
    }
    bool permitted{true};
    std::vector<DeliveryJobKey> checked;
};

class ObserveDispatch final : public DeliveryAdapter {
  public:
    explicit ObserveDispatch(QString path) : path_(std::move(path)) {}

    void start(DeliveryJob claim) override {
        SqliteReader reader;
        require(reader.open(path_) == StorageError::none, "Adapter witness opens independent read");
        const auto result = reader.read(
            {"SELECT state,attempt FROM main.subathon_delivery_events WHERE operation_scope=? "
             "AND operation_ref=? AND job_ref=? AND revision=2",
             {app::blob(claim.key.operation.operation_scope),
              app::blob(claim.key.operation.operation_ref), app::blob(claim.key.job_ref)}});
        require(result.error == StorageError::none && result.rows == SqlRows{{1, 2}},
                "Adapter sees the independently committed sending claim");
        starts.push_back(std::move(claim));
    }

    std::vector<DeliveryJob> starts;

  private:
    QString path_;
};

using Dispatch = DeliveryDisposition;
void dispatch(File &file, DeliveryJobKey key, std::int64_t revision,
              const std::shared_ptr<AllowDispatch> &authority,
              const std::shared_ptr<ObserveDispatch> &adapter, Dispatch expected) {
    const auto result = completed<DeliveryDispatchCompletion>([&](auto callback) {
        return dispatch_delivery_job(*file.writer, std::move(key), revision, authority, adapter,
                                     std::move(callback));
    });
    normal(result.work);
    require(result.result.has_value(), "Original dispatch publishes a normal committed result");
    require(result.result->disposition == expected,
            QString("Original dispatch disposition %1, expected %2")
                .arg(int(result.result->disposition))
                .arg(int(expected)));
    require(!result.adapter_threw &&
                result.result->job.has_value() == (expected == Dispatch::changed),
            "Original dispatch presents a claim only on a committed change");
}

class ObserveTimeline final : public MilestoneGroupRelease {
  public:
    bool review_held(const subathon::core::MilestoneCompletion &) const override { return false; }
    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &) const override {
        // These recovered v12 fixtures contain only ordinary pending groups.
        return MilestoneHeadDisposition::ready;
    }
    void release(CommittedMilestoneGroup group) override { starts.push_back(std::move(group)); }
    std::vector<CommittedMilestoneGroup> starts;
};

class BonusPorts final : public MilestoneBonusAuthority, public MilestoneBonusEffect {
  public:
    bool allows(const MilestoneBonusKey &key, const MilestoneAction &action) override {
        consulted.emplace(key, action);
        return permitted;
    }

    void check_authority(const MilestoneBonusKey &selected) const {
        require(consulted && consulted->first.occurrence == selected.occurrence &&
                    consulted->first.action_ordinal == selected.action_ordinal,
                "Current authority sees the exact selected bonus");
        check_capture(consulted->first, consulted->second);
    }

    std::string apply(const SqliteWorkSession &s, const MilestoneBonusKey &key,
                      const MilestoneAction &action) override {
        check_capture(key, action);
        ++effects;
        const bool is_first = key.occurrence == first;
        const QByteArray ref(is_first ? "effect-first" : "effect-second");
        (void)s.execute(
            {"INSERT INTO main.synthetic_bonus_effects VALUES (?,?)", {ref, is_first ? 37 : 19}});
        return ref.toStdString();
    }

    bool permitted{true};
    int effects{0};
    std::optional<std::pair<MilestoneBonusKey, MilestoneAction>> consulted;

  private:
    static void check_capture(const MilestoneBonusKey &key, const MilestoneAction &action) {
        require(((key.occurrence == first && key.action_ordinal == 2) ||
                 (key.occurrence == second && key.action_ordinal == 1)) &&
                    action == MilestoneAction{MilestoneActionKind::bonus, "award", "v4"},
                "Selected bonus carries its exact key and captured definition");
    }
};

void create_effects(File &file) {
    (void)committed(*file.writer, [](const auto &s) {
        (void)s.execute(
            {"CREATE TABLE synthetic_bonus_effects(ref BLOB PRIMARY KEY,units INTEGER)", {}});
        return true;
    });
}

Bonus bonus(File &file, MilestoneBonusKey key, std::shared_ptr<BonusPorts> ports) {
    return committed(*file.writer, [key = std::move(key), ports = std::move(ports)](const auto &s) {
        return SqliteMilestoneGroupStore(s).apply_bonus(key, *ports, *ports);
    });
}

SqliteWorkResult first_bonus_work(File &file, std::shared_ptr<BonusPorts> ports) {
    return work(*file.writer, [ports = std::move(ports)](auto context) -> QVariant {
        const auto s = context.begin();
        require(SqliteMilestoneGroupStore(s).apply_bonus(first_bonus, *ports, *ports) ==
                    Bonus::applied,
                "First eligible selected bonus stages its actual effect and receipt");
        s.commit();
        return 1;
    });
}
} // namespace

class MilestoneGroupRestartTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void additiveCatalogAdvancesPopulatedV11WithoutInferredMembership() {
        check([] {
            File existing(File::Schema::v11);
            (void)committed(*existing.writer, [&](const auto &s) {
                existing.seed_origin(s);
                existing.seed_following(s);
                return true;
            });
            const auto before = existing.reads(prefix_history);
            require(before.size() == 11, "Populated v11 has the complete history");
            constexpr auto jobs_query = "SELECT * FROM main.subathon_app_jobs ORDER BY "
                                        "operation_scope,operation_ref,ordinal";
            const auto jobs = existing.reads(jobs_query);
            const auto events = existing.reads(delivery_events_query);
            require(jobs.size() == 5 && events.size() == 5, "V11 has generic grouped queued jobs");

            const auto prefix = delivery_jobs_repository_manifest();
            const auto expected = literal_step(prefix);
            const auto actual = milestone_groups_repository_manifest();
            auto starts = prefix.supported_starting_versions;
            starts.append(12);
            require(actual.catalog_format == prefix.catalog_format &&
                        actual.store_identity == prefix.store_identity &&
                        actual.supported_starting_versions == starts && actual.steps.size() == 12,
                    "v12 preserves catalog identity and extends supported starting versions");
            for (qsizetype i = 0; i < 11; ++i) {
                const auto &old = prefix.steps[i];
                const auto &retained = actual.steps[i];
                require(old.version == retained.version && old.id == retained.id &&
                            old.sql == retained.sql &&
                            old.expected_schema == retained.expected_schema,
                        "Retain each frozen prefix statement and schema byte");
            }
            const auto &next = actual.steps.back();
            require(next.version == 12 && next.id == QString("milestone-groups-v1") &&
                        next.sql == expected.sql &&
                        next.expected_schema == expected.expected_schema,
                    "Exact additive v12 schema and migration identity");
            migrated(permission_graph_fixture::migrate(*existing.writer, actual), 12, 1);
            existing.reopen();
            require(existing.reads(prefix_history) == before &&
                        existing.reads(jobs_query) == jobs &&
                        existing.reads(delivery_events_query) == events,
                    "Upgrade preserves actual prefix history and generic job records");

            File fresh(File::Schema::empty);
            migrated(permission_graph_fixture::migrate(*fresh.writer, actual), 12, 12);
            fresh.reopen();
            for (const File *file : {&existing, &fresh}) {
                require(file->reads(releases_query).empty() &&
                            file->reads("SELECT * FROM main.subathon_milestone_actions").empty() &&
                            file->reads(receipts_query).empty(),
                        "Fresh and upgraded databases have empty milestone tables");
            }
            const auto authority = std::make_shared<AllowDispatch>();
            const auto adapter = std::make_shared<ObserveDispatch>(existing.path);
            const DeliveryJobKey legacy{origin, "legacy"};
            dispatch(existing, legacy, 1, authority, adapter, Dispatch::changed);
            require(adapter->starts.size() == 1 && same_key(adapter->starts.front().key, legacy),
                    "A captured independent v11 job remains selectable after actual upgrade");
            check_events(existing, legacy, sending);
            require(existing.reads(releases_query).empty(),
                    "Legacy selection creates no milestone occurrence");
        });
    }

    void originatingUnitRollsBackOrCommitsGroupsWithApplicationAndJobs() {
        check([] {
            File file;
            const auto groups = captures();
            const auto stage = [&](const auto &s) {
                file.seed_origin(s);
                SqliteMilestoneGroupStore(s).stage(origin, {groups[0]});
            };
            const auto rolled_back = work(*file.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                stage(s);
                s.rollback();
                return 1;
            });
            unpublished(rolled_back, TransactionOutcome::rolled_back, WorkError::not_committed);
            file.reopen();
            for (const auto *table : {"subathon_app_operations", "subathon_delivery_captures",
                                      "subathon_delivery_events", "subathon_milestone_groups",
                                      "subathon_milestone_actions"})
                require(file.reads(QString("SELECT * FROM main.") + table).empty(),
                        QString("Known rollback leaves no originating row in %1").arg(table));

            (void)committed(*file.writer, [&](const auto &s) {
                stage(s);
                return true;
            });
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_following(s);
                SqliteMilestoneGroupStore(s).stage(following_origin,
                                                  {groups[1], groups[2], groups[3]});
                return true;
            });
            file.reopen();
            require(file.reads(releases_query) == pending_rows &&
                        file.reads(
                            "SELECT CAST(operation_ref AS TEXT) FROM main.subathon_app_operations "
                            "ORDER BY operation_ref") ==
                            SqlRows{{QString("add10")}, {QString("negative_stop")}},
                    "Capture and originating operation are durable together in input order");
            require(
                file.reads("SELECT CAST(occurrence_ref AS "
                           "TEXT),action_ordinal,kind,CAST(action_ref AS TEXT),"
                           "CAST(version_ref AS TEXT),coalesce(CAST(job_ref AS TEXT),'') "
                           "FROM main.subathon_milestone_actions ORDER BY "
                           "occurrence_ref,action_ordinal") ==
                    SqlRows{
                        {QString("a-second"), 1, 1, QString("award"), QString("v4"), QString("")},
                        {QString("a-second"), 2, 0, QString("announce"), QString("v8"),
                         QString("job")},
                        {QString("m-third"), 1, 1, QString("award"), QString("v4"), QString("")},
                        {QString("z-first"), 1, 0, QString("announce"), QString("v7"),
                         QString("job")},
                        {QString("z-first"), 2, 1, QString("award"), QString("v4"), QString("")},
                        {QString("z-first"), 3, 0, QString("celebrate"), QString("v9"),
                         QString("done")},
                        {QString("z-first"), 4, 0, QString("post"), QString("v2"),
                         QString("stranded")}},
                "Only actual captured jobs and bonuses are enrolled, at exact positions");
            check_events(file, first_job, queued);
            check_events(file, second_job, queued);
        });
    }

    void invalidMembershipRollsBackOriginatingUnit_data() {
        QTest::addColumn<bool>("foreign");
        QTest::addColumn<QString>("action_ref");
        QTest::addColumn<QString>("version_ref");
        QTest::addColumn<int>("ordinal");
        QTest::newRow("foreign-operation") << true << QString("announce") << QString("v7") << 2;
        QTest::newRow("capture-ref") << false << QString("other") << QString("v7") << 2;
        QTest::newRow("capture-version") << false << QString("announce") << QString("other") << 2;
        QTest::newRow("noncontiguous-ordinal")
            << false << QString("announce") << QString("v7") << 3;
    }

    void invalidMembershipRollsBackOriginatingUnit() {
        QFETCH(bool, foreign);
        QFETCH(QString, action_ref);
        QFETCH(QString, version_ref);
        QFETCH(int, ordinal);
        check([&] {
            File file;
            (void)committed(*file.writer, [&](const auto &s) {
                file.seed_origin(s);
                return true;
            });
            const app::Data data;
            app::Observer before(file.path);
            const auto application = data.snapshot(before);
            const auto jobs = file.reads(delivery_captures_query);
            const auto events = file.reads(delivery_events_query);
            constexpr auto actions_query = "SELECT * FROM main.subathon_milestone_actions";

            auto batch = captures();
            auto &valid_prefix = batch[2];
            valid_prefix.group.achievement.target_micro_points = 175;
            auto &invalid = batch[1];
            invalid.group.actions[1].action_ref = action_ref.toStdString();
            invalid.group.actions[1].version_ref = version_ref.toStdString();
            invalid.jobs.front().action_ordinal = ordinal;
            if (foreign)
                invalid.jobs.front().job = {origin, "job"};

            bool reached_stage = false;
            std::exception_ptr ni;
            const auto failed = qt_test_support::run_work(
                *file.writer,
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    // Both candidate jobs capture announce/v7, isolating the operation mismatch.
                    file.seed_following(s, "v7");
                    reached_stage = true;
                    try {
                        SqliteMilestoneGroupStore(s).stage(following_origin,
                                                          {valid_prefix, invalid});
                    } catch (const MilestoneGroupsNotImplemented &) {
                        // Stage 1 must report the named NI instead of accepting it as rejection.
                        ni = std::current_exception();
                        throw;
                    }
                    s.commit();
                    return 1;
                },
                timeout);
            if (ni)
                std::rethrow_exception(ni);
            require(reached_stage, "Application and delivery staging succeeded before enrollment");
            require(failed.error == WorkError::work_exception ||
                        failed.error == WorkError::sql_failure,
                    "Invalid membership surfaces through an existing writer failure");
            require(failed.transaction == TransactionOutcome::rolled_back && !failed.value,
                    "Invalid membership rolls back the unit and publishes no work value");

            file.reopen();
            app::Observer after(file.path);
            require(data.snapshot(after) == application &&
                        file.reads(delivery_captures_query) == jobs &&
                        file.reads(delivery_events_query) == events &&
                        file.reads(releases_query).empty() && file.reads(actions_query).empty() &&
                        file.reads(receipts_query).empty(),
                    "Reopen preserves only preexisting application, delivery and group rows");
        });
    }

    void coldQueueReadsRecordedOrderAndImmutableDefinitions() {
        check([] {
            File file;
            file.record();
            create_effects(file);
            file.reopen();
            const auto restored = pending(file);
            check_pending(restored, 0);
            require(
                file.reads(releases_query) == pending_rows && file.reads(receipts_query).empty() &&
                    file.reads(effects_query).empty(),
                "Reading recovery data leaves all groups pending and no bonus receipt or effect");
            check_events(file, first_job, queued);
            check_events(file, second_job, queued);
            ObserveTimeline outputs;
            MilestoneTimeline timeline({}, outputs);
            timeline.restore_committed(timeline_groups(restored));
            require(
                !timeline.presentation() && outputs.starts.empty(),
                "Installing actual recovered captures does not start any group during recovery");
            timeline.advance(50000ms);
            QCOMPARE(timeline.presentation(), (ActiveMilestonePresentation{"peak-z", 0ms, 2500ms}));
            QCOMPARE(outputs.starts.size(), 1U);
            check_captured(outputs.starts.front(), captures().front().group);
        });
    }

    void firstRecoveredPresentationStartsFreshOnEligibleOwnerTurn() {
        check([] {
            const auto literal = captures();
            ObserveTimeline outputs;
            MilestoneTimeline timeline({}, outputs);
            timeline.restore_committed(timeline_groups(literal));
            require(!timeline.presentation() && outputs.starts.empty(),
                    "Restoration itself schedules no presentation or release");
            timeline.set_queue_paused(true, 200ms);
            timeline.advance(60000ms);
            require(!timeline.presentation() && outputs.starts.empty(),
                    "Paused owner turns do not start a recovered group");
            timeline.set_queue_paused(false, 65000ms);
            require(timeline.presentation() == ActiveMilestonePresentation{"peak-z", 0ms, 2500ms} &&
                        outputs.starts.size() == 1,
                    "First eligible owner turn starts the exact head with a fresh presentation");
            check_captured(outputs.starts.front(), literal[0].group);
            timeline.advance(67499ms);
            QCOMPARE(timeline.presentation(),
                     (ActiveMilestonePresentation{"peak-z", 2499ms, 2500ms}));
            timeline.advance(67500ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(67899ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(outputs.starts.size(), 1U);
            timeline.advance(67900ms);
            QCOMPARE(timeline.presentation(),
                     (ActiveMilestonePresentation{"peak-a", 0ms, 10000ms}));
            QCOMPARE(outputs.starts.size(), 2U);
            check_captured(outputs.starts.back(), literal[1].group);
        });
    }

    void emptyGroupReleasesAndRemainsRecordedOnReopen() {
        check([] {
            File file;
            file.record();
            acknowledged(release(file, empty), Release::refused);
            for (const auto &key : {first, second, third})
                acknowledged(release(file, key), Release::released);
            file.reopen();
            check_pending(pending(file), 3);
            acknowledged(release(file, empty), Release::released);
            file.reopen();
            check_pending(pending(file), 4);
            require(file.reads(releases_query) == SqlRows{{QString("z-first"), 1, 1},
                                                          {QString("a-second"), 2, 1},
                                                          {QString("m-third"), 3, 1},
                                                          {QString("b-empty"), 4, 1}},
                    "The released empty occurrence stays at its recorded order");
            require(
                file.reads(
                    "SELECT operation_scope,operation_ref,milestone_ref,definition_revision_ref,"
                    "target_micro_points,score_micro_points,completion_context_ref,"
                    "presentation_ms IS NULL,gap_ms IS NULL FROM main.subathon_milestone_groups "
                    "WHERE occurrence_ref=?",
                    {QByteArray("b-empty")}) ==
                        SqlRows{{QByteArray("svc"), QByteArray("negative_stop"),
                                 QByteArray("peak-b"), QByteArray("track-v4"), 350, 350,
                                 QByteArray("crossing-10"), 1, 1}} &&
                    file.reads("SELECT * FROM main.subathon_milestone_actions "
                               "WHERE occurrence_ref=?",
                               {QByteArray("b-empty")})
                        .empty(),
                "Empty occurrence retains its captured definition and has no action rows");
        });
    }

    void releaseOnlyExactHeadAndKeepKnownRollbackPending() {
        check([] {
            File file(File::Schema::literal_v12, File::Fault::refuse_release_write);
            file.record();
            acknowledged(release(file, second), Release::refused);
            require(file.reads(releases_query) == pending_rows,
                    "Refusal leaves both predecessors and successors intact");
            const auto failed = release(file, first);
            unpublished(failed.work, TransactionOutcome::rolled_back, WorkError::sql_failure);
            require(!failed.disposition, "Known rollback publishes no eligibility");
            file.reopen();
            require(file.reads(releases_query) == pending_rows, "Exact head stays pending on disk");
            check_events(file, first_job, queued);
            const auto authority = std::make_shared<AllowDispatch>();
            const auto adapter = std::make_shared<ObserveDispatch>(file.path);
            dispatch(file, {origin, "job"}, 1, authority, adapter, Dispatch::refused);
            require(adapter->starts.empty(), "Failed release cannot start delivery");
        });
    }

    void lostReleaseAckPublishesNothingAndReopenReadsCommittedSuffix() {
        check([] {
            File file;
            file.record();
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *file.writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "Install real post-COMMIT acknowledgement loss");
            const auto result = release(file, first);
            unpublished(result.work, TransactionOutcome::outcome_unknown,
                        WorkError::uncertain_outcome);
            require(!result.disposition, "Ambiguity carries no committed eligibility publication");
            file.reopen();
            require(file.reads(releases_query) == first_released_rows,
                    "Independent read sees what actually committed, including the later pending "
                    "groups");
            check_pending(pending(file), 1);
            check_events(file, first_job, queued);
            check_events(file, second_job, queued);
            require(file.reads(receipts_query).empty(),
                    "Release and recovery do not apply bonuses");
        });
    }

    void originalDispatchRefusesPendingExactMembership() {
        check([] {
            File file;
            file.record(1);
            file.reopen();
            const auto authority = std::make_shared<AllowDispatch>();
            const auto adapter = std::make_shared<ObserveDispatch>(file.path);
            dispatch(file, second_job, 1, authority, adapter, Dispatch::refused);
            require(adapter->starts.empty(), "Sibling release does not send a pending group's job");
            check_events(file, second_job, queued);
        });
    }

    void committedReleaseAndCurrentAuthorityPermitOneLaterClaim() {
        check([] {
            File file;
            file.record();
            const auto authority = std::make_shared<AllowDispatch>();
            const auto adapter = std::make_shared<ObserveDispatch>(file.path);
            acknowledged(release(file, first), Release::released);
            acknowledged(release(file, first), Release::refused);
            require(file.reads(releases_query) == first_released_rows && adapter->starts.empty(),
                    "Release is durable before and separate from a delivery claim");
            check_events(file, first_job, queued);
            file.reopen();
            authority->permitted = false;
            dispatch(file, {origin, "job"}, 1, authority, adapter, Dispatch::refused);
            require(adapter->starts.empty(),
                    "Committed release cannot substitute for current authority");
            check_events(file, first_job, queued);
            authority->permitted = true;
            dispatch(file, {origin, "job"}, 1, authority, adapter, Dispatch::changed);
            require(adapter->starts.size() == 1 &&
                        same_key(adapter->starts[0].key, {origin, "job"}) &&
                        adapter->starts[0].state == DeliveryJobState::sending &&
                        adapter->starts[0].revision == 2 && adapter->starts[0].attempt == 2 &&
                        adapter->starts[0].capture.action_version_ref == "v7" &&
                        adapter->starts[0].action_document == action_document,
                    "One committed exact claim hands captured version and bytes to the adapter");
            dispatch(file, {origin, "job"}, 1, authority, adapter, Dispatch::stale);
            require(adapter->starts.size() == 1, "Repeat selection cannot start a second adapter");
            require(
                authority->checked.size() == 2 && same_key(authority->checked[0], first_job) &&
                    same_key(authority->checked[1], first_job),
                "Denied and allowed attempts check current authority for the exact selected job");
            check_events(file, first_job, sending);
        });
    }

    void independentV11JobDoesNotInheritTypedMilestoneGate() {
        check([] {
            File file;
            file.record();
            file.reopen();
            const auto authority = std::make_shared<AllowDispatch>();
            const auto adapter = std::make_shared<ObserveDispatch>(file.path);
            dispatch(file, {origin, "legacy"}, 1, authority, adapter, Dispatch::changed);
            require(adapter->starts.size() == 1 && adapter->starts[0].key.job_ref == "legacy",
                    "Generic grouping alone does not identify milestone membership");
            check_events(file, {origin, "legacy"}, sending);
            check_events(file, second_job, queued);
            require(file.reads(releases_query) == pending_rows,
                    "Independent job did not release any group");
        });
    }

    void recoveryKeepsCompletedEffectsAndOnlySeparateSweepHoldsStrandedSend() {
        check([] {
            File file;
            file.record(1);
            create_effects(file);
            (void)committed(*file.writer, [](const auto &s) {
                (void)s.execute({"INSERT INTO main.subathon_delivery_events VALUES "
                                 "(X'737663',X'6164643130',X'646f6e65',2,1,2,NULL),"
                                 "(X'737663',X'6164643130',X'646f6e65',3,2,2,X'73656e74'),"
                                 "(X'737663',X'6164643130',X'737472616e646564',2,1,2,NULL)",
                                 {}});
                (void)s.execute({"INSERT INTO main.synthetic_bonus_effects VALUES "
                                 "(X'6566666563742d6669727374',37)",
                                 {}});
                (void)s.execute({"INSERT INTO main.subathon_milestone_bonus_receipts VALUES "
                                 "(X'7a2d6669727374',2,X'6566666563742d6669727374')",
                                 {}});
                return true;
            });
            file.reopen();
            check_pending(pending(file), 1);
            check_events(file, {origin, "done"}, delivered);
            check_events(file, {origin, "stranded"}, sending);
            const auto page = committed(*file.writer, [](const auto &s) {
                return SqliteDeliveryJobStore(s).stranded({}, 8);
            });
            require(page.attempts.size() == 1 && !page.next_after &&
                        same_key(page.attempts[0].key, {origin, "stranded"}) &&
                        page.attempts[0].sending_revision == 2,
                    "Separate exclusive #232 sweep discovers only the stranded send");
            const auto transitioned = committed(*file.writer, [&](const auto &s) {
                const auto &attempt = page.attempts.front();
                return SqliteDeliveryJobStore(s).recover(attempt.key, attempt.sending_revision);
            });
            require(transitioned.disposition == Dispatch::changed,
                    "Stranded attempt is classified by #232");
            file.reopen();
            check_pending(pending(file), 1);
            const auto authority = std::make_shared<AllowDispatch>();
            const auto adapter = std::make_shared<ObserveDispatch>(file.path);
            dispatch(file, {origin, "done"}, 3, authority, adapter, Dispatch::refused);
            dispatch(file, {origin, "stranded"}, 3, authority, adapter, Dispatch::refused);
            require(adapter->starts.empty(),
                    "Completed and uncertain sends never reach the adapter again");
            check_events(file, {origin, "done"}, delivered);
            check_events(file, {origin, "stranded"}, uncertain);
            check_events(file, first_job, queued);
            check_events(file, second_job, queued);
            require(file.reads(effects_query) == first_effect &&
                        file.reads(receipts_query) == first_bonus_receipt,
                    "Previously applied bonus effect and its receipt are preserved");
        });
    }

    void bonusRequiresExactReleasedGroupCurrentAuthorityAndCapturedBonusPosition() {
        check([] {
            File file;
            file.record();
            create_effects(file);
            const auto ports = std::make_shared<BonusPorts>();
            QCOMPARE(bonus(file, first_bonus, ports), Bonus::refused);
            require(ports->effects == 0 && file.reads(effects_query).empty() &&
                        file.reads(receipts_query).empty(),
                    "Pending exact group has no provisional or durable bonus effect");
            acknowledged(release(file, first), Release::released);
            QCOMPARE(bonus(file, second_bonus, ports), Bonus::refused);
            QCOMPARE(bonus(file, {first, 1}, ports), Bonus::refused);
            ports->permitted = false;
            ports->consulted.reset();
            QCOMPARE(bonus(file, first_bonus, ports), Bonus::refused);
            ports->check_authority(first_bonus);
            require(ports->effects == 0 && file.reads(effects_query).empty() &&
                        file.reads(receipts_query).empty(),
                    "Sibling bonus, automation position and authority denial leave no effect or "
                    "receipt");
            ports->permitted = true;
            ports->consulted.reset();
            QCOMPARE(bonus(file, first_bonus, ports), Bonus::applied);
            ports->check_authority(first_bonus);
            file.reopen();
            QCOMPARE(bonus(file, first_bonus, ports), Bonus::already_applied);
            require(ports->effects == 1 && file.reads(effects_query) == first_effect &&
                        file.reads(receipts_query) == first_bonus_receipt,
                    "Exactly one actual effect and stable receipt survive reopen");
            acknowledged(release(file, second), Release::released);
            ports->consulted.reset();
            QCOMPARE(bonus(file, second_bonus, ports), Bonus::applied);
            ports->check_authority(second_bonus);
            file.reopen();
            require(ports->effects == 2 &&
                        file.reads(effects_query) == SqlRows{{QString("effect-first"), 37},
                                                             {QString("effect-second"), 19}} &&
                        file.reads(receipts_query) ==
                            SqlRows{{QString("a-second"), 1, QString("effect-second")},
                                    {QString("z-first"), 2, QString("effect-first")}},
                    "Identical bonus definitions in separate occurrences get distinct receipts");
        });
    }

    void bonusReceiptWriteFailureRollsBackActualEffect() {
        check([] {
            File file(File::Schema::literal_v12, File::Fault::refuse_receipt_write);
            file.record(1);
            create_effects(file);
            const auto ports = std::make_shared<BonusPorts>();
            unpublished(first_bonus_work(file, ports), TransactionOutcome::rolled_back,
                        WorkError::sql_failure);
            file.reopen();
            require(
                ports->effects == 1 && file.reads(effects_query).empty() &&
                    file.reads(receipts_query).empty(),
                "Real receipt write failure rolls back the actual effect in the same writer work");
        });
    }

    void lostBonusAckLeavesOneActualEffectAndStableReceipt() {
        check([] {
            File file;
            file.record(1);
            create_effects(file);
            const auto ports = std::make_shared<BonusPorts>();
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *file.writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "Install real post-COMMIT acknowledgement loss");
            unpublished(first_bonus_work(file, ports), TransactionOutcome::outcome_unknown,
                        WorkError::uncertain_outcome);
            file.reopen();
            require(
                file.reads(effects_query) == first_effect &&
                    file.reads(receipts_query) == first_bonus_receipt,
                "Actual committed effect and exact receipt exist despite missing acknowledgement");
            check_pending(pending(file), 1);
            QCOMPARE(bonus(file, first_bonus, ports), Bonus::already_applied);
            require(ports->effects == 1 && file.reads(effects_query) == first_effect &&
                        file.reads(receipts_query) == first_bonus_receipt,
                    "Recovery and later explicit selection do not replay a committed bonus");
        });
    }
};

QTEST_GUILESS_MAIN(MilestoneGroupRestartTests)
#include "milestone_group_restart_tests.moc"
