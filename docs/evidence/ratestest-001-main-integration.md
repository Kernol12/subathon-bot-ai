# RATESTEST-001 current-main and selected1.83 harness integration

Root author,2026-11-04. Merge501ebab integrates current mainfc28558 after normal
PR59 merge from595748a/all14 final PR checks and13 post-main checks SUCCESS.
PR62 is also complete with13 PR/12 post-main checks. Normal SSH works; no owner
question/access blocker remains. All actual rate test reviews and artifacts survive.

The merge retains all406 current main tasks/1497 edges plus the existing required
RATESTEST-009/RATES-009 exact-valuation bridge and EPIC-017 prerequisites:408 tasks,
1508 edges. Current graph contains every previous main and rate edge. Main's password/
permission flags, targets, presets, callbacks and completed baseline statuses remain;
rate preset23 precedes preserved25/26. The extra milestone historical note remains.
No reviewed rate source, header,53-method test, fixture, expected oracle or stub
changed:64fd60b2/8936f4fa/3a06ff18/4cf1c1ed, full actual approvaldcf5dcaa.

The focused label correction uses explicit TEST_LIST plus CTest-time include, as
already selected-CI-verified in PR62/59. Four rate labels and30s timeout remain.
The workflow retains all53 source/unique/label/enabled/inversion checks and adds a
mandatory independent foundation metadata guard and explicit no-tests errors.
Missing/disabled/inverted/red-labelled foundation cannot silently pass. Green CI
still compiles/lists intentional-red features and requires only SDK/old green suites;
it does not execute feature bodies or rewrite their failure expectations.

Actual selected configure/build/list pass:

```sh
cmake --preset rate-snapshot-debug \
  -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 \
  -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset rate-snapshot-debug
ctest --preset rate-snapshot-debug --show-only=json-v1
ctest --preset rate-snapshot-debug -LE '^intentional-red$' \
  -E '^scaffold.rate_snapshot_environment$' --no-tests=error
```

GCC16.2.1/CMake4.4.5/Ninja1.13.2/GTest1.18.0, authentic explicit Boost1.83.
Exactly90 discovered =53 rate+33 numeric+old3+1 independent SDK. The actual CI Python
inventory+new foundation guard passes on real discovery. Workflow syntax7 shell/
2 Python blocks pass. All old36 checks pass. Logs:
`/tmp/ratestest001-main183-{configure,build,discovery}.*` and
`/tmp/ratestest001-main183-old36.log`. Initial local guard lookup used a singular
workflow filename; correcting it to actual rate-snapshots.yml changed no candidate.
That inspection error is not red evidence.

[Selected host proof](ratestest-001-selected183-host.md) already records FIRST
selected1.83 standalone14-fact PASS after actual different source/selection review.
It was not replayed after this merge. No new rate feature/setup/marker or53-case body
has run. RATES-001 and all provider/valuation/bridge/platform behavior remain
unimplemented. Next: different actual changed integration/harness review, normal
publish and selected Ubuntu CI, FIRST ordinary53-case intended red/independent
attribution, final status/current checks/review/discussions/rules/normal merge.
