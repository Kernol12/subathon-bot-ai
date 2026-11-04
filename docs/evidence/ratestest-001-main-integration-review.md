# RATESTEST-001 independent root integration/harness review

Reviewer `/root/codec_author`, different from root config/integration author,
2026-11-04. Clean candidate `bf5986d5ae418fb2c921b147b18927aa3f3ae996` includes
merge `501ebab` of main `fc28558`. Only this review note changes.

**APPROVED for the actual root main/config/harness changes and integration records.**
No blocker found. I authored the original rate test/foundation source and recent
compile preparation, so do NOT reapprove either here. Independent full approval
`dcf5dcaaf8c33d76223278149b018d56efa5a50c` remains the unchanged test/source gate;
root's different selected-library review remains the preparation gate.

| Reviewed changed artifact | Git blob |
| --- | --- |
| Root CMake | `4b7c5437688991e75105238f40f1fe3a6faf8144` |
| Presets | `c9438037af6456ce5dee5a50104e4777412bb040` |
| Core CMake | `9d97e0490513f1ba4dd4e05c0b2e7f4bfccd64f0` |
| Test CMake | `3c7c8389b404cba1aae6dd55ba7d15a75e3fbd32` |
| [Rate label callback](../../tests/cmake/RateSnapshotLabels.cmake) | `720bc8111fdd37642fc5223df95e81c854c6916c` |
| [Workflow](../../.github/workflows/rate-snapshots.yml) | `254559235218a352677580aa166cedae5ed0dc81` |
| [Integration note](ratestest-001-main-integration.md) | `2973ffbe0c02f9ee697b5d5b48ef70ea0a67410b` |
| [Root selected-host receipt](ratestest-001-selected183-host.json) | `147e2f2578316219a7aff610cce24766263002de` |

Read root/core/tests instructions, actual config/workflow diffs, checkpoint/state
and root records. Existing main password/permission options, targets, presets,
callbacks and completed-baseline statuses remain. Rate stays opt-in, brings its
required exact-numeric target, and registers its separate SDK-only executable
without linking a project/SUT library. Preset 23 coexists with 25/26.

The distinct `subathon_rate_snapshot_cases` TEST_LIST is populated before the
CTest-time callback applies all four labels. Timeout 30, ordinary enabled tests
and the exact 53-name CI guard remain. The new foundation guard requires exactly
one enabled/uninverted separately green environment; both execution steps now
explicitly reject empty selections. CI requires foundation then old green checks
while excluding intentional-red feature bodies. No assertion/result inversion,
test skip, source change or successful SDK substitute was introduced.

Graph comparison confirms all main and old-rate nodes/edges survive: 408 identities,
1508 edges. Relative to main, additions are precisely RATESTEST-009/RATES-009 and
their required EPIC-017/bridge prerequisites. Only RATESTEST-001 fields, those bridge
records/epic dependencies and one historical milestone note differ. No valuation
adapter capability or implementation completion is inferred from the bridge plan.
Tests `64fd60b2`, fixture `8936f4fa`, header `3a06ff18`, stub `4cf1c1ed`, standalone
foundation `28a829aa` and independent review blob `5722eba2` are byte-identical to
the preserved rate branch; no original oracle/source review was redone.

Root's different selection review `a5963b39` predates the recorded FIRST selected
host 1.83 run at 19:11:53.997709Z: exit 0/14 facts, receipt `f070ca7f`. The selected
host record clearly distinguishes the earlier unrun compile snapshot from that
subsequent execution. I checked record identity/chronology only, not my preparation
or source correctness, and did not replay a native run.

Read retained selected configure/build/discovery and old36 logs: 90 cases are 53
exact labelled rate methods, 36 old checks and one independent green foundation.
Names/enabled status/timeouts match; old36 passed. Read-only JSON/Git graph/hash
checks, YAML parsing, seven `bash -n` bodies and two Python syntax compilations
passed. No configure/build/discovery, foundation/SDK or feature/test body ran here.
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed: 408 tasks/122 decisions, links/presets/current HTML and
whitespace. Sole-note staged inventory/whitespace was checked before commit. Selected Ubuntu CI,
FIRST ordinary 53-case red/independent attribution, final metadata/current checks/
reviews/discussions/rules and normal merge remain root-owned required gates.
