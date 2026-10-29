# TEST-002 — Demonstrate the intended failing baseline

Module: Core. Kind: test_baseline.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: TEST-04.

## Goal

Build GoogleTest suite; record FeatureNotImplemented failures rather than dependency/build errors.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Configure/build acceptance-debug successfully with real GoogleTest.
- Run ctest --preset acceptance-debug -L acceptance; preserve exact output/revision.
- Demonstrate intended missing-behavior failures; dependency/build/empty/skipped tests do not count.
- Link independent review and observed_red evidence. Hand off to IMPL-001 without implementing it.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
