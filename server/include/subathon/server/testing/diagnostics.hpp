// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/diagnostics.hpp>

namespace subathon::server::diagnostics::testing {
// Manual owner turns and syscall outcomes for deterministic local fixtures.
// Missing mutation outcome suspends the single in-flight change.
struct LogControl {
    std::function<std::optional<Utc>()> utc;
    std::function<std::optional<MutationOutcome>(Mutation,
                                                 const std::function<MutationOutcome()> &)>
        mutate;

    std::unique_ptr<LogOwner> open(QString directory, unsigned days = 14) const;
    static void sweep(LogOwner &);
    static void drain(LogOwner &);
    static void barrier(LogOwner &);
    static std::vector<Notice> notices(LogOwner &);
    static MutationOutcome run_pending(LogOwner &);
    static void settle(LogOwner &, MutationOutcome);
};

} // namespace subathon::server::diagnostics::testing
