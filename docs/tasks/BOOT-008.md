# BOOT-008 — Grow CI with implemented behavior

Kind: scaffold (CI/workflow). Module: Foundation.
Specification: v0.45 BUILD-02, DEV-02, DEV-04, TEST-02, TEST-04, TEST-05.
Status/owner/dependencies: progress/state.json.
Branch: task/BOOT-008-ci-maintenance.

Scope/shared ownership: .github/workflows/acceptance.yml, docs/AGENT_WORKFLOW.md,
docs/COORDINATOR_PROMPT.md, docs/TEST_STRATEGY.md, README.md, this brief,
docs/evidence/ci-maintenance.md, progress/state.json and generated PROGRESS.html.
Product behavior, reviewed assertions and contract revisions remain unchanged.

Acceptance:

- Explicitly require assessment and extension of GitHub Actions for new executable
  modules, suites, build targets and packaging; reuse local commands/presets.
- Preserve independent review, visible intentional red baselines, live/platform
  boundaries and explicit tracking of CI gaps and required-check configuration.
- Run the existing reviewed time-credit acceptance/regression workflow on PRs to
  main and pushes to main, retaining manual dispatch and genuine failure reporting.
- Keep stable check names, read-only permissions and pinned dependencies/actions.
  Avoid path filters that could omit a future required check.
- Replay existing checks and observe the actual PR acceptance job before merge.
  Record evidence and publish a focused PR; do not change repository rulesets.
