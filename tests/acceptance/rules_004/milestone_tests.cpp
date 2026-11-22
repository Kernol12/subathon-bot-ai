// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include <subathon/core/milestone_progression.hpp>
#include <subathon/server/milestone_timeline.hpp>
#include <tuple>

namespace {
using namespace rules_004;
using namespace std::chrono_literals;
using namespace subathon::server;

enum class GroupCase { released, held, skipped };

// The protected timeline selects the next group. The modeled application
// acknowledges release separately and binds the exact occurrence to its source.
class ModelRelease final : public MilestoneGroupRelease {
  public:
    std::map<std::string, MilestoneHeadDisposition> dispositions;
    std::vector<CommittedMilestoneGroup> scheduled;
    std::vector<std::string> acknowledged;

    bool review_held(const MilestoneCompletion &) const override {
        ADD_FAILURE() << "keyed groups must use the occurrence decision";
        return true;
    }

    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &key) const override {
        return dispositions.at(key.ref);
    }

    void release(CommittedMilestoneGroup group) override { scheduled.push_back(std::move(group)); }

    void acknowledge(std::string occurrence) {
        ASSERT_LT(acknowledged.size(), scheduled.size());
        ASSERT_EQ(scheduled[acknowledged.size()].occurrence, MilestoneOccurrenceKey{occurrence});
        acknowledged.push_back(std::move(occurrence));
    }
};

TEST(Rules004, G5CommittedMilestoneReleasesInterleaveInOwnerTurnOrder) {
    for (const auto control : {GroupCase::released, GroupCase::held, GroupCase::skipped}) {
        SCOPED_TRACE(static_cast<int>(control));
        Owner owner;
        auto milestone = named_rule("M", "unused", {delta("plus1", "1")});
        milestone.triggers = {{RuleTriggerKind::milestone_completion,
                               {RulePredicate{RuleComparison<RuleText, RuleEquality>{
                                   {RuleTextField::milestone_ref, "M1"}, RuleEquality::equal}}}},
                              {RuleTriggerKind::milestone_completion,
                               {RulePredicate{RuleComparison<RuleText, RuleEquality>{
                                   {RuleTextField::milestone_ref, "M2"}, RuleEquality::equal}}}}};
        owner.define(
            "CM", {4, 8},
            {named_rule("Root", "seed",
                        {delta("S", "25", RuleDeltaUnit::micro_points), emit("emit-d", "d")}),
             named_rule("D", "d", {emit("emit-n", "n")}), milestone});
        owner.active = "CM";
        const auto r = root();
        const auto d = child(r.event, "D", NamedAction{key("Root", "emit-d", 1)});
        const auto n = child(d, "N", NamedAction{key("D", "emit-n")});
        owner.keep(r.event, "seed");
        owner.keep(d, "d");
        owner.keep(n, "n");
        auto opened = std::get<OpenedChain>(owner.open(r));
        EXPECT_EQ(std::get<BeginDisposition>(
                      begin_occurrences(opened.chain, "unadmitted", "release-unadmitted")),
                  BeginDisposition::not_admitted);
        auto unit = std::get<Unit>(begin_event(opened.chain, r.event, "pass-R"));
        EXPECT_EQ(
            std::get<BeginDisposition>(begin_occurrences(opened.chain, "R", "release-in-flight")),
            BeginDisposition::in_flight_hold);
        const auto plan = plan_event(unit, owner.mapper());
        const auto staged =
            std::get<RuleStagedPass>(owner.consume(unit, r.event, plan, {{"emit-d", {d, "d"}}}));
        ASSERT_EQ(staged.rules.size(), 1U);
        ASSERT_EQ(staged.rules.front().accepted.size(), 2U);
        const auto &score_action = staged.rules.front().accepted.front();
        EXPECT_EQ(score_action.key, key("Root", "S"));
        const auto &effect = std::get<RuleEffectObservation>(score_action.observed);
        EXPECT_EQ(effect.effect_ref, "S");
        const auto &quantity = std::get<RuleQuantityObservation>(effect.change);
        EXPECT_EQ(quantity.unit, "micro_points");
        EXPECT_EQ(quantity.requested.numerator, "25");
        EXPECT_EQ(quantity.requested.denominator, "1");
        EXPECT_EQ(quantity.actual.numerator, "25");
        EXPECT_EQ(quantity.actual.denominator, "1");

        const auto progression = advance_milestone_progression(
            {"T", "V", "J", std::stoll(quantity.actual.numerator), {{"M1", 10}, {"M2", 20}}, {}});
        const auto &p = progression.data();
        EXPECT_EQ(p.track_ref, "T");
        EXPECT_EQ(p.definition_revision_ref, "V");
        EXPECT_EQ(p.progression_context_ref, "J");
        EXPECT_EQ(p.score_micro_points, 25);
        EXPECT_EQ(p.newly_completed_begin, 0U);
        ASSERT_EQ(p.newly_completed_count, 2U);
        ASSERT_EQ(p.completed_prefix.size(), 2U);
        EXPECT_EQ(p.completed_prefix[0].milestone_ref, "M1");
        EXPECT_EQ(p.completed_prefix[1].milestone_ref, "M2");
        EXPECT_EQ(p.completed_prefix[0].target_micro_points, 10);
        EXPECT_EQ(p.completed_prefix[1].target_micro_points, 20);

        const auto source =
            std::get<Publication>(finish_unit(std::move(unit), staged, RuleCompletion::committed));
        EXPECT_EQ(source.identity, event_unit("pass-R", r.event));
        EXPECT_EQ(source.children, (std::vector{AdmittedEvent{d, 1, 1}}));
        owner.publish(source);
        ASSERT_EQ(owner.score, 25);

        const std::vector<CommittedMilestoneGroup> groups{
            {p.completed_prefix[0], {}, {}, MilestoneOccurrenceKey{"O1"}},
            {p.completed_prefix[1], {}, {}, MilestoneOccurrenceKey{"O2"}}};
        ModelRelease release;
        release.dispositions = {{"O1", MilestoneHeadDisposition::ready},
                                {"O2", MilestoneHeadDisposition::ready}};
        if (control == GroupCase::held)
            release.dispositions.at("O2") = MilestoneHeadDisposition::held;
        if (control == GroupCase::skipped)
            release.dispositions.at("O2") = MilestoneHeadDisposition::skipped;
        MilestoneTimeline timeline({1ms, 0ms, false}, release);
        timeline.accept_committed(groups, 0ms);
        ASSERT_EQ(release.scheduled.size(), 1U);
        EXPECT_TRUE(release.acknowledged.empty());
        release.acknowledge("O1");
        ASSERT_EQ(release.acknowledged, (std::vector<std::string>{"O1"}));

        const MilestoneOccurrence o1{"P", "O1", 0};
        const MilestoneOccurrence o2{"P", "O2", 1};
        const auto x1 = child(r.event, "X1", o1);
        const auto x2 = child(r.event, "X2", o2);
        // Select the typed milestone fact from the bounded local suffix of P.
        const auto keep_released = [&](const MilestoneOccurrence &occurrence,
                                       const EventBinding &event, std::size_t release_index) {
            ASSERT_EQ(occurrence.source, "P");
            ASSERT_EQ(release.acknowledged.at(release_index), occurrence.occurrence);
            ASSERT_LT(occurrence.ordinal, p.newly_completed_count);
            const auto &completion =
                p.completed_prefix.at(p.newly_completed_begin + occurrence.ordinal);
            const auto &released = release.scheduled.at(release_index);
            ASSERT_EQ(released.occurrence, MilestoneOccurrenceKey{occurrence.occurrence});
            EXPECT_EQ(released.achievement.milestone_ref, completion.milestone_ref);
            EXPECT_EQ(released.achievement.definition_revision_ref, "V");
            EXPECT_EQ(released.achievement.completion_context_ref, "J");
            owner.keep(event, completion.milestone_ref, RuleTriggerKind::milestone_completion);
        };
        const auto offer_and_commit = [&](std::string id, const MilestoneOccurrence &occurrence,
                                          const std::optional<EventBinding> &event,
                                          OccurrenceState state) {
            auto occurrence_unit = std::get<Unit>(begin_occurrences(opened.chain, "R", id));
            const auto decision = offer_occurrence_child(occurrence_unit, occurrence, event, state);
            auto publication = std::get<Publication>(
                finish_unit(std::move(occurrence_unit), {}, RuleCompletion::committed));
            EXPECT_EQ(publication.identity,
                      (UnitIdentity{id, {"I", "R"}, "R", UnitPurpose::released_occurrences}));
            EXPECT_FALSE(publication.pass);
            EXPECT_FALSE(publication.cutoff);
            EXPECT_TRUE(publication.named_suppressions.empty());
            owner.publish(publication);
            return std::pair{decision, publication};
        };

        keep_released(o1, x1, 0);
        const auto [first_offer, first] =
            offer_and_commit("release-O1", o1, x1, OccurrenceState::released);
        const AdmittedEvent expected_x1{x1, 1, 2};
        EXPECT_EQ(std::get<Reserved>(first_offer).event, expected_x1);
        EXPECT_EQ(first.children, (std::vector{expected_x1}));
        ASSERT_EQ(first.occurrences.size(), 1U);
        EXPECT_EQ(first.occurrences.front().occurrence, o1);
        EXPECT_EQ(first.occurrences.front().parent, "R");
        EXPECT_EQ(std::get<AdmittedEvent>(first.occurrences.front().disposition), expected_x1);

        const auto named = owner.run(opened.chain, d, "pass-D", {{"emit-n", {n, "n"}}});
        EXPECT_EQ(named.children, (std::vector{AdmittedEvent{n, 2, 3}}));
        timeline.advance(1ms);
        const auto [replay, repeat] =
            offer_and_commit("replay-O1", o1, x1, OccurrenceState::released);
        EXPECT_EQ(std::get<Duplicate>(replay).event, expected_x1);
        EXPECT_TRUE(repeat.children.empty());

        if (control == GroupCase::released) {
            ASSERT_EQ(release.scheduled.size(), 2U);
            release.acknowledge("O2");
            ASSERT_EQ(release.acknowledged, (std::vector<std::string>{"O1", "O2"}));
            keep_released(o2, x2, 1);
            auto inconsistent = o1;
            inconsistent.ordinal = 1;
            auto changed = x1;
            changed.cause = inconsistent;
            auto check = std::get<Unit>(begin_occurrences(opened.chain, "R", "changed-ordinal"));
            EXPECT_TRUE(std::holds_alternative<Conflict>(
                offer_occurrence_child(check, inconsistent, changed, OccurrenceState::released)));
            EXPECT_TRUE(std::holds_alternative<RolledBack>(
                finish_unit(std::move(check), {}, RuleCompletion::rolled_back)));

            const auto [offer, second] =
                offer_and_commit("release-O2", o2, x2, OccurrenceState::released);
            const AdmittedEvent expected_x2{x2, 1, 4};
            EXPECT_EQ(std::get<Reserved>(offer).event, expected_x2);
            EXPECT_EQ(second.children, (std::vector{expected_x2}));
            ASSERT_EQ(second.occurrences.size(), 1U);
            EXPECT_EQ(second.occurrences.front().occurrence, o2);
            EXPECT_EQ(second.occurrences.front().parent, "R");
            EXPECT_EQ(std::get<AdmittedEvent>(second.occurrences.front().disposition), expected_x2);
        } else {
            ASSERT_EQ(release.scheduled.size(), 1U);
            EXPECT_EQ(release.acknowledged, (std::vector<std::string>{"O1"}));
            const auto state = control == GroupCase::held ? OccurrenceState::awaiting_release
                                                          : OccurrenceState::skipped;
            const auto [offer, second] = offer_and_commit("select-O2", o2, {}, state);
            EXPECT_TRUE(second.children.empty());
            if (control == GroupCase::held) {
                EXPECT_TRUE(std::holds_alternative<AwaitingRelease>(offer));
                EXPECT_TRUE(second.occurrences.empty());
            } else {
                EXPECT_TRUE(std::holds_alternative<Skipped>(offer));
                ASSERT_EQ(second.occurrences.size(), 1U);
                EXPECT_EQ(second.occurrences.front().occurrence, o2);
                EXPECT_EQ(second.occurrences.front().parent, "R");
                EXPECT_TRUE(
                    std::holds_alternative<Skipped>(second.occurrences.front().disposition));
                const auto [again, retry] = offer_and_commit("replay-O2", o2, {}, state);
                EXPECT_TRUE(std::holds_alternative<Skipped>(again));
                EXPECT_TRUE(retry.children.empty());
            }
        }

        auto order = std::vector{AdmittedEvent{d, 1, 1}, expected_x1, AdmittedEvent{n, 2, 3}};
        if (control == GroupCase::released)
            order.push_back({x2, 1, 4});
        EXPECT_EQ(owner.queued, order);
        const auto first_milestone = owner.run(opened.chain, x1, "pass-X1");
        EXPECT_EQ(lineage(x1), (RuleEventLineage{"X1", "R", "R", {}}));
        ASSERT_EQ(first_milestone.pass->rules.size(), 1U);
        ASSERT_EQ(first_milestone.pass->rules.front().accepted.size(), 1U);
        expect_time(first_milestone.pass->rules.front().accepted.front(), key("M", "plus1"), 1, 1);
        if (control == GroupCase::released) {
            const auto second_milestone = owner.run(opened.chain, x2, "pass-X2");
            EXPECT_EQ(lineage(x2), (RuleEventLineage{"X2", "R", "R", {}}));
            ASSERT_EQ(second_milestone.pass->rules.size(), 1U);
            ASSERT_EQ(second_milestone.pass->rules.front().accepted.size(), 1U);
            expect_time(second_milestone.pass->rules.front().accepted.front(), key("M", "plus1"), 1,
                        2);
        }
        EXPECT_EQ(owner.time, control == GroupCase::released ? 2 : 1);
        EXPECT_EQ(owner.score, 25);
        EXPECT_EQ(p.completed_prefix.size(), 2U);
        EXPECT_TRUE(owner.notices.empty());
    }
}

struct OccurrenceSource {
    Owner owner;
    const RootBinding r = root();
    const EventBinding d = child(r.event, "D", NamedAction{key("Root", "emit-d")});
    const AdmittedEvent admitted{d, 1, 1};
    const MilestoneOccurrence occurrence{"P-D", "O-D", 0};
    const EventBinding x = child(d, "X", occurrence);

    OpenedChain start() {
        owner.define("C", {4, 8},
                     {named_rule("Root", "seed", {emit("emit-d", "d")}),
                      named_rule("Source", "d", {delta("S", "25", RuleDeltaUnit::micro_points)})});
        owner.active = "C";
        owner.keep(r.event, "seed");
        owner.keep(d, "d");
        auto opened = std::get<OpenedChain>(owner.open(r));
        const auto root_pass = owner.run(opened.chain, r.event, "pass-R", {{"emit-d", {d, "d"}}});
        EXPECT_EQ(root_pass.children, (std::vector{admitted}));
        EXPECT_FALSE(root_pass.cutoff);
        return opened;
    }

    auto owner_calls() const {
        return std::tuple{owner.capture_reads, owner.capacity_reads, owner.calls.size(),
                          owner.mapped.size(), owner.offered.size(), owner.handled.size()};
    }

    void expect_effects(std::int64_t score, const std::vector<AdmittedEvent> &queued) const {
        EXPECT_EQ(owner.time, 0);
        EXPECT_EQ(owner.score, score);
        EXPECT_EQ(owner.queued, queued);
        EXPECT_TRUE(owner.audits.empty());
        EXPECT_TRUE(owner.notices.empty());
    }

    RuleStagedPass stage(Unit &unit) {
        const auto plan = plan_event(unit, owner.mapper());
        EXPECT_EQ(plan.event, lineage(d));
        return std::get<RuleStagedPass>(owner.consume(unit, d, plan));
    }

    MilestoneProgressionResult progression(const RuleStagedPass &staged) const {
        const auto &action = staged.rules.at(0).accepted.at(0);
        EXPECT_EQ(action.key, key("Source", "S"));
        const auto &effect = std::get<RuleEffectObservation>(action.observed);
        EXPECT_EQ(effect.effect_ref, "S");
        const auto &score = std::get<RuleQuantityObservation>(effect.change);
        EXPECT_EQ(score.unit, "micro_points");
        EXPECT_EQ(score.actual.numerator, "25");
        EXPECT_EQ(score.actual.denominator, "1");
        return advance_milestone_progression(
            {"T", "V", "J", std::stoll(score.actual.numerator), {{"M1", 10}}, {}});
    }

    void acknowledge_source(Unit &&source, const RuleStagedPass &staged) {
        const auto pub = std::get<Publication>(
            finish_unit(std::move(source), staged, RuleCompletion::committed));
        EXPECT_EQ(pub.identity, event_unit("pass-D", d));
        EXPECT_TRUE(pub.children.empty());
        EXPECT_TRUE(pub.occurrences.empty());
        EXPECT_TRUE(pub.named_suppressions.empty());
        EXPECT_FALSE(pub.cutoff);
        ASSERT_TRUE(pub.pass);
        ASSERT_EQ(pub.pass->rules.size(), 1U);
        const auto &rule = pub.pass->rules.front();
        EXPECT_EQ(rule.captured_index, 1U);
        EXPECT_FALSE(rule.stop);
        ASSERT_EQ(rule.accepted.size(), 1U);
        EXPECT_EQ(rule.accepted.front().key, key("Source", "S"));
        owner.publish(pub);
        expect_effects(25, {admitted});
    }

    void release_and_project(Chain &chain, const MilestoneProgressionResult &progression) {
        const auto &p = progression.data();
        EXPECT_EQ(p.score_micro_points, 25);
        EXPECT_EQ(p.newly_completed_begin, 0U);
        ASSERT_EQ(p.newly_completed_count, 1U);
        ASSERT_EQ(p.completed_prefix.size(), 1U);
        const auto &completion = p.completed_prefix.front();
        EXPECT_EQ(completion.milestone_ref, "M1");
        EXPECT_EQ(completion.target_micro_points, 10);
        ModelRelease release;
        release.dispositions = {{"O-D", MilestoneHeadDisposition::ready}};
        MilestoneTimeline timeline({1ms, 0ms, false}, release);
        timeline.accept_committed({{completion, {}, {}, MilestoneOccurrenceKey{"O-D"}}}, 0ms);
        ASSERT_EQ(release.scheduled.size(), 1U);
        release.acknowledge("O-D");
        ASSERT_EQ(release.acknowledged, (std::vector<std::string>{"O-D"}));
        const auto &released = release.scheduled.front();
        EXPECT_EQ(released.achievement.milestone_ref, "M1");
        EXPECT_EQ(released.achievement.definition_revision_ref, "V");
        EXPECT_EQ(released.achievement.completion_context_ref, "J");
        owner.keep(x, released.achievement.milestone_ref, RuleTriggerKind::milestone_completion);

        auto unit = std::get<Unit>(begin_occurrences(chain, "D", "release-D"));
        const AdmittedEvent expected{x, 2, 2};
        const auto decision =
            offer_occurrence_child(unit, occurrence, x, OccurrenceState::released);
        ASSERT_TRUE(std::holds_alternative<Reserved>(decision));
        EXPECT_EQ(std::get<Reserved>(decision).event, expected);
        const auto pub =
            std::get<Publication>(finish_unit(std::move(unit), {}, RuleCompletion::committed));
        EXPECT_EQ(pub.identity,
                  (UnitIdentity{"release-D", {"I", "R"}, "D", UnitPurpose::released_occurrences}));
        EXPECT_FALSE(pub.pass);
        EXPECT_FALSE(pub.cutoff);
        EXPECT_TRUE(pub.named_suppressions.empty());
        EXPECT_EQ(pub.children, (std::vector{expected}));
        ASSERT_EQ(pub.occurrences.size(), 1U);
        EXPECT_EQ(pub.occurrences.front().occurrence, occurrence);
        EXPECT_EQ(pub.occurrences.front().parent, "D");
        EXPECT_EQ(std::get<AdmittedEvent>(pub.occurrences.front().disposition), expected);
        owner.publish(pub);
        expect_effects(25, {admitted, expected});
    }
};

TEST(Rules004, Correction154AOccurrenceBeginNeedsExactSourceAcknowledgement) {
    OccurrenceSource f;
    auto opened = f.start();
    expect_duplicate(opened.chain, f.admitted);
    const auto before = f.owner_calls();
    ASSERT_EQ(std::get<BeginDisposition>(begin_occurrences(opened.chain, "D", "release-D")),
              BeginDisposition::history_unavailable);
    EXPECT_EQ(f.owner_calls(), before);
    f.expect_effects(0, {f.admitted});

    // R's acknowledged pass can open an occurrence unit, but cannot qualify D.
    auto other = std::get<Unit>(begin_occurrences(opened.chain, "R", "release-R"));
    const auto held = f.owner_calls();
    ASSERT_EQ(std::get<BeginDisposition>(begin_occurrences(opened.chain, "D", "release-D")),
              BeginDisposition::in_flight_hold);
    EXPECT_EQ(f.owner_calls(), held);
    ASSERT_TRUE(std::holds_alternative<RolledBack>(
        finish_unit(std::move(other), {}, RuleCompletion::rolled_back)));
    f.expect_effects(0, {f.admitted});

    auto source = std::get<Unit>(begin_event(opened.chain, f.d, "pass-D"));
    const auto staged = f.stage(source);
    const auto progression = f.progression(staged);
    f.acknowledge_source(std::move(source), staged);
    f.release_and_project(opened.chain, progression);
}

TEST(Rules004, Correction155AWrongPurposeConflictsAndEventPassStillCommits) {
    OccurrenceSource f;
    auto opened = f.start();
    auto source = std::get<Unit>(begin_event(opened.chain, f.d, "pass-D"));
    const auto staged = f.stage(source);
    const auto progression = f.progression(staged);
    f.expect_effects(0, {f.admitted});

    // The selected source has staged a completion. The group can be released
    // only after the source commits, so this offer has no child binding yet.
    const auto before = f.owner_calls();
    EXPECT_TRUE(std::holds_alternative<Conflict>(
        offer_occurrence_child(source, f.occurrence, {}, OccurrenceState::awaiting_release)));
    EXPECT_EQ(f.owner_calls(), before);
    f.expect_effects(0, {f.admitted});

    f.acknowledge_source(std::move(source), staged);
    f.release_and_project(opened.chain, progression);
}

} // namespace
