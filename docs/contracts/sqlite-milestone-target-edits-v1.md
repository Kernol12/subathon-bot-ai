# SQLite milestone target edits, draft revision 2

MILESTONE-002 [#230](https://github.com/Kernol12/subathon-bot-ai/issues/230), Tier A,
contract correction round 1. Authority is specification v0.45
[§8 and MILE-02](../specification.md#8-score-milestones-and-reversals),
[51A/52A](https://github.com/Kernol12/subathon-bot-ai/issues/230#issuecomment-7020230963)
and [70A/71A/72A/73B](https://github.com/Kernol12/subathon-bot-ai/issues/230#issuecomment-7021743068).
Use the existing [progression API](milestone-progression-api-v1.md),
[group store](sqlite-milestone-groups-v1.md), [decision store](sqlite-milestone-group-decisions-v1.md)
and [controlled writer](sqlite-writer-v1.md). Independent verification and closing
review precede the acceptance freeze.

## Edit and preview

The milestone target-edit command service owns preview and confirmation for one
configured track in the current installation. One atomic patch replaces one or
more targets by exact milestone identity. Validate the full result in configured
identity order. Refuse duplicate or unknown identities, additions/removals,
out-of-range values, ties and crossings. A request with no actual changes has no
edit result. Classify changed identities against the actual active completed
prefix. Unfinished edits and completed corrections require separate current
grants. A mixed patch requires both. Targets and actual score use signed
64-bit micro-points. Distances use exact unsigned 64-bit arithmetic. Preserve
progression and writer budgets and preflight order capacity for the whole suffix.

From the same actual score and prefix, compare the old and proposed consecutive
reachable suffixes, starting at the first unfinished target and stopping at the
first target above score. Preview the old and new targets, revisions, unchanged
score, deficits against current targets, next identity/remaining, exact interval
where defined, and ordered new achievements with action and timing consequences.
If policy suppresses progression, retain the actual prefix and flag the
reachable suffix as suppressed. A reachable next target in that view has zero
remaining and no progression interval fraction. A reader with current disclosure
authority may see the consequences of a refusal without a confirmable preview.

- Finished with post-finish progression disabled refuses the whole edit if the
  proposed reachable suffix adds any identity relative to the old one. Stage no
  achievements while that policy disables progression. A completed correction
  that adds no reachable identity can still be eligible.
- With progression enabled, refuse with the distinct typed reason
  `old_reachable_backlog` if any unfinished target is already reachable in the
  old definition. Create no hold. Otherwise append the entire reachable suffix
  under the proposed definition in configured order. Exercise the backlog with
  synthetic authoritative facts. #230 can proceed independently of #183.
- Ready, Active (including pause) and a reopened run use the observed lifecycle
  and selected policies. Finish or reopen invalidates a prior preview even at
  identical score. Editing targets preserves the countdown state, including
  phase, time, grace, debt, deadline, pause reasons and pending-action choices
  from reopening. It cannot start or reopen the timer. #183 owns lifecycle
  policy, catch-up and explicit reopening.

The command service supplies stable candidate definition and occurrence identities.
Preview reserves no durable order and writes no storage. Require exactly one
explicit Run, Defer or Skip per new occurrence, including empty and bonus-only
groups. Refuse missing, duplicate, invalid or extra choices, including choices
for historical occurrences. A first preview may leave choices unset. Confirmation
binds the final choices. The confirming unfinished-edit and/or completed-correction
grants cover these initial choices. Capture ordered action definitions/versions,
authorized route and sender selection where applicable, and timing overrides with
existing inheritance. Bind relevant settings and capture revisions even when
achievement count stays the same.

Run gives ordinary eligibility subject to current gates. Post-finish progression
and post-finish automation are independent. With progression enabled and automation
disabled, record the permitted achievements and retain Run behind the automation
gate, exposed in preview. Release, dispatch claims and bonus work must respect
that gate. Run creates no explicit hold. Defer adds an explicit
hold. Skip is terminal for that occurrence and retains achievement and capture.
Dispatch checks its own current authority.

## Authoritative facts and confirmation

| Selected fact | Source |
| --- | --- |
| Installation and authorization | Authoritative command context binds actor, account/session and effective scoped grants to the installation, exact track and edited identities. Current action/route authority supplies resolved route and sender facts. Unknown, expired, revoked, disabled or mismatched facts refuse. Disclosure also requires current authority. |
| Definition and action/timing selection | Durable milestone definition head and immutable versions below supply ordered targets. Retained configuration supplies the exact selected action, capability and timing revisions and values. Historical groups retain their own captures. |
| Score and application head | Read the selected committed application-state head and the actual signed balance for the track in its ApplicationStateSnapshot. A requested delta, refund basis, displayed score or unspecified absence supplies no score. |
| Completion and queue | Durable active completion-prefix head and ordered memberships below, plus exact occurrence, release, choice and hold facts where relevant. Completion is independent of pending membership. |
| Lifecycle and readiness | Authoritative lifecycle snapshot, progression/automation policies and revisions, installation/lifecycle generation and availability. SQLite startup/inspection establishes schema readiness. Client flags, previews, catalog factories and wall time cannot establish those facts. |

The **milestone track initializer**, called by the owning configuration and
progression composition, is the only bootstrap of the definition and active
prefix heads. In one writer unit it checks the exact selected retained track
configuration and an explicit ordered active completion snapshot from the
authoritative progression owner, including scoped origin evidence for every
member. An explicitly known empty prefix is valid. An absent source, queue scan
or score-derived reconstruction cannot prove a prefix. Do not infer track
membership or current activity by scanning v12 history. Fresh and populated
catalog upgrades install empty additions. With uninitialized or contradictory
facts the edit service is unavailable. Component acceptance supplies explicit
synthetic configuration, completion and lifecycle facts to this boundary.
#294 owns live producers, wiring and publication.

The **milestone definition head** is keyed by installation/track and selects
one **immutable milestone definition version** of that track. Each version owns
the full ordered identity/target list, with a unique scoped revision and a
same-track predecessor for successors. Head and predecessor links reference
those version records. Initialize from the exact retained configuration,
then compare-and-advance the head from the exact bound revision when editing.
The operation retains both versions and the changed values. Never reuse a
revision after a value returns to its earlier setting.

The **active completion-prefix head** is keyed by installation/track and selects
an immutable scoped prefix revision containing ordered memberships from ordinal
1 through its recorded length. The head references the scoped revision and
each ordinal is unique in that revision. Each membership links to that revision
and carries the exact completion (milestone, captured definition revision,
target, actual score and context) and a typed origin. Application origin names
the real application operation and exact v12 occurrence. Edit origin names the
exact edit operation and edit occurrence. Each type has its corresponding
FK-backed occurrence link. Check scoped track
identity against the initializer or originating writer facts and exact capture
agreement, including the application parent for v12. Captured revisions remain
historical even when the current definition changes. Compare prefix identities
against the selected definition's first k positions. When adding achievements,
advance the prefix head to a new revision in the same writer unit and preserve
all earlier member facts. Other progression owners must maintain this source
under the same writer serialization before live use. MILE-07 later owns
authorized reset by changing active membership in a new revision while retaining
old revisions, occurrences and effects. It also owns unstarted group
cancellation and any replay authorization on re-earning.

Read writer-held facts on the same active session. Bind owner-held facts to
that unit under the same serialization and revision/conditional-write boundary
through commit. Bind an owned opaque preview to installation/track, actor and
session lineage, original configuration/revision, exact patch and proposed
definition, score/head token, exact active prefix revision and membership,
lifecycle and policies, ordered affected occurrences, captured actions/timing,
eligibility facts shown and final choices. Include the operation and decision
identity. Use revisions or lineage that detect intermediary changes even when
values return. Recheck current grants and compare bound source revisions before
recomputing the plan at confirmation. A changed lifecycle anchor makes the
old intent stale even if a fresh request would give a policy refusal. Require
an exact binding match. Denied, unavailable, invalid, stale and key-conflict outcomes
have no effects. Protected detail requires current disclosure authority.

## Single writer unit and typed origin

On the caller-owned SQLite writer session, revalidate the preview, append the
immutable version and audit, advance the definition head, append the active
completion suffix and stage its captures and full choice set before one commit.
Stage parents before dependent rows, including occurrences before prefix links.
Audit old/new revisions and targets, actor and decision, completed/unfinished
classification, exact score/head, and owned evidence for the confirmed projection
and stable result. For each achievement record the new revision and target at
the same actual score with the edit context, and one durable occurrence even
if skipped. A correction with no new suffix preserves every existing completion,
group, hold, decision and action history. It neither observes a refund nor
changes any prior group's eligibility.

Use additive edit-origin rows with these keys and parent links. The frozen
application-owned v12 group/action/bonus rows, v13 decision rows and v11
delivery captures/events retain their existing application parents and meaning.

| Selected row | Required key and link |
| --- | --- |
| Target edit operation | Typed scoped key with FKs to prior and resulting same-track definition versions, unique resulting revision, exact confirmed intent and stable result. An identical authorized retry returns that result. Different intent with the same key conflicts. |
| Edit occurrence | Unique occurrence key with FK to its edit and unique positive suffix ordinal within the edit. Immutable completion capture and timing, durable queue position and release state. The new prefix membership links to this occurrence and agrees with its edit, track and captured completion fields. |
| Edit action | FK to the occurrence, with contiguous positive action ordinals and exact action kind/ref/version. Each automation position has exactly one edit job. A bonus has no job. |
| Edit job, capture and events | The typed edit job key selects the exact edit, occurrence and action position. Job FK references that edit action by occurrence/ordinal and verifies its edit parent. Capture FK references that job and retains matching action/version plus resolved route, selection and sender. Event FK references that capture, with unique revision per job and the existing queued, attempt, outcome and evidence semantics, starting at queued revision 1. |
| Edit bonus receipt | PK/FK on the exact edit action position, which must be a bonus, with one stable effect reference. |
| Edit initial decision | PK/FK on the edit occurrence, with Run/Defer/Skip and the confirming actor/decision linked to and checked against its edit. Preserve resolution and hold-ending provenance under #236. This origin has no paused input Normal/Silent disposition. |
| Edit refund decision | PK on occurrence/refund reference and FK to the edit occurrence. Preserve exact before/after/current-target operands, policy, resolution and hold-ending provenance under #236. Each resolution or ending reference selects the exact cause on that occurrence. |

Never use an application key or synthetic receipt, plan, state or application
job as a surrogate edit parent. Generic delivery metadata cannot prove membership.
Application enrollment selects only the exact application and its staged
occurrences. Edit enrollment and initial decisions select only the staged edit
and its complete occurrence set. #197 owns individual/batch/silent input review.

Allocate one strictly increasing positive signed-64 order across the two
origins, after the maximum committed position including released and skipped
rows, or start at 1 when neither origin has a row. The writer checks capacity
for the entire batch before allocation and
rejects key or position collisions across origins. There is one pending head:
the least unreleased, non-skipped position across both origins. A held head
remains pending. Check exact typed keys, parent/capture agreement, contiguous
ordinals, cross-origin uniqueness and head selection under the writer session.
Use bounded keyed and ordered reads, checking the selected closure against
existing writer, collection and reference limits. Malformed or excess data
refuses the work without a partial snapshot, decision set or dispatch fallback.

## Existing readers, release and recovery

- **#232 delivery dispatch, claim, outcome and stranded sweep.** Resolve the
  exact typed job and membership. For edit jobs, require matching edit
  provenance, committed group release and current dispatch authority inside
  queued-to-sending work. Missing or inconsistent links refuse. Genuine
  independent application jobs keep their existing path. Outcome uses the
  exact claimed job and attempt. The exclusive stranded sweep may mark a
  sending attempt outcome-unknown without starting an adapter. No delivery replay.
- **#234/#236 order, disposition, release, restart and decisions.** Use one
  ordered stream and one exact typed occurrence per key for pending/head,
  disposition, hold reads and resolution. On acknowledged edit commit,
  publish the result and ordered captures to the timeline owner. On a separate
  writer unit release rechecks the exact committed pending head under current
  gates. Preserve queue pauses, optional countdown linkage, active windows and
  gaps. Skip consumes neither a presentation nor a gap. Defer and each refund
  Review are independent. Continue clears only its selected cause and terminal
  Skip ends other active causes with provenance. Preserve application,
  financial, recovery and reopen holds. Cold restart restores the committed
  pending suffix in order without starting presentation, release, claim or bonus.
- **Bonus.** Resolve the exact typed occurrence and bonus position. Check
  committed release and current authority and commit the effect and receipt
  together. A receipt prevents reapplication.
- **#199 refund observation and review.** In the genuine refund writer unit,
  `FinancialRefundEffectPort::milestones` selects eligible pending occurrences
  across both origins and reads each current target from the authoritative
  definition head on the same session. Keep each captured target intact.
  Use exact actual scores and a downward crossing of the current target
  (before ≥ target, after < target) while pending and unreleased. Consult and
  record audit, Review/Continue/Skip and later resolution via #236 on the exact
  typed occurrence. Preserve the existing refund accounting and independent
  deferrals. Target edits themselves never invoke this path.

Known rollback retains the prior heads, prefix, audit, captures, decisions and
release history, including after a reached late failure. Keep writer errors and
provisional-result rules. Ambiguous completion or a failed acknowledgement
after commit publishes neither success nor refusal for that attempt. Re-read
durable state by edit identity in fresh writer work before reattempting.
An authorized exact committed retry returns the original result without a second
effect or release. A competing change to a bound fact makes an unused preview
stale even with a different operation key.

## Catalog boundary

Append one additive step with proposed ID `milestone-target-edits-v1` and
its supported starting version to the immediately preceding compiled, reviewed
manifest. Assign the number only in the published night order. Preserve all
earlier ordered SQL, schema projections, versions, IDs and history digests.
Follow the existing main schema projection, exact SQL, INTEGER storage/enum
checks, bounded BLOB references and implicit index conventions. No v11 metadata
backfill or synthesized decisions.

Recognize exact supported version/ID pairs with required schema on dispatch
(including claims, outcomes and stranded recovery), group order/disposition/
release/restart, bonus, decision and #199 refund paths. Preserve v11 independent
delivery, v12 ordinary release, v13 decisions, reviewed v14/v15 and later
approved prefixes. The new exact pair must select compatible earlier behavior
plus the applicable edit-origin rows, even when no edit row matches. An unknown
or mismatched marker or incomplete required schema fails closed. Review the
complete frozen prefix and exact new SQL/projection before runner work. Use
existing operator maintenance and startup inspection readiness.

From the start, each further catalog fixture supplies a positive explicit
`MigrationRequest.limits.manifest_utf8_bytes` based on the measured cumulative
UTF-8 payload of its exact manifest. Count the SQL at each step and every
projected type, name, table name and present SQL at each step, with repetitions.
Keep the default unchanged. The exact published prefix and new frame are needed
before the fixture budget can be measured.

## Independent examples

All numbers are integer micro-points. Each row starts independently with
current grants and an eligible Active lifecycle unless stated otherwise.

| Case | Input and expected result |
| --- | --- |
| Single target, progress | A=100 complete, B=200, score=160. B→180 gives remaining 40→20 and progress 60/100→60/80 (3/5→3/4). No new group. |
| Single target, equality | A=100 complete, B=200, C=300, score=175, tail order 7. B→175 with Run completes only B at score 175, order 8. C remains next, remaining 125, progress 0/125. |
| Tie and crossing | Active and eligible, A=100 complete, B=200, C=300, score=160, unfinished grant current. B→100 ties A. B→301 crosses C. Each refuses with targets, revision, prefix, queue and audit intact. |
| Ordered patch (70A) | A=100 complete, B=200, C=300, D=400, score=175. B→150, C→175, D→250. B and C complete at actual score 175, C by equality. D remains next, remaining 75 and progress 0/75. With pending predecessor 7, B Defer is 8 and C Run is 9. After 7's window and gap, each cause on 8 must clear before 8 starts. Then 9 waits for 8's window and gap. If 8 becomes Skip, 9 can start after 7's gap. |
| Completed correction | A completed at target 100/score 120, B=200, current score=130. A→150 gives deficit 20, next remaining 70 and progress 0/50. Preserve the original completion and group with no refund audit or release. Only the completed-correction grant is needed, including while Finished with progression disabled. |
| Mixed grants (72A/73B) | A=100 complete, B=200, score=175. A→110 and B→175 with Skip needs both grants. Either grant alone refuses both changes. Both grants record A's correction and one skipped B occurrence at score 175. There is no next target or interval. |
| Old backlog (71A) | Progression enabled, A=100 complete, B=180, C=300, score=190. B→170 with B already reachable in the old definition refuses with `old_reachable_backlog`. Preserve both heads and the queue and create no hold. |
| Finished and reopened | While Active, obtain a confirmable preview for the ordered patch with B Defer and C Run. Before confirming, finish independently with post-finish progression disabled and the same score. The Active preview is stale. A fresh Finished preview refuses the patch. Reopen independently, keeping score and prefix. The pre-Finished preview stays stale. A fresh eligible preview can confirm, preserving the reopened countdown as observed. |
| Finished already-reachable target | Finished with progression disabled, A=100 complete, B=180, C=300, score=190. A→150 leaves B as the only reachable unfinished target. A's correction is eligible, B stays suppressed and no group is staged. |
| Finished automation gate | With progression enabled and automation disabled while Finished, use the single-target equality case. Record B and retain its Run with the automation gate shown. The countdown stays Finished. |
| Exhaustion | With sole target A=INT64_MAX−1 already complete at that target and current score INT64_MIN, correct A→INT64_MAX. Deficit increases from 18446744073709551614 to 18446744073709551615 with no next or interval. Separately run the ordered patch input with a durable tail at INT64_MAX−1. Only position INT64_MAX remains for two new occurrences. Refuse with both heads, audit, queue and decisions intact. |
| Genuine refund after correction | A=100 and B=175 are complete, C=300, score=190. B completed by an edit at 175 and its group remains deferred. Correct B→185. A later genuine refund moving actual score 190→180 crosses current target 185 and under Review records deficit 5. Using captured target 175 would miss the crossing. B stays complete, retains captured target 175 and has independent Defer and refund Review holds. |
| Rollback and retry | For the ordered patch, fail a reached choice insertion after the definition, audit, both completions and groups are staged. A fresh read sees B=200, C=300, D=400, only A complete and order still 7. Committing the same valid intent once yields 8/9. A lost acknowledgement and authorized read/retry recover that pair once with its original choices and no release replay. |

Later acceptance uses synthetic captures and observable fake effects for
populated upgrades, cross-origin head and refund selection, missing or
wrong-origin links, concurrent invalidation, limits and ambiguous durability.
