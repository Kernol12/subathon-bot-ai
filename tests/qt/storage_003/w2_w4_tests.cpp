// SPDX-License-Identifier: GPL-3.0-or-later
// #834 W2–W4: real COMMIT, deferred rollback, uncertain completion and
// independent reopened closure for operation S/Q on a typed SAS1 seed.
#include "w2_w4_observer.hpp"

#include <subathon/server/application_service.hpp>
#include <subathon/server/receipt_intake.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace storage_003_w2_w4 {
namespace {
using D = ApplicationServiceDisposition;
using Transaction = TransactionOutcome;
using detail::ClockFixtureArm;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;

// One governed unit on its own writer: open, run, require acknowledged COMMIT.
template <typename Body> void commit_unit(const DisposableFile &file, const char *unit, Body body) {
    SqliteWriter writer;
    require(writer.open(file.path(), unit_writer_limits) == StorageError::none,
            QString("%1: writer opens the authentic v22 store").arg(unit));
    const auto result = qt_test_support::run_work(writer, body, timeout_ms);
    require(result.transaction == Transaction::committed && result.error == WorkError::none &&
                result.value.has_value(),
            QString("%1: acknowledged COMMIT, work reason %2")
                .arg(unit)
                .arg(static_cast<int>(result.error)));
}

// Blank v22 store, then the SAS1 initial unit and the three source units,
// each committed by its own shipped owner (correction 01, item 1).
void seed(const DisposableFile &file) {
    file.create_blank();
    {
        const auto witness = candidate(file, {0, detail::ClockPredecessor::blank, {}, {}});
        DisposableFile backup;
        file.copy_to(backup);
        SqliteWriter migrator;
        require(migrator.open(file.path(), profile(blank_advance)) == StorageError::none,
                "Migration writer opens the blank disposable file");
        expect_guarded_v22_advance(migrator, compiled_successor(), witness);
    }
    commit_unit(file, "Initial SAS1 unit", [](SqliteWorkContext context) -> QVariant {
        const auto session = context.begin();
        const auto staged = detail::stage_clock_checkpoint_initial(
            session, configuration, initial_state, initial_checkpoint);
        const auto *disposition = std::get_if<SqliteBundleInitialDisposition>(&staged);
        require(disposition && *disposition == SqliteBundleInitialDisposition::inserted,
                "Initial owner stages K, A, J root, slot and head");
        session.commit();
        return 1;
    });
    commit_unit(file, "Context and receipt unit", [](SqliteWorkContext context) -> QVariant {
        const auto session = context.begin();
        require(SqliteRetainedContextStore(session, context_limits).retain(arrival_context) ==
                    RetainDisposition::inserted,
                "Arrival context X is staged on the borrowed session");
        SqliteReceiptRepository receipts(adopt_active_receipt_session, session, receipt_limits);
        const auto created = ReceiptIntake(receipts).ingest(receipt_input);
        require(created.receipt_id == receipt_id &&
                    created.disposition == core::ReceiptDisposition::created,
                "The receipt owner creates receipt 1 and commits the unit");
        return 1;
    });
    commit_unit(file, "Contribution unit", [](SqliteWorkContext context) -> QVariant {
        SqliteContributionRepository contributions(context, contribution_limits);
        const auto linked = ContributionEvidenceService(contributions).observe(support);
        require(linked.contribution_id == contribution_id &&
                    linked.disposition == ContributionLinkDisposition::created,
                "The contribution owner creates contribution 0 and commits the unit");
        return 1;
    });
    commit_unit(file, "Plan unit", [](SqliteWorkContext context) -> QVariant {
        const auto session = context.begin();
        require(SqliteApplicationPlanStore(session, plan_limits).retain_initial("F", captured) ==
                    SqlitePlanInitialDisposition::inserted,
                "Plan F revision 1 is staged on the borrowed session");
        session.commit();
        return 1;
    });
    Observer(file).expect(Cut::old);
}

enum class Tail { normal, throw_after_commit };
struct Attempt {
    SqliteWorkResult raw;
    std::optional<ApplicationServiceCompletion> outward;
    int completions{0};
    bool tail_reached{false};
    bool done{false};
};

// Runs one real service work pair. An arm and then a control fault are
// installed on the idle writer immediately before enqueueing.
std::shared_ptr<Attempt> run(SqliteWriter &writer, OriginalApplicationService &service,
                             const OriginalApplicationCommand &input,
                             std::optional<ClockFixtureArm> arm = {},
                             std::optional<WorkControlFault> fault = {}, Tail tail = Tail::normal) {
    const auto attempt = std::make_shared<Attempt>();
    auto work = service.make_work(input, [attempt](ApplicationServiceCompletion completion) {
        ++attempt->completions;
        attempt->outward = std::move(completion);
    });
    if (arm)
        require(detail::arm_clock_checkpoint_fixture(writer, *arm, application_key) ==
                    StorageError::none,
                "Install the closed one-shot owner arm for S/Q on the idle writer");
    if (fault)
        require(SqliteWriterWorkAccess::inject_next_control_ack_loss(writer, *fault) ==
                    StorageError::none,
                "Install the reviewed idle writer acknowledgement fault");
    require(writer.enqueue_work(
                [attempt, body = std::move(work.body), tail](SqliteWorkContext context) {
                    auto value = body(context);
                    if (tail == Tail::throw_after_commit) {
                        require(
                            value.metaType() == QMetaType::fromType<ApplicationServiceResponse>() &&
                                value.value<ApplicationServiceResponse>().disposition == D::applied,
                            "The real body returned after its own acknowledged COMMIT");
                        attempt->tail_reached = true;
                        throw std::runtime_error("Fixed fixture fault after COMMIT");
                    }
                    return value;
                },
                [attempt, completion = std::move(work.completion)](SqliteWorkResult result) {
                    attempt->raw = result;
                    completion(std::move(result));
                    attempt->done = true;
                }) == StorageError::none,
            "The writer admits the real service work pair");
    require(QTest::qWaitFor([attempt] { return attempt->done; }, timeout_ms),
            "The service work pair completes");
    require(attempt->completions == 1 && attempt->outward &&
                attempt->outward->transaction == attempt->raw.transaction &&
                attempt->outward->work_error == attempt->raw.error,
            "The completion is bridged once with the actual settlement and error");
    return attempt;
}

QString tuple(const ApplicationServiceCompletion &c) {
    return QString("{transaction %1, work_error %2, failure %3, response %4}")
        .arg(static_cast<int>(c.transaction))
        .arg(static_cast<int>(c.work_error))
        .arg(c.failure ? QString("source %1").arg(static_cast<int>(c.failure->source))
                       : QString("none"))
        .arg(c.response ? QString::number(static_cast<int>(c.response->disposition))
                        : QString("none"));
}

// {transaction, work_error, failure, response} with no response.
void expect_failure(const Attempt &a, Transaction transaction, WorkError error,
                    std::optional<ApplicationServiceFailure> failure = {}) {
    const auto &c = *a.outward;
    require(c.transaction == transaction && c.work_error == error && !c.response && !a.raw.value,
            "Exact settlement and error without a response, got " + tuple(c));
    if (!failure) {
        require(!c.failure, "No failure detail, got " + tuple(c));
        return;
    }
    require(c.failure && c.failure->source == failure->source &&
                c.failure->reason == failure->reason,
            "Exact failure source and typed reason, got " + tuple(c));
}

void expect_state(const ApplicationStateSnapshot &s, const char *token, const QByteArray &document,
                  qint64 main) {
    require(s.token == token && s.envelope.format_ref == state_format &&
                s.envelope.document == document && s.configuration_ref == "K" &&
                s.timer.lifecycle == core::TimerLifecycle::Active &&
                s.timer.phase == core::CountdownPhase::Main && s.timer.main_remaining_ms == main &&
                s.timer.grace_remaining_ms == 0 && !s.timer.awaiting_confirmation &&
                s.time_debt_ms == 0 && s.balances.empty(),
            QString("Exact SAS1 state %1").arg(token));
}

// The complete S/Q record, derived from the same literals as the observer rows.
void expect_recorded(const RecordedApplicationBundle &r) {
    const auto &i = r.intent;
    require(i.operation.operation_scope == "S" && i.operation.operation_ref == "Q" &&
                i.kind == ApplicationOperationKind::original &&
                i.subject_kind == ApplicationSubjectKind::contribution &&
                i.receipt_id == receipt_id && i.contribution_id == contribution_id &&
                i.plan.plan_ref == "F" && i.plan.revision == 1 &&
                i.choices.format_ref == "subathon.internal.application-command.v1" &&
                i.choices.document == sac && !i.reversal_ref,
            "Exact S/Q intent and command envelope");
    expect_state(r.persisted_head, "A", sas_a, original_main);
    expect_state(r.advanced_before, "D", sas_d, advanced_main);
    expect_state(r.after, "B", sas_b, applied_main);
    require(r.advance_explanation == "clock-observation-v1", "Exact advance explanation");
    require(r.contribution && r.contribution->contribution_id == contribution_id &&
                r.contribution->before == unapplied &&
                r.contribution->application_after == core::ContributionApplication::applied,
            "One contribution snapshot with the real before and after condition");
    require(r.effects.size() == 1, "Exactly one effect");
    const auto &e = r.effects.front();
    require(e.request_ordinal == 1 &&
                e.arithmetic_role == ApplicationArithmeticRole::time_addition &&
                e.unit == core::RequestedUnit::milliseconds &&
                e.exact_requested_units == core::Exact(requested_main) &&
                e.requested_units == requested_main && e.rounding_residual == core::Exact(0) &&
                !e.maximum_main_ms && !e.deficit_policy && !e.deficit_disposition &&
                e.application_explanation == "fixed-main",
            "Exact +137 ms Main effect");
    const std::pair<ApplicationComponentKind, std::int64_t> components[]{
        {ApplicationComponentKind::main_delta, requested_main},
        {ApplicationComponentKind::debt_delta, 0},
        {ApplicationComponentKind::cap_limited_time, 0}};
    require(e.components.size() == 3, "Exactly three #312 components");
    for (size_t k = 0; k < 3; ++k)
        require(e.components[k].kind == components[k].first &&
                    e.components[k].units == components[k].second && !e.components[k].target_ref,
                "Main +137, debt 0 and cap-limited 0 in order");
    require(r.jobs.empty() && r.transitions.empty() && r.reversals.empty(),
            "No job, transition or reversal");
    require(r.result.format_ref == "subathon.internal.application-result.v1" &&
                r.result.document == sar,
            "Exact result envelope");
    require(r.audit.actor_ref == std::optional<std::string>("U") && !r.audit.decision_ref &&
                !r.audit.reason &&
                r.audit.envelope.format_ref == "subathon.internal.application-audit.v1" &&
                r.audit.envelope.document == saa,
            "Exact audit");
}

void expect_response(const Attempt &a, D disposition) {
    const auto &c = *a.outward;
    require(
        c.transaction == Transaction::committed && c.work_error == WorkError::none && !c.failure &&
            c.response && c.response->disposition == disposition,
        QString("Committed response with disposition %1, got ").arg(static_cast<int>(disposition)) +
            tuple(c));
    if (disposition == D::applied || disposition == D::existing_operation)
        expect_recorded(std::get<RecordedApplicationBundle>(c.response->payload));
    else
        require(std::holds_alternative<std::monostate>(c.response->payload),
                "No retained success payload");
}

void expect_revoked(OriginalApplicationService &service) {
    const auto called = std::make_shared<bool>(false);
    const auto admission = service.enqueue(command, [called](auto) { *called = true; });
    require(admission.error == StorageError::shutting_down && !*called,
            "The unknown outcome revoked the writer");
}

// One fresh S/Q service attempt on its own writer; the caller checks the result.
struct Fresh {
    std::shared_ptr<Attempt> attempt;
    Ports::Trace trace;
    std::optional<detail::ClockFixtureArmReceipt> receipt;
    bool receipt_once{false};
};
Fresh fresh(const DisposableFile &file, std::optional<ClockFixtureArm> arm = {},
            std::optional<WorkControlFault> fault = {}, Tail tail = Tail::normal) {
    SqliteWriter writer;
    require(writer.open(file.path(), unit_writer_limits) == StorageError::none,
            "Service writer opens the authentic v22 store");
    Ports facts;
    auto ports = facts.ports();
    OriginalApplicationService service(writer, ports);
    Fresh out{run(writer, service, command, arm, fault, tail), {}, {}, false};
    out.trace = facts.trace();
    if (arm) {
        out.receipt = detail::take_clock_checkpoint_fixture_receipt(writer);
        out.receipt_once = !detail::take_clock_checkpoint_fixture_receipt(writer);
    }
    if (fault)
        expect_revoked(service);
    return out;
}

// One retry of S/Q on a new writer and service over an existing closure.
std::pair<std::shared_ptr<Attempt>, Ports::Trace>
retry(const DisposableFile &file, const OriginalApplicationCommand &input, bool disclose = true) {
    SqliteWriter writer;
    require(writer.open(file.path(), unit_writer_limits) == StorageError::none,
            "A new writer opens the reopened store");
    Ports facts(disclose);
    auto ports = facts.ports();
    OriginalApplicationService service(writer, ports);
    auto attempt = run(writer, service, input);
    return {attempt, facts.trace()};
}

// Seed and produce the genuine W3 B/2/2 closure as a basis for W4 cases.
void applied_basis(const DisposableFile &file) {
    seed(file);
    const auto applied = fresh(file);
    require(applied.trace == fresh_path, "W3 basis reached the real fresh path");
    expect_response(*applied.attempt, D::applied);
    Observer(file).expect(Cut::applied);
}

void expect_receipt(const Fresh &f, ClockFixtureArm arm, bool rollback_acknowledged) {
    require(f.receipt.has_value(), "A consumed-arm receipt is available after completion");
    const auto &r = *f.receipt;
    require(f.receipt_once, "The receipt is detached once");
    require(r.consumed == arm && r.operation.operation_scope == "S" &&
                r.operation.operation_ref == "Q",
            "The arm was consumed by this S/Q unit");
    expect_rows(r.head_changes.rows, {{integer(1)}}, "Head CAS changed exactly one row");
    require(r.head_changes.value_bytes == changes_result_bytes,
            "The head changes witness charges one cell of eight bytes");
    require(r.rollback_acknowledged == rollback_acknowledged,
            "Actual rollback acknowledgement is recorded");
    require(r.failure == StorageError::sql_failure, "The mapped storage error is sql_failure");
    if (arm == ClockFixtureArm::omit_slot_follow) {
        require(!r.slot_changes, "The slot follow-up was omitted");
        require(r.joined && r.joined->value_bytes == joined_bytes,
                "The joined mismatch read charges 298 value bytes");
        expect_rows(r.joined->rows, {mismatch_join}, "B head over the A slot and J root");
        require(r.failure_site == detail::ClockFixtureFailureSite::commit,
                "The deferred foreign key fails at the real COMMIT");
    } else {
        require(r.slot_changes && r.slot_changes->value_bytes == changes_result_bytes,
                "The slot CAS changes witness charges eight bytes");
        expect_rows(r.slot_changes->rows, {{integer(1)}}, "Slot CAS changed exactly one row");
        require(!r.joined, "No mismatch read on the late-abort path");
        require(r.failure_site == detail::ClockFixtureFailureSite::repeated_root,
                "The repeated root INSERT fails as a statement");
    }
}

} // namespace
} // namespace storage_003_w2_w4

using namespace storage_003_w2_w4;

class Storage003W2W4Tests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void w2DeferredCommitRollback() {
        check([] {
            DisposableFile file;
            seed(file);
            const auto w2 = fresh(file, ClockFixtureArm::omit_slot_follow);
            require(w2.trace == fresh_path, "W2 reached the real fresh path");
            expect_failure(*w2.attempt, Transaction::rolled_back, WorkError::sql_failure);
            expect_receipt(w2, ClockFixtureArm::omit_slot_follow, true);
            Observer(file).expect(Cut::old);
        });
    }

    void w3NormalCommit() {
        check([] {
            DisposableFile file;
            seed(file);
            const auto w3 = fresh(file);
            require(w3.trace == fresh_path, "W3 reached the real fresh path");
            expect_response(*w3.attempt, D::applied);
            Observer(file).expect(Cut::applied);
        });
    }

    void w3LateRootAbort() {
        check([] {
            DisposableFile file;
            seed(file);
            const auto late = fresh(file, ClockFixtureArm::repeat_root_append);
            require(late.trace == fresh_path, "Late abort reached the real fresh path");
            expect_failure(*late.attempt, Transaction::rolled_back, WorkError::sql_failure);
            expect_receipt(late, ClockFixtureArm::repeat_root_append, true);
            Observer(file).expect(Cut::old);
        });
    }

    void w4LostCommitAndExactRetry() {
        check([] {
            DisposableFile file;
            seed(file);
            const auto lost = fresh(file, {}, WorkControlFault::lose_next_commit_ack);
            require(lost.trace == fresh_path, "Lost commit reached the real fresh path");
            expect_failure(*lost.attempt, Transaction::outcome_unknown,
                           WorkError::uncertain_outcome);
            Observer(file).expect(Cut::applied);
            const auto [exact, trace] = retry(file, command);
            require(trace == history_path, "The retry discloses history without fresh facts");
            expect_response(*exact, D::existing_operation);
            Observer(file).expect(Cut::applied);
        });
    }

    void w4LostRollback() {
        check([] {
            DisposableFile file;
            seed(file);
            const auto lost = fresh(file, ClockFixtureArm::omit_slot_follow,
                                    WorkControlFault::lose_next_rollback_ack);
            require(lost.trace == fresh_path, "Lost rollback reached the real fresh path");
            expect_failure(*lost.attempt, Transaction::outcome_unknown,
                           WorkError::uncertain_outcome);
            expect_receipt(lost, ClockFixtureArm::omit_slot_follow, false);
            {
                SqliteWriter writer;
                require(writer.open(file.path(), unit_writer_limits) == StorageError::none,
                        "A new writer authenticates the old store");
                Ports facts;
                auto ports = facts.ports();
                OriginalApplicationService service(writer, ports);
                auto work = service.make_work(command, [](auto) {});
                require(work.body && work.completion,
                        "A new service prepares the only valid retry basis");
                require(facts.trace().empty(), "The retry is never enqueued");
            }
            Observer(file).expect(Cut::old);
        });
    }

    void w4BodyThrowAfterCommit() {
        check([] {
            DisposableFile file;
            seed(file);
            const auto thrown = fresh(file, {}, {}, Tail::throw_after_commit);
            require(thrown.trace == fresh_path, "Body throw reached the real fresh path");
            require(thrown.attempt->tail_reached, "The fixture wrapper threw after COMMIT");
            expect_failure(*thrown.attempt, Transaction::committed, WorkError::work_exception);
            Observer(file).expect(Cut::applied);
        });
    }

    void w4DeniedDisclosure() {
        check([] {
            DisposableFile file;
            applied_basis(file);
            const auto [denied, trace] = retry(file, command, false);
            require(trace == history_path, "Disclosure is checked on the historical path");
            expect_response(*denied, D::disclose_denied);
            Observer(file).expect(Cut::applied);
        });
    }

    void w4ConflictingRetry() {
        check([] {
            DisposableFile file;
            applied_basis(file);
            auto conflicting = command;
            conflicting.choices = {{"C", "Z"}};
            const auto [conflict, trace] = retry(file, conflicting);
            require(trace == history_path, "The conflict is found on the historical path");
            expect_response(*conflict, D::operation_conflict);
            Observer(file).expect(Cut::applied);
        });
    }

    void w4MissingAudit() {
        check([] {
            DisposableFile source;
            applied_basis(source);
            DisposableFile copy;
            source.copy_to(copy);
            mutate_stopped_copy(copy, CopyFault::missing_audit);
            Observer(copy).expect(Cut::missing_audit);
            const auto [refused, trace] = retry(copy, command);
            require(trace == fenced_path, "The shipped lookup refuses before disclosure");
            expect_failure(
                *refused, Transaction::rolled_back, WorkError::work_exception,
                ApplicationServiceFailure{ApplicationServiceFailureSource::bundle,
                                          SqliteBundleStorageReason::invalid_stored_data});
            Observer(copy).expect(Cut::missing_audit);
        });
    }

    void w4ContradictoryRoot() {
        check([] {
            DisposableFile source;
            applied_basis(source);
            for (const auto &[fault, cut] :
                 {std::pair{CopyFault::wrong_operation, Cut::wrong_operation},
                  std::pair{CopyFault::wrong_before, Cut::wrong_before}}) {
                DisposableFile copy;
                source.copy_to(copy);
                mutate_stopped_copy(copy, fault);
                Observer(copy).expect(cut);
                commit_unit(copy, "Shipped lookup unit", [](SqliteWorkContext context) -> QVariant {
                    const auto session = context.begin();
                    const auto found = SqliteApplicationBundleStore(session, bundle_limits)
                                           .lookup(application_key);
                    require(found.has_value(), "The shipped bundle lookup succeeds on the copy");
                    expect_recorded(*found);
                    session.commit();
                    return 1;
                });
                Observer(copy).expect(cut);
                const auto [refused, trace] = retry(copy, command);
                require(trace == fenced_path, "The closure fence refuses before disclosure");
                expect_failure(*refused, Transaction::rolled_back, WorkError::sql_failure);
                Observer(copy).expect(cut);
            }
        });
    }
};

QTEST_GUILESS_MAIN(Storage003W2W4Tests)
#include "w2_w4_tests.moc"
