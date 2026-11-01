# STORAGE-013 implementation evidence

Author: `/root/debt_code_reviewer`, explicitly assigned production implementation,
2026-11-01. Isolated `storage013-implementation` worktree, branch
`task/STORAGE-013-implementation`, from fresh claim
`4d10d6115eb31a6b4efca299e61cdc8897e24b47` after normal PR35 baseline merge
`a97e048b198a2293b58a25bd09d92518e95b0da5`.
Authority: specification v0.45 §§3.2–3.4/DB-01,
[brief](../tasks/EPIC-009/STORAGE-013.md),
[logical/API contract](../contracts/sqlite-receipts-v1.md) and
[approved exact physical migration](../contracts/sqlite-receipts-schema-v1.md).

## Scope, source and preserved baseline

Owned paths are only `server/src/sqlite_receipts.cpp` and this new evidence.
Root owns global schema/order, shared build/presets/workflow/progress/map/HTML/
checkpoint. Actual source-only candidate:
`d815b928134710400e16461fb69846cacebf66de`, source blob
`af095d97c255182c79d00a4b574e124f75734aad`. The explicit four-body stubs are
replaced in their existing source; no interface or other production file changed.

The [baseline handoff](storagetest-013-handoff.md),
[different actual full review](storagetest-013-review.md),
[selected official-Qt prerequisite](storagetest-013-ci.md) and
[original stub-red output](storagetest-013-red-ctest.txt) establish the prior gate.
The original22 red is complete and was not repeated on unchanged stubs. New bodies
were first executed here only after actual implementation changes. Frozen hashes
were checked against the claim revision:

| Artifact | Preserved blob |
| --- | --- |
| Public API | `b43f9b6026498af18df22c2f695e76b23cb4064a` |
| Logical/API precision contract | `e1ed975875e8b62d77d8efff67cfe5e8d5c95038` |
| Physical schema contract | `2ef4f1384c5c95c1bef5728b095ff5f4e7877e6a` |
| Reviewed receipt22 | `dab97946568e09f15119a8995735539eb2e14c4d` |
| Green environment3 | `8189f0274a38bb840c612673e0f858fe8d1777eb` |
| Independent JSON vectors | `a79e5e71a3b9ca101441852c7fb7f26f735e9626` |
| Existing writer/work source | `023451987e40a9330fef91c6c0955f0e527f14e6` |
| Existing migration source | `4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f` |
| Existing retention source | `c96dae927f652088d2326e427740975d18ba4df2` |

No fixture, assertion, core/service/port, header or contract was changed. Before
this evidence, the only path differing from the claim was the owned source.

## Implemented behavior and exception safety

The pure codec uses uint64 shifts/unsigned bytes for all0..MAX values and exactly
eight big-endian bytes. Null/wrong-sized decode buffers are input errors. Pure
zero remains valid; allocated and stored domain ReceiptId0 is rejected separately.
No ID passes through signed INTEGER/REAL, decimal or text decoding. The pure manifest
copies the existing immutable retention prefix1 and appends the exact approved
version2 SQL/order/ID/projection, with starts0/1/2. Actual tests compare both complete
payloads, including unchanged2000/SHA0906 prefix and10535/SHA24614 receipt step.
Production reads no fixture/JSON and implements no new catalog/adoption/repair rule.

A private transaction implementation owns copied weak session, fixed options,
immutable construction-owner identity and pending-first-attempt IDs independently
of repository lifetime. Options/input preflight precedes thread/session/state
access as specified. Foreign calls reject before mutable single-use/pending state;
local second port begin is already_begun and leaves the first transaction usable.
Actual context/session protocol errors remain actual SqliteWorkFailure values.

The disarmed wrapper is allocated before any context BEGIN or adoption transfer.
Context mode consumes its one real attempt and attaches the returned session with
a noexcept operation. Adopt mode performs no BEGIN/probe: successful allocation/
construction precedes transfer and the local consumed flag. Returning the owning
unique_ptr requires no further allocation. Construction failure cannot take caller
adoption authority or strand a begun session.

Create validates and resolves targets, reads allocator/greatest typed ID, checks
MAX before addition, reserves checked pending-vector capacity, and prepares both
original INSERT and allocator UPDATE with all owned bindings before the first
write. After both real execute calls succeed, pushing trivial uint64 into reserved
capacity cannot allocate. A guarded mutation scope additionally cleans active SQL
before rethrowing an unexpected allocation/runtime exception crossing execute calls;
uncertain cleanup overrides that exception. No caught failure can leave an
untracked original available for commit. Append prepares its whole statement before
mutation and removes a pending ID only after successful insertion, using trivial
nonallocating erase. Allocation exceptions do not manufacture known port errors.

Every new original must have a first append before commit. An incomplete commit
performs actual session.rollback before its known port failure: stale/terminal/
unknown tickets cannot become false known failures through inactive cleanup's
no-op. Ordinary commit calls the real synchronous COMMIT. Destruction calls only
existing noexcept cleanup; owner finalization remains the backstop and unknown
cleanup records the writer's actual fail-close outcome.

Reads select owned value/typeof/length triples through the existing private session.
They check row/cardinality, BLOB IDs/domain values, required identities/refs, exact
bytes, uint32 range, signed64 integer timestamps, optional-null grouping and known
dispositions. Empty source values bind non-null empty BLOB, optional absence binds
SQLNULL, and returned std::string/vector values own their bytes. No source decimal,
Unicode, scope, timestamp or unknown fact is normalized/inferred. Immutable originals
and every ordered observed attempt are returned independently; ReceiptIntake alone
retains created/retry/conflict policy. Original15 service cases remain unchanged
fake-backed simulation evidence, not relabelled SQL verification.

Incoming arrival targets resolve in the same session through the existing retention
facade using these same configured reference/value bounds. Stored target corruption
is distinct from a missing candidate; no arrival/context-known-config equality is
invented. Selected malformed data/limits establish known rollback before a port
failure; retention's already-known failures map without repair or repeated rollback.
Actual SQL/busy/budget errors retain their sticky underlying work category, and only
known-uncommitted categories map to ReceiptStorageError. Foreign/stale/terminal/
invalid-statement/protocol and unknown outcomes propagate unchanged.

Allocator validation reads all singleton metadata plus the raw greatest stored ID,
requires NULL iff no originals and otherwise exact equality, and checks MAX before
increment. No reset/search/reuse is added. Find fetches ordered count+1 lookahead
within representable limits, validates full returned history or fails without
truncation. Append uses one COUNT/MIN/MAX aggregate plus an invalid ordinal-type/
positivity count, validates contiguous order and checks signed64 MAX before max+1;
find's materialization bound is never a write quota. True MAX contiguous-history
exhaustion is a source-review guard, not a fabricated single-MAX-row runtime proof.
Every SQL/diagnostic/retention operation uses unchanged cumulative work budgets;
there is no new private writer API/handle/connection/thread/wait/retry or hidden
budget increase.

## Actual checks and remaining gates

Linux x86-64/GNU16.2.1/Qt6.12.0/SQLite3.54.0, CMake4.4.5, Ninja1.13.2,
Python3.14.8; synthetic disposable files only. Runtime lines from the full run's
three environment logs confirm host Qt/SQLite versions before discovery overwrote
the transient CTest log. Local captures are `/tmp/storage013-{configure,build,
new-tests,full-tests}.log` and `/tmp/storage013-discovery.json`.

- `cmake --preset sqlite-receipt-debug`: exit0.
- `cmake --build --preset sqlite-receipt-debug -j2`: exit0, zero warnings; actual
  source/library and all executables compiled. Final source refinement was rebuilt.
- `ctest --preset sqlite-receipt-debug -L sqlite-receipt --output-on-failure`:
  exit0,22/22 pass at final source. The first implemented22 also passed; final
  refinement explicitly uses rollback for an incomplete retained wrapper and was
  followed by fresh22/full118 verification.
- `ctest --preset sqlite-receipt-debug --output-on-failure`: exit0,118/118 pass:
  receipt22 plus previous stable96, including all three diagnostic environments.
- CTest JSON versus compiled `-functions`: exact22 receipt,16 retention,24 migration,
 24 work and25 writer names/counts match;118 total registrations. Environment
 executables expose exactly7/3/3 unique slots respectively.
- Exact frozen artifact checks above, formatting with
 `clang-format --dry-run --Werror server/src/sqlite_receipts.cpp` and
 `git diff --check`: pass.
- `python3 tools/check_project.py` and `python3 tools/progress.py render --check`:
 exit0,391 records/122 decisions/local links/presets/current root-owned HTML valid.

Initial local results retain the baseline draft label/manual receipt-CI separation;
every22 body actually ran through the commands above. Root owns unconditional
receipt CI promotion, different actual source/config review, selected official-Qt
CI and final latest-head metadata/discussions/current rules/normal merge gates.
Local passes alone do not finish STORAGE-013 or authorize merge. Caught allocation
safety and the mathematically reachable ordinal guard need actual source review;
no allocator/OOM fault-injection or disk/power-loss evidence is invented here.
Application bundle/effects/jobs, contribution/plan codecs and valuation rates,
provider/wire/evaluator meaning, backup/update/recovery authority, release-platform
and live gates remain separate requirements; all seven integrations/three FX
providers retain intended release scope.
