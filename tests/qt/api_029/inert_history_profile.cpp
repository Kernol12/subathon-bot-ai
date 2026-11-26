// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/connector_history_profile.hpp>

namespace subathon::server::connector_intake {
HandoffOutcome commit_history_handoff(SqliteWriter &, const OwnerInputs &,
                                      const QList<const HistoryProfile *> &,
                                      const AdmittedHandoff &, const QList<CutObservation> &,
                                      const HandoffValidators &, const std::function<bool()> &) {
    return {};
}

IntakeStatus open_history_gap(SqliteWriter &, const OwnerInputs &, const HistoryProfile &,
                              const GapOpening &, const OpenGap &, const GapProducer &) {
    return IntakeStatus::not_implemented;
}

IntakeStatus begin_history_catch_up(SqliteWriter &, const OwnerInputs &, const HistoryProfile &,
                                    const AdmittedHistoryCut &, const CutObservation &,
                                    const CutWitness &) {
    return IntakeStatus::not_implemented;
}

IntakeStatus open_history_import(SqliteWriter &, const OwnerInputs &, const HistoryProfile &,
                                 const AdmittedHistoryCut &, const HistoryRequest &,
                                 const CutObservation &, const CutWitness &) {
    return IntakeStatus::not_implemented;
}

PageQualification qualify_history_page(SqliteWriter &, const OwnerInputs &, const HistoryProfile &,
                                       const AdmittedHistoryRequest &, const Page &) {
    return {};
}

PageQualification reconstruct_history_page(SqliteWriter &, const OwnerInputs &,
                                           const HistoryProfile &, const RetainedPageRequest &) {
    return {};
}

PageOutcome bind_history_page(SqliteWriter &, const OwnerInputs &, const QualifiedPage &) {
    return {};
}

PageOutcome finalize_history_page(SqliteWriter &, const OwnerInputs &, const QualifiedPage &) {
    return {};
}
} // namespace subathon::server::connector_intake
