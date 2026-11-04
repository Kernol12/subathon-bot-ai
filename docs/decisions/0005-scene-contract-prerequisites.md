# ADR-0005 — Separate scene model, presentation, assets and schema contracts

Date: 2026-11-04. Status: proposed; actual independent document and dependency-state
review required. Task: [CONTRACTPLAN-002](../tasks/CONTRACTPLAN-002.md).
Authority: specification v0.45 §11, API-01/03, UI-01/02/03/04,
MEDIA-01/02/03/04/05/06, and the scoped read-only boundary in VIEW-04.
This refines [ADR-0004](0004-wire-contract-prerequisites.md), preserving its gates.

## Context and decision

CONTRACT-008 combines scene structure, domain projections, independent clocks,
media/font packaging and the complete machine schema. Common primitives are now
reviewed and merged through CONTRACT-005; they do not define these domain shapes.
`schemas/scene/` still contains a planning README, with no selected scene validator.
Several domain models needed by bindings, especially milestone presentation and
modifier clocks, have no completed owning baseline. An empty schema or a renderer
default cannot fill those gaps.

Add four focused contract children. Each depends on CONTRACTPLAN-002 in addition
to the exact prerequisites below. These are proposed state edges for integrator
application and independent review, not a claim that state was edited here.

| Task | Other direct prerequisites | Bounded outcome |
| --- | --- | --- |
| [CONTRACT-009](../tasks/CONTRACT-009.md) | CONTRACT-005 | Protocol-neutral scene document model, seven element families, geometry, revisions and typed extension/reference seams |
| [CONTRACT-010](../tasks/CONTRACT-010.md) | CONTRACT-009, CORE-002, CORE-003, CORE-005, ACCT-002, ACCESSTEST-002, MILESTONETEST-001, MILESTONETEST-004, MODIFIERTEST-001, MODIFIERTEST-002 | Typed disclosed data, conditions, exact progress, independent clocks and authoritative presentation semantics |
| [CONTRACT-011](../tasks/CONTRACT-011.md) | CONTRACT-009 | Asset/font references, portable package closure, restricted SVG, budgets and required profile evidence |
| [CONTRACT-012](../tasks/CONTRACT-012.md) | CONTRACT-009, CONTRACT-010, CONTRACT-011 | Complete versioned machine-schema package, resolved imports, independent examples and reviewed validation tooling |

CONTRACT-008 keeps its existing CONTRACT-005 and CONTRACTPLAN-001 dependencies and
adds CONTRACTPLAN-002 and all four children. It remains the full scene integration
gate. CONTRACT-006 still depends on CONTRACT-008, and CONTRACT-003 still depends
on CONTRACT-008 and the other complete wire contracts. No new child depends on
008, 003, HTTP/WS, a SCENE/RENDER consumer or an epic aggregate. Completion of 009
alone does not permit product scene consumption or complete 008.

First assignment after actual plan integration is CONTRACT-009's document stage:
only a new `docs/contracts/scene-document-v1.md` and its author evidence, followed
by different actual review. No schema, fixture, validator, generated client, source
or renderer is authorized by that first assignment. Later stages still need their
own bounded ownership and actual review; executable behavior retains tests/red/code
review rather than inheriting approval from this planning document.

## Why CONTRACT-010 has owning-domain gates

Use existing task IDs, rather than inventing a second timer or presentation model.
The baseline gates below are intentionally conservative: their completed status
requires reviewed contract/examples/tests and valid red, not just a task brief.
Production implementation of a pure model is not an additional prerequisite where
its completed baseline already supplies the reviewed semantics.

| Direct prerequisite | Required evidence and boundary |
| --- | --- |
| CORE-002 | Current lifecycle/Main/Grace/end-mode meanings from the reviewed timer lifecycle contract; no interpretation of remaining time as wall-clock UTC |
| CORE-003 | Independent pause reasons, pending delays and unknown notices; unknown does not become offline and clearing one reason does not globally resume |
| CORE-005 | Main/debt/deficit distinctions; a display cannot turn unapplied time or review into a completed effect |
| ACCT-002 | Known/absent source facts and independent eligibility/evidence dimensions from contribution eligibility; no invented paid amount, settlement or provider identity |
| ACCESSTEST-002 | Reviewed permission/grant constraints for selecting disclosed fields; a scene binding or advertised capability is never an authorization certificate |
| MILESTONETEST-001 | Exact sequential score, current configured thresholds versus captured achievement history, deficit and absent first-interval origin |
| MILESTONETEST-004 | The missing owning model for currently presented group, presentation/gap/queue clocks, pause and reconnect; immediate achievements are not the presentation queue |
| MODIFIERTEST-001 | Persistent definitions versus activation instances and their identities/states |
| MODIFIERTEST-002 | Independent active-subathon and real-elapsed modifier clocks, lifecycle and recovery; CORE-006 is already its transitive prerequisite |

MILESTONETEST-004 already requires MILESTONE-001 and MILESTONE-003; this plan does
not remove those prerequisites or manufacture a presentation model ahead of them.
MODIFIERTEST-002 already requires MODIFIER-001 and CORE-006. These existing paths
do not depend on scene/HTTP/WS contracts in the inspected graph. A completed gate
still needs an actual suitable reviewed model: if its output lacks a required field
or invariant, record the gap with that owner before completing 010. Do not admit an
empty registry, omit an element, return universal unknown as success or invent a
clock/policy merely to unblock the schema. Further decomposition needs a separate
reviewed plan rather than a silent dependency deletion.

VIEWER-001/002 and API-003 are deliberately not prerequisites of 010: they depend
on HTTP/WS and hence 008, which would create a cycle. CONTRACT-010 authors only the
shared, specification-derived disclosed scene-data contract, using the above domain
models and source privacy requirements. Actual viewer credentials, filtering,
revocation, authorized projections and persistent synchronization remain their
existing tasks, consuming the final contracts. A typed reference to public data
never grants access to private source records. Adding a binding outside these models
requires a reviewed owning-domain prerequisite, not arbitrary raw server JSON.

## Complete requirement allocation

The table allocates contract work and preserves implementation/release ownership.
It does not declare any capability tested or reduce any existing task acceptance.
Paired SCENETEST/RENDERTEST baselines remain required wherever already present.

| §11 requirement | Contract responsibility | Later required evidence/owner |
| --- | --- | --- |
| Timer; text; progress bar; image; shape/group; recent support; announcement area | 009 identifies all seven families and typed references; 010 supplies actual data/conditions; 011 supplies assets | SCENE-001/003; independent RENDER-001 oracles; RENDER-002/003 |
| Position/size/layers/styles, grouping, locks, default 1920×1080/custom canvas; aspect-fit default with crop/stretch | 009 defines shared geometry, ordering, units and resize semantics without renderer-local reinterpretation | SCENE-002/004, RENDER-001/002/003 |
| Selectable data/formatting, all/any/not, milestone position/progress intervals, modifier state, pause and grace | 010 defines typed operators, admissible fields and explicit missing/unknown/not-disclosed behavior | SCENE-003, API-003, VIEWER-002, RENDER-002/003; disclosure stays enforced by server |
| Exact progress between current thresholds, 0% and optional deficit below previous threshold, captured history, absent first interval, final completion layout | 010 uses owning milestone model and distinguishes exact calculations from display formatting | MILESTONE-001/002, SCENE-003, RENDER-001/002/003; no invented zero threshold |
| Immediate ordered achievements versus sequential multi-crossing announcements/actions, current presentation and reconnect | 010 uses MILESTONETEST-004 model; queue pause is independent by default and may explicitly follow countdown pauses | MILESTONE-004 and later action-group tasks; disconnected overlays cannot block or replay the queue |
| Separate Main/Grace/pause/recovery/modifier clocks; elapsed-time fade/move/scale/rotate with configurable easing/duration | 010 fixes timeline inputs, clock identities, elapsed units and interpolation/reference expectations; 009 carries typed animation references | CORE/MODIFIER owning tasks, RENDER-004; missed frames cannot extend time, hiding an overlay cannot disable a modifier |
| PNG/JPEG/WebP/static SVG, GIF, MP4/WebM, WAV/MP3/Ogg with explicit codec profiles | 011 defines required format/profile declarations and rejected/unknown states; 012 represents them | SCENE-005, RENDER-004, SPIKE-004 and actual platform evidence; container name is not codec support |
| SVG shapes/paths/groups/fills/gradients/strokes/transforms/internal references; no scripts/external resources/HTML/animation/filters; text to paths | 011 defines restricted static profile and complete rejection examples | SCENE-005 and both renderers' separate acceptance; no silent reinterpretation |
| Uploaded TTF/OTF, configured fallback, identical shared font assets; Noto Sans Regular/Bold default, OFL notices and permitted redistribution | 011 defines font/package provenance, closure and required pinned-asset/license evidence | SCENE-005/006, RENDER-004, PACKAGE/RELEASE; no unverified license or font-support claim |
| Configurable 32 MiB image/font, 128 MiB audio, 512 MiB video, 2 GiB package; 8192 per raster edge/32 MP total; demanding-scene warning | 011 preserves these initial limits and authorized adjustment, checks decoded dimensions and specifies finite execution/decoded-resource profiles for review | SCENE-005/006, RENDER-004; compressed length never proves decoded resource safety |
| Drag/resize/align/distribute/snap, grouping/layer locks, undo/redo, canvas/scaling; previews of lifecycle/modifiers/deficits/large input/long text | 009/010 specify serializable state and preview inputs, preserving edit semantics; no UI implementation | SCENE-002/003/004; full editor desktop only, Qt Quick/QML with C++ logic |
| Saved scenes/reusable themes/import/export with server-owned assets; drafts separate from live, authorized publication, restore and compatibility checks | 009 defines model/revision boundaries; 011 portable closure; 012 complete schemas; 008 audits consistency | SCENE-001/006/007, ACCESS-002, RENDER-005; no local designer paths or credential authority in scene files |
| Revocable scoped read-only overlay access; live revisions | 009/010 declare permitted projection/reference seams; 008 aligns later HTTP/WS imports | VIEWER-001, SESSION/ACCESS/API tasks, RENDER-005; no new credential policy or hosted authorization service |
| Independent native QML and browser HTML/CSS/TypeScript rendering; matching binding/visibility/geometry/progress/timing, minor font raster differences only | 009–012 require one complete contract and independent valid/invalid examples; 008 requires shared reference-scene coverage plan | RENDER-001 authors independently derived oracles, RENDER-002/003 separate implementations; no shared renderer or browser full editor |
| Pre-publication feature compatibility checks for both intended outputs, explicit unsupported rejection | 009/011 feature/profile requirements plus 010 typed projection requirements; 012 represents complete vocabulary | SCENE-007 and renderer conformance tests; unknown is not supported, capability is not permission |
| Target 60 FPS, selectable 30 FPS, elapsed timing; Windows/Linux OBS native/browser output | Contracts preserve targets and record required evidence, never mark performance or platform support complete | RENDER-004/090/091/092, SPIKE-004, VALIDATE/PACKAGE/RELEASE; hardware, OBS/embedded-browser minimums and codec deployment require actual validation |

## State preservation and integration acceptance

The only dependency edits proposed are new children with the table above and the
additional child/planning gates on 008. Preserve every pre-existing edge verbatim:
all 319 existing edges whose prerequisite is a test baseline, every SCENE/RENDER prerequisite, all epic
and release aggregates, HTTP006 → 008 and parent003 → 008. Consumers continue to
wait for the full 008 gate. Root must compare actual state against pre-plan main,
check known IDs/cycles and the exact additions, and obtain different actual review
of both authored documents and integrated state before claiming a child. Do not
mistake author self-checks for independent approval.

Before 012, actual reviewed choices must specify schema dialect, shared import
layout, pinned validation tools, finite per-field/message/decoded/intermediate
profiles, reference resolution and cross-field semantic checks. Common005 is reused
without narrowing opaque IDs or exact accounting to a convenient geometry/parser
type. No validator is selected by this decision; OpenAPI 3.1 on HTTP does not choose
the scene validator. Existing Qt/browser placeholders prove no scene capability.
Root owns dependency/build/CI changes. A new executable checker needs reviewed
checks and actual CI; schema structure alone cannot prove asset safety, permission,
domain semantics, renderer equivalence or platform decoding.

Document checks are project consistency, current HTML and whitespace. Existing
scaffold/component CI remains required for the final PR head. No native, renderer,
provider, codec or performance execution is required or claimed for this plan.
