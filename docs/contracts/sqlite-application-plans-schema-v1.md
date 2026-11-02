# Captured application-plan SQLite schema — migration4 proposal

Root/integrator proposal,2026-11-02; [logical companion](sqlite-application-plans-v1.md).
Not production schema source or an authored test fixture. Different actual logical/
physical review precedes API/stubs/tests. Earlier prefixes1/2/3 are immutable.

Catalogformat1/storeidentity subathon.sqlite-store, step4 id
`captured-application-plans-v1`, supported starts0/1/2/3/4. Future pure factory
copies actual contribution manifest1/2/3 and appends this step; no constructor/
open operation auto-migrates or grants operational update authority.

## Exact SQL values

Each fenced single line is one statement, no embedded newline/trailing semicolon.
All five statements create tables; no allocator, seed row or redundant PlanId.

Statement1:

```sql
CREATE TABLE subathon_plan_headers(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0) PRIMARY KEY, event_ref BLOB NOT NULL CHECK(typeof(event_ref)='blob' AND length(event_ref)>0), parent_event_ref BLOB CHECK(parent_event_ref IS NULL OR (typeof(parent_event_ref)='blob')), root_event_ref BLOB NOT NULL CHECK(typeof(root_event_ref)='blob' AND length(root_event_ref)>0), receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000') REFERENCES subathon_receipts(receipt_id), contribution_id BLOB CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND length(contribution_id)=8)) REFERENCES subathon_contributions(contribution_id), head_revision BLOB NOT NULL CHECK(typeof(head_revision)='blob' AND length(head_revision)=8 AND head_revision<>X'0000000000000000'))
```

Statement2:

```sql
CREATE TABLE subathon_plan_revisions(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0) REFERENCES subathon_plan_headers(plan_ref), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), parent_revision BLOB CHECK(parent_revision IS NULL OR (typeof(parent_revision)='blob' AND length(parent_revision)=8 AND parent_revision<>X'0000000000000000')), payment_evidence INTEGER NOT NULL CHECK(typeof(payment_evidence)='integer' AND payment_evidence BETWEEN 0 AND 3), monetary INTEGER NOT NULL CHECK(typeof(monetary)='integer' AND monetary BETWEEN 0 AND 1), original_decimal BLOB, currency BLOB, resolution_ref BLOB CHECK(resolution_ref IS NULL OR (typeof(resolution_ref)='blob')), missing_rate INTEGER NOT NULL CHECK(typeof(missing_rate)='integer' AND missing_rate BETWEEN 0 AND 1), review INTEGER NOT NULL CHECK(typeof(review)='integer' AND review BETWEEN 0 AND 3), evidence_policy INTEGER NOT NULL CHECK(typeof(evidence_policy)='integer' AND evidence_policy BETWEEN 0 AND 2), policy_ref BLOB NOT NULL CHECK(typeof(policy_ref)='blob'), decision_choice INTEGER, decision_ref BLOB, decision_actor_ref BLOB, application INTEGER NOT NULL CHECK(typeof(application)='integer' AND application BETWEEN 0 AND 1), reversal INTEGER NOT NULL CHECK(typeof(reversal)='integer' AND reversal BETWEEN 0 AND 2), settlement INTEGER NOT NULL CHECK(typeof(settlement)='integer' AND settlement BETWEEN 0 AND 1), valuation_state INTEGER NOT NULL CHECK(typeof(valuation_state)='integer' AND valuation_state BETWEEN 0 AND 2), historical_resolution_ref BLOB, historical_original_decimal BLOB, historical_currency BLOB, comparison_currency BLOB, effective_rate_decimal BLOB, fetched_rate_decimal BLOB, snapshot_ref BLOB, rate_source_ref BLOB, rate_timestamp_utc_ms INTEGER, fetch_timestamp_utc_ms INTEGER, manual_decision_ref BLOB, override_authority INTEGER, override_actor_ref BLOB, override_reason BLOB, override_decision_ref BLOB, PRIMARY KEY(plan_ref,revision), FOREIGN KEY(plan_ref,parent_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), CHECK((original_decimal IS NULL AND currency IS NULL) OR (typeof(original_decimal)='blob' AND typeof(currency)='blob')), CHECK((decision_choice IS NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) OR (typeof(decision_choice)='integer' AND decision_choice IN (0,1) AND typeof(decision_ref)='blob' AND typeof(decision_actor_ref)='blob')), CHECK((valuation_state IN (0,1) AND historical_resolution_ref IS NULL AND historical_original_decimal IS NULL AND historical_currency IS NULL AND comparison_currency IS NULL AND effective_rate_decimal IS NULL AND fetched_rate_decimal IS NULL AND snapshot_ref IS NULL AND rate_source_ref IS NULL AND rate_timestamp_utc_ms IS NULL AND fetch_timestamp_utc_ms IS NULL AND manual_decision_ref IS NULL) OR (valuation_state=2 AND typeof(historical_resolution_ref)='blob' AND typeof(historical_original_decimal)='blob' AND typeof(historical_currency)='blob' AND typeof(comparison_currency)='blob' AND typeof(effective_rate_decimal)='blob' AND typeof(snapshot_ref)='blob' AND (fetched_rate_decimal IS NULL OR typeof(fetched_rate_decimal)='blob') AND (rate_source_ref IS NULL OR typeof(rate_source_ref)='blob') AND (manual_decision_ref IS NULL OR typeof(manual_decision_ref)='blob') AND (rate_timestamp_utc_ms IS NULL OR typeof(rate_timestamp_utc_ms)='integer') AND (fetch_timestamp_utc_ms IS NULL OR typeof(fetch_timestamp_utc_ms)='integer'))), CHECK((revision=X'0000000000000001' AND parent_revision IS NULL AND override_authority IS NULL AND override_actor_ref IS NULL AND override_reason IS NULL AND override_decision_ref IS NULL) OR (revision<>X'0000000000000001' AND typeof(parent_revision)='blob' AND length(parent_revision)=8 AND parent_revision<>X'0000000000000000' AND parent_revision<revision AND typeof(override_authority)='integer' AND override_authority=2 AND typeof(override_actor_ref)='blob' AND length(override_actor_ref)>0 AND typeof(override_reason)='blob' AND length(override_reason)>0 AND typeof(override_decision_ref)='blob' AND length(override_decision_ref)>0)))
```

Statement3:

```sql
CREATE TABLE subathon_plan_rules(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), rule_ref BLOB NOT NULL CHECK(typeof(rule_ref)='blob'), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob'), selected INTEGER NOT NULL CHECK(typeof(selected)='integer' AND selected BETWEEN 0 AND 1), explanation BLOB NOT NULL CHECK(typeof(explanation)='blob'), PRIMARY KEY(plan_ref,revision,ordinal), FOREIGN KEY(plan_ref,revision) REFERENCES subathon_plan_revisions(plan_ref,revision))
```

Statement4:

```sql
CREATE TABLE subathon_plan_modifiers(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), definition_version_ref BLOB NOT NULL CHECK(typeof(definition_version_ref)='blob'), instance_revision_ref BLOB NOT NULL CHECK(typeof(instance_revision_ref)='blob'), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob'), group_ref BLOB NOT NULL CHECK(typeof(group_ref)='blob'), group_operator BLOB NOT NULL CHECK(typeof(group_operator)='blob'), target_operator BLOB NOT NULL CHECK(typeof(target_operator)='blob'), original_factor_decimal BLOB NOT NULL CHECK(typeof(original_factor_decimal)='blob'), PRIMARY KEY(plan_ref,revision,ordinal), FOREIGN KEY(plan_ref,revision) REFERENCES subathon_plan_revisions(plan_ref,revision))
```

Statement5:

```sql
CREATE TABLE subathon_plan_requests(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob'), action_version_ref BLOB NOT NULL CHECK(typeof(action_version_ref)='blob'), rule_ref BLOB NOT NULL CHECK(typeof(rule_ref)='blob'), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob'), unit INTEGER NOT NULL CHECK(typeof(unit)='integer' AND unit BETWEEN 0 AND 2), requires_historical_valuation INTEGER NOT NULL CHECK(typeof(requires_historical_valuation)='integer' AND requires_historical_valuation BETWEEN 0 AND 1), exact_numerator BLOB NOT NULL CHECK(typeof(exact_numerator)='blob' AND length(exact_numerator)>0), exact_denominator BLOB NOT NULL CHECK(typeof(exact_denominator)='blob' AND length(exact_denominator)>0), requested_units INTEGER NOT NULL CHECK(typeof(requested_units)='integer'), calculation_explanation BLOB NOT NULL CHECK(typeof(calculation_explanation)='blob'), reversal_behavior_ref BLOB NOT NULL CHECK(typeof(reversal_behavior_ref)='blob'), PRIMARY KEY(plan_ref,revision,ordinal), FOREIGN KEY(plan_ref,revision) REFERENCES subathon_plan_revisions(plan_ref,revision))
```

## Exact full projection and value mapping

Keep every prefix3 object (20) byte-for-byte, including prior7/13 projections.
Add the following10 objects, then sort/frame full30 by unchanged catalog rules:

| type | name | tbl_name | sql |
| --- | --- | --- | --- |
| table | subathon_plan_headers | subathon_plan_headers | Statement1 |
| index | sqlite_autoindex_subathon_plan_headers_1 | subathon_plan_headers | NULL |
| table | subathon_plan_revisions | subathon_plan_revisions | Statement2 |
| index | sqlite_autoindex_subathon_plan_revisions_1 | subathon_plan_revisions | NULL |
| table | subathon_plan_rules | subathon_plan_rules | Statement3 |
| index | sqlite_autoindex_subathon_plan_rules_1 | subathon_plan_rules | NULL |
| table | subathon_plan_modifiers | subathon_plan_modifiers | Statement4 |
| index | sqlite_autoindex_subathon_plan_modifiers_1 | subathon_plan_modifiers | NULL |
| table | subathon_plan_requests | subathon_plan_requests | Statement5 |
| index | sqlite_autoindex_subathon_plan_requests_1 | subathon_plan_requests | NULL |

All five tables start empty. Header has7 columns, revision34, rule7, modifier10,
request14. Opaque plan_ref and event/root refs are nonempty BLOB. Optional parent
Event ref preserves SQLNULL/presentempty; original receipt and optionalCID point
to actual typed namespaces. Receipt identity/facts/arrival hydrate from immutable
receipt originals; header never duplicates growing attempt history. No plan-per-
receipt/event UNIQUE constraint is invented; only plan_ref identifies a plan.

Actual revision/head/parent are strictly BLOB8/nonzero, optional parent alone can
be SQLNULL. ContributionId0 remains allowed. Revision PK(plan_ref,revision) and
child PK(plan_ref,revision,ordinal) enforce identity/order-position uniqueness.
Ordinals are INTEGER1..signedMAX; empty collections and duplicate CONTENT remain
valid. Actual FK membership protects headers/revisions/parents/children. No cycle,
deferred-FK dependency, trigger, extra writer or driver-specific RETURNING is added.
Header head has no cyclic FK to revisions: whole-unit staging, actual-max/head
agreement, count/continuity and full versus targeted validation are runtime gates.

The revision role CHECK requires initial1/no parent/override. Later rows require
canonical nonzero parent<revision and known authorized2/nonempty actor/reason/
decision. SQL's lexicographic parent<revision is NOT proof of immediate parent=n-1;
that full unsigned invariant is enforced by runtime, without signed/REAL math.
This intentionally permits a disposable sparse diagnostic1,MAX-1,MAX where the
intermediary(MAX-1) names parent1: FKs/basicSQL shape hold, but it is an INVALID
complete core history. Targeted MAX may prove exact field/localparent MAX-1 plus
initial anchor, never certify unselected parent's own chain/body or global
continuity. Full find/append MUST refuse count/head/immediate-parent mismatch;
no valid real-chain exhaustion or supported import/repair path is claimed.

Revision scalar columns retain every independent contribution state dimension.
Own contribution-state original_decimal/currency pair and optional decision group
are complete or SQLNULL together; semantic strings may be BLOBempty as existing
core permits. Plan valuation0/1 has all11 historical columns NULL; resolved2 has
all six required BLOB fields plus five independently optional string/time fields.
Historical field mapping: resolution_ref -> historical_resolution_ref,
original_money -> historical_original_decimal/historical_currency,
comparison_currency/effective_rate_decimal/fetched_rate_decimal/snapshot_ref,
source_ref -> rate_source_ref, rate_timestamp/fetch_timestamp and manual_decision_ref.
No precision/time/source/manual value is inferred or normalized. Quote's original
money equality against event money and missing-historical/request dependency flags
are cross-row runtime checks, not new SQL evaluator/permission truth.

Rules/modifiers/requests preserve their actual fields in ordinal order. Exact
numerator/denominator are nonempty BLOB; canonical signed digits, positive reduced
denominator/zero0over1, configured component limits and exact arithmetic decoding
remain runtime codec gates. SQL never coerces them to INTEGER/REAL or reuses the
numeric parser/finalizer's unrelated bounds. Requested units/timestamps use actual
INTEGER/signed64; enums/bools have explicit typeof/value guards. Other physical
payload/ref bounds remain trusted configurable limits, not hardcoded row quotas.
Unknown semantic namespaces stay upstream, no fake configuration FK or authority.

## Actual independent proposal observations and remaining gates

Root independently generated these literal statements from actual logical fields,
applied frozen prefix SQL with Python stdlib SQLite (no product factory/codec),
compared all30 actual schema tuples/exactCREATE/NULLindex SQL and checked unchanged
20/13/7 prefixes. New5 indexes have SQLNULL. Independent F framing/hashlib yields:

- Full canonical payload: **29015 bytes**.
- SHA256: `bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50`.

Root scratch script /tmp/subathon-plan-schema-proposal.py and ignored proposal
JSON local-evidence/coordinator/storagetest015-schema-proposal.json retain all
literal metadata/SQL/projection/fullpayloadhex for diagnostics. They are NOT a
new baseline fixture, production output or selected official Qt proof. Actual
Qt-driver projection/data diagnostics and different full review remain separate.
No new API/stub/schema-source/test/feature/red has been authored or invoked. All
full-release/provider/FX/UI/application/recovery/platform gates stay required.
