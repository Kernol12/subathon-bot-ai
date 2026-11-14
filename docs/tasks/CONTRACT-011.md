# CONTRACT-011 — Scene assets, fonts and portable package profiles

Parent integration gate: [CONTRACT-008](CONTRACT-008.md). Kind: contract.
Authority: specification v0.45 §11, MEDIA-01/03/04/05/06, UI-04.
Proposed prerequisites:
CONTRACTPLAN-002 and CONTRACT-009. See [ADR-0005](../decisions/0005-scene-contract-prerequisites.md).

## Scope and acceptance

Define protocol-neutral server-owned asset/font identities, versions, metadata,
package closure and portability. A theme/scene package includes all required assets
and permitted fonts, not a designer's local paths, an arbitrary external URL or an
authorization credential. Specify safe reference/path/archive rules, duplicate and
dangling assets, integrity/version mismatches and import/export rejection cases.
Do not introduce an archive library or package codec without a later explicit claim.

Retain static PNG/JPEG/WebP and restricted static SVG; animated GIF; MP4/WebM video
and WAV/MP3/Ogg audio with explicitly documented codec profiles. Define the profile
vocabulary and required compatibility/rejection evidence. Unknown codec/platform
support remains unknown. Choosing a container/import format does not establish
support, and this document does not silently remove a release format.

Specify the initial SVG subset: basic shapes, paths, groups, fills/gradients,
strokes, transforms and internal references. Exclude scripts, external resources,
embedded HTML, SVG animations and filters; require text converted to paths.
Unsupported content rejects rather than being silently reinterpreted. Actual
validation and both renderer implementations require separate reviewed tests.

Retain configurable initial maximums of 32 MiB per image/font, 128 MiB per audio,
512 MiB per video and 2 GiB per packaged scene; raster limits of 8192 pixels per
edge and 32 megapixels total. Preserve authorized adjustment, decoded-dimension
validation and demanding-scene warnings. Specify finite compressed/decoded/intermediate
memory, count and execution profiles for review before implementation; file size
alone never proves safe decoding. Do not claim these defaults are measured performance.

Support uploaded TTF/OTF, explicitly configured fallback and the same font assets
for both renderers. Preserve bundled Noto Sans Regular/Bold for built-in interfaces
and default themes, SIL OFL/notices separate from GPL, and theme redistribution only
where permitted. State requirements for actual font version/hash/weight/license
pinning and packaging validation; do not claim nonexistent assets were checked.

Provide independently derived valid/invalid logical examples for package closure,
opaque references, paths/duplicates, restricted SVG/internal references/text, each
media/profile state, encoded versus decoded resource boundaries, fonts/fallback and
redistribution metadata. Distinguish declarative profile requirements from actual
codec/SVG/font support. Do not create a portable-success fixture from untested media.

## Ownership and completion boundary

Initial author owns a new asset/package logical contract and author evidence only,
with exact paths assigned at claim. No new assets, schema, tool/dependency, validator,
decoder, fixture, renderer or production code is authorized by the document stage.
Root owns shared schemas/imports, state/HTML/maps, dependency pins/build/CI.

Different actual review precedes 012 schema integration. SCENE-005/006 implement
safe upload/validation/import/export; RENDER-004 separately implements both media/
font paths; SPIKE-004 and RENDER-090/091/092 retain minimum OBS/backend/platform
evidence. PACKAGE/RELEASE retain distributed fonts/notices and source obligations.
008 remains the full integration gate. Record exact revisions and actual project
consistency/current-HTML/whitespace checks; follow [workflow](README.md).
