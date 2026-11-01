# STORAGETEST-001 CI assessment

Coordinator `/root`, 2026-11-01. Independent investigator/workflow reviewer
`/root/qt_ci_investigator`; no source/test approval is implied by this review.

The new optional `sqlite-writer-debug` lane requires Qt >=6.12 Core/Sql/Test.
The baseline remains explicit missing-behavior stubs until STORAGE-001; its actual
acceptance run is intentionally red after separate baseline approval. Ordinary
GitHub checks build/discover it and run the Qt event-loop probe, without declaring
writer behavior passing. Manual dispatch `run_acceptance=true` runs real cases and
propagates failure normally. STORAGE-001 must promote the unchanged reviewed cases
to ordinary automatic acceptance before implementation merge.

The workflow uses official Qt6.12.0 archives with fixed SHA-256, bsdtar extraction,
a dedicated prefix and no installer action/aqt, local automatic download, downgrade
or committed binaries. Exact selected URLs/hashes are in
[the workflow](../../.github/workflows/sqlite-writer.yml); primary metadata:
[Qtbase](https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_6120/qt6_6120/qt.qt6.6120.linux_gcc_64/6.12.0-0-202609280346qtbase-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z.mirrorlist),
[ICU](https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_6120/qt6_6120/qt.qt6.6120.linux_gcc_64/6.12.0-0-202609280346icu-linux-Rhel8.6-x86_64.7z.mirrorlist).

The investigator actually downloaded/verified/extracted both archives in disposable
local storage. An independent temporary CMake consumer found Core/Sql/Test, built
and executed QSQLITE `:memory:` SELECT42 with no LD_LIBRARY_PATH or relocation
patch. Qtpaths reported6.12.0 and the extracted prefix. This is Arch-host evidence;
Ubuntu compatibility was initially only an inference from binary requirements.

The first workflow review requested a nonempty discovery check: CTest show-only
can exit0 with no tests. The exact final workflow blob
`9144d8b51056ae25172d7f03f346a726604087af` was separately approved after adding
CTest JSON comparison with all compiled Qt `-functions`, including count equality.
The reviewer ran YAML parse, bash syntax, Python compilation and the exact guard
against Qt6.12 probe output; valid registration passed and empty/mismatched/duplicate
registrations rejected. This is build/registration verification, no failure inversion.

Automatic coverage is Qt compilation/linking, version/plugin-file verification,
event loop and complete case registration. It does not open QSQLITE or run writer
behavior until implementation enables acceptance. Local reviewed/red cases exercise
actual SQLite. Windows, product transport, migrations/recovery, live providers and
release packaging remain separate gates. Runtime apt packages/runner tools are
recorded from logs; this is not a pinned OS image. Qt/ICU distribution obligations
remain in THIRD_PARTY_NOTICES.md and release packaging tasks.

Actual PR Ubuntu workflow execution is pending. It must be observed before baseline
merge. Existing eight stable automatic lanes remain required written gates.
Ruleset26170263 currently requires PR/resolved discussions, zero GitHub approvals,
no bypass and no required-status-check rule; workflow addition does not configure
GitHub enforcement. Recheck actual latest-head CI/rules at merge.

## Actual workflow validation correction

The first actual GitHub attempt at653c62f,
[run41288086752](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41288086752),
failed before creating jobs. The initial syntax review missed GitHub's context
restriction: job-level env does not allow runner. Root moved prefix setup to a
prior run step writing GITHUB_ENV. The independent reviewer acknowledged the gap,
reproduced the original validation error with official actionlint1.7.12, and ran
full actionlint on the corrected workflow: exit0, no findings. The corrected blob
`398733ebeb7bd2c83d4396def554979f01ef4fd1` is independently approved. Archive pins,
complete registration guard and true-failure manual acceptance are unchanged.
[GitHub context availability](https://docs.github.com/en/actions/reference/workflows-and-actions/contexts#context-availability)
supports the diagnosis. Actual corrected Ubuntu execution remains required.

## Observed corrected Ubuntu build

[Run41288299328](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41288299328)
atPR24headda98505 succeeded. Actual retained logs show both archive SHA256 checks
OK, GNU13.3.0, compiled real Qt targets, environment probe1/1pass, and exact
registration guard: `Discovered all 22 compiled writer cases; behavior not run`.
Root and separate integration reviewer inspected actual job results/logs. All10PR
checks passed atda98505. This supplies actual Ubuntu build/registration evidence;
it does not turn red writer behavior into a passing feature. Latest-head CI remains
a separate merge gate.
