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
listener isolation and historical artifact/source/patch/pins/all44 methods immutable.
The execution prerequisite is a NEW additive candidate4 successor under
[APICONTRACT-005](APICONTRACT-005.md) → [APITEST-016 / API-016](API-016.md),
with separate reviewed patch/pins/license/build/ABI/identity, unchanged44
compatibility and new-consumer admission. API-001/APITEST-015 and
SESSIONTEST-011/SESSION-011 depend on API-016; the generic foundation has NO API-001
dependency. APITEST-015 itself changes no SDK. Preserve JSON
HTTP plus versioned WebSocket product scope and the eventual single public
origin behind Caddy; proxy/deployment support remains its own release gate.
Do not rewrite API-012 history, replace the framework, expose a second authority,
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

APICONTRACT-005 owns the exact generic numeric profile and suspension/admission
interface; APITEST-016 owns low-level preallocation L10–L12. Retain only finite,
checked pre-retention invariants here, aligning fixed65536 receive/8192 free and
ONE current HTTP request/connection with accepted account ingress. Do not invent
raw body-sized buffers, concurrent HTTP pipeline acceptance or host-handoff-as-
output-drain claims. SESSION-011 explicitly rebases capture/erasure/pre-log policy
onto API-016 without changing its accepted account budgets or needing API-001
foundation evidence. Preserve pristine source observations as history.

Choose public enableReusePort(true) to skip the unsafe Linux probe/lock and clear
actual SO_REUSEPORT with the before-listen callback, error fail-closed. Disclose
real defaults and unverified kernel behavior; retain both plain occupied-port
exit1 oracles/static-destructor limits. Specify bounded private scratch parent/
child and data-only observation enabling genuine docroot canaries, supervised
terminal cleanup, exact peer cap and separate Q/N elapsed proof levels.

Import the lifecycle/native/consumer contracts and describe the minimum reviewed
amendments needed there and in api_host_drogon.hpp. This task writes documentation
ONLY: do not edit headers/tests/SDK/production code. Coordinator owns shared state,
HTML/checkpoint/epic navigation; author owns the new contract and narrow links/
clarifications in API-001 and affected contracts. Record all actual authors.
Include the complete listener §6 supersession table and both pointer amendments:
topology/buffers/threads, allowlist/refusals/scratch, serialization outside response
pool, Qt endpoint-observer failure and existing signal safety's lock-free gated/
counted quiescence. The frozen dispatcher-return interface supplies no enforcement
of caller allocation before host ownership. Narrow SESSION-011/account-ingress
rebase/navigation amendments are part of this coherent contract review; new
foundation briefs remain coordinator-owned. No implementation is authorized.

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
