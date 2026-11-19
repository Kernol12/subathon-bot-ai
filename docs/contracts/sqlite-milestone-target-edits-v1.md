# SQLite milestone target edits, draft revision 1

MILESTONE-002, Tier A, evening contract preparation. Authority is specification
v0.45 [§8 and MILE-02](../specification.md#8-score-milestones-and-reversals),
[issue #230](https://github.com/Kernol12/subathon-bot-ai/issues/230), and
[owner decisions 51A and 52A](https://github.com/Kernol12/subathon-bot-ai/issues/230#issuecomment-7020230963).
Use the existing [progression API](milestone-progression-api-v1.md),
[group store](sqlite-milestone-groups-v1.md), [decision store](sqlite-milestone-group-decisions-v1.md)
and [controlled writer](sqlite-writer-v1.md). Resolve the proposed choices before
freezing the acceptance baseline.

## Edit and preview

An edit addresses one configured track in the current installation. The proposed
editor accepts a nonempty set of changed targets as one command. Apply the entire
patch by exact milestone identity and validate the resulting list in configured
identity order. Refuse duplicate and unknown identities, additions/removals,
out-of-range values, ties and crossings. A request with no actual changes has no
edit result. Targets and actual score use the full signed 64-bit micro-point
domain. Distances use exact unsigned 64-bit arithmetic. Preserve the progression
and writer budgets. Refuse exhaustion before partial output or sequence reuse.

Read the actual active completed prefix. A changed identity in that prefix
requires distinct current completed-target correction authority. Preserve every
historic completion and its captured definition revision, target, actual score
and context byte for byte. Preserve its occurrence, captured action versions,
timeline state, holds, decisions and delivery/bonus history. Project the deficit
against the current proposed target, with zero interval progress when below the
last completed target. A correction changes a threshold at unchanged score.
It never calls refund policy or releases, skips, cancels or reinstates a prior
group. A later genuine refund uses its owning policy and then-current target.

From the same actual score and prefix, compare the old and proposed consecutive
reachable suffixes (next target through the last target at or below score).
Report the old and new targets, score, revision, completed deficits, next
identity/remaining and exact interval where defined, and the ordered new
achievements with their action and timing consequences. The active completed
prefix records actual achievement. For a policy-suppressed reachable suffix,
show that progression is held and retain the actual prefix. A currently
authorized reader may see a refused edit's consequences without a confirmable
preview.

- Finished with post-finish progression disabled refuses the whole edit if
  it newly reaches a milestone. The proposed comparison uses reachable
  identities relative to the old definition when an unrecorded reachable
  suffix already exists. An authorized correction with no additional
  reachable identity remains eligible. No achievements are staged while
  post-finish progression is disabled.
- With progression enabled, an eligible edit advances the existing prefix
  through the proposed reachable suffix, in order. The proposed attribution
  rule refuses advancement if the old definition already has an unrecorded
  reachable suffix, until the lifecycle owner supplies a consistent baseline.
- Ready, active, paused and reopened use actual lifecycle eligibility and
  selected policies. A finish or reopen invalidates an earlier preview even
  at the same score. Editing targets does not start or reopen the countdown.
  It preserves phase, time, grace, debt, deadline, pause reasons, and any
  pending-action choices from a reopen. #183 owns lifecycle policy, catch-up
  and explicit reopening.

The owner supplies stable candidate revision and occurrence identities for
confirmation. Preview allocates no durable order or storage. Require exactly
one explicit Run, Defer or Skip for each proposed new occurrence,
including empty and bonus-only groups. Refuse missing, duplicate, invalid or
extra choices, including choices for historic completions. A first preview may
show consequences with choices unset. Confirmation uses a preview binding the
final exact choices. Capture resolved ordered action definitions/versions,
authorized route and sender selection where applicable, and timing overrides.
Use existing timeline inheritance. Bind relevant settings and capture revisions
even if a change leaves the achievement count unchanged.
Run gives ordinary eligibility subject to current gates. The proposed interaction
with disabled post-finish automation retains Run behind the lifecycle gate and
shows that disposition in preview. Defer adds an explicit hold. Skip is terminal
for that occurrence and retains achievement and capture.

## Authority and consistency

| Selected fact | Authority at preview and confirmation |
| --- | --- |
| Installation and authorization | The authoritative command context binds the authenticated actor to the current account/session and effective permissions for the exact track, edited identities and affected actions. Unknown, expired, revoked, disabled or mismatched facts refuse. Preview disclosure also requires current authority. |
| Definition and action/timing selection | Retained current configuration supplies the exact definition revision, ordered identities and selected action/capability/timing revisions and values. Current action/route authority supplies resolved sender and route facts. Historic groups preserve their own captures. |
| Score and application head | The selected committed application-state head and the actual signed score balance for the configured track. On the existing SQLite application path read the exact selected balance in ApplicationStateSnapshot. A requested delta, refund basis, displayed score, or unspecified absence supplies no score. |
| Completion and queue | The authoritative active completed prefix with its revision, plus the exact durable occurrence, release, choice and hold facts where relevant. Absence from a pending suffix does not cancel a completion. |
| Lifecycle and readiness | The authoritative lifecycle snapshot and selected progression/action policies with revisions, lifecycle/installation generation and availability. The owning SQLite startup/inspection boundary establishes schema readiness. A client flag, preview, catalog factory or wall time alone supplies none of these facts. |

Read writer-held facts on the same active session. Bind any owner-held facts to
that exact unit under the same serialization and revision/conditional-write
boundary. Prevent invalidation between revalidation and commit. Missing or
contradictory sources are unavailable. Later component acceptance may supply
these facts through bounded synthetic ports. #294 owns the real running
producers and publication.

Bind an owned opaque preview to installation/track, actor and session lineage,
exact original configuration and revision, exact patch and proposed definition,
score/head token, exact active completion revision/prefix, lifecycle and
policy facts, ordered affected occurrences, captured actions/timing and final
choices. Include the decision/operation identity in confirmable intent. Use
revisions or lineage that detect intervening changes even if values return
to their prior values. Bind eligibility facts shown for affected occurrences.
Allocate queue order at commit. Compare exact owned facts or a canonical
binding that preserves the same distinctions. Recheck current session and
permissions, including completed-target correction and applicable choice
authority, at confirmation. Recompute and require an exact match. Denied,
unavailable, invalid, stale and operation-key conflict results have no effects.
Protected detail requires current disclosure authority.

## Single writer unit and typed origin

On the existing caller-owned SQLite writer session, revalidate the preview,
conditionally advance the definition, append the audit and ordered active
completion suffix, stage corresponding group/action captures and record the
complete initial choice set before one commit. Audit original/revised values
and revisions, actor and decision identity, completed/unfinished classification,
actual score/head, and enough owned evidence for the confirmed projection
and stable committed result. New completions capture the revised definition,
new target, same actual score and edit context. Each gets one durable occurrence
including when skipped. Allocate positions from the same global order across
past, pending and skipped occurrences. Preserve any existing queue prefix,
active window and gap. A correction with no newly achieved suffix stages no
new group or action decision.

The proposed physical boundary adds typed records behind the existing milestone
group/decision and delivery interfaces. The frozen group FK requires a real
application operation. That operation requires receipt, plan and state
provenance. Delivery captures require application jobs, while decisions and
bonus receipts require legacy group/action parents. An edit needs its own
origin. Reusing an application's key, fabricating intake, bypassing a FK or
interpreting generic delivery group metadata cannot establish that origin.

| Selected record | Guard shape for the proposed additive step |
| --- | --- |
| Target edit operation | Typed edit identity and immutable audit, with unique resulting revision per installation/track. Store exact confirmed intent and stable result. An existing key with identical intent may return the committed result after current disclosure authorization. Different intent under the same key is conflict. Neither path enrolls again. |
| Edit occurrence link | FK to the exact edit and link to its completion, with unique occurrence key and positive suffix ordinal unique within the edit. Check track, milestone, result revision, target, score and context against the staged edit/completion. Original occurrences retain their existing application FK. Reject occurrence-key or queue-order collisions across origins. |
| Edit actions and jobs | One contiguous action order per occurrence. Each automation action has exactly one typed delivery key linked to the same edit, occurrence, ordinal, action and version. All edit jobs are milestone members. A missing or inconsistent link refuses dispatch even when the lookup returns no member. Bonus positions have no delivery key and use one durable receipt for the exact occurrence/position. |
| Edit choices and holds | Exactly one initial choice per edit occurrence, linked to its edit actor/decision. Edit origin is distinct from paused normal/silent acceptance. Reuse Run/Defer/Skip and later independent refund Review semantics and resolution provenance. Verify the exact typed parent before consulting or changing a hold. |

Use additive origin-specific parents where frozen FKs require them. Keep a
durable current definition revision link and immutable edit/occurrence history.
Check same-track predecessor and result revisions and guard the definition
advance against the exact previous head. Same-session validation covers
cross-origin uniqueness and detects inconsistent persisted links on read.
The existing
group store reads one ordered stream across origins. The existing decision store
resolves a single typed occurrence per key. Original enrollment/acceptance
selects only the exact application and its staged occurrences. Edit enrollment
and choice recording select only the staged edit and its full occurrence set.
Require typed keys at lookup and claim. Genuine independent application jobs
retain their current behavior. Positive membership with malformed provenance
fails closed. Edit delivery reuses the queued/claim/outcome lifecycle and
requires committed group release plus current dispatch authority in the claim
work. Bonus application checks eligibility and commits effect and receipt
together. #197 owns individual/batch/silent input review. An edit has no
application-review disposition.

## Release, failure and recovery

After acknowledged commit, publish one committed result and the ordered captures
to the existing timeline owner. Actual release is separate work on the exact
committed pending head under current gates. A held head blocks the suffix.
Skip consumes no presentation or gap. Preserve queue pause and optional countdown
linkage, active windows and delays. Defer and refund Review are independent.
Continuing one leaves the other active. Terminal Skip and hold-ending provenance
follow the existing decision contract. Preserve application, financial, recovery,
and reopen holds. Cold recovery restores the committed pending suffix in durable
order without starting presentation, release, claim or bonus work.

Known rollback preserves the previous definition, active prefix, audit, captures,
decisions and release history, even after a reached late failure. Preserve the
writer's failure categories and provisional-result rules. After ambiguous
completion or commit followed by work/acknowledgement failure, publish neither
success nor refusal for that failed attempt. Read actual durable state by edit
identity in a fresh writer unit before deciding whether to reattempt. A committed
edit's exact authorized retry reads the same result without additional effect or
release. A competing change to a bound fact makes the old preview stale even
with a different operation key.

## Catalog boundary

One additive step with proposed ID milestone-target-edits-v1 and version
<owner-assigned> in the published night order. Use the immediately preceding
compiled, reviewed manifest. Append one step and its supported starting version.
Preserve earlier ordered SQL, schema projections, versions, IDs and history
digests. Follow existing milestone DDL conventions for exact ordered SQL strings,
main schema projection, INTEGER storage/enum checks, bounded BLOB references
and the canonical projection of implicit PK/unique indexes. Fresh and populated
upgrades install the same empty additions. Existing origins, queue positions and
action history retain their meaning without v11 metadata backfill or synthesized
decisions.

Dispatch, pending/head, bonus and decision paths recognize exact supported
version/ID pairs and verify required schema, including the new tables/columns
on recognized later paths even where no row matches. Preserve v12 ordinary
release and v13+ decision behavior, including reviewed v14 and v15 and later
approved prefixes. An unknown/mismatched marker or incomplete required schema
fails closed without fallback to independent dispatch or earlier behavior.
Review the complete frozen prefix and exact new frame/schema before runner work.
Use existing operator maintenance and startup inspection readiness.

## Independent examples

All numeric examples use integer micro-points. A, B, C and D are configured
identities in order. Each row is a separate starting state unless stated otherwise.

| Case | Input and expected result |
| --- | --- |
| Progress only | A=100 complete, B=200, score=160. B→180 gives remaining 40→20 and progress 60/100→60/80, or 3/5→3/4. No new group. |
| Tie and crossing | A=100, B=200, C=300. B→100 ties A. B→301 crosses C. Both refuse with order, revision and audit intact. |
| Ordered achievement | A=100 complete, B=200, C=300, D=400, score=175. B→150, C→175, D→250. B and C complete at actual score 175, C by equality. Remaining 75 and progress 0/75. With preceding pending order 7, B Defer is 8 and C Run is 9. After 7's window and gap, every active cause on 8 must resolve before it starts. Then 9 waits for 8's window and gap. If 8 becomes Skip, 9 may start after 7's gap with no window or gap for 8. |
| Completed correction | A completed at target 100/score 120, B=200, current score=130. A→150 gives deficit 20, next remaining 70 and progress 0/50. Retain original completion and its queued or performed group. A missing correction grant refuses. A current grant allows the correction even while finished with progression disabled. No refund audit or release. |
| Finished and reopened | Use the ordered-achievement input while finished with progression disabled. Refuse the entire edit. Reopen separately. The old preview is stale. With a current eligible observation and fresh preview, staging leaves the reopened countdown as observed. |
| Exhaustion | With completed target INT64_MAX and current score INT64_MIN, deficit is 18446744073709551615. An all-completed track has no next or interval. Separately, prior queue order INT64_MAX−1 leaves only one position. A two-occurrence edit refuses atomically. |
| Rollback and retry | For the ordered-achievement edit, fail a reached choice insertion after definition, audit, both completions and groups are staged. A fresh read sees B=200, C=300, D=400, only A complete and order still 7. Committing the same valid intent once yields 8/9. Lost acknowledgement followed by authorized read/retry finds that pair once with its original choices and no release replay. |

Later acceptance covers populated prefix upgrades, exact head selection across
origins, missing links and wrong-origin choices, concurrent invalidation,
bounded input, rollback and ambiguous durability. Use synthetic captures and
observable fake effects.
