# ADR-0004 — Separate common, HTTP, WebSocket and scene contracts

Date: 2026-11-04. Status: proposed; actual independent planning review pending.
Authority: specification v0.45 API-01/02/03, NUM-04, §§2/10/11 and CONTRACT-002.
Task: [CONTRACTPLAN-001](../tasks/CONTRACTPLAN-001.md).

## Context and decision

CONTRACT-003 currently combines OpenAPI 3.1, separately versioned persistent
WebSocket synchronization and the declarative scene contract. Its 34 direct
consumers include password hashing, encrypted credential records and a pure
permission evaluator. Those internal components need their own reviewed contracts,
not every future scene and transport schema. The schema directories currently
contain planning READMEs only. Existing reviewed domain boundaries are internal;
SPIKE-001's disposable one-command/server-close protocol is not a product protocol.

Create four contract children, with one owner for shared representations:

| Task | Dependencies | Contract responsibility |
| --- | --- | --- |
| [CONTRACT-005](../tasks/CONTRACT-005.md) | CONTRACT-002 | Common exact values, references, versions/capabilities and envelope primitives; document and independent examples first |
| [CONTRACT-006](../tasks/CONTRACT-006.md) | CONTRACT-005, CONTRACT-008 | HTTP/OpenAPI 3.1 contract with referenced scene schemas and owning endpoint tasks |
| [CONTRACT-007](../tasks/CONTRACT-007.md) | CONTRACT-005, CONTRACT-006, SPIKE-001 | Separately versioned persistent WebSocket synchronization/projection contract |
| [CONTRACT-008](../tasks/CONTRACT-008.md) | CONTRACT-005 | Declarative scene/data/asset contract for separate renderers |

Each child also depends on this reviewed planning task, CONTRACTPLAN-001. No child
or internal prerequisite depends on CONTRACT-003. The parent remains a required
contract integration audit, depending on CONTRACT-002 and all four children. Its
completion still requires consistent versions, references, capability vocabulary,
OpenAPI/WS/scene packages and independently specified examples. A common document
alone never completes the parent. Planned children must be split further before
assignment if their actual scope cannot fit one focused contract review.

HTTP's scene dependency prevents a complete OpenAPI package from advertising an
unresolved imported scene shape. The common boundary allows lifecycle/queue work to
proceed without implementing HTTP endpoint families. Schema authoring cannot settle
unreviewed domain behavior; a child records explicit downstream prerequisites or
splits rather than filling gaps with invented policies or empty success schemas.

## Exact consumer changes

Replace only the old CONTRACT-003 edge for the following 24 non-epic consumers.
Every other existing dependency, including each paired test-first baseline, stays.
The eight removed helper gates are replaced by CONTRACTPLAN-001 so their earlier
start requires this reviewed sequencing change to be integrated.

| Consumers | Replacement dependency |
| --- | --- |
| ACCESSTEST-001 / ACCESS-001 | CONTRACTPLAN-001; retain STORAGE-002 and SESSION-001 |
| ACCESSTEST-002 / ACCESS-002 | CONTRACTPLAN-001; pure permission evaluator |
| SESSIONTEST-001 / SESSION-001 | CONTRACTPLAN-001; password hashing/upgrade primitives |
| SESSIONTEST-006 / SESSION-006 | CONTRACTPLAN-001; retain STORAGE-002 |
| APITEST-001 / API-001 | CONTRACT-005; retain SPIKE-001 and BOOT-003 |
| APITEST-002 / API-002 | CONTRACT-006 |
| APITEST-005 / API-005 | CONTRACT-005; retain CONTRACT-002 |
| SCENETEST-001 / SCENE-001 | CONTRACT-008 |
| RENDER-001 | CONTRACT-008; independent reference-scene oracles |
| WEBAPPTEST-001 / WEBAPP-001 | CONTRACT-006; retain API-004 |
| VIEWERTEST-001 / VIEWER-001 | CONTRACT-006 and CONTRACT-007; retain SESSION-003 and ACCESS-002 |
| RELEASETEST-002 / RELEASE-002 | CONTRACT-005; retain ACCESS-002 and API-005 |
| VALIDATE-001 | CONTRACTPLAN-001; matrix explicitly tracks all four planned contracts |

Add CONTRACT-007 directly to APITEST-003/API-003 and to RENDERTEST-002/RENDER-002
and RENDERTEST-003/RENDER-003. Their persistent/synchronized input contract must not
be lost when API-002's broad parent gate becomes HTTP-only. The ten epic consumers
EPIC-006/007/008/018/019/020/021/022/023/024 keep their CONTRACT-003 gate. Existing
release aggregate and live/platform dependencies are preserved.

## Ownership and acceptance boundaries

Root owns state/HTML, this decision, briefs/navigation, schema index/common imports,
root build/CI/pins and integration. Child writers use isolated task branches and
explicit assigned paths. Common representations have one reviewed definition;
HTTP, WS and scene schemas reference it under integrator coordination. Authors
cannot independently revise shared fields. Contract review precedes generated
clients or product wire use; executable behavior retains its reviewed tests, valid
red, implementation, independent code review and actual passing acceptance.

Keep exact decimals/accounting and signed/unsigned64 JSON values as strings, and
rational values as explicit numerator/denominator strings. Unknown/null/zero and
opaque bytes versus Unicode text need explicit versioned rules. Advertised
capability is neither authorization nor proof of provider support. Every protected
operation still needs current execution-time permission and the authoritative Qt
queue; schema validation is no authorization certificate. No project-hosted auth
service or public-client shortcut is introduced.

Persistent WS, close, pending updates, slow consumers and shutdown need actual
API-001/003 framework validation; the spike does not establish them. Scene contract
review cannot certify native/browser conformance, codecs, media budgets, fonts,
performance, OBS or platform behavior. Desktop-only editing and independent browser
rendering remain. All seven integrations, all three exchange providers and every
original product/release requirement stay in scope. Outstanding timer-edit product
questions stay unresolved; wire authors must not choose defaults for them.

## Alternatives and validation

Keeping the coarse parent on every helper delays independent internal work without
adding relevant wire evidence. Deleting the parent would lose the full integration
contract gate. This split retains that gate and makes the actual prerequisites
explicit. No schema, source, test expectation or supported capability changes here.

Validate actual state against this table, all prior unrelated edges and all ten epic
consumers, detect unknown IDs/cycles, verify document links/current HTML and obtain
independent review of the authored revision. Existing scaffold CI covers planning
consistency; all component CI must still pass on the final PR head. No new executable
suite or workflow is required for this documentation/dependency change. Record real
commands and outcomes in the planning evidence, without claiming product tests ran.
