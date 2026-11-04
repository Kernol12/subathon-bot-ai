# CONTRACT-009 — Independent scene document review

Reviewer: `/root/service_reviewer`, different from actual document author
`/root/codec_author`. Date: 2026-11-04. Dedicated
`task/CONTRACT-009-document-review`, isolated worktree
`local-evidence/worktrees/contract009-review`, initially clean.
Owns only this new review note. The reviewer authored the prerequisite planning
documents, not this scene model; earlier planning review approves none of this new
document prospectively.

## Exact candidate and verdict

Actual author commit `6a88c79c8eaca0ccc9aa60cde03d103a92320e31`, imported at
`0857f31c05b06941531c0f823a6ba60e2c112b1f`:

| Artifact | Git blob |
| --- | --- |
| [Scene document](../contracts/scene-document-v1.md) | `5eb1c845f46b615b5e0a343e5c1283539c5bbecc` |
| [Author evidence](contract-009-author.md) | `cf0cfc95b9aa683a624d6def5cdc8ec2524368b8` |
| [Specification](../specification.md) | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| [Common005](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [ADR0005](../decisions/0005-scene-contract-prerequisites.md) | `e4bb60c82007a48e9ec4f7a0f7978d290d551bf4` |
| [009 brief](../tasks/CONTRACT-009.md) | `2ac98e5ddceff93bd4aa3347704d7e9eda038fed` |
| [010 brief](../tasks/CONTRACT-010.md) | `3a22162be1f8876953eb60e8d4c30a7c9221a13d` |
| [Actual plan review](contractplan-002-review.md) | `de522f0b3df0c112e34135dec84256c773babe43` |

**CHANGES REQUIRED for this actual document.** R1 needs an explicit all-family
binding attachment seam. R2 needs a narrowly qualified identifier-grammar sentence.
These are bounded document corrections, not a new product-policy decision or
authority to define unreviewed 010 destinations. Do not complete/release this
contract from the original candidate; actual revised artifacts need re-review.

Read all 412 source lines and author evidence, root/schema instructions, §11,
common005, ADR0005/plan002 review, full parent008/009/010 briefs and relevant scene
editor boundaries. No author's file, schema, state or shared configuration was
changed by this reviewer.

## R1 — Every element needs an explicit typed binding seam

Specification §11 says each element has data bindings as well as visibility
conditions and animations. The approved 010 allocation requires a finite typed
field/formatting registry for all seven families. The current model's common node
properties (§2, lines 67–73) include visibility/ConditionRef and animations but no
corresponding binding attachment. The family table (§4, lines 155–161) supplies
content BindingRefs for timers/text/progress/recent support/announcements; image
has VisualAssetRef, while shape/group has only intrinsic leaf/group content. The
§5 reference examples do not explain how a binding attaches to those elements or
their permitted presentation properties.

This need not mean the author intended to forbid bindings, and the reference list
says "include" rather than claiming an exhaustive schema. Nevertheless, declaring
010 as an owner without a structural attachment leaves the required shape/group/
image binding behavior unrepresented. A consumer could reasonably implement the
listed static properties and family-specific content bindings only. Visibility or
animation is not a substitute for a data binding.

Required correction: reserve an explicit typed per-element binding attachment,
available across all seven families including intrinsic shape, group and image.
Leave the actual allowed destination registry, types, resolution/precedence and
unavailable-value behavior UNFINALIZED under 010; this review does not demand
that every property be dynamic or choose override/default semantics. Do not add
arbitrary native property bags, server JSON paths, scripts or accounting-to-geometry
coercion. Preserve literal/static scenes and explicit no-binding representation.
Add an independent partial shape/group binding example that retains the precise
unresolved 010 dependency and cannot claim complete package/publication validity.
Update reference/resource-accounting descriptions as needed without weakening the
existing examples or closing 010/011 through a placeholder.

## R2 — Qualify capability identifiers separately from authored Text labels

§5 lines 192–194 says "Labels use common identifier grammar" directly after
CapabilityIdentifiers. In context this appears to mean feature/capability vocabulary
labels, but §1 makes authored scene/node labels common Text (empty and arbitrary
scalar sequences allowed); SD14 explicitly retains combining/precomposed Unicode.
An unqualified label rule could instead impose the common ASCII/64-character
ProtocolIdentifier grammar on those authored labels, contradicting §1/SD14 and
the proposed 64 KiB Text-label allowance.

Required bounded clarification: explicitly apply common identifier grammar to the
required feature/capability identifiers, keeping authored scene/node Text labels
under their distinct Text rules. Preserve SD14, no normalization and literal
markup-looking text. No schema, new label restriction or new default is needed.

## Other reviewed portions and independent facts

No additional blocking defect was found in the following inspected portions; that
does not approve a future correction or complete package:

- One ordered ownership forest has unique placement, group-only children, no
  orphan/dangling/duplicate/cyclic containment and explicit sibling/subtree layer
  composition. Type/scoped identities preserve NUL/high bytes and immutable history;
  scene labels never supply identity or order.
- Group versus shape style compatibility is already represented: §4/table row
  distinguishes group style from intrinsic shape style, and SD15's GroupBase has
  only the two common properties. Group opacity/clipping applies to the composed
  subtree. No additional policy defect or new renderer inheritance rule is inferred.
- Line endpoints are explicit local-coordinate positions subject to the stated
  finite geometry and stroke-range rules. The document does not require endpoints
  inside the frame or strictly positive stroke/size; the specification supplies
  no basis to invent those restrictions. Exact closed style/property records and
  raster tolerances remain 012/renderer work.
- Fit/crop/stretch, y-down clockwise transform order, pivot/parent composition,
  explicit alignment/clipping and singular/zero/reflection handling are coherent.
  Exact domain ratios/source values are not narrowed to geometry FiniteDecimal.
- Current draft/live comparisons, fresh immutable revision/publication identities,
  restore and undo protect history/ABA. Current rights and atomic persistence stay
  with the server; legitimate committed retries and unknown outcomes are preserved.
- All seven families and media/audio/font/package seams remain, with 010/011
  UNFINALIZED as a contract gap rather than successful runtime unknown. Required
  features must cover actual content and both intended outputs; unresolved or
  unsupported definitions cannot authorize compatible publication.
- Proposed new scene-only structural limits have explicit rationale and raw-byte/
  repeated-use accounting. They are unmeasured technical proposals, not MEDIA-04
  defaults or universal domain-reference limits. Existing external bounds and exact
  accounting remain with their owners. 012 still must freeze wire/hex/Text expansion,
  field walks, numeric/intermediate/output budgets and error order before decoding.

A read-only stdlib Python audit (Fraction, Counter, graphlib, Path, re and Git reads;
no project import) passed 21 literal/arithmetic/byte/graph/example/frozen-input facts.
SD07 gives fit 1/2 → 100×50 offset (0,25), crop 1 → 200×100 offset (-50,0), and
stretch (1/2,1). SD08 gives (10,24), versus (10,22) for the incorrect reversed order;
SD09 gives (13,24). Default 1920×1080 → 1280×720 has exact scale 2/3. Literal forest
counts/repeated placement/orphan and graphlib cycle checks agree with SD03–06.
NUL/high-byte hex, 4097 bytes → 8194 hex characters per occurrence, 65536 → 131072,
separate repeated-reference raw charges, distinct Unicode scalars, 26 top-level SD
rows and four frozen source/input hashes also passed. These are independent document
facts, not a model implementation, parser/renderer oracle or executed acceptance.

## Actual checks and remaining gates

Linux/Python 3.14.8; from the isolated review worktree:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed: 406 task records/122 decisions, local links/presets/current HTML.
Read-only source/blob/status inspection completed. Sole-owned-path/frozen-source
and staged whitespace checks accompany commit. Existing document/component CI must
still pass for the eventual final PR head. No new executable workflow is warranted
by these two-document corrections.

Original author may correct only the bounded document/author-note issues under root
assignment, preserving all existing independent examples and gates, then return
actual hashes for different re-review. No future approval is given here. 010/011
own their unresolved semantic definitions, 012 owns complete reviewed schemas/tools/
finite profiles, and 008/003 retain their full integration gates. All editor,
publication/restore, privacy, independent renderer/reference-scene, media/SVG/font/
Noto/OFL/budget, FPS and actual platform/OBS/release requirements remain.

No schema/API/stub/fixture/test/parser/validator/codegen, native, renderer, provider,
media/font, performance or platform behavior ran or was authored. No push, PR or
main merge occurred. Root owns shared state/HTML/build/schema/import/CI/pin and
publication changes; this review supplies no scene capability claim.

## R1/R2 correction re-review — approved revised document stage

Date: 2026-11-04. Same different reviewer `/root/service_reviewer`; corrected
document author remains `/root/codec_author`. Actual author revision
`e72744635a5e3b8a40ed2e9b5fc8c191516983e8`, root import
`0ace3ae2e29f82019af1a84b7093a9732af6eb72`, exact review-tree import
`199f33c9e34a09e4d1c01ebe99a789a6cf3bd86f`:

| Corrected artifact | Git blob |
| --- | --- |
| [Scene document](../contracts/scene-document-v1.md) | `9a3507331376a2beed47b93eac7079b45f0f2c94` |
| [Author evidence](contract-009-author.md) | `8f9f619b6cedca884387335dd91e4688ce5e1cb9` |

**APPROVED for this exact revised logical scene document stage only.** R1 and R2
are closed by the actual correction. This supersedes the changes-required verdict
only for the revised artifacts above; the original candidate/findings and its real
review history remain unchanged. No new owner decision is required. No future
schema/API/code/test/renderer or complete 008/003 approval is implied.

Inspected the actual two-file correction diff and affected node, family, reference,
resource and example definitions. R1 now has an explicit common choice of
NoBindingAttachment or typed ElementBindingsRef for every family, expressly including
image, intrinsic shape and group. Family-specific content BindingRefs remain and
are not erased by the empty common attachment. Permitted destinations/value types,
relationship to content references, override/resolution/conflict precedence and
unavailable behavior remain UNFINALIZED under 010. The document neither promises
every property is dynamic nor authorizes overriding static values. No raw property
bag, server field path, script or accounting-to-geometry coercion is introduced.
An unresolved attachment remains a dependency, never successful publication.

S and SD01–26 explicitly select the empty common attachment as an example condition,
not an omitted-field or product default. Empty attachment creates no external
reference/payload; 012 still owes actual tag/framing/representation budgets. Each
nonempty attachment use counts against the existing reference/aggregate limits,
including repeated sharing, and actual 010 definition bodies are not assumed free
or materialized through an unresolved reference.

New SD27 is a PARTIAL G→A shape/group case, using the compatible GroupBase and S
shape style. Both nodes attach the same pending definition. Resource `appearance`
and definition `binding-r1` are 10 bytes each: 20 per reference, 40 across the two
uses, PLUS actual kind/version bytes. It claims no complete byte total, permitted
destination, binding effect, static-value precedence or publishable closure before
010. This satisfies the requested independent partial example without guessing
the missing semantic registry.

R2 now explicitly applies common identifier grammar to required feature/capability
identifiers. Authored scene/node labels remain Unicode Text under §1/SD14, without
ASCII restriction or normalization. Domain identities retain their separate opaque
byte type. No old Unicode or literal-text example was weakened.

Independent read-only stdlib/Git checks compared old and revised documents:
EVERY original table row is byte-identical and in the same order; SD27 is the sole
added table row, increasing top-level SD examples from 26 to 27. Original sections
1, 3, 6, 7 and 10 are byte-identical. The prior valid geometry/byte/Unicode/forest/
revision audit is preserved rather than replayed as a new result; the changed
reference accounting/empty choice and exact 20/40-byte facts were checked directly.
The correction changes only the two authorized author paths. Revised blobs match
the assigned hashes; common/specification and shared state/HTML remain unchanged.

Actual re-review checks:

```sh
git diff 866befc..199f33c -- docs/contracts/scene-document-v1.md docs/evidence/contract-009-author.md
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed; consistency reports 406 tasks/122 decisions/current links/presets/HTML.
Sole-owned-path, original-review-prefix preservation and staged whitespace checks
accompany this append-only review commit. No schema/API/stub/fixture/test/parser/
validator/renderer/native/media/font/platform/provider execution or code authoring
occurred. Root owns final document/component CI, task closure and publication.

The approved artifact is still a protocol-neutral logical structure. 010 must
supply actual owning models and typed binding/condition/formatting/time definitions;
011 owes asset/font/package closure, 012 the complete reviewed schema/tool/resource
contract, and 008/003 their integration gates. Existing independent renderer/editor/
publication/privacy/media/SVG/font/budget/performance/OBS/platform/release acceptance
and test-first obligations are retained. No scope is dropped and no supported
capability or product behavior is claimed by this corrected document approval.
