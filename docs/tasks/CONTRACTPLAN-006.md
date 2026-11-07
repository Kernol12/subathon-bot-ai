# CONTRACTPLAN-006 — session executable prerequisite decomposition

Kind: contract/planning only. State/ownership/dependencies: progress/state.json.
Specification v0.45 §§10/16/17, AUTH-05/12, DEV-02, TEST-04/05.

Read accepted session-resolution-v1 and session-transport-v1, paired SESSION002/003
briefs, password and writer contracts and current dependency graph. The supplier
concretizes only prepare/resolve; broader lifecycle obligations have no concrete
mutation/inspection APIs or physical account/session store/current producer.
Selected Drogon duplicate/cookie/log ingress gaps also remain explicit owning gates.

Split bounded component baseline/implementation and substantial lifecycle contract,
SQL store/producer, trusted accepted-login/clock and ingress prerequisite work into
explicit tasks/PRs. Define precise ownership/acceptance/dependencies, preserving
all original task IDs, edges, requirement scope and per-behavior test-first/review
and release gates. No SESSION002→ACCESS004 cycle: extract upstream accepted-local-
login proof work while ACCESS004 retains full local/Twitch account linking scope.
Do not finish SESSION002/003 with only supplier component acceptance. Do not choose
new protocol/crypto/library/schema/API/implementation behavior in a planning PR.

Fresh author owns only ADR0010, new bounded briefs and author evidence. Root owns
all shared graph/state/HTML/handoff/build/CI/schema/pins. Actual separate review of
the final written plan and actual graph, preserved IDs/edges/current task states,
acyclic graph and project/tooling checks are required before completion. Existing
CI covers planning; no new runtime suite is enabled. No provider/live/native action.
