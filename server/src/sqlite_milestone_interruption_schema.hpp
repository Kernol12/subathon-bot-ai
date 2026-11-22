// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace subathon::server::milestone_interruption_schema {

inline constexpr auto probe =
    "SELECT j.kind,f.origin_kind,v.predecessor,a.revision,b.actual,c.ordinal,"
    "h.resume,p.handoff,e.outcome FROM subathon_milestone_command_journal j "
    "CROSS JOIN subathon_milestone_stop_fences f "
    "CROSS JOIN subathon_milestone_slot_versions v "
    "CROSS JOIN subathon_milestone_slot_attempts a "
    "CROSS JOIN subathon_milestone_version_bonus b "
    "CROSS JOIN subathon_milestone_stop_cancellations c "
    "CROSS JOIN subathon_milestone_interruption_causes h "
    "CROSS JOIN subathon_milestone_playback_instances p "
    "CROSS JOIN subathon_milestone_playback_evidence e LIMIT 0";

struct Table {
    const char *name;
    const char *sql;
    int autoindexes;
};
inline constexpr Table tables[]{
    {"subathon_milestone_command_journal", R"SQL(CREATE TABLE subathon_milestone_command_journal(
installation BLOB NOT NULL REFERENCES subathon_account_installation(installation_ref),
track BLOB NOT NULL CHECK(typeof(track)='blob' AND length(track) BETWEEN 1 AND 4096),
operation BLOB NOT NULL CHECK(typeof(operation)='blob' AND length(operation) BETWEEN 1 AND 4096),
kind INTEGER NOT NULL CHECK(kind IN (1,2,3)),
actor BLOB NOT NULL, lineage BLOB NOT NULL, decision BLOB NOT NULL,
intent BLOB NOT NULL, result BLOB NOT NULL,
PRIMARY KEY(installation,track,operation)))SQL",
     1},
    {"subathon_milestone_stop_fences", R"SQL(CREATE TABLE subathon_milestone_stop_fences(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL,
origin_kind INTEGER NOT NULL CHECK(origin_kind IN (1,2)),
origin_scope BLOB NOT NULL, origin_ref BLOB NOT NULL, occurrence BLOB NOT NULL,
choice INTEGER NOT NULL CHECK(choice IN (1,2)),
playback_cap INTEGER NOT NULL CHECK(typeof(playback_cap)='integer' AND playback_cap>=0),
PRIMARY KEY(installation,track,stop),
UNIQUE(installation,track,origin_kind,origin_scope,origin_ref,occurrence),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_command_journal(installation,track,operation)))SQL",
     2},
    {"subathon_milestone_slot_versions", R"SQL(CREATE TABLE subathon_milestone_slot_versions(
installation BLOB NOT NULL, track BLOB NOT NULL, replacement BLOB NOT NULL,
origin_kind INTEGER NOT NULL CHECK(origin_kind IN (1,2)),
origin_scope BLOB NOT NULL, origin_ref BLOB NOT NULL, occurrence BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0),
version BLOB NOT NULL, predecessor BLOB,
kind INTEGER NOT NULL CHECK(kind IN (1,2)), action_ref BLOB NOT NULL, action_version BLOB NOT NULL,
payload BLOB NOT NULL, route BLOB, qualified_sender BLOB,
PRIMARY KEY(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,version),
UNIQUE(installation,track,version),
UNIQUE(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,predecessor),
FOREIGN KEY(installation,track,replacement) REFERENCES subathon_milestone_command_journal(installation,track,operation),
FOREIGN KEY(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,predecessor)
REFERENCES subathon_milestone_slot_versions(installation,track,origin_kind,origin_scope,origin_ref,occurrence,ordinal,version),
CHECK((kind=1 AND route IS NOT NULL AND qualified_sender IS NOT NULL) OR
(kind=2 AND route IS NULL AND qualified_sender IS NULL))))SQL",
     3},
    {"subathon_milestone_slot_attempts", R"SQL(CREATE TABLE subathon_milestone_slot_attempts(
installation BLOB NOT NULL, track BLOB NOT NULL, version BLOB NOT NULL,
revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0),
state INTEGER NOT NULL, evidence BLOB,
PRIMARY KEY(installation,track,version,revision),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     1},
    {"subathon_milestone_version_bonus", R"SQL(CREATE TABLE subathon_milestone_version_bonus(
installation BLOB NOT NULL, track BLOB NOT NULL, version BLOB NOT NULL,
effect BLOB NOT NULL, actual INTEGER NOT NULL CHECK(typeof(actual)='integer'),
PRIMARY KEY(installation,track,version),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     1},
    {"subathon_milestone_stop_cancellations",
     R"SQL(CREATE TABLE subathon_milestone_stop_cancellations(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), version BLOB,
PRIMARY KEY(installation,track,stop,ordinal),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_stop_fences(installation,track,stop),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     1},
    {"subathon_milestone_interruption_causes",
     R"SQL(CREATE TABLE subathon_milestone_interruption_causes(
installation BLOB NOT NULL, track BLOB NOT NULL, cause BLOB NOT NULL, stop BLOB NOT NULL,
resume BLOB,
PRIMARY KEY(installation,track,cause), UNIQUE(installation,track,stop),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_stop_fences(installation,track,stop),
FOREIGN KEY(installation,track,resume) REFERENCES subathon_milestone_command_journal(installation,track,operation)))SQL",
     2},
    {"subathon_milestone_playback_instances",
     R"SQL(CREATE TABLE subathon_milestone_playback_instances(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL,
instance BLOB NOT NULL, ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0),
version BLOB, adapter BLOB NOT NULL, attempt BLOB, activation BLOB NOT NULL, generation BLOB NOT NULL,
capability INTEGER NOT NULL, handoff BLOB,
PRIMARY KEY(installation,track,stop,instance),
UNIQUE(installation,track,stop,ordinal,adapter,activation,generation),
FOREIGN KEY(installation,track,stop) REFERENCES subathon_milestone_stop_fences(installation,track,stop),
FOREIGN KEY(installation,track,version) REFERENCES subathon_milestone_slot_versions(installation,track,version)))SQL",
     2},
    {"subathon_milestone_playback_evidence",
     R"SQL(CREATE TABLE subathon_milestone_playback_evidence(
installation BLOB NOT NULL, track BLOB NOT NULL, stop BLOB NOT NULL, instance BLOB NOT NULL,
ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), outcome INTEGER NOT NULL,
evidence BLOB NOT NULL,
PRIMARY KEY(installation,track,stop,instance,ordinal),
FOREIGN KEY(installation,track,stop,instance) REFERENCES subathon_milestone_playback_instances(installation,track,stop,instance)))SQL",
     1},
};

} // namespace subathon::server::milestone_interruption_schema
