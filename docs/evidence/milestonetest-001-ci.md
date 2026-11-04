# MILESTONETEST-001 build, inventory and CI evidence

Coordinator /root, 2026-11-04. The 53-case source author candidate
`9f74e77923cdf8d66df7d2dcd625178713306bb6` is imported as `643de5c` after main
`dd6a141` integration `00d8c57`. No new feature/foundation/marker body has run.

## Actual source and harness

Frozen source: header `9a1678137ba6f85cbfbb545630f08d36a261790b`, TU
`4e3291f0e832237470a8cd0bc349e0106ad8fd77`; logical/API `0115a5cc`/`58b8e129`.
Existing separate minimal review `a2f5d358` remains valid for those unchanged files.
New fixture `409be19d57b2cba4025e236a67a539af63b08be4`, tests
`019973844dceb289aa82ec8aabd3d9274faef98e`, standalone foundation
`7f8c3f9466a0e3b313aa7e3f492b7ee82b91513d` and
[author note](milestonetest-001-test-author.md) `a54088836e54f28e4744378d136f37f485d6bdbd`
are authored candidates, awaiting actual different test/foundation/harness review.

Root adds the acceptance target and deterministic raw parameter names using
NO_PRETTY_VALUES; 20 literal MP rows plus 33 ordinary tests. All receive
milestone-progression/acceptance/intentional-red/core labels and a 30-second timeout.
The foundation has its own main, Boost::headers only, no project includes/link/symbols,
and scaffold/milestone-progression-environment labels. Its 25 literal requirements
are compiled but have not executed. Test-local find_package(Boost1.83 CONFIG) makes
its imported header target explicit in this sibling scope.

## Commands actually run

Local GNU16.2.1/C++20, Boost1.92.0, installed GoogleTest1.18.0, Python3.14.8/Ninja.
These are observed local versions, not the future selected CI1.83 proof.

- `cmake --preset milestone-progression-debug -DSUBATHON_GOOGLETEST_SOURCE_DIR=`:
  PASS, no downloads.
- `cmake --build --preset milestone-progression-debug`: PASS, four incremental
  compilation/link steps, no warnings. Build discovery lists cases only.
- `ctest --preset milestone-progression-debug --show-only=json-v1`: PASS, 90 total:
  53 new feature cases, 33 old numeric, three infrastructure, one new foundation.
- Actual workflow inventory Python block extracted verbatim and executed locally:
  PASS. It requires 33 direct TEST names and 20 unique literal parameter labels,
  exact CTest set/labels, no disabled/inverted feature case, and one enabled,
  uninverted foundation outside the intentional-red label. No bodies executed.
- `nm -C build/milestone-progression-debug/tests/milestone_progression_environment`:
  inspected, no subathon symbols. Executable not invoked.
- Project consistency, current HTML and whitespace checks accompany this commit.
  They do not establish milestone acceptance.

## CI coverage and pending gates

Published PR56 `00d8c57212cb10b988437721ed82607a1d8c1318` has all12 actual exact-head
check-runs SUCCESS; all11 post-main workflows at `dd6a141` passed. Those existing
component checks predate this new test harness and do not execute milestone behavior.

The actual new `.github/workflows/milestone-progression.yml` pins the existing
checkout action, Boost1.83 Ubuntu package/config/header macro and pinned explicit
GoogleTest fetch. It builds/discovers all53 and runs the reviewed independent
foundation plus old checks with `-LE intentional-red`. The source inventory guard
prevents absent/disabled/inverted cases. The FIRST feature red remains a separate,
plain CTest invocation with no failure inversion, skipped baseline or catch-as-pass.

Different review must approve the actual test/foundation/harness/workflow. Only
then execute the approved local foundation. Publishing the new workflow requires
the pending restored SSH/workflow scope; do not bypass permission or publish an
unreviewed foundation for CI execution. Observe actual current selected CI/foundation
green before FIRST plain `ctest --preset milestone-progression-debug -L '^milestone-progression$'`
with saved raw/JUnit/clean-head evidence and independent failure attribution.
No selected new CI, first red, implementation or product milestone acceptance is
claimed. Ruleset26170263 does not enforce this newly authored check automatically;
written workflow still requires all applicable latest-head CI before normal merge.
Root must assess required-check configuration if repository enforcement changes.
