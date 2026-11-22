// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include "../rules_004/fixture.hpp"

namespace {
using namespace rules_005;

struct Milestones {
    ScenarioInputs in = inputs(1000);
    RootBinding source = root("score-root");
    MilestoneOccurrence o1{"score-progression", "O1", 0};
    MilestoneOccurrence o2{"score-progression", "O2", 1};
    EventBinding first = child(source, source.event, "milestone-one", o1);
    EventBinding second = child(source, source.event, "milestone-two", o2);

    Milestones() {
        in.initial.milestone_track =
            MilestoneProgressionInput{"T", "V", "J", 0, {{"M1", 10}, {"M2", 20}}, {}};
        const auto config =
            validate_rule_configuration({"milestones",
                                         {4, 8},
                                         {named("source", "score", {score("reward", "25")}),
                                          rule("milestone", Kind::milestone_completion, {})}});
        add_root(in, source);
        in.captures.push_back(retain(source.event, config, Kind::named_event, "named_event",
                                     {RuleText{RuleTextField::event_name, "score"}}));
        in.captures.push_back(
            retain(first, config, Kind::milestone_completion, "milestone_completion",
                   {RuleText{RuleTextField::milestone_ref, "M1"}}, {}, {}, "modifier-empty", 2));
        in.captures.push_back(
            retain(second, config, Kind::milestone_completion, "milestone_completion",
                   {RuleText{RuleTextField::milestone_ref, "M2"}}, {}, {}, "modifier-empty", 3));
        in.milestone_captures = {{o1, {}, {}, first}, {o2, {}, {}, second}};
        turn(in, "commit-score", event(source.event));
    }
};

void timeline(ScenarioInputs &in, std::string id, std::chrono::milliseconds elapsed,
              std::optional<GroupObservation> head = {},
              TimelineOperation operation = TimelineOperation::sample) {
    turn(in, std::move(id), TimelineTurn{operation, std::move(head)}, {elapsed.count(), elapsed});
}

GroupObservation head(std::string ref, MilestoneHeadDisposition disposition) {
    return {{std::move(ref)}, disposition, RuleCompletion::committed};
}

const std::vector<MilestoneCompletion> achievements{{"M1", "V", 10, 25, "J"},
                                                    {"M2", "V", 20, 25, "J"}};

void expect_completion_prefix(const std::vector<MilestoneCompletion> &prefix) {
    ASSERT_EQ(prefix.size(), achievements.size());
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        EXPECT_EQ(prefix[i].milestone_ref, achievements[i].milestone_ref);
        EXPECT_EQ(prefix[i].definition_revision_ref, "V");
        EXPECT_EQ(prefix[i].target_micro_points, achievements[i].target_micro_points);
        EXPECT_EQ(prefix[i].score_micro_points, 25);
        EXPECT_EQ(prefix[i].completion_context_ref, "J");
    }
}

void expect_achievements(const TurnTrace &source) {
    ASSERT_TRUE(source.progression);
    EXPECT_EQ(source.state.score_micro_points, 25);
    EXPECT_EQ(source.progression->track_ref, "T");
    EXPECT_EQ(source.progression->newly_completed_begin, 0U);
    EXPECT_EQ(source.progression->newly_completed_count, 2U);
    expect_completion_prefix(source.progression->completed_prefix);
    EXPECT_TRUE(source.releases.empty());
    ASSERT_TRUE(source.publication);
    EXPECT_TRUE(source.publication->children.empty());
}

void expect_release(const TurnTrace &trace, const EventBinding &binding, std::uint32_t count,
                    const ActiveMilestonePresentation &presentation) {
    ASSERT_TRUE(trace.publication);
    const auto occurrence = std::get<MilestoneOccurrence>(*binding.cause);
    ASSERT_EQ(trace.releases.size(), 1U);
    EXPECT_EQ(trace.releases.front().occurrence.ref, occurrence.occurrence);
    EXPECT_EQ(trace.releases.front().completion, RuleCompletion::committed);
    EXPECT_EQ(trace.presentation, presentation);
    EXPECT_EQ(trace.committed_children, count);
    const auto &publication = *trace.publication;
    EXPECT_EQ(publication.identity.purpose, UnitPurpose::released_occurrences);
    EXPECT_EQ(publication.identity.anchor, binding.parent);
    EXPECT_FALSE(publication.pass);
    EXPECT_FALSE(publication.cutoff);
    EXPECT_EQ(publication.children, (std::vector{AdmittedEvent{binding, 1, count}}));
    ASSERT_EQ(publication.occurrences.size(), 1U);
    const auto &record = publication.occurrences.front();
    EXPECT_EQ(record.occurrence, occurrence);
    EXPECT_EQ(record.parent, binding.parent);
    EXPECT_EQ(std::get<AdmittedEvent>(record.disposition), (AdmittedEvent{binding, 1, count}));
}

TEST(Rules005, S5AchievementsPrecedeExactMilestoneReleases) {
    for (const bool release_second : {true, false}) {
        SCOPED_TRACE(release_second);
        Milestones model;
        timeline(model.in, "release-first", 0ms, head("O1", MilestoneHeadDisposition::ready));
        timeline(model.in, "first-ends", 10'000ms);
        timeline(model.in, "gap-ends-held", 11'000ms, head("O2", MilestoneHeadDisposition::held));
        timeline(model.in, "check-second", 12'000ms,
                 head("O2", release_second ? MilestoneHeadDisposition::ready
                                           : MilestoneHeadDisposition::held));
        if (release_second) {
            turn(model.in, "run-first-child", event(model.first), {12'000, 12'000ms});
            turn(model.in, "run-second-child", event(model.second), {12'000, 12'000ms});
        }
        const auto scenario =
            replay(release_second ? "release-two" : "hold-second", std::move(model.in));
        const auto result = inspect_simulation(scenario, limits);
        expect_complete(result, scenario);
        ASSERT_EQ(result.trace.size(), release_second ? 7U : 5U);
        expect_achievements(result.trace[0]);
        expect_release(result.trace[1], model.first, 1, {"M1", 0ms, 10'000ms});
        EXPECT_FALSE(result.trace[2].presentation);
        EXPECT_FALSE(result.trace[2].publication);
        EXPECT_TRUE(result.trace[2].releases.empty());
        EXPECT_FALSE(result.trace[3].presentation);
        EXPECT_FALSE(result.trace[3].publication);
        EXPECT_TRUE(result.trace[3].releases.empty());
        ASSERT_TRUE(result.trace[3].group_head);
        EXPECT_EQ(result.trace[3].group_head->disposition, MilestoneHeadDisposition::held);
        if (release_second) {
            expect_release(result.trace[4], model.second, 2, {"M2", 0ms, 10'000ms});
            for (std::size_t i : {5U, 6U}) {
                const auto &child_trace = result.trace[i];
                ASSERT_TRUE(child_trace.selection);
                EXPECT_EQ(child_trace.selection->selection.selected_rule_indices,
                          (std::vector<std::size_t>{1}));
                EXPECT_TRUE(child_trace.actions.empty());
            }
            EXPECT_EQ(result.trace[5].selection->event.event_ref, model.first.event);
            EXPECT_EQ(result.trace[6].selection->event.event_ref, model.second.event);
        } else {
            EXPECT_FALSE(result.trace[4].publication);
            EXPECT_FALSE(result.trace[4].presentation);
            EXPECT_TRUE(result.trace[4].releases.empty());
            EXPECT_EQ(result.trace[4].committed_children, 1U);
        }
        EXPECT_EQ(result.final_state.score_micro_points, 25);
        ASSERT_TRUE(result.final_state.milestone_track);
        expect_completion_prefix(result.final_state.milestone_track->completed_prefix);
    }
}

TEST(Rules005, S5UnknownMilestoneReleaseHoldsLifetime) {
    Milestones model;
    const GroupObservation thrown{
        {"O1"}, MilestoneHeadDisposition::ready, RuleCompletion::outcome_unknown};
    timeline(model.in, "attempt-first", 0ms, thrown);
    timeline(model.in, "later-same-timeline", 12'000ms,
             head("O1", MilestoneHeadDisposition::ready));
    turn(model.in, "unresolved-readback",
         GroupReadback{"attempt-first", {"O1"}, RuleCompletion::outcome_unknown, {}},
         {12'000, 12'000ms});
    timeline(model.in, "still-latched", 12'001ms, head("O1", MilestoneHeadDisposition::ready));
    const std::vector<CommittedMilestoneGroup> suffix{
        {achievements[0], {}, {}, MilestoneOccurrenceKey{"O1"}},
        {achievements[1], {}, {}, MilestoneOccurrenceKey{"O2"}}};
    turn(model.in, "rollback-readback",
         GroupReadback{"attempt-first", {"O1"}, RuleCompletion::rolled_back, suffix},
         {12'001, 12'001ms});
    timeline(model.in, "first-fresh-start", 12'002ms, head("O1", MilestoneHeadDisposition::ready));
    const auto scenario = replay("release-uncertain", std::move(model.in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 7U);
    expect_achievements(result.trace[0]);
    ASSERT_EQ(result.trace[1].releases.size(), 1U);
    EXPECT_EQ(result.trace[1].releases.front().occurrence.ref, "O1");
    EXPECT_EQ(result.trace[1].releases.front().completion, RuleCompletion::outcome_unknown);
    EXPECT_FALSE(result.trace[1].presentation);
    EXPECT_FALSE(result.trace[1].publication);
    for (std::size_t i : {2U, 3U, 4U, 5U}) {
        EXPECT_TRUE(result.trace[i].releases.empty());
        EXPECT_FALSE(result.trace[i].presentation);
        EXPECT_FALSE(result.trace[i].publication);
    }
    expect_release(result.trace[6], model.first, 1, {"M1", 0ms, 10'000ms});
    EXPECT_EQ(result.final_state.score_micro_points, 25);
}

struct Cascade {
    ScenarioInputs in = inputs(1000);
    RootBinding r = root();
    EventBinding a = child(r, r.event, "A", NamedAction{key("R-rule", "emit-A")});
    EventBinding b = child(r, a, "B", NamedAction{key("A-rule", "emit-B")});
    EventBinding c = child(r, b, "C", NamedAction{key("B-rule", "emit-C", 1)});

    Cascade() {
        const auto config = validate_rule_configuration(
            {"cascade",
             {2, 2},
             {named("R-rule", "r", {emit("emit-A", "a")}),
              named("A-rule", "a", {emit("emit-B", "b")}),
              named("B-rule", "b", {score("B-score", "5"), emit("emit-C", "c")})}});
        add_root(in, r);
        retain_named(r.event, config, "r", 1);
        retain_named(a, config, "a", 2);
        retain_named(b, config, "b", 3);
        retain_named(c, config, "c", 4);
        auto first = event(r.event);
        first.named = {a};
        turn(in, "pass-R", std::move(first));
    }

    void retain_named(const EventBinding &binding, const RuleConfiguration &config,
                      std::string name, std::uint64_t receipt) {
        in.captures.push_back(retain(binding, config, Kind::named_event, "named_event",
                                     {RuleText{RuleTextField::event_name, std::move(name)}}, {}, {},
                                     "modifier-empty", receipt));
    }

    ModeledHistory committed_r() const {
        // The restart's starting publication comes from the shipped #210 producer.
        // Its privately selected inputs are the same as the first S6 unit.
        rules_004::Owner owner;
        const auto &configuration = in.captures.front().configuration.data();
        owner.define(configuration.configuration_ref, configuration.generation,
                     configuration.rules);
        owner.active = configuration.configuration_ref;
        owner.keep(r.event, "r");
        owner.keep(a, "a");
        auto opened = std::get<OpenedChain>(owner.open(r, limits.chain));
        const auto publication = owner.run(opened.chain, r.event, "pass-R", {{"emit-A", {a, "a"}}});
        return {r, "admit-R", limits.chain, {{r.event, 0, 0}, {a, 1, 1}}, {publication}, {}, {}};
    }
};

TEST(Rules005, S6CutoffAndExactRepeat) {
    Cascade model;
    auto second = event(model.a);
    second.named = {model.b};
    turn(model.in, "pass-A", std::move(second));
    turn(model.in, "repeat-A", event(model.a));
    auto third = event(model.b);
    third.named = {model.c};
    turn(model.in, "pass-B", std::move(third));
    const auto z = root("Z");
    const auto d = child(z, z.event, "D", NamedAction{key("Z-rule", "emit-D")});
    const auto other = validate_rule_configuration(
        {"independent", {}, {named("Z-rule", "z", {emit("emit-D", "d")})}});
    add_root(model.in, z);
    model.retain_named(z.event, other, "z", 5);
    model.retain_named(d, other, "d", 6);
    auto independent = event(z.event);
    independent.named = {d};
    turn(model.in, "pass-Z", std::move(independent));
    const auto scenario = replay("S6", std::move(model.in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 5U);
    const auto &r = result.trace[0];
    EXPECT_EQ(r.admitted, (AdmittedEvent{model.r.event, 0, 0}));
    ASSERT_TRUE(r.publication);
    EXPECT_EQ(r.publication->children, (std::vector{AdmittedEvent{model.a, 1, 1}}));
    EXPECT_FALSE(r.publication->cutoff);
    const auto &a = result.trace[1];
    EXPECT_EQ(a.admitted, (AdmittedEvent{model.a, 1, 1}));
    ASSERT_TRUE(a.publication);
    EXPECT_EQ(a.publication->children, (std::vector{AdmittedEvent{model.b, 2, 2}}));
    const Cutoff expected{
        model.r.event.chain, model.r.event.capture, {model.b, 2, 2}, {2, 2}, CutoffReason::both};
    ASSERT_TRUE(a.publication->cutoff);
    expect_cutoff(*a.publication->cutoff, expected);
    ASSERT_TRUE(a.cutoff_audit_ref);
    ASSERT_TRUE(a.cutoff_notice_ref);
    EXPECT_FALSE(a.cutoff_audit_ref->empty());
    EXPECT_FALSE(a.cutoff_notice_ref->empty());
    for (std::size_t i : {0U, 2U, 3U, 4U}) {
        EXPECT_FALSE(result.trace[i].cutoff_audit_ref);
        EXPECT_FALSE(result.trace[i].cutoff_notice_ref);
    }
    const auto &duplicate = result.trace[2];
    EXPECT_EQ(duplicate.begin_disposition, BeginDisposition::duplicate_event);
    EXPECT_EQ(duplicate.committed_children, 2U);
    EXPECT_FALSE(duplicate.selection);
    EXPECT_FALSE(duplicate.publication);
    EXPECT_TRUE(duplicate.actions.empty());
    const auto &b = result.trace[3];
    EXPECT_EQ(b.admitted, (AdmittedEvent{model.b, 2, 2}));
    ASSERT_TRUE(b.publication);
    EXPECT_FALSE(b.publication->cutoff);
    EXPECT_TRUE(b.publication->children.empty());
    ASSERT_EQ(b.publication->named_suppressions.size(), 1U);
    const auto &suppressed = b.publication->named_suppressions.front();
    EXPECT_EQ(suppressed.action, key("B-rule", "emit-C", 1));
    expect_cutoff(std::get<Cutoff>(suppressed.stop), expected);
    EXPECT_EQ(b.state.score_micro_points, 5);
    const auto &separate = result.trace[4];
    ASSERT_TRUE(separate.publication);
    EXPECT_EQ(separate.publication->children, (std::vector{AdmittedEvent{d, 1, 1}}));
    EXPECT_FALSE(separate.publication->cutoff);
    EXPECT_EQ(separate.committed_children, 1U);
    EXPECT_EQ(result.final_state.score_micro_points, 5);

    const auto replayed = inspect_simulation(scenario, limits);
    ASSERT_EQ(replayed.trace.size(), 5U);
    EXPECT_EQ(replayed.trace[1].cutoff_audit_ref, a.cutoff_audit_ref);
    EXPECT_EQ(replayed.trace[1].cutoff_notice_ref, a.cutoff_notice_ref);
    EXPECT_EQ(replayed.final_state.score_micro_points, 5);
}

TEST(Rules005, S13ManualPausePersistsThroughGraceRescue) {
    const auto config = validate_rule_configuration(
        {"grace", {}, {rule("quiet", Kind::subscription, {time("add-time", "20")})}});
    const auto r = root("grace-credit");
    for (const int cap : {100, 0}) {
        SCOPED_TRACE(cap);
        auto in = inputs(100, cap);
        add_root(in, r);
        auto accepted = retain(r.event, config, Kind::subscription, "subscription", {});
        accepted.eligibility.conditions.review = ContributionReview::accepted;
        in.captures.push_back(std::move(accepted));
        turn(in, "main-expires", TimeCut{}, {100, 100ms});
        turn(in, "set-manual", PauseControl{PauseReason::Manual, true}, {100, 100ms});
        turn(in, "grace-paused", TimeCut{}, {600, 600ms});
        turn(in, "quiet-credit", event(r.event), {600, 600ms});
        turn(in, "still-paused", TimeCut{}, {700, 700ms});
        const auto scenario = replay(cap ? "rescue" : "zero-cap", std::move(in));

        const auto result = inspect_simulation(scenario, limits);
        expect_complete(result, scenario);
        ASSERT_EQ(result.trace.size(), 5U);
        ASSERT_TRUE(result.trace[0].timer_cut);
        EXPECT_EQ(result.trace[0].timer_cut->progressed_ms, 100);
        EXPECT_EQ(result.trace[0].timer_cut->transitions,
                  (std::vector{TimerTransition::MainExpired, TimerTransition::GraceStarted}));
        for (std::size_t i : {2U, 4U}) {
            const auto &paused = result.trace[i];
            ASSERT_TRUE(paused.timer_cut);
            EXPECT_EQ(paused.timer_cut->progressed_ms, 0);
            EXPECT_TRUE(paused.timer_cut->transitions.empty());
            EXPECT_EQ(paused.state.pause.reasons, PauseReasons{PauseReason::Manual});
            EXPECT_EQ(paused.state.timer.grace_remaining_ms, 120'000);
        }
        const auto &credit = result.trace[3];
        EXPECT_EQ(credit.state.pause.reasons, PauseReasons{PauseReason::Manual});
        ASSERT_EQ(credit.actions.size(), 1U);
        expect_actual_time(credit.actions.front(), key("quiet", "add-time"),
                           {20, cap ? 20 : 0, 0, cap ? 0 : 20, cap ? 20 : 0});
        EXPECT_FALSE(credit.delivery);
        EXPECT_EQ(result.final_state.timer.lifecycle, TimerLifecycle::Active);
        EXPECT_EQ(result.final_state.timer.phase,
                  cap ? CountdownPhase::Main : CountdownPhase::Grace);
        EXPECT_EQ(result.final_state.timer.main_remaining_ms, cap ? 20 : 0);
        EXPECT_EQ(result.final_state.timer.grace_remaining_ms, 120'000);
        EXPECT_EQ(result.final_state.pause.reasons, PauseReasons{PauseReason::Manual});
    }
}

TEST(Rules005, S14QueuePauseGatesStartsIndependentlyOfCountdownPause) {
    Milestones model;
    timeline(model.in, "release-first", 0ms, head("O1", MilestoneHeadDisposition::ready));
    timeline(model.in, "countdown-pause", 1000ms, {}, TimelineOperation::countdown_pause);
    timeline(model.in, "queue-pause", 3000ms, {}, TimelineOperation::queue_pause);
    timeline(model.in, "before-window-end", 9999ms);
    timeline(model.in, "window-end", 10'000ms);
    timeline(model.in, "gap-end-paused", 11'000ms, head("O2", MilestoneHeadDisposition::ready));
    timeline(model.in, "queue-resume", 12'000ms, head("O2", MilestoneHeadDisposition::ready),
             TimelineOperation::queue_resume);
    const auto scenario = replay("S14", std::move(model.in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 8U);
    expect_achievements(result.trace[0]);
    expect_release(result.trace[1], model.first, 1, {"M1", 0ms, 10'000ms});
    EXPECT_EQ(result.trace[4].presentation, (ActiveMilestonePresentation{"M1", 9999ms, 10'000ms}));
    for (std::size_t i : {5U, 6U}) {
        EXPECT_FALSE(result.trace[i].presentation);
        EXPECT_FALSE(result.trace[i].publication);
        EXPECT_TRUE(result.trace[i].releases.empty());
    }
    for (std::size_t i = 2; i < 8; ++i)
        EXPECT_TRUE(result.trace[i].countdown_paused);
    EXPECT_TRUE(result.trace[6].queue_paused);
    EXPECT_FALSE(result.trace[7].queue_paused);
    expect_release(result.trace[7], model.second, 2, {"M2", 0ms, 10'000ms});
}

TEST(Rules005, S15ConnectorReportsAndClientSignalsAreIndependent) {
    auto in = inputs(1000);
    in.initial.pause.reasons = {PauseReason::Manual};
    in.initial.pause_config.client_disconnect_delay_ms = 100;
    const auto report = [&](std::string id, std::int64_t now, ConnectorConnection connection,
                            ConnectorGap gap) {
        turn(in, std::move(id),
             ConnectorReport{
                 {}, ConnectorInstallation::validated, ConnectorConsent::granted, connection, gap},
             {now, std::chrono::milliseconds{now}});
    };
    const auto cut = [&](std::string id, std::int64_t now, std::int64_t elapsed, bool client) {
        turn(in, std::move(id), TimeCut{PauseObservation{elapsed, StreamStatus::Online, client}},
             {now, std::chrono::milliseconds{now}});
    };
    report("connected", 0, ConnectorConnection::connected, ConnectorGap::none);
    report("disconnected", 0, ConnectorConnection::disconnected, ConnectorGap::open);
    cut("client-delay", 50, 50, false);
    cut("client-threshold", 100, 50, false);
    report("connected-gap-open", 100, ConnectorConnection::connected, ConnectorGap::open);
    cut("client-reconnect", 100, 0, true);
    cut("manual-still-held", 150, 50, true);
    report("gap-recovered", 150, ConnectorConnection::connected, ConnectorGap::recovered);
    turn(in, "manual-clear", PauseControl{PauseReason::Manual, false}, {150, 150ms});
    cut("resumed-cut", 200, 50, true);
    const auto scenario = replay("S15", std::move(in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 10U);
    struct ExpectedStatus {
        std::size_t index;
        ConnectorHealth health;
        ConnectorGap gap;
    };
    for (const auto &expected :
         {ExpectedStatus{0, ConnectorHealth::connected, ConnectorGap::none},
          ExpectedStatus{1, ConnectorHealth::disconnected, ConnectorGap::open},
          ExpectedStatus{4, ConnectorHealth::connected, ConnectorGap::open},
          ExpectedStatus{7, ConnectorHealth::connected, ConnectorGap::recovered}}) {
        const auto &trace = result.trace[expected.index];
        ASSERT_TRUE(trace.connector);
        EXPECT_EQ(trace.connector->health, expected.health);
        EXPECT_EQ(trace.connector->gap, expected.gap);
        EXPECT_EQ(trace.connector->setup.installation, ConnectorInstallation::validated);
        EXPECT_EQ(trace.connector->setup.account_consent, ConnectorConsent::granted);
        EXPECT_EQ(trace.connector->setup.application_registration, ConnectorSupport::unknown);
        EXPECT_EQ(trace.connector->setup.api_key_setup, ConnectorSupport::unknown);
        EXPECT_EQ(trace.connector->setup.webhook_setup, ConnectorSupport::unknown);
        EXPECT_EQ(trace.connector->setup.browser_link, ConnectorLink::unknown);
        EXPECT_FALSE(trace.connector->setup.callback_url);
        EXPECT_FALSE(trace.admitted);
        EXPECT_FALSE(trace.publication);
        EXPECT_FALSE(trace.selection);
    }
    const PauseReasons manual{PauseReason::Manual};
    EXPECT_EQ(result.trace[2].state.pause.reasons, manual);
    EXPECT_EQ(result.trace[2].state.pause.client_disconnect_pending_ms, 50);
    EXPECT_EQ(result.trace[3].state.pause.reasons,
              (PauseReasons{PauseReason::Manual, PauseReason::DesignatedClientDisconnected}));
    EXPECT_FALSE(result.trace[3].state.pause.client_disconnect_pending_ms);
    EXPECT_EQ(result.trace[4].state.pause.reasons, result.trace[3].state.pause.reasons);
    EXPECT_EQ(result.trace[5].state.pause.reasons, manual);
    EXPECT_FALSE(result.trace[5].state.pause.client_disconnect_pending_ms);
    EXPECT_EQ(result.trace[6].state.pause.reasons, manual);
    for (std::size_t i = 0; i < 9; ++i)
        EXPECT_EQ(result.trace[i].state.timer.main_remaining_ms, 1000);
    EXPECT_TRUE(result.trace[8].state.pause.reasons.empty());
    ASSERT_TRUE(result.trace[9].timer_cut);
    EXPECT_EQ(result.trace[9].timer_cut->progressed_ms, 50);
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 950);
}

TEST(Rules005, S16RestartQualifiesTimerAndChainEvidenceSeparately) {
    enum class Evidence { exact, missing_gap, missing_chain };
    for (const auto choice : {Evidence::exact, Evidence::missing_gap, Evidence::missing_chain}) {
        SCOPED_TRACE(static_cast<int>(choice));
        Cascade model;
        const auto history = model.committed_r();
        model.in.turns.clear();
        model.in.roots.clear();
        const RecoveryContinuousGap continuous{"gap-evidence", {10'000}, {10'250}};
        RestartTurn restart{
            {"timer-lineage", "checkpoint", "timer-config", "restart-facts"},
            DowntimePolicy::CountElapsed,
            {"last-checkpoint", RecoveryUtcInstant{10'000}},
            {"restart-observation", RecoveryUtcInstant{10'250}},
            choice == Evidence::missing_gap ? RecoveryGapClock{RecoveryGapUnavailable::Unknown}
                                            : RecoveryGapClock{continuous},
            EmptyRecoverySchedule{"empty-schedule", "delivery-revision"},
            {model.r, choice == Evidence::missing_chain ? OwnerSelection{Unavailable{}}
                                                        : OwnerSelection{history}}};
        turn(model.in, "modeled-restart", std::move(restart), {10'250, 0ms});
        auto repeat = event(model.r.event);
        repeat.named = {model.a};
        turn(model.in, "repeat-retained-R", std::move(repeat), {10'250, 0ms});
        const auto scenario = replay("S16", std::move(model.in));
        const auto result = inspect_simulation(scenario, limits);

        ASSERT_FALSE(result.trace.empty());
        const auto &opened = result.trace.front();
        ASSERT_TRUE(opened.recovery);
        EXPECT_EQ(opened.recovery->policy, DowntimePolicy::CountElapsed);
        if (choice == Evidence::missing_gap) {
            EXPECT_FALSE(opened.recovery->count_elapsed);
            EXPECT_EQ(opened.recovery->gap_unavailable, RecoveryGapUnavailable::Unknown);
            ASSERT_TRUE(opened.recovery->review_hold);
            EXPECT_EQ(result.final_state.timer.main_remaining_ms, 1000);
            EXPECT_EQ(result.final_state.pause.reasons, PauseReasons{PauseReason::RecoveryReview});
        } else {
            ASSERT_TRUE(opened.recovery->count_elapsed);
            const auto &projected = *opened.recovery->count_elapsed;
            ASSERT_EQ(projected.progression.size(), 1U);
            EXPECT_EQ(projected.progression.front().from, RecoveryUtcInstant{10'000});
            EXPECT_EQ(projected.progression.front().elapsed_ms, 250);
            EXPECT_TRUE(projected.transitions.empty());
            EXPECT_EQ(result.final_state.timer.main_remaining_ms, 750);
        }
        EXPECT_FALSE(opened.publication);
        EXPECT_FALSE(opened.cutoff_notice_ref);
        EXPECT_FALSE(opened.retained_cutoff);
        if (choice == Evidence::missing_chain) {
            EXPECT_EQ(opened.open_refusal, OpenRefusal::history_unavailable);
            EXPECT_FALSE(opened.committed_children);
        } else {
            EXPECT_FALSE(opened.open_refusal);
            EXPECT_EQ(opened.committed_children, 1U);
        }
        for (const auto &trace : result.trace) {
            EXPECT_TRUE(trace.actions.empty());
            EXPECT_FALSE(trace.publication);
            EXPECT_FALSE(trace.delivery);
        }
        if (choice == Evidence::exact) {
            expect_complete(result, scenario);
            ASSERT_EQ(result.trace.size(), 2U);
            EXPECT_EQ(result.trace[1].begin_disposition, BeginDisposition::duplicate_event);
            EXPECT_EQ(result.trace[1].committed_children, 1U);
            EXPECT_FALSE(result.trace[1].selection);
        }
    }
}
} // namespace
