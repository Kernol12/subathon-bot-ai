// SPDX-License-Identifier: GPL-3.0-or-later
#include "constraint_specimen.hpp"

namespace {
using namespace storage_003_fixture;
using Initial = ConstraintSpecimen::Initial;
using Mutation = ConstraintSpecimen::Mutation;
using detail::ClockPredecessor;

inline constexpr auto select_slots =
    "SELECT singleton,base_token,configuration_ref,format_version,generation,revision,root_ordinal,"
    "pending_revision,pending_ordinal,main_ms,grace_ms,cadence_ms,ledger_revision,document "
    "FROM subathon_clock_checkpoint ORDER BY singleton";
inline constexpr auto select_roots =
    "SELECT "
    "revision,ordinal,kind,installation_ref,timer_ref,lineage_ref,base_token,configuration_ref,"
    "ledger_revision,operation_scope,operation_ref,episode_ref,schedule_id,nominal_ms,fold_run,"
    "member_count,link_revision,link_ordinal,page_no,format_version,document "
    "FROM subathon_clock_evidence ORDER BY revision,ordinal";
inline constexpr auto read_join =
    "SELECT h.token,s.base_token,s.configuration_ref,s.format_version,s.generation,s.revision,"
    "s.root_ordinal,s.pending_revision,s.pending_ordinal,s.main_ms,s.grace_ms,s.cadence_ms,"
    "s.ledger_revision,s.document,e.kind,e.member_count,e.operation_scope,e.operation_ref,"
    "e.base_token,e.configuration_ref,e.document "
    "FROM subathon_clock_checkpoint s "
    "LEFT JOIN subathon_app_head h ON h.singleton=s.singleton "
    "AND length(h.token) BETWEEN 1 AND 4096 "
    "JOIN subathon_clock_evidence e ON e.revision=s.revision AND e.ordinal=s.root_ordinal "
    "WHERE s.singleton=1 LIMIT 2";

SqlRows read(SqliteReader &reader, const QString &sql) {
    const auto result = reader.read({sql, {}}, 2);
    require(result.error == StorageError::none, "Independent selected reader succeeds");
    return result.rows;
}

void expect_seed(const DisposableFile &file) {
    SqliteReader reader;
    require(reader.open(file.path()) == StorageError::none, "Independent reader opens seed");
    expect_rows(read(reader, select_head), initial_head, "Committed seed singleton head");
    expect_rows(read(reader, select_state), {active_state}, "Committed immutable seed A");
    expect_rows(read(reader, select_configuration), initial_configuration, "Committed seed K");
    expect_rows(read(reader, select_slots), {seed_slot}, "Exact committed slot columns and bytes");
    expect_rows(read(reader, select_roots), {seed_evidence},
                "Exact committed root and genuine NULLs");
    expect_rows(read(reader, read_join), {seed_join}, "Independent joined head, slot and root");
}

void advance_to_22(const DisposableFile &file, const detail::ClockPreOpenWitness &expected,
                   const Charge &charge) {
    const auto manifest = compiled_successor();
    const auto witness = candidate(file, expected);
    DisposableFile backup;
    file.copy_to(backup);
    SqliteWriter owner;
    require(owner.open(file.path(), profile(charge)) == StorageError::none,
            "Normal owner opens after read-only admission and matching backup");
    const auto result = advance(owner, request(MigrationMode::advance, manifest), witness);
    require(result.refusal == detail::ClockAdvanceRefusal::none, "Unchanged basis admitted");
    expect_current(result.migration, 22, 22 - expected.version);
}

void advance_blank(const DisposableFile &file) {
    file.create_blank();
    advance_to_22(file, {0, ClockPredecessor::blank, {}, {}}, blank_advance);
}

void install_seed(const DisposableFile &file) {
    using namespace subathon::core;
    {
        SqliteReader reader;
        require(reader.open(file.path()) == StorageError::none, "Read prior initial basis");
        expect_rows(read(reader, select_head), {}, "Initial operation has no prior head");
        expect_rows(read(reader, select_slots), {}, "Initial operation has no prior slot");
        expect_rows(read(reader, select_roots), {}, "Initial operation has no prior root");
    }
    SqliteWriter owner;
    require(owner.open(file.path(), setup_profile) == StorageError::none,
            "Normal writer owns the v22 work item");
    const auto stop = std::make_shared<WorkError>(WorkError::none);
    const auto result = qt_test_support::run_work(
        owner,
        [stop](auto context) {
            const auto session = context.begin();
            const ApplicationStateSnapshot snapshot{
                "A", {"E", "active"},
                "K", {TimerLifecycle::Active, CountdownPhase::Main, 10000, 0, false},
                0,   {}};
            const auto staged = detail::stage_clock_checkpoint_initial(
                session, {{RetainedKind::configuration, "K"}, "configuration", {}}, snapshot,
                {"I", "T", "L", {"S", "J"}, 1000, 0, slot_document, seed_root});
            if (const auto *failure = std::get_if<WorkError>(&staged)) {
                *stop = *failure;
                session.rollback();
                return 0;
            }
            require(std::get<SqliteBundleInitialDisposition>(staged) ==
                        SqliteBundleInitialDisposition::inserted,
                    "Initial stage commits real root and slot before head");
            session.commit();
            return 1;
        },
        10000);
    require(*stop == WorkError::none,
            QString("First stop: private initial checkpoint stage, work reason %1")
                .arg(static_cast<int>(*stop)));
    expect_commit(result);
}

} // namespace

class Storage003WriterTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void w1NormalInitialOwner() {
        check([] {
            DisposableFile file;
            advance_blank(file);
            install_seed(file);
            expect_seed(file);
        });
    }

    void w1JoinedReadBudget() {
        check([] {
            DisposableFile file;
            advance_blank(file);
            install_seed(file);
            for (const qsizetype available : {joined_bytes, joined_bytes - 1}) {
                auto limits = setup_profile;
                limits.result_rows = 1;
                limits.result_value_bytes = available;
                SqliteWriter writer;
                require(writer.open(file.path(), limits) == StorageError::none,
                        "Owner opens with the exact joined read limit");
                const auto result = qt_test_support::run_work(
                    writer,
                    [](auto context) {
                        const auto session = context.begin();
                        const auto rows = session.execute({read_join, {}});
                        expect_rows(rows, {seed_join}, "One exact 21-cell structural row");
                        session.commit();
                        return 1;
                    },
                    10000);
                if (available == joined_bytes)
                    expect_commit(result);
                else
                    require(result.transaction == TransactionOutcome::rolled_back &&
                                result.error == WorkError::result_limit && !result.value,
                            "One less than 298 value bytes refuses the real joined read");
            }
            expect_seed(file);
        });
    }

    void w1MissingHeadPhysical() {
        check([] {
            DisposableFile file;
            advance_blank(file);
            {
                ConstraintSpecimen specimen(file);
                specimen.begin();
                specimen.insert(Initial::configuration);
                specimen.insert(Initial::state);
                specimen.commit();

                specimen.begin();
                specimen.reject_head_first();
                specimen.rollback();

                specimen.begin();
                specimen.insert(Initial::root);
                specimen.insert(Initial::slot);
                specimen.require_missing_head_at_commit();
            }
            SqliteReader reader;
            require(reader.open(file.path()) == StorageError::none, "Independent reopen");
            expect_rows(read(reader, select_state), {active_state}, "Negative specimen retained A");
            expect_rows(read(reader, select_configuration), initial_configuration,
                        "Negative specimen retained K");
            expect_rows(read(reader, select_head), {}, "Missing head stays absent");
            expect_rows(read(reader, select_slots), {}, "Real rollback removes the staged slot");
            expect_rows(read(reader, select_roots), {}, "Real rollback removes the staged root");
        });
    }

    void w1OccupiedSingletonPhysical() {
        check([] {
            DisposableFile owner_file;
            advance_blank(owner_file);
            install_seed(owner_file);
            DisposableFile file;
            owner_file.copy_to(file);

            static constexpr std::pair<Mutation, const char *> controls[]{
                {Mutation::insert_head, "checkpoint head insert"},
                {Mutation::replace_head, "checkpoint head insert"},
                {Mutation::upsert_head, "checkpoint head insert"},
                {Mutation::delete_head, "checkpoint head delete"},
                {Mutation::insert_slot, "checkpoint slot insert"},
                {Mutation::replace_slot, "checkpoint slot insert"},
                {Mutation::upsert_slot, "checkpoint slot insert"},
                {Mutation::delete_slot, "checkpoint slot delete"}};
            {
                ConstraintSpecimen specimen(file);
                for (const auto &[control, message] : controls) {
                    specimen.begin();
                    specimen.reject(control, QString::fromLatin1(message));
                    specimen.rollback();
                }
            }
            expect_seed(file);
        });
    }

    void w1SlotlessHeadUpdatePhysical() {
        check([] {
            ProtectedPredecessor predecessor(true);
            advance_to_22(predecessor.file, {21, ClockPredecessor::ready, "A", "K"}, ready_advance);
            DisposableFile copy;
            predecessor.file.copy_to(copy);
            {
                ConstraintSpecimen specimen(copy);
                specimen.begin();
                specimen.reject(Mutation::update_slotless_head,
                                "checkpoint head update without slot");
                specimen.rollback();
            }
            SqliteReader reader;
            require(reader.open(copy.path()) == StorageError::none,
                    "Independently reopen the copy");
            expect_rows(read(reader, select_head), initial_head, "Original head remains");
            expect_rows(read(reader, select_state), {ready_state}, "Original Ready row remains");
            expect_rows(read(reader, select_slots), {}, "No invented slot");
            expect_rows(read(reader, select_roots), {}, "No invented evidence");
        });
    }

    void w1UnreferencedEvidencePhysical() {
        check([] {
            DisposableFile owner_file;
            advance_blank(owner_file);
            install_seed(owner_file);
            DisposableFile file;
            owner_file.copy_to(file);
            {
                ConstraintSpecimen specimen(file);
                for (auto mutation : {Mutation::update_evidence, Mutation::delete_evidence,
                                      Mutation::replace_evidence}) {
                    specimen.begin();
                    specimen.insert(Initial::unreferenced_root);
                    specimen.permit_unreferenced_change(mutation);
                    specimen.rollback();
                }
            }
            expect_seed(file);
        });
    }

    void w1RuntimeRecipeDenials() {
        check([] {
            DisposableFile source;
            advance_blank(source);
            install_seed(source);
            DisposableFile file;
            source.copy_to(file);
            {
                SqliteWriter writer;
                require(writer.open(file.path(), setup_profile) == StorageError::none,
                        "Real governed writer opens the disposable copy");
                const auto trace_before_select = detail::clock_checkpoint_fixture_trace(writer);
                expect_commit(qt_test_support::run_work(
                    writer,
                    [](auto context) {
                        const auto session = context.begin();
                        expect_rows(session.execute({empty_additions, {}}), {{integer(0)}},
                                    "Fixed nonmutating selector observes the occupied relations");
                        session.commit();
                        return 1;
                    },
                    10000));
                const auto trace_before_batch = detail::clock_checkpoint_fixture_trace(writer);
                auto batch_callback = std::make_shared<bool>(false);
                const SqlStatement occupied_head{"INSERT INTO subathon_app_head VALUES(1,X'41')",
                                                 {}};
                require(writer.enqueue({occupied_head},
                                       [batch_callback](auto) { *batch_callback = true; }) ==
                                StorageError::invalid_batch &&
                            !*batch_callback,
                        "Governed head batch is rejected before work admission");
                const auto trace_after_batch = detail::clock_checkpoint_fixture_trace(writer);
                QList<std::optional<detail::ClockFixtureWriterTrace>> denial_traces;
                const QList<SqlStatement> controls{
                    occupied_head,
                    {"REPLACE INTO subathon_app_head VALUES(1,X'41')", {}},
                    {"INSERT INTO subathon_app_head VALUES(1,X'41') ON CONFLICT(singleton) "
                     "DO UPDATE SET token=excluded.token",
                     {}},
                    {"DELETE FROM subathon_app_head WHERE singleton=1", {}},
                    {"INSERT INTO subathon_clock_checkpoint VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                     seed_slot},
                    {"REPLACE INTO subathon_clock_checkpoint VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                     seed_slot},
                    {"INSERT INTO subathon_clock_checkpoint VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
                     "ON CONFLICT(singleton) DO UPDATE SET revision=excluded.revision",
                     seed_slot},
                    {"DELETE FROM subathon_clock_checkpoint WHERE singleton=1", {}},
                    {"UPDATE subathon_app_states SET main_remaining_ms=9999 WHERE token=X'41'", {}},
                    {"UPDATE subathon_clock_evidence SET ledger_revision=1 WHERE revision=1", {}},
                    {"DELETE FROM subathon_clock_evidence WHERE revision=1", {}},
                    {"REPLACE INTO subathon_clock_evidence "
                     "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                     seed_evidence},
                    {"CREATE INDEX subathon_test_v22_drift ON subathon_clock_evidence(kind)", {}}};
                auto deny = [&](const SqlStatement &statement, const QString &witness) {
                    const auto result = qt_test_support::run_work(
                        writer,
                        [statement](auto context) {
                            const auto session = context.begin();
                            (void)session.execute(statement);
                            session.rollback();
                            return 0;
                        },
                        10000);
                    require(result.transaction == TransactionOutcome::rolled_back &&
                                result.error == WorkError::invalid_statement && !result.value,
                            QString("%1, work reason %2 for %3")
                                .arg(witness)
                                .arg(static_cast<int>(result.error))
                                .arg(statement.sql));
                    denial_traces.append(detail::clock_checkpoint_fixture_trace(writer));
                };
                for (const auto &statement : controls)
                    deny(statement, "Governed write recipe");
                // These already refuse at the protected pin and earn no new red.
                deny({"PRAGMA foreign_keys=OFF", {}}, "Protected writable PRAGMA regression");
                deny({"UNRECOGNIZED STORAGE_003", {}}, "Protected unsupported verb regression");
                require(trace_before_select && trace_before_batch && trace_after_batch,
                        "Pending actual writer prepare/BEGIN tracing in the fixture owner variant");
                require(trace_before_batch->prepares - trace_before_select->prepares == 1 &&
                            trace_before_batch->begins - trace_before_select->begins == 1,
                        "Positive SELECT reaches one prepare and one real BEGIN on this writer");
                require(trace_after_batch->prepares == trace_before_batch->prepares &&
                            trace_after_batch->begins == trace_before_batch->begins,
                        "Rejected batch reaches neither prepare nor BEGIN");
                auto prior = *trace_after_batch;
                for (const auto &after : denial_traces) {
                    require(after && after->prepares == prior.prepares &&
                                after->begins - prior.begins == 1,
                            "Denied work begins once and reaches zero prepares");
                    prior = *after;
                }
            }
            expect_seed(file);
        });
    }
};

QTEST_GUILESS_MAIN(Storage003WriterTests)
#include "writer_tests.moc"
