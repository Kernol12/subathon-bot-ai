// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_financial_refunds_fixture.hpp"
#include <subathon/server/contribution_review.hpp>
#include <subathon/server/financial_refund_application.hpp>
#include <subathon/server/reference_runtime.hpp>
#include <subathon/server/sqlite_milestone_target_edits.hpp>

namespace live_financial_refund_fixture {
using namespace financial_refund_fixture;
using End = core::TimerEndMode;
using Phase = core::CountdownPhase;
using Reuse = core::GraceReuse;
using HoldReason = FinancialTimerHoldReason;

inline constexpr Number maximum = std::numeric_limits<Number>::max();
inline constexpr Number minimum = std::numeric_limits<Number>::min();
inline const MilestoneTrackKey track{"install", "score-track"};

inline bool same_timer(const core::TimerState &a, const core::TimerState &b) {
    return a.lifecycle == b.lifecycle && a.phase == b.phase &&
           a.main_remaining_ms == b.main_remaining_ms &&
           a.grace_remaining_ms == b.grace_remaining_ms &&
           a.awaiting_confirmation == b.awaiting_confirmation;
}

inline ApplicationClockObservation clock_for(const ApplicationStateSnapshot &head, Number utc,
                                             std::vector<core::TimerClockStep> cuts = {}) {
    return {"live-financial-lineage", "cut-" + head.token, head.configuration_ref,
            "live-financial-policy",  head.token,          utc,
            std::move(cuts)};
}

inline FileSeed shipped_seed(FileSeed seed = {}) {
    auto manifest = contribution_review_repository_manifest();
    require(manifest.steps.size() == 19 && manifest.steps.back().id == "contribution-review-v1",
            "Shipped v19 manifest, including the v15 financial tables");
    seed.manifest = std::move(manifest);
    seed.encode_state = [](auto state, auto basis) {
        auto clock = clock_for(state, 90);
        if (basis)
            clock.checked_head_token = *basis;
        return encode_reference_state({state, std::move(basis), clock, clock.policy_revision_ref});
    };
    return seed;
}

struct Inputs final : LiveFinancialRefundInputs {
    core::TimerConfig config{End::Immediate, 19, Reuse::Fresh, {}};
    std::optional<Policy> policy{Policy::RequireReview};
    std::optional<MilestoneRefundPolicy> milestone_policy{MilestoneRefundPolicy::review};
    std::optional<Number> cap;
    MilestoneTrackKey selected_track{track};
    std::vector<core::TimerClockStep> cuts{{0, 100, false}};
    bool lineage{true};
    bool recovery_pending{false};
    int calls{};

    std::optional<LiveFinancialRefundFacts> select(const SqliteWorkSession &s,
                                                   const ApplicationStateSnapshot &head) override {
        ++calls;
        const auto current = SqliteApplicationBundleStore(s).read_state();
        require(current && current->token == head.token &&
                    current->envelope.document == head.envelope.document,
                "Inputs were selected in the held writer unit against its exact head");
        auto clock = clock_for(head, cuts.back().utc_now_ms, cuts);
        if (!lineage)
            clock.clock_lineage_ref.clear();
        return LiveFinancialRefundFacts{std::move(clock),
                                        config,
                                        "pause-recovery-checked",
                                        cuts.back().paused,
                                        recovery_pending,
                                        cap,
                                        policy,
                                        selected_track,
                                        milestone_policy};
    }
};

class ScopedAuthority final : public FinancialRefundAuthority {
  public:
    ScopedAuthority(FinancialRefundAuthority &source, const ApplicationOperationKey &operation,
                    const FinancialPaymentKey &payment)
        : source_(source), operation_(operation), payment_(payment) {}

    bool allows(const SqliteWorkSession &s, FinancialAuthorityUse use,
                const ApplicationPrincipalBinding &who, const ApplicationOperationKey &operation,
                const FinancialPaymentKey &payment) override {
        require(same(operation, operation_) && payment == payment_,
                "Current authority is selected for the exact scoped operation and payment");
        return source_.allows(s, use, who, operation, payment);
    }

  private:
    FinancialRefundAuthority &source_;
    const ApplicationOperationKey &operation_;
    const FinancialPaymentKey &payment_;
};

struct Fixture {
    File file;
    Inputs input;

    explicit Fixture(OriginalCredit credit, FileSeed seed = {}, bool empty_track = true)
        : file(credit, shipped_seed(seed)) {
        file.write([&](const auto &s) {
            const auto state = SqliteApplicationBundleStore(s).read_state();
            const auto selected = SqliteApplicationBundleStore(s).read_inputs(
                original_plan, ApplicationSubjectKind::contribution);
            require(state && decode_reference_state(*state).snapshot.token == state->token &&
                        selected.inputs && selected.inputs->current_contribution &&
                        selected.inputs->selected_plan.captured.event.receipt_id ==
                            file.payment.support_receipt,
                    "Coherent SAS1, retained original capture and real contribution");
            if (!credit.ready) {
                const auto source = SqliteApplicationBundleStore(s).lookup(original);
                require(source &&
                            source->persisted_head.token ==
                                (seed.debt_after ? "prior-debt-awarded" : "initial") &&
                            source->after.time_debt_ms == 0 &&
                            state->time_debt_ms == seed.debt_after && !source->effects.empty() &&
                            (seed.omit_time ||
                             (source->effects.front().components[0].units == credit.main &&
                              source->effects.front().components[1].units == -credit.repayment &&
                              source->effects.front().components[2].units == credit.cap_tail)),
                        "The committed original retains the exact actual credited components");
            }
            if (empty_track)
                require(initialize_milestone_track(s, {track,
                                                       "cfg",
                                                       "current-definition",
                                                       "current-prefix",
                                                       {},
                                                       std::vector<MilestoneTrackSeed::Member>{}}),
                        "Authoritative empty current track and completed prefix are known");
        });
    }

    FinancialRefundCommand command(const std::string &name, const char *amount, Number position) {
        auto c = file.command(
            name, file.refund(name + "-delivery", core::ReceiptMoney{amount, "EUR"}), position);
        file.check_raw(c.receipt);
        require(c.correlation && c.correlation->payment == file.payment &&
                    c.correlation->contribution == file.cid,
                "Retained raw refund and synthetic verified correlation to this original");
        return c;
    }

    FinancialWorkReply stage(const SqliteWorkSession &s, const FinancialRefundCommand &c,
                             const FinancialEntitlement &p) {
        ScopedAuthority authority(file.authority, c.operation, file.payment);
        return stage_live_financial_refund(s, c, p, authority, input);
    }

    FinancialWorkReply stage(const SqliteWorkSession &s, const FinancialTimerCommand &c) {
        ScopedAuthority authority(file.authority, c.operation, file.payment);
        return stage_live_financial_timer_resolution(s, c, authority, input);
    }

    FinancialWorkReply refund(const FinancialRefundCommand &c, const FinancialEntitlement &p) {
        return file.run_reply([&](const auto &s) { return stage(s, c, p); });
    }

    FinancialWorkReply resolve(const FinancialTimerCommand &c) {
        return file.run_reply([&](const auto &s) { return stage(s, c); });
    }

    // Separate committed fixture application. Its observation and direct
    // balance effects go through the shipped bundle/effect/plan validators.
    void move(const std::string &ref, core::TimerState new_timer,
              std::vector<Transition> transitions = {},
              std::optional<Number> other_score_delta = {}) {
        const auto receipt = file.intake(ref, {}, "synthetic-owner-command");
        file.write([&](const auto &s) {
            auto b = File::receipt_application(s, receipt, ref);
            const auto prior = b.advanced_before.token;
            const auto before = b.advanced_before.timer;
            if (!same_timer(before, new_timer)) {
                if (before.lifecycle == Life::Active && new_timer.lifecycle == Life::Active &&
                    new_timer.phase == Phase::Main &&
                    new_timer.main_remaining_ms > before.main_remaining_ms) {
                    const auto delta = new_timer.main_remaining_ms - before.main_remaining_ms;
                    require(b.advanced_before.time_debt_ms == 0,
                            "Independent Main top-up has no outstanding debt");
                    b.effects.push_back(addition(delta, delta, 0, 0));
                } else {
                    b.advanced_before.token = ref + "-cut";
                    b.advanced_before.timer = new_timer;
                    b.advanced_before = file.encoded(std::move(b.advanced_before), prior);
                    b.advance_explanation = "controlled earlier observation";
                }
                b.after.timer = new_timer;
            }
            if (other_score_delta) {
                b.effects.push_back(direct(
                    Number(b.effects.size()) + 1,
                    request(Unit::micro_points, *other_score_delta, "other-score"), CK::score));
                b.after.balances.front().units += *other_score_delta;
            }
            std::vector<core::RequestedEffect> requests;
            for (const auto &effect : b.effects)
                requests.push_back(
                    request(effect.unit, effect.requested_units,
                            effect.unit == Unit::milliseconds ? "main" : "other-score"));
            b.transitions = std::move(transitions);
            b.after = file.encoded(std::move(b.after), prior);
            File::retain_plan(s, b.intent.plan, File::plain_capture(receipt, {}, requests));
            require(SqliteApplicationBundleStore(s).stage(b).disposition ==
                        SqliteBundleStageDisposition::staged,
                    "Independent owner state change is committed through the actual bundle");
        });
    }

    void main(std::string ref, Number value, Number grace = 9) {
        move(ref, {Life::Active, Phase::Main, value, grace, false});
    }
};

inline FinancialEntitlement quote(Number previous_tenths, Number accepted_tenths,
                                  Number original_tenths, bool full,
                                  std::optional<GroupedRefundEntitlement> time,
                                  std::vector<ApplicationReversalLink> components) {
    return {fraction(previous_tenths, 10),
            fraction(accepted_tenths, 10),
            fraction(accepted_tenths, original_tenths),
            full,
            std::move(time),
            std::move(components)};
}

inline GroupedRefundEntitlement time(Number position, RefundTimeAllocation prior,
                                     RefundTimeAllocation next, core::Exact exact, Number main,
                                     Number restore) {
    return {{original, 1, CK::main_delta},        1,    position, prior, next, exact,
            exact - next.main - next.restoration, main, restore};
}

inline ApplicationReversalLink link(CK kind, Number ask, Number source, Number position,
                                    Number prior, Number cumulative, core::Exact f,
                                    core::Exact exact, bool full) {
    return {ask,   {original, source, kind}, position, prior, cumulative, prior - cumulative, f,
            exact, exact - cumulative,       full};
}

inline FinancialEntitlement full_quote(Number total, Number main, Number restore, Number score,
                                       Number count) {
    const auto has_time = main || restore;
    std::vector<ApplicationReversalLink> components;
    Number ordinal = has_time ? 2 : 1;
    if (score)
        components.push_back(
            link(CK::score, ordinal++, has_time ? 2 : 1, 0, 0, score, 1, score, true));
    if (count)
        components.push_back(link(CK::whole_count, ordinal, (has_time ? 2 : 1) + (score ? 1 : 0), 0,
                                  0, count, 1, count, true));
    return quote(0, total * 10, total * 10, true,
                 has_time
                     ? std::optional{time(0, {}, {main, restore}, main + restore, -main, restore)}
                     : std::nullopt,
                 std::move(components));
}

inline FinancialRefundRecord accepted(const FinancialWorkReply &reply, D disposition = D::applied) {
    require(reply.disposition == disposition &&
                std::holds_alternative<FinancialRefundRecord>(reply.record),
            "Exact financial acceptance disposition and typed record");
    return std::get<FinancialRefundRecord>(reply.record);
}

inline FinancialTimerResolution resolved(const FinancialWorkReply &reply,
                                         const FinancialTimerCommand &c,
                                         RefundTimerOutcome expected, D d = D::resolved) {
    require(reply.disposition == d &&
                std::holds_alternative<FinancialTimerResolution>(reply.record),
            "Exact continuation disposition and typed resolution");
    const auto &r = std::get<FinancialTimerResolution>(reply.record);
    require(r.timer == expected && same(r.command.operation, c.operation) &&
                same(r.held.locator, c.origin) && r.command.override_policy == c.override_policy &&
                r.closed.state == ApplicationHoldState::resolved &&
                r.closed.parent_version_ref == c.origin.version_ref && r.closed.position == 2 &&
                r.audit.actor_ref == c.audit.actor_ref &&
                r.audit.decision_ref == c.audit.decision_ref,
            "Only the exact retained request closes with the typed physical outcome and audit");
    return r;
}

inline void refused(const FinancialWorkReply &reply, D expected) {
    require(reply.disposition == expected && std::holds_alternative<std::monostate>(reply.record),
            "Exact typed refusal without a disclosed record");
}

inline void unchanged(File &f, const FinancialRefundCommand &c, const std::string &head) {
    f.check_raw(c.receipt);
    require(f.head().token == head, "Refusal advances neither the timer nor the state head");
    f.write([&](const auto &s) {
        require(!SqliteFinancialRefundStore(s).lookup(c.operation) &&
                    !SqliteApplicationPlanStore(s).find_revision(c.financial_plan.plan_ref,
                                                                 c.financial_plan.revision),
                "The attempted operation, money progress and capture were never claimed");
    });
}

inline FinancialRefundRecord by_ref(File &f, const ApplicationOperationKey &key) {
    std::optional<FinancialRefundRecord> selected;
    f.write([&](const auto &s) { selected = SqliteFinancialRefundStore(s).lookup(key); });
    require(selected.has_value(), "Fresh committed typed refund by exact operation selector");
    return *selected;
}

inline FinancialTimerResolution by_resolution(File &f, const ApplicationOperationKey &key) {
    std::optional<FinancialTimerResolution> selected;
    f.write(
        [&](const auto &s) { selected = SqliteFinancialRefundStore(s).lookup_resolution(key); });
    require(selected.has_value(), "Fresh committed typed resolution by exact operation selector");
    return *selected;
}

inline void clock_and_state(File &f, const FinancialRefundRecord &r, core::TimerState before,
                            core::TimerState advanced, core::TimerState after,
                            RefundTimerOutcome outcome, Number debt,
                            const std::vector<Transition> &transitions = {}) {
    require(r.timer == outcome && same_timer(r.application.persisted_head.timer, before) &&
                same_timer(r.application.advanced_before.timer, advanced) &&
                same_timer(r.application.after.timer, after) &&
                r.application.after.time_debt_ms == debt &&
                r.application.transitions == transitions &&
                f.head().token == r.application.after.token,
            "Exact before/observed/after lifecycle, debt, timer legs and ordered transitions");
    const auto &app = r.application;
    const auto persisted = decode_reference_state(app.after);
    require(persisted.observation.checked_head_token == app.persisted_head.token,
            "SAS1 after frame is coherent and bound to the observed head");
    if (app.advanced_before.token == app.persisted_head.token)
        require(app.advanced_before.envelope.document == app.persisted_head.envelope.document,
                "A reused observation token retains the exact immutable SAS1 frame");
    const auto selected = by_ref(f, r.command.operation);
    require(selected.timer == r.timer && selected.application.transitions == transitions &&
                selected.application.after.envelope.document == app.after.envelope.document,
            "Selected committed journal reproduces the exact lifecycle and owner envelope");
}

inline std::vector<ApplicationHoldVersionLocator>
locators(std::initializer_list<FinancialTimerHold> holds) {
    std::vector<ApplicationHoldVersionLocator> result;
    for (const auto &h : holds)
        result.push_back(h.locator);
    return result;
}

inline FinancialTimerHold require_hold(const FinancialRefundRecord &r, HoldReason why,
                                       Number position, Number main, Number restore) {
    require(r.hold && r.hold->reason == why && r.hold->payment_position == position &&
                r.hold->time.main_request == main && r.hold->time.restoration_request == restore,
            "Durable hold belongs to the exact whole grouped request and payment position");
    return *r.hold;
}

inline void active(File &f, std::initializer_list<FinancialTimerHold> expected) {
    const auto selected = f.financial_active();
    require(selected.size() == expected.size() &&
                std::ranges::all_of(expected,
                                    [&](const auto &h) {
                                        return std::ranges::any_of(selected, [&](const auto &a) {
                                            return same(a.locator, h.locator) &&
                                                   a.payment_position == h.payment_position &&
                                                   a.payment == f.payment && a.subject == f.cid &&
                                                   a.reason == h.reason;
                                        });
                                    }),
            "Complete selected financial active set with exact payment positions and versions");
}

} // namespace live_financial_refund_fixture
