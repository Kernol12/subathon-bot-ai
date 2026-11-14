// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-003 v3: literal oracles from specification v0.45 section 10 / AUTH-07.
#include <subathon/core/delegated_permissions.hpp>

#include <algorithm>
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

TEST(DelegatedPermissions, ExplicitActionAndSenderGrantContainment) {
    auto in = input();
    // GRANT half only; dedicated dual-channel tests below cover owner Q3/Q4.
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
PermissionEditInput edit_input() {
    const auto in = input();
    PermissionEditInput edit;
    edit.actor = in.actor;
    edit.snapshot = in.snapshot;
    edit.catalog = in.catalog;
    edit.catalog.operations.push_back(
        {"access.edit", PermissionOperationClass::ordinary, {}, {}, {}});
    edit.owner = in.owner;
    edit.delegation = in.delegation;
    edit.management = {"access.edit", {}, {}};
    auto role = authority(capability(), "role-grant");
    role.origin = PermissionOrigin::role;
    role.role = "R";
    const auto manage = authority({"access.edit", {}, {}}, "manage");
    edit.before = {PermissionFactsState::complete,
                   {{"R", {role}}},
                   {{"admin", {}, {manage}}, {"helper", {"R"}, {}}}};
    edit.after = edit.before;
    return edit;
}

const PermissionAffectedAccountDecision *affected(const PermissionEditDecision &result,
                                                  const std::string &account) {
    const auto it = std::find_if(result.affected.begin(), result.affected.end(),
                                 [&](const auto &row) { return row.account == account; });
    return it == result.affected.end() ? nullptr : &*it;
}

TEST(DelegatedPermissions, RestrictiveRoleEditsNeedManagementUseAndScope) {
    for (unsigned kind = 0; kind < 3; ++kind) {
        auto in = edit_input();
        auto &rows = in.after.roles[0].entries;
        if (kind == 0) {
            rows[0].quantities[0].upper = PermissionRational{"200000", "1"};
        } else if (kind == 1) {
            rows.clear(); // Revoke the allow.
        } else {
            auto deny = rows[0];
            deny.source = "new-deny";
            deny.effect = PermissionEffect::deny;
            deny.grant.reset();
            rows.push_back(deny); // Add a deny.
        }
        SCOPED_TRACE(kind);
        const auto allowed = preview_permission_edit(in);
        EXPECT_EQ(allowed.management.outcome, Outcome::permit);
        EXPECT_EQ(allowed.outcome, Outcome::permit);
        ASSERT_FALSE(allowed.changed.empty());
        in.delegation.entries.clear();
        EXPECT_EQ(preview_permission_edit(in).outcome, Outcome::deny);
        in = edit_input();
        in.after.roles[0].entries.clear();
        in.before.accounts[0].individual_entries.clear();
        in.after.accounts[0].individual_entries.clear();
        const auto no_management = preview_permission_edit(in);
        EXPECT_EQ(no_management.management.outcome, Outcome::deny);
        EXPECT_EQ(no_management.outcome, Outcome::deny);
    }
}

TEST(DelegatedPermissions, UntouchedOutOfScopeRowsMayRemainButChangedRowsMayNot) {
    auto in = edit_input();
    auto outside = in.before.roles[0].entries[0];
    outside.source = "outside";
    outside.resources[0].allowed = {"B"};
    outside.quantities[0].upper = PermissionRational{"600000", "1"};
    in.before.roles[0].entries.push_back(outside);
    in.after = in.before;
    in.after.roles[0].entries[0].quantities[0].upper = PermissionRational{"200000", "1"};
    const auto preserved = preview_permission_edit(in);
    EXPECT_EQ(preserved.outcome, Outcome::permit);
    ASSERT_EQ(preserved.changed.size(), 2U); // Old and new A tuple only.
    for (const auto &row : preserved.changed)
        EXPECT_NE(row.source, "outside");
    in.after.roles[0].entries[1].quantities[0].upper = PermissionRational{"599999", "1"};
    EXPECT_EQ(preview_permission_edit(in).outcome, Outcome::deny);
}

TEST(DelegatedPermissions, RemovingOrNarrowingDenyChecksNewlyExposedRights) {
    for (bool remove : {false, true}) {
        auto in = edit_input();
        auto &allow = in.before.roles[0].entries[0];
        allow.quantities[0].upper = PermissionRational{"600000", "1"};
        auto deny = allow;
        deny.source = "role-deny";
        deny.effect = PermissionEffect::deny;
        deny.grant.reset();
        deny.quantities[0].lower = PermissionRational{"300001", "1"};
        in.before.roles[0].entries.push_back(deny);
        in.after = in.before;
        if (remove)
            in.after.roles[0].entries.pop_back();
        else
            in.after.roles[0].entries[1].quantities[0].upper = PermissionRational{"500000", "1"};
        const auto result = preview_permission_edit(in);
        EXPECT_EQ(result.outcome, Outcome::deny);
        const auto *helper = affected(result, "helper");
        ASSERT_NE(helper, nullptr);
        EXPECT_EQ(helper->newly_exposed, Outcome::deny); // >300000 newly allowed.
    }
}

TEST(DelegatedPermissions, RemovingOrWideningOverrideChecksRoleFallbackExposure) {
    for (bool remove : {false, true}) {
        auto in = edit_input();
        in.before.roles[0].entries[0].quantities[0].upper = PermissionRational{"600000", "1"};
        in.before.accounts[1].individual_entries = {authority(capability(), "override")};
        in.after = in.before;
        if (remove)
            in.after.accounts[1].individual_entries.clear();
        else
            in.after.accounts[1].individual_entries[0].quantities[0].upper =
                PermissionRational{"600000", "1"};
        const auto result = preview_permission_edit(in);
        EXPECT_EQ(result.outcome, Outcome::deny);
        const auto *helper = affected(result, "helper");
        ASSERT_NE(helper, nullptr);
        EXPECT_EQ(helper->newly_exposed, Outcome::deny);
        // For removal the changed old tuple itself fits; exposure still rejects.
        if (remove) {
            ASSERT_EQ(result.changed.size(), 1U);
            EXPECT_EQ(result.changed[0].outcome, Outcome::permit);
        }
    }
}

TEST(DelegatedPermissions, MembershipInvitationAndSelfChangesHaveNoException) {
    for (const auto *target : {"helper", "admin", "invitee"}) {
        auto in = edit_input();
        in.before.accounts[1].assigned_roles.clear();
        in.after = in.before;
        if (std::string(target) == "invitee")
            in.after.accounts.push_back({target, {"R"}, {}});
        else
            in.after.accounts[std::string(target) == "admin" ? 0 : 1].assigned_roles = {"R"};
        const auto within = preview_permission_edit(in);
        EXPECT_EQ(within.outcome, Outcome::permit);
        ASSERT_NE(affected(within, target), nullptr);
        in.delegation.entries.clear();
        const auto outside = preview_permission_edit(in);
        EXPECT_EQ(outside.outcome, Outcome::deny);
        const auto *account = affected(outside, target);
        ASSERT_NE(account, nullptr);
        EXPECT_EQ(account->newly_exposed, Outcome::deny);
    }
}

TEST(DelegatedPermissions, RemovingMembershipWithDenyCannotExposeBroaderRoleAllow) {
    auto in = edit_input();
    in.before.roles[0].entries[0].quantities[0].upper = PermissionRational{"600000", "1"};
    auto deny = in.before.roles[0].entries[0];
    deny.source = "deny";
    deny.role = "D";
    deny.effect = PermissionEffect::deny;
    deny.grant.reset();
    in.before.roles.push_back({"D", {deny}});
    in.before.accounts[1].assigned_roles.push_back("D");
    in.after = in.before;
    in.after.accounts[1].assigned_roles = {"R"};
    const auto result = preview_permission_edit(in);
    EXPECT_EQ(result.outcome, Outcome::deny);
    const auto *helper = affected(result, "helper");
    ASSERT_NE(helper, nullptr);
    EXPECT_EQ(helper->newly_exposed, Outcome::deny);
}

TEST(DelegatedPermissions, SharedRoleImpactIncludesEveryMemberDespiteIndividualMasking) {
    auto in = edit_input();
    in.before.accounts[1].individual_entries = {authority(capability("A", "100000"), "mask")};
    in.before.accounts.push_back({"other-member", {"R"}, {}});
    in.after = in.before;
    in.after.roles[0].entries[0].quantities[0].upper = PermissionRational{"600000", "1"};
    const auto result = preview_permission_edit(in);
    EXPECT_EQ(result.outcome, Outcome::deny);
    ASSERT_EQ(result.affected.size(), 2U);
    const auto *masked = affected(result, "helper");
    const auto *exposed = affected(result, "other-member");
    ASSERT_NE(masked, nullptr);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(masked->newly_exposed, Outcome::permit); // No effective increase.
    EXPECT_EQ(exposed->newly_exposed, Outcome::deny);  // New range (300000,600000].
}

TEST(DelegatedPermissions, SelfGrantCannotAuthorizeItsOwnManagementOperation) {
    auto in = edit_input();
    in.before.accounts[0].individual_entries.clear();
    // Proposed state alone would permit access.edit; current state denies.
    in.delegation.entries.push_back(authority({"access.edit", {}, {}}, "manage-scope"));
    const auto result = preview_permission_edit(in);
    EXPECT_EQ(result.management.outcome, Outcome::deny);
    EXPECT_EQ(result.outcome, Outcome::deny);
    ASSERT_NE(affected(result, "admin"), nullptr);
}

TEST(DelegatedPermissions, EditPreviewOwnsBeforeAfterSourcesAndExactLimits) {
    auto in = edit_input();
    in.before.accounts[1].assigned_roles.clear();
    in.after = in.before;
    in.after.accounts[1].individual_entries = {authority(capability(), "new-override")};
    in.probes = {
        {"helper", {"timer.adjust", {{"timer", "A"}}, {{"amount", "ms", {"300000", "1"}}}}},
        {"helper", {"timer.adjust", {{"timer", "A"}}, {{"amount", "ms", {"300001", "1"}}}}}};
    const auto result = preview_permission_edit(in);
    EXPECT_EQ(result.outcome, Outcome::permit);
    ASSERT_EQ(result.previews.size(), 2U);
    EXPECT_EQ(result.previews[0].before.outcome, Outcome::deny);
    EXPECT_EQ(result.previews[0].before.reason, PermissionReason::default_deny);
    EXPECT_EQ(result.previews[0].after.outcome, Outcome::permit);
    EXPECT_EQ(result.previews[0].after.reason, PermissionReason::individual_allow);
    ASSERT_EQ(result.previews[0].after.sources.size(), 1U);
    EXPECT_EQ(result.previews[0].after.sources[0].source, "new-override");
    EXPECT_EQ(result.previews[1].after.outcome, Outcome::deny);
    EXPECT_EQ(result.previews[1].after.reason, PermissionReason::individual_limit);
    in.after.accounts[1].individual_entries.clear();
    EXPECT_EQ(result.previews[0].after.sources[0].source, "new-override"); // Owned preview.
}

TEST(DelegatedPermissions, EditRevalidationRecomputesClosureWithCurrentAuthority) {
    for (unsigned revocation = 0; revocation < 3; ++revocation) {
        auto in = edit_input();
        in.after.roles[0].entries[0].quantities[0].upper = PermissionRational{"200000", "1"};
        EXPECT_EQ(preview_permission_edit(in).outcome, Outcome::permit);
        in.snapshot = "current-at-save";
        if (revocation == 0)
            in.delegation.entries.clear();
        else if (revocation == 1) {
            in.before.accounts[0].individual_entries.clear();
            in.after.accounts[0].individual_entries.clear();
        } else
            in.delegation.entries[0].quantities[0].upper = PermissionRational{"199999", "1"};
        const auto current = revalidate_permission_edit(in);
        EXPECT_EQ(current.snapshot, "current-at-save");
        EXPECT_EQ(current.outcome, Outcome::deny);
    }
}

TEST(DelegatedPermissions, UnknownGraphCannotPretendUnaffectedAccountsWereLoaded) {
    auto in = edit_input();
    in.before = {};
    try {
        (void)preview_permission_edit(in);
        FAIL() << "Unknown graph must fail";
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::facts_unavailable);
    }
}

PermissionAutomationInput automation_input() {
    const auto in = input();
    return {in.actor, in.snapshot, in.catalog, in.owner, in.delegation, in.delegation, in.proposed};
}
void automation_action(const PermissionAutomationDecision &result, std::size_t index, Outcome use,
                       Outcome grant) {
    ASSERT_LT(index, result.actions.size());
    EXPECT_EQ(result.actions[index].use, use);
    EXPECT_EQ(result.actions[index].grant, grant);
    EXPECT_EQ(result.actions[index].disposition, use == Outcome::permit && grant == Outcome::permit
                                                     ? PermissionAutomationDisposition::authorized
                                                     : PermissionAutomationDisposition::hold);
}

TEST(DelegatedPermissions, AutomationRequiresBothChannelsForEveryAction) {
    for (bool use : {false, true})
        for (bool grant : {false, true}) {
            auto in = automation_input();
            if (!use)
                in.use.entries.clear();
            if (!grant)
                in.delegation.entries.clear();
            const auto result = authorize_permission_automation(in);
            EXPECT_EQ(result.author, "admin");
            automation_action(result, 0, use ? Outcome::permit : Outcome::deny,
                              grant ? Outcome::permit : Outcome::deny);
        }
}

TEST(DelegatedPermissions, AutomationSelectedSenderIsSeparateInBothChannels) {
    for (bool missing_use : {false, true}) {
        auto in = automation_input();
        in.catalog.operations.push_back(
            {"chat.send", PermissionOperationClass::ordinary, {"sender"}, {}, {}});
        const DelegatedPermissionGrant bot{
            "chat.send", {{"sender", PermissionResourceMode::allowed_set, {"bot"}}}, {}};
        auto broadcaster = bot;
        broadcaster.resources[0].allowed = {"broadcaster"};
        in.use.entries.push_back(authority(bot, "bot-use"));
        in.delegation.entries.push_back(authority(bot, "bot-grant"));
        (missing_use ? in.delegation : in.use)
            .entries.push_back(authority(broadcaster, "broadcaster"));
        in.actions = {capability(), bot, broadcaster};
        const auto result = authorize_permission_automation(in);
        ASSERT_EQ(result.actions.size(), 3U);
        automation_action(result, 0, Outcome::permit, Outcome::permit);
        automation_action(result, 1, Outcome::permit, Outcome::permit);
        automation_action(result, 2, missing_use ? Outcome::deny : Outcome::permit,
                          missing_use ? Outcome::permit : Outcome::deny);
    }
}

TEST(DelegatedPermissions, AutomationRevalidationHoldsAfterEitherChannelRevocationOrRoleChange) {
    for (unsigned change = 0; change < 5; ++change) {
        auto in = automation_input();
        auto role = in.use.entries[0];
        role.origin = PermissionOrigin::role;
        role.role = "author-role";
        if (change == 2 || change == 3)
            in.use = {PermissionFactsState::complete, {"author-role"}, {role}};
        automation_action(authorize_permission_automation(in), 0, Outcome::permit, Outcome::permit);
        in.snapshot = "execution-current";
        if (change == 0 || change == 2)
            in.use = {PermissionFactsState::complete, {}, {}};
        if (change == 1)
            in.delegation.entries.clear();
        if (change == 3)
            in.use.entries[0].quantities[0].upper = PermissionRational{"299999", "1"};
        if (change == 4)
            in.delegation.entries[0].quantities[0].upper = PermissionRational{"299999", "1"};
        const auto result = revalidate_permission_automation(in);
        EXPECT_EQ(result.author, "admin");
        EXPECT_EQ(result.snapshot, "execution-current");
        automation_action(result, 0, (change == 1 || change == 4) ? Outcome::permit : Outcome::deny,
                          (change == 1 || change == 4) ? Outcome::deny : Outcome::permit);
    }
}

TEST(DelegatedPermissions, AutomationRevalidationHoldsOnlyUnauthorizedActions) {
    auto in = automation_input();
    in.actions.push_back(capability("B"));
    in.use.entries.push_back(authority(capability("B"), "B"));
    in.delegation.entries.push_back(authority(capability("B"), "B"));
    const auto preview = authorize_permission_automation(in);
    automation_action(preview, 1, Outcome::permit, Outcome::permit);
    in.delegation.entries.pop_back();
    const auto current = revalidate_permission_automation(in);
    automation_action(current, 0, Outcome::permit, Outcome::permit);
    automation_action(current, 1, Outcome::permit, Outcome::deny);
}

TEST(DelegatedPermissions, AutomationUnknownAuthorityIsFailureNotAuthorizedOrEmptyPolicy) {
    auto in = automation_input();
    in.use = {};
    try {
        (void)revalidate_permission_automation(in);
        FAIL() << "Unknown USE must fail";
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::facts_unavailable);
    }
}

TEST(DelegatedPermissions, DualChannelAutomationExactAggregateBudgets) {
    const auto base = global_input();
    const PermissionAutomationInput in{base.actor,      base.snapshot,   base.catalog, base.owner,
                                       base.delegation, base.delegation, base.proposed};
    // A1+S1+catalog5+owner2+USE8+GRANT8+action1=26; I=1+1+1+1=4.
    // Output A1+S1+C1+action(use1+grant1+disposition1)=6; I=1.
    auto limits = EffectivePermissionLimits{};
    limits.input_owned_bytes = 26;
    limits.input_items = 4;
    limits.result_owned_bytes = 6;
    limits.result_items = 1;
    automation_action(authorize_permission_automation(in, limits), 0, Outcome::permit,
                      Outcome::permit);
    --limits.input_owned_bytes;
    try {
        (void)authorize_permission_automation(in, limits);
        FAIL();
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::input_limit);
    }
    ++limits.input_owned_bytes;
    --limits.result_owned_bytes;
    try {
        (void)authorize_permission_automation(in, limits);
        FAIL();
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::result_limit);
    }
}

TEST(DelegatedPermissions, CompleteEditGraphAndNestedManagementExactBudgets) {
    const auto base = global_input();
    PermissionEditInput in{
        base.actor,
        base.snapshot,
        base.catalog,
        base.owner,
        {PermissionFactsState::complete, {}, {}},
        {"O", {}, {}},
        {PermissionFactsState::complete, {}, {{"A", {}, base.delegation.entries}}},
        {},
        {}};
    in.after = in.before;
    // Input A1+S1+catalog5+owner2+GRANT1+request1+before9+after9=29,I=5.
    // Result A1+S1+C1+outcome1+full management decision26=30,I=1 source row.
    auto limits = EffectivePermissionLimits{};
    limits.input_owned_bytes = 29;
    limits.input_items = 5;
    limits.result_owned_bytes = 30;
    limits.result_items = 1;
    const auto result = preview_permission_edit(in, limits);
    EXPECT_EQ(result.outcome, Outcome::permit);
    EXPECT_EQ(result.management.outcome, Outcome::permit);
    EXPECT_TRUE(result.changed.empty());
    EXPECT_TRUE(result.affected.empty());
    --limits.input_owned_bytes;
    try {
        (void)preview_permission_edit(in, limits);
        FAIL();
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::input_limit);
    }
    ++limits.input_owned_bytes;
    --limits.result_owned_bytes;
    try {
        (void)preview_permission_edit(in, limits);
        FAIL();
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::result_limit);
    }
}
TEST(DelegatedPermissions, ReorderingUntouchedOutsideScopeRowsDoesNotBecomeAnEdit) {
    auto in = edit_input();
    auto outside = in.before.roles[0].entries[0];
    outside.source = "outside";
    outside.resources[0].allowed = {"B"};
    in.before.roles[0].entries.push_back(outside);
    in.after = in.before;
    std::reverse(in.after.roles[0].entries.begin(), in.after.roles[0].entries.end());
    const auto result = preview_permission_edit(in);
    EXPECT_EQ(result.outcome, Outcome::permit);
    EXPECT_TRUE(result.changed.empty());
    EXPECT_TRUE(result.affected.empty());
}

TEST(DelegatedPermissions, ManagementRoleRevocationInvalidatesEditPreview) {
    auto in = edit_input();
    auto manage = in.before.accounts[0].individual_entries[0];
    manage.origin = PermissionOrigin::role;
    manage.role = "managers";
    in.before.roles.push_back({"managers", {manage}});
    in.before.accounts[0].individual_entries.clear();
    in.before.accounts[0].assigned_roles = {"managers"};
    in.after = in.before;
    in.after.roles[0].entries.clear();
    EXPECT_EQ(preview_permission_edit(in).outcome, Outcome::permit);
    // Current membership changed before save; recreate the proposal on current state.
    in.before.accounts[0].assigned_roles.clear();
    in.after.accounts[0].assigned_roles.clear();
    const auto result = revalidate_permission_edit(in);
    EXPECT_EQ(result.management.outcome, Outcome::deny);
    EXPECT_EQ(result.outcome, Outcome::deny);
}

TEST(DelegatedPermissions, CompleteChangedAndAffectedResultItemsCannotBeTruncated) {
    auto in = edit_input();
    in.after.roles[0].entries[0].quantities[0].upper = PermissionRational{"200000", "1"};
    auto limits = EffectivePermissionLimits{};
    // One management source + old/new changed rows + one affected account =4.
    limits.result_items = 4;
    const auto result = preview_permission_edit(in, limits);
    EXPECT_EQ(result.outcome, Outcome::permit);
    ASSERT_EQ(result.changed.size(), 2U);
    ASSERT_EQ(result.affected.size(), 1U);
    --limits.result_items;
    try {
        (void)preview_permission_edit(in, limits);
        FAIL();
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::result_limit);
    }
}

TEST(DelegatedPermissions, AutomationCompleteActionResultItemsCannotBeTruncated) {
    auto in = automation_input();
    in.actions.push_back(capability());
    auto limits = EffectivePermissionLimits{};
    limits.result_items = 2;
    const auto result = authorize_permission_automation(in, limits);
    ASSERT_EQ(result.actions.size(), 2U);
    automation_action(result, 0, Outcome::permit, Outcome::permit);
    automation_action(result, 1, Outcome::permit, Outcome::permit);
    --limits.result_items;
    try {
        (void)authorize_permission_automation(in, limits);
        FAIL();
    } catch (const EffectivePermissionFailure &failure) {
        EXPECT_EQ(failure.code(), EffectivePermissionError::result_limit);
    }
}
} // namespace
