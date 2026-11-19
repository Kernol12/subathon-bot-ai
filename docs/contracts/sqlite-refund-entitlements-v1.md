# SQLite financial refund entitlements — Stage 1 draft

Tier A, ACCT-006 [#199](https://github.com/Kernol12/subathon-bot-ai/issues/199),
[draft PR #726](https://github.com/Kernol12/subathon-bot-ai/pull/726). Fix rounds
0/2. Specification v0.45 §§3, 4, 8 and NUM-03. Draft for review. Items marked
open need a disposition before their behavior can be frozen.

The base is `06dbc9806cb89468f6e7470a80bfb4a7a29d13f3` /
`51a80a76d1759fef9bd7cab7842b4da84991b07e`. The registered A01–A15 core
source and assertions stand as reviewed. Their
[observed red](https://github.com/Kernol12/subathon-bot-ai/pull/726#issuecomment-7011613636)
covers calculation only. W01–W07 below are independent boundary oracles.

## Scope and authority

[#199](https://github.com/Kernol12/subathon-bot-ai/issues/199) owns admission of
a supplied verified financial correlation, exact accepted-money and requested
entitlement progress, durable financial disposition and explicit continuation
with no replay. [#201](https://github.com/Kernol12/subathon-bot-ai/issues/201)
owns real timer deficit, score deficit and action/progression wiring, including
authoritative live composition. Provider verification, public transport and
external delivery retain their owning slices.

The [owner rulings](https://github.com/Kernol12/subathon-bot-ai/pull/726#issuecomment-7008905863)
settle three behaviors:

- **31B** Active mixed refund with qualifying timer RequireReview. Apply score
  once and hold the entire exact timer adjustment, including both legs if it
  restores original debt. Authorized resolution applies the retained adjustment
  as one unit with no replay.
- **32B** Finished refund. By default apply score and explicitly retain the timer
  request as non-applied with reason `finished`. No timer hold or automatic
  debt. Timer stays Finished.
- **33A** Round the cumulative total time entitlement once, allocate first to
  original actual Main and then to original actual debt repayment. Consume
  requested allocations independently of actual physical effects.

The service checks current execution authority on application, retry and
continuation, including installation, session, permission, scope and policy.
Disclosure of stored results and their linked facts has its own current check.
A verified fact, admitted receipt, old result or hold version grants neither.
Unavailable clock, policy, permissions or evidence fails closed. Ready is a
distinct prerequisite refusal for this bounded path, preserving the existing
service guard. A Ready snapshot never enters the started reducer or takes the
Finished disposition. Refunds cannot start or reopen the timer. For a started
snapshot, use the current authoritative policy and clock observation, including
hard end at equality even during pause, grace or confirmation wait. Classify
Finished after observation and before choosing refund timer effects. An
application refusal does not commit a clock-only advance.

## Financial input, progress and result

Resolve a durable refund receipt and supplied verified scoped economic refund
identity and correlation, with evidence reference, original payment/support
receipt, CID, immutable original operation/capture and exact original effect
locators. Resolve matching actual targets and the original settled money in its
currency. Retain source decimal spellings, parsed exact amounts, source and
arrival context, original historical valuation/rate and rule/action/version
references. The original awarded quantities come from actual effects, including
cap provenance. Current caps, rates and rules cannot change those quantities.
Receipt transport identity and connection provenance follow the receipt
contract. Names, nearby timestamps, equal amounts and connection changes cannot
establish economic identity. Missing or unverified facts remain unknown; the
raw receipt can exist without accepted financial progress.

The payment journal has an immutable entry per accepted economic refund and a
checked payment head keyed to the verified original payment and support anchor.
An entry retains previous/next position and exact accepted totals, original and
incoming money and spelling provenance, and the prior/new basis positions and
requested allocations. Guard actual receipt and economic refund identity
uniqueness in addition to existing operation key uniqueness. Bind the operation
key to complete stable command intent, including the financial receipt, verified
correlation, selected plan/capture, expected positions/state and explicit
choices or overrides. Compare complete canonical intent and selected retained
facts, including optionality. Exact retry returns the immutable original
outcome after current authorization, even if intervening commands changed the
head or closed a hold. It does not recalculate against that head. Conflicting
key reuse, receipt reuse under a different key or economic identity reuse
under a second receipt makes no financial writes or publication. Raw intake
and its attempt history are independently durable.

After identity checks, revalidate payment position, each selected basis
position, actual plan/source and authoritative state/contribution independently.
Each incoming amount is positive and in the verified original currency. Original
amount is positive, prior accepted total is nonnegative and the sum cannot exceed
original. An over-original refund is refused in full. Exact equality is full.
An accepted refund with zero rounded request still consumes its identity, exact
money and a position. Never infer payment-wide
money from requested or actual effects, fractions in a legacy effect link, CID
alone or the number of receipts.

Let F be cumulative accepted money after this refund divided by original money.
Every selected time, score and count link in this refund uses that same F.
For each eligible actual credit X, keep C = X × F, rounded cumulative
magnitude R and residual C − R.
Time and score use half-even at ms and micro-point precision. A count holds R
at zero until exactly full. The signed increment is previous R − new R. At full
R equals X. Time uses one X = actual Main added + actual debt repaid. Repayment
is the magnitude of the original negative debt delta. Split R at original Main
and subtract each prior allocation. Retain signed Main request and nonnegative
restore request. Use bounded exact codecs and checked conversion, totals and
physical balances. Numeric overflow follows the review refusal/hold boundary
with no financial effects or accepted advance for that attempt. A numeric
review disposition is durable only if its own audited retention commits.

Keep immutable requested entitlement and the actual effect/disposition separately
inspectable. Time records actual Main subtraction, original debt restored,
new deficit debt and unapplied request by leg. With nonnegative magnitudes,
Main requested = Main removed + new deficit debt + unapplied Main, and
restoration requested = original debt restored + unapplied restoration.
The debt balance increases by restored debt plus new deficit debt. The grouped
outcome distinguishes `applied`, `held` and `finished`. An applied outcome
records any policy-limited unapplied Main. Both unapplied amounts are
nonnegative magnitudes. The Main request is signed and nonpositive. The original
debt restoration request is nonnegative. Under RequireReview, 31B holds both
legs with zero actual timer deltas only if the Main request's magnitude exceeds
live Main at the fresh observation. A restoration-only request and exact
exhaustion of Main apply without a deficit hold. Under 32B keep both requests
unapplied with reason `finished` and represent the clock's own transitions
separately. Preserve independent holds, completed milestones and
performed actions. Any effect on unreleased action policy is driven by actual
score before/after on first application, with the stable refund ref. Continuation
must not submit a second score change or policy observation.

## Composition with shipped writer types

Use one caller-owned [`SqliteWorkSession`](sqlite-writer-v1.md). Reuse typed
[receipt](sqlite-receipts-v1.md) and selected [plan](sqlite-application-plans-v1.md)
reads, [bundle](sqlite-application-bundle-v1.md) locators and snapshots,
score/count basis links where compatible, and typed v13
[milestone decision](sqlite-milestone-group-decisions-v1.md) observations.
Existing writer limits, canonical encoding, checked selected
read closure and real rollback/error precedence apply. Reuse bundle validation
and write paths where their invariants still hold. The original service and its
result/command formats retain their original-kind semantics.

The smallest proposed extension is a typed financial variant in the bundle
boundary with additive retained records. It uses the existing state head and an
immutable current request ordinal. It has three distinct pieces:

1. **Accepted refund.** One financial journal entry and payment head update
   linked to the real reversal operation, refund receipt and original. Recommend
   staging a deterministic financial capture in the same unit on first
   acceptance, binding it to admitted inputs and positions. The ordinary
   bundle requires the actual current plan revision and exact/final request
   equality. Use integral exact values for the finalized instruction in the
   capture, and retain exact cumulative entitlement/residual in typed financial
   provenance. The instruction is already finalized by the cumulative
   calculation. Applying ordinary half-even to exact cumulative minus prior
   rounded is unsound: C = 2.5 and prior R = 1 yield increment magnitude 1,
   while independently rounding 1.5 yields 2. A separately committed candidate
   capture would still need final revalidation and audited replacement; it
   cannot be silently changed on retry.
2. **Grouped time basis and disposition.** Reference the original time
   addition by operation and ordinal, with checked prior/new split, total
   fraction, exact entitlement and residual. Legacy basis reads reject additions
   that repaid debt. Their Main/score/count component invariant multiplies the
   selected component by F and cannot validate the Main-first split. Even
   Main-only financial time can use the grouped shape with zero original debt.
   Its typed validation replaces the legacy per-effect link requirement only
   for that ordinal and checks both legs and balance identities above. Finished
   and held are distinct dispositions with typed evidence.
   Do not encode either as cap limitation. Score/count links keep ordinary basis
   checks and must match the journal fraction. Refuse financial acceptance when
   any selected time, score or count basis already has non-financial reversal
   history. Refuse non-financial reversal links to any basis with financial
   history. The guard uses exact original operation and effect occurrence
   identity and covers the whole unit. Keep the real ordinals and effect records.
3. **Financial timer continuation.** Add a `FinancialTimerAdjustmentDeficit`
   cause on the original contribution's CID as `HoldSubject`. The cause retains
   the exact signed Main and debt restoration requests, prior and next basis
   allocations, selected time occurrence and financial refund operation. A
   distinct resolution outcome binds resolution intent and exact hold/version.
   Reuse current [hold](sqlite-application-holds-api-v1.md) versioning,
   active-set checks and audit shape where compatible. Existing
   associated-hold and resolution-application guards require original-kind
   bundles. A second same-receipt reversal would also violate source uniqueness
   and require a new entitlement link. The continuation advances the same
   application state head when applied and appends audited closure and actual
   time effects with no financial claim or money/basis-head advance. The first
   outcome remains immutable, including its original held disposition. No reuse
   of original or manual correction kind to bypass a guard.

Resolution needs current authority, the actual active origin/version, current
active set and fresh observation. A deficient continuation without an override
refuses and retains the exact hold and its version. An explicit case override
may select `StopAtZero` or `CarryTimeDebt` under current override authority.
Bind that choice into the immutable resolution intent and audit. Pass the whole
retained time adjustment to the owning deficit policy once, restoring original
debt as a separate checked leg. Record actual, unapplied and deficit-debt
components. #199 validates and records the resolution. #201 supplies live
deficit composition. The typed outcome supplies the disposition and effects
needed for per-case closure without repeating score, money or milestone policy.

Successfully applying the retained adjustment closes its exact financial timer
hold in the same unit. A separate active application hold on the same CID
blocks application-linked resolution. Preserve it and the financial hold.
Milestone Review is a decision in its own store and does not block timer
resolution. An exact retry reports the recorded resolution. A competing or stale
resolution cannot consume it again.

## Transaction and acknowledgement

Prior intake can commit separately. The financial owner begins one real writer
unit and stages capture if needed, identity, journal, entitlement/history,
state/effects, hold changes, audit and applicable decision/job writes together.
All component returns are provisional. The owner commits once. Publish only
from a normal, acknowledged committed completion with the matching typed
result. No adapter opens a second writer, pumps the event loop or owns an extra
commit. Healthy conflicts/refusals stage no application. Failure after staging
requires whole-unit rollback; preserve the previously committed intake.

Lost physical acknowledgement is outcome unknown and closes writer admission
under the existing writer contract. An acknowledged commit followed by a body
error is committed without a success payload. Neither publishes success via
the failed completion. Reconcile through fresh authoritative committed reads:
match exact immutable operation/resolution identity and complete selected
closure, including payment and effect occurrences, disposition, audit, jobs
and hold history. The latest state head may have moved. A complete match proves
the retained outcome; absence can support a newly authorized/revalidated
attempt only when rollback or absence of the entire unit is established.
Partial/conflicting closure or unavailable evidence keeps the outcome unresolved.
Never blindly replay or treat a staged value as proof.

## Open owner decisions and recommendations

| Decision | Recommended rule and alternative |
| --- | --- |
| 34. Later refund while an earlier timer request is held | 34A retains later intake and refuses financial acceptance as a prerequisite until the hold resolves. Explicit authorized resubmission revalidates the input. Accepted money and requested progress stay fixed and the receipt stays available for resubmission. 34B accepts later proven money and consumes the next allocations with ordered timer effects, permitting later score sooner. Numeric overflow without accepted progress is outside 34. |
| 35. Held continuation observes Finished for any reason | 35A closes the hold with audited `finished` and zero timer effects, recording a newly observed finish with the closure. 35B retains it for a later authorized disposition. Both preserve prior score and avoid implicit reopening. This covers Main expiry, grace or confirmation completion, hard end and authorized deadline edits. 32B already covers first application in Finished. 34A with 35B can delay later score reversals until authorized disposition. |
| 36. Additive catalog sequence | 36A gives #215's reviewed v14 implementation priority before #199's durable writer integration. 36B keeps the queue and leaves #199's integration pending catalog assignment. Both preserve #719's reviewed v14 boundary. |

Unknown older payment history stays unavailable pending verified reconciliation.
Only pristine checked history starts at zero. Conflicting economic identities
refuse financial mutation and preserve raw intake and prior history. Verified
alias selection belongs to its source authority. The financial journal exposes
accepted money and fraction separately from existing effect and review summaries.
In #201's live Stop/Carry composition the Main deficit rule applies only to the
Main leg. Restore original repayment separately in the same checked unit.

Storage layout, canonical codec reuse, bounded paging and one-unit capture are
implementation choices within these semantics. Any broader lifecycle or post-finish
configuration mapping belongs to its owning policy work. Active main catalog is
v13. #719 froze the v14 design and #215 owns its implementation. This draft
makes no catalog assignment or sequencing decision.

## Independent writer/service examples

Use real controlled SQLite work on the owner thread, already committed synthetic
intake and disposable state. An independently modeled same-session effect port
may supply literal authoritative observation/effect proposals for #199. Exercise
the real financial validation, writes, closure reads and acknowledgement. W05
may invoke shipped v13 milestone decisions on test-supplied actual score and
current target. That covers the handoff operands and transaction, while #201
retains production clock, deficit and progression integration. Amounts below
are exact EUR unless stated. Time is ms, score is in micro-points. For a future
writer-only intended red while the core entry points still throw, the reviewed
fixture may supply independent literal entitlement proposals. The target must
reach the named missing writer/service behavior, with real writer setup and
separate evidence from A01–A15.

| Case | Inputs and independent expected outcome |
| --- | --- |
| W01 — Immutable basis | Original `20.0000`, opaque historical rate spelling `0.972500`, requested 600000 time, actual Main 300000, score 40000000, count 7. Change current cap, rate and rules. With sufficient live balances, refund 5, 5, 10. Accepted totals 5, 10, 20. Requests and actual deltas are (−75000,−10000000,0), then the same, then (−150000,−20000000,−7). Full only at 20. Every link carries F = 1/4, 1/2, then 1 for the corresponding journal entry. Read immutable receipt/capture/effect links, rule/action versions, historical spelling, cap tail, audit and committed state. |
| W02 — Retry and conflict | Commit W01's first refund under K, scoped r1. Advance state with another command. Exact authorized retry returns K's immutable prior result. Changed intent under K, receipt reuse under K2, or another receipt claiming r1 refuses with no financial writes or new publication. Test revoked execution and denied disclosure independently. Prior intake remains. |
| W03 — Exact admission and stale positions | From accepted 19 of 20, two 0.75 proposals share a payment position. One commits at 19.75. The other is stale. Freshly evaluated, the entire 0.75 is refused because it exceeds the remaining 0.25. Both raw receipts remain. Exercise stale component and state guards separately with payment position matched. Wrong currency, unknown amount or unverified correlation advances nothing. A separate 0.01 of 1 against actual 1 ms records F = 1/100, C = 1/100, request and actual zero, and advances the accepted-money position. Refuse a score/count link with a fraction other than the journal's F, even with internally consistent amount fields. Reject cross-path reuse for each basis kind in either order with no partial financial advance. |
| W04 — Atomic closure | Use W05 first application with prior committed intake. Witness staged writes and previous committed state on a separate read connection. Force a reached late SQL write failure. Known rollback leaves accepted money, score, timer, hold, milestone audit/job closure, refund audit and head at prior values. Inject lost acknowledgement of a successful real COMMIT in a fresh unit: unknown, no success publication, then fresh read finds the whole new unit and stable intent once. Separate acknowledged COMMIT followed by body failure: committed/no response, same durable closure. Also force failure after staging resolution effects to check that the active hold and prior committed mixed outcome survive with no partial resolution. |
| W05 — Active 31B | Original 20, Main credit 300000, score credit 40000000. Live Main 50000/debt 0/score 40000000. Refund 5 with RequireReview: score 30000000 once, Main/debt unchanged, financial hold v1 on the original CID for all −75000, accepted money 5, requested magnitudes 75000/10000000. Existing unreleased achieved target 35000000 under milestone Review retains achievement, deficit 5000000 and one refund review. This decision does not block timer resolution. After authorized credit raises Main to 100000, resolve v1 to Main 25000 and debt 0. In separate controls from Main 50000, plain continuation refuses and keeps v1. Authorized Carry closes v1 with Main 0 and 25000 new deficit debt. Authorized Stop closes v1 with Main 0, debt 0 and 25000 unapplied Main. Score, accepted money, requested allocations and milestone observations stay fixed across successful resolutions. Denied, stale or competing resolutions do nothing. Exact retry reads closure. A separate application hold on the same CID blocks application-linked resolution, preserving both holds. With W07's second request at Main 0, RequireReview holds both −1 and +1 while scoring once. |
| W06 — Finished and Ready authority | Same 5-of-20 mixed refund with performed actions. In already Finished, actual timer/debt changes zero, requested Main −75000 explicitly non-applied as `finished`, score 30000000, no timer hold/debt/reopen. Repeat with paused Active at exact hard end and verify committed HardEndReached then Finished and identical refund disposition. Ready gives prerequisite refusal with raw retention only and no started reduction, money or effect. |
| W07 — 33A restoration | Original 8 and actual time addition (Main +3, debt −3). Independent credit leaves live Main 10/debt 0. Refunds 2, 4, 2 yield exact totals 1.5, 4.5, 6, rounded 2, 4, 6 and residuals −0.5, +0.5, 0. Cumulative splits (2,0), (3,1), (3,3) give requests (−2,0), (−1,+1), (0,+2) and live (8,0), (7,1), (7,3). New deficit debt remains zero. The third, restoration-only request applies under RequireReview. An independent second-request fixture at Main 1/debt 0 with RequireReview ends at (0,1) without a hold. Tie control (3,2) at half gives 2 and (−2,0). Full clearing control (6,4), prior (5,0), gives (−1,+4). Read linked histories, including original negative debt delta and distinct actual restoration. |
