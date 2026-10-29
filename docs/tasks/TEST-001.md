# TEST-001 — Review and expand the draft credit tests

Module: Core. Kind: test_draft.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: §17, §18, TEST-04.

## Goal

Test-author task: independently derive expected values and extend meaningful boundary cases.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Review six draft GoogleTest assertions against the contract/specification.
- Add meaningful invalid/boundary cases after their contract is defined; no guessed behavior.
- Expected values are independently derived. Do not calculate them with production code.
- Hand authored cases to REVIEW-001. Drafting can finish before baseline review; this is not a ready implementation gate.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
