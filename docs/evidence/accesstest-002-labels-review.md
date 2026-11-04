# ACCESSTEST-002 independent label-compatibility correction review

Reviewer service_reviewer, different from root correction author, 2026-11-04.
Clean isolated `task/ACCESSTEST-002-labels-review` starts at actual correction
`3dc7108165178c49e4762f6489e7db03007676d4`. Only this new review note changes.

**APPROVED for this actual harness correction.** It preserves native discovery,
all four labels, exact inventory/property guards and intentional-red separation.
The earlier `7d7f651` local harness review remains historical. This closes its
label-compatibility gap; no source/oracle/future behavior or merge approval follows.

| Actual corrected artifact | Git blob |
| --- | --- |
| tests/CMakeLists.txt | `f1b205e8f366f1e0d922b97d244a33680bd1483b` |
| tests/cmake/EffectivePermissionLabels.cmake | `022fc6a829df9683c30b2f1c70bd92e14fb4d952` |
| Root correction evidence | `3776e3c672b02d9f4dba753eb3994546ef031f68` |

Workflow remains `5dc7b4de4fbbdc566927290788c3b1e4f865c7ed`, test source
`fc82d17ede5d2afe66e42572c17762510e5682f4`, inventory
`d0c25108ec233cdc11260aa1263974805b0fba15`. Their earlier separate approvals are
not expanded here. I authored the minimal module and SDK foundation; neither is
self-reviewed or replayed by this correction review.

The explicit `TEST_LIST subathon_permission_cases` is populated by discovery.
Only scalar TIMEOUT 10 travels through the GoogleTest helper; the appended
TEST_INCLUDE_FILES script runs after the generated discovery include and assigns
the quoted four-label list directly. It rejects an absent/empty discovered list.
There is no second test enumeration, version-specific escaping, disabled case,
failure inversion, custom skip/result rewrite, filtering or source assertion edit.
The unchanged strict workflow still requires exactly the actual 30 names, labels,
timeouts and enabled/uninverted properties before any green lane runs.

This uses the extension mechanism documented by
[CMake 3.28 GoogleTest](https://cmake.org/cmake/help/v3.28/module/GoogleTest.html)
and [TEST_INCLUDE_FILES](https://cmake.org/cmake/help/v3.28/prop_dir/TEST_INCLUDE_FILES.html).
Read the [official 3.28.3 helper](https://github.com/Kitware/CMake/blob/v3.28.3/Modules/GoogleTestAddTests.cmake)
and local 4.4 generated output: property-list handling differs. The failed CI log
`/tmp/accesstest002-ci-6b9-failed.log` establishes the guard failure, not the runner's
precise CMake version or actual malformed label shape. That specific attribution
remains an inference, not an observed old-tool runtime result or behavioral red.

Actual independent fresh configure/build/CTest show-only PASS in this worktree,
with the existing authenticated Boost prefix and installed GTest, no fetch. Both
EXACT unchanged workflow discovery guards PASS: 67 total = 30 feature + one SDK
environment + 36 old cases. Every feature has the exact four labels, timeout 10,
standard GoogleTest skip detector and no custom result properties. The generated
CTestTestfile explicitly includes discovery before this label script.
Commands/raw JSON remain `/tmp/subathon-accesstest002-labels-review-vv618w_f`;
commands.json SHA-256 `fb4bcc6fbb01e1fe6fd27a508727efddbf6077e30907adf5b54740faf9a6b317`,
discovery.stdout `1c681e6652acf2c7f129ece6bf699f74c33ad31afa88a83df9ea8c489da896cc`.

Also read/cross-checked root's `/tmp/accesstest002-cmake328-labels` records:
the authentic old helper source plus host CMake/CTest and the actual label include
produce the SAME 30 names/four labels/timeouts. discovery.json SHA-256
`f7fadefd24545e4882e42a82d78f61d73032ba9dbf7e975395748bbdb391921d`.
That reproduction was not rerun and does not claim installation/execution of an
older CMake distribution. No feature/test/foundation/SDK arithmetic/marker body ran
in this review. Project consistency/current HTML/sole-note scope/whitespace are
checked before commit. Root reports corrected selected CI run41886146750 green
at `4674822`; current CI/first-red/attribution/status/normal merge remain root-owned
gates, with no future source/test or merge approval from this note.
