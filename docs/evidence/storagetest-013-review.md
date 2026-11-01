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

## Actual API, explicit stubs and compile/CI harness review

Verdict: **approved for new receipt test/fixture authoring only**. No new
receipt behavior, executable test baseline, selected13-object fixture proof or
intended red is approved. No concrete API/config finding remains.

Reviewed author `21a396b94f185890118adbb8c2cbef8de8a2a052`, integrated root
`54ebbd8`; minimal harness `50c1e8b23dfb5f79514379536429a7454cf8eaec`;
workflow/profile revision `b37daad74940b32439972d9c4affd52bf41ef4b7` and actual
root `a152a82ed66413c73e99a5fbe443eabdaf569528`, normally merged here.
Header blob `b43f9b6026498af18df22c2f695e76b23cb4064a`, source stub
`faf2856d8d61a91e518c50b3c0fdffe80a0f33a3`, logical contract
`e1ed975875e8b62d77d8efff67cfe5e8d5c95038`; physical companion2ef unchanged.
The approved f8 logical body is an exact byte prefix of the appended precision.

Actual declarations preserve context versus explicitly tagged active-session
adoption, noncopyable/nonmovable single-use repository values, copied weak tickets
and options, and construction-thread identity. Constructors only capture inert
values; destructor releases them without cleanup/SQL. The four source functions
unconditionally throw the distinct logic-error stub marker, which makes no false
known-rollback assertion about an adopted caller unit.

Precision correctly distinguishes ordered pure input errors, foreign-thread
rejection before mutable flags, local already_begun leaving the first unit usable,
and actual writer capability/sticky protocol errors. Adoption transfer is after
successful wrapper construction; context BEGIN allocation is exception-safe.
Transaction session/options/owner/pending state must outlive the repository
independently. Primitive preflight, known port failure categories and cleanup
uncertainty retain their agreed separation. Representable positive bounds reserve
checked find lookahead; supplied limits also bound retained-target documents.
Append uses per-receipt aggregate order checks, not find's history bound as a
write quota. No missing-reference/corruption normalization or commit vote is added.

Root's new flag, static target and sqlite-receipt-debug preset provide the existing
core/intake/writer/migration/retention dependencies and Qt include/link settings.
The actual workflow moves every configure/build/discovery/stable-body/executable
path to that preset, preserving pins, permissions, job name, guards and existing
lanes. No new receipt case is registered yet and no failure is skipped/inverted.

Actual independent checks in this worktree:

| Command/check | Result |
| --- | --- |
| `cmake --preset sqlite-receipt-debug`; `cmake --build --preset sqlite-receipt-debug -j2` | Exit0;48 build steps, including new stub archive, zero warnings; GNU16.2.1/Qt6.12 |
| CTest `--show-only=json-v1` plus suite-prefix counts | Exact95 old registrations, no new receipt feature case |
| `ctest --preset sqlite-receipt-debug --output-on-failure` | Exit0; old95/95 pass,2.26s; ignored capture build/sqlite-receipt-debug/reviewer-old95.log |
| Git prefix/hash/diff checks | Approved logical body exact; physical schema2ef and old core/service/ports/tests/retention/work/migrations unchanged |
| `clang-format --dry-run --Werror` on new header/source; actionlint1.7.12 on workflow | Exit0 |
| `python3 tools/check_project.py`; `python3 tools/progress.py render --check`; `git diff --check` | Exit0;391 records/122 decisions/current snapshot |

Also independently read actual ignored metadata/logs for selected official-Qt
[run41329002993](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41329002993)
at exacta152a82. They show completed SUCCESS, both archive hashes OK, GNU13.3,
new sqlite_receipts.cpp compilation/archive, Qt6.12/SQLite3.53.4, old25/work24/
migration24/retention16 and existing diagnostic7+3 passing with zero skips. This
is API compilation and existing-foundation evidence only: it proves no new
receipt method or new13-object fixture. No new receipt body or historical red
was invoked by this reviewer.

Next actual test/harness/oracle review remains mandatory. A single MAX-ordinal
row is corrupt noncontiguous history, not proof of reachable signed64 ordinal
exhaustion; constructing MAX contiguous rows is impractical. Future tests must
state that limit honestly and later code review must inspect the checked guard.
Independent full-prefix/typed-codec/schema fixtures, selected official-Qt green
proof and intended missing-behavior red still precede production. This verdict
does not transfer to absent tests, later source or a future CI head.
