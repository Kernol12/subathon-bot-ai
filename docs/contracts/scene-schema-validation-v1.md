# Scene schema and validation — pre-schema proposal v1

[CONTRACT-012 #164](https://github.com/Kernol12/subathon-bot-ai/issues/164), Tier B, first stage.
Authority is specification v0.45 §11, API-01/03, UI-01–04, MEDIA-01–06 and VIEW-03/04.
The proposal requires independent review before concrete schemas or checker work.
The coordinator owns schema paths, imports, pins, CI, acceptance map and checkpoint.

## 1. Source boundary

Protected commit `2ed9ef63d09a666c69d05b1df5f946ce53eaba39`, tree
`bc4ba218c2800f80abc7f659b783968df552f98b`. The following are Git blob IDs at that
commit. The pinned examples in §6 inherit the full definitions and premises there.

| Owner and source | Exact blob |
| --- | --- |
| [Specification v0.45](../specification.md) | `0100f859a6c180e5895d12a9d23675b0697c9d0d` |
| #156 [ADR-0005](../decisions/0005-scene-contract-prerequisites.md) | `5e4df88ecaaf54fa5fce42545ab5ccde4435e8cc` |
| #157 [Common wire primitives](wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| #161 [Scene document](scene-document-v1.md), 009 | `9a3507331376a2beed47b93eac7079b45f0f2c94` |
| #162 [Bindings, conditions and timeline](scene-bindings-conditions-timeline-v1.md), 010, #793 merge `aedf78817ba15434e3fb114004bf453b16636788` | `9ca333ece5c80140f0d86d960fcf5b51e50294a6` |
| #163 [Assets, fonts and packages](scene-assets-packages-v1.md), 011 | `abce9c3b8a14d5a612aaef7a3ab372018c91c4a6` |
| [Tool contract v1 r2](schema-validation-tool-v1.md) and [ADR-0008](../decisions/0008-schema-validation-foundation.md) | `7880dc2371876db2f2f872e5ec9d0740968784f8`, `5c2f9a8c05fb9d314a1968b1ba6ac94dbbfa1b34` |
| [Hash lock](../../tools/schema_validation/requirements.lock), [wheel manifest](../../tools/schema_validation/wheels.json), [project checker](../../tests/schema_validation/test_project_schemas.py) | `8613fc8a595a93d62affe0c656bdefac86badadd`, `6f4ffec82e73274bebeac2ea1aa0ad31e28f4539`, `5acec491f99553ae065556b215b9104cc267bb7a` |

009 and 011's outgoing references marked UNFINALIZED use 010's pinned definitions
where supplied. Use 010 §1's exact source table T/S/M/P/V/E for those projections.
Only one admitted, authorized view supplies the operands used together. Missing
production name evidence stays unavailable under 133A and [#777](https://github.com/Kernol12/subathon-bot-ai/issues/777).
Use the exact modeled name only with matching configuration, definition and revision.
[#792](https://github.com/Kernol12/subathon-bot-ai/issues/792) owns the current event
announcement, its sequence, qualified elapsed position and reconnect observation.
An action declaration, queued job or old observation supplies none of those facts.
Milestone presentation retains its separate committed owner.

## 2. Candidate dialect, version and imports

Use standalone JSON Schema Draft 2020-12 with explicit
`$schema = https://json-schema.org/draft/2020-12/schema` and one absolute,
fragment-free `$id` per resource. Candidate base URI
`https://schemas.subathon.invalid/` identifies offline registry resources. The
coordinator confirms the path/URI map. Use `scene/v1/{document,bindings,assets,package,revision}.schema.json`
as the candidate resource layout and `common/v1/wire-primitives.schema.json` as the
candidate single common import. Use stable `#/$defs/Name` pointers and absolute
resource URIs on cross-file references. Use static `$ref` only. Local recursion for
conditions stays inside the bindings resource and consumes a child instance at each
step. Keep common → no Scene dependencies, document → typed 010/011 references,
package/revision → document and relevant definitions, with no reverse imports.

Schema IDs, document model versions and transport/protocol versions are distinct.
Select from a closed registry by exact version before interpreting that root.
Document model is `subathon.scene-document.v1`; assets/package owner identity is
`subathon.scene-assets-packages.v1`. Candidate 010 wire identity is
`subathon.scene-bindings-conditions-timeline.v1`; the name is an encoding choice.
Use exact version and kind in typed references, plus opaque resource and immutable
definition/revision IDs in the applicable namespace. Separate closed entry shapes
for content, definitions, manifest and lifecycle boundary records. No caller
selection of a schema URI or implicit version upgrade. A changed closed layout or
interpretation requires a reviewed version and explicit compatibility/migration.

Use required members, explicit discriminators, and `oneOf` over disjoint
`const` tags for variants. Close each complete object with `additionalProperties:
false`, declaring its full allowed key set at that boundary. Reuse property schemas
with `$ref`; repeat the small fixed member declarations across variants when needed.
Use arrays of typed entries for identity-indexed definitions and ordered placement,
avoiding property names derived from IDs. Instance data keys must form the selected
record exactly. No open extensions, defaults, custom vocabularies, nested resource
IDs, anchors, dynamic refs, or assertion dependence on `format` or content
annotations. Record locale/formatter/layout choices only at their owning boundary.
Keep omission, explicit null (only where an owner admits it), known empty/zero,
absent, unknown and undisclosed distinct. Disclosure can remove a whole row without
a placeholder/count. Text is Unicode scalar data without normalization. Use
common exact strings/rational objects and lowercase hex OpaqueBytes. Small ordinary
UI integers such as RGBA use JSON integer values with explicit ranges.

A well-formed but unknown required capability ID survives the version's feature-list
shape. Derive the actually needed features from the resolved content and check the
declaration and **every intended output** against the trusted versioned registry and
actual profile evidence. Owner decision 137A refuses publication or application
when a required feature is unsupported. Unknown selected versions, variants or
fields have no typed successful interpretation. An explicitly supported draft/import
path may retain the bounded original bytes with an issue, separately from the typed
content and with current access checks. Retention confers no validation or partial
rendering success. Unknown offered protocol IDs follow common005 negotiation.

## 3. Required coverage and ownership

Each node has the 009 common ID, label, family/variant, frame/pivot/scale/rotation,
lock, style, explicit attachment or no attachment, static/typed visibility and
ordered animation references. Close each family and compatible style variant
separately. Canvas includes positive extents, fit/crop/stretch, alignment and clip.
Keep the 1920×1080 aspect-fit new-scene default in its authoring owner. Validate
nonnegative frame/stroke/radius, positive font size and opacity/alignment in [0,1].
Some valid frames/scales are singular. Compose transforms in 009 order and apply
group opacity/clip to the composed subtree. Check unique typed IDs, closure, one
parent per node, reachability, acyclicity, sibling order and compatible style use.

| Node family | Required schema records and domain checks |
| --- | --- |
| Timer | Main, Grace or exact selected modifier duration reference; compatible text style/font and duration formatter. T lifecycle/phase/confirmation and pause/recovery qualifiers stay separate. Never substitute budgets, debt, or another clock. |
| Text | Literal or registered typed scalar binding, matching formatter and text style/font. Include enum/Boolean catalog labels, authorized authored labels/descriptions, elapsed displays, exact points/percent/counts/spellings; preserve locale and 010 rounding solely for display. |
| Progress bar | Typed track binding, interval/no-origin/empty/final layout and optional deficit, remaining and target slots. Preserve actual score/current target vs immutable achievement capture, exact ratio and widened signed extrema, absent first origin and unavailable fill. |
| Image | Typed visual revision, fit/alignment, 011 media profile. Asset identity and fit are explicit. Same common visibility/animation/opacity paths as other families. |
| Shape/group | Rect/ellipse/line with compatible fill/stroke and explicit endpoints or corner radius as applicable; group alone has ordered children. Radius at most half smaller frame extent; no hidden layer index or interleaving. |
| Recent support | Authorized ordered rows and closed layouts for Subscription/Gift/Bits/Donation, permitted name, original exact money/currency, matching gift count or Bits quantity. Hide rows/slots at source and preserve literal text, with no local classification or valuation fallback. |
| Announcement area | Typed current milestone or event observation and matching authored version/identity and layout. Preserve actual milestone captures and commit order, current half-open presentation window, gap, keyed hold/skip, independent pause/link and reconnect semantics. Event source remains gated by #792. |

All seven have the common 010 opacity attachment from a disclosed progress fraction
as absolute base opacity, with static opacity on unavailable input. Validate the
126A condition tree and every branch regardless of short-circuit outcome. Compare
typed exact registered operands only; no presence tests or comparisons of
display-only rows, names, money or announcement content. Absent, unknown,
undisclosed and explicit null evaluate U, including for inequality; only T activates.
Preserve 010's three-valued truth tables. Apply typed formatting after comparison.

Encode fade/move/scale/rotate endpoints with geometry units and 010 easing,
nonnegative signed64 ms start, positive duration and explicit scene-start or
qualified server presentation context. Check exclusive end within signed64, no
overlap for one node/property, and consistent context. Track override follows the
context; absent qualification uses base and removed context removes override.
Separate elapsed observations for countdown, pause/recovery basis, ActiveSubathon
and RealElapsed modifiers and server presentation. No UTC/frame-count substitution.
The five roots encode authored records and typed references. Synthetic preview inputs
remain with [SCENE-003 #437](https://github.com/Kernol12/subathon-bot-ai/issues/437).
Live facts stay with their source owners. [VIEWER-002 #486](https://github.com/Kernol12/subathon-bot-ai/issues/486)
owns supporter disclosure and [CONTRACT-007 #159](https://github.com/Kernol12/subathon-bot-ai/issues/159)
owns synchronization. Admitted observations are separate interpretation inputs.

011 definitions cover declared vs inspected vs compatible evidence tied to exact
payload/version/profile/output. Include all ten closed media profiles (PNG, JPEG,
WebP, static SVG, GIF, MP4, WebM, WAV, MP3, Ogg), every admitted stream/track/codec
variant and required field in §§2.1–2.5, plus scoped scene-level media attachments
as MediaAttachmentRef. [#804](https://github.com/Kernol12/subathon-bot-ai/issues/804)
owns the deferred trigger, mute and autoplay rules. SVG descriptor is separate
from source validation in §3 (closed XML, shapes, paths, paint, gradients,
transforms, internal reference typing and finite expansion). Include both closed
SFNT profiles (TTF/glyf and OTF/CFF), §4 table/name/cmap/outline/hint rules, exact
font/fallback selection, default Noto Sans Regular/Bold and OFL notice provenance.
No license, codec or platform compatibility follows from a declaration.

Represent immutable asset/version, exact length, SHA-256, provenance and rights
evidence; manifest scene selection, typed dependency mappings, exact inventory,
assets/fonts and notices. Only `manifest`, `scene`, `assets/<digest>` and
`notices/<digest>` under 011 §5. Contents charge actual expanded bytes. Check
transitive closure, exact lengths/hashes, explicit collision remapping, typed
references and current redistribution/delivery rights. Validate archive entry
types, actual staged expansion and containment separately from JSON metadata.

Lifecycle records distinguish draft head and its known live base, immutable
content revision, publication/history selection, current live pointer, and fresh
edit/publish/restore intent with specific target and applicable expected head.
Known-no-live is explicit. Undo/redo and restore create new identities under 009.
Scoped access and compare-and-set use current authoritative state. Valid stored
outcome retries follow their owning operation and current disclosure rules.

## 4. Limits and check precedence

Use inclusive maxima. Apply field, enclosing message, actual allocation/work and
trusted configured profile limits together. Count repeated occurrences before growth.
MiB = 2^20 bytes and GiB = 2^30 bytes. Inherited ceilings below keep the full
constraints of their cited sections, including each admitted enum/range.

| Charge and owner | Inherited or candidate ceiling |
| --- | --- |
| 005 values | Version/kind capability identifiers 64 ASCII bytes by grammar. Signed64 20 bytes incl. sign; unsigned64 20 bytes; both exact endpoints. Correlation (if enclosing transport uses it) 256 decoded bytes; CommonFailure 1024 JSON bytes. Keep endpoint-specific operation/reference limits. |
| 009 content revision §8 | 4096 nodes, 4096 styles, 4096 total placements, forest depth 64, 32768 reference occurrences, 256 feature IDs. New scene/node/style/revision/draft/publication ID 65536 raw bytes (131072 hex chars). Labels 65536 UTF-8 bytes, each literal 1 MiB, geometry 256 ASCII bytes, aggregate owned variable bytes 16 MiB. Fixed color channels 0..255. |
| 010 domain fields §§2–6 | Exact Signed64 ms/score/threshold fields and Unsigned64 differences/counts; precision 0–6. Progress projected from 64-bit threshold distances needs up to 20 decimal digits per reduced component. Retain DecimalSource factor spelling under its selected source profile (CORE-001 there uses 512 bytes, 256 mantissa digits, exponent magnitude 256); original money and any pre-existing reference retain their actual owning profile. |
| New authored 010 records, candidate | Selected Scene closure: up to 32768 exact definitions per collection and 32768 across collections, tied to the 009 reference-occurrence ceiling. 4096 nodes with four distinct animation refs and one style ref each require 16384 animation definitions and charge 20480 refs. Condition cap 4096 nodes/tree, depth 64 and 65536 nodes/entry. 32768 animation refs/tracks/entry. New definition/revision ID 65536 raw bytes, authored labels/markers 65536 UTF-8 bytes, each literal 1 MiB, 16 MiB aggregate owned variable bytes per bindings root. Authored progress-literal components up to 256 decimal digits each. |
| 011 descriptors/inventory §§1–7 | New IDs 65536 raw bytes, label/source description 65536 UTF-8 bytes, notice 1 MiB, metadata/notices 16 MiB, 4096 entries/mappings each, 32768 reference uses, SHA-256 64 lowercase ASCII hex, entry name ≤72 ASCII bytes within closed namespace, unsigned exact lengths/counts, §2.1 Q ≤32 digits/component. SVG descriptor values retain §3 exact reduced components ≤4096 bits (≤1234 decimal digits), with §3.4 intermediate components ≤8192 bits. |
| 011 payload and decode §§2.5, 3–4, 6–7 | Initial configured encoded caps 32 MiB image/font, 128 MiB audio, 512 MiB video and 2 GiB package; raster 8192 per edge and 32000000 pixels. 4 GiB staged expanded entries; 256 MiB one decoded raster, 1 GiB concurrent decoded/intermediate inspection, 16 MiB held audio, 65536 sample frames per block, 16 Mi checked work occurrences, one expensive worker/30 s per supervised attempt. All tighter per-profile source, packet, SVG expanded/coordinate/bit and font table/outline/hint limits apply. |
| Scene JSON input and output, candidate | Each entry root ≤128 MiB UTF-8 input and serialized output, including framing/whitespace/escapes, and ≤64 JSON levels for non-condition roots, ≤160 for roots with nested conditions. At most 2^22 JSON values/member names combined, 64 members/object, 2^20 items/array (tighter record caps apply), 64 bytes per decoded member name, 6 MiB per raw string token body, 32 bytes per JSON number token. Across one complete validation/export operation, all Scene JSON entry roots together ≤512 MiB serialized in each direction, ≤2^24 values/member names and ≤64 MiB decoded variable payload using owning byte measures. |
| Scene JSON intermediates, candidate | Per operation ≤1 GiB concurrent owned JSON buffers, decoded data, results, indexes and scratch combined, with ≤2^27 charged token/field/reference/graph visits including retries of branches. Charge output and staging separately in their enclosing budgets; plan projected sizes before constructing/copying. Owned unknown original bytes charge the same input/retention budgets. |
| Exact and renderer intermediates, candidate split | Bounded pose scalars need at most 851 coefficient bits from 256 decimal digits, with scale ≤254; reserve 2048-bit local decimal/range comparison operands. Two 256-digit rational components need ≤851 bits each and ≤1702-bit comparison products. Keep inherited wider domain/source arithmetic separate. World-transform, offscreen, glyph/rendered-text and frame-work budgets require Q3. |

UTF-8 scalar bytes and hex-decoded bytes are explicit charges beyond schema
`maxLength`. Hex uses two chars per byte and up to six wire bytes per escaped
ASCII character; general UTF-8 Text has at most six wire bytes per decoded UTF-8
byte. Thus a full 16 MiB of hex-owned bytes could expand to 192 MiB using escapes.
The 128 MiB wire limit makes that spelling a resource refusal while permitting
the unescaped encoding of the same values subject to all other charges. Count
raw bytes independently of the chosen encoding. Exact byte retention uses the
actual incoming spelling. The candidate operation cap is a resource boundary. A
nine-field GIF Frame object charges 19 values/member names. At 65536 frames a
descriptor charges at least 1,245,184. Four exceed the entry cap and fourteen exceed
the operation cap even if all media bytes fit. Review this refusal before adopting
the checker profile. A larger value needs a sufficient reviewed version/profile.

Proposed validation order for the detached candidate (authorization governs access
to protected inputs and results independently):

1. Select the bounded entry channel and check received/expanded bytes before growth;
   perform a bounded complete UTF-8/JSON scan, count depth/items/token bytes, reject
   BOM, malformed Unicode, duplicate decoded keys, trailing data and non-JSON constants.
   Stop at the first scan fault or bound crossing in source order after the entry cap.
2. Select exact root version from the well-formed envelope. Then check closed structural
   shapes, primitive types, presence, branch tags, local cardinality and allowed enums.
   Check decoded whole-token grammar, exact numeric ranges/reduction and byte/work
   budgets before materializing the typed graph. Use bounded digits and exact
   comparisons; check every branch/set and budget schema branch evaluation separately.
3. Check unique identities and local forest, then external typed closure, then
   cross-field/domain relations and required feature/profile compatibility. Separately
   preflight archive entries and budgets before staging and require complete
   byte/content inspection for compatible use. No lookup outside declared local
   schema resources or authorized domain sources.
4. The operation owner checks current disclosure/access, immutable intent and
   retry rules, actual rights, required compatibility and expected head at execution.
   Only that owner commits and reports outcome. Detached check ordering cannot
   leak conflicting revisions, asset existence, rights or raw payload errors.

Within each post-scan candidate phase use root kind and declared entry order,
then a fixed field order and array index for diagnostics; freeze schema/profile
field walks and typed error mapping before checker authoring. Never expose the
library's error ordering/raw values as public protocol. No HTTP/WS status,
deduplication precedence or authority is selected here.

## 5. Offline checking and review questions

Draft 2020-12 integer permits a zero fractional part. Scan unsigned small UI
integers with grammar `0|[1-9][0-9]*`, and enforce the field range. For signed
small fields also allow a minus before a nonzero permitted magnitude. Fractional
or exponent spellings of RGBA fail this lexical check. Full-value ASCII patterns
use `^(?:...)(?![\s\S])` without multiline mode and explicit `[0-9]` digits.
Count bytes separately. Schema equality covers whole values; keyed uniqueness,
hashes, gcd, graph checks, clock provenance, rights and publication require their
own checks. [Core](https://json-schema.org/draft/2020-12/json-schema-core) ·
[Validation](https://json-schema.org/draft/2020-12/json-schema-validation)

Candidate reuse of admitted `jsonschema 4.26.0` Draft202012Validator and
`referencing 0.37.0` DRAFT202012 Registry with denied retrieval, exact unchanged
hash lock and metadata admission in the isolated developer environment. Register
only reviewed resources under their explicit IDs, fail missing and duplicate IDs,
and traverse all real schema subschemas to resolve references even in unused
definitions. Keep instance text opaque. The library supports explicit in-memory
resources and retrieval policy. [jsonschema 4.26.0 referencing](https://python-jsonschema.readthedocs.io/en/v4.26.0/referencing/).

Current checks cover one OpenAPI inventory (17 components, 23 manifest examples)
and the separate progress editor schema. The HTTP checker enumerates only its
existing artifacts, asserts local-fragment refs and validates in the whole API
context. It rejects duplicate keys and literal NaN/Infinity tokens. The loader
does not bound finite-token conversions such as `1e400`. It supplies no
FormatChecker or bounded hostile decoding. It has no Scene registry,
cross-root inventory/path policy, domain checker or runtime role. Later adaptation
needs separate review and CI for trusted artifact inventory, explicit dialect/ID
registration, metaschema checks, complete offline reference walk and independent
positive/negative bindings for every root/branch. Include missing/forbidden targets,
unused refs and repeated runs for deterministic classification. Keep ADR-0008
isolation and current HTTP/progress evidence. The retired generic validator stays retired.

1. **Q1 — Domain profile.** CORE-001 supplies the admitted DecimalSource bound.
   SCENE-003 and the live projection owners select their observation and text
   bounds for money, currency, names, rows and other admitted values. #777 and
   #792 remain producer gates. These inputs stay outside the five authored roots.
2. **Q2 — Package profiles.** Under correction 159 to [011 §5](scene-assets-packages-v1.md#5-portable-closure-and-patharchive-safety),
   `scene` holds exactly one pinned content revision. Zero or multiple revisions
   are invalid. Each package has its own MEDIA-04 budget. A separate reviewed
   container profile defines framing and staging. Retained unknown originals
   require an explicitly admitted draft/import path under 137A.
3. **Q3 — Execution budgets.** Review the JSON allocation and work caps, including
   the large-GIF resource refusal, before adopting the checker profile. Renderer
   owners qualify world-transform, surface, cache, glyph and frame-work bounds
   before rendering; delivery owners qualify their corresponding boundaries.

## 6. Independent example pins and scope

Independently authored fixtures use the §1 pins below. Each mutation starts in
a fresh disposable context.

| Complete logical input or pinned case family | Control and first relevant check |
| --- | --- |
| 009 §9 S/SD01 and SD02 provide nonempty and empty document controls; SD03–14 cover forest, transforms, decimal/range, opaque and Unicode edges. | With S unchanged except shape A adds children=[], shape fails closure of its structural branch. With roots=[A,A], the same defined A is placed twice. With label Text containing escaped lone surrogate, lexical scan refuses. SD07 fit gives (0,25), crop (-50,0), stretch (0,0). |
| SD15–18, SD26–27 require all family, attachment and privacy variants; resolve SD27's attachment via 010 §2. | A and G sharing a known 1/3 opacity binding each charge one reference; G applies opacity after child composition. Unknown ratio uses each static opacity. FontRef in the image slot fails typed shape; missing correct VisualAssetRef target fails closure. Unknown required feature on browser with both outputs refuses application even if native supports it. |
| SD19–25 exercise revision/edit/publish/restore and resource boundaries. | From p2→r2, fresh authorized restore with expected p2 and target r1 selects new p3→r1 under stated owner premises; expected p1 conflicts for fresh intent. At 65536 raw ID bytes the isolated field fits; 65537 refuses; graph root depth 64 fits and 65 refuses. |
| 010 §7 T/V/M/S/E/D/P rows pin every selected value, clock, 126A, privacy and preview oracle. | S=1, P=0, N=3 µpt gives exact 1/3, formatted 33.33%; before first has no ratio. `any(T,U)=T` while `any(T,lifecycle > Active)` is invalid. Gift count on Bits fails typed shape. EUR `9007199254740993.25` stays exact and withheld money has no substitute. |
| 010 §7 current presentation and animation rows. | A starts at 0, gap at 10000–10999 ms, B starts at owner turn 11000; reconnect 11500 sees B at 500 ms, no replay or fabricated score. Queued event alone has no current observation. Tracks [0,500) and [500,750) are adjacent; [499,749) overlaps. Start signed64 max + duration 1 is invalid arithmetic. |
| 011 §8 AP01–29, MD01–10 and alternate codec branches, FD01/02 and hinted FD01, S01–05 and controls. | MD01 declared width 8192×3906 fits the raster pixel bound, ×3907 exceeds it; MD04 delay 2 is 1/50 s, 1 outside profile. FD01 face 0 vs 1, S02 use of tile vs gradient, S05 n=18 (1048573) vs n=19 (2097149) distinguish field/profile/reference/work checks. Matching descriptors still require actual inspected bytes, rights and output evidence. |
| AP07 logical manifest closure under matching entry/length/hash premises. | A second identical `assets/<digest>` fails inventory. An actual digest different from the declared digest fails integrity. A forbidden entry type/path fails before extraction. An output flush failure cannot yield a complete portable artifact. |

Candidate spelling of the complete empty revision (SD02, structural scope):

```json
{
  "modelVersion": "subathon.scene-document.v1",
  "sceneId": "53", "revisionId": "7231", "label": "",
  "intendedOutputs": ["native", "browser"], "requiredFeatures": [],
  "canvas": {"width": "200", "height": "100", "fit": "aspect-fit",
             "alignX": "0.5", "alignY": "0.5", "clip": true},
  "roots": [], "nodes": [], "styles": [], "externalReferences": [],
  "mediaAttachments": []
}
```

Replace canvas width alone with JSON number 200 for structural rejection or
"+200" for grammar rejection. Roots ["41"] create a dangling ID. A root
"credential" member fails object closure.

CONTRACT-008 audits 009/010/011/012 and reference-scene coverage before HTTP006
and CONTRACT-003. Downstream Scene, renderer, API/WS, viewer and deployment work
needs its own tests, review and CI. Native/browser conformance, font licensing,
60/30 FPS and Windows/Linux/OBS/browser evidence remain release gates.
