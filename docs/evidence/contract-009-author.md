# CONTRACT-009 scene document author handoff

Date: 2026-11-04. Author: `/root/codec_author`. Status: logical document authored;
different actual review is required. No schema, API, executable fixture, test,
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
work. The proposed contract is 412 lines, Git blob
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

## Independent examples and actual checks

There are 26 top-level SD examples plus seven literal family rows under SD15.
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
