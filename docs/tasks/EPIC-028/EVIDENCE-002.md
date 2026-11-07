# EVIDENCE-002 — Enforce concise task notes and current suite summaries

Module: Quality and tooling. Kind: cleanup. Specification: v0.45; §§17–18.
Parent: EPIC-028. Depends on: EVIDENCE-001. Shared-file ownership: `progress/state.json`,
`tools/progress.py`, `tests/tooling/test_progress.py`, `docs/TEST_STRATEGY.md`, and
`PROGRESS.html`.

## Scope

Shorten every task's combined `notes` to no more than 500 characters without dropping
open gates, evidence level or the next action. Keep the notes useful as an at-a-glance
outcome, not a second PR report. Reduce `docs/TEST_STRATEGY.md` to the current state and
meaningful coverage of each suite; remove task-by-task history while retaining active
test guidance and requirements. Detailed results remain in PRs, and area-level evidence
claims remain in `docs/evidence/README.md`.

## Acceptance criteria

- Every task in `progress/state.json` has a combined notes length of at most 500
  characters; retain relevant open gates and do not claim unrun checks or unsupported
  behavior.
- `tools/progress.py validate` rejects any task, whatever its status, whose concatenated
  notes exceed 500 characters. Because `progress.py set` appends its note and validates
  before writing, it refuses an over-limit note without changing state; its error tells
  the agent to shorten the task's notes in JSON. Add focused tests for the boundary and
  over-limit rejection without weakening unrelated validation.
- `docs/TEST_STRATEGY.md` describes current suite purpose, coverage, and gaps without
  preserving per-task completion history; update its links and statements to match the
  current state.
- Regenerate `PROGRESS.html`; `progress.py validate`, `render --check`, the tooling tests,
  and `tools/check_project.py` pass.
- A separate reviewer checks note quality for retained gates, the validator boundary,
  and the current-suite summary before approval. Keep the resulting task note within
  500 characters and update the matching area summary per the workflow.

## Verification and handoff

Record exact commands and results in the PR. This is evidence/readability maintenance;
it must not alter test assertions, verification outcomes, or product scope.