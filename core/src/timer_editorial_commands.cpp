// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/timer_editorial_commands.hpp>

#include <utility>

namespace subathon::core {

TimerEditorialProposal::TimerEditorialProposal(TimerEditorialProposalData data)
    : data_(std::move(data)) {}

const TimerEditorialProposalData &TimerEditorialProposal::data() const noexcept { return data_; }

TimerEditorialFailure::TimerEditorialFailure(TimerEditorialError code) noexcept : code_(code) {}

TimerEditorialError TimerEditorialFailure::code() const noexcept { return code_; }

const char *TimerEditorialFailure::what() const noexcept {
    return "Timer editorial command projection failed";
}

TimerEditorialCommandsNotImplemented::TimerEditorialCommandsNotImplemented()
    : std::logic_error(
          "Timer editorial commands are intentionally unimplemented; review tests first") {}

// Inert baseline: no validation, observation, numeric helper or success path.
TimerEditorialProposal project_timer_editorial_command(const TimerEditorialInput &,
                                                       const TimerEditorialLimits &) {
    throw TimerEditorialCommandsNotImplemented{};
}

} // namespace subathon::core
