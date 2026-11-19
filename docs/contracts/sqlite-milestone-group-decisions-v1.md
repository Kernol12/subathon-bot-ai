# Milestone group decisions, draft revision 1

Task ID: MILESTONE-005
Work type: implementation
Issue: https://github.com/Kernol12/subathon-bot-ai/issues/236

Specification v0.45 §5, §8 and MILE-03. The owner ruled on the start boundary,
target, independent holds, presentation and catalog order in [PR #721](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7002464326).
This draft extends the [ordinary group contract](sqlite-milestone-groups-v1.md).
The C++ boundary is `milestone_group_decisions.hpp`.
[#721 numbered correction 1](https://github.com/Kernol12/subathon-bot-ai/pull/721)
and the [owner's 17A/18A/19A decision](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7003511184)
authorize the timeline boundary and policy below.

## Writer and decisions

- The owner revalidates permissions and the paused preview. Planning requires
  exactly one explicit choice for each distinct affected occurrence. Missing,
  duplicate or extra choices give no plan. Normal keeps ordinary alert intents
  in order. Silent drops ordinary message, sound and visual intents and retains
  the milestone choices. Accounting, application deduplication and preview
  revalidation belong to the caller.
- Record the complete acceptance choice set after staging the originating
  bundle, delivery captures and group batch in the same writer session. Every
  selected occurrence must belong to that application operation. A refusal
  invalidates the originating unit. Retain the mode, original choice, actor and
  decision. Run creates no hold, Defer creates an explicit hold, and Skip marks
  the occurrence skipped while retaining completion and captures.
- Apply refund policy in the work that applies the refund, using exact signed
  before/after scores and the current authoritative target supplied by the
  caller. A downward crossing (before ≥ current target and after < current
  target) qualifies while the exact group is pending and unreleased. Equality after the
  refund leaves the group eligible. Further decreases starting below the
  current target create no policy audit or hold. A fresh crossing can qualify
  again if the score recovered and the group is still pending. The exact
  unsigned deficit is current target less score after, using the full unsigned
  64-bit range. The captured v12 target stays in the immutable occurrence.
  A committed release starts
  the group, including a release with all actions still queued. Refunds after
  release leave its pending jobs, bonus eligibility and performed history
  intact. Only an applied pre-start policy is entered in the refund policy audit.
- Continue records the qualifying refund and leaves existing holds in force.
  Review is the default and creates a separate hold for that exact occurrence
  and refund ref even when deferral is active. Skip records the refund and
  omits that occurrence's pending presentation and actions.
- A resolution selects one exact occurrence, cause and source ref. It succeeds
  once for an active record and audits the selected resolution, actor and
  decision. No matching active record means false and no change. Continue clears
  only the selected cause. Hold enumeration for an occurrence returns deferral
  first and then refund reviews in bytewise refund-ref order. No resolved
  cause is reported as active. Committed Skip is terminal for the occurrence.
  Record the selected resolution or configured policy and end every other
  still-active hold on the same occurrence in that work. Preserve the other
  holds' original decisions and record which Skip ended them, separately
  from a selected resolution. Earlier resolved holds retain their resolutions.
  Retries of an identical refund policy observation leave its prior state
  intact, including its resolution. A conflicting reuse of the same
  occurrence/ref fails the refund work.
- SQL writes and reads are provisional within the caller's session. Known
  rollback discards both the originating changes and their decision records.
  Publish only on normally acknowledged commit. For ambiguous completion the
  owner reads actual committed facts before deciding whether to retry.
- References are owned opaque bytes of length 1–4096. Planning bounds each
  affected, choice and ordinary-intent list to 4096 entries. Operations on
  persisted rows respect existing writer input/result limits and report errors
  on overflow. No partial choice set, hold set or queue snapshot is published.

## Order and effects

Ready, Held and Skipped are exact occurrence dispositions. Unknown and released
occurrences return AbsentOrReleased. Ready still requires head order and
current authority before starting. Cold recovery returns unreleased, non-skipped
captures in durable order, including held heads. Release rechecks the same
head selection and the exact active holds in its writer work. Held heads
block the suffix. Skip has no release, presentation window or gap of its own.
Previously started presentations and their gaps finish on the #234 timeline.
Skipped captures and their queued delivery histories stay readable. Existing
job/bonus authority and committed-release requirements apply. The v13 dispatch
path must recognize the exact catalog identity and preserve v12 occurrence
membership, including the independent-job path. A recognized v13 store with
missing required schema fails the affected work.

On an eligible owner turn the warm timeline consults the exact port for a
keyed head. Ready starts that head. Held or unavailable stops the walk.
AbsentOrReleased, stale or missing decision facts supply no disposition.
Skipped discards that exact head and continues on the same turn, subject to
the next head's decision. The legacy completion lookup applies to keyless
heads. Queue pauses, active presentations and running gaps keep their #234
boundaries. #294 owns live composition, including writer acknowledgements and
timeline publication.

## Additive catalog

Append version 13, `milestone-group-decisions-v1`, immediately after
`milestone-groups-v1`. Append 13 to the supported starting versions.
Preserve versions 1–12 byte for byte, including ordered SQL, projected schema,
and history digests. Add the following two tables in order, in `main`. Each
has only the implicit primary-key index. Both are initially empty on a fresh
install and on upgrade. v12 release state and captured rows retain their
meaning without synthesized decisions. #719 builds on version 13.

The following SQL strings are single lines without trailing semicolons. Enum
values are normal=0/silent=1, Run=0/Defer=1/Skip=2, policy
Continue=0/Review=1/Skip=2 and resolution Continue=0/Skip=1. A NULL resolution
has NULL actor and decision refs. A non-NULL resolution has both refs and
belongs to a Defer or Review row. A skipped occurrence retains `released=0`.
An indirectly ended hold has a NULL resolution triple and a non-NULL
`ended_by_skip_kind` and `ended_by_skip_ref`. The kind identifies a deferral
resolution (0), refund Review resolution (1), or configured refund policy (2).
Together with the occurrence, the ref selects the acceptance decision ref for
kind 0 or the refund ref for kinds 1 and 2. The writer verifies that the source
is the actual terminal Skip and updates the affected rows atomically. Only
Defer and Review rows can carry this provenance.
Policy audit stores exact operands. Derive the unsigned deficit from those
operands without SQLite numeric coercion.

```sql
CREATE TABLE subathon_milestone_acceptance_decisions(occurrence_ref BLOB NOT NULL, mode INTEGER NOT NULL CHECK(typeof(mode)='integer' AND mode IN (0,1)), choice INTEGER NOT NULL CHECK(typeof(choice)='integer' AND choice IN (0,1,2)), actor_ref BLOB NOT NULL CHECK(typeof(actor_ref)='blob' AND length(actor_ref) BETWEEN 1 AND 4096), decision_ref BLOB NOT NULL CHECK(typeof(decision_ref)='blob' AND length(decision_ref) BETWEEN 1 AND 4096), resolution INTEGER, resolution_actor_ref BLOB, resolution_ref BLOB, ended_by_skip_kind INTEGER, ended_by_skip_ref BLOB, CHECK((resolution IS NULL AND resolution_actor_ref IS NULL AND resolution_ref IS NULL) OR (choice=1 AND typeof(resolution)='integer' AND resolution IN (0,1) AND typeof(resolution_actor_ref)='blob' AND length(resolution_actor_ref) BETWEEN 1 AND 4096 AND typeof(resolution_ref)='blob' AND length(resolution_ref) BETWEEN 1 AND 4096)), CHECK((ended_by_skip_kind IS NULL AND ended_by_skip_ref IS NULL) OR (choice=1 AND resolution IS NULL AND typeof(ended_by_skip_kind)='integer' AND ended_by_skip_kind IN (0,1,2) AND typeof(ended_by_skip_ref)='blob' AND length(ended_by_skip_ref) BETWEEN 1 AND 4096)), PRIMARY KEY(occurrence_ref), FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref))
CREATE TABLE subathon_milestone_refund_decisions(occurrence_ref BLOB NOT NULL, refund_ref BLOB NOT NULL CHECK(typeof(refund_ref)='blob' AND length(refund_ref) BETWEEN 1 AND 4096), score_before_micro_points INTEGER NOT NULL CHECK(typeof(score_before_micro_points)='integer'), score_after_micro_points INTEGER NOT NULL CHECK(typeof(score_after_micro_points)='integer'), current_target_micro_points INTEGER NOT NULL CHECK(typeof(current_target_micro_points)='integer'), policy INTEGER NOT NULL CHECK(typeof(policy)='integer' AND policy IN (0,1,2)), resolution INTEGER, resolution_actor_ref BLOB, resolution_ref BLOB, ended_by_skip_kind INTEGER, ended_by_skip_ref BLOB, CHECK((resolution IS NULL AND resolution_actor_ref IS NULL AND resolution_ref IS NULL) OR (policy=1 AND typeof(resolution)='integer' AND resolution IN (0,1) AND typeof(resolution_actor_ref)='blob' AND length(resolution_actor_ref) BETWEEN 1 AND 4096 AND typeof(resolution_ref)='blob' AND length(resolution_ref) BETWEEN 1 AND 4096)), CHECK((ended_by_skip_kind IS NULL AND ended_by_skip_ref IS NULL) OR (policy=1 AND resolution IS NULL AND typeof(ended_by_skip_kind)='integer' AND ended_by_skip_kind IN (0,1,2) AND typeof(ended_by_skip_ref)='blob' AND length(ended_by_skip_ref) BETWEEN 1 AND 4096)), PRIMARY KEY(occurrence_ref,refund_ref), FOREIGN KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref))
```
