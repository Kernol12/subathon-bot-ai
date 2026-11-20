// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_writer_access.hpp"

#include <subathon/server/sqlite_application_bundle.hpp>

namespace subathon::server::review_detail {

// Private reader for the single immutable preview transaction. Each selector
// still goes through the owning plan and contribution decoders.
using PreviewInputs = std::function<ApplicationBundleInputResult(const ApplicationPlanKey &,
                                                                 detail::ReviewReadCost *)>;
[[nodiscard]] PreviewInputs preview_inputs(const SqliteWorkSession &);
[[nodiscard]] ApplicationBundleInputResult
live_review_inputs(const SqliteWorkSession &, const ApplicationPlanKey &, detail::ReviewReadCost &);

using BundleHistory =
    std::function<std::optional<RecordedApplicationBundle>(const ApplicationOperationKey &)>;

// One private, read-only snapshot window. Adjacent original operations can
// share a validated state token. Actual effects always come from owning reads.
[[nodiscard]] BundleHistory bundle_history(const SqliteWorkSession &);

} // namespace subathon::server::review_detail
