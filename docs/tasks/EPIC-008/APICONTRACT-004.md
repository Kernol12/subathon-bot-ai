# APICONTRACT-004 — Isolated HTTP and native WebSocket host listeners

Kind: contract. Parent: EPIC-008. Specification v0.45 §2, SERVER-01, API-01,
DEP-02/05 and §17–18. Status/owner/dependencies: progress/state.json.

## Exposed conflict and bounded decision

API-001 initial implementation review B1, at df5d703, found that the frozen
API-012 corrected SDK rejects every non-Upgrade request on its native listener,
while APITEST-001 expects ordinary HTTP diagnostic service there. See
https://github.com/Kernol12/subathon-bot-ai/pull/121#issuecomment-6816497937.
This is a contract/baseline inconsistency, not missing provider behavior or
permission to weaken tests. Source b4e758d has not run host acceptance.

Choose separate INTERNAL ordinary-HTTP and native-WebSocket listeners in the
SAME application/framework lifetime and existing single IO loop. Keep API-012
listener isolation, source/patch/pins and all44 methods unchanged. Preserve JSON
HTTP plus versioned WebSocket product scope and the eventual single public
origin behind Caddy; proxy/deployment support remains its own release gate.
Do not reopen SDK behavior, replace the framework, expose a second authority,
or remove HTTP diagnostics. This is an implementation-topology decision allowed
by the standing execution scope, subject to the normal independent contract gate.

## Authoring scope and acceptance

Author docs/contracts/api-host-listeners-v1.md with concrete options/endpoint
selection, readiness for BOTH real listeners, finite startup/stop and shared
peer/association/resource budgets. Pin HTTP/WS separation, addresses/port0 and
conflicting-bind behavior, all failure/refusal and one-start semantics. Define
how actual framework endpoints are identified without a fixture readiness flag
or an unbounded observer payload. Keep installed schema/current-source/session
UTC authority and all existing HL01–17 requirements unchanged.

Specify the ordinary HTTP surface explicitly: no implicit static-file router,
index/404/405/OPTIONS/upload/log behavior, unbounded pipelining/body buffering or
untracked framework state. Select bounded rejection/closure and address any new
HTTP route/response/drain allocation bounds. Define safe process-local paths,
framework global-retention limits and all callback/writer/IO/framework teardown
barriers. Preserve no live/provider/chat/payment side effects.

Import the lifecycle/native/consumer contracts and describe the minimum reviewed
amendments needed there and in api_host_drogon.hpp. This task writes documentation
ONLY: do not edit headers/tests/SDK/production code. Coordinator owns shared state,
HTML/checkpoint/epic navigation; author owns the new contract and narrow links/
clarifications in API-001 and affected contracts. Record all actual authors.

## Explicit baseline revision gate

APITEST-015 separately owns reviewed fixture/header/test changes and meaningful
missing-behavior red before API-001 implements the new topology. Preserve ALL old
behavior assertions, literal expected results, test/resource/clock/source/SQL
barriers and historical PR112 red/approvals. Routing an HTTP case to its dedicated
HTTP listener is a disclosed baseline revision, not a silent deletion or green
result. Any proposed assertion removal needs an explicit itemized ADR-0012 scope
and separate approved review; this brief authorizes no removal. Add cross-listener
refusal, actual HTTP service/no-file-leak and both-listener failure/stop examples.

Independently review contract/examples: Claude initial, Codex corrections and
separate verification, then Claude closing. Publish one focused contract PR from
current main and merge only with live CI/reviews/protected gates. No host support,
new implementation or revised test execution follows from a document approval.
