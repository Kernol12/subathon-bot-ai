// SPDX-License-Identifier: GPL-3.0-or-later
#include "copy_comparison.hpp"

#include <subathon/server/sqlite_migration_manifest.hpp>

namespace subathon::server::detail {
namespace {

// Exact bytes from the sealed r3 preservation probes. Both select INTEGER 1
// for a mismatch and cap the positive result at one row.
// clang-format off
constexpr std::string_view source_mismatch = R"sql(SELECT 1 FROM main.subathon_plan_revisions AS old LEFT JOIN main.subathon_plan_revisions_rebuild AS new ON old.rowid=new.rowid WHERE new.rowid IS NULL OR NOT ((typeof(old.plan_ref)=typeof(new.plan_ref) AND old.plan_ref IS new.plan_ref COLLATE BINARY) AND (typeof(old.revision)=typeof(new.revision) AND old.revision IS new.revision COLLATE BINARY) AND (typeof(old.parent_revision)=typeof(new.parent_revision) AND old.parent_revision IS new.parent_revision COLLATE BINARY) AND (typeof(old.payment_evidence)=typeof(new.payment_evidence) AND old.payment_evidence IS new.payment_evidence COLLATE BINARY) AND (typeof(old.monetary)=typeof(new.monetary) AND old.monetary IS new.monetary COLLATE BINARY) AND (typeof(old.original_decimal)=typeof(new.original_decimal) AND old.original_decimal IS new.original_decimal COLLATE BINARY) AND (typeof(old.currency)=typeof(new.currency) AND old.currency IS new.currency COLLATE BINARY) AND (typeof(old.resolution_ref)=typeof(new.resolution_ref) AND old.resolution_ref IS new.resolution_ref COLLATE BINARY) AND (typeof(old.missing_rate)=typeof(new.missing_rate) AND old.missing_rate IS new.missing_rate COLLATE BINARY) AND (typeof(old.review)=typeof(new.review) AND old.review IS new.review COLLATE BINARY) AND (typeof(old.evidence_policy)=typeof(new.evidence_policy) AND old.evidence_policy IS new.evidence_policy COLLATE BINARY) AND (typeof(old.policy_ref)=typeof(new.policy_ref) AND old.policy_ref IS new.policy_ref COLLATE BINARY) AND (typeof(old.decision_choice)=typeof(new.decision_choice) AND old.decision_choice IS new.decision_choice COLLATE BINARY) AND (typeof(old.decision_ref)=typeof(new.decision_ref) AND old.decision_ref IS new.decision_ref COLLATE BINARY) AND (typeof(old.decision_actor_ref)=typeof(new.decision_actor_ref) AND old.decision_actor_ref IS new.decision_actor_ref COLLATE BINARY) AND (typeof(old.application)=typeof(new.application) AND old.application IS new.application COLLATE BINARY) AND (typeof(old.reversal)=typeof(new.reversal) AND old.reversal IS new.reversal COLLATE BINARY) AND (typeof(old.settlement)=typeof(new.settlement) AND old.settlement IS new.settlement COLLATE BINARY) AND (typeof(old.valuation_state)=typeof(new.valuation_state) AND old.valuation_state IS new.valuation_state COLLATE BINARY) AND (typeof(old.historical_resolution_ref)=typeof(new.historical_resolution_ref) AND old.historical_resolution_ref IS new.historical_resolution_ref COLLATE BINARY) AND (typeof(old.historical_original_decimal)=typeof(new.historical_original_decimal) AND old.historical_original_decimal IS new.historical_original_decimal COLLATE BINARY) AND (typeof(old.historical_currency)=typeof(new.historical_currency) AND old.historical_currency IS new.historical_currency COLLATE BINARY) AND (typeof(old.comparison_currency)=typeof(new.comparison_currency) AND old.comparison_currency IS new.comparison_currency COLLATE BINARY) AND (typeof(old.effective_rate_decimal)=typeof(new.effective_rate_decimal) AND old.effective_rate_decimal IS new.effective_rate_decimal COLLATE BINARY) AND (typeof(old.fetched_rate_decimal)=typeof(new.fetched_rate_decimal) AND old.fetched_rate_decimal IS new.fetched_rate_decimal COLLATE BINARY) AND (typeof(old.snapshot_ref)=typeof(new.snapshot_ref) AND old.snapshot_ref IS new.snapshot_ref COLLATE BINARY) AND (typeof(old.rate_source_ref)=typeof(new.rate_source_ref) AND old.rate_source_ref IS new.rate_source_ref COLLATE BINARY) AND (typeof(old.rate_timestamp_utc_ms)=typeof(new.rate_timestamp_utc_ms) AND old.rate_timestamp_utc_ms IS new.rate_timestamp_utc_ms COLLATE BINARY) AND (typeof(old.fetch_timestamp_utc_ms)=typeof(new.fetch_timestamp_utc_ms) AND old.fetch_timestamp_utc_ms IS new.fetch_timestamp_utc_ms COLLATE BINARY) AND (typeof(old.manual_decision_ref)=typeof(new.manual_decision_ref) AND old.manual_decision_ref IS new.manual_decision_ref COLLATE BINARY) AND (typeof(old.override_authority)=typeof(new.override_authority) AND old.override_authority IS new.override_authority COLLATE BINARY) AND (typeof(old.override_actor_ref)=typeof(new.override_actor_ref) AND old.override_actor_ref IS new.override_actor_ref COLLATE BINARY) AND (typeof(old.override_reason)=typeof(new.override_reason) AND old.override_reason IS new.override_reason COLLATE BINARY) AND (typeof(old.override_decision_ref)=typeof(new.override_decision_ref) AND old.override_decision_ref IS new.override_decision_ref COLLATE BINARY) AND typeof(new.valuation_format)='integer' AND new.valuation_format=0 AND new.exact_format_ref IS NULL AND new.exact_document IS NULL AND new.exact_snapshot_ref IS NULL) LIMIT 1)sql";
constexpr std::string_view extra_staging_row = R"sql(SELECT 1 FROM main.subathon_plan_revisions_rebuild AS new LEFT JOIN main.subathon_plan_revisions AS old ON old.rowid=new.rowid WHERE old.rowid IS NULL LIMIT 1)sql";
// clang-format on

void require_no_mismatch(const SqlRows &rows) {
    if (rows.isEmpty())
        return;
    if (rows.size() != 1 || rows.front().size() != 1)
        throw MigrationFailure(MigrationReason::inspection_failed);
    const auto &value = rows.front().front();
    const auto type = value.metaType().id();
    if (value.isNull() || (type != QMetaType::LongLong && type != QMetaType::Int) ||
        value.toLongLong() != 1)
        throw MigrationFailure(MigrationReason::inspection_failed);
    throw MigrationFailure(MigrationReason::integrity_failed);
}

} // namespace

void require_v23_copy_match(const std::function<SqlRows(std::string_view)> &read) {
    require_no_mismatch(read(source_mismatch));
    require_no_mismatch(read(extra_staging_row));
}

} // namespace subathon::server::detail
