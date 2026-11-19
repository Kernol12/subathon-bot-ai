# SQLite financial refund entitlements — revision 2 draft

Tier A, ACCT-006 [#199](https://github.com/Kernol12/subathon-bot-ai/issues/199),
[draft PR #726](https://github.com/Kernol12/subathon-bot-ai/pull/726). Stage 1,
fix rounds 0 of 2. Specification v0.45 §§3, 4, 8 and NUM-03.

The registered A01–A15 core source and assertions at `06dbc980` stand as reviewed. Their
[observed red](https://github.com/Kernol12/subathon-bot-ai/pull/726#issuecomment-7011613636)
covers calculation. W01–W07 give independent writer oracles.

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
requested allocations. Financial acceptance claims receipt, economic refund and
operation identities in the same unit. Bind the acceptance operation
key to complete stable command intent, including the financial receipt, verified
correlation, selected plan/capture, expected positions/state and explicit
choices or overrides. Compare canonical intent and retained facts including
optionality. Exact authorized acceptance retry returns its immutable outcome
even after the head changes. Conflicting key, receipt or economic identity
reuse refuses with no financial writes or publication. Raw intake and its
attempts are independently durable.

After identity checks, revalidate payment position, each selected basis
position, actual plan/source and authoritative state/contribution independently.
Each incoming amount is positive and in the verified original currency. Original
amount is positive, prior accepted total is nonnegative and the sum cannot exceed
original. An over-original refund is refused in full. Exact equality is full.
An accepted refund with zero rounded request still consumes its identity, exact
money and a position. Never infer payment totals from effects, legacy link
fractions, CID alone or receipt counts.

Let F be cumulative accepted money after this refund divided by original money.
Every selected time, score and count link in this refund uses that same F.
For each eligible actual credit X, retain C = X × F, rounded cumulative
magnitude R and residual C − R. Time and score use half-even at ms and
micro-point precision. A count holds R
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
debt restoration request is nonnegative. On a started timer with no earlier
pending request for this payment, RequireReview holds both legs with zero actual
timer deltas when the Main request's magnitude exceeds live Main at the fresh
observation. A restoration-only request and exact exhaustion of Main apply
without a deficit hold. A prior request invokes 34B. Under 32B keep both requests
unapplied with reason `finished` and represent the clock's own transitions
separately. Preserve independent holds, completed milestones and
performed actions. Any effect on unreleased action policy is driven by actual
score before/after at acceptance, with the stable refund ref. Continuation
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
   linked to the real reversal operation, refund receipt and original. Stage a
   deterministic financial capture in the same unit, bound to admitted inputs
   and the actual current plan and positions. The capture instruction uses the
   finalized integral request, so exact and final agree without rounding the
   increment again. Keep the exact cumulative entitlement and residual in the
   typed provenance. A separately committed candidate would still need final
   revalidation and audited replacement. Never silently change it on retry.
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
3. **Financial timer continuation.** Add a `FinancialTimerAdjustment` cause on
   the original contribution's CID as `HoldSubject`. Its typed reason is
   `deficit` or `prior_request`. It retains the exact signed Main and debt
   restoration requests, prior and next basis allocations, selected time
   occurrence, payment position and financial refund operation. A
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
Bind the choice into the immutable intent and audit, even when Main covers the
request and no deficit results. #199 validates and records closure. #201 applies
the whole retained request once with separate checked restoration, and reports
actual, unapplied and deficit-debt components. Preserve score, money and
milestone policy history.

Successfully applying the retained adjustment closes its exact financial timer
hold in the same unit. Only the typed continuation can close these holds. It
checks the complete expected active set and closes just the oldest request for
that payment. An independent nonfinancial application hold on the same CID
blocks this continuation. Ordinary application-linked resolutions compare the
complete active set and close all its nonfinancial entries while preserving
financial holds. Ordinary resolution or replacement cannot consume a financial
hold. Milestone Review lives in its own store and does not block continuation.
An exact retry reports its immutable resolution. A competing or stale
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

## Ordered timer requests, Finished continuation and catalog order

**[34B](https://github.com/Kernol12/subathon-bot-ai/pull/726#issuecomment-7014595148).**
For new acceptance, check the complete active set for the original CID. Only
`FinancialTimerAdjustment` holds for the verified payment pass this gate. If
any other hold is active, return `active_hold` with raw intake preserved and
no claim, financial advance, effect or hold write. Otherwise claim identities
and commit money, requested entitlement, score/count and milestone policy once.
While the timer has not Finished, an earlier unresolved request makes a request
with either leg nonzero a distinct `prior_request` hold with zero timer deltas,
even when Main would cover it. With both legs zero, no new hold. For Finished,
use 32B. An exact retry returns its original outcome and hold identity.

Use committed payment position to order requests, including after store reopen. Only
the oldest can resolve, with fresh authority, state and hold checks. Out-of-order
attempts leave effects and holds intact. Closure preserves later requests for
separate authorized resolutions. Reconcile uncertain completion before another
application. [#201 wires live composition](https://github.com/Kernol12/subathon-bot-ai/issues/201#issuecomment-7012411967).

**35A.** If an authorized continuation observes Finished for any reason, close
the exact hold with an audited `finished` disposition and zero timer adjustment.
Where the same observation first detects finish, record the transition with
closure. This applies after Main, grace, confirmation, hard end or an authorized
deadline edit. Preserve prior score, money, requested progress and any later
holds. A retry returns the immutable closure.

**36A.** #215 implements the reviewed v14 boundary first. Assign #199's
additive catalog version against protected main after #215 merges. The
independent v13 writer-only red precedes durable integration.

Unknown older payment history stays unavailable pending verified reconciliation.
Only pristine checked history starts at zero. Conflicting economic identities
refuse financial mutation and preserve raw intake and prior history. Verified
alias selection belongs to its source authority. The financial journal exposes
accepted money and fraction separately from existing effect and review summaries.
In #201's live Stop/Carry composition the Main deficit rule applies only to the
Main leg. Restore original repayment separately in the same checked unit.

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
| W07 — 33A restoration | Original 8 and actual time addition (Main +3, debt −3). Independent credit leaves live Main 10/debt 0. Refunds 2, 4, 2 give accepted totals 2, 6, 8 and exact cumulative time 1.5, 4.5, 6, rounded 2, 4, 6 and residuals −0.5, +0.5, 0. Cumulative splits (2,0), (3,1), (3,3) give requests (−2,0), (−1,+1), (0,+2) and live (8,0), (7,1), (7,3). New deficit debt remains zero. The third, restoration-only request applies under RequireReview. An independent second-request fixture at Main 1/debt 0 with RequireReview ends at (0,1) without a hold. Tie control (3,2) at half gives 2 and (−2,0). Full clearing control (6,4), prior (5,0), gives (−1,+4). Read linked histories, including original negative debt delta and distinct actual restoration. |

W05 ordered control: after holding the timer for the refund of 5, raise Main to
200000. Accept 2 and then 3 while the first hold remains. Money reaches 7 and
10 and score reaches 26000000 and 20000000 at acceptance. Main stays 200000.
Reopen the store and refuse a fresh out-of-order continuation. Resolve the requests
75000, 30000 and 45000 in payment order. Main reaches 125000, 95000 and 50000.
Score and money stay fixed. Exact acceptance and resolution retries preserve
their respective historical results. When an independent application hold
blocks acceptance and continuation, resolve it and preserve the financial
hold. Retry the authorized financial continuation against the fresh active set.

W06 continuation control: finish a timer with W05's hold active, once through
hard end and once through ordinary Main expiry. Authorized continuation closes
v1 with `finished` and zero timer effect, leaving score 30000000 and accepted
money 5. Retry preserves the same closure. In the ordered control, Finished
closure leaves later requests active for their own authorized continuations.
