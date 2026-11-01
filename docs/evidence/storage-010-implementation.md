# STORAGE-010 implementation evidence

Author: `/root/debt_code_reviewer`, explicitly reassigned as implementer by the
coordinator, 2026-11-01. Isolated `storage010-implementation` worktree, branch
`task/STORAGE-010-implementation`. Authority: specification v0.45 §2/§3.4,
DB-01/SERVER-01, [task brief](../tasks/EPIC-009/STORAGE-010.md),
[writer transaction work revision1](../contracts/writer-transactions-v1.md) and
[existing writer revision1](../contracts/sqlite-writer-v1.md).

## Scope and preserved baseline

Owned paths: `server/src/sqlite_writer.cpp` and this evidence only. The coordinator
owns shared build/preset/workflow/state/map/HTML/checkpoint changes. New source
commit `ad66ded737f60e66b6dae5bffc2989914011212e` is based on merged main `c60e1ed`
and the coordinator branch normally merged through `3b3c84f`; it includes shared
harness `ce0a325`, which removes the explicit work stub and promotes the reviewed
new cases to ordinary CI. Source blob:
`023451987e40a9330fef91c6c0955f0e527f14e6`.

The [STORAGETEST-010 handoff](storagetest-010-handoff.md),
[actual independent baseline review](storagetest-010-review.md) and
[original intended-red output](storagetest-010-red-ctest.txt) establish the prior
test-first gate. That approved red was not repeated. Public interfaces, contracts,
fault header and all existing test assertions are unchanged:

| Artifact | Preserved Git blob |
| --- | --- |
| Public writer/work header | `afc9f483da7d8e61a5e73715ca33819216e9e668` |
| Fault seam declaration | `a2cd7b8900848d8e7b7091b676aeaf16754efe0e` |
| New24 work tests | `eb9e3b600788f9473c5be7abdda86440cd73a3f4` |
| Old22 writer acceptance | `f8418e83dbd88d74505c047e6b3e2c30304ade69` |
| Old3 ownership regressions | `34c25c2786db42f6a4483fc874da9d4d3fc525a3` |
| Work contract revision1 | `4ba20506db9a858450099b82888a22c8f6dbd761` |
| Existing writer contract revision1 | `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |

## Actual implementation

A distinct private state per executing work owns its phase, first framework error,
begin-attempt flag and cumulative budgets. Public context/session values keep only
weak capabilities. Live operations compare immutable owner identity before reading
mutable state or touching SQL; foreign failures do not poison the owner. Finalizing
the item revokes its capability before completion. State retained transiently by a
foreign operation cannot keep the connection alive or reach a later item's state.

The existing private Qt SQL writer connection performs real synchronous BEGIN
IMMEDIATE, statements and terminal COMMIT/ROLLBACK. Commit returns only after the
physical SQL command succeeds and its acknowledgement is classified. Active SQL,
protocol and budget failures synchronously establish rollback before throwing a
known-uncommitted error. The first framework error survives caught subsequent
errors; uncertainty overrides all earlier failures and fail-closes admission and
the connection. Known commit remains committed through a later body exception or
terminal-session misuse. No outer transaction, deferred commit, SQL handle escape,
second writer, nested Qt wait or retry was added.

Noexcept cleanup only rolls back a live active owner transaction. Already terminal,
expired and default tickets are no-ops. Failure to establish rollback records
uncertainty; no redundant rollback is attempted. The narrowly declared test fault
is consumed only after the targeted physical successful new-work COMMIT/ROLLBACK,
before acknowledgement classification. Failed controls, inactive cleanup and old
batch controls do not consume it. It does not manufacture SQL execution, service
results or a final work result.

Old batches and new callable work share the existing FIFO, active-plus-queued
capacity, owner dispatch and graceful shutdown. An item retains its slot through
body return/finalization even if it committed earlier. The slot is released and the
next dispatch scheduled before user completion; no implementation state is accessed
after a completion that can destroy the writer. Completion exceptions remain
contained. Queued items behind fail-close receive their specified old/new failure
without invoking more SQL or a new work body.

Each execute validates and snapshots SQL/bindings and uses cumulative statement,
binding, row and result-byte budgets. Existing supported types, SQL allowlist,
semicolon rule and old deep-owned batch input remain intact. Returned rows are
owned after query/session/writer closure. Trusted callable captures and logical
results retain the contract's owning-input precondition; arbitrary closure memory
is not inspected or bounded.

## Commands and observed results

Linux x86-64, GNU C++16.2.1, Qt Core/Sql6.12.0, CMake4.4.5, Ninja1.13.2,
Python3.14.8; disposable synthetic SQLite fixtures only.

- `cmake --preset sqlite-work-debug`: exit0, configured with system Qt6.12.0.
- `cmake --build --preset sqlite-work-debug`: exit0; actual writer library, work
  executable, old acceptance and ownership executables built.
- `ctest --preset sqlite-work-debug -R '^sqlite-work\.' --output-on-failure`:
  exit0, all24 new cases passed before the final validation-exception containment
  refinement; the final full run below includes all24 again.
- `ctest --preset sqlite-work-debug --output-on-failure`: exit0 at source candidate
  `ad66ded`, 53/53 passed: new24, old25, infrastructure4. This includes actual
  ReceiptIntake synchronous service-port composition, known rollback and lost
  commit acknowledgement classification with independently observed physical rows.
- `ctest --preset sqlite-work-debug -L sqlite-work --show-only=json-v1` plus a
  Python comparison against the compiled executable's `-functions`: exact nonempty
  24 slots match, including duplicate/count checks. Equivalent grouping/check for
  `-L sqlite-writer`: exact25 slots in the two old executables match.
- `clang-format --dry-run --Werror server/src/sqlite_writer.cpp`: exit0.
- `git diff --check`: exit0.

A first exploratory regex `^SqliteWriterWork` selected zero tests and exited8;
actual registered names use `sqlite-work.`. This selection error is not a behavior
red or acceptance result. The corrected explicit24 and full53 runs above executed
all intended bodies. No tests, assertions, interfaces or fixtures were changed.

Independent actual source/config review and observed Ubuntu CI remain required;
local passing results alone do not finish STORAGE-010 or authorize merge. The
coordinator owns final status and latest-head CI/discussion/merge gates. This is an
internal transaction mechanism only: application adapters/schema/migrations,
recovery authorization, power-loss proof, platform and live provider gates remain.
No permission, contribution eligibility or full release capability is inferred.
