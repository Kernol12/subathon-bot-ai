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

## Actual official minimal-interface workflow

[41523469879](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41523469879)
actually SUCCESS at exact published
`848e76d7b4269391ff457e572cf9cafbe78cf313`,2026-11-02T20:54:29Z–20:56:03Z.
All11 PR checks SUCCESS. Actual log verifies new sqlite-bundle-debug configure,
GNU13.3.0/pinned Boost1.83 compiler macro108300, selected checked Qt6.12 archives,
fresh84-step build including new.cpp/newarchive, zero compiler warnings, existing
164 writer/work/migration/retention/receipt/contribution/plan cases and required
7+3+3+3+4 fixture methods/Qtprobe. Existing mandatory diagnostics all0skips and
report actual SQLite3.53.4. No new bundle fixture/feature exists or ran.

Raw local-evidence/storagetest002-api-official-ci.txt SHA256
`1819b577ef7e431817ad0104a481fab7ec767403d2328d6806f9a32290308b6a`.
This is observed same-source official interface compilation/old-suite CI, not
new66-object fixture/newgreen or bundle acceptance.

Actual different extension/API/config approvalef0ba764c6f0b24f534c373d6e28ef5c696a1f71
(root e033177/reviewMD49876fc2) accepts exactheader8e24/source83e9/logical2f6/
physicalf13/all5configurationpins. Own fresh84build0warnings/old173PASS4.48s/0skip.
CI and independent review were parallel and correctly pinned to848 source, not a
future artifact. Header/source/physical remain frozen for independent fixtures.
Latest-head merge CI remains a separate future gate.

## Actual independent fixture/helper host diagnostic (review pending)

Author7375b327/root603091b added ONLY independent JSONe8b6c77e (SHA aebff706),
initial644-line oldwriter-helperd9d441b5 and own evidence. Root first configure/
incremental18build/5compiledslots/174registrations PASS0warnings, then hosthelper
FAILED0.45s/6PASS1FAIL0skip at trigger installation BEFOREapplicationunit.
The real writer's reviewed prohibition of ANYsemicolon rejected compound trigger
DDL; no new facade/feature/missing-behavior red was invoked. Raw failure preserved.

Narrow authorfix5b161be6/roots73b0167 changed only helper+ownnote:667linehelper
`b0a5c72f152f193370d6c39af526fba61d332fdb`, same JSONaeb/e8b and predicates.
Test-only triggerDDL uses the separate owned observer while the controlledwriter
is idle, with real lock/catalog visibility checks on BOTH connections. Production
SQL filter, data, schema, all rollback/staged/observer/faultoff assertions unchanged.
Written actual reason and independent final-review requirement are in authornote.

Root corrected `cmake --build --preset sqlite-bundle-debug`: incremental3 steps
PASS/0warnings. `ctest --preset sqlite-bundle-debug -R
'^scaffold.sqlite_application_bundle_environment$' --verbose`: **7PASS0FAIL0SKIP**,
443ms methods/0.45s CTest, Qt6.12/SQLite3.54. Five diagnostics plus init/cleanup;
actual intermediate pending0→staged1, prepared effects/state with oldhead/outside
pending0, realSQLabort/insidecatchBEGIN/fulltypedrollback and faultoffcommit proved
ONLY for oldwriter/rawliteral foundation, never futurefacade reach/product behavior.

174 registrations are old173+one newdriver executable; NO bundle feature case.
Old definitions are byte-identical after removing exactly the new15-line CMake
block, so old173 hostproof remains valid without another unrelated full replay.
Source/header/physical allfrozen; helper links only oldwriter+QtTest. Workflow
appends exactly18 lines: unconditional required5-slot guard/run. All old commands/
pins/permissions/checknames otherwise unchanged. Actionlint/project/render/diffPASS.

These tests used the exact listed helper/JSON and integrator harness working-tree
bytes; this review candidate freezes those unchanged. Actual full fixture/helper/
harness review and selected official NEW-green remain pending before featuretest
authoring/firstred. No new entrypoint or previously completed red ran.

| Preserved local diagnostic | SHA256 |
| --- | --- |
| `local-evidence/storagetest002-fixture-build.txt` | `e3fdfd04b45c0b9ae5c96d5d954b655f92d2bc5ae55476555bf6252337a78cfc` |
| `local-evidence/storagetest002-fixture-green-initial.txt` | `a1dba3dab2e923a6edb65d4bbce2cd4eb53cd4b5818db4495502f2d252592148` |
| `local-evidence/storagetest002-fixture-build-corrected.txt` | `b500a464aa63fa487501cc6764b98f9d36f4a6436e0547e462a576f1731a1af6` |
| `local-evidence/storagetest002-fixture-discovery.json` | `b294482ff4f0f8b9300a12579f3e73449ac5fc21b5bedcc8b2f60e412207b4a3` |
| `local-evidence/storagetest002-fixture-green-corrected.txt` | `43a5a5fd0130cf1dad4c257ab9120214889450436394f9a5146f5a4420cce304` |

| New actual harness | Git blob |
| --- | --- |
| `tests/qt/CMakeLists.txt` | `203db2f439add44c6fe20b6ce5152134aa61f572` |
| `.github/workflows/sqlite-writer.yml` | `b9e1a49583e8e54800997215ba4e65ec58ee2b8c` |
