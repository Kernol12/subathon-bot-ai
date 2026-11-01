# STORAGE-012 — independent code and configuration review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01; different from source
author `/root/debt_code_reviewer`. Dedicated branch `task/STORAGE-012-review`,
worktree `local-evidence/worktrees/storage012-review`, from claim `49719cc`
after merged PR33/main `ae57f2b`. Own only this evidence file.

## Actual source reviewed

Reviewed author `a1142253db276f83f6c5e42ea99a51671038b8ae`, root integration
`41ffb5a`, source blob `c96dae927f652088d2326e427740975d18ba4df2` in
[sqlite_retained_context.cpp](../../server/src/sqlite_retained_context.cpp).
Read author evidence commit `b8f2577688b13d48103768f4cb0fb9f068608702` directly
through Git before its root integration. Source inspection found no defect;
the combined code/config verdict below includes the actual shared CI promotion.

Read root/server AGENTS, the [brief](../tasks/EPIC-009/STORAGE-012.md),
[retention contract](../contracts/retained-context-v1.md), actual public API,
existing work/session and migration contracts/source, and the
[merged baseline handoff](storagetest-012-handoff.md) and
[independent baseline review](storagetest-012-review.md).
Actual reviewed/red baseline is integrated before claim49719cc and implementation;
no historical red was repeated or compiled stub treated as implemented behavior.

Checked the complete 181-line actual source beyond the passing assertions:

- Pure checks enforce positive limits, known kind, nonempty bounded key and then
  required document/link validity in the frozen order. Invalid input precedes
  any capability/SQL operation and never cleans up earlier caller work. Positive
  qsizetype bounds are checked before reference size conversion.
- Keys and optional links copy exact std::string lengths into BLOBs, without text
  decoding, normalization or numeric conversion. Documents copy their full raw
  byte length; null input is invalid and present empty remains non-null empty.
  Owned copies protect returned values and synchronous insertion from borrowed
  Qt backing storage within the existing caller lifetime precondition.
- Fixed main-qualified SELECTs return value plus actual SQLite typeof/length.
  Shape/cardinality, BLOB type/length, exact nonempty key and optional NULL/BLOB
  link are checked. SQL zero-byte BLOB normalizes to non-null empty regardless
  of the driver's empty-buffer representation. Required NULL/mistyped values
  are not repaired or conflated with missing; every returned field owns bytes.
- Existing selected context and its stored known configuration target validate
  before equality. Configuration lookup cannot resolve through the context
  namespace. Recursion terminates at configuration rows, which have no links.
  Healthy duplicate compares complete key/document/link presence and bytes;
  conflict precedes a proposed replacement-target lookup. Only a new identity
  resolves its candidate link. Missing target/conflict perform no writes or
  exceptional cleanup; ordinary INSERT preserves uniqueness/FK enforcement.
- Well-typed selected fields above limits use retention result_limit; malformed
  rows/dangling stored links use invalid_stored_data. Both invoke real session
  cleanup before claiming known rollback, and cleanup uncertainty overrides the
  local reason. Underlying SQL/capability/budget SqliteWorkFailure exceptions
  are not translated or caught as a domain result. The live selected-data path
  reaches cleanup only after successful owner-session SQL; foreign/stale calls
  reject at that SQL boundary. No unrelated-row audit is performed.
- Every runtime statement uses the supplied revocable session and therefore its
  cumulative statement/binding/row/value budgets, owner/phase checks and sticky
  failure rules. There is no raw Qt SQL handle, private connection/thread, new
  BEGIN/COMMIT, lazy migration, nested wait, retry or publication callback. Facade
  constructor/destructor and ordinary caller transaction policy are unchanged.
- The pure factory returns exact migration1/retain-config-context-v1, ordered two
  DDL statements, complete seven-object projection including three NULL-SQL
  indexes, catalog defaults and starts `[0,1]`. Its ordinary owned Qt values do
  not install or mutate a database or grant OPS-02 upgrade authority. The frozen
  payload/digest assertion observes full2000 bytes/SHA0906f121…91f5e; production
  reads no test fixture or generates its own expected result.

This is opaque retention, not format parsing, evaluator/resolver, authorization,
mutable current settings or a typed uint64 SQL codec. Existing core reference
types and later actual ReceiptId/ContributionId/CapturedPlanRevision numeric
codec/range/zero gates in013/014/015 remain intact.

## Independent actual execution and invariants

Host Linux x86-64, GNU16.2.1, Qt6.12.0/QSQLITE SQLite3.54.0. Commands ran in
this isolated review worktree against actual sourcec96, using only synthetic
disposable state:

| Command/check | Actual result |
| --- | --- |
| `cmake --preset sqlite-retained-debug` | Exit0 |
| `cmake --build --preset sqlite-retained-debug -j2` | Exit0; all46 compile/AUTOMOC/link steps, no warnings |
| `ctest --preset sqlite-retained-debug -L sqlite-retained --output-on-failure` | Exit0; real16/16 pass,0.10s |
| `ctest --preset sqlite-retained-debug --output-on-failure` | Exit0; real95/95 pass,2.35s, including retention16, old73 and infrastructure6 with both diagnostic executables |
| CTest JSON versus each compiled executable's `-functions` | Exact95 entries and nonduplicate16/24/24/25 cases; environment7 and3 unique slots |
| Focused existing-writer `server_sqlite_retained_context_environment emptyNullAndOpaqueBindingsRemainDistinct` | Exit0; runtime Qt6.12/SQLite3.54, one case plus init/cleanup =3 passes,0skips |
| `clang-format --dry-run --Werror server/src/sqlite_retained_context.cpp`; `git diff --check` | Exit0 |
| `python3 tools/check_project.py`; `python3 tools/progress.py render --check` | Exit0;391 records/122 decisions/current snapshot |

The first post-run verification script expected runtime text in LastTest.log
after CTest discovery had replaced that file. Its bookkeeping assertion was
corrected to check registration/blob invariants separately; the focused existing-
writer case above provided the explicit host runtime observation. No product
failure, baseline change or historical red was inferred from that script error.

Exact preserved blobs against claim49719cc:

| Artifact | Git blob |
| --- | --- |
| Retention API | 55c82815986052b996a46c408ab616e68815a4f2 |
| Retention contract | 5b0ee84acbbf1a3ce8dbc57f99c110f3073d353b |
| Reviewed retention16 | c0e54003802f63a6f0393250f3bcca58cea8ee04 |
| Green fixture3 | aaa1644771decdb722b1105d220bac5313d54c6e |
| Frozen vectors | 0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8 |
| Existing writer/work | 023451987e40a9330fef91c6c0955f0e527f14e6 |
| Existing migrations | 4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f |

`git diff --exit-code 49719cc..HEAD -- core server/include tests docs/contracts`
passed at source41ffb5a. No assertion/interface/fixture/contract edit obtained
the green result. Shared CI promotion and applicable official runtime evidence
are reviewed separately, not assumed from the old baseline's green CI/red.

## Actual shared configuration and verdict

Verdict: **approved actual source/configuration for integration**, with observed
official acceptance and latest-head publication gates still required. No unresolved
source finding. Approval covers sourcec96 and actual shared commit
`caeb60de727ed87e6ef5f0f8532ed5acf3b55298`, normally fast-forward merged here,
plus author evidence integrated as `4b3d116` (current evidence blob
`e81c87e654ac6a37f46f6e660ed94926f4c1e3d0`). This is a different review from
the completed test-baseline approval; it is not inferred from build success.

Reviewed all four promotion files. Root CMake and preset changes update
descriptions only. Qt CMake retains all sixteen exact baseline names, executable
commands, limits and assertions, removes only the draft label and updates its
comment. Workflow removes the optional manual input and condition and runs
`ctest --preset sqlite-retained-debug -L sqlite-retained` unconditionally after
old writer/work/migration bodies, required environment7/environment3 and exact
compiled registration checks. It retains action/archive pins, read-only
permissions, check names, dependencies and preset flags. No failure inversion,
blanket continuation or weakened acceptance is introduced.

`/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml`
passes. Reconfigured actual promoted CMake, then inspected CTest JSON:95 entries,
all sixteen retention cases retain ordinary acceptance/qt/server/sqlite-retained
labels without draft. The seven exact frozen artifacts above still match49719cc.
Source remains the identical c96 tested by independent16/full95. No native suite
was repeated for description/label/workflow-only changes. Project391/render/diff
checks pass after normal integration.

At this verdict actual new automatic official-Qt execution is still pending;
neither previous baseline CI nor host3.54 passing acceptance proves it. Root must
observe new16 plus old73 and diagnostics on the selected official bundle, integrate
this source/config review, and verify current metadata/latest-head checks,
discussions/rules before normal merge. No future revision is approved here.

Mechanism scope remains immutable opaque document/reference/link retention and a
pure reviewed migration description. Caller validation, BEGIN/real COMMIT/normal
terminal publication and operational migration authority remain actual obligations;
no standalone receipt adapter, format/evaluator, typed numeric converter, domain
application bundle, backup/update/recovery or power-loss/platform/provider proof
is supplied. All seven integrations, three FX providers and full release scope
remain required. No live action occurred.
