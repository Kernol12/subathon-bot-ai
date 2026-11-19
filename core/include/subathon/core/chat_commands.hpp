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
    // tuple. Configuration must have passed the owner's applicable GRANT checks.
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
    // Only explicitly chat-enabled ordinary operations can enter a proposal.
    // These are trusted server facts, separate from the command configuration.
    std::vector<std::string> chat_operations;
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
    terminal_capacity,
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
// 1. Validate the exact message key, including source and per-reference limits.
//    Look up the complete key before considering any other input. A hit returns
//    the original terminal decision even if aliases, arguments, sender, rights
//    or clock inputs differ. Replay never returns a proposal or advances time.
// 2. Preflight complete supplied inputs, validate sender ID, category tags and
//    alias syntax, and check monotonic_ms against the last completed fresh
//    evaluation, if any. Resolve the alias. An unknown valid alias is
//    terminal with no command. All other terminals name the selected command.
//    Bind every declared argument, including unused ones, at its stated type.
//    Wrong count, invalid type, or a bound request outside the operation's declared
//    domain is invalid_arguments. Validate all action bindings and complete
//    request shapes before checking authority. Unknown catalog or incompatible
//    configured action shape is authority_unavailable. An operation that exists
//    but is outside the ordinary chat-enabled set cannot be permitted.
// 3. Evaluate eligibility. False is ineligible, unknown eligibility_unavailable.
//    For each action, use the selected route. Linked account requires the exact
//    sender's current link, never another user's policy. Apply current effective
//    USE with resource-first individual selection and role-deny precedence.
//    Delegated category requires a present category on one current grant covering
//    the exact operation and EVERY resource and exact requested quantity together.
//    Each constraint uses existing set/Any and inclusive exact quantity semantics.
//    Different rows, categories or routes cannot supply partial witnesses. Compare
//    the requested value before caps. Both requires independent complete permits
//    through both routes for each operation. A denied/unavailable required action
//    refuses the whole invocation. Omit optional actions lacking a complete permit.
//    Missing/malformed relevant facts cannot permit. A source mismatch or an
//    asserted link for a different sender is unavailable. A known absent link
//    or known lack of a fitting grant is a denial. A fitting present category can
//    witness authorization despite an unrelated unknown category. If none fits
//    and a fitting grant's category is unknown, that route is unavailable.
//    Inspect all required outcomes: definite denial takes precedence over
//    unavailable, including within both. Only routes selected by the command
//    participate. They have no effect on a response-only command.
// 4. Before admission, check both windows against the same monotonic sample.
//    Global key is the resolved command within this source/revision, shared by all
//    aliases and users. User key is that command and exact Twitch user, independent
//    of display name or linked account. A window of d>0 blocks iff now-start<d.
//    Equality opens it. d=0 disables it. Use checked elapsed arithmetic, no
//    timestamp+duration overflow. Only admission starts both enabled windows.
// 5. Publish the terminal, sample and any cooldown changes atomically before returning a
//    fully built result. Denials, invalid arguments and cooldown refusals are
//    terminals too. They do not start/extend a cooldown. No callbacks, other
//    state mutation, effect enqueue or cross-thread publication inside evaluation.
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
// sums and size conversions for representability at construction. Store terminal
// records without eviction until revision end, retaining exact
// message keys and terminal state only. Global timers are bounded by command count.
// Store at most active_user_entries unexpired user windows, reclaiming expired
// windows before an admission would exceed capacity. Reject new work on capacity
// failure without evicting a terminal/active window. Existing keys still replay
// at capacity. Limits, identity, clock and atomic resource failures are exceptions,
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
