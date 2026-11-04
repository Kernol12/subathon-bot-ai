# CONTRACTPLAN-001 — Split the product wire-contract prerequisites

Kind: contract/planning. Module: Server/API.
Status, owner and dependencies: progress/state.json.
Authority: specification v0.45 API-01/02/03, NUM-04, §10/§11 and CONTRACT-002.

## Goal and acceptance

Split CONTRACT-003 into independently reviewable common-representation, HTTP,
WebSocket and scene contract tasks. Preserve every original requirement and the
parent completion gate. Map existing direct consumers to the contracts they actually
need, without requiring pure password/encryption/permission components to wait for
scene schemas. No child may depend on its parent or a downstream implementation.

Provide a decision record with exact dependency changes, bounded child briefs,
explicit ownership of shared schema primitives, and independent review of the actual
planning revision. Regenerate progress and validate the dependency graph and links.
No product contract is finalized by this decomposition; children need versioned
examples and separate review before their clients/generated code or production use.

## Scope and ownership

Coordinator /root owns progress/state.json, generated PROGRESS.html, CONTRACT-003,
this brief, new child briefs, the decision record, backlog navigation and evidence.
Other module schemas, source/build/CI, test assertions and provider capabilities are
outside this planning change. Authors/reviewers use separate branches/worktrees.

## Evidence and checks

Record exact revision, commands, outcomes and independent reviewer. Required checks:
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check`. Existing CI checks document/graph consistency and unchanged
components. Assess coverage; no new executable suite or workflow is created here.
Contracts/tests/implementation/platform/live verification remain their own tasks.
