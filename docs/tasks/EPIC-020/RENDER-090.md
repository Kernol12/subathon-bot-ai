# RENDER-090 — Verify native OBS capture on KDE Wayland

Parent: [EPIC-020](../EPIC-020.md). Kind: environment.
Specification: v0.45; §11, PLATFORM-04, MEDIA-01.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Actual native timer-window OBS/PipeWire/portal tests on reference KDE Plasma with recorded graphics/Qt/OBS versions.

Primary module paths: desktop/ native output, web/overlay/ and tests/ reference scenes
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Exercise transparency, resize, scene/media playback, reconnect and capture restart against the reference scene set.
- Record limitations/minimum tested profiles and separate Arch/Kubuntu evidence from generic Qt tests.

## Boundary and failure cases

No X11/XWayland support claim; absent hardware/compositor remains unverified and no browser simulation can complete this gate.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: Qt Quick Test for native; Vitest/Playwright for browser; actual OBS checks separately.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
