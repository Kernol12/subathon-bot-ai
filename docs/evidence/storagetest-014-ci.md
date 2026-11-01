# STORAGETEST-014 CI coverage and observations

Coordinator `/root`,2026-11-01. Specification v0.45 §3.2–3.5/DB-01.
This task establishes a reviewed intentional-red baseline; STORAGE-014 remains a
separate implementation task. Existing source/test/review/red evidence is preserved.

## Actual coverage before the new baseline

Draft [PR37](https://github.com/Kernol12/subathon-bot-ai/pull/37) was published at
2b14939551c5607fe1b39efe9339ea0b6517e1f3. All11 automatic checks passed. The API/
precision revision2530654e8363d63fd26b03f0c452c08d30b10a84 was subsequently pushed;
all11 automatic checks also passed when actually observed. At both heads the SQL
workflow still selected sqlite-receipt-debug: these results cover inherited stable
behavior, not the new contribution source build, fixtures or feature body.

Actual local minimal API/root build passed62 Ninja steps/0warnings and all118
inherited tests,2.56s. Different API/stub/minimal-config review77a9544 accepted
exact header0abeea2/stub0ad1315/logicalf503/physicaleeb and shared configuration.
[Build scope](storagetest-014-probe.md), [actual reviews](storagetest-014-review.md).

## New workflow candidate and remaining proof

Root's actual shared candidate extends the same sqlite-writer-build job using
sqlite-contribution-debug, inherited from the passing receipt/retention/migration
profiles. It builds the explicit new stubs, all stable SQL components, the new
feature test executable and separate literal-fixture diagnostic. The diagnostic
links only existing writer/Qt; feature setup cannot obtain expectations from new
production codec/factory/repository behavior.

All old stable25 writer/ownership,24 work,24 migration,16 retention and22 receipt
cases remain unconditional. Existing environment probes remain unconditional.
The new three-slot diagnostic and exact compiled-versus-CTest feature discovery
are automatic. New feature execution remains an explicit workflow_dispatch boolean
`run_contribution_feature_tests`, defaultfalse, while those stubs are intentionally
red. No WILL_FAIL, blanket continue-on-error or assertion removal hides a result.
The first intended red must be observed separately only AFTER actual complete
baseline review, successful compilation and selected official Qt green proof.
The optional feature control has not been invoked.

Selected archives/actions, Ubuntu24.04, hashes, permissions, timeout and check name
remain unchanged. Actual command coverage will be inspected in the new PR workflow
before merging; source14 must later promote passing feature cases to unconditional
ordinary CI. A host Qt6.12/SQLite3.54 helper result is not selected official bundled
Qt6.12/SQLite3.53.4 evidence.

Root actionlint1.7.12 passes the workflow candidate. Complete actual test/fixture/
harness review and the first new selected-runtime workflow result remain pending
at this checkpoint. No new contribution feature body or red has run. Final evidence
must append exact reviewed hashes/head/run/log observations rather than infer them.

The last observed active ruleset26170263 requires PR/resolved discussions,0 GitHub
approvals, no bypass/force/delete; no required-status-check rule. Written latest-
head all-CI and independent-review merge gates remain mandatory. This document
claims no stronger server enforcement. Recheck actual rules and discussions before
merge. All platform/live/provider/UI/application bundle/recovery/packaging release
gates remain separate; no credential-dependent or live operation is authorized.

## Actual complete host harness before full baseline review

Actual feature d438e2e/roota8eaf15, sourceaf2dc41; literal JSON44df and corrected
green577 have preliminary independent27aa55f/rootdb633fb approval. Root actual
configure/build/link passes,0warnings. Exact143 CTest registrations include24
feature slots matching the compiled -functions list; required green exactly3.
No feature method was invoked. Actually ran:

```sh
cmake --preset sqlite-contribution-debug
cmake --build --preset sqlite-contribution-debug
ctest --preset sqlite-contribution-debug --show-only=json-v1
ctest --preset sqlite-contribution-debug -L '^sqlite-contribution$' --show-only=json-v1
build/sqlite-contribution-debug/tests/qt/server_sqlite_contributions_acceptance -functions
build/sqlite-contribution-debug/tests/qt/server_sqlite_contributions_environment -functions
ctest --preset sqlite-contribution-debug -LE '^sqlite-contribution$' --output-on-failure --parallel 1
```

Stable/green CTest exit0,119/119 PASS2.59s. Actual new green3 plus init/cleanup
5PASS/0skip; Qt6.12.0/SQLite3.54.0. Ignored raw log:
local-evidence/coordinator/storagetest014-full-harness-green.txt. Formatting and
actionlint pass. Full different actual baseline/harness review and selected NEW
official Qt fixture proof remain future; new feature red has not run.

Latest owner instruction: finish sole open PR37, then STOP. No next task or
STORAGE-014 implementation is authorized after completion of this PR.

Full actual review at e03 requested three additive feature coverage fixes: typed
original integer extrema/reopen, active adopted wrapper destruction before owner
finalization and typed modifier-context/known-config resolution. Actual12ba6df
integrated3f85f3b, final feature46c901e0225b7755153decf25f4a5ff226d063db preserves
all24 slots/old assertions. Root rebuilt/linked0warnings and rechecked exact143/24
compiled discovery. Green/JSON/API/stub/physical/shared harness unchanged. No new
feature body/red has run; actual full re-review is pending at this point.

Actual full authored-baseline/harness approval81e0d5a integrated1b4c48c accepts
final46c/577/44df/header0abee/stub0ad/logicalf503/physicaleeb, CMakef308ecb and
workflow48ece7b. All three findings resolved; reviewer exact143/24+green3 and
safe119 stable/green PASS2.62s/green5PASS0skip. Final46c incremental build/list
passes0warnings; stable inputs unchanged. No new feature/init/red was invoked by
reviewers/authors. Publish this approved candidate next for actual selected
official Qt NEW577-green proof before the first intended red.

## Actual selected official Qt prerequisite and first red

Actual automatic [run41347563063](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41347563063)
at exact approved head399fc442b960af943191b1ab35ca5a53e94df05d completed SUCCESS.
Full approval81e0d5a/root1b4c48c preceded publication of that head. Read/cached the
actual job JSON and full log under ignored local-evidence/coordinator/
storagetest014-fixture-ci-399fc44.{json,log}; no result is inferred from a label.

Both pinned archive hashes report OK. Actual project compiler is GNU13.3.0;
Qt6.12.0/bundled SQLite3.53.4. The library's own GCC build banner describes Qt,
not the compiler used for this project. Full70-step build compiles the explicit
new stub and feature/green targets. Old25 writer/ownership,24 work,24 migration,
16 retention and22 receipt cases all pass unconditionally; old environment7/3/3
also pass. Required NEW577 green reports exact3 slots plus init/cleanup5PASS,
0failed/0skipped; exact24 compiled feature discovery passes without execution.
The explicit manual feature step is SKIPPED as designed (input not invoked).
All11 PR checks at399fc44 are completed SUCCESS.

Only AFTER inspecting that proof, root verified clean HEAD399fc44, full-review
ancestry, all exact reviewed hashes and not_run baseline status. FIRST actual new
feature run began2026-11-01T20:37:06.321566Z at that SAME head:

```sh
ctest --preset sqlite-contribution-debug -L '^sqlite-contribution$' --output-on-failure --parallel 1
```

Actual exit8; all24 fail explicitly on SqliteContributionsNotImplemented or its
intentionally-unimplemented message. All24 init and24 cleanup phases PASS; every
process totals2PASS/1FAIL/0skip, no crash/setup/dependency failure. Total0.33s,
host GNU16.2.1/Qt6.12.0/SQLite3.54.0. [Original raw red](storagetest-014-red-ctest.txt).
Ignored exact timestamp/head/command/hash/count metadata:
local-evidence/coordinator/storagetest014-red-metadata.json. This is observed red,
not a passing product suite or implementation. Do NOT repeat it. Final metadata/
CI-attribution review and latest-head PR checks still precede normal merge. After
PR37/main reconciliation, STOP under the latest owner instruction.
