// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_live_financial_refunds_fixture.hpp"
#include <QtEndian>
#include <array>
#include <subathon/core/time_credit.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace live_financial_refund_fixture;

template <class Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const LiveFinancialRefundNotImplemented &error) {
        QFAIL(qPrintable(QString("LiveFinancialRefundNotImplemented: ") + error.what()));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

const OriginalCredit grouped{.amount = "10",
                             .requested = 10,
                             .main = 6,
                             .repayment = 4,
                             .cap_tail = 0,
                             .score = 100,
                             .count = 8};
const OriginalCredit ordered{
    .amount = "10", .requested = 10, .main = 10, .cap_tail = 0, .score = 100, .count = 8};
const OriginalCredit capped{
    .amount = "10", .requested = 130, .main = 5, .cap_tail = 125, .score = 100, .count = 8};
const OriginalCredit scores{
    .amount = "400", .requested = 0, .main = 0, .cap_tail = 0, .score = 400, .count = 8000};
const std::vector<Transition> grace{Transition::MainExpired, Transition::GraceStarted};
const std::vector<Transition> immediate{Transition::MainExpired, Transition::Finished};
const std::vector<Transition> confirmation{Transition::MainExpired,
                                           Transition::ConfirmationRequired};
const std::vector<Transition> hard_end{Transition::HardEndReached, Transition::Finished};

FileSeed cap_seed() {
    const auto result = core::calculate_time_credit({55, 130, 60});
    require(result.applied_ms == 5 && result.remaining_after_ms == 60,
            "Original production cap calculator witnesses 5 ms actual of 130 attempted");
    return {.main_before = 55, .maximum_main = 60};
}

FinancialEntitlement order_quote(std::size_t i) {
    // Cumulative fractions .40/.41/.71/1 give rounded Main 4/4/7/10,
    // exact score 40/41/71/100 and whole count 0/0/0/8.
    constexpr std::array<Number, 5> paid{0, 40, 41, 71, 100};
    constexpr std::array<Number, 5> main{0, 4, 4, 7, 10};
    const auto before = paid.at(i), after = paid.at(i + 1);
    const auto f = fraction(after, 100);
    const auto full = i == 3;
    return quote(before, after, 100, full,
                 time(Number(i), {main[i], 0}, {main[i + 1], 0}, fraction(after, 10),
                      main[i] - main[i + 1], 0),
                 {link(CK::score, 2, 2, Number(i), before, after, f, after, full),
                  link(CK::whole_count, 3, 3, Number(i), 0, full ? 8 : 0, f, f * 8, full)});
}

void journal(File &f, Number position, core::Exact paid, std::size_t resolved_count = 0) {
    const auto h = f.history();
    require(h && h->position == position && h->accepted_money == paid &&
                h->refunds.size() == std::size_t(position) &&
                h->resolutions.size() == resolved_count,
            "Committed selected history has the exact payment position, money and closures");
}

QByteArray payload(File &f, bool resolution, const ApplicationOperationKey &key) {
    const auto rows = f.rows(
        resolution ? "SELECT document FROM subathon_financial_resolutions WHERE operation_scope=? "
                     "AND operation_ref=?"
                   : "SELECT document FROM subathon_financial_refunds WHERE operation_scope=? "
                     "AND operation_ref=?",
        {bytes(key.operation_scope), bytes(key.operation_ref)});
    require(rows.size() == 1 && rows[0][0].toByteArray().size() >= 8,
            "Exact persisted financial journal document");
    return rows[0][0].toByteArray();
}

void version(File &f, bool resolution, const ApplicationOperationKey &key, char expected) {
    require(payload(f, resolution, key).first(8) == QByteArray(7, '\0') + expected,
            "Private journal version is encoded in the first signed big-endian word");
}

struct OrderedPrefix {
    Fixture f{ordered};
    std::array<FinancialTimerHold, 3> holds;
    FinancialRefundRecord zero;

    explicit OrderedPrefix(bool legacy) {
        f.main("main-to-two", 2);
        constexpr std::array refs{"g", "order-2", "b", "e"};
        constexpr std::array amounts{"4", "0.10", "3", "2.90"};
        constexpr std::array<Number, 4> scores_after{60, 59, 29, 0};
        constexpr std::array<Number, 4> requested{4, 0, 3, 3};
        for (std::size_t i = 0; i < refs.size(); ++i) {
            const auto c = f.command(refs[i], amounts[i], Number(i));
            const auto p = order_quote(i);
            Effects port;
            if (i == 1)
                port.apply(0, 0, {});
            auto r = legacy ? f.file.accept(c, p, port) : accepted(f.refund(c, p));
            require(r.payment_position == Number(i + 1) &&
                        r.entitlement.accepted_money == p.accepted_money &&
                        r.entitlement.previous_money == p.previous_money && r.entitlement.time &&
                        r.entitlement.time->main_request == -requested[i] &&
                        r.entitlement.time->residual == p.time->residual &&
                        r.timer == RefundTimerOutcome{i == 1 ? TD::applied : TD::held,
                                                      {0, 0, 0, requested[i], 0}},
                    "Each committed payment retains its exact requested progress and actual legs");
            f.file.check_head(i == 0 ? 2 : 10, 0, scores_after[i], i == 3 ? 0 : 8);
            if (i == 1) {
                require(!r.hold, "Zero whole timer request creates no hold behind the oldest");
                zero = r;
            } else {
                holds[i == 0 ? 0 : i - 1] =
                    require_hold(r, i == 0 ? HoldReason::deficit : HoldReason::prior_request,
                                 Number(i + 1), -requested[i], 0);
            }
            if (i == 0)
                f.main("independent-top-up", 10);
        }
        require(holds[0].locator.hold_ref > holds[1].locator.hold_ref &&
                    holds[1].locator.hold_ref > holds[2].locator.hold_ref,
                "Fixture's bytewise hold key order reverses the committed payment order");
        f.file.reopen();
        journal(f.file, 4, fraction(10));
        active(f.file, {holds[0], holds[1], holds[2]});
    }
};

ApplicationHoldResolutionBatch ordinary_batch(Fixture &f, const FinancialTimerHold &financial) {
    const auto a = f.file.detect(f.file.ordinary_detection("ordinary-a", false));
    const auto b = f.file.detect(f.file.ordinary_detection("ordinary-b", false));
    const auto c = f.file.continuation("ordinary-bound", financial, {a, b, financial.locator});
    return {"ordinary-clear",
            {f.file.cid},
            c.expected_active,
            {{a.hold_ref, a.version_ref, "v2", document(), {}},
             {b.hold_ref, b.version_ref, "v2", document(), {}}},
            "tester",
            "ordinary-clear",
            "independent linked ordinary acceptance",
            document(),
            f.file.ordinary_link(false, a)};
}

FinancialWorkReply apply_ordinary(File &f, const ApplicationHoldResolutionBatch &batch) {
    return f.run_reply(
        [&](const auto &s) { return stage_financial_aware_linked_resolution(s, batch); });
}

void ordinary_cleared(File &f, const ApplicationHoldResolutionBatch &batch) {
    require(apply_ordinary(f, batch).disposition == D::ordinary_resolved,
            "Linked operation closes exactly the ordinary side of the mixed set");
    f.write([&](const auto &s) {
        for (const auto &entry : batch.entries) {
            const auto h = SqliteApplicationHoldStore(s).find(entry.hold_ref);
            require(h && h->current_version.state == ApplicationHoldState::resolved &&
                        h->current_version.locator.version_ref == "v2",
                    "Ordinary version was closed, without new hold creation");
        }
    });
}

const MilestoneOccurrenceKey a{"completed-A"}, b{"completed-B"}, c{"completed-C"};

MilestoneTrackSeed::Member stage_group(Fixture &f, const MilestoneOccurrenceKey &occurrence,
                                       std::string id, Number target, Number score,
                                       MilestoneActionChoice choice, bool release) {
    const auto ref = "accept-" + id;
    const auto receipt = f.file.intake(ref, {}, "synthetic-milestone-owner");
    const core::MilestoneCompletion captured{id, "captured-definition", target, score, "ctx"};
    const ApplicationOperationKey origin{"test", ref};
    f.file.write([&](const auto &s) {
        auto bundle = File::receipt_application(s, receipt, ref);
        bundle.after = f.file.encoded(std::move(bundle.after), bundle.advanced_before.token);
        // Keep both the originating bundle and the captured group job in real stores.
        const auto q = request(Unit::micro_points, 0, "score");
        File::retain_plan(s, bundle.intent.plan, File::plain_capture(receipt, {}, {q}));
        bundle.effects.push_back(direct(1, q, CK::score));
        bundle.jobs.push_back({"job", {}, occurrence.ref, 1, {delivery_job_format, "job-payload"}});
        require(SqliteDeliveryJobStore(s)
                        .stage(bundle, {{"job", "fixture-route",
                                         DeliverySenderSelection::configured_default, "sender",
                                         "action", "v1"}})
                        .disposition == SqliteBundleStageDisposition::staged,
                "Committed originating bundle and exact delivery job capture");
        SqliteMilestoneGroupStore(s).stage(
            origin,
            {{{captured, {{MilestoneActionKind::automation, "action", "v1"}}, {}, occurrence},
              {{1, {origin, "job"}}}}});
        SqliteMilestoneGroupDecisions(s).record_acceptance(
            origin,
            {PausedAcceptanceMode::normal, {"tester", "choice-" + id}, {{occurrence, choice}}});
    });
    if (release) {
        const auto result = completed<MilestoneReleaseCompletion>([&](auto done) {
            return release_milestone_head(*f.file.writer, occurrence, std::move(done));
        });
        require(result.disposition == MilestoneReleaseDisposition::released &&
                    result.work.transaction == TransactionOutcome::committed &&
                    result.work.error == WorkError::none,
                "Release is a real committed action boundary while its job remains queued");
    }
    return {captured, origin, occurrence};
}

void track_seed(Fixture &f, std::vector<core::MilestoneTarget> targets,
                std::vector<MilestoneTrackSeed::Member> prefix) {
    f.file.write([&](const auto &s) {
        require(initialize_milestone_track(s, {track, "cfg", "initial-definition", "initial-prefix",
                                               std::move(targets), std::move(prefix)}),
                "Owning v18 store validates the definition and exact typed prefix membership");
    });
}

class TargetEditAuthority final : public MilestoneEditAuthority {
  public:
    std::optional<MilestoneEditFacts> select(const SqliteWorkSession &s,
                                             const MilestoneEditActor &actor,
                                             const MilestoneEditIntent &intent) override {
        const auto head = SqliteApplicationBundleStore(s).read_state();
        require(head && actor.actor_ref == "tester" && actor.session_lineage == "edit-session" &&
                    intent.key.track == track,
                "Correction authority selects the actual writer head and scoped track");
        return MilestoneEditFacts{
            head->configuration_ref,
            "current-captures",
            "target-correction-grant",
            {"target-edit-lifecycle", head->timer, head->time_debt_ms, {}, {}, {}, true, true},
            true,
            true,
            true,
            "edited-definition",
            "edited-prefix",
            {}};
    }
};

void edit_completed_target(Fixture &f, const std::string &id, Number target) {
    const MilestoneEditActor who{"tester", "edit-session"};
    const MilestoneEditIntent intent{
        {track, "edit-completed-target"}, {"tester", "edit-decision"}, {{id, target}}, {}};
    TargetEditAuthority authority;
    std::shared_ptr<const MilestoneEditBinding> binding;
    f.file.write([&](const auto &s) {
        const auto preview = preview_milestone_target_edit(s, authority, who, intent);
        require(preview.plan.status == core::MilestoneEditStatus::eligible &&
                    preview.plan.newly_completed.empty() && preview.confirmation,
                "Existing owner previews an actual correction of the completed target");
        binding = preview.confirmation;
    });
    f.file.write([&](const auto &s) {
        const auto edit = confirm_milestone_target_edit(s, authority, who, intent, *binding);
        require(edit.status == core::MilestoneEditStatus::eligible && edit.result &&
                    edit.result->definition_revision == "edited-definition" &&
                    edit.result->achievements.empty(),
                "Actual authorized edit commits the current target without repeating achievement");
    });
}

class LocalDelivery final : public DeliveryDispatchAuthority, public DeliveryAdapter {
  public:
    bool allows(const DeliveryJob &) override { return true; }
    void start(DeliveryJob job) override { claim = std::move(job); }
    std::optional<DeliveryJob> claim;
};

void retain_performed_job(Fixture &f, const ApplicationOperationKey &origin) {
    const auto adapter = std::make_shared<LocalDelivery>();
    const DeliveryJobKey key{origin, "job"};
    const auto dispatch = completed<DeliveryDispatchCompletion>([&](auto done) {
        return dispatch_delivery_job(*f.file.writer, key, 1, adapter, adapter, std::move(done));
    });
    normal(dispatch.work);
    require(adapter->claim && adapter->claim->attempt && dispatch.result &&
                dispatch.result->disposition == DeliveryDisposition::changed,
            "Actual committed delivery attempt is handed to a synthetic local adapter");
    f.file.write([&](const auto &s) {
        const auto finished = SqliteDeliveryJobStore(s).finish(
            key, adapter->claim->revision, *adapter->claim->attempt, DeliveryEvidence::delivered,
            "synthetic-local-evidence");
        require(finished.job && finished.job->state == DeliveryJobState::delivered,
                "Earlier performed action has independently committed delivery history");
    });
}

struct Milestones {
    MilestoneTrackSeed::Member first;
    MilestoneTrackSeed::Member second;
    Milestones(Fixture &f, bool released = false)
        : first(stage_group(f, a, "A", 600, 1100, MilestoneActionChoice::run, true)),
          second(stage_group(f, b, "B", 950, 1100,
                             released ? MilestoneActionChoice::run : MilestoneActionChoice::defer,
                             released)) {
        track_seed(f, {{"A", 600}, {"B", 950}, {"C", 2000}}, {first, second});
        edit_completed_target(f, "B", 1000);
        retain_performed_job(f, first.application);
    }
};

Number committed_score(File &file) {
    core::Exact result{};
    for (const auto &value : file.head().balances)
        if (value.kind == ApplicationBalanceKind::score)
            result += value.units;
    require(result >= minimum && result <= maximum, "Selected committed aggregate is signed64");
    return result.convert_to<Number>();
}

std::vector<core::MilestoneTarget> committed_targets(File &file) {
    const auto rows = file.rows(
        "SELECT t.milestone_ref,t.target_micro_points FROM subathon_milestone_definition_heads h "
        "JOIN subathon_milestone_definition_targets t "
        "USING(installation_ref,track_ref,definition_revision) "
        "WHERE h.installation_ref=? AND h.track_ref=? ORDER BY t.ordinal",
        {bytes(track.installation_ref), bytes(track.track_ref)});
    std::vector<core::MilestoneTarget> result;
    for (const auto &row : rows) {
        bool signed64 = false;
        const auto value = row[1].toLongLong(&signed64);
        require(!row[0].toByteArray().isEmpty() && signed64,
                "Selected persisted current target has an exact signed64 value");
        result.push_back({row[0].toByteArray().toStdString(), value});
    }
    return result;
}

core::MilestoneProgressionResult project(File &file, const Milestones &m) {
    return core::advance_milestone_progression({track.track_ref,
                                                "edited-definition",
                                                "ctx",
                                                committed_score(file),
                                                committed_targets(file),
                                                {m.first.completion, m.second.completion}});
}

const QString jobs = "SELECT * FROM subathon_delivery_events ORDER BY "
                     "operation_scope,operation_ref,job_ref,revision";
const QString groups = "SELECT * FROM subathon_milestone_groups ORDER BY queue_order";
const QString decisions = "SELECT * FROM subathon_milestone_refund_decisions ORDER BY refund_ref";

class SqliteLiveFinancialRefundsTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void l1ActualCapBasisAndLiveClock() {
        check([] {
            Fixture f(capped, cap_seed());
            f.main("elapsed-before-refund", 15);
            f.input.cap = 3;
            f.input.cuts = {{2, 97, false}, {10, 98, true}, {2, 100, false}};
            const auto initial = f.file.head().timer;
            auto observed = initial;
            observed.main_remaining_ms = 11;
            auto after = observed;
            after.main_remaining_ms = 6;
            const auto command = f.command("cap-refund", "10", 0);
            const auto result = accepted(f.refund(command, full_quote(10, 5, 0, 100, 8)));
            clock_and_state(f.file, result, initial, observed, after, {TD::applied, {5}}, 0);
            require(result.entitlement.time && result.entitlement.time->main_request == -5 &&
                        result.original_capture.captured.valuation.historical &&
                        result.original_capture.captured.requested_effects[0].requested_units ==
                            130 &&
                        result.application.after.balances[0].units == 0 &&
                        result.application.after.balances[1].units == 0,
                    "Refund selects actual historic 5, retaining the 130 request and rate capture");
            const auto frame = decode_reference_state(result.application.after);
            const auto &cuts = frame.observation.segments;
            require(cuts.size() == 3 && cuts[0].elapsed_ms == 2 && !cuts[0].paused &&
                        cuts[1].elapsed_ms == 10 && cuts[1].paused && cuts[2].elapsed_ms == 2 &&
                        !cuts[2].paused && frame.observation.observed_utc_ms == 100,
                    "Retained observation preserves the exact pause segmentation and UTC");
            journal(f.file, 1, fraction(10));
            version(f.file, false, command.operation, 2);
        });
    }

    void l2GroupedOverflowAndExhaustion_data() {
        QTest::addColumn<int>("policy");
        QTest::addColumn<bool>("fresh");
        QTest::newRow("shared-stop") << int(Policy::StopAtZero) << false;
        QTest::newRow("shared-carry") << int(Policy::CarryTimeDebt) << false;
        QTest::newRow("shared-review") << int(Policy::RequireReview) << false;
        QTest::newRow("fresh-stop") << int(Policy::StopAtZero) << true;
    }

    void l2GroupedOverflowAndExhaustion() {
        QFETCH(int, policy);
        QFETCH(bool, fresh);
        check([&] {
            Fixture f(grouped, {.debt_after = 7});
            f.main("earlier-elapsed", 4);
            const auto before = f.file.head().timer;
            f.input.policy = Policy(policy);
            f.input.config = {End::Grace, 19, fresh ? Reuse::Fresh : Reuse::Shared, {}};
            f.input.cuts = {{3, 100, true}};
            const auto cmd = f.command("paired", "10", 0);
            const auto r = accepted(f.refund(cmd, full_quote(10, 6, 4, 100, 8)));
            if (f.input.policy == Policy::RequireReview) {
                clock_and_state(f.file, r, before, before, before, {TD::held, {0, 0, 0, 6, 4}}, 7);
                active(f.file, {require_hold(r, HoldReason::deficit, 1, -6, 4)});
            } else {
                const bool carry = f.input.policy == Policy::CarryTimeDebt;
                clock_and_state(f.file, r, before, before,
                                {Life::Active, Phase::Grace, 0, fresh ? 19 : 9, false},
                                {TD::applied, {4, 4, carry ? 2 : 0, carry ? 0 : 2, 0}},
                                carry ? 13 : 11, grace);
                active(f.file, {});
            }
            require(r.application.reversals.size() == 2 &&
                        r.application.reversals[0].requested_increment == -100 &&
                        r.application.reversals[1].requested_increment == -8 &&
                        f.file.head().balances[0].units == 0 &&
                        f.file.head().balances[1].units == 0,
                    "Committed score and count reverse even with whole timer held");
            journal(f.file, 1, fraction(10));
        });
    }

    void l2HeldTimeStillCommitsScoreCountAndReview() {
        check([] {
            auto credit = grouped;
            credit.amount = "400";
            credit.score = 300;
            Fixture f(credit, {.debt_after = 7, .score_before = 100, .other_score = 700}, false);
            const Milestones m(f);
            f.main("main-for-held-review", 4);
            const auto before = f.file.head().timer;
            f.input.config = {End::Grace, 19, Reuse::Shared, {}};
            f.input.cuts = {{0, 100, true}};
            const auto cmd = f.command("held-review", "400", 0);
            const auto r = accepted(f.refund(cmd, full_quote(400, 6, 4, 300, 8)));
            clock_and_state(f.file, r, before, before, before, {TD::held, {0, 0, 0, 6, 4}}, 7);
            active(f.file, {require_hold(r, HoldReason::deficit, 1, -6, 4)});
            require(committed_score(f.file) == 800 && f.file.head().balances.back().units == 0 &&
                        r.milestone_observations.size() == 1 &&
                        r.milestone_observations.front().occurrence == b &&
                        f.file.rows(decisions).size() == 1,
                    "A whole timer hold still commits score, count and the independent B Review");
            journal(f.file, 1, fraction(400));
        });
    }

    void l3EqualityConfirmation() {
        check([] {
            Fixture equality(grouped, {.debt_after = 7});
            equality.main("retain-grace-nine", 6);
            equality.input.config.end_mode = End::Confirmation;
            const auto before = equality.file.head().timer;
            const auto q = full_quote(10, 6, 4, 100, 8);
            const auto r = accepted(equality.refund(equality.command("equality", "10", 0), q));
            clock_and_state(equality.file, r, before, before,
                            {Life::Active, Phase::Main, 0, 9, true}, {TD::applied, {6, 4}}, 11,
                            confirmation);
        });
    }

    void l3RestorationOnlyLeavesGrace() {
        check([] {
            Fixture restoration({.amount = "10",
                                 .requested = 4,
                                 .main = 0,
                                 .repayment = 4,
                                 .cap_tail = 0,
                                 .score = 0,
                                 .count = 0});
            const core::TimerState before{Life::Active, Phase::Grace, 0, 7, false};
            restoration.move("entered-grace", before, grace);
            restoration.input.config = {End::Grace, 7, Reuse::Shared, {}};
            const auto r = accepted(restoration.refund(restoration.command("restore-only", "10", 0),
                                                       full_quote(10, 0, 4, 0, 0)));
            clock_and_state(restoration.file, r, before, before, before, {TD::applied, {0, 4}}, 4);
        });
    }

    void l3CarryOverflowRefusesAndStopFits() {
        check([] {
            const auto q = full_quote(10, 6, 4, 100, 8);
            Fixture range(grouped, {.debt_after = maximum - 5});
            range.main("before-overflow", 4);
            range.input.policy = Policy::CarryTimeDebt;
            const auto overflow = range.command("carry-overflows", "10", 0);
            const auto head = range.file.head();
            refused(range.refund(overflow, q), D::refused);
            unchanged(range.file, overflow, head.token);
            range.input.policy = Policy::StopAtZero;
            const auto r = accepted(range.refund(range.command("stop-fits", "10", 0), q));
            clock_and_state(range.file, r, head.timer, head.timer,
                            {Life::Finished, Phase::Main, 0, 9, false}, {TD::applied, {4, 4, 0, 2}},
                            maximum - 1, immediate);
        });
    }

    void l4OldestFinancialRequestAndZeroBasis() {
        check([] {
            OrderedPrefix p(false);
            auto &f = p.f;
            const auto &h = p.holds;
            auto wrong = f.file.continuation("wrong-order", h[1], locators({h[0], h[1], h[2]}));
            refused(f.resolve(wrong), D::out_of_order);
            std::vector<ApplicationHoldVersionLocator> remaining = locators({h[0], h[1], h[2]});
            constexpr std::array<Number, 3> main_after{6, 3, 0};
            for (std::size_t i = 0; i < h.size(); ++i) {
                const auto cmd =
                    f.file.continuation("consume-" + std::to_string(i), h[i], remaining);
                const auto r = resolved(f.resolve(cmd), cmd, {TD::applied, {i == 0 ? 4 : 3}});
                const auto finished = i == 2;
                f.file.check_head(main_after[i], 0, 0, 0, finished ? Life::Finished : Life::Active);
                require(
                    r.transitions == (finished ? immediate : std::vector<Transition>{}),
                    "Only the last exact closure exhausts Main and emits lifecycle transitions");
                std::erase_if(remaining, [&](const auto &l) { return same(l, h[i].locator); });
                const auto current = f.file.financial_active();
                require(current.size() == remaining.size() &&
                            std::ranges::all_of(current,
                                                [&](const auto &hold) {
                                                    return std::ranges::any_of(
                                                        remaining, [&](const auto &l) {
                                                            return same(l, hold.locator);
                                                        });
                                                }),
                        "Resolution removes only the selected exact locator and keeps later holds");
                journal(f.file, 4, fraction(10), i + 1);
                require(by_ref(f.file, h[i].refund_operation).timer->disposition == TD::held,
                        "Continuation preserves immutable first acceptance");
            }
        });
    }

    void l5MixedSetAndCurrentAuthority() {
        check([] {
            Fixture f(ordered);
            f.main("main-to-two", 2);
            const auto initial = f.command("g", "4", 0);
            const auto first = accepted(f.refund(initial, order_quote(0)));
            const auto held = require_hold(first, HoldReason::deficit, 1, -4, 0);
            const auto head = f.file.head();
            const auto batch = ordinary_batch(f, held);
            const auto next = f.command("fits-money", "0.10", 1);
            refused(f.refund(next, order_quote(1)), D::active_hold);
            unchanged(f.file, next, head.token);
            const auto excess = f.command("over-money", "6.01", 1);
            refused(f.refund(excess, order_quote(1)), D::exceeds_original);
            unchanged(f.file, excess, head.token);
            journal(f.file, 1, fraction(4));

            auto cmd = f.file.continuation("mixed", held, batch.expected_active);
            auto incomplete = cmd;
            std::erase_if(incomplete.expected_active, [&](const auto &loc) {
                return loc.hold_ref == batch.entries[0].hold_ref;
            });
            refused(f.resolve(incomplete), D::stale_hold);
            auto stale = cmd;
            stale.expected_active.front().version_ref = "stale-version";
            refused(f.resolve(stale), D::stale_hold);
            refused(f.resolve(cmd), D::active_hold);
            ordinary_cleared(f.file, batch);
            active(f.file, {held});
            require(f.file.head().token == head.token,
                    "Ordinary resolution triggers no financial pickup or head move");

            f.input.config.end_mode = End::Confirmation;
            f.input.policy = Policy::CarryTimeDebt;
            cmd = f.file.continuation("finish-oldest", held, {held.locator}, Policy::CarryTimeDebt);
            f.file.authority.execution = false;
            refused(f.resolve(cmd), D::denied_execution);
            f.file.authority.execution = true;
            f.file.authority.disclosure = false;
            refused(f.resolve(cmd), D::denied_disclosure);
            f.file.authority.disclosure = true;
            f.file.authority.override_timer = false;
            refused(f.resolve(cmd), D::denied_execution);
            f.file.authority.override_timer = true;
            auto ambient = cmd;
            ambient.override_policy.reset();
            refused(f.resolve(ambient), D::deficit);
            require(f.file.head().token == head.token,
                    "Refusals preserve the exact committed head");
            active(f.file, {held});
            const auto closure = resolved(f.resolve(cmd), cmd, {TD::applied, {2, 0, 2}});
            require(same_timer(closure.after.timer, {Life::Active, Phase::Main, 0, 9, true}) &&
                        closure.transitions == confirmation,
                    "An authorized explicit Carry enters confirmation wait with retained Grace9");
            f.file.check_head(0, 2, 60, 8);
            active(f.file, {});
            journal(f.file, 1, fraction(4), 1);
            require(by_ref(f.file, initial.operation).timer == first.timer,
                    "No financial, score or count reapplication at resolution");
        });
    }

    void l5LaterHoldsSurviveExplicitOverride() {
        check([] {
            OrderedPrefix p(false);
            auto &f = p.f;
            f.main("elapsed-after-entitlement", 2);
            f.input.config.end_mode = End::Confirmation;
            f.input.policy = Policy::CarryTimeDebt;
            const auto &h = p.holds;
            const auto cmd = f.file.continuation(
                "override-first", h[0], locators({h[0], h[1], h[2]}), Policy::CarryTimeDebt);
            const auto r = resolved(f.resolve(cmd), cmd, {TD::applied, {2, 0, 2}});
            require(r.transitions == confirmation && r.after.timer.awaiting_confirmation,
                    "The oldest deficient request closes with explicit Carry and confirmation");
            f.file.check_head(0, 2, 0, 0);
            active(f.file, {h[1], h[2]});
            journal(f.file, 4, fraction(10), 1);
        });
    }

    void l6FinishPrecedence_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("hard-end-first-acceptance") << 0;
        QTest::newRow("hard-end-exact-continuation") << 1;
        QTest::newRow("already-finished-grace") << 2;
        QTest::newRow("already-finished-confirmation") << 3;
        QTest::newRow("one-before-hard-end-effect-finish") << 4;
        QTest::newRow("ready") << 5;
        QTest::newRow("hard-end-in-grace") << 6;
        QTest::newRow("hard-end-in-confirmation-wait") << 7;
    }

    void l6FinishPrecedence() {
        QFETCH(int, mode);
        check([&] {
            if (mode == 5) {
                auto credit = grouped;
                credit.ready = true;
                Fixture f(credit);
                const auto command = f.command("ready", "10", 0);
                const auto head = f.file.head();
                require(head.timer.lifecycle == Life::Ready, "Real selected producer is Ready");
                refused(f.refund(command, full_quote(10, 6, 4, 100, 8)), D::prerequisite);
                unchanged(f.file, command, head.token);
                require(f.input.calls == 0, "Ready refuses before live clock input selection");
                return;
            }
            Fixture f(grouped, {.debt_after = 7});
            f.main("main-before-finish", 4);
            const auto q = full_quote(10, 6, 4, 100, 8);
            if (mode >= 1 && mode <= 3) {
                const auto origin = f.command("hold-before-finish", "10", 0);
                // The genuine #199 writer supplies a committed v1 origin, so
                // continuation is itself the first new product entry here.
                Effects v1;
                const auto first = f.file.accept(origin, q, v1);
                const auto h = require_hold(first, HoldReason::deficit, 1, -6, 4);
                const auto old = f.file.head();
                f.move("coherent-owner-after-v1", old.timer);
                if (mode == 2)
                    f.move("grace-expired", {Life::Finished, Phase::Grace, 0, 0, false},
                           {Transition::MainExpired, Transition::GraceStarted,
                            Transition::GraceExpired, Transition::Finished});
                if (mode == 3) {
                    f.move("waiting-confirmation", {Life::Active, Phase::Main, 0, 9, true},
                           confirmation);
                    f.move("confirmed", {Life::Finished, Phase::Main, 0, 9, false},
                           {Transition::Finished});
                }
                f.input.config.hard_end_utc_ms = 100;
                f.input.cuts = {{5, 100, true}};
                const auto cmd = f.file.continuation("close-finished", h, {h.locator});
                const auto r = resolved(f.resolve(cmd), cmd, {TD::finished, {0, 0, 0, 6, 4}});
                require(
                    r.transitions == (mode == 1 ? hard_end : std::vector<Transition>{}) &&
                        r.after.time_debt_ms == 7 &&
                        r.after.timer.main_remaining_ms == (mode == 1 ? 4 : 0) &&
                        r.after.timer.lifecycle == Life::Finished &&
                        r.after.balances[0].units == old.balances[0].units &&
                        by_ref(f.file, origin.operation).timer == first.timer,
                    "Observed or already committed finish closes with no actual grouped effect");
                version(f.file, true, cmd.operation, 2);
                active(f.file, {});
                return;
            }

            if (mode == 6) {
                f.move("grace-before-hard-end", {Life::Active, Phase::Grace, 0, 7, false}, grace);
                f.input.config.end_mode = End::Grace;
            }
            if (mode == 7) {
                f.move("waiting-before-hard-end", {Life::Active, Phase::Main, 0, 9, true},
                       confirmation);
                f.input.config.end_mode = End::Confirmation;
            }
            const auto before = f.file.head().timer;
            f.input.config.hard_end_utc_ms = 100;
            f.input.policy = Policy::StopAtZero;
            f.input.cuts = {{0, mode == 4 ? 99 : 100, true}};
            const auto r = accepted(f.refund(f.command("finish-boundary", "10", 0), q));
            auto after = before;
            after.lifecycle = Life::Finished;
            after.awaiting_confirmation = false;
            if (mode == 4) {
                after.main_remaining_ms = 0;
                clock_and_state(f.file, r, before, before, after, {TD::applied, {4, 4, 0, 2}}, 11,
                                immediate);
            } else {
                clock_and_state(f.file, r, before, after, after, {TD::finished, {0, 0, 0, 6, 4}}, 7,
                                hard_end);
            }
            require(committed_score(f.file) == 0 && f.file.head().balances.back().units == 0,
                    "Observed finish does not veto eligible money and score/count");
            journal(f.file, 1, fraction(10));
        });
    }

    void l6ObservedFinishLeavesLaterFinancialHolds() {
        check([] {
            OrderedPrefix p(true);
            auto &f = p.f;
            f.move("coherent-owner-after-legacy-history", f.file.head().timer);
            const auto &h = p.holds;
            const auto main = f.file.head().timer.main_remaining_ms;
            f.input.config.hard_end_utc_ms = 100;
            f.input.cuts = {{5, 100, true}};
            const auto cmd = f.file.continuation("finish-exact-oldest", h[0],
                                                 locators({h[0], h[1], h[2]}), Policy::StopAtZero);
            f.file.authority.override_timer = false;
            refused(f.resolve(cmd), D::denied_execution);
            active(f.file, {h[0], h[1], h[2]});
            f.file.authority.override_timer = true;
            const auto r = resolved(f.resolve(cmd), cmd, {TD::finished, {0, 0, 0, 4}});
            require(r.transitions == hard_end && r.after.timer.main_remaining_ms == main,
                    "Hard end wins even with authorized explicit Stop and pending later requests");
            active(f.file, {h[1], h[2]});
            journal(f.file, 4, fraction(10), 1);
        });
    }

    void l7CurrentTargetAndActionHistory_data() {
        QTest::addColumn<int>("policy");
        QTest::addColumn<int>("reverse");
        QTest::addColumn<bool>("released");
        QTest::newRow("continue-800")
            << int(MilestoneRefundPolicy::continue_actions) << 300 << false;
        QTest::newRow("review-800") << int(MilestoneRefundPolicy::review) << 300 << false;
        QTest::newRow("skip-800") << int(MilestoneRefundPolicy::skip) << 300 << false;
        QTest::newRow("released-queued") << int(MilestoneRefundPolicy::review) << 300 << true;
        QTest::newRow("above-capture-below-current-980")
            << int(MilestoneRefundPolicy::review) << 120 << false;
        QTest::newRow("current-target-equality")
            << int(MilestoneRefundPolicy::review) << 100 << false;
    }

    void l7CurrentTargetAndActionHistory() {
        QFETCH(int, policy);
        QFETCH(int, reverse);
        QFETCH(bool, released);
        check([&] {
            Fixture f(scores, {.main_before = 15, .other_score = 700, .omit_time = true}, false);
            Milestones m(f, released);
            const auto initial = committed_score(f.file);
            require(initial == 1100, "Real separately persisted score slots sum to 1100");
            const auto existing_jobs = f.file.rows(jobs);
            const auto existing_groups = f.file.rows(groups);
            const auto requested = fraction(reverse, 400);
            f.input.milestone_policy = MilestoneRefundPolicy(policy);
            const auto cmd = f.command("cross-B", std::to_string(reverse).c_str(), 0);
            const auto result = accepted(f.refund(
                cmd,
                quote(0, reverse * 10, 4000, false, {},
                      {link(CK::score, 1, 1, 0, 0, reverse, requested, reverse, false),
                       link(CK::whole_count, 2, 2, 0, 0, 0, requested, requested * 8000, false)})));
            const auto result_score = 1100 - reverse;
            require(!result.timer && committed_score(f.file) == result_score &&
                        f.file.head().balances.back().units == 8000 &&
                        f.file.rows(jobs) == existing_jobs &&
                        f.file.rows(groups) == existing_groups,
                    "Only actual selected score is reversed and prior captures/jobs are intact");
            const auto projection = project(f.file, m);
            const auto &after = projection.data();
            require(after.completed_prefix.size() == 2 &&
                        after.completed_prefix[1].target_micro_points == 950 &&
                        after.newly_completed_count == 0 && after.projection.next &&
                        after.projection.next->milestone_ref == "C" &&
                        after.projection.remaining_micro_points ==
                            std::uint64_t(2000 - result_score) &&
                        after.projection.track_deficit_micro_points ==
                            std::uint64_t(1000 - result_score) &&
                        after.projection.interval &&
                        after.projection.interval->earned_micro_points == 0,
                    "Committed score gives exact current-target deficit and suppressed suffix");
            const bool crossing = reverse > 100 && !released;
            require(result.milestone_observations.size() == std::size_t(crossing),
                    "Crossing is only eligible for a still-unreleased B below its current target");
            f.file.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions store(s);
                const auto &refund_ref = cmd.operation.operation_ref;
                require(!store.refund_audit(a, refund_ref),
                        "Released A does not receive a refund decision");
                const auto captured_job =
                    SqliteDeliveryJobStore(s).lookup({m.second.application, "job"});
                require(captured_job && captured_job->state == DeliveryJobState::queued,
                        "B's queued job persists independently of pending/released state");
                const auto prior = store.acceptance_audit(b);
                require(prior && prior->evidence.decision_ref == "choice-B",
                        "Independent acceptance action history persists");
                if (!crossing) {
                    require(!store.refund_audit(b, refund_ref),
                            "No crossing decision was persisted");
                    return;
                }
                const auto &observation = result.milestone_observations.front();
                const auto saved = store.refund_audit(b, refund_ref);
                require(observation.occurrence == b && observation.refund_ref == refund_ref &&
                            observation.score_before_micro_points == 1100 &&
                            observation.score_after_micro_points == result_score &&
                            observation.current_target_micro_points == 1000 && saved &&
                            saved->policy == f.input.milestone_policy &&
                            saved->deficit_micro_points == std::uint64_t(1000 - result_score),
                        "Only B has exact current-target evidence on the stable refund reference");
                if (f.input.milestone_policy == MilestoneRefundPolicy::skip) {
                    require(prior->ended_by_skip ==
                                    MilestoneSkipProvenance{MilestoneSkipOrigin::refund_policy,
                                                            refund_ref} &&
                                store.active_holds(b).empty() &&
                                store.pending_disposition(b) ==
                                    MilestonePendingDisposition::skipped,
                            "Skip ends Defer with terminal provenance");
                    return;
                }
                const auto expected =
                    f.input.milestone_policy == MilestoneRefundPolicy::review ? 2U : 1U;
                require(
                    store.active_holds(b).size() == expected,
                    "Existing Defer and independent refund Review causes coexist when applicable");
                if (expected == 2) {
                    require(store.resolve(b, {MilestoneHoldCause::deferred_acceptance, "choice-B"},
                                          MilestoneHoldResolution::continue_actions,
                                          {"tester", "clear-defer"}) &&
                                store.active_holds(b) ==
                                    std::vector<MilestoneHoldKey>{
                                        {MilestoneHoldCause::refund_review, refund_ref}},
                            "Clearing Defer keeps exactly the refund Review cause");
                }
            });
            if (crossing) {
                const auto total = reverse + 1;
                const auto fraction_after = fraction(total, 400);
                const auto deeper = f.command("already-below-B", "1", 1);
                const auto same_lane = accepted(f.refund(
                    deeper,
                    quote(reverse * 10, total * 10, 4000, false, {},
                          {link(CK::score, 1, 1, 1, reverse, total, fraction_after, total, false),
                           link(CK::whole_count, 2, 2, 1, 0, 0, fraction_after,
                                fraction_after * 8000, false)})));
                require(same_lane.milestone_observations.empty() &&
                            f.file.rows(decisions).size() == 1,
                        "A further decrease below B records no fresh crossing or policy audit");
            }
            if (result_score < 1000) {
                const auto snapshot = f.file.head();
                f.move("score-recovery", snapshot.timer, {}, 1000 - committed_score(f.file));
                require(
                    committed_score(f.file) == 1000 &&
                        project(f.file, m).data().newly_completed_count == 0 &&
                        f.file.rows(jobs) == existing_jobs &&
                        f.file.rows(groups) == existing_groups,
                    "Recovery to the completed current target produces no repeat action or reward");
                if (crossing &&
                    f.input.milestone_policy != MilestoneRefundPolicy::continue_actions) {
                    f.move("score-reaches-c", f.file.head().timer, {}, 1000);
                    require(committed_score(f.file) == 2000 &&
                                project(f.file, m).data().newly_completed_count == 1,
                            "Independent subsequent application makes C eligible");
                    stage_group(f, c, "C", 2000, 2000, MilestoneActionChoice::run, false);
                    const auto release = completed<MilestoneReleaseCompletion>([&](auto done) {
                        return release_milestone_head(*f.file.writer, c, std::move(done));
                    });
                    const auto expected = f.input.milestone_policy == MilestoneRefundPolicy::review
                                              ? MilestoneReleaseDisposition::refused
                                              : MilestoneReleaseDisposition::released;
                    require(release.disposition == expected &&
                                release.work.transaction == TransactionOutcome::committed,
                            "Remaining Review blocks C while terminal Skip permits the next group");
                }
            }
        });
    }

    void l8SameHeadReceiptRace() {
        check([] {
            Fixture f(ordered);
            const auto winner = f.command("race-winner", "4", 0);
            const auto loser = f.command("race-loser", "4", 0);
            require(winner.receipt != loser.receipt &&
                        winner.expected_state_token == loser.expected_state_token,
                    "Two real receipts select the same as-yet-unclaimed head and payment position");
            const auto p = order_quote(0);
            const auto first = accepted(f.refund(winner, p));
            refused(f.refund(loser, p), D::stale_payment);
            unchanged(f.file, loser, first.application.after.token);
            journal(f.file, 1, fraction(4));
            f.file.check_head(6, 0, 60, 8);

            const auto times = f.input.calls;
            f.input.lineage = false;
            const auto replay = accepted(f.refund(winner, p), D::existing);
            require(
                replay.application.after.token == first.application.after.token &&
                    replay.timer == first.timer && f.input.calls == times,
                "Exact historical repeat neither observes a new timer cut nor duplicates effects");
            f.file.authority.disclosure = false;
            refused(f.refund(winner, p), D::denied_disclosure);
            f.file.authority.disclosure = true;
            f.file.authority.execution = false;
            refused(f.refund(winner, p), D::denied_execution);
            require(f.input.calls == times, "Current retry authority gates precede clock input");
        });
    }

    void l8AcceptAcknowledgement_data() { fault_rows(); }

    void l8AcceptAcknowledgement() {
        QFETCH(int, fault);
        check([&] {
            auto credit = grouped;
            credit.amount = "400";
            credit.score = 300;
            Fixture f(credit, {.debt_after = 7, .score_before = 100, .other_score = 700}, false);
            const Milestones m(f);
            const auto p = full_quote(400, 6, 4, 300, 8);
            f.main("before-paired-refund", 4);
            const auto cmd = f.command("atomic-refund", "400", 0);
            const auto before = f.file.head();
            const auto jobs_before = f.file.rows(jobs);
            f.input.config = {End::Grace, 19, Reuse::Fresh, {}};
            f.input.policy = Policy::StopAtZero;
            std::optional<FinancialRefundRecord> provisional;
            const auto result = faulted(f, fault, [&](const auto &s) {
                auto reply = f.stage(s, cmd, p);
                const auto r = accepted(reply);
                const auto saved = SqliteFinancialRefundStore(s).lookup(cmd.operation);
                const auto audit =
                    SqliteMilestoneGroupDecisions(s).refund_audit(b, "atomic-refund");
                require(saved && r.timer == RefundTimerOutcome{TD::applied, {4, 4, 0, 2}} &&
                            same_timer(r.application.after.timer,
                                       {Life::Active, Phase::Grace, 0, 19, false}) &&
                            r.application.after.time_debt_ms == 11 &&
                            r.application.transitions == grace && audit &&
                            audit->deficit_micro_points == 200 && f.file.rows(decisions).empty(),
                        "Actual real phase, financial closure and Review are staged before fault");
                provisional = *saved;
                return reply;
            });
            if (fault == 0) {
                unchanged(f.file, cmd, before.token);
                require(f.file.rows(decisions).empty() && f.file.rows(jobs) == jobs_before &&
                            committed_score(f.file) == 1100,
                        "Known rollback leaves the entire old, independently committed closure");
                return;
            }
            static_cast<void>(result);
            const auto saved = by_ref(f.file, cmd.operation);
            f.move("later-committed-head", f.file.head().timer);
            const auto later = f.file.head().token;
            const auto recovered = by_ref(f.file, cmd.operation);
            require(
                later != saved.application.after.token && provisional &&
                    recovered.command.receipt == cmd.receipt &&
                    recovered.application.persisted_head.token == before.token &&
                    recovered.application.after.envelope.document ==
                        provisional->application.after.envelope.document &&
                    recovered.application.transitions == grace && recovered.timer == saved.timer &&
                    recovered.entitlement.accepted_money == 400 &&
                    recovered.milestone_observations.size() == 1 &&
                    committed_score(f.file) == 800 && f.file.rows(jobs) == jobs_before &&
                    f.file.rows(decisions).size() == 1,
                "Exact committed refund closure survives a later head, with one audit and no jobs");
            const auto calls = f.input.calls;
            f.input.lineage = false;
            const auto replay = accepted(f.refund(cmd, p), D::existing);
            require(
                replay.application.after.token == saved.application.after.token &&
                    f.file.head().token == later && f.input.calls == calls,
                "Exact repeat returns the reconciled historic acceptance with no new observation");
            f.file.authority.disclosure = false;
            refused(f.refund(cmd, p), D::denied_disclosure);
            const auto complete = payload(f.file, false, cmd.operation);
            put_payload(f.file, false, cmd.operation, complete.first(complete.size() - 1));
            rejected_history(f.file, false, cmd.operation);
            require(f.file.head().token == later && f.file.rows(decisions).size() == 1,
                    "Incomplete durable evidence cannot reconcile or cause a second application");
        });
    }

    void l8ResolutionAcknowledgement_data() { fault_rows(); }

    void l8ResolutionAcknowledgement() {
        QFETCH(int, fault);
        check([&] {
            Fixture f(ordered);
            f.main("deficient-origin", 2);
            Effects legacy;
            const auto first =
                f.file.accept(f.command("legacy-origin", "4", 0), order_quote(0), legacy);
            const auto held = require_hold(first, HoldReason::deficit, 1, -4, 0);
            f.main("resolution-funded-to-exact", 4);
            f.input.config = {End::Grace, 19, Reuse::Fresh, {}};
            const auto before = f.file.head().token;
            const auto cmd = f.file.continuation("atomic-closure", held, {held.locator});
            std::optional<FinancialTimerResolution> provisional;
            faulted(f, fault, [&](const auto &s) {
                auto reply =
                    stage_live_financial_timer_resolution(s, cmd, f.file.authority, f.input);
                const auto r = resolved(reply, cmd, {TD::applied, {4}});
                const SqliteFinancialRefundStore store(s);
                provisional = store.lookup_resolution(cmd.operation);
                require(provisional && store.active({f.file.cid}).empty() &&
                            r.transitions == grace &&
                            same_timer(r.after.timer, {Life::Active, Phase::Grace, 0, 19, false}),
                        "Real exhaustion phase and hold closure staged before fault");
                return reply;
            });
            if (fault == 0) {
                f.file.write([&](const auto &s) {
                    require(!SqliteFinancialRefundStore(s).lookup_resolution(cmd.operation),
                            "Known rollback retains no closure row");
                });
                active(f.file, {held});
                require(f.file.head().token == before, "Known rollback preserves selected head");
                journal(f.file, 1, fraction(4));
                return;
            }

            f.move("post-closure-later-head", f.file.head().timer);
            const auto head = f.file.head().token;
            const auto saved = by_resolution(f.file, cmd.operation);
            require(provisional && saved.before.token == before && saved.after.token != head &&
                        saved.after.envelope.document == provisional->after.envelope.document &&
                        saved.transitions == grace &&
                        saved.audit.decision_ref == cmd.audit.decision_ref &&
                        saved.timer == provisional->timer && saved.held.payment_position == 1,
                    "Exact committed resolution and audit reconcile even after later head");
            active(f.file, {});
            journal(f.file, 1, fraction(4), 1);
            const auto calls = f.input.calls;
            f.input.lineage = false;
            const auto replay = resolved(f.resolve(cmd), cmd, {TD::applied, {4}}, D::existing);
            require(
                replay.after.token == saved.after.token && f.input.calls == calls &&
                    f.file.head().token == head &&
                    by_ref(f.file, first.command.operation).timer == first.timer,
                "Replay returns one immutable closure and the original acceptance remains held");
            f.file.authority.disclosure = false;
            refused(f.resolve(cmd), D::denied_disclosure);
            const auto complete = payload(f.file, true, cmd.operation);
            put_payload(f.file, true, cmd.operation, complete.first(complete.size() - 1));
            rejected_history(f.file, true, cmd.operation);
            rejected<SqliteBundleStorageFailure>([&] { (void)f.file.financial_active(); });
            require(
                f.file.head().token == head &&
                    by_ref(f.file, first.command.operation).timer == first.timer,
                "Incomplete resolution evidence cannot hide the origin or publish another effect");
        });
    }

    void l9UtcExtremes_data() {
        QTest::addColumn<bool>("deadline_due");
        QTest::newRow("min-utc-max-deadline") << false;
        QTest::newRow("max-utc-equality") << true;
    }

    void l9UtcExtremes() {
        QFETCH(bool, deadline_due);
        check([&] {
            Fixture f(capped, cap_seed());
            f.main("pre-extreme", 15);
            f.input.config.hard_end_utc_ms = maximum;
            f.input.cuts = {{0, deadline_due ? maximum : minimum, false}};
            const auto before = f.file.head().timer;
            auto after = before;
            if (deadline_due)
                after.lifecycle = Life::Finished;
            else
                after.main_remaining_ms = 10;
            const auto cmd = f.command("extreme-utc", "10", 0);
            const auto r = accepted(f.refund(cmd, full_quote(10, 5, 0, 100, 8)));
            clock_and_state(f.file, r, before, deadline_due ? after : before, after,
                            deadline_due ? RefundTimerOutcome{TD::finished, {0, 0, 0, 5}}
                                         : RefundTimerOutcome{TD::applied, {5}},
                            0, deadline_due ? hard_end : std::vector<Transition>{});
        });
    }

    void l9CommittedScoreDeficitAndAggregateOverflow() {
        check([] {
            const OriginalCredit point{
                .amount = "1", .requested = 0, .main = 0, .cap_tail = 0, .score = 1, .count = 0};
            Fixture witness(point, {.main_before = 15, .other_score = 0, .omit_time = true}, false);
            const auto member =
                stage_group(witness, a, "A", 1, 1, MilestoneActionChoice::defer, false);
            track_seed(witness, {{"A", 1}}, {member});
            edit_completed_target(witness, "A", maximum);
            witness.move("score-slot-to-min", witness.file.head().timer, {}, minimum);
            require(witness.file.head().balances[0].units == minimum &&
                        witness.file.head().balances[1].units == 1 &&
                        committed_score(witness.file) == minimum + 1,
                    "Two distinct real committed score slots contain INT64_MIN and +1");
            const auto p = full_quote(1, 0, 0, 1, 0);
            const auto result = accepted(witness.refund(witness.command("reverse-one", "1", 0), p));
            journal(witness.file, 1, fraction(1));
            const auto after_score = committed_score(witness.file);
            require(after_score == minimum && result.milestone_observations.empty(),
                    "Removing one from the second slot commits at the signed minimum");
            const auto projection =
                core::advance_milestone_progression({track.track_ref,
                                                     "edited-definition",
                                                     "ctx",
                                                     after_score,
                                                     committed_targets(witness.file),
                                                     {member.completion}});
            require(projection.data().projection.track_deficit_micro_points ==
                        std::numeric_limits<std::uint64_t>::max(),
                    "Committed INT64_MIN against INT64_MAX derives the exact unsigned maximum");

            Fixture overflow(
                point,
                {.main_before = 15, .score_before = -1, .other_score = minimum, .omit_time = true});
            const auto old = overflow.file.head();
            const auto cmd = overflow.command("score-overflow", "1", 0);
            require(old.balances[0].units == minimum && old.balances[1].units == 0 &&
                        committed_score(overflow.file) == minimum,
                    "Both proposed balance slots fit but their sum would be INT64_MIN minus one");
            refused(overflow.refund(cmd, p), D::refused);
            unchanged(overflow.file, cmd, old.token);
        });
    }

    void l9UnavailableFacts() {
        check([] {
            Fixture f(ordered);
            const auto head = f.file.head().token;
            const auto cmd = f.command("unavailable-facts", "4", 0);
            const auto p = order_quote(0);
            f.input.policy.reset();
            refused(f.refund(cmd, p), D::unavailable);
            unchanged(f.file, cmd, head);
            f.input.policy = Policy::RequireReview;
            f.input.lineage = false;
            refused(f.refund(cmd, p), D::unavailable);
            unchanged(f.file, cmd, head);
            f.input.lineage = true;
            f.input.recovery_pending = true;
            refused(f.refund(cmd, p), D::unavailable);
            unchanged(f.file, cmd, head);
            f.input.recovery_pending = false;
            auto unknown = cmd;
            unknown.correlation.reset();
            refused(f.refund(unknown, p), D::invalid_financial_input);
            unchanged(f.file, cmd, head);
            f.input.selected_track.track_ref = "no-current-member-evidence";
            refused(f.refund(cmd, p), D::unavailable);
            unchanged(f.file, cmd, head);
            f.input.selected_track = track;
            accepted(f.refund(cmd, p));
            journal(f.file, 1, fraction(4));
        });
    }

    void c2PinnedLegacyWriterAndReader() {
        check([] {
            File v15(ordered);
            v15.move_main("v15-positive-main-to-four", 4);
            const auto id = v15.refund("v15-refund", core::ReceiptMoney{"4", "EUR"});
            const auto cmd = v15.command("v15", id, 0);
            Effects port;
            port.apply(-4, 0, {4});
            const auto first = v15.accept(cmd, order_quote(0), port);
            v15.reopen();
            const auto old = by_ref(v15, cmd.operation);
            require(old.timer == first.timer &&
                        old.application.after.timer.lifecycle == Life::Active &&
                        old.application.after.timer.main_remaining_ms == 0 &&
                        old.application.transitions.empty(),
                    "Genuine v15 v1 writer/reader retains numeric-only legacy Active Main zero");
            version(v15, false, cmd.operation, 1);
            journal(v15, 1, fraction(4));

            OrderedPrefix v19(true);
            auto &f = v19.f.file;
            const auto &h = v19.holds;
            auto all = locators({h[0], h[1], h[2]});
            Effects old_port;
            refused(f.continue_reply(f.continuation("legacy-wrong-order", h[1], all), old_port),
                    D::out_of_order);
            old_port.apply(-4, 0, {4});
            const auto close = f.continuation("legacy-right-order", h[0], all);
            resolved(f.continue_reply(close, old_port), close, {TD::applied, {4}});
            version(f, true, close.operation, 1);
            require(by_resolution(f, close.operation).after.timer.main_remaining_ms == 6,
                    "Pinned v1 continuation writes and reads its own actual historical closure");
            active(f, {h[1], h[2]});
            journal(f, 4, fraction(10), 1);

            Fixture mixed(ordered);
            mixed.main("legacy-main-two", 2);
            Effects held_port;
            const auto origin = mixed.command("legacy-mixed-origin", "4", 0);
            const auto h1 = require_hold(mixed.file.accept(origin, order_quote(0), held_port),
                                         HoldReason::deficit, 1, -4, 0);
            const auto batch = ordinary_batch(mixed, h1);
            const auto next = mixed.command("legacy-mixed-next", "0.10", 1);
            refused(mixed.file.refund_reply(next, order_quote(1), held_port), D::active_hold);
            refused(mixed.file.continue_reply(
                        mixed.file.continuation("legacy-mixed-close", h1, batch.expected_active),
                        held_port),
                    D::active_hold);
            ordinary_cleared(mixed.file, batch);
            active(mixed.file, {h1});

            auto ready_credit = ordered;
            ready_credit.ready = true;
            Fixture ready(ready_credit);
            Effects ready_port;
            refused(ready.file.refund_reply(ready.command("legacy-ready", "4", 0), order_quote(0),
                                            ready_port),
                    D::prerequisite);
            require(ready_port.observations == 0, "Pinned legacy gate never reduces Ready");

            Fixture finish(ordered);
            auto finished = finish.file.head().timer;
            finished.lifecycle = Life::Finished;
            Effects finished_port;
            finished_port.hard_end = 100;
            finished_port.observed = finished;
            finished_port.transitions = hard_end;
            const auto fc = finish.command("legacy-finished", "4", 0);
            const auto fr = finish.file.accept(fc, order_quote(0), finished_port);
            require(fr.timer == RefundTimerOutcome{TD::finished, {0, 0, 0, 4}} &&
                        by_ref(finish.file, fc.operation).application.transitions == hard_end,
                    "Pinned v1 writer and reader preserve observation-caused hard end");
        });
    }

    void c2PinnedLegacyWriterFaults_data() { fault_rows(); }

    void c2PinnedLegacyWriterFaults() {
        QFETCH(int, fault);
        check([&] {
            Fixture f(ordered);
            const auto cmd = f.command("legacy-winner", "4", 0);
            const auto stale = f.command("legacy-stale", "4", 0);
            const auto head = f.file.head().token;
            Effects port;
            port.apply(-4, 0, {4});
            faulted(f, fault, [&](const auto &s) {
                auto reply = stage_financial_refund(s, cmd, order_quote(0), f.file.authority, port);
                require(accepted(reply).timer == RefundTimerOutcome{TD::applied, {4}} &&
                            SqliteFinancialRefundStore(s).lookup(cmd.operation),
                        "Genuine legacy financial SQL reached the injected writer fault");
                return reply;
            });
            if (fault == 0) {
                unchanged(f.file, cmd, head);
                return;
            }
            require(by_ref(f.file, cmd.operation).application.after.timer.main_remaining_ms == 6,
                    "Genuine legacy selected read reconciles the physically committed result");
            refused(f.file.refund_reply(stale, order_quote(0), port), D::stale_payment);
            unchanged(f.file, stale, f.file.head().token);
            version(f.file, false, cmd.operation, 1);
            journal(f.file, 1, fraction(4));
        });
    }

    void c3AdjacentPredecessors_data() {
        QTest::addColumn<bool>("legacy_first");
        QTest::newRow("v1-to-v2") << true;
        QTest::newRow("fixture-only-v2-to-v1") << false;
    }

    void c3AdjacentPredecessors() {
        QFETCH(bool, legacy_first);
        check([&] {
            Fixture f(ordered);
            f.main("start-for-mixed-prefix", legacy_first ? 10 : 4);
            f.input.config = {End::Grace, 19, Reuse::Fresh, {}};
            Effects v1;
            v1.apply(-4, 0, {4});
            const auto first = f.command("adjacent-first", "4", 0);
            const auto r1 = legacy_first ? f.file.accept(first, order_quote(0), v1)
                                         : accepted(f.refund(first, order_quote(0)));
            version(f.file, false, first.operation, legacy_first ? 1 : 2);
            if (legacy_first)
                f.move("coherent-owner-after-v1", f.file.head().timer);
            else {
                require(r1.application.transitions == grace &&
                            r1.application.after.timer.grace_remaining_ms == 19,
                        "Original v2 acceptance independently establishes Fresh Grace");
                f.main("fund-legacy-successor", 6);
            }
            v1.apply(-6, 0, {6});
            const auto final = f.command("adjacent-next", "6", 1);
            const auto q = quote(40, 100, 100, true, time(1, {4, 0}, {10, 0}, fraction(10), -6, 0),
                                 {link(CK::score, 2, 2, 1, 40, 100, 1, 100, true),
                                  link(CK::whole_count, 3, 3, 1, 0, 8, 1, 8, true)});
            const auto r2 =
                legacy_first ? accepted(f.refund(final, q)) : f.file.accept(final, q, v1);
            version(f.file, false, final.operation, legacy_first ? 2 : 1);
            f.file.reopen();
            journal(f.file, 2, fraction(10));
            const auto saved = by_ref(f.file, final.operation);
            require(saved.entitlement.previous_money == 4 && saved.entitlement.time &&
                        saved.entitlement.time->expected_basis_position == 1 &&
                        saved.entitlement.time->previous == RefundTimeAllocation{4, 0} &&
                        saved.entitlement.time->cumulative == RefundTimeAllocation{10, 0} &&
                        saved.application.transitions == r2.application.transitions &&
                        by_ref(f.file, first.operation).timer == r1.timer &&
                        (legacy_first ? saved.application.after.timer.phase == Phase::Grace
                                      : saved.application.after.timer.phase == Phase::Main &&
                                            saved.application.transitions.empty()),
                    "Real adjacent predecessor decoding retains exact cumulative and version "
                    "semantics");
        });
    }

    void c3VersionTwoEvidence_data() {
        QTest::addColumn<bool>("resolution");
        QTest::addColumn<int>("fault");
        for (const bool resolution : {false, true}) {
            const auto journal = resolution ? QByteArray("resolution-") : QByteArray("refund-");
            QTest::newRow(qPrintable(journal + "grace-operand")) << resolution << 0;
            QTest::newRow(qPrintable(journal + "effect-suffix")) << resolution << 1;
            QTest::newRow(qPrintable(journal + "missing-evidence")) << resolution << 2;
            QTest::newRow(qPrintable(journal + "observation-prefix")) << resolution << 3;
        }
    }

    void c3VersionTwoEvidence() {
        QFETCH(bool, resolution);
        QFETCH(int, fault);
        check([&] {
            // An admitted distinct Fresh operand cannot alias debt, money, ordinals or timer legs.
            constexpr Number grace_operand = 117193563703;
            Fixture f(ordered);
            f.main("initial-evidence-main", resolution ? 2 : 4);
            const auto origin = f.command("evidence-origin", "4", 0);
            f.input.config = {End::Grace, grace_operand, Reuse::Fresh, {}};
            f.input.policy = Policy::StopAtZero;
            ApplicationOperationKey key = origin.operation;
            if (resolution) {
                Effects v1;
                const auto initial = f.file.accept(origin, order_quote(0), v1);
                const auto h = require_hold(initial, HoldReason::deficit, 1, -4, 0);
                version(f.file, false, origin.operation, 1);
                f.main("fund-evidence-resolution", 4);
                const auto cmd =
                    f.file.continuation("evidence-resolution", h, {h.locator}, Policy::StopAtZero);
                key = cmd.operation;
                const auto r = resolved(f.resolve(cmd), cmd, {TD::applied, {4}});
                require(r.transitions == grace && r.after.timer.phase == Phase::Grace &&
                            r.after.timer.grace_remaining_ms == grace_operand,
                        "Genuine v2 closure of a v1 origin durably exhausts into Fresh Grace");
            } else {
                const auto r = accepted(f.refund(origin, order_quote(0)));
                require(r.application.transitions == grace &&
                            r.application.after.timer.phase == Phase::Grace &&
                            r.application.after.timer.grace_remaining_ms == grace_operand,
                        "Genuine v2 acceptance durably exhausts into Fresh Grace");
            }
            version(f.file, resolution, key, 2);
            f.input.config = {End::Immediate, 1, Reuse::Shared, {}};
            f.file.reopen();
            if (resolution)
                require(by_resolution(f.file, key).transitions == grace,
                        "v2 resolution history uses retained Grace configuration");
            else
                require(by_ref(f.file, key).application.transitions == grace,
                        "v2 refund history uses retained Grace configuration");

            if (fault == 3) {
                // The positive control above proves suffix production. Use a fresh
                // history for a nonempty observation prefix and no effect suffix.
                Fixture observed(ordered);
                observed.main("main-with-positive-cut", resolution ? 2 : 4);
                observed.input.config = {End::Grace, grace_operand, Reuse::Fresh, {}};
                observed.input.policy = Policy::StopAtZero;
                const auto source = observed.command("observed-expiry", "4", 0);
                if (resolution) {
                    Effects legacy;
                    const auto hold =
                        require_hold(observed.file.accept(source, order_quote(0), legacy),
                                     HoldReason::deficit, 1, -4, 0);
                    observed.move("coherent-owner-after-v1", observed.file.head().timer);
                    const auto cmd = observed.file.continuation("observed-close", hold,
                                                                {hold.locator}, Policy::StopAtZero);
                    observed.input.cuts = {{2, 100, false}};
                    const auto closure =
                        resolved(observed.resolve(cmd), cmd, {TD::applied, {0, 0, 0, 4}});
                    require(closure.transitions == grace &&
                                closure.after.timer.phase == Phase::Grace,
                            "Positive cut really committed MainExpired/GraceStarted before effect");
                    corrupt_transitions(observed.file, true, cmd.operation, false);
                    rejected_history(observed.file, true, cmd.operation);
                } else {
                    observed.input.cuts = {{4, 100, false}};
                    const auto r = accepted(observed.refund(source, order_quote(0)));
                    require(r.application.transitions == grace &&
                                r.application.advanced_before.timer.phase == Phase::Grace &&
                                r.timer == RefundTimerOutcome{TD::applied, {0, 0, 0, 4}},
                            "Observation changes phase before the zero-removal refund");
                    corrupt_transitions(observed.file, false, source.operation, false);
                    rejected_history(observed.file, false, source.operation);
                }
                return;
            }
            if (fault == 1) {
                corrupt_transitions(f.file, resolution, key, true);
            } else {
                auto doc = payload(f.file, resolution, key);
                if (fault == 0) {
                    const auto expected = word(grace_operand);
                    const auto at = doc.indexOf(expected);
                    require(at >= prefix_end(doc, resolution) &&
                                doc.indexOf(expected, at + 1) == -1,
                            "Only the retained Grace operand is targeted in this v2 witness");
                    doc.replace(at, 8, word(grace_operand + 1));
                } else {
                    doc.truncate(prefix_end(doc, resolution));
                }
                put_payload(f.file, resolution, key, doc);
            }
            rejected_history(f.file, resolution, key);
        });
    }

    void c1BoundedWholeClosureAndLinkedResolution() {
        check([] {
            Fixture f({.amount = "64",
                       .requested = 64,
                       .main = 64,
                       .cap_tail = 0,
                       .score = 0,
                       .count = 0});
            f.move("elapsed-to-wait", {Life::Active, Phase::Main, 0, 9, true}, confirmation);
            f.input.config.end_mode = End::Confirmation;
            constexpr Number closed = 24;
            auto slice = [](Number i) {
                return quote(i * 10, (i + 1) * 10, 640, false,
                             time(i, {i, 0}, {i + 1, 0}, fraction(i + 1), -1, 0), {});
            };
            for (Number i = 0; i <= closed; ++i) {
                Effects legacy;
                const auto accepted = f.file.accept(
                    f.command("archived-" + std::to_string(i), "1", i), slice(i), legacy);
                const auto h = require_hold(accepted, HoldReason::deficit, i + 1, -1, 0);
                const auto cmd = f.file.continuation("archive-close-" + std::to_string(i), h,
                                                     {h.locator}, Policy::StopAtZero);
                if (i == closed) {
                    const auto actual = resolved(f.resolve(cmd), cmd, {TD::applied, {0, 0, 0, 1}});
                    require(actual.transitions.empty() && actual.after.timer.awaiting_confirmation,
                            "Current v2 closure on long committed history preserves wait");
                    version(f.file, true, cmd.operation, 2);
                    break;
                }
                legacy.expected_policy = Policy::StopAtZero;
                legacy.apply(-1, 0, {0, 0, 0, 1}, Policy::StopAtZero);
                resolved(f.file.continue_reply(cmd, legacy), cmd, {TD::applied, {0, 0, 0, 1}});
            }
            f.file.reopen();
            require(f.file.rows("SELECT count(*) FROM subathon_financial_resolutions") ==
                        SqlRows{{qint64(closed + 1)}},
                    "Long history contains real independently committed closure rows");
            const auto next = f.command("still-open", "1", closed + 1);
            const auto h = require_hold(accepted(f.refund(next, slice(closed + 1))),
                                        HoldReason::deficit, closed + 2, -1, 0);
            const auto batch = ordinary_batch(f, h);
            active(f.file, {h});

            // Count the cumulative statements needed for the selected reader
            // through the real writer's limits, with all other limits unchanged.
            auto probe = [&](qsizetype budget) {
                auto caps = limits();
                caps.statements_per_batch = budget;
                f.file.reopen(caps);
                return f.file.work(
                    [&](auto ctx) -> QVariant {
                        const auto s = ctx.begin();
                        const auto set = SqliteFinancialRefundStore(s).active({f.file.cid});
                        require(set.size() == 1 && same(set.front().locator, h.locator),
                                "A successful bounded read returns the complete active set");
                        s.commit();
                        return 1;
                    },
                    false);
            };
            qsizetype low = 1, high = limits().statements_per_batch;
            normal(probe(high));
            while (high - low > 1) {
                const auto mid = low + (high - low) / 2;
                const auto result = probe(mid);
                if (result.transaction == TransactionOutcome::committed)
                    high = mid;
                else {
                    require(result.error == WorkError::resource_limit && !result.value,
                            "Cumulative writer statement limit denies the whole selected read");
                    low = mid;
                }
            }
            normal(probe(high));
            const auto exceeded = probe(high - 1);
            require(exceeded.transaction != TransactionOutcome::committed &&
                        exceeded.error == WorkError::resource_limit && !exceeded.value,
                    "One statement below measured boundary cannot publish a partial hold set");

            const auto denied = f.file.work(
                [&](auto ctx) -> QVariant {
                    const auto s = ctx.begin();
                    auto reply = stage_financial_aware_linked_resolution(s, batch);
                    s.commit();
                    return QVariant::fromValue(reply);
                },
                false);
            require(!acknowledged_financial_reply(denied) &&
                        denied.error == WorkError::resource_limit,
                    "Over-budget linked resolution cannot publish or stage partial closures");
            f.file.reopen();
            const ApplicationOperationKey corrupted{"test", "archive-close-24"};
            const auto good = payload(f.file, true, corrupted);
            put_payload(f.file, true, corrupted, good.first(good.size() - 1));
            rejected<SqliteBundleStorageFailure>([&] { (void)f.file.financial_active(); });
            rejected<SqliteBundleStorageFailure>([&] { (void)apply_ordinary(f.file, batch); });
            require(f.file.rows("SELECT count(*) FROM subathon_financial_resolutions") ==
                        SqlRows{{qint64(closed + 1)}},
                    "Contradictory selected evidence cannot create new financial closures");
            put_payload(f.file, true, corrupted, good);
            active(f.file, {h});
            f.file.write([&](const auto &s) {
                for (const auto &entry : batch.entries) {
                    const auto version = SqliteApplicationHoldStore(s).find(entry.hold_ref);
                    require(
                        version && version->current_version.state == ApplicationHoldState::active &&
                            version->current_version.locator.version_ref ==
                                entry.expected_version_ref,
                        "Over-budget and contradictory closures leave every ordinary hold active");
                }
            });
            ordinary_cleared(f.file, batch);
            active(f.file, {h});
        });
    }

  private:
    static void fault_rows() {
        QTest::addColumn<int>("fault");
        QTest::newRow("reached-late-sql-rollback") << 0;
        QTest::newRow("physical-commit-lost-ack") << 1;
        QTest::newRow("physical-commit-abnormal-body") << 2;
    }

    template <class Stage> static SqliteWorkResult faulted(Fixture &f, int fault, Stage stage) {
        const auto before = f.file.head().token;
        if (fault == 1)
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *f.file.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Arm the actual writer's physical COMMIT acknowledgement fault");
        bool witnessed = false;
        bool returned_from_commit = false;
        std::exception_ptr early;
        const auto result = f.file.work(
            [&](auto context) -> QVariant {
                const auto s = context.begin();
                FinancialWorkReply reply;
                try {
                    reply = stage(s);
                    require(
                        f.file.rows("SELECT token FROM subathon_app_head") ==
                            SqlRows{{bytes(before)}},
                        "Independent committed reader still sees the original head inside work");
                    witnessed = true;
                } catch (...) {
                    early = std::current_exception();
                    throw;
                }
                if (fault == 0)
                    (void)s.execute(
                        {"INSERT INTO subathon_app_audit SELECT * FROM subathon_app_audit "
                         "WHERE operation_scope=? AND operation_ref=?",
                         {bytes(original.operation_scope), bytes(original.operation_ref)}});
                s.commit();
                returned_from_commit = true;
                if (fault == 2)
                    throw std::runtime_error("synthetic body exception after physical commit");
                return QVariant::fromValue(reply);
            },
            false);
        if (early)
            std::rethrow_exception(early);
        require(witnessed && !result.value && !acknowledged_financial_reply(result),
                "Fault is downstream of a real stage and publication has no success payload");
        switch (fault) {
        case 0:
            require(result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::sql_failure && !returned_from_commit,
                    "Reached late write failure rolls back the entire unit");
            break;
        case 1:
            require(
                result.transaction == TransactionOutcome::outcome_unknown &&
                    result.error == WorkError::uncertain_outcome && !returned_from_commit &&
                    f.file.writer->enqueue_work([](auto) { return QVariant{}; }, [](auto) {}) !=
                        StorageError::none,
                "Lost physical acknowledgement makes writer outcome unknown and closes admission");
            f.file.reopen_closed();
            break;
        case 2:
            require(result.transaction == TransactionOutcome::committed &&
                        result.error == WorkError::work_exception && returned_from_commit,
                    "Abnormal work body after an acknowledged physical commit publishes nothing");
            f.file.reopen();
            break;
        }
        return result;
    }

    template <class Error, class Read> static void rejected(Read read) {
        bool refused = false;
        try {
            read();
        } catch (const Error &error) {
            refused = error.reason() == SqliteBundleStorageReason::invalid_stored_data;
        }
        require(refused, "Contradictory selected evidence fails closed with no typed result");
    }

    static QByteArray word(qint64 value) {
        QByteArray data(8, '\0');
        qToBigEndian(value, data.data());
        return data;
    }

    // Independently walk the frozen v1 variable-length prefix. Mutation offsets
    // follow the shipped v1 primitive framing only, with no production v2 decoder.
    static qsizetype prefix_end(const QByteArray &doc, bool resolution,
                                qsizetype *transition_start = nullptr) {
        qsizetype at = 8;
        const auto read = [&] {
            require(at + 8 <= doc.size(), "Complete private journal integer");
            const auto n = qFromBigEndian<qint64>(doc.constData() + at);
            at += 8;
            return n;
        };
        const auto blob = [&] {
            const auto n = read();
            require(n >= 0 && n <= doc.size() - at, "Complete private journal byte field");
            at += n;
        };
        if (resolution) {
            blob();      // immutable command
            at += 7 * 8; // disposition, five actual deltas, policy
            const auto n = read();
            require(n == 2, "Two retained transitions in the Fresh Grace witness");
            if (transition_start)
                *transition_start = at;
            at += n * 8;
        } else {
            for (int i = 0; i < 6; ++i)
                blob();   // previous money, accepted money, fraction
            (void)read(); // full
            require(read() == 1, "Fresh witness has a grouped time basis");
            blob();      // original operation scope
            blob();      // original operation ref
            at += 8 * 8; // original ordinal/kind, request/position, prior/cumulative
            for (int i = 0; i < 4; ++i)
                blob();  // exact grouped request and residual
            at += 9 * 8; // two requested legs, timer outcome, hold reason
            require(read() == 0, "Fresh witness has no milestone decision rows");
        }
        require(at < doc.size(), "Valid v2 evidence extends the retained v1 payload");
        return at;
    }

    static void put_payload(File &f, bool resolution, const ApplicationOperationKey &key,
                            const QByteArray &doc) {
        f.write([&](const auto &s) {
            (void)s.execute({resolution
                                 ? "UPDATE subathon_financial_resolutions SET document=? WHERE "
                                   "operation_scope=? AND operation_ref=?"
                                 : "UPDATE subathon_financial_refunds SET document=? WHERE "
                                   "operation_scope=? AND operation_ref=?",
                             {doc, bytes(key.operation_scope), bytes(key.operation_ref)}});
            require(s.execute({"SELECT changes()", {}}) == SqlRows{{1}},
                    "Fixture mutation touches only the selected physical journal row");
        });
    }

    static void corrupt_transitions(File &f, bool resolution, const ApplicationOperationKey &key,
                                    bool reorder) {
        if (resolution) {
            auto doc = payload(f, true, key);
            qsizetype at{};
            (void)prefix_end(doc, true, &at);
            require(doc.mid(at, 16) ==
                        word(int(Transition::MainExpired)) + word(int(Transition::GraceStarted)),
                    "Corruption grounds its two transitions in the established valid v2 closure");
            if (reorder)
                doc.replace(at, 16,
                            word(int(Transition::GraceStarted)) +
                                word(int(Transition::MainExpired)));
            else
                doc.replace(at, 8, word(int(Transition::HardEndReached)));
            put_payload(f, true, key, doc);
            return;
        }
        f.write([&](const auto &s) {
            (void)s.execute(
                {reorder
                     ? "UPDATE subathon_app_transitions SET transition=CASE ordinal "
                       "WHEN 1 THEN 1 WHEN 2 THEN 0 END WHERE operation_scope=? AND operation_ref=?"
                     : "UPDATE subathon_app_transitions SET transition=4 WHERE "
                       "operation_scope=? AND operation_ref=? AND ordinal=1",
                 {bytes(key.operation_scope), bytes(key.operation_ref)}});
            require(s.execute({"SELECT changes()", {}}) == SqlRows{{reorder ? 2 : 1}},
                    "Separate fixture corruption touches only the intended transition positions");
        });
    }

    static void rejected_history(File &file, bool resolution, const ApplicationOperationKey &key) {
        rejected<SqliteBundleStorageFailure>([&] {
            file.write([&](const auto &s) {
                const SqliteFinancialRefundStore store(s);
                if (resolution)
                    (void)store.lookup_resolution(key);
                else
                    (void)store.lookup(key);
            });
        });
        rejected<SqliteBundleStorageFailure>([&] { (void)file.history(); });
    }
};
} // namespace

QTEST_GUILESS_MAIN(SqliteLiveFinancialRefundsTests)
#include "sqlite_live_financial_refunds_tests.moc"
