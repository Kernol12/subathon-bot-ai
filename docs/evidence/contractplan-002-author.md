# CONTRACTPLAN-002 — Scene prerequisite plan author handoff

Date: 2026-11-04. Author: `/root/service_reviewer`, acting as document author for
this task, not its independent reviewer. Status: authored; different actual review
of the documents and root's eventual state mapping is required. No child is claimed
or approved by this note.

## Source and ownership

Dedicated branch: `task/CONTRACTPLAN-002-document-author`, isolated worktree
`local-evidence/worktrees/contractplan002-author`. Started clean at claim
`0fdbc5e3db5d0c1cd21d258d2673287831935e2f`, based on merged main
`009632386564f3a00fb413580ac1cee3ef43cc11` (PR58/common005). Parent reported actual
normal merge after final checks; this document author did not perform the merge or
independently query GitHub. Local Git ancestry and source files were inspected.

Read root/schema instructions, specification §11 and registered decisions, ADR0004,
the plan/008 briefs, actual scene/renderer epics and relevant domain briefs/contracts
and state dependencies. Scoped docs instructions do not add a separate AGENTS file.

| Input at claim | Git blob |
| --- | --- |
| specification v0.45 | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| ADR0004 | `b1a2965e8fff9ad9406995dffa45c264aa9af21d` |
| CONTRACTPLAN-002 brief | `b9953d6ff16e18f79f953f8a5a544c3ac34f4369` |
| Original CONTRACT-008 brief | `aa245dcd03b89003bcc8e1a642b03188541e47b9` |
| Approved common wire document | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| progress/state.json | `d03e8e0a7be2c4769025feb513f994e12b575308` |

Author owns only ADR0005, new briefs009–012, this author note and an appended section
of008. Root owns all state/HTML/navigation/maps/schema/imports/build/CI/pins and
integration. No other file was edited, and no child scene document was created.

## Authored result

[ADR0005](../decisions/0005-scene-contract-prerequisites.md) proposes focused model,
typed binding/time, asset/package and complete-schema tasks. The original008 text
is an identical byte prefix; its added section retains the complete integration
gate. Existing HTTP006 → 008 and parent003 → 008, all SCENE/RENDER/epic/release
dependencies and every old test-baseline edge stay. Each new child requires the
planning task. No child depends on its parent or a downstream consumer.

010's exact owning-domain dependencies are CORE-002/003/005, ACCT-002,
ACCESSTEST-002, MILESTONETEST-001/004 and MODIFIERTEST-001/002, in addition to
CONTRACTPLAN-002/009. ADR0005 explains each. Completed baselines supply reviewed
logical contracts/examples/tests and actual red; their production feature need not
be implemented merely to document a pure projection. Existing transitive gates
(including MILESTONE-001/003 and CORE-006) remain. Missing presentation/clock fields
are explicit owning-model blockers; neither a future renderer nor universal unknown
can stand in for them. Viewer/HTTP/WS consumers cannot be dependencies because their
paths return to008. Narrow source/privacy/permission boundaries are retained without
claiming credential/filtering implementation.

The requirement table allocates every §11 area: seven element families, geometry,
all/any/not, exact current/captured/deficit/absent-origin progress, separate clocks,
sequential server presentation, built-in animations, complete media/SVG targets,
fonts/Noto/OFL, configurable compressed/decoded budgets, desktop editor operations,
portable themes, drafts/live/publish/restore, independent renderer oracles,
conformance/performance and actual OBS/platform evidence. No new product default or
validator/library/codec/font/platform capability is chosen or claimed.

First009 assignment is expressly limited to NEW scene-document-v1.md plus its author
note, followed by different actual review. 012 cannot begin schema production until
logical contracts and actual choices for dialect, imports, finite profiles and
tooling are reviewed. A schema still proves no permission, safe decoding, renderer
equivalence or platform result. Complete008/003 and existing release gates remain.

| Authored artifact | Git blob |
| --- | --- |
| ADR0005 | `e4bb60c82007a48e9ec4f7a0f7978d290d551bf4` |
| CONTRACT-008 with appended integration section | `ce9ad8e4602d49fa37b85dd96c88efb828eced52` |
| CONTRACT-009 | `2ac98e5ddceff93bd4aa3347704d7e9eda038fed` |
| CONTRACT-010 | `3a22162be1f8876953eb60e8d4c30a7c9221a13d` |
| CONTRACT-011 | `23fe99dcc1a6f1134c761e1833b90c9a60165552` |
| CONTRACT-012 | `2780483b460c2b59853f020f0cf0c0403329c31e` |

## Actual checks and remaining gates

Environment: Linux, Python3.14.8, Git2.56.0. From the isolated worktree:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed: project check reports402 task records/122 decisions/local links/presets/
current HTML; render check reports current. Those are the unchanged claim's state
records, not proof that the four proposed children have been integrated. New briefs
and links are authored, with actual state/nav/map changes reserved to root.

An additional read-only Python graph check constructed the exact ADR table in memory:
009 → PLAN002/005; 010 → PLAN002/009 plus its nine named owning-domain gates;
011 → PLAN002/009; 012 → PLAN002/009/010/011; 008 adds PLAN002 and009–012. It checked
known IDs, DFS cycle absence, preservation of every original edge, only008 changed
among existing nodes, and no new child's transitive ancestors included003/008/006/
007/API003/VIEWER001/002 or SCENE/RENDER/epic consumers. Result:406 proposed nodes,
all1473 old edges retained, including319 edges whose prerequisite is a test baseline.
It also compared008 against `git show HEAD:docs/tasks/CONTRACT-008.md` as an exact
byte prefix, and state/HTML against their HEAD bytes. All assertions passed; no graph
file or generated HTML was written. This author check does not approve actual
integrated state. Exact child edges must be compared again after root authors it.

CI assessment: documentation/dependency planning uses existing consistency/component
checks; no executable module or workflow is added. Final PR head CI, branch/review
rules and actual independent planning/state review remain root integration gates.
No schema/parser/validator/codegen, fixture/test/feature, native/crypto, renderer,
asset/codec/font, provider, performance or OBS run occurred for this task.

Handoff: root imports only the assigned seven Markdown files, authors the exact state
and navigation mapping without removing prior edges, obtains different actual review,
then completes/merges the planning task before claiming009. No push, PR creation or
main merge was performed by this author. No new owner decision is requested; absent
domain models and unobserved tooling/platform evidence are engineering gates, not
authorization to fill in behavior.
