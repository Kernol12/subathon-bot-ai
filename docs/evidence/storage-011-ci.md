# STORAGE-011 CI assessment — implementation in progress

Coordinator `/root`, 2026-11-01. Task starts from merged PR31/main ef29c87,
with actual22 reviewed/red baseline, green selectedQt7 and frozen vectors/API.
Original [test-first evidence](storagetest-011-handoff.md) stays valid and is not
repeated. New production source is still being authored; no new pass is claimed.

Shared66be523 changes only acceptance descriptions/draft label and the workflow's
manual migration guard: unchanged22 now execute unconditionally on PR/main/manual,
after the mandatory selected-driver green7 and existing49 acceptance. Pinned
Qt/ICU/action sources, sqlite-writer-build name, full compiled registration guards,
fixture path and all76 registrations remain. There is no failure inversion,
continue-on-error, WILL_FAIL, source/test skip or hidden limit adjustment.

Root actionlint1.7.12/project391/current HTML/diff checks pass for that config.
Different code reviewer prepared from the actual frozen contract/API/new22/10
vectors/green7/old49; actual source/config approval waits the implementation.
Acceptance requires real unchangednew22+old49+green7/full76 locally, a different
actual code/config review, observed ordinary automatic Ubuntu migration execution
and all applicable latest-head CI/discussion/current-rule merge gates. Editing a
workflow adds no required-check enforcement rule.

Default runs use disposable synthetic databases. The framework will not prove
backup/update authority, production domain schemas/adapters, application bundle,
recovery, power-loss or live/platform release behavior. OPS02 remains a real
caller/product precondition, never a fabricated proof field. Full scope is retained.

## Local source candidate and review finding

Source9eb1fef/blobd91c7d integratede183527, evidence0ac896c/root0118fea, compiled
and passed all76 local CTest registrations (original22+old49+infrastructure5,
including green seven-case environment executable), with Qt6.12/SQLite3.54.0.
Separate actual code review then found missing valid initial-U+FEFF coverage;
QString::fromUtf8 strips that character, causing invalid_manifest rejection.
No approval, Ubuntu result or implementation completion follows from local green.
Focused separately authored/reviewed Unicode regressions and an intended red
against this source precede correction. Original reviewed baseline remains intact.

Focused two-case regression234c2ec/teste94befdc was actually approvedad4f54d
(root8c70e6e) before root observedexit8 on both bodies againstd91. Fixture/raw
guards/initcleanup passed,0skip. [Regression evidence](storage-011-unicode-regression.md),
[raw red](storage-011-unicode-red-ctest.txt). Root harness8534118 registers both;
compiled24 and total78 exactly match. Dynamic workflow discovery guards include
the new cases automatically. Source correction/different review/Ubuntu pending.
