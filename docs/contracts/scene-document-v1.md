# Scene document model — proposal v1

CONTRACT-009 first logical document stage, from main `b2b14bb` and claim `24752c0`.
Authority: specification v0.45 §11, API-01/03, UI-01/02/03/04 and MEDIA-02;
[task](../tasks/CONTRACT-009.md),
[ADR-0005](../decisions/0005-scene-contract-prerequisites.md) and reviewed
[common values](wire-primitives-v1.md), blob `11eeb5f4`. Proposed model identity:
`subathon.scene-document.v1`. It is not an advertised or implemented capability.

This fixes a protocol-neutral logical structure and independent examples, not a
JSON schema, C++/TypeScript API, decoder, executable fixture or renderer. Different
actual document review precedes another stage. CONTRACT-010 owns disclosed data,
conditions, formatting and presentation/animation time; CONTRACT-011 owns assets,
fonts and packages. Their referenced definitions are explicitly UNFINALIZED here.
CONTRACT-012 must resolve them in complete reviewed schemas before 008 can pass its
integration gate. No partial document is a complete publishable scene package.

## 1. Values, identities and ownership

All identities are nonempty opaque byte sequences, with exact byte equality and
unsigned-byte ordering only where ordering is explicitly needed. They are not
Unicode labels, paths, UUIDs, counters, clocks, credentials or permission grants.
Common OpaqueBytes supplies a later wire mapping; no text decoding, trimming,
case folding or NUL termination changes their identity. Common Text supplies labels
and authored literal text, preserving scalar sequences without normalization.

| Identity | Scope and meaning |
| --- | --- |
| SceneId | Stable saved-scene resource within its owning installation |
| SceneRevisionId | Immutable content revision within one SceneId |
| NodeId | Stable node identity within one SceneId, retained across edits while it is the same node |
| StyleId | Style definition within one content revision, a separate namespace from NodeId |
| DraftId | Editing workspace within one SceneId |
| PublicationId | Immutable publication selection/history entry within one SceneId |
| Typed external reference | Its owning contract, kind, resource identity and immutable definition/version reference; not an untyped path |

Different namespaces may contain equal bytes; a reference's type and scene scope
are essential. A revision never acquires different content under the same identity.
Removing a node does not make a different replacement the old node: do not reuse a
retired NodeId for a different node within that scene's retained history. Copies
into a different SceneId have a different scope. Import/merge remapping must be
explicit and update the complete reference set; a decoder cannot silently alias IDs.
Identity generation, installation persistence and retention are downstream work.

A content revision owns its label, canvas, ordered roots, finite node definitions,
typed style definitions, required feature identifiers and typed external references.
It contains no live authority, connector payloads, private raw server objects or
renderer-native objects. Labels may repeat or be empty; node/style identities may
not. Node labels do not determine identity, graph order or locale-dependent sorting.
Authored text is literal content, not automatically translated UI text or markup.

## 2. One ordered ownership forest

The scene owns an ordered list of root NodeIds and a finite set of node definitions.
Only the group variant of the shape/group family owns an ordered child list.
Every node occurs exactly once, either as a root or one group's child. Every node
definition is reachable from a root; no orphan, duplicate placement, second parent,
self-reference, missing child or containment cycle is valid. A non-group has no
child list. An empty scene, empty root list and empty group are representable.

Sibling order is back to front. A group occupies one sibling position; its children
are composed in their own order within that subtree. Descendants do not interleave
with the group's siblings. There is no second numeric z-index that can disagree
with list order. Reordering an explicit list expresses a layer change; sorting
NodeIds or labels would change the document's meaning.

Each node owns an explicit family/variant, label, frame/transform, editing-lock
state, compatible StyleId, binding-attachment choice, visibility choice and ordered
animation references. Its binding attachment is explicitly NoBindingAttachment or
an ElementBindingsRef owned by 010. This common slot is available to all seven
families, including image, intrinsic shape and group; visibility/animation references
cannot stand in for it. NoBindingAttachment means no definition attached at this
slot, not omission or an unknown value, and does not erase family-specific content
BindingRefs below.

010's permitted binding destinations and value types, relationship to family content
BindingRefs, resolution/override precedence, conflicting-use rules and unavailable-
value behavior are all UNFINALIZED. This slot promises neither that every property
is dynamic nor that a binding may override a static frame/style value. No arbitrary
property bag, raw server field path, script or accounting-to-geometry coercion is
introduced. An unresolved ElementBindingsRef remains a precise contract dependency,
not a complete or publishable binding; NoBindingAttachment cannot make other
unresolved content references valid.

Visibility is explicitly static-visible, static-hidden or a ConditionRef owned by
010; omission is not hidden/visible/unknown. Empty animation references means no
authored animation, not an unknown clock. Static hiding affects presentation only:
it does not stop a timer, deactivate a modifier, retract an achievement or pause
the authoritative announcement queue. Those meanings remain with 010's owners.

Local StyleId references must resolve once to the correct style kind in this
revision. Geometry/containment/style references cannot depend on future runtime
server lookups. External bindings/assets have a different, explicit boundary below.

## 3. Canvas, geometry and composition

Coordinates are design pixels, with origin at the canvas top-left, x rightward and
y downward. Design pixels are layout units, not physical display pixels, DPI,
countdown milliseconds or UTC instants. Canvas width/height are strictly positive.
New-scene design defaults remain the specified 1920×1080 with aspect-fit output;
custom positive extents and crop/stretch remain available. Stored revisions record
their actual extents, fit mode, alignment and canvas-clip choice explicitly.
No new alignment, clipping, color, text or editor default is selected here.

Geometry values use common canonical FiniteDecimal, under the scene-only profile
in §8. Width/height and stroke/corner widths are nonnegative; positions, pivots,
rotation and scales may be signed. Opacity and alignment factors are in [0,1].
Font size is strictly positive. Zero-size frames and zero scale are representable
degenerate geometry, not unknown data or an implicit visibility instruction;
explicit content/clip styles still apply. Negative scale expresses reflection; negative
width/height is invalid. No NaN, infinity, binary-float coercion, exponent spelling,
negative zero or silent rounding belongs to the stored exact value.

A frame records x/y, width/height, pivot x/y, scale x/y and rotation in degrees.
For a local column point p, its parent-space point is:

`T(x,y) · T(pivot) · R(rotation) · S(scale_x,scale_y) · T(-pivot) · p`.

Positive rotation is clockwise in the stated x-right/y-down coordinates. Parent
transforms multiply outside child transforms. Frame dimensions define the local
content/clip rectangle; they do not silently resize child coordinates. A group
resize intended to scale descendants must express a transform or explicit child
edits. A singular transform is valid data; an editing operation needing its inverse
must handle that case explicitly rather than manufacture an inverse or position.
Trigonometric rendering is not exact money arithmetic; complete numerical rendering
tolerances/oracles still require separate conformance review. Bound/captured domain
values keep their own exact type and limits, never this geometry scalar profile.

For canvas (cw,ch) and positive viewport (vw,vh): aspect-fit uses the smaller of
vw/cw and vh/ch as one uniform scale; crop uses the larger and clips the viewport;
stretch uses separate x/y scales. Explicit alignment factors ax/ay position remaining
space as `(vw - scaled_width) * ax`, `(vh - scaled_height) * ay`. Negative remaining
space in crop has the same rule. Canvas clipping, when chosen, clips to its design
rectangle before output mapping; a viewport is always a finite output surface.
These are geometry definitions, not an instruction to use an accounting finalizer
or round intermediate coordinates to whole physical pixels.

## 4. Typed styles and the seven element families

Style definitions are owned declarative values, not CSS, QML, HTML, JavaScript or
arbitrary name/value bags. Every style has a compatible typed role. Common
presentation properties are explicit opacity and clip-to-frame. Paint has three
sRGB-encoded color channels and one linear alpha channel, each integer 0..255,
with nonnegative stroke width where a stroke is present. A missing optional stroke
means no stroke, not a guessed color.
No style inheritance or renderer-native default supplies omitted required data.

Shape styles describe a bounded intrinsic primitive (rectangle, ellipse or line),
its explicit fill/stroke, and rectangle corner radius where applicable. A rectangle
radius cannot exceed half the smaller frame extent; do not silently clamp it.
Lines have
explicit endpoints and stroke; their endpoints use the same local coordinates.
Rich static SVG paths/gradients/internal references remain required through 011's
asset profile; this primitive list does not remove them from release scope.
Text styles name explicit foreground, positive size, alignment, wrapping/overflow
choices and FontRef from 011; text remains literal scalar content. Visual-media
styles record fit/alignment explicitly. Bound collection/progress layouts carry
typed PresentationLayoutRefs from 010 rather than arbitrary templates or markup.
Concrete closed property records and their canonical representation must be frozen
under 012; an unrecognized property cannot silently become a native style extension.

Groups compose their ordered children as a subtree, then apply the group's opacity
and clipping to that composite; a child's own opacity has already applied within
it. This is a logical composition rule, not a requirement to allocate an unbounded
offscreen surface. Detailed color/text rasterization, blend tolerances, font metrics
and output conformance still require independently reviewed renderer profiles and
finite intermediate budgets. None is established by these style values alone.

| Family | Content and typed reference boundary |
| --- | --- |
| Timer | TimerBindingRef and text/formatting presentation from 010; Main, Grace and modifier duration remain distinct owning-domain values |
| Text | Literal Text or TextBindingRef from 010, with explicit text style and 011 FontRef; no implicit expression evaluator |
| Progress bar | ProgressBindingRef and PresentationLayoutRef from 010, retaining exact target/deficit/absent-origin semantics and configurable completion layout |
| Image | VisualAssetRef from 011, including its static/animated/video profile and explicit fit; a filename/container is not verified support |
| Shape/group | Intrinsic shape leaf with shape style, or group with ordered children and group style; group identity is not inferred from an empty child list |
| Recent support | RecentSupportBindingRef and PresentationLayoutRef from 010; selection, source privacy, hidden amounts and authorized projection remain authoritative |
| Announcement area | AnnouncementBindingRef and PresentationLayoutRef from 010; event/milestone presentation follows the server's current sequence, not a second award queue |

Every row additionally has the common binding-attachment choice from §2. Image and
both shape/group variants are not restricted to static data merely because their
intrinsic content references differ. Their actual admissible destinations still
require 010's reviewed typed registry, with no new dynamic-property claim here.

Scene-level typed MediaAttachmentRefs preserve the audio/other package attachment
seam; 011 owns the asset/profile and 010 any timing/trigger reference. They do not
create an eighth family, autoplay rule, new audio default or implicit network fetch.
All seven families are part of the model even though their complete 010/011 content
definitions are not finalized by this document. An authoring draft may preserve
pending references; it cannot treat an unresolved family as an empty successful one.

## 5. References, versions and required features

A typed external reference identifies the owning contract/version, reference kind,
resource and immutable definition/version identity. Kinds include Timer/Text/
Progress/RecentSupport/AnnouncementBindingRef, ConditionRef, AnimationRef,
ElementBindingsRef, FormattingRef, PresentationLayoutRef (010), and VisualAsset/
Font/MediaAttachment/PackageRef (011).
Equal resource bytes in different kinds do not allow substitution. No raw field
path into server JSON, script, URL, local designer path or credential can replace
one of these references. Definitions, field layouts and closure rules supplied by
010/011 are UNFINALIZED; declaring a reference does not resolve or validate it.
UNFINALIZED is a development-stage contract gap, not a valid runtime unknown or
unsupported-success value. Actual absent/unknown/not-disclosed/unsupported states
must use those owners' reviewed meanings once defined; this document invents none.

Internal node/style references must already be closed under §2. A complete scene
package additionally requires every external reference's actual compatible reviewed
definition, transitive asset/font closure, limits, privacy rules and feature/profile
requirements. Unknown, missing and unsupported are not available/supported. A draft
may retain a precise unresolved reference as an editing issue; publication eligibility
cannot use universal unknown, a placeholder registry or an empty family as success.

The document records its exact model version, unique required CapabilityIdentifiers
and explicit intended outputs (native, browser or both). Required feature/capability
identifiers use common identifier grammar; authored scene/node labels remain common
Unicode Text under §1/SD14, without an ASCII identifier restriction or normalization.
Domain identities keep their separate byte type. Required features must
cover actual nodes/styles/references, not just a caller's shorter claimed list.
Feature-to-definition imports remain 010/011/012 work. A required unknown version,
node/style property or capability refuses compatible publication/consumption rather
than being ignored, deleted or downgraded. Unused unknown offered versions follow
the common negotiation rules; they are not selected document semantics.

For both intended outputs, all requirements must be supported by both outputs'
actual compatible profiles before publication. A document or client claim proves
neither implementation, platform codec support nor permission. The editor must
report unsupported features before publishing. A draft can retain unsupported
requirements for editing/export with an explicit issue; it is not render-ready.
Concrete compatibility errors, transport framing and error precedence are not
selected by this logical document.

## 6. Drafts, publication and restore

A SceneRevision is immutable content. A DraftId has a current content-revision head
and records the live PublicationId it was based on, or an explicit known-no-live
state. Unknown current publication is not that known absence. Draft creation/editing
does not change the live selection. Every accepted edit, including undo/redo,
produces a fresh immutable SceneRevisionId; equal restored content does not rewind
or reuse the earlier draft-head identity.

A PublicationId identifies an immutable selection of one content revision. Each
accepted publish or restore produces a fresh publication identity/history entry,
even when selecting previously published content. Historical content and previous
publication entries remain unchanged. The live head selects the current publication,
not a mutable copy of its node graph. A restore may select an old content revision
in a new publication; it does not rewrite history or reset unrelated state/clocks.

A fresh edit needs the expected current draft head; a fresh publish/restore needs
the expected current live publication (or authoritative known-no-live state), the
specific immutable target and current execution-time rights. A stale draft/live
expectation conflicts without silently overwriting a newer head. Rebase/merge or
reselection is explicit, never a field-by-field last-writer overwrite. Structural
compatibility and dependency closure must also hold at actual execution.

These are logical mutation preconditions, not an endpoint/transaction/API design
or universal retry order. The owning operation must retain stable intent and check
current disclosure, deduplication and actual committed outcomes. A valid stored
result retry cannot simply be rejected because today's live head differs. A timeout
does not prove rollback. A caller-supplied PublicationId, revision, preview,
capability or permission-shaped record proves neither authority nor currentness.
SCENE-007/ACCESS and the server/writer own actual rights, atomic comparisons,
persistence, publication/restore and safe outcome reporting; clients do not write
authoritative state. Viewer credential/revocation and live synchronization remain
their existing tasks, with no credentials embedded in a scene or package.

## 7. Editing, themes and preview boundaries

Drag/resize/align/distribute/snap change explicit frames/transforms. Group/ungroup
and layer operations change one coherent ownership forest and its order. Locks are
explicit editing state for a node/subtree, not authorization or a visibility rule;
editor gesture handling/unlocking still needs its own acceptance. Undo/redo stores
or reconstructs valid candidate revisions without mutating committed history or
using obsolete head identities as current. Canvas changes must preserve the chosen
explicit edit/scale result rather than silently rescale accounting or child values.

Reusable themes refer to the same model and typed dependencies. Import/export
requires 011's portable package closure and permitted font redistribution; relative
references are not arbitrary filesystem paths. There is no designer-local success
mode, script download or credential in a theme. Publication and runtime access are
independent of a package's integrity or syntactic validity.

Preview is a clearly separate synthetic context with an explicit candidate revision
and typed 010 sample inputs. Lifecycle/modifier states, deficits, large contributions
and long names/descriptions must remain representable. A preview cannot start a
timer, resolve a hold, create an achievement or release an action. No missing score,
first-interval origin, provider amount, disclosure state or clock is invented for
layout. Actual preview/editor and both renderer behaviors need separate reviewed
oracles and tests; no screenshot or renderer result is supplied here.

## 8. Finite structural profile proposal

Proposed review profile `scene-structure.v1` bounds this logical layer only. These
are technical admission proposals, not measured performance, new product defaults,
caller-selected authority or configurable MEDIA-04 upload limits. They do not narrow
external domain references, exact accounting, 010 data or 011 asset contents.

| Scene-owned structural quantity | Proposed inclusive maximum |
| --- | ---: |
| Nodes; style definitions | 4096 each |
| Root depth = 1, each child depth = parent + 1 | 64 |
| Root/child placement entries in total | 4096 |
| Reference occurrences, including repeated local/external uses | 32768 |
| Required feature identifiers | 256 |
| One new Scene/Node/Style/Draft/SceneRevision/Publication identity, raw bytes | 65536 |
| One label, UTF-8 bytes | 65536 |
| One authored literal text, UTF-8 bytes | 1048576 |
| One scene geometry FiniteDecimal, ASCII bytes | 256 |
| Aggregate owned variable bytes in one logical content revision | 16777216 |

Rationale: node/style counts bound traversal and lookup work; depth separately
bounds containment recursion, and the reference count bounds multiple uses per
node instead of assuming one use each. A 64 KiB new-identity allowance deliberately
avoids borrowing the common transport-correlation cap and admits long byte IDs.
The label/text allowances preserve long authored content while the 16 MiB aggregate
prevents multiplying every per-field maximum across thousands of nodes. The
256-byte geometry bound limits that scalar's construction work only, with no claim
about accounting or runtime raster cost. These are concrete first-review ceilings,
not benchmark-derived capacity. A later required valid value needs an explicitly
reviewed sufficient profile/version, never a hidden smaller field cap or rewriting
of retained data. Profile adoption/change and concrete allocation safety remain
review gates before decoding; these numbers cannot certify a scene's performance.

Aggregate bytes charge every occurrence of a scene-owned identity/reference, Text,
geometry/number spelling and version/feature label, including repeated uses; sharing
equal bytes earns no discount. Common fixed grammar bounds still apply to feature
labels and color channels. An external reference's 010/011 identity uses its owning
profile, with its actual bytes also charged to this aggregate: never impose the
new scene-identity maximum on a pre-existing domain ID by convenience. Reference
counts include each ElementBindingsRef attachment use, even when multiple nodes
share its identity; its actual reference-component bytes are charged at each use.
NoBindingAttachment adds no external-reference occurrence or referenced payload;
012 must still budget its concrete representation/tag/framing. No unfinalized 010
definition body is materialized or assumed to cost zero through this reference.
Collection limits and aggregate bytes apply together, using checked counts before copies or
numeric construction. A resource refusal is explicit, not truncation, fitting-prefix
acceptance, omission, guessed zero or a rewritten identity.

This is not yet a complete wire/decoder allocation contract. 012 must freeze the
actual field walk, wire/text/hex expansion and framing limits, nested 010/011 input,
output ownership, scratch/intermediate bounds and deterministic failure priority
before any decoder/code baseline. For example, 65536 raw identity bytes require
131072 hex characters before framing; Text escaping has separate expansion. A
geometry bound must never route an exact 010 ratio/accounting value through this
smaller profile. No selected parser, profile negotiation or allocator guarantee
follows from these proposed numbers. Different review is required before adoption.

## 9. Independent logical examples

These are hand-authored conceptual records/tables, NOT a serialization grammar or
executable fixture. Rows use independent disposable contexts: a variation of S is
not a mutation under S's retained revision identity. `b"A"` means the single raw
byte 41; decimal quotes denote common FiniteDecimal values. Each example states
its validation scope. A complete
structural example still proves no full 010/011/schema/renderer compatibility.

All nodes in SD01–26, including SD15's family rows, explicitly use
NoBindingAttachment at the common slot. Their stated family-specific content
references remain unchanged; 010 still owns their relationship. This is an example
condition, not an implicit missing-field or product default. SD27 has explicit
nonempty attachments instead.

S is this explicit structural base: model `subathon.scene-document.v1`, scene b"S",
revision b"r1", empty Text label, intended outputs native and browser,
canvas ("200","100"), aspect-fit, alignment ("0.5","0.5"), canvas clipping true;
roots [b"A"], one rectangle shape A with label "panel", frame ("0","0","100","40"),
pivot ("0","0"), scale ("1","1"), rotation "0", unlocked, static-visible, no
animations and explicit NoBindingAttachment. Its local style b"paint" has opacity
"1", clip-to-frame false, solid
RGBA (0,0,0,255), no stroke and corner radius "0". No external dependencies or
media attachments. Required features are [example.rectangle.v1]; both hypothetical
example profiles understand this synthetic feature and model version. This is not
an advertised product capability or a final feature-to-model registry.

SD15's family records below use S's explicit frame/transform/lock/visibility and
empty animations, renaming the node as shown. T/X/P/I/G/R/N denote their ASCII-byte
NodeIds; local style names denote their ASCII-byte StyleIds. Every local style is explicitly
defined with opacity "1" and clip-to-frame false. TextBase additionally has black
foreground RGBA (0,0,0,255), size "24", start/top alignment, word wrapping, clipped
overflow and the pending FontRef (011, b"body", b"font-r1"). MediaBase has aspect-fit
and alignment ("0.5","0.5"). BarBase/ListBase/BannerBase have the respective typed
010 layout references below. These are literal partial reference examples, not
complete documents or a way to accept missing definitions.

| SD15 family row | Node and literal reference uses | Local style role |
| --- | --- | --- |
| Timer | T; TimerBindingRef (010, b"main", b"binding-r1") | TextBase; FormattingRef (010, b"timer-format", b"format-r1") |
| Text | X; TextBindingRef (010, b"headline", b"binding-r1") | TextBase |
| Progress bar | P; ProgressBindingRef (010, b"progress", b"binding-r1") | BarBase; PresentationLayoutRef (010, b"bar", b"layout-r1") |
| Image | I; VisualAssetRef (011, b"visual", b"asset-r1") | MediaBase |
| Shape/group | A from S; or empty group G with its explicitly empty child list | S's shape style; or GroupBase with the two common properties only |
| Recent support | R; RecentSupportBindingRef (010, b"support", b"binding-r1") | ListBase; PresentationLayoutRef (010, b"rows", b"layout-r1") |
| Announcement area | N; AnnouncementBindingRef (010, b"announcements", b"binding-r1") | BannerBase; PresentationLayoutRef (010, b"banner", b"layout-r1") |

Here 010/011 abbreviate the named owning contract/version slot, not actual finalized
version identifiers. Opaque resource names such as b"main" do not certify their
domain meaning. Literal text may replace X's binding by an explicit literal choice;
its FontRef remains a pending 011 requirement. No profile/clock/font is invented.

| ID | Independent input / condition | Required logical result and scope |
| --- | --- | --- |
| SD01 | S as specified | Closed ownership/style/geometry structure; one root and one node, no missing external definition; not a complete machine-schema or rendering oracle |
| SD02 | S with root list [], no nodes/styles and explicit empty feature/dependency lists | Empty structural scene is representable; creates no implicit timer/text/default element |
| SD03 | Roots [G,B]; group G children [A,H]; group H child [C]; B/A/C leaves, all explicit compatible styles/frames | One forest; A then H/C compose inside G, then B is in front of G; ID sorting cannot replace these orders |
| SD04 | SD03 with A repeated under H as well as G, or root B also a child of G | Invalid duplicate placement/second parent; identical bytes do not justify an alias |
| SD05 | Two node definitions b"A" with different labels, or missing child b"Z", or an unreachable defined node | Invalid duplicate identity, dangling reference or orphan respectively; do not overwrite/drop definitions |
| SD06 | G children [G], or G→H→G; or shape A carries children | Invalid containment cycle/self-cycle or non-group children; an empty root list cannot conceal the cycle |
| SD07 | S into viewport ("100","100") | Fit scale 1/2, painted canvas 100×50, offset (0,25); crop scale 1, 200×100, offset (-50,0); stretch scales (1/2,1), 100×100, offset (0,0) |
| SD08 | Position (10,20), pivot (0,0), scale (2,1), clockwise rotation 90°, local point (2,0) | Parent point (10,24); applying rotation before scale would incorrectly give (10,22) |
| SD09 | Group translation (10,20), child translation (3,4), all other transforms identity | Child origin is (13,24); parent coordinates are not reset at the group boundary |
| SD10 | Width "0" and scale_x "0"; versus width "-1", canvas width "0", opacity "1.1" | First is valid degenerate geometry; latter three are invalid range values; none denotes unknown |
| SD11 | Geometry "0.5", "-0.25" versus "-0", "1.00", "1e3", NaN | First two are canonical exact values; latter spellings/NaN are not this geometry type; preserve accounting through its separate owning types |
| SD12 | Node b"\x00\x80\xff" and distinct node b"\x00" | Both nonempty and distinct; common hex spellings 0080ff and 00 preserve NUL/high bytes; no UTF-8 or C-string collapse |
| SD13 | One NodeId is exactly 4097 bytes 4c; same bytes appear once in its root reference, sufficient aggregate budget | Fits the proposed 65536-byte new-ID profile; 8194 hex characters per occurrence, not a 256-byte correlation or geometry limit |
| SD14 | S's label "e\u0301" versus "\u00e9"; separately X's partial text content is literal "<b>&" | Distinct labels stay distinct; literal text is not markup or a reason to normalize/execute it; X still awaits its FontRef |
| SD15 | Timer T, TextBinding text X, Progress P, Image I, RecentSupport R and Announcement N as typed families, each with the matching 010/011 references and compatible styles | All six families complement S's shape/group family structurally; their unresolved definitions prevent a claim of complete semantic/package validity |
| SD16 | TimerBindingRef names Main versus Grace versus modifier duration; ProgressBindingRef before first interval | Retain distinct typed reference intents; this model supplies no clock, zero origin, fabricated percentage or fallback value; actual definitions await 010 |
| SD17 | Image I references an absent VisualAsset definition or a FontRef is used as its visual asset | First has unresolved external closure; second has wrong reference kind. Neither is a portable/render-ready package or permission to fetch a local path |
| SD18 | Required synthetic feature example.spark.v1 unknown to browser, native understands it; intended outputs both | Cannot publish as compatible with both; keep the requirement as a precise draft issue, do not discard the node or infer support from the identifier |
| SD19 | Scene b"S" / revision b"r1" is also used for modified content | Invalid immutable-identity reuse. Equal StyleId/NodeId bytes in their separate namespaces are allowed only with correctly typed references |
| SD20 | Draft D at r2, based on p1; live head p1; fresh authorized publish selects r2 | Logical result may create fresh p2→r2 while p1→r1 and r1/r2 stay immutable; this is a mutation example, not a commit/permission assertion |
| SD21 | Same draft expectation p1, actual live p2, or edit expects r2 while actual draft head r3 | Fresh mutation conflicts; newer head is not overwritten. A previously committed retry follows its own preserved result/current disclosure contract |
| SD22 | Live p2→r2; authorized fresh restore of r1 with current expected p2 | New p3→r1, preserving p1/p2 and r1/r2; a stale caller expecting old p1 cannot be fooled by content returning to r1 |
| SD23 | Undo r3's edit restores r1's content in a draft | Fresh r4 with that content, not mutation of r1 or reuse of old draft head r1; publication remains unchanged until a separate valid publish |
| SD24 | Root at depth 1 and chain ending at depth 64, versus an additional child at depth 65 | First fits proposed depth limit; second explicit structural resource refusal, no truncation of the subtree |
| SD25 | 4096 nodes vs 4097; identity 65536 vs 65537 raw bytes; aggregate 16777216 vs 16777217 | At-boundary values fit those proposed individual limits when all other invariants/limits fit; one-over refuses. No permission or performance inference follows |
| SD26 | Theme contains valid graph but substitutes a designer-local font path, renderer QML/JS/HTML program/object, or an account-credential field for typed scene data | Not a complete valid scene dependency; typed package/font closure and no-script/no-credential boundary are mandatory. Literal Text resembling markup remains literal Text, as SD14 states |
| SD27 | PARTIAL variation of S: root group G owns shape A, with GroupBase from SD15; G and A each use ElementBindingsRef (010, b"appearance", b"binding-r1") | Two attachment-reference occurrences, charging the 20 known resource/definition bytes twice (40), plus actual kind/version bytes. Shape/group destinations, value types, static-value precedence and unavailable behavior remain UNFINALIZED 010; no property is made dynamic or publication-ready by this unresolved shared reference |

## 10. Completion and later evidence

009 document review cannot finish 008/003, choose 012's schema dialect/tools or
authorize generated clients/decoders. 010 must supply actual owning models for
disclosure, conditions, formatting, exact progress and separate authoritative clocks;
011 must supply complete assets/fonts/packages and required media/SVG profiles.
They cannot fill those gaps with unreviewed fields or universal unknown success.

All §11 media targets, static SVG text-to-path/rejection rules, TTF/OTF/fallback,
shared Noto Sans Regular/Bold/OFL and configurable 32/128/512 MiB/2 GiB and
8192-edge/32 MP limits remain required, under 011 and actual implementation tasks.
Those asset limits are separate from §8's logical document proposal. Exact safe
decode/execution/animation memory profiles and distributed font pins/notices remain.
Shared reference scenes must independently verify bindings, visibility, geometry,
progress and elapsed animation timing on native QML and browser renderers. Minor
font raster differences do not waive those requirements. Desktop-only full editing,
pre-publication compatibility checks, 60 FPS target/30 FPS mode, supported-platform
media deployment and actual Windows/Linux/Wayland/OBS output retain their existing
SCENE/RENDER/test-first/performance/platform/packaging/release gates. No passing
result or supported capability is claimed by this proposal.
