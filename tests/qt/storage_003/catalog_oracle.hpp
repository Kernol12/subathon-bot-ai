// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>

namespace storage_003_fixture {

// Independent static-catalog.json seal at the protected pin. Each step hash
// binds its exact ordered SQL and all projected tuples, including SQL NULL.
inline constexpr std::array<const char *, 22> step_digests{
    "0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e",
    "24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2",
    "b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d",
    "bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50",
    "c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb",
    "9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a",
    "efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218",
    "ec41d4b3bf7d04a3e846b401060cc3ed9430020db3dfce0e2650ed76f6a46d5b",
    "a818324e2ff37a638fa00dcb89e3c4b89140fe7e09e1c6eb06d20bcc38be7f68",
    "d0c8f1348abbc3521293ee2d3af3fa75fbeb65c40e5a5589c3c1e9953e172315",
    "342ab385bd4b70d75ec5bbf16ef0b34d6173d5a4f41da539a85d1c6412cdfbc7",
    "590881656952aa89350784c4de3ea50a7d002c3d1b00baf21f1e04aede36f18e",
    "06479f8418119285626522c9f4fa84096b28a594c617a017c8b842ddbd37f7de",
    "a6ca951e91632dd7a2f5bb0f4028a95eb6b0397545cf942a8e34d690e65ea1c8",
    "93a31cfe433644a09ce603bc211390950e481b1e0bdef8d36e579160e7d2c7b6",
    "7246930c294455e1d6542dccaaa0d7f3727fd6c2444cf29f8a4f8b32d2ca6b3d",
    "b14cfdd3a3f69b84d884c4a274d9f4d14389b958726ad5b500e648fddac6783f",
    "1cc2095026cf3d296c11ceb05a74b56e5228951161acc64834c9a00cc17646f4",
    "407229e831e3f1368f2b5cba415c56fe0cc1b45a30ddfb496c39ec72504e8e87",
    "eb1758529cd052df4f8ed80ab693a6219afa519ae9db5f1b00f2f6bae05bf7e1",
    "52b68a611a96311f137c033cf5b0a376a585e555a56b555e20de3938d4db463e",
    "2c8b634d2ed28a81f949e8a31e7d9de991bf513f4f5000194b1eaafb725d27bf",
};

inline constexpr auto prefix21 =
    "d49bca7c166ff9ecc7649b0eafd13270bc2434d6521b89b02f77171404f634b8";
inline constexpr auto prefix22 =
    "382a5e86e3b6f4b2e2b8a1f3aa73f7e8f20f3e24413a7fb50d6f5a7ea6f8e3ff";

// Exact byte strings from the frozen writer/catalog fix2 SQL block.
inline constexpr std::array<const char *, 15> successor_sql{
    R"SQL(CREATE UNIQUE INDEX subathon_app_head_checkpoint_pair ON subathon_app_head(singleton COLLATE BINARY,token COLLATE BINARY))SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_app_state_checkpoint_pair ON subathon_app_states(token COLLATE BINARY,configuration_ref COLLATE BINARY))SQL",
    R"SQL(CREATE TABLE subathon_clock_evidence(revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=0), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind BETWEEN 0 AND 7), installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref) BETWEEN 1 AND 4096), timer_ref BLOB NOT NULL CHECK(typeof(timer_ref)='blob' AND length(timer_ref) BETWEEN 1 AND 4096), lineage_ref BLOB NOT NULL CHECK(typeof(lineage_ref)='blob' AND length(lineage_ref) BETWEEN 1 AND 4096), base_token BLOB NOT NULL CHECK(typeof(base_token)='blob' AND length(base_token) BETWEEN 1 AND 4096), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref) BETWEEN 1 AND 4096), ledger_revision INTEGER NOT NULL CHECK(typeof(ledger_revision)='integer' AND ledger_revision>=0), operation_scope BLOB, operation_ref BLOB, episode_ref BLOB CHECK(episode_ref IS NULL OR (typeof(episode_ref)='blob' AND length(episode_ref) BETWEEN 1 AND 4096)), schedule_id BLOB, nominal_ms INTEGER, fold_run INTEGER, member_count INTEGER, link_revision INTEGER, link_ordinal INTEGER, page_no INTEGER, format_version INTEGER NOT NULL CHECK(typeof(format_version)='integer' AND format_version=1), document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 262144), PRIMARY KEY(revision,ordinal), FOREIGN KEY(base_token,configuration_ref) REFERENCES subathon_app_states(token,configuration_ref), FOREIGN KEY(link_revision,link_ordinal) REFERENCES subathon_clock_evidence(revision,ordinal), CHECK((kind=0 AND ordinal=0 AND typeof(member_count)='integer' AND member_count>0 AND typeof(operation_scope)='blob' AND length(operation_scope) BETWEEN 1 AND 4096 AND typeof(operation_ref)='blob' AND length(operation_ref) BETWEEN 1 AND 4096) OR (kind<>0 AND ordinal>0 AND member_count IS NULL AND operation_scope IS NULL AND operation_ref IS NULL)), CHECK((kind IN (1,2) AND episode_ref IS NOT NULL) OR (kind NOT IN (1,2) AND episode_ref IS NULL)), CHECK((kind=3 AND typeof(schedule_id)='blob' AND length(schedule_id) BETWEEN 1 AND 4096 AND typeof(nominal_ms)='integer' AND typeof(fold_run)='integer' AND fold_run IN (0,1)) OR (kind<>3 AND schedule_id IS NULL AND nominal_ms IS NULL AND fold_run IS NULL)), CHECK((kind IN (2,4,6,7) AND typeof(link_revision)='integer' AND typeof(link_ordinal)='integer' AND (link_revision<revision OR (link_revision=revision AND link_ordinal<ordinal))) OR (kind NOT IN (2,4,6,7) AND link_revision IS NULL AND link_ordinal IS NULL)), CHECK((kind=7 AND typeof(page_no)='integer' AND page_no>0) OR (kind<>7 AND page_no IS NULL))))SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_clock_operation_once ON subathon_clock_evidence(installation_ref,operation_scope,operation_ref) WHERE kind=0)SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_clock_episode_once ON subathon_clock_evidence(installation_ref,episode_ref,kind) WHERE kind IN (1,2))SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_clock_nominal_once ON subathon_clock_evidence(installation_ref,timer_ref,lineage_ref,schedule_id,nominal_ms,fold_run) WHERE kind=3)SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_clock_close_once ON subathon_clock_evidence(link_revision,link_ordinal) WHERE kind IN (4,6))SQL",
    R"SQL(CREATE UNIQUE INDEX subathon_clock_page_once ON subathon_clock_evidence(link_revision,link_ordinal,page_no) WHERE kind=7)SQL",
    R"SQL(CREATE INDEX subathon_clock_ledger ON subathon_clock_evidence(installation_ref,timer_ref,lineage_ref,ledger_revision,revision,ordinal) WHERE kind IN (3,4,5,6))SQL",
    R"SQL(CREATE TABLE subathon_clock_checkpoint(singleton INTEGER PRIMARY KEY CHECK(singleton=1), base_token BLOB NOT NULL CHECK(typeof(base_token)='blob' AND length(base_token) BETWEEN 1 AND 4096), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref) BETWEEN 1 AND 4096), format_version INTEGER NOT NULL CHECK(typeof(format_version)='integer' AND format_version=1), generation INTEGER NOT NULL CHECK(typeof(generation)='integer' AND generation>0), revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>0 AND revision<=generation), root_ordinal INTEGER NOT NULL CHECK(root_ordinal=0), pending_revision INTEGER, pending_ordinal INTEGER, main_ms INTEGER NOT NULL CHECK(typeof(main_ms)='integer' AND main_ms>=0), grace_ms INTEGER NOT NULL CHECK(typeof(grace_ms)='integer' AND grace_ms>=0), cadence_ms INTEGER NOT NULL CHECK(typeof(cadence_ms)='integer' AND cadence_ms>0), ledger_revision INTEGER NOT NULL CHECK(typeof(ledger_revision)='integer' AND ledger_revision>=0), document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 65536), FOREIGN KEY(singleton,base_token) REFERENCES subathon_app_head(singleton,token) ON UPDATE NO ACTION ON DELETE NO ACTION DEFERRABLE INITIALLY DEFERRED, FOREIGN KEY(base_token,configuration_ref) REFERENCES subathon_app_states(token,configuration_ref), FOREIGN KEY(revision,root_ordinal) REFERENCES subathon_clock_evidence(revision,ordinal), FOREIGN KEY(pending_revision,pending_ordinal) REFERENCES subathon_clock_evidence(revision,ordinal), CHECK((pending_revision IS NULL AND pending_ordinal IS NULL) OR (typeof(pending_revision)='integer' AND pending_revision>0 AND pending_revision<=revision AND typeof(pending_ordinal)='integer' AND pending_ordinal>0))))SQL",
    R"SQL(CREATE TRIGGER subathon_clock_head_no_delete BEFORE DELETE ON subathon_app_head BEGIN SELECT RAISE(ABORT,'checkpoint head delete'); END)SQL",
    R"SQL(CREATE TRIGGER subathon_clock_head_insert BEFORE INSERT ON subathon_app_head WHEN EXISTS(SELECT 1 FROM subathon_app_head) OR NOT EXISTS(SELECT 1 FROM subathon_clock_checkpoint WHERE singleton=NEW.singleton AND base_token=NEW.token) BEGIN SELECT RAISE(ABORT,'checkpoint head insert'); END)SQL",
    R"SQL(CREATE TRIGGER subathon_clock_head_update BEFORE UPDATE ON subathon_app_head WHEN NOT EXISTS(SELECT 1 FROM subathon_clock_checkpoint WHERE singleton=OLD.singleton) BEGIN SELECT RAISE(ABORT,'checkpoint head update without slot'); END)SQL",
    R"SQL(CREATE TRIGGER subathon_clock_slot_no_delete BEFORE DELETE ON subathon_clock_checkpoint BEGIN SELECT RAISE(ABORT,'checkpoint slot delete'); END)SQL",
    R"SQL(CREATE TRIGGER subathon_clock_slot_insert BEFORE INSERT ON subathon_clock_checkpoint WHEN EXISTS(SELECT 1 FROM subathon_clock_checkpoint) BEGIN SELECT RAISE(ABORT,'checkpoint slot insert'); END)SQL",
};

} // namespace storage_003_fixture
