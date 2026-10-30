# RENDER-091 — Verify native and browser OBS outputs on Windows

Parent: [EPIC-020](../EPIC-020.md). Kind: environment.
Specification: v0.45; §11, PLATFORM-01, WEB-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Actual Windows 11 x86-64 native capture and OBS browser-source validation.

Primary module paths: desktop/ native output, web/overlay/ and tests/ reference scenes
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record concrete Windows/MSVC/Qt/OBS/embedded-browser versions and media/font/capture results.
- Compare supported bindings/timing/geometry with reference scenes and verify resize/reconnect/transparency.

## Boundary and failure cases

Choose minimum OBS/profile support from evidence; unit tests or a Linux capture cannot establish Windows support.

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
