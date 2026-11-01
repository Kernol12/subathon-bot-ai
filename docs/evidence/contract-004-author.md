# CONTRACT-004 — writer transaction contract authoring

Author: `/root/storage_decomposition`, 2026-11-01.
Branch `task/CONTRACT-004-author`, isolated worktree
`local-evidence/worktrees/contract004-author`, from claim3330013 and integrated
main1259cfb. Authority: specification v0.45 §2/§3.4, DB-01/SERVER-01 and
[task brief](../tasks/CONTRACT-004.md).

## Change and rationale

[Writer-owned repository work revision1](../contracts/writer-transactions-v1.md)
defines an additive trusted work/session boundary on the existing authoritative
Qt owner and private writer connection. It resolves the mismatch between existing
asynchronous fixed SQL batches and synchronous Receipt/Contribution transaction
ports, without opening another writer or presenting an outer commit vote as real
commit. The existing services/ports and SQLite writer source/interfaces/tests are
unchanged.

The contract defines lazy real BEGIN, one transaction attempt per item, synchronous
actual COMMIT/rollback, dynamic read–decide–write methods, owned speculative rows,
separate transaction/work outcomes and error precedence. It explicitly distinguishes
known-uncommitted repository errors from uncertain durable outcomes. A body exception
after confirmed commit cannot undo it or become an automatic retry. Unknown outcome
fail-closes admission/connection and completes queued items without executing them.

Capabilities are ownable but revocable by work generation, scope and writer lifetime;
foreign/stale calls cannot reach SQL or a later transaction. Mixed FIFO/admission,
cumulative existing SQL limits, completion/shutdown lifetimes and compatibility
with the original25 writer cases are stated. Trusted closure capture ownership is
a caller precondition, not something a generic callable copy can inspect or fix.
The old batch API's actual deep-owned SQL/binding guarantee remains intact.

Twenty WT examples map to future STORAGETEST-010 coverage, including independent
before/after-commit visibility, conditional writes, rollback, busy/no replay,
postcommit exceptions, lost acknowledgment, revoked/foreign capabilities, budgets
and shutdown. They are manually derived contract oracles, not authored executable
tests, observed red or implemented transaction behavior.

## Verification and CI assessment

This author changed only the contract and this evidence note. No C++, schema,
migration, dependency, build target, workflow or executable test was added. Existing
project/link/progress/tooling checks are appropriate; stable feature CI does not
implement or prove this new contract. The coordinator owns shared state/HTML,
storage decomposition/ADR/task briefs, final integration checks and the focused PR.

The first `python3 tools/check_project.py` invocation failed on the shared claim
snapshot's unrecognized structured specification reference `§3.4`, before assessing
the new contract. The coordinator corrected structured references to `§3` while
retaining precise prose citations. The author performed the authorized
`git merge --ff-only task/CONTRACT-004-writer-transactions` from3330013 to40fbddf;
no shared file was edited by this author. That documentation validation failure is
not an intended feature-red result.

After the shared refresh, author checks in the isolated worktree:

| Command | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit0;391 task records,122 decisions, local links/presets/current HTML |
| `python3 tools/progress.py render --check` | Exit0; current snapshot |
| `git diff --check` | Exit0 |
| `git status --short` | Only the two owned Markdown paths untracked before commit |

These checks were repeated after the final contract cleanup clarification. No
native suite, test body, database, live service or provider check ran. Independent
contract/dependency-split review remains required; this note is not self-approval.

## Handoff and remaining gates

Changed paths: `docs/contracts/writer-transactions-v1.md` and this file only.
No support branch is pushed. The next test author must implement a minimal versioned
boundary/stub and derive focused STORAGETEST-010 tests, obtain independent review,
then observe actual intended failures before STORAGE-010 production implementation.
No new red or acceptance result is claimed here.

Final domain row layouts, uint64 ID representation, exact money/rate storage,
wire schemas, actual repository adapters, migrations, business permissions/
deduplication/application atomicity, recovery, live providers and platform release
verification remain later gates. All seven integrations and three rate providers
remain required. SQL/callback budgets do not claim a hard execution deadline,
generic closure memory bound or power-loss outcome proof.
