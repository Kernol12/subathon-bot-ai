// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/modifier_clock.hpp>

namespace subathon::core {

ModifierCutProposal project_modifier_cut(const ModifierClockObservation &,
                                         const ModifierClockObservation &, const TimerState &,
                                         const TimerClockStep &, const TimerAdvance &,
                                         const std::optional<ModifierClockValue> &,
                                         const std::optional<ModifierActivationAtCut> &) {
    throw ModifierClockNotImplemented("modifier clock cut is intentionally unimplemented");
}

ModifierRecoveryProjection project_modifier_restart(const DowntimeRecoveryInput &,
                                                    const DowntimeRecoveryPreview &,
                                                    const ModifierClockValue &) {
    throw ModifierClockNotImplemented("modifier restart projection is intentionally unimplemented");
}

ModifierRecoveryProjection project_modifier_review_wait(const RecoveryReviewSnapshot &,
                                                        const RecoveryReviewSnapshot &,
                                                        const RecoveryReviewWait &,
                                                        const TimerClockStep &,
                                                        const ModifierClockValue &) {
    throw ModifierClockNotImplemented("modifier review wait is intentionally unimplemented");
}

ModifierRecoveryProjection project_modifier_review_resolution(
    const DowntimeRecoveryInput &, const RecoveryReviewSnapshot &,
    const std::variant<RecoveryReviewResolution, RecoveryReviewRefusal, RecoveryCorrectionRefusal>
        &,
    const ModifierClockValue &, const ModifierClockValue &) {
    throw ModifierClockNotImplemented("modifier review resolution is intentionally unimplemented");
}

ModifierScoreBasis capture_modifier_score(const ModifierScoreArrival &, const TimerState &,
                                          const PauseState &,
                                          const std::optional<ModifierClockValue> &) {
    throw ModifierClockNotImplemented("modifier score capture is intentionally unimplemented");
}

ModifierScoreProposal request_modifier_score(const ModifierScoreBasis &) {
    throw ModifierClockNotImplemented("modifier score request is intentionally unimplemented");
}

} // namespace subathon::core
