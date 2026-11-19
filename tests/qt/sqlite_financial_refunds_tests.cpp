// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_financial_refunds_fixture.hpp"
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace financial_refund_fixture;

template <class Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

FinancialEntitlement entitlement(core::Exact previous, core::Exact accepted, core::Exact f,
                                 bool full, Number position, RefundTimeAllocation prior,
                                 RefundTimeAllocation next, core::Exact exact, core::Exact residual,
                                 Number main, Number restore) {
    return {previous,
            accepted,
            f,
            full,
            GroupedRefundEntitlement{{original, 1, CK::main_delta},
                                     1,
                                     position,
                                     prior,
                                     next,
                                     exact,
                                     residual,
                                     main,
                                     restore},
            {}};
}

ApplicationReversalLink component(CK kind, Number position, Number previous, Number cumulative,
                                  Number request, core::Exact f, core::Exact exact, bool full,
                                  core::Exact residual = fraction(0)) {
    const auto ordinal = kind == CK::score ? 2 : 3;
    return {ordinal,    {original, ordinal, kind},
            position,   previous,
            cumulative, request,
            f,          exact,
            residual,   full};
}

FinancialEntitlement first_quarter(bool include_count = true) {
    auto p = entitlement(fraction(0), fraction(5), fraction(1, 4), false, 0, {}, {75000, 0},
                         fraction(75000), fraction(0), -75000, 0);
    p.components.push_back(
        component(CK::score, 0, 0, 10000000, -10000000, fraction(1, 4), fraction(10000000), false));
    if (include_count)
        p.components.push_back(component(CK::whole_count, 0, 0, 0, 0, fraction(1, 4),
                                         fraction(7, 4), false, fraction(7, 4)));
    return p;
}

FinancialRefundCommand refund(File &f, const std::string &ref, const char *amount, Number position,
                              const std::string &economic_ref = {}) {
    return f.command(ref, f.refund(ref + "-delivery", core::ReceiptMoney{amount, "EUR"}), position,
                     economic_ref);
}

void refusal(const FinancialWorkReply &reply, D expected) {
    require(reply.disposition == expected && std::holds_alternative<std::monostate>(reply.record),
            "Exact refusal, without a disclosed or newly applied record");
}

void timer_result(const FinancialRefundRecord &r, TD d, RefundTimerActual actual) {
    require(r.timer == RefundTimerOutcome{d, actual}, "Exact retained timer disposition and legs");
}

void history_progress(File &f, Number position, const core::Exact &accepted,
                      size_t resolutions = 0) {
    const auto h = f.history();
    require(h && h->position == position && h->accepted_money == accepted &&
                h->refunds.size() == size_t(position) && h->resolutions.size() == resolutions,
            "Exact selected payment head and complete refund/resolution history");
}

void same_position(const FinancialRefundRecord &r, Number position, const FinancialEntitlement &p) {
    require(r.payment_position == position && r.entitlement.previous_money == p.previous_money &&
                r.entitlement.accepted_money == p.accepted_money &&
                r.entitlement.fraction == p.fraction && r.entitlement.full == p.full &&
                r.entitlement.time && p.time,
            "Exact immutable cumulative money and requested provenance");
    const auto &time = *r.entitlement.time;
    require(same(time.original.operation, original) && time.original.request_ordinal == 1 &&
                time.original.component_kind == CK::main_delta &&
                time.request_ordinal == p.time->request_ordinal &&
                time.expected_basis_position == p.time->expected_basis_position &&
                time.previous == p.time->previous && time.cumulative == p.time->cumulative &&
                time.exact_cumulative == p.time->exact_cumulative &&
                time.residual == p.time->residual && time.main_request == p.time->main_request &&
                time.restoration_request == p.time->restoration_request,
            "Exact grouped time occurrence, Main-first split, residual and both requested legs");
    require(r.application.reversals.size() == p.components.size(),
            "All selected score/count occurrences retained");
    for (size_t i = 0; i < p.components.size(); ++i) {
        const auto &expected = p.components[i];
        const auto &actual = r.application.reversals[i];
        require(same(actual.original.operation, expected.original.operation) &&
                    actual.original.request_ordinal == expected.original.request_ordinal &&
                    actual.original.component_kind == expected.original.component_kind &&
                    actual.request_ordinal == expected.request_ordinal &&
                    actual.basis_position == expected.expected_prior_position + 1 &&
                    actual.previous_cumulative == expected.expected_prior_cumulative &&
                    actual.cumulative == expected.cumulative &&
                    actual.cumulative_fraction == p.fraction &&
                    actual.exact_entitlement == expected.exact_entitlement &&
                    actual.rounding_residual == expected.rounding_residual &&
                    actual.requested_increment == expected.requested_increment &&
                    actual.full_refund == p.full,
                "Component occurrence retains its basis, exact journal F and requested increment");
        const auto effect = std::find_if(
            r.application.effects.begin(), r.application.effects.end(),
            [&](const auto &e) { return e.request_ordinal == expected.request_ordinal; });
        require(effect != r.application.effects.end() &&
                    effect->requested_units == expected.requested_increment &&
                    effect->components.size() == 1 &&
                    effect->components.front().kind == expected.original.component_kind &&
                    effect->components.front().units == expected.requested_increment,
                "Real score/count effect occurrence carries the exact actual delta");
    }
}

FinancialRefundRecord read_refund(File &f, const ApplicationOperationKey &key) {
    std::optional<FinancialRefundRecord> r;
    f.write([&](const auto &s) { r = SqliteFinancialRefundStore(s).lookup(key); });
    require(r.has_value(), "Fresh authoritative refund closure exists by exact operation");
    return *r;
}

FinancialTimerResolution read_resolution(File &f, const ApplicationOperationKey &key) {
    std::optional<FinancialTimerResolution> r;
    f.write([&](const auto &s) { r = SqliteFinancialRefundStore(s).lookup_resolution(key); });
    require(r.has_value(), "Fresh authoritative resolution closure exists by exact operation");
    return *r;
}

void absent(File &f, const FinancialRefundCommand &c) {
    f.check_raw(c.receipt);
    f.write([&](const auto &s) {
        require(!SqliteFinancialRefundStore(s).lookup(c.operation) &&
                    !SqliteApplicationPlanStore(s).find_revision(c.financial_plan.plan_ref,
                                                                 c.financial_plan.revision),
                "Refusal retains intake and claims no operation or capture");
    });
}

void active(File &f, const std::vector<FinancialTimerHold> &expected) {
    const auto current = f.financial_active();
    require(current.size() == expected.size(), "Complete selected active financial hold count");
    for (const auto &hold : expected) {
        const auto seen = std::find_if(current.begin(), current.end(), [&](const auto &h) {
            return same(h.locator, hold.locator);
        });
        require(seen != current.end() && seen->reason == hold.reason &&
                    seen->payment_position == hold.payment_position && seen->subject == f.cid &&
                    seen->payment == f.payment &&
                    same(seen->refund_operation, hold.refund_operation) &&
                    seen->time.main_request == hold.time.main_request &&
                    seen->time.restoration_request == hold.time.restoration_request,
                "Each exact financial request, payment ordinal, CID and version remains active");
    }
}

FinancialTimerHold held(const File &f, const FinancialRefundRecord &r, FinancialTimerHoldReason why,
                        Number position, Number main, Number restoration) {
    require(r.hold && r.hold->subject == f.cid && r.hold->payment == f.payment &&
                r.hold->payment_position == position && r.hold->reason == why &&
                same(r.hold->refund_operation, r.command.operation) &&
                r.hold->time.main_request == main &&
                r.hold->time.restoration_request == restoration,
            "Finite typed financial hold preserves the whole request and first version");
    return *r.hold;
}

FinancialTimerResolution resolve(File &f, const FinancialTimerCommand &c, Effects &effects,
                                 RefundTimerOutcome outcome, D disposition = D::resolved) {
    const auto result = f.continue_reply(c, effects);
    require(result.disposition == disposition, "Exact resolution/retry disposition");
    auto r = std::get<FinancialTimerResolution>(result.record);
    require(r.timer == outcome && same(r.held.locator, c.origin) &&
                r.closed.locator.hold_ref == c.origin.hold_ref &&
                r.closed.parent_version_ref == c.origin.version_ref && r.closed.position == 2 &&
                r.closed.state == ApplicationHoldState::resolved &&
                same(r.command.operation, c.operation) &&
                r.command.override_policy == c.override_policy &&
                r.audit.actor_ref == c.audit.actor_ref &&
                r.audit.decision_ref == c.audit.decision_ref,
            "Typed resolution is bound to immutable intent, the exact origin and audit");
    return r;
}

const auto decisions = "SELECT * FROM subathon_milestone_refund_decisions ORDER BY refund_ref";
const auto jobs = "SELECT * FROM subathon_delivery_events ORDER BY "
                  "operation_scope,operation_ref,job_ref,revision";
const auto groups = "SELECT * FROM subathon_milestone_groups ORDER BY queue_order";
const auto audits = "SELECT * FROM subathon_app_audit ORDER BY operation_scope,operation_ref";

struct Mixed {
    File f{OriginalCredit{.milestone = true}};
    Effects effects;
    FinancialRefundCommand first;

    Mixed() : first(refund(f, "refund-5", "5.00", 0)) {
        f.move_main("elapsed-to-50000", 50000);
        first.expected_state_token = f.head().token;
        effects.milestone = true;
    }

    FinancialTimerHold accept_first() {
        const auto r = f.accept(first, first_quarter(), effects);
        check_first(r);
        check_review();
        f.check_head(50000, 0, 30000000, 7);
        require(effects.adjustments == 0, "Deficit never partially runs the held timer effect");
        return held(f, r, FinancialTimerHoldReason::deficit, 1, -75000, 0);
    }

    void check_first(const FinancialRefundRecord &r) {
        same_position(r, 1, first_quarter());
        timer_result(r, TD::held, {0, 0, 0, 75000, 0});
        require(r.milestone_observations.size() == 1, "One committed milestone observation");
        const auto &decision = r.milestone_observations.front();
        require(decision.occurrence == achievement && decision.refund_ref == "refund-5" &&
                    decision.score_before_micro_points == 40000000 &&
                    decision.score_after_micro_points == 30000000 &&
                    decision.current_target_micro_points == 35000000 &&
                    decision.policy == MilestoneRefundPolicy::review,
                "Actual score before/after and current target feed existing refund review");
    }

    void check_review() {
        f.write([&](const auto &s) {
            const SqliteMilestoneGroupDecisions store(s);
            const auto r = store.refund_audit(achievement, "refund-5");
            const auto pending = SqliteMilestoneGroupStore(s).pending();
            const auto job = SqliteDeliveryJobStore(s).lookup({original, "original-job"});
            require(r && r->deficit_micro_points == 5000000 &&
                        r->current_target_micro_points == 35000000 && !r->resolution &&
                        pending.size() == 1 &&
                        pending.front().group.achievement.score_micro_points == 40000000 &&
                        store.pending_disposition(achievement) ==
                            MilestonePendingDisposition::held &&
                        job && job->state == DeliveryJobState::queued,
                    "Real v13 review retains original achievement and unreleased captured job");
        });
        require(f.rows(decisions).size() == 1, "Exactly one immutable milestone refund audit");
    }
};

class FakeDelivery final : public DeliveryDispatchAuthority, public DeliveryAdapter {
  public:
    bool allows(const DeliveryJob &) override { return true; }
    void start(DeliveryJob job) override { started = std::move(job); }
    std::optional<DeliveryJob> started;
};

void perform_original_action(File &f) {
    const auto release = completed<MilestoneReleaseCompletion>(
        [&](auto done) { return release_milestone_head(*f.writer, achievement, std::move(done)); });
    normal(release.work);
    require(release.disposition == MilestoneReleaseDisposition::released,
            "Shipped v13 store records the original release");
    const auto fake = std::make_shared<FakeDelivery>();
    const DeliveryJobKey key{original, "original-job"};
    const auto dispatch = completed<DeliveryDispatchCompletion>([&](auto done) {
        return dispatch_delivery_job(*f.writer, key, 1, fake, fake, std::move(done));
    });
    normal(dispatch.work);
    require(fake->started && fake->started->attempt && dispatch.result &&
                dispatch.result->disposition == DeliveryDisposition::changed,
            "Real delivery claim reaches the local fake only");
    f.write([&](const auto &s) {
        const auto r =
            SqliteDeliveryJobStore(s).finish(key, fake->started->revision, *fake->started->attempt,
                                             DeliveryEvidence::delivered, "synthetic");
        require(r.job && r.job->state == DeliveryJobState::delivered,
                "Actual delivered job history before the refund");
    });
}

class SqliteFinancialRefundsTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void w01ImmutableActualCreditsAndHistoricalCapture() {
        check([] {
            File f;
            f.move_main("independent-credit", 600000);
            auto p2 = entitlement(fraction(5), fraction(10), fraction(1, 2), false, 1, {75000, 0},
                                  {150000, 0}, fraction(150000), fraction(0), -75000, 0);
            p2.components = {component(CK::score, 1, 10000000, 20000000, -10000000, fraction(1, 2),
                                       fraction(20000000), false),
                             component(CK::whole_count, 1, 0, 0, 0, fraction(1, 2), fraction(7, 2),
                                       false, fraction(7, 2))};
            auto p3 = entitlement(fraction(10), fraction(20), fraction(1), true, 2, {150000, 0},
                                  {300000, 0}, fraction(300000), fraction(0), -150000, 0);
            p3.components = {
                component(CK::score, 2, 20000000, 40000000, -20000000, fraction(1),
                          fraction(40000000), true),
                component(CK::whole_count, 2, 0, 7, -7, fraction(1), fraction(7), true)};
            const std::vector<FinancialEntitlement> proposals{first_quarter(), p2, p3};
            const char *amounts[]{"5.00", "5.000", "10"};
            const Number mains[]{525000, 450000, 300000};
            const Number removals[]{75000, 75000, 150000};
            const Number scores[]{30000000, 20000000, 0};
            const Number counts[]{7, 7, 0};
            Effects effects;
            for (Number i = 0; i < 3; ++i) {
                const auto c = refund(f, "w01-" + std::to_string(i), amounts[i], i);
                effects.apply(-removals[i], 0, {removals[i]});
                (void)f.accept(c, proposals[i], effects);
                const auto r = read_refund(f, c.operation);
                same_position(r, i + 1, proposals[i]);
                timer_result(r, TD::applied, {removals[i]});
                require(
                    r.refund_receipt.id == c.receipt &&
                        r.refund_receipt.original_facts.money ==
                            core::ReceiptMoney{amounts[i], "EUR"} &&
                        r.refund_receipt.attempts.size() == 1 &&
                        r.support_receipt.id == f.payment.support_receipt &&
                        r.support_receipt.original_facts.money ==
                            core::ReceiptMoney{"20.0000", "EUR"} &&
                        r.support_receipt.original_arrival.configuration_ref == "cfg" &&
                        r.support_receipt.original_arrival.modifier_context_ref == "ctx" &&
                        r.original_capture.captured.valuation.historical &&
                        r.original_capture.captured.valuation.historical->effective_rate_decimal ==
                            "0.972500" &&
                        r.original_capture.rules.size() == 1 &&
                        r.original_capture.captured.requested_effects.size() == 3 &&
                        !r.financial_capture.captured.requested_effects.empty() &&
                        r.original_capture.rules.front().version_ref == "historical-rule-v1" &&
                        r.original_capture.captured.requested_effects.front().action_version_ref ==
                            "historical-action-v2" &&
                        r.financial_capture.captured.event.receipt_id == c.receipt &&
                        r.financial_capture.captured.requested_effects.front()
                                .exact_requested_units == fraction(-removals[i]) &&
                        r.financial_capture.captured.requested_effects.front().requested_units ==
                            -removals[i] &&
                        r.application.audit.actor_ref == c.audit.actor_ref &&
                        r.application.audit.decision_ref == c.audit.decision_ref && !r.hold,
                    "Committed closure preserves source spelling, capture lineage, audit and "
                    "intake");
                f.write([&](const auto &s) {
                    const auto source = SqliteApplicationBundleStore(s).lookup(original);
                    require(source && source->effects.size() == 3,
                            "Original award has exactly the three selected immutable occurrences");
                    const auto &time = source->effects.front();
                    require(time.request_ordinal == 1 && time.requested_units == 600000 &&
                                time.maximum_main_ms == 300000 && time.components.size() == 3 &&
                                time.components.front().kind == CK::main_delta &&
                                time.components.front().units == 300000 &&
                                time.components.back().kind == CK::cap_limited_time &&
                                time.components.back().units == 300000,
                            "Original requested 600000, actual 300000 and immutable cap tail");
                });
                f.check_head(mains[i], 0, scores[i], counts[i]);
            }
            history_progress(f, 3, fraction(20));
        });
    }

    void w02RetryIdentityAndCurrentAuthority() {
        check([] {
            File f;
            Effects effects;
            effects.apply(-75000, 0, {75000});
            const auto c = refund(f, "K", "5.00", 0, "r1");
            const auto p = first_quarter();
            const auto before = f.accept(c, p, effects);
            f.move_main("after-K", 325000);
            const auto count = f.rows(audits).size();
            const auto observations = effects.observations;
            const auto again = f.accept(c, p, effects, D::existing);
            same_position(again, 1, p);
            require(again.application.after.token == before.application.after.token &&
                        again.application.after.timer.main_remaining_ms == 225000 &&
                        effects.observations == observations && effects.adjustments == 1,
                    "Exact retry returns the historical result after head movement without replay");

            auto changed = c;
            changed.audit.decision_ref = "changed stable intent";
            refusal(f.refund_reply(changed, p, effects), D::identity_conflict);
            auto receipt_reuse = c;
            receipt_reuse.operation.operation_ref = "K2";
            receipt_reuse.financial_plan.plan_ref = "capture-K2";
            refusal(f.refund_reply(receipt_reuse, p, effects), D::identity_conflict);
            absent(f, receipt_reuse);
            const auto economic_reuse = refund(f, "K3", "5", 1, "r1");
            refusal(f.refund_reply(economic_reuse, p, effects), D::identity_conflict);
            absent(f, economic_reuse);

            f.authority.execution = false;
            refusal(f.refund_reply(c, p, effects), D::denied_execution);
            f.authority.execution = true;
            f.authority.disclosure = false;
            refusal(f.refund_reply(c, p, effects), D::denied_disclosure);
            require(f.rows(audits).size() == count && effects.adjustments == 1,
                    "Conflicts and either denial append no audit, timer adjustment or publication");
            f.check_raw(c.receipt);
            f.check_head(325000, 0, 30000000, 7);
            history_progress(f, 1, fraction(5));
        });
    }

    void w03PaymentBasisAndStateFreshness() {
        check([] {
            File f;
            f.move_main("funded", 600000);
            Effects e;
            const auto initial = refund(f, "nineteen", "19", 0);
            auto p19 = entitlement(fraction(0), fraction(19), fraction(19, 20), false, 0, {},
                                   {285000, 0}, fraction(285000), fraction(0), -285000, 0);
            p19.components = {component(CK::score, 0, 0, 38000000, -38000000, fraction(19, 20),
                                        fraction(38000000), false),
                              component(CK::whole_count, 0, 0, 0, 0, fraction(19, 20),
                                        fraction(133, 20), false, fraction(133, 20))};
            e.apply(-285000, 0, {285000});
            (void)f.accept(initial, p19, e);
            auto p =
                entitlement(fraction(19), fraction(79, 4), fraction(79, 80), false, 1, {285000, 0},
                            {296250, 0}, fraction(296250), fraction(0), -11250, 0);
            p.components = {component(CK::score, 1, 38000000, 39500000, -1500000, fraction(79, 80),
                                      fraction(39500000), false),
                            component(CK::whole_count, 1, 0, 0, 0, fraction(79, 80),
                                      fraction(553, 80), false, fraction(553, 80))};
            const auto winner = refund(f, "first-075", "0.75", 1);
            const auto loser = refund(f, "second-075", "0.75", 1);
            auto stale_basis = p;
            stale_basis.components.front().expected_prior_position = 0;
            refusal(f.refund_reply(winner, stale_basis, e), D::stale_basis);
            absent(f, winner);
            auto stale_time = p;
            stale_time.time->expected_basis_position = 0;
            refusal(f.refund_reply(winner, stale_time, e), D::stale_basis);
            absent(f, winner);
            auto stale_state = winner;
            stale_state.expected_state_token = "awarded";
            refusal(f.refund_reply(stale_state, p, e), D::stale_state);
            absent(f, winner);

            e.apply(-11250, 0, {11250});
            same_position(f.accept(winner, p, e), 2, p);
            // The previously matched payment position precedes basis revalidation.
            refusal(f.refund_reply(loser, p, e), D::stale_payment);
            const auto fresh = f.command(loser.operation.operation_ref, loser.receipt, 2);
            // The new entire 0.75 would reach 20.50, so no eligible entitlement exists.
            auto excess = p;
            excess.previous_money = fraction(79, 4);
            excess.accepted_money = fraction(41, 2);
            excess.fraction = fraction(41, 40);
            excess.time->expected_basis_position = 2;
            excess.time->previous = {296250, 0};
            excess.time->cumulative = {307500, 0};
            excess.time->exact_cumulative = fraction(307500);
            excess.components = {component(CK::score, 2, 39500000, 41000000, -1500000,
                                           fraction(41, 40), fraction(41000000), false),
                                 component(CK::whole_count, 2, 0, 0, 0, fraction(41, 40),
                                           fraction(287, 40), false, fraction(287, 40))};
            refusal(f.refund_reply(fresh, excess, e), D::exceeds_original);
            absent(f, loser);
            f.check_head(303750, 0, 500000, 7);
            history_progress(f, 2, fraction(79, 4));
        });
    }

    void w03RawUnknownWrongCurrencyAndUnverified() {
        check([] {
            File f;
            Effects effects;
            auto wrong = f.command("wrong", f.refund("wrong", core::ReceiptMoney{"5", "CHF"}), 0);
            auto unknown = f.command("unknown", f.refund("unknown", {}), 0);
            auto unverified = refund(f, "unverified", "5", 0);
            unverified.correlation.reset();
            for (const auto &c : {wrong, unknown, unverified}) {
                refusal(f.refund_reply(c, first_quarter(), effects), D::invalid_financial_input);
                absent(f, c);
            }
            require(effects.adjustments == 0, "No financial effect applied by invalid raw input");
            f.check_head(300000, 0, 40000000, 7);
        });
    }

    void w03ZeroRoundedRequestStillAdvancesMoney() {
        check([] {
            File f(OriginalCredit{
                .amount = "1", .requested = 1, .main = 1, .cap_tail = 0, .score = 0, .count = 0});
            Effects e;
            const auto c = refund(f, "one-cent", "0.01", 0);
            const auto p = entitlement(fraction(0), fraction(1, 100), fraction(1, 100), false, 0,
                                       {}, {}, fraction(1, 100), fraction(1, 100), 0, 0);
            e.apply(0, 0, {});
            const auto r = f.accept(c, p, e);
            same_position(r, 1, p);
            timer_result(r, TD::applied, {});
            require(!r.hold, "A zero rounded request never creates a timer hold");
            history_progress(f, 1, fraction(1, 100));
            f.check_head(1, 0, 0, 0);
        });
    }

    void w03LinkFractionMustEqualJournal_data() {
        QTest::addColumn<bool>("score");
        QTest::newRow("score") << true;
        QTest::newRow("count") << false;
    }

    void w03LinkFractionMustEqualJournal() {
        QFETCH(bool, score);
        check([&] {
            File f;
            Effects effects;
            auto p = first_quarter();
            if (score) {
                p.components[0] = component(CK::score, 0, 0, 20000000, -20000000, fraction(1, 2),
                                            fraction(20000000), false);
            } else {
                p.components[1] = component(CK::whole_count, 0, 0, 0, 0, fraction(1, 2),
                                            fraction(7, 2), false, fraction(7, 2));
            }
            const auto c = refund(f, "different-fraction", "5", 0);
            refusal(f.refund_reply(c, p, effects), D::invalid_financial_input);
            absent(f, c);
            f.check_head(300000, 0, 40000000, 7);
        });
    }

    void w03GroupedTimeMustMatchJournalAndAllocateMainFirst_data() {
        QTest::addColumn<bool>("wrong_split");
        QTest::newRow("main-first") << true;
        QTest::newRow("exact-product") << false;
    }

    void w03GroupedTimeMustMatchJournalAndAllocateMainFirst() {
        QFETCH(bool, wrong_split);
        check([&] {
            File f(OriginalCredit{.amount = "8",
                                  .requested = 6,
                                  .main = 3,
                                  .repayment = 3,
                                  .cap_tail = 0,
                                  .score = 0,
                                  .count = 0});
            Effects effects;
            const auto c = refund(f, "invalid-time", "2", 0);
            const auto p = wrong_split
                               ? entitlement(fraction(0), fraction(2), fraction(1, 4), false, 0, {},
                                             {1, 1}, fraction(3, 2), fraction(-1, 2), -1, 1)
                               : entitlement(fraction(0), fraction(2), fraction(1, 4), false, 0, {},
                                             {2, 0}, fraction(2), fraction(0), -2, 0);
            refusal(f.refund_reply(c, p, effects), D::invalid_financial_input);
            absent(f, c);
            f.check_head(3, 0, 0, 0);
        });
    }

    void w03CrossPathHistory_data() {
        QTest::addColumn<int>("basis");
        QTest::addColumn<bool>("financial_first");
        for (const auto &[name, kind] : {std::pair{"time", CK::main_delta},
                                         {"score", CK::score},
                                         {"count", CK::whole_count}}) {
            QTest::newRow(qPrintable(QString(name) + "-legacy-first")) << int(kind) << false;
            QTest::newRow(qPrintable(QString(name) + "-financial-first")) << int(kind) << true;
        }
    }

    void w03CrossPathHistory() {
        QFETCH(int, basis);
        QFETCH(bool, financial_first);
        check([&] {
            File f;
            f.move_main("funded", 600000);
            Effects e;
            const auto kind = static_cast<CK>(basis);
            const auto attempt = refund(f, "financial", "5", 0);
            const auto p = first_quarter();
            if (financial_first) {
                e.apply(-75000, 0, {75000});
                (void)f.accept(attempt, p, e);
                const auto legacy = f.legacy_reversal("legacy", kind, true);
                const auto result = f.run_reply([&](const auto &s) {
                    return stage_financial_aware_nonfinancial_reversal(s, legacy);
                });
                refusal(result, D::basis_conflict);
                f.write([&](const auto &s) {
                    require(!SqliteApplicationBundleStore(s).lookup(legacy.intent.operation),
                            "Legacy alternate path stages no operation or effects");
                });
                history_progress(f, 1, fraction(5));
                f.check_head(525000, 0, 30000000, 7);
            } else {
                // A real zero-effect link advances its position without competing for
                // any cumulative quantity or implying accepted financial money.
                const auto legacy = f.legacy_reversal("legacy", kind, false);
                f.write([&](const auto &s) {
                    require(SqliteApplicationBundleStore(s).stage(legacy).disposition ==
                                SqliteBundleStageDisposition::staged,
                            "Shipped nonfinancial historical reversal is real and committed");
                });
                auto fresh = p;
                f.write([&](const auto &s) {
                    const auto selected = SqliteApplicationBundleStore(s).read_reversal_basis(
                        legacy.reversals.front().original);
                    require(selected.basis && selected.basis->progress.position == 1 &&
                                selected.basis->progress.cumulative == 0 &&
                                selected.basis->progress.cumulative_fraction == fraction(0),
                            "The legacy link advances only its real basis position");
                    if (kind == CK::main_delta) {
                        fresh.time->expected_basis_position = selected.basis->progress.position;
                    } else {
                        auto &component = fresh.components[kind == CK::score ? 0 : 1];
                        component.expected_prior_position = selected.basis->progress.position;
                    }
                });
                const auto current = f.command(attempt.operation.operation_ref, attempt.receipt, 0);
                e.apply(-75000, 0, {75000});
                refusal(f.refund_reply(current, fresh, e), D::basis_conflict);
                absent(f, current);
                f.check_head(600000, 0, 40000000, 7);
            }
        });
    }

    void w04AcceptanceAtomicity_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("late-sql-rollback") << 0;
        QTest::newRow("real-commit-lost-ack") << 1;
        QTest::newRow("acknowledged-commit-body-failure") << 2;
    }

    void w04AcceptanceAtomicity() {
        QFETCH(int, fault);
        check([&] {
            Mixed m;
            auto &f = m.f;
            const auto head_before = f.head().token;
            const auto jobs_before = f.rows(jobs);
            const auto groups_before = f.rows(groups);
            const auto audits_before = f.rows(audits);
            if (fault == 1)
                require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            *f.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Arm the real writer physical COMMIT acknowledgement fault");

            bool witnessed = false;
            bool after_commit = false;
            const auto result = f.work(
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    const auto reply =
                        stage_financial_refund(s, m.first, first_quarter(), f.authority, m.effects);
                    require(reply.disposition == D::applied, "Actual mixed outcome staged");
                    const auto staged = SqliteFinancialRefundStore(s).lookup(m.first.operation);
                    const auto h = SqliteFinancialRefundStore(s).history(f.payment, 16);
                    const auto observed = SqliteApplicationBundleStore(s).read_state();
                    require(staged && h && h->accepted_money == fraction(5) && h->position == 1 &&
                                observed && observed->token != head_before &&
                                observed->timer.main_remaining_ms == 50000 &&
                                observed->balances[0].units == 30000000 &&
                                SqliteFinancialRefundStore(s).active({f.cid}).size() == 1 &&
                                SqliteMilestoneGroupDecisions(s).refund_audit(achievement,
                                                                              "refund-5") &&
                                SqliteDeliveryJobStore(s).lookup({original, "original-job"}),
                            "Witness complete real staged mixed unit on the active session");
                    m.check_first(*staged);
                    require(f.rows("SELECT token FROM subathon_app_head") ==
                                    SqlRows{{bytes(head_before)}} &&
                                f.rows(audits) == audits_before && f.rows(decisions).empty() &&
                                f.rows(jobs) == jobs_before && f.rows(groups) == groups_before,
                            "Separate physical reader keeps seeing the previous commit");
                    witnessed = true;
                    if (fault == 0) {
                        // This deliberately duplicates an existing PK through a real SQLite write,
                        // after all of the new financial closure was witnessed in this same unit.
                        (void)s.execute(
                            {"INSERT INTO subathon_app_audit SELECT * FROM subathon_app_audit "
                             "WHERE operation_scope=? AND operation_ref=?",
                             {bytes(original.operation_scope), bytes(original.operation_ref)}});
                    }
                    s.commit();
                    after_commit = true;
                    if (fault == 2)
                        throw std::runtime_error(
                            "synthetic body failure after acknowledged COMMIT");
                    return QVariant::fromValue(reply);
                },
                false);

            require(witnessed, "Fault lies past the witnessed stage on this physical unit");
            require(!result.value && !acknowledged_financial_reply(result),
                    "Failed completion cannot publish a success reply");
            if (fault == 0) {
                require(result.transaction == TransactionOutcome::rolled_back &&
                            result.error == WorkError::sql_failure && !after_commit,
                        "Real late SQL error establishes known whole-unit rollback");
                absent(f, m.first);
                active(f, {});
                const auto h = f.history();
                require(!h || (h->position == 0 && h->refunds.empty() && h->accepted_money == 0),
                        "No money or position advanced on rollback");
                f.check_head(50000, 0, 40000000, 7);
                require(f.rows(audits) == audits_before && f.rows(decisions).empty() &&
                            f.rows(jobs) == jobs_before && f.rows(groups) == groups_before,
                        "Refund, milestone and job closure all stay at the previous commit");
                return;
            }
            if (fault == 1) {
                require(result.transaction == TransactionOutcome::outcome_unknown &&
                            result.error == WorkError::uncertain_outcome && !after_commit &&
                            f.writer->enqueue_work([](auto) { return QVariant{}; }, [](auto) {}) !=
                                StorageError::none,
                        "Lost physical COMMIT acknowledgement closes writer admission");
                f.reopen_closed();
            } else {
                require(result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::work_exception && after_commit,
                        "Acknowledged physical COMMIT followed by body failure is committed");
                f.reopen();
            }
            const auto durable = read_refund(f, m.first.operation);
            m.check_first(durable);
            require(
                durable.command.receipt == m.first.receipt &&
                    durable.command.expected_state_token == head_before &&
                    durable.command.expected_payment_position == 0 &&
                    durable.application.after.token == f.head().token &&
                    durable.application.audit.decision_ref == m.first.audit.decision_ref,
                "Fresh typed selected read reconciles exact immutable intent and retained effect");
            m.check_review();
            active(f, {held(f, durable, FinancialTimerHoldReason::deficit, 1, -75000, 0)});
            history_progress(f, 1, fraction(5));
            f.check_head(50000, 0, 30000000, 7);
            require(f.rows(audits).size() == audits_before.size() + 1 &&
                        f.rows(jobs) == jobs_before && f.rows(groups) == groups_before &&
                        m.effects.observations == 1,
                    "Exactly one reconciled application and no blind replay or job mutation");
            f.check_raw(m.first.receipt);
        });
    }

    void w04LateSqlFailurePreservesHeldAcceptance() {
        check([] {
            Mixed m;
            auto &f = m.f;
            const auto h = m.accept_first();
            f.move_main("resolution-funding", 100000);
            const auto c = f.continuation("close", h, {h.locator});
            m.effects.apply(-75000, 0, {75000});
            const auto head_before = f.head().token;
            const auto audits_before = f.rows(audits);
            const auto decisions_before = f.rows(decisions);
            const auto jobs_before = f.rows(jobs);
            bool witnessed = false;
            const auto result = f.work(
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    const auto reply =
                        stage_financial_timer_resolution(s, c, f.authority, m.effects);
                    const SqliteFinancialRefundStore store(s);
                    const auto staged = store.lookup_resolution(c.operation);
                    require(reply.disposition == D::resolved && staged &&
                                staged->after.timer.main_remaining_ms == 25000 &&
                                store.active({f.cid}).empty() &&
                                f.rows("SELECT token FROM subathon_app_head") ==
                                    SqlRows{{bytes(head_before)}},
                            "Real staged resolution changes effects and hold while committed "
                            "reader stays old");
                    witnessed = true;
                    (void)s.execute(
                        {"INSERT INTO subathon_app_audit SELECT * FROM subathon_app_audit "
                         "WHERE operation_scope=? AND operation_ref=?",
                         {bytes(original.operation_scope), bytes(original.operation_ref)}});
                    s.commit();
                    return QVariant::fromValue(reply);
                },
                false);
            require(witnessed, "Fault occurred after staging real resolution effects");
            require(result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::sql_failure &&
                        !acknowledged_financial_reply(result),
                    "Actual failed SQL append rolls back resolution with no response");
            f.write([&](const auto &s) {
                require(!SqliteFinancialRefundStore(s).lookup_resolution(c.operation),
                        "Fresh selected resolution closure is absent after rollback");
            });
            active(f, {h});
            history_progress(f, 1, fraction(5));
            f.check_head(100000, 0, 30000000, 7);
            m.check_first(read_refund(f, m.first.operation));
            require(f.rows(audits) == audits_before && f.rows(decisions) == decisions_before &&
                        f.rows(jobs) == jobs_before,
                    "Prior committed mixed outcome and original jobs survive exactly");
        });
    }

    void w05DeficitAndAuditedContinuation_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("funded") << 0;
        QTest::newRow("deficient-carry") << 1;
        QTest::newRow("deficient-stop") << 2;
    }

    void w05DeficitAndAuditedContinuation() {
        QFETCH(int, mode);
        check([&] {
            Mixed m;
            auto &f = m.f;
            const auto h = m.accept_first();
            const auto milestone_reads = m.effects.milestone_reads;
            auto c = f.continuation("resolution", h, {h.locator});
            if (mode == 0) {
                f.authority.execution = false;
                refusal(f.continue_reply(c, m.effects), D::denied_execution);
                f.authority.execution = true;
                auto stale = c;
                stale.origin.version_ref = "wrong-version";
                stale.expected_active = {stale.origin};
                refusal(f.continue_reply(stale, m.effects), D::stale_hold);
                active(f, {h});
                f.move_main("authorized-credit", 100000);
            } else if (mode == 1) {
                refusal(f.continue_reply(c, m.effects), D::deficit);
                active(f, {h});
                f.authority.override_timer = false;
                const auto denied =
                    f.continuation("override-denied", h, {h.locator}, Policy::CarryTimeDebt);
                refusal(f.continue_reply(denied, m.effects), D::denied_execution);
                f.authority.override_timer = true;
                f.check_head(50000, 0, 30000000, 7);
            }
            const auto policy = mode == 0   ? Policy::RequireReview
                                : mode == 1 ? Policy::CarryTimeDebt
                                            : Policy::StopAtZero;
            const auto override = mode == 0 ? std::nullopt : std::optional{policy};
            c = f.continuation("resolution", h, {h.locator}, override);
            const RefundTimerActual outcome{mode == 0 ? 75000 : 50000, 0, mode == 1 ? 25000 : 0,
                                            mode == 2 ? 25000 : 0, 0};
            m.effects.apply(-75000, 0, outcome, policy);
            const auto first = resolve(f, c, m.effects, {TD::applied, outcome});
            const auto again = resolve(f, c, m.effects, {TD::applied, outcome}, D::existing);
            require(same(first.closed.locator, again.closed.locator) &&
                        first.after.token == again.after.token,
                    "Exact retry discloses the same immutable resolution");
            const auto fresh = read_resolution(f, c.operation);
            require(same(fresh.closed.locator, first.closed.locator) && fresh.timer == first.timer,
                    "Fresh committed read returns the same single closure");
            auto competing = c;
            competing.operation.operation_ref = "competing-resolution";
            competing.expected_state_token = f.head().token;
            refusal(f.continue_reply(competing, m.effects), D::stale_hold);
            // Compare the acceptance closure after resolution, including its original held
            // disposition.
            m.check_first(read_refund(f, m.first.operation));
            m.check_review();
            active(f, {});
            f.check_head(mode == 0 ? 25000 : 0, mode == 1 ? 25000 : 0, 30000000, 7);
            history_progress(f, 1, fraction(5), 1);
            require(m.effects.adjustments == 1 && m.effects.milestone_reads == milestone_reads,
                    "Only one physical adjustment and no extra milestone policy observation");
        });
    }

    void w05MixedAndFinancialOnlyActiveSets_data() {
        QTest::addColumn<bool>("correction");
        QTest::newRow("whole-input") << false;
        QTest::newRow("timer-correction") << true;
    }

    void w05MixedAndFinancialOnlyActiveSets() {
        QFETCH(bool, correction);
        check([&] {
            Mixed m;
            auto &f = m.f;
            const auto financial = m.accept_first();
            const auto milestone_reads = m.effects.milestone_reads;
            const auto first = f.detect(f.ordinary_detection("ordinary-a", correction));
            const auto second = f.detect(f.ordinary_detection("ordinary-b", false));
            const auto link = f.ordinary_link(correction, first);
            const auto audit_before_refusals = f.rows(audits);
            const auto blocked_refund = refund(f, "blocked", "2", 1);
            auto later =
                entitlement(fraction(5), fraction(7), fraction(7, 20), false, 1, {75000, 0},
                            {105000, 0}, fraction(105000), fraction(0), -30000, 0);
            later.components = {component(CK::score, 1, 10000000, 14000000, -4000000,
                                          fraction(7, 20), fraction(14000000), false),
                                component(CK::whole_count, 1, 0, 0, 0, fraction(7, 20),
                                          fraction(49, 20), false, fraction(49, 20))};
            refusal(f.refund_reply(blocked_refund, later, m.effects), D::active_hold);
            absent(f, blocked_refund);
            const auto blocked =
                f.continuation("blocked-close", financial, {first, second, financial.locator});
            refusal(f.continue_reply(blocked, m.effects), D::active_hold);
            // Complete set order is by key. Closure entries name only the nonfinancial holds.
            ApplicationHoldResolutionBatch batch{
                "clear-ordinary",
                {f.cid},
                blocked.expected_active,
                {{first.hold_ref, first.version_ref, "v2", document(), {}},
                 {second.hold_ref, second.version_ref, "v2", document(), {}}},
                "tester",
                "clear-ordinary",
                "independent ordinary clearance",
                document(),
                link};
            const auto apply_ordinary = [&](const ApplicationHoldResolutionBatch &candidate) {
                return f.run_reply([&](const auto &s) {
                    return stage_financial_aware_linked_resolution(s, candidate);
                });
            };
            auto omitted_entry = batch;
            omitted_entry.mutation_ref = "omit-entry";
            omitted_entry.entries.pop_back();
            refusal(apply_ordinary(omitted_entry), D::refused);
            auto omitted_active = batch;
            omitted_active.mutation_ref = "omit-active";
            std::erase_if(omitted_active.expected_active,
                          [&](const auto &loc) { return same(loc, financial.locator); });
            refusal(apply_ordinary(omitted_active), D::stale_hold);
            auto replacement = batch;
            replacement.mutation_ref = "replace-ordinary";
            auto proposed = f.ordinary_detection("ordinary-replacement", false);
            proposed.cause.supersedes_hold_ref = second.hold_ref;
            replacement.entries[1].replacement = proposed;
            refusal(apply_ordinary(replacement), D::refused);
            f.write([&](const auto &s) {
                const SqliteApplicationHoldStore store(s);
                for (const auto &h : {first, second}) {
                    const auto view = store.find(h.hold_ref);
                    require(view && view->current_version.state == ApplicationHoldState::active &&
                                same(view->current_version.locator, h),
                            "Blocked/refused attempts preserve both exact ordinary versions");
                }
                require(!store.find("ordinary-replacement"), "No replacement was inserted");
            });
            active(f, {financial});
            require(f.rows(audits) == audit_before_refusals,
                    "Refusals append no application audit");
            history_progress(f, 1, fraction(5));
            f.check_head(50000, 0, 30000000, 7);

            const auto cleared = apply_ordinary(batch);
            require(cleared.disposition == D::ordinary_resolved &&
                        std::get<ApplicationHoldMutationOutcome>(cleared.record)
                                .produced_versions.size() == 2,
                    "Valid linked resolution closes all and only the ordinary entries");
            f.write([&](const auto &s) {
                const SqliteApplicationHoldStore store(s);
                for (const auto &h : {first, second}) {
                    const auto view = store.find(h.hold_ref);
                    require(view && view->current_version.state == ApplicationHoldState::resolved &&
                                view->current_version.parent_version_ref == h.version_ref,
                            "Both exact ordinary versions were independently resolved");
                }
            });
            active(f, {financial});
            auto only_financial = batch;
            only_financial.mutation_ref = "financial-only";
            only_financial.expected_active = {financial.locator};
            only_financial.entries.clear();
            refusal(apply_ordinary(only_financial), D::refused);
            auto consume = only_financial;
            consume.mutation_ref = "consume-financial";
            consume.entries = {{financial.locator.hold_ref,
                                financial.locator.version_ref,
                                "ordinary-v2",
                                document(),
                                {}}};
            if (correction)
                consume.application->held_origin->version = financial.locator;
            refusal(apply_ordinary(consume), D::refused);
            active(f, {financial});
            f.move_main("fresh-authorized-credit", 100000);
            const auto next =
                f.continuation("authorized-financial-close", financial, {financial.locator});
            m.effects.apply(-75000, 0, {75000});
            (void)resolve(f, next, m.effects, {TD::applied, {75000}});
            active(f, {});
            history_progress(f, 1, fraction(5), 1);
            f.check_head(25000, 0, 30000000, 7);
            m.check_review();
            require(m.effects.milestone_reads == milestone_reads,
                    "Clearing through either linked kind does not replay milestone policy");
        });
    }

    void w05OrderedRequestsAcrossReopen_data() {
        QTest::addColumn<bool>("finished");
        QTest::newRow("resolve-in-payment-order") << false;
        QTest::newRow("finished-acceptance-behind-two-holds") << true;
    }

    void w05OrderedRequestsAcrossReopen() {
        QFETCH(bool, finished);
        check([&] {
            Mixed m;
            auto &f = m.f;
            const auto h1 = m.accept_first();
            f.move_main("fund-ordered-requests", 200000);

            auto p2 = entitlement(fraction(5), fraction(7), fraction(7, 20), false, 1, {75000, 0},
                                  {105000, 0}, fraction(105000), fraction(0), -30000, 0);
            p2.components = {component(CK::score, 1, 10000000, 14000000, -4000000, fraction(7, 20),
                                       fraction(14000000), false),
                             component(CK::whole_count, 1, 0, 0, 0, fraction(7, 20),
                                       fraction(49, 20), false, fraction(49, 20))};
            const auto c2 = refund(f, "refund-2", "2", 1);
            const auto r2 = f.accept(c2, p2, m.effects);
            same_position(r2, 2, p2);
            timer_result(r2, TD::held, {0, 0, 0, 30000, 0});
            const auto h2 = held(f, r2, FinancialTimerHoldReason::prior_request, 2, -30000, 0);
            f.check_head(200000, 0, 26000000, 7);
            history_progress(f, 2, fraction(7));

            if (finished)
                f.move_main("ordinary-finish", 0, Life::Finished,
                            {Transition::MainExpired, Transition::Finished});
            auto p3 = entitlement(fraction(7), fraction(10), fraction(1, 2), false, 2, {105000, 0},
                                  {150000, 0}, fraction(150000), fraction(0), -45000, 0);
            p3.components = {component(CK::score, 2, 14000000, 20000000, -6000000, fraction(1, 2),
                                       fraction(20000000), false),
                             component(CK::whole_count, 2, 0, 0, 0, fraction(1, 2), fraction(7, 2),
                                       false, fraction(7, 2))};
            const auto c3 = refund(f, "refund-3", "3", 2);
            const auto r3 = f.accept(c3, p3, m.effects);
            same_position(r3, 3, p3);
            timer_result(r3, finished ? TD::finished : TD::held, {0, 0, 0, 45000, 0});
            std::vector<FinancialTimerHold> pending{h1, h2};
            if (finished)
                require(!r3.hold, "Finished behind a prior hold creates no new timer hold");
            else
                pending.push_back(
                    held(f, r3, FinancialTimerHoldReason::prior_request, 3, -45000, 0));
            require(m.effects.adjustments == 0,
                    "Neither a prior request nor Finished mutates Main");
            f.check_head(finished ? 0 : 200000, 0, 20000000, 7,
                         finished ? Life::Finished : Life::Active);
            history_progress(f, 3, fraction(10));

            f.reopen();
            active(f, pending);
            const auto persisted = f.history();
            require(persisted && persisted->refunds.size() == 3,
                    "Reopen retains all three payments and immutable request allocations");
            same_position(persisted->refunds[0], 1, first_quarter());
            same_position(persisted->refunds[1], 2, p2);
            same_position(persisted->refunds[2], 3, p3);
            const auto locators = [&]() {
                std::vector<ApplicationHoldVersionLocator> selected;
                for (const auto &h : pending)
                    selected.push_back(h.locator);
                return selected;
            };
            const auto before_wrong_order = f.rows(audits);
            const auto out_of_order = f.continuation("too-early", h2, locators());
            refusal(f.continue_reply(out_of_order, m.effects), D::out_of_order);
            require(f.rows(audits) == before_wrong_order,
                    "Ordering refusal leaves the audit unchanged");
            active(f, pending);
            const auto milestone_reads = m.effects.milestone_reads;

            std::optional<FinancialTimerCommand> first_closure;
            std::optional<FinancialTimerResolution> first_outcome;
            const Number removals[]{75000, 30000, 45000};
            const Number remaining[]{125000, 95000, 50000};
            for (size_t i = 0; !pending.empty(); ++i) {
                const auto request = pending.front();
                // Keep one explicit audited override even when live Main covers the request.
                const auto override =
                    !finished && i == 0 ? std::optional{Policy::CarryTimeDebt} : std::nullopt;
                const auto c = f.continuation("ordered-close-" + std::to_string(i), request,
                                              locators(), override);
                if (!finished)
                    m.effects.apply(-removals[i], 0, {removals[i]},
                                    override.value_or(Policy::RequireReview));
                const RefundTimerOutcome wanted =
                    finished ? RefundTimerOutcome{TD::finished, {0, 0, 0, removals[i], 0}}
                             : RefundTimerOutcome{TD::applied, {removals[i]}};
                const auto r = resolve(f, c, m.effects, wanted);
                if (i == 0) {
                    first_closure = c;
                    first_outcome = r;
                }
                pending.erase(pending.begin());
                active(f, pending);
                f.check_head(finished ? 0 : remaining[i], 0, 20000000, 7,
                             finished ? Life::Finished : Life::Active);
                history_progress(f, 3, fraction(10), i + 1);
            }

            const auto replay = f.accept(c2, p2, m.effects, D::existing);
            const auto replay_hold =
                held(f, replay, FinancialTimerHoldReason::prior_request, 2, -30000, 0);
            require(same(replay_hold.locator, h2.locator) &&
                        replay.application.after.timer.main_remaining_ms == 200000 &&
                        replay.application.after.balances[0].units == 26000000,
                    "Acceptance retry preserves the historical paid/score/held outcome");
            const auto replay_close =
                resolve(f, *first_closure, m.effects, first_outcome->timer, D::existing);
            require(replay_close.after.token == first_outcome->after.token &&
                        same(replay_close.closed.locator, first_outcome->closed.locator),
                    "Resolution retry preserves its own original closure after later requests");
            m.check_review();
            require(
                m.effects.milestone_reads == milestone_reads &&
                    m.effects.adjustments == (finished ? 0 : 3),
                "Replay and ordered continuation add neither milestone reads nor extra effects");
        });
    }

    void w06FinishedAndReady_data() {
        QTest::addColumn<int>("lifecycle");
        QTest::newRow("already-finished") << 0;
        QTest::newRow("paused-at-hard-end-equality") << 1;
        QTest::newRow("ready-prerequisite") << 2;
    }

    void w06FinishedAndReady() {
        QFETCH(int, lifecycle);
        check([&] {
            File f(OriginalCredit{.milestone = lifecycle != 2, .ready = lifecycle == 2});
            Effects e;
            if (lifecycle == 2) {
                const auto c = refund(f, "ready-refund", "5", 0);
                refusal(f.refund_reply(c, first_quarter(), e), D::prerequisite);
                absent(f, c);
                require(e.observations == 0 && e.adjustments == 0,
                        "Ready never invokes the started observation/effect path");
                f.check_head(0, 0, 0, 0, Life::Ready);
                return;
            }
            perform_original_action(f);
            const auto performed = f.rows(jobs);
            const auto earned = f.rows(groups);
            e.milestone = true;
            if (lifecycle == 0) {
                f.move_main("prior-finish", 0, Life::Finished,
                            {Transition::MainExpired, Transition::Finished});
            } else {
                e.hard_end = 100;
                e.paused = true;
                e.observed = timer(Life::Finished, 300000);
                e.transitions = {Transition::HardEndReached, Transition::Finished};
            }
            const auto c = refund(f, "finished-refund", "5", 0);
            const auto r = f.accept(c, first_quarter(), e);
            same_position(r, 1, first_quarter());
            timer_result(r, TD::finished, {0, 0, 0, 75000, 0});
            require(!r.hold && e.adjustments == 0 && r.application.transitions == e.transitions &&
                        r.application.advanced_before.timer.lifecycle == Life::Finished,
                    "Clock records ordered hard-end/finish at equality, then refund leaves timer "
                    "alone");
            f.check_head(lifecycle == 0 ? 0 : 300000, 0, 30000000, 7, Life::Finished);
            active(f, {});
            history_progress(f, 1, fraction(5));
            require(f.rows(jobs) == performed && f.rows(groups) == earned &&
                        f.rows(decisions).empty(),
                    "Finished preserves performed actions and the achieved milestone");
        });
    }

    void w06ContinuationObservesFinish_data() {
        QTest::addColumn<bool>("hard_end");
        QTest::newRow("hard-end") << true;
        QTest::newRow("ordinary-main-expiry") << false;
    }

    void w06ContinuationObservesFinish() {
        QFETCH(bool, hard_end);
        check([&] {
            Mixed m;
            auto &f = m.f;
            const auto h = m.accept_first();
            const auto milestone_reads = m.effects.milestone_reads;
            m.effects.observed = timer(Life::Finished, hard_end ? 50000 : 0);
            m.effects.transitions = {hard_end ? Transition::HardEndReached
                                              : Transition::MainExpired,
                                     Transition::Finished};
            if (hard_end) {
                m.effects.hard_end = 100;
                m.effects.paused = true;
            } else {
                m.effects.elapsed = 50000;
            }
            const auto c = f.continuation("finish-close", h, {h.locator});
            const auto closure = resolve(f, c, m.effects, {TD::finished, {0, 0, 0, 75000, 0}});
            require(closure.transitions == m.effects.transitions &&
                        closure.before.token == c.expected_state_token,
                    "First detecting finish records clock transitions with exact closure");
            const auto retry = resolve(f, c, m.effects, closure.timer, D::existing);
            require(retry.transitions == closure.transitions &&
                        same(retry.closed.locator, closure.closed.locator),
                    "Finished retry returns immutable audited closure and transition history");
            f.check_head(hard_end ? 50000 : 0, 0, 30000000, 7, Life::Finished);
            active(f, {});
            history_progress(f, 1, fraction(5), 1);
            m.check_review();
            require(
                m.effects.adjustments == 0 && m.effects.milestone_reads == milestone_reads,
                "35A performs zero timer adjustment and makes no repeated milestone observation");
        });
    }

    void w07RestorationAndExactExhaustion_data() {
        QTest::addColumn<int>("second_main");
        QTest::newRow("full-three-part-history") << 8;
        QTest::newRow("exact-exhaustion") << 1;
        QTest::newRow("deficit-holds-both-legs") << 0;
    }

    void w07RestorationAndExactExhaustion() {
        QFETCH(int, second_main);
        check([&] {
            File f(OriginalCredit{.amount = "8",
                                  .requested = 6,
                                  .main = 3,
                                  .repayment = 3,
                                  .cap_tail = 0,
                                  .score = 40000000,
                                  .count = 0});
            f.move_main("independent-credit", 10);
            Effects e;
            const auto c1 = refund(f, "refund-2", "2", 0);
            auto p1 = entitlement(fraction(0), fraction(2), fraction(1, 4), false, 0, {}, {2, 0},
                                  fraction(3, 2), fraction(-1, 2), -2, 0);
            p1.components = {component(CK::score, 0, 0, 10000000, -10000000, fraction(1, 4),
                                       fraction(10000000), false)};
            e.apply(-2, 0, {2});
            const auto r1 = f.accept(c1, p1, e);
            same_position(r1, 1, p1);
            timer_result(r1, TD::applied, {2});
            f.check_head(8, 0, 30000000, 0);

            if (second_main != 8)
                f.move_main("second-observation", second_main);
            const auto c2 = refund(f, "refund-4", "4", 1);
            auto p2 = entitlement(fraction(2), fraction(6), fraction(3, 4), false, 1, {2, 0},
                                  {3, 1}, fraction(9, 2), fraction(1, 2), -1, 1);
            p2.components = {component(CK::score, 1, 10000000, 30000000, -20000000, fraction(3, 4),
                                       fraction(30000000), false)};
            if (second_main != 0)
                e.apply(-1, 1, {1, 1});
            const auto r2 = f.accept(c2, p2, e);
            same_position(r2, 2, p2);
            if (second_main == 0) {
                timer_result(r2, TD::held, {0, 0, 0, 1, 1});
                const auto h = held(f, r2, FinancialTimerHoldReason::deficit, 2, -1, 1);
                active(f, {h});
                f.check_head(0, 0, 10000000, 0);
                f.move_main("fund-both-legs", 1);
                const auto c = f.continuation("restore-and-remove", h, {h.locator});
                e.apply(-1, 1, {1, 1});
                (void)resolve(f, c, e, {TD::applied, {1, 1}});
                f.check_head(0, 1, 10000000, 0);
                history_progress(f, 2, fraction(6), 1);
                return;
            }
            timer_result(r2, TD::applied, {1, 1});
            require(!r2.hold,
                    "Exact exhaustion and original restoration both apply under RequireReview");
            f.check_head(second_main == 1 ? 0 : 7, 1, 10000000, 0);
            if (second_main == 1) {
                history_progress(f, 2, fraction(6));
                return;
            }

            const auto c3 = refund(f, "last-2", "2", 2);
            auto p3 = entitlement(fraction(6), fraction(8), fraction(1), true, 2, {3, 1}, {3, 3},
                                  fraction(6), fraction(0), 0, 2);
            p3.components = {component(CK::score, 2, 30000000, 40000000, -10000000, fraction(1),
                                       fraction(40000000), true)};
            e.apply(0, 2, {0, 2});
            const auto r3 = f.accept(c3, p3, e);
            timer_result(r3, TD::applied, {0, 2});
            require(!r3.hold, "Restoration-only works under RequireReview");
            f.check_head(7, 3, 0, 0);
            const auto h = f.history();
            require(h && h->position == 3 && h->accepted_money == 8 && h->refunds.size() == 3 &&
                        h->resolutions.empty(),
                    "Single committed ordered grouped-time history");
            same_position(h->refunds[0], 1, p1);
            same_position(h->refunds[1], 2, p2);
            same_position(h->refunds[2], 3, p3);
            f.write([&](const auto &s) {
                const auto source = SqliteApplicationBundleStore(s).lookup(original);
                require(source && source->effects[0].components[0].units == 3 &&
                            source->effects[0].components[1].kind == CK::debt_delta &&
                            source->effects[0].components[1].units == -3,
                        "Original negative debt occurrence remains exactly retained");
            });
        });
    }

    void w07ZeroAndEitherLegBehindPriorHold() {
        check([] {
            File f(OriginalCredit{.amount = "8",
                                  .requested = 6,
                                  .main = 3,
                                  .repayment = 3,
                                  .cap_tail = 0,
                                  .score = 0,
                                  .count = 0});
            f.move_main("elapsed-to-zero", 0);
            Effects e;
            const auto c1 = refund(f, "first-2", "2", 0);
            const auto p1 = entitlement(fraction(0), fraction(2), fraction(1, 4), false, 0, {},
                                        {2, 0}, fraction(3, 2), fraction(-1, 2), -2, 0);
            const auto first = f.accept(c1, p1, e);
            const auto h1 = held(f, first, FinancialTimerHoldReason::deficit, 1, -2, 0);
            f.move_main("new-main-sufficient", 10);

            // 2.01/8 = 201/800; times 6 = 603/400, still rounds to 2.
            const auto zero = refund(f, "zero", "0.01", 1);
            const auto p0 =
                entitlement(fraction(2), fraction(201, 100), fraction(201, 800), false, 1, {2, 0},
                            {2, 0}, fraction(603, 400), fraction(-197, 400), 0, 0);
            e.apply(0, 0, {});
            const auto z = f.accept(zero, p0, e);
            same_position(z, 2, p0);
            timer_result(z, TD::applied, {});
            require(!z.hold, "While a prior hold is active, zero legs consume money only");

            const auto c2 = refund(f, "both-legs", "3.99", 2);
            const auto p2 = entitlement(fraction(201, 100), fraction(6), fraction(3, 4), false, 2,
                                        {2, 0}, {3, 1}, fraction(9, 2), fraction(1, 2), -1, 1);
            const auto both = f.accept(c2, p2, e);
            same_position(both, 3, p2);
            const auto h2 = held(f, both, FinancialTimerHoldReason::prior_request, 3, -1, 1);
            timer_result(both, TD::held, {0, 0, 0, 1, 1});

            const auto c3 = refund(f, "restoration-only", "2", 3);
            const auto p3 = entitlement(fraction(6), fraction(8), fraction(1), true, 3, {3, 1},
                                        {3, 3}, fraction(6), fraction(0), 0, 2);
            const auto restore = f.accept(c3, p3, e);
            same_position(restore, 4, p3);
            const auto h3 = held(f, restore, FinancialTimerHoldReason::prior_request, 4, 0, 2);
            timer_result(restore, TD::held, {0, 0, 0, 0, 2});
            f.check_head(10, 0, 0, 0);
            active(f, {h1, h2, h3});
            history_progress(f, 4, fraction(8));

            e.apply(-2, 0, {2});
            (void)resolve(f,
                          f.continuation("close-first", h1, {h1.locator, h2.locator, h3.locator}),
                          e, {TD::applied, {2}});
            e.apply(-1, 1, {1, 1});
            (void)resolve(f, f.continuation("close-both", h2, {h2.locator, h3.locator}), e,
                          {TD::applied, {1, 1}});
            e.apply(0, 2, {0, 2});
            (void)resolve(f, f.continuation("close-restoration", h3, {h3.locator}), e,
                          {TD::applied, {0, 2}});
            f.check_head(7, 3, 0, 0);
            active(f, {});
            history_progress(f, 4, fraction(8), 3);
        });
    }

    void w07HalfEvenTieAndExactFullClearing_data() {
        QTest::addColumn<bool>("full");
        QTest::newRow("half-even-2.5-to-2") << false;
        QTest::newRow("full-clears-one-main-and-four-restoration") << true;
    }

    void w07HalfEvenTieAndExactFullClearing() {
        QFETCH(bool, full);
        check([&] {
            File f(OriginalCredit{.amount = "10",
                                  .requested = full ? 10 : 5,
                                  .main = full ? 6 : 3,
                                  .repayment = full ? 4 : 2,
                                  .cap_tail = 0,
                                  .score = 0,
                                  .count = 0});
            f.move_main("independent-credit", 10);
            Effects e;
            const auto c1 = refund(f, "half", "5", 0);
            const auto half = full ? entitlement(fraction(0), fraction(5), fraction(1, 2), false, 0,
                                                 {}, {5, 0}, fraction(5), fraction(0), -5, 0)
                                   : entitlement(fraction(0), fraction(5), fraction(1, 2), false, 0,
                                                 {}, {2, 0}, fraction(5, 2), fraction(1, 2), -2, 0);
            e.apply(full ? -5 : -2, 0, {full ? 5 : 2});
            same_position(f.accept(c1, half, e), 1, half);
            f.check_head(full ? 5 : 8, 0, 0, 0);
            if (full) {
                const auto c2 = refund(f, "clear", "5", 1);
                const auto clear = entitlement(fraction(5), fraction(10), fraction(1), true, 1,
                                               {5, 0}, {6, 4}, fraction(10), fraction(0), -1, 4);
                e.apply(-1, 4, {1, 4});
                const auto r = f.accept(c2, clear, e);
                same_position(r, 2, clear);
                timer_result(r, TD::applied, {1, 4});
                f.check_head(4, 4, 0, 0);
                history_progress(f, 2, fraction(10));
            }
        });
    }
};
} // namespace

QTEST_GUILESS_MAIN(SqliteFinancialRefundsTests)
#include "sqlite_financial_refunds_tests.moc"
