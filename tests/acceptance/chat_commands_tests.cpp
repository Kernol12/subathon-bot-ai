// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-007 revision 1. Supplied facts, identified synthetic envelopes, and explicit clock
// samples.
#include <subathon/core/chat_commands.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace subathon::core;
using Disposition = ChatCommandDisposition;
using Error = ChatCommandError;
using Category = TwitchChatCategory;
using Fact = TwitchChatFact;
using Group = ChatEligibilityGroup;
using Route = ChatManagementRoute;
using Type = ChatArgumentType;
using Sender = ChatResponseSender;
using State = PermissionFactsState;
using Mode = PermissionQuantityMode;
using Effect = PermissionEffect;
using Number = PermissionNumberKind;

const ReceiptOrigin source{"twitch", "chan-A"};

ChatCommandEnvelope message(std::string id, std::string alias = "!wave", std::string user = "007",
                            std::vector<std::string> tokens = {}) {
    return {{source, "fake-chat", std::move(id)},
            std::move(user),
            {},
            std::move(alias),
            std::move(tokens)};
}

void fact(ChatCommandEnvelope &envelope, Category category, Fact value) {
    envelope.categories[static_cast<std::size_t>(category)] = value;
}

ChatCommandDefinition wave(std::uint64_t global = 0, std::uint64_t user = 0) {
    return {"wave",
            {"!wave", "!w"},
            {},
            {},
            Route::linked_account,
            {},
            ChatResponseTemplate{Sender::configured_default, {std::string("wave")}},
            global,
            user};
}

ChatCommandDefinition add(Type delta = Type::integer) {
    auto cmd = wave();
    cmd.command = "add";
    cmd.aliases = {"!add", "!plus"};
    cmd.arguments = {{"timer", Type::resource}, {"delta", delta}};
    cmd.actions = {{"adjust",
                    true,
                    {{"timer", ChatArgumentReference{"timer"}}},
                    {{"delta", "ms", ChatArgumentReference{"delta"}}}}};
    cmd.response.reset();
    return cmd;
}

ChatCommandAction publish(bool required) {
    return {"publish", required, {{"scene", std::string("scene-X")}}, {}};
}

ChatCommandEnvelope adjustment(std::string id, std::string timer = "A",
                               std::string delta = "300000") {
    return message(std::move(id), "!add", "007", {std::move(timer), std::move(delta)});
}

ChatCommandAuthority ignored_authority() {
    ChatCommandAuthority authority{};
    authority.source = source;
    return authority;
}

ChatCommandAuthority authority(Number kind = Number::integer) {
    auto facts = ignored_authority();
    facts.link_state = State::complete;
    facts.snapshot = "p1";
    facts.catalog = {State::complete,
                     "catalog-fixture",
                     {{"adjust",
                       PermissionOperationClass::ordinary,
                       {"timer"},
                       {{"delta", "ms", kind, {}, {}, 15}},
                       {}},
                      {"publish", PermissionOperationClass::ordinary, {"scene"}, {}, {}}}};
    facts.owner = {PermissionOwnerState::absent, {}};
    facts.category_grants_state = State::complete;
    return facts;
}

PermissionResourceConstraint resources(std::string slot, std::vector<std::string> allowed) {
    return {std::move(slot), PermissionResourceMode::allowed_set, std::move(allowed)};
}

PermissionQuantityConstraint cap(std::string maximum, Mode mode = Mode::at_most) {
    return {"delta", "ms", mode, {}, PermissionRational{std::move(maximum), "1"}};
}

DelegatedPermissionGrant adjust_scope(std::vector<std::string> allowed = {"A"},
                                      std::string maximum = "300000") {
    return {"adjust", {resources("timer", std::move(allowed))}, {cap(std::move(maximum))}};
}

DelegatedPermissionGrant publish_scope() {
    return {"publish", {resources("scene", {"scene-X"})}, {}};
}

PermissionPolicyEntry policy_row(const DelegatedPermissionGrant &scope, std::string id,
                                 Effect effect = Effect::allow,
                                 std::optional<std::string> role = {}) {
    const auto origin = role ? PermissionOrigin::role : PermissionOrigin::individual;
    auto grant = effect == Effect::allow ? std::optional("grant-" + id) : std::nullopt;
    return {origin, std::move(id),    std::move(role), scope.operation,
            effect, std::move(grant), scope.resources, scope.quantities};
}

void set_link(ChatCommandAuthority &facts, std::vector<PermissionPolicyEntry> rows = {},
              std::vector<std::string> roles = {}) {
    facts.link =
        ChatLinkedAccount{"007", "acct-U", {State::complete, std::move(roles), std::move(rows)}};
}

void expect_response(const ChatCommandResult &result, Sender sender, const std::string &text) {
    ASSERT_TRUE(result.proposal);
    ASSERT_TRUE(result.proposal->response);
    EXPECT_EQ(result.proposal->response->sender, sender);
    EXPECT_EQ(result.proposal->response->text, text);
}

void expect_action(const PermissionRequest &actual, const PermissionRequest &expected) {
    EXPECT_EQ(actual.operation, expected.operation);
    ASSERT_EQ(actual.resources.size(), expected.resources.size());
    ASSERT_EQ(actual.quantities.size(), expected.quantities.size());
    for (const auto &binding : expected.resources) {
        const auto actual_slot =
            std::ranges::find(actual.resources, binding.slot, &PermissionResourceBinding::slot);
        ASSERT_NE(actual_slot, actual.resources.end());
        EXPECT_EQ(actual_slot->resource, binding.resource);
    }
    for (const auto &binding : expected.quantities) {
        const auto actual_slot =
            std::ranges::find(actual.quantities, binding.slot, &PermissionQuantityBinding::slot);
        ASSERT_NE(actual_slot, actual.quantities.end());
        EXPECT_EQ(actual_slot->unit, binding.unit);
        EXPECT_EQ(actual_slot->value.numerator, binding.value.numerator);
        EXPECT_EQ(actual_slot->value.denominator, binding.value.denominator);
    }
}

template <typename Operation> void expect_failure(Error expected, Operation operation) {
    try {
        operation();
        ADD_FAILURE() << "Expected ChatCommandFailure with code " << static_cast<int>(expected);
    } catch (const ChatCommandFailure &failure) {
        EXPECT_EQ(failure.code(), expected);
    }
}

class ChatCommands : public testing::Test {
  protected:
    void start(std::vector<ChatCommandDefinition> commands, const ChatCommandLimits &limits = {},
               std::string revision = "r1") {
        evaluator.reset();
        evaluator = std::make_unique<ChatCommandEvaluator>(
            ChatCommandRevision{source, revision, std::move(commands)}, limits);
        active_revision = std::move(revision);
    }

    ChatCommandResult expect(const ChatCommandEnvelope &envelope, Disposition disposition,
                             std::optional<std::string> command = "wave", std::uint64_t now = 0,
                             const ChatCommandAuthority &facts = ignored_authority(),
                             bool replay = false) {
        SCOPED_TRACE(envelope.message.opaque_id);
        const auto result = evaluator->evaluate(envelope, facts, now);
        EXPECT_EQ(result.message, envelope.message);
        EXPECT_EQ(result.revision, active_revision);
        EXPECT_EQ(result.terminal.command, command);
        EXPECT_EQ(result.terminal.disposition, disposition);
        EXPECT_EQ(result.replay, replay);
        EXPECT_EQ(result.proposal.has_value(), disposition == Disposition::admitted && !replay);
        return result;
    }

    void rejects(const ChatCommandEnvelope &envelope, Error code, std::uint64_t now = 0,
                 const ChatCommandAuthority &facts = ignored_authority()) {
        SCOPED_TRACE(envelope.message.opaque_id);
        expect_failure(code, [&] { (void)evaluator->evaluate(envelope, facts, now); });
    }

  private:
    std::unique_ptr<ChatCommandEvaluator> evaluator;
    std::string active_revision;
};

TEST_F(ChatCommands, PublicAliasesAreExactAndRenderTextBytes) {
    auto cmd = wave();
    cmd.command = "hello";
    cmd.aliases = {"!hello", "!hi"};
    cmd.arguments = {{"who", Type::text}};
    cmd.response =
        ChatResponseTemplate{Sender::bot, {std::string("Hi "), ChatArgumentReference{"who"}}};
    cmd.route = Route::both;
    start({cmd});
    auto facts = ignored_authority();
    facts.source = {"twitch", "another-source"};
    auto result = expect(message("m1", "!hi", "007", {std::string("A\0da", 4)}),
                         Disposition::admitted, "hello", 0, facts);
    ASSERT_TRUE(result.proposal);
    EXPECT_TRUE(result.proposal->actions.empty());
    expect_response(result, Sender::bot, std::string("Hi A\0da", 7));
    result = expect(message("m2", "!hello", "007", {""}), Disposition::admitted, "hello");
    expect_response(result, Sender::bot, "Hi ");
    expect(message("m3", "!HI", "007", {"Ada"}), Disposition::unrecognized_alias, std::nullopt);
    rejects(message("m4", "!hi Ada"), Error::invalid_envelope);
}

TEST_F(ChatCommands, BindsTypedActionAndResponseArgumentsWithoutCoercion) {
    auto cmd = add(Type::rational);
    cmd.arguments.push_back({"count", Type::integer});
    cmd.response = ChatResponseTemplate{Sender::broadcaster,
                                        {ChatArgumentReference{"timer"}, std::string(" "),
                                         ChatArgumentReference{"delta"}, std::string(" "),
                                         ChatArgumentReference{"count"}}};
    start({cmd});
    auto facts = authority(Number::rational);
    set_link(facts, {policy_row(adjust_scope({"00A"}), "use")});
    const auto result = expect(message("m1", "!plus", "007", {"00A", "-7/2", "-8"}),
                               Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 1U);
    expect_action(result.proposal->actions[0],
                  {"adjust", {{"timer", "00A"}}, {{"delta", "ms", {"-7", "2"}}}});
    expect_response(result, Sender::broadcaster, "00A -7/2 -8");
    const auto integral = expect(message("m2", "!add", "007", {"00A", "1/1", "9"}),
                                 Disposition::admitted, "add", 0, facts);
    expect_response(integral, Sender::broadcaster, "00A 1 9");
}

TEST_F(ChatCommands, BindsEvenUnusedArgumentsBeforeEligibilityOrAuthority) {
    auto cmd = add();
    cmd.arguments.push_back({"unused", Type::rational});
    cmd.eligibility = ChatEligibility{Category::vip, {}};
    start({cmd});
    auto facts = authority();
    set_link(facts);
    facts.link->use = {};
    const std::vector<std::vector<std::string>> invalid{
        {"A", "300000", "2/4"}, {"A", "300000"},     {"A", "300000", "1/2", "x"},
        {"", "300000", "1/2"},  {"A", "+1", "1/2"},  {"A", "01", "1/2"},
        {"A", "-0", "1/2"},     {"A", "1/2", "1/2"}, {"A", "1", "0/2"},
        {"A", "1", "1/0"},      {"A", "1", "1/-2"},  {"A", "1", "1/02"},
        {"A", "1", "0.5"}};
    for (std::size_t row = 0; row < invalid.size(); ++row) {
        auto in = message("bad-" + std::to_string(row), "!add", "007", invalid[row]);
        fact(in, Category::vip, Fact::absent);
        expect(in, Disposition::invalid_arguments, "add", 0, facts);
    }

    auto in = message("false-eligibility", "!add", "007", {"A", "300000", "1/2"});
    fact(in, Category::vip, Fact::absent);
    expect(in, Disposition::ineligible, "add", 0, ignored_authority());
    in.message.opaque_id = "pending-use";
    fact(in, Category::vip, Fact::present);
    expect(in, Disposition::authority_unavailable, "add", 0, facts);
}

TEST_F(ChatCommands, NestedAllAnyUseIndependentFactsAndExactExplicitUsers) {
    auto cmd = wave();
    cmd.eligibility =
        ChatEligibility{Group::any,
                        {{ChatExplicitUser{"007"}, {}},
                         {Group::all, {{Category::vip, {}}, {Category::subscriber, {}}}}}};
    start({cmd});
    auto in = message("exact-user");
    expect(in, Disposition::admitted);
    struct Row {
        const char *id;
        const char *user;
        Fact vip;
        Fact subscriber;
        Disposition expected;
    };
    for (const auto &row : std::vector<Row>{
             {"short-id", "7", Fact::unknown, Fact::unknown, Disposition::eligibility_unavailable},
             {"other-id", "008", Fact::unknown, Fact::unknown,
              Disposition::eligibility_unavailable},
             {"false-dominates", "7", Fact::absent, Fact::unknown, Disposition::ineligible},
             {"subscriber-false", "7", Fact::present, Fact::absent, Disposition::ineligible},
             {"subscriber-unknown", "7", Fact::present, Fact::unknown,
              Disposition::eligibility_unavailable},
             {"both-present", "7", Fact::present, Fact::present, Disposition::admitted}}) {
        in = message(row.id, "!wave", row.user);
        fact(in, Category::vip, row.vip);
        fact(in, Category::subscriber, row.subscriber);
        fact(in, Category::moderator, Fact::present);
        fact(in, Category::broadcaster, Fact::present);
        expect(in, row.expected);
    }
}

TEST_F(ChatCommands, AnyAndUnsubscribedViewerTreatUnknownAsItsOwnFact) {
    auto cmd = wave();
    cmd.eligibility = ChatEligibility{Group::any, {{Category::moderator, {}}, {Category::vip, {}}}};
    start({cmd});
    auto in = message("present-unknown");
    fact(in, Category::moderator, Fact::present);
    expect(in, Disposition::admitted);
    in.message.opaque_id = "absent-unknown";
    fact(in, Category::moderator, Fact::absent);
    expect(in, Disposition::eligibility_unavailable);
    in.message.opaque_id = "both-absent";
    fact(in, Category::vip, Fact::absent);
    expect(in, Disposition::ineligible);
    in.message.opaque_id = "absent-present";
    fact(in, Category::vip, Fact::present);
    expect(in, Disposition::admitted);

    cmd.eligibility =
        ChatEligibility{Group::all, {{Category::vip, {}}, {Category::unsubscribed_viewer, {}}}};
    start({cmd}, {}, "r2");
    in = message("viewer-unknown");
    fact(in, Category::vip, Fact::present);
    fact(in, Category::subscriber, Fact::absent);
    expect(in, Disposition::eligibility_unavailable);
    in.message.opaque_id = "viewer-present";
    fact(in, Category::unsubscribed_viewer, Fact::present);
    expect(in, Disposition::admitted);
}

TEST_F(ChatCommands, MixedDomainFaultsPreferStaticLiteralInEitherActionOrder) {
    auto cmd = add();
    cmd.global_cooldown_ms = 200;
    cmd.user_cooldown_ms = 200;
    auto literal = cmd.actions.front();
    literal.resources[0].value = std::string("A");
    literal.quantities[0].value = PermissionRational{"300001", "1"};
    cmd.actions.push_back(literal);
    auto facts = authority();
    facts.catalog.operations[0].quantity_slots[0].maximum = PermissionRational{"300000", "1"};
    set_link(facts, {policy_row(adjust_scope(), "use")});

    start({cmd});
    expect(adjustment("bound-first", "A", "300001"), Disposition::authority_unavailable, "add", 100,
           facts);
    std::ranges::reverse(cmd.actions);
    start({cmd}, {}, "r2");
    expect(adjustment("literal-first", "A", "300001"), Disposition::authority_unavailable, "add",
           100, facts);

    auto widened = facts;
    widened.catalog.revision = "wider-domain";
    widened.catalog.operations[0].quantity_slots[0].maximum = PermissionRational{"300001", "1"};
    set_link(widened, {policy_row(adjust_scope({"A"}, "300001"), "use-wider")});
    expect(adjustment("literal-now-in-domain", "A", "300001"), Disposition::admitted, "add", 100,
           widened);

    cmd.actions.front().quantities[0].value = PermissionRational{"300000", "1"};
    start({cmd}, {}, "r3");
    const auto permitted = facts;
    facts.link->use = {};
    expect(adjustment("only-bound-fault", "A", "300001"), Disposition::invalid_arguments, "add",
           100, facts);
    expect(adjustment("both-valid"), Disposition::admitted, "add", 100, permitted);
}

TEST_F(ChatCommands, CatalogShapePreflightIncludesAnOptionalActionBeforeArgumentDomains) {
    auto cmd = add();
    cmd.actions.push_back(publish(false));
    start({cmd});
    auto facts = authority();
    facts.catalog.operations[0].quantity_slots[0].maximum = PermissionRational{"300000", "1"};
    facts.catalog.operations[1].resource_slots = {"wrong-slot"};
    expect(adjustment("shape-before-domain", "A", "300001"), Disposition::authority_unavailable,
           "add", 0, facts);
    facts.catalog.operations[1].resource_slots = {"scene"};
    expect(adjustment("bound-domain", "A", "300001"), Disposition::invalid_arguments, "add", 0,
           facts);
}

TEST_F(ChatCommands, DefaultLinkedRouteRequiresAnExactSenderAndAuthoritySource) {
    start({add()});
    auto facts = authority();
    facts.category_grants = {{Category::moderator, adjust_scope()}};
    auto in = adjustment("unlinked");
    fact(in, Category::moderator, Fact::present);
    expect(in, Disposition::required_action_denied, "add", 0, facts);

    set_link(facts, {policy_row(adjust_scope(), "use")});
    in.message.opaque_id = "source-mismatch";
    facts.source.source_account = "chan-B";
    rejects(in, Error::invalid_envelope, 100, facts);
    facts.source = source;
    expect(in, Disposition::admitted, "add", 0, facts);

    in.message.opaque_id = "sender-mismatch";
    facts.link->twitch_user = "7";
    rejects(in, Error::invalid_envelope, 100, facts);
    facts.link->twitch_user = "007";
    expect(in, Disposition::admitted, "add", 0, facts);
}

TEST_F(ChatCommands, LinkedScopeSelectsResourcesBeforeCheckingExactRequestedQuantity) {
    start({add()});
    auto facts = authority();
    set_link(facts,
             {policy_row(adjust_scope(), "individual"),
              policy_row(adjust_scope({"A", "B"}, "600000"), "role", Effect::allow, "editor")},
             {"editor"});
    expect(adjustment("inclusive"), Disposition::admitted, "add", 0, facts);
    expect(adjustment("one-over", "A", "300001"), Disposition::required_action_denied, "add", 0,
           facts);
    const auto outside = expect(adjustment("outside-individual", "B", "600000"),
                                Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(outside.proposal);
    ASSERT_EQ(outside.proposal->actions.size(), 1U);
    expect_action(outside.proposal->actions[0],
                  {"adjust", {{"timer", "B"}}, {{"delta", "ms", {"600000", "1"}}}});
}

TEST_F(ChatCommands, LinkedRoleDenyHasInclusiveBoundsAndCurrentFactsReplacePermits) {
    start({add()});
    auto facts = authority();
    auto deny = adjust_scope();
    deny.quantities[0].mode = Mode::closed_interval;
    deny.quantities[0].lower = PermissionRational{"0", "1"};
    set_link(facts,
             {policy_row(adjust_scope({"A"}, "600000"), "role-allow", Effect::allow, "editor"),
              policy_row(deny, "role-deny", Effect::deny, "restricted")},
             {"editor", "restricted"});
    expect(adjustment("deny-at-upper"), Disposition::required_action_denied, "add", 0, facts);
    const auto in = adjustment("above-deny", "A", "300001");
    expect(in, Disposition::admitted, "add", 1, facts);

    facts.snapshot = "p2";
    set_link(facts);
    expect(in, Disposition::admitted, "add", 2, facts, true);
    expect(adjustment("revoked", "A", "300001"), Disposition::required_action_denied, "add", 2,
           facts);
}

TEST_F(ChatCommands, DelegatedScopeRequiresExactOperationResourceAndQuantity) {
    auto cmd = add();
    cmd.route = Route::delegated_category;
    auto other = cmd;
    other.command = "other";
    other.aliases = {"!other"};
    other.actions[0].operation = "other-adjust";
    start({cmd, other});
    auto facts = authority();
    auto definition = facts.catalog.operations[0];
    definition.operation = "other-adjust";
    facts.catalog.operations.push_back(definition);
    facts.category_grants = {{Category::vip, adjust_scope()},
                             {Category::subscriber, adjust_scope({"B"}, "600000")}};
    // The unselected linked route supplies both a mismatch and an explicit deny.
    set_link(facts, {policy_row(adjust_scope(), "unselected", Effect::deny)});
    facts.link->twitch_user = "7";

    auto in = adjustment("vip");
    fact(in, Category::vip, Fact::present);
    fact(in, Category::subscriber, Fact::present);
    expect(in, Disposition::admitted, "add", 0, facts);
    in.message.opaque_id = "over-vip";
    in.argument_tokens[1] = "300001";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    in.message.opaque_id = "no-subscriber-fallback";
    in.argument_tokens[1] = "600000";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    in.message.opaque_id = "subscriber";
    in.argument_tokens[0] = "B";
    expect(in, Disposition::admitted, "add", 0, facts);
    in.message.opaque_id = "wrong-operation";
    in.alias = "!other";
    expect(in, Disposition::required_action_denied, "other", 0, facts);

    facts.category_grants = {{Category::vip,
                              {"adjust",
                               {{"timer", PermissionResourceMode::any, {}}},
                               {{"delta", "ms", Mode::any, {}, {}}}}}};
    in = adjustment("any-resource-and-quantity", "another-timer", "-600001");
    fact(in, Category::vip, Fact::present);
    expect(in, Disposition::admitted, "add", 0, facts);
}

TEST_F(ChatCommands, UnknownCategoryMattersOnlyForAFittingGrantWithoutAPresentWitness) {
    auto cmd = add();
    cmd.route = Route::delegated_category;
    start({cmd});
    auto facts = authority();
    facts.category_grants = {{Category::vip, adjust_scope()}};
    auto in = adjustment("unknown-vip");
    fact(in, Category::moderator, Fact::present);
    expect(in, Disposition::authority_unavailable, "add", 0, facts);

    in.message.opaque_id = "known-permit";
    facts.category_grants.push_back({Category::moderator, adjust_scope()});
    expect(in, Disposition::admitted, "add", 0, facts);

    in.message.opaque_id = "unknown-but-out-of-scope";
    facts.category_grants = {{Category::vip, adjust_scope({"B"})}};
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    in.message.opaque_id = "fitting-but-absent";
    fact(in, Category::vip, Fact::absent);
    facts.category_grants = {{Category::vip, adjust_scope()}};
    expect(in, Disposition::required_action_denied, "add", 0, facts);
}

TEST_F(ChatCommands, CompleteMoveWitnessCannotBlendResourceOrQuantityScopes) {
    auto cmd = add();
    cmd.command = "move";
    cmd.aliases = {"!move"};
    cmd.route = Route::both;
    cmd.arguments = {{"from", Type::resource}, {"to", Type::resource}, {"delta", Type::integer}};
    cmd.actions[0].operation = "move";
    cmd.actions[0].resources = {{"source", ChatArgumentReference{"from"}},
                                {"destination", ChatArgumentReference{"to"}}};
    start({cmd});

    auto facts = authority();
    facts.catalog.operations.push_back({"move",
                                        PermissionOperationClass::ordinary,
                                        {"source", "destination"},
                                        {{"delta", "ms", Number::integer, {}, {}, 15}},
                                        {}});
    const DelegatedPermissionGrant narrow{
        "move", {resources("source", {"A"}), resources("destination", {"B"})}, {cap("300000")}};
    const DelegatedPermissionGrant wide{
        "move", {resources("source", {"C"}), resources("destination", {"D"})}, {cap("600000")}};
    facts.category_grants = {{Category::vip, narrow}, {Category::moderator, wide}};
    auto in = message("split-resources", "!move", "007", {"A", "D", "300000"});
    fact(in, Category::vip, Fact::present);
    fact(in, Category::moderator, Fact::present);
    expect(in, Disposition::required_action_denied, "move", 0, facts);

    in.message.opaque_id = "split-quantity";
    in.argument_tokens = {"A", "B", "600000"};
    expect(in, Disposition::required_action_denied, "move", 0, facts);
    in.message.opaque_id = "complete";
    in.argument_tokens[2] = "300000";
    const auto result = expect(in, Disposition::admitted, "move", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 1U);
    expect_action(
        result.proposal->actions[0],
        {"move", {{"source", "A"}, {"destination", "B"}}, {{"delta", "ms", {"300000", "1"}}}});

    // A linked individual limit is a nonpermit without a veto. Neither partial
    // route fits A->B/600000; the very same category facts do fully permit C->D.
    set_link(facts, {policy_row(narrow, "linked-narrow")});
    facts.category_grants.erase(facts.category_grants.begin());
    in.message.opaque_id = "split-routes";
    in.argument_tokens[2] = "600000";
    expect(in, Disposition::required_action_denied, "move", 0, facts);
    in.message.opaque_id = "complete-category-route";
    in.argument_tokens = {"C", "D", "600000"};
    expect(in, Disposition::admitted, "move", 0, facts);

    // The same category can hold two rows without merging their constraints.
    facts.category_grants = {{Category::vip, narrow}, {Category::vip, wide}};
    in.message.opaque_id = "split-same-category";
    in.argument_tokens = {"A", "D", "300000"};
    expect(in, Disposition::required_action_denied, "move", 0, facts);
}

TEST_F(ChatCommands, RationalAndSignedMagnitudeLimitsCompareWithoutRoundingOrOverflow) {
    auto cmd = add(Type::rational);
    cmd.route = Route::delegated_category;
    start({cmd});
    auto facts = authority(Number::rational);
    facts.category_grants = {{Category::vip, adjust_scope()}};
    auto in = adjustment("half-over", "A", "600001/2");
    fact(in, Category::vip, Fact::present);
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    in.message.opaque_id = "half-under";
    in.argument_tokens[1] = "599999/2";
    expect(in, Disposition::admitted, "add", 0, facts);

    facts.category_grants[0].grant.quantities[0] = {"delta", "ms", Mode::closed_interval,
                                                    PermissionRational{"-7", "2"},
                                                    PermissionRational{"300000", "1"}};
    in.message.opaque_id = "negative-endpoint";
    in.argument_tokens[1] = "-7/2";
    expect(in, Disposition::admitted, "add", 0, facts);
    in.message.opaque_id = "below-negative-endpoint";
    in.argument_tokens[1] = "-15/4";
    expect(in, Disposition::required_action_denied, "add", 0, facts);

    cmd.arguments[1].type = Type::integer;
    start({cmd}, {}, "r2");
    facts = authority();
    auto &domain = facts.catalog.operations[0].quantity_slots[0];
    domain.minimum = PermissionRational{"-9223372036854775808", "1"};
    domain.maximum = PermissionRational{"9223372036854775807", "1"};
    auto grant = adjust_scope();
    grant.quantities[0] = cap("9223372036854775808", Mode::magnitude_at_most);
    facts.category_grants = {{Category::vip, grant}};

    in.message.opaque_id = "signed-minimum";
    in.argument_tokens[1] = "-9223372036854775808";
    const auto result = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 1U);
    expect_action(result.proposal->actions[0],
                  {"adjust", {{"timer", "A"}}, {{"delta", "ms", {"-9223372036854775808", "1"}}}});
    facts.category_grants[0].grant.quantities[0].upper =
        PermissionRational{"9223372036854775807", "1"};
    in.message.opaque_id = "beyond-magnitude";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    in.message.opaque_id = "beyond-domain";
    in.argument_tokens[1] = "-9223372036854775809";
    expect(in, Disposition::invalid_arguments, "add", 0, facts);
}

TEST_F(ChatCommands, BothRoutesAcceptEitherCompletePermitAndHonorOnlyPrevailingDenies) {
    auto cmd = add();
    cmd.route = Route::both;
    start({cmd});
    auto facts = authority();
    auto in = adjustment("linked-only");
    fact(in, Category::vip, Fact::present);
    set_link(facts, {policy_row(adjust_scope(), "allow")});
    expect(in, Disposition::admitted, "add", 0, facts);

    facts.category_grants = {{Category::vip, adjust_scope()}};
    in.message.opaque_id = "both-permit";
    expect(in, Disposition::admitted, "add", 0, facts);
    set_link(facts);
    in.message.opaque_id = "category-with-default-deny";
    expect(in, Disposition::admitted, "add", 0, facts);
    facts.link.reset();
    in.message.opaque_id = "category-with-absent-link";
    expect(in, Disposition::admitted, "add", 0, facts);
    set_link(facts, {policy_row(adjust_scope({"A"}, "299999"), "narrow")});
    in.message.opaque_id = "category-with-individual-limit";
    expect(in, Disposition::admitted, "add", 0, facts);

    set_link(facts, {policy_row(adjust_scope(), "block", Effect::deny)});
    in.message.opaque_id = "individual-veto";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    const auto role_deny = policy_row(adjust_scope(), "role-block", Effect::deny, "restricted");
    set_link(facts, {role_deny}, {"restricted"});
    in.message.opaque_id = "role-veto";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    set_link(facts, {role_deny, policy_row(adjust_scope(), "individual-allow")}, {"restricted"});
    in.message.opaque_id = "suppressed-role-deny";
    expect(in, Disposition::admitted, "add", 0, facts);
}

TEST_F(ChatCommands, BothRoutesDistinguishNonpermitsUnknownsAndVetoes) {
    auto cmd = add();
    cmd.route = Route::both;
    start({cmd});
    auto facts = authority();
    auto in = adjustment("both-deny");
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    facts.category_grants_state = State::unknown;
    in.message.opaque_id = "category-unavailable";
    expect(in, Disposition::authority_unavailable, "add", 0, facts);
    set_link(facts, {policy_row(adjust_scope(), "allow")});
    in.message.opaque_id = "linked-permit-category-unavailable";
    expect(in, Disposition::admitted, "add", 0, facts);
    set_link(facts, {policy_row(adjust_scope(), "block", Effect::deny)});
    in.message.opaque_id = "veto-category-unavailable";
    expect(in, Disposition::required_action_denied, "add", 0, facts);

    facts.category_grants_state = State::complete;
    facts.link_state = State::unknown;
    facts.link.reset();
    in.message.opaque_id = "link-unavailable";
    expect(in, Disposition::authority_unavailable, "add", 0, facts);
    facts.category_grants = {{Category::vip, adjust_scope()}};
    fact(in, Category::vip, Fact::present);
    in.message.opaque_id = "category-permit-link-unavailable";
    expect(in, Disposition::admitted, "add", 0, facts);
}

TEST_F(ChatCommands, RequiredActionsAreAtomicAndOnlyAdmissionConsumesCooldowns) {
    auto cmd = add();
    cmd.actions.push_back(publish(true));
    cmd.response = ChatResponseTemplate{Sender::bot, {std::string("done")}};
    cmd.global_cooldown_ms = 500;
    cmd.user_cooldown_ms = 500;
    start({cmd});
    auto facts = authority();
    set_link(facts);
    facts.link->use = {};
    expect(adjustment("unknown"), Disposition::authority_unavailable, "add", 99, facts);
    set_link(facts, {policy_row(adjust_scope(), "adjust")});
    expect(adjustment("half-permitted"), Disposition::required_action_denied, "add", 100, facts);
    set_link(facts, {policy_row(adjust_scope(), "adjust"), policy_row(publish_scope(), "publish")});
    const auto result =
        expect(adjustment("all-required"), Disposition::admitted, "add", 101, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 2U);
    expect_action(result.proposal->actions[0],
                  {"adjust", {{"timer", "A"}}, {{"delta", "ms", {"300000", "1"}}}});
    expect_action(result.proposal->actions[1], {"publish", {{"scene", "scene-X"}}, {}});
    expect_response(result, Sender::bot, "done");
    expect(adjustment("at-admission-time"), Disposition::both_cooldowns, "add", 101, facts);
    facts.snapshot = "p2";
    set_link(facts, {policy_row(adjust_scope(), "adjust")});
    expect(adjustment("required-revoked-during-cooldown"), Disposition::required_action_denied,
           "add", 101, facts);
}

TEST_F(ChatCommands, RequiredDenialWinsOverUnavailabilityInEitherOrder) {
    auto cmd = add();
    cmd.route = Route::delegated_category;
    cmd.actions.push_back(publish(true));
    auto facts = authority();
    facts.category_grants = {{Category::vip, adjust_scope()}};
    // The fitting VIP grant is unknown; the complete set has no publish grant.
    start({cmd});
    expect(adjustment("unknown-first"), Disposition::required_action_denied, "add", 0, facts);
    std::ranges::reverse(cmd.actions);
    start({cmd}, {}, "r2");
    expect(adjustment("unknown-second"), Disposition::required_action_denied, "add", 0, facts);
}

TEST_F(ChatCommands, ProtectedOperationCannotBeAuthorizedAsAnOrdinaryAction) {
    auto cmd = add();
    cmd.route = Route::both;
    start({cmd});
    auto facts = authority();
    facts.category_grants = {{Category::vip, adjust_scope()}};
    set_link(facts, {policy_row(adjust_scope(), "allow")});
    facts.owner = {PermissionOwnerState::present, "acct-U"};
    auto in = adjustment("ordinary");
    fact(in, Category::vip, Fact::present);
    expect(in, Disposition::admitted, "add", 0, facts);

    facts.catalog.revision = "protected-fixture";
    facts.catalog.operations[0].classification =
        PermissionOperationClass::protected_access_management;
    in.message.opaque_id = "protected";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
}

TEST_F(ChatCommands, LinkedOnlyOptionalDenyOmitsThatActionAndKeepsTheResponse) {
    auto cmd = add();
    cmd.actions.push_back(publish(false));
    cmd.response = ChatResponseTemplate{Sender::configured_default, {std::string("done")}};
    start({cmd});
    auto facts = authority();
    auto in = adjustment("optional-without-grant");
    fact(in, Category::vip, Fact::present);
    facts.category_grants = {{Category::vip, publish_scope()}};
    set_link(facts, {policy_row(adjust_scope(), "required")});
    const PermissionRequest required{
        "adjust", {{"timer", "A"}}, {{"delta", "ms", {"300000", "1"}}}};
    auto result = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 1U);
    expect_action(result.proposal->actions.front(), required);
    expect_response(result, Sender::configured_default, "done");

    set_link(facts, {policy_row(adjust_scope(), "required"),
                     policy_row(publish_scope(), "optional-deny", Effect::deny)});
    in.message.opaque_id = "optional-explicit-deny";
    result = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 1U);
    expect_action(result.proposal->actions.front(), required);
    expect_response(result, Sender::configured_default, "done");

    set_link(facts,
             {policy_row(adjust_scope(), "required"), policy_row(publish_scope(), "optional")});
    in.message.opaque_id = "optional-permitted";
    result = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 2U);
    expect_action(result.proposal->actions[0], required);
    expect_action(result.proposal->actions[1], {"publish", {{"scene", "scene-X"}}, {}});
}

TEST_F(ChatCommands, CombinedOptionalVetoRefusesEvenWhenRequiredAuthorityIsUnavailable) {
    auto cmd = add();
    cmd.route = Route::both;
    cmd.actions.push_back(publish(false));
    cmd.response = ChatResponseTemplate{Sender::bot, {std::string("done")}};
    start({cmd});
    auto facts = authority();
    facts.category_grants = {{Category::vip, adjust_scope()}, {Category::vip, publish_scope()}};
    const auto optional_veto = policy_row(publish_scope(), "optional-veto", Effect::deny);
    set_link(facts, {policy_row(adjust_scope(), "required"), optional_veto});
    auto in = adjustment("optional-category-permit-vetoed");
    fact(in, Category::vip, Fact::present);
    expect(in, Disposition::required_action_denied, "add", 0, facts);

    facts.category_grants.clear();
    facts.category_grants_state = State::unknown;
    set_link(facts, {optional_veto});
    in.message.opaque_id = "optional-veto-after-required-unavailable";
    expect(in, Disposition::required_action_denied, "add", 0, facts);
    set_link(facts, {policy_row(adjust_scope(), "required")});
    in.message.opaque_id = "optional-unavailable-omitted";
    const auto result = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 1U);
    expect_action(result.proposal->actions[0],
                  {"adjust", {{"timer", "A"}}, {{"delta", "ms", {"300000", "1"}}}});
    expect_response(result, Sender::bot, "done");
}

TEST_F(ChatCommands, MalformedSelectedPolicyIsAnUnavailableTerminalAndReplays) {
    start({add()});
    auto facts = authority();
    auto malformed = policy_row(adjust_scope(), "malformed");
    malformed.quantities[0].unit = "seconds";
    set_link(facts, {malformed});
    const auto in = adjustment("malformed-policy");
    expect(in, Disposition::authority_unavailable, "add", 10, facts);
    facts.snapshot = "p2";
    set_link(facts, {policy_row(adjust_scope(), "valid")});
    expect(in, Disposition::authority_unavailable, "add", 90, facts, true);
    expect(adjustment("new-policy"), Disposition::admitted, "add", 10, facts);
}

TEST_F(ChatCommands, CooldownWindowsShareAliasesAndOpenAtExactElapsedThresholds) {
    start({wave(100, 250)});
    const auto original = message("m01");
    expect_response(expect(original, Disposition::admitted, "wave", 1000),
                    Sender::configured_default, "wave");
    expect(message("m02", "!w"), Disposition::both_cooldowns, "wave", 1099);
    const auto denied = message("m03");
    expect(denied, Disposition::user_cooldown, "wave", 1100);
    expect(message("m04", "!w", "7"), Disposition::admitted, "wave", 1100);
    expect(message("m05", "!wave", "008"), Disposition::global_cooldown, "wave", 1199);
    expect(message("m06", "!w"), Disposition::user_cooldown, "wave", 1249);
    expect(message("m07"), Disposition::admitted, "wave", 1250);

    expect(message("m01", "!unknown", "", {"unused"}), Disposition::admitted, "wave", 2000,
           ignored_authority(), true);
    expect(message("m03", "!unknown", "7"), Disposition::user_cooldown, "wave", 2000,
           ignored_authority(), true);
    expect(message("m08", "!w", "7"), Disposition::admitted, "wave", 2000);
}

TEST_F(ChatCommands, UserWindowsUseExactTwitchIdsAcrossAccountChangesAndSeparateCommands) {
    auto cmd = add();
    cmd.user_cooldown_ms = 250;
    auto second = cmd;
    second.command = "other";
    second.aliases = {"!other"};
    start({cmd, second});
    auto facts = authority();
    set_link(facts, {policy_row(adjust_scope(), "use")});
    expect(adjustment("first"), Disposition::admitted, "add", 1000, facts);

    facts.link->twitch_user = "7";
    // Both Twitch IDs are supplied with the same application account and USE.
    auto in = message("second-user", "!plus", "7", {"A", "300000"});
    expect(in, Disposition::admitted, "add", 1000, facts);
    facts.link->twitch_user = "007";
    facts.link->account = "acct-relinked";
    facts.snapshot = "p2";
    in = message("relinked", "!plus", "007", {"A", "300000"});
    expect(in, Disposition::user_cooldown, "add", 1249, facts);
    in.message.opaque_id = "other-command";
    in.alias = "!other";
    expect(in, Disposition::admitted, "other", 1249, facts);
    expect(adjustment("opens"), Disposition::admitted, "add", 1250, facts);
}

TEST_F(ChatCommands, ReplayDoesNotAdvanceOrRegressClockAndFreshDenialsSampleTime) {
    start({add()});
    auto facts = authority();
    const auto original = adjustment("original-denial");
    expect(original, Disposition::required_action_denied, "add", 1000, facts);
    facts.snapshot = "p2";
    set_link(facts, {policy_row(adjust_scope(), "use")});
    expect(original, Disposition::required_action_denied, "add", 1500, facts, true);
    expect(adjustment("now-permitted"), Disposition::admitted, "add", 1100, facts);
    expect(original, Disposition::required_action_denied, "add", 999, facts, true);

    const auto fresh = adjustment("fresh-at-bound");
    rejects(fresh, Error::clock_regressed, 1099, facts);
    expect(fresh, Disposition::admitted, "add", 1100, facts);
    expect(message("unknown", "!unknown"), Disposition::unrecognized_alias, std::nullopt, 1200);
    rejects(adjustment("following-denial"), Error::clock_regressed, 1199, facts);
}

TEST_F(ChatCommands, GlobalElapsedArithmeticDoesNotOverflowAtMaximumClockSample) {
    const auto last = std::numeric_limits<std::uint64_t>::max();
    auto no_windows = wave();
    no_windows.command = "disabled";
    no_windows.aliases = {"!disabled"};
    start({wave(10), no_windows});
    expect(message("starts-near-max"), Disposition::admitted, "wave", last - 5);
    expect(message("blocked-at-max", "!w", "7"), Disposition::global_cooldown, "wave", last);
    expect(message("disabled-at-max", "!disabled"), Disposition::admitted, "disabled", last);
    expect(message("disabled-still-open", "!disabled"), Disposition::admitted, "disabled", last);
    rejects(message("lower-fresh"), Error::clock_regressed, last - 1);
}

TEST_F(ChatCommands, ExactMessageIdentityIncludesOpaqueBytesNamespaceAndSource) {
    start({wave()});
    const auto original = message("m01");
    expect(original, Disposition::admitted);
    expect(message("M01"), Disposition::admitted);
    auto in = original;
    in.message.identifier_namespace = "fake-chat-2";
    expect(in, Disposition::admitted);
    expect(message(std::string(1, '\0')), Disposition::admitted);
    expect(message(std::string(1, '\0')), Disposition::admitted, "wave", 0, ignored_authority(),
           true);

    in.message.identifier_namespace = "ab";
    in.message.opaque_id = "c";
    expect(in, Disposition::admitted);
    in.message.identifier_namespace = "a";
    in.message.opaque_id = "bc";
    expect(in, Disposition::admitted);

    in = original;
    in.message.origin.source_account = "chan-B";
    in.twitch_user.clear();
    rejects(in, Error::invalid_identity);
    in = original;
    in.message.identifier_namespace.clear();
    rejects(in, Error::invalid_identity);
    rejects(message(""), Error::invalid_identity);

    in = original;
    in.message.origin.source_account = "chan-B";
    // Keep the chan-A evaluator alive while addressing the distinct source owner.
    const ChatCommandRevision second{in.message.origin, "r1", {wave()}};
    ChatCommandEvaluator separate(second);
    auto facts = ignored_authority();
    facts.source = second.source;
    const auto result = separate.evaluate(in, facts, 0);
    EXPECT_EQ(result.message, in.message);
    EXPECT_EQ(result.revision, "r1");
    EXPECT_EQ(result.terminal.disposition, Disposition::admitted);
    EXPECT_EQ(result.terminal.command, "wave");
    EXPECT_FALSE(result.replay);
    expect_response(result, Sender::configured_default, "wave");
    expect(original, Disposition::admitted, "wave", 0, ignored_authority(), true);
}

TEST_F(ChatCommands, SerializedCopiesProposeOnceAndReplayBothKindsOfInputDenial) {
    auto cmd = wave(100);
    cmd.arguments = {{"word", Type::text}};
    start({cmd});
    auto in = message("bad-args");
    expect(in, Disposition::invalid_arguments);
    in.argument_tokens = {"fixed"};
    expect(in, Disposition::invalid_arguments, "wave", 0, ignored_authority(), true);

    in = message("unrecognized", "!unknown");
    expect(in, Disposition::unrecognized_alias, std::nullopt);
    in.alias = "!wave";
    in.argument_tokens = {"fixed"};
    expect(in, Disposition::unrecognized_alias, std::nullopt, 0, ignored_authority(), true);

    in.message.opaque_id = "admitted";
    expect(in, Disposition::admitted);
    expect(in, Disposition::admitted, "wave", 0, ignored_authority(), true);
    in.message.opaque_id = "contender";
    in.twitch_user = "7";
    expect(in, Disposition::global_cooldown);
}

TEST_F(ChatCommands, FifoReplayEvictionLeavesBothLiveWindowsAndDoesNotRefreshOnRead) {
    ChatCommandLimits limits;
    limits.terminal_entries = 2;
    start({wave(50, 250)}, limits);
    const auto original = message("m01");
    expect(original, Disposition::admitted, "wave", 100);
    expect(message("m02", "!unknown"), Disposition::unrecognized_alias, std::nullopt, 101);
    expect(message("m03", "!unknown"), Disposition::unrecognized_alias, std::nullopt, 102);
    expect(message("m02"), Disposition::unrecognized_alias, std::nullopt, 103, ignored_authority(),
           true);
    expect(original, Disposition::both_cooldowns, "wave", 149);
    expect(message("m02", "!wave", "7"), Disposition::global_cooldown, "wave", 149);
    expect(message("m04"), Disposition::user_cooldown, "wave", 150);
    expect(message("m05"), Disposition::admitted, "wave", 350);
}

TEST_F(ChatCommands, UserCapacityReclaimsAtEqualityAndFailurePublishesNoPartialState) {
    ChatCommandLimits limits;
    limits.terminal_entries = 3;
    limits.active_user_entries = 1;
    start({wave(10, 100)}, limits);
    const auto first = message("U");
    const auto second = message("V", "!w", "7");
    expect(first, Disposition::admitted);
    expect(second, Disposition::global_cooldown, "wave", 9);
    auto contender = second;
    contender.message.opaque_id = "V-after-global";
    rejects(contender, Error::user_cooldown_capacity, 99);
    // If the capacity error published time, a terminal, or a global start, one
    // of the following observations would regress, lose the old replay, or block.
    expect(message("U-98"), Disposition::user_cooldown, "wave", 98);
    expect(first, Disposition::admitted, "wave", 99, ignored_authority(), true);
    expect(contender, Disposition::admitted, "wave", 100);

    auto other = wave(0, 100);
    other.command = "other";
    other.aliases = {"!other"};
    start({wave(0, 100), other}, limits, "r2");
    expect(first, Disposition::admitted);
    const auto same_user_other_command = message("same-user-other-command", "!other");
    rejects(same_user_other_command, Error::user_cooldown_capacity, 99);
    expect(same_user_other_command, Disposition::admitted, "other", 100);
}

TEST_F(ChatCommands, RevisionReplacementAndFreshOwnerResetReplayCooldownsAndSample) {
    const auto cmd = wave(100, 250);
    const auto key = message("same-id");
    start({cmd});
    expect(key, Disposition::admitted, "wave", 100);
    expect(key, Disposition::admitted, "wave", 100, ignored_authority(), true);
    expect(message("other-id"), Disposition::both_cooldowns, "wave", 100);

    start({cmd}, {}, "r2");
    expect(key, Disposition::admitted, "wave", 0);
    expect(key, Disposition::admitted, "wave", 0, ignored_authority(), true);
    expect(message("other-id"), Disposition::both_cooldowns, "wave", 0);

    start({cmd}, {}, "r2");
    expect(key, Disposition::admitted, "wave", 0);
    expect(message("other-id"), Disposition::both_cooldowns, "wave", 0);
}

ChatCommandDefinition tiny() {
    auto cmd = wave();
    cmd.command = "tiny";
    cmd.aliases = {"!t"};
    cmd.arguments = {{"x", Type::text}, {"y", Type::text}};
    cmd.response =
        ChatResponseTemplate{Sender::bot, {ChatArgumentReference{"x"}, ChatArgumentReference{"x"}}};
    return cmd;
}

TEST_F(ChatCommands, InputBytesIncludeUnusedArgumentAndIgnoredAuthorityBeforeEnvelopeSyntax) {
    ChatCommandLimits limits;
    // Config: 14 source+revision, 4 command, 2 alias, 4 arguments, 1 absence
    // tag, 1 route, 6 response (presence, sender, two tagged "x" references),
    // and 16 cooldown bytes = 48. Evaluation: 33 envelope fixed, 2 tokens,
    // 18 default authority, 7 ignored snapshot and 8 clock = 68.
    limits.input_bytes = 68;
    start({tiny()}, limits);
    auto facts = ignored_authority();
    facts.snapshot = "ignored";
    const auto in = message("m1", "!t", "007", {"a", "b"});
    expect_response(expect(in, Disposition::admitted, "tiny", 0, facts), Sender::bot, "aa");

    auto oversized = in;
    oversized.message.opaque_id = "m2";
    oversized.alias = "! ";
    oversized.argument_tokens[1] = "bc";
    rejects(oversized, Error::input_limit, 100, facts);
    oversized.argument_tokens[1] = "b";
    rejects(oversized, Error::invalid_envelope, 100, facts);

    oversized.alias = "!t";
    expect(oversized, Disposition::admitted, "tiny", 0, facts);
}

TEST_F(ChatCommands, InputItemLimitCountsIgnoredNestedAuthorityEntries) {
    ChatCommandLimits limits;
    limits.input_items = 6;
    // Config uses 1 command + 1 alias + 2 arguments + 2 template parts.
    start({tiny()}, limits);
    auto facts = authority();
    facts.catalog = {};
    facts.link_state = State::unknown;
    facts.category_grants = {{Category::vip, publish_scope()}};
    facts.category_grants[0].grant.resources[0].allowed.push_back("scene-Y");
    // Evaluation uses 2 tokens + 1 grant + 1 resource constraint + 2 members.
    auto in = message("m1", "!t", "007", {"a", "b"});
    expect(in, Disposition::admitted, "tiny", 0, facts);
    facts.category_grants[0].grant.resources[0].allowed.push_back("scene-Z");
    in.message.opaque_id = "m2";
    rejects(in, Error::input_limit, 1, facts);
    facts.category_grants[0].grant.resources[0].allowed.pop_back();
    expect(in, Disposition::admitted, "tiny", 0, facts);
}

TEST_F(ChatCommands, ReplayLookupPrecedesInputBudgetEnvelopeSyntaxAndClock) {
    ChatCommandLimits limits;
    limits.input_bytes = 128;
    limits.permission.reference_bytes = 16;
    start({wave()}, limits);
    auto in = message("m1");
    expect(in, Disposition::admitted, "wave", 100);
    in.alias = "! ";
    in.twitch_user.clear();
    in.argument_tokens = {std::string(256, 'x')};
    fact(in, Category::vip, static_cast<Fact>(255));
    expect(in, Disposition::admitted, "wave", 99, ignored_authority(), true);

    in.message.opaque_id = std::string(17, 'k');
    rejects(in, Error::invalid_identity, 99);
    in.message.opaque_id = std::string(16, 'k');
    rejects(in, Error::input_limit, 99);
    in.argument_tokens.clear();
    rejects(in, Error::invalid_envelope, 99);
    in.alias = "!wave";
    rejects(in, Error::invalid_envelope, 99);
    in.twitch_user = "007";
    rejects(in, Error::invalid_envelope, 99);
    fact(in, Category::vip, Fact::unknown);
    rejects(in, Error::clock_regressed, 99);
    expect(in, Disposition::admitted, "wave", 100);
}

TEST_F(ChatCommands, ResultByteLimitCountsEverySubstitutionWithoutPublishingOnFailure) {
    auto cmd = tiny();
    cmd.global_cooldown_ms = 10;
    cmd.user_cooldown_ms = 10;
    ChatCommandLimits limits;
    limits.terminal_entries = 1;
    // 23 message key + 2 revision + 1 disposition + 5 present command
    // + 1 replay + 1 present proposal + 2 present response/sender
    // + 4 bytes ("ab" twice) = 39.
    limits.result_bytes = 39;
    start({cmd}, limits);

    const auto denied = message("m0", "!unknown");
    expect(denied, Disposition::unrecognized_alias, std::nullopt, 99);
    const auto wide = message("m1", "!t", "007", {"abc", ""});
    rejects(wide, Error::result_limit, 100);
    expect(denied, Disposition::unrecognized_alias, std::nullopt, 100, ignored_authority(), true);
    const auto narrow = message("m2", "!t", "007", {"ab", ""});
    expect_response(expect(narrow, Disposition::admitted, "tiny", 100), Sender::bot, "abab");
    const auto corrected = message("m1", "!t", "007", {"ab", ""});
    expect(corrected, Disposition::admitted, "tiny", 110);

    start({cmd}, limits, "r2");
    const auto one_byte_over = message("m12", "!t", "007", {"ab", ""});
    rejects(one_byte_over, Error::result_limit, 100);
    expect(narrow, Disposition::admitted, "tiny", 100);
}

TEST_F(ChatCommands, ResultItemLimitCountsRequestsAndEveryBinding) {
    auto cmd = add();
    cmd.actions.push_back(publish(false));
    ChatCommandLimits limits;
    // Two action entries + two resources + one quantity = five items.
    limits.result_items = 5;
    auto facts = authority();
    set_link(facts,
             {policy_row(adjust_scope(), "required"), policy_row(publish_scope(), "optional")});
    const auto in = adjustment("m1");
    start({cmd}, limits);
    const auto result = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(result.proposal);
    ASSERT_EQ(result.proposal->actions.size(), 2U);

    limits.result_items = 4;
    start({cmd}, limits, "r2");
    rejects(in, Error::result_limit, 1, facts);
    set_link(facts, {policy_row(adjust_scope(), "required")});
    const auto smaller = expect(in, Disposition::admitted, "add", 0, facts);
    ASSERT_TRUE(smaller.proposal);
    ASSERT_EQ(smaller.proposal->actions.size(), 1U);
    expect_action(smaller.proposal->actions[0],
                  {"adjust", {{"timer", "A"}}, {{"delta", "ms", {"300000", "1"}}}});
}

TEST_F(ChatCommands, EligibilityDepthCountsTheRootAndRefusesTheFirstExcessLevel) {
    auto cmd = wave();
    cmd.eligibility = ChatEligibility{Group::all, {{Group::any, {{ChatExplicitUser{"007"}, {}}}}}};
    ChatCommandLimits limits;
    limits.eligibility_depth = 3;
    start({cmd}, limits);
    expect(message("depth-three"), Disposition::admitted);

    limits.eligibility_depth = 2;
    expect_failure(Error::input_limit, [&] { start({cmd}, limits, "r2"); });
}

TEST(ChatCommandConstruction, RejectsInvalidLimitsAndUnrepresentableCapacityProducts) {
    ChatCommandLimits limits;
    const ChatCommandRevision revision{source, "r1", {wave()}};
    limits.active_user_entries = 0;
    expect_failure(Error::invalid_limits, [&] { ChatCommandEvaluator invalid(revision, limits); });
    limits.active_user_entries = 1;
    limits.terminal_entries = std::numeric_limits<std::uint64_t>::max();
    expect_failure(Error::invalid_limits, [&] { ChatCommandEvaluator invalid(revision, limits); });
}

TEST(ChatCommandConstruction, RejectsBadTagsRegistrationsAndTemplateReferences) {
    const auto rejects_revision = [](std::vector<ChatCommandDefinition> commands) {
        const ChatCommandRevision revision{source, "r1", std::move(commands)};
        expect_failure(Error::invalid_revision, [&] { ChatCommandEvaluator invalid(revision); });
    };
    {
        SCOPED_TRACE("An alias collides across commands");
        auto other = wave();
        other.command = "other";
        rejects_revision({wave(), other});
    }
    {
        SCOPED_TRACE("An argument is declared twice");
        auto cmd = add();
        cmd.arguments.push_back({"timer", Type::resource});
        rejects_revision({cmd});
    }
    {
        SCOPED_TRACE("A resource slot binds an integer");
        auto cmd = add();
        cmd.actions[0].resources[0].value = ChatArgumentReference{"delta"};
        rejects_revision({cmd});
    }
    {
        SCOPED_TRACE("A response references an undeclared argument");
        auto cmd = tiny();
        cmd.response->parts[0] = ChatArgumentReference{"missing"};
        rejects_revision({cmd});
    }
    {
        SCOPED_TRACE("An eligibility group has no children");
        auto cmd = wave();
        cmd.eligibility = ChatEligibility{Group::any, {}};
        rejects_revision({cmd});
    }
    {
        SCOPED_TRACE("A command has an invalid route tag");
        auto cmd = wave();
        cmd.route = static_cast<Route>(255);
        rejects_revision({cmd});
    }
    {
        SCOPED_TRACE("An action-only command has no required action");
        auto cmd = add();
        cmd.actions[0].required = false;
        rejects_revision({cmd});
    }
    {
        SCOPED_TRACE("A static number is not reduced");
        auto cmd = add();
        cmd.actions[0].quantities[0].value = PermissionRational{"2", "4"};
        rejects_revision({cmd});
    }
}

TEST(ChatCommandPermissionControls, LinkedRowsDistinguishExplicitDenyFromAResourceFirstLimit) {
    auto facts = authority();
    const auto deny =
        policy_row(adjust_scope({"A"}, "600000"), "role-deny", Effect::deny, "restricted");
    set_link(facts, {deny, policy_row(adjust_scope(), "narrow")}, {"restricted"});
    EffectivePermissionInput use{
        facts.link->account, facts.snapshot,
        facts.catalog,       facts.owner,
        facts.link->use,     {"adjust", {{"timer", "A"}}, {{"delta", "ms", {"300000", "1"}}}}};
    auto decision = evaluate_effective_permission(use);
    EXPECT_EQ(decision.data().outcome, PermissionOutcome::permit);
    EXPECT_EQ(decision.data().reason, PermissionReason::individual_allow);

    use.request.quantities[0].value.numerator = "300001";
    decision = evaluate_effective_permission(use);
    EXPECT_EQ(decision.data().outcome, PermissionOutcome::deny);
    EXPECT_EQ(decision.data().reason, PermissionReason::individual_limit);

    use.policy.entries.pop_back();
    decision = evaluate_effective_permission(use);
    EXPECT_EQ(decision.data().outcome, PermissionOutcome::deny);
    EXPECT_EQ(decision.data().reason, PermissionReason::role_deny);
}

TEST(ChatCommandPermissionControls, SyntheticCategoryScopeRequiresOriginalAuthorUseAndGrant) {
    const auto facts = authority();
    const auto scope = adjust_scope();
    PermissionAutomationInput supplied{"original-author",
                                       facts.snapshot,
                                       facts.catalog,
                                       facts.owner,
                                       {State::complete, {}, {policy_row(scope, "author-use")}},
                                       {State::complete, {}, {policy_row(scope, "author-grant")}},
                                       {scope}};
    auto decision = revalidate_permission_automation(supplied);
    ASSERT_EQ(decision.actions.size(), 1U);
    EXPECT_EQ(decision.actions[0].use, PermissionOutcome::permit);
    EXPECT_EQ(decision.actions[0].grant, PermissionOutcome::permit);
    EXPECT_EQ(decision.actions[0].disposition, PermissionAutomationDisposition::authorized);

    supplied.snapshot = "p2";
    supplied.delegation.entries.clear();
    decision = revalidate_permission_automation(supplied);
    ASSERT_EQ(decision.actions.size(), 1U);
    EXPECT_EQ(decision.actions[0].use, PermissionOutcome::permit);
    EXPECT_EQ(decision.actions[0].grant, PermissionOutcome::deny);
    EXPECT_EQ(decision.actions[0].disposition, PermissionAutomationDisposition::hold);
}
} // namespace
