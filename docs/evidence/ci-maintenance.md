# BOOT-008 — CI maintenance and automatic credit checks

Author: Codex /root. Date: 2026-10-30.
Branch: task/BOOT-008-ci-maintenance, based on main 57a5729.
Specification: v0.45 BUILD-02, DEV-02, DEV-04, TEST-02, TEST-04, TEST-05.
Credit contract revision 1, implementation and reviewed assertions unchanged.

The owner authorized adding CI growth instructions and automatic execution of the
reviewed time-credit suite. docs/AGENT_WORKFLOW.md now requires a per-task CI assessment,
workflow extensions with executable capabilities, observed workflow results and explicit
gaps/enforcement needs. docs/COORDINATOR_PROMPT.md includes this in the task loop.
README.md and docs/TEST_STRATEGY.md describe the actual new trigger configuration.

The existing acceptance.yml now runs for every PR targeting main and every push to
main, retaining manual dispatch. The suite is small (19 acceptance + 4 regression
cases), so no path filters are needed. Workflow/job names, read-only permissions,
pinned checkout/GoogleTest source and configure/build/CTest commands are unchanged.
GitHub's [workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)
documents triggers and warns that path-filtered required workflows can remain pending.
No failing test is hidden, no baseline changed and no new test dependency introduced.

Local Linux replay (GCC 16.2.1, Python 3.14.8):

- `cmake --preset acceptance-debug -DSUBATHON_FETCH_GOOGLETEST=ON` — exit 0;
  local installed GoogleTest used. Clean runner dependency fetching awaits PR CI.
- `cmake --build --preset acceptance-debug` — exit 0.
- `ctest --preset acceptance-debug -L 'acceptance|regression'` — exit 0, 23/23
  (19 reviewed acceptance + 4 reviewed regression).
- `python3 -m unittest discover -s tests/tooling -v` — exit 0, 19/19.
- `node tools/check_progress_ui.mjs` — exit 0, offline DOM simulation.
- `cmake --preset scaffold-debug` and `cmake --build --preset scaffold-debug`
  — exit 0.
- `ctest --preset scaffold-debug` — exit 0, 3/3 infrastructure checks.
- `python3 tools/progress.py render` and `python3 tools/check_project.py`
  — exit 0, current HTML / 377 tasks / 122 decisions.
- `git diff --check` — exit 0.

CI assessment: the previously manual credit lane now automatically checks the
implemented numeric helper on PRs/main. No Qt/browser/platform/packaging coverage
is inferred; those future executable capabilities must extend CI under the new policy.
Intentional red and credential-dependent live checks remain separately documented.
Actual PR workflow execution must pass before completing this task.

Automatic PR execution observed for authored revision
21e865a7cad6c9b9eee4dea12679adac840683aa on PR
[9](https://github.com/Kernol12/subathon-bot-ai/pull/9):
[acceptance run 41137002005](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41137002005)
triggered by pull_request and passed on ubuntu-24.04. The runner built the GoogleTest
dependency and executed all 23 cases successfully. Both scaffold runs passed too.
This establishes the automatic PR trigger and real runner suite, not Windows/Qt/live
verification. The completion-record revision must pass CI again before merge.

Required-check configuration inspected with
`gh api repos/Kernol12/subathon-bot-ai/branches/main` and
`gh api repos/Kernol12/subathon-bot-ai/rules/branches/main` on 2026-10-30:
rules enforce PRs, discussion resolution and deletion/non-fast-forward restrictions,
but contain no required-status-check rule; legacy required-check contexts are empty.
Remaining enforcement action: configure scaffold and acceptance as required checks
in the main ruleset if GitHub-enforced CI is desired. No repository settings changed;
agents must still apply the documented latest-revision CI merge gate.

No independent product review is claimed: this changes CI triggers/documentation,
not production behavior or reviewed tests. Changed paths are the acceptance workflow,
workflow/coordinator/test-strategy/README docs, task/evidence notes and progress artifacts.
Final revision, remote checks and merge outcome are recorded on the task PR.
