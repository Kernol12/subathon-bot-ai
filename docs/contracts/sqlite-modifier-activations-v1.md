# SQLite modifier definitions and activations, revision 1 draft

Task ID: MODIFIERTEST-007
Work type: test_baseline
Issue: https://github.com/Kernol12/subathon-bot-ai/issues/719

Tier A, [draft PR #725](https://github.com/Kernol12/subathon-bot-ai/pull/725).
Specification v0.45 §7, MOD-03 and MOD-04. The catalog follows the
[v13 milestone decisions](sqlite-milestone-group-decisions-v1.md) from #236.
The [owner's lifetime and duration rulings](https://github.com/Kernol12/subathon-bot-ai/pull/725#issuecomment-7008906515)
are included. Independent contract review precedes test authoring.

## Boundary and ownership

[#214](https://github.com/Kernol12/subathon-bot-ai/issues/214) owns the frozen
[pure activation contract](../../core/include/subathon/core/modifier_activation.hpp)
and assertions. [#215](https://github.com/Kernol12/subathon-bot-ai/issues/215)
implements both reviewed baselines.
[#223](https://github.com/Kernol12/subathon-bot-ai/issues/223) owns first-party
arrival capture/selection, pending contribution integration and historical
application. Existing approval, authorization, preview and clock authorities
remain with the caller. MOD-04 previewed active-instance edits need their own
reviewed operation representation.

## Typed writer boundary

Proposed header `subathon/server/sqlite_modifier_activations.hpp` exposes the
manifest and `SqliteModifierActivationStore(SqliteWorkSession)`. The inert
facade returns synchronous owned values through the active, revocable
[writer session](writer-transactions-v1.md). An absent expected head requires absence.

| Operation | Inputs and result |
| --- | --- |
| `append_definition` | Core `ModifierDefinition`, optional expected definition revision ref. Append after the exact head or create position 1. Returns the appended definition revision. |
| `activate` | Definition ref, expected definition revision ref, optional expected live instance/revision key, and core `ModifierActivation`. Resolve the exact current definition and current live revision in the session. Return core `ModifierActivationProposal` after staging its durable transition. |
| `checkpoint_remaining` | Instance/ref revision key for the exact live head, fresh instance revision ref, authoritative observed remaining milliseconds. Append one checkpoint and return the exact revision with its lifetime metadata. Zero ends the instance. |
| `current_definition(definition_ref)`, `definition_revision(revision_ref)` | Current or exact lookup. Return an optional owned definition revision including its owning definition ref and append position. |
| `live_instance(definition_ref)`, `instance_revision(revision_ref)` | Sole live head or exact lookup. Return an optional owned core instance including its owning instance ref, append position, operation kind, optional applied definition revision and lifetime metadata. |

The caller supplies refs and owns transaction control, clocks and publication.
Admission proves queueing. Success requires known COMMIT and normal work
completion. Tests drain and close, then reopen the same file for fresh session
reads and independent observations.

Every lookup checks its bare ref before SQL. Empty or over 4096 bytes gives
`invalid_input`. A well-formed unknown ref gives absence on admitted v14 with
required schema. Exact reads check the requested revision and its owner.
Current/live reads check the requested definition against the selected owner.
Check SQL types, enums, head, parent, capture, applied revision and lifetime
within the selected closure. Identity mismatch or a missing relation on a
present row gives `invalid_stored_data`. SQL addresses `main`; missing required
schema fails through the writer even on an empty lookup. Input and selected
values use 1–4096-byte refs, up to 1024 targets and up to 512 bytes per factor.
Selected target ordinals and count must agree. Cumulative writer budgets may
require finite configuration for large reads. Failures expose no partial data.

### Refusals and completion

Healthy refusals leave the active session and preceding caller staging usable.
Unit refusals call the existing `session.cleanup()` and rethrow the typed cause
after known rollback. The table gives the point relative to the operation's
own writes. Shape checks precede SQL, expected definition precedes expected
live instance, and comparisons see earlier staging in the same session.

| Operation | Refusal and point | Session |
| --- | --- | --- |
| All three mutations | `invalid_input` for malformed refs/enums at shape check. | Healthy |
| `append_definition` | `invalid_input` for nonpositive duration or too many targets, and `invalid_factor` for invalid, negative or oversized factor, at preflight. `stale_definition` for a head contrary to expectation, including unexpected presence or absence. `identity_conflict` for a used definition revision ref. All precede own writes. | Healthy |
| `activate` | `missing_trigger_timing` for a supplied trigger with unspecified timing at preflight. `stale_definition` for absent/different current definition. `stale_instance` for an absent/different live key contrary to expectation, including a terminal expected instance. `identity_conflict` if an instance or revision ref to be inserted is already used. Each precedes own writes. | Healthy |
| `activate` | `duration_overflow` if Extend's checked live remaining (including any staged checkpoint) plus incoming duration exceeds signed-64 max. Detected before own writes. | Unit |
| `checkpoint_remaining` | `invalid_input` for negative remaining at preflight or remaining above the selected live head after freshness checks. `stale_instance` for missing, terminal or different live head. `identity_conflict` for reuse of the proposed instance revision ref. Each precedes own writes. | Healthy |
| Any append | `sequence_exhausted` when the matching head is at signed-64 max and this operation would append on that lineage. Detected before increment or own writes. | Unit |
| Mutations and reads | `invalid_stored_data` for corrupt selected closure or wrong catalog marker, including malformed stored factor/trigger data. | Unit |
| Mutations | Any local domain refusal after own provisional writes. | Unit |

Catching a healthy refusal permits a normal commit of prior staging
(`committed/none` with the caller's normal payload). An escaping refusal
completes `rolled_back/work_exception` after known rollback. Catching a healthy
refusal and returning with the session still active gives `rolled_back/uncommitted`.
Unit cleanup discards all staging and leaves the session terminal. The first
three rows below assume known rollback and no earlier framework error.

| After unit cleanup | Work outcome and error |
| --- | --- |
| Typed cause escapes the body | `rolled_back/work_exception` |
| Caller catches typed cause and returns normally without a commit attempt | `rolled_back/not_committed` |
| Caller catches typed cause and calls `commit()` in that body | No COMMIT SQL. It throws sticky `SqliteWorkFailure(session_closed)`. Completion is `rolled_back/session_closed` whether that error is caught or escapes. |
| Rollback cannot be established | `outcome_unknown/uncertain_outcome`, writer fail-closed |

Rollback outcomes carry no success value. Preserve writer SQL, budget,
capability, uncertain COMMIT and post-COMMIT classifications and precedence.
An already-aborted writer failure requires no second rollback.

## Identity, lifetime and retained values

- Definition ref is the logical key. Definition revision refs are globally
  unique within their namespace. Immutable history starts at position 1 and
  appends at current+1 after checking the expected head. Position is a
  positive signed-64 ordinal. Current means the greatest position for that
  definition. An edit leaves the live revision and its capture intact.
- Instance ref is globally unique and bound to one definition. Its immutable
  revisions follow the same position rule in a separate global ref namespace.
  The first has NULL parent. Each later one names its immediate predecessor.
  Refs compare bytewise, including NUL and case. Reuse conflicts even for
  identical payloads. Application/receipt deduplication stays upstream.
- An instance starts live on committed first activation. Extend and Restart append
  on that instance, retaining the captured definition but recording the incoming
  definition revision used for policy/duration, plus that operation's source and
  trigger. Ignore changes no rows; its trigger view follows #214. Replace
  atomically closes the old lifetime and starts a fresh instance/revision
  at position 1 with NULL parent and the new definition capture. The old
  instance records its successor and keeps its history. First activation and
  Replace both use the incoming revision as capture and applied definition.
  Proposed refs entering storage must be unused in their namespace. The live
  index keys on definition ref.
- Duration uses signed-64 milliseconds. Configured duration and live remaining
  are positive. Checkpoints keep or reduce remaining and retain capture and
  source/trigger with no applied definition. Zero saves a terminal revision
  and expires the instance in that work. Historical reads give saved remaining
  and current lifetime separately. Later activation uses a fresh instance.
  The caller owns time observations and downtime policy. Trigger selections
  use the in-transaction pre/post views from #214.
- Reconstruct capture from its immutable revision and ordered targets. Retain
  target kind/ref, optional qualifier, exact factor spelling, reactivation
  choice, source kind/ref and optional event ref with Include/Subsequent timing.
  Validate exact factor syntax and nonnegativity. Preserve qualifier and trigger
  absence.
- [Receipts](sqlite-receipts-v1.md), [application plans](sqlite-application-plans-v1.md)
  and [application bundles/audit](sqlite-application-bundle-v1.md) keep their
  existing schema, keys and selected reads. The plan keeps its own ordered
  modifier rows, original arrival context, factor bytes and exact request. The
  application keeps its operation/receipt/CID/plan keys, actual components and
  audit. Resolve history with `definition_revision(definition_version_ref)` and
  `instance_revision(instance_revision_ref)`. When reconciling the pair, the
  instance capture must match the selected definition revision. No alias, FK,
  backfill or rewrite on existing plans. Unresolvable refs supply no new
  history or credit. Reversal uses recorded actuals and immutable provenance.

## Proposed additive catalog

Only `modifier_activations_repository_manifest()` takes a copy of the shipped
`milestone_group_decisions_repository_manifest()`, adds step 14
`modifier-activations-v1`, and extends supported starts to 0 through 14.
The v13 function stays pinned to exactly 13 steps and starts 0 through 13.
Preserve v1–v13 ordered SQL, projections, IDs and digests byte for byte.
Fresh and upgraded v14 stores start with empty modifier tables. API host and
development startup still select the existing v10 reference authority catalog.

For this compatibility seam, #215 owns only exact recognition of
`(14, 'modifier-activations-v1')` as decisions-capable in the shared catalog
projection in `server/src/sqlite_milestone_detail.hpp`. Reuse its v13 schema
checks, ordering, holds and independent-job handling. Keep older lanes and
unknown/mismatched marker rejection as shipped.

These are the exact migration statements in order. All objects are in `main`.
Refs are BLOBs. Positions and ordinals are INTEGERs. In addition to SQL
constraints, check contiguous append/parent membership, same-definition
capture/applied revision and successor, and checkpoint/terminal consistency
with the selected head. The deferred FK permits closing the old instance
before inserting its successor in one unpublished transition.

```sql
CREATE TABLE subathon_modifier_definition_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), duration_ms INTEGER NOT NULL CHECK(typeof(duration_ms)='integer' AND duration_ms>0), reactivation INTEGER NOT NULL CHECK(typeof(reactivation)='integer' AND reactivation BETWEEN 0 AND 3), target_count INTEGER NOT NULL CHECK(typeof(target_count)='integer' AND target_count BETWEEN 0 AND 1024), PRIMARY KEY(revision_ref), UNIQUE(definition_ref,position))
CREATE TABLE subathon_modifier_targets(definition_revision_ref BLOB NOT NULL, ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 1024), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind BETWEEN 0 AND 2), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob' AND length(target_ref) BETWEEN 1 AND 4096), qualifier_ref BLOB CHECK(qualifier_ref IS NULL OR (typeof(qualifier_ref)='blob' AND length(qualifier_ref) BETWEEN 1 AND 4096)), factor_decimal BLOB NOT NULL CHECK(typeof(factor_decimal)='blob' AND length(factor_decimal) BETWEEN 1 AND 512), PRIMARY KEY(definition_revision_ref,ordinal), FOREIGN KEY(definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref))
CREATE TABLE subathon_modifier_instances(instance_ref BLOB NOT NULL CHECK(typeof(instance_ref)='blob' AND length(instance_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), lifetime INTEGER NOT NULL CHECK(typeof(lifetime)='integer' AND lifetime BETWEEN 0 AND 2), successor_instance_ref BLOB, CHECK((lifetime=1 AND typeof(successor_instance_ref)='blob' AND length(successor_instance_ref) BETWEEN 1 AND 4096 AND successor_instance_ref<>instance_ref) OR (lifetime IN (0,2) AND successor_instance_ref IS NULL)), PRIMARY KEY(instance_ref), FOREIGN KEY(successor_instance_ref) REFERENCES subathon_modifier_instances(instance_ref) DEFERRABLE INITIALLY DEFERRED)
CREATE TABLE subathon_modifier_instance_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), instance_ref BLOB NOT NULL, position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), parent_revision_ref BLOB, captured_definition_revision_ref BLOB NOT NULL, applied_definition_revision_ref BLOB, remaining_ms INTEGER NOT NULL CHECK(typeof(remaining_ms)='integer' AND remaining_ms>=0), operation INTEGER NOT NULL CHECK(typeof(operation)='integer' AND operation BETWEEN 0 AND 1), source_kind INTEGER NOT NULL CHECK(typeof(source_kind)='integer' AND source_kind BETWEEN 0 AND 4), source_ref BLOB NOT NULL CHECK(typeof(source_ref)='blob' AND length(source_ref) BETWEEN 1 AND 4096), trigger_ref BLOB, trigger_timing INTEGER, CHECK((position=1 AND parent_revision_ref IS NULL AND operation=0) OR (position>1 AND typeof(parent_revision_ref)='blob')), CHECK((operation=0 AND typeof(applied_definition_revision_ref)='blob' AND remaining_ms>0) OR (operation=1 AND applied_definition_revision_ref IS NULL)), CHECK((trigger_ref IS NULL AND trigger_timing IS NULL) OR (typeof(trigger_ref)='blob' AND length(trigger_ref) BETWEEN 1 AND 4096 AND typeof(trigger_timing)='integer' AND trigger_timing IN (1,2))), PRIMARY KEY(revision_ref), UNIQUE(instance_ref,position), FOREIGN KEY(instance_ref) REFERENCES subathon_modifier_instances(instance_ref), FOREIGN KEY(parent_revision_ref) REFERENCES subathon_modifier_instance_revisions(revision_ref), FOREIGN KEY(captured_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref), FOREIGN KEY(applied_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref))
CREATE UNIQUE INDEX subathon_modifier_one_live ON subathon_modifier_instances(definition_ref) WHERE lifetime=0
```

Codes start at 0 in each ordered list. Reactivation is Extend/Restart/Replace/Ignore.
Target is AddedTime/MilestoneScore/Counter. Lifetime is Live/Replaced/Expired.
Operation is Activation/Checkpoint. Source is Moderator/ChatCommand/Milestone/Rule/Schedule.
Trigger is Include=1, Subsequent=2, or both fields SQL NULL for absence.
The [catalog projection](migration-catalog-v1.md) adds 11 exact tuples, four tables,
one named index with SQL, and six implicit indexes with NULL SQL. Each PK has
`sqlite_autoindex_<table>_1`. The two revision tables also have `_2` for UNIQUE.

## Independent examples

Use literal synthetic inputs and independent expectations. Fork controls
from the stated committed state and use existing stores for unrelated records.

1. **Definition edit, Replace and reopen.** Create `D/z-old` at position 1,
   duration 480000, Extend, with ordered targets (AddedTime, `timer.main`,
   `payment`, `2.00`) and (Counter, `counter.synthetic`, NULL, `1.250`).
   With no expected live head, activate `I/j0` from Moderator `operator-A`
   without trigger. Position 1, NULL parent, 480000, capture and applied
   definition `z-old`. Checkpoint to 180000 as `I/j1` (position 2, parent
   `j0`, applied definition absent). Record example 2 here. Append `D/a-new`
   (expected `z-old`, position 2), duration 600000, Replace, with sole target
   (AddedTime, `timer.main`, `payment`, `0.50`). Reopen with head `a-new`
   and sole live `I/j1` still captured at `z-old`.
   Expect `a-new` and `I/j1` when activating replacement `J/k0` from Rule
   `rule-B`, event `event-B`, Subsequent. Trigger view is `I/j1`; new live
   `J/k0` is position 1, NULL parent, remaining 600000, capture/applied `a-new`.
   After commit and reopen, `I` is Replaced by `J` with 480000/180000 history.
   An activation proposing `L/l0` with the same source/timing and expected
   definition `a-new` but expected live `I/j1` gives `stale_instance`.
   Exact reads `definition_revision(z-old)` and `instance_revision(j1)` own
   `D` and `I` and return the old targets and, for `j1`, the original
   source and absent trigger. Bare refs `missing-def` and `missing-rev`
   yield absence; current/live reads for `D` yield `a-new` and `J/k0`.
   Raw second live row `L` for `D` fails the unique index. In a fresh fork
   create independent `E/e0` (Extend, 1 ms, no targets), then activate
   `L/l0` for `E` from Moderator `operator-A`, no trigger. In another fork
   checkpoint `J/k0` to `k1` (position 2, parent `k0`, remaining 0, capture
   `a-new`, applied definition absent). Reopen with no live row for `D`,
   `J` Expired and both terminal histories readable. Appending `D/d-zero`
   with expected `a-new`, Extend, no targets and duration 0 gives
   `invalid_input`. A caught control can commit a preceding independent
   append `E/e0` at position 1 and leave `D/a-new` unchanged.

2. **Immutable original request and credit.** With `I/j1` live before the
   edit, use committed receipt 7/CID 0, root event `event-A` with no parent,
   arrival (1000 UTC ms, `arrival-A` connection, `cfg-arrival-A`,
   `modifier-context-A`), and `plan-A` revision 1. Retain the arrival
   configuration/context and application configuration `cfg-apply-A`.
   Operation (`synthetic`, `apply-A`) uses that plan. Request ordinal 1
   records `rule-A`, action `action-A` version `v1`, `timer.main`, and
   modifier selection (`z-old`, `j1`, `timer.main`, `g-A`, multiply,
   multiply, `2.00`). With states `main-before-A` → `main-after-A`,
   independently, 60000 × 2 = 120000 ms, residual 0. From Main 3540000, cap
   3600000 and debt 0, credit 60000, debt delta 0, cap-limited 60000, Main
   after 3600000 with debt 0. Audit is (`operator-A`, `decision-A`, `synthetic reason`,
   `audit-v1`, bytes `original-A`). Compare the receipt/arrival, plan,
   operation, effect and audit using existing selected paths and literal
   rows after Replace, each failed work and reopen. The original reversal
   basis ((`synthetic`, `apply-A`), ordinal 1, Main delta) credits 60000 ms,
   progress position 0/cumulative 0/fraction 0/full false/no last occurrence.
   A later hypothetical 60000 × 0.50 requests 30000 and leaves the original
   effect as recorded. Read the reversal basis without applying a reversal.

3. **Overflow rolls back the originating work.** Fork from committed
   `D/a-new` (position 2), `J/k0` live at 600000 and example 2's original
   bundle. Also precommit unrelated receipt 8 (no CID, arrival 1001 UTC ms,
   connection `arrival-B`, `cfg-arrival-B`, `modifier-context-B`). In each
   overflow fork stage `D/overflow` at position 3, Extend, no targets,
   duration 9223372036854175808, and unrelated `plan-B` revision 1, root
   `event-B-plan` without parent using receipt 8's original. Expect
   `a-new` in the definition append. Read both
   staged revisions, then activate with expected `overflow` and `J/k0`,
   proposed `P/k-overflow`, Moderator `operator-C`, absent trigger. The
   sum is 9223372036854775808, signed-64 max + 1. Exercise the escaping case,
   the caught normal-return case and both terminal commit misuse completions
   in the table. After reopen, `overflow`, `plan-B` and `k-overflow` are
   absent. Receipt 8, both original heads and all example 2 facts survive.
   In the boundary fork append `D/boundary` after `a-new` (position 3, Extend, no targets)
   with duration 9223372036854175807. Activate with expected `boundary`
   and `J/k0`, proposed `P/k-max`, same source and no trigger. Commit
   `J/k-max` at position 2, parent `k0`, remaining 9223372036854775807,
   capture `a-new`, applied definition `boundary`; `P` is unused.

4. **Later SQL failure rolls back a staged Replace.** Fork from example 3's
   committed start. Stage `D/replace-fault` (expected `a-new`, position 3,
   duration 900000, Replace, no targets). Activate with expected
   `replace-fault` and `J/k0`, proposed `K/m0`, Schedule `schedule-C`
   and no trigger. Read `K/m0` (position 1, NULL parent, remaining 900000,
   capture and applied `replace-fault`) and `J` Replaced by `K` in-session.
   Use raw `session.execute` to INSERT into `main.subathon_modifier_instances`
   with the existing instance PK `I`, spare definition ref `fault-lane`,
   lifetime 0 and NULL successor (bind refs as BLOBs). Expect known rollback
   with sticky `sql_failure` and no success value. A normally completed read
   on the same writer and another after reopen see `J/k0` sole live for
   `D`, `K`, `m0` and `replace-fault` absent, the earlier `I` → `J`
   link and all example 2 facts intact.

5. **Catalog and prefix controls.** Compare all 13 ordered prefix entries
   with the shipped prefix, using v12/v13 SQL and projections from the frozen
   fixtures. Check literal canonical digests and exact projections
   against independent inputs. Populated v13 inspects as upgrade required
   (13/14/0), then advances one step. Fresh advances 14 steps; current
   reinspection and re-advance apply 0. Preserve v13 history, existing
   receipt/plan/bundle/effect, release/hold and delivery rows. Cancel at
   `before_history(14,5)` after DDL: known rollback, modifier objects
   absent, v13 history and rows intact. Separately tamper the v13 digest
   (catalog mismatch), use an unrecognized head (dispatch fails closed),
   and remove required v14 schema (selected modifier read fails through SQL).
   Opening the writer performs no migration.

   For compatibility, seed v13's unreleased `z-first` with active deferral
   `accept-31` and queued revision-1 jobs (`svc`, `add10`, `job`) with
   group membership and (`svc`, `add10`, `legacy`) without membership.
   Advance using independent literal v14 fixture data. The protected projection
   yields NULL for v14. The release and dispatch bodies throw corrupt
   `invalid_stored_data` before release, claim or authority. Static expected
   red for the coordinator to observe later is `rolled_back/work_exception`,
   absent result, no adapter/effect calls and no new release or event rows.
   After #215, release of the held head and dispatch of its job each complete
   `committed/none` with `refused` and no writes, authority or adapter calls.
   The independent job completes `committed/none` with `changed`, one authority
   call, one sending event at revision 2/attempt 2 and one post-commit fake start.
