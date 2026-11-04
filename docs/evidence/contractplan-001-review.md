# CONTRACTPLAN-001 independent planning review

Reviewer: codec_author, different from actual planning author coordinator /root.
Date: 2026-11-04. service_reviewer supplied read-only preparation, not this plan's
authorship or independent verdict. Owns only this new review note on
`task/CONTRACTPLAN-001-independent-review`; no plan/state/brief/ADR/HTML is edited.

Reviewed actual diff from main `3217c21233bfbe1e4e2732c7c7819bdffe00ef1c` through
plan `76c7b6e2d626857aba27eda92785f43030149fa5`, then the actual coordinator metadata/
link correction through `69ccc21439db1243714989ac1566983ced4a8c83`. After checking a
clean worktree, merged the current integrator branch normally (clean fast-forward
to 69ccc21). ADR, consumer dependencies and child briefs are unchanged by the later
metadata correction. Exact final artifacts inspected:

| Artifact | Git blob |
| --- | --- |
| [ADR-0004](../decisions/0004-wire-contract-prerequisites.md) | `24724f512b08cbf9392707d967f8a580e98dc480` |
| [Planning brief](../tasks/CONTRACTPLAN-001.md) | `aded70fae26f1c31e24b2a0a630b2ee828ac8833` |
| [Original parent brief, extended](../tasks/CONTRACT-003.md) | `c24a29683ed100187833070e31b3f647b5b1d2c0` |
| [Common child 005](../tasks/CONTRACT-005.md) | `67723a402ff0f51cd8aa8c9b445e63d0b1a6f67d` |
| [HTTP child 006](../tasks/CONTRACT-006.md) | `7262339f3f8790b2f240d36774220c95ca95c3d5` |
| [WebSocket child 007](../tasks/CONTRACT-007.md) | `cecebf71bbde5d5aef8d4c1e462bc9df2db640c9` |
| [Scene child 008](../tasks/CONTRACT-008.md) | `aa245dcd03b89003bcc8e1a642b03188541e47b9` |
| progress/state.json | `064109196b516e60e27f906cd55a13c5f12ea3d3` |
| PROGRESS.html | `064621b4698075941e771aa6fdb5154e1c94e62d` |
| [Coordinator checkpoint](coordinator-handoff.md) | `aca856950d7a4b8e207496e28beb03672bc5268f` |
| [Author evidence](contractplan-001-author.md) | `63c5ce0da45e5e148661a3355f6a454cb1266374` |
| Task navigation | `59298d824b689103ee0d3296e99f349f33112997` |
| Decision navigation | `3f88695fa95a719f5a27048b27676631ee22ad15` |

Read root/schema instructions, workflow, original CONTRACT-003, specification
API-01/02/03 and §§2/10/11, relevant API/identity/session/scene/renderer briefs and
the reviewed internal boundary/spike limitations. API-03 is registered at
specification line 968; it is neither missing nor a new owner decision.

## Verdict

**Approved for this documentation/dependency planning change at 69ccc21.** No
blocking defect or product requirement conflict was found. This approves the
actual decomposition and gates, not any future contract, schema, client, validator,
baseline, implementation or capability. Root still owns final publication/CI,
status/checkpoint integration and the normal PR/merge gates. No push, PR or main
merge is performed by this review.

## Exact dependency audit

An independently written read-only Python audit loaded both actual JSON states
and checked a literal expected replacement table, not only task counts:

| New prerequisite | Old CONTRACT-003 consumers replaced |
| --- | --- |
| CONTRACTPLAN-001 | ACCESSTEST-001/ACCESS-001, ACCESSTEST-002/ACCESS-002, SESSIONTEST-001/SESSION-001, SESSIONTEST-006/SESSION-006, VALIDATE-001: 9 |
| CONTRACT-005 | APITEST-001/API-001, APITEST-005/API-005, RELEASETEST-002/RELEASE-002: 6 |
| CONTRACT-006 | APITEST-002/API-002, WEBAPPTEST-001/WEBAPP-001: 4 |
| CONTRACT-006 and CONTRACT-007 | VIEWERTEST-001/VIEWER-001: 2 |
| CONTRACT-008 | SCENETEST-001/SCENE-001, RENDER-001: 3 |

That is exactly 24 substitutions. The eight internal account/permission/password/
encryption tasks and the separate evidence-matrix task retain a direct planning
gate before earlier starts. Exactly SIX further consumers add CONTRACT-007:
APITEST-003/API-003, RENDERTEST-002/RENDER-002 and RENDERTEST-003/RENDER-003. No
other old prerequisite is removed. Including the parent, exactly 31 old dependency
lists change. All 319 old dependency edges whose target is a test-baseline task
remain; paired implementation baselines cannot be bypassed.

All ten original epic dependency lists are unchanged, including CONTRACT-003:
EPIC-006/007/008/018/019/020/021/022/023/024. The parent remains planned and depends
directly on CONTRACT-002 and all four children. Its original OpenAPI 3.1, separately
versioned WS, scene, exact encoding and capability/integration scope is preserved.
Common documentation alone cannot complete it.

The four new children are planned/unassigned with empty verification/evidence and
no reviews. Each depends on CONTRACTPLAN-001. HTTP006 requires common005 AND scene008
so a completed package cannot contain an unresolved imported scene shape. WS007
requires common005, HTTP006 and SPIKE-001; scene008 requires common005. None depends
on its parent or a downstream implementation. Independent traversal confirms all
401 task IDs resolve and the graph is acyclic. The planning task is currently
in_review, not done; this diff does not prematurely release a child.

All old verification/evidence/review fields remain unchanged. Concurrent milestone
claim and rate-publication-block metadata are reconciled without changing their
dependencies or claiming a new passing baseline. This review does not certify
those separate branches' feature or publication outcomes.

## Scope and sequencing assessment

Removing the whole HTTP/WS/scene prerequisite from bounded password hashing,
credential encryption, grant evaluation and account/bootstrap services is
appropriate: their own contracts/tests can precede product endpoint schemas.
API-002 and its baseline still require HTTP006, API-001, SESSION-003 and ACCESS-002;
current authorization and protected HTTP publication are not released by the
internal helper change. AUTH cookie/header adapters retain their own acceptance
and test-first work. The explicit WS gates preserve synchronized renderer and
subscriber requirements after HTTP's broad ancestor is narrowed.

Common representations have one integrator-coordinated definition; HTTP/WS/scene
writers have assigned paths and separate review. The briefs explicitly retain
exact numeric strings/rational pairs, byte/text/unknown distinctions, execution-time
authorization and privacy, persistent synchronization, complete §11 scene/media/
font/budget/conformance scope and independent renderer/editor boundaries. Unknown
provider support and unresolved timer policy cannot become schema defaults.

The child briefs are bounded planning seams, not automatic assignments to finish
every endpoint or scene/media family in one change. Their explicit further-split
requirement applies before a claim exceeds focused review scope. Schema/client
generation or product use waits actual contract/example review; executable behavior
still needs independent tests, genuine red, implementation and separate code review.
No existing assertion, code, schema, workflow, pin or provider capability changes.

## Metadata correction, checks and CI

The coordinator reported an initial checkpoint link-check failure at 1b6fb0c: local
links referred to milestone documents absent from the planning branch. Inspected
the exact 1b6fb0c→69ccc21 correction: five links now name artifacts at the exact
3fc99cee GitHub commit. Local Git-object checks confirm all five linked paths exist
there. No network reachability or remote CI result is independently claimed. The
failed intermediate check is NOT recorded as passing. The later state diff changes
only the planning review status/owner; all dependency lists remain identical.

Actual independent checks at the corrected final candidate:

- Literal old/new dependency-table, baseline-edge, epic-edge and 401-node DAG
  audit: exit 0; all checks above pass.
- Final metadata-vs-76c7b6e comparison and five linked Git-object checks: exit 0.
- `python3 tools/check_project.py`: exit 0; 401 tasks, 122 decisions and valid
  local links/presets/current HTML.
- `python3 tools/progress.py render --check`: exit 0, current snapshot.
- Final owned-note staging/path and whitespace checks accompany its commit.

The unchanged scaffold workflow runs on push/PR without path exclusions and calls
the existing scaffold checks, including graph/link/tooling consistency. No new
executable suite/workflow is warranted for this plan. Actual final-PR checks remain
required before merge. No native/feature/provider test or schema execution ran;
these document/graph checks are not product acceptance or future contract approval.
