// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-003 v2: literal oracles from specification v0.45 section 10 / AUTH-07.
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

void fails(const PermissionDelegationInput &in, EffectivePermissionError code,
           const EffectivePermissionLimits &limits = {}) {
    try {
        (void)evaluate_permission_delegation(in, limits);
        FAIL() << "Expected invalid/unavailable facts, not a decision";
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), code);
    }
}

TEST(DelegatedPermissions, InclusiveBoundAndAuditFieldEcho) {
    for (auto context : {Context::role_edit, Context::invitation, Context::individual_override}) {
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

TEST(DelegatedPermissions, KnownEmptyDelegationDenies) {
    auto in = input();
    in.delegation.entries.clear();
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, GrantContainmentDoesNotRequireUseFacts) {
    // USE facts are deliberately absent from this API; management USE is separate.
    check(input(), Outcome::permit, {Outcome::permit});
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

TEST(DelegatedPermissions, EmptyDelegationAfterPopulatedDelegationDenies) {
    auto in = input();
    check(in, Outcome::permit, {Outcome::permit});
    in.delegation.entries.clear();
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, AbsentRoleAndNarrowRoleAuthorityDeny) {
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

TEST(DelegatedPermissions, ExplicitActionAndSenderContainmentForPendingAutomationBinding) {
    auto in = input();
    // Retains D14 action/sender oracles for the GRANT primitive only. Q3/Q4
    // MUST bind the automation channel/lifetime before automation acceptance.
    // This test does not authorize creating/enabling/executing an automation.
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
// AUTH-03: protected ownership is not editable delegation, including for owner actors.
TEST(DelegatedPermissions, ProtectedCapabilitiesCannotBeGrantedEvenByOwner) {
    for (auto classification : {PermissionOperationClass::protected_access_management,
                                PermissionOperationClass::protected_recovery}) {
        for (const auto *actor : {"admin", "owner"}) {
            auto in = input();
            in.actor = actor;
            in.catalog.operations[0].classification = classification;
            check(in, Outcome::deny, {Outcome::deny});
        }
    }
}

TEST(DelegatedPermissions, HelperResetOwnerTargetCannotBeDelegated) {
    for (const auto *actor : {"admin", "owner"}) {
        auto in = input();
        in.actor = actor;
        auto &operation = in.catalog.operations[0];
        operation.classification = PermissionOperationClass::helper_password_reset;
        operation.helper_target_slot = "timer"; // Catalog slot keys are opaque.
        in.delegation.entries[0].resources[0] = {"timer", PermissionResourceMode::any, {}};
        check(in, Outcome::permit, {Outcome::permit}); // Non-owner target A.
        in.proposed[0].resources[0].allowed = {"A", "owner"};
        check(in, Outcome::deny, {Outcome::deny});
        in.proposed[0].resources[0] = {"timer", PermissionResourceMode::any, {}};
        check(in, Outcome::deny, {Outcome::deny});
    }
}

TEST(DelegatedPermissions, OwnerHasNoImplicitOrdinaryGrantAuthority) {
    auto in = input();
    in.actor = "owner";
    check(in, Outcome::permit, {Outcome::permit}); // Explicit owner-configured grant.
    in.delegation.entries.clear();
    check(in, Outcome::deny, {Outcome::deny});
    in.delegation.state = PermissionFactsState::unknown;
    fails(in, EffectivePermissionError::facts_unavailable);
}

TEST(DelegatedPermissions, IntegerAdjacencyCoversButRationalGapDoesNot) {
    auto in = input();
    in.catalog.operations[0].quantity_slots[0] = {"count", "items", PermissionNumberKind::integer,
                                                  {},      {},      15};
    auto first = capability("A", "10");
    first.quantities[0].slot = "count";
    first.quantities[0].unit = "items";
    auto second = first;
    second.quantities[0].lower = PermissionRational{"11", "1"};
    second.quantities[0].upper = PermissionRational{"20", "1"};
    auto low = authority(first, "low");
    auto high = authority(second, "high");
    low.origin = high.origin = PermissionOrigin::role;
    low.role = high.role = "R";
    in.delegation = {PermissionFactsState::complete, {"R"}, {low, high}};
    in.proposed = {first};
    in.proposed[0].quantities[0].upper = PermissionRational{"20", "1"};
    check(in, Outcome::permit, {Outcome::permit}); // No integer between 10 and 11.
    in.catalog.operations[0].quantity_slots[0].kind = PermissionNumberKind::rational;
    check(in, Outcome::deny, {Outcome::deny}); // 21/2 is an uncovered request.
}

TEST(DelegatedPermissions, AtMostUsesInclusiveUpperBoundWithoutImplicitLowerBound) {
    auto in = input();
    auto &bound = in.delegation.entries[0].quantities[0];
    bound.mode = PermissionQuantityMode::at_most;
    bound.lower.reset();
    auto &proposal = in.proposed[0].quantities[0];
    proposal = bound;
    check(in, Outcome::permit, {Outcome::permit});
    proposal.upper = PermissionRational{"300001", "1"};
    check(in, Outcome::deny, {Outcome::deny});
    proposal = {"amount", "ms", PermissionQuantityMode::closed_interval,
                PermissionRational{"-300001", "1"}, PermissionRational{"300000", "1"}};
    check(in, Outcome::permit, {Outcome::permit});
    // In the reverse direction, a finite interval cannot cover an unbounded tail.
    in.delegation.entries[0].quantities[0] = proposal;
    proposal = {
        "amount", "ms", PermissionQuantityMode::at_most, {}, PermissionRational{"300000", "1"}};
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, MagnitudeAndClosedIntervalCoverTheSameSignedRange) {
    auto in = input();
    auto &bound = in.delegation.entries[0].quantities[0];
    bound = {"amount",
             "ms",
             PermissionQuantityMode::magnitude_at_most,
             {},
             PermissionRational{"300000", "1"}};
    auto &proposal = in.proposed[0].quantities[0];
    proposal.lower = PermissionRational{"-300000", "1"};
    check(in, Outcome::permit, {Outcome::permit});
    proposal.lower = PermissionRational{"-300001", "1"};
    check(in, Outcome::deny, {Outcome::deny});
    bound = {"amount", "ms", PermissionQuantityMode::closed_interval,
             PermissionRational{"-300000", "1"}, PermissionRational{"300000", "1"}};
    proposal = {"amount",
                "ms",
                PermissionQuantityMode::magnitude_at_most,
                {},
                PermissionRational{"300000", "1"}};
    check(in, Outcome::permit, {Outcome::permit});
    proposal.upper = PermissionRational{"300001", "1"};
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, AnyQuantityIsIntersectedWithCatalogDomain) {
    auto in = input();
    auto &domain = in.catalog.operations[0].quantity_slots[0];
    domain.minimum = PermissionRational{"0", "1"};
    domain.maximum = PermissionRational{"300000", "1"};
    in.proposed[0].quantities[0] = {"amount", "ms", PermissionQuantityMode::any, {}, {}};
    check(in, Outcome::permit, {Outcome::permit});
    in.delegation.entries[0].quantities[0] = in.proposed[0].quantities[0];
    check(in, Outcome::permit, {Outcome::permit});
    in.delegation.entries[0].quantities[0] = capability("A", "299999").quantities[0];
    check(in, Outcome::deny, {Outcome::deny});
}

TEST(DelegatedPermissions, ContradictoryIndividualScopesFailEvenWithDisjointQuantities) {
    auto in = input();
    auto deny = authority(capability("A", "600000"), "deny");
    deny.effect = PermissionEffect::deny;
    deny.grant.reset();
    deny.quantities[0].lower = PermissionRational{"300001", "1"};
    in.delegation.entries.push_back(deny);
    in.proposed[0].resources[0].allowed = {"B"}; // Conflict is invalid off-scope too.
    fails(in, EffectivePermissionError::contradictory_individual);
}

PermissionDelegationInput global_input() {
    // Independently counted: input B=20/I=3, result B=6/I=1.
    // A1+S1+catalog(1+1+operation(1+1+optional1)) + owner(1+optional1)
    // + policy(1+entry(1+1+optional1+1+1+optional(1+1))) + context1+T1+proposal1.
    return {"A",
            "S",
            {PermissionFactsState::complete,
             "C",
             {{"O", PermissionOperationClass::ordinary, {}, {}, {}}}},
            {PermissionOwnerState::absent, {}},
            {PermissionFactsState::complete,
             {},
             {{PermissionOrigin::individual, "I", {}, "O", PermissionEffect::allow, "G", {}, {}}}},
            Context::role_edit,
            "T",
            {{"O", {}, {}}}};
}

TEST(DelegatedPermissions, ExactInputAndResultByteItemLimitsFit) {
    const auto in = global_input();
    auto limits = EffectivePermissionLimits{};
    limits.input_owned_bytes = 20;
    limits.input_items = 3;
    limits.result_owned_bytes = 6;
    limits.result_items = 1;
    const auto result = evaluate_permission_delegation(in, limits);
    EXPECT_EQ(result.outcome, Outcome::permit);
    EXPECT_EQ(result.grants, std::vector<Outcome>{Outcome::permit});
    --limits.input_owned_bytes;
    fails(in, EffectivePermissionError::input_limit, limits);
    ++limits.input_owned_bytes;
    --limits.input_items;
    fails(in, EffectivePermissionError::input_limit, limits);
    ++limits.input_items;
    --limits.result_owned_bytes;
    fails(in, EffectivePermissionError::result_limit, limits);
}

TEST(DelegatedPermissions, ExtraProposalCountsInBothInputAndCompleteResult) {
    auto in = global_input();
    in.proposed.push_back({"O", {}, {}}); // Input B=21/I=4; result B=7/I=2.
    auto limits = EffectivePermissionLimits{};
    limits.input_owned_bytes = 21;
    limits.input_items = 4;
    limits.result_owned_bytes = 7;
    limits.result_items = 2;
    const auto result = evaluate_permission_delegation(in, limits);
    EXPECT_EQ(result.outcome, Outcome::permit);
    EXPECT_EQ(result.grants, (std::vector<Outcome>{Outcome::permit, Outcome::permit}));
    --limits.input_items;
    fails(in, EffectivePermissionError::input_limit, limits);
    ++limits.input_items;
    --limits.result_items;
    fails(in, EffectivePermissionError::result_limit, limits);
}

TEST(DelegatedPermissions, SymbolicCellProductIsCheckedBeforeEnumeration) {
    auto in = global_input();
    for (unsigned i = 0; i < 14; ++i) {
        const auto slot = "r" + std::to_string(i);
        in.catalog.operations[0].resource_slots.push_back(slot);
        const PermissionResourceConstraint constraint{
            slot, PermissionResourceMode::allowed_set, {"A", "B"}};
        in.delegation.entries[0].resources.push_back(constraint);
        in.proposed[0].resources.push_back(constraint);
    }
    // I=3 + 14 catalog keys + 2*(14 constraints+28 members) =101.
    // 2^14=16384 nonempty cells > L^2=10201, though input I fits exactly.
    auto limits = EffectivePermissionLimits{};
    limits.input_items = 101;
    fails(in, EffectivePermissionError::input_limit, limits);
}

TEST(DelegatedPermissions, ExactNumericComponentLimitFitsAndOneBelowFails) {
    auto limits = EffectivePermissionLimits{};
    limits.component_bytes = 6; // 300000 has six bytes; denominator 1 has one.
    const auto in = input();
    const auto result = evaluate_permission_delegation(in, limits);
    EXPECT_EQ(result.outcome, Outcome::permit);
    EXPECT_EQ(result.grants, std::vector<Outcome>{Outcome::permit});
    --limits.component_bytes;
    fails(in, EffectivePermissionError::input_limit, limits);
}

TEST(DelegatedPermissions, UnrepresentableDerivedWorkBoundIsInvalidLimits) {
    auto limits = EffectivePermissionLimits{};
    limits.input_items = UINT64_MAX;
    fails(global_input(), EffectivePermissionError::invalid_limits, limits);
}
} // namespace
