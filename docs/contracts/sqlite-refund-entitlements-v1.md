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
original. Exact equality is full. An accepted refund with zero rounded request
still consumes its identity, exact money and a position. Never infer payment-wide
money from requested or actual effects, fractions in a legacy effect link, CID
alone or the number of receipts.

Let F be new accepted money divided by original money. For each eligible actual
credit X, keep C = X × F, rounded cumulative magnitude R and residual C − R.
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
The debt balance increases by restored debt plus new deficit debt. Under 31B
hold both legs and leave both actual timer deltas zero. Under 32B keep both
requests as unapplied with reason `finished`; the clock's own transitions are
represented separately. Preserve independent holds, completed milestones and
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
   for that ordinal and checks both legs and balance identities above. Guard
   against legacy and financial requests independently spending the same time
   basis. Finished and held are distinct dispositions with typed evidence.
   Do not encode either as cap limitation. Score/count links keep ordinary basis
   checks; no fabricated ordinals, targets or effect records.
3. **Financial timer continuation.** Add a typed association from the financial
   hold to the reversal and selected time occurrence, and a distinct resolution
   outcome keyed by resolution intent and exact hold/version. Reuse current
   [hold](sqlite-application-holds-api-v1.md) versioning, active-set checks and
   audit shape where compatible. Existing
   associated-hold and resolution-application guards require original-kind
   bundles. A second same-receipt reversal would also violate source uniqueness
   and require a new entitlement link. The continuation advances the same
   application state head when applied and appends audited closure and actual
   time effects with no financial claim or money/basis-head advance. The first
   outcome remains immutable, including its original held disposition. No reuse
   of original or manual correction kind to bypass a guard.

Resolution needs current authority, the actual active origin/version, current
active set and fresh observation. Successful application closes only the
selected financial timer hold atomically with the entire retained adjustment.
An exact retry reports the recorded resolution; a competing or stale resolution
cannot consume it again. The conservative draft guard refuses application if
another active prerequisite would require clearing or bypassing an independent
hold. No side effect on another hold follows from resolving this one.

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
| Later refund while an earlier timer request is pending | Retain later intake and defer acceptance for that payment until the exact hold resolves. Payment position stays fixed meanwhile. This avoids a later debt restoration overtaking held Main. Alternatively accept later proven money and consume the next allocations, with distinct ordered holds and an explicit rule for when each time effect can apply. Later score would apply sooner. 31B decides the first mixed result, not ordering between receipts. |
| Older payment history with unknown money | Mark the payment history unavailable for new entitlement until independently verified reconciliation binds money, requested positions and prior identities. Pristine checked history may start at zero. Alternative is an owner-approved, audited import/reconciliation workflow. Old requests, even full effect consumption, cannot reconstruct money or certify missing identities. Existing records remain readable. |
| Verified identity collision and aliases | Refuse financial mutation and route conflicting evidence for authorized review. Keep raw intake and prior immutable history. To coalesce known mirror receipts, require an explicit verified alias/source-selection authority and define whether callers may receive the old result after current disclosure checks. Neither equal fields nor a second operation key establish aliasing. |
| Held continuation observes hard end | Recommend explicit authorized terminal closure with `finished` reason and zero timer effects, preserving prior score and recording the live finish once. Alternative keeps the hold active for a later separately authorized decision after explicit reopening. 32B settles first application in Finished; it does not settle the fate of an existing hold. |
| Held continuation is still deficient | Recommend keep exact hold/version active and return a prerequisite refusal, so no partial time effect, debt or reauthorization is implicit. Alternative is an explicit authorized override policy defining and recording actual/unapplied time and whether debt may be incurred. It may not shrink the retained request or replay score. |
| Mixed Main deficit and debt restoration under StopAtZero/Carry | Recommend apply the configured deficit rule to the Main leg and restore original repayment as a separate checked leg in the same unit. Example Main 1, request Main −3 and restore +2: Stop yields Main 0, debt +2, unapplied Main 2; Carry yields Main 0, debt +4, of which only 2 is new deficit debt. 31B holds both legs for RequireReview and 32B applies neither in Finished. A different rule withholding restoration changes actual accounting and needs owner approval with #201. |
| Contribution reversal projection | Recommend expose financial fraction separately while the existing effect/review summaries keep their meaning. If public None/partial/full follows accepted money, a full refund may report Full while time remains held or was non-applied. Expose actual/held disposition alongside it. |
| Independent blocking hold | Keep the current conservative active-set guard. Alternatively authorize a selected continuation to apply while another named cause remains active, with an explicit exemption and active-set recheck. This could subtract time sooner without resolving the other cause. |

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
| W01 — Immutable basis | Original `20.0000`, historical rate spelling `0.972500`, requested 600000 time, actual Main 300000, score 40000000, count 7. Change current cap, rate and rules. With sufficient live balances, refund 5, 5, 10. Accepted totals 5, 10, 20. Requests and actual deltas are (−75000,−10000000,0), then the same, then (−150000,−20000000,−7). Full only at 20. Read each immutable receipt/capture/effect link, rule/action version, historical spelling, cap tail, audit and committed state. |
| W02 — Retry and conflict | Commit W01's first refund under K, scoped r1. Advance state with another command. Exact authorized retry returns K's immutable prior result. Changed intent under K, receipt reuse under K2, or another receipt claiming r1 refuses with no financial writes or new publication. Test revoked execution and denied disclosure independently. Prior intake remains. |
| W03 — Exact admission and stale positions | From accepted 19 of 20, two 0.75 proposals share a payment position. One commits at 19.75. The other is stale; freshly evaluated it exceeds original by 0.50. Both raw receipts remain. Exercise stale component and state guards separately with payment position matched. Wrong currency, unknown amount or unverified correlation advances nothing. A separate 0.01 of 1 against actual 1 ms records F = 1/100, C = 1/100, request and actual zero, and advances the accepted-money position. |
| W04 — Atomic closure | Use W05 first application with prior committed intake. Witness staged writes and previous committed state on a separate read connection. Force a reached late SQL write failure. Known rollback leaves accepted money, score, timer, hold, milestone audit/job closure, refund audit and head at prior values. Inject lost acknowledgement of a successful real COMMIT in a fresh unit: unknown, no success publication, then fresh read finds the whole new unit and stable intent once. Separate acknowledged COMMIT followed by body failure: committed/no response, same durable closure. Also force failure after staging resolution effects to check that the active hold and prior committed mixed outcome survive with no partial resolution. |
| W05 — Active 31B | Original 20, Main credit 300000, score credit 40000000. Live Main 50000/debt 0/score 40000000. Refund 5 with RequireReview: score 30000000 once, Main/debt unchanged, hold v1 for all −75000, accepted money 5, requested magnitudes 75000/10000000. Existing unreleased achieved target 35000000 under Review retains achievement, exact deficit 5000000 and one refund review. Separate authorized credit gives Main 100000. Resolve v1: Main 25000, debt 0 and unchanged score, money, basis and milestone decisions. Denied/stale/competing resolutions do nothing; exact retry reads closure. A separate blocking hold survives a refused resolution. Mixed-leg control: W07's second request with Main 0 and RequireReview holds both −1 and +1 while scoring once. |
| W06 — Finished and Ready authority | Same 5-of-20 mixed refund with performed actions. In already Finished, actual timer/debt changes zero, requested Main −75000 explicitly non-applied as `finished`, score 30000000, no timer hold/debt/reopen. Repeat with paused Active at exact hard end and verify committed HardEndReached then Finished and identical refund disposition. Ready gives prerequisite refusal with raw retention only and no started reduction, money or effect. |
| W07 — 33A restoration | Original 8 and actual time addition (Main +3, debt −3). Independent credit leaves live Main 10/debt 0. Refunds 2, 4, 2 yield exact totals 1.5, 4.5, 6, rounded 2, 4, 6 and residuals −0.5, +0.5, 0. Cumulative splits (2,0), (3,1), (3,3) give requests (−2,0), (−1,+1), (0,+2) and live (8,0), (7,1), (7,3). New deficit debt remains zero. Tie control (3,2) at half gives 2 and (−2,0). Full clearing control (6,4), prior (5,0), gives (−1,+4). Read linked histories, including original negative debt delta and distinct actual restoration. |
