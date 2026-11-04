// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/exact_decimal.hpp>
#include <utility>
#include <vector>

namespace subathon::core {
// Owned raw values; references and supplied history do not establish authority.
struct MilestoneTarget {
    std::string milestone_ref;
    std::int64_t target_micro_points{0};
};
struct MilestoneCompletion {
    std::string milestone_ref;
    std::string definition_revision_ref;
    std::int64_t target_micro_points{0};
    std::int64_t score_micro_points{0};
    std::string completion_context_ref;
};
struct MilestoneProgressionInput {
    std::string track_ref;
    std::string definition_revision_ref;
    std::string progression_context_ref;
    std::int64_t score_micro_points{0};
    std::vector<MilestoneTarget> targets;
    std::vector<MilestoneCompletion> completed_prefix;
};
struct MilestoneProgressionLimits {
    std::size_t reference_bytes{256};
    std::size_t targets{4096};
    std::size_t completed_prefix{4096};
    std::size_t items{16384};
    std::size_t owned_bytes{8388608};
};
enum class MilestoneTrackState { empty, before_first, between_targets, all_completed };
struct MilestoneDeficit {
    MilestoneTarget current_target;
    std::uint64_t deficit_micro_points{0};
};
struct MilestoneInterval {
    std::uint64_t earned_micro_points{0};
    std::uint64_t span_micro_points{0};
    Exact fraction{0};
};
struct MilestoneProjection {
    MilestoneTrackState state{MilestoneTrackState::empty};
    std::vector<MilestoneDeficit> completed_deficits;
    std::optional<MilestoneTarget> previous;
    std::optional<MilestoneTarget> next;
    std::uint64_t track_deficit_micro_points{0};
    std::optional<std::uint64_t> remaining_micro_points;
    std::optional<MilestoneInterval> interval;
};
struct MilestoneProgressionData {
    std::string track_ref;
    std::string definition_revision_ref;
    std::string progression_context_ref;
    std::int64_t score_micro_points{0};
    std::vector<MilestoneCompletion> completed_prefix;
    std::uint64_t newly_completed_begin{0};
    std::uint64_t newly_completed_count{0};
    MilestoneProjection projection;
};

// Borrows from data() remain valid until this owner is destroyed, assigned or moved.
class MilestoneProgressionResult final {
  public:
    MilestoneProgressionResult(const MilestoneProgressionResult &) = default;
    MilestoneProgressionResult(MilestoneProgressionResult &&) = default;
    MilestoneProgressionResult &operator=(const MilestoneProgressionResult &) = default;
    MilestoneProgressionResult &operator=(MilestoneProgressionResult &&) = default;
    [[nodiscard]] const MilestoneProgressionData &data() const noexcept { return data_; }

  private:
    explicit MilestoneProgressionResult(MilestoneProgressionData data) : data_(std::move(data)) {}
    friend MilestoneProgressionResult
    advance_milestone_progression(const MilestoneProgressionInput &input,
                                  const MilestoneProgressionLimits &limits);
    MilestoneProgressionData data_;
};

enum class MilestoneProgressionError {
    invalid_options,
    resource_limit,
    invalid_reference,
    duplicate_milestone,
    nonascending_targets,
    invalid_completed_prefix
};
class MilestoneProgressionFailure final : public std::invalid_argument {
  public:
    explicit MilestoneProgressionFailure(MilestoneProgressionError code)
        : std::invalid_argument("Invalid milestone progression input"), code_(code) {}
    [[nodiscard]] MilestoneProgressionError code() const noexcept { return code_; }

  private:
    MilestoneProgressionError code_;
};
class MilestoneProgressionNotImplemented final : public std::logic_error {
  public:
    MilestoneProgressionNotImplemented()
        : std::logic_error(
              "Milestone progression is intentionally unimplemented; review tests first") {}
};

// Revision 1: docs/contracts/milestone-progression-api-v1.md. Missing feature until
// independently reviewed tests and intended red; never invoke at build/discovery.
[[nodiscard]] MilestoneProgressionResult
advance_milestone_progression(const MilestoneProgressionInput &input,
                              const MilestoneProgressionLimits &limits = {});
} // namespace subathon::core
