# TWITCH-002 — EventSub subscriptions gifts and Bits normalization

Parent: [EPIC-010](../EPIC-010.md). Kind: implementation.
Specification: v0.45; §14, §3.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Twitch receipt/event normalization and authoritative source selection; no direct timer/SQL writes.

Primary module paths: integrations/ Twitch and server/ delivery/auth adapters
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Map documented subscription/gift/bundle/Bits fields with IDs, actor/source time/evidence and unknown classifications preserved.
- Prevent overlaps and gift-bundle double counting; distinguish new-sub events from shared resub notifications and renewal-payment gaps.

## Boundary and failure cases

Cover duplicates/out-of-order events with redacted fixtures; a configured subscription value is not an actual cash receipt.

## Separate baseline and implementation tasks

- `TWITCHTEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `TWITCH-002` (implementation): depends on that completed baseline and its other
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
