// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <subathon/core/downtime_recovery.hpp>
#include <subathon/core/modifier_clock.hpp>
#include <subathon/core/refund_entitlement.hpp>
#include <subathon/core/rule_chain.hpp>
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/milestone_timeline.hpp>
#include <subathon/server/modifier_stacking.hpp>

namespace subathon::server::rule_simulator {

namespace chain = core::rule_chain;

struct Limits {
    std::size_t turns;
    std::size_t owned_bytes;
    std::size_t rows;
    chain::ChainProfile chain;
    core::ModifierStackLimits modifiers;
    core::MilestoneProgressionLimits milestones;
};

struct SelectedModifiers {
    CapturedModifierContext context;
    ModifierTraceProfile admitted_profile;
    core::ModifierStackPolicy policy;
    std::optional<std::vector<core::ModifierInstance>> eligible;
    std::vector<std::string> matched_qualifiers;
};

// Own the backing for the producer capture views for every event, including children.
struct EventCapture {
    chain::EventBinding binding;
    core::RuleConfiguration configuration;
    core::RuleTriggerKind trigger;
    core::PlanEvent receipt;
    core::ContributionState eligibility;
    core::PlanValuation valuation;
    std::vector<core::RuleCapturedFact> facts;
    std::vector<std::string> authors;
    std::optional<core::EffectiveRateSnapshot> rates;
    std::vector<core::RuleExplicitValuation> explicit_valuations;
    std::optional<SelectedModifiers> modifiers;
    std::optional<std::int64_t> quote_observation_utc_ms;
    std::optional<std::int64_t> quote_age_threshold_ms;
};

struct ClockSample {
    std::int64_t utc_ms{};
    std::chrono::milliseconds elapsed{};
};

struct ScenarioState {
    core::TimerConfig timer_config;
    core::TimerState timer;
    std::int64_t time_debt_ms{};
    std::optional<std::int64_t> main_cap_ms;
    core::PauseConfig pause_config;
    core::PauseState pause;
    std::int64_t score_micro_points{};
    std::optional<core::MilestoneProgressionInput> milestone_track;
    MilestoneTimelineSettings timeline;
    std::string state_revision;
    std::optional<core::ModifierClockValue> current_modifier;
};

struct RootSelection {
    chain::RootBinding root;
    chain::OwnerSelection selection;
};

struct PreparedPlan {
    core::ApplicationPlan selected_history;
    core::PlanPreview preview;
};

struct MessageIntent {
    std::string route;
    std::string sender;
    std::string template_version;
    std::optional<std::string> synthetic_text;
    bool operator==(const MessageIntent &) const = default;
};

struct ChatObservation {
    core::RuleActionKey action;
    std::string sender;
    core::RuleGate execution;
    std::optional<std::string> text;
    std::optional<core::RuleDeliveryObservation> queue_acceptance;
};

struct EventTurn {
    chain::EventBinding event;
    core::RuleCompletion completion{core::RuleCompletion::committed};
    std::optional<PreparedPlan> prepared;
    std::optional<ChatObservation> chat;
    // Each binding's captured cause identifies the exact named proposal.
    std::vector<chain::EventBinding> named;
    std::vector<std::pair<core::RuleActionKey, std::string>> effect_refs;
};

struct TimeCut {
    std::optional<core::PauseObservation> signals;
};

struct PauseControl {
    core::PauseReason reason;
    bool active;
};

struct RefundTurn {
    chain::EventBinding event;
    core::RuleActionKey original_action;
    std::string original_event;
    core::RefundAmounts amounts;
    core::RefundTimeCredit previous;
    core::RefundDeficitPolicy policy;
    core::TimerState selected_current_timer;
};

struct MilestoneCapture {
    chain::MilestoneOccurrence occurrence;
    std::vector<MilestoneAction> actions;
    MilestoneTimingOverride timing;
    chain::EventBinding released_event;
};

enum class TimelineOperation {
    sample,
    countdown_pause,
    countdown_resume,
    queue_pause,
    queue_resume
};
struct GroupObservation {
    MilestoneOccurrenceKey head;
    std::optional<MilestoneHeadDisposition> disposition;
    // If ready, the exact modeled release either returns normally or throws.
    core::RuleCompletion release_completion{core::RuleCompletion::committed};
};

struct TimelineTurn {
    TimelineOperation operation;
    std::optional<GroupObservation> head;
};

// Complete selected committed suffix for restoring a fresh timeline after an
// exact release readback. Unavailable evidence keeps the existing timeline held.
struct GroupReadback {
    std::string attempt_turn;
    MilestoneOccurrenceKey attempted;
    core::RuleCompletion completion;
    std::vector<CommittedMilestoneGroup> committed_pending_suffix;
};

struct DeliveryTurn {
    std::string selected_job;
    core::RuleDeliveryObservation observation;
};

struct UnitReconciliation {
    chain::ChainKey chain;
    chain::UnitIdentity unit;
    chain::UnitReadback selected;
};

// The r3 restart examples select a complete empty schedule, or unavailable
// coverage. Keep this owned rather than storing the recovery producer's spans.
struct EmptyRecoverySchedule {
    std::string facts_ref;
    std::string delivery_revision;
};
struct RestartTurn {
    core::RecoveryBasis basis;
    core::DowntimePolicy policy;
    core::RecoveryUtcObservation checkpoint;
    core::RecoveryUtcObservation restart;
    core::RecoveryGapClock gap;
    std::variant<core::RecoveryScheduleUnavailable, EmptyRecoverySchedule> schedule;
    RootSelection retained_chain;
};

using TurnInput =
    std::variant<EventTurn, TimeCut, PauseControl, RefundTurn, TimelineTurn, GroupReadback,
                 ConnectorReport, DeliveryTurn, UnitReconciliation, RestartTurn>;

struct OwnerTurn {
    std::string ref;
    ClockSample sample;
    TurnInput input;
};

struct ScenarioInputs {
    ScenarioState initial;
    std::vector<RootSelection> roots;
    std::vector<EventCapture> captures;
    std::vector<MilestoneCapture> milestone_captures;
    std::vector<OwnerTurn> turns;
};
enum class CaptureSelection { exact, unavailable, conflict };
struct CapturedReplay {
    std::string capture;
    CaptureSelection selected;
    ScenarioInputs inputs;
    std::optional<SelectedModifiers> attempted_manual_modifiers;
};
struct Hypothetical {
    std::string capture;
    ScenarioInputs inputs;
};
// Available current context outside the exact selected capture.
struct CurrentSources {
    std::optional<core::RuleConfiguration> configuration;
    std::optional<core::ModifierClockValue> modifier;
    std::optional<core::EffectiveRateSnapshot> rates;
};
struct Scenario {
    std::string ref;
    std::uint32_t version;
    std::variant<CapturedReplay, Hypothetical> input;
    CurrentSources current{};
};

enum class Mode { captured_replay, hypothetical };
enum class Completion { complete, unavailable, conflict, invalid_input, resource_hold, unit_hold };
enum class Gate { use, grant, execution, target, capability };
struct CheckedGate {
    Gate gate;
    core::RuleGate outcome;
    bool operator==(const CheckedGate &) const = default;
};

struct ActionTrace {
    core::RuleActionIntent intent;
    std::vector<CheckedGate> gates;
    std::optional<core::RuleStop> stop;
    bool submitted{};
    std::optional<core::RuleProjection> proposed;
    std::optional<core::RuleOwnerObservation> accepted;
    std::optional<MessageIntent> message;
};

struct QuoteTrace {
    core::HistoricalValuation provenance;
    core::Exact rate;
    core::Exact converted;
    std::optional<std::int64_t> age_ms;
    std::optional<bool> stale;
};

struct PlanTrace {
    std::optional<core::PlanPreviewValidity> preview;
    core::CapturedPlanRevision inspected;
    std::uint64_t head;
    std::vector<core::RequestedEffect> head_requests;
};

struct GroupReleaseTrace {
    MilestoneOccurrenceKey occurrence;
    core::RuleCompletion completion;
};

struct TurnTrace {
    std::string ref;
    ScenarioState state;
    // Captured decisions index the configuration, including the skipped rules'
    // retained actions. Only selected action intents receive per-action traces.
    std::optional<core::RuleConfiguration> configuration;
    std::optional<core::RuleActionPlan> selection;
    std::vector<ActionTrace> actions;
    std::optional<QuoteTrace> quote;
    std::optional<PlanTrace> plan;
    std::optional<ModifierStackEvaluation> modifiers;
    std::optional<core::RulePassFailure> pass_failure;
    std::optional<core::RuleCompletion> completion;
    std::optional<chain::OpenRefusal> open_refusal;
    std::optional<chain::BeginDisposition> begin_disposition;
    std::optional<chain::AdmittedEvent> admitted;
    std::optional<std::uint32_t> committed_children;
    std::optional<chain::Cutoff> retained_cutoff;
    std::optional<chain::Publication> publication;
    std::optional<std::string> cutoff_audit_ref;
    std::optional<std::string> cutoff_notice_ref;
    std::optional<core::MilestoneProgressionData> progression;
    std::optional<ActiveMilestonePresentation> presentation;
    bool countdown_paused{};
    bool queue_paused{};
    std::optional<GroupObservation> group_head;
    std::vector<GroupReleaseTrace> releases;
    std::optional<core::RuleDeliveryTrace> delivery;
    std::optional<core::PlanEvent> refund_receipt;
    std::optional<core::RefundTimeEntitlement> refund;
    std::optional<core::TimeReversalResult> reversal;
    std::optional<core::TimerAdvance> timer_cut;
    std::optional<ConnectorStatus> connector;
    std::optional<core::DowntimeRecoveryPreview> recovery;
};

struct SimulationResult {
    std::string scenario;
    Mode mode;
    bool simulated;
    Completion completion;
    std::vector<TurnTrace> trace;
    ScenarioState final_state;
    std::size_t owner_callbacks;
};

class RuleSimulatorNotImplemented final : public std::logic_error {
  public:
    RuleSimulatorNotImplemented() : std::logic_error("RULES-005 inspect_simulation") {}
};

[[nodiscard]] SimulationResult inspect_simulation(const Scenario &, const Limits &);
} // namespace subathon::server::rule_simulator
