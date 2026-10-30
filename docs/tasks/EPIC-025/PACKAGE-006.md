# PACKAGE-006 — Verify Linux archives Arch package and secure keyrings

Parent: [EPIC-025](../EPIC-025.md). Kind: environment.
Specification: v0.45; §16, PLATFORM-04, PLATFORM-05, AUTH-14, PLATFORM-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Arch rolling/Kubuntu 24.04 Wayland/KDE package and keyring matrix.

Primary module paths: deploy/ packaging and CMake release presets
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Verify clean startup, archive relocation/replacement, settings location, plugins/media and Arch recipe against system packages.
- Exercise usable/locked/unavailable KWallet/Secret Service and ensure memory-only fallback without plaintext tokens.

## Boundary and failure cases

Keep per-OS/per-format evidence separate; no untested compositor/architecture support claim.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: Package/manifest checks plus distinct clean-machine installation evidence.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
