# TWITCH-003 — Chat ingress and stream-state signals

Parent: [EPIC-010](../EPIC-010.md). Kind: implementation.
Specification: v0.45; §10, §14, TIMER-03, TIMER-05.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Incoming chat command/category adapter and known-online/offline/unknown stream signal adapter.

Primary module paths: integrations/ Twitch and server/ delivery/auth adapters
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Forward normalized chat to the command evaluator with stable identity and configured cooldown/authorization paths.
- Feed authoritative stream-state signals into independent delayed pause reasons without treating unknown as offline or online.

## Boundary and failure cases

Test reconnect, duplicate command delivery, flapping/unknown status and stale category information with fake transport.

## Separate baseline and implementation tasks

- `TWITCHTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `TWITCH-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: CTest adapter tests with synthetic/redacted fixtures and fake HTTP/WS/chat.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
