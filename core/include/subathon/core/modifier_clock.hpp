// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/downtime_recovery.hpp>
#include <subathon/core/modifier_activation.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace subathon::core {

// Pure proposal boundary for MODIFIERTEST-002 r3. These values carry supplied
// evidence and candidate effects. They confer no authority to write or deliver.
enum class ModifierClockMode { ActiveSubathon, RealElapsed };

struct ModifierClockValue {
    ModifierInstance revision; // Captured revision, including its historical remaining value.
    std::optional<std::int64_t>
        remaining_ms; // Unknown across an unqualified gap; zero is terminal.
    ModifierClockMode mode = ModifierClockMode::ActiveSubathon;
    bool operator==(const ModifierClockValue &) const = default;
};

struct ModifierClockObservation {
    std::string timer_lineage;
    std::string ref;
    std::int64_t utc_ms;
    bool operator==(const ModifierClockObservation &) const = default;
};

enum class ModifierRuleEvent { Expired, Activated };

struct ModifierRuleCandidate {
    ModifierRuleEvent event;
    ModifierClockObservation observation;
    std::string instance_ref;
    std::string revision_ref;
    // Present only when evidence allows reconstruction of the crossing.
    std::optional<RecoveryUtcInstant> reconstructed_utc;
    bool operator==(const ModifierRuleCandidate &) const = default;
};

struct ModifierActivationAtCut {
    ModifierDefinition definition;
    ModifierActivation request;
    std::optional<ModifierClockMode> clock;
};

using ModifierCutEvent = std::variant<TimerTransition, ModifierRuleCandidate>;

struct ModifierCutProposal {
    std::optional<ModifierClockValue> head;
    std::optional<ModifierActivationProposal> activation;
    std::vector<ModifierCutEvent> events;
};

// Bind to consecutive observations on the same timer lineage. The timer owner
// supplies the pre-cut state, admitted step and ordered reducer output. Advance
// the old head before applying the optional activation at the observation.
[[nodiscard]] ModifierCutProposal
project_modifier_cut(const ModifierClockObservation &before, const ModifierClockObservation &now,
                     const TimerState &timer_before, const TimerClockStep &step,
                     const TimerAdvance &timer_advance,
                     const std::optional<ModifierClockValue> &head,
                     const std::optional<ModifierActivationAtCut> &activation = {});

struct ModifierRecoveryProjection {
    ModifierClockValue value;
    std::vector<ModifierRuleCandidate> events;
};

// The core recovery proposals and their exact input/episode bindings come from
// CORE-006. The restart cut uses the selected offline policy; review wait uses
// only qualified wait elapsed. Resolution selects offline progression from the
// checkpoint and retains the real-elapsed outcome already projected.
[[nodiscard]] ModifierRecoveryProjection
project_modifier_restart(const DowntimeRecoveryInput &input, const DowntimeRecoveryPreview &preview,
                         const ModifierClockValue &checkpoint);

[[nodiscard]] ModifierRecoveryProjection
project_modifier_review_wait(const RecoveryReviewSnapshot &previous,
                             const RecoveryReviewSnapshot &observation,
                             const RecoveryReviewWait &timer_wait, const TimerClockStep &step,
                             const ModifierClockValue &current);

[[nodiscard]] ModifierRecoveryProjection project_modifier_review_resolution(
    const DowntimeRecoveryInput &input, const RecoveryReviewSnapshot &snapshot,
    const std::variant<RecoveryReviewResolution, RecoveryReviewRefusal, RecoveryCorrectionRefusal>
        &timer_resolution,
    const ModifierClockValue &checkpoint, const ModifierClockValue &current);

struct ModifierScoreArrival {
    ModifierClockObservation observation;
    std::string input_ref;
    std::string_view target_ref;
    std::int64_t base_micro_points;
    std::span<const std::string_view> matched_qualifiers;
    bool post_finish_modifiers{};
    bool post_finish_score_progression{};
    bool overlay_visible = true;
};

struct ModifierScoreBasis {
    ModifierClockObservation observation;
    std::string input_ref;
    std::int64_t base_micro_points;
    std::optional<ModifierTargetSelection> selection;
    PauseReasons blockers;
    bool score_progression{};
};

struct ModifierScoreProposal {
    bool record_input;
    std::optional<std::int64_t> requested_micro_points;
};

// Capture the authorized arrival view (ordinary or triggering) before any later
// clock or definition change. Score proposal consumes only that owned basis.
[[nodiscard]] ModifierScoreBasis
capture_modifier_score(const ModifierScoreArrival &arrival, const TimerState &timer,
                       const PauseState &pause, const std::optional<ModifierClockValue> &head);

[[nodiscard]] ModifierScoreProposal request_modifier_score(const ModifierScoreBasis &basis);

class ModifierClockNotImplemented final : public std::logic_error {
  public:
    explicit ModifierClockNotImplemented(const char *seam) : std::logic_error(seam) {}
};

} // namespace subathon::core
