# CONTRACT-004 — independent contract and storage-plan review

Reviewer: `/root/writer_transaction_reviewer`, 2026-11-01. Contract author:
`/root/storage_decomposition`; shared graph/brief/ADR author: `/root`.
Review took place in the isolated `task/CONTRACT-004-review` worktree at
`local-evidence/worktrees/contract004-review`. This reviewer authored only this
evidence file and did not author the contract, task graph, production code or tests.

## Reviewed revision and authority

Reviewed candidate: `8acbc0a84b4a2a306097398f5e7cf7f1fef3d7d3`, integrating
contract author commit `f68fd54` and shared decomposition commit `40fbddf`, against
main `1259cfb`. The review covers these actual Git blobs:

| Artifact | Reviewed blob |
| --- | --- |
| [Writer transactions revision 1](../contracts/writer-transactions-v1.md) | `4ba20506db9a858450099b82888a22c8f6dbd761` |
| [Author evidence](contract-004-author.md) | `ab423c4c7379347ffe553baa4330845c4671c359` |
| [ADR 0002](../decisions/0002-storage-executable-slices.md) | `965fd8158beaf74a7502fd346f6fd7a50a675ea4` |
| [CONTRACT-004 brief](../tasks/CONTRACT-004.md) | `6fc950a7d4ad54233348c5920853dc0b3d82a610` |
| [Task state](../../progress/state.json) | `724c3f1cbe735102881daa45e6e7f777ff114d05` |
| [Final STORAGE-002 brief](../tasks/EPIC-009/STORAGE-002.md) | `adfb05b26e50b5fb4150b5168780dd2c05bfcbbc` |

Authority and comparison material: root and server AGENTS instructions, agent
workflow, specification v0.45 §2 and §3.2–3.5 (DB-01/SERVER-01), the event/state,
SQLite writer, receipt-intake and contribution-eligibility contracts, actual
`SqliteWriter` interface/source, and the receipt/contribution repository ports and
services. All six new storage slice briefs, the storage-contract brief, EPIC-009
and task-index changes were inspected. This is an independent review of the
written boundary and plan; proposed behavior remains unimplemented.

## Contract assessment

No blocking finding. The additive work/session boundary can bridge the existing
synchronous repository ports onto the same authoritative Qt-controlled connection.
The concrete intake service commits before reporting success, and the new contract
requires real synchronous COMMIT before that return. A transaction is neither
fragmented across unrelated SQL batches nor deferred to an outer callback vote.
Lazy BEGIN permits validation before SQL; each work item has one transaction
attempt. A second begin is a protocol error, including after busy, rollback or
commit. No second writer, nested transaction, nested event-loop wait or SQL/native
handle escape is authorized.

Transaction outcome and work outcome are deliberately separate. Known commit
survives a later body exception or protocol error, with no normal result payload
and no automatic replay. A COMMIT error alone does not prove rollback; lost commit
acknowledgment and uncertain rollback produce an unknown outcome and fail-close
the writer. A known-uncommitted repository error requires confirmed rollback or
no transaction. The first sticky framework error wins over later body errors,
while uncertain durability takes precedence. Queued work behind a failed writer
completes in order without execution or replay. These rules support truthful
synchronous error mapping without inventing durability knowledge.

Ownable capabilities are revoked by item identity/generation, phase and writer
lifetime. Foreign-thread calls cannot touch SQL or mutate the owner transaction;
stale capabilities cannot poison a later item. Retaining a capability does not
retain the connection. The contract distinguishes safe retained capability values
from undefined use of dangling C++ references. Wrapper destruction is nonthrowing;
active owner cleanup rolls back, expired/terminal cleanup is a no-op, and unknown
cleanup outcome fail-closes. Returned rows own their data and expose no query or
driver handle.

The old batch API and its 25 existing cases remain applicable. Work and batches
share FIFO admission and the active-plus-queued limit of 64. An active slot lasts
until body return/finalization even when commit occurs earlier, so the next item
cannot run during that gap. SQL and binding limits accumulate across a session:
64 execute attempts, 65,536 SQL bytes, 1,048,576 bound bytes using the existing
eight-byte-plus-payload charge, 1,024 result rows and 1,048,576 result bytes.
Transaction-control statements do not silently consume the application's statement
budget. Existing scalar/SQL restrictions remain; migrations and integrity PRAGMA
extensions require later reviewed contracts.

Generic callable copying does not claim ownership of arbitrary captures. Trusted
callers must supply owned immutable inputs or valid lifetimes; the existing batch
API's deep-copy guarantee for SQL/bindings is preserved. Callback budgets do not
claim a hard SQL/C++ deadline. Trusted bodies cannot block on network, sleep, pump
nested event loops or wait for later work. Completion occurs outside the active
count, contains completion exceptions and permits writer destruction after the
terminal outcome without subsequent implementation access. Graceful shutdown
requires writer lifetime through all completions, closes admission, finishes the
active item, drains FIFO and closes SQL before shutdown completion.

All 20 WT examples were reviewed as future independent test oracles. They cover
mixed FIFO and adaptive read/write work, visibility immediately after real commit,
rollback and no-BEGIN paths, sticky SQL/protocol errors, busy/no replay, deferred
constraint commit failure, postcommit exceptions, lost commit acknowledgment,
uncertain rollback, revoked/foreign capabilities, exact and exceeded cumulative
budgets, capacity retention, shutdown, completion exceptions/destruction, owned
inputs/results and truthful synchronous repository errors. Their expected results
are coherent with the written contract and current service promises. They are not
executable tests, observed red evidence or proof of runtime behavior.

## Decomposition and scope assessment

The graph has 391 records. CONTRACT-004 precedes six genuine planned
test-baseline/implementation pairs, STORAGE-010 through STORAGE-015: transaction
sessions, migrations, immutable configuration/context versions, receipts,
contributions and plan history. Every implementation depends on its own baseline.
The chain establishes context retention before receipt persistence and plan
history before the final bundle. STORAGECONTRACT-001 follows STORAGE-015,
ACCT-003 and CORE-005 and precedes the original final STORAGE-002 pair.

The original STORAGETEST-002 remains a real test-baseline task and STORAGE-002
remains an implementation task. Their final scope still includes actual
application effects/state/jobs/operation results/audit and cross-record B08
integration. Prerequisite maps preserve migrations, lossless/provenance storage
and integrity obligations. No aggregate task is relabeled as an observed red.
The new children do not depend on STORAGE-002 or ACCT-004, and existing downstream
ACCT-004/ACCTTEST-004 dependencies are unchanged. The only changed dependency sets
among pre-existing records are EPIC-009, STORAGETEST-002 and STORAGE-002.

ADR 0002 and the briefs describe planned work, not implemented schemas. Unsigned
64-bit IDs/revisions cannot silently narrow to SQLite signed integers or REAL;
exact rationals, original monetary/rate spelling, optional unknown values and
historical meaning remain requirements for later reviewed representations. No
new schema, encoding, provider capability or business policy is invented here.
All seven integrations and all three rate providers remain in release scope.

## Independent verification

Observed environment: Linux `7.2.11-arch1-1` x86-64, Python `3.14.8`, Node
`v24.19.0`. Checks ran in the isolated reviewer worktree at the candidate above:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit 0; 391 records, 122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit 0; current snapshot |
| `python3 -m unittest discover -s tests/tooling -v` | Exit 0; 19 tests passed |
| `node tools/check_progress_ui.mjs` | Exit 0; execution/epic views, filters and theme DOM simulation passed |
| `git diff --check` | Exit 0 |
| Independent Python comparison of `origin/main` and candidate task graphs | Exit 0; record count, six real baseline/implementation pairs, required edges, unchanged downstream edges and absence of new STORAGE-002/ACCT-004 backedges asserted |
| `git diff origin/main --name-only -- server core tests schemas CMakeLists.txt CMakePresets.json .github` | Empty; no source, test, schema, build or workflow change |
| `git hash-object` on the six artifacts listed above | Matched the recorded reviewed blobs |

DOM simulation is not actual-browser, responsive, accessibility or OBS validation.
No native build/suite, SQL runtime, executable transaction tests, intended feature
red, live service/provider or platform verification ran in this documentation
review. Existing feature checks cannot prove the new proposed behavior.

## Verdict and remaining gates

**Approved for contract and planning integration at the exact reviewed revision.**
No blocking issue or user decision remains from this review. This approval covers
writer-transactions revision 1 and the associated executable storage decomposition;
it does not approve a future test baseline, implementation or product capability.

STORAGETEST-010 must still author the minimal interface/stub and focused,
specification-linked tests, obtain separate review and observe intended failures
before STORAGE-010 production implementation. Later slice contracts, tests,
schemas/codecs, migrations and repository adapters retain their own gates. Final
atomic bundles, recovery, live providers and platform verification remain open.
The coordinator must assess the final integrated revision, required latest-head
CI, unresolved discussions and the normal merge checklist before merging.

## Final completion-metadata review

The same independent reviewer approved final candidate
`d6be1d10847b11bd72bf452090d6bd6447d28fc2` on 2026-11-01. Normal merges into
the isolated reviewer branch preserved original approval `27ac83b`; before this
appendix the reviewer tree exactly matched that candidate. The contract blob,
author evidence, future executable slice briefs, all 391 task identities and every
dependency list are unchanged from the substantive review above. Every task's data
except CONTRACT-004 is unchanged. No source/header/test/schema/migration/build or
workflow change exists against main.

The completion changes accept ADR 0002 on the reviewed planning basis, mark only
the contract task done, link its independent evidence and describe the unimplemented
transaction boundary in architecture/handoff material. Runtime behavior, the new
test baseline and provider/platform verification remain explicitly `not_run`.
Neither existing passing suites nor the 20 future oracles are presented as proof of
the new transaction mechanism. The latest coordinator checkpoint accurately keeps
publication, final-head checks and normal merge gates open before STORAGETEST-010.
One stale present-tense paragraph in the task handoff was identified and corrected
in `d6be1d1`; it now records completed independent approval and the exact successful
CI head. There is no remaining contradictory latest-state claim.

Final metadata blobs inspected:

| Artifact | Reviewed blob |
| --- | --- |
| Task handoff | `9a503a0c0f049b9464fc4d894c923a3a3ce867f1` |
| Coordinator checkpoint | `0e3a453fe8a646a1fb77ecb514b14fef99aec0c8` |
| Task state | `f1789fbba5dea08ffb0ebbd2e2bd5cf816c91158` |
| Architecture | `8236bf58ef58514c14b3de2847ae0503c08cf860` |
| Accepted ADR 0002 | `2b4c14c7a9ea8096f24a0138fb5a7a076a67927f` |

Independent `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` passed at the
final candidate. An independent Python/Git comparison asserted the unchanged
contract/briefs/task dependencies, metadata-only scope, explicit unrun categories,
absence of source/test/schema/build/workflow changes and exact reviewer-tree
equivalence. Unchanged native suites were not repeated.

A live `gh pr view 28 --json ...` query independently observed
[PR 28](https://github.com/Kernol12/subathon-bot-ai/pull/28) OPEN/CLEAN at
`ca5f7a85b7b0cff4f61fe8a023d7bcc0baaf9eb1`, with all 11 check runs completed
SUCCESS and no GitHub reviews. This verifies the handoff's CI attribution; it does
not attribute those results to the later local candidate or this review appendix.
The repository's independent review is the recorded evidence above.

**Final verdict: approved for documentation/planning completion at `d6be1d1`.**
No blocking finding or user decision remains. Publish the integrated approval,
obtain successful required checks for that exact final PR head, inspect unresolved
discussions/current normal rules and satisfy the usual merge checklist before
merging. All future test-baseline, intended-red, implementation and release gates
remain in force.
