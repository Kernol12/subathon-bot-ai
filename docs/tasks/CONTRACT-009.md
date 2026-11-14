# CONTRACT-009 — Protocol-neutral scene document model

Parent integration gate: [CONTRACT-008](CONTRACT-008.md). Kind: contract.
Authority: specification v0.45 §11, API-01/03, UI-01/02/03/04, MEDIA-02.
Proposed prerequisites:
CONTRACTPLAN-002 and CONTRACT-005, as recorded in [ADR-0005](../decisions/0005-scene-contract-prerequisites.md).

## Scope and acceptance

Define one versioned protocol-neutral scene document for separate native QML and
browser renderers. Cover stable identities, ordered elements/groups, geometry,
position/size/layers/styles, grouping/locks, canvas units/transforms, custom dimensions,
default 1920×1080 and aspect-fit default/crop/stretch. Distinguish opaque-byte
identities from Unicode labels and exact domain values from geometry. Use reviewed
CONTRACT-005 primitives without inventing a universal shorter reference limit.

Include all seven families: timer, text, progress bar, image, shape/group, recent
support and announcement area. Define typed references to the later binding/condition/
animation and asset/font contracts; record incomplete referenced definitions
explicitly. A placeholder shape is not a complete scene package or executable oracle.
Preserve selectable formatting, final completion layouts, theme reuse, long text,
preview/edit state and required cross-renderer conformance. Do not decide domain
clocks, missing milestone origins or supporter disclosure through a layout default.

Describe saved scenes, draft/live revision separation, revision identity, authorized
publication/restore boundaries, compatibility/feature requirements and server-owned
asset references. Map drag/resize, align/distribute/snap, grouping/layer locks,
undo/redo, canvas changes, preview and portable import/export to document invariants.
This is an editable declarative model, not an arbitrary script, raw server object,
designer-local filesystem path or grant of publication/overlay authority.

Provide independently written valid/invalid logical examples for nested groups and
ordering, duplicate/dangling/cyclic references, geometry/scaling boundaries, each
element family, unsupported features, draft versus live revisions and long/opaque
identities. Distinguish complete structural examples from partial examples whose
binding/asset semantics await 010/011. Specify finite structural limits for actual
review before executable decoding; do not invent new product defaults here.

## First document stage and ownership

After actual plan integration/claim, the first author owns only NEW
`docs/contracts/scene-document-v1.md` and `https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/contract-009-author.md`.
Obtain different actual logical review before another stage. No schema/API/stub,
fixture/test/code, validator, renderer, native or provider execution is authorized
by that first assignment. Further scope needs an explicit bounded claim.

Root owns state/HTML/maps/navigation, schemas/common imports, build/CI/pins and
integration. Read root/schema AGENTS. Record exact source revision, examples,
reviewer and real consistency/current-HTML/whitespace checks. This child does not
complete CONTRACT-008/003 or certify native/browser/editor/platform behavior.
See [workflow](README.md) and ADR-0005 for the preserved downstream gates.
