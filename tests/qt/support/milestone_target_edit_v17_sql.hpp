// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_migrations.hpp>

namespace milestone_target_edit_fixture {
using namespace subathon::server;

// Literal Stage 1 proposal for independent review. Keep statement whitespace
// exact because sqlite_schema SQL and the migration digest retain these bytes.
struct EditTable {
    const char *name;
    int indexes;
    const char *sql;
};

inline constexpr EditTable edit_tables[]{
    {"subathon_milestone_definition_versions", 1,
     R"sql(CREATE TABLE subathon_milestone_definition_versions(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    definition_revision BLOB NOT NULL CHECK(typeof(definition_revision)='blob' AND length(definition_revision) BETWEEN 1 AND 4096),
    configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref) BETWEEN 1 AND 4096),
    predecessor_revision BLOB CHECK(predecessor_revision IS NULL OR (typeof(predecessor_revision)='blob' AND length(predecessor_revision) BETWEEN 1 AND 4096)),
    PRIMARY KEY(installation_ref,track_ref,definition_revision),
    FOREIGN KEY(installation_ref,track_ref,predecessor_revision) REFERENCES subathon_milestone_definition_versions(installation_ref,track_ref,definition_revision)
))sql"},
    {"subathon_milestone_definition_targets", 2,
     R"sql(CREATE TABLE subathon_milestone_definition_targets(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    definition_revision BLOB NOT NULL CHECK(typeof(definition_revision)='blob' AND length(definition_revision) BETWEEN 1 AND 4096),
    ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1),
    milestone_ref BLOB NOT NULL CHECK(typeof(milestone_ref)='blob' AND length(milestone_ref) BETWEEN 1 AND 4096),
    target_micro_points INTEGER NOT NULL CHECK(typeof(target_micro_points)='integer'),
    PRIMARY KEY(installation_ref,track_ref,definition_revision,ordinal),
    UNIQUE(installation_ref,track_ref,definition_revision,milestone_ref),
    FOREIGN KEY(installation_ref,track_ref,definition_revision) REFERENCES subathon_milestone_definition_versions(installation_ref,track_ref,definition_revision)
))sql"},
    {"subathon_milestone_definition_heads", 1,
     R"sql(CREATE TABLE subathon_milestone_definition_heads(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    definition_revision BLOB NOT NULL CHECK(typeof(definition_revision)='blob' AND length(definition_revision) BETWEEN 1 AND 4096),
    PRIMARY KEY(installation_ref,track_ref),
    FOREIGN KEY(installation_ref,track_ref,definition_revision) REFERENCES subathon_milestone_definition_versions(installation_ref,track_ref,definition_revision)
))sql"},
    {"subathon_milestone_prefix_revisions", 1,
     R"sql(CREATE TABLE subathon_milestone_prefix_revisions(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    prefix_revision BLOB NOT NULL CHECK(typeof(prefix_revision)='blob' AND length(prefix_revision) BETWEEN 1 AND 4096),
    prefix_length INTEGER NOT NULL CHECK(typeof(prefix_length)='integer' AND prefix_length>=0),
    PRIMARY KEY(installation_ref,track_ref,prefix_revision)
))sql"},
    {"subathon_milestone_prefix_heads", 1, R"sql(CREATE TABLE subathon_milestone_prefix_heads(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    prefix_revision BLOB NOT NULL CHECK(typeof(prefix_revision)='blob' AND length(prefix_revision) BETWEEN 1 AND 4096),
    PRIMARY KEY(installation_ref,track_ref),
    FOREIGN KEY(installation_ref,track_ref,prefix_revision) REFERENCES subathon_milestone_prefix_revisions(installation_ref,track_ref,prefix_revision)
))sql"},
    {"subathon_milestone_edit_operations", 2, R"sql(CREATE TABLE subathon_milestone_edit_operations(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    edit_ref BLOB NOT NULL CHECK(typeof(edit_ref)='blob' AND length(edit_ref) BETWEEN 1 AND 4096),
    prior_revision BLOB NOT NULL CHECK(typeof(prior_revision)='blob' AND length(prior_revision) BETWEEN 1 AND 4096),
    resulting_revision BLOB NOT NULL CHECK(typeof(resulting_revision)='blob' AND length(resulting_revision) BETWEEN 1 AND 4096),
    actor_ref BLOB NOT NULL CHECK(typeof(actor_ref)='blob' AND length(actor_ref) BETWEEN 1 AND 4096),
    decision_ref BLOB NOT NULL CHECK(typeof(decision_ref)='blob' AND length(decision_ref) BETWEEN 1 AND 4096),
    score_head_ref BLOB NOT NULL CHECK(typeof(score_head_ref)='blob' AND length(score_head_ref) BETWEEN 1 AND 4096),
    score_micro_points INTEGER NOT NULL CHECK(typeof(score_micro_points)='integer'),
    intent BLOB NOT NULL CHECK(typeof(intent)='blob' AND length(intent) BETWEEN 0 AND 8388608),
    result BLOB NOT NULL CHECK(typeof(result)='blob' AND length(result) BETWEEN 0 AND 8388608),
    PRIMARY KEY(installation_ref,track_ref,edit_ref),
    UNIQUE(installation_ref,track_ref,resulting_revision),
    FOREIGN KEY(installation_ref,track_ref,prior_revision) REFERENCES subathon_milestone_definition_versions(installation_ref,track_ref,definition_revision),
    FOREIGN KEY(installation_ref,track_ref,resulting_revision) REFERENCES subathon_milestone_definition_versions(installation_ref,track_ref,definition_revision)
))sql"},
    {"subathon_milestone_edit_occurrences", 4,
     R"sql(CREATE TABLE subathon_milestone_edit_occurrences(
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    edit_ref BLOB NOT NULL CHECK(typeof(edit_ref)='blob' AND length(edit_ref) BETWEEN 1 AND 4096),
    suffix_ordinal INTEGER NOT NULL CHECK(typeof(suffix_ordinal)='integer' AND suffix_ordinal>=1),
    queue_order INTEGER NOT NULL CHECK(typeof(queue_order)='integer' AND queue_order>=1),
    milestone_ref BLOB NOT NULL CHECK(typeof(milestone_ref)='blob' AND length(milestone_ref) BETWEEN 1 AND 4096),
    definition_revision BLOB NOT NULL CHECK(typeof(definition_revision)='blob' AND length(definition_revision) BETWEEN 1 AND 4096),
    target_micro_points INTEGER NOT NULL CHECK(typeof(target_micro_points)='integer'),
    score_micro_points INTEGER NOT NULL CHECK(typeof(score_micro_points)='integer'),
    completion_context_ref BLOB NOT NULL CHECK(typeof(completion_context_ref)='blob' AND length(completion_context_ref) BETWEEN 1 AND 4096),
    presentation_ms INTEGER CHECK(presentation_ms IS NULL OR (typeof(presentation_ms)='integer' AND presentation_ms>0)),
    gap_ms INTEGER CHECK(gap_ms IS NULL OR (typeof(gap_ms)='integer' AND gap_ms>=0)),
    released INTEGER NOT NULL CHECK(typeof(released)='integer' AND released IN (0,1)),
    PRIMARY KEY(occurrence_ref),
    UNIQUE(installation_ref,track_ref,edit_ref,occurrence_ref),
    UNIQUE(installation_ref,track_ref,edit_ref,suffix_ordinal),
    UNIQUE(queue_order),
    FOREIGN KEY(installation_ref,track_ref,edit_ref) REFERENCES subathon_milestone_edit_operations(installation_ref,track_ref,edit_ref),
    FOREIGN KEY(installation_ref,track_ref,definition_revision) REFERENCES subathon_milestone_definition_versions(installation_ref,track_ref,definition_revision)
))sql"},
    {"subathon_milestone_prefix_members", 2, R"sql(CREATE TABLE subathon_milestone_prefix_members(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    prefix_revision BLOB NOT NULL CHECK(typeof(prefix_revision)='blob' AND length(prefix_revision) BETWEEN 1 AND 4096),
    ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1),
    milestone_ref BLOB NOT NULL CHECK(typeof(milestone_ref)='blob' AND length(milestone_ref) BETWEEN 1 AND 4096),
    definition_revision BLOB NOT NULL CHECK(typeof(definition_revision)='blob' AND length(definition_revision) BETWEEN 1 AND 4096),
    target_micro_points INTEGER NOT NULL CHECK(typeof(target_micro_points)='integer'),
    score_micro_points INTEGER NOT NULL CHECK(typeof(score_micro_points)='integer'),
    completion_context_ref BLOB NOT NULL CHECK(typeof(completion_context_ref)='blob' AND length(completion_context_ref) BETWEEN 1 AND 4096),
    origin INTEGER NOT NULL CHECK(typeof(origin)='integer' AND origin IN (0,1)),
    operation_scope BLOB CHECK(operation_scope IS NULL OR (typeof(operation_scope)='blob' AND length(operation_scope) BETWEEN 1 AND 4096)),
    operation_ref BLOB CHECK(operation_ref IS NULL OR (typeof(operation_ref)='blob' AND length(operation_ref) BETWEEN 1 AND 4096)),
    application_occurrence_ref BLOB CHECK(application_occurrence_ref IS NULL OR (typeof(application_occurrence_ref)='blob' AND length(application_occurrence_ref) BETWEEN 1 AND 4096)),
    edit_ref BLOB CHECK(edit_ref IS NULL OR (typeof(edit_ref)='blob' AND length(edit_ref) BETWEEN 1 AND 4096)),
    edit_occurrence_ref BLOB CHECK(edit_occurrence_ref IS NULL OR (typeof(edit_occurrence_ref)='blob' AND length(edit_occurrence_ref) BETWEEN 1 AND 4096)),
    CHECK((origin=0 AND operation_scope IS NOT NULL AND operation_ref IS NOT NULL
           AND application_occurrence_ref IS NOT NULL AND edit_ref IS NULL AND edit_occurrence_ref IS NULL)
       OR (origin=1 AND operation_scope IS NULL AND operation_ref IS NULL
           AND application_occurrence_ref IS NULL AND edit_ref IS NOT NULL AND edit_occurrence_ref IS NOT NULL)),
    PRIMARY KEY(installation_ref,track_ref,prefix_revision,ordinal),
    UNIQUE(installation_ref,track_ref,prefix_revision,milestone_ref),
    FOREIGN KEY(installation_ref,track_ref,prefix_revision) REFERENCES subathon_milestone_prefix_revisions(installation_ref,track_ref,prefix_revision),
    FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref),
    FOREIGN KEY(application_occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref),
    FOREIGN KEY(installation_ref,track_ref,edit_ref,edit_occurrence_ref) REFERENCES subathon_milestone_edit_occurrences(installation_ref,track_ref,edit_ref,occurrence_ref)
))sql"},
    {"subathon_milestone_edit_actions", 2, R"sql(CREATE TABLE subathon_milestone_edit_actions(
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    action_ordinal INTEGER NOT NULL CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1),
    kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind IN (0,1)),
    action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob' AND length(action_ref) BETWEEN 1 AND 4096),
    version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref) BETWEEN 1 AND 4096),
    PRIMARY KEY(occurrence_ref,action_ordinal),
    UNIQUE(occurrence_ref,action_ordinal,kind),
    FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_edit_occurrences(occurrence_ref)
))sql"},
    {"subathon_milestone_edit_jobs", 2, R"sql(CREATE TABLE subathon_milestone_edit_jobs(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    edit_ref BLOB NOT NULL CHECK(typeof(edit_ref)='blob' AND length(edit_ref) BETWEEN 1 AND 4096),
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    action_ordinal INTEGER NOT NULL CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1),
    kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind=0),
    job_format BLOB NOT NULL CHECK(typeof(job_format)='blob' AND job_format=X'7375626174686f6e2e696e7465726e616c2e64656c69766572792d6a6f622e7631'),
    action_document BLOB NOT NULL CHECK(typeof(action_document)='blob' AND length(action_document) BETWEEN 0 AND 262144),
    PRIMARY KEY(installation_ref,track_ref,edit_ref,occurrence_ref,action_ordinal),
    UNIQUE(occurrence_ref,action_ordinal),
    FOREIGN KEY(installation_ref,track_ref,edit_ref,occurrence_ref) REFERENCES subathon_milestone_edit_occurrences(installation_ref,track_ref,edit_ref,occurrence_ref),
    FOREIGN KEY(occurrence_ref,action_ordinal,kind) REFERENCES subathon_milestone_edit_actions(occurrence_ref,action_ordinal,kind)
))sql"},
    {"subathon_milestone_edit_captures", 1, R"sql(CREATE TABLE subathon_milestone_edit_captures(
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    action_ordinal INTEGER NOT NULL CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1),
    action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob' AND length(action_ref) BETWEEN 1 AND 4096),
    version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref) BETWEEN 1 AND 4096),
    route_ref BLOB NOT NULL CHECK(typeof(route_ref)='blob' AND length(route_ref) BETWEEN 1 AND 4096),
    sender_selection INTEGER NOT NULL CHECK(typeof(sender_selection)='integer' AND sender_selection IN (0,1)),
    sender_ref BLOB NOT NULL CHECK(typeof(sender_ref)='blob' AND length(sender_ref) BETWEEN 1 AND 4096),
    PRIMARY KEY(occurrence_ref,action_ordinal),
    FOREIGN KEY(occurrence_ref,action_ordinal) REFERENCES subathon_milestone_edit_jobs(occurrence_ref,action_ordinal)
))sql"},
    {"subathon_milestone_edit_events", 1, R"sql(CREATE TABLE subathon_milestone_edit_events(
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    action_ordinal INTEGER NOT NULL CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1),
    revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>=1),
    state INTEGER NOT NULL CHECK(typeof(state)='integer' AND state IN (0,1,2,3,4)),
    attempt INTEGER CHECK(attempt IS NULL OR (typeof(attempt)='integer' AND attempt>=2 AND attempt<=revision)),
    evidence_ref BLOB CHECK(evidence_ref IS NULL OR (typeof(evidence_ref)='blob' AND length(evidence_ref) BETWEEN 1 AND 4096)),
    CHECK((state=0 AND attempt IS NULL)
       OR (state=1 AND attempt IS NOT NULL AND attempt=revision)
       OR (state BETWEEN 2 AND 4 AND attempt IS NOT NULL AND attempt<revision)),
    CHECK((state=1 OR revision=1)=(evidence_ref IS NULL)),
    CHECK(revision<>1 OR state=0),
    PRIMARY KEY(occurrence_ref,action_ordinal,revision),
    FOREIGN KEY(occurrence_ref,action_ordinal) REFERENCES subathon_milestone_edit_captures(occurrence_ref,action_ordinal)
))sql"},
    {"subathon_milestone_edit_bonus_receipts", 1,
     R"sql(CREATE TABLE subathon_milestone_edit_bonus_receipts(
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    action_ordinal INTEGER NOT NULL CHECK(typeof(action_ordinal)='integer' AND action_ordinal>=1),
    kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind=1),
    effect_ref BLOB NOT NULL CHECK(typeof(effect_ref)='blob' AND length(effect_ref) BETWEEN 1 AND 4096),
    PRIMARY KEY(occurrence_ref,action_ordinal),
    FOREIGN KEY(occurrence_ref,action_ordinal,kind) REFERENCES subathon_milestone_edit_actions(occurrence_ref,action_ordinal,kind)
))sql"},
    {"subathon_milestone_edit_choices", 1, R"sql(CREATE TABLE subathon_milestone_edit_choices(
    installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096),
    track_ref BLOB NOT NULL CHECK(typeof(track_ref)='blob' AND length(track_ref) BETWEEN 1 AND 4096),
    edit_ref BLOB NOT NULL CHECK(typeof(edit_ref)='blob' AND length(edit_ref) BETWEEN 1 AND 4096),
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    choice INTEGER NOT NULL CHECK(typeof(choice)='integer' AND choice IN (0,1,2)),
    actor_ref BLOB NOT NULL CHECK(typeof(actor_ref)='blob' AND length(actor_ref) BETWEEN 1 AND 4096),
    decision_ref BLOB NOT NULL CHECK(typeof(decision_ref)='blob' AND length(decision_ref) BETWEEN 1 AND 4096),
    resolution INTEGER CHECK(resolution IS NULL OR (typeof(resolution)='integer' AND resolution IN (0,1))),
    resolution_actor_ref BLOB CHECK(resolution_actor_ref IS NULL OR (typeof(resolution_actor_ref)='blob' AND length(resolution_actor_ref) BETWEEN 1 AND 4096)),
    resolution_ref BLOB CHECK(resolution_ref IS NULL OR (typeof(resolution_ref)='blob' AND length(resolution_ref) BETWEEN 1 AND 4096)),
    ended_by_skip_kind INTEGER CHECK(ended_by_skip_kind IS NULL OR (typeof(ended_by_skip_kind)='integer' AND ended_by_skip_kind IN (0,1,2))),
    ended_by_skip_ref BLOB CHECK(ended_by_skip_ref IS NULL OR (typeof(ended_by_skip_ref)='blob' AND length(ended_by_skip_ref) BETWEEN 1 AND 4096)),
    CHECK((resolution IS NULL AND resolution_actor_ref IS NULL AND resolution_ref IS NULL) OR
          (resolution IS NOT NULL AND resolution_actor_ref IS NOT NULL AND resolution_ref IS NOT NULL)),
    CHECK((ended_by_skip_kind IS NULL AND ended_by_skip_ref IS NULL) OR
          (ended_by_skip_kind IS NOT NULL AND ended_by_skip_ref IS NOT NULL)),
    CHECK(resolution IS NULL OR ended_by_skip_kind IS NULL),
    PRIMARY KEY(occurrence_ref),
    FOREIGN KEY(installation_ref,track_ref,edit_ref,occurrence_ref) REFERENCES subathon_milestone_edit_occurrences(installation_ref,track_ref,edit_ref,occurrence_ref)
))sql"},
    {"subathon_milestone_edit_refund_decisions", 1,
     R"sql(CREATE TABLE subathon_milestone_edit_refund_decisions(
    occurrence_ref BLOB NOT NULL CHECK(typeof(occurrence_ref)='blob' AND length(occurrence_ref) BETWEEN 1 AND 4096),
    refund_ref BLOB NOT NULL CHECK(typeof(refund_ref)='blob' AND length(refund_ref) BETWEEN 1 AND 4096),
    score_before_micro_points INTEGER NOT NULL CHECK(typeof(score_before_micro_points)='integer'),
    score_after_micro_points INTEGER NOT NULL CHECK(typeof(score_after_micro_points)='integer'),
    current_target_micro_points INTEGER NOT NULL CHECK(typeof(current_target_micro_points)='integer'),
    policy INTEGER NOT NULL CHECK(typeof(policy)='integer' AND policy IN (0,1,2)),
    resolution INTEGER CHECK(resolution IS NULL OR (typeof(resolution)='integer' AND resolution IN (0,1))),
    resolution_actor_ref BLOB CHECK(resolution_actor_ref IS NULL OR (typeof(resolution_actor_ref)='blob' AND length(resolution_actor_ref) BETWEEN 1 AND 4096)),
    resolution_ref BLOB CHECK(resolution_ref IS NULL OR (typeof(resolution_ref)='blob' AND length(resolution_ref) BETWEEN 1 AND 4096)),
    ended_by_skip_kind INTEGER CHECK(ended_by_skip_kind IS NULL OR (typeof(ended_by_skip_kind)='integer' AND ended_by_skip_kind IN (0,1,2))),
    ended_by_skip_ref BLOB CHECK(ended_by_skip_ref IS NULL OR (typeof(ended_by_skip_ref)='blob' AND length(ended_by_skip_ref) BETWEEN 1 AND 4096)),
    CHECK(score_before_micro_points>=current_target_micro_points AND score_after_micro_points<current_target_micro_points),
    CHECK((resolution IS NULL AND resolution_actor_ref IS NULL AND resolution_ref IS NULL) OR
          (resolution IS NOT NULL AND resolution_actor_ref IS NOT NULL AND resolution_ref IS NOT NULL)),
    CHECK((ended_by_skip_kind IS NULL AND ended_by_skip_ref IS NULL) OR
          (ended_by_skip_kind IS NOT NULL AND ended_by_skip_ref IS NOT NULL)),
    CHECK(resolution IS NULL OR ended_by_skip_kind IS NULL),
    PRIMARY KEY(occurrence_ref,refund_ref),
    FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_edit_occurrences(occurrence_ref)
))sql"},
};

inline constexpr auto edit_pending_index = "subathon_milestone_edit_pending_order";
inline constexpr auto edit_pending_sql =
    "CREATE INDEX subathon_milestone_edit_pending_order ON "
    "subathon_milestone_edit_occurrences(queue_order) WHERE released=0";

} // namespace milestone_target_edit_fixture
