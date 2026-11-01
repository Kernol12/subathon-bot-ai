# STORAGETEST-014 — contribution SQL baseline handoff

Coordinator `/root`,2026-11-01, branch
`task/STORAGETEST-014-contribution-repository`,
[draft PR37](https://github.com/Kernol12/subathon-bot-ai/pull/37), from actual main
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

## Reviews, commands and remaining gates

Different actual contract approvalf76817a/root0a2661d and API/stub/minimal-config
approval77a9544/root29443f5 preceded test authorization. Different actual JSON/
green-only approval27aa55f/rootdb633fb accepts corrected577/44df, including isolated
strict-ID-type/length INSERTs. Full review then requested three additive feature
coverage corrections, author12ba6df/root3f85f3b: new private original-reader integer
extrema/reopen, active adopted wrapper destructor cleanup before outer finalization,
and typed context/known-configuration resolution including allowed unequal cfg.
All24 names and every prior assertion remain. Actual full re-review81e0d5a integrated1b4c48c accepts final46c and the exact
fixtures/shared harness, resolves all3 findings and approves the authored baseline.
Independent build/discovery passes; safe119 stable/green tests pass2.62s. [Review evidence](storagetest-014-review.md).

Actual root configure/build/link succeeds without warnings; exact143 CTest
registrations include24 feature slots matching compiled -functions and3 green
slots. `ctest --preset sqlite-contribution-debug -LE '^sqlite-contribution$'
--output-on-failure --parallel 1` passes119/119,2.59s. New green has5 passes including
init/cleanup,0skips, host Qt6.12/SQLite3.54. Latest revised feature rebuild/discovery,
format/actionlint/project391/render checks pass. No new feature body or red has
run. [CI assessment](storagetest-014-ci.md), [probe](storagetest-014-probe.md),
[author](storagetest-014-author.md), [literal fixtures](storagetest-014-fixtures.md).

Pending before merge: selected official Qt
NEWgreen3/compiled24 discovery and stable lanes; FIRST genuine intended red after
those gates; final metadata/CI-attribution review and latest-head checks/rules/
resolved discussions; normal exact-head merge. The candidate workflow preserves
all old111 SQL cases unconditionally, adds automatic newgreen/discovery and keeps
feature execution explicit/separate until implementation. No failure inversion or
baseline weakening is used. No original historical red has been repeated.

## Completion boundary

Root owns CMake/presets/workflow/map/state/HTML/schema ordering/checkpoint. Separate
author/fixture/review worktrees are preserved, not pruned. All seven integrations,
three FX providers, application bundles/effects/jobs, semantic permissions/config,
backup/update/recovery/power-loss, both renderers/UI/locales/themes/packaging and
platform/live/OBS verification remain required. No owner decision/access is pending.
After PR37 merges, reconcile actual GitHub/main status and STOP; STORAGE-014 stays
planned for a future explicit continuation.
