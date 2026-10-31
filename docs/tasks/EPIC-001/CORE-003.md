# CORE-003 — Independent pause reasons and delayed status signals

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §4, TIMER-03, TIMER-05.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: [pause-policy revision 1](../../contracts/pause-policy-v1.md).
The37-case baseline is independently reviewed and observed red; see
[handoff](../../evidence/coretest-003-baseline-handoff.md). The subsequent bounded pure implementation passes37 cases with separate code review
and observed CI; [implementation handoff](../../evidence/core-003-handoff.md).

## Scope and ownership

Core pause-reason set, offline/client signal delays and unknown-status policy; provider transport is excluded.

The proposed bounded value API supplies `set_pause_reason` and `observe_pause_signals`;
the latter receives normalized constant stream/client segments and explicit monotonic
elapsed duration. It reads no system clock or UTC and returns no authoritative state
write. Countdown integration, schedule occurrence calculation, recovery decisions,
configuration changes during a pending episode and Qt/network/provider behavior
remain their separately assigned gates. The coordinator owns all root/module build
files, presets, shared progress/HTML, acceptance map and publication; baseline authors
supplied the contract/header/stubs/tests/brief/evidence, while the implementer owns
only source and implementation evidence.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Default offline and designated-client delays independently to 30 seconds; countdown continues while the delays are pending.
- Recovery clears only its signal reason; unknown status retains current state by default and cannot clear confirmed-offline or manual pauses.

## Boundary and failure cases

Test overlapping reasons, flapping signals, exact delay expiry and optional unknown-status timeout with a fake clock.

Thirty-seven reviewed baseline cases cover the independently specified behavior using explicit
monotonic durations. The original30 cases remain unchanged; seven focused additions
encode the [recorded owner decisions](../../evidence/pause-signal-policy-questions.md):
Unknown cancels pending Offline, later Offline receives the full fresh delay, and
both known statuses clear/reset Unknown with a fresh timeout on each new episode.
Established Offline survives Unknown and known Offline. Known Online independently
recovers Offline; connected-client recovery independently clears its reason. External
and unrecovered holds remain. No product-policy question remains.
Full independent review86c0bbd preceded actual37-case stub-red atf39ccc8 and
baseline PR20 merge. CORE-003 subsequently implemented both functions;37 unchanged
cases pass with separate code review and automatic CI. Intended release scope remains.

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

Historical baseline gate: full review86c0bbd approved the complete37 cases, then actual
CTest atf39ccc8 failed37/37 on explicit stubs. Original30 bodies/header remain
unchanged; explicit stubs are retained in baseline history. CORE-003 now passes these cases with separate code review and observed automatic
pause PR/main CI; final publication/merge and downstream integration remain separate.
