# CORETEST-008 baseline harness author handoff

Author `service_author`, 2026-11-04. Explicit coordinator delegation on isolated
`task/CORETEST-008-harness-author`, created from frozen test-author
`78a7936b51b81f7a14c8adc85ed75598eea204bf`; primary/integrator checkouts untouched.
Only [tests CMake](../../tests/CMakeLists.txt),
[new CTest labels](../../tests/cmake/TimerEditorialLabels.cmake),
[new CI workflow](../../.github/workflows/timer-editorial.yml) and this note change.
Actual test approval bd7f5e3e was read; tests/source/contracts/inventory are unchanged.

Blobs: CMake `f6cf5e14b91c6cf205d08dd015eaaa10fc3884ce`, labels
`d6e6aa448586bd401c4164caba1024e00c6107d5`, workflow
`ed04c5e649bf207ddd0f11986154230cee56559b`. Existing default-OFF option/preset27
is reused. The new target links the actual module/GTest main and discovers with
prefix timer-editorial., TEST_LIST, timeout30 and complete CTest-time labels.
CI pins the existing checkout SHA on Ubuntu24.04, explicitly fetches the repository's
existing pinned GTest (system find disabled), and guards all 27 source/JSON names,
44 ED mappings, enabled/uninverted/unsuppressed properties and exact 101=27+74
registration before running only the approved 74 dependency checks. No SDK added.

Actual local GCC16.2.1/CMake4.4.5/installed GTest1.18.0 commands:

```sh
cmake --preset timer-editorial-debug
cmake --build --preset timer-editorial-debug
ctest --preset timer-editorial-debug --show-only=json-v1
ctest --preset timer-editorial-debug -LE '^intentional-red$' --no-tests=error --verbose
```

Configure/build PASS; discovery101 and the ACTUAL extracted workflow guard PASS.
Then one CTest run exited0: exactly the expected74 Passed names, 32 debt/39 lifecycle/
3 infrastructure,0 editorial; 0.76s. A subsequent Python collector assertion expected
old CTest summary wording and exited1 AFTER successful CTest. Raw byte logs/exit0
receipt remained intact. Recovered exact-name/exit0 proof without test replay; no
strict-UTF8 decode failure or SUT/editorial failure is disguised. Receipts/raw logs
are at `/tmp/subathon-coretest008-harness-aqpgn_n1`, including `discovery.json`,
`guard.json`, `green.json`, `green-validated.json`, binary stdout/stderr and the
collector explanation. Build/guard/green children inherited RLIMIT_CORE=0.

YAML parse, four shell blocks and one Python guard syntax PASS; frozen test/source
hashes, `python3 tools/check_project.py` (local406/122), progress render --check and
staged scope/whitespace PASS. No editorial/marker/FIRST-red body was invoked, no
source/test/fixture/spec/state/schema/pin edits or push occurred. Selected Ubuntu
CI has not run for this new workflow. Root's current-main integration, different
actual harness review and current CI/environment/FIRST red remain required; no
self/future approval or completed baseline is claimed.
