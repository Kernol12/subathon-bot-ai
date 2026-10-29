# BOOT-004 — Task branch and PR workflow handoff

Date: 2026-10-30. Author: Codex. Branch: TEST_PR (explicitly requested by owner).
Specification: v0.45, §17. Scope: documentation and progress only.

The owner approved one branch per executable task and authorized PR creation and
merging after required checks and applicable quality/review gates. AGENTS.md now
states that rule; docs/AGENT_WORKFLOW.md defines branch naming, PR contents and the
merge checklist. Epics remain planning containers split into focused tasks. Existing
independent-review, test-first and product-scope requirements are preserved.
Branch protection settings were not changed.

Environment: same Arch Linux x86-64 toolchain as initial-publication.md.
Commands observed, all exit 0:

- `python3 tools/progress.py render`
- `python3 tools/check_project.py` — 42 tasks, 122 decision IDs; links/presets/HTML pass.
- `python3 -m unittest discover -s tests/tooling -v` — 12 tests pass.
- `python3 tools/progress.py render --check` — current snapshot.
- `cmake --preset scaffold-debug` — configured.
- `cmake --build --preset scaffold-debug` — built, no compilation changes.
- `ctest --preset scaffold-debug` — 3/3 pass.

No production behavior or baseline assertions changed. No new tests are warranted
for the documentation policy. No independent reviewer is claimed; this documentation
change does not complete any product implementation or acceptance-review gate.

Publication gate: SSH push access is available, but GitHub CLI, CLI auth configuration
and GitHub API token environment variables were absent when checked. PR creation,
remote checks/reviews and merging await API authentication. The owner was asked to
install `gh` and run `gh auth login` locally, without sharing credentials in chat.
Leave BOOT-004 in review until the PR and its required checks/gates are handled.

Changed paths: AGENTS.md, docs/AGENT_WORKFLOW.md, docs/tasks/BOOT-004.md, this note,
progress/state.json, generated PROGRESS.html.
Next action: create the PR from TEST_PR to main; inspect latest-revision CI,
branch protection and review status, then merge when eligible.
