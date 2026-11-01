# STORAGETEST-012 — evolving CI assessment

Coordinator `/root`, 2026-11-01. [Draft PR33](https://github.com/Kernol12/subathon-bot-ai/pull/33)
starts from main29f3818 (merged STORAGE-011). Initial contract-stage head
791cacf09cc018fb78b1c36736fc8f03f68be996 had all11 checks SUCCESS and was
DRAFT/MERGEABLE/CLEAN. Those are existing executable checks only; no new retention
API, schema implementation, feature test, selected fixture gate or red was run.
The existing73 feature cases, green7, source/interfaces/build/workflows remain
unchanged at that head. All ten postmerge main workflows were also observed pass.

Actual contract/root migration/brief reconciliation approval40d0dc6 is integrated
eddbe2b; it does not approve absent interfaces/tests or establish passing retention.
Host [binding/DDL probes](storagetest-012-probe.md) distinguish empty BLOB/null/
binary bytes and proposed exact schema/index shape using existing capabilities.
They are not selected bundled-driver or new feature results.

## Required next coverage

Root will extend the existing pinned Qt SQL lane when the minimal interface/stubs
and actual cases exist. Preserve automatic old25/work24/migration24 and mandatory
green7, exact compiled-registration guards, selected Qt/ICU/action pins and limited
permissions. Add explicit new baseline configure/build/discovery and a small green
fixture diagnostic for the new literal DDL/generated indexes and binary/empty/null
binding. Observe that fixture using selected official Qt before feature red.

New retention bodies are intentionally unimplemented until actual separate test
review and a compiling intended red; keep their real failure visible and separate
from ordinary passing CI. A documented optional manual acceptance run can propagate
that real failure. STORAGE-012 must promote the reviewed new suite to unconditional
automatic CI when implemented and observe real execution before completion.
No WILL_FAIL, blanket continue-on-error, hidden skips or assertion/fixture weakening.

The API precision review still needs concrete input/options/kind/positive-limit
error precedence and owned present-empty result normalization. Root owns shared
CMake/presets/workflows/schema order/state/map/HTML; author owns only new module
interface/explicit stubs and existing contract/evidence at this stage. No new Qt
cases/fixture/vector ownership is assigned until that next review.

No workflow addition configures a required-status-check rule. Current ruleset
requires a PR/resolved discussions and zero GitHub approvals, no bypass/force/delete;
written latest-head CI gates remain mandatory. New feature fixture/review/red, final
metadata and normal latest-head merge gates are unperformed. Full domain semantics,
typed numeric fields in013/014/015, app bundle, recovery/backup/update authority,
platform/package/browser/provider/live gates remain separate mandatory scope.

## Actual minimal API and draft library build

Author6bfbb90 integrateda6a4402 adds header55c828, explicitstubb27eb488 and
contractprecision5b0ee84. Three public bodies throw feature_not_implemented;
constructing the facade only copies the weak revocable session/limits. No lookup,
retention, byte conversion or manifest generation is implemented.

Root shared e5532d1 adds SUBATHON_BUILD_SQLITE_RETAINED_TESTS, the
sqlite-retained-debug configure/build/test preset (inherits all existing migration
flags), and subathon_sqlite_retained_context linked to existing dependencies.
`cmake --preset sqlite-retained-debug` and `cmake --build --preset
sqlite-retained-debug` both exit0, no warnings, actual new stub library built.
CTest JSON discovery preserves exactly78 old registrations and zero new retention
entries; no new feature body was run. Logs ignored under
local-evidence/coordinator/storagetest012-api-{configure,build}.log.

Different actual API/stub/precision/minimal-harness approval46fc8ee integrated
2d0289b includes independent targeted configure/build and exact old-prefix counts.
Original contract40d0dc6 review remains preserved. Draft PR33 head6fdf620
all11 existing checks passed; the new module has not yet compiled/run in selected
CI and no future test fixture/body is approved. New cases/environment/vectors are
now assigned for authoring, with full actual review and selected fixture/red gates
still pending. Root will extend the workflow and actual registration when those
files exist. No earlier check is transferred to a later head.
