# EVIDENCE-001 — Archive detailed receipts and publish a readable summary

Module: Quality and tooling. Kind: cleanup. Specification: v0.45; §§17–18.
Parent: EPIC-028. Dependencies: none. Shared-file ownership: `docs/evidence/`, references
to its contents in Markdown, `progress/state.json`, `tests/acceptance-map.json`,
`tools/check_project.py`, `tests/tooling/test_progress.py`, and the receipt-path comment
in `tests/acceptance/time_credit_tests.cpp`.

## Scope

Preserve the current detailed receipts at the annotated git tag
`evidence-archive-2026-11`, then remove them from the working tree and replace them with a
concise, human-readable area summary in `docs/evidence/README.md`. The summary should be
about 300 lines or fewer and state for each area what is proven, the evidence level
(simulated, real SQLite, CI, live/platform), open gates, and links to relevant PRs. Do
not upgrade a claim beyond its recorded evidence. Keep all existing receipt details
available through the tag and PR history.

## Acceptance criteria

- Create an annotated tag at the current `main` commit containing all currently tracked
  `docs/evidence/` receipts, verify every deleted path is present at that tag, and push
  the tag before merging the cleanup PR. Do not overwrite an existing tag.
- Replace the bulky evidence files in the PR with `README.md`; the area summary is about
  300 lines or fewer and describes evidence and remaining gates without claiming product
  behavior that is not proven.
- Replace references to removed local receipt paths in `progress/state.json`,
  `tests/acceptance-map.json`, Markdown documents and the comment in
  `tests/acceptance/time_credit_tests.cpp` with links to the archive tag or the relevant
  PR. Leave non-evidence test paths local. `BASELINE_MANIFEST.json` is a historical
  baseline and keeps its entries.
- `tests/tooling/test_progress.py` uses `docs/evidence/baseline-validation.md` as an
  existing local evidence file. Point those tests at a file that remains tracked; change
  only the fixture path, not the assertions.
- Update `tools/check_project.py` to accept valid HTTPS references in acceptance-map
  `evidence` entries while still requiring local test artifacts to exist. Require the
  tracked `docs/evidence/` tree to contain only `README.md` after the archive.
- Validate all references, progress state and generated HTML; the tooling tests and
  `tools/check_project.py` pass. Verify after merge that each removed receipt remains
  accessible from the pushed tag.
- A separate reviewer verifies the tag target/content, summary claims, rewritten links,
  and check behavior before approval. Keep the task note within 500 characters and
  update the matching area summary as specified by the workflow.

## Verification and handoff

Record exact commands, tag revision, PR and CI results in the PR. This task archives
historical receipts; it does not change product behavior or erase repository history.