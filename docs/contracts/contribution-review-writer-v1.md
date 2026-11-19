# Contribution review and continuation writer, revision 2

ACCT-005 [#197](https://github.com/Kernol12/subathon-bot-ai/issues/197), Tier A, Stage 1 contract candidate.
Specification v0.45 §§3–5, 8, 10. Bind [59A–62A and 64A](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7020230829)
and [63B](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7022815950), which supersedes 63A.
Preserve the [prior verified boundary](https://github.com/Kernol12/subathon-bot-ai/pull/742#issuecomment-7022781679)
outside that change. The coordinator assigns one additive step after #270. #197 and #230 have separate active
night slots because they share milestone staging. Independent contract review precedes the tests baseline.
[Coordinator claim](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7021460016)

## Owned command and result

Use the existing [application service](application-service-api-v1.md), [original claim](sqlite-original-claims-v1.md),
[bundle](sqlite-application-bundle-v1.md), [plan](sqlite-application-plans-v1.md),
[hold](sqlite-application-holds-api-v1.md), [milestone choice](sqlite-milestone-group-decisions-v1.md) and [writer
session](writer-transactions-v1.md) value types. The internal Qt owner offers `preview(ReviewPreviewRequest)`,
`execute(ReviewCommand)` and permissioned `lookup(ApplicationOperationKey)`. Single input uses the same path as a
batch of one. Preview and lookup are read-only work. These are owned DTOs, not a wire API.

| Value | Fields and invariants |
| --- | --- |
| Selection | Stable `ApplicationPrincipalBinding` and 1–16 unique contribution selectors, each CID plus original positive support ReceiptId. Resolve both against the same committed anchor. Session proof is supplied separately through the current trusted admission path. |
| Entry intent | Selector and verb `Accept`, `Reject`, `ApplyAccepted`, `ContinueTimer`, `RejectHeld` or `RetainHeld`. The latter three select an exact ordinary timer hold/version, occurrence and signed remainder. `ApplyAccepted` references the exact accepted-pending decision. |
| Entry options | Accept/ApplyAccepted carry `PausedAcceptanceMode`, optional complete ordered `MilestoneAcceptanceChoice` vector, and optional authorized plan revision proposal bound to the expected parent. Present empty choices mean zero affected occurrences. Absence means choices have yet to be supplied. ContinueTimer carries a present ordered deficit-choice vector (see below) and the apply options for an unclaimed whole input. It carries no calculation override. ContinueTimer preview may omit the deficit vector to discover the first reached ordinal. Reject and Retain have no apply options. |
| Preview row | Canonical selector, current review and application dimensions, authorized blocker and resolved-request details, expected basis below, and prospective actual amounts and ordered affected occurrences when available. Unknown amounts/occurrences stay absent. Preview denial uses the same redaction as execute. Return separate eligibility for the requested decision and its application. |
| Preview request | Selection and one entry intent per selector. Preview derives occurrences before validating supplied choices, so an initial request with absent choices can show the prospective effects and choices required. |
| Execute request | Stable work `ApplicationOperationKey`, selection, entry intents/options with server-bound preview identities/bases, nonempty actor, per-entry decision ref and reason. Actor is checked against authenticated provenance. |
| Hold closure options | For linked apply or whole-input rejection, an ordered vector of exact ordinary version locators and explicit owning decisions for additional resolvable holds. The timer selector/verb supplies its own closure. No user-provided decision is proof of resolution eligibility. |
| Row outcome | `AcceptedPending`, `Applied`, `Rejected`, `TimerResolved`, `Retained`, `Ineligible(reason)`, `Stale(reason)` or `Denied`. An unresolved selector uses disclosure-safe `Ineligible(Unavailable)`. Pending or retained carries permitted blockers/hold locators. Applied or resolved links the actual committed bundle/closure and disposition data. A denied row returns only its canonical entry ordinal and `Denied`. No verified anchor, blocker, revision, permission explanation or indication which check failed. Ineligible/stale return only authorized facts. |
| Work result | Ordered row dispositions `Committed(outcome)`, `Existing(outcome)`, `WriteFailed`, `CommitUncertain`, `CommittedResultUnavailable` or `Unattempted`. Whole-work refusals are `InvalidSelection`, `Denied`, `IdentityConflict`, `BatchTooLarge` and `Unavailable`. Writer outcome/error stays separate and only disclosure-safe classifications enter the result. |

Use committed original support ReceiptId order (unsigned ascending) for preview, execution, persisted entry ordinals
and response. Receipt allocation is durable intake order. A confirmation ID, source time, clock time, client list
order and duplicate intake attempt cannot change it. Duplicate selectors or conflicting anchors invalidate the
request. An unresolved but well-formed selector yields individual unavailability with disclosure-safe data. Receipt
intake precedes the review transaction. A batched execution cannot supply another unpreviewed row.

## Preview, authority and freshness

Preview simulates the selected verbs/options in intake order. Later rows see the effects of the projected eligible
prefix, including consumed cap headroom and milestones. Return a server-produced expected basis per row bound to the
complete selection and intents, stable principal, original receipt/CID, actual plan head, all current contribution
fields, application state token, relevant authority and policy/configuration revisions, complete mixed hold versions
and projected prefix. Use stable proposed occurrence identities bound to the original selector, target revision and
crossing order, with authoritative durable occurrence refs allocated and checked on stage. The basis binds the
projected application and occurrence set. An initially absent choice set can be filled for exactly the previewed
occurrences. Bind a canonical Normal or Silent mode to the preview. Any mode change requires a fresh preview and
explicit audited execute intent. Silent requires its current grant. Preview writes no review, plan, hold, clock,
achievement, job or claim. Preview rejection shows what would be rejected, including any prospective application
suppressed by it. Bind the starting token and projected predecessor identities to the preview. Recompute on stage.

Under the owner turn, check current session and execution authority before protected lookups and check disclosure
before returning any selected data. Recheck effective grants for each actual subject, capability, resource and
quantity, including any configured batch scope. Apply §10 complete-grant and deny precedence. Review, silent
application, override, timer continuation and milestone choices require their applicable current grants. Approval
requires neither blanket permission over every row nor joining constraints from different grants to approve one use.
Unavailable authority fails closed. A request with invalid session or batch authority has no individual protected
outcomes. A per-row denial does not approve other work on that subject. Reading a past result has its own current
disclosure check.

Resolve the live basis in the selected row's transaction. Recheck session and batch/key authority at every turn.
Loss of that authority stops forwarding. Check same-row execution permissions first, then compare actual
fields and provenance, including plan/head, contribution, complete mixed hold set and policy/permission revisions.
Use the actual committed predecessor outcomes and current state/head to reobserve clock and compute exact cap, score
and milestones. A matched prefix commit can account for the head change it produced. Its token is never authority
for other changes. An unrelated review-only decision does not stale a later row. Material prefix drift is stale even
with the same token. Clock passage can update estimates within current grants if branch, blockers and choice set
remain unchanged. Changes to those require fresh preview. Ready or unknown lifecycle/policy/clock blocks application.

## Canonical work and durable prefix

Use an installation-scoped stable work key in a reserved review namespace disjoint from ordinary application,
correction and financial keys. Require checked operation scope/ref and at most 256 encoded key bytes.
The immutable manifest encodes magic `SRW1`, installation ref, operation scope/ref, principal kind/ref, actor, row
count, then entry records in canonical ordinal order. Each entry encodes CID, ReceiptId, verb, full expected preview
basis, tagged verb options, decision ref and reason, in that order. Use checked integer/ref codecs, u32 big-endian lengths/counts and
explicit presence tags for all optional fields. UTF-8 text is exact, without normalization. Ordered vectors retain order and expected sets
use their owning canonical order. Require exact EOF and reject malformed or duplicate values. Exclude session proofs and newly
observed outcomes. Compare complete canonical bytes, including absent versus empty, before using a stored cursor.

Authenticate and check current execution scope for the installation/key before revealing existence or conflict.
Changed intent on an existing key refuses the whole request. Exact retry checks current session, batch and key
authority. For each stored outcome, recheck current execution grants against its stored scope/actual quantities
and current disclosure for its same-row facts before returning it. Read prefix before live preview checks on the suffix.
Stored refusal remains terminal even if eligibility later changes. A denied row stores only an ordinal/status and
caller-supplied intent, with protected columns absent. A redacted row exposes only ordinal and `Denied`, including
when a formerly disclosable outcome can no longer be returned. History lookup is read-only and never resumes work.

The first terminal row creates the manifest and advances the cursor atomically. A failed first row leaves no key
after known rollback. Cursor is the next ordinal, initially 1, with count+1 complete. Require an exact contiguous
immutable terminal prefix [1, cursor), matching manifest, anchors when verified and linked effects. Append only at
cursor. CAS its old value and check a single change in the same transaction as the terminal entry and any domain
writes. Broken prefix/links fail closed. Each applied entry has a distinct checked immutable bundle key linked to
work key and ordinal. Reserve IDs before staging with no alias of any previous bundle, operation, occurrence,
decision or hold. Keep existing claim uniqueness, review/plan/state guards and terminal rejection gates across keys.

## Decisions, blockers and lifecycle

- Accept or Reject requires ordinary review pending and no original award. Reject patches ordinary review to rejected
  and records the decision. Accept patches it to accepted. Neither patch rewrites the evidence-review decision, live
  evidence, valuation, application, reversal, settlement or original capture. Keep independent blockers intact on
  Reject. A terminal rejection prevents later application on another key. Reversal of such a decision needs its own
  authority outside this contract. Withdrawal of an accepted-pending decision is also outside this contract.
- Accept-and-apply stages the ordinary review patch first. Pass the resulting full current contribution snapshot to
  bundle staging, which independently compares it and patches only application. A failure after either patch rolls
  both back.
- Accept may be recorded while a known independent blocker (payment, valuation, settlement, recovery or unrelated
  hold, for example) prevents application. Record `AcceptedPending`, including mode, supplied choices or their
  absence, actor, original context and blockers. Make no claim, effect, milestone, job or application clock/head
  change. Approval admits the ordinary review dimension only. Missing choices that are required for an otherwise
  eligible application require a corrected preview/intent. A pending acceptance never queues replay.
- A subsequent `ApplyAccepted` requires the pending decision, a new key, fresh preview and current execution grants
  for its own actor, options and actions. It keeps the accepted mode unless a new explicitly authorized audited change
  is recorded. Revalidate every choice against the newly projected occurrences. Record both decisions and actors.
  Confirmation, rate entry, pause release, recovery or reopen cannot perform this apply on their own. Paused
  acceptance leaves each pause reason intact and cannot override unresolved recovery.
- Select original arrival rules, modifier context, quote and provenance from the actual capture. Preserve source
  facts, optional unknowns and prior revisions. An authorized replacement appends the immediate child revision using
  the actual plan-head CAS and retains actor, decision, reason and exact changed selections, valuation and requests.
  It shares the unit with the decision using it. Authorization precedes this append. A rejected, stale or denied row
  never revises a plan. A missing historical rate requires the owning producer's explicit retained resolution. Current
  rules or rates supply no implicit replacement.
- For a started timer, observe current authoritative clock/lifecycle first, including hard end at equality through
  pause, grace or confirmation wait. Active signed original requests require the owning CORE-008 derivation and
  authority. A RequireReview deficit on any timer occurrence defers the **whole** original, before original claim or
  any effect, job or achievement. Retain the captured plan, exact full signed blocked request, first blocking plan
  ordinal, exact active hold/version locator and basis in a `defer_whole_input` timer hold. Disclose the locator,
  signed request and ordinal only with current hold disclosure. Other evaluated deltas are speculative. A review
  decision can still be accepted-pending in this same unit. Ordinary numeric overflow retains the existing whole-input
  overflow behavior and reason.
- In Finished, record every time request with terminal disposition `Finished` and zero Main/debt deltas. Retain its
  full signed request and exact provenance. Give siblings only the application effects permitted by current
  post-finish policy. Record a lifecycle-policy suppression for each denied request, including a denied score/count
  request. No cap-limited amount, timer hold, debt creation or latent time replay. A terminal original consumes its
  original claim even if all requests are suppressed. Keep actual lifecycle transitions separate from the effect
  deltas. [#183](https://github.com/Kernol12/subathon-bot-ai/issues/183) supplies live Finished and explicit reopen
  policy. Missing current independent lifecycle, sibling, milestone or release policy required by the proposal blocks
  application. The non-review original route remains fail closed in Finished pending separate integration. Reopen
  preserves history, score, achievements, debt and independent holds under its owning policy. Only its explicitly
  selected pending action handling can affect genuinely pending groups.

## Complete hold set and ordinary continuation

Read the selected ordinary and financial active sets for the actual original subject. Compare the complete union in
v15 raw hold-ref order, with exact versions and no duplicate locators. Any malformed member fails the unit. Presence
of a financial hold is never execution authority. Ordinary linked application uses
`stage_financial_aware_linked_resolution`, closes **all** active ordinary members with exact versions and explicit
authorized decisions, has no replacements, and preserves all financial holds. Missing/stale members or an unresolved
ordinary blocker prevent linked application. Apply through a preserved financial hold only if the owning policy
independently establishes it is no blocker. Review-only acceptance and unlinked authorized rejection may coexist with
foreign holds and leave them intact. Additional ordinary closures require an explicit entry for each actual version
and checked resolution from its owning authority. Compare the mixed set before projecting the ordinary entries into
the existing v15 staging API. Bind the full expected set into review audit, including preserved financial locators.

ContinueTimer selects only an unclaimed `defer_whole_input` hold by its exact locator, original blocked ordinal and
full signed remainder. Require accepted or unnecessary ordinary review and link the pending decision when present.
Re-evaluate the entire retained request sequence in plan order against the fresh observed state. Preview walks the
proposal using supplied deficit decisions in order. At the first reached deficit lacking a decision, expose that
ordinal and its exact signed request and stop the projection there. A subsequent preview can advance farther using the
preceding choices. Execute requires the complete fresh preview and exact ordered `(ordinal, signed request, StopAtZero
| CarryTimeDebt)` choices for every reached deficit, including a present empty vector when none is reached. Stale,
omitted, duplicate or extra choices make the row ineligible or stale without any domain write. Explicit RetainHeld is
the way to retain with audit. A changed live default cannot silently choose Stop or Carry. Stop/Carry settles each
whole request with no pending residual. For negative requests require `request = MainDelta - DebtDelta + unapplied`
with checked exact arithmetic.

If Finished is observed, apply the same whole-original terminal partition and live independent sibling policy as for
fresh review. No Active deficit choice is then applicable. Missing current policy leaves the hold active and the
original unclaimed. Successful continuation consumes the single original claim and closes the complete ordinary set
with the original bundle link and exact owning decisions. RejectHeld on a whole input requires exact versions and
explicit decisions for the complete ordinary set, closes it without a bundle and records terminal no-claim rejection
of the original. Neither case modifies financial holds. RetainHeld journals its decision and leaves every hold active.
For a beside-complete hold, only explicit RejectHeld or RetainHeld is supported. RejectHeld closes its selected exact
ordinary version without changing the existing claim or effects. RetainHeld preserves it. No sibling is replayed.

Financial #199 owns 31B (mixed refund scores once and holds time), 34B (ordered financial requests) and 35A (financial
Finished closure). Preserve v15 financial hold identity, payment/basis guards and their original credited-component
rules. Do not send refunds or financial continuations through this service.
[#201](https://github.com/Kernol12/subathon-bot-ai/issues/201) retains real reversal timer/score-deficit and
milestone-policy composition.

## Atomic effects, milestone choices and SQL

Default Normal keeps permitted ordinary actions. Silent removes ordinary message/sound/visual alert intents before
staging; authorized internal effects, score/time and ordered milestone completion persist. Require a present ordered
vector with exactly one Run/Defer/Skip choice per revalidated occurrence, no duplicates or extras. An explicit empty
vector is valid only for zero crossings. Absent choices in an otherwise eligible apply write no domain state. Record
achievement immediately. Stage the originating bundle, captured delivery and milestone groups, then acceptance choices
on the same active session. Run uses the normal ordered release, Defer retains its hold, Skip retains its achievement
and captures with no release. None can lift another release guard. Separate the original review actor from the
effective apply/choice actor. [#230](https://github.com/Kernol12/subathon-bot-ai/issues/230) owns target-edit and
completed-correction staging. Require current milestone and delivery guard policies before applying any effect that
depends on them.

Append one catalog step with empty review work and entry journals and terminal request dispositions. Keep prior
migration identities and row meanings. The schema shape below uses existing strict BLOB8 unsigned and signed INTEGER
codecs, typed refs, known tags and versioned owned intent/outcome documents. Match relational keys and document
fields. Absent is SQL NULL. Prior rows gain no inferred decisions.

| Relation | Keys, references and writer invariants |
| --- | --- |
| `subathon_review_work` | PK installation-scoped work key. Immutable canonical manifest and row count, checked next-ordinal cursor. |
| `subathon_review_entries` | PK work key + positive ordinal, FK work. UNIQUE work key + verified CID. Nullable FK pair CID and original ReceiptId, verify the original anchor. Append immutable typed terminal outcome/decision audit, expected and actual bases where known, original and selected capture refs, hold/closure refs and optional unique linked application key FK. Index CID for pending/terminal gate and enforce at most one acceptance and at most one terminal no-claim rejection per CID. On denied/unavailable selectors store disclosure-safe status and leave verified FK pair absent. |
| `subathon_review_terminal_requests` | PK application key + positive request ordinal, FK to operation/selected plan revision and its immutable request ordinal. Disposition `Finished` for time or `PostFinishPolicySuppressed` for denied non-time, deriving the exact finalized request/residual from that revision. Zero actual components and no reversal basis for these rows. |

Use result format `subathon.internal.application-result.v2` only for terminal partitions. Its document is literal
`SAR2`, the existing SAR1 field sequence after its magic (command, tokens, revalidation flag, actual-effect vector,
jobs and transitions, identical codecs), then big-endian u32 immutable request count, u32 terminal count and exactly
request-count one-byte tags in plan ordinal order. Tag 0 = physical effect, 1 = Finished time, 2 = policy-suppressed
non-time. Require positive terminal count. The mask and count live on the result keyed by bundle, independent of the
review journal and its links. Require EOF, known tags, matching plan count and exact correspondence among the mask,
decoded effect vector, physical effect count and rows, terminal count and rows. Every ordinal is covered once.
Terminal rows bind the selected revision and ordinal, with no component, job source, reversal/progress or refund
basis. Validate signed request and residual from the retained revision. Source-less jobs require independent
provenance.

Apply closure at bundle prepare/stage and `lookup`, `lookup_original_claim`,
`selected_record_read::read_application`/SelectedMetadata, `read_reversal_basis` and internal basis/reversal
traversal, and hold resolution link validation and selected hold/history reads. A terminal ordinal has zero actual
credited units and yields an empty `ApplicationReversalBasisResult::basis` after valid closure. It cannot produce a
reversal or a #199 credit link. Preserve existing basis objects and actual credited sibling components. #199 refund
prepare/read, time-credit and direct financial basis selection must join by the original request ordinal, including
replacing dense effect indexing in refund preparation. Milestone origin/enrollment and reopen reads must validate
partitioned provenance before using an origin while keeping completed occurrences and prior release actions. Keep
historical full bundle shapes and strict legacy validators. Old readers fail closed on unknown format.

Review-only compare checks the full contribution and patches only ordinary review. Ordinary apply enforces the
accepted-pending and terminal rejection gates across restart. Gate integration on exact catalog, schema and immutable
history prefix: v15 `financial-refunds-v1`, #270 v16 `provider-credentials-v1`, then the coordinator-assigned step.
Verify exact successor IDs in `milestone_detail::catalog`'s milestone selector, `financial_sql::selected` and
`ModifierRows`, preserving prior branches. Reject unknown or lookalike markers. Compare review document tags, counts,
order, raw refs, option presence and selected links. Store no session secrets. Fixture requests set an explicit bounded
`MigrationRequest.limits.manifest_utf8_bytes` above the measured cumulative UTF-8 of all SQL and repeated
type/name/table/SQL projections of the complete selected manifest. Measure after step assignment and retain the
exact count and explicit limit. This request budget is separate from the writer's SQL budgets.

## Bounds and admission

Maximum 16 rows, with the same bound for preview, execute and lookup output. Owned request and response each cap
at 1 MiB, with at most 4 KiB of envelope and 60 KiB per encoded entry including its basis or outcome (987,136 bytes
at 16 rows). Charge owned input at ingress before copying/canonicalizing past the limit. Before any writes, bound
the entire selection, manifest, persisted/result documents and final reply, including denied/stop/suffix cases.
Use checked addition and bounded length/count probes, with their own charges, before materializing stored data.
Check duplicate/conflicting anchors, identities and all row shapes in read-only preflight. Refuse invalid, oversize
or unproven requests without starting journal or domain writes. Recheck each live shape before staging. If facts
drift after preflight, record stale only when established within the row budget, otherwise stop with a row failure.

Use one explicitly configured existing writer. Candidate profile per unit: 768 attempts, 64 KiB per SQL statement,
9 MiB binding bytes, 768 result rows, 16 MiB result values. Reserve 128 attempts/rows shared plus 384 for a single
row work (128 discovery/proposal, 256 staging/validation), or 40 per ordinal for a compact 16-row reconciliation
(128 + 16 × 40 = 768). Allow one initial read, at most one qualification read per row and one final reconciliation
read, for at most 34 works including row writes. Bound both count dimensions independently. All ports charge before use, including authority/policy,
sentinels, duplicate projections, plan append, mixed hold closure, terminal partition, milestones, jobs and
stage/readback. Per attempt/row, reserve 32 binding/104 result cells, each 8 overhead + 128 payload bytes, and
4,096 additional payload bytes. Add at most 2 MiB in each direction per unit for repeated bounded manifest/journal
document payload, with cell overhead still charged. The largest ceiling is 768 × 8,448 + 2 MiB = 8,585,216
binding bytes and 768 × 18,240 + 2 MiB = 16,105,472 result bytes. These are finite design ceilings.

Prior trace for one ordinary apply: inputs 35 + stage guards 37 + stage writes 12–14 = 84–86 attempts, plus 52
core readback = 136–138, before authority, policy, holds and journaling. Review-only local trace is 47 before
shared and authority/policy work. The current 64-attempt default cannot admit the traced apply. The explicit
development profile fits on paper. Use actual configured-writer measurements as tests gates for per-row
stage/readback, real authority, reconciliation and preflight, review-only/apply/mixed, supported partition/hold/
milestone shapes, exact maxima and one-over. [Host #294 capacity handoff](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-7022815339)
requires measured compatibility too. Production settings stay unchanged.

## Owner execution, stop and reconciliation

One selected contribution per real BEGIN/COMMIT unit. A checked ineligible, stale, unavailable or denied row is a
terminal journal decision with no domain writes. Commit it and advance the cursor even if every row is refused.
Issue only the next row from a normal committed completion. No prequeue of row writes, open transaction between
rows, event pumping or extra writer. Keep one active review execution on the owner from preflight through completion.
Concurrent component executions fail with a safe busy/unavailable result without staging. The shipped host keeps
one active command across completion forwarding, so other host commands cannot enter between rows. Other work can
run only where the owning FIFO permits it. A later row observes any such committed changes through its live guards.
Each unit rechecks stable intent and cursor, so neither a retry nor a competing intent can skip, reorder or reapply
a terminal row. Distinct keys have independent intake-ordered selections and share the contribution/claim guards.

Any write, unexpected stage conflict, readback, commit or abnormal completion failure stops forwarding immediately. A known
uncommitted current row is `WriteFailed`; an unknown outcome is `CommitUncertain` with a null durable outcome;
a confirmed commit with abnormal completion is `CommittedResultUnavailable` until reconciled. The earlier prefix
survives. Report the current ordinal and `Unattempted` suffix without asserting absent or zero effects. A refused unit
admission stops too, with no current transaction or host reply. Failure markers are attempt observations and never terminal
journal entries. Arithmetical/domain ineligibility classified before effect staging is a terminal decision.

Build protected responses only from a reconciled committed journal, with current same-row authority and disclosure
in a bounded final read on the owner. Validate the compact journal and its links without replaying bundle staging.
Require normal completion of the matched read and current disclosure at owner handoff before emission. A normal
row commit can release its actual effects once through the owning publication path. History reads never republish.
If current authority or storage cannot be established, withhold protected results, including cached prefix rows,
and use only a safe stop report or whole-work Unavailable. Unknown COMMIT fail-closes the writer. Do not resume on
it, infer rollback, or retry automatically. On a recovered authorized owner, reconcile the exact key and manifest,
cursor and any current entry together. Present and valid means committed. Absence proves no row commit only from
a validated recovered current store with matching prefix (or no manifest for a first-row attempt). A mismatch or
unproved recovery remains unavailable. An authorized exact retry then returns the prefix and starts at the first
missing ordinal. The [shipped host reply gate](api-host-lifecycle-boundary-v1.md#host-reply-eligibility-after-prepared-units)
withholds transport replies after unknown outcome or abnormal committed completion. The component stop status does
not clear that latch. A fresh authorized request may obtain the reconciled response.
