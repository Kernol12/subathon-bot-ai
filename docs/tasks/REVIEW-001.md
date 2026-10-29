# REVIEW-001 — Independently review the acceptance baseline

Module: Core. Kind: test_review.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: TEST-04, TEST-05.

## Goal

A separate reviewer checks contracts, oracles, failure cases and baseline integrity.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- A reviewer other than the test author checks contract, oracles and missing boundary/failure cases.
- Record author/reviewer identities and baseline revision in docs/evidence/ using REVIEW.md.
- Resolve change requests. No self-approval and no feature implementation.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
