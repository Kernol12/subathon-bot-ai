# CONTRACT-010 — Typed scene bindings, conditions and presentation time

Parent integration gate: [CONTRACT-008](CONTRACT-008.md). Kind: contract.
Authority: specification v0.45 §§4/7/8/10/11/13, UI-04, MEDIA-01/02,
MILE-01/08, MOD-02, TIMER-09, API-03 and source-respecting privacy.
Proposed prerequisites:
CONTRACTPLAN-002, CONTRACT-009, CORE-002, CORE-003, CORE-005, ACCT-002,
ACCESSTEST-002, MILESTONETEST-001, MILESTONETEST-004, MODIFIERTEST-001 and
MODIFIERTEST-002. [ADR-0005](../decisions/0005-scene-contract-prerequisites.md)
records each reason and the cycle exclusions.

## Required owning models

Use the actual reviewed timer lifecycle, pause, debt and contribution-eligibility
contracts and the completed owning milestone/modifier/permission baselines. Do not
copy a still-unmerged contract as approved integrated authority. MILESTONETEST-004
must supply the missing authoritative presentation/gap/queue/reconnect model;
MODIFIERTEST-002 supplies independent clocks and already requires CORE-006.
Baseline completion includes separate contract/test review and valid red; it is not
production completion. An existing task brief is not an executable domain oracle.

If these models lack a required invariant, stop that contract claim at an explicit
owning-domain gap. Do not invent fields or silently omit recent support, modifiers,
sequential presentation or first-interval absence. Further decomposition requires
review. No dependency on HTTP/WS, API-003, VIEWER-001/002, SCENE/RENDER or 008 is
allowed: those would route back through the full scene gate.

## Scope and acceptance

Define a finite typed field/formatting registry for the seven scene element families,
with source ownership, exact value/unit, availability and disclosure semantics for
each field. Separate absent/unknown/not-disclosed/null from known zero. Select only
already authorized scene data; source anonymity, hidden amounts and configured
privacy survive projection. Neither scene capability nor permission-shaped data
grants access. No arbitrary paths into private server JSON, raw credentials or
provider payloads. Actual server filtering/revocation remains downstream work.

Define structured typed all/any/not conditions and permitted comparisons for milestone
position/progress intervals, modifier state, pause and grace. Specify treatment of
unavailable operands and invalid type combinations explicitly under actual review,
without converting uncertainty into a positive permission/support assertion.

Bind current configured targets separately from immutable captured achievements.
Use exact progression and deficit calculations: e.g. completed threshold 1000,
score 800 and next target 2000 retain completion, show 0% progress and optional
deficit 200, with 1200 points to the next target. Before the first interval, preserve
an absent numeric origin unless the owning reviewed model supplies one; never
invent threshold zero. Preserve configurable final completion layouts and historical
captures after target edits; no float shortcut for money/score.

Distinguish actual achievement/score from the currently presented milestone group.
Follow the authoritative server presentation timeline, including its configured
presentation/gap, queue pause and reconnect rules; disconnected overlays neither
block the queue nor replay every previous group. A presentation is not a second
award or intermediate fabricated score. Main/Grace, pause/recovery, active-subathon
modifier, real-elapsed modifier and presentation clocks stay distinct. Hiding a
modifier's element does not deactivate it. A countdown pause does not implicitly
freeze every animation or the independently paused presentation queue.

Specify fade/move/scale/rotate, easing/duration, time units, interpolation and
reference-observation requirements in elapsed time. Missed frames cannot extend
countdowns/animations; UTC instants are not monotonic elapsed durations. Preserve
lifecycle/modifier/deficit/large-contribution/long-text preview inputs. Actual
renderer interpolation and platform scheduling stay under RENDER-004.

Include independent valid/invalid logical examples for every operand/value family,
all/any/not, unknown/privacy, exact boundaries, reversal/deficit, current versus
captured targets, absent first origin, multi-crossing presentation/reconnect and
different clock pause/expiry behavior. Tie each oracle to its exact owning revision;
no renderer/SUT-generated expected output or success placeholder.

## Ownership and completion boundary

First author stage owns a new focused logical contract and author note only, with
paths explicitly assigned at claim. Root owns shared schemas/imports, state/HTML,
maps/build/CI/pins. Different actual review precedes concrete schemas/consumers.
012 later integrates the vocabulary; 008 remains the complete gate. VIEWER/API/
ACCESS and renderer tasks retain actual authorization, credential, delivery,
synchronization, test-first and platform duties. Run project consistency/current
HTML/whitespace checks and record real revisions/commands; no native/provider or
renderer execution is implied by this document task. See [workflow](README.md).
