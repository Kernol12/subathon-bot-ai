# MILESTONETEST-001 independent logical document review

Reviewer: codec_author, different from author service_author. Date: 2026-11-04.
Branch `task/MILESTONETEST-001-contract-review`, isolated worktree
`local-evidence/worktrees/milestonetest001-contract-review`. Ownership is this new
review note only; no contract, code, test, fixture or shared state is edited.

Actual author candidate `4362fe29f5caac35f12ce5f2366784a570594fde`, integrated by
root as `af7e2be55e073513f9e1c9e8854c7e78332009bc`, was read in full:

| Artifact | Git blob |
| --- | --- |
| [Logical proposal](../contracts/milestone-progression-v1.md) | `0115a5cc3cdc4fb1ebb8c9739716a27cb3379933` |
| [Author evidence](milestonetest-001-contract-author.md) | `67593a9ebabafc44976fb1c5f4260f2951b11baa` |

Read root/core instructions, architecture/workflow/test strategy, specification
v0.45 §§3.5/8 and MILE-01/NUM-04, MILESTONE-001 and adjacent 002–008 briefs,
event/state boundaries, exact-decimal revision 1 and public header, and captured
application-plan contract/public types. There is no narrower docs AGENTS file.

## Verdict

**Approved for this logical document stage.** No blocking defect, requirement
conflict or additional owner decision was found. This verdict covers the exact
two documents above, not a future API, header/stub/harness, test baseline, observed
red, implementation, application integration or platform verification.
MILESTONETEST-001 remains incomplete; MILESTONE-001 production remains planned.

## Model and history boundary

The helper consumes one already authoritative signed64 micro-point score. It does
not add a delta to an old score, calculate/round an award or substitute requested
plan amounts for actual applied state. This respects NUM-04 and the existing
captured-plan/application split. Actual aggregate overflow and whole-input atomic
commit remain upstream obligations; a pure returned proposal is not a durable
achievement or permission to publish intermediate state.

Full signed64 score/target values, including negative/zero targets, are coherent
with a specification that requires ascending targets and supplies no positive-only
restriction. Strict current order and unique identities make the newly achieved
portion a consecutive suffix. Every appended row captures the same actual supplied
score and current revision/context, rather than invented intermediate threshold
scores. Existing completions never disappear or repeat on a refund/recovery.

Current targets and immutable captured completion fields are correctly separate.
A completed-target correction can change current deficit/progress without
rewriting the historical target, actual score or completion context. Historical
captured targets need not form a newly sorted sequence after legitimate edits.
A manual completion may record score below target. Structural prefix validity
proves neither such authority nor completeness against an unseen prior history.
Older inactive reset history remains a durable caller obligation.

Each completed milestone receives its current nonnegative deficit; the last one's
deficit is the track maximum, rather than a sum of overlapping deficits. The first
unfinished target determines exact remaining distance. After append, its value
is strictly greater than score, so an existing P/N interval has positive span and
earned strictly below span. Below P, earned/fraction are zero while deficit remains
visible. At N, achievement occurs first and projection moves forward; there is no
completed next target waiting at 100% for the presentation queue.

The explicit absent interval before any completion is a sound engineering
projection: the specification defines the interval from a previous completed
milestone but supplies no numeric origin when there is none. First target and
remaining distance are still present; zero is not fabricated as a target and
negative first targets are not excluded. This does not finalize a numeric first-bar
UI policy. Empty and all-completed tracks also have absent intervals/next amounts,
but remain distinct states and all-completed can still have a deficit.

## Independent example and numeric review

All MP01–22 rows were inspected against those invariants:

- MP01/02/21/22 distinguish empty, before-first, negative-first and all-completed
  states without a hidden zero origin.
- MP03/08/09 record threshold equality and multi-crossings immediately in order,
  with actual shared source score and immutable earlier evidence.
- MP04–07/10/11 preserve completion through ordinary progress, refund, recovery
  and manual deficit; reaching an old target again creates no new achievement.
- MP12–16 cover signed targets/zero and both signed64 extremes without narrowing
  valid projection distances.
- MP17 uses the current corrected threshold for projection while preserving its
  captured historical value; MP18 keeps exact 1/3.
- MP19 rejects duplicate/descending/equal targets, malformed prefix and empty
  references; MP20 refuses oversized input without a fitting prefix/history loss.

A separate read-only standard-Python integer/Fraction command asserted 23 literal
arithmetic facts, importing no project module and writing no fixture or model.
They include deficit 200/remaining 1200; per-completed deficits 200/1200 with track
1200 and remaining 2200; manual deficit 1050/remaining 2050; corrected-current
deficit 100/remaining 900; fractions 1/2, 1/10 and 1/3; and the ordinary crossed-
threshold inequalities. All pass. Ordinary point examples scale exactly by
1,000,000; explicitly marked extreme examples are raw micro-points.

For MP14, INT64_MAX−INT64_MIN is exactly UINT64_MAX
`18446744073709551615`. Earned at score 0 is `9223372036854775808`, remaining is
`9223372036854775807`, and the fraction is exactly
`9223372036854775808/18446744073709551615`. Its gcd is 1 and both components use
64 bits. MP15/16 use the same maximum distance for deficit/remaining. A generic
signed subtraction can require 65 signed bits; widening before subtraction avoids
negating INT64_MIN or checking an already-overflowed result. None of these derived
distances is a new credited score or a signed SQLite balance field.

The proposal explicitly reserves exact DTO field walks, error priority and scratch
bounds for separate API review. Its local count/reference/owned-byte limits require
preflight and complete output projection before growth, with no partial proposal
or discarded history. This stage makes no allocator, transport or SQL capability
claim from those logical bounds.

## Adjacent scope and remaining gates

MILESTONE-002 still owns edit/correction authority, previews and audit. 003/004
own durable sequential action groups and server presentation; 005 owns independent
holds, Run/Defer/Skip and refund-before-start policy; 006 owns captured definitions
and interruption; 007 owns manual completion authority/reasons; 008 owns reset,
cancellation/history and replay safeguards. A projected deficit does not choose
those policies, release a group, clear a hold or roll back performed effects.
The explicit composition note also leaves actual ordered score observations and
whole-input atomic integration to their owning application boundary.

Concrete API review, minimal inert interfaces/harness review, independently
authored literal tests and their review, selected foundation and actual intended
behavioral red remain required before production. No new API/code/stub/test or
future expected result is prospectively approved here.

## Actual checks

- Exact Git-object/specification/contract/public-type inspection: done.
- Read-only standard-Python integer/Fraction audit: exit 0, 23 assertions pass.
- `python3 tools/check_project.py`: exit 0; 396 task records, 122 decisions,
  local links/presets/current HTML in this isolated milestone branch.
- `python3 tools/progress.py render --check`: exit 0, current snapshot.
- Final owned-note staging/path and whitespace checks accompany the commit.

No project feature, native executable, provider operation, test body or model
implementation was invoked. These are document/arithmetic checks, not milestone
feature acceptance or an observed red. Root owns shared progress, PR/CI publication
and subsequent stage releases. Prior rate/locale work and reviews are untouched.
