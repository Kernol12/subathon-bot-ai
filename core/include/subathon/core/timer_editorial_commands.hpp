// SPDX-License-Identifier: GPL-3.0-or-later
// Approved timer editorial API v1; raw facts and proposals confer no authority.
#pragma once

#include <subathon/core/time_debt.hpp>
#include <subathon/core/timer_lifecycle.hpp>

#include <array>
#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace subathon::core {
enum class TimerEditorialAccessState : std::uint8_t { unknown, denied, allowed };
enum class TimerCapChoice : std::uint8_t { keep, reduce, abort };

struct TimerEditorialValues {
    TimerState timer;
    std::int64_t time_debt_ms = 0;
    std::optional<std::int64_t> maximum_remaining_ms;
    TimerConfig config;
};
struct TimerEditorialSnapshot {
    std::string revision;
    TimerEditorialValues values;
};
struct TimerEditorialAccess {
    TimerEditorialAccessState state = TimerEditorialAccessState::unknown;
    std::optional<std::string> permission_revision;
};
struct TimerSignedDelta {
    std::int64_t requested_ms = 0;
    std::optional<RefundDeficitPolicy> negative_policy;
};
struct TimerCapWarningPreview {
    std::string revision;
    std::int64_t main_remaining_ms = 0;
    std::optional<std::int64_t> old_cap_ms;
    std::int64_t proposed_cap_ms = 0;
};
struct TimerCapReplacement {
    std::optional<std::int64_t> maximum_remaining_ms;
    std::optional<TimerCapChoice> choice;
    std::optional<TimerCapWarningPreview> warning_preview;
};
struct TimerDeadlineReplacement {
    std::optional<std::int64_t> hard_end_utc_ms;
};
struct TimerFirstStartDeadlineCheck {};
using TimerEditorialPayload = std::variant<TimerSignedDelta, TimerCapReplacement,
                                           TimerDeadlineReplacement, TimerFirstStartDeadlineCheck>;
struct TimerEditorialCommand {
    std::string operation;
    std::string expected_revision;
    std::optional<std::string> actor;
    std::optional<std::string> reason;
    TimerEditorialPayload payload;
};
struct TimerEditorialInput {
    TimerEditorialSnapshot current;
    TimerEditorialCommand command;
    TimerEditorialAccess access;
    std::vector<TimerClockStep> observations;
    std::int64_t final_utc_ms = 0;
};
struct TimerEditorialLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t observation_steps = 4096;
    std::uint64_t input_owned_bytes = 8388608;
    std::uint64_t result_owned_bytes = 16777216;
};

enum class TimerEditorialDisposition : std::uint8_t {
    accepted,
    access_denied,
    access_unknown,
    revision_conflict,
    cap_choice_required,
    aborted,
    prerequisite_not_ready,
    policy_not_ready,
    timer_review_required,
    numeric_overflow_review,
    start_deadline_blocked,
    start_deadline_clear
};
enum class TimerEditorialPrerequisite : std::uint8_t {
    none,
    first_start_absolute_main,
    finished_policy
};
struct TimerEditorialTransitionBatch {
    std::uint8_t count = 0;
    std::array<TimerTransition, 4> entries{};
};
struct TimerEditorialAdjustment {
    std::int64_t requested_ms = 0;
    std::int64_t main_adjustment_ms = 0;
    std::int64_t debt_repaid_ms = 0;
    std::int64_t debt_added_ms = 0;
    std::int64_t cap_limited_ms = 0;
    std::int64_t unapplied_ms = 0;
};
struct TimerCapWarning {
    std::int64_t main_remaining_ms = 0;
    std::optional<std::int64_t> old_cap_ms;
    std::int64_t proposed_cap_ms = 0;
    std::int64_t reduction_ms = 0;
};
struct TimerEditorialProposalData {
    TimerEditorialInput input;
    TimerEditorialDisposition disposition = TimerEditorialDisposition::prerequisite_not_ready;
    TimerEditorialPrerequisite prerequisite = TimerEditorialPrerequisite::none;
    std::optional<TimerEditorialValues> observed_before;
    std::optional<TimerEditorialValues> accepted_after;
    std::vector<TimerEditorialTransitionBatch> observation_transitions;
    TimerEditorialTransitionBatch editorial_transitions;
    std::optional<TimerEditorialAdjustment> adjustment;
    std::optional<TimerCapWarning> warning;
};

class TimerEditorialProposal;
[[nodiscard]] TimerEditorialProposal
project_timer_editorial_command(const TimerEditorialInput &input,
                                const TimerEditorialLimits &limits = {});
class TimerEditorialProposal final {
  public:
    TimerEditorialProposal(const TimerEditorialProposal &) = default;
    TimerEditorialProposal(TimerEditorialProposal &&) noexcept = default;
    TimerEditorialProposal &operator=(const TimerEditorialProposal &) = default;
    TimerEditorialProposal &operator=(TimerEditorialProposal &&) noexcept = default;
    [[nodiscard]] const TimerEditorialProposalData &data() const noexcept;

  private:
    explicit TimerEditorialProposal(TimerEditorialProposalData data);
    TimerEditorialProposalData data_;
    friend TimerEditorialProposal project_timer_editorial_command(const TimerEditorialInput &,
                                                                  const TimerEditorialLimits &);
};
enum class TimerEditorialError : std::uint8_t {
    invalid_limits,
    input_limit,
    invalid_snapshot,
    invalid_command,
    invalid_access,
    invalid_observation,
    result_limit,
    allocation_failure
};
class TimerEditorialFailure final : public std::exception {
  public:
    explicit TimerEditorialFailure(TimerEditorialError code) noexcept;
    [[nodiscard]] TimerEditorialError code() const noexcept;
    [[nodiscard]] const char *what() const noexcept override;

  private:
    TimerEditorialError code_;
};
class TimerEditorialCommandsNotImplemented final : public std::logic_error {
  public:
    TimerEditorialCommandsNotImplemented();
};
} // namespace subathon::core
