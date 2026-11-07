# TESTCLEAN-003 — Simplify test registration and labels

Module: Build and test infrastructure. Kind: cleanup. Specification: v0.45; §17.
Contract revision: current CMake presets, test labels and CI workflow contracts.
Parent: EPIC-028. Depends on: TESTCLEAN-002. Shared-file ownership: root/test CMake,
presets, test label modules, and affected CI workflows; coordinate with the integrator.

## Scope

Reduce hand-maintained test wiring. Use one acceptance option, a small helper to register
Qt suites, and labels at executable/suite level where practical. Remove per-slot count
lists and repeated vector-path definitions when test discovery already reports missing
or failing cases. Preserve scaffold, optional Qt probe and intentional red acceptance
semantics; do not make a draft feature suite appear green by relabeling or skipping it.

## Acceptance criteria

- Existing executable suites and intended labels remain discoverable through CTest.
- New Qt test registration follows one documented helper/pattern; no duplicate manually
  synchronized case inventory remains unless discovery cannot meet a specific need.
- Presets and CI accurately distinguish infrastructure checks, reviewed passing behavior,
  intentional red baselines and live/platform operations.
- Relevant presets configure/build and targeted CTest invocations run as appropriate;
  preserve failing acceptance behavior without `WILL_FAIL`, blanket continuation or skips.
- Independent review approves changed coverage, labels, CI semantics and migration of all
  call sites before the cleanup task is marked done.
