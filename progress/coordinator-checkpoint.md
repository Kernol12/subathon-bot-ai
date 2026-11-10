# Coordinator checkpoint

Owner authorization (2026-11-10): start and finish one new task, then STOP.
API-012 is the dependency-ready priority. Complete only that task through independent
reviews, acceptance, publication and normal protected merge; do not start
APITEST-001/API-001 or actual session-clock binding afterward.

## Current handoff

- Task: [API-012](../docs/tasks/EPIC-008/API-012.md); status authority:
  [state.json](state.json). Native profile v1 and consumer boundary v1 are normative.
- Branch: `task/API-012-native-websocket-profile`; base main
  `642c9a0e7e7a5f7e97b2f27da3a0508edb85ec83`. Primary checkout is repository root.
- Startup verified local/remote main, clean checkout, no open PRs and no active state
  claims. Historical worktrees remain preserved; no old worker is assumed active.
  CLion project path and HEAD matched this checkout before branch creation.
- Coordinator: `codex:27731ff9-5bb1-4e9f-b2f3-b78e37ae011e`; owns shared
  documentation/state/HTML, CMake/CI integration and dependency admission. A fresh
  Codex implementation worker owns native implementation/patch paths; new Claude
  source/resource reviews are required. Record actual links on assignment.
- Reuse accepted [PR108](https://github.com/Kernol12/subathon-bot-ai/pull/108)
  test/source/resource reviews and observed stock WS02 wrong-Pong red. Frozen
  tests remain unchanged; missing facade/resource observations remain unimplemented.
  The clean stock SDK at `build/deps/drogon-1.9.14` must remain untouched.
- Pending: fresh implementation, exact corrected patch/build/ABI/license admission,
  independent source/resource review, native38 acceptance, ordinary selected CI,
  final packet/current checks/discussions and normal protected merge.
- Next action: commit claim and hand off a fresh implementation session. Any new
  substantial prerequisite must be explicitly split before scope expansion.

## Preserved completed work

WORKFLOW-001 normally merged in
[PR109](https://github.com/Kernol12/subathon-bot-ai/pull/109) as main642c9a0 at
2026-11-09T22:04:14Z. Startup independently verified all21 head checks successful
and separate source/docs and final metadata approvals recorded in the PR. Its
committed pending-publication notes are historical; final outcome is in that PR.

APITEST-012 normally merged as main6594758 in PR108. Detailed baseline, source,
build, runtime, CI and independent reviews remain in PR108 and ignored
`local-evidence/APITEST-012/`. Preserve receipts, old branches/worktrees and SDKs.
Full release, host/readiness, authentication, synchronization and platform verification
remain incomplete; no live provider or payment effects are authorized.
