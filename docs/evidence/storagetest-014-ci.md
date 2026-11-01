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
