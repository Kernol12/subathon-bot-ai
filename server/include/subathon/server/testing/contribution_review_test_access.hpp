// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server::testing {

enum class ReviewUnit { initial_read, qualification_read, row, final_read };
enum class ReviewUnitPhase {
    before_enqueue,
    began,
    before_commit,
    after_commit,
    completed,
    published
};

struct ReviewUnitEvent {
    ReviewUnit unit;
    ReviewUnitPhase phase;
    int ordinal{};                        // zero for a whole-selection read
    const SqliteWorkResult *completion{}; // only during the real writer completion
};

// Optional acceptance observer. Announces the real boundaries in owner order.
// The caller may throw at before_commit (after staging/readback) or after_commit
// (after the real acknowledgement). It may arm the existing writer seam during
// before_enqueue. No SQL, commit capability, or fabricated outcomes pass here.
using ContributionReviewObserver = std::function<void(const ReviewUnitEvent &)>;

// Owner-only state shared by the observer copies held by the review attempt.
struct FinalReviewAllocationFault {
    bool armed{};
    unsigned hits{};
};

struct FinalReviewAllocationObserver {
    ContributionReviewObserver forward;
    std::shared_ptr<FinalReviewAllocationFault> fault;

    void operator()(const ReviewUnitEvent &event) const { forward(event); }
};

} // namespace subathon::server::testing
