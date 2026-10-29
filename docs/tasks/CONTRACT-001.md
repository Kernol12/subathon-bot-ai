# CONTRACT-001 — Review the draft time-credit boundary

Module: Core. Kind: contract.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: §3, §4, §18.

## Goal

Review valid examples and define invalid-input/overflow behavior before implementation.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Independently review docs/contracts/time-credit-v0.md and six expected-result examples.
- Define checked invalid-input/overflow/error behavior and separate lifecycle/debt responsibilities.
- Update interface, contract and test plan together; do not implement credit calculations.
- Identify exact spec links and unresolved policies before assigning tests.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
