# BOOT-007 — Coordinator prompt handoff

Author: Codex /root. Date: 2026-10-30.
Branch: task/BOOT-007-coordinator-prompt, based on main 16b9d6d.
Specification: v0.45 §17, DEV-02, TEST-04, TEST-05; product contracts unchanged.

The owner requested a repository Markdown coordinator prompt and copyable starting
and continuation prompts. docs/COORDINATOR_PROMPT.md provides all three. README.md
links it without replacing FIRST_AGENT_PROMPT.md. The coordinator must reconcile
completed preparation against current evidence rather than restart the initial phase.

The standing loop explicitly delegates bounded work, isolates concurrent writers,
preserves independent test/code review and actual red/pass evidence, and publishes
one focused PR per executable task. Eligible merges lead to the next task. The
document preserves all release scope and GitHub merge gates, including required
GitHub approvals beyond repository review evidence.

The future checkpoint is docs/evidence/coordinator-handoff.md. It is deliberately
not created here: no continuous implementation run or assignments have begun.
Starting/resuming instructions require inspection of real worktrees, uncommitted work,
agents/processes and PR state. A stopped session does not continue automatically.

Validation on local Linux, Python 3.14.8 and Node v24.19.0:

- `python3 tools/progress.py render` — exit 0, snapshot regenerated.
- `python3 tools/progress.py render --check` — exit 0, current snapshot.
- `python3 tools/check_project.py` — exit 0, 376 task records and 122 decisions.
- `python3 -m unittest discover -s tests/tooling -v` — exit 0, 19/19 tests pass.
- `node tools/check_progress_ui.mjs` — exit 0, offline DOM/controller checks pass.
- `cmake --preset scaffold-debug` — exit 0.
- `cmake --build --preset scaffold-debug` — exit 0.
- `ctest --preset scaffold-debug` — exit 0, 3/3 infrastructure checks pass.
- `git diff --check` — exit 0.
- Structured comparison against origin/main — all pre-existing task records,
  global verification records and product summary unchanged.

This documentation task adds no tests and changes no product behavior. No product
acceptance, independent product review, browser/OBS, Windows or live provider check
is claimed. Applicable documentation gates are consistency, existing tooling and
latest-revision CI; publication and merge status are recorded on the task PR.

Changed paths: docs/COORDINATOR_PROMPT.md, README.md, docs/tasks/BOOT-007.md,
this evidence note, progress/state.json and generated PROGRESS.html.
Next action for the owner: paste the Starting prompt into the intended coordinator
session. Use the Continuation prompt after interruption, including in a new session.
