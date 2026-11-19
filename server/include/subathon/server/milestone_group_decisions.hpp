// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_milestone_groups.hpp>

namespace subathon::server {

// Proposed revision 1 for #236. References are owned, opaque application data.
// The authoritative caller supplies current permission, accepted-input and
// refund facts. These values do not themselves confer authority.
enum class PausedAcceptanceMode { normal, silent };
enum class MilestoneActionChoice { run, defer, skip };
enum class MilestoneRefundPolicy { continue_actions, review, skip };
enum class MilestoneHoldResolution { continue_actions, skip };
enum class MilestoneHoldCause { deferred_acceptance, refund_review };

struct MilestoneDecisionEvidence {
    std::string actor_ref;
    std::string decision_ref;

    bool operator==(const MilestoneDecisionEvidence &) const = default;
};

struct MilestoneAcceptanceChoice {
    MilestoneOccurrenceKey occurrence;
    MilestoneActionChoice choice;

    bool operator==(const MilestoneAcceptanceChoice &) const = default;
};

struct PausedMilestoneAcceptance {
    PausedAcceptanceMode mode;
    MilestoneDecisionEvidence evidence;
    std::vector<MilestoneAcceptanceChoice> choices;
};

struct PausedMilestoneActionPlan {
    PausedMilestoneAcceptance acceptance;
    std::vector<std::string> ordinary_alert_refs;
};

struct MilestoneHoldKey {
    MilestoneHoldCause cause;
    std::string source_ref;

    bool operator==(const MilestoneHoldKey &) const = default;
};

struct MilestoneRefundObservation {
    MilestoneOccurrenceKey occurrence;
    std::string refund_ref;
    std::int64_t score_before_micro_points;
    std::int64_t score_after_micro_points;
    std::int64_t target_micro_points;
    MilestoneRefundPolicy policy{MilestoneRefundPolicy::review};
};

struct MilestoneRefundAudit {
    MilestoneRefundPolicy policy;
    std::uint64_t deficit_micro_points;
    std::optional<MilestoneHoldResolution> resolution;
    std::optional<MilestoneDecisionEvidence> resolution_evidence;
};

struct MilestoneAcceptanceAudit {
    PausedAcceptanceMode mode;
    MilestoneActionChoice choice;
    MilestoneDecisionEvidence evidence;
};

class MilestoneGroupDecisionsNotImplemented final : public std::logic_error {
  public:
    explicit MilestoneGroupDecisionsNotImplemented(const char *entry)
        : std::logic_error(std::string("MILESTONE-005 group decisions NI: ") + entry) {}
};

// The caller supplies the affected occurrences from its revalidated preview
// and alert refs already classified as ordinary messages, sounds or visuals.
// Require one explicit choice for each occurrence before planning any effects.
[[nodiscard]] inline std::optional<PausedMilestoneActionPlan>
plan_paused_milestone_actions(const PausedMilestoneAcceptance &,
                              const std::vector<MilestoneOccurrenceKey> &,
                              const std::vector<std::string> &) {
    throw MilestoneGroupDecisionsNotImplemented("plan_paused_milestone_actions");
}

// Provisional operations on the caller's active writer session. The caller
// publishes only after normal committed completion. Release, delivery claims
// and bonus receipts continue through the exact v12 occurrence boundary.
class SqliteMilestoneGroupDecisions final {
  public:
    explicit SqliteMilestoneGroupDecisions(SqliteWorkSession session)
        : session_(std::move(session)) {}

    // Stage after the originating bundle, delivery captures and group batch,
    // on the same session. Retain the original choices and their evidence.
    void record_acceptance(const ApplicationOperationKey &,
                           const PausedMilestoneAcceptance &) const {
        throw MilestoneGroupDecisionsNotImplemented("record_acceptance");
    }

    // Stage against current authoritative exact score/target and persisted
    // action state in the work that applies the refund. The caller supplies
    // the refund identity, rather than asking this store to calculate credit.
    void observe_refund(const MilestoneRefundObservation &) const {
        throw MilestoneGroupDecisionsNotImplemented("observe_refund");
    }

    // Select one exact active cause on one occurrence. An authorized Continue
    // clears that cause. Skip records the selection and bars pending actions.
    [[nodiscard]] bool resolve(const MilestoneOccurrenceKey &, const MilestoneHoldKey &,
                               MilestoneHoldResolution, const MilestoneDecisionEvidence &) const {
        throw MilestoneGroupDecisionsNotImplemented("resolve");
    }

    [[nodiscard]] std::vector<MilestoneHoldKey> active_holds(const MilestoneOccurrenceKey &) const {
        throw MilestoneGroupDecisionsNotImplemented("active_holds");
    }

    [[nodiscard]] std::optional<MilestoneAcceptanceAudit>
    acceptance_audit(const MilestoneOccurrenceKey &) const {
        throw MilestoneGroupDecisionsNotImplemented("acceptance_audit");
    }

    [[nodiscard]] std::optional<MilestoneRefundAudit> refund_audit(const MilestoneOccurrenceKey &,
                                                                   const std::string &) const {
        throw MilestoneGroupDecisionsNotImplemented("refund_audit");
    }

  private:
    SqliteWorkSession session_;
};

} // namespace subathon::server
