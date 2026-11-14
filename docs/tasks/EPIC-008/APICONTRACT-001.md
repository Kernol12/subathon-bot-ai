# APICONTRACT-001 — Actual host lifecycle and native WebSocket profile

Parent: [EPIC-008](../EPIC-008.md). Kind: contract. Specification v0.45 §§2/10/16/17/18,
SERVER-01, API-01/02/03, AUTH-05/12, TEST-04/05.
Inputs: accepted common wire v1, session clock v1 §2, physical account/session v1,
writer/work contracts and actual pinned SPIKE-001 source/evidence. This claim accepts
no new concrete mechanism before a different contract/example review.

## Scope and ownership

Author owns ONLY new docs/contracts/api-host-lifecycle-v1.md and
new docs/contracts/native-websocket-profile-v1.md. Root owns shared task graph,
briefs, schemas, CMake/CI, dependencies, exact patch/ABI/license admission, SDK and
progress. No code, header/stub, tests, dependency build or native observation is
part of this document task. Use fresh author and independent reviewer sessions.

Define one bounded production host lifecycle: actual invocation membership/current
installation generation, owned command/reply lifetimes and budgets, distinct
post-work publication scope, refusal/readiness ownership, startup/signals,
admission/drain/cancellation/deadlines and owner-thread teardown barriers. Listener
running, writer open, a DTO or a retained SqliteWorkSession is not readiness or
publication authority; writer revokes its ticket before completion. Unknown
readiness refuses. Name actual sources and ownership; split substantial absent
reusable readiness/durable recovery machinery before adding it. Preserve clock
contract scope and full API-001 acceptance; do not invent a parallel clock host.

Separately define the exact admitted native control/fragment/Close profile needed
before persistent WS production: selected clean Drogon/Trantor source, legal data
fragmentation and interleaved control isolation, text versus Close UTF-8, mask/FIN/
length/size rules, absent/valid/invalid status, single reply, simultaneous closure,
no subsequent application data admission and bounded forced teardown. A blind
Close echo is insufficient. Preserve original code/status absence instead of
manufacturing 0 or1000. Use primary RFC/current registry and pinned source, with
independent literal wire/ownership/error examples and explicit native proof limits.

Concrete source findings: selected Drogon1.9.14 commit3aae769c/Trantor78e480f5
forwards Close payload AFTER requesting TCP shutdown; immediate versus queued-write
shutdown differ. One parser accumulator/type also mixes fragmented data/control.
Trantor bind failure exits the process. Choose an explicit supported process-lifetime
profile or identify a separately reviewed recoverable-start prerequisite; no
same-process recovery promise follows from the old probe.

SESSION-011's account H1 ingress profile refuses Upgrade. The native WS profile
must not widen it or claim account/auth/sync support. API-003 retains complete
synchronization/authorization scope. Own a distinct fresh profile prefix and
complete source/patch/ABI/build/license identity; preserve the clean stock SDK.
No implementation, source upgrade, package mutation or framework workaround is
accepted by documenting a desired behavior.

## Acceptance and handoff

Separate review must approve exact contract revisions, independent valid/invalid
examples, finite allocation/call/lifetime commitments, explicit unknown/refusal
states, complete existing release scope and downstream test/implementation gates.
Record requirement conflicts rather than reducing acceptance. Then run project
consistency, current HTML and whitespace checks; update readable state/evidence,
open one focused PR, and use current CI/review/rules/normal merge gates. Detailed
commands/review belong in the PR, not new evidence files. APITEST-012/API-012 follow
contract approval with real reviewed tests/intended red/source review/selected CI;
APITEST-001 and clock tasks remain unimplemented.
