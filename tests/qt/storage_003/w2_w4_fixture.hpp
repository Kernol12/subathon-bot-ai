// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// #834 W2–W4 typed seed data, unit limits and the five fixed fact ports.
// Literals are independent field-by-field encodings carried from the reviewed
// inert assembly r2; no product codec produced any expected value here.
#include "../../../server/src/sqlite_clock_checkpoint_fixture.hpp"
#include "../support/application_port_fake.hpp"
#include "protected_predecessor_fixture.hpp"

#include <subathon/server/contribution_evidence.hpp>
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

#include <string>
#include <vector>

namespace storage_003_w2_w4 {
using namespace storage_003_fixture;
namespace core = subathon::core;

// Unit limits come from shipped declarations, never from runtime measurement
// (correction 01, item 2). The migration unit keeps the reviewed W1 profile.
// Every other unit opens its writer with the shipped writer declaration and
// passes each owner its shipped default limits. The asserts pin the sources.
inline constexpr StorageLimits unit_writer_limits{64, 64, 65536, 1048576, 1024, 1048576};
static_assert(unit_writer_limits.pending_batches == StorageLimits{}.pending_batches &&
              unit_writer_limits.statements_per_batch == StorageLimits{}.statements_per_batch &&
              unit_writer_limits.statement_utf8_bytes == StorageLimits{}.statement_utf8_bytes &&
              unit_writer_limits.bound_value_bytes == StorageLimits{}.bound_value_bytes &&
              unit_writer_limits.result_rows == StorageLimits{}.result_rows &&
              unit_writer_limits.result_value_bytes == StorageLimits{}.result_value_bytes);
inline constexpr RetainedLimits context_limits{4096, 262144};
static_assert(context_limits.reference_bytes == RetainedLimits{}.reference_bytes &&
              context_limits.document_bytes == RetainedLimits{}.document_bytes);
inline constexpr SqliteReceiptLimits receipt_limits{4096, 262144, 1024};
static_assert(receipt_limits.reference_bytes == SqliteReceiptLimits{}.reference_bytes &&
              receipt_limits.value_bytes == SqliteReceiptLimits{}.value_bytes &&
              receipt_limits.attempts_per_find == SqliteReceiptLimits{}.attempts_per_find);
inline constexpr SqliteContributionLimits contribution_limits{4096, 262144, 1024};
static_assert(contribution_limits.reference_bytes == SqliteContributionLimits{}.reference_bytes &&
              contribution_limits.value_bytes == SqliteContributionLimits{}.value_bytes &&
              contribution_limits.unresolved_per_lookup ==
                  SqliteContributionLimits{}.unresolved_per_lookup);
inline constexpr SqlitePlanLimits plan_limits{4096, 262144, 16384, 1024, 1024, 4096};
static_assert(plan_limits.reference_bytes == SqlitePlanLimits{}.reference_bytes &&
              plan_limits.value_bytes == SqlitePlanLimits{}.value_bytes &&
              plan_limits.component_bytes == SqlitePlanLimits{}.component_bytes &&
              plan_limits.revisions_per_full_find == SqlitePlanLimits{}.revisions_per_full_find &&
              plan_limits.items_per_vector == SqlitePlanLimits{}.items_per_vector &&
              plan_limits.items_per_full_find == SqlitePlanLimits{}.items_per_full_find);
inline constexpr SqliteBundleLimits bundle_limits{4096, 262144, 16384, 1024,  1024,  1024,
                                                  1024, 1024,   1024,  12288, 24576, 1048576};
static_assert(bundle_limits.reference_bytes == SqliteBundleLimits{}.reference_bytes &&
              bundle_limits.value_bytes == SqliteBundleLimits{}.value_bytes &&
              bundle_limits.component_bytes == SqliteBundleLimits{}.component_bytes &&
              bundle_limits.balances_per_state == SqliteBundleLimits{}.balances_per_state &&
              bundle_limits.plan_items_per_vector == SqliteBundleLimits{}.plan_items_per_vector &&
              bundle_limits.effects_per_bundle == SqliteBundleLimits{}.effects_per_bundle &&
              bundle_limits.jobs_per_bundle == SqliteBundleLimits{}.jobs_per_bundle &&
              bundle_limits.transitions_per_bundle == SqliteBundleLimits{}.transitions_per_bundle &&
              bundle_limits.links_per_bundle == SqliteBundleLimits{}.links_per_bundle &&
              bundle_limits.input_items == SqliteBundleLimits{}.input_items &&
              bundle_limits.result_items == SqliteBundleLimits{}.result_items &&
              bundle_limits.owned_bytes == SqliteBundleLimits{}.owned_bytes);
// The service units use the default ApplicationServiceLimits; its nested
// bundle ceilings are the pinned bundle_limits above.

inline constexpr int timeout_ms = 10000;

// SAS1/SAC1/SAR1/SAA1 envelopes. A: no basis, K/L/O/P, UTC 0, no segments.
// D and B: basis A, K/L/N/P, UTC 250 and one unpaused +250 ms segment.
// All are Active/Main with Grace 0, no wait, debt 0 and no balances.
inline const QByteArray sas_a =
    QByteArray::fromHex("53415331000000014b00000000014c000000014f00000000000000000000000000000001"
                        "5001000000000000002710000000000000000000000000000000000000000000");
inline const QByteArray sas_d =
    QByteArray::fromHex("53415331000000014b010000000141000000014c000000014e00000000000000fa000000"
                        "0100000000000000fa00000000000000fa00000000015001000000000000002616000000"
                        "000000000000000000000000000000000000");
inline const QByteArray sas_b =
    QByteArray::fromHex("53415331000000014b010000000141000000014c000000014e00000000000000fa000000"
                        "0100000000000000fa00000000000000fa0000000001500100000000000000269f000000"
                        "000000000000000000000000000000000000");
inline const QByteArray sac =
    QByteArray::fromHex("534143310000000153000000015100000000000000000101000000000000000000000001"
                        "460000000000000001000000000149000000015500010100000001410100000001550000"
                        "0000000100000001430000000159");
inline const QByteArray sar =
    QByteArray::fromHex("534152310000005653414331000000015300000001510000000000000000010100000000"
                        "000000000000000146000000000000000100000000014900000001550001010000000141"
                        "010000000155000000000001000000014300000001590000000141000000014400000001"
                        "420000000001000000000000000101000000000331333700000001310000000000000089"
                        "000000013000000001310000000000000a66697865642d6d61696e000000030000000000"
                        "000000890001000000000000000000050000000000000000000000000000000000");
inline const QByteArray saa =
    QByteArray::fromHex("534141310000005653414331000000015300000001510000000000000000010100000000"
                        "000000000000000146000000000000000100000000014900000001550001010000000141"
                        "01000000015500000000000100000001430000000159000000014b000000015000000001"
                        "480000000000000000000000000000000000015600000006616374697665000000014c00"
                        "0000014e00000000000000fa0000000100000000000000fa00000000000000fa00010000"
                        "00015500000000000146000000000000000100000000000000000000076e6f2d6a6f6273");
// Contradictory root copies: only the operation ref (Q to Z) or only the
// before ref (A to B) differs from structural_fixture's applied_root.
inline const QByteArray wrong_operation_root =
    QByteArray::fromHex("0000000035010000000153000000015a0000000141000000014200000000000000010000"
                        "00000000000100000000000000020000000000000002");
inline const QByteArray wrong_before_root =
    QByteArray::fromHex("000000003501000000015300000001510000000142000000014200000000000000010000"
                        "00000000000100000000000000020000000000000002");

inline const std::string state_format = "subathon.internal.application-state.v1";

// Initial owner unit: K, SAS1 A/1/1 and the J root, slot and head.
inline const RetainedDocument configuration{
    {RetainedKind::configuration, "K"}, "configuration", {}};
inline const ApplicationStateSnapshot initial_state{
    "A", {state_format, sas_a},
    "K", {core::TimerLifecycle::Active, core::CountdownPhase::Main, original_main, 0, false},
    0,   {}};
inline const detail::ClockInitialCheckpoint initial_checkpoint{"I",  "T", "L",           {"S", "J"},
                                                               1000, 0,   slot_document, seed_root};

// Source units: context X tied to K, original receipt 1, contribution 0, plan F/1.
inline const RetainedDocument arrival_context{
    {RetainedKind::arrival_context, "X"}, "arrival", ConfigurationReference{"K"}};
inline const core::ReceiptId receipt_id{1};
inline const core::ContributionId contribution_id{0};
inline const core::ReceiptInput receipt_input{{{"synthetic", "account"}, "event", "one"},
                                              {"nonmoney", 1, 0, {}, {}, {}, {}, {}},
                                              {0, "C", "K", "X"}};
inline const ContributionReceipt original{receipt_id, receipt_input.identity, receipt_input.facts,
                                          receipt_input.arrival};
inline const core::ContributionState unapplied{
    core::PaymentEvidence::not_applicable,
    {{false, {}, {}, false},
     core::ContributionReview::not_required,
     {core::EvidencePolicy::require_evidence_review, "G", {}},
     core::ContributionApplication::not_applied,
     core::ContributionReversal::none,
     core::SettlementGate::clear}};
inline const ContributionObservation support{original,
                                             ContributionObservationKind::support,
                                             core::PaymentEvidence::not_applicable,
                                             {},
                                             unapplied.conditions};
inline const core::CapturedPlanRevision captured{
    1,
    {},
    {{"E",
      {},
      "E",
      receipt_id,
      receipt_input.identity,
      receipt_input.facts,
      receipt_input.arrival,
      contribution_id},
     unapplied,
     {core::PlanValuationState::not_applicable, {}},
     {{"A1", "v", "R", "T", core::RequestedUnit::milliseconds, false, core::Exact(137), 137,
       "fixed-main", "rev"}}},
    {{"R", "v", true, "selected"}},
    {},
    {}};

// Operation S/Q for installation I, principal U, with bound choice C/Y.
inline const ApplicationOperationKey application_key{"S", "Q"};
inline const ApplicationOriginalClaimSelector source{ApplicationSubjectKind::contribution,
                                                     receipt_id, contribution_id};
inline const ApplicationPlanKey plan{"F", 1};
inline const OriginalApplicationCommand command{application_key,
                                                source.subject_kind,
                                                receipt_id,
                                                contribution_id,
                                                plan,
                                                {ApplicationPrincipalKind::account, "I", "U"},
                                                OriginalApplicationPurpose::captured_original,
                                                ApplicationPreviewMode::require_exact,
                                                std::string("A"),
                                                std::string("U"),
                                                {},
                                                {},
                                                {{"C", "Y"}}};
inline const BoundApplicationPolicy policy{
    application_key,
    source,
    plan,
    "A",
    "K",
    "H",
    "P",
    {},
    {core::TimerEndMode::Immediate, 0, core::GraceReuse::Fresh, {}},
    false,
    false,
    "V",
    "active",
    {ApplicationPolicyAdmission::allow_active, {}},
    {}};
// Clock facts bind A and lineage L to one unpaused 250 ms segment.
inline const ApplicationClockObservation clock_facts{
    "L", "N", "K", "P", "A", qualified_elapsed, {{qualified_elapsed, qualified_elapsed, false}}};

// The five fixed fact ports. Each call records its name; disclosure is the
// only configurable answer. No port derives a bundle or a result.
class Ports final : public qt_test_support::ApplicationPortFake {
  public:
    using Trace = std::vector<std::string>;
    explicit Ports(bool disclose = true) : disclose_(disclose) {}
    const Trace &trace() const { return trace_; }

    AccessDecision check(const ApplicationReferenceReadView &,
                         const OriginalApplicationCommand &candidate) override {
        trace_.push_back("access");
        require(candidate.principal.kind == ApplicationPrincipalKind::account &&
                    candidate.principal.installation_ref == "I" &&
                    candidate.principal.principal_ref == "U" &&
                    candidate.operation.operation_scope == "S" &&
                    candidate.operation.operation_ref == "Q",
                "Only the fixed synthetic I/U principal submits S/Q");
        return ApplicationAccessAllowed{"H"};
    }
    bool disclose_application(const ApplicationReferenceReadView &,
                              const OriginalApplicationCommand &,
                              const RecordedApplicationBundle &) override {
        trace_.push_back("disclose");
        return disclose_;
    }
    bool disclose_holds(const ApplicationReferenceReadView &, const OriginalApplicationCommand &,
                        const ApplicationHoldActiveSet &) override {
        trace_.push_back("holds");
        return false;
    }
    PolicyResolution resolve(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &,
                             const ApplicationBundleInputs &inputs,
                             const AccessDecision &access) override {
        trace_.push_back("policy");
        require(inputs.current_state.token == "A" &&
                    inputs.current_state.configuration_ref == "K" &&
                    std::get<ApplicationAccessAllowed>(access).permission_revision_ref == "H",
                "Policy binds this specimen's A/K and the allowed access");
        return policy;
    }
    ClockResolution observe(const ApplicationReferenceReadView &,
                            const OriginalApplicationCommand &,
                            const ApplicationBundleInputs &inputs,
                            const BoundApplicationPolicy &selected) override {
        trace_.push_back("clock");
        require(inputs.current_state.token == "A" && selected.policy_revision_ref == "P",
                "Clock facts bind A and P");
        return clock_facts;
    }
    ActionResolution prepare(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                             const BoundApplicationPolicy &,
                             const ApplicationCalculatedProposal &proposal) override {
        trace_.push_back("actions");
        require(proposal.clock_advanced_timer.main_remaining_ms == advanced_main &&
                    proposal.timer_after.main_remaining_ms == applied_main &&
                    proposal.timer_after.grace_remaining_ms == 0 && proposal.debt_after == 0 &&
                    proposal.effects.size() == 1 && proposal.transitions.empty(),
                "The real reducer reaches D 9,750 and B 9,887 with Grace and debt 0");
        return PreparedApplicationActions{{}, "no-jobs"};
    }
    ReferenceResolution issue(ApplicationReferencePurpose purpose,
                              const ApplicationReferenceBinding &binding) override {
        trace_.push_back(purpose == ApplicationReferencePurpose::advanced_state ? "advanced"
                                                                                : "after");
        require(
            binding.operation.operation_scope == "S" && binding.operation.operation_ref == "Q" &&
                binding.source.subject_kind == source.subject_kind &&
                binding.source.receipt_id == receipt_id &&
                binding.source.contribution_id == contribution_id && binding.plan.plan_ref == "F" &&
                binding.plan.revision == 1 && binding.persisted_head_token == "A" &&
                binding.purpose == purpose && !binding.request_ordinal &&
                (purpose == ApplicationReferencePurpose::advanced_state ||
                 purpose == ApplicationReferencePurpose::after_state),
            "Only D and B are issued for the fixed typed operation");
        return IssuedApplicationReference{
            binding, purpose == ApplicationReferencePurpose::advanced_state ? "D" : "B"};
    }

  private:
    bool disclose_;
    Trace trace_;
};

inline const Ports::Trace fresh_path{"access", "policy", "clock", "actions", "advanced", "after"};
inline const Ports::Trace history_path{"access", "disclose"};
// Brief P1: the closure fence runs after lookup and before disclosure.
inline const Ports::Trace fenced_path{"access"};

} // namespace storage_003_w2_w4
