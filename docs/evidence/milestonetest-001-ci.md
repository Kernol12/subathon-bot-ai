# MILESTONETEST-001 build, inventory and CI evidence

Coordinator /root, 2026-11-04. The 53-case source author candidate
`9f74e77923cdf8d66df7d2dcd625178713306bb6` is imported as `643de5c` after main
`dd6a141` integration `00d8c57`. No milestone feature acceptance test/function/marker body has run. The approved local primitive foundation
subsequently passed as recorded below.

## Actual source and harness

Frozen source: header `9a1678137ba6f85cbfbb545630f08d36a261790b`, TU
`4e3291f0e832237470a8cd0bc349e0106ad8fd77`; logical/API `0115a5cc`/`58b8e129`.
Existing separate minimal review `a2f5d358` remains valid for those unchanged files.
New fixture `409be19d57b2cba4025e236a67a539af63b08be4`, tests
`019973844dceb289aa82ec8aabd3d9274faef98e`, standalone foundation
`7f8c3f9466a0e3b313aa7e3f492b7ee82b91513d` and
[author note](milestonetest-001-test-author.md) `a54088836e54f28e4744378d136f37f485d6bdbd`
are authored candidates. Narrow foundation review is now approved; full feature
test/harness review208d0580/importac9234a is now APPROVED with no correction.

Root adds the acceptance target and deterministic raw parameter names using
NO_PRETTY_VALUES; 20 literal MP rows plus 33 ordinary tests. All receive
milestone-progression/acceptance/intentional-red/core labels and a 30-second timeout.
The foundation has its own main, Boost::headers only, no project includes/link/symbols,
and scaffold/milestone-progression-environment labels. Its 25 literal requirements
were initially compiled without execution; the approved first local run is below. Test-local find_package(Boost1.83 CONFIG) makes
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

## First approved local foundation execution

Different narrow [foundation review](milestonetest-001-foundation-review.md), commit
`de0a30f09e0f2769d482989ceff23cd8eef2222d`/root import
`7d7e62174fd8872641cbfd68d35cc73cd69bfe91`, approves the exact primitive program,
link boundary and safe green harness. On that CLEAN head at
2026-11-04T11:55:23Z, the FIRST native invocation was:

`ctest --preset milestone-progression-debug -R '^scaffold.milestone_progression_environment$' --verbose --output-junit <ignored-foundation-first.xml>`

PASS 1/1, 25 independent std/Boost literal facts, no skip/inversion, exit0, 0.00s.
Raw/JUnit/metadata are ignored local-evidence/milestonetest001-ci/foundation-first.*;
raw SHA256 `9a83525e34fb2ee0557715cdb97dbda45162babd3e87f291f82f60c371c14de8`.
This uses local Boost1.92/GNU16.2/GTest1.18 context. No new feature/test/marker body
ran; this is neither current selectedBoost1.83 CI nor intended feature red.

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

The narrow foundation/link/green-harness review released only that foundation
execution. Full different53-test/harness/workflow [review](milestonetest-001-test-review.md)
208d0580/importac9234a also APPROVES the unchanged candidatefa098e2. Feature red
still waits for actual current selected CI. Publishing the new workflow requires the pending restored SSH/workflow
scope; do not bypass permission. Observe actual current selected CI/foundation
green and full test approval before FIRST plain `ctest --preset milestone-progression-debug -L '^milestone-progression$'`
with saved raw/JUnit/clean-head evidence and independent failure attribution.
No selected new CI, first red, implementation or product milestone acceptance is
claimed. Ruleset26170263 does not enforce this newly authored check automatically;
written workflow still requires all applicable latest-head CI before normal merge.
Root must assess required-check configuration if repository enforcement changes.

After actual full approval, gh authentication was rechecked: workflow scope still
absent; SSH_AUTH_SOCK absent and existing GPG agent identity count0. The task is
blocked before selected CI/first feature red, with all reviewed work preserved.
No permission bypass, fake success or baseline completion is claimed.

## Superseding current selected lane and FIRST feature result

The earlier publication block above is historical and resolved. SSH publication
succeeded ordinarily. Exacta15a70e selected Ubuntu run41907483021 passed: pinned
Boost1.83.0-2.1ubuntu3/macro108300, exactenabled53 discovery,25 independentfacts
plusold36=37/37green0.98s. Actual changed harness approval75e62c2a importedc2bcb08
precedes FIRSTplain53 run on cleanc2bcb08,20:36:28Z/directexit8/all53fail/0skip.
See [native evidence](milestonetest-001-first-red.md); different actual attribution
and finalmetadata/latestchecks/normalmerge remain. No previous hostfoundation,
first red or frozen source/oracle has been replayed or changed.
