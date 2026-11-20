// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/chat_commands.hpp>

#include <stdexcept>

namespace subathon::core {

ChatCommandFailure::ChatCommandFailure(ChatCommandError code) noexcept : code_(code) {}

ChatCommandError ChatCommandFailure::code() const noexcept { return code_; }

const char *ChatCommandFailure::what() const noexcept { return "Chat command evaluation failed"; }

struct ChatCommandEvaluator::State {};

// Stage 1 link surface. Construction does not validate or retain configuration.
ChatCommandEvaluator::ChatCommandEvaluator(const ChatCommandRevision &, const ChatCommandLimits &) {
}
ChatCommandEvaluator::~ChatCommandEvaluator() = default;

ChatCommandResult ChatCommandEvaluator::evaluate(const ChatCommandEnvelope &,
                                                 const ChatCommandAuthority &, std::uint64_t) {
    throw std::logic_error("ACCESS-007 chat command evaluation is intentionally unimplemented");
}

} // namespace subathon::core
