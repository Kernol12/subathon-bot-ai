# Ordinary milestone group storage, revision 1

MILESTONE-009 (#709), specification v0.45 §8, MILE-01 and MILE-08.
The owner approved this boundary on [PR #716](https://github.com/Kernol12/subathon-bot-ai/pull/716#issuecomment-7000427007).
The C++ entry points are in `sqlite_milestone_groups.hpp` and `milestone_timeline.hpp`.

## Additive catalog

Advance explicitly from the frozen delivery catalog (v11) with step 12,
`milestone-groups-v1`. Leave every prefix statement, schema object and history
digest intact. A fresh store and an upgraded store gain the same three empty
tables. Generic v11 job group fields carry no milestone identity. Do not infer
membership, a release, or a receipt from them.

The following projection is in `main`. References are BLOB bytes, nonempty
and at most 4096 bytes. Required integers use SQLite INTEGER storage. The
first order and each first action position are 1.

| Table | Columns in order | Keys and constraints |
| --- | --- | --- |
| `subathon_milestone_groups` | `occurrence_ref`, `operation_scope`, `operation_ref`, `queue_order`, `milestone_ref`, `definition_revision_ref`, `target_micro_points`, `score_micro_points`, `completion_context_ref`, `presentation_ms`, `gap_ms`, `released` | PK `occurrence_ref`. Unique positive `queue_order`. Origin FK to `subathon_app_operations`. Nullable positive presentation and nullable nonnegative gap. `released` is 0 or 1. |
| `subathon_milestone_actions` | `occurrence_ref`, `action_ordinal`, `kind`, `action_ref`, `version_ref`, `operation_scope`, `operation_ref`, `job_ref` | PK `(occurrence_ref, action_ordinal)` with positive ordinal. Occurrence FK to group. Kind 0 automation or 1 bonus. Automation has all three delivery key columns, which form a unique tuple and FK to `subathon_delivery_captures`. Bonus has NULL for those three columns. |
| `subathon_milestone_bonus_receipts` | `occurrence_ref`, `action_ordinal`, `effect_ref` | PK and FK `(occurrence_ref, action_ordinal)` to a captured action. The writer checks exact bonus kind. `effect_ref` is the supplied durable identity of the actual effect. |

`subathon_milestone_pending_order` indexes `queue_order` for rows with
`released=0`. The writer checks semantic invariants that span rows (complete
contiguous action order, same originating operation for member jobs, and
matching captured action reference and version). Missing or contradictory
positively identified membership cannot authorize execution.
Invalid enrollment fails the originating writer work. With known rollback,
completion reports `rolled_back` with an existing writer error and no work
value. The application bundle, delivery captures and events, and the entire
group batch from that work are absent on reopen.

## Origin, release and recovery

An occurrence is an ordinary group instance, distinct from the editable
milestone definition. The originating application writer work stages the
application bundle and delivery captures, then enrolls the ordered group batch
on the same session before commit. Enrollment appends a durable order across
batches. A rollback leaves none of that unit visible. Group data is captured
at enrollment and retained after release, including bonus-only and empty
groups. Timing NULL represents inheritance from the timeline settings.
Ordinary recovery performs no edits to captured definitions or delivery history.

Release selects only the exact committed pending head. It uses separate writer
work from a delivery claim or bonus effect. Only a normal committed release
completion publishes eligibility. Known rollback keeps the head pending.
Ambiguous completion publishes no release or refusal. A cold owner re-reads the
actual committed pending suffix and restores it without starting a presentation
during recovery. The first eligible owner turn starts a full presentation and
uses the captured timing override. The existing timeline release port carries
the exact occurrence to the owner. The #294 host owns the asynchronous
composition of that port, including when to advance the running timeline.
Existing hold consultation still applies. #236 owns deferred groups and
persisted hold recovery.

## Effects and delivery

The existing #232 dispatch entry checks exact membership and committed group
release inside the queued-to-sending work, in addition to current authority.
A genuine v11 store retains the existing #232 dispatch behavior. Recorded
catalog identity distinguishes that case from a v12 store with
missing milestone tables, where a failed membership query remains a writer
SQL failure. Independent v11 jobs remain independent after upgrade. Release
and ordinary queue recovery claim no job. The exclusive #232 stranded-attempt
sweep remains separate. A sending attempt can become outcome-unknown through
that sweep without starting an adapter. Completed or uncertain delivery is
never replayed through the ordinary queue.

A bonus is selected by exact occurrence and captured action position. The
writer checks group release and current authority before invoking the trusted
effect port in the same caller-owned work. Record the returned stable effect
reference in that work. An existing receipt stops reapplication. A fault that
rolls back the work leaves both effect and receipt absent. Loss of acknowledgement
requires reading actual committed state after reopen. Recovery alone calls no
effect port. Actual bonus calculations and running host integration are owned
by their respective product boundaries.
