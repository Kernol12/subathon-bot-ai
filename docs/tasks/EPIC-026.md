# EPIC-026 — Target platform/browser/OBS matrix and performance evidence

Module: Release. Kind: epic.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: §19.

## Goal

Planning container: split into reviewed contract/test-first implementation tasks before assigning production work.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

This is a planning container, not a single implementation task. Decompose it into
bounded contract, test-authoring/review/red-baseline, implementation and verification
tasks. Include failure paths and all relevant spec policies. Assign no whole-epic code
task to an agent. The epic completes only when its required children and explicit
live/platform/release gates are evidenced; unknown capability must remain visible.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
