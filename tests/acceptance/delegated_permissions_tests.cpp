// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-003 v1: literal oracles from specification v0.45 section 10 / AUTH-07.
#include <subathon/core/delegated_permissions.hpp>

#include <gtest/gtest.h>
#include <utility>

namespace {
using namespace subathon::core;
using Outcome = PermissionOutcome;
using Context = PermissionDelegationContext;

DelegatedPermissionGrant capability(std::string timer = "A", std::string upper = "300000",
                                    std::string denominator = "1") {
    return {"timer.adjust",
            {{"timer", PermissionResourceMode::allowed_set, {std::move(timer)}}},
            {{"amount", "ms", PermissionQuantityMode::closed_interval, PermissionRational{"0", "1"},
              PermissionRational{std::move(upper), std::move(denominator)}}}};
}

PermissionPolicyEntry authority(const DelegatedPermissionGrant &grant, std::string source = "D") {
    return {PermissionOrigin::individual,
            std::move(source),
            {},
            grant.operation,
            PermissionEffect::allow,
            "grant",
            grant.resources,
            grant.quantities};
}

PermissionDelegationInput input() {
    PermissionDelegationInput in;
    in.actor = "admin";
    in.snapshot = "preview-1";
    in.catalog = {PermissionFactsState::complete,
                  "catalog-1",
                  {{"timer.adjust",
                    PermissionOperationClass::ordinary,
                    {"timer"},
                    {{"amount", "ms", PermissionNumberKind::rational, {}, {}, 15}},
                    {}}}};
    in.owner = {PermissionOwnerState::present, "owner"};
    in.delegation = {PermissionFactsState::complete, {}, {authority(capability())}};
    in.target = "helper";
    in.proposed = {capability()};
    return in;
}

// The not-implemented marker is deliberately NOT accepted as a denial or expected
// exception. Each ordinary assertion becomes behavioral red only after review.
void check(const PermissionDelegationInput &in, Outcome expected,
           const std::vector<Outcome> &grants) {
    const auto result = evaluate_permission_delegation(in);
    EXPECT_EQ(result.actor, in.actor);
    EXPECT_EQ(result.snapshot, in.snapshot);
    EXPECT_EQ(result.catalog_revision, in.catalog.revision);
    EXPECT_EQ(result.target, in.target);
    EXPECT_EQ(result.outcome, expected);
    EXPECT_EQ(result.grants, grants);
}

void fails(const PermissionDelegationInput &in, EffectivePermissionError code) {
    try {
        (void)evaluate_permission_delegation(in);
        FAIL() << "Expected invalid/unavailable facts, not a decision";
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), code);
    }
}

TEST(DelegatedPermissions, InclusiveOwnerConfiguredBoundForEveryEditContextIncludingSelf) {
    for (auto context : {Context::role_edit, Context::invitation, Context::individual_override,
                         Context::automation_authorization}) {
        for (const auto *target : {"helper", "admin"}) {
            SCOPED_TRACE(static_cast<unsigned>(context));
            SCOPED_TRACE(target);
            auto in = input();
            in.context = context;
            in.target = target;
            check(in, Outcome::permit, {Outcome::permit}); // Exactly five minutes.
            in.proposed[0] = capability("A", "300001");
            check(in, Outcome::deny, {Outcome::deny}); // One ms beyond owner scope.
        }
    }
}

TEST(DelegatedPermissions, UsePermissionDoesNotBecomeGrantPermission) {
    auto in = input();
    const PermissionRequest request{
        "timer.adjust", {{"timer", "A"}}, {{"amount", "ms", {"300000", "1"}}}};
    const auto use = evaluate_effective_permission(
        {in.actor, in.snapshot, in.catalog, in.owner, in.delegation, request});
    ASSERT_EQ(use.data().outcome, Outcome::permit);
    // Same actor can use this operation, but no owner-configured delegation exists.
    in.delegation.entries.clear();
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, GrantPermissionDoesNotBecomeUsePermission) {
    const auto in = input();
    check(in, Outcome::permit, {Outcome::permit});
    const auto use = evaluate_effective_permission(
        {in.actor,
         in.snapshot,
         in.catalog,
         in.owner,
         {PermissionFactsState::complete, {}, {}},
         {"timer.adjust", {{"timer", "A"}}, {{"amount", "ms", {"1", "1"}}}}});
    EXPECT_EQ(use.data().outcome, Outcome::deny);
}

TEST(DelegatedPermissions, CompleteGrantsCannotLendAnotherTimersLargerLimit) {
    auto in = input();
    in.delegation.entries.push_back(authority(capability("B", "600000"), "D-B"));
    in.proposed = {capability("A", "600000"), capability("B", "600000"), capability()};
    check(in, Outcome::deny, {Outcome::deny, Outcome::permit, Outcome::permit});
}

TEST(DelegatedPermissions, EveryResourceAndQuantityDimensionMustFitTogether) {
    auto in = input();
    in.catalog.operations[0].resource_slots.push_back("scene");
    in.catalog.operations[0].quantity_slots.push_back(
        {"count", "items", PermissionNumberKind::integer, {}, {}, 15});
    auto first = capability();
    first.resources.push_back({"scene", PermissionResourceMode::allowed_set, {"S1"}});
    first.quantities.push_back({"count", "items", PermissionQuantityMode::closed_interval,
                                PermissionRational{"0", "1"}, PermissionRational{"10", "1"}});
    auto second = first;
    second.resources[0].allowed = {"B"};
    second.resources[1].allowed = {"S2"};
    second.quantities[0].upper = PermissionRational{"600000", "1"};
    second.quantities[1].upper = PermissionRational{"20", "1"};
    in.delegation.entries = {authority(first, "D1"), authority(second, "D2")};
    in.proposed = {first, second};
    check(in, Outcome::permit, {Outcome::permit, Outcome::permit});
    in.proposed[0].resources[1].allowed = {"S2"}; // No A/S2 tuple exists.
    in.proposed[1].quantities[1].upper = PermissionRational{"21", "1"};
    check(in, Outcome::deny, {Outcome::deny, Outcome::deny});
}

TEST(DelegatedPermissions, WholeProposedSetAndUnboundedRangeAreCheckedNotOneSample) {
    auto in = input();
    in.proposed[0].resources[0].allowed = {"A", "B"};
    check(in, Outcome::deny, {Outcome::deny});
    in.proposed[0] = capability();
    in.proposed[0].resources[0] = {"timer", PermissionResourceMode::any, {}};
    check(in, Outcome::deny, {Outcome::deny});
    in.proposed[0] = capability();
    in.proposed[0].quantities[0] = {"amount", "ms", PermissionQuantityMode::any, {}, {}};
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, SeparateCompleteWitnessesCanCoverDistinctRequestsInOneCapability) {
    auto in = input();
    in.delegation.entries.push_back(authority(capability("B"), "D-B"));
    in.proposed[0].resources[0].allowed = {"A", "B"};
    // Each possible request has its own complete witness; no request borrows
    // a resource or a quantity from another grant.
    check(in, Outcome::permit, {Outcome::permit});
}

TEST(DelegatedPermissions, RationalBoundaryDoesNotRoundToDelegationLimit) {
    auto in = input();
    in.delegation.entries = {authority(capability("A", "1", "3"))};
    in.proposed = {capability("A", "1", "3"), capability("A", "166667", "500000")};
    check(in, Outcome::deny, {Outcome::permit, Outcome::deny});
}

TEST(DelegatedPermissions, NarrowIndividualDelegationPreventsRoleFallbackOnlyInItsScope) {
    auto in = input();
    auto role = authority(capability("A", "600000"), "role-source");
    role.origin = PermissionOrigin::role;
    role.role = "delegated-admin";
    role.resources[0] = {"timer", PermissionResourceMode::any, {}};
    in.delegation.assigned_roles = {"delegated-admin"};
    in.delegation.entries.push_back(role);
    in.proposed = {capability("A", "600000"), capability("B", "600000"), capability()};
    check(in, Outcome::deny, {Outcome::deny, Outcome::permit, Outcome::permit});
}

TEST(DelegatedPermissions, InteriorRoleDenialCannotBeMissedByCheckingOnlyEndpoints) {
    auto in = input();
    auto allow = authority(capability());
    allow.origin = PermissionOrigin::role;
    allow.role = "allow-role";
    auto deny = allow;
    deny.source = "deny-source";
    deny.role = "deny-role";
    deny.effect = PermissionEffect::deny;
    deny.grant.reset();
    deny.quantities[0].lower = PermissionRational{"100000", "1"};
    deny.quantities[0].upper = PermissionRational{"200000", "1"};
    in.delegation = {PermissionFactsState::complete, {"allow-role", "deny-role"}, {allow, deny}};
    check(in, Outcome::deny, {Outcome::deny});
    in.proposed[0] = capability("A", "99999");
    check(in, Outcome::permit, {Outcome::permit});
}

TEST(DelegatedPermissions, SaveReevaluationUsesRevokedCurrentDelegationNotPreview) {
    auto in = input();
    const auto preview = evaluate_permission_delegation(in);
    ASSERT_EQ(preview.outcome, Outcome::permit);
    in.snapshot = "execution-2";
    in.delegation.entries.clear();
    check(in, Outcome::deny, {Outcome::deny});
    EXPECT_EQ(preview.snapshot, "preview-1");
    EXPECT_EQ(preview.outcome, Outcome::permit); // Historical calculation stays historical.
}

TEST(DelegatedPermissions, RoleRemovalAndRoleLimitChangeRequireFreshEvaluation) {
    auto in = input();
    auto role = authority(capability());
    role.origin = PermissionOrigin::role;
    role.role = "R";
    in.delegation = {PermissionFactsState::complete, {"R"}, {role}};
    check(in, Outcome::permit, {Outcome::permit});
    in.snapshot = "role-removed";
    // Current loader supplies no rows from a role the actor no longer holds.
    in.delegation = {PermissionFactsState::complete, {}, {}};
    check(in, Outcome::deny, {Outcome::deny});
    role.quantities[0].upper = PermissionRational{"299999", "1"};
    in.snapshot = "role-narrowed";
    in.delegation = {PermissionFactsState::complete, {"R"}, {role}};
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, AutomationRequiresEachExplicitActionIncludingSelectedChatSender) {
    auto in = input();
    in.context = Context::automation_authorization;
    in.catalog.operations.push_back(
        {"chat.send", PermissionOperationClass::ordinary, {"sender"}, {}, {}});
    const DelegatedPermissionGrant bot{
        "chat.send", {{"sender", PermissionResourceMode::allowed_set, {"bot"}}}, {}};
    auto broadcaster = bot;
    broadcaster.resources[0].allowed = {"broadcaster"};
    in.delegation.entries.push_back(authority(bot, "chat-source"));
    in.proposed = {capability(), bot};
    check(in, Outcome::permit, {Outcome::permit, Outcome::permit});
    in.proposed.push_back(broadcaster);
    check(in, Outcome::deny, {Outcome::permit, Outcome::permit, Outcome::deny});
}

TEST(DelegatedPermissions, ConcreteEffectivePreviewExplainsTargetSourcesAndLimits) {
    const auto in = input();
    EffectivePermissionInput preview{
        "helper",
        "proposed-revision",
        in.catalog,
        in.owner,
        {PermissionFactsState::complete, {}, {authority(capability(), "new-override")}},
        {"timer.adjust", {{"timer", "A"}}, {{"amount", "ms", {"300000", "1"}}}}};
    auto before = preview;
    before.snapshot = "current-revision";
    before.policy.entries.clear();
    EXPECT_EQ(evaluate_effective_permission(before).data().outcome, Outcome::deny);
    const auto within = evaluate_effective_permission(preview);
    EXPECT_EQ(within.data().actor, "helper");
    EXPECT_EQ(within.data().snapshot, "proposed-revision");
    EXPECT_EQ(within.data().outcome, Outcome::permit);
    ASSERT_EQ(within.data().sources.size(), 1U);
    EXPECT_EQ(within.data().sources[0].source, "new-override");
    EXPECT_EQ(within.data().reason, PermissionReason::individual_allow);
    preview.request.quantities[0].value = {"300001", "1"};
    const auto beyond = evaluate_effective_permission(preview);
    EXPECT_EQ(beyond.data().outcome, Outcome::deny);
    EXPECT_EQ(beyond.data().reason, PermissionReason::individual_limit);
    EXPECT_EQ(within.data().request.quantities[0].value.numerator, "300000");
}

TEST(DelegatedPermissions, QueuedUseRechecksCurrentAccountPolicyAfterRevocation) {
    const auto in = input();
    EffectivePermissionInput use{
        in.actor,      "queued-preview",
        in.catalog,    in.owner,
        in.delegation, {"timer.adjust", {{"timer", "A"}}, {{"amount", "ms", {"300000", "1"}}}}};
    ASSERT_EQ(evaluate_effective_permission(use).data().outcome, Outcome::permit);
    use.snapshot = "execute-revoked";
    use.policy.entries.clear();
    EXPECT_EQ(evaluate_effective_permission(use).data().outcome, Outcome::deny);
}

TEST(DelegatedPermissions, TwitchLinkCannotSupplyMissingApplicationAuthority) {
    // Local and Twitch sign-in resolve the SAME account, owner and policy facts.
    // Link/session storage and real identity resolution are separate integration gates.
    auto local = input();
    local.delegation.entries.clear();
    auto twitch = local;
    twitch.snapshot = "same-account-after-twitch-link";
    check(local, Outcome::deny, {Outcome::deny});
    check(twitch, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, UnknownDelegationOrOwnerIsNotKnownEmptyAuthority) {
    auto in = input();
    in.delegation.state = PermissionFactsState::unknown;
    fails(in, EffectivePermissionError::facts_unavailable);
    in = input();
    in.owner = {};
    fails(in, EffectivePermissionError::facts_unavailable);
}

TEST(DelegatedPermissions, MalformedCandidateCannotLoseItsFailingConstraint) {
    auto in = input();
    in.proposed[0].resources.clear();
    fails(in, EffectivePermissionError::invalid_policy);
    in = input();
    in.proposed[0].quantities[0].unit = "seconds";
    fails(in, EffectivePermissionError::invalid_policy);
    in = input();
    in.proposed[0].quantities[0].lower = PermissionRational{"300001", "1"};
    fails(in, EffectivePermissionError::invalid_policy);
}

TEST(DelegatedPermissions, ZeroLimitsAndOversizedReferencesAreFailuresNotDenials) {
    const auto in = input();
    auto limits = EffectivePermissionLimits{};
    limits.reference_bytes = 0;
    try {
        (void)evaluate_permission_delegation(in, limits);
        FAIL() << "Zero reference limit must fail";
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::invalid_limits);
    }
    limits = EffectivePermissionLimits{};
    limits.reference_bytes = 4; // "admin" has five bytes.
    try {
        (void)evaluate_permission_delegation(in, limits);
        FAIL() << "Oversized actor must fail";
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::input_limit);
    }
}
} // namespace
