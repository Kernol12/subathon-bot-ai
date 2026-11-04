# CONTRACT-003 — Define HTTP, WebSocket and scene contracts

Module: Server/API. Kind: contract.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: API-01, API-02, API-03, §11.

## Goal

OpenAPI 3.1/WS/schema/capability contracts with exact numeric strings and explicit shared-file ownership.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Refine this task's bounded acceptance criteria before editing dependent production code.
- For a feasibility task, identify protocol/platform questions, reproduce the experiment and record limitations.
- For a contract task, define versioned examples and error cases before downstream implementation.
- For scaffolding/environment, distinguish successful checks from feature correctness.
- Use the applicable template, record exact evidence and leave a durable handoff.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.

## Required child contracts and integration audit

[ADR-0004](../decisions/0004-wire-contract-prerequisites.md) proposes separate
[common primitives](CONTRACT-005.md), [HTTP/OpenAPI](CONTRACT-006.md),
[persistent WebSocket](CONTRACT-007.md) and [scene](CONTRACT-008.md) contract tasks.
All four remain required; this parent audits their consistent versions, imports,
capabilities, complete packages and independent examples. Common documentation alone
cannot finish this contract. Read current state/evidence for actual review status.
The split changes helper sequencing only; original API-01/02/03 and §11 scope stays.
