// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "common_fixture.hpp"

#include <subathon/server/reference_capture_values.hpp>

namespace storage_026_fixture {

inline std::pair<ManagedResult, SqliteWorkResult>
manage(CatalogFile &file, const ManagedInput &input,
       const std::function<bool()> &before_commit = {}) {
    auto result = std::make_shared<std::optional<ManagedResult>>();
    const auto work = qt_test_support::run_work(
        file.writer(),
        [result, input, before_commit](auto ctx) -> QVariant {
            const auto s = ctx.begin();
            result->emplace(stage_managed(s, input, before_commit));
            if (result->value().stop == Stop::not_implemented)
                s.rollback();
            // On adoption the managed port owns the receipt transaction.
            return 1;
        },
        10000);
    require(result->has_value(), "Managed port reached after real BEGIN");
    if (result->value().stop == Stop::not_implemented)
        require(work.transaction == TransactionOutcome::rolled_back &&
                    work.error == WorkError::not_committed && !work.value,
                "Missing managed port returns only an explicit rolled-back first stop");
    return {std::move(result->value()), work};
}

inline ReferenceRule capture_rule() {
    return {"R", "r", "a", "b", "T", 600000, {ApplicationPrincipalKind::automation, "I", "U"}, "K"};
}

inline constexpr auto receipt_namespace = "00000001530000000152000000014e";
inline ManagedInput managed(std::string ref, qint64 arrival, QByteArray capture) {
    const auto ns = QByteArray::fromHex(receipt_namespace).toStdString();
    return {QByteArray::fromStdString(ref),
            {{{"P", "A"}, ns, ref},
             {"support", 1, 45, {}, {}, core::ReceiptMoney{"20.00", "EUR"}, {}, {}},
             {arrival, "connection", "cfg", "ctx"}},
            {{{"P", "A"}, "S/link", "K"}, "verify"},
            std::move(capture)};
}

inline core::ReceiptId legacy_receipt(CatalogFile &file, const ManagedInput &input,
                                     qsizetype attempts = 1) {
    std::optional<core::ReceiptId> id;
    const auto work = qt_test_support::run_work(
        file.writer(),
        [&](auto ctx) {
            SqliteReceiptRepository repo(ctx);
            auto unit = repo.begin();
            require(!unit->find(input.receipt.identity), "Synthetic receipt key starts unoccupied");
            id = unit->create(input.receipt.identity, input.receipt.facts, input.receipt.arrival);
            for (qsizetype ordinal = 1; ordinal <= attempts; ++ordinal) {
                auto arrival = input.receipt.arrival;
                arrival.server_timestamp_utc_ms += ordinal - 1;
                unit->append_attempt(*id, {arrival, input.receipt.facts,
                                           ordinal == 1 ? core::ReceiptDisposition::created
                                                        : core::ReceiptDisposition::retry});
            }
            unit->commit();
            return 1;
        },
        10000);
    finance::normal(work);
    require(id.has_value(), "Real legacy receipt unit committed the selected receipt");
    require(file.rows("SELECT count(*),min(ordinal),max(ordinal) "
                      "FROM main.subathon_receipt_attempts WHERE receipt_id=?",
                      {blob8(id->value)}) == SqlRows{{attempts, 1, attempts}},
            "Independent physical ordinal range is contiguous by PK and exact count");
    return *id;
}

inline void entry_anchor(const SqliteWorkSession &s, const ManagedInput &input, core::ReceiptId id,
                         qint64 ordinal) {
    (void)s.execute(
        {"INSERT INTO main.subathon_intake_entries("
         "installation,provider,account,stream,intake_ref,epoch,generation,policy_revision,"
         "input,provenance,arrival_ms,configuration_ref,modifier_context_ref,"
         "activity_namespace,activity_id,verification_ref,phase,disposition,receipt_id,"
         "attempt_ordinal) VALUES(?,?,?,?,?,1,?,1,?,?,?,?,?,?,?,?,1,0,?,?)",
         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
          input.intake_ref, QByteArray("g3"), QByteArray("observation"), input.capture,
          qint64(input.receipt.arrival.server_timestamp_utc_ms),
          QByteArray::fromStdString(input.receipt.arrival.configuration_ref),
          QByteArray::fromStdString(input.receipt.arrival.modifier_context_ref),
          QByteArray("S/link"), QByteArray("K"), QByteArray("verify"), blob8(id.value), ordinal}});
}

inline void peer_anchors(CatalogFile &file, const ManagedInput &first, core::ReceiptId first_id,
                         const ManagedInput &peer, core::ReceiptId peer_id, bool retain_basis = true) {
    file.write([&](const auto &s) {
        // Only owned new relations get literal rows. The receipt IDs and
        // attempts come from independently committed ACCT-001 transactions.
        entry_anchor(s, first, first_id, 1);
        entry_anchor(s, peer, peer_id, 1);
        if (retain_basis)
            (void)s.execute({"INSERT INTO main.subathon_intake_basis VALUES(?,?,?,?,?,?,?,?,1,?)",
                             {QByteArray("install"), QByteArray("P"), QByteArray("A"),
                              QByteArray("S/link"), QByteArray("K"), QByteArray("S"),
                              first.intake_ref, blob8(first_id.value), first.capture}});
    });
}

inline std::pair<std::optional<ContributionLinkResult>, SqliteWorkResult>
evidence_work(CatalogFile &file, const ManagedInput &input, core::ReceiptId id,
              bool abnormal = false) {
    std::optional<ContributionLinkResult> result;
    const auto work = qt_test_support::run_work(
        file.writer(),
        [&](auto ctx) {
            core::ContributionConditions conditions;
            conditions.valuation = {true, input.receipt.facts.money, "quote", false};
            conditions.review = core::ContributionReview::pending;
            conditions.evidence_admission = {
                core::EvidencePolicy::accept_provider_report, "review-policy", {}};
            conditions.settlement = core::SettlementGate::clear;
            SqliteContributionRepository repo(ctx);
            result = ContributionEvidenceService(repo).observe(
                {{id, input.receipt.identity, input.receipt.facts, input.receipt.arrival},
                 ContributionObservationKind::support,
                 core::PaymentEvidence::confirmed,
                 input.activity,
                 conditions});
            if (abnormal)
                throw std::runtime_error("Abnormal completion after real owning evidence COMMIT");
            return 1;
        },
        10000);
    return {std::move(result), work};
}

inline ContributionLinkResult evidence(CatalogFile &file, const ManagedInput &input,
                                       core::ReceiptId id) {
    auto [result, work] = evidence_work(file, input, id);
    finance::normal(work);
    require(result.has_value(), "ACCT-002 owns its separate evidence COMMIT");
    return *result;
}

} // namespace storage_026_fixture
