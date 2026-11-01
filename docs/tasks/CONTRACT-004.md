# CONTRACT-004 — Writer-owned repository transaction work

Module: Operations. Kind: contract. Specification v0.45 §2, §3.4; DB-01, SERVER-01.
Authoritative owner/status/dependencies: progress/state.json.

## Problem and scope

The current SqliteWriter accepts prebuilt asynchronous SQL batches on its private
controlled connection. Existing ReceiptRepository and ContributionRepository expose
synchronous begin/read/decide/write/commit ports; ReceiptIntake returns only after
atomic commit. Define the internal writer-owned work/session boundary that composes
those promises without a second writer, fragmented batches, nested event-loop waits
or deferred commit presented as synchronous success.

Author owns ONLY docs/contracts/writer-transactions-v1.md and
 docs/evidence/contract-004-author.md in an isolated worktree. Coordinator owns this
brief, storage execution split/ADR, shared progress/HTML/checkpoint/architecture/PR.
Independent reviewer consumes the actual authored revision. No C++/schema/migration/
production behavior or executable/red-baseline claim belongs to this contract task.

## Acceptance criteria

- Specify ownership, admission/FIFO/resource budgets, owner-thread execution,
  scoped session lifetime and no raw Qt SQL handle escape. Preserve existing writer
  batch/read-only contracts and old25 cases; name additive compatibility boundaries.
- Specify dynamic SQL read/decision/write and actual begin/commit/rollback semantics
  for synchronous existing service ports. A service's commit cannot mean only a vote
  for a later commit. Publish externally only after the actual transaction outcome.
- Define callback exceptions before/after commit, known rollback versus uncertain
  durable outcomes, busy/no blind replay, fail-closed recovery and shutdown/drain.
  Prevent stale/foreign session use and unbounded or blocking untrusted work claims.
- Provide concrete examples/error cases and a future STORAGETEST-010 coverage map.
  Distinguish trusted closure/input ownership from the existing deep-owned SQL batch
  guarantee; do not promise to introspect arbitrary captured pointers.
- Keep DB-01 one controlled writer and CONTRACT-002 same authoritative Qt state
  queue. Final domain row layouts, wire contracts, provider behavior, migrations,
  actual repository adapters, live operations and release verification remain later.
- Independently review contract plus the necessary dependency-safe storage split.
  Assess CI: documentation/project/progress/tooling only; no feature behavior is
  implemented. STORAGETEST-010 still needs authored/reviewed tests and actual red.

Record exact reviewed artifacts/checks and remaining gates; publish one focused PR
and require latest checks/discussions/current normal merge rules before completion.
