# CORE-002 — Main countdown and ending-mode transitions

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §4, TIMER-01, TIMER-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: [timer lifecycle revision1](../../contracts/timer-lifecycle-v1.md),
independently reviewed with39 cases and observed intended stub-red; see the
[baseline handoff](../../evidence/coretest-002-baseline-handoff.md).
The [owner clarification](../../evidence/timer-confirmation-credit-decision.md)
settles confirmation-wait credit. CORE-002 now implements the bounded reducer;
[implementation evidence](../../evidence/core-002-handoff.md) records39 passing cases,
separate review and observed CI. No API or layout migration was required.

## Scope and ownership

Core lifecycle reducer with explicit nonnegative elapsed milliseconds and a separate
UTC observation; persistence wiring belongs to STORAGE-003. Return both a snapshot
and ordered phase/finish proposals so a long elapsed step cannot lose Main/Grace
transitions. No Qt/system-clock/provider/service access occurs in this primitive.

This boundary advances already-started Active/Finished snapshots. Ready is represented
but is not a start command; first-start/deadline behavior remains an explicit
API-007/CORE-008 gate. Reopening, independent pause reasons/status delays, schedules,
recovery and signed manual/configuration changes remain their own bounded contracts.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Represent ready/active/finished and main/grace separately; default to two-minute grace then finish, with immediate and confirmation modes.
- Rescue from grace only after positive applicable credit; implement fresh-grace default and optional shared budget without consuming grace while main runs.
- Carry unpaused elapsed overshoot through Main and Grace in one step, emitting
  ordered transitions through finish. Freeze both clocks under an external paused
  observation while retaining hard-end precedence.
- Specify UTC versus monotonic-duration domains, advance-before-command ordering and
  local snapshot/validation choices explicitly; do not invent cross-domain event
  timestamps or a clock-skew policy.
- Positive actual credit while awaiting finish confirmation automatically resumes
  Active/Main and clears the wait, retaining Grace and independent pause reasons.
  Zero actual credit keeps waiting; eligibility/debt/caps are resolved by the caller.
  A stale confirmation rejects after resumption, and advancing to an enabled hard
  end first produces Finished so subsequent credit cannot reopen it.

## Boundary and failure cases

Exercise zero crossings, repeated expirations, exact timestamp boundaries and hard-end precedence during running, pause and grace.

## Separate baseline and implementation tasks

- `CORETEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `CORE-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest via CTest; explicit clock-step values and manually derived
duration/transition vectors. Coordinator-owned `timer-lifecycle-debug` configures
the stable passing lane, also executed automatically on PRs/main. The original
reviewed/red gate is retained in baseline evidence; preserve its assertions.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
