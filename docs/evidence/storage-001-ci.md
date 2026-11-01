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
