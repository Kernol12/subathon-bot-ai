# STORAGETEST-012 — retained context contract authoring

Author: `/root/writer_transaction_reviewer`, 2026-11-01. Different reviewer:
`/root/storage_work_code_reviewer`. Initial authorization: contract/evidence only.
Branch `task/STORAGETEST-012-author`, isolated worktree
`local-evidence/worktrees/storagetest012-author`, from claim
`036fad8d9c2da414ef92e69496b32311d0ade9b7` after merged PR32/main29f3818.
Own only [retained-context revision 1](../contracts/retained-context-v1.md) and
this note. Root owns briefs, actual schema/migration ordering, builds/workflows,
state/HTML and checkpoint. No support push/PR is authorized to this author.

## Scope, existing types and decisions

Read root/server/tests AGENTS, STORAGE-012 brief, specification §§3/6/7/DB-01,
event/state boundary, actual receipt/contribution ports and application-plan types/
contracts. Existing `ReceiptArrival.configuration_ref`/`modifier_context_ref` and
rule/modifier version refs are opaque std::string. No numeric configuration/context
version-key type exists. ReceiptId/ContributionId and CapturedPlanRevision have
actual uint64 fields in their later persistence slices.

Proposed minimal contract:

- Two immutable document kinds, each keyed by exact version-specific opaque bytes;
  BLOB storage avoids text/number normalization and Qt initial-BOM binding traps.
- Required present opaque document bytes, with present empty BLOB distinct from
  missing/null; actual SQLite typeof/length controls read interpretation.
- At most one optional typed context-to-configuration link. Missing stays absent;
  present targets must resolve in the configuration namespace, even when staged
  earlier in the same session. No guessed current values or unknown facts.
- Exact duplicate bytes plus link presence/value are idempotent; changed reuse
  conflicts without mutation. Primitive domain refusals do not write or claim
  earlier caller writes rolled back. The caller owns whole-command commit/abort.
- Existing active SqliteWorkSession composition, real commit/terminal publication,
  no independent connection/transaction/automatic migration or new writer API.
- Two small proposed BLOB tables with uniqueness and optional foreign key; root
  must accept actual DDL/order/fixtures before test freeze. No domain doc schema,
  evaluator, wire format, provider facts or authorization fields are invented.

The brief's uint64/codec language is reconciled explicitly, not removed. This
slice reviews/proves the exact BLOB byte codec and full-range decimal/binary
representations. It introduces no unused numeric ID/helper. Typed lossless numeric
codecs and zero policies remain gates in STORAGE-013/014 for actual ReceiptId/
ContributionId and STORAGE-015 for actual plan revisions. Root agreed to update
brief cross-references after actual contract review. Preserving numeric-looking
bytes here does not claim a uint64-to-SQL conversion was implemented or verified.

Twelve literal future examples cover empty/binary/BOM/full-range bytes, exact
duplicate/conflict, optional/missing/wrong-kind links, historical isolation,
same-session atomic caller composition, bounds, reopen and inherited uncertainty.
They are not authored executable tests or observed red.

## Verification and remaining gates

Inspected baseline source blobs: migrations
`4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f`, controlled writer
`023451987e40a9330fef91c6c0955f0e527f14e6`, migration contract
`cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61`. Preserve their valid reviews and
24 migration/49 writer-work/seven environment cases. No completed run/red was
repeated. No source/header/test/fixture/schema/build/workflow file changed here.

Documentation checks actually run in this author worktree:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit0; 391 records/122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit0; current root-owned snapshot |
| `git diff --check` | Exit0 |
| `git status --short` | Only the two owned Markdown paths untracked before commit |

The author normally refreshed shared root revision `3f40af2` before validation.
Its [existing-writer probe evidence](storagetest-012-probe.md) records actual host
Qt6.12/SQLite3.54 empty-BLOB/null/binary distinctions through unchanged session
SQL. This author read that evidence but did not independently rerun the probe;
it is not new retention, official-bundle or platform acceptance. The contract
keeps null input, present empty BLOB and absent optional reference separate and
requires actual stored typeof/length checks.

No new native build, test body, schema, baseline, feature behavior, live provider
or platform verification is claimed. Existing writer null/empty ownership behavior
is a compatibility boundary, not something this document silently changes.

Different-agent actual contract review and integrator acceptance of the proposed
metadata format come first. Only then may root assign minimal interfaces/stubs/
tests, followed by independent actual test review, compilation/intended red,
implementation, code review and applicable real acceptance/CI. Full application,
backup/recovery, formats/resolvers/evaluators, all integrations/rate providers and
release-platform gates remain mandatory. No user decision or access is identified.
