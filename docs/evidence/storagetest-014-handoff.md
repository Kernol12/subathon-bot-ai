# STORAGETEST-014 — contribution SQL baseline handoff

Coordinator `/root`,2026-11-01, branch
`task/STORAGETEST-014-contribution-repository`,
[PR37](https://github.com/Kernol12/subathon-bot-ai/pull/37), from actual main
e331714e0a03aae0a7c908721f92ea19a1756f62. Latest owner instruction is to finish
the open PRs, then STOP; this is the only open PR. No next task/implementation is
started. Full product remains incomplete.

## Scope and actual authored revisions

Specification v0.45 §§3.2–3.5/DB-01;
[logical/API contract](../contracts/sqlite-contributions-v1.md),
[physical migration3 proposal](../contracts/sqlite-contributions-schema-v1.md).
The existing contribution service owns association and evidence decisions. The
new SQL adapter will preserve immutable original/observation provenance and
independent state on one controlled writer. It does not produce effects/jobs or
invent provider/authorization/semantic-reference proof.

ContributionId supports0..UINT64_MAX, first allocation0; SQLNULL alone means absent.
ReceiptId remains1..MAX. The receipt projection and typed retained arrival targets
must resolve; prior commitment/nonconflicting classification remains a trusted
upstream requirement. Actual rate/source/time history remains STORAGE-015.

| Artifact | Actual Git blob |
| --- | --- |
| Logical/API revision1 | f503f385bbd4fd4ce4b43dd3243911dc3bce8f40 |
| Exact migration3 proposal | eeb2fe04be49b7eeced0db6cf9eea29520dffc64 |
| Public minimal API | 0abeea2bb6d1e99faad7d6585400ad3d80e2eae8 |
| Four named immediate stubs | 0ad1315601218601289923fe7b02eebfb8573aaa |
| 24 feature cases | 46c901e0225b7755153decf25f4a5ff226d063db |
| Independent JSON literals | 44df9918cc8494dc209e3b1812784a080860e06c |
| Independent green consumer | 577edeb0517e02e79263f5d439dedf5bc12306b1 |

Migration3 proposes fourSQL/20 total objects, preserving immutable prefixes1/2,
canonical payload18126 bytes/SHA256
b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d.
No actual new codec/manifest/repository implementation is supplied: four entrypoints
throw SqliteContributionsNotImplemented. Test-author completion will not complete
STORAGE-014.

## Completed baseline gates and remaining merge gates

Actual different contractf76817a/root0a2661d and API/stub/config77a9544/root29443f5
preceded test authorization. Corrected JSON/green27aa55f/rootdb633fb and FULL actual
baseline/harness81e0d5a/root1b4c48c approve the exact artifacts above, including
CMakef308ecb/workflow48ece7b and all three coverage corrections. Every original
assertion/slot and both previous migration prefixes remain unchanged.
[Actual reviews](storagetest-014-review.md), [author](storagetest-014-author.md),
[independent fixtures](storagetest-014-fixtures.md).

Root build/link/discovery succeeds without warnings:143 registrations, exact24
compiled feature methods and3 green slots. Local stable/green119 PASS2.59s;
different reviewer119 PASS2.62s. New green5PASS/0skip uses hostQt6.12/SQLite3.54.
Format/actionlint/project391/tooling19/render/DOM checks pass; DOM simulation is
not actual browser/OBS evidence. [Build scope](storagetest-014-probe.md).

Full approval preceded actual automatic selected official Qt
[run41347563063](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41347563063)
at399fc442b960af943191b1ab35ca5a53e94df05d: both pinned hashes/GNU13.3/Qt6.12/
SQLite3.53.4/70-step build/old111 unconditional/current577-green3 (5PASS0skip)/
exact24 discovery PASS. Manual feature step was not invoked; all11 checks SUCCESS.
Only then root's FIRST feature execution at that same clean399fc44 began
2026-11-01T20:37:06.321566Z:

```sh
ctest --preset sqlite-contribution-debug -L '^sqlite-contribution$' --output-on-failure --parallel 1
```

Actual exit8/all24 explicit unimplemented failures,24init+24cleanup PASS,0skip/
crash/setup/dependency failures,total0.33s. [Original raw red](storagetest-014-red-ctest.txt),
[exact CI/red chronology](storagetest-014-ci.md). DO NOT repeat this completed red.
This finishes STORAGETEST-014 test authoring, not STORAGE-014 implementation.
The feature API remains the four reviewed stubs; product acceptance is observed_red.

Remaining PR37 merge gates: separate final metadata/CI-attribution review; latest-
head all checks; current rules/required GitHub approvals/resolved discussions and
GitHub mergeability; normal exact-head merge. The candidate workflow preserves
old111 cases unconditionally, required new green/discovery automatically and
explicit separate feature red until implementation. No failure inversion/skip/
assertion weakening obtains green CI. Publication/merge status must be inspected
on PR37; baseline done alone does not mean integrated.

## Completion boundary

Root owns CMake/presets/workflow/map/state/HTML/schema ordering/checkpoint. Separate
author/fixture/review worktrees are preserved, not pruned. All seven integrations,
three FX providers, application bundles/effects/jobs, semantic permissions/config,
backup/update/recovery/power-loss, both renderers/UI/locales/themes/packaging and
platform/live/OBS verification remain required. No owner decision/access is pending.
After PR37 merges, reconcile actual GitHub/main status and STOP; STORAGE-014 stays
planned for a future explicit continuation.
