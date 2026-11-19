// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-007 #257, Tier A Stage 1 contract draft.
#pragma once

#include <subathon/core/delegated_permissions.hpp>
#include <subathon/core/receipt.hpp>

#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace subathon::core {

enum class TwitchChatCategory : std::uint8_t {
    broadcaster,
    moderator,
    vip,
    subscriber,
    unsubscribed_viewer
};
enum class TwitchChatFact : std::uint8_t { unknown, absent, present };

// Only synthetic inputs to this contract. A real producer binds the stable sender
// and all four ReceiptIdentity components before delivery (#337). Required opaque
// identities compare exact bytes, are nonempty and are never decoded as numbers.
// The revision's source must equal message.origin, with provider "twitch".
// Categories are current facts for this sender in that source's chat context.
// Each entry is independent, including subscriber and unsubscribed_viewer.
struct ChatCommandEnvelope {
    ReceiptIdentity message;
    std::string twitch_user;
    // Indexed by TwitchChatCategory in declaration order.
    std::array<TwitchChatFact, 5> categories{};
    std::string alias;
    std::vector<std::string> argument_tokens;
};

struct ChatExplicitUser {
    std::string twitch_user;
};
enum class ChatEligibilityGroup : std::uint8_t { all, any };
struct ChatEligibility {
    std::variant<TwitchChatCategory, ChatExplicitUser, ChatEligibilityGroup> term;
    // Groups require >=1 child. Leaves have zero. No rule means public eligibility.
    // all: false if any false, true iff all true, otherwise unknown.
    // any: true if any true, false iff all false, otherwise unknown.
    // Category absent/present means false/true. An explicit user compares the exact
    // Twitch ID. Only a true root passes, including when another any branch is unknown.
    std::vector<ChatEligibility> children;
};

enum class ChatArgumentType : std::uint8_t { text, resource, integer, rational };
struct ChatArgumentDefinition {
    std::string name;
    ChatArgumentType type;
};
struct ChatArgumentReference {
    std::string name;
};

// Bind every declared argument by position before evaluating any authority.
// Aliases are complete, case-sensitive command tokens (visible ASCII without
// spaces). No tokenization of raw chat frames here. All arguments are required
// and positional. Text preserves bytes, including empty tokens. A resource is a
// nonempty exact reference. Integers use canonical signed decimal and implicitly
// have denominator 1. Rationals accept the same integer syntax or n/d with a
// positive canonical decimal denominator and reduced terms. No plus, leading
// zeros, negative zero, decimal conversion, unit inference, or coercion.
// Resource slots bind only resource arguments. Quantity slots bind integer or
// rational arguments, using the configured unit and the operation's exact domain.
struct ChatActionResource {
    std::string slot;
    std::variant<std::string, ChatArgumentReference> value;
};
struct ChatActionQuantity {
    std::string slot;
    std::string unit;
    std::variant<PermissionRational, ChatArgumentReference> value;
};
struct ChatCommandAction {
    std::string operation;
    bool required = true;
    std::vector<ChatActionResource> resources;
    std::vector<ChatActionQuantity> quantities;
};

enum class ChatResponseSender : std::uint8_t { configured_default, bot, broadcaster };
struct ChatResponseTemplate {
    ChatResponseSender sender = ChatResponseSender::configured_default;
    // Ordered literal text and typed argument substitutions. Exact numbers use
    // canonical n or n/d (denominator 1 omitted). This is plain text, not a script.
    std::vector<std::variant<std::string, ChatArgumentReference>> parts;
};

enum class ChatManagementRoute : std::uint8_t { linked_account, delegated_category, both };
struct ChatCategoryGrant {
    TwitchChatCategory category;
    // One allow for one exact catalog operation and one complete resource/quantity
    // tuple. The producer checks current USE and GRANT for the original author
    // with revalidate_permission_automation. A complete set includes only currently
    // authorized grants. Unavailable revalidation produces unknown facts.
    DelegatedPermissionGrant grant;
};

struct ChatCommandDefinition {
    std::string command;
    // At least one alias. All aliases are unique across the source revision.
    std::vector<std::string> aliases;
    std::vector<ChatArgumentDefinition> arguments;
    std::optional<ChatEligibility> eligibility;
    ChatManagementRoute route = ChatManagementRoute::linked_account;
    // Response-only commands need neither account nor operation policy.
    // Commands with actions require at least one required action.
    std::vector<ChatCommandAction> actions;
    std::optional<ChatResponseTemplate> response;
    std::uint64_t global_cooldown_ms = 0;
    std::uint64_t user_cooldown_ms = 0;
};

struct ChatCommandRevision {
    ReceiptOrigin source;
    std::string revision;
    std::vector<ChatCommandDefinition> commands;
};

struct ChatLinkedAccount {
    std::string twitch_user;
    std::string account;
    PermissionPolicyFacts use;
};
struct ChatCommandAuthority {
    // One coherent current server-established snapshot for the envelope source.
    // complete + no link means known unlinked. unknown has no link payload.
    ReceiptOrigin source;
    PermissionFactsState link_state = PermissionFactsState::unknown;
    std::optional<ChatLinkedAccount> link;
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    PermissionFactsState category_grants_state = PermissionFactsState::unknown;
    std::vector<ChatCategoryGrant> category_grants;
};

enum class ChatCommandDisposition : std::uint8_t {
    admitted,
    unrecognized_alias,
    invalid_arguments,
    ineligible,
    eligibility_unavailable,
    required_action_denied,
    authority_unavailable,
    global_cooldown,
    user_cooldown,
    both_cooldowns
};

struct ChatCommandTerminal {
    ChatCommandDisposition disposition;
    std::optional<std::string> command;
};
struct ChatResponseProposal {
    ChatResponseSender sender;
    std::string text;
};
struct ChatCommandProposal {
    // Permitted actions in configuration order, with complete original requests.
    std::vector<PermissionRequest> actions;
    std::optional<ChatResponseProposal> response;
};
struct ChatCommandResult {
    ReceiptIdentity message;
    std::string revision;
    ChatCommandTerminal terminal;
    bool replay;
    // Present only on fresh admission. Inert proposals convey no execution rights.
    std::optional<ChatCommandProposal> proposal;
};

struct ChatCommandLimits {
    std::uint64_t input_bytes = 16777216;
    std::uint64_t input_items = 4096;
    std::uint64_t result_bytes = 1048576;
    std::uint64_t result_items = 4096;
    std::uint64_t eligibility_depth = 16;
    std::uint64_t terminal_entries = 4096;
    std::uint64_t active_user_entries = 4096;
    EffectivePermissionLimits permission;
};

enum class ChatCommandError : std::uint8_t {
    invalid_limits,
    invalid_revision,
    invalid_identity,
    invalid_envelope,
    input_limit,
    result_limit,
    clock_regressed,
    user_cooldown_capacity,
    allocation_failure
};
class ChatCommandFailure final : public std::exception {
  public:
    explicit ChatCommandFailure(ChatCommandError code) noexcept;
    [[nodiscard]] ChatCommandError code() const noexcept;
    [[nodiscard]] const char *what() const noexcept override;

  private:
    ChatCommandError code_;
};

// One serialized, non-reentrant owner per source with one active immutable command
// revision. Construction validates and owns a complete configuration; replacement
// by a new revision or process restart starts empty. Never recreate a lost cache
// for the same live revision or route to old parallel owners. Permission facts can
// change without changing the command revision. Pass one coherent current snapshot
// on each fresh evaluation. The application owns actual execution-time rechecks
// and operation guards, including outgoing sender authority and delivery policy.
// Reject invalid tags, duplicate command/alias/argument identities, bad eligibility
// trees, invalid alias tokens, missing outputs and unresolved or wrong-type template
// references at construction. Reject duplicate action slots and invalid static
// literals. The current operation catalog supplies the request shapes and domains
// at evaluation. Command, argument, slot and unit names compare exact bytes.
//
// Evaluation order:
// 1. Validate the entire message key (invalid_identity on bad source, empty field
//    or per-reference overflow), then look it up before considering other input.
//    A hit projects and returns the original terminal even if aliases, arguments,
//    sender, rights or clock inputs differ. Replay never returns a proposal or
//    advances time.
// 2. Preflight the complete supplied input budgets before checking syntax and
//    tags of the envelope. Over-budget input throws input_limit. An empty sender,
//    invalid category tag or malformed alias throws invalid_envelope. A sender
//    exceeding the reference bound throws input_limit. Then check monotonic_ms
//    against the last completed fresh evaluation (clock_regressed).
// 3. Resolve the alias. An unknown valid alias is terminal with no command. All
//    other terminals name the selected command. Bind every declared argument,
//    including unused ones, at its stated type. Wrong count or invalid grammar
//    is invalid_arguments. Evaluate eligibility next: false is ineligible and
//    unknown is eligibility_unavailable, without consulting operation authority.
// 4. For an eligible command with actions, verify the authority source. For any
//    selected linked route also verify that an asserted link names this sender.
//    A mismatch throws invalid_envelope. Check current catalog, all action slots
//    and units before validating any request value domains. Unknown or malformed
//    catalog or incompatible action definition yields authority_unavailable.
//    An out-of-domain bound argument yields invalid_arguments. A static literal
//    outside its current domain is an incompatible definition. Do these checks
//    for all configured actions before route authorization.
// 5. A nonordinary catalog operation is a definite action denial. For each
//    ordinary action, use the selected route. Linked account requires the exact
//    sender's current link, never another user's policy. Apply current effective
//    USE with resource-first individual selection and role-deny precedence.
//    Delegated category requires a present category on one current grant covering
//    the exact operation and EVERY resource and exact requested quantity together.
//    Each constraint uses existing set/Any and inclusive exact quantity semantics.
//    Different rows, categories or routes cannot supply partial witnesses. Compare
//    the requested value before caps. In both, either route's complete permit
//    suffices unless an applicable linked-account deny vetoes the whole command,
//    even for an optional action. Use the linked decision's individual_deny or
//    role_deny reason under existing USE precedence. An unmatched grant, an
//    individual limit or a known absent link is a nonpermit without a veto.
//    With neither route permitting, both must deny for definite action denial.
//    Otherwise that action is unavailable. A denied/unavailable required action
//    refuses the whole invocation. Omit optional actions lacking a complete permit.
//    Missing/malformed relevant facts cannot permit. A known absent link or lack
//    of a fitting grant in a complete set is a denial. A fitting present category can
//    witness authorization despite an unrelated unknown category. If none fits
//    and a fitting grant's category is unknown, that route is unavailable.
//    Inspect the whole action set: a linked-account veto refuses as
//    required_action_denied. Among required actions, definite denial takes
//    precedence over unavailability. Only routes selected by the command
//    participate. They have no effect on a response-only command. Effective
//    permission helper failures map as follows after the preflight above:
//    - input_limit, result_limit and allocation_failure throw the matching
//      ChatCommandFailure. invalid_limits is a construction failure.
//    - facts_unavailable, invalid_reference, invalid_catalog, invalid_facts,
//      invalid_request, invalid_policy, contradictory_individual and invalid_number
//      on the prevalidated call make that route unavailable. Request shape, grammar
//      and bound argument domain defects are attributed in steps 3–4 before this call.
// 6. Before admission, check both windows against the same monotonic sample.
//    Global key is the resolved command within this source/revision, shared by all
//    aliases and users. User key is that command and exact Twitch user, independent
//    of display name or linked account. A window of d>0 blocks iff now-start<d.
//    Equality opens it. d=0 disables it. Use checked elapsed arithmetic, no
//    timestamp+duration overflow. Only admission starts both enabled windows.
// 7. Check active user window capacity only for an otherwise admitted command
//    that would add an unexpired entry (user_cooldown_capacity). Project the
//    complete result (result_limit), then construct it and atomically publish the
//    terminal, sample and any cooldown changes. Store the new terminal and evict
//    the oldest if full as part of the same atomic publication. Allocation failure throws
//    allocation_failure. Denials, invalid arguments and cooldown refusals are
//    terminals too. They do not start or extend windows. No callbacks or effects
//    run inside evaluation.
//
// Bounds: preflight borrowed inputs before copying/parsing and project the whole
// owned result before materializing. Bytes sum string payloads and scalar widths
// (enums, bools, optional presence and variant tags are one byte, u64 is eight).
// Items sum vector entry counts recursively, including eligibility nodes and all
// facts and constraints. Fixed array elements count their scalar bytes. Structs
// sum fields. Both configuration and evaluation have separate full input budgets;
// evaluation charges envelope, authority and the clock sample. The root
// eligibility node has depth 1. Preflight depth before descending past the limit.
// Use permission limits for exact references, numeric components and per-operation
// work and scratch. Preflight an expanded request before materializing it, including
// repeated substitutions; keep at most one additional owned request and one
// permission evaluation workspace at a time. Other projection scratch consists
// of bounded indices/views over the declared input items and one projected result.
// Validate aggregate arithmetic, growth and max_size before allocation. Source
// review must establish these bounds.
// All limits must be positive except durations. Check capacity products, byte
// sums and size conversions for representability at construction. Retain only exact
// message keys and terminal state. Oldest is first completed fresh evaluation.
// A replay neither reorders the ledger nor extends retention. Evicted keys are
// fresh at this synthetic boundary. #337 suppresses live redelivery separately.
// Global timers are bounded by command count.
// Store at most active_user_entries unexpired user windows, reclaiming expired
// windows before an admission would exceed capacity. Refuse a new active window
// if they are all running. Terminal eviction never evicts a live cooldown.
// Retained keys replay at capacity. Limits, identity, clock and resource failures are exceptions,
// not terminal decisions. Any such failure leaves logical state intact. Retained
// dynamic identity bytes are bounded by terminal_entries*4*reference_bytes plus
// active_user_entries*reference_bytes and the one bounded configuration copy.
// Terminal records can refer to immutable command indices. No input or response
// payloads, permission facts, job handles or cached authority are retained.
class ChatCommandEvaluator final {
  public:
    explicit ChatCommandEvaluator(const ChatCommandRevision &, const ChatCommandLimits & = {});
    ~ChatCommandEvaluator();
    ChatCommandEvaluator(const ChatCommandEvaluator &) = delete;
    ChatCommandEvaluator &operator=(const ChatCommandEvaluator &) = delete;

    [[nodiscard]] ChatCommandResult
    evaluate(const ChatCommandEnvelope &, const ChatCommandAuthority &, std::uint64_t monotonic_ms);

  private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace subathon::core
