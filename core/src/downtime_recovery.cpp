// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/downtime_recovery.hpp>

#include <stdexcept>

namespace subathon::core {

std::variant<DowntimeRecoveryPreview, RecoveryPreviewRefusal>
preview_downtime_recovery(const DowntimeRecoveryInput &) {
    throw std::logic_error("CORE-006 NI: preview_downtime_recovery");
}

std::variant<RecoveryReviewWait, RecoveryWaitRefusal>
observe_recovery_review_wait(const RecoveryReviewSnapshot &) {
    throw std::logic_error("CORE-006 NI: observe_recovery_review_wait");
}

std::variant<RecoveryReviewResolution, RecoveryReviewRefusal, RecoveryCorrectionRefusal>
resolve_recovery_review(const DowntimeRecoveryInput &, const RecoveryReviewSnapshot &,
                        const RecoveryReviewCommand &, const RecoveryReviewAccess &) {
    throw std::logic_error("CORE-006 NI: resolve_recovery_review");
}

} // namespace subathon::core
