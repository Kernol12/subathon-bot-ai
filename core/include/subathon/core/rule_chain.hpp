// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <subathon/core/rule_actions.hpp>

namespace subathon::core::rule_chain {

// RULES-004 r3. References compare byte-exactly. The application supplies
// selected owner evidence and owns atomic commit, queueing and disclosure.
struct ChainKey {
    std::string installation;
    std::string root;
    bool operator==(const ChainKey &) const = default;
};

struct ChainProfile {
    RuleGenerationLimits maximum;
    std::uint32_t active_roots;
    std::uint32_t pending_events;
    std::size_t owned_bytes;
};

struct NamedAction {
    RuleActionKey key;
    bool operator==(const NamedAction &) const = default;
};

struct MilestoneOccurrence {
    std::string source;
    std::string occurrence;
    std::uint32_t ordinal;
    bool operator==(const MilestoneOccurrence &) const = default;
};
using ChainCause = std::variant<NamedAction, MilestoneOccurrence>;

struct EventBinding {
    ChainKey chain;
    std::string event;
    std::optional<std::string> parent;
    std::string capture;
    std::optional<ChainCause> cause;
    bool operator==(const EventBinding &) const = default;
};

struct RootBinding {
    EventBinding event;
    std::string normalized_occurrence;
    bool operator==(const RootBinding &) const = default;
};

struct AdmittedEvent {
    EventBinding binding;
    std::uint32_t depth;
    std::uint32_t ordinal;
    bool operator==(const AdmittedEvent &) const = default;
};

enum class CutoffReason { depth, count, both };
struct Cutoff {
    ChainKey chain;
    std::string root_capture;
    AdmittedEvent last;
    RuleGenerationLimits limits;
    CutoffReason reason;
};

enum class UnitPurpose { event_pass, released_occurrences };
struct UnitIdentity {
    std::string unit;
    ChainKey chain;
    std::string anchor;
    UnitPurpose purpose;
    bool operator==(const UnitIdentity &) const = default;
};

struct RetainedPolicyExceedsProfile {};
using GenerationStop = std::variant<Cutoff, RetainedPolicyExceedsProfile>;

struct Skipped {};
struct GenerationSuppressed {
    GenerationStop stop;
};
using CommittedOccurrence = std::variant<AdmittedEvent, Skipped, GenerationSuppressed>;

struct OccurrenceDisposition {
    MilestoneOccurrence occurrence;
    std::string parent;
    CommittedOccurrence disposition;
};

struct NamedSuppression {
    RuleActionKey action;
    GenerationStop stop;
};

struct Publication {
    UnitIdentity identity;
    std::optional<RuleCommittedPass> pass;
    std::vector<AdmittedEvent> children;
    std::vector<OccurrenceDisposition> occurrences;
    std::vector<NamedSuppression> named_suppressions;
    std::optional<Cutoff> cutoff; // Only the first committed cutoff is new publication.
};

enum class BlockReason { unresolved, conflict };
struct BlockedUnit {
    UnitIdentity identity;
    BlockReason reason;
};

struct ModeledHistory {
    RootBinding root;
    std::string admission;
    ChainProfile admitted_profile;
    std::vector<AdmittedEvent> events;
    std::vector<Publication> publications;
    std::optional<Cutoff> cutoff;
    std::optional<BlockedUnit> blocked;
};
struct VerifiedNew {
    std::string proposed_admission;
};
struct Unavailable {};
using OwnerSelection = std::variant<VerifiedNew, ModeledHistory, Unavailable>;

// The owner keeps the immutable capture and all backing for its borrowed view
// available through the selection call. An absent view is unavailable evidence.
struct CaptureView {
    std::reference_wrapper<const RuleConfiguration> configuration;
    RuleCapturedInput input;
    std::span<const std::string> authors;
    std::span<const RuleExplicitValuation> valuations;
};
struct CapacityUse {
    std::uint32_t active_roots;
    std::uint32_t pending_events;
    std::size_t owned_bytes;
};
struct OwnerView {
    // Captures retain the exact immutable meaning selected by the binding.
    // The ports and their backing stay available for the lifetime of the chain.
    std::function<std::optional<CaptureView>(const EventBinding &)> capture;
    // Usage of other contexts. Core accounts for this chain and its staged unit.
    std::function<CapacityUse(const ChainKey &)> other_usage;
};

class Chain {
  public:
    Chain(Chain &&) noexcept;
    Chain &operator=(Chain &&) noexcept;
    ~Chain();

  private:
    Chain();
    struct State;
    std::unique_ptr<State> state_;
    friend struct ChainOperations;
};

class Unit {
  public:
    Unit(Unit &&) noexcept;
    Unit &operator=(Unit &&) noexcept;
    ~Unit();

  private:
    Unit();
    struct State;
    std::unique_ptr<State> state_;
    friend struct ChainOperations;
};

enum class PolicyCheck { supported, invalid_profile, exceeds_profile };
[[nodiscard]] PolicyCheck check_generation_policy(const RuleConfiguration &, const ChainProfile &);

enum class OpenRefusal {
    invalid_profile,
    conflict,
    history_unavailable,
    needs_reactivation,
    capacity_hold
};
struct OpenedChain {
    Chain chain;
    std::uint32_t committed_children;
    std::optional<Cutoff> cutoff;
};
using OpenResult = std::variant<OpenedChain, OpenRefusal>;
[[nodiscard]] OpenResult open_chain(const RootBinding &, const ChainProfile &,
                                    const OwnerSelection &, OwnerView);

enum class BeginDisposition {
    duplicate_event,
    conflict,
    not_admitted,
    in_flight_hold,
    history_unavailable,
    capacity_hold
};
using BeginResult = std::variant<Unit, BeginDisposition>;
[[nodiscard]] BeginResult begin_event(Chain &, const EventBinding &, std::string unit);
[[nodiscard]] BeginResult begin_occurrences(Chain &, const std::string &parent, std::string unit);

// Plan through shipped #208 using the exact event and resolved immutable capture
// of a successfully begun event unit.
[[nodiscard]] RuleActionPlan plan_event(const Unit &, const RuleActionMapper &);

struct Duplicate {
    AdmittedEvent event;
};
struct NotRecorded {};
struct Conflict {};
using ChildLookup = std::variant<Duplicate, NotRecorded, Conflict, Unavailable>;
[[nodiscard]] ChildLookup lookup_child(const Chain &, const EventBinding &);

struct Reserved {
    AdmittedEvent event;
};
struct AwaitingRelease {};
struct CapacityHold {};
using ChildDecision = std::variant<Reserved, Duplicate, Conflict, AwaitingRelease, Skipped,
                                   GenerationSuppressed, CapacityHold>;
[[nodiscard]] ChildDecision offer_named_child(Unit &, const EventBinding &, const RuleActionKey &,
                                              const RuleNamedEvent &);

struct OutcomeUnknown {};
using NamedOutcome = std::variant<RuleOwnerResult, OutcomeUnknown>;
enum class NamedConfirmation {
    verified,
    known_refusal,
    numeric_hold,
    invalid_owner_result,
    unresolved
};
[[nodiscard]] NamedConfirmation confirm_named_child(Unit &, const AdmittedEvent &,
                                                    const NamedOutcome &);

// Wrap #208's submit callback. The proposal port supplies an owner-qualified
// child binding. The named handler runs only with an exact reservation.
using NamedProposal = std::function<EventBinding(const RuleActionKey &, const RuleNamedEvent &)>;
using NamedSubmit = std::function<NamedOutcome(const AdmittedEvent &, const RuleNamedEvent &,
                                               std::string_view checked_snapshot)>;
[[nodiscard]] RuleActionSubmit chain_action_submit(Unit &, NamedProposal, NamedSubmit,
                                                   RuleActionSubmit other_actions);

enum class OccurrenceState { awaiting_release, released, skipped };
[[nodiscard]] ChildDecision offer_occurrence_child(Unit &, const MilestoneOccurrence &,
                                                   const std::optional<EventBinding> &,
                                                   OccurrenceState);

struct RolledBack {};
struct Unresolved {};
using UnitResult = std::variant<Publication, RolledBack, Unresolved, Conflict>;
using UnitReadback = std::variant<ModeledHistory, UnitIdentity, Unavailable>;
[[nodiscard]] UnitResult finish_unit(Unit &&, const std::optional<RuleStagedPass> &,
                                     RuleCompletion);
[[nodiscard]] UnitResult reconcile_unit(Chain &, const std::string &unit, const UnitReadback &);

class RuleChainNotImplemented final : public std::logic_error {
  public:
    explicit RuleChainNotImplemented(const char *entry);
};

} // namespace subathon::core::rule_chain
