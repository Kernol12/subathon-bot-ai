// SPDX-License-Identifier: GPL-3.0-or-later
// Supplied facts require complete permission witnesses, exact quantities and owned explanations.
// Raw synthetic inputs and independent literals; no SUT factory or runtime setup.
#include <subathon/core/effective_permissions.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace subathon::core;
using Outcome = PermissionOutcome;
using Reason = PermissionReason;
using Error = EffectivePermissionError;
using Match = PermissionMatch;
using Disposition = PermissionSourceDisposition;
using Source = PermissionSourceKind;
using Mode = PermissionQuantityMode;
using Class = PermissionOperationClass;

PermissionRational number(std::string numerator, std::string denominator = "1") {
    return {std::move(numerator), std::move(denominator)};
}

EffectivePermissionInput global() {
    return {"A",
            "S",
            {PermissionFactsState::complete, "C", {{"O", Class::ordinary, {}, {}, {}}}},
            {PermissionOwnerState::absent, {}},
            {PermissionFactsState::complete, {}, {}},
            {"O", {}, {}}};
}

EffectivePermissionInput timer(std::string resource = "A", std::string amount = "300000") {
    auto in = global();
    in.catalog.operations[0].resource_slots = {"timer"};
    in.catalog.operations[0].quantity_slots = {
        {"adjustment", "ms", PermissionNumberKind::rational, {}, {}, 15}};
    in.request.resources = {{"timer", std::move(resource)}};
    in.request.quantities = {{"adjustment", "ms", number(std::move(amount))}};
    return in;
}

PermissionPolicyEntry allow(std::string source = "I", std::string grant = "G") {
    return {PermissionOrigin::individual, std::move(source), {}, "O",
            PermissionEffect::allow,      std::move(grant),  {}, {}};
}

PermissionPolicyEntry timer_allow(std::string source, std::string resource,
                                  std::string maximum = "300000", std::string grant = "G") {
    auto entry = allow(std::move(source), std::move(grant));
    entry.resources = {{"timer", PermissionResourceMode::allowed_set, {std::move(resource)}}};
    entry.quantities = {{"adjustment", "ms", Mode::at_most, {}, number(std::move(maximum))}};
    return entry;
}

PermissionPolicyEntry as_role(PermissionPolicyEntry entry, std::string role = "R") {
    entry.origin = PermissionOrigin::role;
    entry.role = std::move(role);
    return entry;
}

PermissionPolicyEntry as_deny(PermissionPolicyEntry entry) {
    entry.effect = PermissionEffect::deny;
    entry.grant.reset();
    return entry;
}

struct DecisionCase {
    const char *id;
    EffectivePermissionInput input;
    Outcome outcome;
    Reason reason;
    std::optional<std::uint64_t> witness;
};

void check_context(const EffectivePermissionDecisionData &data,
                   const EffectivePermissionInput &input) {
    EXPECT_EQ(data.actor, input.actor);
    EXPECT_EQ(data.snapshot, input.snapshot);
    EXPECT_EQ(data.catalog_revision, input.catalog.revision);
    EXPECT_EQ(data.owner.state, input.owner.state);
    EXPECT_EQ(data.owner.account, input.owner.account);
    EXPECT_EQ(data.request.operation, input.request.operation);
    EXPECT_EQ(data.request.resources.size(), input.request.resources.size());
    EXPECT_EQ(data.request.quantities.size(), input.request.quantities.size());
    for (const auto &binding : input.request.resources) {
        const auto found =
            std::find_if(data.request.resources.begin(), data.request.resources.end(),
                         [&](const auto &value) { return value.slot == binding.slot; });
        EXPECT_NE(found, data.request.resources.end());
        if (found != data.request.resources.end()) {
            EXPECT_EQ(found->resource, binding.resource);
        }
    }
    for (const auto &binding : input.request.quantities) {
        const auto found =
            std::find_if(data.request.quantities.begin(), data.request.quantities.end(),
                         [&](const auto &value) { return value.slot == binding.slot; });
        EXPECT_NE(found, data.request.quantities.end());
        if (found != data.request.quantities.end()) {
            EXPECT_EQ(found->unit, binding.unit);
            EXPECT_EQ(found->value.numerator, binding.value.numerator);
            EXPECT_EQ(found->value.denominator, binding.value.denominator);
        }
    }
}

// Catching an unexpected marker reports a failure for this row and permits the next row.
void decisions(const std::vector<DecisionCase> &cases,
               const EffectivePermissionLimits &limits = {}) {
    for (const auto &row : cases) {
        SCOPED_TRACE(row.id);
        try {
            const auto result = evaluate_effective_permission(row.input, limits);
            const auto &data = result.data();
            check_context(data, row.input);
            EXPECT_EQ(data.outcome, row.outcome);
            EXPECT_EQ(data.reason, row.reason);
            EXPECT_EQ(data.witness_row, row.witness);
        } catch (const std::exception &error) {
            ADD_FAILURE() << "Expected a decision, got exception: " << error.what();
        }
    }
}

void expect_error(const EffectivePermissionInput &input, Error expected,
                  const EffectivePermissionLimits &limits = {}) {
    try {
        (void)evaluate_effective_permission(input, limits);
        ADD_FAILURE() << "Expected typed failure " << static_cast<unsigned>(expected);
    } catch (const EffectivePermissionFailure &error) {
        EXPECT_EQ(error.code(), expected);
        EXPECT_STREQ(error.what(), "Effective permission evaluation failed");
    } catch (const std::exception &error) {
        ADD_FAILURE() << "Wrong exception (including the unimplemented marker): " << error.what();
    }
}

using Mutation = std::function<void(EffectivePermissionInput &)>;
struct InvalidCase {
    const char *id;
    Mutation change;
    Error error;
};

void invalid_cases(const EffectivePermissionInput &base, const std::vector<InvalidCase> &cases) {
    for (const auto &row : cases) {
        SCOPED_TRACE(row.id);
        auto input = base;
        row.change(input);
        expect_error(input, row.error);
    }
}

void check_rows(const std::vector<PermissionSourceExplanation> &actual,
                const std::vector<PermissionSourceExplanation> &expected) {
    EXPECT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < std::min(actual.size(), expected.size()); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(actual[i].kind, expected[i].kind);
        EXPECT_EQ(actual[i].source, expected[i].source);
        EXPECT_EQ(actual[i].grant, expected[i].grant);
        EXPECT_EQ(actual[i].role, expected[i].role);
        EXPECT_EQ(actual[i].operation, expected[i].operation);
        EXPECT_EQ(actual[i].effect, expected[i].effect);
        EXPECT_EQ(actual[i].match, expected[i].match);
        EXPECT_EQ(actual[i].disposition, expected[i].disposition);
        EXPECT_EQ(actual[i].failed_resources, expected[i].failed_resources);
        EXPECT_EQ(actual[i].failed_quantities, expected[i].failed_quantities);
    }
}

TEST(EffectivePermissions, DefaultDenialZeroAndWholeRoleGrants) {
    auto zero = timer("A", "0");
    auto exact = timer();
    exact.policy.assigned_roles = {"R1", "R2"};
    exact.policy.entries = {as_role(timer_allow("S1", "A"), "R1"),
                            as_role(timer_allow("S2", "B", "600000"), "R2")};
    auto excess = exact;
    excess.request.quantities[0].value = number("600000");
    auto other = excess;
    other.request.resources[0].resource = "B";
    auto same_source = exact;
    same_source.policy.assigned_roles = {"R1"};
    same_source.policy.entries = {as_role(timer_allow("S", "A", "300000", "G1"), "R1"),
                                  as_role(timer_allow("S", "B", "600000", "G2"), "R1")};
    same_source.request.quantities[0].value = number("600000");
    decisions({{"EP01 zero needs authority", zero, Outcome::deny, Reason::default_deny, {}},
               {"EP02 inclusive role limit", exact, Outcome::permit, Reason::role_allow, 0},
               {"EP03/04 requested amount cannot borrow B or actual cap",
                excess,
                Outcome::deny,
                Reason::default_deny,
                {}},
               {"EP05 B has its own witness", other, Outcome::permit, Reason::role_allow, 1},
               {"EP10 same source never unions grants",
                same_source,
                Outcome::deny,
                Reason::default_deny,
                {}}});
}

TEST(EffectivePermissions, IndividualTierUsesResourcesBeforeQuantity) {
    auto a = timer("A", "600000");
    a.policy.assigned_roles = {"R"};
    auto broad = as_role(timer_allow("R-entry", "A", "600000"));
    broad.resources[0] = {"timer", PermissionResourceMode::any, {}};
    a.policy.entries = {timer_allow("I", "A"), broad};
    auto b = a;
    b.request.resources[0].resource = "B";
    auto boundary = a;
    boundary.request.quantities[0].value = number("300000");
    auto one_over = boundary;
    one_over.request.quantities[0].value = number("300001");
    auto beats_deny = boundary;
    beats_deny.policy.entries[1] = as_deny(beats_deny.policy.entries[1]);
    auto explicit_deny = boundary;
    explicit_deny.policy.entries[0] = as_deny(explicit_deny.policy.entries[0]);
    decisions(
        {{"D1-A selected scope fails quantity", a, Outcome::deny, Reason::individual_limit, {}},
         {"D1-B other resource uses role", b, Outcome::permit, Reason::role_allow, 1},
         {"EP07 individual allow", boundary, Outcome::permit, Reason::individual_allow, 0},
         {"EP29 one over cannot fall back", one_over, Outcome::deny, Reason::individual_limit, {}},
         {"EP28 individual defeats role denial", beats_deny, Outcome::permit,
          Reason::individual_allow, 0},
         {"EP08 individual denial", explicit_deny, Outcome::deny, Reason::individual_deny, {}}});
}

TEST(EffectivePermissions, RoleDenialAndOtherOperations) {
    auto input = timer();
    input.policy.assigned_roles = {"R1", "R2"};
    input.policy.entries = {as_role(timer_allow("S1", "A", "600000"), "R1"),
                            as_role(as_deny(timer_allow("S2", "A")), "R2")};
    input.policy.entries[1].quantities[0] = {"adjustment", "ms", Mode::closed_interval, number("0"),
                                             number("300000")};
    auto outside = input;
    outside.request.quantities[0].value = number("600000");
    auto unrelated = outside;
    unrelated.catalog.operations.push_back({"P", Class::ordinary, {}, {}, {}});
    auto entry = as_deny(allow("Individual-other"));
    entry.operation = "P";
    unrelated.policy.entries.push_back(entry);
    decisions(
        {{"EP06/34 role deny inclusive", input, Outcome::deny, Reason::role_deny, {}},
         {"EP33 constrained role deny fails", outside, Outcome::permit, Reason::role_allow, 0},
         {"EP09 other-operation individual cannot suppress O", unrelated, Outcome::permit,
          Reason::role_allow, 1}});
}

TEST(EffectivePermissions, ConditionalIndividualDenyAndDisjointScopes) {
    auto input = timer();
    input.policy.assigned_roles = {"R"};
    input.policy.entries = {as_deny(timer_allow("I", "A")),
                            as_role(timer_allow("Role", "B", "600000"))};
    input.policy.entries[0].quantities[0] = {"adjustment", "ms", Mode::closed_interval, number("0"),
                                             number("300000")};
    input.policy.entries[1].resources[0] = {"timer", PermissionResourceMode::any, {}};
    auto outside_quantity = input;
    outside_quantity.request.quantities[0].value = number("600000");
    auto b = outside_quantity;
    b.request.resources[0].resource = "B";
    auto disjoint = timer();
    disjoint.policy.entries = {timer_allow("Allow-A", "A"), as_deny(timer_allow("Deny-B", "B"))};
    auto deny_b = disjoint;
    deny_b.request.resources[0].resource = "B";
    decisions({{"EP30 full conditional denial", input, Outcome::deny, Reason::individual_deny, {}},
               {"EP31 denial condition fails but scope stays",
                outside_quantity,
                Outcome::deny,
                Reason::individual_limit,
                {}},
               {"EP32 other resource role", b, Outcome::permit, Reason::role_allow, 1},
               {"EP39 disjoint allow", disjoint, Outcome::permit, Reason::individual_allow, 0},
               {"EP39 disjoint deny", deny_b, Outcome::deny, Reason::individual_deny, {}}});
}

TEST(EffectivePermissions, CompleteMultipleQuantityWitness) {
    auto input = timer("A", "600000");
    input.catalog.operations[0].quantity_slots.push_back(
        {"count", "count", PermissionNumberKind::rational, {}, {}, 15});
    input.request.quantities.push_back({"count", "count", number("10")});
    auto g1 = timer_allow("I", "A", "300000", "G1");
    auto g2 = timer_allow("I", "A", "600000", "G2");
    g1.quantities.push_back({"count", "count", Mode::at_most, {}, number("10")});
    g2.quantities.push_back({"count", "count", Mode::at_most, {}, number("1")});
    input.policy.entries = {g2, g1};
    decisions({{"EP11 no time/count fragment union",
                input,
                Outcome::deny,
                Reason::individual_limit,
                {}}});
}

TEST(EffectivePermissions, SameEffectAlternativesHaveNoSpecificityPriority) {
    auto input = timer("A", "600000");
    input.policy.entries = {timer_allow("I", "A", "300000", "G1"),
                            timer_allow("I", "A", "600000", "G2")};
    auto broad = input;
    broad.policy.entries[1].resources[0] = {"timer", PermissionResourceMode::any, {}};
    decisions(
        {{"EP35 complete larger alternative", input, Outcome::permit, Reason::individual_allow, 1},
         {"EP36 broad individual alternative remains", broad, Outcome::permit,
          Reason::individual_allow, 1}});
}

TEST(EffectivePermissions, OpposingIndividualScopesAreInvalid) {
    auto input = timer("A", "1");
    auto broad = timer_allow("Allow", "A", "600000");
    broad.resources[0] = {"timer", PermissionResourceMode::any, {}};
    input.policy.entries = {broad, as_deny(timer_allow("Deny", "A"))};
    SCOPED_TRACE("EP37 Any versus A");
    expect_error(input, Error::contradictory_individual);
    input.policy.entries[0].resources[0] = {"timer", PermissionResourceMode::allowed_set, {"A"}};
    input.policy.entries[0].quantities[0] = {"adjustment", "ms", Mode::closed_interval, number("0"),
                                             number("300000")};
    input.policy.entries[1].quantities[0] = {"adjustment", "ms", Mode::closed_interval,
                                             number("300001"), number("600000")};
    input.request.resources[0].resource = "B";
    SCOPED_TRACE("EP38 disjoint quantities and off-request still contradict");
    expect_error(input, Error::contradictory_individual);
    auto other = input.catalog.operations[0];
    other.operation = "P";
    input.catalog.operations.push_back(other);
    for (auto &entry : input.policy.entries)
        entry.operation = "P";
    SCOPED_TRACE("opposing scopes on irrelevant operation still invalid");
    expect_error(input, Error::contradictory_individual);
}

TEST(EffectivePermissions, AtomicResourceScopesNeverUnion) {
    auto input = timer("A", "600000");
    input.catalog.operations[0].resource_slots = {"source", "destination"};
    input.request.resources = {{"source", "A"}, {"destination", "D"}};
    auto g1 = timer_allow("I", "A", "300000", "G1");
    auto g2 = timer_allow("I", "C", "600000", "G2");
    g1.resources = {{"source", PermissionResourceMode::allowed_set, {"A"}},
                    {"destination", PermissionResourceMode::allowed_set, {"B"}}};
    g2.resources = {{"source", PermissionResourceMode::allowed_set, {"C"}},
                    {"destination", PermissionResourceMode::allowed_set, {"D"}}};
    auto role = as_role(g2);
    role.source = "Role";
    role.resources[0].allowed = {"A"};
    input.policy.assigned_roles = {"R"};
    input.policy.entries = {g1, g2, role};
    auto selected = input;
    selected.request.resources[1].resource = "B";
    selected.policy.entries[2].resources[1].allowed = {"B"};
    auto sets = input;
    sets.policy.entries = {g1, role};
    sets.policy.entries[0].resources[0].allowed = {"A", "C"};
    sets.policy.entries[0].resources[1].allowed = {"B", "D"};
    auto roles_only = input;
    roles_only.policy.entries = {as_role(g1), as_role(g2)};
    decisions({{"EP40 fragments cannot select individual tier", input, Outcome::permit,
                Reason::role_allow, 2},
               {"EP41 one whole individual scope selects before amount",
                selected,
                Outcome::deny,
                Reason::individual_limit,
                {}},
               {"EP42 one explicit set conjunction is a whole scope",
                sets,
                Outcome::deny,
                Reason::individual_limit,
                {}},
               {"EP43 role scopes cannot union either",
                roles_only,
                Outcome::deny,
                Reason::default_deny,
                {}}});
}

TEST(EffectivePermissions, GlobalEmptyTupleStillSelectsIndividualLimit) {
    auto input = global();
    input.catalog.operations[0].quantity_slots = {
        {"count", "count", PermissionNumberKind::integer, {}, {}, 15}};
    input.request.quantities = {{"count", "count", number("10")}};
    auto i = allow();
    i.quantities = {{"count", "count", Mode::at_most, {}, number("5")}};
    auto r = as_role(i);
    r.source = "Role";
    r.quantities[0].upper = number("10");
    input.policy.assigned_roles = {"R"};
    input.policy.entries = {i, r};
    decisions({{"EP44 global does not omit a required binding",
                input,
                Outcome::deny,
                Reason::individual_limit,
                {}}});
}

TEST(EffectivePermissions, ExactFractionsAndIntegerDomains) {
    auto fractional = timer();
    fractional.request.quantities[0].value = number("600001", "2");
    fractional.policy.entries = {timer_allow("I", "A")};
    auto negative = fractional;
    negative.request.quantities[0].value = number("-1", "3");
    negative.policy.entries[0].quantities[0] = {"adjustment", "ms", Mode::closed_interval,
                                                number("-1"), number("0")};
    auto zero = negative;
    zero.request.quantities[0].value = number("0");
    decisions(
        {{"EP12 no permission rounding", fractional, Outcome::deny, Reason::individual_limit, {}},
         {"signed exact fraction", negative, Outcome::permit, Reason::individual_allow, 0},
         {"zero inclusive interval", zero, Outcome::permit, Reason::individual_allow, 0}});
    fractional.catalog.operations[0].quantity_slots[0].kind = PermissionNumberKind::integer;
    expect_error(fractional, Error::invalid_request);
}

TEST(EffectivePermissions, SignedMinimumMagnitudeDoesNotOverflow) {
    auto input = timer("A", "-9223372036854775808");
    auto &definition = input.catalog.operations[0].quantity_slots[0];
    definition.kind = PermissionNumberKind::integer;
    definition.minimum = number("-9223372036854775808");
    definition.maximum = number("9223372036854775807");
    auto entry = timer_allow("I", "A");
    entry.quantities[0] = {
        "adjustment", "ms", Mode::magnitude_at_most, {}, number("9223372036854775808")};
    input.policy.entries = {entry};
    auto small = input;
    small.policy.entries[0].quantities[0].upper = number("9223372036854775807");
    decisions({{"EP13 minimum fits its exact magnitude", input, Outcome::permit,
                Reason::individual_allow, 0},
               {"EP13 bound one smaller", small, Outcome::deny, Reason::individual_limit, {}}});
}

TEST(EffectivePermissions, LargeExactValuesDoNotBorrowFinalizerBounds) {
    auto wide = timer("A", "18446744073709551616");
    wide.policy.entries = {timer_allow("I", "A", "18446744073709551616")};
    auto long_value = timer("A", "1" + std::string(5000, '0'));
    long_value.policy.entries = {timer_allow("I", "A", "1" + std::string(5000, '0'))};
    decisions({{"EP14 2^64 is not narrowed", wide, Outcome::permit, Reason::individual_allow, 0},
               {"EP27 literal 5001-byte integer", long_value, Outcome::permit,
                Reason::individual_allow, 0}});
}

TEST(EffectivePermissions, ProtectedOwnerHasOnlyNarrowCapabilities) {
    auto owner = global();
    owner.owner = {PermissionOwnerState::present, "A"};
    owner.catalog.operations[0].classification = Class::protected_access_management;
    owner.policy = {PermissionFactsState::unknown, {}, {}};
    auto recovery = owner;
    recovery.catalog.operations[0].classification = Class::protected_recovery;
    auto editable_deny = owner;
    editable_deny.policy = {PermissionFactsState::complete,
                            {"Owner"},
                            {as_deny(allow("I")), as_role(as_deny(allow("Role")), "Owner")}};
    auto ordinary = owner;
    ordinary.catalog.operations[0].classification = Class::ordinary;
    ordinary.policy = {PermissionFactsState::complete, {}, {}};
    auto label = ordinary;
    label.owner.account = "Someone-else";
    label.policy.assigned_roles = {"Owner", "Administrator"};
    auto transferred = owner;
    transferred.owner.account = "New-owner";
    transferred.policy = {PermissionFactsState::complete, {}, {}};
    decisions(
        {{"EP15 narrow owner access", owner, Outcome::permit, Reason::protected_owner, 0},
         {"EP15 recovery classification", recovery, Outcome::permit, Reason::protected_owner, 0},
         {"EP18 editable denials cannot erase protection", editable_deny, Outcome::permit,
          Reason::protected_owner, 0},
         {"EP16 ordinary owner operation still needs grants",
          ordinary,
          Outcome::deny,
          Reason::default_deny,
          {}},
         {"EP17 role labels confer nothing", label, Outcome::deny, Reason::default_deny, {}},
         {"EP21 transfer removes old protection",
          transferred,
          Outcome::deny,
          Reason::default_deny,
          {}}});
}

TEST(EffectivePermissions, HelperResetCannotTargetCurrentOwner) {
    auto input = global();
    input.owner = {PermissionOwnerState::present, "Owner"};
    input.catalog.operations[0].classification = Class::helper_password_reset;
    input.catalog.operations[0].resource_slots = {"account"};
    input.catalog.operations[0].helper_target_slot = "account";
    input.request.resources = {{"account", "Owner"}};
    auto entry = allow();
    entry.resources = {{"account", PermissionResourceMode::any, {}}};
    input.policy.entries = {entry};
    auto self = input;
    self.actor = "Owner";
    auto helper = input;
    helper.request.resources[0].resource = "Helper";
    decisions({{"EP19 admin cannot reset owner through helper route",
                input,
                Outcome::deny,
                Reason::protected_owner_target,
                {}},
               {"owner must use dedicated recovery too",
                self,
                Outcome::deny,
                Reason::protected_owner_target,
                {}},
               {"other target uses normal complete grant", helper, Outcome::permit,
                Reason::individual_allow, 0}});
    try {
        const auto result = evaluate_effective_permission(input);
        check_rows(result.data().sources, {{Source::protected_owner,
                                            "Owner",
                                            {},
                                            {},
                                            "O",
                                            PermissionEffect::deny,
                                            Match::complete,
                                            Disposition::decisive_deny,
                                            {},
                                            {}},
                                           {Source::individual,
                                            "I",
                                            "G",
                                            {},
                                            "O",
                                            PermissionEffect::allow,
                                            Match::complete,
                                            Disposition::suppressed,
                                            {},
                                            {}}});
    } catch (const std::exception &error) {
        ADD_FAILURE() << error.what();
    }
}

TEST(EffectivePermissions, CurrentSnapshotDoesNotReuseEarlierPermission) {
    auto before = timer();
    before.snapshot = "P1";
    before.policy.assigned_roles = {"R"};
    before.policy.entries = {as_role(timer_allow("Role", "A"))};
    auto after = timer();
    after.snapshot = "P2";
    decisions({{"EP20 preview only describes P1", before, Outcome::permit, Reason::role_allow, 0},
               {"EP20 revocation in P2", after, Outcome::deny, Reason::default_deny, {}}});
}

TEST(EffectivePermissions, CompleteSourceExplanationsKeepLosingAndDecisiveRows) {
    auto input = timer();
    input.policy.assigned_roles = {"R"};
    auto off_resource = as_role(timer_allow("B", "B"));
    auto too_small = as_role(timer_allow("C", "A", "1"));
    auto denied1 = as_role(as_deny(timer_allow("D", "A")));
    auto denied2 = as_role(as_deny(timer_allow("E", "A")));
    input.catalog.operations.push_back({"P", Class::ordinary, {}, {}, {}});
    auto other = allow("A");
    other.operation = "P";
    input.policy.entries = {denied2,     as_role(timer_allow("F", "A")), too_small, other, denied1,
                            off_resource};
    const std::vector<PermissionSourceExplanation> expected{{Source::individual,
                                                             "A",
                                                             "G",
                                                             {},
                                                             "P",
                                                             PermissionEffect::allow,
                                                             Match::other_operation,
                                                             Disposition::not_applicable,
                                                             {},
                                                             {}},
                                                            {Source::role,
                                                             "B",
                                                             "G",
                                                             "R",
                                                             "O",
                                                             PermissionEffect::allow,
                                                             Match::resource_mismatch,
                                                             Disposition::not_applicable,
                                                             {"timer"},
                                                             {}},
                                                            {Source::role,
                                                             "C",
                                                             "G",
                                                             "R",
                                                             "O",
                                                             PermissionEffect::allow,
                                                             Match::quantity_mismatch,
                                                             Disposition::quantity_limit,
                                                             {},
                                                             {"adjustment"}},
                                                            {Source::role,
                                                             "D",
                                                             {},
                                                             "R",
                                                             "O",
                                                             PermissionEffect::deny,
                                                             Match::complete,
                                                             Disposition::decisive_deny,
                                                             {},
                                                             {}},
                                                            {Source::role,
                                                             "E",
                                                             {},
                                                             "R",
                                                             "O",
                                                             PermissionEffect::deny,
                                                             Match::complete,
                                                             Disposition::decisive_deny,
                                                             {},
                                                             {}},
                                                            {Source::role,
                                                             "F",
                                                             "G",
                                                             "R",
                                                             "O",
                                                             PermissionEffect::allow,
                                                             Match::complete,
                                                             Disposition::suppressed,
                                                             {},
                                                             {}}};
    for (int permutation = 0; permutation != 2; ++permutation) {
        SCOPED_TRACE(permutation);
        try {
            const auto result = evaluate_effective_permission(input);
            EXPECT_EQ(result.data().outcome, Outcome::deny);
            EXPECT_EQ(result.data().reason, Reason::role_deny);
            EXPECT_FALSE(result.data().witness_row);
            check_rows(result.data().sources, expected);
        } catch (const std::exception &error) {
            ADD_FAILURE() << error.what();
        }
        std::reverse(input.policy.entries.begin(), input.policy.entries.end());
    }
}

TEST(EffectivePermissions, IndividualSelectionAndProtectedRowsExplainSuppression) {
    auto input = timer("A", "600000");
    input.policy.assigned_roles = {"R"};
    input.policy.entries = {as_role(timer_allow("Role", "A", "600000")), timer_allow("I", "A")};
    try {
        const auto result = evaluate_effective_permission(input);
        EXPECT_EQ(result.data().reason, Reason::individual_limit);
        check_rows(result.data().sources, {{Source::individual,
                                            "I",
                                            "G",
                                            {},
                                            "O",
                                            PermissionEffect::allow,
                                            Match::quantity_mismatch,
                                            Disposition::quantity_limit,
                                            {},
                                            {"adjustment"}},
                                           {Source::role,
                                            "Role",
                                            "G",
                                            "R",
                                            "O",
                                            PermissionEffect::allow,
                                            Match::complete,
                                            Disposition::suppressed,
                                            {},
                                            {}}});
    } catch (const std::exception &error) {
        ADD_FAILURE() << error.what();
    }
    auto owner = global();
    owner.owner = {PermissionOwnerState::present, "A"};
    owner.catalog.operations[0].classification = Class::protected_access_management;
    owner.policy.entries = {as_deny(allow())};
    try {
        const auto result = evaluate_effective_permission(owner);
        EXPECT_EQ(result.data().witness_row, std::optional<std::uint64_t>{0});
        check_rows(result.data().sources, {{Source::protected_owner,
                                            "A",
                                            {},
                                            {},
                                            "O",
                                            PermissionEffect::allow,
                                            Match::complete,
                                            Disposition::permit_witness,
                                            {},
                                            {}},
                                           {Source::individual,
                                            "I",
                                            {},
                                            {},
                                            "O",
                                            PermissionEffect::deny,
                                            Match::complete,
                                            Disposition::suppressed,
                                            {},
                                            {}}});
    } catch (const std::exception &error) {
        ADD_FAILURE() << error.what();
    }
}

TEST(EffectivePermissions, FailedSlotListsAreCompleteAndSorted) {
    auto input = timer("A", "10");
    input.catalog.operations[0].resource_slots = {"z", "a"};
    input.catalog.operations[0].quantity_slots = {
        {"z", "count", PermissionNumberKind::rational, {}, {}, 15},
        {"a", "count", PermissionNumberKind::rational, {}, {}, 15}};
    input.request.resources = {{"z", "Z"}, {"a", "A"}};
    input.request.quantities = {{"z", "count", number("10")}, {"a", "count", number("10")}};
    auto resources = allow("R-fails");
    resources.resources = {{"z", PermissionResourceMode::allowed_set, {"Other"}},
                           {"a", PermissionResourceMode::allowed_set, {"Other"}}};
    resources.quantities = {{"z", "count", Mode::at_most, {}, number("1")},
                            {"a", "count", Mode::at_most, {}, number("1")}};
    auto quantities = resources;
    quantities.source = "Q-fails";
    quantities.resources = {{"z", PermissionResourceMode::any, {}},
                            {"a", PermissionResourceMode::any, {}}};
    input.policy.entries = {resources, quantities};
    try {
        const auto result = evaluate_effective_permission(input);
        EXPECT_EQ(result.data().reason, Reason::individual_limit);
        check_rows(result.data().sources, {{Source::individual,
                                            "Q-fails",
                                            "G",
                                            {},
                                            "O",
                                            PermissionEffect::allow,
                                            Match::quantity_mismatch,
                                            Disposition::quantity_limit,
                                            {},
                                            {"a", "z"}},
                                           {Source::individual,
                                            "R-fails",
                                            "G",
                                            {},
                                            "O",
                                            PermissionEffect::allow,
                                            Match::resource_mismatch,
                                            Disposition::not_applicable,
                                            {"a", "z"},
                                            {}}});
        const auto &request = result.data().request;
        EXPECT_EQ(request.resources.size(), 2U);
        EXPECT_EQ(request.quantities.size(), 2U);
        if (request.resources.size() == 2 && request.quantities.size() == 2) {
            EXPECT_EQ(request.resources[0].slot, "a");
            EXPECT_EQ(request.resources[0].resource, "A");
            EXPECT_EQ(request.resources[1].slot, "z");
            EXPECT_EQ(request.resources[1].resource, "Z");
            EXPECT_EQ(request.quantities[0].slot, "a");
            EXPECT_EQ(request.quantities[1].slot, "z");
        }
    } catch (const std::exception &error) {
        ADD_FAILURE() << error.what();
    }
}

TEST(EffectivePermissions, OpaqueBytesAndFullGrantWitnessOrder) {
    auto input = global();
    const std::string high(1, static_cast<char>(0x80));
    const std::string nul(1, '\0');
    input.actor = nul;
    input.policy.entries = {allow(high), allow("O", high), allow("O", nul)};
    for (int permutation = 0; permutation != 2; ++permutation) {
        SCOPED_TRACE(permutation);
        try {
            const auto result = evaluate_effective_permission(input);
            EXPECT_EQ(result.data().actor, nul);
            EXPECT_EQ(result.data().witness_row, std::optional<std::uint64_t>{0});
            check_rows(result.data().sources, {{Source::individual,
                                                "O",
                                                nul,
                                                {},
                                                "O",
                                                PermissionEffect::allow,
                                                Match::complete,
                                                Disposition::permit_witness,
                                                {},
                                                {}},
                                               {Source::individual,
                                                "O",
                                                high,
                                                {},
                                                "O",
                                                PermissionEffect::allow,
                                                Match::complete,
                                                Disposition::fitting_allow,
                                                {},
                                                {}},
                                               {Source::individual,
                                                high,
                                                "G",
                                                {},
                                                "O",
                                                PermissionEffect::allow,
                                                Match::complete,
                                                Disposition::fitting_allow,
                                                {},
                                                {}}});
        } catch (const std::exception &error) {
            ADD_FAILURE() << error.what();
        }
        std::reverse(input.policy.entries.begin(), input.policy.entries.end());
    }
}

TEST(EffectivePermissions, ReturnedDecisionOwnsAllInputLifetimes) {
    std::optional<EffectivePermissionDecision> retained;
    try {
        auto input = timer();
        input.actor = std::string(128, 'a');
        input.snapshot = "Original-snapshot";
        input.catalog.revision = "Original-catalog";
        input.policy.entries = {timer_allow(std::string(128, 's'), "A")};
        retained.emplace(evaluate_effective_permission(input));
        input.actor.assign(128, 'x');
        input.snapshot = "Changed";
        input.policy.entries.clear();
        input.request.quantities[0].value = number("999");
    } catch (const std::exception &error) {
        ADD_FAILURE() << error.what();
    }
    // Caller-level gate: failed/marker evaluation never leaves data to inspect.
    ASSERT_TRUE(retained.has_value());
    const auto copy = *retained;
    auto moved = std::move(*retained);
    retained.reset();
    for (const auto *data : {&copy.data(), &moved.data()}) {
        EXPECT_EQ(data->actor, std::string(128, 'a'));
        EXPECT_EQ(data->snapshot, "Original-snapshot");
        EXPECT_EQ(data->catalog_revision, "Original-catalog");
        EXPECT_EQ(data->request.operation, "O");
        EXPECT_EQ(data->outcome, Outcome::permit);
        EXPECT_EQ(data->reason, Reason::individual_allow);
        EXPECT_EQ(data->sources.size(), 1U);
        EXPECT_EQ(data->request.resources.size(), 1U);
        EXPECT_EQ(data->request.quantities.size(), 1U);
        if (data->sources.size() == 1) {
            EXPECT_EQ(data->sources[0].source, std::string(128, 's'));
        }
        if (data->request.resources.size() == 1) {
            EXPECT_EQ(data->request.resources[0].resource, "A");
        }
        if (data->request.quantities.size() == 1) {
            EXPECT_EQ(data->request.quantities[0].unit, "ms");
            EXPECT_EQ(data->request.quantities[0].value.numerator, "300000");
            EXPECT_EQ(data->request.quantities[0].value.denominator, "1");
        }
    }
}

TEST(EffectivePermissions, UnknownFactsAreNotKnownEmptyOrPartialSuccess) {
    invalid_cases(
        global(),
        {{"catalog unavailable", [](auto &x) { x.catalog = {}; }, Error::facts_unavailable},
         {"owner unavailable", [](auto &x) { x.owner = {}; }, Error::facts_unavailable},
         {"ordinary policy unavailable", [](auto &x) { x.policy = {}; }, Error::facts_unavailable},
         {"catalog unknown with payload",
          [](auto &x) { x.catalog.state = PermissionFactsState::unknown; }, Error::invalid_catalog},
         {"owner absent with account", [](auto &x) { x.owner.account = "Owner"; },
          Error::invalid_facts},
         {"owner present missing account",
          [](auto &x) { x.owner.state = PermissionOwnerState::present; }, Error::invalid_facts},
         {"owner unknown with account",
          [](auto &x) { x.owner = {PermissionOwnerState::unknown, "Owner"}; },
          Error::invalid_facts},
         {"policy unknown with entries",
          [](auto &x) { x.policy = {PermissionFactsState::unknown, {}, {allow()}}; },
          Error::invalid_policy},
         {"policy unknown with membership",
          [](auto &x) { x.policy = {PermissionFactsState::unknown, {"R"}, {}}; },
          Error::invalid_policy},
         {"empty present owner identity",
          [](auto &x) { x.owner = {PermissionOwnerState::present, ""}; }, Error::invalid_reference},
         {"empty actor", [](auto &x) { x.actor.clear(); }, Error::invalid_reference},
         {"empty snapshot", [](auto &x) { x.snapshot.clear(); }, Error::invalid_reference},
         {"empty complete catalog revision", [](auto &x) { x.catalog.revision.clear(); },
          Error::invalid_reference}});
    auto former_owner = global();
    former_owner.catalog.operations[0].classification = Class::protected_access_management;
    former_owner.owner = {PermissionOwnerState::present, "Different-owner"};
    former_owner.policy = {};
    expect_error(former_owner, Error::facts_unavailable);
}

TEST(EffectivePermissions, CatalogAndOperationShapesAreStrict) {
    invalid_cases(
        timer(),
        {{"duplicate operation",
          [](auto &x) { x.catalog.operations.push_back(x.catalog.operations[0]); },
          Error::invalid_catalog},
         {"duplicate resource slot",
          [](auto &x) { x.catalog.operations[0].resource_slots.push_back("timer"); },
          Error::invalid_catalog},
         {"duplicate quantity slot",
          [](auto &x) {
              x.catalog.operations[0].quantity_slots.push_back(
                  x.catalog.operations[0].quantity_slots[0]);
          },
          Error::invalid_catalog},
         {"empty catalog slot identity",
          [](auto &x) { x.catalog.operations[0].resource_slots[0].clear(); },
          Error::invalid_reference},
         {"empty catalog unit",
          [](auto &x) { x.catalog.operations[0].quantity_slots[0].unit.clear(); },
          Error::invalid_reference},
         {"no admitted quantity mode",
          [](auto &x) { x.catalog.operations[0].quantity_slots[0].allowed_modes = 0; },
          Error::invalid_catalog},
         {"unknown quantity mode bit",
          [](auto &x) { x.catalog.operations[0].quantity_slots[0].allowed_modes = 16; },
          Error::invalid_catalog},
         {"unknown classification",
          [](auto &x) { x.catalog.operations[0].classification = static_cast<Class>(255); },
          Error::invalid_catalog},
         {"unknown number kind",
          [](auto &x) {
              x.catalog.operations[0].quantity_slots[0].kind =
                  static_cast<PermissionNumberKind>(255);
          },
          Error::invalid_catalog},
         {"ordinary cannot declare helper target",
          [](auto &x) { x.catalog.operations[0].helper_target_slot = "timer"; },
          Error::invalid_catalog},
         {"helper must declare target",
          [](auto &x) { x.catalog.operations[0].classification = Class::helper_password_reset; },
          Error::invalid_catalog},
         {"helper target must be resource slot",
          [](auto &x) {
              x.catalog.operations[0].classification = Class::helper_password_reset;
              x.catalog.operations[0].helper_target_slot = "adjustment";
          },
          Error::invalid_catalog},
         {"irrelevant operation metadata still validated",
          [](auto &x) {
              auto other = x.catalog.operations[0];
              other.operation = "P";
              other.quantity_slots[0].allowed_modes = 0;
              x.catalog.operations.push_back(other);
          },
          Error::invalid_catalog}});
}

TEST(EffectivePermissions, RequestsRequireCompleteShapesAndDeclaredDomains) {
    invalid_cases(
        timer(),
        {{"unknown operation", [](auto &x) { x.request.operation = "Unknown"; },
          Error::invalid_request},
         {"missing resource", [](auto &x) { x.request.resources.clear(); }, Error::invalid_request},
         {"duplicate resource",
          [](auto &x) { x.request.resources.push_back(x.request.resources[0]); },
          Error::invalid_request},
         {"extra resource", [](auto &x) { x.request.resources.push_back({"extra", "A"}); },
          Error::invalid_request},
         {"missing quantity", [](auto &x) { x.request.quantities.clear(); },
          Error::invalid_request},
         {"duplicate quantity",
          [](auto &x) { x.request.quantities.push_back(x.request.quantities[0]); },
          Error::invalid_request},
         {"extra quantity",
          [](auto &x) { x.request.quantities.push_back({"extra", "ms", number("1")}); },
          Error::invalid_request},
         {"wrong unit is not converted", [](auto &x) { x.request.quantities[0].unit = "seconds"; },
          Error::invalid_request},
         {"empty resource", [](auto &x) { x.request.resources[0].resource.clear(); },
          Error::invalid_reference},
         {"below request domain",
          [](auto &x) { x.catalog.operations[0].quantity_slots[0].minimum = number("300001"); },
          Error::invalid_request},
         {"above request domain",
          [](auto &x) { x.catalog.operations[0].quantity_slots[0].maximum = number("299999"); },
          Error::invalid_request},
         {"inverted declared domain",
          [](auto &x) {
              auto &d = x.catalog.operations[0].quantity_slots[0];
              d.minimum = number("2");
              d.maximum = number("1");
          },
          Error::invalid_catalog},
         {"integer definition endpoints integral",
          [](auto &x) {
              auto &d = x.catalog.operations[0].quantity_slots[0];
              d.kind = PermissionNumberKind::integer;
              d.minimum = number("1", "2");
          },
          Error::invalid_catalog}});
    auto endpoints = timer("A", "0");
    auto &d = endpoints.catalog.operations[0].quantity_slots[0];
    d.minimum = number("0");
    d.maximum = number("300000");
    endpoints.policy.entries = {timer_allow("I", "A")};
    auto maximum = endpoints;
    maximum.request.quantities[0].value = number("300000");
    decisions(
        {{"declared minimum is inclusive", endpoints, Outcome::permit, Reason::individual_allow, 0},
         {"declared maximum is inclusive", maximum, Outcome::permit, Reason::individual_allow, 0}});
}

TEST(EffectivePermissions, PolicyIdentitiesAndTupleTagsAreStrict) {
    auto base = timer();
    base.policy.entries = {timer_allow("I", "A")};
    invalid_cases(
        base,
        {{"missing allow grant", [](auto &x) { x.policy.entries[0].grant.reset(); },
          Error::invalid_policy},
         {"deny cannot have grant",
          [](auto &x) { x.policy.entries[0].effect = PermissionEffect::deny; },
          Error::invalid_policy},
         {"individual cannot claim role", [](auto &x) { x.policy.entries[0].role = "R"; },
          Error::invalid_policy},
         {"role must have identity",
          [](auto &x) { x.policy.entries[0].origin = PermissionOrigin::role; },
          Error::invalid_policy},
         {"role must be assigned",
          [](auto &x) { x.policy.entries[0] = as_role(x.policy.entries[0]); },
          Error::invalid_policy},
         {"duplicate membership", [](auto &x) { x.policy.assigned_roles = {"R", "R"}; },
          Error::invalid_policy},
         {"duplicate grant even identical",
          [](auto &x) { x.policy.entries.push_back(x.policy.entries[0]); }, Error::invalid_policy},
         {"duplicate deny even identical",
          [](auto &x) {
              x.policy.entries[0] = as_deny(x.policy.entries[0]);
              x.policy.entries.push_back(x.policy.entries[0]);
          },
          Error::invalid_policy},
         {"source cannot change effect",
          [](auto &x) {
              auto entry = as_deny(x.policy.entries[0]);
              x.policy.entries.push_back(entry);
          },
          Error::invalid_policy},
         {"source cannot change role",
          [](auto &x) {
              x.policy.assigned_roles = {"R", "S"};
              x.policy.entries[0] = as_role(x.policy.entries[0]);
              auto entry = x.policy.entries[0];
              entry.role = "S";
              entry.grant = "G2";
              x.policy.entries.push_back(entry);
          },
          Error::invalid_policy},
         {"unknown operation entry", [](auto &x) { x.policy.entries[0].operation = "Unknown"; },
          Error::invalid_policy},
         {"missing complete resource", [](auto &x) { x.policy.entries[0].resources.clear(); },
          Error::invalid_policy},
         {"missing complete quantity", [](auto &x) { x.policy.entries[0].quantities.clear(); },
          Error::invalid_policy},
         {"duplicate constraint slot",
          [](auto &x) {
              x.policy.entries[0].resources.push_back(x.policy.entries[0].resources[0]);
          },
          Error::invalid_policy},
         {"empty allow-set", [](auto &x) { x.policy.entries[0].resources[0].allowed.clear(); },
          Error::invalid_policy},
         {"duplicate allowed member",
          [](auto &x) { x.policy.entries[0].resources[0].allowed.push_back("A"); },
          Error::invalid_policy},
         {"Any resource has no allowed payload",
          [](auto &x) { x.policy.entries[0].resources[0].mode = PermissionResourceMode::any; },
          Error::invalid_policy},
         {"unknown origin",
          [](auto &x) { x.policy.entries[0].origin = static_cast<PermissionOrigin>(255); },
          Error::invalid_policy},
         {"unknown effect",
          [](auto &x) { x.policy.entries[0].effect = static_cast<PermissionEffect>(255); },
          Error::invalid_policy},
         {"unknown resource tag",
          [](auto &x) {
              x.policy.entries[0].resources[0].mode = static_cast<PermissionResourceMode>(255);
          },
          Error::invalid_policy},
         {"unknown quantity tag",
          [](auto &x) { x.policy.entries[0].quantities[0].mode = static_cast<Mode>(255); },
          Error::invalid_policy},
         {"mode outside declared mask",
          [](auto &x) { x.catalog.operations[0].quantity_slots[0].allowed_modes = 1; },
          Error::invalid_policy},
         {"policy unit not inferred",
          [](auto &x) { x.policy.entries[0].quantities[0].unit = "seconds"; },
          Error::invalid_policy},
         {"Any quantity has no endpoint payload",
          [](auto &x) { x.policy.entries[0].quantities[0].mode = Mode::any; },
          Error::invalid_policy},
         {"AtMost requires upper", [](auto &x) { x.policy.entries[0].quantities[0].upper.reset(); },
          Error::invalid_policy},
         {"AtMost forbids lower",
          [](auto &x) { x.policy.entries[0].quantities[0].lower = number("0"); },
          Error::invalid_policy},
         {"interval requires both endpoints",
          [](auto &x) { x.policy.entries[0].quantities[0].mode = Mode::closed_interval; },
          Error::invalid_policy},
         {"interval cannot invert",
          [](auto &x) {
              x.policy.entries[0].quantities[0] = {"adjustment", "ms", Mode::closed_interval,
                                                   number("2"), number("1")};
          },
          Error::invalid_policy},
         {"magnitude bound nonnegative",
          [](auto &x) {
              x.policy.entries[0].quantities[0] = {
                  "adjustment", "ms", Mode::magnitude_at_most, {}, number("-1")};
          },
          Error::invalid_policy},
         {"empty present grant", [](auto &x) { x.policy.entries[0].grant = ""; },
          Error::invalid_reference}});
}

TEST(EffectivePermissions, RationalInputsAreCanonicalAndNeverRepaired) {
    const std::vector<PermissionRational> malformed{
        {"", "1"},   {"1", ""},    {"00", "1"},  {"-0", "1"}, {"01", "1"},
        {"+1", "1"}, {"1.0", "1"}, {"1e1", "1"}, {" 1", "1"}, {"1", "0"},
        {"1", "-1"}, {"1", "01"},  {"2", "4"},   {"0", "2"},  {std::string("1\0", 2), "1"}};
    for (std::size_t i = 0; i < malformed.size(); ++i) {
        SCOPED_TRACE(i);
        auto input = timer();
        input.request.quantities[0].value = malformed[i];
        expect_error(input, Error::invalid_number);
    }
    auto irrelevant = timer();
    auto definition = irrelevant.catalog.operations[0];
    definition.operation = "P";
    irrelevant.catalog.operations.push_back(definition);
    auto entry = timer_allow("Other", "B");
    entry.operation = "P";
    entry.quantities[0].upper = number("2", "4");
    irrelevant.policy.entries = {entry};
    expect_error(irrelevant, Error::invalid_number);
    irrelevant.policy.entries.clear();
    irrelevant.catalog.operations[1].quantity_slots[0].minimum = number("0", "2");
    expect_error(irrelevant, Error::invalid_number);
    auto protected_owner = irrelevant;
    protected_owner.owner = {PermissionOwnerState::present, "A"};
    protected_owner.catalog.operations[0].classification = Class::protected_access_management;
    expect_error(protected_owner, Error::invalid_number);
}

TEST(EffectivePermissions, StagePriorityIsGlobalAndIndependentOfInputOrder) {
    auto input = timer();
    input.actor.clear();
    input.catalog.operations[0].classification = static_cast<Class>(255);
    input.owner = {PermissionOwnerState::absent, "Unexpected"};
    input.policy.state = static_cast<PermissionFactsState>(255);
    expect_error(input, Error::invalid_catalog);
    input.catalog.operations[0].classification = Class::ordinary;
    expect_error(input, Error::invalid_facts);
    input.owner = {PermissionOwnerState::absent, {}};
    expect_error(input, Error::invalid_policy);
    input.policy.state = PermissionFactsState::complete;
    expect_error(input, Error::invalid_reference);

    auto shapes = timer();
    shapes.catalog.operations.push_back(shapes.catalog.operations[0]);
    shapes.request.resources.clear();
    auto missing_grant = timer_allow("I", "A");
    missing_grant.grant.reset();
    shapes.policy.entries = {missing_grant};
    expect_error(shapes, Error::invalid_catalog);
    shapes.catalog.operations.pop_back();
    expect_error(shapes, Error::invalid_request);
    shapes.request.resources = {{"timer", "A"}};
    expect_error(shapes, Error::invalid_policy);

    auto conflict = timer();
    conflict.request.quantities[0].value = number("2", "4");
    conflict.policy.entries = {timer_allow("Allow", "A"), as_deny(timer_allow("Deny", "A"))};
    for (int permutation = 0; permutation != 2; ++permutation) {
        SCOPED_TRACE(permutation);
        expect_error(conflict, Error::contradictory_individual);
        std::reverse(conflict.policy.entries.begin(), conflict.policy.entries.end());
    }
    auto numbers = timer();
    numbers.catalog.operations[0].quantity_slots[0].minimum = number("2");
    numbers.catalog.operations[0].quantity_slots[0].maximum = number("1");
    numbers.request.quantities[0].value = number("2", "4");
    expect_error(numbers, Error::invalid_number);
    numbers.request.quantities[0].value = number("0");
    expect_error(numbers, Error::invalid_catalog);
    auto unknown = timer();
    unknown.policy = {};
    unknown.request.quantities[0].value = number("2", "4");
    expect_error(unknown, Error::invalid_number);
    unknown.catalog = {};
    unknown.request.resources.clear();
    expect_error(unknown, Error::facts_unavailable);
}

TEST(EffectivePermissions, ReferenceAndComponentLimitsUseActualBytes) {
    auto input = global();
    input.actor = std::string(4096, 'a');
    decisions({{"EP23 exact reference ceiling", input, Outcome::deny, Reason::default_deny, {}}});
    input.actor.push_back('a');
    expect_error(input, Error::input_limit);
    auto limits = EffectivePermissionLimits{};
    limits.reference_bytes = 4097;
    decisions(
        {{"larger explicit reference profile", input, Outcome::deny, Reason::default_deny, {}}},
        limits);
    auto optional = global();
    optional.policy.entries = {allow("I", std::string(4097, 'g'))};
    expect_error(optional, Error::input_limit);

    auto exact = timer("A", std::string(16384, '9'));
    exact.policy.entries = {timer_allow("I", "A", std::string(16384, '9'))};
    decisions({{"exact canonical component ceiling", exact, Outcome::permit,
                Reason::individual_allow, 0}});
    exact.request.quantities[0].value.numerator.push_back('9');
    expect_error(exact, Error::input_limit);
    exact.policy.entries[0].quantities[0].upper->numerator.push_back('9');
    limits = {};
    limits.component_bytes = 16385;
    decisions(
        {{"larger exact component profile", exact, Outcome::permit, Reason::individual_allow, 0}},
        limits);

    auto signed_bytes = timer("A", "-9");
    limits = {};
    limits.component_bytes = 2;
    decisions({{"minus counts as one of two bytes",
                signed_bytes,
                Outcome::deny,
                Reason::default_deny,
                {}}},
              limits);
    signed_bytes.request.quantities[0].value = number("-99");
    expect_error(signed_bytes, Error::input_limit, limits);
    signed_bytes.request.quantities[0].value = number("1", "97");
    decisions(
        {{"denominator exact two bytes", signed_bytes, Outcome::deny, Reason::default_deny, {}}},
        limits);
    signed_bytes.request.quantities[0].value.denominator = "101";
    expect_error(signed_bytes, Error::input_limit, limits);
}

TEST(EffectivePermissions, WholeInputBudgetsIncludeIrrelevantAndNestedData) {
    auto input = global();
    auto limits = EffectivePermissionLimits{};
    limits.input_owned_bytes = 11;
    limits.input_items = 1;
    decisions({{"literal global B11/I1", input, Outcome::deny, Reason::default_deny, {}}}, limits);
    limits.input_owned_bytes = 10;
    expect_error(input, Error::input_limit, limits);

    input.policy.entries = {allow()};
    limits.input_owned_bytes = 18;
    // Global operation plus policy entry is I2, even though vectors have no header bytes.
    expect_error(input, Error::input_limit, limits);
    limits.input_items = 2;
    decisions({{"literal allow B18/I2", input, Outcome::permit, Reason::individual_allow, 0}},
              limits);
    limits.input_owned_bytes = 17;
    expect_error(input, Error::input_limit, limits);

    auto irrelevant = global();
    irrelevant.catalog.operations.push_back({"P", Class::ordinary, {}, {}, {}});
    auto other = allow();
    other.operation = "P";
    irrelevant.policy.entries = {other};
    limits.input_owned_bytes = 21;
    limits.input_items = 3;
    decisions({{"other definition and source B21/I3",
                irrelevant,
                Outcome::deny,
                Reason::default_deny,
                {}}},
              limits);
    limits.input_owned_bytes = 20;
    expect_error(irrelevant, Error::input_limit, limits);
    limits.input_owned_bytes = 21;
    limits.input_items = 2;
    expect_error(irrelevant, Error::input_limit, limits);

    auto nested = timer();
    nested.policy.entries = {timer_allow("I", "A")};
    limits = {};
    limits.input_owned_bytes = 93;
    limits.input_items = 9;
    decisions({{"nested slots/set/optional endpoint B93/I9", nested, Outcome::permit,
                Reason::individual_allow, 0}},
              limits);
    limits.input_owned_bytes = 92;
    expect_error(nested, Error::input_limit, limits);
    limits.input_owned_bytes = 93;
    limits.input_items = 8;
    expect_error(nested, Error::input_limit, limits);
}

TEST(EffectivePermissions, WholeResultBudgetsNeverTruncateSourcesOrFailedKeys) {
    auto input = global();
    auto limits = EffectivePermissionLimits{};
    limits.result_owned_bytes = 9;
    limits.result_items = 1;
    decisions({{"denial result B9/I0", input, Outcome::deny, Reason::default_deny, {}}}, limits);
    limits.result_owned_bytes = 8;
    expect_error(input, Error::result_limit, limits);

    input.policy.entries = {allow()};
    limits.result_owned_bytes = 26;
    decisions({{"permit result B26/I1", input, Outcome::permit, Reason::individual_allow, 0}},
              limits);
    limits.result_owned_bytes = 25;
    expect_error(input, Error::result_limit, limits);
    input.policy.entries.push_back(allow("J"));
    limits.result_owned_bytes = 35;
    expect_error(input, Error::result_limit, limits); // I2 cannot fit I1.
    limits.result_items = 2;
    decisions(
        {{"two complete source rows B35/I2", input, Outcome::permit, Reason::individual_allow, 0}},
        limits);
    limits.result_owned_bytes = 34;
    expect_error(input, Error::result_limit, limits);

    auto owner = global();
    owner.owner = {PermissionOwnerState::present, "A"};
    owner.catalog.operations[0].classification = Class::protected_access_management;
    owner.policy = {};
    limits.result_owned_bytes = 26;
    limits.result_items = 1;
    decisions(
        {{"virtual source and witness B26/I1", owner, Outcome::permit, Reason::protected_owner, 0}},
        limits);
    limits.result_owned_bytes = 25;
    expect_error(owner, Error::result_limit, limits);

    auto nested = timer();
    nested.policy.entries = {timer_allow("I", "A")};
    limits = {};
    limits.result_owned_bytes = 51;
    limits.result_items = 3;
    decisions(
        {{"owning nested request B51/I3", nested, Outcome::permit, Reason::individual_allow, 0}},
        limits);
    limits.result_owned_bytes = 50;
    expect_error(nested, Error::result_limit, limits);
    limits.result_owned_bytes = 51;
    limits.result_items = 2;
    expect_error(nested, Error::result_limit, limits);

    nested.request.quantities[0].value = number("600000");
    limits.result_owned_bytes = 53;
    limits.result_items = 4;
    decisions(
        {{"quantity failure key B53/I4", nested, Outcome::deny, Reason::individual_limit, {}}},
        limits);
    limits.result_owned_bytes = 52;
    expect_error(nested, Error::result_limit, limits);
    limits.result_owned_bytes = 53;
    limits.result_items = 3;
    expect_error(nested, Error::result_limit, limits);
}

TEST(EffectivePermissions, InvalidLimitsAndDerivedOverflowPrecedeAllInputWork) {
    using Member = std::uint64_t EffectivePermissionLimits::*;
    const std::array<Member, 6> members{&EffectivePermissionLimits::reference_bytes,
                                        &EffectivePermissionLimits::component_bytes,
                                        &EffectivePermissionLimits::input_items,
                                        &EffectivePermissionLimits::input_owned_bytes,
                                        &EffectivePermissionLimits::result_items,
                                        &EffectivePermissionLimits::result_owned_bytes};
    auto malformed = global();
    malformed.actor.clear();
    for (std::size_t i = 0; i < members.size(); ++i) {
        SCOPED_TRACE(i);
        auto limits = EffectivePermissionLimits{};
        limits.*members[i] = 0;
        expect_error(malformed, Error::invalid_limits, limits);
        limits.*members[i] = std::numeric_limits<std::uint64_t>::max();
        expect_error(malformed, Error::invalid_limits, limits);
    }
    for (const auto component : {UINT64_C(2305843009213693952), UINT64_C(1152921504606846976)}) {
        auto limits = EffectivePermissionLimits{};
        limits.component_bytes = component;
        expect_error(malformed, Error::invalid_limits, limits);
    }
    auto limits = EffectivePermissionLimits{};
    limits.input_items = UINT64_C(2305843009213693952);
    expect_error(malformed, Error::invalid_limits, limits);

    limits = {};
    limits.reference_bytes = 1;
    auto oversized = global();
    oversized.actor = "AA";
    oversized.snapshot.clear();
    expect_error(oversized, Error::input_limit, limits);
    auto missing = global();
    missing.actor.clear();
    expect_error(missing, Error::invalid_reference, limits);
}
} // namespace
