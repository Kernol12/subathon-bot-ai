# CONTRACT-004 writer-owned service transaction boundary

Coordinator `/root`, author `/root/storage_decomposition`, 2026-11-01, task branch
`task/CONTRACT-004-writer-transactions` from merged main1259cfb after PR27.
Specification v0.45 §2/§3, DB-01/SERVER-01; internal writer-transactions revision1.
[Contract](../contracts/writer-transactions-v1.md).

Authorf68fd54 integrated8acbc0a supplies contractblob
`4ba20506db9a858450099b82888a22c8f6dbd761`, evidenceab423c4c. It defines additive
trusted queued work, same private authoritative connection, revocable owned-value
capabilities, one real synchronous BEGIN/COMMIT attempt, rollback/outcome/error
precedence, pre/postcommit exceptions, cumulative SQL budgets and mixed FIFO/drain.
Actual service commit is never deferred to an outer vote. Closure ownership
preconditions are explicit; old deep-owned batch inputs/25 tests are not weakened.
Twenty WT cases are future independently derived oracles, not authored tests.

Root40fbddf supplies proposed [ADR0002](../decisions/0002-storage-executable-slices.md),
paired planned010–015 and future STORAGECONTRACT001, preserving original002 IDs,
kinds, consumers and a REAL final missing-bundle baseline. Migration/integrity,
retained config/context, receipts, contributions, plans and final atomic application
bundle all remain required.391records validate without cycles; no aggregate-red
shortcut, new provider fact or final SQL/wire/domain format is invented.

Author/root project/render/diff checks pass; root tooling19 and DOM simulation pass
on the full graph. One stale shared claim snapshot used an unrecognized structured
subsection ref; it was corrected centrally to §3 and the author ff-refreshed shared
state before final checks. No tooling gate was changed and this is not feature red.

Independent `/root/writer_transaction_reviewer` is examining actual8acbc0a in an
isolated worktree; final approval and actual GitHub checks are pending. This task
changes documentation/planning only, no C++/test/schema/migration/workflow behavior.
Existing automatic suites apply and no new executable CI job is required. No local
native/probe/provider/platform run is claimed. Existing test suites do not establish
that the new transaction mechanism is implemented.

After reviewed contract integration, STORAGETEST010 must still refine public test
interfaces, author actual Qt/SQL cases, obtain a separate review and observe intended
missing-behavior red. Only then may STORAGE010 implement; later repository slices
retain their own gates. Full release scope and all seven integrations/three rate
providers remain planned/partially verified as recorded, not completed by this task.
