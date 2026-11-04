# CORETEST-008 independent test and oracle review

**APPROVED — exact authored tests and inventory only.**
Reviewer `service_reviewer`, different from test author `service_author`,
2026-11-04. Reviewed actual author
`78a7936b51b81f7a14c8adc85ed75598eea204bf` in isolated
`task/CORETEST-008-tests-review`. Only this note changes.

| Actual artifact | Reviewed Git blob |
| --- | --- |
| [Acceptance source](../../tests/acceptance/timer_editorial_commands_tests.cpp) | 3b0c80411e73736925ed2491e12741cbb03aedeb |
| [Name/coverage inventory](../../tests/fixtures/timer-editorial-commands-v1.json) | 1a4fb38d1a28b768e138590dca46e8b3dc5e0abc |
| [Author evidence](coretest-008-tests-author.md) | e8e8d07022b52f29682ee1b48cc5a8dc4b8097f5 |

Compared the entire 1188-line source and complete inventory with approved logical
`f4a4c3ceea45a58bc8116f73169d7bac081e9919`, API
`a2ac5a2c620dbd2f1ff2805e1d3a1a973ccb3797`, specification
`81406806c02df0f015a8c71c40b149ae1ff0ed19` and the recorded three owner choices.
Header `92281931bd09e9130b3266b5465f6cff6d5f81de` and sole immediate-stub TU
`651c68b3db599849c3e539c908e6dd57f633c0bf` remain unchanged. This review does not
repeat their different source/harness approval `1648acf1` or old 74-check run.

No oracle defect or requirement conflict was found. All 27 exact unique
TimerEditorial.* groups match the JSON in order; all ED01–44 and API sections 1–6
are mapped. The JSON is metadata, with no expected values or runtime fixture reader.
Raw builders contain explicit aggregate literals and no permission/validity factory,
prior successful projector/Start or CORE-002/005 call. Expected snapshots, audit,
adjustments, warnings and traces are independently declared rather than generated
by a reducer or the function under test. Only the new projector is invoked.

The cases preserve per-attempt Keep/Reduce/Abort and missing-choice behavior,
current-versus-preview reduction, countdown while warning is open, no added Main
when excess disappears, and old-revision/access refusal. Keep's above-cap Main,
debt-first future additions and restored headroom after real countdown are covered.
Ready/Finished settings preserve lifecycle/history; their ordinary deltas retain
required first-start/finished-policy outcomes. First-Start clear/blocked checks
never claim a complete Start or grant authority.

Old deadline observations cannot be erased by disabling/moving it; new reached
Active deadlines preserve pre-call Main/Grace and clear confirmation wait, including
paused states. Separate monotonic elapsed/UTC, signed UTC extrema, huge independent
steps, explicit pause segments, chronological observation/editorial batches and
nonrepeated Finished transitions follow the frozen contracts. Immediate,
Confirmation, Fresh/Shared and four-event zero-Grace exhaustion have literal oracles.
Non-exhausting RequireReview subtraction, full signed minimum, partial-debt
addition overflow rollback, whole-request timer review and preserved prefixes are
also correctly bound. No mixed/global application policy or financial original
effect is invented.

Full result comparison checks every raw input field/variant, values/configuration,
status/prerequisite, optional presence, all six adjustment scalars, all warning
fields and used/unused fixed transition slots. Variant indices and optional
presence are checked before access; actual/expected container sizes are compared
before bounded traversal. The lifetime case destroys/mutates supplied input,
copies/moves the result and checks the complete independent expectation, using
long strings, embedded NUL, retained preview and both trace containers.

Structural cases preserve full enum domains, invalid raw/access/preview/clock
shapes, category ordering and dependent preview validation after revision/access
refusal. Resource cases cover four option profiles, zero/maximum/derived overflow,
configurable 4096/4097 string and step boundaries, complete input/result charges,
refusal audit and exact/one-under limits. No physical heap or successful fault-
injection claim follows from these scalar tests.

## Checks, first-red cautions and remaining gates

Stdlib-only static/hash checks passed: 27 names exactly match inventory; all 44 ED
IDs/API sections are mapped; every TEST reaches cases/invalid_cases/expect_error
or the direct ownership call. The only three projector source sites are inside
those body-invoked helpers or the ownership test. There is no old primitive call,
skip, disabled group, generic throw-as-success or implementation-generated oracle.
Independent integer/B checks passed 22 facts: input/result 82/224, denied 81/92,
Reduce 93/284, held 83/186, two-step 99/246, signed partitions, current reduction,
debt overflow, option-derived overflow and default 16388/24580 bounds. Eight exact
candidate/frozen blobs were verified. No evaluator is run by that audit.

Proposal helpers turn module-marker exceptions into nonfatal ADD_FAILURE; error
helpers require the exact typed domain failure/code/fixed text, and report markers
as wrong exceptions. Later independent rows remain reachable. In the ownership
group, the caught primary marker is followed by a deliberate retained-value
ASSERT_TRUE safety gate; FIRST attribution must classify that second failure as a
consequence, not another marker. Some logical variant rows reuse a trace label;
attribute their observed multiplicities against their source loops, not an invented
unique CTest name. Assertions later in a throwing lambda have not executed.

`python3 tools/check_project.py` (406 tasks / 122 decisions),
`python3 tools/progress.py render --check` and `git diff --check` pass. This reviewer
performed static/document checks only: no compile, SDK/foundation, test body,
project feature/marker, old native test or red replay. Author compile/link/list
records are explicitly inert evidence, not behavioral acceptance.

The actual changed root harness/CI/current-main integration requires its separate
review. Only after that and valid selected setup may root record FIRST plain red
and obtain actual attribution. This approval does not complete CORETEST-008 or
approve that future run, implementation or merge. Production source audit must
still verify preallocation/helper-copy ordering, valueless variant handling and
allocation/length-error mapping; public budget assertions do not measure those.
Host rights/queue/clock/CAS/UI-warning, first Start/reopen/absolute-Main/debt,
mixed/global review and full release/platform gates remain required.
