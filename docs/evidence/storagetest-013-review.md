# STORAGETEST-013 — independent review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01; different from logical
author `/root/writer_transaction_reviewer` and physical-proposal author `/root`.
Branch `task/STORAGETEST-013-review`, isolated worktree
`local-evidence/worktrees/storagetest013-review`, from claim `b263f60` after
merged PR34/main `a4bb565`. Own only this review evidence.

## Actual contract and physical proposal verdict

Verdict: **approved for minimal API/stub drafting, contract stage only**.
No API, test, executable baseline/red or future implementation is approved.
No blocking finding remains in these actual documents.

Reviewed author `0c154322d86789a1dbdf7e91927933b715ef6151` (initial logical
blob `c85cc8b966c6a799a04dbd4ed22ef4a56ced51f4`) and actual coordinated root
`845ae2012f9de9092e70e833b4169e2032f64d19`, fast-forward merged into the
clean review branch. Exact actual reviewed artifacts:

- [Logical contract](../contracts/sqlite-receipts-v1.md):
  `f8af1b99ca0348250ca24ff5ac98846f4066e55d`.
- [Physical proposal](../contracts/sqlite-receipts-schema-v1.md):
  `2ef4f1384c5c95c1bef5728b095ff5f4e7877e6a`.
- Root's actual STORAGE-013 brief, author evidence and schema-probe evidence at
  the same coordinated revision, including the money/rate scope reconciliation.

Read root/core/server/tests AGENTS, specification §§3.2–3.4/DB-01, brief,
identified receipt and event/state contracts, actual Receipt* values/service/
ports, all fifteen unchanged simulation cases and primitive fake, application-plan
zero policy, retained-context source/API and existing session/migration contracts.
Initial preparation and initial logical inspection were read-only. Approval waited
for the actual physical proposal rather than inferring an absent schema.

Preparation found a real cross-contract constraint: application-plans-v1 Capture
rejects zero receipt ID, enforced by application_plan.cpp line19 and its reviewed
test at line262. Root withdrew the tentative first0 proposal before any such
behavior was authored/approved. Actual contracts preserve pure uint64 codec
0..MAX while persisted/allocated ReceiptId stays1..MAX, with explicit NULL
allocator absence before first1. The fake's incidental first1 is not the authority.
No existing plan contract or assertion was changed to resolve the issue.

The logical contract preserves these required boundaries:

- ReceiptIntake alone classifies created/retry/conflict from all original facts.
  The repository performs primitive lookup/create/append/commit, with no upsert,
  fact normalization, new correlation or transport-based identity. Four exact
  BLOB identity components have matching database uniqueness.
- Immutable originals and every complete attempt preserve their own facts and
  arrival. A private positive signed64 ordinal orders repeated equal-time
  deliveries; it is not a new core ID or a timestamp uniqueness policy. New
  originals need an appended first attempt before commit, while an own staged
  original may be found before that append. History is complete or fails bounded
  materialization; it is never truncated or silently expired.
- Pure typed uint64 encoding is exactly eight unsigned big-endian bytes. Raw
  stored IDs require BLOB8/nonzero domain validation, with no signed INTEGER,
  REAL or text path. NULL allocator state is distinct from zero/corruption;
  allocator/greatest-original consistency, MAX once, no wrap/reuse and rollback
  of uncommitted allocation are explicit. Missing/malformed metadata is not reset.
- Optional strings and groups distinguish SQL NULL from present-empty BLOB and
  retain exact money/link spelling and bytes. uint32 schema versions and signed64
  UTC extrema are preserved without a new time/schema-zero rule. No source time,
  actor, confirmation or rate is invented. ReceiptFacts has no rate field;
  actual HistoricalValuation/rate provenance remains a mandatory later014/015 gate.
- Arrival configuration and context resolve independently in their typed retained
  namespaces. Retention validates a context's known configuration, but the adapter
  adds no unsupported equality with arrival.configuration_ref. Stored missing or
  malformed targets fail rather than enrich unknown or return fabricated records.
- Context mode uses the existing one real BEGIN. Explicit adoption grants the port
  wrapper real commit/cleanup authority over the already active unit, including
  earlier retention. Service success follows acknowledged physical commit; normal
  terminal work completion still controls publication. Repository values are inert,
  wrapper cleanup is nonthrowing and owner finalization remains a backstop. No new
  connection/queue/raw handle/nested wait/deferred vote/retry is authorized.
- Input errors precede primitive SQL and do not falsely claim known rollback or
  undo earlier caught caller writes. Actual local storage failure establishes
  known cleanup before ReceiptStorageError; mapped SQL/budget errors preserve
  existing outer WorkError/outcome. Actual foreign/stale/terminal errors remain
  SqliteWorkFailure, and uncertainty always outranks local reasons. All relevant
  lookups/results remain charged to cumulative session budgets without reset.

Reviewed all SR01–SR12 future oracles, including independent typed literal IDs,
zero/domain distinctions, exhaustion, equal-time histories, optional/value extrema,
reopen, adoption, uniqueness, post-stage flags, synchronous rollback lock probes,
bounds, capability lifetime and physical acknowledgment loss. They are proposed
oracles, not authored tests or observed results. Existing fifteen fake-backed cases
remain service simulation, not retroactive SQL proof.

## Exact additive migration and independent host diagnostics

Accepted contract proposal: version2 `identified-receipts-v1`, four exact ordered
SQL statements, supported starts `[0,1,2]`, unchanged format1/store identity and
bit-for-bit retention step1. The complete version2 projection has13 objects:
old7 plus allocator/original/attempt tables and three new generated indexes.
The allocator INTEGER PRIMARY KEY generates no index; all six total autoindexes
have NULL SQL. The fourth statement seeds exactly `(1,NULL)`.

The DDL covers every actual original/attempt field, BLOB8 domain IDs, four-part
UNIQUE, optional all-or-none BLOB pairs, signed INTEGER timestamps, uint32 range,
positive ordinals, known disposition values and typed retained/original FKs.
It neither introduces semantic crosslink equality nor relies on triggers to
sandbox arbitrary trusted SQL. Runtime structural/history/selected-type/bound/
allocator checks remain required; the schema alone does not establish port errors.
Migration1 is not rewritten and opening/constructing does not advance a database.
Whole-prefix atomicity and operational inspection/advance preconditions remain.

Independently executed an inline Python/sqlite3 in-memory diagnostic derived from
the actual Markdown's four SQL literals and the unchanged existing prefix JSON,
without a production factory/codec. Host SQLite3.54.0, actual results:

- Exactly13 schema tuples, exact stored CREATE SQL, six named NULL-SQL autoindexes,
  unchanged seven prefix objects and allocator singleton1/SQL NULL all match.
- Reconstructed complete F-framed payload from actual sorted UTF-8 object tuples:
  **10535 bytes**, SHA256
  `24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2`.
  No value from future product code supplied an expected result.
- A MAX-ID original and two equal-time attempts preserve signed64/u32 extrema,
  exact long decimal bytes, present-empty actor/connection/link groups versus NULL
  and independently resolvable differing configuration/context known link.
- Fourteen deliberate constraint violations fail: zero domain ID/allocator,
  empty identity, out-of-range schema values, noninteger timestamp, actor TEXT,
  partial optional groups, wrong-namespace FK, invalid ordinal/disposition and
  duplicate four-part identity under another ID. These are SQL-only checks,
  not implemented allocation, intake classification or rollback behavior.

Root's [Qt/private-writer probe](storagetest-013-probe.md) is separately attributed;
this reviewer read its evidence but did not rerun it. Neither host diagnostic
establishes selected official-Qt support or new feature acceptance/red.

## Checks and next actual gates

Actual reviewer checks: clean status and `git merge --ff-only 845ae20`, full
document/source/oracle inspection, the independent disposable SQL/framing program
above, existing-artifact diff/hash checks, project391/render and whitespace checks.
No native acceptance or historical red was repeated and no source/test/build/
schema file was authored by this reviewer. Only the review note changes here.

API precision is a separate actual review before test authoring. It must pin
representable positive options/checked lookahead, exact error precedence/categories,
owner-thread/single-use wrapper rules, local second-port-begin protocol policy
versus actual writer sticky errors, adoption transfer only after successful wrapper
construction, context-begin allocation exception cleanup, and transaction-owned
session/options/pending-first-attempt state independent of repository lifetime.
Append must not turn attempts_per_find into a retry quota; bounded per-receipt
count/min/max or equivalent can establish its order without materializing history.
These implementation/API details remain within the accepted logical contract;
no absent concrete declaration or behavior is approved here.

Next minimal interfaces/explicit stubs require their own actual review. Exact
literal full-prefix/schema/payload fixtures and typed-codec/SQL integration tests
then need different actual test/harness review, successful compilation, selected
official-Qt green environment proof and genuine intended missing-behavior red
before STORAGE-013 implementation. No provider, semantic wire/evaluator, complete
application bundle, backup/update/recovery/power-loss or platform gate is completed.
All seven integrations and three FX providers retain the agreed release scope.
