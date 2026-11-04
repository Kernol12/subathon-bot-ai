# CONTRACT-009 scene document author handoff

Date: 2026-11-04. Author: `/root/codec_author`. Status: revised after actual
changes-required review; the same different reviewer must assess the correction.
No schema, API, executable fixture, test,
decoder, renderer or product behavior is supplied or approved by this note.

## Assignment, source and ownership

Started clean on `task/CONTRACT-009-document-author`, isolated worktree
`local-evidence/worktrees/contract009-author`, at claim
`24752c0150ea8ff63f8740a4e78e2b98c2a3e065` from main
`b2b14bb5cc6d509f45784c55d5dbed7e55f7556b`. Local state records CONTRACT-005 and
CONTRACTPLAN-002 done and this bounded document claim in progress. Root reported
PR60's normal merge after its actual final-head gates; this author inspected local
ancestry/state and did not independently query GitHub or perform publication.

Read root/schema instructions, the 009 brief, original/full parent008, ADR-0005,
specification §11 and its API/UI/MEDIA boundaries, approved common005, architecture,
workflow/test-first rules and the actual scene-plan review. The author's earlier
independent review of another author's planning documents does not approve this
new scene-model document; a different reviewer must assess the actual candidate.

| Actual input | Git blob |
| --- | --- |
| [Specification v0.45](../specification.md) | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| [Common wire values](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [ADR-0005](../decisions/0005-scene-contract-prerequisites.md) | `e4bb60c82007a48e9ec4f7a0f7978d290d551bf4` |
| [009 brief](../tasks/CONTRACT-009.md) | `2ac98e5ddceff93bd4aa3347704d7e9eda038fed` |
| [Actual scene-plan review](contractplan-002-review.md) | `de522f0b3df0c112e34135dec84256c773babe43` |
| Claim state | `1bf24b9cc963cee493a572323a669dfae1b2b832` |

Owns only NEW [scene-document-v1.md](../contracts/scene-document-v1.md) and this
note. Root owns all shared state/HTML/navigation, schema/import/build/CI/pin and PR
work. First submission `6a88c79` was 412 lines, Git blob
`5eb1c845f46b615b5e0a343e5c1283539c5bbecc`. No prior contract, assertion, fixture,
task brief or dependency edge was changed.

## Authored proposals

The model uses one ordered ownership forest, with group-only children, exactly one
placement per node and closed local style references. Typed opaque scene, content,
node, style, draft and publication identities stay distinct from labels and from
each other. IDs retain NUL/high bytes and longer lengths rather than acquiring the
common transport-correlation bound. List order is the layer order; no second
z-index or label sort can contradict it. Locks remain editing state, never rights.

Geometry is exact canonical finite-decimal data in design pixels, with an explicit
pivot/scale/rotation/translation order, custom canvas, required stored alignment/
clipping choices and fit/crop/stretch mathematics. The specified new-scene
1920×1080/aspect-fit defaults remain; no other product default is proposed.
Typed style roles, explicit paint/text/media properties and group composition do
not claim actual renderer/font/raster performance or finalized machine records.
Complete conformance and finite intermediate budgets still require later review.

All seven families have explicit content roles and literal reference examples.
010 binding/format/condition/animation/layout and 011 asset/font/package definitions
are UNFINALIZED. This is a development-stage gap, not a successful runtime unknown
or permission to use a placeholder. Complete structural examples are distinguished
from partial family/reference examples and from full package/renderer validity.
Audio/media attachment references preserve those release targets without selecting
autoplay, a codec or a clock. No raw server JSON, program, markup object, designer
path or credential replaces a typed reference; literal authored Text stays literal.

Immutable content revisions are separate from immutable publication history.
Accepted edits/undo/redo use fresh content identities; publish/restore use fresh
publication identities, so returning to old content cannot hide a stale head.
Current execution-time rights and atomic comparisons remain downstream duties.
The proposal preserves committed-retry/current-disclosure/unknown-outcome boundaries
instead of making stale-head checking a universal retry order. Drafts, previews,
themes and capability declarations cannot publish or grant authority themselves.

Section 8 proposes finite scene-only structural limits with their rationale:
4096 nodes/styles, depth 64, 32768 reference uses, 256 feature labels, 64 KiB new
identity/label bounds, 1 MiB authored text, 256-byte geometry and 16 MiB aggregate
variable bytes. Counts bound traversal/closure work; large identities/text remain
possible while aggregate bounds prevent multiplying every per-field maximum.
These are unmeasured first-review technical proposals, not product defaults or
global accounting/domain limits. External references retain owning bounds. Actual
wire expansion, field walk, scratch/output ownership and concrete error priority
must be frozen before decoding under 012; a sufficient reviewed profile is required
for valid retained data, with no silent truncation/narrowing.

The full 008/003 completion gates and 010/011/012 work remain. All required media,
SVG restrictions/text paths, fonts/Noto/OFL, configurable file/dimension budgets,
desktop-only editor, independent renderers/reference scenes, performance and actual
OBS/platform/deployment/package gates stay in scope. No owner product decision is
requested or silently resolved; these model/profile choices need actual review.

## First-submission examples and actual checks

The first submission has 26 top-level SD examples plus seven literal family rows under SD15.
SD01/02 fix a shape-only and an empty structural example. SD03–06 pin order,
duplicate placement/definitions, dangling/orphan nodes and cycles. SD07–11 cover
fit/crop/stretch, noncommuting/nested transforms and geometry ranges/grammar.
SD12–14 pin opaque NUL/high bytes, a 4097-byte identity and distinct Unicode/literal
markup-looking Text. SD15–18 retain all families while exposing pending references,
wrong kinds and unknown required features. SD19–23 cover immutable identities,
fresh publish, stale draft/live conflict, restore/ABA and undo history. SD24/25
cover proposed resource boundaries; SD26 distinguishes typed dependencies from
program/path/credential substitutions. No expected value came from a renderer,
model function, parser, code generator or implementation under test.

Environment: Linux; documentation scripts run with Python 3.14.8. Actual commands:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
git diff --cached --check
```

Project/render checks exited 0: 406 tasks, 122 decisions, local links/presets and
current HTML. Whitespace checks passed on the candidate and staged owned files.
The state/HTML remain root's unchanged claim snapshot; this is not scene acceptance.

Also ran a read-only `python3 -` script using only Fraction, Counter, graphlib and
literal inputs. Fourteen independent facts passed: fit/crop/stretch; scale-before-
90°-rotation versus the contrasting order; nested translation; exact 2/3 default-
canvas mapping; NUL/high bytes; 4097-byte and 65536-byte hex expansion; distinct
combining/precomposed Unicode; unique forest placements; repeated-node detection;
and graphlib's actual G/H cycle. It called no project model/schema/renderer or
fixture. These are arithmetic/graph document checks, not feature tests or a red run.

CI assessment: only two Markdown files change, so existing project/link/render and
unchanged component CI apply; no new executable suite/workflow is warranted here.
Root must observe all applicable checks for the final PR revision. Different
actual document review comes next, before another bounded authoring stage. No
schema/parser/validator/API/stub/fixture/test/codegen, renderer/native/provider or
media/font/performance execution occurred. No push, PR creation or main merge was
performed. Root integrates review/status/evidence and owns publication.

## Actual R1/R2 review correction

Read the actual changes-required findings via:

```sh
git show 866befcb382399e6ec710ebe20f1663b985fd3f4:docs/evidence/contract-009-review.md
```

Reviewer: `/root/service_reviewer`, still different from this document author.
Review blob `1b3fc34ae271f343be318f3adbed8e24c13fa872`; root imported that review as
`e0112218928789acc873e0723739438e283c47c3`. Its original verdict is retained:
CHANGES REQUIRED, not approval of the first submission or this future correction.

R1 identified a real representation omission: §11 gives every element data
bindings, while the original common node structure exposed only visibility and
animation seams and the family content references did not cover shape/group/image
attachment. The revised common node explicitly chooses NoBindingAttachment or
010 ElementBindingsRef across all seven families, including both shape/group
variants and image. Existing family-specific content references remain intact.
Permitted destinations/types, their relationship to content references, resolution/
override/conflict precedence and unavailable-value rules remain UNFINALIZED 010.
This adds no dynamic-property promise, arbitrary property bag, server path, script,
accounting coercion or placeholder success. Empty attachment is explicit; it cannot
make another unresolved reference valid or authorize publication.

Each attachment reference occurrence counts against the existing reference limit;
its actual component bytes are charged every time, even when shared. The empty
choice has no referenced payload, while its eventual representation/framing and
any actual definition body still require 010/012 bounds. SD27 is an independently
written PARTIAL shape/group case: both nodes attach the same named pending
definition. Their known resource/definition bytes are 10+10 per use, so two uses
charge 40 before actual kind/version bytes. No destination or binding effect is
invented. S and SD01–26 explicitly keep NoBindingAttachment as an example condition,
not a missing-field or product default. All original SD and family table rows are
byte-identical; SD27 is the sole appended table row.

R2 identified ambiguity in "Labels use common identifier grammar". The revision
limits that sentence to required feature/capability identifiers, expressly keeping
authored scene/node labels as common Unicode Text. Empty/combining/precomposed/
markup-looking literal Text, no normalization and SD14 remain unchanged. No new
label restriction or product default was added.

| Scene document revision | Git blob |
| --- | --- |
| Original reviewed first submission, 412 lines | `5eb1c845f46b615b5e0a343e5c1283539c5bbecc` |
| Corrected candidate, 448 lines | `9a3507331376a2beed47b93eac7079b45f0f2c94` |

The prior author note blob was `cf0cfc95b9aa683a624d6def5cdc8ec2524368b8`;
this additive history records the actual findings and correction, not a revised
claim that the original candidate passed. Current coverage is 27 SD rows and seven
family rows. A read-only stdlib check compared every original table row with the
first submission and verified SD27's 40-byte known payload charge, plus unchanged
common005/specification blobs; all passed. No project feature was imported/called.
The existing 14 geometry/byte/Unicode/graph facts are unchanged; they were not
replayed as a new feature result.

After the source correction, the same project/render/whitespace commands above
passed: 406 tasks/122 decisions, local links/presets/current HTML and clean diff.
Only the two assigned Markdown files change. No schema/API/stub/parser/validator/
fixture/test/codegen, renderer/native/provider or shared configuration was authored
or executed. Existing document/component CI still applies; actual revised-artifact
review by the same different reviewer is required before root completes this stage.
All 010/011/012, 008/003 and original release gates remain. No password review was
started; that is a separate future assignment.
