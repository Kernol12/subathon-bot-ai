// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../support/contribution_review_fixture.hpp"

namespace storage_026_fixture {
using namespace contribution_review_fixture;

inline constexpr auto successor_id = "connector-intake-storage-v1";

// Exact ordered fixture delta. Historical locators FK the stable source and
// retained policy. A mutable generation or epoch is never a source parent key.
inline const QList<QString> delta_sql{
    R"SQL(CREATE TABLE subathon_intake_policies(
installation BLOB NOT NULL REFERENCES subathon_account_installation(installation_ref),
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 16384),
PRIMARY KEY(installation,revision)))SQL",
    R"SQL(CREATE TABLE subathon_intake_members(
installation BLOB NOT NULL, policy_revision INTEGER NOT NULL,
provider BLOB NOT NULL, account BLOB NOT NULL, stream BLOB NOT NULL,
award_class BLOB NOT NULL, effect_scope BLOB NOT NULL, overlap_set BLOB NOT NULL,
primary_slot INTEGER CHECK(primary_slot IS NULL OR (typeof(primary_slot)='integer' AND primary_slot=1)),
document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 16384),
PRIMARY KEY(installation,policy_revision,provider,account,stream,award_class,effect_scope),
UNIQUE(installation,policy_revision,overlap_set,effect_scope,primary_slot),
FOREIGN KEY(installation,policy_revision) REFERENCES subathon_intake_policies(installation,revision),
CHECK(typeof(provider)='blob' AND typeof(account)='blob' AND typeof(stream)='blob'
  AND typeof(award_class)='blob' AND typeof(effect_scope)='blob' AND typeof(overlap_set)='blob')))SQL",
    R"SQL(CREATE TABLE subathon_intake_sources(
installation BLOB NOT NULL, provider BLOB NOT NULL, account BLOB NOT NULL, stream BLOB NOT NULL,
epoch INTEGER NOT NULL CHECK(typeof(epoch)='integer' AND epoch>0),
generation BLOB NOT NULL CHECK(typeof(generation)='blob' AND length(generation) BETWEEN 1 AND 4096),
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
policy_revision INTEGER NOT NULL, baseline_state INTEGER NOT NULL CHECK(baseline_state IN (0,1,2)),
cut BLOB CHECK(cut IS NULL OR (typeof(cut)='blob' AND length(cut)<=16384)),
frontier BLOB CHECK(frontier IS NULL OR (typeof(frontier)='blob' AND length(frontier)<=4096)),
document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 16384),
PRIMARY KEY(installation,provider,account,stream),
FOREIGN KEY(installation,policy_revision) REFERENCES subathon_intake_policies(installation,revision),
CHECK(typeof(provider)='blob' AND typeof(account)='blob' AND typeof(stream)='blob')))SQL",
    R"SQL(CREATE TABLE subathon_intake_gaps(
installation BLOB NOT NULL, provider BLOB NOT NULL, account BLOB NOT NULL, stream BLOB NOT NULL,
epoch INTEGER NOT NULL CHECK(typeof(epoch)='integer' AND epoch>0),
gap_ref BLOB NOT NULL CHECK(typeof(gap_ref)='blob' AND length(gap_ref) BETWEEN 1 AND 4096),
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
policy_revision INTEGER NOT NULL, status INTEGER NOT NULL CHECK(status IN (0,1,2)),
start_frontier BLOB CHECK(start_frontier IS NULL OR (typeof(start_frontier)='blob' AND length(start_frontier)<=4096)),
upper_cut BLOB CHECK(upper_cut IS NULL OR (typeof(upper_cut)='blob' AND length(upper_cut)<=16384)),
document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 16384),
PRIMARY KEY(installation,provider,account,stream,epoch,gap_ref),
FOREIGN KEY(installation,provider,account,stream) REFERENCES subathon_intake_sources(installation,provider,account,stream),
FOREIGN KEY(installation,policy_revision) REFERENCES subathon_intake_policies(installation,revision)))SQL",
    R"SQL(CREATE INDEX subathon_intake_gap_status ON subathon_intake_gaps(installation,provider,account,stream,status,epoch,gap_ref))SQL",
    R"SQL(CREATE TABLE subathon_intake_operations(
installation BLOB NOT NULL, operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref) BETWEEN 1 AND 4096),
provider BLOB NOT NULL, account BLOB NOT NULL, stream BLOB NOT NULL,
epoch INTEGER NOT NULL CHECK(typeof(epoch)='integer' AND epoch>0), policy_revision INTEGER NOT NULL,
gap_ref BLOB, purpose INTEGER NOT NULL CHECK(purpose IN (0,1,2,3,4)),
generation BLOB NOT NULL CHECK(typeof(generation)='blob' AND length(generation) BETWEEN 1 AND 4096),
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
intent BLOB NOT NULL CHECK(typeof(intent)='blob' AND length(intent) BETWEEN 1 AND 16384),
continuation BLOB CHECK(continuation IS NULL OR (typeof(continuation)='blob' AND length(continuation)<=4096)),
next_ordinal INTEGER NOT NULL CHECK(typeof(next_ordinal)='integer' AND next_ordinal BETWEEN 1 AND 257),
slots INTEGER NOT NULL CHECK(typeof(slots)='integer' AND slots BETWEEN 0 AND 4096),
outcome INTEGER NOT NULL CHECK(outcome IN (0,1,2,3)),
prior_cut BLOB CHECK(prior_cut IS NULL OR (typeof(prior_cut)='blob' AND length(prior_cut) BETWEEN 1 AND 16384)),
selected_cut BLOB CHECK(selected_cut IS NULL OR (typeof(selected_cut)='blob' AND length(selected_cut) BETWEEN 1 AND 16384)),
PRIMARY KEY(installation,operation_ref),
UNIQUE(installation,operation_ref,provider,account,stream),
UNIQUE(installation,operation_ref,purpose),
FOREIGN KEY(installation,provider,account,stream) REFERENCES subathon_intake_sources(installation,provider,account,stream),
FOREIGN KEY(installation,policy_revision) REFERENCES subathon_intake_policies(installation,revision),
FOREIGN KEY(installation,provider,account,stream,epoch,gap_ref) REFERENCES subathon_intake_gaps(installation,provider,account,stream,epoch,gap_ref),
CHECK((purpose=0 AND typeof(gap_ref)='blob') OR (purpose<>0 AND gap_ref IS NULL)),
CHECK((purpose IN (0,1) AND prior_cut IS NULL AND selected_cut IS NULL) OR
 (purpose IN (2,3,4) AND selected_cut IS NOT NULL AND outcome=1 AND continuation IS NULL AND next_ordinal=1 AND slots=0))))SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_intake_active_operation ON subathon_intake_operations(installation,provider,account,stream) WHERE outcome=0 AND purpose IN (0,1))SQL",
    R"SQL(CREATE TABLE subathon_intake_segments(
installation BLOB NOT NULL, provider BLOB NOT NULL, account BLOB NOT NULL, stream BLOB NOT NULL,
epoch INTEGER NOT NULL, gap_ref BLOB NOT NULL, operation_ref BLOB NOT NULL,
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0),
coverage INTEGER NOT NULL CHECK(coverage IN (0,1,2)),
document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 16384),
PRIMARY KEY(installation,provider,account,stream,epoch,gap_ref,revision,ordinal),
FOREIGN KEY(installation,provider,account,stream,epoch,gap_ref) REFERENCES subathon_intake_gaps(installation,provider,account,stream,epoch,gap_ref),
FOREIGN KEY(installation,operation_ref,provider,account,stream) REFERENCES subathon_intake_operations(installation,operation_ref,provider,account,stream)))SQL",
    R"SQL(CREATE TABLE subathon_intake_pages(
installation BLOB NOT NULL, operation_ref BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 256),
page BLOB NOT NULL CHECK(typeof(page)='blob' AND length(page) BETWEEN 1 AND 262144),
expected_continuation BLOB CHECK(expected_continuation IS NULL OR (typeof(expected_continuation)='blob' AND length(expected_continuation)<=4096)),
item_count INTEGER NOT NULL CHECK(typeof(item_count)='integer' AND item_count BETWEEN 0 AND 16),
committed_prefix INTEGER NOT NULL CHECK(typeof(committed_prefix)='integer' AND committed_prefix BETWEEN 0 AND item_count),
outcome INTEGER NOT NULL CHECK(outcome IN (0,1,2,3)),
proof BLOB CHECK(proof IS NULL OR (typeof(proof)='blob' AND length(proof)<=16384)),
purpose INTEGER NOT NULL CHECK(purpose IN (0,1)),
PRIMARY KEY(installation,operation_ref,ordinal),
FOREIGN KEY(installation,operation_ref,purpose) REFERENCES subathon_intake_operations(installation,operation_ref,purpose)))SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_intake_inflight_page ON subathon_intake_pages(installation,operation_ref) WHERE outcome=0)SQL",
    R"SQL(CREATE TABLE subathon_intake_entries(
installation BLOB NOT NULL, provider BLOB NOT NULL, account BLOB NOT NULL, stream BLOB NOT NULL,
intake_ref BLOB NOT NULL CHECK(typeof(intake_ref)='blob' AND length(intake_ref) BETWEEN 1 AND 4096),
epoch INTEGER NOT NULL CHECK(typeof(epoch)='integer' AND epoch>0),
generation BLOB NOT NULL CHECK(typeof(generation)='blob' AND length(generation) BETWEEN 1 AND 4096),
policy_revision INTEGER NOT NULL,
operation_ref BLOB, page_ordinal INTEGER, item_ordinal INTEGER,
input BLOB NOT NULL CHECK(typeof(input)='blob' AND length(input) BETWEEN 1 AND 32768),
provenance BLOB NOT NULL CHECK(typeof(provenance)='blob' AND length(provenance) BETWEEN 1 AND 16384),
arrival_ms INTEGER NOT NULL CHECK(typeof(arrival_ms)='integer'),
configuration_ref BLOB REFERENCES subathon_retained_configurations(reference),
modifier_context_ref BLOB REFERENCES subathon_retained_contexts(reference),
activity_namespace BLOB, activity_id BLOB, verification_ref BLOB,
phase INTEGER NOT NULL CHECK(phase IN (0,1,2,3)), disposition INTEGER NOT NULL CHECK(disposition IN (0,1,2,3,4,5)),
quarantine_bytes INTEGER CHECK(quarantine_bytes IS NULL OR (typeof(quarantine_bytes)='integer' AND quarantine_bytes BETWEEN 1 AND 32768)),
receipt_id BLOB, attempt_ordinal INTEGER,
contribution_id BLOB REFERENCES subathon_contributions(contribution_id),
PRIMARY KEY(installation,provider,account,stream,intake_ref),
UNIQUE(installation,operation_ref,page_ordinal,item_ordinal),
UNIQUE(receipt_id,attempt_ordinal),
FOREIGN KEY(installation,provider,account,stream) REFERENCES subathon_intake_sources(installation,provider,account,stream),
FOREIGN KEY(installation,policy_revision) REFERENCES subathon_intake_policies(installation,revision),
FOREIGN KEY(installation,operation_ref,provider,account,stream) REFERENCES subathon_intake_operations(installation,operation_ref,provider,account,stream),
FOREIGN KEY(installation,operation_ref,page_ordinal) REFERENCES subathon_intake_pages(installation,operation_ref,ordinal),
FOREIGN KEY(receipt_id,attempt_ordinal) REFERENCES subathon_receipt_attempts(receipt_id,ordinal),
CHECK((operation_ref IS NULL AND page_ordinal IS NULL AND item_ordinal IS NULL) OR
 (typeof(operation_ref)='blob' AND typeof(page_ordinal)='integer' AND typeof(item_ordinal)='integer' AND item_ordinal BETWEEN 1 AND 16)),
CHECK((receipt_id IS NULL AND attempt_ordinal IS NULL) OR
 (typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000' AND typeof(attempt_ordinal)='integer' AND attempt_ordinal>0 AND configuration_ref IS NOT NULL AND modifier_context_ref IS NOT NULL)),
CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND length(contribution_id)=8 AND receipt_id IS NOT NULL)),
CHECK((activity_namespace IS NULL AND activity_id IS NULL AND verification_ref IS NULL) OR
 (typeof(activity_namespace)='blob' AND typeof(activity_id)='blob' AND typeof(verification_ref)='blob'
  AND length(activity_namespace) BETWEEN 1 AND 4096 AND length(activity_id) BETWEEN 1 AND 4096 AND length(verification_ref) BETWEEN 1 AND 4096))))SQL",
    R"SQL(CREATE INDEX subathon_intake_original_cid ON subathon_intake_entries(contribution_id,receipt_id) WHERE contribution_id IS NOT NULL)SQL",
    R"SQL(CREATE INDEX subathon_intake_quarantine ON subathon_intake_entries(installation,provider,account,stream,intake_ref,quarantine_bytes) WHERE quarantine_bytes IS NOT NULL)SQL",
    R"SQL(CREATE TABLE subathon_intake_basis(
installation BLOB NOT NULL, provider BLOB NOT NULL, account BLOB NOT NULL,
linkage_namespace BLOB NOT NULL CHECK(typeof(linkage_namespace)='blob' AND length(linkage_namespace) BETWEEN 1 AND 4096),
activity_id BLOB NOT NULL CHECK(typeof(activity_id)='blob' AND length(activity_id) BETWEEN 1 AND 4096),
stream BLOB NOT NULL, intake_ref BLOB NOT NULL,
receipt_id BLOB NOT NULL, attempt_ordinal INTEGER NOT NULL,
capture BLOB NOT NULL CHECK(typeof(capture)='blob' AND length(capture) BETWEEN 1 AND 16384),
PRIMARY KEY(installation,provider,account,linkage_namespace,activity_id),
FOREIGN KEY(installation,provider,account,stream,intake_ref) REFERENCES subathon_intake_entries(installation,provider,account,stream,intake_ref),
FOREIGN KEY(receipt_id,attempt_ordinal) REFERENCES subathon_intake_entries(receipt_id,attempt_ordinal)))SQL",
};

inline const QList<CatalogObject> delta_objects{
    {"index", "sqlite_autoindex_subathon_intake_basis_1", "subathon_intake_basis", {}},
    {"index", "sqlite_autoindex_subathon_intake_entries_1", "subathon_intake_entries", {}},
    {"index", "sqlite_autoindex_subathon_intake_entries_2", "subathon_intake_entries", {}},
    {"index", "sqlite_autoindex_subathon_intake_entries_3", "subathon_intake_entries", {}},
    {"index", "sqlite_autoindex_subathon_intake_gaps_1", "subathon_intake_gaps", {}},
    {"index", "sqlite_autoindex_subathon_intake_members_1", "subathon_intake_members", {}},
    {"index", "sqlite_autoindex_subathon_intake_members_2", "subathon_intake_members", {}},
    {"index", "sqlite_autoindex_subathon_intake_operations_1", "subathon_intake_operations", {}},
    {"index", "sqlite_autoindex_subathon_intake_operations_2", "subathon_intake_operations", {}},
    {"index", "sqlite_autoindex_subathon_intake_operations_3", "subathon_intake_operations", {}},
    {"index", "sqlite_autoindex_subathon_intake_pages_1", "subathon_intake_pages", {}},
    {"index", "sqlite_autoindex_subathon_intake_policies_1", "subathon_intake_policies", {}},
    {"index", "sqlite_autoindex_subathon_intake_segments_1", "subathon_intake_segments", {}},
    {"index", "sqlite_autoindex_subathon_intake_sources_1", "subathon_intake_sources", {}},
    {"index", "subathon_intake_active_operation", "subathon_intake_operations", delta_sql[6]},
    {"index", "subathon_intake_gap_status", "subathon_intake_gaps", delta_sql[4]},
    {"index", "subathon_intake_inflight_page", "subathon_intake_pages", delta_sql[9]},
    {"index", "subathon_intake_original_cid", "subathon_intake_entries", delta_sql[11]},
    {"index", "subathon_intake_quarantine", "subathon_intake_entries", delta_sql[12]},
    {"table", "subathon_intake_basis", "subathon_intake_basis", delta_sql[13]},
    {"table", "subathon_intake_entries", "subathon_intake_entries", delta_sql[10]},
    {"table", "subathon_intake_gaps", "subathon_intake_gaps", delta_sql[3]},
    {"table", "subathon_intake_members", "subathon_intake_members", delta_sql[1]},
    {"table", "subathon_intake_operations", "subathon_intake_operations", delta_sql[5]},
    {"table", "subathon_intake_pages", "subathon_intake_pages", delta_sql[8]},
    {"table", "subathon_intake_policies", "subathon_intake_policies", delta_sql[0]},
    {"table", "subathon_intake_segments", "subathon_intake_segments", delta_sql[7]},
    {"table", "subathon_intake_sources", "subathon_intake_sources", delta_sql[2]},
};

inline MigrationManifest successor() {
    auto manifest = literal_v19();
    const auto &tail = manifest.steps.back();
    require(tail.version == 19 && tail.id == "contribution-review-v1" &&
                digest(tail).toHex() == v19_digest_hex && tally(manifest) == v19_manifest_bytes,
            "Protected v19 carries the independently frozen digest and byte total");
    MigrationStep step{tail.version + 1, successor_id, delta_sql, tail.expected_schema};
    step.expected_schema.append(delta_objects);
    std::ranges::sort(step.expected_schema, schema_order);
    manifest.supported_starting_versions.append(step.version);
    manifest.steps.append(std::move(step));
    return manifest;
}

inline MigrationRequest successor_request(MigrationMode mode) {
    return {mode, successor(), {}, {}};
}

inline StorageLimits migration_profile() {
    // Separate profile for real catalog admission (three schema projections,
    // then typed and raw history, encoding, header and integrity projections).
    return {64, 64, 65536, 1048576, 1024, 1048576};
}

inline void advance(CatalogFile &file) {
    const auto target = successor().steps.back().version;
    file.reopen(migration_profile());
    report(migrate(file.writer(), successor_request(MigrationMode::inspect)),
           CatalogState::upgrade_required, target - 1, target, 0);
    report(migrate(file.writer(), successor_request(MigrationMode::advance)), CatalogState::current,
           target, target, 1);
    file.reopen({64, 768, 65536, 8585216, 768, 16105472});
    report(migrate(file.writer(), successor_request(MigrationMode::inspect)), CatalogState::current,
           target, target, 0);
}
} // namespace storage_026_fixture
