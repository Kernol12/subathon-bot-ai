// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include <subathon/core/milestone_progression.hpp>
#include <subathon/server/milestone_timeline.hpp>

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
        auto unit = std::get<Unit>(begin_event(opened.chain, r.event, "pass-R"));
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

} // namespace
