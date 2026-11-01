# STORAGE-001 ordinary Qt SQL acceptance CI

Coordinator `/root`, 2026-11-01. Specification v0.45 §2/DB-01; internal writer
revision1. This task promotes the exact22 reviewed cases from build/discovery
baseline CI into ordinary PR/main acceptance. The `sqlite-writer-build` check
name stays stable. Every PR/main/manual run executes real CTest writer cases,
with no input guard, failure inversion, skip or continue-on-error.

Official Qtbase6.12.0 and ICU archive URLs/SHA256, bsdtar extraction, private prefix,
Qt environment probe and compiled-function/CTest registration equality guard are
unchanged from the separately reviewed/actually observed baseline setup:
[baseline CI evidence](storagetest-001-ci.md). No new dependency/action is added.
Root actionlint1.7.12 on the actual promoted workflow exited0. Automatic cases use
real disposable files/QSQLITE; no provider data, sockets or production database.

Root and author configure/build succeeded; all22 unchanged cases and fullpreset25
pass on Linux/GNU16.2.1/Qt6.12.0. The original reviewed stub-red remains retained.
Independent code/config review and actual promoted Ubuntu acceptance run are
pending. Before merge, inspect actual latest-head checks, not only local results.

This is bounded library/file SQL evidence, not crash/power-loss durability,
application transaction/repository wiring, recovery/backup, actual deployments,
Windows/OBS or provider validation. All later release gates remain recorded.
Ruleset26170263 requires PR/resolved threads with zero GitHub approvals and no
required-status-check rule; stable check names are written gates, not a new claim
that workflow addition changed GitHub enforcement. Recheck actual rules at merge.

## Observed ordinary Ubuntu acceptance

[Run41289246502](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41289246502)
atPR25headed5da87 succeeded. Root retained/inspected actual logs: both archive
SHA256 checksOK, GNU13.3.0, Qt environment probe1/1, registration guard22, followed
by the unconditional `Run reviewed writer acceptance` step with22/22 passing real
file cases. All10PR checks passed at that revision. Code/config reviewd6f9c49
independently approves sourcea8e5c8e and promoted workflow504240ef.

This resolves the baseline build-only CI gap for this bounded writer library;
no full server/application/recovery/platform support follows from it. Workflow
source has no behavior conditional, hidden failure or modified acceptance assertions.
Final metadata review and all latest-head CI remain publication/merge gates.


## Corrected-source25-case automatic acceptance

The premerge raw-buffer finding held sourcea8ownership approval, leading to3
separately authored/reviewed regressions34c25c27, actual intended red1499d57, then
normalization fixbe533bd/source829d9c66. Fresh source/configapprovalc029a53 resolves
the affected hold for that corrected source, preserving original22 and new3 tests.
The CI registration guard now requires both executables and exact per-binary
compiled-function/CTest equality (25total), with unconditional real acceptance.

[Run41290532883](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41290532883)
atPR25head883d0ab succeeded. Root inspected actual retained logs: Qt/ICU hashesOK,
GNU13.3.0, Qtprobe1pass, exact2binary25registration and unconditional25/25 real
writer cases passed. All10PR checks passed there. This new evidence, rather than
the earlier22-only run, covers correctedsource829 and newgroupedCIguardd99df8ad.
Final completionmetadatareview and all latest-head normal gates still apply.
