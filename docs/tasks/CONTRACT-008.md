# CONTRACT-008 — Declarative scene/schema contract

Parent contract gate: [CONTRACT-003](CONTRACT-003.md). Kind: contract.
Authority: specification v0.45 §11, API-01/03, UI-01/02/04.
Status/owner/dependencies: progress/state.json.

## Scope and acceptance

Define the versioned declarative scene/data contract: elements, bindings, all/any/not
conditions, geometry/layers/styles, animation timing and server-owned media/font
references. Cover draft/live revisions, authorized publication/restore, unsupported-
feature rejection and data projection boundaries. Include independent valid/invalid
scene examples and shared reference-scene requirements for separate renderers.

Retain all §11 element/media formats, restricted static SVG/text-to-path rules,
configurable asset/dimension budgets, font profiles, canvas/resize modes, elapsed-time
animation semantics, deficit/progress behavior and conformance targets. Preserve
editor pre-publication checks, packaged assets, revocable scoped overlay access and
desktop-only full editing. Unknown domain behavior cannot be silently filled with a
renderer default. Split further before assignment if the actual contract is too large.

Schema review does not establish decoder safety, native/browser equivalence, codec/
font/platform support, performance or OBS behavior. SCENE/RENDER and their actual
test-first/platform tasks retain those requirements; browser management gains no
full editor or shared rendering engine.

## Ownership and handoff

Author owns assigned schemas/scene/ files, contract/examples and evidence. Root
coordinates common imports/index, state/HTML/build/CI/pins/maps. Read root/schema
AGENTS. Review actual artifacts before consumers/generated code; record exact
validation commands and CI coverage for later executable additions. No renderer,
asset processing or production code is authorized by this brief. Follow
[ADR-0004](../decisions/0004-wire-contract-prerequisites.md) and [workflow](README.md).

## Child contracts and preserved integration gate

[ADR-0005](../decisions/0005-scene-contract-prerequisites.md) proposes four focused
children, each additionally gated by CONTRACTPLAN-002: [CONTRACT-009](CONTRACT-009.md)
for the protocol-neutral model, [CONTRACT-010](CONTRACT-010.md) for typed bindings,
conditions and presentation clocks, [CONTRACT-011](CONTRACT-011.md) for assets/fonts/
portable packages and [CONTRACT-012](CONTRACT-012.md) for the complete schema package.
Actual independent document/state review precedes integration and child claims.

CONTRACT-008 retains every scope and acceptance requirement above. Its completion
requires all four children and an actual integration audit of their exact versions,
resolved references, complete valid/invalid examples, feature/profile vocabulary,
disclosed data and shared reference-scene requirements. Partial model approval is
not a complete scene contract. Preserve the existing CONTRACT-005/CONTRACTPLAN-001
gates and add CONTRACTPLAN-002 plus all four children in state under root ownership.

CONTRACT-006 still waits for 008, CONTRACT-003 still waits for 008 and the other
wire contracts, and every existing SCENE/RENDER/epic/release prerequisite remains.
No child depends on 008 or a downstream consumer. Required missing owning-domain
models remain explicit prerequisites; no empty schema, omitted element or renderer
default may stand in for them. Schema review still establishes none of the separate
implementation/conformance/media/font/performance/platform/OBS results above.
