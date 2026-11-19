# Contribution review and continuation writer, revision 1

ACCT-005 [#197](https://github.com/Kernol12/subathon-bot-ai/issues/197), Tier A, Stage 1 contract candidate.
Specification v0.45 §§3–5, 8, 10. Bind [owner decisions
59A–64A](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7020230829) and [coordinator
claim](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7021460016). Design base
`8056048c238bdbd7c208eac11117f9b73b04097f`, tree `29ef6bff2909acfd2847502070b69b140cb906d5`, protected v15 through
#199. The coordinator assigns one additive step after #270. #197 and #230 require separate active night slots because
they share milestone staging. Independent contract review precedes freezing examples and authoring the baseline.

## Owned command and result

Use the existing [application service](application-service-api-v1.md), [original claim](sqlite-original-claims-v1.md),
[bundle](sqlite-application-bundle-v1.md), [plan](sqlite-application-plans-v1.md),
[hold](sqlite-application-holds-api-v1.md), [milestone choice](sqlite-milestone-group-decisions-v1.md) and [writer
session](writer-transactions-v1.md) value types. The internal Qt owner offers `preview(ReviewPreviewRequest)`,
`execute(ReviewCommand)` and permissioned `lookup(ApplicationOperationKey)`. Single input uses the same path as a
batch of one. Preview and lookup are read-only work. These are owned DTOs, not a wire API.

| Value | Fields and invariants |
| --- | --- |
| Selection | Stable `ApplicationPrincipalBinding` and 1–2 unique contribution selectors, each CID plus original positive support ReceiptId. Resolve both against the same committed anchor. Session proof is supplied separately through the current trusted admission path. |
| Entry intent | Selector and verb `Accept`, `Reject`, `ApplyAccepted`, `ContinueTimer`, `RejectHeld` or `RetainHeld`. The latter three select an exact ordinary timer hold/version, occurrence and signed remainder. `ApplyAccepted` references the exact accepted-pending decision. |
| Entry options | Accept/ApplyAccepted carry `PausedAcceptanceMode`, optional complete ordered `MilestoneAcceptanceChoice` vector, and optional authorized plan revision proposal bound to the expected parent. Present empty choices mean zero affected occurrences. Absence means choices have yet to be supplied. ContinueTimer carries a present ordered deficit-choice vector (see below) and the apply options for an unclaimed whole input. It carries no calculation override. ContinueTimer preview may omit the deficit vector to discover the first reached ordinal. Reject and Retain have no apply options. |
| Preview row | Canonical selector, current review and application dimensions, authorized blocker and resolved-request details, expected basis below, and prospective actual amounts and ordered affected occurrences when available. Unknown amounts/occurrences stay absent. Preview denial uses the same redaction as execute. Return separate eligibility for the requested decision and its application. |
| Preview request | Selection and one entry intent per selector. Preview derives occurrences before validating supplied choices, so an initial request with absent choices can show the prospective effects and choices required. |
| Execute request | Stable work `ApplicationOperationKey`, selection, entry intents/options with preview bases, nonempty actor, per-entry decision ref and reason. Actor is checked against authenticated provenance. |
| Hold closure options | For linked apply or whole-input rejection, an ordered vector of exact ordinary version locators and explicit owning decisions for additional resolvable holds. The timer selector/verb supplies its own closure. No user-provided decision is proof of resolution eligibility. |
| Row outcome | `AcceptedPending`, `Applied`, `Rejected`, `TimerResolved`, `Retained`, `Ineligible(reason)`, `Stale(reason)` or `Denied`. Pending or retained carries permitted blockers/hold locators. Applied or resolved links the actual committed bundle/closure and disposition data. A denied row returns only its canonical entry ordinal and `Denied`. No verified anchor, blocker, revision, permission explanation or indication which check failed. Ineligible/stale return only authorized facts. |
| Work result | Ordered complete row outcomes, `Existing` with the immutable authorized result, or a whole-work refusal (`InvalidSelection`, `Denied`, `IdentityConflict`, `BatchTooLarge`, `Unavailable`). Writer outcome/error stays separate. |

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
suppressed by it. Use the persisted starting token for freshness. Validate later rows against the projected preceding
state and stage them against actual preceding in-unit tokens.

Under the owner turn, check current session and execution authority before protected lookups and check disclosure
before returning any selected data. Recheck effective grants for each actual subject, capability, resource and
quantity, including any configured batch scope. Apply §10 complete-grant and deny precedence. Review, silent
application, override, timer continuation and milestone choices require their applicable current grants. Approval
requires neither blanket permission over every row nor joining constraints from different grants to approve one use.
Unavailable authority fails closed. A request with invalid session or batch authority has no individual protected
outcomes. A per-row denial does not approve other work on that subject. Reading a past result has its own current
disclosure check.

Resolve the live basis inside the same transaction. A changed plan/head, any contribution cell, complete hold set,
applicable policy/permission revision or material prefix is stale even with an unchanged state token. Check current
permission first so revoked scope returns denial with no facts. Compare actual fields, never a digest alone. An
earlier row changing only an unrelated decision does not stale a later row. Reobserve the authoritative clock and
recompute arithmetic over the in-unit state. Clock passage alone can update preview estimates if still within current
grants and the eligible branch, blockers and identified milestone choices are unchanged. A changed branch, blocker or
choice set requires a fresh preview. Disclose actual amounts, including observed lifecycle transitions, with the
result. Treat Ready or unknown lifecycle/policy/clock as an application prerequisite.

Bind the stable work key to the complete canonical intent, including ordered subjects, verbs, expectations, options,
actor/decision/reason and stable principal. Exclude transient session credentials and newly observed outcomes.
Existing authorized exact retry returns the original journal outcome before current-basis comparisons. Different
immutable intent is a conflict. Recheck execution authority against the stored scope/quantities and disclosure for the
complete result. Return denial or disclosure-safe refusal if that result cannot be returned. Authenticate before
revealing key existence and require authority for that installation/key scope. After preflight, journal admitted work
and every ordinal even if all outcomes are ineligible, stale or denied. A read-only preflight is not admission.
Whole-work refusal writes neither journal nor domain rows. Journal only caller-supplied intent and disclosure-safe
status for a denied row. Exact retry never upgrades the stored outcome or exposes redacted facts. Reserve a review-key
namespace by installation, disjoint from the ordinary application, correction and financial operation namespaces. Each
applied entry has its own distinct immutable bundle key, linked to the work key and ordinal. Reserve checked
identifiers before staging. Never alias a previous bundle, operation, occurrence, decision or hold.

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
| `subathon_review_work` | PK installation-scoped work key. Immutable canonical intent and ordered row count. |
| `subathon_review_entries` | PK work key + positive ordinal, FK work. UNIQUE work key + verified CID. Nullable FK pair CID and original ReceiptId, verify the original anchor. Persist typed outcome/decision audit, expected and actual bases where known, original and selected capture refs, hold/closure refs and optional unique linked application key FK. Index CID for pending/terminal gate and enforce at most one acceptance and at most one terminal no-claim rejection per CID. On denied/unavailable selectors store disclosure-safe status and leave verified FK pair absent. |
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
order, raw refs, option presence and selected links. Store no session secrets. Fixture requests set explicit
`MigrationRequest.limits.manifest_utf8_bytes` above the counted UTF-8 of all SQL and repeated type/name/table/SQL
projections through the selected manifest. Keep that cumulative request budget separate from the writer's SQL budgets.

## Batch bound and completion

Maximum two selectors. Use one work on the existing owner writer and one real BEGIN/COMMIT. Refuse oversize in full,
including a valid selection whose checked shape exceeds the available budget. Candidate trusted writer profile for
this entry point: at least 896 attempts, 64 KiB per statement, 8 MiB bindings, 896 SQL result rows and 16 MiB SQL
result values. This fits inside the explicit development writer profile. A default writer does not qualify. Production
activation requires a verified explicitly configured profile on its single owner writer. Keep the production defaults.
Bound owned intent and response to 1 MiB each.

Source trace on the committed catalog path: `OriginalApplicationService` → `Execute::run` →
`SqliteApplicationBundleStore::read_inputs/stage/lookup` → `Access` in `sqlite_application_bundle_access.hpp`.
Representative contribution: one linked configuration/arrival, confirmed support, revision 1, one rule, no modifiers,
one positive time request, three effect components, no balances, jobs, transitions, reversals, hold history or active
financial holds. Allow two new state snapshots. Review-only reads the same 35-attempt input shape with a known
independent blocker such as pending settlement. Its local subtotal is 47 attempts (35 inputs, eight claim and hold reads, four review attempts),
plus shared and authority/policy work. Both read a fresh claim miss (2), empty ordinary hold history (4) and empty
financial active set (2 including its catalog selector). Review-only has no bundle stage/readback. An independently
proposed review CAS + changes check, decision gate read, and journal entry add four attempts per row, with work-key
lookup and work journal charged once. Existing apply charge on this shape is:

| Port | SQL attempts |
| --- | ---: |
| Inputs | 35 = plan 15 + current 5 + contribution 10 + explicit support observation 5 |
| Stage guards | 37 = catalog/namespace/claim lookups 5 + current 5 + contribution 10 + plan 15 + two token probes |
| Stage writes and head confirmation | 14 = two states + operation/contribution snapshot/claim/application patch/result/audit/head CAS/changes (8) + effect and three components (4) |
| Readback | 52 = fixed 34 (operation 1, states 12, plan 15, remaining anchors 6) + children 17 (five count guards, effect scan/count, effect/component reads, component count, jobs scan/count and two guards, transitions scan/count, reversal count) + reversal scan 1 |

Plan 15 includes header/source+anchor 7, history summary/page/dependency 3, selected scalar/dependency 2 and three
child queries. Contribution 10 includes the row, retained receipt (4) and anchored observation (5). The default 64
attempts cannot fit even one apply: inputs and stage alone take 86 (84 if the advanced snapshot is reused). With
readback they take 138 at the two-snapshot bound, before authority, policy, clock, hold reads and journal work. Valid
core result rows are 33 for inputs, 29 for stage guards, one for changes and 47 for readback (110 total).
The shipped adapter adds 26 retained calls: access 6, policy 7, clock 1 and actions 12 (policy 7 again plus binding 5).
The original service also probes the missing operation once before inputs. The SQL authority's capture/permission
closure and the new review ports require separate charges within the reserved slots.

For the candidate profile reserve 128 attempts and result rows for shared work including catalog, session, key probes
and journal, and 384 each per row, including up to 128 for discovery/proposal and 256 for stage and validation. Meter
32 binding cells per attempt and 104 result cells per row (the traced 34-field revision projects 102 cells). Charge
eight bytes per cell plus up to 128 payload bytes per cell and 4,096 additional payload bytes per statement's bindings
or per result row. At 128 + 2 × 384 = 896 units, binding charge is 896 × (32 × 136 + 4,096) = 7,569,408 bytes
and output charge is 896 × (104 × 136 + 4,096) = 16,343,040 bytes, including guards and readback. These are design ceilings. Verify widths and fanout from
narrow count/length probes before materializing stored data. Reserve and charge probes too, including sentinel rows
and repeated projections.

Before staging, account for spent discovery and prove the remaining bounded calls, bindings, outputs and final
response for the full projected sequence. Include authority/denial/lookup, plan append, namespaces, hold closure,
terminal, milestone and job guards and writes. Every dependent port must declare and enforce that bound on the same
session, including readback. Unlimited or unproven fanout refuses as BatchTooLarge before journal or domain staging.
Meter each call before executing it. Required verification on the real writer: review-only, representative apply, both
in one unit, boundary/one-over dimension, and additional supported shape costs with actual policy/authority ports.
Partition/hold/milestone paths need independent charges within this profile before admission. Source analysis
establishes a candidate ceiling.

Expected individual ineligibility has no domain writes and later eligible rows can proceed. Any unexpected stage
conflict after preflight or writer/validation failure rolls back all staged rows and emits no success payload.
Acknowledged COMMIT plus normal, typed work completion is the sole publication gate. Exact replay returns history
without republishing. Commit uncertainty or an exception after commit emits no normal result and triggers no automatic
retry. Reconcile via authorized exact-key reads on the recovered writer before deciding further work.

## Independent examples

| Case | Inputs and exact oracle |
| --- | --- |
| Authority and stale preview | Account A can preview C7 under grant r1. Revoking A before execution denies it, even if token and plan are unchanged. If A still has execution but loses disclosure, history is withheld. B previews the same pending row and commits first. A's different key with the old basis cannot apply again. Same authorized key+intent returns its original result. Changed mode on that key conflicts. A plan append r1→r2 or evidence-only transition invalidates an unchanged state token. |
| Arrival and acceptance only | Receipt 8 captures a valuation-independent exact 5/2 ms, half-even request 2, residual +1/2. A later current rule yields 9 ms but the original candidate remains 2. Missing confirmation and quote allow review-only acceptance if both actually block C8. With confirmation alone still no claim. An authorized recorded quote/revision and freshly previewed ApplyAccepted may apply. If the authorized replacement request is exact 7/2, request 4 and residual −1/2. The original remains 5/2. |
| Active deficit and continuation | With a fixed clock (no elapsed advance), observed Main 99 ms/debt 7, request −100 ms and sibling +15 score micro-points, RequireReview holds the full −100 and all actual deltas are 0. With observed Main 100, equality fits: Main→0/debt 7 and score +15 under otherwise eligible policy. From the 99 hold, fresh Main 99 with no choices makes no domain write. Explicit RetainHeld journals retention. At Main 120 whole settlement leaves 20/debt 7 and applies the sibling once. Independent Stop from 99 gives Main 0/debt 7/unapplied −1. Carry gives Main 0/debt 8/unapplied 0. Explicit RejectHeld gives no effect. Beside-complete RejectHeld/RetainHeld preserve prior +15. For requests −100 then −2 from Main 99/debt 7, first choose Carry for ordinal 1 (Main 0/debt 8), then preview ordinal 2 and explicitly choose Stop (Main 0/debt 8/unapplied −2 at ordinal 2). Supplying only the first choice changes nothing. |
| Finished and reopen | At fixed hard-end equality while paused with debt 7, classify Finished before applying request +20 ms, +15 score and +1 count from score 90 and count 3. Live policy permits score only. Result retains terminal time +20 with Main/debt delta 0, score 105, count 3 and count suppression +1, mask [1,0,2], terminal count 2. Continuing an exact unclaimed whole-input hold at the same Finished observation uses this partition and closes its ordinary holds. Without current sibling policy, keep the hold and do not claim. After resolving the passed hard end, authorized reopen with Main 50, retained grace and debt, and Keep deferred leaves Main 50/debt 7/score 105/count 3. The original claim prevents any second +20, +15 or +1. |
| Two namespaces | Actual set is ordinary h1:v2, h2:v1 and financial f1:v4. Omitting f1 or substituting v3 is stale, even when every ordinary locator matches. Omitting an ordinary closure or giving it a replacement refuses a linked stage. A permitted v15 linked stage with all exact members and both ordinary closures retains f1:v4. If f1 blocks #197 application, keep all holds and record only an independently authorized review decision. |
| Silent and ordered milestones | Score 90 +35 = 125 crosses thresholds 100, 110, 120 in order. Choose Run, Defer, Skip respectively. Record all three achievements, suppress ordinary message/sound/visual jobs, keep authorized time and internal effects, and leave ordered release to the milestone queue. A missing choice or new fourth crossing invalidates the supplied set. |
| Ordered batch and rollback | Select receipts 12 then 11 (equal arrival timestamps). With a fixed clock and Main 95/debt 0/cap 100 and request +4 each, intake order applies +4 to 99 then +1 to 100 with 3 cap-limited on receipt 12. If receipt 11 is individually blocked in the preview and recheck, receipt 12 alone goes to 99. A post-stage failure on row 12 rolls back row 11 too, leaving Main 95 and no decision, claim, group or job from the batch. Three selectors or a projected charge beyond the profile refuse whole. Two individually stale rows commit only the two journal outcomes. In a physical commit/ack loss control, reopened lookup may find both immutable results. No second credit or publication follows the original work failure. |
