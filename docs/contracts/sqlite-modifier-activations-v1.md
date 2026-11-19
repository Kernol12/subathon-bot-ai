# SQLite modifier definitions and activations, agreement draft 1

[MODIFIERTEST-007 #719](https://github.com/Kernol12/subathon-bot-ai/issues/719),
Tier A, [draft PR #725](https://github.com/Kernol12/subathon-bot-ai/pull/725).
Specification v0.45 §7, MOD-03 and MOD-04. The catalog follows the
[v13 milestone decisions](sqlite-milestone-group-decisions-v1.md) from #236.
Sections below describe the recommended choices for agreement before test authoring.

## Boundary and ownership

[#214](https://github.com/Kernol12/subathon-bot-ai/issues/214) owns the frozen
[pure activation contract](../../core/include/subathon/core/modifier_activation.hpp)
and its assertions. This baseline covers storage, selected reads, transaction
outcomes and compatibility. [#215](https://github.com/Kernol12/subathon-bot-ai/issues/215)
implements the repository and activation behavior against both reviewed baselines.
[#223](https://github.com/Kernol12/subathon-bot-ai/issues/223) owns first-party
arrival capture/selection, pending contribution integration and historical
application. Existing approval, authorization, preview and clock authorities
remain with the caller. MOD-04's explicit previewed active-instance edits remain
required in the product. This baseline exercises ordinary definition edits and
retained history. It supplies no active-edit command or recovery-clock policy.

## Typed writer boundary

Proposed header `subathon/server/sqlite_modifier_activations.hpp`. A standalone
`modifier_activations_repository_manifest() -> MigrationManifest` describes the
catalog. A `SqliteModifierActivationStore(SqliteWorkSession)` is an inert facade
over the active, revocable [writer work session](writer-transactions-v1.md).
Operations are synchronous and return owned values. Names below describe the
proposed typed surface. An absent expected head is an explicit expectation of absence.

| Operation | Inputs and result |
| --- | --- |
| `append_definition` | Core `ModifierDefinition`, optional expected definition revision ref. Append after the exact head or create position 1. Returns the appended definition revision. |
| `activate` | Definition ref, expected definition revision ref, optional expected live instance/revision key, and core `ModifierActivation`. Resolve the exact current definition and current live revision in the session. Return core `ModifierActivationProposal` after staging its durable transition. |
| `checkpoint_remaining` | Instance/ref revision key for the exact live head, fresh instance revision ref, authoritative observed remaining milliseconds. Append one checkpoint and return the exact revision with its lifetime metadata. Zero ends the instance. |
| `current_definition`, `definition_revision` | Definition ref for current, or definition/ref revision key for exact lookup. Return an optional owned definition revision with append position. |
| `live_instance`, `instance_revision` | Definition ref for the sole live head, or instance/ref revision key for exact lookup. Return optional owned core instance plus append position, operation kind, optional applied definition revision and instance lifetime metadata. |

The caller supplies explicit refs, owns the work item, and begins, commits or
rolls it back. The repository never opens a connection, commits, retries, reads
wall time or publishes. Admission alone proves queueing. Reads and writes inside
the session are provisional. Normal success requires known COMMIT and normal
owner completion with the owned value. Tests await completion, drain and close,
then open a fresh writer on the same file and use a fresh session and independent
read-only observations. Reopen neither migrates the file nor advances clocks.
Unknown outcomes, stale/revoked capabilities and post-commit exceptions retain
the existing writer classifications.

Proposed repository refusal categories are `invalid_input`, `stale_definition`,
`stale_instance`, `identity_conflict`, `invalid_stored_data` and the frozen core
activation errors. Preserve native writer failures, including SQL failure for
missing schema. Check the expected definition before the expected live head.
A missing optional lookup key on a valid store gives absence. A missing capture
on a present instance is a failure.
Validate owned input and selected output under finite bounds before publishing:
refs 1–4096 bytes, factor spellings within the existing exact-decimal bound
(512 bytes), at most 1024 targets, contiguous target ordinals and matching
declared count, plus cumulative writer budgets. A valid large selection can
require explicitly configured finite writer budgets. Reject oversized or
invalid exact values without partial results. Selected reads resolve
the requested revision and its capture, with no global history materialization.
SQL statements address `main` explicitly. Modifier work requires the admitted v14
catalog and actual required schema, including when a lookup would be empty.
Check selected identities, enums, actual types and required relations.
Reject corruption without repair.

Checked elapsed input and policy remain owned by the caller. For Extend, resolve
the live head after any caller-staged checkpoint, use the incoming duration and
policy, and validate the signed-64 sum before appending. Overflow is typed
`duration_overflow`, preserving the frozen core error. The owning work lets
this failure escape or explicitly rolls back before reporting it. For the
escaping path completion is `rolled_back/work_exception` with no success value.
If needed the owner carries the typed cause in separate failure evidence. A
caught domain refusal cannot be used to commit an originating unit. A statement
failure retains the writer's sticky error and rollback behavior.

## Identity, lifetime and retained values

- Definition ref is the logical key. Definition revision refs are globally
  unique within their revision namespace. Each revision is immutable, starts at
  position 1, and appends at current+1 with the checked expected head. Position
  is a positive signed-64 internal ordinal; exhaustion refuses. Current means
  greatest position for the exact definition. An edit alone leaves the live
  revision and its captured settings unchanged.
- Instance ref is globally unique and bound permanently to one definition ref.
  Its revision refs are globally unique in a separate namespace. Revisions are
  immutable, with the same position rule. A first revision has NULL parent;
  each later one names that instance's immediately preceding revision. Opaque
  refs compare by raw byte equality, including embedded NUL and case. They
  carry no order or authority. Reuse within a namespace is a conflict, even if
  the payload matches. Duplicate application/receipt handling stays upstream.
- An instance starts live on committed first activation. Extend and Restart append
  on that instance, retaining the captured definition but recording the incoming
  definition revision used for policy/duration, plus that operation's source and
  trigger. Ignore changes no rows; its trigger view follows #214. Replace
  atomically closes the old lifetime and starts a fresh instance/revision
  at position 1 with NULL parent and the new definition capture. The old
  instance records the successor instance ref, retaining all old revisions.
  The live index keys on definition ref, so revisions of one definition cannot
  create parallel live lanes.
- Remaining duration is exact signed-64 milliseconds. Definition duration must
  be positive in this draft. Live remaining is positive. An authoritative
  checkpoint may only reduce remaining or leave it equal. It retains capture
  and source/trigger; its applied-definition revision is absent. A checkpoint
  at zero stores a terminal revision and expires the lifetime in that work.
  Historical reads expose each revision's saved remaining and the instance's
  current lifetime disposition separately. A terminal instance cannot be
  reactivated or reused; a later activation uses a fresh instance for the same
  definition. Pure trigger selections use the in-transaction pre/post views
  specified in #214.
- A captured definition is reconstructed from the exact immutable revision
  and ordered targets, including kind, target ref, optional qualifier, exact
  decimal spelling and reactivation choice. Keep source kind/ref and either an
  absent trigger or the exact event ref and Include/Subsequent timing. Validate
  factor syntax and nonnegativity under the frozen exact-decimal contract.
  A missing qualifier or trigger stays absent; neither is backfilled from a
  current definition or plan.
- [Receipts](sqlite-receipts-v1.md), [application plans](sqlite-application-plans-v1.md)
  and [application bundles/audit](sqlite-application-bundle-v1.md) keep their
  existing schema, keys and selected reads. The plan keeps its own ordered
  modifier rows, original arrival context, factor bytes and exact request. The
  application keeps its operation/receipt/CID/plan keys, actual components and
  audit. Resolve the modifier history by the retained revision refs, never the
  current head. No new FK or rewrite on pre-v14 plans, synthetic seed data or
  unrelated historical modifier refs. An old unresolvable ref supplies no new
  history or computed credit. The original reversal basis uses the recorded
  actual component with its immutable selected plan and source provenance.

## Proposed additive catalog

Append only v14 `modifier-activations-v1` to
`milestone_group_decisions_repository_manifest()`, and append 14 to the
supported starting versions (0 through 14). Preserve all delivered v1–v13
ordered SQL, schema projections, IDs and digests byte for byte. No backfill.
Fresh and upgraded stores start with empty modifier tables. Existing startup
catalog selection remains where it is. In the v14 lane, recognize its exact
marker in the existing milestone/dispatch gate as decisions-capable and
preserve v13 schema checks, ordering, holds and independent-job handling.
Unknown/mismatched markers still fail closed. This is a compatibility
requirement for #215; the baseline makes no change to dispatched behavior.

The recommended schema below gives each exact migration statement on a single
line, with no trailing semicolon. All tables and the index are in `main`. All
references are BLOBs. Byte length and stored type of selected fields are also
validated by the repository. A revision position and target ordinal are stored
as INTEGERs; neither is used as a domain ref. The writer checks contiguous
append/parent membership, same-definition capture/applied revision and
same-definition successor, and that checkpoint/terminal state agrees with the
selected head. The deferred successor FK permits closing the old instance
before inserting the new live instance. No intermediate lifetime is published.

```sql
CREATE TABLE subathon_modifier_definition_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), duration_ms INTEGER NOT NULL CHECK(typeof(duration_ms)='integer' AND duration_ms>0), reactivation INTEGER NOT NULL CHECK(typeof(reactivation)='integer' AND reactivation BETWEEN 0 AND 3), target_count INTEGER NOT NULL CHECK(typeof(target_count)='integer' AND target_count BETWEEN 0 AND 1024), PRIMARY KEY(revision_ref), UNIQUE(definition_ref,position))
CREATE TABLE subathon_modifier_targets(definition_revision_ref BLOB NOT NULL, ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 1024), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind BETWEEN 0 AND 2), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob' AND length(target_ref) BETWEEN 1 AND 4096), qualifier_ref BLOB CHECK(qualifier_ref IS NULL OR (typeof(qualifier_ref)='blob' AND length(qualifier_ref) BETWEEN 1 AND 4096)), factor_decimal BLOB NOT NULL CHECK(typeof(factor_decimal)='blob' AND length(factor_decimal) BETWEEN 1 AND 512), PRIMARY KEY(definition_revision_ref,ordinal), FOREIGN KEY(definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref))
CREATE TABLE subathon_modifier_instances(instance_ref BLOB NOT NULL CHECK(typeof(instance_ref)='blob' AND length(instance_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), lifetime INTEGER NOT NULL CHECK(typeof(lifetime)='integer' AND lifetime BETWEEN 0 AND 2), successor_instance_ref BLOB, CHECK((lifetime=1 AND typeof(successor_instance_ref)='blob' AND length(successor_instance_ref) BETWEEN 1 AND 4096 AND successor_instance_ref<>instance_ref) OR (lifetime IN (0,2) AND successor_instance_ref IS NULL)), PRIMARY KEY(instance_ref), FOREIGN KEY(successor_instance_ref) REFERENCES subathon_modifier_instances(instance_ref) DEFERRABLE INITIALLY DEFERRED)
CREATE TABLE subathon_modifier_instance_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), instance_ref BLOB NOT NULL, position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), parent_revision_ref BLOB, captured_definition_revision_ref BLOB NOT NULL, applied_definition_revision_ref BLOB, remaining_ms INTEGER NOT NULL CHECK(typeof(remaining_ms)='integer' AND remaining_ms>=0), operation INTEGER NOT NULL CHECK(typeof(operation)='integer' AND operation BETWEEN 0 AND 1), source_kind INTEGER NOT NULL CHECK(typeof(source_kind)='integer' AND source_kind BETWEEN 0 AND 4), source_ref BLOB NOT NULL CHECK(typeof(source_ref)='blob' AND length(source_ref) BETWEEN 1 AND 4096), trigger_ref BLOB, trigger_timing INTEGER, CHECK((position=1 AND parent_revision_ref IS NULL AND operation=0) OR (position>1 AND typeof(parent_revision_ref)='blob')), CHECK((operation=0 AND typeof(applied_definition_revision_ref)='blob' AND remaining_ms>0) OR (operation=1 AND applied_definition_revision_ref IS NULL)), CHECK((trigger_ref IS NULL AND trigger_timing IS NULL) OR (typeof(trigger_ref)='blob' AND length(trigger_ref) BETWEEN 1 AND 4096 AND typeof(trigger_timing)='integer' AND trigger_timing IN (1,2))), PRIMARY KEY(revision_ref), UNIQUE(instance_ref,position), FOREIGN KEY(instance_ref) REFERENCES subathon_modifier_instances(instance_ref), FOREIGN KEY(parent_revision_ref) REFERENCES subathon_modifier_instance_revisions(revision_ref), FOREIGN KEY(captured_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref), FOREIGN KEY(applied_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref))
CREATE UNIQUE INDEX subathon_modifier_one_live ON subathon_modifier_instances(definition_ref) WHERE lifetime=0
```

Codes in listed order: reactivation Extend/Restart/Replace/Ignore; target
AddedTime/MilestoneScore/Counter; lifetime Live/Replaced/Expired; operation
Activation/Checkpoint; source Moderator/ChatCommand/Milestone/Rule/Schedule.
Each list starts at 0. Trigger Include=1, Subsequent=2, absence is SQL NULL
for both fields. The v14 projection adds 11 rows: four tables, the explicitly
named index with the SQL above, and six implicit indexes with NULL SQL.
For each table the primary-key index is `sqlite_autoindex_<table>_1`; definition
and instance revisions additionally have `_2` for the respective UNIQUE.
Project and sort exact tuples under the [catalog contract](migration-catalog-v1.md).

## Independent examples

Use literal synthetic rows, including literal exact values and spells. Never
derive expectations from the activation reducer, the catalog under test, or
selection against the current definition. The names below are opaque bytes.

1. **Definition edit, Replace and reopen.** Definition `D/z-old` has duration
   480000, Extend, ordered targets (AddedTime, `timer.main`, `payment`, `2.00`)
   and (Counter, `counter.synthetic`, NULL, `1.250`). Activate it as `I/j0`
   from Moderator `operator-A` with absent trigger. Position 1, remaining
   480000. Checkpoint at 180000 as `I/j1`, position 2, parent `j0`.
   Append `D/a-new` at position 2 with duration 600000, Replace, and sole
   target (AddedTime, `timer.main`, `payment`, `0.50`). Reopen with definition
   head `a-new` and sole live instance `I/j1`, still pinned to `z-old`.
   Replace with `J/k0`, Rule `rule-B`, event `event-B`, Subsequent. Trigger
   view is `I/j1`; new live is `J/k0`, position 1, NULL parent and remaining
   600000. After acknowledged commit and reopen, `I` is Replaced by `J`,
   retains both revisions with remaining 480000/180000, and `J` is the sole
   live row for `D`. An activation still expecting `I/j1` refuses as stale
   before staging any new revision. Exact read of `j1` resolves the old
   ordered targets, original source and absent trigger. The same ref with the wrong owner gives
   no match. An independent constraint control attempts a second live row
   for `D` with a fresh instance ref and must fail; another definition may
   have its own live instance. Expire `J` in a separate control with checkpoint
   `k1` (parent `k0`, remaining 0); reopen and read no live row for `D`, with
   both terminal histories still readable.
2. **Immutable original request and credit.** Before the edit, record synthetic
   receipt 7/CID 0, `event-A`, `arrival-A`, `plan-A` revision 1 and operation
   (`synthetic`, `apply-A`), using the existing stores. Request ordinal 1
   records `rule-A`, `action-A/v1`, `timer.main`, modifier selection
   (`z-old`, `j1`, `timer.main`, `g-A`, multiply, multiply, `2.00`).
   Independently, 60000 × 2 = 120000 ms exactly, residual 0. With Main 3540000,
   cap 3600000 and debt 0, credit 60000, debt delta 0, cap-limited 60000;
   after Main 3600000. Store the supplied audit
   (`operator-A`, `decision-A`, `synthetic reason`, `audit-v1`, bytes `original-A`).
   Compare the exact receipt/arrival, plan, operation, effect and audit via
   their existing selected paths and literal row observations after Replace,
   after each failed work, and after reopen. The original Main reversal basis
   credits 60000 ms, with progress position 0/cumulative 0/fraction 0/full
   false/no last occurrence. A subsequent hypothetical 60000 × 0.50 would
   request 30000, and supplies neither a rewrite nor a new calculation for
   that original effect. No reversal or intake operation is exercised.
3. **Overflow rolls back the originating work.** Start with committed
   `D/a-new`, `J/k0` live at 600000 and the original bundle from example 2.
   In one work stage definition `D/overflow`, Extend with duration
   9223372036854175808, and an unrelated new synthetic plan using the existing
   plan store with an already committed synthetic receipt. Read both staged
   rows in that session before calling `activate`. The sum is
   9223372036854775808, exactly signed-64 maximum + 1. Use fresh proposed
   revision `k-overflow`. Expect typed overflow and known rollback with no
   success payload. After reopen, `overflow`, the staged plan and
   `k-overflow` are absent; the definition head, live head, original receipt,
   plan, effect and audit are unchanged. A separate boundary work using
   duration 9223372036854175807 gives exactly 9223372036854775807 on
   `J`, parent `k0`, capture `a-new`, applied definition the incoming revision.
4. **Later SQL failure rolls back a staged Replace.** Use a separate disposable
   copy of the committed state from example 3's start. Stage a new Replace
   definition and replacement `K/m0`, then observe `K/m0` and `J`'s staged
   supersession in-session. Cause an actual constraint failure by inserting
   an already-present instance primary key. Expect the writer's SQL failure
   and known rollback, no success payload. Demonstrate a subsequent normally
   completed read on the same writer, then reopen. Both reads see `J/k0` sole
   live, `K` and the staged definition absent, the earlier `I` → `J` link and
   all original effect/audit facts intact.
5. **Catalog and prefix controls.** Compare all 13 candidate prefix entries
   to the pinned shipped prefix, with independent v12/v13 SQL and projections
   from the frozen milestone fixtures. Verify literal canonical history digests
   and exact projected objects using inputs independent of the v14 manifest. Inspect populated v13 as upgrade
   required (version 13/target 14/applied 0); advance applies one step.
   Fresh advances 14; current reinspection and re-advance apply 0. Preserve
   v13 history rows and existing receipt/plan/bundle/effect, release/hold
   and delivery history on upgrade. Exercise a held group and independent
   job in a disposable v14 store with existing synthetic delivery
   infrastructure. Cancel at `before_history(14,5)`, after the new DDL:
   known rollback, no modifier objects, unchanged v13 history and rows.
   Tamper v13's digest separately and require catalog mismatch. Independently
   verify an unrecognized head fails dispatch and missing required v14 schema
   fails a selected modifier read. Opening either writer alone performs no
   migration.

## Decisions for agreement

1. **Identity and layout.** **A (recommended)** uses global raw revision refs
   per namespace and normalized immutable capture, as drafted. Existing
   plan selection carries bare revision refs. **B** scopes revision refs by
   parent identity and needs an additional durable globally unique alias
   mapping for plan resolution. Under B revise keys, FKs, selected reads and
   migration examples before freezing v14.
2. **Lifetime and clocks.** **A (recommended)** stores caller-supplied remaining
   checkpoints and terminal expiry without sampling time. **B** reserves v14
   for activation/supersession only. Under B omit checkpoint/expiry and the
   operation column, seed the prior live remaining as a literal fixture, and
   agree a later clock checkpoint boundary. A restart policy that recomputes
   elapsed time requires its own reviewed facts and composition.
3. **Zero configured duration.** **A (recommended)** rejects it at the
   persistence boundary, with zero reserved for a terminal checkpoint under
   2A. **B** allows an immediately expired activation. Under B relax the
   duration/activation checks, specify Include-trigger selection for that
   revision and add a literal example before freezing. Neither choice
   revises the frozen pure assertions.
4. **Domain refusal ownership.** **A (recommended)** uses a typed repository
   refusal and caller-owned rollback, as drafted. **B** makes a caught domain
   refusal sticky in the session itself. Under B agree a separate additive
   writer capability and its error matrix before changing this contract.
