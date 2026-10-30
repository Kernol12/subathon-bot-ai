# TWITCH-090 — Verify Twitch grants events senders and gap limitations live

Parent: [EPIC-010](../EPIC-010.md). Kind: environment.
Specification: v0.45; §14, §19, AUTH-15, CHAT-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Separately scheduled owner-authorized test installation/account exercise; synthetic checks cannot complete this gate.

Primary module paths: integrations/ Twitch and server/ delivery/auth adapters
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record actual authorization mode/scopes/token types, EventSub identifiers/overlaps, stream state and permitted sender outcomes.
- Document observed gaps/unknowns and current provider documentation with redacted evidence and exact versions/date.

## Boundary and failure cases

No unsolicited chat or real contribution processing; absent accounts/permissions remain blocked rather than assumed supported.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: CTest adapter tests with synthetic/redacted fixtures and fake HTTP/WS/chat.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
