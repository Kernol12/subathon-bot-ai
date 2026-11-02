# STORAGE-014 — contribution evidence SQLite implementation

Coordinator `/root`,2026-11-02, task/STORAGE-014-contribution-repository from
actual integrated main8859b644e8d99b34b3e899d0e44e1e2cce386e78. Current owner request
resumes the standing loop; the earlier stop was honored and is superseded.
Specification v0.45 §§3.2–3.5/DB-01. Implementation component is accepted/done; [PR38](https://github.com/Kernol12/subathon-bot-ai/pull/38)
awaits final metadata/latest-head merge gates. Full release remains incomplete.

## Scope and preserved baseline

Implement unchanged contribution ports on the existing controlled Qt SQL session.
The service remains the policy owner; storage preserves independent current state,
immutable support/observation provenance, compatible associations and exact values.
ContributionId0..MAX includes allocated/present0, distinct from SQLNULL absence.
ReceiptId remains1..MAX. Receipt original-only hydration validates real retained
configuration/context/known-config targets without imposing historical cfg equality.
Prior committed/nonconflicting receipt classification and semantic/provider authority
remain upstream prerequisites; same-session visibility cannot prove them.

Actual source5acd3d6 integrateda7659b5, blob61569bd13e34f515802271da1c2ea9c89b23c473,
889lines. Pure manifest3 appends the approved fourSQL/20-object projection while
keeping exact prefixes1/2; full18126-byte payload/SHA b72249ba...d1d matches the
independent literal oracle. Private layered observation/contribution/anchor readers
avoid recursive hydration. Owner/single-use/explicit adoption, allocation-safe
pending tracking, actual active RAII rollback/commit and truthful error/uncertainty
boundaries follow the approved contract; no raw writer handle or extra connection.

[Original baseline](storagetest-014-handoff.md) had actual full81e0d5a and final6ee
reviews, selected officialQt41347563063 and then one real24 explicit-stub red,
all init/cleanup PASS/0skip. It is integrated by PR37. The unchanged original red
was inspected, not rerun. Frozen feature46c/green577/JSON44df/API0abee/logicalf503/
physicaleeb, all earlier producer sources and every assertion remain unchanged.

## Actual commands and pending gates

Author fresh configure/build70 steps/0warnings,24PASS0.76s/full143PASS3.25s.
Root separately configure/build/link0warnings, targeted24PASS0.74s/full143PASS3.17s.
Host GCC16.2.1/Qt6.12.0/SQLite3.54.0/CMake4.4.5. Actual commands:

```sh
cmake --preset sqlite-contribution-debug
cmake --build --preset sqlite-contribution-debug
ctest --preset sqlite-contribution-debug -L '^sqlite-contribution$' --output-on-failure --parallel 1
ctest --preset sqlite-contribution-debug --output-on-failure --parallel 1
```

Formatting/actionlint/project391/currentHTML/tooling19/DOM simulation pass; browser/
OBS remain separate. [Author evidence](storage-014-implementation.md),
[CI assessment](storage-014-ci.md). Different actual code/config approvala751d54 integrated5ed4194 accepts source615
and exact shared configuration; fresh reviewer24PASS0.73s/full143PASS3.18s.
[Actual code review](storage-014-code-review.md). Actual automatic selected official Qt run41477399818 atd0ce5c8 passes pins/GNU13.3/
Qt6.12SQLite3.53.4/70build/old111+requiredgreen/exact24/UNCONDITIONAL24. All11 checks
pass for that source head. Final metadata/CI review/latest-head checks/current rules/
resolved discussions and normal exact-head merge remain. Root promotion removes the manual
feature control and makes24 unconditional while preserving old111/green/discovery/
pins/permissions/check name. No test weakening or failure inversion.

Changed paths: new adapter CPP/evidence; root CMake option/preset/test-label metadata,
SQL workflow and progress/map/HTML/checkpoint integration. No API/old source/test/
fixture changes. Actual source review and CI are complete; final evidence and latest-head merge
gates remain before integration. Afterwards
refresh main and select the next dependency-ready task under the standing loop.

All seven integrations/three FX providers, actual plan/rate provenance, application
bundles/effects/jobs, semantic configuration/evaluation/permission, backup/update/
recovery/power-loss, both renderers/UI/locales/themes/packaging/platform/live/OBS
remain incomplete release gates. No live credential/supporter/chat/payment action.
