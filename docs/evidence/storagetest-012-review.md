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
