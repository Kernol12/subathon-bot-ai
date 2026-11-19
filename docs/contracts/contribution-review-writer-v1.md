# Contribution review and continuation writer, revision 1

ACCT-005 [#197](https://github.com/Kernol12/subathon-bot-ai/issues/197), Tier A,
Stage 1 contract candidate. Specification v0.45 §§3, 5, 8, 10. Bind
[owner decisions 59A–64A](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7020230829)
and [coordinator claim](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7021460016).
Design base `8056048c238bdbd7c208eac11117f9b73b04097f`, tree
`29ef6bff2909acfd2847502070b69b140cb906d5`, protected v15 through #199.
The coordinator assigns one additive catalog step in the claimed order after #270.
Independent contract review precedes freezing examples and authoring the baseline.

## Owned command and result

Use the existing [application service](application-service-api-v1.md),
[original claim](sqlite-original-claims-v1.md), [bundle](sqlite-application-bundle-v1.md),
[plan](sqlite-application-plans-v1.md), [hold](sqlite-application-holds-api-v1.md),
[milestone choice](sqlite-milestone-group-decisions-v1.md) and
[writer session](writer-transactions-v1.md) value types. The internal Qt owner
offers `preview(ReviewPreviewRequest)`, `execute(ReviewCommand)` and permissioned
`lookup(ApplicationOperationKey)`. Single input uses the same path as a batch
of one. Preview and lookup are read-only work. These are owned DTOs, not a wire API.

| Value | Fields and invariants |
| --- | --- |
| Selection | Stable `ApplicationPrincipalBinding` and 1–4 unique contribution selectors, each CID plus original positive support ReceiptId. Resolve both against the same committed anchor. Session proof is supplied separately through the current trusted admission path. |
| Entry intent | Selector and verb `Accept`, `Reject`, `ApplyAccepted`, `ContinueTimer`, `RejectHeld` or `RetainHeld`. The latter three select an exact ordinary timer hold/version, occurrence and signed remainder. `ApplyAccepted` references the exact accepted-pending decision. |
| Entry options | Accept/ApplyAccepted carry `PausedAcceptanceMode`, optional complete ordered `MilestoneAcceptanceChoice` vector, and optional authorized plan revision proposal bound to the expected parent. Present empty choices mean zero affected occurrences. Absence means choices have yet to be supplied. ContinueTimer optionally carries explicit StopAtZero or CarryTimeDebt and, for an unclaimed whole input, carries the apply options without a calculation override. Reject and Retain have no apply options. |
| Preview row | Canonical selector, current review and application dimensions, authorized blocker and resolved-request details, expected basis below, and prospective actual amounts and ordered affected occurrences when available. Unknown amounts/occurrences stay absent. Return separate eligibility for the requested decision and its application. |
| Preview request | Selection and one entry intent per selector. Preview derives occurrences before validating supplied choices, so an initial request with absent choices can show the prospective effects and choices required. |
| Execute request | Stable work `ApplicationOperationKey`, selection, entry intents/options with preview bases, nonempty actor, per-entry decision ref and reason. Actor is checked against authenticated provenance. |
| Hold closure options | For linked apply, an optional ordered vector of exact ordinary version locators and explicit owning decisions for additional resolvable holds. The timer selector/verb supplies its own closure. No user-provided decision is proof of resolution eligibility. |
| Row outcome | `AcceptedPending`, `Applied`, `Rejected`, `TimerResolved`, `Retained`, `Ineligible(reason)` or `Stale(reason)`. Pending or retained carries permitted blockers/hold locators. Applied or resolved links the actual committed bundle/closure and disposition data. Ineligible/stale carry no protected facts without disclosure permission. |
| Work result | Ordered complete row outcomes, `Existing` with the immutable authorized result, or a whole-work refusal (`InvalidSelection`, `Denied`, `IdentityConflict`, `BatchTooLarge`, `Unavailable`). Writer outcome/error stays separate. |

Use committed original support ReceiptId order (unsigned ascending) for preview,
execution, persisted entry ordinals and response. Receipt allocation is durable
intake order. A confirmation ID, source time, clock time, client list order and
duplicate intake attempt cannot change it. Duplicate selectors or conflicting
anchors invalidate the request. An unresolved but well-formed selector yields
individual unavailability with disclosure-safe data. Receipt intake precedes the
review transaction. A batched execution cannot supply another unpreviewed row.

## Preview, authority and freshness

Preview simulates the selected verbs/options in intake order. Later rows see the
effects of the projected eligible prefix, including consumed cap headroom and
milestones. Return a server-produced expected basis per row bound to the complete
selection and intents, stable principal, original receipt/CID, actual plan head,
all current contribution fields, application state token, relevant authority and
policy/configuration revisions, complete mixed hold versions and projected prefix.
Use stable proposed occurrence identities bound to the original selector, target
revision and crossing order, with authoritative durable occurrence refs allocated
and checked on stage. The basis binds the projected application and occurrence
set. An initially absent choice set can be filled on execution for exactly those
occurrences. A changed calculation or mode requires a new preview. Preview writes no
review, plan, hold, clock, achievement, job or claim. Preview rejection shows
what would be rejected, including any prospective application suppressed by it.
Use the persisted starting token for freshness. Validate later rows against the
projected preceding state and stage them against actual preceding in-unit tokens.

Under the owner turn, check current session and execution authority before protected
lookups and check disclosure before returning any selected data. Recheck effective
grants for each actual subject, capability, resource and quantity, including any
configured batch scope. Apply §10 complete-grant and deny precedence. Review,
silent application, override, timer continuation and milestone choices require their
applicable current grants. Approval requires neither blanket permission over every
row nor joining constraints from different grants to approve one use. Unavailable
authority fails closed. A request with invalid session or batch authority has no
individual protected outcomes. A per-row denial does not approve other work on
that subject. Reading a past result has its own current disclosure check.

Resolve the live basis inside the same transaction. A changed plan/head, any
contribution cell, complete hold set, applicable policy/permission revision or
material prefix is stale even with an unchanged state token. Check current permission
first so revoked scope returns denial with no facts. Compare actual fields, never a
digest alone. An earlier row changing only an unrelated decision does not stale a
later row. Reobserve the authoritative clock and recompute arithmetic over the
in-unit state. Clock passage alone can update preview estimates if still within
current grants and the eligible branch, blockers and identified milestone choices
are unchanged. A changed branch, blocker or choice set requires a fresh preview.
Disclose actual amounts, including observed lifecycle transitions, with the result.
Treat Ready or unknown lifecycle/policy/clock as an application prerequisite.

Bind the stable work key to the complete canonical intent, including ordered subjects,
verbs, expectations, options, actor/decision/reason and stable principal. Exclude
transient session credentials and newly observed outcomes. Existing authorized
exact retry returns the original journal outcome before current-basis comparisons.
Different immutable intent is a conflict. Recheck execution authority against the
stored scope/quantities and disclosure for the complete result. Return denial or
disclosure-safe refusal if that result cannot be returned. Authenticate before
revealing key existence and require authority for that installation/key scope.
Reserve a review-key namespace by installation, disjoint
from the ordinary application, correction and financial operation namespaces.
Each applied entry has its own distinct immutable bundle key, linked to the
work key and ordinal. Reserve checked identifiers before staging. Never alias a
previous bundle, operation, occurrence, decision or hold.

## Decisions, blockers and lifecycle

- Accept or Reject requires ordinary review pending and no original award.
  Reject patches ordinary review to rejected and records the decision. Accept
  patches it to accepted. Neither patch rewrites the evidence-review decision,
  live evidence, valuation, application, reversal, settlement or original capture.
  Keep independent blockers intact on Reject. A terminal rejection prevents later
  application on another key. Reversal of such a decision needs its own authority
  outside this contract.
- Accept-and-apply stages the ordinary review patch first. Pass the resulting
  full current contribution snapshot to bundle staging, which independently
  compares it and patches only application. A failure after either patch
  rolls both back.
- Accept may be recorded while a known independent blocker (payment, valuation,
  settlement, recovery or unrelated hold, for example) prevents application.
  Record `AcceptedPending`, including mode, supplied choices or their absence,
  actor, original context and blockers. Make no claim, effect, milestone, job or
  application clock/head change. Approval admits the ordinary review dimension
  only. Missing choices that are required for an otherwise eligible application
  require a corrected preview/intent. A pending acceptance never queues replay.
- A subsequent `ApplyAccepted` requires the pending decision, a new key, fresh
  preview and current execution grants for its own actor, options and actions.
  It keeps the accepted mode unless a new explicitly authorized audited change
  is recorded. Revalidate every choice against the newly projected occurrences.
  Record both decisions and actors. Confirmation, rate entry, pause release,
  recovery or reopen cannot perform this apply on their own. Paused acceptance
  leaves each pause reason intact and cannot override unresolved recovery.
- Select original arrival rules, modifier context, quote and provenance from the
  actual capture. Preserve source facts, optional unknowns and prior revisions.
  An authorized replacement appends the immediate child revision using the actual
  plan-head CAS and retains actor, decision, reason and exact changed selections/
  valuation/requests. It shares the unit with the decision using it. Authorization
  precedes this append. A rejected, stale or denied row never revises a plan.
  A missing historical rate requires the owning producer's explicit retained
  resolution. Current rules or rates supply no implicit replacement.
- For a started timer, observe current authoritative clock/lifecycle first,
  including hard end at equality through pause, grace or confirmation wait.
  Active signed original requests require the owning CORE-008 derivation and
  authority. A RequireReview deficit on any timer occurrence defers the **whole**
  original, before original claim or any effect, job or achievement. Retain the
  captured plan, exact full signed blocked request, ordinal, basis and explicit
  `defer_whole_input` timer hold. Other evaluated deltas are speculative. A review
  decision can still be accepted-pending in this same unit. Ordinary numeric
  overflow retains the existing whole-input overflow behavior and reason.
- In Finished, record every time request with terminal disposition `Finished`
  and zero Main/debt deltas. Retain its full signed request and exact provenance.
  Give siblings only the application effects permitted by current post-finish
  policy. Record a lifecycle-policy suppression for each denied request, including
  a denied score/count request. No cap-limited amount, timer hold, debt creation
  or latent time replay. A terminal original consumes its original claim even if
  all requests are suppressed. Keep actual lifecycle transitions separate from
  the effect deltas. [#183](https://github.com/Kernol12/subathon-bot-ai/issues/183)
  supplies live Finished and explicit reopen policy. An unavailable policy blocks
  application. Reopen preserves history, score, achievements, debt and independent
  holds under its owning policy. Only its explicitly selected pending action
  handling can affect genuinely pending groups.

## Complete hold set and ordinary continuation

Read the selected ordinary and financial active sets for the actual original
subject. Compare the complete union in v15 raw hold-ref order, with exact versions
and no duplicate locators. Any malformed member fails the unit. Presence of a
financial hold is never execution authority. Ordinary linked application uses
`stage_financial_aware_linked_resolution`, closes **all** active ordinary members with exact
versions and explicit authorized decisions, has no replacements, and preserves
all financial holds. Missing/stale members or an unresolved ordinary blocker
prevent linked application. Known financial hold preservation by the guard grants
no permission to apply through that blocker. Review-only acceptance and unlinked
authorized rejection may coexist with foreign holds and leave them intact.
Additional ordinary closures require an explicit entry for each actual version
and checked resolution from its owning authority. Compare the mixed set before
projecting the ordinary entries into the existing v15 staging API. Bind the
full expected set into review audit, including preserved financial locators.

Continue selects the whole exact signed remainder from the currently active
hold and uses a fresh decision and authority. For an unclaimed whole-input hold,
require ordinary review accepted or not required and link the pending decision
when present. Stage the same original exactly once with the complete retained request list
and eligible siblings. Close the hold(s) with the new original bundle link in
the same work. Revalidate every ordinary request in plan order. A choice for one
held ordinal cannot settle a deficit at another ordinal. For a supported prior ordinary timer-only hold beside an already
claimed original, use a distinct authorized correction receipt and bundle linked
as `timer_correction` to that exact version/ordinal/remainder. Stage the timer
settlement alone. Preserve the earlier original claim, results, siblings and jobs.
Accept neither application of a smaller requested remainder nor retroactive
revision of held units. RejectHeld closes the selected remainder with a terminal
audited rejection and no effect. For a whole unclaimed original it also bars
future original application. RetainHeld records an explicit audit and leaves the
active hold unchanged. A still-insufficient unoverridden continuation returns
retained, even if the live default has since changed to Stop or Carry. With an
explicitly authorized Stop/Carry choice, settle the whole
request and record signed requested, actual Main/debt and unapplied quantities.
No leftover pending request survives settlement. For negative requests enforce
`request = MainDelta - DebtDelta + unapplied` with checked exact arithmetic.
If a continuation finds Finished and its ordinary disposition is unavailable,
keep the existing hold and fail closed for application. Reject/Retain remain explicit.

Financial #199 owns 31B (mixed refund scores once and holds time), 34B (ordered
financial requests) and 35A (financial Finished closure). Preserve v15 financial
hold identity, payment/basis guards and their original credited-component rules.
Do not send refunds or financial continuations through this service.
[#201](https://github.com/Kernol12/subathon-bot-ai/issues/201) retains real reversal
timer/score-deficit and milestone-policy composition.

## Atomic effects, milestone choices and SQL

Default Normal keeps permitted ordinary actions. Silent removes ordinary
message/sound/visual alert intents before staging; authorized internal effects,
score/time and ordered milestone completion persist. Require exactly one
Run/Defer/Skip choice per revalidated occurrence, no duplicates or extras.
Record achievement immediately. Stage the originating bundle, captured delivery
and milestone groups, then acceptance choices on the same active session.
Run uses the normal ordered release, Defer retains its hold, Skip retains its
achievement and captures with no release. None can lift another release guard.
Separate the original review actor from the effective apply/choice actor.
[#230](https://github.com/Kernol12/subathon-bot-ai/issues/230) owns target-edit and
completed-correction staging. Catalog and staging integration need coordinated slots.

Append one catalog step with empty review work and entry journals and terminal
request dispositions. Keep prior migration identities and row meanings. The schema
shape below uses existing strict BLOB8 unsigned and signed INTEGER codecs, typed
refs, known tags and versioned owned intent/outcome documents. Match relational
keys and document fields. Absent is SQL NULL. Prior rows gain no inferred decisions.

| Relation | Keys, references and writer invariants |
| --- | --- |
| `subathon_review_work` | PK installation-scoped work key. Immutable canonical intent and ordered row count. |
| `subathon_review_entries` | PK work key + positive ordinal, FK work. UNIQUE work key + verified CID. Nullable FK pair CID and original ReceiptId, verify the original anchor. Persist typed outcome/decision audit, expected and actual bases where known, original and selected capture refs, hold/closure refs and optional unique linked application key FK. Index CID for pending/terminal gate and enforce at most one acceptance and at most one terminal no-claim rejection per CID. On denied/unavailable selectors store disclosure-safe status and leave verified FK pair absent. |
| `subathon_review_terminal_requests` | PK application key + positive request ordinal, FK to operation/selected plan revision and its immutable request ordinal. Disposition `Finished` for time or `PostFinishPolicySuppressed` for denied non-time, deriving the exact finalized request/residual from that revision. Zero actual components and no reversal basis for these rows. |

Extend bundle prepare/read/claim/selected-basis closure for this exact catalog:
normal effect ordinals and terminal ordinals are disjoint and together cover every
immutable request exactly once. Persist the terminal descriptors with the original
claim, snapshots and actual effects. Identify partitioned bundles with a new
result format and a declared terminal count in the linked entry. The existing
effect count counts only the physical effect rows. Validate both counts, the
linked entry and provenance before any partitioned record or basis is returned.
Never substitute made-up cap, deficit or credit components. Keep job FKs on actual
eligible source rows, with existing source-less linkage only for truly independent
jobs. Preserve actual credited sibling lookup and reversal links. Old complete
bundle shapes retain unchanged validation. A recognized new-format reader validates
both partitions. Incompatible readers fail closed. A review-only patch uses a
selected full contribution comparison and changes only the review cell. Make the ordinary
apply path enforce the review journal's accepted-pending and terminal rejection
gates, including on restart. Gate on verified catalog/schema before this path.
Review documents compare explicit tags, ordered counts and elements, optionality
and raw byte refs. Never store session secrets. Validate actual known formats
and complete selected keys, count and links on read. At integration, extend
catalog dispatch only for recognized successors with the expected v15 financial
prefix and the new step where required. A later version number alone proves
neither boundary. Unknown or missing applicable schema fails closed.
From first authorship, catalog fixtures set an explicit bounded manifest
request above the measured cumulative UTF-8 SQL and repeated projection size
of the complete selected manifest. Keep the server default unchanged.

## Batch bound and completion

Maximum four selectors per preview or execution. A larger list is refused whole
before work admission. Bound total owned input and output to 1 MiB each, with checked
counts and lengths under each owning port's lower ceilings. Use one owner work and
one real BEGIN/COMMIT. Preflight all variable work before staging: meter actual
bounded discovery and compute the remaining conservative charge for the full
ordered proposal, including CAS, guarded reads, closure checks, plan append,
hold/group/job staging, retry journal and complete response. Earlier eligible rows
feed the later proposals through an in-memory projected state. Count shared work
once and bound every per-row operation including denied paths. Check actual writer
limits and cap the total at 64 SQL attempts, 64 KiB SQL text per statement,
1 MiB bound values, 1,024 result rows and 1 MiB result values, or lower configured
limits. Charge 8 bytes per SQL value plus actual UTF-8/BLOB payload. Meter
discovery before executing its next call using checked cardinalities/length bounds.
An unprovable or excessive charge refuses the whole batch as BatchTooLarge.
No split, per-row transaction, truncated hold set or widened writer limit.
For example, a proven shared charge of 4 attempts and four independently bounded
row charges of 15 attempts fit exactly 64. A row charged 16 makes 65 and refuses
the whole work. This arithmetic does not assert a per-row cost for current stores.

Expected individual ineligibility has no domain writes and later eligible rows
can proceed. Any unexpected stage conflict after preflight or writer/validation
failure rolls back all staged rows and emits no success payload. Acknowledged
COMMIT plus normal, typed work completion is the sole publication gate. Exact
replay returns history without republishing. Commit uncertainty or an exception
after commit emits no normal result and triggers no automatic retry. Reconcile
via authorized exact-key reads on the recovered writer before deciding further work.

## Independent examples

| Case | Inputs and exact oracle |
| --- | --- |
| Authority and stale preview | Account A can preview C7 under grant r1. Revoking A before execution denies it, even if token and plan are unchanged. If A still has execution but loses disclosure, history is withheld. B previews the same pending row and commits first. A's different key with the old basis cannot apply again. Same authorized key+intent returns its original result. Changed mode on that key conflicts. A plan append r1→r2 or evidence-only transition invalidates an unchanged state token. |
| Arrival and acceptance only | Receipt 8 captures a valuation-independent exact 5/2 ms, half-even request 2, residual +1/2. A later current rule yields 9 ms but the original candidate remains 2. Missing confirmation and quote allow review-only acceptance if both actually block C8. With confirmation alone still no claim. An authorized recorded quote/revision and freshly previewed ApplyAccepted may apply. If the authorized replacement request is exact 7/2, request 4 and residual −1/2. The original remains 5/2. |
| Active deficit and continuation | With fixed observed Main 99 ms/debt 7, request −100 ms and sibling +15 score micro-points, RequireReview holds the full −100 and all actual deltas are 0. With observed Main 100, equality fits: Main→0/debt 7 and score +15 under otherwise eligible policy. From the 99 hold, fresh Main 99 still retains it. At Main 120 whole settlement leaves 20/debt 7 and applies the sibling once. Independent Stop from 99 gives Main 0/debt 7/unapplied −1. Carry gives Main 0/debt 8/unapplied 0. Explicit Reject gives no effect. Legacy linked timer-only settlement leaves prior +15 unchanged. |
| Finished and reopen | At hard-end equality while paused, classify Finished before applying request +20 ms, +15 score and +1 count from score 90 and count 3. Live policy permits score only. Result retains terminal time +20 with Main/debt delta 0, score 105, count 3 and count suppression +1. Reopen with explicit Main 50/debt retained at 7 leaves Main 50/debt 7/score 105/count 3. The original claim prevents any second +20, +15 or +1. |
| Two namespaces | Actual set is ordinary h1:v2, h2:v1 and financial f1:v4. Omitting f1 or substituting v3 is stale, even when every ordinary locator matches. Omitting an ordinary closure or giving it a replacement refuses a linked stage. A permitted v15 linked stage with all exact members and both ordinary closures retains f1:v4. If f1 blocks #197 application, keep all holds and record only an independently authorized review decision. |
| Silent and ordered milestones | Score 90 +35 = 125 crosses thresholds 100, 110, 120 in order. Choose Run, Defer, Skip respectively. Record all three achievements, suppress ordinary message/sound/visual jobs, keep authorized time and internal effects, and leave ordered release to the milestone queue. A missing choice or new fourth crossing invalidates the supplied set. |
| Ordered batch and rollback | Select receipts 12 then 11 (equal arrival timestamps). With Main 95/debt 0/cap 100 and request +4 each, intake order applies +4 to 99 then +1 to 100 with 3 cap-limited on receipt 12. If receipt 11 is individually blocked in the preview and recheck, receipt 12 alone goes to 99. A post-stage failure on row 12 rolls back row 11 too, leaving Main 95 and no decision, claim, group or job from the batch. Five selectors or a projected 65th SQL attempt refuse whole. In a physical commit/ack loss control, reopened lookup may find both immutable results. No second credit or publication follows the original work failure. |
