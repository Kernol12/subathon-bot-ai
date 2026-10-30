# TWITCH-004 — Explicit bot and broadcaster message delivery

Parent: [EPIC-010](../EPIC-010.md). Kind: implementation.
Specification: v0.45; §10, CHAT-01, CHAT-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Outbound Twitch chat adapter and sender capability resolution for rule/command/milestone jobs.

Primary module paths: integrations/ Twitch and server/ delivery/auth adapters
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Support configured default, explicit bot and explicit broadcaster per message, with separate permission checks and sender audit.
- Default unavailable sender to visible hold; fallback only if explicitly authorized and known undelivered.

## Boundary and failure cases

Uncertain outcome requires review; never silently switch identity or blindly retry a possibly delivered message.

## Separate baseline and implementation tasks

- `TWITCHTEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `TWITCH-004` (implementation): depends on that completed baseline and its other
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
