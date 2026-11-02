# STORAGETEST-015 minimal interface/build integration

Coordinator `/root`, 2026-11-02. This is a successful **minimal interface build**
and inherited regression run, not new plan behavior, test approval or a red run.

Actual full logical/physical contract approval `a2926ee` was integrated as
`c61b990` before authoring these minimal interfaces. Author candidate
`134c9f4dddb67cee5d5d365774f9b1fbb8dff94d` was integrated without conflict as
`dc3676b`. Its four paths contain the new header, nine immediate named logic-error
stubs, appended Concrete API precision and the author's own evidence. The original
approved logical prefix and complete physical proposal remain unchanged.

| Artifact | Actual blob |
| --- | --- |
| New header | `4634d7b57dccc5c559f2ab05a968833b4b925feb` |
| Nine-throw source | `e989d70f5021a9285db0dbc79c32d1153b4e7700` |
| Logical contract with appended precision | `3af256cdf82a4737c2fc71a67c88b8b332d6ba71` |
| Root CMake | `58e443305f4c5fbc5834d0946e3a7facdf416e42` |
| Presets | `16cdf68c05f10907a6b6de24fd2032bcf05bb37d` |
| Core CMake | `90de2e1e3add9063066c58fb843b2eb2d648b58d` |
| Server CMake | `5f4a3179d1e16a77d7da421d91799cbb51037d0b` |
| SQL workflow | `81ba8204cfc8d10abec8ec3f8b7a87c710d132ac` |

The new `sqlite-plan-debug` configure/build/test profile inherits all existing
SQL flags and enables `SUBATHON_BUILD_SQLITE_PLAN_TESTS`. It builds the new
`subathon_sqlite_application_plans` static library with existing contribution and
core plan targets. Core stays independent of Qt. Existing Boost headers are
available without enabling core plan GoogleTest suites or GoogleTest fetching;
the actual new cache confirms both remain OFF.

The SQL workflow selects this profile and the already selected Boost
1.83.0-2.1ubuntu3 package/config directories, verifies the actual compiler header
macro, and preserves all existing Qt archive/action pins, permissions, check name,
diagnostic registration checks and unconditional acceptance suites. The complete
old workflow body from its environment-probe step onward is byte-identical after
profile-path substitution. No new known-red suite is registered yet. This actual
workflow revision must still run on GitHub; local actionlint is not runtime proof.

Actual root commands, all exit 0:

```sh
cmake --list-presets
cmake --preset sqlite-plan-debug
cmake --build --preset sqlite-plan-debug
ctest --preset sqlite-plan-debug
ctest --preset sqlite-plan-debug --show-only=json-v1
/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Configure/build used Linux x86-64, GNU16.2.1, CMake/CTest4.4.5, Qt6.12.0 and host
Boost1.92.0. The actual build completed all **74 steps with zero warnings**, including
the new source object and static library. All **143 inherited cases passed in
3.23 seconds**: 25 writer/ownership, 24 work, 24 migrations, 16 retention,
22 receipts, 24 contributions and eight scaffold/environment registrations.
No new plan test is registered or invoked. No old missing-behavior red was repeated.
Ignored raw logs and registration JSON are under
`local-evidence/coordinator/storagetest015-interface-*`.

New fixture author `plan_fixture_author` completed read-only preparation only:
no files, SQL, runtime or tests. Different reviewer `contribution_reviewer` must
review this actual API/stub/precision/shared-build/workflow candidate before any
new test, literal fixture or green-helper authoring. Later full baseline review,
compilation, selected official Qt green and the first genuine missing-behavior red
remain required before STORAGE-015 implementation. Earlier reviews, fixtures,
assertions, producer sources and release gates remain preserved. There is no owner
decision/access request.
