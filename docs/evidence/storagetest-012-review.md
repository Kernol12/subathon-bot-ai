# STORAGETEST-012 — independent review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01; different from author
`/root/writer_transaction_reviewer`. Dedicated branch
`task/STORAGETEST-012-review`, worktree
`local-evidence/worktrees/storagetest012-review`. Own only this review file.

## Actual contract, brief reconciliation and physical proposal

Verdict: **approved for subsequent interface/test authoring, contract stage only**.
No executable interface, test baseline, intended red, implementation or acceptance
is approved by this verdict.

Reviewed author commit `3cca59fd30b1fa1da918c2ed1705ad72fc06e454`
(initial contract blob `c53329e5686b8dd3fa171b71af7230ec4e7dc446`), integrated
as `8813dcc`, and actual coordinated candidate
`791cacf09cc018fb78b1c36736fc8f03f68be996`. The latter's
[contract](../contracts/retained-context-v1.md) blob is
`a884f01693ceda555779909e907d3c31bf19b6ef`; author evidence blob is
`39ac9dfccac18ccae3aed83e0ff9995eb007bbd8`.
This review includes root's actual migration proposal and the written
[012](../tasks/EPIC-009/STORAGE-012.md),
[013](../tasks/EPIC-009/STORAGE-013.md),
[014](../tasks/EPIC-009/STORAGE-014.md) and
[015](../tasks/EPIC-009/STORAGE-015.md) reconciliation, not an absent future design.

Read root/server/tests AGENTS, the brief, specification v0.45 §§3/6/7 and DB-01,
event/state, receipt-intake and captured-plan contracts and actual core/port
types, existing writer/work and migration contracts, author/probe evidence and
coordinated state/handoff. Preparation made no edits. Actual candidate was then
normally fast-forward merged from `task/STORAGETEST-012-retained-context`.

The following decisions preserve the specified boundary:

- Identity is kind plus exact nonempty version-reference bytes. Current
  configuration/context and modifier definition/instance references remain opaque
  strings. BLOB storage preserves lengths, NUL, non-UTF-8 and initial U+FEFF without
  claiming a product format accepts them. Documents are required present bytes;
  present empty BLOB, SQL NULL and a missing row remain distinct.
- Configuration and context namespaces are distinct. The one optional typed
  context-to-configuration link remains absent when unknown; a present target must
  resolve in the same session. A context-only target cannot satisfy it. Exact
  duplicates include payload and link presence/value; changed reuse conflicts
  without enriching or retargeting the original. Existing immutable content is
  compared before resolving a proposed replacement link.
- Primitives use the supplied active session and make no independent connection,
  transaction or commit. Same-session staged references compose with later intake.
  Domain refusal makes no primitive writes but does not undo earlier caller writes;
  the caller owns that atomic policy. Rows/status remain speculative until real
  commit and normal terminal work completion. SQL/budget/capability failures and
  malformed stored rows cannot become missing/default records or known-rollback
  claims without established cleanup. Uncertainty and no-retry rules remain intact.
- Exact two-table DDL, migration 1 ID `retain-config-context-v1`, configuration
  before context, starts `[0,1]` and complete seven-object projection are suitable
  for independent fixture freeze. The projection includes the two retained tables,
  two generated NULL-SQL indexes and all three unchanged catalog objects. Runtime
  operations qualify `main`; schema creation belongs only to reviewed migration.
  Existing history is not adopted, rewritten or repaired.
- Raw-byte bounds are positive configurable engineering limits, default 4096 for
  references and 262144 for documents; optional present references share the bound.
  Primitive input validation is distinct from remaining cumulative work budgets.
  Owned results survive closure; synchronous inputs remain immutable through each
  call and existing queued-capture ownership preconditions still apply.

Reviewed all RC01–RC12 literal future examples against these decisions: reopen,
empty/null/binary/Unicode distinctions, duplicate/conflict and missing/wrong-kind
links, original history, before/after-commit visibility, explicit rollback policy,
byte boundaries/cumulative failure and inherited terminal uncertainty. They remain
future oracles, not authored tests or observed results.

The numeric reconciliation satisfies this slice's brief without inventing a
numeric configuration key. Its exact BLOB codec must preserve decimal and binary
representations covering the complete uint64 range, including zero/max and distinct
spellings. That proves byte preservation only. Actual `ReceiptId`, `ContributionId`
and `CapturedPlanRevision` typed SQL codecs, raw representations above signed64,
range and domain-zero policies are explicit mandatory gates before their respective
013/014/015 persistence. Their original acceptance criteria and dependencies remain.
Opaque payloads still need later format/resolver work to supply historical meaning;
no rule/modifier evaluation, current pointer, provider fact or permission is inferred.

Before freezing executable assertions, actual API review must pin concrete
invalid-input/kind/positive-limit failure mapping and precedence, distinct domain
status versus storage/capability failure, and a present-empty returned document
representation that can be retained again as an exact duplicate. These are required
interface details within the approved contract, not an approval of future artifacts.

## Actual checks and remaining gates

| Command/check | Actual result |
| --- | --- |
| `git status --short`, then normal `git merge --no-edit task/STORAGETEST-012-retained-context` | Initially clean at `036fad8`; fast-forward to actual `791cacf` |
| `git diff --exit-code 036fad8..HEAD -- core server tests CMakeLists.txt CMakePresets.json .github/workflows docs/contracts/writer-transactions-v1.md docs/contracts/migration-catalog-v1.md` | Exit0; no implementation, interface, assertion, fixture or harness changes in the reviewed candidate |
| Independent Python `sqlite3` in-memory DDL/constraint consumer described below | Exit0, host SQLite3.54.0; exact proposed shape and byte constraints verified |
| `python3 tools/check_project.py` | Exit0; 391 task records, 122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit0; current |
| `git diff --check` | Exit0 |

The independent Python consumer extracted the two literal CREATE statements from
the actual Markdown, enabled foreign keys on its disposable connection, executed
both and read all `main.sqlite_schema` rows. It asserted exactly four objects,
exact original CREATE text, both documented generated index names and SQL NULL for
both indexes. It inserted BLOB key `00ffefbbbf61`, empty configuration document,
linked binary context and a separate absent-link context. Raw readback/type/length
assertions passed. Empty/text keys, NULL document and a missing configuration FK
each raised `sqlite3.IntegrityError`. This is independent host schema/constraint
evidence, not the new adapter, selected Qt bundle, feature acceptance or a red run.
Root's actual Qt6.12/hostSQLite3.54 probes are separately attributed in
[its evidence](storagetest-012-probe.md); this reviewer did not rerun them.

No native suite was repeated for this doc-only review. Existing passing suites and
historical red retain their original attribution. Root still must coordinate exact
manifest/projection/digest fixtures and observe selected official-Qt schema/byte
capabilities before new feature red. Actual minimal interfaces/stubs/tests require
their own independent review, successful compilation and intended missing-behavior
red before STORAGE-012 implementation. Code review, real acceptance and applicable
CI/publication gates then follow. Current task state correctly leaves those unrun.

Backup/recovery, actual caller validation/intake/reference resolution, document
formats/evaluators, authorization, platforms/packaging, all seven integrations and
three rate providers remain separate required work. No live operation occurred.

## Actual API, stub and minimal build precision review

Verdict: **approved for full test/probe/fixture authoring, API/stub stage only**.
This is a new actual review; the earlier contract verdict did not approve these
artifacts. It approves no future tests, feature red, production behavior or CI.

Reviewed author `6bfbb900a0171c16da10d50c8bfd882ded936791`, root integration
`a6a4402df6f209d2feaaf6927b56d818b45a6f3e`, and actual shared harness
`e5532d120d2cf4998243be676064db11f60e9096` after normal merges into this
review worktree (local reviewed merge `9e3e2cdf73882b90ff4a1b01d617ac7ecfb5106b`).
Exact blobs:

- [Header](../../server/include/subathon/server/sqlite_retained_context.hpp):
  `55c82815986052b996a46c408ab616e68815a4f2`.
- [Explicit stub](../../server/src/sqlite_retained_context.cpp):
  `b27eb48867df8a6dbc2c0c92da3eba0a80b11d76`.
- Amended contract: `5b0ee84acbbf1a3ce8dbc57f99c110f3073d353b`.

No blocking finding. Concrete key kind and std::string bytes preserve the approved
namespaces; the strong configuration-link wrapper and optional field preserve
typed absence. Required QByteArray input distinguishes null from present empty;
loaded empty BLOB normalizes to owned non-null empty so read/retain is idempotent.
The value facade holds only a copied revocable session and limits. Construction
and implicit destruction neither use SQL nor finalize the caller's transaction.
The factory and both methods are unconditional explicit feature-not-implemented
throws; no manifest, validation, codec or retention algorithm is hidden in them.

Actual precision resolves the previous interface gates. Before SQL, options,
kind, key bytes, document and link validation have ordered named errors even for
stale/foreign tickets. Valid inputs delegate original capability/SQL/budget errors.
Healthy selected existing records and stored known targets are validated before
immutable comparison; conflict precedes a proposed new-link lookup. A new record
alone resolves its candidate link for insertion. Malformed selected rows/dangling
stored links require confirmed rollback before invalid_stored_data; well-typed
selected fields above configured limits use the distinct retention result_limit.
Uncertain cleanup overrides through the existing SqliteWorkFailure category.
Normal domain refusal/input errors do not perform that exceptional cleanup or
claim earlier caller writes were undone. No global audit or private state/handle
escape is introduced. The supplied capability retains its established owner,
phase and lifetime checks.

Reviewed all three shared-file changes: root CMake enables the selected Qt/server
path for the new option; server CMake links the new static library to existing
migrations/writer/Core and project options; sqlite-retained-debug inherits the
existing migration/Qt lane and has matching build/test presets. No feature test
is registered yet. Existing tests, workflow and writer/migration source/interfaces
are unchanged. This minimal compile harness does not claim CI feature coverage.

| Actual reviewer command/check | Result |
| --- | --- |
| Python argument-vector `c++ -std=c++20 -fPIC -Iserver/include <pkg-config --cflags Qt6Core Qt6Sql> -fsyntax-only server/src/sqlite_retained_context.cpp` | Exit0, syntax only |
| `cmake --preset sqlite-retained-debug` | Exit0, GNU16.2.1, host Qt6.12.0 |
| `cmake --build --preset sqlite-retained-debug --target subathon_sqlite_retained_context -j2` | Exit0, all six dependency/stub compile/link steps, no warnings |
| `ctest --preset sqlite-retained-debug --show-only=json-v1`, parsed independently | Exit0; configured 78 old entries: scaffold5, migration24, work24, writer22, ownership3; no bodies executed |
| `git diff --exit-code 791cacf..HEAD -- core server/src/sqlite_writer.cpp server/src/sqlite_migrations.cpp server/include/subathon/server/sqlite_writer.hpp server/include/subathon/server/sqlite_migrations.hpp tests .github/workflows docs/contracts/migration-catalog-v1.md docs/contracts/writer-transactions-v1.md` | Exit0 |
| `python3 tools/check_project.py`; `python3 tools/progress.py render --check`; `git diff --check` | Exit0, 391 records/122 decisions/current snapshot |

The first discovery script used an overbroad substring check for `retained`, which
matched the old `retainedCapabilitiesCannotReachLaterWork` name. That reviewer
script assertion was corrected to exact registered suite-prefix counts; it was
neither a product failure nor red. Discovery here is configured registration only:
only the targeted libraries were built, not the 78 test executables.

No new factory/find/retain call, feature test/probe body or historical red was run.
Next artifacts require independently frozen schema/payload/digest oracles and
actual separate test/harness review; selected official-Qt capability/fixture
validation and genuine intended feature red remain required before production.
All earlier scope and external/platform gates remain.
