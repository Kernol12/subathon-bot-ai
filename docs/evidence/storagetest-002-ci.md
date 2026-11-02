# STORAGETEST-002 CI / minimal build evidence (active)

Coordinator `/root`,2026-11-02. Minimal interface/build stage, NOT bundle behavior
or the feature baseline. Author1662219 imported2d0794e supplies owned data and
seven immediate named unimplemented entrypoints; constructor only stores weak
session/options. New typed-read contract extension2f6e9a8e is pending actual
different API/extension review; prior4d review remains valid for exactcf462 subset.

## Scope and actual artifact identities

New SUBATHON_BUILD_SQLITE_BUNDLE_TESTS enables subathon_sqlite_application_bundle.
Preset sqlite-bundle-debug inherits existing sqlite-plan-debug. Core stays framework
independent; server has existing Qt/Boost dependencies. Workflow diff normalizes
EXACTLY to the previous file after changing only preset/build-directory tokens.
All dependency/action/archive pins, permissions, check names, old commands/guards
remain byte-identical. No new executable test/fixture is registered yet.

| Actual artifact | Git blob at observed build |
| --- | --- |
| `CMakeLists.txt` | `1f25732eb1d6b114df21bd9327203dd2e551149f` |
| `core/CMakeLists.txt` | `9fb0b58ba84fd081b34c59013c858120eb0fc42a` |
| `server/CMakeLists.txt` | `23b740486282e1bf01c85e5845c1403795e3ac6d` |
| `CMakePresets.json` | `70af89b3f04995224b1210baa53b6affacd71495` |
| `.github/workflows/sqlite-writer.yml` | `a9835c8cded5d0568ad27dd6696cbe3271e5b402` |
| `server/include/subathon/server/sqlite_application_bundle.hpp` | `8e24f0624673bf88f86405651f9aabee1026a226` |
| `server/src/sqlite_application_bundle.cpp` | `83e9a0a745bad413066ad75cf9eb82fd210b2186` |

## Actual local checks so far

- `cmake --preset sqlite-bundle-debug`: PASS, GNU16.2.1/Python3.14.8/Boost1.92.0;
  source package selection unchanged. Log local-evidence/storagetest002-api-configure.txt.
- `cmake --build --preset sqlite-bundle-debug`: fresh84 steps PASS, no compiler
  warning/error. Actual new.cpp object and new library archive built; no entrypoint
  invoked. Log local-evidence/storagetest002-api-build.txt.
- `ctest --preset sqlite-bundle-debug --show-only=json-v1`: exactly173 existing
  registered tests; no new bundle feature. Raw local-evidence/storagetest002-api-discovery.json.
- `/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml`,
  `cmake --list-presets`, project391/122/render/diff: PASS.
- Direct byte comparison: removing only declared method/subsection/SB15 restores
  prior approved logicalcf462 exactly; entire physicalf13 and original14 SB rows
  remain unchanged. Only newheader/newstub differ in existing core/server source
  and tests/fixtures scopes; old producers/headers/assertions are untouched.

This initial build used these exact working-tree bytes (root2d plus integrator
wiring); the ensuing commit freezes them unchanged before the old173 execution.
No old-suite execution, new feature call or feature red is claimed yet here.

## CI coverage and remaining gates

Prior document-only published6ca had all11 inherited/document checksSUCCESS; that
was the old build preset, not this interface build or a future newgreen. Root must
publish this compiled interface/build revision and observe its own pinned ordinary
Ubuntu/GNU13.3/Boost1.83/selectedQt6.12 job before claiming CI success. Existing
164 individual writer/work/migration/retention/receipt/contribution/plan cases and
all required fixture diagnostics remain automatic, with no manual feature guards
or weakened assertions. Foundation checks remain in their existing scaffold job.

New real bundle fixture/feature suites do not exist yet. After actual extension/
API/config review, independently author/review them, add required selectedQt
newgreen + exact compiled registration guards, and observe those on the actual
reviewed revision before FIRST intended missing-behavior red. Intentional red stays
separate from passing automatic gates; no skipped assertions/failure inversion.
STORAGE-002 implementation waits for that real baseline and its normal later
source review/acceptance/automatic CI gates. No runtime/financial/authority/job/
recovery/provider/platform/release capability follows from this minimal build.

The existing check name is preserved. No branch-protection/ruleset change is made;
written latest-head CI/review/threads/approvals/mergeability gates still apply.
All seven integrations/three FX and full release scope remain mandatory.

## Actual frozen existing-suite execution

Root froze these exact code/config bytes as
`79b69d437d12df5511f7e38fdc975eb7f0452e12`, clean tree, then ran
`ctest --preset sqlite-bundle-debug`: **173/173 PASS,4.51s**, actual exit0.
All recorded Qt totals0fail/0skip/0blacklisted; existing diagnostic logs confirm
Qt6.12.0/SQLite3.54.0 on hostGNU16.2.1. This remains old-suite stability, no new
entrypoint/feature/red. Raw LastTest copied before any future run can overwrite it.

First scratch postprocessor expected older CTest summary wording; it was corrected
to the actual CTest4.4 `100% tests passed out of 173` plus exact173 individual
Passed lines. This was read-only parsing, no test rerun, baseline change or red.

| Preserved local evidence | SHA256 |
| --- | --- |
| `local-evidence/storagetest002-api-configure.txt` | `8b0af1ea03a16d9fdea862570954585d3e70100c1fdd5458bca76cfcf71c472f` |
| `local-evidence/storagetest002-api-build.txt` | `a9d1b3e2522519c8fd19a6053c172ff4e94faecfd672aa51ded1f0c692de0fc0` |
| `local-evidence/storagetest002-api-discovery.json` | `08b7fb3222fdab5d8f84fa7c186ef01f36b7ae317cd17673ab9fa51a869d07cc` |
| `local-evidence/storagetest002-api-old173.txt` | `a0d061f246c79923f0b61fa37d4ebdaa5aba541a78546931438a78ad7ddb0c58` |
| `local-evidence/storagetest002-api-old173-LastTest.log` | `6fbc4e2dde08c383abcef879f6f2ed65b70fbb751acfba3d212808dc9da3eef9` |
