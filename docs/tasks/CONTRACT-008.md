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
