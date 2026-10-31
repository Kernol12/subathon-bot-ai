# CORE-003 — Independent pause reasons and delayed status signals

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §4, TIMER-03, TIMER-05.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: [pause-policy proposed revision 1](../../contracts/pause-policy-v1.md).
The draft is not a finalized reviewed/red baseline; see [authoring evidence](../../evidence/coretest-003-authoring.md).

## Scope and ownership

Core pause-reason set, offline/client signal delays and unknown-status policy; provider transport is excluded.

The proposed bounded value API supplies `set_pause_reason` and `observe_pause_signals`;
the latter receives normalized constant stream/client segments and explicit monotonic
elapsed duration. It reads no system clock or UTC and returns no authoritative state
write. Countdown integration, schedule occurrence calculation, recovery decisions,
configuration changes during a pending episode and Qt/network/provider behavior
remain their separately assigned gates. The coordinator owns all root/module build
files, presets, shared progress/HTML, acceptance map and publication; the test author
owns only the new contract/header/stub/tests, this brief and authoring evidence.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Default offline and designated-client delays independently to 30 seconds; countdown continues while the delays are pending.
- Recovery clears only its signal reason; unknown status retains current state by default and cannot clear confirmed-offline or manual pauses.

## Boundary and failure cases

Test overlapping reasons, flapping signals, exact delay expiry and optional unknown-status timeout with a fake clock.

Thirty draft cases now cover the independently specified behavior using explicit
durations, with successful configure/build/discovery only. Both functions remain
unconditional `PausePolicyNotImplemented` stubs. Two combined-policy edges lack
repository authority: Unknown interrupting an unexpired offline delay, and clearing/
resetting the optional Unknown pause after known status returns. The
[recorded policy questions](../../evidence/pause-signal-policy-questions.md) require
owner clarification, then affected examples/cases and independent review. Existing
cases do not select either policy; full baseline completion and production behavior
wait for those gates. This leaves the full intended release behavior required.

## Separate baseline and implementation tasks

- `CORETEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `CORE-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest via CTest; controllable clock and exact reference vectors.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
