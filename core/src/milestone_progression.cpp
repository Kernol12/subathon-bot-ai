// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/milestone_progression.hpp>

#include <array>
#include <functional>
#include <numeric>
#include <string_view>

namespace subathon::core {
namespace {
using Error = MilestoneProgressionError;
constexpr MilestoneProgressionLimits maximum_limits;

void require(bool condition, Error error) {
    if (!condition)
        throw MilestoneProgressionFailure(error);
}

void validate_limit(std::size_t value, std::size_t maximum) {
    require(value != 0 && value <= maximum, Error::invalid_options);
}

// The accumulator never exceeds its ceiling, so subtraction is safe even for
// caller-owned lengths near size_t's maximum. No owning value is created here.
class ByteBudget {
  public:
    explicit ByteBudget(std::size_t ceiling) : ceiling_(ceiling) {}

    void add(std::size_t bytes) {
        require(bytes <= ceiling_ - used_, Error::resource_limit);
        used_ += bytes;
    }
    void target(const MilestoneTarget &value) {
        add(value.milestone_ref.size());
        add(8);
    }
    void completion(const MilestoneCompletion &value) {
        add(value.milestone_ref.size());
        add(value.definition_revision_ref.size());
        add(16);
        add(value.completion_context_ref.size());
    }
    void context(const MilestoneProgressionInput &input) {
        add(input.track_ref.size());
        add(input.definition_revision_ref.size());
        add(input.progression_context_ref.size());
        add(8);
    }

  private:
    std::size_t ceiling_;
    std::size_t used_{0};
};

void validate_items(std::size_t first, std::size_t second, std::size_t ceiling) {
    require(first <= ceiling && second <= ceiling - first, Error::resource_limit);
}

// Both passes use the declared depth-first field order. All reference byte
// ceilings are checked before any required-reference or identity semantics.
template <typename Check>
void for_each_reference(const MilestoneProgressionInput &input, Check check) {
    check(input.track_ref);
    check(input.definition_revision_ref);
    check(input.progression_context_ref);
    for (const auto &target : input.targets)
        check(target.milestone_ref);
    for (const auto &record : input.completed_prefix) {
        check(record.milestone_ref);
        check(record.definition_revision_ref);
        check(record.completion_context_ref);
    }
}

void validate_unique_targets(const std::vector<MilestoneTarget> &targets) {
    // Nonempty references make an empty view an unused slot. At most 4096
    // entries are inserted, in target order; collision probing borrows bytes.
    std::array<std::string_view, maximum_limits.targets> slots{};
    for (const auto &target : targets) {
        const std::string_view id = target.milestone_ref;
        auto position = std::hash<std::string_view>{}(id) % slots.size();
        while (!slots[position].empty()) {
            require(slots[position] != id, Error::duplicate_milestone);
            position = (position + 1) % slots.size();
        }
        slots[position] = id;
    }
}

// Call only with high >= low. Unsigned conversion BEFORE subtraction gives
// the exact 0..UINT64_MAX distance, including INT64_MAX minus INT64_MIN.
std::uint64_t distance(std::int64_t high, std::int64_t low) {
    return static_cast<std::uint64_t>(high) - static_cast<std::uint64_t>(low);
}

std::uint64_t deficit(std::int64_t target, std::int64_t score) {
    return target > score ? distance(target, score) : 0;
}

std::size_t decimal_digits(std::uint64_t value) {
    std::size_t count = 1;
    while (value >= 10) {
        value /= 10;
        ++count;
    }
    return count;
}
} // namespace

MilestoneProgressionResult advance_milestone_progression(const MilestoneProgressionInput &input,
                                                         const MilestoneProgressionLimits &limits) {
    validate_limit(limits.reference_bytes, maximum_limits.reference_bytes);
    validate_limit(limits.targets, maximum_limits.targets);
    validate_limit(limits.completed_prefix, maximum_limits.completed_prefix);
    validate_limit(limits.items, maximum_limits.items);
    validate_limit(limits.owned_bytes, maximum_limits.owned_bytes);

    const auto target_count = input.targets.size();
    const auto old_count = input.completed_prefix.size();
    require(target_count <= limits.targets, Error::resource_limit);
    require(old_count <= limits.completed_prefix, Error::resource_limit);
    validate_items(target_count, old_count, limits.items);
    for_each_reference(input, [&](std::string_view reference) {
        require(reference.size() <= limits.reference_bytes, Error::resource_limit);
    });
    ByteBudget input_bytes(limits.owned_bytes);
    input_bytes.context(input);
    for (const auto &target : input.targets)
        input_bytes.target(target);
    for (const auto &record : input.completed_prefix)
        input_bytes.completion(record);

    for_each_reference(input, [](std::string_view reference) {
        require(!reference.empty(), Error::invalid_reference);
    });
    validate_unique_targets(input.targets);
    for (std::size_t i = 1; i < target_count; ++i)
        require(input.targets[i - 1].target_micro_points < input.targets[i].target_micro_points,
                Error::nonascending_targets);
    require(old_count <= target_count, Error::invalid_completed_prefix);
    for (std::size_t i = 0; i < old_count; ++i)
        require(input.completed_prefix[i].milestone_ref == input.targets[i].milestone_ref,
                Error::invalid_completed_prefix);

    auto completed_count = old_count;
    while (completed_count < target_count &&
           input.targets[completed_count].target_micro_points <= input.score_micro_points)
        ++completed_count;
    require(completed_count <= limits.completed_prefix, Error::resource_limit);
    validate_items(completed_count, completed_count, limits.items);

    const auto *previous = completed_count != 0 ? &input.targets[completed_count - 1] : nullptr;
    const auto *next = completed_count < target_count ? &input.targets[completed_count] : nullptr;
    std::uint64_t earned = 0;
    std::uint64_t span = 0;
    std::uint64_t numerator = 0;
    std::uint64_t denominator = 1;
    if (previous && next) {
        span = distance(next->target_micro_points, previous->target_micro_points);
        earned = input.score_micro_points > previous->target_micro_points
                     ? distance(input.score_micro_points, previous->target_micro_points)
                     : 0;
        const auto divisor = std::gcd(earned, span);
        numerator = earned / divisor;
        denominator = span / divisor;
    }

    // Project every returned field and repeated copy before the first output
    // string/vector/Exact construction. Only bounded scalar arithmetic and
    // borrowed input have been used so far.
    ByteBudget output_bytes(limits.owned_bytes);
    output_bytes.context(input);
    for (const auto &record : input.completed_prefix)
        output_bytes.completion(record);
    for (std::size_t i = old_count; i < completed_count; ++i) {
        output_bytes.add(input.targets[i].milestone_ref.size());
        output_bytes.add(input.definition_revision_ref.size());
        output_bytes.add(16);
        output_bytes.add(input.progression_context_ref.size());
    }
    output_bytes.add(16); // New suffix begin/count.
    output_bytes.add(1);  // Projection state.
    for (std::size_t i = 0; i < completed_count; ++i) {
        output_bytes.target(input.targets[i]);
        output_bytes.add(8);
    }
    if (previous)
        output_bytes.target(*previous);
    if (next)
        output_bytes.target(*next);
    output_bytes.add(8); // Track deficit.
    if (next)
        output_bytes.add(8); // Remaining.
    if (previous && next) {
        output_bytes.add(16); // Earned/span.
        output_bytes.add(decimal_digits(numerator));
        output_bytes.add(decimal_digits(denominator));
    }

    MilestoneProgressionData data;
    data.track_ref = input.track_ref;
    data.definition_revision_ref = input.definition_revision_ref;
    data.progression_context_ref = input.progression_context_ref;
    data.score_micro_points = input.score_micro_points;
    data.newly_completed_begin = static_cast<std::uint64_t>(old_count);
    data.newly_completed_count = static_cast<std::uint64_t>(completed_count - old_count);
    data.completed_prefix.reserve(completed_count);
    for (const auto &record : input.completed_prefix)
        data.completed_prefix.push_back(record);
    for (std::size_t i = old_count; i < completed_count; ++i)
        data.completed_prefix.push_back({input.targets[i].milestone_ref,
                                         input.definition_revision_ref,
                                         input.targets[i].target_micro_points,
                                         input.score_micro_points, input.progression_context_ref});

    auto &projection = data.projection;
    projection.state = target_count == 0                 ? MilestoneTrackState::empty
                       : completed_count == 0            ? MilestoneTrackState::before_first
                       : completed_count == target_count ? MilestoneTrackState::all_completed
                                                         : MilestoneTrackState::between_targets;
    projection.completed_deficits.reserve(completed_count);
    for (std::size_t i = 0; i < completed_count; ++i)
        projection.completed_deficits.push_back(
            {input.targets[i],
             deficit(input.targets[i].target_micro_points, input.score_micro_points)});
    if (previous) {
        projection.previous = *previous;
        projection.track_deficit_micro_points =
            deficit(previous->target_micro_points, input.score_micro_points);
    }
    if (next) {
        projection.next = *next;
        projection.remaining_micro_points =
            distance(next->target_micro_points, input.score_micro_points);
    }
    if (previous && next)
        projection.interval = MilestoneInterval{earned, span, Exact(numerator) / denominator};
    return MilestoneProgressionResult(std::move(data));
}
} // namespace subathon::core
