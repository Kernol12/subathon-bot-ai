# Durable application-review holds — physical proposal revision 1

STORAGETEST-016 PHYSICAL DOCUMENT phase only, after actual concrete approvalc0ed4b6.
Authority: [approved concrete hold contract](sqlite-application-holds-v1.md)
blob701c710f83b0361a075c0d0bca7cf6ec07fb351e, H01–H16; accepted service6d0cd67f,
ADR0003, NUM-02/04 and DB-01. This proposes exact additive step6 DDL and persistence
mapping. It supplies NO schema source, manifest factory, header/API/stub, fixture,
feature test, intended red or production implementation. Different actual physical
DOCUMENT review precedes those separately assigned artifacts.

## Catalog identity and immutable prefix

Proposed version6 ID `durable-application-holds-v1`, catalogformat1, storeidentity
`subathon.sqlite-store`, supported starts `[0,1,2,3,4,5,6]`. The future manifest copies
ALL actual versions1–5, their statement order/IDs/payloads/projections unchanged,
then appends these12 statements. No ALTER, trigger, seed, allocator, reserved-catalog
write, transaction control, new domain numeric ID or connection is added. All10
new tables start empty. The two explicit indexes are nonunique query indexes.
Opening/constructing the adapter does not inspect or migrate automatically.
Existing maintenance/backup/explicit runner/normal-store readiness gates remain.

| Prefix | Objects | Canonical bytes | Existing SHA256 |
| --- | --- | --- | --- |
| 1 | 7 | 2000 | 0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e |
| 2 | 13 | 10535 | 24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2 |
| 3 | 20 | 18126 | b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d |
| 4 | 30 | 29015 | bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50 |
| 5 | 66 | 62465 | c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb |

## Exact proposed SQL and full new inventory

Each fenced SINGLE line below is ONE exact UTF-8 statement, with no newline or
semicolon inside its value. Statements1–10 create tables in this order;11–12 create
indexes. Forward FK declarations are legal, but this proposal needs no deferred FK:
its data-write order satisfies every immediate FK. No FK proves application policy.
The first detection and expected/proposed produced-version ownership are checked
at runtime rather than adding cyclic cause↔detection↔version insertion dependencies;
parent versions, current heads, resolution outputs and held-origin locators still
have their explicit typed FKs. Whole missing/duplicate ownership is never repaired.

Statement1:

```sql
CREATE TABLE subathon_hold_mutations(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), mutation_kind INTEGER NOT NULL CHECK(typeof(mutation_kind)='integer' AND mutation_kind BETWEEN 0 AND 1), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind BETWEEN 0 AND 1), subject_id BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), expected_count INTEGER NOT NULL CHECK(typeof(expected_count)='integer' AND expected_count>=0), entry_count INTEGER NOT NULL CHECK(typeof(entry_count)='integer' AND entry_count>=1), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), reason BLOB CHECK(reason IS NULL OR (typeof(reason)='blob')), options_format_ref BLOB CHECK(options_format_ref IS NULL OR (typeof(options_format_ref)='blob' AND length(options_format_ref)>0)), options_document BLOB CHECK(options_document IS NULL OR (typeof(options_document)='blob')), application_link_kind INTEGER CHECK(application_link_kind IS NULL OR (typeof(application_link_kind)='integer' AND application_link_kind BETWEEN 0 AND 1)), application_scope BLOB CHECK(application_scope IS NULL OR (typeof(application_scope)='blob' AND length(application_scope)>0)), application_ref BLOB CHECK(application_ref IS NULL OR (typeof(application_ref)='blob' AND length(application_ref)>0)), application_receipt_id BLOB CHECK(application_receipt_id IS NULL OR (typeof(application_receipt_id)='blob' AND length(application_receipt_id)=8 AND application_receipt_id<>X'0000000000000000')), application_plan_ref BLOB CHECK(application_plan_ref IS NULL OR (typeof(application_plan_ref)='blob' AND length(application_plan_ref)>0)), application_plan_revision BLOB CHECK(application_plan_revision IS NULL OR (typeof(application_plan_revision)='blob' AND length(application_plan_revision)=8 AND application_plan_revision<>X'0000000000000000')), held_hold_ref BLOB CHECK(held_hold_ref IS NULL OR (typeof(held_hold_ref)='blob' AND length(held_hold_ref)>0)), held_version_ref BLOB CHECK(held_version_ref IS NULL OR (typeof(held_version_ref)='blob' AND length(held_version_ref)>0)), held_request_ordinal INTEGER CHECK(held_request_ordinal IS NULL OR (typeof(held_request_ordinal)='integer' AND held_request_ordinal>=1)), held_component_kind INTEGER CHECK(held_component_kind IS NULL OR (typeof(held_component_kind)='integer' AND held_component_kind=4)), remaining_units INTEGER CHECK(remaining_units IS NULL OR (typeof(remaining_units)='integer')), PRIMARY KEY(mutation_ref), CHECK(subject_kind=0 OR subject_id<>X'0000000000000000'), CHECK((actor_ref IS NULL AND decision_ref IS NULL AND reason IS NULL AND options_format_ref IS NULL AND options_document IS NULL) OR (actor_ref IS NOT NULL AND decision_ref IS NOT NULL AND reason IS NOT NULL AND options_format_ref IS NOT NULL AND options_document IS NOT NULL)), CHECK((application_link_kind IS NULL AND application_scope IS NULL AND application_ref IS NULL AND application_receipt_id IS NULL AND application_plan_ref IS NULL AND application_plan_revision IS NULL) OR (application_link_kind IS NOT NULL AND application_scope IS NOT NULL AND application_ref IS NOT NULL AND application_receipt_id IS NOT NULL AND application_plan_ref IS NOT NULL AND application_plan_revision IS NOT NULL)), CHECK((held_hold_ref IS NULL AND held_version_ref IS NULL AND held_request_ordinal IS NULL AND held_component_kind IS NULL) OR (held_hold_ref IS NOT NULL AND held_version_ref IS NOT NULL AND held_request_ordinal IS NOT NULL AND held_component_kind IS NOT NULL)), CHECK((mutation_kind=0 AND expected_count=0 AND entry_count=1 AND actor_ref IS NULL AND application_link_kind IS NULL AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (mutation_kind=1 AND actor_ref IS NOT NULL)), CHECK((application_link_kind IS NULL AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (application_link_kind IS NOT NULL AND ((application_link_kind=0 AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (application_link_kind=1 AND held_hold_ref IS NOT NULL)))), FOREIGN KEY(application_scope,application_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(application_receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(application_plan_ref,application_plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(held_hold_ref,held_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(held_hold_ref,held_request_ordinal) REFERENCES subathon_hold_occurrences(hold_ref,ordinal))
```

Statement2:

```sql
CREATE TABLE subathon_hold_causes(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind BETWEEN 0 AND 1), subject_id BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), contribution_id BLOB CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND length(contribution_id)=8)), source_receipt_id BLOB NOT NULL CHECK(typeof(source_receipt_id)='blob' AND length(source_receipt_id)=8 AND source_receipt_id<>X'0000000000000000'), event_ref BLOB CHECK(event_ref IS NULL OR (typeof(event_ref)='blob')), parent_event_ref BLOB CHECK(parent_event_ref IS NULL OR (typeof(parent_event_ref)='blob')), root_event_ref BLOB CHECK(root_event_ref IS NULL OR (typeof(root_event_ref)='blob')), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), arrival_context_ref BLOB NOT NULL CHECK(typeof(arrival_context_ref)='blob' AND length(arrival_context_ref)>0), command_scope BLOB NOT NULL CHECK(typeof(command_scope)='blob' AND length(command_scope)>0), command_ref BLOB NOT NULL CHECK(typeof(command_ref)='blob' AND length(command_ref)>0), command_format_ref BLOB NOT NULL CHECK(typeof(command_format_ref)='blob' AND length(command_format_ref)>0), command_document BLOB NOT NULL CHECK(typeof(command_document)='blob'), cause_kind INTEGER NOT NULL CHECK(typeof(cause_kind)='integer' AND cause_kind BETWEEN 0 AND 2), supersedes_hold_ref BLOB CHECK(supersedes_hold_ref IS NULL OR (typeof(supersedes_hold_ref)='blob' AND length(supersedes_hold_ref)>0)), first_detection_ref BLOB NOT NULL CHECK(typeof(first_detection_ref)='blob' AND length(first_detection_ref)>0), plan_evidence_kind INTEGER NOT NULL CHECK(typeof(plan_evidence_kind)='integer' AND plan_evidence_kind BETWEEN 0 AND 1), plan_ref BLOB CHECK(plan_ref IS NULL OR (typeof(plan_ref)='blob' AND length(plan_ref)>0)), plan_revision BLOB CHECK(plan_revision IS NULL OR (typeof(plan_revision)='blob' AND length(plan_revision)=8 AND plan_revision<>X'0000000000000000')), extent INTEGER NOT NULL CHECK(typeof(extent)='integer' AND extent BETWEEN 0 AND 2), stopped_for INTEGER CHECK(stopped_for IS NULL OR (typeof(stopped_for)='integer' AND stopped_for BETWEEN 0 AND 1)), failed_capture_format_ref BLOB CHECK(failed_capture_format_ref IS NULL OR (typeof(failed_capture_format_ref)='blob' AND length(failed_capture_format_ref)>0)), failed_capture_document BLOB CHECK(failed_capture_document IS NULL OR (typeof(failed_capture_document)='blob')), occurrence_count INTEGER NOT NULL CHECK(typeof(occurrence_count)='integer' AND occurrence_count>=0), failure_stage INTEGER CHECK(failure_stage IS NULL OR (typeof(failure_stage)='integer' AND failure_stage BETWEEN 0 AND 1)), failing_ordinal INTEGER CHECK(failing_ordinal IS NULL OR (typeof(failing_ordinal)='integer' AND failing_ordinal>=1)), failing_target_ref BLOB CHECK(failing_target_ref IS NULL OR (typeof(failing_target_ref)='blob')), failure_numerator BLOB CHECK(failure_numerator IS NULL OR (typeof(failure_numerator)='blob' AND length(failure_numerator)>0)), failure_denominator BLOB CHECK(failure_denominator IS NULL OR (typeof(failure_denominator)='blob' AND length(failure_denominator)>0)), failure_format_ref BLOB CHECK(failure_format_ref IS NULL OR (typeof(failure_format_ref)='blob' AND length(failure_format_ref)>0)), failure_document BLOB CHECK(failure_document IS NULL OR (typeof(failure_document)='blob')), timer_request_ordinal INTEGER CHECK(timer_request_ordinal IS NULL OR (typeof(timer_request_ordinal)='integer' AND timer_request_ordinal>=1)), timer_component_kind INTEGER CHECK(timer_component_kind IS NULL OR (typeof(timer_component_kind)='integer' AND timer_component_kind=4)), timer_request_units INTEGER CHECK(timer_request_units IS NULL OR (typeof(timer_request_units)='integer' AND timer_request_units<0)), deficit_policy INTEGER CHECK(deficit_policy IS NULL OR (typeof(deficit_policy)='integer' AND deficit_policy=2)), deficit_disposition INTEGER CHECK(deficit_disposition IS NULL OR (typeof(deficit_disposition)='integer' AND deficit_disposition=1)), unapplied_units INTEGER CHECK(unapplied_units IS NULL OR (typeof(unapplied_units)='integer' AND unapplied_units<0)), composition INTEGER CHECK(composition IS NULL OR (typeof(composition)='integer' AND composition BETWEEN 0 AND 1)), composition_format_ref BLOB CHECK(composition_format_ref IS NULL OR (typeof(composition_format_ref)='blob' AND length(composition_format_ref)>0)), composition_document BLOB CHECK(composition_document IS NULL OR (typeof(composition_document)='blob')), missing_policy_reason BLOB CHECK(missing_policy_reason IS NULL OR (typeof(missing_policy_reason)='blob')), associated_scope BLOB CHECK(associated_scope IS NULL OR (typeof(associated_scope)='blob' AND length(associated_scope)>0)), associated_ref BLOB CHECK(associated_ref IS NULL OR (typeof(associated_ref)='blob' AND length(associated_ref)>0)), associated_request_ordinal INTEGER CHECK(associated_request_ordinal IS NULL OR (typeof(associated_request_ordinal)='integer' AND associated_request_ordinal>=1)), PRIMARY KEY(hold_ref), CHECK(subject_kind=0 OR subject_id<>X'0000000000000000'), CHECK((subject_kind=0 AND contribution_id IS NOT NULL AND contribution_id=subject_id) OR (subject_kind=1 AND contribution_id IS NULL AND subject_id=source_receipt_id)), CHECK((plan_ref IS NULL AND plan_revision IS NULL) OR (plan_ref IS NOT NULL AND plan_revision IS NOT NULL)), CHECK((failed_capture_format_ref IS NULL AND failed_capture_document IS NULL) OR (failed_capture_format_ref IS NOT NULL AND failed_capture_document IS NOT NULL)), CHECK((failure_numerator IS NULL AND failure_denominator IS NULL) OR (failure_numerator IS NOT NULL AND failure_denominator IS NOT NULL)), CHECK((failure_stage IS NULL AND failure_format_ref IS NULL AND failure_document IS NULL) OR (failure_stage IS NOT NULL AND failure_format_ref IS NOT NULL AND failure_document IS NOT NULL)), CHECK((timer_request_ordinal IS NULL AND timer_component_kind IS NULL AND timer_request_units IS NULL AND deficit_policy IS NULL AND deficit_disposition IS NULL AND unapplied_units IS NULL AND composition IS NULL AND composition_format_ref IS NULL AND composition_document IS NULL) OR (timer_request_ordinal IS NOT NULL AND timer_component_kind IS NOT NULL AND timer_request_units IS NOT NULL AND deficit_policy IS NOT NULL AND deficit_disposition IS NOT NULL AND unapplied_units IS NOT NULL AND composition IS NOT NULL AND composition_format_ref IS NOT NULL AND composition_document IS NOT NULL)), CHECK((associated_scope IS NULL AND associated_ref IS NULL AND associated_request_ordinal IS NULL) OR (associated_scope IS NOT NULL AND associated_ref IS NOT NULL AND associated_request_ordinal IS NOT NULL)), CHECK(supersedes_hold_ref IS NULL OR supersedes_hold_ref<>hold_ref), CHECK((plan_evidence_kind=0 AND plan_ref IS NOT NULL AND extent=0 AND stopped_for IS NULL AND failed_capture_format_ref IS NULL) OR (plan_evidence_kind=1 AND extent IN (1,2) AND stopped_for IS NOT NULL AND failed_capture_format_ref IS NOT NULL)), CHECK((cause_kind=0 AND failure_stage IS NOT NULL AND timer_request_ordinal IS NULL AND missing_policy_reason IS NULL AND associated_scope IS NULL AND ((plan_evidence_kind=0 AND failure_stage=1) OR (plan_evidence_kind=1 AND stopped_for=0 AND failure_stage=0))) OR (cause_kind=1 AND plan_evidence_kind=0 AND failure_stage IS NULL AND failing_ordinal IS NULL AND failing_target_ref IS NULL AND failure_numerator IS NULL AND timer_request_ordinal IS NOT NULL AND unapplied_units=timer_request_units AND missing_policy_reason IS NULL AND ((composition=0 AND associated_scope IS NULL) OR (composition=1 AND associated_scope IS NOT NULL AND associated_request_ordinal=timer_request_ordinal))) OR (cause_kind=2 AND failure_stage IS NULL AND failing_ordinal IS NULL AND failing_target_ref IS NULL AND failure_numerator IS NULL AND timer_request_ordinal IS NULL AND missing_policy_reason IS NOT NULL AND associated_scope IS NULL AND (plan_evidence_kind=0 OR stopped_for=1))), FOREIGN KEY(contribution_id) REFERENCES subathon_contributions(contribution_id), FOREIGN KEY(source_receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(configuration_ref) REFERENCES subathon_retained_configurations(reference), FOREIGN KEY(arrival_context_ref) REFERENCES subathon_retained_contexts(reference), FOREIGN KEY(supersedes_hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(first_detection_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(plan_ref,plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(associated_scope,associated_ref,associated_request_ordinal) REFERENCES subathon_app_effects(operation_scope,operation_ref,request_ordinal))
```

Statement3:

```sql
CREATE TABLE subathon_hold_cause_observations(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), persisted_state_token BLOB CHECK(persisted_state_token IS NULL OR (typeof(persisted_state_token)='blob' AND length(persisted_state_token)>0)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), policy_revision_ref BLOB CHECK(policy_revision_ref IS NULL OR (typeof(policy_revision_ref)='blob' AND length(policy_revision_ref)>0)), permission_revision_ref BLOB CHECK(permission_revision_ref IS NULL OR (typeof(permission_revision_ref)='blob' AND length(permission_revision_ref)>0)), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), observed_utc_ms INTEGER CHECK(observed_utc_ms IS NULL OR (typeof(observed_utc_ms)='integer')), elapsed_ms INTEGER CHECK(elapsed_ms IS NULL OR (typeof(elapsed_ms)='integer' AND elapsed_ms>=0)), PRIMARY KEY(hold_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(persisted_state_token) REFERENCES subathon_app_states(token))
```

Statement4:

```sql
CREATE TABLE subathon_hold_occurrences(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), action_ref BLOB CHECK(action_ref IS NULL OR (typeof(action_ref)='blob')), action_version_ref BLOB CHECK(action_version_ref IS NULL OR (typeof(action_version_ref)='blob')), rule_ref BLOB CHECK(rule_ref IS NULL OR (typeof(rule_ref)='blob')), target_ref BLOB CHECK(target_ref IS NULL OR (typeof(target_ref)='blob')), unit INTEGER CHECK(unit IS NULL OR (typeof(unit)='integer' AND unit BETWEEN 0 AND 2)), attempted_numerator BLOB CHECK(attempted_numerator IS NULL OR (typeof(attempted_numerator)='blob' AND length(attempted_numerator)>0)), attempted_denominator BLOB CHECK(attempted_denominator IS NULL OR (typeof(attempted_denominator)='blob' AND length(attempted_denominator)>0)), finalized_units INTEGER CHECK(finalized_units IS NULL OR (typeof(finalized_units)='integer')), explanation BLOB CHECK(explanation IS NULL OR (typeof(explanation)='blob')), PRIMARY KEY(hold_ref,ordinal), CHECK((attempted_numerator IS NULL AND attempted_denominator IS NULL) OR (attempted_numerator IS NOT NULL AND attempted_denominator IS NOT NULL)), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref))
```

Statement5:

```sql
CREATE TABLE subathon_hold_detections(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), expected_version_ref BLOB CHECK(expected_version_ref IS NULL OR (typeof(expected_version_ref)='blob' AND length(expected_version_ref)>0)), proposed_version_ref BLOB NOT NULL CHECK(typeof(proposed_version_ref)='blob' AND length(proposed_version_ref)>0), audit_format_ref BLOB NOT NULL CHECK(typeof(audit_format_ref)='blob' AND length(audit_format_ref)>0), audit_document BLOB NOT NULL CHECK(typeof(audit_document)='blob'), PRIMARY KEY(mutation_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref))
```

Statement6:

```sql
CREATE TABLE subathon_hold_detection_observations(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), persisted_state_token BLOB CHECK(persisted_state_token IS NULL OR (typeof(persisted_state_token)='blob' AND length(persisted_state_token)>0)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), policy_revision_ref BLOB CHECK(policy_revision_ref IS NULL OR (typeof(policy_revision_ref)='blob' AND length(policy_revision_ref)>0)), permission_revision_ref BLOB CHECK(permission_revision_ref IS NULL OR (typeof(permission_revision_ref)='blob' AND length(permission_revision_ref)>0)), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), observed_utc_ms INTEGER CHECK(observed_utc_ms IS NULL OR (typeof(observed_utc_ms)='integer')), elapsed_ms INTEGER CHECK(elapsed_ms IS NULL OR (typeof(elapsed_ms)='integer' AND elapsed_ms>=0)), PRIMARY KEY(mutation_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_detections(mutation_ref), FOREIGN KEY(persisted_state_token) REFERENCES subathon_app_states(token))
```

Statement7:

```sql
CREATE TABLE subathon_hold_versions(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>=1), parent_version_ref BLOB CHECK(parent_version_ref IS NULL OR (typeof(parent_version_ref)='blob' AND length(parent_version_ref)>0)), state INTEGER NOT NULL CHECK(typeof(state)='integer' AND state BETWEEN 0 AND 1), mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), replacement_hold_ref BLOB CHECK(replacement_hold_ref IS NULL OR (typeof(replacement_hold_ref)='blob' AND length(replacement_hold_ref)>0)), replacement_detection_ref BLOB CHECK(replacement_detection_ref IS NULL OR (typeof(replacement_detection_ref)='blob' AND length(replacement_detection_ref)>0)), PRIMARY KEY(hold_ref,version_ref), UNIQUE(hold_ref,position), CHECK((replacement_hold_ref IS NULL AND replacement_detection_ref IS NULL) OR (replacement_hold_ref IS NOT NULL AND replacement_detection_ref IS NOT NULL)), CHECK((position=1 AND parent_version_ref IS NULL AND state=0) OR (position>1 AND parent_version_ref IS NOT NULL)), CHECK(state=1 OR replacement_hold_ref IS NULL), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(hold_ref,parent_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(replacement_hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(replacement_detection_ref) REFERENCES subathon_hold_detections(mutation_ref))
```

Statement8:

```sql
CREATE TABLE subathon_hold_heads(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), version_count INTEGER NOT NULL CHECK(typeof(version_count)='integer' AND version_count>=1), PRIMARY KEY(hold_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(hold_ref,version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref))
```

Statement9:

```sql
CREATE TABLE subathon_hold_resolution_expected(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), PRIMARY KEY(mutation_ref,hold_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref,version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref))
```

Statement10:

```sql
CREATE TABLE subathon_hold_resolution_entries(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), expected_version_ref BLOB NOT NULL CHECK(typeof(expected_version_ref)='blob' AND length(expected_version_ref)>0), proposed_version_ref BLOB NOT NULL CHECK(typeof(proposed_version_ref)='blob' AND length(proposed_version_ref)>0), decision_format_ref BLOB NOT NULL CHECK(typeof(decision_format_ref)='blob' AND length(decision_format_ref)>0), decision_document BLOB NOT NULL CHECK(typeof(decision_document)='blob'), replacement_hold_ref BLOB CHECK(replacement_hold_ref IS NULL OR (typeof(replacement_hold_ref)='blob' AND length(replacement_hold_ref)>0)), replacement_detection_ref BLOB CHECK(replacement_detection_ref IS NULL OR (typeof(replacement_detection_ref)='blob' AND length(replacement_detection_ref)>0)), PRIMARY KEY(mutation_ref,hold_ref), CHECK((replacement_hold_ref IS NULL AND replacement_detection_ref IS NULL) OR (replacement_hold_ref IS NOT NULL AND replacement_detection_ref IS NOT NULL)), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref,expected_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(hold_ref,proposed_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(replacement_hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(replacement_detection_ref) REFERENCES subathon_hold_detections(mutation_ref))
```

Statement11:

```sql
CREATE INDEX subathon_hold_subject_idx ON subathon_hold_causes(subject_kind,subject_id,hold_ref)
```

Statement12:

```sql
CREATE INDEX subathon_hold_producer_idx ON subathon_hold_versions(mutation_ref,hold_ref,position)
```

The complete projection is EXACT unchanged prefix5's66 tuples PLUS the following23
new tuples, sorted by the existing unsigned UTF-8 `(type,name,tbl_name,NULL-marker,sql)`
rule. A table's SQL is its exact statement above; every autoindex SQL is SQLNULL,
not empty text. There are no other new objects; full step6 has89 objects.

| type | name | tbl_name | sql |
| --- | --- | --- | --- |
| index | sqlite_autoindex_subathon_hold_cause_observations_1 | subathon_hold_cause_observations | NULL |
| index | sqlite_autoindex_subathon_hold_causes_1 | subathon_hold_causes | NULL |
| index | sqlite_autoindex_subathon_hold_detection_observations_1 | subathon_hold_detection_observations | NULL |
| index | sqlite_autoindex_subathon_hold_detections_1 | subathon_hold_detections | NULL |
| index | sqlite_autoindex_subathon_hold_heads_1 | subathon_hold_heads | NULL |
| index | sqlite_autoindex_subathon_hold_mutations_1 | subathon_hold_mutations | NULL |
| index | sqlite_autoindex_subathon_hold_occurrences_1 | subathon_hold_occurrences | NULL |
| index | sqlite_autoindex_subathon_hold_resolution_entries_1 | subathon_hold_resolution_entries | NULL |
| index | sqlite_autoindex_subathon_hold_resolution_expected_1 | subathon_hold_resolution_expected | NULL |
| index | sqlite_autoindex_subathon_hold_versions_1 | subathon_hold_versions | NULL |
| index | sqlite_autoindex_subathon_hold_versions_2 | subathon_hold_versions | NULL |
| index | subathon_hold_producer_idx | subathon_hold_versions | Statement12 |
| index | subathon_hold_subject_idx | subathon_hold_causes | Statement11 |
| table | subathon_hold_cause_observations | subathon_hold_cause_observations | Statement3 |
| table | subathon_hold_causes | subathon_hold_causes | Statement2 |
| table | subathon_hold_detection_observations | subathon_hold_detection_observations | Statement6 |
| table | subathon_hold_detections | subathon_hold_detections | Statement5 |
| table | subathon_hold_heads | subathon_hold_heads | Statement8 |
| table | subathon_hold_mutations | subathon_hold_mutations | Statement1 |
| table | subathon_hold_occurrences | subathon_hold_occurrences | Statement4 |
| table | subathon_hold_resolution_entries | subathon_hold_resolution_entries | Statement10 |
| table | subathon_hold_resolution_expected | subathon_hold_resolution_expected | Statement9 |
| table | subathon_hold_versions | subathon_hold_versions | Statement7 |

Table column counts in statement1–10 order are22,45,10,11,6,10,8,3,3,8.
Exact statement UTF-8 byte lengths in1–12 order are4696,8804,1248,1284,795,1280,
1665,468,504,1559,96,97. PKs produce one NULL-SQL autoindex per table; versions has
its second index for UNIQUE(hold_ref,position). Neither explicit index changes
identity uniqueness. Their role is efficient complete subject and producer selection.

The independent literal framing, using the existing [catalog F format](migration-catalog-v1.md),
is `F("migration-catalog-v1") || F("6") || F("durable-application-holds-v1") ||
F("12") || F(each statement in order) || F("89") || each complete sorted Object`.
The proposed full payload is **89890 bytes**, SHA256
`9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a`.
Lengths count UTF-8 bytes, not characters; NULL SQL has no SQL field. Prefix5 was
independently reframed to the unchanged62465-byte/c9ba98 digest before this delta.
These are hand-specified proposal values independently framed without a new UUT.
The later independent fixture author must freeze its own approved literal oracle.

## Domains, field mapping and immutable records

All references are opaque exact BLOB bytes; PK references/versions/format tags are
nonempty. Payloads are non-null BLOB, including present-empty. Optional values use
SQLNULL only; empty BLOB is never substituted for absence. Typed ReceiptId and
positive selected plan revision use canonical big-endian BLOB8 excluding zero;
ContributionId uses all BLOB8 values including0/MAX. No ID goes through signed64,
REAL, arithmetic affinity conversion or a numeric absence sentinel. INTEGER is
signed64 for actual units/time/internal positions/counts, and explicit small ranges
for enums. Every selected value is decoded with typeof/value/length and actual
QVariant class checks; affinity/CHECK/FK/predicate matches alone are insufficient.

| Relation | Exact logical mapping and runtime obligations |
| --- | --- |
| mutations | ONE raw mutation_ref PK across detection0/resolution1; mutation_kind is DATA, never part of uniqueness. subject_kind/id is the exact global subject. For detection, counts0/1 and resolution fields/link are NULL. For resolution, actor/decision/reason/options are supplied DATA; expected_count/entry_count describe the complete owned collections. Optional application group and held-origin group are complete, not partial |
| causes | Immutable HoldCause/source/command/plan/failure/timer/composition; hold_ref identifies ONE cause and first_detection_ref names its actual creating detection. contribution_id is a physically redundant typed FK projection: present and EQUAL subject_id exactly for tag0, absent for tag1. Receipt-tagged subject_id equals its actual source_receipt_id. Source event/parent/root refs retain absence/known bytes, not inferred provider facts. command_scope/ref is an intended command key, NOT an FK requiring an already successful application |
| cause_observations | Exactly one immutable first HoldObservation per cause: optional persisted snapshot token, versioned annotation and individually optional known policy/permission/actor/decision/time fields. CapturedRevision requires known token; FailedCapture may honestly omit it. elapsed_ms is nonnegative when known; UTC remains signed64 |
| occurrences | EXACT ordered ProposedOccurrence sequence by1-based ordinal; declaration occurrence_count must equal actual count/min/max/visited rows. Known action/version/rule/target refs may be BLOBempty where their existing domains permit; absent remains unknown. unit optional; canonical Exact attempted pair all-present or all-NULL; finalized_units absent until actually produced. explanation retains absent versus present-empty |
| detections | Exactly one kind0 record per selected mutation; owned request consists of the immutable cause selected by hold_ref, expected/proposed versions, attempt observation and audit. It does not snapshot today's mutable head. expected_version_ref NULL ONLY for creation; proposed_version_ref names its actual produced historical active version |
| detection_observations | Exactly one attempt observation per detection, same fields/known-state rules as first observation. It does not overwrite first cause evidence. This and the non-success audit retain each detection separately |
| versions | Immutable HoldVersion `(hold_ref,version_ref)`, unique positive position, preceding version or NULL only at position1, actual state active0/resolved1 and actual producing mutation_ref. Replacement pair names the exact new cause/creating detection only on a resolved version. No mutation-kind discriminator is another key namespace |
| heads | Exactly one current version and declared version_count PER hold; no subject-wide mutable head/status that would erase independent causes. Head version/position/count/terminal state must agree with complete actual history; it advances LAST. There is no separate authoritative cached active flag |
| resolution_expected | Complete strictly unsigned-HoldRef-ordered expected active VersionLocators; PK also prohibits duplicate hold keys. These are HISTORICAL active versions, not pointers required to remain current forever |
| resolution_entries | Complete strictly ordered immutable ResolutionEntries, expected/proposed versions, decision annotation and optional replacement pair. Actual version parent/state/producer must agree with THIS entry. A batch may close a subset without an application link; with a link it closes ALL subject-active holds and creates NO replacement |

Physical enums: cause_kind0 WholeInputNumericOverflow/1 TimerAdjustmentDeficit/
2 UnresolvedPolicy; plan_evidence_kind0 CapturedRevision/1 FailedCapture; extent0
captured_complete/1 proposed_complete/2 known_prefix; stopped_for0 numeric_conversion/
1 unresolved_capture_policy; failure_stage0 final_conversion/1 aggregate_after_finalization;
composition0 DeferWholeInput/1 BesideCompleteApplication. Existing unit0ms/1micro-points/
2whole-count, deficit_policy2 RequireReview, deficit_disposition1 RequiresReview and
component_kind4 unapplied retain their actual existing meanings. Associated timer
columns retain the signed negative request/full unapplied value, including MIN;
no unsigned magnitude or negation is required. Full selected timer effects must
have Main/debt0, unapplied=request and exact actual policy/disposition.

The exhaustive approved cause/plan/failure/extent matrix remains binding. SQL
encodes its major branches and whole optional groups; runtime enforces every
branch, known failing ordinal membership, final-conversion failure's absent finalized
value, complete captured occurrence equality and all canonical/equality conditions.
Numeric failure attempted pair and known target/ordinal are independently optional
where genuinely unknown. Failure format/document are required for numeric cause;
failed_capture format/document are required for FailedCapture, never a fake plan.
Missing-policy reason is non-null DATA only for UnresolvedPolicy. Timer composition
annotation is required DATA for either explicit composition, not authorization.

All rows except heads are append-only through this adapter. No UPDATE/DELETE/rebind
of causes, mutations, observations, occurrences, versions or resolution collections
is exposed. Unique version_ref is scoped to ONE hold for its full lifetime; position
is internal ordering only. There is no automatic reference issuance, retry loop or
repair on collision, no rollover and no imported sparse-chain exhaustion shortcut.

## Exact reference/source crosspaths

Incoming missing required reference uses missing_reference; present individually
valid references that disagree use source_mismatch, after real caller-unit rollback.
A missing/mismatching reference inside an already stored selected record instead
means invalid_stored_data. A genuine well-typed selected overage is result_limit.
The same real writer budgets cover ALL these queries and borrowed port calls.

1. source_receipt_id resolves `subathon_receipts.receipt_id` and its complete typed
   immutable identity/facts/arrival, without loading growing attempt history. Resolve
   retained configuration/context records; source.configuration_ref and context
   must agree with actual receipt.configuration_ref/modifier_context_ref, and a
   known retained context configuration must equal that same configuration. Existing
   unrelated actor/action/format/event strings are not invented external FKs.
2. Contribution subject resolves actual `subathon_contributions.contribution_id`,
   original_receipt_id and immutable support observation/identity, not an owning
   legacy transaction. The event receipt may be a later linked event instead of
   original support: its actual observation MUST link the SAME CID; original support
   receipt/observation/CID/support anchor MUST agree. Receipt subject has no CID and
   uses its exact source receipt; caller-established non-support classification and
   earlier durability/permission remain external preconditions. Current mutable
   evidence/valuation/review/application/reversal/settlement need NOT equal history.
3. A present PlanKey resolves `subathon_plan_headers.plan_ref` source event/root/
   parent/receipt/CID and complete selected `subathon_plan_revisions(plan_ref,revision)`
   with rules/modifiers/requests, exact body and healthy contiguous full scalar
   revision history. Its header receipt/CID MUST equal the actual HoldSource event/
   subject; known event/root/parent source fields agree EXACTLY (absence does not
   invent unknown source facts). A later healthy plan head is allowed; neither
   current plan freshness nor old snapshot body materialization of every revision
   is imposed. CapturedRevision/captured_complete matches ALL selected finalized
   request occurrences1..N and every supplied known copied member. FailedCapture's
   optional prior PlanKey has the SAME full closure/source checks, but its proposed
   occurrences are NOT falsely compared as finalized requests of that old revision.
4. FailedCapture without PlanKey still requires paths1–2, its explicit stopped_for/
   pre-finalization evidence/extent/envelope and complete KNOWN sequence. It never
   creates a plan/revision0 or final amount0. A known observation token in either
   observation table resolves full `subathon_app_states`/balances/configuration
   snapshot and all actual counts/keys/invariants. CapturedRevision requires it;
   FailedCapture may omit it without reading/inventing the current app head. A
   historical token need not equal today's `subathon_app_head` or current cap/policy.
5. Cause's associated_scope/ref/request selects a COMPLETE original bundle by its
   actual OperationKey (the full002 lookup closure, not reversal-basis projection).
   Actual subject, receipt, selected plan and occurrence match this timer cause;
   request is negative time_reversal with RequireReview/RequiresReview, Main/debt0
   and full unapplied delta. This is allowed ONLY for BesideCompleteApplication.
   Full bundle closure already validates claims/results/audit/source, including
   actual current contribution identity without imposing mutable current conditions.
6. Resolution application_link_kind0 WholeInput selects full original bundle at
   application_scope/ref. Its actual receipt and selected PlanKey equal the explicit
   application_* fields and the SAME original tagged subject as the closed causes.
   Retain each cause's actual historical event independently: a later confirmation
   can name the same CID without equaling its original support receipt or an earlier
   held event. The application's own source/proposed-plan path must be coherent with
   that same CID/support identity; it never rewrites any cause's historical event or
   plan. Receipt-tagged subjects still require their exact actual receipt. A changed
   selected revision retains its actual source/override provenance. Link_kind1 TimerCorrection instead selects
   a DISTINCT complete original correction OperationKey/source (receipt subject,
   no CID) and retains held_hold_ref/version/request/component plus optional known
   remaining_units. Origin is one of this batch's closed TimerAdjustmentDeficit
   entries; held_version equals its historical expected version and selector equals
   the immutable held timer occurrence/component. Correction key/source differ from
   the held original/associated application; it cannot point at old processed claim
   to replay it. Actual correction receipt/plan equal explicit application_* fields.
   Storage validates supplied exact links/known scalar representation, NOT partial/
   full entitlement, correction permission, numeric derivation or continuation policy.

Application-link validation is also COMPOSITION-SENSITIVE across ALL closed entries:

| Link form | Eligible held origin / explicit exclusion |
| --- | --- |
| kind0 WholeInput | Numeric-overflow or unresolved-policy whole-input causes, and timer causes explicitly DeferWholeInput. It represents the complete previously unapplied input with the same tagged subject and coherent actual source/proposed plan. If ANY closed timer cause is BesideCompleteApplication, kind0 is REFUSED even if the old associated bundle is otherwise fully valid and shares subject/receipt/plan |
| kind1 TimerCorrection | The explicit held origin must be a closed TimerAdjustmentDeficit entry. BesideCompleteApplication requires THIS distinct-correction route for application-linked resolution, never kind0 or its old associated original. A DeferWholeInput timer can use kind0 for the complete deferred original; any separately supplied kind1 distinct-correction declaration still requires its explicit future reviewed continuation policy and the SAME distinct key/source/origin checks, not an inferred default |
| no application link | May record the supplied resolution/replacement decision within approved storage rules. It does not claim a timer was numerically corrected, authorize reject/defer policy or clear another independent condition |

Thus pointing kind0 at an already-processed BesideCompleteApplication original cannot
evade H11 by avoiding the kind1 guard. Incoming composition/link inconsistency is
source_mismatch after actual rollback; the same inconsistency in a selected stored
mutation is invalid_stored_data. Shape-valid kind1 DATA does not establish remaining
entitlement, partial/full continuation meaning or current authority. Unknown actual
continuation/reject/defer semantics remain not-ready under their owning tasks.

There is currently NO public nonowning standalone reader for arbitrary historical
state tokens or full selected-plan-body plus metadata-only lifetime history. Existing
PlanStore.find_revision is local, find materializes full history under result caps,
Bundle.read_state/read_inputs target CURRENT state/plan, and017 absence is not source
admission. A later implementation needs explicit same-session strict SQL/internal
decoder factoring for paths1–4; no dummy stage/reversal query, owning legacy adapter,
second reader connection, new public API or provider inference is assumed here.
Public Exact/uint64 codecs are reused only after actual SQL-class/size checks.

## Finite selected closure and stable historical outcomes

A scalar locator is NOT a recursively owning HoldView. This is essential when a
resolution closes many holds and introduces replacement detections:

- `HoldMetadata(h)` reads its anchor/key/subject/first-detection locator, actual head
  and COMPLETE version metadata history. It validates scalar producer ownership,
  parent/status/position/link structure, not historical document bodies. For each
  producing mutation read its ACTUAL kind/subject/counts and corresponding actual
  detection or resolution-entry scalar locator. Check exact back-reference to the
  version, expected parent and producing outcome. This does NOT recursively call
  find/active/full-event on sibling or historical holds.
- `FullCause(h)` loads this immutable cause, first observation/occurrences and the
  reference paths above. `FullDetection(d)` loads actual kind0 mutation, this cause,
  attempt observation/audit and its ONE produced HISTORICAL version. It does not
  replace that result with today's head or recursively expand another current view.
- `FullResolution(r)` loads actual kind1 registry, complete expected/entry collections,
  entry annotations, their exact historical produced versions, optional application
  and each explicitly owned replacement Detection/cause. Referenced old/new holds
  receive scalar metadata/link checks; unrelated sibling current HoldViews are NOT
  recursively returned/loaded. Replacement detection validation never follows its
  parent resolution back into FullResolution. It validates that backlink as scalar
  ownership. Historical expected versions must have been active; entries produce
  its direct next resolved version with producer r. Replacements produce fresh
  position1 active versions under their own detection keys.
- A full HoldView uses FullCause, first FullDetection, current version and its actual
  full producing detection/resolution event. The graph expands only that finite
  event's owned request collections/replacement causes; historical version/hold
  locators remain scalar projections. Exact mutation replay similarly loads its
  actual kind's FullDetection/FullResolution BEFORE comparing incoming kind/payload.
  No lifetime full-history vector or recursive batch→sibling-view→batch expansion.
- Supersedes ancestry checks actual same-subject anchor/first-producer scalar links
  and rejects self/cycles. Use iterative bounded batches (at most64 plus lookahead65)
  and constant-space cycle detection, not recursive FullCause/HoldView expansion or
  a growing visited set. For an atomic replacement, both resolved-old version/
  entry replacement pair and new cause.supersedes/first_detection pair must agree.
  Standalone new cause may supersede an already resolved same-subject cause; it
  cannot silently replace an active one outside its explicit atomic resolution.

Past mutation outcomes remain their actual produced VersionLocators even when a
new detection advanced the hold or another resolution closed it. A detection exact
retry therefore returns its old active version, without implying CURRENTly active
or appending/reactivating. A resolution retry returns its recorded old/new version
results, not today's heads. API ownership/layout/byte formulas remain NEXT review;
this projection rule does not exempt actually owned repeated members from quotas.

## Complete active membership and checked64/65 metadata

Select ALL `subathon_hold_causes` anchors by `subject_kind=? AND subject_id=?`,
with no scope, source/plan/key or active-status filter and no INNER JOIN. The subject
index accelerates that declared scope; it is not a new uniqueness constraint.
Read the first page WITHOUT a lower cursor. Before keyset traversal, obtain actual
unfiltered selected COUNT and scalar invalid-type/domain flag (e.g. MAX(CASE...)),
not a count of already filtered positive/valid keys. On later pages use exact
validated BLOB HoldRef comparison in unsigned byte order,64 plus65 lookahead.
Every selected anchor's actual tag/id/duplicate contribution projection must agree;
NULL/empty/non-BLOB selected keys cannot hide behind `hold_ref>empty` or joins.
Walk every selected hold's complete head/history metadata BEFORE classifying status.
Resolved/inactive anchors are included; missing head/status/version is failure.

For EACH selected hold, use ALL `versions WHERE hold_ref=?`, including position0/
negative, NULL/wrong type or malformed ref rows. Actual COUNT, MIN/MAX positions,
type/domain flags, unique version/position keys and visited count must prove exactly
positions1..N, initial NULL-parent and subsequent immediate previous-version parent,
matching subject/event ownership, active detections followed by at most ONE terminal
resolved version, no versions after resolved, and head_count=N/head.position=N.
No initial `position>0` hides corruption; after validating all scalar summaries,
keyset pages use only the last checked position. A count/max snapshot or FK alone
is not chain validation. All producer-entry actual counts/identities are checked
against registry declarations; joins must not drop missing endpoints. Shared mutation
kind is actual DATA: wrong-kind rows/extra counterpart rows are corrupt, not another
identity. History65 malformed metadata cannot escape a one-page audit.

These walks have finite buffers, SAME actual cumulative SQL budgets, and no lifetime
materialization quota. Old historical row/document counts do not acquire today's
input_items/resolution_entries/history-page/active-result ceilings merely for a
metadata-only walk. Full selected immutable body overage is result_limit under its
appropriate body/result limits. Unselected inactive cause/observation/doc bodies
remain OUTSIDE active discovery's certificate; their key/head/history metadata is
still checked. Active() fully loads ALL returned active views or fails result_limit;
257 active under default256 cannot return the first256 or imply clear.

BEFORE allocating, reserving or materializing EACH full occurrence, resolution
expected-set, entry or replacement collection, obtain its ACTUAL unfiltered typed
COUNT and domain/ordinal/key summaries and compare declarations/visited metadata.
Understated occurrence_count/expected_count/entry_count is invalid_stored_data,
not permission to grow vectors or a result_limit substitute. Validate empty versus
1..N ordinal MIN/MAX, exact key uniqueness/order and complete optional pairs before
using counts. Replacement count is derived from the checked non-NULL entry pairs;
verify exactly those cause/detection/produced-version targets and back-references,
not an unchecked separately growing full-payload collection. THEN apply relevant
per-vector and checked combined ACTUALLY returned count bounds before allocation.
For active views, finish ALL selected-subject anchor/head/history metadata first,
count the complete actual active set with checked addition, apply active-result
limits, THEN load its full returned views. Never materialize the first page before
discovering the rest. Metadata-only walks retain no lifetime/materialization quota;
only their finite page buffers and actual cumulative writer budgets bound them.

find() resolves its selected full current/first bodies even when resolved. history()
first validates full metadata, compares optional checked head (stale_history before
cursor), then requires cursor0 or an existing actual position (cursor_conflict), and
loads only bounded FULL requested event/version bodies after it with explicit next/
has_more. A stale/history-conflict result contains no page. Returned page bodies may
fail independently for malformed documents that narrow metadata did not certify.

## Mutation priority, preparation and write order

Pure options and input precedence/default bounds remain EXACT approved contract;
no SQL/ticket/cleanup on invalid input. Aggregate items/bytes use checked addition,
whole NULL groups and both Exact component SQL classes BEFORE both sizes BEFORE
canonical ASCII/sign/gcd/zero0over1 decode. Bounds and returned-byte formulas are a
separate API review, not guessed from DDL column widths or SQLite heap layout.
Metadata64/65 is independent of default returned page64/active256/entries64.

After ticket access, selected raw mutation identity comes FIRST. Validate its full
ACTUAL stored-kind closure, then exact request equality: exact→already_recorded/
historical produced versions; different kind OR payload→mutation_conflict. Do not
reinterpret a stored resolution as detection because the new caller supplied kind0.
Only a genuinely absent mutation proceeds to current checks.

Detection: absent HoldRef requires absent expected version; existing hold's complete
current closure precedes disposition. CLOSED gives closed_hold BEFORE cause/expected/
proposed-version comparisons. ACTIVE compares cause→hold_conflict, expected current
version→stale_version, then lifetime proposed-version namespace→version_conflict.
Missing hold with supplied expected version is stale_version. A new HoldRef whose
supersedes names active cause without this batch's replacement is hold_conflict.
All healthy conflicts make NO own writes/cleanup; selected corrupt data rolls back.
Validate complete actual sequence before checked N+1; valid signedMAX sequence
exhaustion gives sequence_exhausted/actual rollback, never sparse1/MAX acceptance.

Resolution: full mutation replay/conflict first; then complete actual ActiveSet vs
expected locators→stale_active_set; then each selected entry existence→missing_hold,
closed→closed_hold, expected→stale_version, fresh token→version_conflict. Validate
all replacement fresh hold/detection/version identities and bidirectional immutable
supersedes links→replacement_conflict for healthy collisions; raw mutation-ref
opposite-kind/exact existing identity still follows the shared identity rules above.
Application link requires ALL active set entries closed, zero replacements and full
selected typed application closure. No application link allows closing a subset.
No staged result is proof of current authority, producer policy or same-unit newness.

Resolve ALL bodies/links/counts, exact replay comparisons and expected head CAS;
prepare every result buffer, statement, binding and post-write tracking BEFORE
first own write. Stage without RETURNING/nested transactions:

1. Insert new raw registry record(s): a resolution, and any replacement detections,
   or the one detection. No registry identity is overwritten on conflict.
2. For creation/replacements insert causes (first registry already exists), first
   observations and occurrences. Existing causes stay immutable. Insert detection
   records, attempt observations and audit fields.
3. Insert replacement/new active initial versions; append active existing-detection
   versions or old resolved versions with complete parent/producer/replacement links.
   Existing expected versions and new replacement causes/detections now satisfy FKs.
4. Insert resolution expected/entry rows after their produced versions exist.
5. Insert new heads or UPDATE existing heads LAST under exact prior VersionRef AND
   version_count. Check actual single-row change with a preprepared SELECT changes().
   Every head of a multi-hold batch must advance, or actual WHOLE unit rolls back.

No allocation/control exception after first write can leave a partial batch: guard
all post-write failures with actual rollback; if outcome cannot be established,
preserve original unknown WorkFailure/fail-close. Local selected known failures
establish actual WHOLE caller-unit rollback before reason escapes; writer failures
already rolled back are mapped without a second terminal cleanup. Foreign/stale/
terminal/protocol and unknown stay original. No successful local result/bundle/
historical duplicate substitutes for THIS unit's COMMIT+normal completion.

Adapter writes ONLY these new tables/heads. Whole-input hold creation changes NONE
of application head/state/effect/job/claim/result/plan/contribution dimensions. That
stronger service attempt no-success rule and same-unit hold+application composition
cannot be certified against arbitrary caller SQL by an FK or a returned envelope.
Linked application is DATA only and can be already committed; caller current access,
complete active/other-condition gate, fresh plan authority and actual one-unit commit
are still separate service obligations. Timer mixed/global-status, postfinish,
partial/full continuation and reject/defer semantics remain explicit owning-task
policy gates; recording a supplied annotated choice does not enable those paths.

## Proposal diagnostic and remaining gates

Author independently enumerated new10 tables/11 NULL autoindexes/2 explicit indexes,
framed literals using stdlib decimal-length F/hashlib and checked the unchanged
frozen prefix5, then applied the proposed SQL ONLY to a `/tmp` stdlib SQLite3.54.0
scratch database. Actual89 raw tuples matched the independently assembled full
inventory/CREATE/NULL values; table column counts match; foreign_key_check empty
and integrity_check ok. This is SQL/catalog/framing proposal validation, NOT Qt,
fixture/test authoring, hold behavior, API invocation, intended red or acceptance.
No production objects/database/schema source or old baseline were changed.

Actual physical document review is next. Only afterward explicitly assign minimal
schema/API/stubs with separate review, independent frozen foundation/selected green,
feature tests/full review/discovery and own FIRST intended missing-behavior red.
STORAGE-016 production waits that baseline and separate implementation review,
acceptance/ordinary promoted CI. Preserve all old objects/tests/fixtures and H01–H16.
API exact ownership/result formulas remain a separate concrete stage. All service/
provider/permission/financial/recovery/UI/platform/backup/live/release obligations
remain; no unknown policy/producer capability is invented by this proposal.
