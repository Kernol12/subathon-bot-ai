# SQLite application bundle — physical proposal revision 1

Author document proposal for [the concrete contract](sqlite-application-bundle-v1.md).
No migration source, manifest implementation, API, fixture or executable test is supplied.
Root owns additive migration ordering/integration. Proposed step5 ID
`atomic-application-bundles-v1`, catalog format1/storeidentity unchanged, starts0–5.
Pure future manifest copies exact actual prefix1–4 and appends these statements.
No constructor auto-migrates. Different actual review precedes later API/stubs/tests.

## Exact proposed SQL

Each fenced SINGLE line is one exact statement, no newline/trailing semicolon.
All14 tables start empty; no trigger, allocator, seed or new numeric domain ID.

Statement1: `subathon_app_states`.

```sql
CREATE TABLE subathon_app_states(token BLOB NOT NULL CHECK(typeof(token)='blob' AND length(token)>0), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), lifecycle INTEGER NOT NULL CHECK(typeof(lifecycle)='integer' AND lifecycle>=0 AND lifecycle<=2), phase INTEGER NOT NULL CHECK(typeof(phase)='integer' AND phase>=0 AND phase<=1), main_remaining_ms INTEGER NOT NULL CHECK(typeof(main_remaining_ms)='integer' AND main_remaining_ms>=0), grace_remaining_ms INTEGER NOT NULL CHECK(typeof(grace_remaining_ms)='integer' AND grace_remaining_ms>=0), awaiting_confirmation INTEGER NOT NULL CHECK(typeof(awaiting_confirmation)='integer' AND awaiting_confirmation>=0 AND awaiting_confirmation<=1), time_debt_ms INTEGER NOT NULL CHECK(typeof(time_debt_ms)='integer' AND time_debt_ms>=0), balance_count INTEGER NOT NULL CHECK(typeof(balance_count)='integer' AND balance_count>=0), PRIMARY KEY(token), FOREIGN KEY(configuration_ref) REFERENCES subathon_retained_configurations(reference), CHECK(phase<>1 OR (main_remaining_ms=0 AND awaiting_confirmation=0)), CHECK(awaiting_confirmation=0 OR (lifecycle=1 AND phase=0 AND main_remaining_ms=0)))
```

Statement2: `subathon_app_balances`.

```sql
CREATE TABLE subathon_app_balances(token BLOB NOT NULL CHECK(typeof(token)='blob' AND length(token)>0), balance_kind INTEGER NOT NULL CHECK(typeof(balance_kind)='integer' AND balance_kind>=2 AND balance_kind<=3), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob' AND length(target_ref)>0), units INTEGER NOT NULL CHECK(typeof(units)='integer'), PRIMARY KEY(token,balance_kind,target_ref), FOREIGN KEY(token) REFERENCES subathon_app_states(token))
```

Statement3: `subathon_app_head`.

```sql
CREATE TABLE subathon_app_head(singleton INTEGER PRIMARY KEY CHECK(singleton=1), token BLOB NOT NULL CHECK(typeof(token)='blob' AND length(token)>0), FOREIGN KEY(token) REFERENCES subathon_app_states(token))
```

Statement4: `subathon_app_operations`.

```sql
CREATE TABLE subathon_app_operations(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), operation_kind INTEGER NOT NULL CHECK(typeof(operation_kind)='integer' AND operation_kind>=0 AND operation_kind<=1), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind>=0 AND subject_kind<=1), receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), contribution_id BLOB CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND length(contribution_id)=8)), plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND plan_revision<>X'0000000000000000'), head_token BLOB NOT NULL CHECK(typeof(head_token)='blob' AND length(head_token)>0), before_token BLOB NOT NULL CHECK(typeof(before_token)='blob' AND length(before_token)>0), after_token BLOB NOT NULL CHECK(typeof(after_token)='blob' AND length(after_token)>0), advance_explanation BLOB NOT NULL CHECK(typeof(advance_explanation)='blob'), choices_format_ref BLOB NOT NULL CHECK(typeof(choices_format_ref)='blob' AND length(choices_format_ref)>0), choices_document BLOB NOT NULL CHECK(typeof(choices_document)='blob'), reversal_ref BLOB CHECK(reversal_ref IS NULL OR (typeof(reversal_ref)='blob' AND length(reversal_ref)>0)), PRIMARY KEY(operation_scope,operation_ref), UNIQUE(after_token), UNIQUE(operation_kind,receipt_id), UNIQUE(operation_scope,reversal_ref), UNIQUE(operation_scope,operation_ref,plan_ref,plan_revision), UNIQUE(operation_scope,operation_ref,before_token), FOREIGN KEY(receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(contribution_id) REFERENCES subathon_contributions(contribution_id), FOREIGN KEY(plan_ref,plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(head_token) REFERENCES subathon_app_states(token), FOREIGN KEY(before_token) REFERENCES subathon_app_states(token), FOREIGN KEY(after_token) REFERENCES subathon_app_states(token), CHECK((subject_kind=0 AND contribution_id IS NOT NULL) OR (subject_kind=1 AND contribution_id IS NULL)), CHECK((operation_kind=0 AND reversal_ref IS NULL) OR (operation_kind=1 AND reversal_ref IS NOT NULL)), CHECK(before_token<>after_token AND head_token<>after_token))
```

Statement5: `subathon_app_contributions`.

```sql
CREATE TABLE subathon_app_contributions(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), contribution_id BLOB NOT NULL CHECK(typeof(contribution_id)='blob' AND length(contribution_id)=8), payment_evidence INTEGER NOT NULL CHECK(typeof(payment_evidence)='integer' AND payment_evidence>=0 AND payment_evidence<=3), monetary INTEGER NOT NULL CHECK(typeof(monetary)='integer' AND monetary>=0 AND monetary<=1), original_decimal BLOB CHECK(original_decimal IS NULL OR (typeof(original_decimal)='blob')), currency BLOB CHECK(currency IS NULL OR (typeof(currency)='blob')), resolution_ref BLOB CHECK(resolution_ref IS NULL OR (typeof(resolution_ref)='blob')), missing_rate INTEGER NOT NULL CHECK(typeof(missing_rate)='integer' AND missing_rate>=0 AND missing_rate<=1), review INTEGER NOT NULL CHECK(typeof(review)='integer' AND review>=0 AND review<=3), evidence_policy INTEGER NOT NULL CHECK(typeof(evidence_policy)='integer' AND evidence_policy>=0 AND evidence_policy<=2), policy_ref BLOB NOT NULL CHECK(typeof(policy_ref)='blob'), decision_choice INTEGER CHECK(decision_choice IS NULL OR (typeof(decision_choice)='integer' AND decision_choice>=0 AND decision_choice<=1)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob')), decision_actor_ref BLOB CHECK(decision_actor_ref IS NULL OR (typeof(decision_actor_ref)='blob')), application INTEGER NOT NULL CHECK(typeof(application)='integer' AND application>=0 AND application<=1), reversal INTEGER NOT NULL CHECK(typeof(reversal)='integer' AND reversal>=0 AND reversal<=2), settlement INTEGER NOT NULL CHECK(typeof(settlement)='integer' AND settlement>=0 AND settlement<=1), application_after INTEGER NOT NULL CHECK(typeof(application_after)='integer' AND application_after>=0 AND application_after<=1), PRIMARY KEY(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(contribution_id) REFERENCES subathon_contributions(contribution_id), CHECK((original_decimal IS NULL AND currency IS NULL) OR (typeof(original_decimal)='blob' AND typeof(currency)='blob')), CHECK((decision_choice IS NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) OR (typeof(decision_choice)='integer' AND decision_choice IN (0,1) AND typeof(decision_ref)='blob' AND typeof(decision_actor_ref)='blob')))
```

Statement6: `subathon_app_claims`.

```sql
CREATE TABLE subathon_app_claims(subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind>=0 AND subject_kind<=1), subject_id BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), PRIMARY KEY(subject_kind,subject_id), UNIQUE(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref), CHECK(subject_kind=0 OR subject_id<>X'0000000000000000'))
```

Statement7: `subathon_app_effects`.

```sql
CREATE TABLE subathon_app_effects(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND plan_revision<>X'0000000000000000'), arithmetic_role INTEGER NOT NULL CHECK(typeof(arithmetic_role)='integer' AND arithmetic_role>=0 AND arithmetic_role<=2), unit INTEGER NOT NULL CHECK(typeof(unit)='integer' AND unit>=0 AND unit<=2), exact_numerator BLOB NOT NULL CHECK(typeof(exact_numerator)='blob' AND length(exact_numerator)>0), exact_denominator BLOB NOT NULL CHECK(typeof(exact_denominator)='blob' AND length(exact_denominator)>0), requested_units INTEGER NOT NULL CHECK(typeof(requested_units)='integer'), residual_numerator BLOB NOT NULL CHECK(typeof(residual_numerator)='blob' AND length(residual_numerator)>0), residual_denominator BLOB NOT NULL CHECK(typeof(residual_denominator)='blob' AND length(residual_denominator)>0), maximum_main_ms INTEGER CHECK(maximum_main_ms IS NULL OR (typeof(maximum_main_ms)='integer' AND maximum_main_ms>=0)), deficit_policy INTEGER CHECK(deficit_policy IS NULL OR (typeof(deficit_policy)='integer' AND deficit_policy>=0 AND deficit_policy<=2)), deficit_disposition INTEGER CHECK(deficit_disposition IS NULL OR (typeof(deficit_disposition)='integer' AND deficit_disposition>=0 AND deficit_disposition<=1)), application_explanation BLOB NOT NULL CHECK(typeof(application_explanation)='blob'), PRIMARY KEY(operation_scope,operation_ref,request_ordinal), FOREIGN KEY(operation_scope,operation_ref,plan_ref,plan_revision) REFERENCES subathon_app_operations(operation_scope,operation_ref,plan_ref,plan_revision), FOREIGN KEY(plan_ref,plan_revision,request_ordinal) REFERENCES subathon_plan_requests(plan_ref,revision,ordinal), CHECK((arithmetic_role=0 AND unit IN (1,2) AND maximum_main_ms IS NULL AND deficit_policy IS NULL AND deficit_disposition IS NULL) OR (arithmetic_role=1 AND unit=0 AND deficit_policy IS NULL AND deficit_disposition IS NULL) OR (arithmetic_role=2 AND unit=0 AND maximum_main_ms IS NULL AND deficit_policy IS NOT NULL AND deficit_disposition IS NOT NULL)))
```

Statement8: `subathon_app_components`.

```sql
CREATE TABLE subathon_app_components(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), component_kind INTEGER NOT NULL CHECK(typeof(component_kind)='integer' AND component_kind>=0 AND component_kind<=5), units INTEGER NOT NULL CHECK(typeof(units)='integer'), before_token BLOB NOT NULL CHECK(typeof(before_token)='blob' AND length(before_token)>0), target_ref BLOB CHECK(target_ref IS NULL OR (typeof(target_ref)='blob' AND length(target_ref)>0)), PRIMARY KEY(operation_scope,operation_ref,request_ordinal,component_kind), FOREIGN KEY(operation_scope,operation_ref,request_ordinal) REFERENCES subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN KEY(operation_scope,operation_ref,before_token) REFERENCES subathon_app_operations(operation_scope,operation_ref,before_token), FOREIGN KEY(before_token,component_kind,target_ref) REFERENCES subathon_app_balances(token,balance_kind,target_ref), CHECK((component_kind IN (2,3) AND target_ref IS NOT NULL) OR (component_kind NOT IN (2,3) AND target_ref IS NULL)))
```

Statement9: `subathon_app_transitions`.

```sql
CREATE TABLE subathon_app_transitions(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), transition INTEGER NOT NULL CHECK(typeof(transition)='integer' AND transition>=0 AND transition<=5), PRIMARY KEY(operation_scope,operation_ref,ordinal), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref))
```

Statement10: `subathon_app_jobs`.

```sql
CREATE TABLE subathon_app_jobs(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), job_ref BLOB NOT NULL CHECK(typeof(job_ref)='blob' AND length(job_ref)>0), source_request_ordinal INTEGER CHECK(source_request_ordinal IS NULL OR (typeof(source_request_ordinal)='integer' AND source_request_ordinal>=1)), group_ref BLOB CHECK(group_ref IS NULL OR (typeof(group_ref)='blob' AND length(group_ref)>0)), group_ordinal INTEGER CHECK(group_ordinal IS NULL OR (typeof(group_ordinal)='integer' AND group_ordinal>=1)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), PRIMARY KEY(operation_scope,operation_ref,ordinal), UNIQUE(operation_scope,operation_ref,job_ref), UNIQUE(operation_scope,operation_ref,group_ref,group_ordinal), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref,source_request_ordinal) REFERENCES subathon_app_effects(operation_scope,operation_ref,request_ordinal), CHECK((group_ref IS NULL AND group_ordinal IS NULL) OR (group_ref IS NOT NULL AND group_ordinal IS NOT NULL)))
```

Statement11: `subathon_app_results`.

```sql
CREATE TABLE subathon_app_results(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), effect_count INTEGER NOT NULL CHECK(typeof(effect_count)='integer' AND effect_count>=0), component_count INTEGER NOT NULL CHECK(typeof(component_count)='integer' AND component_count>=0), job_count INTEGER NOT NULL CHECK(typeof(job_count)='integer' AND job_count>=0), transition_count INTEGER NOT NULL CHECK(typeof(transition_count)='integer' AND transition_count>=0), reversal_count INTEGER NOT NULL CHECK(typeof(reversal_count)='integer' AND reversal_count>=0), PRIMARY KEY(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref))
```

Statement12: `subathon_app_audit`.

```sql
CREATE TABLE subathon_app_audit(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob')), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob')), reason BLOB CHECK(reason IS NULL OR (typeof(reason)='blob')), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), PRIMARY KEY(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref))
```

Statement13: `subathon_app_reversals`.

```sql
CREATE TABLE subathon_app_reversals(operation_scope BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), original_scope BLOB NOT NULL CHECK(typeof(original_scope)='blob' AND length(original_scope)>0), original_operation BLOB NOT NULL CHECK(typeof(original_operation)='blob' AND length(original_operation)>0), original_request_ordinal INTEGER NOT NULL CHECK(typeof(original_request_ordinal)='integer' AND original_request_ordinal>=1), original_component_kind INTEGER NOT NULL CHECK(typeof(original_component_kind)='integer' AND original_component_kind>=0 AND original_component_kind<=3), basis_position INTEGER NOT NULL CHECK(typeof(basis_position)='integer' AND basis_position>=1), fraction_numerator BLOB NOT NULL CHECK(typeof(fraction_numerator)='blob' AND length(fraction_numerator)>0), fraction_denominator BLOB NOT NULL CHECK(typeof(fraction_denominator)='blob' AND length(fraction_denominator)>0), entitlement_numerator BLOB NOT NULL CHECK(typeof(entitlement_numerator)='blob' AND length(entitlement_numerator)>0), entitlement_denominator BLOB NOT NULL CHECK(typeof(entitlement_denominator)='blob' AND length(entitlement_denominator)>0), residual_numerator BLOB NOT NULL CHECK(typeof(residual_numerator)='blob' AND length(residual_numerator)>0), residual_denominator BLOB NOT NULL CHECK(typeof(residual_denominator)='blob' AND length(residual_denominator)>0), previous_cumulative INTEGER NOT NULL CHECK(typeof(previous_cumulative)='integer'), cumulative INTEGER NOT NULL CHECK(typeof(cumulative)='integer'), requested_increment INTEGER NOT NULL CHECK(typeof(requested_increment)='integer'), full_refund INTEGER NOT NULL CHECK(typeof(full_refund)='integer' AND full_refund>=0 AND full_refund<=1), PRIMARY KEY(operation_scope,operation_ref,request_ordinal,original_scope,original_operation,original_request_ordinal,original_component_kind), UNIQUE(original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position), FOREIGN KEY(operation_scope,operation_ref,request_ordinal) REFERENCES subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN KEY(original_scope,original_operation,original_request_ordinal,original_component_kind) REFERENCES subathon_app_components(operation_scope,operation_ref,request_ordinal,component_kind), CHECK(original_component_kind IN (0,2,3)))
```

Statement14: `subathon_app_reversal_heads`.

```sql
CREATE TABLE subathon_app_reversal_heads(original_scope BLOB NOT NULL CHECK(typeof(original_scope)='blob' AND length(original_scope)>0), original_operation BLOB NOT NULL CHECK(typeof(original_operation)='blob' AND length(original_operation)>0), original_request_ordinal INTEGER NOT NULL CHECK(typeof(original_request_ordinal)='integer' AND original_request_ordinal>=1), original_component_kind INTEGER NOT NULL CHECK(typeof(original_component_kind)='integer' AND original_component_kind>=0 AND original_component_kind<=3), basis_position INTEGER NOT NULL CHECK(typeof(basis_position)='integer' AND basis_position>=1), cumulative INTEGER NOT NULL CHECK(typeof(cumulative)='integer'), last_scope BLOB NOT NULL CHECK(typeof(last_scope)='blob' AND length(last_scope)>0), last_operation BLOB NOT NULL CHECK(typeof(last_operation)='blob' AND length(last_operation)>0), last_request_ordinal INTEGER NOT NULL CHECK(typeof(last_request_ordinal)='integer' AND last_request_ordinal>=1), PRIMARY KEY(original_scope,original_operation,original_request_ordinal,original_component_kind), FOREIGN KEY(original_scope,original_operation,original_request_ordinal,original_component_kind) REFERENCES subathon_app_components(operation_scope,operation_ref,request_ordinal,component_kind), FOREIGN KEY(original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position) REFERENCES subathon_app_reversals(original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position), FOREIGN KEY(last_scope,last_operation,last_request_ordinal,original_scope,original_operation,original_request_ordinal,original_component_kind) REFERENCES subathon_app_reversals(operation_scope,operation_ref,request_ordinal,original_scope,original_operation,original_request_ordinal,original_component_kind), CHECK(original_component_kind IN (0,2,3)))
```

## Expected catalog projection

All existing30 prefix4 objects remain byte-for-byte. The new14 tables and
22 autoindexes give **66 cumulative objects**. Sort by UTF8 type/name with
existing catalog framing; each table SQL is its exact statement above; every
autoindex SQL is NULL.

| Table | Statement | Autoindex suffixes |
| --- | --- | --- |
| `subathon_app_states` | 1 | _1 |
| `subathon_app_balances` | 2 | _1 |
| `subathon_app_head` | 3 | none (INTEGER PRIMARY KEY) |
| `subathon_app_operations` | 4 | _1, _2, _3, _4, _5, _6 |
| `subathon_app_contributions` | 5 | _1 |
| `subathon_app_claims` | 6 | _1, _2 |
| `subathon_app_effects` | 7 | _1 |
| `subathon_app_components` | 8 | _1 |
| `subathon_app_transitions` | 9 | _1 |
| `subathon_app_jobs` | 10 | _1, _2, _3 |
| `subathon_app_results` | 11 | _1 |
| `subathon_app_audit` | 12 | _1 |
| `subathon_app_reversals` | 13 | _1, _2 |
| `subathon_app_reversal_heads` | 14 | _1 |

## Column mapping and row closure

All strings/documents/components/keys are exact BLOB; SQL NULL is optional absence,
not empty BLOB or ID0. INTEGER fields require actual integer storage class. SQL
INTEGER affinity may store an exactly converted value as INTEGER; runtime checks
inspect the ACTUAL stored class/value, not its submitted literal spelling. No REAL
money/ID/rational conversion is allowed. Runtime limits remain configurable and are
not hardcoded into DDL; exact rational grammar/reduction is also a runtime check.

| Table | Logical role / selected validation beyond SQL |
| --- | --- |
| states | Immutable opaque token binds format/document/configuration +existing TimerState/debt and declared balance_count. No update/delete/rebind API; fresh token requires absence, even if payload equal. Actual retained cfg resolves; format is owned version tag, not fake cfg FK |
| balances | Actual typed snapshot counter namespace `(token,kind2/3,target_ref)`. Strict canonical key order/unique keys; actual count equals states.balance_count. Typed values are signed64; meaning/authority of target remains caller contract |
| head | Singleton1 points to actual immutable current snapshot. Absence is fresh only when states is empty; it never resets history. Stage CAS checks expected head; no numeric StateRevision |
| operations | Own scoped operation key and full structured intent; explicit subject role/CID presence, actual receipt/plan FKs, persisted head token, supplied advanced-before token, after token, advance explanation, choices and optional reversal identity. UNIQUE after token/source receipt/reversal identity protects same logical scopes as service checks. Head/before may coincide; after differs and was never present |
| contributions | Exactly one historical expected CURRENT-state row iff operation has CID; contribution_id MUST equal operation CID. Complete15 independent dimensions preserved. application_after=1 for original and unchanged for reversal. No owning14 transaction is instantiated; only live application column is patched for original |
| claims | Exactly one iff original-kind; kind/id MUST equal operation's explicitly tagged CID or ReceiptId, scoped operation must match. Reversal has no original-award claim. Polymorphic subject FK equivalence is a runtime check, not a fabricated cross-domain FK |
| effects | Every actual selected immutable plan request ordinal1..N exactly once; copied plan_ref/revision equals operation through composite FK. Unit/Exact/requested values MUST match actual request, role/components must agree. No splitting immutable request ordinals to fit reversal layout |
| components | Fixed kind identity within current request, typed signed64 amount, before_token MUST equal operation advanced-before token. Counter2/3 FK selects actual before balance/target; target must match selected request. Role exact sets/signs/partition and exact before→after deltas/keysets/counts are runtime checks; FKs alone prove none of those equations |
| transitions | Ordered supplied core TimerTransition vocabulary0..5, contiguous ordinal1..N, duplicates preserved. Not generated event timestamps or proof a clock was advanced legally |
| jobs | Global vector ordinal1..N; UNIQUE job_ref WITHIN operation; optional source ordinal belongs to that operation's actual effect; optional grouped references are a whole pair, group ordinal1..group count with no gaps/duplicates. Groups/jobrefs are operation-scoped, not global namespaces. Envelope is immutable intent, no delivery state machine |
| results | Exactly one fixed row, own versioned document and declared effect/component/job/transition/reversal counts equal actual collections. Same-unit row/read is provisional. No success publication or incomplete-result repair |
| audit | Exactly one fixed row preserving optional known actor/decision/reason plus owned versioned content. Nonempty references grant no permission; absent remains SQLNULL |
| reversals | MANY original-component links can belong to ONE current immutable request. Composite PK is current request PLUS original component; a link's requested_increment is typed signed64. `(original component,basis_position)` is unique append order, NOT a new domain ID. One basis can recur at later current ordinals/positions in the same operation, ordered as declared in logical contract |
| reversal_heads | Mutable progress per actual original component. BOTH FK paths must identify the SAME reversal link/basis/position/cumulative; not just two individually existent rows. Actual count/min/max/sequence/cumulative chain and last-row identity must agree; stale expected progress refuses without own writes. No reset/wrap/repair |

FKs resolve only declared ACTUAL physical namespaces: receipts, contributions,
plan revision/request, retained configuration, state token/counter, operation,
effect/component and reversal occurrence. Original-support ReceiptId need not equal
this operation's event receipt. No actor/rule/action/job-format/event ancestry string
is converted into an unrelated configuration FK or authorization proof.

Full original operation closure includes exactly one claim, optional expected
contribution row, result and audit; all declared counts and actual ordered children
must agree before a lookup returns. Reversal original-source closure is the narrower
fixed metadata+selected actual effect/component/target/plan/source scope defined in
logical document, not a claim that all unselected old job bodies were decoded.
Historical lookups do not require their after token or historical contribution
snapshot to equal today's head/current state. Stateful freshness checks occur only
for a NEW stage after existing-operation/claim handling.

## Storage-only envelopes and numeric boundaries

The concrete baseline may use these explicit synthetic format meanings:

- `subathon.storage.synthetic-state.v1`: document is an owned opaque annotation of
  the supplied typed snapshot/observation. Typed timer/debt/balance fields are kept
  separately. It provides NO clock authority, portable checkpoint/recovery policy,
  pause/deadline interpretation or parser for real state configuration.
- `subathon.storage.synthetic-job.v1`: document is an owned opaque synthetic action
  label. Typed operation/source/group order supplies storage provenance. It is NOT
  deliverable and supplies no provider/recipient/sending identity/retry semantics.
- `subathon.storage.synthetic-choice.v1`, `synthetic-result.v1` and
  `synthetic-audit.v1` (same prefix): owned opaque labels for storage-only choices,
  result annotation and audit annotation. Structured Intent/numeric fields remain
  separate. No command permissions or wire DTO follows from these labels.

These bytes/version tags have complete stated TEST meaning, not a promised full
job/domain format. The adapter stores any caller-contract-defined bounded envelope
losslessly, but actual production consumers must separately agree/validate the
format they understand before enablement. Unknown semantic strings remain unknown.
No schema parser or executable payload is invented by a BLOB column.

Signed64 actual units/time/count/cumulatives and canonical rational VALUE components
remain lossless. Runtime checks use exact/checked algebra; no unsigned ID through
signed64, no floating point, no signed negation of INT64_MIN. Whole snapshots and
all links commit or roll back together. An ordinary direct signed correction is
retained as such; the proposed financial progress model uses actual nonnegative
credited Main/score/count bases, not debt delta or a new financial policy for undoing
negative corrections. Original Main credit that repaid debt is gated until ACCT-006
settles allocation; it is never silently treated as fully refundable Main.

## Write sequence, injected failures and migration boundary

No UPDATE except head advance, current contribution application-only patch and
reversal progress is exposed. All snapshots/operations/claims/effects/children/
results/audits are immutable. Prepare ALL allocating buffers before writes; guard
post-write allocation/control errors across the whole existing caller unit.

Stage inserts fresh advanced-before/after snapshots+balances, operation, historical
contribution snapshot/original claim, effects/components/transitions; patches ONLY
application for original; inserts jobs/result/audit/reversal links and progress;
advances current head LAST after actual expected-token check. It never commits.
This order permits a disposable BEFORE INSERT job/result fault conditional on
ACTUAL operation/effect/after-snapshot/current application patch existence. A real
RAISE(ABORT) statement and separate old-writer/raw proof establish fault reach;
inside-catch independent BEGIN IMMEDIATE/full typed snapshot must establish actual
whole rollback. A fabricated callback outcome or later caller fault alone is not
that mid-stage proof. Real COMMIT/ROLLBACK acknowledgement-loss seams cover their
separate unknown outcomes; no test trigger enters the supported production catalog.

Earlier prefix1/2/3/4 objects/payloads remain EXACT7/13/20/30 objects and
2000/10535/18126/29015 framed bytes. Migration source/manifest/order belongs to root
only after this ACTUAL proposal is independently reviewed. Empty raw catalog/FK
acceptance proves neither data constraints nor implementation/transaction correctness.
Independent data constraint/fixture authoring, actual API review/test baseline/red,
implementation/source review and ordinary automatic acceptance remain later gates.

## Actual independent catalog/framing diagnostic

Root independently parsed these exact14 SQL values and constructed the expected
14-table/22-NULL-index inventory before comparing actual raw SQLite3.54.0 catalog.
Existing30 prefix4 tuples are byte-exact; empty foreign_key_check and integrity_check
pass. Independent manual canonical framing reproduces old step4 F29015/SHA256
`bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50`, then gives this
proposed step5 **F62465** / SHA256
`c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb`.
This is an attributed RAW schema/framing diagnostic, not future API execution,
fixture/test approval or passing application behavior. Independent fixture author/
reviewer must derive/freeze actual later literal expectations from the reviewed
proposal; production manifest/codec must never generate its test oracle.

Actual column counts in statement order are11,4,2,15,19,4,16,7,4,9,9,7,18,9.
Exact SQL UTF8 byte lengths are1322,453,207,2479,2499,662,2417,1271,566,1390,
981,729,2548,1862. This digest belongs to the revised MANY-basis/three-snapshot
proposal; the earlier67-object/F61448 draft is historical and NOT this candidate.
