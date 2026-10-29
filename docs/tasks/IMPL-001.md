# IMPL-001 — Implement reviewed cap-limited credit

Module: Core. Kind: implementation.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: §3, §4, §18.

## Goal

Implement only the reviewed contract, preserve tests and record passing acceptance plus independent review.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Implement only the reviewed time-credit contract after TEST-002 is done.
- Preserve reviewed acceptance cases; legitimate test changes require separate review and rationale.
- Pass acceptance/regression checks and arrange an independent code review.
- Record exact evidence and remaining gates; do not extend into refunds, persistence or UI without tasks.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
