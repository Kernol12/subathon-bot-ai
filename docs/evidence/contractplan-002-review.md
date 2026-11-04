# CONTRACTPLAN-002 independent scene-planning review

Reviewer: `/root/codec_author`, different from planning-document author
`/root/service_reviewer` and state integrator `/root`. Date: 2026-11-04.
Dedicated branch: `task/CONTRACTPLAN-002-independent-review`, worktree
`local-evidence/worktrees/contractplan002-review`. This reviewer owns only this
new evidence note; no plan, brief, state, schema, build or product file is changed.

## Verdict: approve the actual planning documents and dependency mapping

Reviewed integrated candidate `84a80bd3b2004c7a044d1810a954bdef5e4b457f`, including
the seven authored Markdown files imported as
`2c76307bc68a4cac3043aa72051cf59083cd64d7` and the actual state/navigation mapping.
No blocking defect, requirement conflict or additional owner decision was found.
The four-way model, binding/time, asset/package and complete-schema split preserves
the full scene gate and makes missing owning models explicit.

This approves planning only. It approves no future scene document, API, schema,
validator, fixture, test, renderer, media/font support or platform result. The new
children remain planned and unverified in this exact reviewed state. Root must
integrate this review, complete the planning acceptance and normal publication
gates before a child claim. Final PR revision CI and merge rules remain required.

## Exact reviewed inputs

Published main was `009632386564f3a00fb413580ac1cee3ef43cc11`. Initial planning
claim was `0fdbc5e3db5d0c1cd21d258d2673287831935e2f`. Git inspection confirmed
that the document import changed exactly its seven assigned Markdown files.

| Actual artifact | Git blob |
| --- | --- |
| [Specification v0.45](../specification.md) | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| [Accepted ADR-0004](../decisions/0004-wire-contract-prerequisites.md) | `b1a2965e8fff9ad9406995dffa45c264aa9af21d` |
| [Common wire primitives](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [Planning brief](../tasks/CONTRACTPLAN-002.md) | `b9953d6ff16e18f79f953f8a5a544c3ac34f4369` |
| [ADR-0005](../decisions/0005-scene-contract-prerequisites.md) | `e4bb60c82007a48e9ec4f7a0f7978d290d551bf4` |
| [Parent CONTRACT-008](../tasks/CONTRACT-008.md) | `ce9ad8e4602d49fa37b85dd96c88efb828eced52` |
| [CONTRACT-009](../tasks/CONTRACT-009.md) | `2ac98e5ddceff93bd4aa3347704d7e9eda038fed` |
| [CONTRACT-010](../tasks/CONTRACT-010.md) | `3a22162be1f8876953eb60e8d4c30a7c9221a13d` |
| [CONTRACT-011](../tasks/CONTRACT-011.md) | `23fe99dcc1a6f1134c761e1833b90c9a60165552` |
| [CONTRACT-012](../tasks/CONTRACT-012.md) | `2780483b460c2b59853f020f0cf0c0403329c31e` |
| [Author evidence](contractplan-002-author.md) | `71c8df063efe65c88fae79b62d7ad53224bbe292` |
| [State integration evidence](contractplan-002-state-integration.md) | `02ef713d7ffe9a192df15dc18d30e61c58231f31` |
| [Actual dependency state](../../progress/state.json) | `cfe30cf0bac037e8a66026bc0c0e0f8ffb722fce` |

Read root/schema instructions, workflow and first-baseline gates, relevant
architecture/test-strategy boundaries, full specification §11 and registered
UI/MEDIA/API/VIEW decisions, ADR-0004, the original/full parent briefs, both
presentation epics and the owning milestone/modifier task briefs. The schema
directory still contains a planning README, not a selected complete validator.

## Dependency review

I compared parsed actual state with both initial claim and published main, using
read-only `python3 -` scripts with standard-library JSON, pathlib and `git show`.
Assertions checked exact node sets, ordered old dependency lists, the entire edge
set and exact additions, rather than accepting the reported counts alone.

- Claim to candidate: 402 to 406 nodes and 1,473 to 1,497 edges. Every old edge
  remains, including all 319 whose prerequisite has kind `test_baseline`.
- CONTRACT-008 is the sole old dependency list changed. Its original list remains
  an identical ordered prefix followed by PLAN002 and 009–012, exactly five gates.
- New 009 has exactly 005/PLAN002; new 011 has exactly 009/PLAN002; new 012 has
  exactly 009/010/011/PLAN002. New 010 has exactly 009, CORE-002/003/005, ACCT-002,
  ACCESSTEST-002, MILESTONETEST-001/004, MODIFIERTEST-001/002 and PLAN002. Those
  four lists contribute exactly 19 new edges, with no unknown or duplicate IDs.
- All other old lists are exactly equal, including 26 SCENE/RENDER task lists
  and 33 epic/release task lists. HTTP006 → 008, parent003 → 008, every paired
  baseline, all original aggregate/release gates and ADR-0004 consumers remain.
- Whole-graph DFS passed. Each new child's transitive prerequisites exclude its
  parents and downstream HTTP/WS/API/viewer/SCENE/RENDER/epic/release consumers.
  Existing MILESTONETEST-004 → MILESTONE-001/003 and MODIFIERTEST-002 →
  MODIFIER-001/CORE-006 are retained exactly. Conservative baseline gates cannot
  substitute for a suitable actual reviewed model; ADR-0005 explicitly says so.
- Direct comparison against published main also passed: all 1,471 old main edges
  remain. The difference is the planning claim's two edges plus these 24 additions.
- The original CONTRACT-008 brief is an exact byte prefix of the new brief,
  followed by 23 lines. Each new child is planned, unassigned and has empty
  verification/evidence with no review; PLAN002 is correctly still `in_review`.

## Requirement and stage boundaries

The ADR allocation and child acceptance preserve the complete §11 contract work:

- All seven element families, geometry/layers/styles/grouping/locks, default/custom
  canvas and aspect-fit/crop/stretch; saved scenes, reusable themes and editor
  operations/previews, with full editing confined to desktop.
- Typed disclosed data, formatting and all/any/not conditions; explicit unknown,
  absent and private values; no permission or capability-shaped data grants access.
  Actual credential/filtering/revocation work stays with its existing owners.
- Exact current-threshold progress, retained captured achievements, deficit after
  reversal, absent first-interval origin and configurable final completion layout.
  The 1000/800/2000 example is coherent: deficit 200 and next remaining 1200;
  it does not manufacture a zero origin or erase an achievement.
- Immediate achievements remain distinct from ordered presentation/action groups.
  Main/Grace, pause/recovery, modifier clocks and queue/presentation clocks remain
  separate. Reconnect cannot replay/block the queue, and hiding an element cannot
  deactivate a modifier. Elapsed timing, easing/duration and missed-frame behavior
  remain explicit rather than frame-count or UTC-duration defaults.
- Every required media format, documented codec profiles and the complete static
  SVG subset/exclusions/text-to-path rule remain. Contract vocabulary certifies no
  decoder or platform support and does not remove unknown release formats.
- TTF/OTF uploads, shared assets, configured fallbacks, Noto Sans Regular/Bold,
  OFL/notices and redistribution/pinning obligations remain. Initial configurable
  32/128/512 MiB and 2 GiB budgets plus 8192-edge/32 MP raster bounds remain, with
  decoded/intermediate/execution controls and demanding-scene warnings still due.
- Portable server-owned packages, draft/live revisions, permissioned publication,
  restore and unsupported-feature checks remain. Native QML and browser renderers
  stay independent, with shared reference scenes and separate conformance evidence.
  FPS targets, actual OBS/embedded-browser minimums, Windows/Linux/Wayland/media
  deployment and packaging evidence remain required downstream gates.

009's first assignment is only a new logical scene document and author note,
followed by different actual review. Its explicit unresolved typed references do
not complete 008 or authorize schema/source/fixture production. 010 must wait for
the named actual owning models; no universal unknown or empty registry can bypass
them. 011 initially defines asset/profile requirements without certifying codecs.
012 requires actual prior review of dialect, imports, limits, reference/semantic
validation and pinned tooling before schema authoring. 008 still audits the complete
package before HTTP006/parent003 and downstream consumers can complete. Further
decomposition is required if those actual assignments exceed a focused review.

## Actual checks and remaining gates

From the clean dedicated review worktree, all exited 0:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Project consistency reported 406 tasks, 122 decisions, valid local links/presets and
current HTML; render check reported current. The separate inline stdlib graph and
original-brief checks above passed. These are document/graph checks, not product
tests. No schema/checker/fixture, renderer, native, provider, asset/codec/font or
performance/OBS behavior was executed or claimed. No build/CI workflow is introduced
by this planning change; existing consistency/component CI must pass for the final
published PR head. Root owns state/HTML, completion, publication and merge checks.
