# Milestone minimal interface harness — coordinator evidence

Coordinator /root, 2026-11-04, task/MILESTONETEST-001-progression-baseline.
Actual source candidate author `2e36bc8cb3a13e6bd26f23dd41b1f2de5e728d90`, import
`d481d08`, adds only the inert header/source and author note. Frozen logical/API
blobs remain `0115a5cc`/`58b8e129`; actual API approval is `b8932cd`/import `21ba96d`.

Root's minimal harness adds one default-off option, which enables the existing
exact-numeric lane, an explicit `subathon_milestone_progression` static target linked
PUBLIC to `subathon_exact_numeric`, and configure/build/test preset
`milestone-progression-debug`. Display slot24 reserves slot23 used by the separately
active rate branch; preset names are distinct. No new feature tests or execution,
fixture, environment program or workflow is included in this initial harness.
The library is compiled; its one immediate throw and marker are never invoked.

Actual local checks against the authored harness and imported source:

- Initial configure with guessed `-DSUBATHON_GOOGLETEST_SOURCE_DIR=/tmp/subathon-googletest`
  FAILED because that directory did not exist. This is a setup error, not any red.
- `cmake --preset milestone-progression-debug -DSUBATHON_GOOGLETEST_SOURCE_DIR=`:
  PASS using installed GoogleTest1.18.0 (minimum required1.17), GNU16.2.1/C++20, Boost1.92, Python3.14.8,
  Ninja. No downloads or personal paths committed.
- `cmake --build --preset milestone-progression-debug`: PASS, 14 steps, no warnings.
- `ctest --preset milestone-progression-debug --show-only=json-v1`: PASS; saved
  ignored discovery inventory lists 36 old checks, zero milestone feature bodies.
- `ctest --preset milestone-progression-debug`: PASS 36/36, no skipped checks,
  0.61s. This is 33 existing exact-numeric cases plus three infrastructure checks,
  not new milestone acceptance or its own intended red.
- `python3 tools/check_project.py`: PASS 396 records/122 decisions/current HTML.
- `python3 tools/progress.py render --check` and `git diff --check`: PASS.

CI assessment: current PR56 proposal head `3fc99ce` has all12 actual check-runs
successful. Existing component CI does not yet compile this new explicit target.
A dedicated selected Boost1.83 configure/build/discovery/foundation lane is required
before the baseline merge; first review its actual workflow/harness. Adding that
workflow is subject to the same pending GitHub workflow access as PR54; no bypass
or selected-environment claim is made. Later new test bodies must remain plainly
intentional-red and outside green checks until production passes; no WILL_FAIL,
skipping or blanket error inversion. Root retains CI ownership.

Next: different actual minimal source/harness review, reviewed tests and independent
selected foundation, then FIRST plain feature red and independent attribution.
No progression algorithm, product state/action integration, providers, UI, platform
or supported feature is established here. No fixture/assertion was weakened.
