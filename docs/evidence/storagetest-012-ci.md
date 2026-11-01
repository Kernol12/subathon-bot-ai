# STORAGETEST-012 — CI coverage and observed results

Coordinator `/root`, 2026-11-01. [Draft PR33](https://github.com/Kernol12/subathon-bot-ai/pull/33)
from main29f3818/merged STORAGE-011. Contract40d0dc6 and API/stub/precision/
minimalbuild46fc8ee approvals are preserved in [review](storagetest-012-review.md).
New test fullreview and intended red are still separate gates. No retention
implementation or feature pass is claimed from compiling explicit stubs.

## Actual current artifacts and coverage

Author7d89c4c/root320ae61 supplies16 feature cases, green3 existing-capability
fixture cases and independent JSON0f76fd2c. Negative fixture1eda545/root21fddd6
explicitly disables FK only on the independent corrupt-data setup connection.
Product writer enforcement remains ON. Actual different source review identified
two oracle gaps, separately strengthened by e76/180 and648/ab4: post-retain stage
proof before later failures, binary context/link byte preservation and independent
positive bounds. Helper70/306 consumes qWaitFor's result and records timeout
without faking completion. Current featuretestc0e54003, environmentaaa16447;
API55c828/stubb27eb488/contract5b0ee84 and all old implementation/tests are unchanged.

Shared e5532d1 adds the sqlite-retained-debug draft preset/library. Workflow5e9a266
uses that preset to build the reviewed stubs while running every existing suite.
Shared27fea1d registers16 isolated draft acceptance cases plus a separate green
fixture executable (3 slots), supplies the frozen vector macro to both targets,
and extends CI with mandatory exact3 fixture execution and exact compiled16
discovery. Green fixture target links only the existing writer, never new adapter
or factory; no new UUT call supplies its expected rows or hashes.

All old25/work24/migration24 and mandatorygreen7 remain unconditional. New
unimplemented bodies are separate and visible: optional workflow_dispatch boolean
run_retained_acceptance executes their real CTest result without inversion or
continue-on-error. Default passing CI does not claim new retention acceptance.
STORAGE-012 must promote reviewed16 to unconditional execution after implementation.
No assertion/fixture weakening, WILL_FAIL, zero-test success or changed dependency
pins/permissions/check names. Root owns shared schema/migration order/build/state.

## Actual local verification before any new feature body

Linux x86-64, GNU16.2.1, Qt6.12.0/hostSQLite3.54.0.

| Command/check | Actual result |
| --- | --- |
| cmake --preset sqlite-retained-debug | Exit0 |
| cmake --build --preset sqlite-retained-debug | Exit0; final helper build zero warnings |
| Compiled -functions vs CTest JSON | Exact16 feature/3 green slots;95 total registrations |
| Green fixture executable only, verbose | 3 cases plus init/cleanup pass,0skip |
| Old suite excluding new16 and already-run new fixture | 78/78 pass; no new feature body |
| Independent F framing/unsigned sort/hashlib | Complete2000-byte payload and SHA0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e match |
| Independent opaque vectors | All12 exact uint64 spelling/binary/BOM/NUL forms verified |
| Formatting/actionlint1.7.12/project391/currentHTML/render/diff | Pass |

Ignored logs under local-evidence/coordinator/storagetest012-*: tests-configure,
tests-build, tests-final-build, environment-local and existing-local. Initial build
had the ignored-nodiscard warning, fixed explicitly before final zero-warning build.
Root's first vector-check script incorrectly assumed a steps array; correcting it
to the actual flat fixture manifest independently matched full bytes. Those setup/
script errors are not feature red. Full95 has NOT been run while new16 await review.

## Actual selected-Qt API compilation, new fixture still pending

[Run41315776368](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41315776368)
at head5e9a266b0662197d5a06927489835d090cbf19d9 completed successfully. Actual
metadata/logs show officialQt/ICU hashes OK, GNU13.3, explicit new stub source
compiled and static library linked, Qt6.12 with bundledSQLite3.53.4, old25+24+24
pass and existinggreen7 (9 including init/cleanup,0skip) pass. This predates the
new feature files/fixture/harness and does not establish their selected support.
Ignored log: local-evidence/coordinator/storagetest012-api-ci-5e9a266.log.
Earlier contract-stage791cacf and API metadata6fdf620 all11 checks passed; none
is transferred to later heads.

Next observe actual automatic selected officialQt execution of the new green3
and discovery/build of new16 at current frozen artifacts. A different reviewer
must approve the actual complete tests/interface/fixture/harness. Only after BOTH
gates may root run new16 for genuine intended missing-behavior red. No dependency,
fixture, compile, skipped or zero-test failure counts. Final metadata/CI attribution,
latest-head checks/discussions/current rules/normal merge follow baseline completion.

Ruleset26170263 requires PR/resolved threads, zero GitHubapprovals, no bypass/force/
delete; no required-status-check rule. Written all applicable latest-head CI still
applies. Synthetic/disposable state only. Domain semantics, typed numeric fields
in013/014/015, app bundle, recovery/backup/update authority, browser/platform/
packaging/provider/live verification remain mandatory separate scope.

Actual fulltest/interface/fixture/harness approval672f93c is integrated beforeany
newbody. It approves exact27fea/c0/AAA/0f/55/b27/5b with independent95/16/3/green3
checks, preserving required selectedofficialQtfixture success beforefirst16red.
No baseline completion, feature pass or implementation is claimed yet.
