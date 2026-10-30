# Event, effect, clock and persistence boundaries — revision 1

Task: CONTRACT-002. Authority: [specification v0.45](../specification.md),
§§2–5 and §8; decisions SERVER-01, DB-01, NUM-01–04 and TIMER-01–10.
This is an internal foundation contract for subsequent bounded test baselines.
It defines ownership, invariants and examples, not implemented behavior, C++ layouts,
SQL tables, provider capability promises or finalized wire schemas. CONTRACT-003
owns HTTP/WebSocket/scene contracts; its exact numeric representation must follow
API-03. The existing [time-credit revision 1](time-credit-v0.md) is unchanged.

## Distinct records and references

The names below are conceptual records. Durable references must remain resolvable
for retained accounting/audit history; storing a version number without the retained
versioned meaning does not preserve provenance. IDs are opaque internal references;
provider identity has its own explicit scope. Unknown source fields remain unknown.

| Record | Owns | Does not imply |
| --- | --- | --- |
| Receipt / delivery attempt | Arrival identity/time, origin/connection, supplied source identifiers, validation/disposition and linkage to an existing or new event | A new contribution or credit for every transport delivery |
| Normalized event | Internal ID, event type/schema version, origin/connection, available provider identifiers/source time/actor, authoritative arrival time, contribution link when applicable, parent/root IDs and captured configuration/context references | Provider payload access in core, financial confirmation or application |
| Contribution | One tracked support activity: evidence, original amount/currency/precision where available, valuation, review, application and reversal dimensions, with linked events/effects | A single combined eligibility status or automatic credit when one condition resolves |
| Application plan | Captured rules/modifiers, matches/exclusions, intended actions/calculation explanation, configuration versions and any explicit override revision | A committed effect, guaranteed future capacity, or permission to apply a stale preview |
| Applied effect | Requested and actual change, target, calculation/rounding/cap detail, rule/action/version, event/contribution/plan links and reversal behavior | That requested and actual amounts are equal, or that presentation/delivery succeeded |
| Reversal effect / entitlement record | Original-effect reference, original valuation/rules, cumulative refund entitlement and prior reversed amount, signed adjustment and chosen deficit/review policy | A fresh award under current rules, deletion of original history or undoing a performed action |
| Delivery job / action group | Captured action definition, source effect/event/group order, sending identity, durable state/attempt evidence and policy/review holds | Internal application can wait for network delivery, or remote exactly-once execution |
| Command result / audit entry | Operation identity, actor, decision/options/reason, checked revision and committed result or conflict | Permission inherited from a client-side preview |
| Clock checkpoint | Durable lifecycle/phase, remaining values, independent pause reasons, separate debt/grace budget and sufficient clock/configuration references for recovery | That display refreshes or stored process-local ticks alone reconstruct a restart |

Receipts/events include non-support inputs such as manual commands, expiry and
schedules. A later confirmation/refund is a distinct linked event, not a second
unrelated contribution. Generated events retain parent/root ancestry through the
chain; a configured generation bound stops further generation while preserving
already committed effects (§6). Retention/privacy projections must not collapse the
private authoritative records into public viewer fields (§§13,16).

## Eligibility, planning and actual application

The independent dimensions in §3.3 remain separate: payment evidence (including
unknown/not applicable), valuation, review, application and reversal. Acceptance
resolves the review decision only. It does not fabricate payment confirmation, a
missing rate or unresolved clock eligibility. A contribution may remain blocked by
several conditions simultaneously. Rejecting review does not erase arrival/history.

Capture relevant arrival-time configuration and modifier context durably before
later edits/expiry can change their meaning. Provider source time supports history
and statistics where reliable; it does not replace server arrival for rule/modifier
selection. Missing valuation is resolved with an explicitly recorded rate; do not
pretend that rate existed at arrival. Authorized overrides create audited revisions.
Plans must explain silent acceptance and its separate Run/Defer/Skip milestone
choice (§5); suppressing ordinary alerts does not suppress internal progression.

At application, recheck authorization, eligibility, operation/contribution identity
and relevant authoritative revision. Resolve actual caps, debt, lifecycle/hard-end
policy and milestone progression against current state. A stale preview must be
revalidated and produce a current result or conflict, never silently commit its old
assumptions (§§3.4,5,18). Exact conflict payloads/revision granularity belong to
CONTRACT-003 and ACCTTEST-004/005; transport validation alone cannot authorize writes.

Preserve money and rate decimals losslessly; use Boost.Multiprecision exact
intermediates. Round each final credited time/score effect half-to-even to signed
64-bit milliseconds/micro-points, independently of display precision. Check final
conversion and aggregate state bounds. Overflow holds the affected input for review
with no effects from that input; neither partial effects nor associated delivery jobs
may escape that failed application. Bounds for parsers, resource use and exact public
error types are defined in CORETEST-001, not guessed here (NUM-01/02/04).

The numeric time-credit helper accepts only its narrow already-eligible active-main
case without debt/hard-end conflict; it does not select lifecycle or transact effects.
Callers must resolve those prerequisites and preserve requested versus actual values.
Reversals reference actual effects. Cumulative partial-refund entitlement minus prior
reversals prevents rounding drift; full refund clears the exact remaining credited
effect and whole count credits reverse only on full refund (NUM-03). Timer subtraction
then follows Stop at zero / Carry debt / Require review. Preserve the full reversal
record, completed milestones and performed actions even when timer application is
held or limited; debt never becomes negative grace time (§§4,8).

## Authoritative queue and transaction ownership

Engineering choice for this revision: one authoritative Qt command/state queue and
its Qt SQL writer execute on one owned thread. Adapters pass owned command/input
values into that queue, never borrowed transport request objects or mutable shared
state. Core takes explicit domain values/clock samples and returns proposals; it
has no Qt, Drogon, SQL or provider dependency. Thread lifecycle and concrete bridge
interfaces are defined/tested under APITEST-001 and STORAGETEST-001 before production.

Drogon handlers authenticate/parse bounded inputs and enqueue work; connectors submit
normalized inputs. Neither handler, connector, client nor delivery callback mutates
authoritative state or directly uses the writer connection. Any separate read
connection belongs to its actual using thread and cannot become a second writer.
Use Qt SQL/SQLite with WAL and synchronous=FULL as required by DB-01. Publish committed
snapshots/results across the boundary; readers must not observe speculative effects.

Durable intake and durable application are distinct stages (§3.4):

1. Persist validated arrival/context plus receipt disposition and event linkage. An
   input waiting on evidence, review or valuation remains durably pending. Provider
   acknowledgment semantics are connector-specific and must not promise durability
   before the relevant intake commit succeeds.
2. Under the writer, perform deduplication/eligibility/revision checks and derive the
   current application result. One transaction commits application status, all
   internal effects and state (including achievements/clock changes), required
   jobs/groups and the retrievable operation result/audit. No successful application
   result or effect publication precedes its commit.
3. If that transaction fails, roll it back as a unit. Durable intake remains pending;
   no partial application, job or advanced application status survives. Retry must
   recheck operation/contribution identity before applying.
4. Dispatch eligible jobs and publish authorized projections only from committed
   state. Delivery transitions/results return through the same writer queue.

Database uniqueness/transaction constraints must protect the same identities as
service checks; thread serialization alone is not crash-safe deduplication. Detailed
schema/migrations and injected-failure acceptance belong to STORAGE-002 and
ACCTTEST-001/004, not this documentation task.

Provider identity is scoped by provider and installation/source account plus the
provider's documented identifier domain; a transport connection instance is retained
as provenance, not a reason to lose deduplication across reconnects. Do not claim an
identifier exists or is stable without provider evidence. Retried deliveries link
back to the same logical activity/application while attempts remain traceable. For
cross-service mirrors, use explicit source selection or verified correlation; equal
names/amounts/times do not establish identity. Source-selection and absent-identifier
policies must be reviewed in API-006/individual connector baselines before claiming
supported duplicate-safe processing. Command operation IDs have an explicit scope
and payload-consistency policy in ACCTTEST-004/API-002; never treat an ID reused for
a different command as authority to apply another result.

## Durable jobs and uncertain outcomes

Persist queued/sending/delivered/failed/outcome-unknown states (§3.4). Capture queued
milestone definitions; edits do not replace them implicitly. Permissioned replacement
of unstarted actions, deferred/review holds, group ordering and stop/replay choices
remain explicit (MILE-01–08). A successful internal commit does not wait for remote
network success and must not be rolled back because a chat message fails.

A crash or timeout after sending can leave delivery uncertain. Do not mark it known
undelivered or blindly resend; use verified provider duplicate-safety mechanisms or
explicit review/retry policy. A provider callback does not bypass queue ownership.
Exact state transitions and retry eligibility require MILESTONE/connector test
baselines. Remote exactly-once behavior is not promised by local transaction atomicity.
Backup restore likewise holds outbound actions and enters Recovery review rather
than silently replaying them (OPS-04).

## Controlled clocks and recovery

Core receives explicit clock observations/elapsed durations; no direct system clock,
wall-clock sleep or UI animation drives authoritative transitions. Tests can advance
clocks independently. Engineering choice: measure in-process elapsed countdown
progress with a monotonic clock; carry UTC wall instants separately for hard end,
source/arrival time, schedules, persisted checkpoints and downtime reconstruction.
Process-local monotonic points are not persisted as portable restart timestamps.
A paired observation identifies its clock domain; never subtract UTC and monotonic
values as interchangeable integers. Concrete clock interfaces and discontinuity
handling require reviewed CORETEST-002/004/006 baselines before implementation.

Persist lifecycle (Ready/active/finished), phase (main/grace), pause-reason set,
nonnegative remaining durations, separate time debt and optional shared grace budget.
Pauses retain phase; clearing one reason cannot clear another. Main/grace and
active-subathon modifier progression follows countdown eligibility; real-elapsed
modifiers have their separate policy. An enabled hard end remains authoritative
during pauses, grace, recovery and review (TIMER-01/09). Ordinary input cannot reopen
a finished event. Reopening needs its own permission, passed-hard-end resolution,
explicit time/grace choices and separate authorization to correct debt (TIMER-06/10).

Save clock state with each timer-changing commit and checkpoint advancing clocks
every second by default (configurable). Freeze restores the last durable checkpoint,
so an abrupt crash may restore already-elapsed time since that checkpoint. Count
elapsed reconstructs schedule/phase/deadline progression from the checkpoint; it is
not merely remaining minus wall gap. Recovery review freezes main/grace and
active-subathon modifiers while the decision waits; real-elapsed modifiers and hard
end continue. Hold arrivals/unresolved clock eligibility, preview Freeze/Count elapsed
for the offline gap and record any authorized correction. Decision-wait time remains
paused and resolving recovery clears only Recovery review (TIMER-04/08/09).

Recurring schedules use the configured subathon timezone with UTC occurrences.
Default gap handling shifts forward by the DST gap (Skip alternative); repeated
local time runs once (Run twice alternative). Preview future occurrences; timezone
changes do not replay history (TIMER-07). Clock discontinuities, ambiguous/offline
observations and combined schedule/phase transitions require explicit tests in
CORETEST-004/006 and STORAGE-003. This contract does not select an unreviewed
clock-skew tolerance or invent elapsed time from invalid observations.

## Specification-derived examples for downstream baselines

These expected results are manually derived from the cited requirements; they are
contract examples, not authored executable tests or observed results. Times are
whole milliseconds unless a readable minute/second unit is shown.

| ID / input sequence | Required observable boundary result | Authority / primary baseline |
| --- | --- | --- |
| B01: same identified payment delivered twice, then later confirmation | Attempts remain traceable; one contribution; confirmation links to it; at most one application once every condition resolves | §3.2–4, §18 same input twice; ACCTTEST-001/002 |
| B02: paused €20 input has unknown payment evidence and missing rate; reviewer accepts | Review is accepted; unknown evidence and missing valuation remain; no effects/jobs yet; resolving one does not resolve the other | §§3.3,5; ACCTTEST-002/005 |
| B03: input arrives with ×2 modifier, waits; modifier expires and rules are edited before acceptance | Original captured rule/modifier context governs unless authorized audited override; actual capacity comes from current state | §§3.4,5,18 modifier expires; ACCTTEST-003 |
| B04: plan requests +10 min at 55 min/cap60; another command reaches 58 min before application | Stale preview is revalidated; if applied with current capacity, requested +600000 and actual +120000 leave 3600000; never apply the old +300000 | §§3.4,5; ACCTTEST-004/005 |
| B05: original €20 credited +300000 ms and +40000000 micro-points; €5 refund, then remaining €15 refund | First reversal entitlement is 75000 ms/10000000 micro-points; second reverses exactly remaining 225000/30000000, linked to original effects/rate; current rule edits do not recalculate | §8, NUM-03; ACCTTEST-006 |
| B06: original 1 micro-point; cumulative refunded fraction 1/3, then 2/3, then full | Half-even cumulative entitlements are 0,1,1; incremental reversals 0,1,0. Whole-count credit stays until full refund | §§3.5,8; ACCTTEST-006 |
| B07: two authorized reviewers submit acceptance for one contribution | One atomic application; the other gets existing current result/conflict. A stale preview cannot create additional effects/jobs | §§5,18; ACCTTEST-004/005 |
| B08: receipt is committed, then application transaction fails before commit after preparing an effect/job | Receipt stays pending; no prepared effect/job/state/application status survives; retry can apply once | §3.4, DB-01; ACCTTEST-004, STORAGETEST-002 |
| B09: remote send may have happened but process stops before delivery result is durable | Applied effects remain; job outcome is uncertain and is not blindly replayed on restart | §§3.4,8,18; milestone delivery baseline |
| B10: final credited result or aggregate would exceed signed64 bounds | Preserve input/provenance and hold for review; no effect or associated delivery job from that failed application, no wrap/saturation | §3.5, NUM-02/04; CORETEST-001, ACCTTEST-004 |
| B11: durable main checkpoint 600000 at 12:00Z, restart12:05Z; no schedules, pauses or hard end | Freeze restores600000; Count elapsed yields300000; Review holds main/grace while awaiting choice, with hard end still authoritative | §4, TIMER-04/08/09; CORETEST-006 |
| B12: Manual and Recovery review pauses overlap; review resolved after two minutes waiting | Waiting interval does not count down; only Recovery review is removed; Manual still freezes main/grace | §4, TIMER-09; CORETEST-003/006 |
| B13: main/grace is paused when enabled UTC hard end is reached | Finish despite pause/grace; no countdown pause extends deadline | §4, TIMER-01; CORETEST-002/008 |
| B14: schedule's selected local occurrence falls in a one-hour DST gap; later repeated local occurrence occurs twice | Default shifts missing occurrence forward one hour; default repeated occurrence executes once, while chosen Skip/Run twice alternatives differ explicitly; timezone edits do not replay past occurrences | §4, TIMER-07; CORETEST-004 |

## Revision, downstream work and acceptance boundary

Revision 1 introduces no persisted schema migration and changes no implemented
public boundary. It supplies the shared invariants above; it does not finish an
executable baseline or authorize dependent production implementation without its own
reviewed tests and observed intended red. CONTRACT-003 may refine transport/version
representations; bounded core/accounting/storage/API baselines refine local interfaces,
limits, errors and clock/transaction examples while preserving these invariants.
A conflict with the specification must be recorded and reviewed, not resolved by
silently reducing requirements.

All seven integrations, all three rate providers, supported platforms/renderers,
UI/localization, packaging and live/platform validation remain in release scope.
No provider capability or production clock/SQLite correctness follows from this
document or the bounded SPIKE-001 experiment. That spike's WebSocket close limitation,
persistent/multi-message connections, bounded shutdown/backpressure/startup signals
remain explicit APITEST-001/003 gates. See [its handoff](../evidence/spike-001-bridge-validation.md).

CI assessment: this task adds documentation only, with no executable suite, module,
dependency or workflow change. Existing project/link/progress checks apply; reviewed
credit CI continues unchanged. Future baseline/implementation tasks must add their
new stable suites to CI under [the evolving-CI policy](../AGENT_WORKFLOW.md#evolving-continuous-integration).
Independent review of this exact document revision is required before task completion;
publication/CI/merge status is recorded by the coordinator separately.
