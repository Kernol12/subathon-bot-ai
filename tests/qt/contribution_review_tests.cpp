// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/contribution_review_fixture.hpp"

#include <QThread>
#include <array>
#include <subathon/server/sqlite_modifier_activations.hpp>
#include <subathon/server/sqlite_rate_overrides.hpp>
#include <tuple>

namespace {
using namespace contribution_review_fixture;
using D = ReviewDisposition;
using S = ReviewStatus;
using P = ReviewUnitPhase;
using U = ReviewUnit;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &failure) {
        QFAIL(failure.what());
    }
}

void refusal(const ContributionReview::Result &result, ReviewRefusal expected) {
    require(std::holds_alternative<ReviewRefusal>(result) &&
                std::get<ReviewRefusal>(result) == expected,
            "Whole-work refusal returns the exact class and no row payload");
}

void clean_review_tables(CatalogFile &file) {
    for (const auto *name :
         {"subathon_review_work", "subathon_review_entries", "subathon_review_terminal_requests"})
        require(file.rows(QString("SELECT count(*) FROM %1").arg(name)) == SqlRows{{0}},
                "No history is backfilled into new review tables");
}

template <class Read> void marker_control(Read read, bool correct) {
    CatalogFile file;
    file.write(read); // Genuine v18 control precedes the new consumer check.
    file.advance_literal();
    require(file.rows(history_query) == expected_history(literal_v19()),
            "Literal catalog committed with exact independent history framing");
    if (correct) {
        file.write(read);
        return;
    }
    file.write([](const auto &s) {
        (void)s.execute({"UPDATE subathon_migration_history SET migration_id=? WHERE version=19",
                         {QString("Contribution-review-v1")}});
    });
    bool rejected = false;
    try {
        file.write(read);
    } catch (const SqliteBundleStorageFailure &error) {
        rejected = error.reason() == SqliteBundleStorageReason::invalid_stored_data;
    } catch (const SqliteModifierStorageFailure &error) {
        rejected = error.reason() == SqliteModifierStorageReason::invalid_stored_data;
    }
    require(rejected, "Lookalike marker yields the consumer's typed invalid-stored-data refusal");
}

struct Batch : Case {
    std::vector<ReviewEntryIntent> display;

    Batch() {
        for (int i = 1; i <= 10; ++i)
            (void)intake("unreviewed-" + std::to_string(i));
        for (int i = 11; i <= 13; ++i) {
            auto entry = seed("receipt-" + std::to_string(i), positive());
            require(entry.selector.receipt.value == std::uint64_t(i),
                    "Committed original receipt IDs are the frozen 11, 12 and 13");
            display.push_back(std::move(entry));
        }
        display.back().milestones.reset();
        std::ranges::reverse(display);
        facts.current->milestones = {{"threshold-110", 110}};
        db.reopen();
        head(95, 0, 90);
    }

    ReviewCommand command(ContributionReview &review) {
        const auto rows = preview(review, display);
        require(rows.size() == 3 && rows[2].facts && rows[2].facts->occurrences.size() == 1,
                "Only the third projected row reaches threshold 110");
        display[0].milestones = std::vector<MilestoneAcceptanceChoice>{
            {rows[2].facts->occurrences.front(), MilestoneActionChoice::defer}};
        auto cmd = bind("batch", display, rows);
        // Execute receives display order too, so physical ordering is independently witnessed.
        std::ranges::reverse(cmd.entries);
        empty_journal();
        trace.events.clear();
        return cmd;
    }

    void prefix_one() {
        prefix(2, {{1, int(S::applied), blob8(11)}});
        head(99, 0, 97);
        claims(1);
        require(db.rows("SELECT request_ordinal,component_kind,units FROM subathon_app_components "
                        "WHERE units<>0 ORDER BY request_ordinal,component_kind") ==
                        SqlRows{{1, 0, 4}, {2, 2, 7}} &&
                    db.rows("SELECT occurrence_ref FROM subathon_milestone_groups").isEmpty() &&
                    db.rows("SELECT job_ref FROM subathon_app_jobs").isEmpty(),
                "Only the first row's literal +4 Main/+7 score and no suffix side effects survive");
    }

    void complete_prefix() {
        prefix(4, {{1, int(S::applied), blob8(11)},
                   {2, int(S::applied), blob8(12)},
                   {3, int(S::applied), blob8(13)}});
        head(100, 0, 111);
        claims(3);
        require(
            db.rows("SELECT e.ordinal,c.component_kind,c.units FROM subathon_review_entries e "
                    "JOIN subathon_app_components c ON c.operation_scope=e.application_scope "
                    "AND c.operation_ref=e.application_ref WHERE c.units<>0 "
                    "ORDER BY e.ordinal,c.component_kind") ==
                SqlRows{
                    {1, 0, 4}, {1, 2, 7}, {2, 0, 1}, {2, 2, 7}, {2, 5, 3}, {3, 2, 7}, {3, 5, 4}},
            "Independent cap consumption is 4, 1, 0 Main and 0, 3, 4 limited");
        require(
            db.rows(
                "SELECT g.target_micro_points,g.score_micro_points,d.choice,g.released "
                "FROM subathon_milestone_groups g JOIN subathon_milestone_acceptance_decisions d "
                "USING(occurrence_ref)") == SqlRows{{110, 111, 1, 0}},
            "One ordered Defer achievement remains unreleased");
    }
};

void inject_rollback(Batch &batch, int ordinal) {
    batch.trace.control = [ordinal](const auto &event) {
        if (event.unit == U::row && event.ordinal == ordinal && event.phase == P::before_commit)
            throw std::runtime_error("synthetic after-stage failure before real COMMIT");
    };
}

enum class AllocationPredecessor { last_row, row_rollback, live_budget };

using WriterAck = std::tuple<U, int, TransactionOutcome, WorkError, bool>;

struct AllocationObservation {
    std::vector<WriterAck> acknowledgements;
    std::optional<ContributionReview::Result> result;
    unsigned unit_events{};
    unsigned callbacks{};
    bool predecessor_seen{};
    bool predecessor_turn_ended{};
    bool on_owner{};
    bool in_predecessor_turn{};
};

struct AllocationAttempt {
    ReviewCommand command;
    std::shared_ptr<AllocationObservation> observed;
    std::shared_ptr<FinalReviewAllocationFault> fault;
    bool delivered{};
};

AllocationAttempt observe_allocation(Batch &batch, AllocationPredecessor predecessor, bool armed) {
    const auto observed = std::make_shared<AllocationObservation>();
    const auto fault = std::make_shared<FinalReviewAllocationFault>();
    auto *const writer = &batch.db.writer();
    auto *const owner = writer->thread();
    const auto last_unit =
        predecessor == AllocationPredecessor::live_budget ? U::qualification_read : U::row;
    const int last_ordinal = predecessor == AllocationPredecessor::last_row ? 3 : 2;
    ContributionReview service{
        *writer, batch.facts,
        FinalReviewAllocationObserver{
            [&batch, observed, writer, last_unit, last_ordinal](const ReviewUnitEvent &event) {
                ++observed->unit_events;
                if (event.completion) {
                    const auto &result = *event.completion;
                    observed->acknowledgements.emplace_back(event.unit, event.ordinal,
                                                            result.transaction, result.error,
                                                            result.value.has_value());
                    if (event.unit == last_unit && event.ordinal == last_ordinal) {
                        observed->predecessor_seen = true;
                        // Mark the next owner turn from inside the real writer completion.
                        QMetaObject::invokeMethod(
                            writer, [observed] { observed->predecessor_turn_ended = true; },
                            Qt::QueuedConnection);
                    }
                }
                batch.trace.observe(event);
            },
            fault}};
    require(batch.db.rows(history_query) == expected_history(literal_v19()),
            "The disposable file holds the exact committed v19 migration history");
    expect_schema(batch.db.rows(schema_query), literal_v19().steps.back());
    batch.head(95, 0, 90);
    batch.claims(0);
    auto command = batch.command(service);
    observed->acknowledgements.clear(); // Preview is a separate completed request.

    if (predecessor == AllocationPredecessor::row_rollback) {
        inject_rollback(batch, 2);
    } else if (predecessor == AllocationPredecessor::live_budget) {
        batch.trace.control = [&batch](const ReviewUnitEvent &event) {
            if (event.unit == U::row && event.ordinal == 1 && event.phase == P::completed) {
                auto &jobs = batch.facts.current->internal_jobs;
                for (int i = 0; i < 769; ++i)
                    jobs.push_back({"bounded-fanout-" + std::to_string(i),
                                    {},
                                    {},
                                    {},
                                    {delivery_job_format, "synthetic internal"}});
            }
        };
    }

    fault->armed = armed;
    service.execute(command, "synthetic", [observed, owner](auto result) {
        if (++observed->callbacks == 1) {
            observed->on_owner = QThread::currentThread() == owner;
            observed->in_predecessor_turn =
                observed->predecessor_seen && !observed->predecessor_turn_ended;
            observed->result.emplace(std::move(result));
        }
    });
    require(QTest::qWaitFor([observed] { return observed->predecessor_turn_ended; }, 10000),
            "The selected real writer completion reaches its owner-turn checkpoint");
    const bool delivered =
        QTest::qWaitFor([observed] { return observed->result.has_value(); }, armed ? 250 : 10000);

    const auto committed = [](U unit, int ordinal) -> WriterAck {
        return {unit, ordinal, TransactionOutcome::committed, WorkError::none, true};
    };
    std::vector<WriterAck> expected{committed(U::initial_read, 0),
                                    committed(U::qualification_read, 1), committed(U::row, 1),
                                    committed(U::qualification_read, 2)};
    std::vector<int> admitted{1};
    std::vector<int> published{1};
    if (predecessor == AllocationPredecessor::last_row) {
        expected.insert(expected.end(), {committed(U::row, 2), committed(U::qualification_read, 3),
                                         committed(U::row, 3)});
        admitted = published = {1, 2, 3};
    } else if (predecessor == AllocationPredecessor::row_rollback) {
        expected.emplace_back(U::row, 2, TransactionOutcome::rolled_back, WorkError::work_exception,
                              false);
        admitted.push_back(2);
    }
    if (!armed)
        expected.push_back(committed(U::final_read, 0));
    require(observed->acknowledgements == expected,
            "All writer acknowledgements match the exact ordered predecessor and settlement");
    require(batch.trace.ordinals(U::row, P::before_enqueue) == admitted &&
                batch.trace.ordinals(U::row, P::began) == admitted &&
                batch.trace.ordinals(U::row, P::before_commit) == admitted &&
                batch.trace.ordinals(U::row, P::after_commit) == published &&
                batch.trace.ordinals(U::row, P::published) == published,
            "Only the selected rows stage, and only normally committed rows publish once");
    const auto final = armed ? std::vector<int>{} : std::vector{0};
    for (const auto phase : {P::before_enqueue, P::began, P::completed})
        require(batch.trace.ordinals(U::final_read, phase) == final,
                "The final read has zero admission, body and completion on the allocation fault");
    require(!fault->armed && fault->hits == unsigned(armed),
            "The provisional final allocation consumes the fixture arm exactly once when armed");
    return {std::move(command), observed, fault, delivered};
}

std::optional<ReviewStop> allocation_prior_stop(AllocationPredecessor predecessor) {
    switch (predecessor) {
    case AllocationPredecessor::last_row:
        return {};
    case AllocationPredecessor::row_rollback:
        return ReviewStop::write_failed;
    case AllocationPredecessor::live_budget:
        return ReviewStop::row_budget_exceeded;
    }
    Q_UNREACHABLE();
}

void expect_applied_link(Batch &batch, const ReviewReply &reply, const ReviewCommand &command,
                         int ordinal, D disposition) {
    expected_row(reply, ordinal, disposition, S::applied);
    const auto &selector = command.entries.at(std::size_t(3 - ordinal)).intent.selector;
    const auto &facts = reply.rows.at(std::size_t(ordinal - 1)).outcome->facts;
    const auto link = batch.db.rows(
        "SELECT application_ref FROM subathon_review_entries WHERE ordinal=?", {ordinal});
    require(facts && facts->verified == selector && facts->bundle &&
                facts->bundle->operation_scope == "review-application" &&
                link == SqlRows{{QByteArray::fromStdString(facts->bundle->operation_ref)}},
            "Applied discloses the verified CID/receipt and exact owning bundle link");
}

void expect_allocation_reply(Batch &batch, const AllocationAttempt &attempt,
                             AllocationPredecessor predecessor, bool faulted) {
    const auto &result = *attempt.observed->result;
    const auto stop = allocation_prior_stop(predecessor);
    if (faulted && !stop) {
        refusal(result, ReviewRefusal::unavailable);
        return;
    }
    const auto &reply = Case::reply(result);
    require(reply.stop == stop && reply.rows.size() == 3,
            "The exact prior stop and all three ordinals are preserved");
    for (int ordinal = 1; ordinal <= 3; ++ordinal) {
        if (!stop || ordinal == 1) {
            if (faulted)
                expected_row(reply, ordinal, D::committed);
            else
                expect_applied_link(batch, reply, attempt.command, ordinal, D::committed);
        } else {
            expected_row(reply, ordinal,
                         ordinal == 2 && stop == ReviewStop::write_failed ? D::write_failed
                                                                          : D::unattempted);
        }
    }
}

void expect_allocation_durable(Batch &batch, const ReviewCommand &command, int applied) {
    if (applied == 3)
        batch.complete_prefix();
    else
        batch.prefix_one();
    require(batch.db.rows("SELECT installation_ref,work_scope,work_ref,entry_count,"
                          "substr(manifest,1,4) FROM subathon_review_work") ==
                SqlRows{{QByteArray("install"), QByteArray("review"), QByteArray("batch"), 3,
                         QByteArray("SRW1")}},
            "One manifest binds the exact installation, work key and selected entry count");
    SqlRows expected;
    SqlRows contributions;
    for (int ordinal = 1; ordinal <= 3; ++ordinal) {
        const bool committed = ordinal <= applied;
        contributions.append({blob8(10 + ordinal), committed ? 2 : 0, committed ? 1 : 0});
    }
    require(
        batch.db.rows("SELECT original_receipt_id,review,application FROM subathon_contributions "
                      "ORDER BY original_receipt_id") == contributions,
        "Only the committed prefix is accepted and applied in contribution storage");
    for (int ordinal = 1; ordinal <= applied; ++ordinal) {
        const auto cid =
            blob8(command.entries.at(std::size_t(3 - ordinal)).intent.selector.cid.value);
        expected.append({ordinal, 1, blob8(10 + ordinal), cid, QByteArray("review-application"),
                         QByteArray::number(10 + ordinal).prepend("receipt-").append("-plan"),
                         blob8(1), 0, cid, QByteArray("operator"),
                         QByteArray::number(ordinal).prepend("decision-"),
                         QByteArray::number(ordinal).prepend("synthetic reason ")});
    }
    require(
        batch.db.rows(
            "SELECT e.ordinal,e.decision_kind,o.receipt_id,o.contribution_id,o.operation_scope,"
            "o.plan_ref,o.plan_revision,c.subject_kind,c.subject_id,a.actor_ref,a.decision_ref,"
            "a.reason FROM subathon_review_entries e JOIN subathon_app_operations o "
            "ON e.application_scope=o.operation_scope AND e.application_ref=o.operation_ref "
            "JOIN subathon_app_claims c USING(operation_scope,operation_ref) "
            "JOIN subathon_app_audit a USING(operation_scope,operation_ref) ORDER BY e.ordinal") ==
            expected,
        "Each durable accepted decision links the selected receipt, plan, claim and exact audit");
    require(batch.db.rows("SELECT count(*) FROM subathon_app_operations") == SqlRows{{applied}} &&
                batch.db.rows("SELECT count(*) FROM subathon_app_audit") == SqlRows{{applied}} &&
                batch.db.rows("SELECT count(*) FROM subathon_app_results") == SqlRows{{applied}} &&
                batch.db.rows("SELECT count(*) FROM subathon_app_effects") ==
                    SqlRows{{2 * applied}} &&
                batch.db.rows("SELECT * FROM subathon_app_jobs").isEmpty() &&
                batch.db.rows("SELECT * FROM subathon_delivery_captures").isEmpty(),
            "No extra bundle, audit, effect or delivery job persists");
}

QList<SqlRows> allocation_snapshot(Batch &batch) {
    QList<SqlRows> rows;
    for (const auto *table : {"subathon_review_work",
                              "subathon_review_entries",
                              "subathon_review_terminal_requests",
                              "subathon_contributions",
                              "subathon_app_head",
                              "subathon_app_states",
                              "subathon_app_balances",
                              "subathon_app_operations",
                              "subathon_app_contributions",
                              "subathon_app_claims",
                              "subathon_app_effects",
                              "subathon_app_components",
                              "subathon_app_transitions",
                              "subathon_app_audit",
                              "subathon_app_results",
                              "subathon_app_jobs",
                              "subathon_delivery_captures",
                              "subathon_milestone_groups",
                              "subathon_milestone_actions",
                              "subathon_milestone_acceptance_decisions"})
        rows.append(batch.db.rows(QString("SELECT * FROM %1 ORDER BY rowid").arg(table)));
    return rows;
}

void rollback_reply(Batch &batch, const ContributionReview::Result &result, int ordinal) {
    const auto &reply = Case::reply(result);
    require(reply.stop == ReviewStop::write_failed && reply.rows.size() == 3,
            "Known rollback stops the batch with the full ordinal shape");
    for (int i = 1; i < ordinal; ++i)
        expected_row(reply, i, D::committed, S::applied);
    expected_row(reply, ordinal, D::write_failed);
    for (int i = ordinal + 1; i <= 3; ++i)
        expected_row(reply, i, D::unattempted);
    const auto final_reads = batch.trace.ordinals(U::final_read, P::began);
    require(final_reads == std::vector{0} || (ordinal == 1 && final_reads.empty()),
            "Known rollback permits one final read and first-row rollback may stop without one");
    const auto completions = batch.trace.completions();
    require(completions.size() == std::size_t(ordinal) &&
                completions.back().transaction == TransactionOutcome::rolled_back,
            "The real writer acknowledged the failing row's rollback");
    std::vector<int> expected;
    for (int i = 1; i <= ordinal; ++i)
        expected.push_back(i);
    require(batch.trace.ordinals(U::row, P::before_enqueue) == expected &&
                batch.trace.ordinals(U::row, P::began) == expected,
            "No row admission or BEGIN beyond the stopped ordinal");
}

std::vector<FinancialTimerHold> seed_mixed_refund(finance::File &f, const std::string &ref,
                                                  bool hold_timer) {
    const auto main = hold_timer ? 50000 : 125000;
    f.move_main("financial-fixture-elapsed", main);
    const auto quarter = finance::fraction(1, 4);
    const FinancialEntitlement entitlement{
        0,
        5,
        quarter,
        false,
        GroupedRefundEntitlement{{finance::original, 1, finance::CK::main_delta},
                                 1,
                                 0,
                                 {},
                                 {75000, 0},
                                 75000,
                                 0,
                                 -75000,
                                 0},
        {{2,
          {finance::original, 2, finance::CK::score},
          0,
          0,
          10000000,
          -10000000,
          quarter,
          10000000,
          0,
          false},
         {3,
          {finance::original, 3, finance::CK::whole_count},
          0,
          0,
          0,
          0,
          quarter,
          finance::fraction(7, 4),
          finance::fraction(7, 4),
          false}}};
    finance::Effects effects;
    if (!hold_timer)
        effects.apply(-75000, 0, {75000});
    const auto refund = f.refund(ref, core::ReceiptMoney{"5.00", "EUR"});
    (void)f.accept(f.command(ref, refund, 0), entitlement, effects);
    auto financial = f.financial_active();
    if (hold_timer) {
        require(financial.size() == 1 && effects.adjustments == 0,
                "Real #199 mixed refund held time and credited its score adjustment once");
    } else {
        require(financial.empty() && effects.adjustments == 1,
                "Real #199 mixed refund with enough Main applies time and leaves no hold");
    }
    return financial;
}

void advance_review_catalog(finance::File &f) {
    report(migrate(*f.writer,
                   request(MigrationMode::advance, milestone_target_edits_repository_manifest())),
           CatalogState::current, 18, 18, 3);
    report(migrate(*f.writer, request(MigrationMode::advance, literal_v19())),
           CatalogState::current, 19, 19, 1);
}

struct CreditedOriginal {
    static constexpr std::int64_t deficit = -400000;
    finance::File file{finance::OriginalCredit{.beside_complete_request = deficit}};
    std::optional<ApplicationHoldVersionLocator> financial;
    ApplicationHoldDetection detection;
    Facts facts;

    explicit CreditedOriginal(const std::string &refund_ref, bool hold_timer = true) {
        const auto holds = seed_mixed_refund(file, refund_ref, hold_timer);
        if (!holds.empty())
            financial = holds.front().locator;
        // Give the separate stores equal current observations via a genuine committed operation.
        file.move_main("credited-preview-head", 50000);
        file.check_head(50000, 0, 30000000, 7);
        detection = file.ordinary_detection("beside-complete", false);
        auto &cause = detection.cause;
        cause.command.operation = finance::original;
        cause.kind = ApplicationHoldCauseKind::timer_deficit;
        cause.missing_policy_reason.reset();
        cause.plan_evidence = {};
        cause.plan_evidence.plan = finance::original_plan;
        for (const auto &request : file.support_capture.captured.requested_effects) {
            const auto ordinal =
                static_cast<std::int64_t>(cause.plan_evidence.occurrences.size() + 1);
            cause.plan_evidence.occurrences.push_back({ordinal,
                                                       {},
                                                       {},
                                                       {},
                                                       {},
                                                       request.unit,
                                                       request.exact_requested_units,
                                                       request.requested_units,
                                                       {}});
        }
        cause.timer = {4,
                       finance::CK::unapplied_time,
                       deficit,
                       finance::Policy::RequireReview,
                       core::TimeReversalDisposition::RequiresReview,
                       deficit,
                       ApplicationHoldComposition::beside_complete_application,
                       finance::document(),
                       ApplicationHoldAssociatedApplication{finance::original, 4}};
        (void)file.detect(detection);
        record_second_version();
        advance_review_catalog(file);
        facts.configuration_document = "configuration";
    }

    void record_second_version() {
        detection.request.mutation_ref = "beside-complete-second-detection";
        detection.request.expected_version_ref = "v1";
        detection.request.proposed_version_ref = "v2";
        file.write([&](const auto &s) {
            require(SqliteApplicationHoldStore(s).record_detection(detection).disposition ==
                        SqliteHoldMutationDisposition::recorded,
                    "Owning store records the exact second active beside-complete version");
        });
    }

    ReviewEntryIntent intent(ReviewVerb verb) const {
        ReviewEntryIntent result;
        result.selector = {file.cid, file.payment.support_receipt};
        result.verb = verb;
        if (verb == ReviewVerb::reject_held || verb == ReviewVerb::retain_held)
            result.timer = ReviewTimerSelector{{detection.cause.hold_ref, "v2"}, 4, deficit};
        if (verb == ReviewVerb::reject_held)
            result.ordinary_closures = {
                {detection.cause.hold_ref, "v2", "v3", finance::document(), {}}};
        return result;
    }

    static ReviewCommand bind(ContributionReview &service, std::string key,
                              ReviewEntryIntent intent) {
        auto result = await<ContributionReview::PreviewResult>([&](auto done) {
            service.preview({principal, {intent}}, "synthetic", std::move(done));
        });
        const auto *preview = std::get_if<std::vector<ReviewPreviewRow>>(&result);
        require(preview, "Credited original returns a same-row preview for this review verb");
        auto command = Case::bind(key, {intent}, *preview);
        command.entries.front().decision_ref = std::move(key);
        return command;
    }

    auto protected_rows() {
        return std::array{
            file.rows("SELECT * FROM subathon_contributions ORDER BY contribution_id"),
            file.rows("SELECT * FROM subathon_app_claims ORDER BY subject_kind,subject_id"),
            file.rows("SELECT * FROM subathon_app_head"),
            file.rows(
                "SELECT * FROM subathon_app_operations ORDER BY operation_scope,operation_ref"),
            file.rows("SELECT * FROM subathon_app_effects ORDER BY "
                      "operation_scope,operation_ref,request_ordinal"),
            file.rows("SELECT * FROM subathon_app_components "
                      "ORDER BY operation_scope,operation_ref,request_ordinal,component_kind"),
            file.rows("SELECT * FROM subathon_financial_refunds"),
            file.rows("SELECT * FROM subathon_financial_bases ORDER BY original_ordinal,kind"),
            file.rows("SELECT * FROM subathon_financial_resolutions")};
    }

    auto nonfinancial_basis_rows() {
        return std::array{
            file.rows("SELECT * FROM subathon_contributions"),
            file.rows("SELECT * FROM subathon_app_claims WHERE subject_kind=0"),
            file.rows(
                "SELECT s.* FROM subathon_app_states s JOIN subathon_app_head h USING(token)"),
            file.rows("SELECT b.* FROM subathon_app_balances b JOIN subathon_app_head h "
                      "USING(token) ORDER BY b.balance_kind,b.target_ref"),
            file.rows("SELECT * FROM subathon_plan_revisions WHERE plan_ref=?",
                      {QByteArray::fromStdString(finance::original_plan.plan_ref)}),
            file.rows("SELECT * FROM subathon_hold_causes"),
            file.rows("SELECT * FROM subathon_hold_versions ORDER BY hold_ref,position")};
    }

    void expect_ordinary(ApplicationHoldState state, std::string_view version) {
        file.write([&](const auto &s) {
            const auto ordinary = SqliteApplicationHoldStore(s).find(detection.cause.hold_ref);
            require(ordinary && ordinary->current_version.state == state &&
                        ordinary->current_version.locator.version_ref == version &&
                        ordinary->cause.timer &&
                        ordinary->cause.timer->composition ==
                            ApplicationHoldComposition::beside_complete_application &&
                        (state == ApplicationHoldState::active ||
                         ordinary->current_version.parent_version_ref == "v2"),
                    "Exact beside-complete origin and immutable ordinary version survive");
        });
    }
};

} // namespace

class ContributionReviewTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void catalogProducerAdvancesExactlyOneStep() {
        check([] {
            // A genuine shipped v15 original and achievement predate the upgrades.
            finance::OriginalCredit credit;
            credit.milestone = true;
            finance::File file(credit);
            report(migrate(*file.writer, request(MigrationMode::advance,
                                                 milestone_target_edits_repository_manifest())),
                   CatalogState::current, 18, 18, 3);
            const auto literal = literal_v19();
            const auto v18_history = file.rows(history_query);
            const auto original = file.rows("SELECT * FROM subathon_app_results");
            const auto pending = file.rows("SELECT * FROM subathon_milestone_groups");
            require(original.size() == 1 && pending.size() == 1 && v18_history.size() == 18,
                    "Shipped history, bundle and milestone origin are present before new producer");
            const auto producer = contribution_review_repository_manifest(); // Stage 1 NI
            require(producer.store_identity == literal.store_identity &&
                        producer.catalog_format == literal.catalog_format &&
                        producer.supported_starting_versions ==
                            literal.supported_starting_versions &&
                        producer.steps.size() == literal.steps.size(),
                    "Producer adds only the assigned start and the assigned step");
            for (qsizetype i = 0; i < literal.steps.size(); ++i) {
                const auto &a = producer.steps[i];
                const auto &e = literal.steps[i];
                require(a.version == e.version && a.id == e.id && a.sql == e.sql &&
                            a.expected_schema == e.expected_schema,
                        "Exact ordered migration statements and full projection for each step");
            }
            report(migrate(*file.writer, request(MigrationMode::inspect, producer)),
                   CatalogState::upgrade_required, 18, 19, 0);
            require(file.rows(history_query) == v18_history,
                    "Read-only inspection preserves the whole v18 prefix");
            report(migrate(*file.writer, request(MigrationMode::advance, producer)),
                   CatalogState::current, 19, 19, 1);
            const auto history = file.rows(history_query);
            require(history == expected_history(literal) && history.mid(0, 18) == v18_history,
                    "Committed history matches the independently framed hashes");
            expect_schema(file.rows(schema_query), literal.steps.back());
            require(file.rows("SELECT * FROM subathon_app_results") == original &&
                        file.rows("SELECT * FROM subathon_milestone_groups") == pending,
                    "Original bundle and shipped milestone origin remain byte-identical");
            for (const auto *name : {"subathon_review_work", "subathon_review_entries",
                                     "subathon_review_terminal_requests"})
                require(file.rows(QString("SELECT count(*) FROM %1").arg(name)) == SqlRows{{0}},
                        "New journal and terminal relation start empty");
        });
    }

    void literalCatalogBudgetAtAndOneBelow() {
        check([] {
            CatalogFile file;
            const auto literal = literal_v19();
            const auto charge = tally(literal);
            require(charge == v19_manifest_bytes &&
                        literal.steps.back().expected_schema.size() == 225 &&
                        digest(literal.steps.back()).toHex() == v19_digest_hex,
                    "Independent static tally, projected count, and canonical v19 digest match");
            const auto before = file.rows(history_query);
            auto under = request(MigrationMode::advance, literal);
            under.limits.manifest_utf8_bytes = charge - 1;
            bool completion = false;
            const auto rejected = SqliteMigrationRunner::enqueue(file.writer(), std::move(under),
                                                                 [&](auto) { completion = true; });
            require(rejected.error == StorageError::invalid_options &&
                        rejected.reason == MigrationReason::invalid_manifest && !completion &&
                        file.rows(history_query) == before,
                    "One byte below cumulative charge is refused at migration admission");
            auto exact = request(MigrationMode::advance, literal);
            exact.limits.manifest_utf8_bytes = charge;
            report(migrate(file.writer(), std::move(exact)), CatalogState::current, 19, 19, 1);
            clean_review_tables(file);
            expect_schema(file.rows(schema_query), literal.steps.back());
        });
    }

    void exactV19FinancialMarker() {
        check([] {
            marker_control(
                [](const auto &s) {
                    require(
                        SqliteFinancialRefundStore(s).active({core::ContributionId{0}}).empty(),
                        "Known healthy financial selection at the explicitly supported catalog");
                },
                true);
        });
    }

    void exactV19MilestoneMarker() {
        check([] {
            marker_control(
                [](const auto &s) {
                    require(SqliteMilestoneGroupStore(s).pending().empty(),
                            "Known healthy shipped milestone selection at the supported catalog");
                },
                true);
        });
    }

    void exactV19ModifierMarker() {
        check([] {
            marker_control(
                [](const auto &s) {
                    require(!SqliteModifierActivationStore(s).instance_revision("absent"),
                            "Known healthy shipped modifier selection at the supported catalog");
                },
                true);
        });
    }

    void exactV19RateMarker() {
        check([] {
            CatalogFile file;
            const auto read_head = [&] {
                std::optional<RateRevisionResult> result;
                file.write([&](const auto &s) { result = SqliteRateOverrideUnit(s).head(); });
                return result.value();
            };
            const auto before = read_head();
            const auto *root = std::get_if<core::EffectiveRateSnapshot>(&before);
            require(root && root->data().snapshot_ref == "E",
                    "Protected v18 typed rate head selects the reserved root E");
            file.advance_literal();
            require(file.rows(history_query) == expected_history(literal_v19()),
                    "Literal catalog committed with exact independent history framing");
            file.reopen();
            const auto after = read_head();
            const auto *upgraded = std::get_if<core::EffectiveRateSnapshot>(&after);
            require(upgraded && upgraded->data().snapshot_ref == "E",
                    "Exact v19 typed rate head selects the preserved reserved root E");
        });
    }

    void lookalikeV19MarkersStayClosed() {
        check([] {
            marker_control(
                [](const auto &s) {
                    (void)SqliteFinancialRefundStore(s).active({core::ContributionId{0}});
                },
                false);
            marker_control([](const auto &s) { (void)SqliteMilestoneGroupStore(s).pending(); },
                           false);
            marker_control(
                [](const auto &s) {
                    (void)SqliteModifierActivationStore(s).instance_revision("absent");
                },
                false);
            CatalogFile file;
            file.advance_literal();
            file.write([](const auto &s) {
                (void)s.execute({"UPDATE subathon_migration_history SET migration_id=? "
                                 "WHERE version=19",
                                 {QString("Contribution-review-v1")}});
                const auto result = SqliteRateOverrideUnit(s).head();
                require(std::get_if<RateStoreRefusal>(&result) &&
                            std::get<RateStoreRefusal>(result) == RateStoreRefusal::unavailable,
                        "Case-changed v19 marker fails closed in the typed rate reader");
            });
        });
    }

    void lookupWithCurrentScopeDoesNotResumeOrCreate() {
        check([] {
            Case f;
            f.db.reopen();
            auto service = f.service();
            const auto lookup = [&](const std::string &key) {
                return await<ContributionReview::Result>([&](auto done) {
                    service.lookup({"review", key}, principal, "synthetic", std::move(done));
                });
            };
            f.facts.batch_allowed = false;
            refusal(lookup("absent"), ReviewRefusal::denied); // Independent NI entry
            f.empty_journal();
            require(f.trace.ordinals(U::row, P::before_enqueue).empty(), "Lookup cannot stage");
        });
    }

    void rollbackPrefixAndExactRetry() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            inject_rollback(batch, 2);
            rollback_reply(batch, Case::execute(service, command), 2);
            batch.prefix_one();
            const auto first = batch.db.rows("SELECT * FROM subathon_review_entries");
            const auto manifest = batch.db.rows("SELECT manifest FROM subathon_review_work");
            batch.trace = {};
            const auto resumed = Case::execute(service, command, "refreshed");
            const auto &reply = Case::reply(resumed);
            expected_row(reply, 1, D::existing, S::applied);
            expected_row(reply, 2, D::committed, S::applied);
            expected_row(reply, 3, D::committed, S::applied);
            batch.complete_prefix();
            require(batch.db.rows("SELECT * FROM subathon_review_entries WHERE ordinal=1") ==
                            first &&
                        batch.db.rows("SELECT manifest FROM subathon_review_work") == manifest &&
                        batch.trace.ordinals(U::row, P::published) == std::vector{2, 3},
                    "Immutable seed row and manifest remain exact, and only new commits publish");
            auto permutation = command;
            std::ranges::reverse(permutation.entries);
            batch.trace = {};
            const auto replay = Case::execute(service, permutation);
            for (int i = 1; i <= 3; ++i)
                expected_row(Case::reply(replay), i, D::existing, S::applied);
            batch.complete_prefix();
            require(batch.trace.ordinals(U::row, P::before_enqueue).empty() &&
                        batch.trace.ordinals(U::row, P::published).empty(),
                    "Exact canonical replay appends no write and publishes nothing");
        });
    }

    void firstRowRollbackLeavesNoBinding() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            inject_rollback(batch, 1);
            rollback_reply(batch, Case::execute(service, command), 1);
            batch.empty_journal();
            batch.head(95, 0, 90);
            batch.claims(0);
        });
    }

    void uncertainAndAbnormalCommitStop_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("lost-real-commit-ack") << 0;
        QTest::newRow("lost-real-rollback-ack") << 1;
        QTest::newRow("acknowledged-commit-body-throws") << 2;
    }

    void uncertainAndAbnormalCommitStop() {
        QFETCH(int, fault);
        check([&] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            batch.trace.control = [&](const auto &e) {
                if (e.unit != U::row || e.ordinal != 2)
                    return;
                if (fault != 2 && e.phase == P::before_enqueue)
                    require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                                batch.db.writer(),
                                fault == 0 ? WorkControlFault::lose_next_commit_ack
                                           : WorkControlFault::lose_next_rollback_ack) ==
                                StorageError::none,
                            "Existing owner/idle writer fault arms on the exact upcoming row");
                if ((fault == 1 && e.phase == P::before_commit) ||
                    (fault == 2 && e.phase == P::after_commit))
                    throw std::runtime_error("synthetic boundary exception");
            };
            const auto stopped = Case::execute(service, command);
            const auto &reply = Case::reply(stopped);
            require(reply.stop == (fault == 2 ? ReviewStop::committed_result_unavailable
                                              : ReviewStop::commit_uncertain),
                    "Abnormal acknowledged completion and unknown outcome remain distinct");
            expected_row(reply, 2,
                         fault == 2 ? D::committed_result_unavailable : D::commit_uncertain);
            expected_row(reply, 3, D::unattempted);
            require(
                batch.trace.ordinals(U::row, P::before_enqueue) == std::vector{1, 2} &&
                    batch.trace.ordinals(U::final_read, P::before_enqueue).empty(),
                "No suffix or final reconciliation is sent after an uncertain or abnormal unit");
            const auto completed = batch.trace.completions();
            require(completed.size() == 2 &&
                        completed.back().transaction ==
                            (fault == 2 ? TransactionOutcome::committed
                                        : TransactionOutcome::outcome_unknown) &&
                        completed.back().error != WorkError::none,
                    "Classification comes from real writer control and completion");
            if (fault == 1)
                batch.prefix_one();
            else {
                batch.prefix(3, {{1, int(S::applied), blob8(11)}, {2, int(S::applied), blob8(12)}});
                batch.head(100, 0, 104);
                batch.claims(2);
            }
            batch.db.reopen();
            auto recovered = batch.service();
            batch.trace = {};
            const auto retry = Case::execute(recovered, command, "refreshed");
            expected_row(Case::reply(retry), 1, D::existing, S::applied);
            expected_row(Case::reply(retry), 2, fault == 1 ? D::committed : D::existing,
                         S::applied);
            expected_row(Case::reply(retry), 3, D::committed, S::applied);
            batch.complete_prefix();
        });
    }

    void changedIntentOrBrokenCursorNeverStagesSuffix() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            inject_rollback(batch, 2);
            rollback_reply(batch, Case::execute(service, command), 2);
            batch.prefix_one();
            batch.trace = {};
            const auto manifest = batch.db.rows("SELECT manifest FROM subathon_review_work");
            const auto fresh = batch.preview(service, batch.display);
            require(fresh.at(0).facts && fresh.at(0).facts->basis != command.entries.back().basis,
                    "A genuine same-selector preview after the prefix has a different basis");
            for (int change = 0; change < 4; ++change) {
                auto changed = command;
                auto &row = changed.entries.back();
                if (change == 0)
                    row.intent.mode = PausedAcceptanceMode::silent;
                if (change == 1)
                    row.intent.milestones.reset();
                if (change == 2)
                    row.reason += "!";
                if (change == 3)
                    row.basis = fresh.at(0).facts->basis;
                refusal(Case::execute(service, changed), ReviewRefusal::identity_conflict);
                batch.prefix_one();
            }
            batch.facts.batch_allowed = false;
            refusal(Case::execute(service, command), ReviewRefusal::denied);
            auto absent = command;
            absent.key.operation_ref = "absent";
            refusal(Case::execute(service, absent), ReviewRefusal::denied);
            batch.facts.batch_allowed = true;
            batch.db.write([](const auto &s) {
                (void)s.execute({"UPDATE subathon_review_work SET next_ordinal=3", {}});
            });
            refusal(Case::execute(service, command), ReviewRefusal::unavailable);
            batch.db.write([](const auto &s) {
                (void)s.execute({"UPDATE subathon_review_work SET next_ordinal=2", {}});
                (void)s.execute({"UPDATE subathon_review_entries SET receipt_id=? WHERE ordinal=1",
                                 {blob8(12)}});
            });
            refusal(Case::execute(service, command), ReviewRefusal::unavailable);
            require(batch.trace.ordinals(U::row, P::before_enqueue).empty() &&
                        batch.db.rows("SELECT manifest FROM subathon_review_work") == manifest,
                    "Conflicts, unsafe authority and a gapped cursor cannot stage any row");
            batch.claims(1);
        });
    }

    void liveCapAndUnrelatedScoreDriftStaleSuffix() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            inject_rollback(batch, 2);
            rollback_reply(batch, Case::execute(service, command), 2);
            batch.prefix_one();
            batch.trace = {};
            batch.facts.current->cap = 101;
            batch.facts.current->revision = "policy-2";
            // Represent another authorized owner command with a fresh immutable head.
            batch.db.write([](const auto &s) {
                const QByteArray fresh("unrelated-score-correction");
                (void)s.execute(
                    {"INSERT INTO subathon_app_states "
                     "SELECT ?,format_ref,document,configuration_ref,lifecycle,phase,"
                     "main_remaining_ms,grace_remaining_ms,awaiting_confirmation,time_debt_ms,"
                     "balance_count FROM subathon_app_states WHERE token=(SELECT token FROM "
                     "subathon_app_head)",
                     {fresh}});
                (void)s.execute(
                    {"INSERT INTO subathon_app_balances "
                     "SELECT ?,balance_kind,target_ref,units+CASE WHEN balance_kind=2 THEN 6 ELSE "
                     "0 END "
                     "FROM subathon_app_balances WHERE token=(SELECT token FROM subathon_app_head)",
                     {fresh}});
                (void)s.execute({"UPDATE subathon_app_head SET token=?", {fresh}});
            });
            batch.head(99, 0, 103);
            const auto retry = Case::execute(service, command);
            expected_row(Case::reply(retry), 1, D::existing, S::applied);
            expected_row(Case::reply(retry), 2, D::committed, S::stale);
            expected_row(Case::reply(retry), 3, D::committed, S::stale);
            batch.prefix(4, {{1, int(S::applied), blob8(11)},
                             {2, int(S::stale), blob8(12)},
                             {3, int(S::stale), blob8(13)}});
            batch.head(99, 0, 103);
            batch.claims(1);
        });
    }

    void eligibilityAndDisclosureAreSameRow_data() {
        QTest::addColumn<bool>("deny_execution");
        QTest::newRow("unavailable-selector") << false;
        QTest::newRow("revoke-row-after-preview") << true;
    }

    void eligibilityAndDisclosureAreSameRow() {
        QFETCH(bool, deny_execution);
        check([&] {
            Case f;
            for (int i = 1; i <= 20; ++i)
                (void)f.intake("earlier-" + std::to_string(i));
            ReviewEntryIntent row1;
            if (deny_execution) {
                row1 = f.seed("first", Case::positive(), true, true);
            } else {
                const auto received = f.intake("unresolved-original-anchor");
                row1.selector = {{999}, received.id};
                row1.milestones = std::vector<MilestoneAcceptanceChoice>{};
            }
            auto row2 = f.seed("pending-rate", {}, true, true);
            auto row3 = f.seed("third", Case::positive());
            require(row1.selector.receipt.value == 21 && row2.selector.receipt.value == 22 &&
                        row3.selector.receipt.value == 23,
                    "The real intake has the frozen anchor order");
            std::vector entries{row3, row2, row1};
            f.db.reopen();
            auto service = f.service();
            const auto rows = f.preview(service, entries);
            require(rows.size() == 3 && rows[1].facts && rows[1].facts->may_decide &&
                        !rows[1].facts->may_apply &&
                        std::ranges::find(rows[1].facts->blockers,
                                          core::ContributionBlocker::missing_rate) !=
                            rows[1].facts->blockers.end(),
                    "Preview exposes independent valuation block with permission to decide");
            const auto command = Case::bind("privacy", entries, rows);
            if (deny_execution)
                f.facts.execution_denied = row1.selector;
            const auto result = Case::execute(service, command);
            const auto &r = Case::reply(result);
            expected_row(r, 1, D::committed, deny_execution ? S::denied : S::ineligible);
            expected_row(r, 2, D::committed, S::accepted_pending);
            expected_row(r, 3, D::committed, S::applied);
            require(!r.rows[0].outcome->facts,
                    "Unavailable or denied row exposes no verified facts");
            f.prefix(4, {{1, deny_execution ? 7 : 8, {}},
                         {2, int(S::accepted_pending), blob8(22)},
                         {3, int(S::applied), blob8(23)}});
            require(
                f.db.rows("SELECT contribution_id IS NULL,receipt_id IS NULL,decision_kind IS NULL,"
                          "outcome IS NULL,application_scope IS NULL,application_ref IS NULL "
                          "FROM subathon_review_entries WHERE ordinal=1") ==
                    SqlRows{{1, 1, 1, 1, 1, 1}},
                "Denied/unavailable journal ordinal has no protected cells");
            require(f.db.rows("SELECT review,application,missing_rate FROM subathon_contributions "
                              "WHERE original_receipt_id=?",
                              {blob8(22)}) == SqlRows{{2, 0, 1}},
                    "Only the ordinary review dimension is patched on a missing-rate contribution");
            f.head(99, 0, 97);
            f.claims(1);
            f.facts.execution_denied.reset();
            f.facts.disclosure_denied = row3.selector;
            f.trace = {};
            const auto replay = Case::execute(service, command, "refreshed");
            expected_row(Case::reply(replay), 1, D::existing,
                         deny_execution ? S::denied : S::ineligible);
            expected_row(Case::reply(replay), 2, D::existing, S::accepted_pending);
            const auto &redacted = Case::reply(replay).rows.at(2);
            require(redacted.ordinal == 3 && redacted.outcome &&
                        redacted.outcome->status == S::denied && !redacted.outcome->facts &&
                        !redacted.disposition &&
                        f.trace.ordinals(U::row, P::before_enqueue).empty(),
                    "Replayed row 3 reveals only denial and never rewrites the journal");
            f.claims(1);
        });
    }

    void authoritySeesCanonicalSelectionAndRowEffectsOnExecuteAndRetry() {
        check([] {
            struct Witness final : ContributionReviewFacts {
                Facts &fixture;
                std::array<ReviewSelector, 2> expected;
                bool checked_batch{};
                std::array<bool, 2> checked_effects{};

                Witness(Facts &backing, std::array<ReviewSelector, 2> selection)
                    : fixture(backing), expected(selection) {}

                AccessDecision check(const ApplicationReferenceReadView &view,
                                     const ReviewAuthorityQuery &query) override {
                    if (query.use == ReviewAuthorityUse::batch_key) {
                        require(std::ranges::equal(query.selection, expected),
                                "Batch authority sees both selectors in durable intake order");
                        checked_batch = true;
                    }
                    if (query.use == ReviewAuthorityUse::execute_row && query.effects) {
                        require(std::ranges::equal(query.selection, expected),
                                "Effect authority sees the complete canonical batch");
                        const auto selector = query.committed ? query.committed->verified
                                              : query.entry   ? std::optional{query.entry->selector}
                                                              : std::nullopt;
                        require(selector.has_value(),
                                "Typed effects are bound to one verified row");
                        const auto row = std::ranges::find(expected, *selector);
                        require(row != expected.end(), "Effect authority selects a batch member");
                        const auto index = std::size_t(row - expected.begin());
                        const auto effects = *query.effects;
                        require(effects.size() == 2 && effects[0].request_ordinal == 1 &&
                                    effects[1].request_ordinal == 2 &&
                                    effects[0].requested_units == 4 &&
                                    effects[1].requested_units == 7,
                                "Authority sees both exact retained request ordinals");
                        using Kind = ApplicationComponentKind;
                        const auto &time = effects[0].components;
                        const auto &score = effects[1].components;
                        require(time.size() == 3 && time[0].kind == Kind::main_delta &&
                                    time[0].units == (index == 0 ? 4 : 1) &&
                                    time[1].kind == Kind::debt_delta && time[1].units == 0 &&
                                    time[2].kind == Kind::cap_limited_time &&
                                    time[2].units == (index == 0 ? 0 : 3) && score.size() == 1 &&
                                    score[0].kind == Kind::score && score[0].units == 7 &&
                                    score[0].target_ref == "score",
                                "Typed authority uses exact cap-adjusted time and score");
                        checked_effects[index] = true;
                    }
                    return fixture.check(view, query);
                }

                std::optional<ReviewPolicyFacts> policy(const ApplicationReferenceReadView &view,
                                                        const ReviewSelector &selector) override {
                    return fixture.policy(view, selector);
                }
            };

            Case f;
            const auto first = f.seed("authority-first", Case::positive());
            const auto second = f.seed("authority-second", Case::positive());
            const std::vector entries{second, first};
            f.db.reopen();
            Witness witness{f.facts, {first.selector, second.selector}};
            ContributionReview service{
                f.db.writer(), witness, [&](const auto &event) {
                    if (event.unit == U::row && event.phase == P::before_commit)
                        require(event.ordinal >= 1 && event.ordinal <= 2 &&
                                    witness.checked_effects[std::size_t(event.ordinal - 1)],
                                "The exact prospective effects reached authority before commit");
                    f.trace.observe(event);
                }};
            const auto preview = f.preview(service, entries);
            auto command = Case::bind("authority-inputs", entries, preview);
            std::ranges::reverse(command.entries);
            witness.checked_batch = false;
            witness.checked_effects.fill(false);
            const auto applied = Case::execute(service, command);
            expected_row(Case::reply(applied), 1, D::committed, S::applied);
            expected_row(Case::reply(applied), 2, D::committed, S::applied);
            require(witness.checked_batch && witness.checked_effects == std::array{true, true},
                    "Execute checked batch scope and each prospective effect vector");
            f.head(100, 0, 104);
            f.claims(2);

            witness.checked_batch = false;
            witness.checked_effects.fill(false);
            f.trace.events.clear();
            const auto retry = Case::execute(service, command, "refreshed");
            expected_row(Case::reply(retry), 1, D::existing, S::applied);
            expected_row(Case::reply(retry), 2, D::existing, S::applied);
            require(witness.checked_batch && witness.checked_effects == std::array{true, true} &&
                        f.trace.ordinals(U::row, P::before_enqueue).empty(),
                    "Exact retry checked retained effects at a full cap without staging rows");
            f.head(100, 0, 104);
            f.claims(2);
        });
    }

    void sameTokenEvidenceChangeAndRejectionKeepBlockers() {
        check([] {
            Case f;
            auto first = f.seed("independent-blocker", {}, true, true);
            auto second = f.seed("evidence-changes", Case::positive());
            first.verb = ReviewVerb::reject;
            first.milestones.reset();
            f.db.reopen();
            auto service = f.service();
            const std::vector entries{second, first};
            const auto preview = f.preview(service, entries);
            const auto command = Case::bind("reject-stale", entries, preview);
            f.db.write([&](const auto &s) {
                (void)s.execute({"UPDATE subathon_contributions SET payment_evidence=1 "
                                 "WHERE contribution_id=?",
                                 {blob8(second.selector.cid.value)}});
            });
            require(f.db.rows("SELECT token FROM subathon_app_head") ==
                        SqlRows{{QByteArray("review-initial")}},
                    "An independent evidence mutation left the global application head unchanged");
            const auto applied = Case::execute(service, command);
            expected_row(Case::reply(applied), 1, D::committed, S::rejected);
            expected_row(Case::reply(applied), 2, D::committed, S::stale);
            require(
                f.db.rows(
                    "SELECT review,missing_rate,original_decimal,currency,resolution_ref IS NULL "
                    "FROM subathon_contributions WHERE contribution_id=?",
                    {blob8(first.selector.cid.value)}) ==
                    SqlRows{{3, 1, QByteArray("20.0000"), QByteArray("EUR"), 1}},
                "Rejection retains source amount spelling, missing quote and other dimensions");
            f.head(95, 0, 90);
            f.claims(0);
        });
    }

    void sixteenRowsAreDistinctAndFifteenRowPrefixReconciles() {
        check([] {
            Case f;
            std::vector<ReviewEntryIntent> entries;
            for (int i = 1; i <= 16; ++i)
                entries.push_back(f.seed("bounded-" + std::to_string(i), {}, true, true));
            std::ranges::reverse(entries);
            f.db.reopen();
            auto service = f.service();
            const auto command = Case::bind("sixteen", entries, f.preview(service, entries));
            f.trace = {};
            f.trace.control = [](const auto &e) {
                if (e.unit == U::row && e.ordinal == 16 && e.phase == P::before_commit)
                    throw std::runtime_error("synthetic last-row rollback");
            };
            const auto stopped = Case::execute(service, command);
            const auto &reply = Case::reply(stopped);
            require(reply.stop == ReviewStop::write_failed, "Sixteenth row is a known rollback");
            for (int ordinal = 1; ordinal < 16; ++ordinal)
                expected_row(reply, ordinal, D::committed, S::accepted_pending);
            expected_row(reply, 16, D::write_failed);
            const auto completed = f.trace.completions();
            require(completed.size() == 16 &&
                        completed.back().transaction == TransactionOutcome::rolled_back &&
                        f.trace.works() <= 34 &&
                        f.trace.ordinals(U::final_read, P::began) == std::vector{0},
                    "Sixteen real row units and bounded reconciliation at the request limit");
            for (std::size_t i = 0; i < 15; ++i)
                require(completed[i].ordinal == int(i + 1) &&
                            completed[i].transaction == TransactionOutcome::committed &&
                            completed[i].error == WorkError::none,
                        "Every earlier ordinal independently acknowledged COMMIT");
            const auto prefix = f.db.rows("SELECT * FROM subathon_review_entries ORDER BY ordinal");
            require(prefix.size() == 15 &&
                        f.db.rows("SELECT next_ordinal FROM subathon_review_work") == SqlRows{{16}},
                    "Physical accepted-pending prefix contains exactly fifteen ordinals");
            f.trace = {};
            const auto resumed = Case::execute(service, command);
            for (int ordinal = 1; ordinal < 16; ++ordinal)
                expected_row(Case::reply(resumed), ordinal, D::existing, S::accepted_pending);
            expected_row(Case::reply(resumed), 16, D::committed, S::accepted_pending);
            require(f.trace.ordinals(U::row, P::began) == std::vector{16} &&
                        f.trace.ordinals(U::initial_read, P::began) == std::vector{0} &&
                        f.trace.ordinals(U::final_read, P::began) == std::vector{0} &&
                        f.db.rows("SELECT next_ordinal FROM subathon_review_work") ==
                            SqlRows{{17}} &&
                        f.db.rows("SELECT * FROM subathon_review_entries ORDER BY ordinal")
                                .mid(0, 15) == prefix,
                    "One suffix write between bounded initial and final reconciliation");
            f.claims(0);
            f.head(95, 0, 90);
        });
    }

    void executeIngressRefusesSeventeenWithoutPreview() {
        check([] {
            Case f;
            const auto eligible = f.seed("ingress-anchor", Case::positive());
            f.db.reopen();
            auto service = f.service();
            ReviewCommand command{{"review", "ingress-over-limit"}, principal, "operator", {}};
            for (std::uint64_t i = 1; i <= 17; ++i) {
                auto intent = eligible;
                if (i != 1)
                    intent.selector = {{i + 1000}, {i}};
                command.entries.push_back({intent, QByteArray("opaque-basis"),
                                           "decision-" + std::to_string(i), "bounded reason"});
            }
            // A 17-row selection cannot have an authorized batch preview. Ingress
            // checks this owned count before any parsing of opaque row bases.
            refusal(Case::execute(service, command), ReviewRefusal::batch_too_large);
            require(f.trace.events.empty(), "Oversize owns no writer work");
            f.empty_journal();
            f.head(95, 0, 90);
            f.claims(0);
        });
    }

    void preflightRefusesSeventeenAndUnqualifiedWriter() {
        check([] {
            Case f;
            const auto eligible = f.seed("qualifying-input", Case::positive());
            f.db.reopen();
            auto qualified = f.service();
            const std::vector selected{eligible};
            auto command = Case::bind("one", selected, f.preview(qualified, selected));
            auto oversize = command;
            for (std::uint64_t i = 2; i <= 17; ++i) {
                auto row = command.entries.front();
                row.intent.selector = {{i + 1000}, {i}};
                oversize.entries.push_back(std::move(row));
            }
            auto oversized_reason = command;
            oversized_reason.entries.front().reason = std::string(61441, 'a');
            f.trace = {};
            refusal(Case::execute(qualified, oversize), ReviewRefusal::batch_too_large);
            refusal(Case::execute(qualified, oversized_reason), ReviewRefusal::batch_too_large);
            f.empty_journal();
            require(f.trace.ordinals(U::row, P::before_enqueue).empty(),
                    "One-over refuses before row admission");
            f.db.reopen(StorageLimits{});
            auto default_writer = f.service();
            f.trace = {};
            refusal(Case::execute(default_writer, command), ReviewRefusal::unavailable);
            f.empty_journal();
            require(f.trace.ordinals(U::row, P::before_enqueue).empty(),
                    "64-attempt default cannot stage review");
            f.head(95, 0, 90);
            f.claims(0);
        });
    }

    void singleActiveAttemptAndSessionLossStopForwarding() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            using Result = ContributionReview::Result;
            std::array<std::optional<Result>, 3> competitors;
            batch.trace.control = [&](const auto &e) {
                if (e.unit != U::row || e.ordinal != 1 || e.phase != P::after_commit)
                    return;
                require(batch.trace.ordinals(U::row, P::before_enqueue) == std::vector{1},
                        "The next row has not even been queued while row 1 completion is pending");
                batch.prefix_one();
                for (std::size_t i = 0; i < competitors.size(); ++i) {
                    auto request = command;
                    if (i == 2)
                        request.key.operation_ref = "competitor";
                    service.execute(std::move(request), "synthetic",
                                    [&, i](auto reply) { competitors[i] = std::move(reply); });
                }
                batch.facts.batch_allowed = false;
            };
            const auto stopped = Case::execute(service, command);
            require(Case::reply(stopped).stop == ReviewStop::denied,
                    "Current batch authority loss stops forwarding with definite denial");
            expected_row(Case::reply(stopped), 2, D::unattempted);
            expected_row(Case::reply(stopped), 3, D::unattempted);
            require(!Case::reply(stopped).rows.at(0).outcome,
                    "Stop without current scope has no protected historical row payload");
            require(QTest::qWaitFor(
                        [&] {
                            return std::ranges::all_of(competitors,
                                                       [](const auto &r) { return r.has_value(); });
                        },
                        10000),
                    "Active service refuses every competing request");
            for (const auto &result : competitors)
                refusal(*result, ReviewRefusal::unavailable);
            require(batch.trace.ordinals(U::row, P::began) == std::vector{1} &&
                        batch.trace.ordinals(U::final_read, P::began).empty(),
                    "Known scope loss allows no next write and no final reconciliation");
            batch.prefix_one();
            batch.facts.batch_allowed = true;
            batch.trace = {};
            const auto retry = Case::execute(service, command);
            expected_row(Case::reply(retry), 1, D::existing, S::applied);
            expected_row(Case::reply(retry), 2, D::committed, S::applied);
            expected_row(Case::reply(retry), 3, D::committed, S::applied);
            batch.complete_prefix();
        });
    }

    void unknownScopeStopsWithoutReadingThePrefix() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            batch.trace.control = [&](const auto &event) {
                if (event.unit == U::row && event.ordinal == 1 && event.phase == P::after_commit)
                    batch.facts.session_known = false;
            };
            const auto result = Case::execute(service, command);
            const auto &reply = Case::reply(result);
            require(reply.stop == ReviewStop::unavailable && reply.rows.size() == 3 &&
                        !reply.rows.front().outcome,
                    "Unknown current session yields Unavailable without protected prefix data");
            expected_row(reply, 2, D::unattempted);
            expected_row(reply, 3, D::unattempted);
            require(batch.trace.ordinals(U::row, P::before_enqueue) == std::vector{1} &&
                        batch.trace.ordinals(U::row, P::began) == std::vector{1} &&
                        batch.trace.ordinals(U::final_read, P::before_enqueue).empty() &&
                        batch.trace.ordinals(U::final_read, P::began).empty(),
                    "Unknown current session forbids further admissions and reconciliation");
            batch.prefix_one();
        });
    }

    void refusedWriterAdmissionStopsWithoutStartingTheRow() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            auto stopped = std::make_shared<bool>(false);
            batch.trace.control = [&](const auto &event) {
                if (event.unit == U::row && event.ordinal == 2 && event.phase == P::before_enqueue)
                    require(batch.db.writer().shutdown([stopped] { *stopped = true; }) ==
                                StorageError::none,
                            "Close real writer admission immediately before the second enqueue");
            };
            const auto result = Case::execute(service, command);
            const auto &reply = Case::reply(result);
            require(reply.stop == ReviewStop::admission_refused && reply.rows.size() == 3 &&
                        !reply.rows.front().outcome,
                    "Actual writer admission refusal returns the safe ordinal stop shape");
            expected_row(reply, 2, D::unattempted);
            expected_row(reply, 3, D::unattempted);
            require(batch.trace.ordinals(U::row, P::before_enqueue) == std::vector{1, 2} &&
                        batch.trace.ordinals(U::row, P::began) == std::vector{1} &&
                        batch.trace.ordinals(U::final_read, P::before_enqueue).empty() &&
                        batch.trace.ordinals(U::final_read, P::began).empty(),
                    "Refused admission starts neither the refused row nor any subsequent unit");
            require(QTest::qWaitFor([stopped] { return *stopped; }, 10000),
                    "The real writer acknowledges shutdown");
            batch.prefix_one();
        });
    }

    void silentKeepsEffectsAndOrderedMilestoneChoices() {
        check([] {
            Case f;
            auto intent = f.seed("silent", Case::positive(4, 35));
            intent.mode = PausedAcceptanceMode::silent;
            intent.milestones.reset();
            f.facts.current->clock.paused = true;
            f.facts.current->milestones = {{"z-first", 100}, {"m-second", 110}, {"a-last", 120}};
            f.facts.current->ordinary_alerts = {
                {"ordinary-message", {}, {}, {}, {delivery_job_format, "synthetic message"}},
                {"ordinary-sound", {}, {}, {}, {delivery_job_format, "synthetic sound"}},
                {"ordinary-visual", {}, {}, {}, {delivery_job_format, "synthetic visual"}}};
            f.facts.current->internal_jobs = {
                {"internal-progress", {}, {}, {}, {delivery_job_format, "synthetic internal"}}};
            f.db.reopen();
            auto service = f.service();
            const auto preview = f.preview(service, {intent});
            require(preview.size() == 1 && preview[0].facts &&
                        preview[0].facts->occurrences.size() == 3,
                    "Absent choices discover three prospective ordered crossings without writes");
            f.empty_journal();
            f.head(95, 0, 90);
            const auto &refs = preview[0].facts->occurrences;
            intent.milestones =
                std::vector<MilestoneAcceptanceChoice>{{refs[0], MilestoneActionChoice::run},
                                                       {refs[1], MilestoneActionChoice::defer},
                                                       {refs[2], MilestoneActionChoice::skip}};
            auto command = Case::bind("silent-valid", {intent}, preview);
            for (int fault = 0; fault < 3; ++fault) {
                auto invalid = command;
                invalid.key.operation_ref = "bad-choice-" + std::to_string(fault);
                auto &choices = invalid.entries.front().intent.milestones;
                if (fault == 0)
                    choices.reset();
                if (fault == 1)
                    choices->push_back(choices->back());
                if (fault == 2)
                    std::ranges::reverse(*choices);
                const auto rejected = Case::execute(service, invalid);
                const auto &row = Case::reply(rejected).rows.at(0);
                require(row.outcome && (row.outcome->status == S::ineligible ||
                                        row.outcome->status == S::stale),
                        "Missing, extra and reordered occurrence choices cannot apply");
                f.head(95, 0, 90);
                f.claims(0);
            }
            const auto accepted = Case::execute(service, command);
            expected_row(Case::reply(accepted), 1, D::committed, S::applied);
            f.head(99, 0, 125);
            f.claims(1);
            const auto results = f.db.rows("SELECT format_ref,document FROM subathon_app_results");
            require(results.size() == 1 &&
                        results.front()[0].toByteArray() ==
                            "subathon.internal.application-result.v1" &&
                        results.front()[1].toByteArray().startsWith("SAR1"),
                    "An ordinary complete application retains the v1 result format and SAR1");
            require(f.db.rows("SELECT g.target_micro_points,g.score_micro_points,d.mode,d.choice "
                              "FROM subathon_milestone_groups g JOIN "
                              "subathon_milestone_acceptance_decisions d "
                              "USING(occurrence_ref) ORDER BY g.queue_order") ==
                        SqlRows{{100, 125, 1, 0}, {110, 125, 1, 1}, {120, 125, 1, 2}},
                    "Physical achievement order is crossing order with Silent Run/Defer/Skip");
            require(f.db.rows("SELECT job_ref FROM subathon_app_jobs WHERE group_ref IS NULL "
                              "ORDER BY job_ref") == SqlRows{{QByteArray("internal-progress")}},
                    "Only internal ungrouped job remains after removing all ordinary alerts");
        });
    }

    void wholeOriginalDeficits_data() {
        QTest::addColumn<int>("choice");
        QTest::newRow("one-stop") << 0;
        QTest::newRow("one-carry") << 1;
        QTest::newRow("ordered-carry-then-stop") << 2;
        QTest::newRow("fresh-main-fits") << 3;
        QTest::newRow("explicit-retain") << 4;
        QTest::newRow("explicit-reject") << 5;
    }

    void wholeOriginalDeficits() {
        QFETCH(int, choice);
        check([&] {
            using Policy = core::RefundDeficitPolicy;
            Case f(99, 7);
            f.facts.current->signed_original = {{1, Policy::RequireReview}};
            std::vector requests{finance::request(finance::Unit::milliseconds, -100, "main")};
            if (choice == 2) {
                auto next = finance::request(finance::Unit::milliseconds, -2, "main");
                next.action_ref = "second-debit";
                requests.push_back(next);
                f.facts.current->signed_original.push_back({2, Policy::RequireReview});
            }
            requests.push_back(finance::request(finance::Unit::micro_points, 15, "score"));
            auto intent = f.seed("whole-debit", requests);
            f.db.reopen();
            auto service = f.service();
            const auto command = Case::bind("initial-hold", {intent}, f.preview(service, {intent}));
            const auto first = Case::execute(service, command);
            expected_row(Case::reply(first), 1, D::committed, S::accepted_pending);
            const auto &outcome = Case::reply(first).rows.at(0).outcome->facts;
            require(outcome && outcome->timer && outcome->timer->ordinal == 1 &&
                        outcome->timer->signed_request == -100,
                    "59A holds the full signed original request at its first blocked ordinal");
            const auto &hold = outcome->timer->locator;
            f.db.write([&](const auto &s) {
                const auto row = SqliteApplicationHoldStore(s).find(hold.hold_ref);
                require(row && row->cause.timer && row->cause.timer->request_units == -100 &&
                            row->cause.timer->unapplied_units == -100 &&
                            row->cause.timer->composition ==
                                ApplicationHoldComposition::defer_whole_input &&
                            finance::same(row->current_version.locator, hold),
                        "Shipped ordinary hold retains the exact active version and whole signed "
                        "request");
            });
            f.head(99, 7, 90);
            f.claims(0);
            require(f.db.rows("SELECT * FROM subathon_app_effects").isEmpty() &&
                        f.db.rows("SELECT * FROM subathon_app_jobs").isEmpty(),
                    "No score, timer effect or job survives a whole-input deferral");
            intent.verb = ReviewVerb::continue_timer;
            intent.timer = outcome->timer;
            intent.pending_decision_ref = command.entries.front().decision_ref;
            if (choice >= 4) {
                intent.verb = choice == 4 ? ReviewVerb::retain_held : ReviewVerb::reject_held;
                intent.milestones.reset();
                intent.pending_decision_ref.reset();
            } else if (choice == 3) {
                // An independent authorized fixture change moves Main to exactly the retained
                // debit.
                f.db.write([](const auto &s) {
                    const QByteArray token("fresh-fit-head");
                    (void)s.execute({"INSERT INTO subathon_app_states SELECT ?,format_ref,document,"
                                     "configuration_ref,lifecycle,phase,100,grace_remaining_ms,"
                                     "awaiting_confirmation,time_debt_ms,balance_count FROM "
                                     "subathon_app_states "
                                     "WHERE token=(SELECT token FROM subathon_app_head)",
                                     {token}});
                    (void)s.execute(
                        {"INSERT INTO subathon_app_balances SELECT ?,balance_kind,target_ref,units "
                         "FROM subathon_app_balances WHERE token=(SELECT token FROM "
                         "subathon_app_head)",
                         {token}});
                    (void)s.execute({"UPDATE subathon_app_head SET token=?", {token}});
                });
                intent.deficits = std::vector<ReviewDeficitChoice>{};
            } else {
                const auto discover = f.preview(service, {intent});
                require(discover[0].facts && discover[0].facts->reached_deficit &&
                            discover[0].facts->reached_deficit->ordinal == 1 &&
                            discover[0].facts->reached_deficit->signed_request == -100,
                        "A fresh preview stops at the exact first unresolved deficit");
                intent.deficits = std::vector<ReviewDeficitChoice>{
                    {1, -100, choice == 0 ? Policy::StopAtZero : Policy::CarryTimeDebt}};
                if (choice == 2) {
                    const auto missing = f.preview(service, {intent});
                    require(missing[0].facts && missing[0].facts->reached_deficit &&
                                missing[0].facts->reached_deficit->ordinal == 2 &&
                                missing[0].facts->reached_deficit->signed_request == -2,
                            "Carry on ordinal 1 exposes the entire reached ordinal 2 request");
                    const auto rejected =
                        Case::execute(service, Case::bind("missing-second", {intent}, missing));
                    const auto &row = Case::reply(rejected).rows.at(0);
                    require(row.outcome && (row.outcome->status == S::ineligible ||
                                            row.outcome->status == S::stale),
                            "Omitting a reached second choice refuses the entire domain unit");
                    f.head(99, 7, 90);
                    f.claims(0);
                    intent.deficits->push_back({2, -2, Policy::StopAtZero});
                }
            }

            auto next = Case::bind("fresh-continuation", {intent}, f.preview(service, {intent}));
            next.entries.front().decision_ref = "continuation-decision";
            const auto applied = Case::execute(service, next);
            if (choice >= 4) {
                expected_row(Case::reply(applied), 1, D::committed,
                             choice == 4 ? S::retained : S::rejected);
                f.head(99, 7, 90);
                f.claims(0);
            } else {
                expected_row(Case::reply(applied), 1, D::committed, S::timer_resolved);
                f.head(0, choice == 1 || choice == 2 ? 8 : 7, 105);
                f.claims(1);
                const SqlRows expected =
                    choice == 0   ? SqlRows{{1, 0, -99}, {1, 4, -1}, {2, 2, 15}}
                    : choice == 1 ? SqlRows{{1, 0, -99}, {1, 1, 1}, {2, 2, 15}}
                    : choice == 2 ? SqlRows{{1, 0, -99}, {1, 1, 1}, {2, 4, -2}, {3, 2, 15}}
                                  : SqlRows{{1, 0, -100}, {2, 2, 15}};
                require(
                    f.db.rows(
                        "SELECT request_ordinal,component_kind,units FROM subathon_app_components "
                        "WHERE units<>0 ORDER BY request_ordinal,component_kind") == expected,
                    "60A conserves each signed debit as Main minus Debt plus unapplied in ordinal "
                    "order");
            }
            f.db.write([&](const auto &s) {
                const auto row = SqliteApplicationHoldStore(s).find(hold.hold_ref);
                require(row && row->current_version.state == (choice == 4
                                                                  ? ApplicationHoldState::active
                                                                  : ApplicationHoldState::resolved),
                        "Only a genuine rejected or resolved choice closes the exact hold");
                if (choice != 4)
                    require(row->current_version.parent_version_ref == hold.version_ref,
                            "Closure is the immediate exact active-version successor");
            });
        });
    }

    void finishedPartitionAndRetainedFinancialBasis_data() {
        QTest::addColumn<bool>("continuation");
        QTest::newRow("hard-end-equality-through-pause") << false;
        QTest::newRow("whole-original-held-then-finished") << true;
    }

    void finishedPartitionAndRetainedFinancialBasis() {
        QFETCH(bool, continuation);
        check([&] {
            using Policy = core::RefundDeficitPolicy;
            Case f(99, 7);
            const auto signed_time = continuation ? -100 : 20;
            auto original =
                f.seed("terminal",
                       {finance::request(finance::Unit::milliseconds, signed_time, "main"),
                        finance::request(finance::Unit::micro_points, 15, "score"),
                        finance::request(finance::Unit::whole_count, 1, "count")},
                       false, true);
            f.db.reopen();
            auto service = f.service();
            f.facts.current->timer.hard_end_utc_ms = 50;
            f.facts.current->clock.paused = true;
            std::optional<ReviewTimerSelector> held;
            if (continuation) {
                f.facts.current->signed_original = {{1, Policy::RequireReview}};
                const auto start =
                    Case::bind("terminal-hold", {original}, f.preview(service, {original}));
                const auto result = Case::execute(service, start);
                expected_row(Case::reply(result), 1, D::committed, S::accepted_pending);
                const auto &data = Case::reply(result).rows.at(0).outcome->facts;
                require(data && data->timer && data->timer->signed_request == -100,
                        "A real whole-original hold predates Finished");
                held = data->timer;
                original.verb = ReviewVerb::continue_timer;
                original.pending_decision_ref = start.entries.front().decision_ref;
                original.timer = held;
                original.deficits = std::vector<ReviewDeficitChoice>{};
                f.head(99, 7, 90);
                f.claims(0);
            }
            f.facts.current->clock.utc_now_ms = 50;
            if (continuation) {
                const auto blocked = f.preview(service, {original});
                require(blocked[0].facts && !blocked[0].facts->may_apply,
                        "Missing current independent sibling policy blocks whole continuation");
                f.claims(0);
                f.db.write([&](const auto &s) {
                    const auto hold = SqliteApplicationHoldStore(s).find(held->locator.hold_ref);
                    require(hold && hold->current_version.state == ApplicationHoldState::active &&
                                finance::same(hold->current_version.locator, held->locator),
                            "Missing sibling policy leaves the exact version active");
                });
            }
            f.facts.current->finished_siblings = std::vector{finance::Unit::micro_points};
            const auto prepared = f.preview(service, {original});
            auto command = Case::bind("terminal-apply", {original}, prepared);
            command.entries.front().decision_ref = "finished-decision";
            const auto result = Case::execute(service, command);
            expected_row(Case::reply(result), 1, D::committed,
                         continuation ? S::timer_resolved : S::applied);
            const auto &data = Case::reply(result).rows.at(0).outcome->facts;
            require(data && data->bundle,
                    "One committed partition has a real original bundle link");
            const auto key = *data->bundle;
            f.head(99, 7, 105);
            f.claims(1);
            require(
                f.db.rows("SELECT s.lifecycle FROM subathon_app_states s JOIN subathon_app_head h "
                          "ON s.token=h.token") == SqlRows{{2}},
                "Hard end at equality finishes through pause separately from effect deltas");
            require(
                f.db.rows("SELECT t.request_ordinal,t.disposition,r.requested_units "
                          "FROM subathon_review_terminal_requests t JOIN subathon_plan_requests r "
                          "ON r.plan_ref=t.plan_ref AND r.revision=t.plan_revision AND "
                          "r.ordinal=t.request_ordinal ORDER BY t.request_ordinal") ==
                    SqlRows{{1, 1, signed_time}, {3, 2, 1}},
                "60A records exact signed time and policy-suppressed count at original ordinals");
            require(
                f.db.rows(
                    "SELECT request_ordinal,component_kind,units FROM subathon_app_components") ==
                    SqlRows{{2, 2, 15}},
                "Only the physical score sibling has positive actual credit");
            const auto persisted =
                f.db.rows("SELECT format_ref,document,effect_count,component_count "
                          "FROM subathon_app_results");
            require(
                persisted.size() == 1 &&
                    persisted[0][0].toByteArray() == "subathon.internal.application-result.v2" &&
                    persisted[0][1].toByteArray().startsWith("SAR2") &&
                    persisted[0][1].toByteArray().endsWith(
                        QByteArray::fromHex("0000000300000002010002")) &&
                    persisted[0][2].toInt() == 1 && persisted[0][3].toInt() == 1,
                "SAR2 has three immutable requests, exactly two terminal rows, and mask [1,0,2]");
            FinancialRefundCommand refund;
            f.db.write([&](const auto &s) {
                const SqliteApplicationBundleStore store(s);
                require(!store.read_reversal_basis({key, 1, finance::CK::main_delta}).basis &&
                            !store.read_reversal_basis({key, 3, finance::CK::whole_count}).basis,
                        "Neither terminal ordinal can produce a reversal basis");
                const auto score = store.read_reversal_basis({key, 2, finance::CK::score});
                require(score.basis && score.basis->actual_credit_units == 15 &&
                            score.basis->effect.request_ordinal == 2,
                        "Sparse physical score resolves at original ordinal 2");
                const auto claim =
                    store.lookup_original_claim({ApplicationSubjectKind::contribution,
                                                 original.selector.receipt, original.selector.cid});
                require(claim && finance::same(claim->existing.intent.operation, key),
                        "Original claim read validates and retains the partition");
                const ApplicationPlanKey plan{"terminal-plan", 1};
                const auto inputs = store.read_inputs(plan, ApplicationSubjectKind::contribution);
                require(inputs.inputs && inputs.inputs->current_contribution,
                        "Current persisted contribution is selected before staging #199 refund");
                refund = {{"refund", "one-fifth"},
                          {0},
                          VerifiedRefundCorrelation{{{"review-fixture", "account"},
                                                     {"payment-v1", "payment"},
                                                     original.selector.receipt},
                                                    {"refund-v1", "one-fifth"},
                                                    "verified-synthetic",
                                                    original.selector.cid,
                                                    key,
                                                    plan},
                          {"one-fifth-financial", 1},
                          0,
                          inputs.inputs->current_state.token,
                          inputs.inputs->current_contribution->contribution.state,
                          finance::principal,
                          finance::audit("one-fifth")};
                if (continuation) {
                    const auto closed = SqliteApplicationHoldStore(s).find(held->locator.hold_ref);
                    require(
                        closed && closed->current_version.state == ApplicationHoldState::resolved &&
                            closed->current_version.parent_version_ref == held->locator.version_ref,
                        "Finished whole continuation closes the exact ordinary hold once");
                }
            });
            refund.receipt =
                f.intake("refund-receipt", core::ReceiptMoney{"4.00", "EUR"}, "refund").id;
            const auto fifth = finance::fraction(1, 5);
            const FinancialEntitlement candidate{
                0,     4,  fifth,
                false, {}, {{1, {key, 2, finance::CK::score}, 0, 0, 3, -3, fifth, 3, 0, false}}};
            finance::Authority authority;
            finance::Effects effects;
            const auto domain = [&] {
                return std::array{
                    f.db.rows("SELECT * FROM subathon_app_head"),
                    f.db.rows("SELECT * FROM subathon_app_operations ORDER BY "
                              "operation_scope,operation_ref"),
                    f.db.rows("SELECT * FROM subathon_app_results ORDER BY "
                              "operation_scope,operation_ref"),
                    f.db.rows("SELECT * FROM subathon_financial_refunds"),
                    f.db.rows("SELECT * FROM subathon_financial_bases"),
                    f.db.rows("SELECT * FROM subathon_financial_payment_heads"),
                    f.db.rows("SELECT * FROM subathon_plan_revisions ORDER BY plan_ref,revision")};
            };
            const auto before = domain();
            auto with_terminal_time = candidate;
            with_terminal_time.time = GroupedRefundEntitlement{
                {key, 1, finance::CK::main_delta}, 1, 0, {}, {4, 0}, 4, 0, -4, 0};
            with_terminal_time.components.front().request_ordinal = 2;
            auto with_terminal_count = candidate;
            with_terminal_count.components.push_back(
                {2, {key, 3, finance::CK::whole_count}, 0, 0, 0, 0, fifth, fifth, fifth, false});
            for (const auto &terminal : {with_terminal_time, with_terminal_count}) {
                f.db.write([&](const auto &s) {
                    const auto outcome =
                        stage_financial_refund(s, refund, terminal, authority, effects);
                    require(outcome.disposition == FinancialDisposition::invalid_financial_input &&
                                std::holds_alternative<std::monostate>(outcome.record),
                            "#199 refuses a candidate referencing a terminal zero-credit ordinal");
                });
                require(domain() == before && effects.observations == 0 && effects.adjustments == 0,
                        "Refused terminal credit writes no plan, financial record, or application");
            }
            f.db.write([&](const auto &s) {
                const auto outcome =
                    stage_financial_refund(s, refund, candidate, authority, effects);
                require(outcome.disposition == FinancialDisposition::applied,
                        "Actual #199 refund takes one fifth of +15 from sparse original ordinal 2");
            });
            f.head(99, 7, 102);
            f.claims(1);
            require(
                f.db.rows("SELECT original_ordinal,kind,position FROM subathon_financial_bases") ==
                    SqlRows{{2, 2, 1}},
                "Only the real score ordinal becomes a #199 basis link");
            f.db.write([&](const auto &s) {
                const auto history = SqliteFinancialRefundStore(s).lookup(refund.operation);
                require(history && history->entitlement.components.size() == 1 &&
                            history->entitlement.components.front().original.request_ordinal == 2,
                        "Committed #199 history traversal reconstructs the same sparse original "
                        "ordinal");
            });
        });
    }

    void creditedOriginalReviewPreservesItsFinancialHold() {
        check([] {
            CreditedOriginal current{"mixed-refund"};
            ContributionReview review{*current.file.writer, current.facts};
            const auto execute = [&](std::string key, ReviewVerb verb, S status) {
                auto command = CreditedOriginal::bind(review, std::move(key), current.intent(verb));
                const auto result = Case::execute(review, std::move(command));
                expected_row(Case::reply(result), 1, D::committed, status);
            };
            const auto domain = current.protected_rows();
            const auto ordinary = current.file.rows("SELECT * FROM subathon_hold_versions "
                                                    "ORDER BY hold_ref,position");
            const auto expect_financial = [&] {
                const auto active = current.file.financial_active();
                require(
                    current.financial && active.size() == 1 &&
                        finance::same(active.front().locator, *current.financial) &&
                        current.protected_rows() == domain,
                    "Review preserves the exact generated financial version, claim and effects");
                current.file.check_head(50000, 0, 30000000, 7);
            };
            const auto expect_all_active = [&] {
                expect_financial();
                current.expect_ordinary(ApplicationHoldState::active, "v2");
                require(current.file.rows("SELECT * FROM subathon_hold_versions "
                                          "ORDER BY hold_ref,position") == ordinary,
                        "The ordinary hold has no new version on a review-only outcome");
            };

            execute("credited-accept", ReviewVerb::accept, S::ineligible);
            expect_all_active();
            execute("credited-reject", ReviewVerb::reject, S::ineligible);
            expect_all_active();

            const auto expected_basis_rows = current.nonfinancial_basis_rows();
            const auto closing = current.intent(ReviewVerb::reject_held);
            for (const auto &source : {std::optional<std::string>{},
                                       std::optional<std::string>{"prior-financial-refund"}}) {
                CreditedOriginal prior{source.value_or("mixed-refund"), source.has_value()};
                require(prior.nonfinancial_basis_rows() == expected_basis_rows,
                        "Alternate refund preserves the exact current state, contribution, plan, "
                        "claim and ordinary versions");
                require(
                    prior.financial.has_value() == source.has_value() && current.financial &&
                        (!prior.financial || !finance::same(*prior.financial, *current.financial)),
                    "Alternate genuine refund omits the financial hold or yields a different "
                    "exact financial locator");
                ContributionReview prior_review{*prior.file.writer, prior.facts};
                const auto stale = CreditedOriginal::bind(
                    prior_review, source ? "stale-financial" : "missing-financial", closing);
                const auto result = Case::execute(review, stale);
                expected_row(Case::reply(result), 1, D::committed, S::stale);
                expect_all_active();
            }

            execute("credited-retain", ReviewVerb::retain_held, S::retained);
            expect_all_active();
            execute("credited-reject-held", ReviewVerb::reject_held, S::rejected);
            current.expect_ordinary(ApplicationHoldState::resolved, "v3");
            expect_financial();
        });
    }

    void shippedLinkedRefundBoundaryPreservesFinancialHold() {
        check([] {
            finance::File f;
            const auto locator = seed_mixed_refund(f, "mixed-refund", true).front().locator;
            auto first = f.ordinary_detection("ordinary-a", false);
            (void)f.detect(first);
            first.request.mutation_ref = "ordinary-a-new-version";
            first.request.expected_version_ref = "v1";
            first.request.proposed_version_ref = "v2";
            f.write([&](const auto &s) {
                const auto next = SqliteApplicationHoldStore(s).record_detection(first);
                require(
                    next.disposition == SqliteHoldMutationDisposition::recorded,
                    "Real ordinary hold advances to its second active version before migration");
            });
            const ApplicationHoldVersionLocator h1{"ordinary-a", "v2"};
            const auto h2 = f.detect(f.ordinary_detection("ordinary-b", false));
            f.check_head(50000, 0, 30000000, 7);
            report(migrate(*f.writer, request(MigrationMode::advance,
                                              milestone_target_edits_repository_manifest())),
                   CatalogState::current, 18, 18, 3);
            report(migrate(*f.writer, request(MigrationMode::advance, literal_v19())),
                   CatalogState::current, 19, 19, 1);
            std::vector active{h1, h2, locator};
            std::ranges::sort(active, {}, &ApplicationHoldVersionLocator::hold_ref);
            ApplicationHoldResolutionBatch batch{
                "linked-closure",
                {f.cid},
                active,
                {{h1.hold_ref, h1.version_ref, "v3", finance::document(), {}},
                 {h2.hold_ref, h2.version_ref, "v2", finance::document(), {}}},
                "tester",
                "ordinary-only-closure",
                "synthetic owning eligibility",
                finance::document(),
                f.ordinary_link(false, h1)};
            const auto apply = [&](const auto &candidate) {
                return f
                    .run_reply([&](const auto &s) {
                        return stage_financial_aware_linked_resolution(s, candidate);
                    })
                    .disposition;
            };
            auto wrong = batch;
            std::erase_if(wrong.expected_active,
                          [&](const auto &h) { return finance::same(h, locator); });
            require(apply(wrong) == FinancialDisposition::stale_hold,
                    "Missing financial locator invalidates the complete raw-order mixed set");
            wrong = batch;
            for (auto &h : wrong.expected_active)
                if (finance::same(h, locator))
                    h.version_ref += "-stale";
            require(apply(wrong) == FinancialDisposition::stale_hold,
                    "Stale financial version also invalidates the mixed set");
            wrong = batch;
            wrong.entries.pop_back();
            require(apply(wrong) == FinancialDisposition::refused,
                    "Partial ordinary closure is refused even with an exact financial member");
            wrong = batch;
            auto replacement = f.ordinary_detection("ordinary-replacement", false);
            replacement.cause.supersedes_hold_ref = h2.hold_ref;
            wrong.entries.back().replacement = replacement;
            require(apply(wrong) == FinancialDisposition::refused,
                    "A replacement cannot be smuggled into whole-original linked closure");
            require(apply(batch) == FinancialDisposition::ordinary_resolved,
                    "Shipped linked boundary closes all exact ordinary members at v19");
            const auto remaining = f.financial_active();
            require(remaining.size() == 1 && finance::same(remaining.front().locator, locator),
                    "Real v15 financial hold identity and version survive unchanged");
            f.write([&](const auto &s) {
                const SqliteApplicationHoldStore holds(s);
                for (const auto &h : {h1, h2}) {
                    const auto row = holds.find(h.hold_ref);
                    require(row && row->current_version.state == ApplicationHoldState::resolved &&
                                row->current_version.parent_version_ref == h.version_ref,
                            "Both ordinary closures reference their exact immutable predecessors");
                }
            });
            f.check_head(50000, 0, 30000000, 7);
            const auto history = f.history();
            require(history && history->position == 1,
                    "Review boundary never replays a financial refund");
        });
    }

    void budgetSpecimenSingleRealUnit_data() {
        QTest::addColumn<bool>("apply");
        QTest::newRow("review-only-with-real-valuation-blocker") << false;
        QTest::newRow("bundle-and-readback") << true;
    }

    void budgetSpecimenSingleRealUnit() {
        QFETCH(bool, apply);
        check([&] {
            Case f;
            std::vector<core::RequestedEffect> requests;
            if (apply) {
                auto timer = finance::request(finance::Unit::milliseconds, 2, "main");
                timer.exact_requested_units = finance::fraction(5, 2);
                requests.push_back(std::move(timer));
            }
            const auto intent = f.seed("one-budget-unit", requests, !apply, true);
            f.db.reopen();
            auto service = f.service();
            const auto command = Case::bind("budget", {intent}, f.preview(service, {intent}));
            f.trace = {};
            const auto result = Case::execute(service, command);
            expected_row(Case::reply(result), 1, D::committed,
                         apply ? S::applied : S::accepted_pending);
            const auto completed = f.trace.completions();
            require(completed.size() == 1 && completed[0].ordinal == 1 &&
                        completed[0].transaction == TransactionOutcome::committed &&
                        completed[0].error == WorkError::none && f.trace.works() <= 4,
                    "Qualifying profile has one acknowledged row, at most one qualification, "
                    "and bounded initial and final reads");
            f.prefix(2, {{1, int(apply ? S::applied : S::accepted_pending), blob8(1)}});
            f.head(apply ? 97 : 95, 0, 90);
            f.claims(apply ? 1 : 0);
            if (apply)
                require(f.db.rows(
                            "SELECT exact_numerator,exact_denominator,requested_units,"
                            "residual_numerator,residual_denominator FROM subathon_app_effects") ==
                            SqlRows{{QByteArray("5"), QByteArray("2"), 2, QByteArray("1"),
                                     QByteArray("2")}},
                        "Captured 5/2 independently rounds half-even to 2 with residual +1/2");
            else
                require(f.db.rows("SELECT * FROM subathon_app_effects").isEmpty(),
                        "Qualifying review-only work has no effect or claim");
        });
    }

    void mixedSixteenWithRealApplyAndUnavailableSelectors() {
        check([] {
            Case f;
            std::vector<ReviewEntryIntent> entries;
            for (std::uint64_t i = 1; i <= 16; ++i) {
                const auto ref = "mixed-" + std::to_string(i);
                if (i % 2 != 0) {
                    entries.push_back(f.seed(ref, Case::positive()));
                } else {
                    const auto unlinked = f.intake(ref);
                    ReviewEntryIntent entry;
                    entry.selector = {{999 + i}, unlinked.id};
                    entry.milestones = std::vector<MilestoneAcceptanceChoice>{};
                    entries.push_back(std::move(entry));
                }
            }
            std::ranges::reverse(entries);
            f.db.reopen();
            auto service = f.service();
            const auto command = Case::bind("mixed-sixteen", entries, f.preview(service, entries));
            f.trace = {};
            const auto result = Case::execute(service, command);
            for (int i = 1; i <= 16; ++i)
                expected_row(Case::reply(result), i, D::committed,
                             i % 2 != 0 ? S::applied : S::ineligible);
            const auto completed = f.trace.completions();
            require(
                completed.size() == 16 && f.trace.works() <= 34 &&
                    std::ranges::all_of(completed,
                                        [](const auto &e) {
                                            return e.transaction == TransactionOutcome::committed &&
                                                   e.error == WorkError::none;
                                        }) &&
                    f.db.rows("SELECT next_ordinal FROM subathon_review_work") == SqlRows{{17}},
                "Sixteen terminal writer units include real applies within the owned work bound");
            f.head(100, 0, 146); // 90 + 8*7, Main consumes its five available milliseconds.
            f.claims(8);
            require(f.db.rows(
                        "SELECT sum(units) FROM subathon_app_components WHERE component_kind=5") ==
                        SqlRows{{27}},
                    "Eight requests total 32 ms, of which exactly 27 ms is cap-limited");
        });
    }

    void acceptedPendingNeedsCurrentQuoteAndNewApplyKey() {
        check([] {
            Case f;
            auto intent = f.seed("requires-quote", {}, true, true);
            f.db.reopen();
            auto service = f.service();
            const auto first =
                Case::bind("review-missing-rate", {intent}, f.preview(service, {intent}));
            const auto accepted = Case::execute(service, first);
            expected_row(Case::reply(accepted), 1, D::committed, S::accepted_pending);
            f.head(95, 0, 90);
            f.claims(0);
            const auto confirmed =
                f.intake("confirmation", core::ReceiptMoney{"20.0000", "EUR"}, "confirmation");
            const auto work = qt_test_support::run_work(
                f.db.writer(),
                [&](auto context) {
                    SqliteContributionRepository repository(context);
                    const auto linked =
                        ContributionEvidenceService(repository)
                            .observe({confirmed,
                                      ContributionObservationKind::confirmation,
                                      core::PaymentEvidence::confirmed,
                                      VerifiedActivityLink{{confirmed.identity.origin,
                                                            "fixture-support", "requires-quote"},
                                                           "confirmed-synthetic"},
                                      {}});
                    require(
                        linked.disposition == ContributionLinkDisposition::linked &&
                            linked.contribution_id == intent.selector.cid,
                        "Real confirmation links to the existing contribution, not a new anchor");
                    return 1;
                },
                10000);
            finance::normal(work);
            const auto replay = Case::execute(service, first);
            expected_row(Case::reply(replay), 1, D::existing, S::accepted_pending);
            f.head(95, 0, 90);
            f.claims(0);

            // Synthetic owning rate resolution is a separate real committed fact.
            // The apply below owns the authorized append and its exact parent CAS.
            std::optional<core::CapturedPlanRevision> revised;
            const std::string plan = "requires-quote-plan";
            f.db.write([&](const auto &s) {
                const auto source = SqliteApplicationPlanStore(s).find_revision(plan, 1);
                require(source && source->revision.captured.valuation.state ==
                                      core::PlanValuationState::missing_historical,
                        "Original immutable arrival still has no historical quote");
                revised = source->revision;
                require(SqliteRetainedContextStore(s).retain(
                            {{RetainedKind::arrival_context, "independent-quote"},
                             "synthetic authorized quote resolution",
                             ConfigurationReference{"cfg"}}) == RetainDisposition::inserted,
                        "Persist the synthetic owning rate evidence independently of review");
                (void)s.execute(
                    {"UPDATE subathon_contributions SET resolution_ref=?,missing_rate=0 "
                     "WHERE contribution_id=?",
                     {QByteArray("child-quote"), blob8(intent.selector.cid.value)}});
            });
            f.head(95, 0, 90);
            f.claims(0);
            revised->revision = 2;
            revised->parent_revision = 1;
            auto &capture = revised->captured;
            capture.contribution_state.conditions.valuation.missing_rate = false;
            capture.contribution_state.conditions.valuation.resolution_ref = "child-quote";
            capture.contribution_state.conditions.review = core::ContributionReview::accepted;
            capture.valuation = {core::PlanValuationState::resolved,
                                 core::HistoricalValuation{"child-quote",
                                                           *capture.event.facts.money,
                                                           "CHF",
                                                           "1.75",
                                                           {},
                                                           "independent-quote",
                                                           "synthetic-rates",
                                                           40,
                                                           41,
                                                           {}}};
            auto request = finance::request(finance::Unit::milliseconds, 4, "main");
            request.exact_requested_units = finance::fraction(7, 2);
            request.requires_historical_valuation = true;
            capture.requested_effects = {request};
            revised->override_decision =
                core::PlanOverrideDecision{core::OverrideAuthority::authorized, "operator",
                                           "retained synthetic quote", "apply-quote"};
            intent.verb = ReviewVerb::apply_accepted;
            intent.pending_decision_ref = first.entries.front().decision_ref;
            intent.revision = std::move(revised);
            auto fresh =
                Case::bind("fresh-apply-with-rate", {intent}, f.preview(service, {intent}));
            fresh.entries.front().decision_ref = "apply-quote";
            fresh.entries.front().reason = "retained synthetic quote";
            auto conflicting = fresh;
            conflicting.key = first.key;
            refusal(Case::execute(service, conflicting), ReviewRefusal::identity_conflict);
            f.claims(0);
            const auto applied = Case::execute(service, fresh);
            expected_row(Case::reply(applied), 1, D::committed, S::applied);
            f.head(99, 0, 90);
            f.claims(1);
            require(
                f.db.rows("SELECT plan_revision,exact_numerator,exact_denominator,requested_units,"
                          "residual_numerator,residual_denominator FROM subathon_app_effects") ==
                    SqlRows{{blob8(2), QByteArray("7"), QByteArray("2"), 4, QByteArray("-1"),
                             QByteArray("2")}},
                "Authorized immediate child rounds 7/2 to 4 with exact residual -1/2");
            f.db.write([&](const auto &s) {
                const auto history = SqliteApplicationPlanStore(s).find(plan);
                require(
                    history && history->revisions.size() == 2 &&
                        history->revisions[0].captured.valuation.state ==
                            core::PlanValuationState::missing_historical &&
                        history->revisions[1].parent_revision == 1 &&
                        history->revisions[1].captured.event ==
                            history->revisions[0].captured.event,
                    "Immediate child preserves the original arrival, unknown quote and revision");
            });
        });
    }

    void preflightFanoutAndLiveRowBudgetStopAreDistinct() {
        check([] {
            Batch batch;
            auto service = batch.service();
            const auto command = batch.command(service);
            const auto oversized = [&] {
                auto &jobs = batch.facts.current->internal_jobs;
                for (int i = 0; i < 769; ++i)
                    jobs.push_back({"bounded-fanout-" + std::to_string(i),
                                    {},
                                    {},
                                    {},
                                    {delivery_job_format, "synthetic internal"}});
            };
            oversized();
            refusal(Case::execute(service, command), ReviewRefusal::batch_too_large);
            require(batch.trace.ordinals(U::row, P::before_enqueue).empty(),
                    "A known oversize SQL fanout fails in read-only qualification");
            batch.empty_journal();

            batch.facts.current->internal_jobs.clear();
            batch.trace = {};
            batch.trace.control = [&](const auto &e) {
                if (e.unit == U::row && e.ordinal == 1 && e.phase == P::completed)
                    oversized();
            };
            const auto stopped = Case::execute(service, command);
            const auto &reply = Case::reply(stopped);
            require(reply.stop == ReviewStop::row_budget_exceeded &&
                        batch.trace.ordinals(U::final_read, P::began) == std::vector{0},
                    "An unbounded live shape after the normal prefix has a separate stop and one "
                    "final read");
            expected_row(reply, 1, D::committed, S::applied);
            expected_row(reply, 2, D::unattempted);
            expected_row(reply, 3, D::unattempted);
            batch.prefix_one();
        });
    }

    void finalReconciliationAllocationFailure_data() {
        QTest::addColumn<int>("predecessor");
        QTest::newRow("all-rows-committed") << int(AllocationPredecessor::last_row);
        QTest::newRow("row-two-rolled-back") << int(AllocationPredecessor::row_rollback);
        QTest::newRow("live-row-budget") << int(AllocationPredecessor::live_budget);
    }

    void finalReconciliationAllocationFailure() {
        QFETCH(int, predecessor);
        check([&] {
            const auto scenario = AllocationPredecessor(predecessor);
            const int applied = scenario == AllocationPredecessor::last_row ? 3 : 1;
            QList<SqlRows> control_snapshot;
            {
                Batch control;
                const auto attempt = observe_allocation(control, scenario, false);
                control.db.reopen();
                require(attempt.delivered && attempt.observed->callbacks == 1 &&
                            attempt.observed->on_owner,
                        "A disarmed attempt completes once on the owner through drain");
                expect_allocation_reply(control, attempt, scenario, false);
                expect_allocation_durable(control, attempt.command, applied);
                control_snapshot = allocation_snapshot(control);
            }

            Batch batch;
            const auto attempt = observe_allocation(batch, scenario, true);
            expect_allocation_durable(batch, attempt.command, applied);
            const auto before_retry = allocation_snapshot(batch);
            require(before_retry == control_snapshot,
                    "The allocation fault preserves the exact durable rows of its control");

            QVERIFY2(attempt.delivered,
                     "Final reconciliation allocation fault must deliver one owned component "
                     "callback by the deadline");
            require(attempt.observed->callbacks == 1 && attempt.observed->on_owner &&
                        attempt.observed->in_predecessor_turn,
                    "The owned result arrives once on the owner in the predecessor continuation");
            expect_allocation_reply(batch, attempt, scenario, true);

            const auto manifest = batch.db.rows("SELECT manifest FROM subathon_review_work");
            const auto first =
                batch.db.rows("SELECT * FROM subathon_review_entries WHERE ordinal=1");
            const auto fault_events = attempt.observed->unit_events;
            batch.trace = {};
            batch.facts.current->internal_jobs.clear();
            // Keep the original writer so fresh admission exercises the same active lease.
            ContributionReview service{
                batch.db.writer(), batch.facts,
                FinalReviewAllocationObserver{
                    [&batch](const ReviewUnitEvent &event) { batch.trace.observe(event); },
                    attempt.fault}};
            const auto replayed = Case::execute(service, attempt.command, "refreshed");
            const auto &reply = Case::reply(replayed);
            require(!reply.stop && reply.rows.size() == 3,
                    "Freshly authorized exact replay completes the full selection");
            for (int ordinal = 1; ordinal <= 3; ++ordinal)
                expect_applied_link(batch, reply, attempt.command, ordinal,
                                    ordinal <= applied ? D::existing : D::committed);
            const auto suffix = applied == 3 ? std::vector<int>{} : std::vector{2, 3};
            batch.db.reopen();
            require(attempt.observed->callbacks == 1 &&
                        attempt.observed->unit_events == fault_events && attempt.fault->hits == 1 &&
                        batch.trace.ordinals(U::row, P::before_enqueue) == suffix &&
                        batch.trace.ordinals(U::row, P::published) == suffix,
                    "Drain preserves one fault callback and only fresh suffix writes publish");
            expect_allocation_durable(batch, attempt.command, 3);
            require(batch.db.rows("SELECT manifest FROM subathon_review_work") == manifest &&
                        batch.db.rows("SELECT * FROM subathon_review_entries WHERE ordinal=1") ==
                            first,
                    "Refreshed recovery retains the exact original manifest and prefix");
            if (applied == 3)
                require(allocation_snapshot(batch) == before_retry,
                        "A fully committed replay preserves the complete durable snapshot");
        });
    }
};
QTEST_GUILESS_MAIN(ContributionReviewTests)
#include "contribution_review_tests.moc"
