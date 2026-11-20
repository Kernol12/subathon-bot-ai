// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <span>
#include <subathon/server/sqlite_financial_refunds.hpp>

namespace subathon::server {

namespace review_detail {

// Both values come from owning validated reads in the current review row BEGIN,
// before and after application staging respectively.
struct LinkedReviewReads {
    std::span<const ApplicationHoldView> ordinary;
    const RecordedApplicationBundle &application;
};

[[nodiscard]] FinancialWorkReply
stage_review_linked_resolution(const SqliteWorkSession &, const ApplicationHoldResolutionBatch &,
                               const LinkedReviewReads &);

} // namespace review_detail

// Called exclusively after the financial boundary checks the complete mixed set.
// The shipped hold mutation stores the ordinary projection, including its link.
[[nodiscard]] ApplicationHoldMutationResult
stage_checked_ordinary_resolution(const SqliteWorkSession &, const ApplicationHoldResolutionBatch &,
                                  const review_detail::LinkedReviewReads * = nullptr);

} // namespace subathon::server
