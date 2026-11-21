// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "catalog_fixture.hpp"

namespace storage_026_fixture {

// Stage 1's linkable, test-only entry points. Future owning adapters must replace
// these calls. The stubs own no persistence, never claim a COMMIT, and cannot
// parse or manufacture a durable result.
enum class Stop {
    not_implemented,
    committed,
    known_rollback,
    exact_replay,
    different_ordinal,
    different_facts,
    different_configuration,
    different_context,
    intake_limit,
    quarantine_full,
    pending_original,
    inconsistent_original,
    may_create,
    may_link,
    stale,
    conflict,
    busy,
    incomplete_prefix
};
struct ManifestMissing {};
inline std::variant<ManifestMissing, MigrationManifest> compiled_connector_manifest() {
    return ManifestMissing{};
}

struct ManagedInput {
    QByteArray intake_ref;
    core::ReceiptInput receipt;
    VerifiedActivityLink activity;
    QByteArray capture;
};
struct AttemptLocator {
    core::ReceiptId receipt;
    qint64 ordinal;
    bool operator==(const AttemptLocator &) const = default;
};
struct ManagedResult {
    Stop stop;
    std::optional<AttemptLocator> attempt;
    std::optional<core::ReceiptAttempt> selected;
};
inline ManagedResult stage_managed(const SqliteWorkSession &, const ManagedInput &,
                                   const std::function<bool()> & = {}) {
    return {Stop::not_implemented, {}, {}};
}
inline ManagedResult read_attempt(const SqliteWorkSession &, const ManagedInput &, AttemptLocator) {
    return {Stop::not_implemented, {}, {}};
}

struct EvidenceSelection {
    Stop stop;
    std::optional<AttemptLocator> selected_basis;
    std::optional<core::ContributionId> original;
    std::optional<QByteArray> capture;
};
inline EvidenceSelection select_evidence(const SqliteWorkSession &, const ManagedInput &) {
    return {Stop::not_implemented, {}, {}, {}};
}

struct QuarantineInput {
    QByteArray installation, provider, account, stream, key;
    QByteArray normalized, provenance;
};
inline Stop admit_quarantine(const SqliteWorkSession &, const QuarantineInput &) {
    return Stop::not_implemented;
}

struct SourceChange {
    QByteArray installation, provider, account, stream;
    qint64 expected_epoch;
    QByteArray expected_generation;
    qint64 expected_source_revision, expected_policy_revision;
    QByteArray new_generation, new_policy, new_member, authorized_cut;
};
inline Stop switch_source_policy(const SqliteWorkSession &, const SourceChange &) {
    return Stop::not_implemented;
}

struct OperationIntent {
    QByteArray installation, provider, account, stream, operation_ref;
    qint64 epoch, policy_revision;
    QByteArray generation, initial_continuation, local_date, zone_id, zone_basis;
    qint64 resolved_utc_start;
};
inline Stop bind_history_operation(const SqliteWorkSession &, const OperationIntent &) {
    return Stop::not_implemented;
}

struct PageBinding {
    QByteArray operation_ref;
    qint64 source_revision;
    qint64 operation_revision;
    QByteArray expected_continuation;
    QByteArray page;
    qint64 ordinal;
    qint64 items;
    QByteArray next_continuation;
    QByteArray frontier_before;
    QByteArray frontier_after;
    QByteArray qualified_proof;
};
inline Stop finalize_page(const SqliteWorkSession &, const PageBinding &) {
    return Stop::not_implemented;
}

} // namespace storage_026_fixture
