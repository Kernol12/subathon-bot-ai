// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_modifier_history_fixture.hpp"

#include <QTest>
#include <type_traits>

namespace {
using namespace modifier_activation_fixture;
using Lifetime = ModifierInstanceLifetime;
using Operation = ModifierInstanceOperation;
using Input = SqliteModifierInputError;
using Reason = SqliteModifierStorageReason;
using Domain = core::ModifierActivationError;

const ModifierInstanceKey old_live{"I", "j1"}, new_live{"J", "k0"};
const core::ModifierActivation replace_activation{"J", "k0", rule, subsequent};

template <class Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

template <class Exception, class Code, class Call> void refuse(Code expected, Call call) {
    try {
        call();
    } catch (const Exception &error) {
        if constexpr (std::is_same_v<Exception, SqliteModifierStorageFailure>)
            require(error.reason() == expected, "Exact modifier storage refusal");
        else if constexpr (std::is_same_v<Exception, SqliteModifierInputFailure>)
            require(error.error() == expected, "Exact modifier input refusal");
        else
            require(error.code() == expected, "Exact core modifier failure");
        return;
    }
    require(false, "Expected typed modifier refusal");
}

void rolled_back(const SqliteWorkResult &result, WorkError error) {
    require(
        result.transaction == TransactionOutcome::rolled_back && result.error == error &&
            !result.value,
        QString("Known rollback without payload (expected error %1, got transaction %2/error %3)")
            .arg(int(error))
            .arg(int(result.transaction))
            .arg(int(result.error)));
}

// Capture escaped causes without short-circuiting the writer completion oracle.
struct FailedWork {
    SqliteWorkResult result;
    std::exception_ptr cause;
};
FailedWork failed_work(File &file, SqliteWriter::Work body) {
    const auto cause = std::make_shared<std::exception_ptr>();
    auto result = qt_test_support::run_work(
        *file.writer,
        [cause, body = std::move(body)](auto context) {
            try {
                return body(context);
            } catch (...) {
                *cause = std::current_exception();
                throw;
            }
        },
        timeout);
    return {std::move(result), *cause};
}

void require_replaced(const SqliteWorkSession &s) {
    const SqliteModifierActivationStore store(s);
    require(store.current_definition("D") == SqliteModifierDefinitionRevision{new_definition, 2} &&
                store.live_instance("D") ==
                    SqliteModifierInstanceRevision{
                        k0, 1, Operation::activation, "a-new", Lifetime::live, {}},
            "D has the edited head and J/k0 is its sole live revision");
    require(
        store.definition_revision("z-old") == SqliteModifierDefinitionRevision{old_definition, 1} &&
            store.instance_revision("j0") ==
                SqliteModifierInstanceRevision{initial_instance, 1, Operation::activation, "z-old",
                                               Lifetime::replaced, "J"} &&
            store.instance_revision("j1") ==
                SqliteModifierInstanceRevision{
                    checkpointed_instance, 2, Operation::checkpoint, {}, Lifetime::replaced, "J"},
        "Bare exact refs preserve owner D/I, saved values and I's successor J");
}

template <class Stage> void stage(File &file, Stage call) {
    (void)committed(*file.writer, [&](const auto &s) {
        call(s);
        return true;
    });
}

void seed_replaced_with_original(File &file) {
    stage(file, seed_old_live);
    seed_original_application(file);
    stage(file, [](const auto &s) {
        seed_definition_edit(s);
        seed_replacement(s);
    });
}

void require_original_after_reopen(File &file) {
    reopen(file);
    stage(file, require_original);
    require_original_rows(file);
}

struct LegacyRows {
    SqlRows releases, holds, jobs, events, operations, receipts, plans, effects, bonuses;
    bool operator==(const LegacyRows &) const = default;
};

LegacyRows legacy_rows(const File &file) {
    return {
        file.reads("SELECT * FROM main.subathon_milestone_groups ORDER BY queue_order"),
        file.reads(
            "SELECT * FROM main.subathon_milestone_acceptance_decisions ORDER BY occurrence_ref"),
        file.reads("SELECT * FROM main.subathon_delivery_captures "
                   "ORDER BY operation_scope,operation_ref,job_ref"),
        file.reads("SELECT operation_scope,operation_ref,job_ref,revision,state,attempt,"
                   "evidence_ref IS NULL,hex(evidence_ref) FROM main.subathon_delivery_events "
                   "ORDER BY operation_scope,operation_ref,job_ref,revision"),
        file.reads(
            "SELECT * FROM main.subathon_app_operations ORDER BY operation_scope,operation_ref"),
        file.reads("SELECT * FROM main.subathon_receipts ORDER BY receipt_id"),
        file.reads("SELECT * FROM main.subathon_plan_revisions ORDER BY plan_ref,revision"),
        file.reads("SELECT * FROM main.subathon_app_effects "
                   "ORDER BY operation_scope,operation_ref,request_ordinal"),
        file.reads("SELECT * FROM main.subathon_milestone_bonus_receipts "
                   "ORDER BY occurrence_ref,action_ordinal"),
    };
}

void seed_held_group(File &file) {
    stage(file, [&](const auto &s) {
        file.seed_origin(s);
        seed_head(s, 0);
        SqliteMilestoneGroupDecisions(s).record_acceptance(
            origin, {PausedAcceptanceMode::normal,
                     {"operator-a", "accept-31"},
                     {{{"z-first"}, MilestoneActionChoice::defer}}});
    });
    require(file.reads("SELECT occurrence_ref,released FROM main.subathon_milestone_groups") ==
                    SqlRows{{QByteArray("z-first"), 0}} &&
                file.reads("SELECT occurrence_ref,choice,actor_ref,decision_ref,resolution IS NULL "
                           "FROM main.subathon_milestone_acceptance_decisions") ==
                    SqlRows{{QByteArray("z-first"), 1, QByteArray("operator-a"),
                             QByteArray("accept-31"), 1}},
            "Populated v13 starts with its unreleased head and the active Defer decision");
    require(
        file.reads("SELECT e.job_ref,e.state,e.revision,e.attempt IS NULL,EXISTS("
                   "SELECT 1 FROM main.subathon_milestone_actions a "
                   "WHERE a.operation_scope=e.operation_scope AND a.operation_ref=e.operation_ref "
                   "AND a.job_ref=e.job_ref) FROM main.subathon_delivery_events e "
                   "WHERE e.operation_scope=X'737663' AND e.operation_ref=X'6164643130' "
                   "AND e.job_ref IN (X'6a6f62',X'6c6567616379') ORDER BY e.job_ref") ==
            SqlRows{{QByteArray("job"), 0, 1, 1, 1}, {QByteArray("legacy"), 0, 1, 1, 0}},
        "Grouped and independent jobs are both queued at revision 1");
}

SqlRows expected_history(qint32 version) {
    SqlRows rows;
    for (qint32 entry = 1; entry <= version; ++entry) {
        const auto &pin = catalog_pins.at(static_cast<std::size_t>(entry - 1));
        rows.append({qint64(entry), QString::fromLatin1(pin.id), pinned_digest(entry)});
    }
    return rows;
}

void require_schema(File &file, const MigrationManifest &literal) {
    QList<CatalogObject> expected;
    for (const auto &object : literal.steps.back().expected_schema)
        if (object.table_name.startsWith("subathon_modifier_"))
            expected.append(object);
    const auto actual = file.reads(modifiers_schema_sql);
    require(actual.size() == 11 && expected.size() == 11, "Exactly eleven additive schema objects");
    for (qsizetype i = 0; i < actual.size(); ++i) {
        const auto &row = actual[i];
        const auto &object = expected[i];
        require(row[0] == object.type && row[1] == object.name && row[2] == object.table_name &&
                    (object.sql ? row[3] == *object.sql : row[3].isNull()),
                "The independent projection matches exact SQL or SQL NULL");
    }
}

void require_empty_modifier_tables(const File &file) {
    for (const auto *table :
         {"subathon_modifier_definition_revisions", "subathon_modifier_targets",
          "subathon_modifier_instances", "subathon_modifier_instance_revisions"})
        require(file.reads(QString("SELECT COUNT(*) FROM main.") + table) == SqlRows{{0}},
                QString("Additive catalog leaves %1 empty").arg(table));
}

MigrationResult migrate_request(File &file, MigrationRequest request) {
    auto result = std::make_shared<std::optional<MigrationResult>>();
    const auto accepted =
        SqliteMigrationRunner::enqueue(*file.writer, std::move(request),
                                       [result](auto done) { result->emplace(std::move(done)); });
    require(accepted.error == StorageError::none && accepted.reason == MigrationReason::none,
            "Explicit catalog operation is admitted");
    require(QTest::qWaitFor([result] { return result->has_value(); }, timeout),
            "Catalog operation completes");
    return std::move(result->value());
}

template <class Reply, class Submit> Reply completed(Submit submit) {
    const auto reply = std::make_shared<std::optional<Reply>>();
    require(submit([reply](Reply result) { reply->emplace(std::move(result)); }) ==
                StorageError::none,
            "Existing owner entry admits work");
    require(QTest::qWaitFor([reply] { return reply->has_value(); }, timeout),
            "Existing owner entry posts a completion");
    return std::move(reply->value());
}

class FakeDelivery final : public DeliveryDispatchAuthority, public DeliveryAdapter {
  public:
    explicit FakeDelivery(const File &file) : file_(file) {}
    bool allows(const DeliveryJob &job) override {
        authorized.push_back(job);
        return true;
    }
    void start(DeliveryJob job) override {
        starts.push_back(std::move(job));
        // Independent connection proves the event is already durable at start.
        events_at_start =
            file_.reads("SELECT state,revision,attempt FROM main.subathon_delivery_events "
                        "WHERE operation_scope=X'737663' AND operation_ref=X'6164643130' "
                        "AND job_ref=X'6c6567616379' AND revision=2");
    }

    std::vector<DeliveryJob> authorized, starts;
    SqlRows events_at_start;

  private:
    const File &file_;
};

DeliveryDispatchCompletion dispatch(File &file, const char *job,
                                    const std::shared_ptr<FakeDelivery> &fake) {
    return completed<DeliveryDispatchCompletion>([&](auto callback) {
        return dispatch_delivery_job(*file.writer, {origin, job}, 1, fake, fake,
                                     std::move(callback));
    });
}

void literal_v14_from_held_v13(File &file) {
    seed_held_group(file);
    const auto before = legacy_rows(file);
    const auto v14 = literal_modifier_manifest();
    migrated(permission_graph_fixture::migrate(*file.writer, v14), 14, 1);
    reopen(file);
    require(file.reads(history_sql) == expected_history(14) && legacy_rows(file) == before,
            "Literal v14 marker and prior hold/job/application rows survive reopen");
    require_schema(file, v14);
}

class SqliteModifierActivationsTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void appendedDefinitionEditKeepsTheLiveCapture() {
        check([] {
            ModifierFile file;
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.append_definition(old_definition, {}) ==
                            SqliteModifierDefinitionRevision{old_definition, 1},
                        "First definition revision owns D at position 1");
                const auto first = store.activate("D", "z-old", {}, {"I", "j0", moderator, {}});
                require(first.revision_to_append == initial_instance &&
                            !first.superseded_instance_ref && !first.for_trigger,
                        "No expected live head creates I/j0 with no trigger");
                require(store.checkpoint_remaining({"I", "j0"}, "j1", 180000) ==
                            SqliteModifierInstanceRevision{checkpointed_instance,
                                                           2,
                                                           Operation::checkpoint,
                                                           {},
                                                           Lifetime::live,
                                                           {}},
                        "Checkpoint keeps original capture and source without an applied revision");
            });
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.append_definition(new_definition, "z-old") ==
                            SqliteModifierDefinitionRevision{new_definition, 2},
                        "Definition append uses ordinal rather than lexical revision order");
            });
            reopen(file);
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.current_definition("D") ==
                                SqliteModifierDefinitionRevision{new_definition, 2} &&
                            store.live_instance("D") ==
                                SqliteModifierInstanceRevision{checkpointed_instance,
                                                               2,
                                                               Operation::checkpoint,
                                                               {},
                                                               Lifetime::live,
                                                               {}},
                        "Reopened head is a-new and live I/j1 still captures z-old");
            });
        });
    }

    void replacementKeepsExactHistoryAndOriginalCredit() {
        check([] {
            ModifierFile file;
            stage(file, seed_old_live);
            seed_original_application(file);
            stage(file, seed_definition_edit);
            reopen(file);
            stage(file, [](const auto &s) {
                const auto proposal = SqliteModifierActivationStore(s).activate(
                    "D", "a-new", old_live, replace_activation);
                require(proposal.revision_to_append == k0 &&
                            proposal.superseded_instance_ref == "I" &&
                            proposal.for_trigger == checkpointed_instance,
                        "Subsequent Replace stages J/k0 and supplies I/j1 to event-B");
            });
            require_original_after_reopen(file);
            stage(file, [](const auto &s) {
                require_replaced(s);
                const SqliteModifierActivationStore store(s);
                const auto plan = SqliteApplicationPlanStore(s).find_revision("plan-A", 1);
                require(plan && plan->revision.modifiers.size() == 1,
                        "Original plan has one exact selected modifier");
                const auto &selected = plan->revision.modifiers.front();
                const auto definition = store.definition_revision(selected.definition_version_ref);
                const auto instance = store.instance_revision(selected.instance_revision_ref);
                require(definition == SqliteModifierDefinitionRevision{old_definition, 1} &&
                            instance &&
                            instance->instance.captured_definition == definition->definition,
                        "The existing selected plan resolves its original pair by bare refs");
                refuse<SqliteModifierStorageFailure>(Reason::stale_instance, [&] {
                    (void)store.activate("D", "a-new", old_live, {"L", "l0", rule, subsequent});
                });
            });
            require(file.reads("SELECT instance_ref,definition_ref,lifetime,"
                               "COALESCE(successor_instance_ref,X'') "
                               "FROM main.subathon_modifier_instances ORDER BY instance_ref") ==
                        SqlRows{{QByteArray("I"), QByteArray("D"), 1, QByteArray("J")},
                                {QByteArray("J"), QByteArray("D"), 0, QByteArray("")}},
                    "Independent lifetime rows retain I to J and leave no stale activation");
            // A later 60000 × 0.50 would request 30000; the original rows above
            // and the reversal basis continue to describe the original 2.00 request.
        });
    }

    void bareLookupsValidateBeforeSqlAndOwnTheSelectedRefs() {
        check([] {
            File without_catalog(File::Schema::empty);
            stage(without_catalog, [](const auto &s) {
                // Shape validation precedes both catalog and required schema SQL.
                const SqliteModifierActivationStore store(s);
                for (const auto &bad : {std::string{}, std::string(4097, 'x')}) {
                    refuse<SqliteModifierInputFailure>(
                        Input::invalid_input, [&] { (void)store.definition_revision(bad); });
                    refuse<SqliteModifierInputFailure>(Input::invalid_input,
                                                       [&] { (void)store.instance_revision(bad); });
                    refuse<SqliteModifierInputFailure>(
                        Input::invalid_input, [&] { (void)store.current_definition(bad); });
                    refuse<SqliteModifierInputFailure>(Input::invalid_input,
                                                       [&] { (void)store.live_instance(bad); });
                }
            });
            ModifierFile file;
            stage(file, seed_replaced);
            reopen(file);
            const auto exact = committed(*file.writer, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(!store.current_definition("missing-def") &&
                            !store.live_instance("missing-def") &&
                            !store.definition_revision("missing-rev") &&
                            !store.instance_revision("missing-rev") &&
                            !store.definition_revision("Z-old") &&
                            !store.definition_revision(std::string("z-old\0", 6)) &&
                            !store.instance_revision("J1") &&
                            !store.instance_revision(std::string("j1\0", 3)),
                        "Bare refs are matched bytewise, including case and NUL");
                return std::pair{store.definition_revision("z-old"), store.instance_revision("j1")};
            });
            require(exact.first == SqliteModifierDefinitionRevision{old_definition, 1} &&
                        exact.second == SqliteModifierInstanceRevision{checkpointed_instance,
                                                                       2,
                                                                       Operation::checkpoint,
                                                                       {},
                                                                       Lifetime::replaced,
                                                                       "J"},
                    "After session revocation, exact values own their original refs and targets");
        });
    }

    void zeroCheckpointExpiresWithoutErasingHistory() {
        check([] {
            ModifierFile file;
            stage(file, seed_replaced);
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.checkpoint_remaining(new_live, "k1", 0) ==
                            SqliteModifierInstanceRevision{
                                k1, 2, Operation::checkpoint, {}, Lifetime::expired, {}},
                        "Zero remaining stages terminal k1 with retained source and trigger");
            });
            reopen(file);
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(!store.live_instance("D") &&
                            store.instance_revision("k0") ==
                                SqliteModifierInstanceRevision{
                                    k0, 1, Operation::activation, "a-new", Lifetime::expired, {}} &&
                            store.instance_revision("k1") ==
                                SqliteModifierInstanceRevision{
                                    k1, 2, Operation::checkpoint, {}, Lifetime::expired, {}} &&
                            store.instance_revision("j1") ==
                                SqliteModifierInstanceRevision{checkpointed_instance,
                                                               2,
                                                               Operation::checkpoint,
                                                               {},
                                                               Lifetime::replaced,
                                                               "J"},
                        "Saved positive and terminal remaining are distinct from current expiry");
                refuse<SqliteModifierStorageFailure>(Reason::stale_instance, [&] {
                    (void)store.checkpoint_remaining({"J", "k1"}, "k2", 0);
                });
                const auto fresh = store.activate("D", "a-new", {}, {"L", "l0", moderator, {}});
                const core::ModifierInstance l0{"L",    "l0",      {}, new_definition,
                                                600000, moderator, {}};
                require(fresh.revision_to_append == l0 && !fresh.superseded_instance_ref &&
                            !fresh.for_trigger,
                        "A later activation after expiry is a fresh lineage");
            });
        });
    }

    void literalLiveIndexRejectsSecondLiveInstance() {
        check([] {
            ModifierFile file;
            stage(file, seed_replaced);
            const auto before = file.reads(instance_rows);
            const auto failure = failed_work(file, [](auto context) -> QVariant {
                const auto s = context.begin();
                (void)s.execute({"INSERT INTO main.subathon_modifier_instances VALUES(?,?,0,NULL)",
                                 {QByteArray("L"), QByteArray("D")}});
                s.commit();
                return 1;
            });
            rolled_back(failure.result, WorkError::sql_failure);
            reopen(file);
            require(before.size() == 2 && file.reads(instance_rows) == before,
                    "A new PK on the same live definition violates the literal partial index");
        });
    }

    void independentDefinitionCanHaveItsOwnLiveInstance() {
        check([] {
            ModifierFile file;
            stage(file, seed_replaced);
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.append_definition(independent_definition, {}) ==
                            SqliteModifierDefinitionRevision{independent_definition, 1},
                        "Different logical definition starts another history");
                const core::ModifierInstance l0{"L", "l0",      {}, independent_definition,
                                                1,   moderator, {}};
                const auto first = store.activate("E", "e0", {}, {"L", "l0", moderator, {}});
                require(first.revision_to_append == l0 && !first.superseded_instance_ref &&
                            !first.for_trigger,
                        "Independent lane can become live");
            });
            reopen(file);
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                const core::ModifierInstance l0{"L", "l0",      {}, independent_definition,
                                                1,   moderator, {}};
                require(store.live_instance("E") ==
                                SqliteModifierInstanceRevision{
                                    l0, 1, Operation::activation, "e0", Lifetime::live, {}} &&
                            store.live_instance("D") ==
                                SqliteModifierInstanceRevision{
                                    k0, 1, Operation::activation, "a-new", Lifetime::live, {}},
                        "Live uniqueness is per logical definition");
            });
        });
    }

    void healthyZeroDurationRefusal_data() {
        QTest::addColumn<int>("finish");
        QTest::newRow("caught-commit") << 0;
        QTest::newRow("escaped") << 1;
        QTest::newRow("caught-active-return") << 2;
    }

    void healthyZeroDurationRefusal() {
        QFETCH(int, finish);
        check([&] {
            ModifierFile file;
            stage(file, seed_replaced);
            const auto outcome = failed_work(file, [finish](auto context) -> QVariant {
                const auto s = context.begin();
                const SqliteModifierActivationStore store(s);
                require(store.append_definition(independent_definition, {}) ==
                            SqliteModifierDefinitionRevision{independent_definition, 1},
                        "Preceding independent definition is staged");
                try {
                    (void)store.append_definition(
                        {"D", "d-zero", 0, {}, core::ModifierReactivation::extend}, "a-new");
                } catch (const SqliteModifierInputFailure &error) {
                    require(error.error() == Input::invalid_input,
                            "Configured zero is invalid_input, unlike observed terminal zero");
                    if (finish == 1)
                        throw;
                    if (finish == 0)
                        s.commit();
                    return 47;
                }
                throw std::runtime_error("Expected zero-duration refusal");
            });
            if (outcome.cause) {
                if (finish == 1) {
                    refuse<SqliteModifierInputFailure>(
                        Input::invalid_input, [&] { std::rethrow_exception(outcome.cause); });
                } else {
                    std::rethrow_exception(outcome.cause);
                }
            }
            require(bool(outcome.cause) == (finish == 1),
                    "Only the escaping healthy refusal propagates a typed exception");
            if (finish == 0) {
                normal(outcome.result);
                require(outcome.result.value == QVariant(47),
                        "Healthy commit keeps caller payload");
            } else {
                rolled_back(outcome.result,
                            finish == 1 ? WorkError::work_exception : WorkError::uncommitted);
            }
            reopen(file);
            stage(file, [finish](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.current_definition("D") ==
                                SqliteModifierDefinitionRevision{new_definition, 2} &&
                            !store.definition_revision("d-zero"),
                        "No zero-duration revision or moved head");
                const auto independent = store.current_definition("E");
                require(finish == 0
                            ? independent ==
                                  SqliteModifierDefinitionRevision{independent_definition, 1}
                            : !independent,
                        "Prior staging survives only ordinary acknowledged commit");
            });
        });
    }

    void typedPreflightAndFreshnessRefusalsLeaveTheSessionHealthy() {
        check([] {
            ModifierFile file;
            stage(file, seed_replaced);
            const auto before = file.reads(revision_rows);
            stage(file, [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                refuse<core::ModifierActivationFailure>(Domain::invalid_factor, [&] {
                    (void)store.append_definition(
                        {"D",
                         "bad-factor",
                         1,
                         {{core::ModifierTargetKind::added_time, "timer.main", {}, "-0.01"}},
                         core::ModifierReactivation::extend},
                        "a-new");
                });
                refuse<core::ModifierActivationFailure>(Domain::missing_trigger_timing, [&] {
                    (void)store.activate(
                        "D", "z-old", old_live,
                        {"L", "l0", rule,
                         core::ModifierTriggeringContribution{
                             "event-C", core::ModifierTriggerTiming::unspecified}});
                });
                refuse<SqliteModifierStorageFailure>(Reason::stale_definition, [&] {
                    (void)store.activate("D", "z-old", old_live, {"L", "l0", rule, subsequent});
                });
                refuse<SqliteModifierStorageFailure>(Reason::identity_conflict, [&] {
                    (void)store.append_definition(
                        {"E", "z-old", 1, {}, core::ModifierReactivation::extend}, {});
                });
                refuse<SqliteModifierStorageFailure>(Reason::identity_conflict, [&] {
                    (void)store.activate("D", "a-new", new_live, {"I", "l0", moderator, {}});
                });
                refuse<SqliteModifierInputFailure>(Input::invalid_input, [&] {
                    (void)store.checkpoint_remaining(new_live, "k2", -1);
                });
                refuse<SqliteModifierInputFailure>(Input::invalid_input, [&] {
                    (void)store.checkpoint_remaining(new_live, "k2", 600001);
                });
                refuse<SqliteModifierStorageFailure>(Reason::identity_conflict, [&] {
                    (void)store.checkpoint_remaining(new_live, "j1", 600000);
                });
                require(store.append_definition(independent_definition, {}) ==
                            SqliteModifierDefinitionRevision{independent_definition, 1},
                        "Caller can still stage and commit after prewrite refusals");
            });
            require(file.reads(revision_rows) == before,
                    "Prewrite refusals add no instance revision");
        });
    }

    void selectedCorruptionSettlesTheUnit() {
        check([] {
            ModifierFile file;
            stage(file, seed_old_live);
            // The stored count disagrees with the selected ordered targets.
            stage(file, [](const auto &s) {
                (void)s.execute({"DELETE FROM main.subathon_modifier_targets WHERE ordinal=2", {}});
            });
            const auto outcome = failed_work(file, [](auto context) -> QVariant {
                const auto s = context.begin();
                (void)s.execute({"INSERT INTO main.subathon_modifier_definition_revisions "
                                 "VALUES(X'6530',X'45',1,1,0,0)",
                                 {}});
                refuse<SqliteModifierStorageFailure>(Reason::invalid_stored_data, [&] {
                    (void)SqliteModifierActivationStore(s).definition_revision("z-old");
                });
                return 47;
            });
            if (outcome.cause)
                std::rethrow_exception(outcome.cause);
            rolled_back(outcome.result, WorkError::not_committed);
            reopen(file);
            require(
                file.reads("SELECT revision_ref FROM main.subathon_modifier_definition_revisions "
                           "ORDER BY revision_ref") == SqlRows{{QByteArray("z-old")}},
                "Catching selected corruption has already discarded caller staging");
        });
    }

    void extendOverflowSettlesTheOriginatingUnit_data() {
        QTest::addColumn<int>("finish");
        QTest::newRow("typed-escape") << 0;
        QTest::newRow("caught-return") << 1;
        QTest::newRow("caught-terminal-commit-escape") << 2;
        QTest::newRow("caught-terminal-commit-caught") << 3;
    }

    void extendOverflowSettlesTheOriginatingUnit() {
        QFETCH(int, finish);
        check([&] {
            ModifierFile file;
            seed_replaced_with_original(file);
            const auto revisions = file.reads(revision_rows);
            const auto lifetimes = file.reads(instance_rows);
            const auto outcome = failed_work(file, [finish](auto context) -> QVariant {
                const auto s = context.begin();
                const SqliteModifierActivationStore store(s);
                const SqliteApplicationPlanStore plans(s);
                // 600000 + 9223372036854175808 = INT64_MAX + 1.
                const core::ModifierDefinition overflow{
                    "D", "overflow", 9223372036854175808LL, {}, core::ModifierReactivation::extend};
                const SqliteModifierDefinitionRevision staged{overflow, 3};
                require(store.append_definition(overflow, "a-new") == staged &&
                            store.definition_revision("overflow") == staged &&
                            store.current_definition("D") == staged,
                        "Exact definition revision and head are staged before overflow");
                require(plans.retain_initial("plan-B", plan_b) ==
                                SqlitePlanInitialDisposition::inserted &&
                            plans.find_revision("plan-B", 1) ==
                                SqlitePlanRevisionSnapshot{"plan-B", plan_b},
                        "Unrelated receipt-8 plan is staged and selected before overflow");
                try {
                    (void)store.activate("D", "overflow", new_live,
                                         {"P",
                                          "k-overflow",
                                          {core::ModifierActivationKind::moderator, "operator-C"},
                                          {}});
                } catch (const core::ModifierActivationFailure &error) {
                    require(error.code() == Domain::duration_overflow,
                            "Extend refuses exact overflow");
                    if (finish == 0)
                        throw;
                    if (finish >= 2) {
                        try {
                            s.commit();
                        } catch (const SqliteWorkFailure &terminal) {
                            require(terminal.error() == WorkError::session_closed,
                                    "Commit of cleaned session is terminal misuse");
                            if (finish == 2)
                                throw;
                            return 47;
                        }
                        throw std::runtime_error("Terminal commit must throw");
                    }
                    return 47;
                }
                throw std::runtime_error("Expected Extend duration overflow");
            });
            if (outcome.cause) {
                if (finish == 0) {
                    refuse<core::ModifierActivationFailure>(
                        Domain::duration_overflow, [&] { std::rethrow_exception(outcome.cause); });
                } else if (finish == 2) {
                    try {
                        std::rethrow_exception(outcome.cause);
                    } catch (const SqliteWorkFailure &error) {
                        require(error.error() == WorkError::session_closed,
                                "Escaped terminal commit carries the exact sticky error");
                    }
                } else {
                    std::rethrow_exception(outcome.cause);
                }
            }
            require(bool(outcome.cause) == (finish == 0 || finish == 2),
                    "Only the selected escape modes propagate an exception");
            rolled_back(outcome.result, finish == 0   ? WorkError::work_exception
                                        : finish == 1 ? WorkError::not_committed
                                                      : WorkError::session_closed);
            require_original_after_reopen(file);
            stage(file, [](const auto &s) {
                require_replaced(s);
                const SqliteModifierActivationStore store(s);
                require(!store.definition_revision("overflow") &&
                            !store.instance_revision("k-overflow") &&
                            !SqliteApplicationPlanStore(s).find_revision("plan-B", 1),
                        "Whole unit lost all staging and retained committed receipt-8 source");
            });
            require(file.reads(revision_rows) == revisions &&
                        file.reads(instance_rows) == lifetimes,
                    "Independent instance rows were not advanced or rewritten");
        });
    }

    void extendAtSignedMaxKeepsCaptureAndRecordsAppliedDefinition() {
        check([] {
            ModifierFile file;
            seed_replaced_with_original(file);
            const core::ModifierActivationSource source{core::ModifierActivationKind::moderator,
                                                        "operator-C"};
            const core::ModifierInstance maximum{
                "J", "k-max", "k0", new_definition, 9223372036854775807LL, source, {}};
            stage(file, [&](const auto &s) {
                const SqliteModifierActivationStore store(s);
                const core::ModifierDefinition boundary{
                    "D", "boundary", 9223372036854175807LL, {}, core::ModifierReactivation::extend};
                require(store.append_definition(boundary, "a-new") ==
                            SqliteModifierDefinitionRevision{boundary, 3},
                        "Boundary definition appends at position 3");
                const auto proposal =
                    store.activate("D", "boundary", new_live, {"P", "k-max", source, {}});
                require(proposal.revision_to_append == maximum &&
                            !proposal.superseded_instance_ref && !proposal.for_trigger,
                        "Exact maximum extends J with original capture and incoming source");
            });
            require_original_after_reopen(file);
            stage(file, [&](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.live_instance("D") ==
                                SqliteModifierInstanceRevision{maximum,
                                                               2,
                                                               Operation::activation,
                                                               "boundary",
                                                               Lifetime::live,
                                                               {}} &&
                            store.instance_revision("k0") ==
                                SqliteModifierInstanceRevision{
                                    k0, 1, Operation::activation, "a-new", Lifetime::live, {}},
                        "Durable position, parent, capture and incoming applied revision agree");
            });
            require(file.reads("SELECT instance_ref FROM main.subathon_modifier_instances "
                               "ORDER BY instance_ref") ==
                        SqlRows{{QByteArray("I")}, {QByteArray("J")}},
                    "Extend never consumes the proposed fresh instance ref P");
        });
    }

    void laterSqlFaultRollsBackAnActuallyStagedReplacement() {
        check([] {
            ModifierFile file;
            seed_replaced_with_original(file);
            const auto revisions = file.reads(revision_rows);
            const auto lifetimes = file.reads(instance_rows);
            const auto outcome = failed_work(file, [](auto context) -> QVariant {
                const auto s = context.begin();
                const SqliteModifierActivationStore store(s);
                const core::ModifierDefinition definition{
                    "D", "replace-fault", 900000, {}, core::ModifierReactivation::replace};
                const core::ModifierActivationSource schedule{
                    core::ModifierActivationKind::schedule, "schedule-C"};
                const core::ModifierInstance m0{"K", "m0", {}, definition, 900000, schedule, {}};
                require(store.append_definition(definition, "a-new") ==
                            SqliteModifierDefinitionRevision{definition, 3},
                        "New definition is staged before SQL fault");
                const auto proposal =
                    store.activate("D", "replace-fault", new_live, {"K", "m0", schedule, {}});
                require(proposal.revision_to_append == m0 &&
                            proposal.superseded_instance_ref == "J" && !proposal.for_trigger &&
                            store.instance_revision("m0") ==
                                SqliteModifierInstanceRevision{m0,
                                                               1,
                                                               Operation::activation,
                                                               "replace-fault",
                                                               Lifetime::live,
                                                               {}} &&
                            store.instance_revision("k0") ==
                                SqliteModifierInstanceRevision{k0, 1, Operation::activation,
                                                               "a-new", Lifetime::replaced, "K"},
                        "Writer sees complete provisional Replace before injecting duplicate PK");
                (void)s.execute({"INSERT INTO main.subathon_modifier_instances"
                                 "(instance_ref,definition_ref,lifetime,successor_instance_ref) "
                                 "VALUES(?,?,0,NULL)",
                                 {QByteArray("I"), QByteArray("fault-lane")}});
                s.commit();
                return 47;
            });
            if (outcome.cause) {
                try {
                    std::rethrow_exception(outcome.cause);
                } catch (const SqliteWorkFailure &error) {
                    require(error.error() == WorkError::sql_failure,
                            "First error is the actual SQL fault");
                }
            }
            rolled_back(outcome.result, WorkError::sql_failure);
            const auto durable = [](const auto &s) {
                require_replaced(s);
                const SqliteModifierActivationStore store(s);
                require(!store.instance_revision("m0") &&
                            !store.definition_revision("replace-fault"),
                        "Neither provisional revision survived");
                require_original(s);
            };
            stage(file, durable);
            reopen(file);
            stage(file, durable);
            require_original_rows(file);
            require(file.reads(revision_rows) == revisions &&
                        file.reads(instance_rows) == lifetimes,
                    "Raw rows retain I to J; K and every provisional row are gone");
        });
    }

    void additiveManifestPreservesExactPrefixAndPopulatedUpgrade() {
        check([] {
            DecisionFile upgraded;
            seed_held_group(upgraded);
            const auto shipped = milestone_group_decisions_repository_manifest();
            const auto literal = literal_modifier_manifest();
            const auto before = legacy_rows(upgraded);
            const auto prefix_rows = upgraded.reads(history_sql);
            require(shipped.steps.size() == 13 &&
                        shipped.supported_starting_versions ==
                            QList<qint32>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13} &&
                        prefix_rows == expected_history(13) &&
                        upgraded.reads(modifiers_schema_sql).empty(),
                    "Shipped v13 stays pinned and a populated v13 file has no modifier objects");

            const auto actual = modifier_activations_repository_manifest();
            require(actual.catalog_format == literal.catalog_format &&
                        actual.store_identity == literal.store_identity &&
                        actual.supported_starting_versions ==
                            QList<qint32>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14} &&
                        actual.steps.size() == 14,
                    "Only the modifier manifest adds version 14 and a supported start");
            const auto compare = [](const MigrationStep &candidate, const MigrationStep &expected) {
                require(candidate.version == expected.version && candidate.id == expected.id &&
                            candidate.sql == expected.sql &&
                            candidate.expected_schema == expected.expected_schema &&
                            literal_digest(expected) == pinned_digest(expected.version) &&
                            migration_content_sha256(candidate) == pinned_digest(expected.version),
                        QString("Exact step %1 identity, ordered SQL, schema and digest")
                            .arg(expected.version));
            };
            for (qsizetype i = 0; i < actual.steps.size(); ++i) {
                compare(actual.steps[i], literal.steps[i]);
                if (i < shipped.steps.size())
                    compare(shipped.steps[i], literal.steps[i]);
            }

            File fresh(File::Schema::empty);
            require(fresh.reads("SELECT name FROM main.sqlite_schema").empty(),
                    "Opening the real writer performs no migration");
            const auto inspected =
                permission_graph_fixture::migrate(*upgraded.writer, actual, MigrationMode::inspect);
            normal(inspected.work);
            require(inspected.reason == MigrationReason::none && inspected.report &&
                        inspected.report->state == CatalogState::upgrade_required &&
                        inspected.report->version == 13 && inspected.report->target_version == 14 &&
                        inspected.report->applied == 0 &&
                        upgraded.reads(history_sql) == prefix_rows &&
                        upgraded.reads(modifiers_schema_sql).empty(),
                    "v13 inspection reports 13/14/0 and leaves the database untouched");

            migrated(permission_graph_fixture::migrate(*fresh.writer, actual), 14, 14);
            migrated(permission_graph_fixture::migrate(*upgraded.writer, actual), 14, 1);
            for (auto *file : {&fresh, static_cast<File *>(&upgraded)}) {
                reopen(*file);
                migrated(permission_graph_fixture::migrate(*file->writer, actual,
                                                           MigrationMode::inspect),
                         14, 0);
                migrated(permission_graph_fixture::migrate(*file->writer, actual), 14, 0);
                require(file->reads(history_sql) == expected_history(14),
                        "Fresh and upgraded catalogs have exact independently framed history");
                require_schema(*file, literal);
                require_empty_modifier_tables(*file);
            }
            require(legacy_rows(upgraded) == before,
                    "Only the additive migration changes durable rows");
        });
    }

    void cancellingAfterV14DdlKeepsV13AndEveryPriorRow() {
        check([] {
            DecisionFile file;
            seed_held_group(file);
            const auto before = legacy_rows(file);
            const auto history = file.reads(history_sql);
            const auto reached = std::make_shared<int>(0);
            const auto result = migrate_request(
                file, {MigrationMode::advance,
                       modifier_activations_repository_manifest(),
                       {},
                       [reached](const auto &at) {
                           if (at.checkpoint != MigrationCheckpoint::before_history ||
                               at.version != 14 || at.statement != 5)
                               return false;
                           ++*reached;
                           return true;
                       }});
            rolled_back(result.work, WorkError::work_exception);
            require(*reached == 1 && result.reason == MigrationReason::cancelled && !result.report,
                    "Cancellation happens after all five DDL statements and before v14 history");
            reopen(file);
            require(history == expected_history(13) && file.reads(history_sql) == history &&
                        file.reads(modifiers_schema_sql).empty() && legacy_rows(file) == before,
                    "Cancellation is atomic across schema, history and populated v13 rows");
        });
    }

    void tamperedV13DigestRefusesTheAdditiveCatalog() {
        check([] {
            DecisionFile file;
            stage(file, [](const auto &s) {
                (void)s.execute({"UPDATE main.subathon_migration_history SET content_sha256=? "
                                 "WHERE version=13",
                                 {QByteArray(32, '\x01')}});
            });
            const auto history = file.reads(history_sql);
            const auto result =
                permission_graph_fixture::migrate(*file.writer, literal_modifier_manifest());
            rolled_back(result.work, WorkError::work_exception);
            require(result.reason == MigrationReason::catalog_mismatch && !result.report,
                    "Tampered shipped digest cannot advance into v14");
            reopen(file);
            require(file.reads(history_sql) == history && file.reads(modifiers_schema_sql).empty(),
                    "A rejected prefix is never repaired or advanced implicitly");
        });
    }

    void missingV14SchemaFailsEvenOnUnknownSelection() {
        check([] {
            ModifierFile file;
            stage(file, [](const auto &s) {
                (void)s.execute({"DROP TABLE main.subathon_modifier_targets", {}});
            });
            const auto failure = failed_work(file, [](auto context) -> QVariant {
                const auto s = context.begin();
                (void)SqliteModifierActivationStore(s).definition_revision("missing-rev");
                s.commit();
                return 47;
            });
            if (failure.cause) {
                try {
                    std::rethrow_exception(failure.cause);
                } catch (const SqliteWorkFailure &error) {
                    require(error.error() == WorkError::sql_failure,
                            "Missing schema uses the existing writer SQL classification");
                }
            }
            rolled_back(failure.result, WorkError::sql_failure);
        });
    }

    // These three compatibility witnesses use the published owner APIs and
    // literal v14, so their first red comes from the shipped projection.
    void literalV14HeldHeadRefusesRelease() {
        check([] {
            DecisionFile file;
            literal_v14_from_held_v13(file);
            const auto before = legacy_rows(file);
            const auto reply = completed<MilestoneReleaseCompletion>([&](auto callback) {
                return release_milestone_head(*file.writer, {"z-first"}, std::move(callback));
            });
            require(legacy_rows(file) == before,
                    "Held release appends no release, delivery or effect");
            if (reply.work.error == WorkError::work_exception)
                require(!reply.disposition && !reply.work.value,
                        "V14 projection exception has no release result");
            normal(reply.work);
            require(reply.disposition == MilestoneReleaseDisposition::refused,
                    "Recognized v14 honors the v13 hold at the exact queue head");
        });
    }

    void literalV14GroupedJobRefusesBeforeAuthority() {
        check([] {
            DecisionFile file;
            literal_v14_from_held_v13(file);
            const auto before = legacy_rows(file);
            const auto fake = std::make_shared<FakeDelivery>(file);
            const auto reply = dispatch(file, "job", fake);
            require(fake->authorized.empty() && fake->starts.empty() && !reply.adapter_threw &&
                        legacy_rows(file) == before,
                    "Grouped held job cannot reach authority, adapter or durable writes");
            if (reply.work.error == WorkError::work_exception)
                require(!reply.result && !reply.work.value,
                        "V14 projection exception has no claim");
            normal(reply.work);
            require(reply.result && reply.result->disposition == DeliveryDisposition::refused &&
                        !reply.result->job,
                    "Recognized v14 keeps the grouped job queued under its hold");
        });
    }

    void literalV14IndependentJobClaimsAndStartsOnlyAfterCommit() {
        check([] {
            DecisionFile file;
            literal_v14_from_held_v13(file);
            const auto before = legacy_rows(file);
            const auto fake = std::make_shared<FakeDelivery>(file);
            const auto reply = dispatch(file, "legacy", fake);
            if (reply.work.error == WorkError::work_exception)
                require(!reply.result && !reply.work.value && !reply.adapter_threw &&
                            fake->authorized.empty() && fake->starts.empty() &&
                            legacy_rows(file) == before,
                        "V14 projection exception precedes authority, claim and adapter");
            normal(reply.work);
            require(reply.result && reply.result->disposition == DeliveryDisposition::changed &&
                        reply.result->job && !reply.adapter_threw && fake->authorized.size() == 1 &&
                        fake->starts.size() == 1,
                    "Exact independent job is authorized once and starts after normal commit");
            const auto selected = [](const DeliveryJob &job) {
                return job.key.operation.operation_scope == "svc" &&
                       job.key.operation.operation_ref == "add10" && job.key.job_ref == "legacy";
            };
            const auto sending = [&](const DeliveryJob &job) {
                return selected(job) && job.state == DeliveryJobState::sending &&
                       job.revision == 2 && job.attempt == 2 && !job.evidence_ref;
            };
            require(selected(fake->authorized.front()) && fake->authorized.front().revision == 1 &&
                        fake->authorized.front().state == DeliveryJobState::queued &&
                        !fake->authorized.front().attempt && sending(*reply.result->job) &&
                        sending(fake->starts.front()) &&
                        fake->events_at_start == SqlRows{{1, 2, 2}},
                    "Authority sees revision 1 and the adapter sees durable sending "
                    "revision/attempt 2");
            reopen(file);
            auto expected = before;
            // In job-ref order the sending event follows queued legacy and precedes stranded.
            expected.events.insert(3, {QByteArray("svc"), QByteArray("add10"), QByteArray("legacy"),
                                       2, 1, 2, 1, QString("")});
            require(legacy_rows(file) == expected,
                    "Reopen shows exactly one independent sending event and no other changes");
        });
    }

    void unrecognizedMarkerPreservesFailClosedDispatch() {
        check([] {
            DecisionFile file;
            literal_v14_from_held_v13(file);
            stage(file, [](const auto &s) {
                (void)s.execute({"UPDATE main.subathon_migration_history SET migration_id=? "
                                 "WHERE version=14",
                                 {QString("unrecognized-v14")}});
            });
            reopen(file);
            const auto before = legacy_rows(file);
            const auto fake = std::make_shared<FakeDelivery>(file);
            const auto reply = dispatch(file, "legacy", fake);
            rolled_back(reply.work, WorkError::work_exception);
            require(!reply.result && !reply.adapter_threw && fake->authorized.empty() &&
                        fake->starts.empty() && legacy_rows(file) == before,
                    "An unrecognized v14 ID does not fall through to independent dispatch");
        });
    }
};

} // namespace

QTEST_GUILESS_MAIN(SqliteModifierActivationsTests)
#include "sqlite_modifier_activations_tests.moc"
