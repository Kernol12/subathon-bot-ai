# Original application service — logical proposal revision 1

Task: ACCTCONTRACT-001, prerequisite to ACCTTEST-004 and later ACCT-004. Authority: specification v0.45 §§3–5,
NUM-01–04, DB-01 and SERVER-01; [event/state boundaries](event-state-boundaries-v1.md)
B03/B04/B07/B08/B10; [application bundle](application-transaction-bundle-v1.md)
AT01–21; [concrete SQLite bundle](sqlite-application-bundle-v1.md).
This document proposes logical operations and independent examples ONLY. It supplies
no C++ interface, stub, executable fixture/test, physical schema or implementation.
Different actual review precedes those artifacts and their separate review/red gates.

## Purpose and scope

One authoritative Qt owner applies ONE previously captured original operation.
It rechecks current access and eligibility, advances the clock observation, resolves
actual time/debt/counter effects, and owns one real writer transaction containing
all successful effects, state, application claim/status, required job intents,
result and audit. Neither a connector nor a transport/client owns this authority.
The service never calls network providers, delivers a job or waits for another
queued command. All external transport and actual access-policy implementations
remain their separate required tasks.

Original means `ApplicationOperationKind::original`, not necessarily a positive
payment. It includes already-authorized signed original corrections where their
own command contract supplies the source, target and explicit arithmetic policy.
A negative original time correction uses CORE-005 time reversal arithmetic without
pretending it is an ACCT-006 financial refund. The manual-command permission,
preview/cap/deadline edit and request-derivation contracts remain CORE-008/API-007.
An original manual correction uses its own caller-established non-support Receipt
subject; it cannot reuse an already-awarded contribution's original claim merely
because it corrects that contribution. Its retained reason can refer to prior
records without fabricating a new contribution identity. ACCT-006 owns financial
correlation/cumulative entitlements/whole-count refund policy and original Main-versus-debt-repayment allocation. A financial-reversal
command is explicitly dispatched to that required future path; this service cannot
mark it applied or silently treat it as an original correction.

Intake, contribution linkage and capture were acknowledged in EARLIER units.
Caller-owned receipt/configuration/context/plan presence inside this unit proves
neither that history nor source classification. Existing receipt/contribution
owning services are not invoked or adopted inside the application transaction.
All returned repository data remains provisional until this attempt's actual
COMMIT and normal writer-work completion.

## Command identity and preview constraints

Propose an owned `OriginalApplicationCommand` with these logical members:

| Member | Meaning |
| --- | --- |
| Operation key | Stable installation-scoped opaque scope/reference, preserved across reconnect/retry; trusted installation/principal binding is checked separately |
| Source | Explicit original role, typed ReceiptId, tagged contribution/receipt subject and optional CID including0; positive selected plan revision/reference |
| Caller binding | Stable authenticated account OR explicitly authorized automation identity, with actor/decision references only where actually known; never a transient connection/session ID or an implicit internal superuser |
| Preview constraint | Optional opaque preview state token and the selected plan revision; explicit `RequireExactPreview` or `RevalidateCurrentState` choice |
| Explicit choices | Owned ordered options, source/correction intent and any actual decision references; no current cap, newly computed effect or newly allocated state token |

The complete command intent is retained in a versioned choices envelope, with a
lossless canonical encoding defined at the later concrete API/codec stage. Equality
compares every logical member and distinguishes absent from present-empty values;
a hash, current state, or lossy serialization is insufficient. Concrete codec tests
must freeze independent bytes before implementation. There is no service-level
numeric StateRevision.

This retained command intent is DISTINCT from the existing storage
`ApplicationIntent.expected_state_token`: that token is the actual head checked
when a NEW bundle stages. For revalidated stale previews it can legitimately differ
from the command's original preview token. The service therefore does NOT rebuild
a duplicate's physical intent using today's head, or change its retained command
choices to make it compare equal. It reads the old complete record and compares
its retained original command. Historical physical expected-token/current-state
snapshots remain immutable evidence of the earlier application.

For a new operation, actual selected plan head unequal to the requested revision
returns explicit plan conflict after structural metadata validation. It does not
silently select an override/new request list. Changed rules/modifiers/rates do not
revalue an already captured selected plan. Missing historical valuation remains
missing until a separately authorized retained resolution/revision exists; current
valuation cannot invent an arrival-time rate.

A mismatching preview state token is an explicit state conflict in
RequireExactPreview mode. RevalidateCurrentState instead derives one new proposal
from the current checked head, current contribution dimensions and current policy;
its result reports that revalidation occurred and retains both preview and actual
application-before references. A matching preview is only a freshness observation,
not permission or frozen capacity. In BOTH modes, actual cap/clock/policy comes from
the current resolver, never a cap copied from the preview or opaque document.
If current configuration changed through a valid state/configuration protocol,
RequireExactPreview observes its changed state token; an unrelated current-policy
revision still needs current execution checks even when that token is unchanged.

## Current access, policy and observation ports

These are trusted synchronous owner-thread contracts, not client-supplied approval
DTOs. They may use the SAME active session to resolve their own reviewed retained
records. They do not commit, adopt, pump Qt events, call remote services or publish.
Concrete interfaces and capability lifetime tests follow actual logical approval.

| Port | Required current meaning |
| --- | --- |
| Execution access | Check the actual installation scope, authenticated account/automation, current effective grants/constraints and this exact requested source/action. Unknown/missing authority denies. Before returning existing data, separately check access to that actual recorded operation/source/result; an apply grant alone is not unrestricted historical disclosure |
| Original application repository | Compose existing bounded nonowning bundle reads/stage and full operation lookup. An early typed original-subject/source claim lookup must return complete validated existing data or explicit absence without evaluating today's amounts; it owns no transaction. This additive STORAGE-017 primitive is not supplied by the current bundle API and needs its own concrete review/baseline before use |
| Current application policy | Resolve retained configuration meaning, optional nonnegative Main cap, full TimerConfig/hard-end, pause/recovery/lifecycle admission and explicit per-request correction/deficit policy at execution. Bind the decision to the actual checked token/configuration reference, selected source/plan and current policy/permission revision. No nonempty ref, resolved quote, serialized boolean or storage envelope is authority |
| Segmented clock observation | Provide a bounded ordered sequence of explicit elapsed-monotonic/UTC/constant-pause segments from the current snapshot to ONE command observation, with configuration binding. No UTC-minus-monotonic subtraction, sleeps, restart-gap reconstruction or inferred segmentation from a final pause boolean |
| Fresh state token | Return owned nonempty deterministic-test tokens for each required fresh snapshot. A production issuer needs a reviewed uniqueness strategy; service/storage verify actual namespace non-reuse. A collision is a conflict/error, never overwrite, ABA or an unbounded generate-again loop |
| Required action preparation | Resolve actual retained action meaning from the selected captured context and complete calculated outcome. Return owned ordered required job intents/groups and audit explanation, or an explicit unresolved-policy hold. It does not deliver, evaluate new rules, mutate authoritative state or claim provider capability |
| Review hold persistence | Stage a durable non-success input hold with source/command/reason/provenance on this SAME session when numeric overflow or unresolved required policy needs review. Never mark application applied or create success effects/jobs/claim. This nonowning primitive is missing today and needs a concrete reviewed schema/API/baseline before durable hold enforcement is claimed |

The owner queue excludes concurrent policy mutations during this synchronous unit;
mutable policy outside the state-token fields must be versioned/serialized by the
same authority and rechecked here. A stale external permission decision is not a
valid port implementation. Independent fakes with explicitly stated synthetic
semantics can prove service call ordering; they do not verify real identity,
permissions, clocks, captured action semantics or the future adapters.

STORAGE-002's token covers its timer/debt/balances/envelope/configuration only.
It does not cover old contribution writers, plan-head appenders or permissions.
Resolve and compare all15 current contribution fields and actual selected plan
head separately; preserve original support anchor and current evidence/valuation/
review/reversal/settlement independently. A confirmation can promote CURRENT
evidence without rewriting the original support or awarding again.
Use the reviewed contribution eligibility assessor; empty blockers permit further
planning only. Current policy still must reject/hold paused, unresolved recovery,
Ready/Finished and unready action paths as applicable. Ordinary incoming support
cannot bypass paused review, clear a recovery/pause reason, start or reopen.
An authorized paused acceptance/silent/batch decision belongs ACCT-005. A separately
approved post-finish score/action policy remains required; absence of that policy
is unresolved, not an invented default to award or silently discard the input.

## Transaction protocol and result order

Admission copies bounded owned inputs onto the authoritative queue. Pure malformed
input/admission failure is separate from an executed logical outcome. One executed
request begins ONE actual writer session before accessing current policy/storage.
Every normal executed result, including denial/conflict/held/duplicate, requires
an acknowledged COMMIT and normal work-body completion. A denial may commit an
otherwise read-only unit; it contains no protected historical result or fabricated
application audit. Returning without BEGIN/COMMIT would produce existing writer
missing_transaction/not_committed with no normal payload, and is not this protocol.

Within that unit, the required order is:

1. Establish current execution access to this installation and command. Denial
   returns no protected existing data; perform no application/clock/token work.
2. Look up the exact operation key. Validate its complete recorded closure before
   using it. Verify current disclosure permission against its actual source/result.
   Same retained complete command returns that historical outcome; changed command
   is operation conflict, with no old result attached. Do not reevaluate current
   eligibility/caps, invoke arithmetic/action preparation or issue fresh tokens.
3. Resolve original typed subject/source claims through the proposed early claim
   projection. Authorize disclosure before returning an existing record. A claimed
   subject is explicit existing/conflict; no new key, confirmation, changed preview
   or plan revision can award it twice. Historical data is not compared with today's
   state/contribution/cap. Healthy refusals make no own changes.
4. For genuinely new work, read actual current state, selected complete plan/current
   head, actual typed source and current contribution/support anchor. Enforce plan
   and preview rules above and current independent eligibility/access constraints.
   Resolve current bound policy and clock segments. Unsupported/unresolved required
   modules return an explicit held/not-ready outcome, not a fabricated successful
   application. A required durable review hold uses its reviewed hold port.
5. Advance the clock first through the supplied valid segments using CORE-002;
   preserve ordered transitions and hard-end precedence. Bind clock-only advanced
   before to current head: preserve debt/counter keys+values/configuration/format.
   Reuse head token only when every bound field is identical; otherwise issue a
   fresh observation token and retain its explicit observation explanation.
6. Compute every effect and final state in memory using the rules below. Resolve
   required jobs/result/audit before writes. Any invalid numeric input, aggregate
   overflow or unresolved mixed policy produces NO application subset. A numeric
   overflow must stage a durable review hold through the reviewed hold port; merely
   returning Held while automatic eligibility stays clear is insufficient.
7. Issue a different unused after token even for actual0. Prepare ONE complete
   immutable bundle with historical requests/provenance, actual components and
   all required outputs. Stage it through the real borrowed store, preserving
   actual CAS, full current contribution and plan checks. Map every existing/
   conflict/stale/claim result explicitly; no blind retry or new key generation.
8. Perform actual COMMIT, return normally from the work body, and only then use
   its committed normal completion payload for authorized publication/response.
   A database row, staged result, duplicate or synchronous commit return alone
   never invokes the external publisher.

Access and lookup operations may themselves encounter known storage failures;
those retain actual whole-unit rollback and no normal payload. A domain refusal
can finish by committing its read-only unit. A new durable hold commits only its
specified non-success hold state and retained audit, never an application claim or
success record. Prior intake remains durable in all known application rollbacks.
An existing durable hold is not a completed successful operation, cannot consume
the original award claim, and cannot authorize automatic repeated application.
ACCT-005's later explicit authorized resolution must link that hold and preserve
its history; concrete revision/continuation identity is a required hold-contract
question before signatures or a durable-hold implementation.

## Durable hold identity, gate and resolution

STORAGETEST-016/STORAGE-016 must concretize the following nonowning port obligations
before ACCTTEST-004's executable service baseline. This logical contract does not
implement them in existing bundle/contribution storage or declare them verified.

- An active hold binds the trusted installation scope and actual tagged ORIGINAL
  input subject: CID including0 for tracked support or positive ReceiptId for a
  caller-established non-support original. It also retains the exact source/event,
  captured plan revision and command intent that caused it. Scope is independent
  of operation key, transient session, new confirmation receipt and later plan
  revision. Creating another operation key cannot bypass it. Never correlate holds
  using names, amounts, timestamps or an unverified external identifier.
- Separate `WholeInputNumericOverflow` from `TimerAdjustmentDeficit` and unresolved
  required-policy/recovery holds. Whole-input overflow retains the complete failed
  request list and exact failure target/ordinal when known; no fitting subset may
  award. Timer deficit retains the exact negative request, selected explicit policy,
  full unapplied amount and the timer occurrence/component selector. It does not
  rewrite payment/valuation/review/settlement or label unrelated requests overflow.
  The one atomic operation remains pending until its required held adjustment has
  an explicit disposition; this is not permission to commit a partial application.
- Records retain a stable hold identity, opaque current version, complete reason/
  scope/source/command/provenance, actual actor/policy/observation data where known,
  and immutable creation/resolution history. Exactly one active state for a scoped
  hold identity is visible; repeated detection appends/returns traceable history
  under the reviewed identity rules, never silently overwrites the earlier reason.
  Concrete uniqueness, simultaneous independent hold representation, size limits,
  allocator/version exhaustion and exact error categories belong its own baseline.
- After access and exact-operation/claim handling, read all ACTIVE holds for the
  actual new-operation subject BEFORE clock/token/arithmetic/action preparation.
  Healthy active hold returns held with permitted references; a separate disclosure
  check governs protected hold details. No new operation/plan revision/confirmation
  bypasses it. A malformed/dangling hold is a typed storage failure with actual
  rollback, not absence, automatic repair or permission to apply.
- A fresh numeric/policy hold stages its non-success record/audit on the same active
  unit and commits only that hold. It creates no application effect/job/claim,
  no after-state/head advance and no applied status/success result. Earlier durable
  intake/capture is retained. A failure staging/committing the hold follows the
  same real rollback/unknown distinctions; do not report a durably held outcome
  unless THIS hold unit's actual COMMIT and normal completion establish it.
- Resolution requires explicit CURRENT authorized review/correction capability,
  exact hold identity/version and recorded decision/actor/reason/options. Unknown
  authority, stale version or another unresolved independent hold blocks execution.
  Clearing one hold never clears other eligibility/pause/recovery/evidence/rate holds.
  Preserve original arrival/capture provenance and create an authorized plan revision
  where the intended calculation changes; never rewrite the held original request.
- Resolve-and-apply is ONE controlled transaction: compare current hold version,
  recheck all current conditions, stage its immutable resolution and complete new
  successful bundle together, then actual COMMIT/normal completion. No separate
  clear-hold commit leaves an automatic-apply gap. A new overflow during revalidation
  may retain/replace the active hold only under reviewed version/history rules;
  no success claim survives. Explicit reject/defer-only review outcomes and the
  concrete continuation command identity remain ACCT-005's reviewed command policy;
  the storage primitive must support atomic versioned resolution without granting
  that policy or creating a second original award.

Actual production admission remains unavailable for any path needing an absent
hold adapter/current-policy meaning. Synthetic hold fakes can establish orchestration
order only; passing those tests is not durable hold acceptance. The hold task must
have independent real-store fixtures, reviewed tests and observed intended red,
then implementation/code review/ordinary CI before the service baseline consumes it.

## Effect and lifecycle composition

Selected request order is the effect order. Preserve each action/rule/version/
target, original money/rates, exact request, final signed64 request and residual.
This service consumes the already-resolved plan amounts; the upstream evaluator/
CORE-001 final conversion owns their half-even correctness and parser limits.
It does not rerun current rules/modifiers or round a cap-limited result again.
Copied Exact minus resolved request supplies the unchanged rounding residual.
Unresolved original numeric derivation is held; a typed stored number alone does
not certify that derivation.

For a nonnegative original milliseconds request, run CORE-005 time addition on
current Main and separate debt with the CURRENT cap. Effects are actual Main delta,
negative debt-repaid delta and cap-limited remainder, including explicit zeros.
Only actual `main_added_ms` is passed to CORE-002 applicable credit; debt repayment
and requested/capped amounts cannot rescue Grace or confirmation wait. Preserve
retained Grace and independent pauses. At/above cap keep existing Main unchanged,
still repay debt, and cap-limit the remainder.

For an explicitly authorized negative original milliseconds correction, require a
retained explicit deficit policy, use CORE-005 time reversal, retain Main/debt/
unapplied components and policy/disposition, and run CORE-002's zero-elapsed
observation at the same instant after Main exhaustion. Do not negate INT64_MIN.
A deficit requiring review is a TIMER-ADJUSTMENT hold, not NUM-02 numerical
overflow and not a fabricated review state for all other dimensions. The complete
atomic operation remains pending while that required adjustment has no authorized
disposition; no subset is applied merely because the other proposals fit. The
record retains its specific request/component scope and all unapplied provenance.
An explicitly reviewed mixed-effect application/continuation policy is required
before enabling a different disposition. This preserves CORE-005's timer-only
numeric meaning and the correction path without inventing financial refund policy.

Score micro-points and whole counts use exact checked addition of the already
resolved signed64 request to the actual matching target. The direct component
must equal the request; there is no invented score cap/hold or silent saturation.
Check the entire input's aggregate final bounds before staging any effect/job.
Multiple requests to one timer/counter are applied in captured occurrence order
and use each preceding calculated balance; retain separate occurrence identities.
A later overflow holds the WHOLE input, including earlier fitting proposals.

Advance-first may reach a hard end or natural finish before ordinary credit.
Ordinary support then cannot reopen and does not accidentally repay debt/change
Main. Required post-finish score/automation behavior is selected only by its own
explicit current reviewed policy; unresolved policy holds that operation. The
service is not a start/reopen, cap/deadline edit, schedule engine or recovery engine.
A held/denied application does not secretly commit a clock-only partial application;
periodic/independent clock advancement belongs its own authoritative command.

## Versioned retained meanings and future concrete gates

The later fixture/API stage must freeze actual bounded codecs, not use labels alone:

| Envelope | Required meaning |
| --- | --- |
| `subathon.internal.application-command.v1` | Complete canonical original command intent, original preview/mode and stable caller binding; distinct from derived actual state token/cap/effects |
| `subathon.internal.application-state.v1` | Owned observation annotation binding the typed stored timer/debt/counter snapshot to current configuration and clock observation/segments; no portable downtime checkpoint or process-local tick masquerading as one |
| `subathon.internal.application-result.v1` | Operation/source/plan identity; original preview versus checked head/advanced-before/after tokens; revalidation flag; ordered requested/actual effects and required job references; new versus existing result is a response disposition, never a rewritten historical document |
| `subathon.internal.application-audit.v1` | Actual stable actor/automation and decision/reason/options where known, current permission/policy/configuration references, selected plan/capture, observation explanation and calculated outcome; no invented absent actor/source fields |
| Action-specific captured job format | Retained captured action definition/meaning, stable source occurrence, owned job/group order and actual intended synthetic adapter action. A fixture-only annotation is explicitly non-deliverable; real provider sending identity/payload/state/retry remains the job/provider contract |

These are logical content requirements, not finalized C++/JSON/wire/binary layouts.
Result duplication of typed fields must be checked against those fields before
publication; do not permit an opaque document to contradict the authoritative
record. Unknown format is explicit not-ready, not opportunistic execution.
No new dependency, physical schema, account/grant codec, policy engine or provider
capability is selected by this proposal. Before executable authoring, review the
actual concrete signatures/error/bounds, claim projection, hold record/continuation,
retained codecs and fake action/clock/current-policy meanings independently.

## Independent literal examples

These are hand-derived expectations for later tests, not generated by production
helpers and not observed service acceptance. Existing child passes/old209 are not
this service's reviewed baseline or missing-behavior red. Units are milliseconds
and millionths of points unless shown otherwise.

| ID | Setup and required observation |
| --- | --- |
| AS01 current capacity | Plan request+600000; preview Main3300000/cap3600000. Actual head Main3480000/debt0. RequireExactPreview→state conflict if token changed. RevalidateCurrentState→Main+120000/debt0/capped480000, after3600000; retain old preview and actual head separately |
| AS02 current cap | Same actual Main3480000 but current cap3540000, while preview cap was3600000. Authorized current config/token binding→actual+60000/capped540000, never preview-derived+120000. A mismatching bound policy token/config is rejected, not guessed from document bytes |
| AS03 debt first | Main55/debt2,request10,current cap60→debt-repaid2/Main+5/capped3, afterMain60/debt0. Preserve components+5/-2/+3 and request10 |
| AS04 above cap | Main100/debt5,request8,cap60→Main100/debt0; Main0/debt-5/capped3. No shrink to60; no invalid call to the older narrow credit helper |
| AS05 zero rescue | Active Grace90000 or confirmation wait Main0, debt5/request5/cap0→Main0/debt0, actualMain0; Grace/wait unchanged. Request7/no cap→debt0/Main2, Grace→Main or wait cleared, retained Grace preserved, pauses unchanged |
| AS06 ordered occurrences | Main55/debt2/cap60, requests10 then4→first Main+5/debt-2/capped3, second Main0/debt0/capped4; two effects retain their own ordinal even if action refs equal. FinalMain60/debt0 |
| AS07 mixed overflow | Main55/debt0/time request5 fits; scoreINT64_MAX plus score request1 overflows. NO time/score/effect/job/application claim/success result commits. Original receipt/capture remain; durable numeric review hold records the reason through its required hold adapter |
| AS08 no hypothetical overflow | MainINT64_MAX/debtINT64_MAX/requestINT64_MAX/no cap→debt repaidINT64_MAX/Main+0/capped0; Main unchangedINT64_MAX/debt0. Never reject hypothetical Main+request when actual fits |
| AS09 independent eligibility | Current evidenceunknown plus missing historical rate plus reviewpending; accepting review elsewhere resolves only review. No application. Later confirmation promotes current evidence without rewriting original support; missing valuation still blocks until explicit retained resolution/planrevision |
| AS10 historical duplicate/access | Original opA committed AS01. Current Main lowered/rules changed/access revoked: deny with no historical payload and no arithmetic/action/token calls. With current authorized disclosure and same complete command: return original+120000 record after THIS unit's acknowledged read-only COMMIT/normal completion; no new effects/jobs/audit |
| AS11 complete intent/claims | Same key but changed source/planrevision/previewmode/explicit choice/stable caller→operation conflict, no existing payload. Different key for same CID0, including a linked confirmation receipt→existing subject/conflict before current revaluation. Tagged receipt0 is invalid; CID0 remains valid and distinct |
| AS12 concurrency | Two queued authorized attempts for the same earlier-durable source run in actual FIFO units. First stages and commits once; second sees the complete recorded result/claim and returns existing/conflict. It never computes an extra award or publishes an own-session speculative record |
| AS13 B08 known rollback | Prior receipt/capture acknowledged. Real fault after state/effects/application patch but before complete jobs/result/audit→whole caller unit rolls back; prior intake/not-applied retained, no after-state/claim/effect/job/success payload. A later explicitly permitted attempt rechecks everything and can apply once |
| AS14 unknown commit | Physical whole bundle COMMIT with lost acknowledgement→actualunknown/no normal result/publication, fail-closed writer/no blind retry. Earlier recorded duplicate data cannot turn THIS attempt's unknown into success |
| AS15 completion boundary | Confirmed COMMIT then body exception→accounting durable/no normal payload; later response failure AFTER normal committed completion→record stays durable and original normal result existed. Fresh service instance/reopened file retrieves it without new effect; recovery/outcome resolution itself remains separate |
| AS16 deadline first | Active Grace or wait paused, enabled deadline1000, observationUTC1000→HardEndReached/Finished before ordinary positive credit. No automatic reopen, debt repayment or guessed post-finish action policy; unresolved required post-finish behavior held explicitly |
| AS17 signed correction | Original authorized correction Main100/debt7/request-150, explicitStopAtZero→Main0/debt7/Main-100/unapplied-50, then existing zero-elapsed ending policy/trace. CarryTimeDebt→Main0/debt57. RequireReview→no automatic partial mixed-input application; require reviewed hold disposition |
| AS18 signed minimum | Original authorized Main1/debt0/requestINT64_MIN, CarryTimeDebt→Main0/debtINT64_MAX with Main-1/debt+INT64_MAX; Main0/debt0 same request overflows and holds whole input. No signed negation, financial-refund identity or original award recomputation |
| AS19 token and configuration | Clock advance changes Main100→90 only: head a retained, fresh before b, fresh after c. Changing debt/counter/config/format during that advance is source mismatch/actual known rollback. Identical bound observation may reuse a as before; after is always unused, including actual0. A token collision never overwrites or loops |
| AS20 independent freshness | State token a still current but current contribution conditions differ from the old capture→reassess ALL actual dimensions, never overwrite them from capture. A change to any checked one of15 fields between input read and stage→stale contribution, no award; actual plan head change→plan conflict. RevalidateCurrentState cannot silently choose a different requested plan revision or infer permission from matching a |
| AS21 required module hold | Current pause/recovery policy or required milestone/action meaning unresolved→explicit held/not-ready, no successful application and no delivery. Numeric hold cannot be claimed durable from a returned enum alone; its reviewed persistence/continuation is required before enablement |

## Remaining mandatory scope and next gate

No owner requirement change is proposed. Concrete gaps requiring bounded work are
STORAGE-017 early typed claim resolution, STORAGE-016 durable hold record/continuation, actual current
access/policy/segmented-clock/action adapters and exact retained envelope codecs.
They must be resolved through actual reviewed artifacts, not optimistic fake ports.
The original ACCT-004 acceptance and numerical hold requirement are not satisfied
by storage-only success or simulation of a missing durable adapter.

ACCT-005 retains individual/batch/silent acceptance and milestone Run/Defer/Skip;
ACCT-006 retains financial refund entitlements/allocation/holds; milestone jobs and
post-finish rules retain their owning tasks. API/session/access/recovery/start/reopen/
schedule/deployment/wire/provider scopes remain required. All seven integrations,
three exchange-rate providers, both renderers, English/German/themes, supported
platforms/packaging/security/backup/live checks remain in the agreed release.
Actual different logical review is next for this DOCUMENT task ACCTCONTRACT-001.
Then the separately baseline-gated STORAGETEST-016/STORAGE-016 hold slice and
STORAGETEST-017/STORAGE-017 typed early claim query must integrate before
ACCTTEST-004 consumes them. The query is additive: no migration, weakening of old
stage-intent equality, raw SQL escape or dummy PreparedBundle lookup shortcut.
Only then concretize/review the minimal service API/stubs/harness, author independent
fixtures/tests, compile/discover and observe this service's own intended missing
behavior. ACCT-004 production waits for that gate. This contract does not finish
ACCTTEST-004, STORAGE-016 or the application feature by relabeling planned ports.
