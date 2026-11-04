# MILESTONETEST-001 current-main harness integration

Root,2026-11-04. Isolated continuation e90d9f9 integrates mainfe727c8 after
normal PR62/59/54 merges, preserving408 tasks/1508 dependency edges, including the
required RATESTEST-009/RATES-009 bridge. PR54 post-main14 checks now all SUCCESS.
The primary8721770 checkout and historical worktrees remain untouched.

The original53-case test/fixture/source review208d0580 remains valid: exact blobs
01997384/409be19d/9a167813/4e3291f0 unchanged. Full review is linked in
[milestone test review](milestonetest-001-test-review.md).
The old CMake GoogleTest helper flattens list-valued LABELS. Current change moves
only complete labels to the native CTest include, after TEST_LIST discovery,
preserving NO_PRETTY_VALUES and all20 named parameter cases. Missing discovery
fails closed. The workflow additionally says --no-tests=error; its existing strict
33+20 enabled/uninverted inventory/foundation guard is unchanged. No test assertion,
fixture, marker implementation, selected dependency or green/red policy changes.

Actual isolated selected1.83 configure/build pass, GCC16.2.1, with the authentic
prefix described in [selected host receipt](milestonetest-001-selected183-host.md):

```sh
cmake --preset milestone-progression-debug -DSUBATHON_FETCH_GOOGLETEST=ON -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset milestone-progression-debug
ctest --preset milestone-progression-debug --show-only=json-v1
ctest --preset milestone-progression-debug -LE '^intentional-red$' -E '^scaffold.milestone_progression_environment$' --no-tests=error --output-on-failure
```

Actual workflow inline guard passes exact53 features and one normally enabled
foundation; total90 names. Old36 pass0.74s/exit0. Logs in /tmp/milestonetest001-
selected-configure.log, selected-build.log and old36-green.stdout/.stderr.
No feature body/marker or foundation body ran here; already observed FIRST selected
host25 is preserved without replay. Different actual current harness review,
selected Ubuntu CI25+old36, FIRST plain53 red with native bytes/direct return code
and separate attribution remain before final baseline status/normal merge.
