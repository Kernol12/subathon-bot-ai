# ACCESSTEST-002 independent test-harness review

Reviewer service_reviewer, 2026-11-04; root authored this harness. Clean isolated
`task/ACCESSTEST-002-harness-review` starts at actual
`b519153670b894b2a21b8bc4346d7c7e926f9c77`. Only this new note changes.

**APPROVED for the actual CMake/CI test harness only.** No blocking harness defect
found. Test author service_author's oracles/inputs remain codec_author's separate
review. This reviewer authored the inert module and SDK foundation, so does NOT
self-review either: their earlier different reviews remain the applicable gates.
In particular [minimal-source approval](accesstest-002-source-review.md)
`7ddc58b0`/root import `f1acce3` is inherited, not replaced or expanded here.

| Exact reviewed artifact | Git blob |
| --- | --- |
| tests/CMakeLists.txt | `57963624af45fd1ad882f5a21670a5a46a247383` |
| .github/workflows/effective-permissions.yml | `5dc7b4de4fbbdc566927290788c3b1e4f865c7ed` |
| Unchanged GoogleTest dependency helper | `ecedcf7a9f7084d9af7d5f12b9cca2612283fb39` |
| Unchanged preset file | `948f3b9bcdd9b869837dadfa99d4d41b45ce8080` |
| Unchanged core target file | `1f01466ed7231e2a2852c3b494e7ad167cc1f88e` |
| Case inventory, names only | `d0c25108ec233cdc11260aa1263974805b0fba15` |

Read root/tests instructions, actual source-approval evidence, full current workflow/
target/preset/dependency definitions and their delta from `a047009`. The added
feature flag participates in finding GTest even without another suite flag. The
new executable links the reviewed inert module and GTest main; existing warning
options apply. GoogleTest discovery lists names, with prefix `effective-permissions.`
and exact labels `effective-permissions;acceptance;core;intentional-red`, TIMEOUT 10.
No added configure/build rule invokes a feature/test body. The source has ordinary
TEST registrations, with no parameter/global setup registration needed for listing.

The workflow compares real CTest discovery against the actual unique, nonempty
30-name inventory. All 30 source registration names match that file. It rejects
missing/extra/duplicate discovered names, disabled/failure-inverted cases, custom
skip/result-rewriting properties, changed labels and changed timeout. The only
allowed skip expression is CMake's literal standard detector `\[  SKIPPED \]`
(or its absence); the actual generated property matches it. Installed
GoogleTestAddTests.cmake independently confirms this default. SKIP_RETURN_CODE and
PASS/FAIL_REGULAR_EXPRESSION are rejected even when present with empty values.
This preserves the standard discovery behavior without admitting a custom success
or skip rule. Source/inventory changes still require their actual independent
review; this verdict is not approval of future edited tests.

The separate SDK environment is discovered exactly once, enabled/uninverted and
outside intentional-red. CI requires its actual run, then the old numeric/
infrastructure lane with `-LE '^intentional-red$'` and the environment excluded.
Current discovery gives 30 new feature cases + one SDK environment + 36 old cases.
Ordinary CI therefore compiles/discovers the new baseline but never invokes its
unimplemented behavior; FIRST intended red remains a separate root-owned command.
There is no blanket continue-on-error, WILL_FAIL or test-result inversion.

CI retains Ubuntu 24.04, apt's exact `libboost1.83-dev=1.83.0-2.1ubuntu3`, the
compiler macro check for 108300 and both explicit Boost CMake directories.
Checkout remains immutable commit `3d3c42e5aac5ba805825da76410c181273ba90b1`, with
read-only contents and no persisted credentials. GoogleTest uses the existing
system/local-first >=1.17 config policy; explicit fetch authorization permits its
pinned fallback `063de7e9578f82b369302001269680b4b1553359`. This is not a claim
that every runner necessarily chooses fetch instead of a qualifying installed SDK.
No dependency helper/pin/source assertion was changed by this harness delta.

## Actual independent checks and limits

In this fresh worktree, configure/build/discovery-only PASS with the authenticated
isolated Boost 1.83 prefix and existing `/usr/lib/cmake/GTest` (fetch OFF):

```sh
cmake --preset effective-permissions-debug -DSUBATHON_FETCH_GOOGLETEST=OFF \
  -DGTest_DIR=/usr/lib/cmake/GTest \
  -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 \
  -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset effective-permissions-debug
ctest --preset effective-permissions-debug --show-only=json-v1
```

Configure 0.297s/build 2.974s/discovery 0.010s, all exit 0; build stderr empty,
zero compiler warnings, 67 listed cases. Ran the EXACT two workflow discovery
guards on the real configured build: both PASS, 30 feature names/properties and
one independent environment. All eight shell blocks pass bash -n; all three
embedded Python blocks pass AST syntax checking. No shell build/install/test lane
was executed by that syntax check. Static registration/inventory/property checks
also PASS; no feature, SDK arithmetic/foundation, test body or marker was run.
No SDK preparation/download or source reapproval was performed.

Raw commands/discovery/guard outputs remain under
`/tmp/subathon-accesstest002-harness-review-x79fjcjl`:
commands.json SHA-256 `165f74a921f4dab31d7c87424fd81dde72947e5f2f059b2d2e5b4d2483b3345b`;
discovery.stdout `68ec1829c84d264470ffafc79a847bd126da4ffc4556961943160384e329c993`;
harness-audit.json `b239a617ee87d5d7c63d100f5057746c6d25b5ffe0bc3319a52f82dc830e3463`.
Project consistency, unchanged current HTML, sole-note staged scope and whitespace
are checked before commit. Root retains publication/state/build/CI ownership.
Actual full test review, current selected Ubuntu CI, FIRST intended red and later
production/source/acceptance/merge gates remain separate. No future approval.
