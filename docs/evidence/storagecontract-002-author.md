# STORAGECONTRACT-002 — document author packet

Author: fresh `/root/session_storage_contract_author`, 7 November 2026.
Worktree `/tmp/subathon-storagecontract002-author`, branch
`task/STORAGECONTRACT-002-author`, starting claim
`db09a6c942e6c039128bd3a711554fd9723126b2` from actual main
`e4d46d335e52850acc58a73f2500725cffb7c534` (lifecycle PR83 integrated).
Root reports subsequent normal SESSION-008 PR84 main `e440c7f`; the selected public
session/password ports remain unchanged. Integrator must recheck latest main on import.

Own ONLY this NEW author record and NEW
[physical/internal API proposal](../contracts/sqlite-account-sessions-v1.md).
Root owns state/HTML/shared migration order/schema/build/CI/pins/PR/integration.
No production header, migration SQL file, native source, fixture, test, stub,
dependency or runtime support is authored or changed. No subagents were spawned
by this author and no independent approval is claimed.

Final authored contract blob: `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1`.

Read root AGENTS/FIRST_AGENT_PROMPT, architecture/workflow/test strategy/current
progress/task brief/handoff, server AGENTS, specification v0.45 §10/16 and imported
contracts plus actual public supplier/password/writer/migration ports/current
manifest source/independent current fixture. This is a later bounded DOCUMENT task,
not a repeat of already completed first-assignment/historical test baselines.

The proposal covers:

- Four empty additive relations and one explicit page index (ten new schema
  objects, proposed total102 after actual prefix7's92); exact one-line DDL,
  BLOB/INTEGER codecs, FK/unique/CHECK rules, complete projection inventory.
- Separate immutable captured session epochs and current account/install epochs;
  recovery/restore fences require only selected current-row updates, no session
  scan/cascade rewrite or silent history mutation.
- Current session/local-credential reader records, complete hidden-format/revision/
  retention/link validation, metadata-before-payload ownership and aggregate budgets,
  explicit44-byte read-scope/ordinal snapshot references and finite BLOB keyset pages.
- Full expected-record/fence CAS and staged/stale/collision separation; caller-only
  begin/commit/RAII cleanup, unknown/sticky outcome truth, accepted-attempt reservation/
  consumption/known-rollback release and source-lineage conditional rehash handoff.
- Existing256-byte password account/revision/attempt and127-byte verifier profile
  reconciliation with wider generic refs; its fixed unavailable shape preserves
  truthful inability while typed internal corrupt/resource results stay distinct.
- Twenty-four independently derived AS01–24 SQL/logical examples, exact signed/U64
  arithmetic, literal H0 input and independent complete DTO/probe-budget arithmetic.

No new proof format, authentication helper subsystem, second connection/writer,
nested transaction, history scan, cache authority, provider capability or permission
policy is chosen. The already accepted libsodium random-byte primitive is proposed
for fixed32-byte NONSECRET identity/revision/read-scope candidates, with actual
current uniqueness/full CAS and honest probabilistic/historical ABA limits. Root
confirmed this lies within this identity/snapshot DOCUMENT scope; actual native
producer/source lifetime/freshness review remains required. No identity call was run.

## Imported exact blobs

All hashes are actual Git blobs read at this author's claim, not a reserialization.

| Path | Blob |
| --- | --- |
| docs/contracts/session-resolution-v1.md | c5f5a6e71664042e18475068d3caf577b0c8d009 |
| docs/contracts/session-lifecycle-v1.md | 48e3375bda594a39cca126ba14153d47664f0cdd |
| docs/contracts/session-transport-v1.md | 2c33c2bbe9052455107c3056783d6bb605a5fef2 |
| docs/contracts/password-hashing-v1.md | bc9ccbf66a41c9ae69abde4bd905401900e17461 |
| docs/contracts/password-hashing-api-v1.md | 7ad1e8d3f3311673dd368f2ef4d297de31dbafcd |
| docs/contracts/writer-transactions-v1.md | 4ba20506db9a858450099b82888a22c8f6dbd761 |
| docs/contracts/sqlite-writer-v1.md | 4d280461966f3fd9fcb13ab6d054c88228ca05b8 |
| docs/contracts/migration-catalog-v1.md | cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61 |
| server/include/subathon/server/session_resolution.hpp | 0c0f5d30647e525d6c9bd1d5daeb1b5288a3b5b8 |
| server/include/subathon/server/password_hashing.hpp | 6699670861bbfa8ab49ea0c3c24517665820601b |
| server/include/subathon/server/sqlite_writer.hpp | afc9f483da7d8e61a5e73715ca33819216e9e668 |
| server/include/subathon/server/sqlite_migrations.hpp | 02f83ce592b110176c1dac46f3e5ea4d93529fcd |
| server/src/sqlite_reference_capture.cpp | ccdc4f616529aedc310ffc75259a79f7690239b5 |
| tests/fixtures/sqlite_reference_capture_vectors.json | 878005d95a0c12d69df6b836ac09cd59ba5168ba |

Actual manifest source copies steps1–6 and appends immutable step7 with92 objects.
An independent Python decimal-length framing audit of EXISTING literal fixture7
reproduced71601 bytes/SHA256
`efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218`.
This audit calls no project encoder, SQL, future feature or native primitive.
The proposed version8/ID/order/full102 objects are subject to different actual
review and root allocation; this author creates no actual manifest/hash for step8.

## Actual limited checks and gates

Environment: existing Linux shared workspace; shell bash; Python3.14.9. Commands
run in the worktree above. No selected Qt/native runtime/build capability is inferred.

| Command/check | Observed result |
| --- | --- |
| `git status --short`, `git branch --show-current`, `git rev-parse HEAD` before editing | Clean task author branch atdb09a6c. |
| `git ls-files -s` for exact imported paths above | Actual blob identities recorded above. |
| `python3 --version` | Python3.14.9. |
| Inline `python3 - <<'PY'` pure document/independent fixture audit | PASS: exact five single-line no-semicolon SQL proposal blocks (4 TABLE/1 INDEX); all unique AS01–24; H0 length96; snapshot44/source55/B113/local152/password101/page240 and full MIN+U64MAX=MAX arithmetic; independently framed EXISTING manifest7 hash/length match. No SQL was executed. |
| `git diff --check` | PASS; no whitespace errors. |
| `python3 tools/check_project.py` | Exit0/PASS:443 task records,122 decisions, local links, presets and current HTML. This does not compile/run or approve draft feature tests. |
| `python3 -m unittest discover -s tests/tooling -v` | Exit0/PASS:19 tooling tests in0.372s, no product verification. |
| `python3 tools/progress.py render --check` | Exit0:Progress snapshot is current; no root-owned state/HTML edits. |

Not run: CMake/configure/build/CTest, future SQL statements, native crypto/identity,
new feature baselines or live operations. Running unreviewed future behavior would
violate this document-only stage. Existing tests/results cannot substitute for the
future independent STORAGETEST-020 baseline.

Next: root imports this coherent packet; a DIFFERENT actual reviewer checks exact
DDL/API/resource/lifetime/epoch/accepted-attempt/conditional-rehash examples against
all imported contracts. Resolve findings as one packet, retain actual review record,
then root performs normal documentation/task/PR/CI gates. Later concrete API/harness/
fixtures/tests require their own approval and observed intended red before STORAGE-020.
Actual local accepted-attempt owner, bridge/current producers, permissions/clock/
lifecycle publication, setup/invitation/recovery/provider/operations and all release/
platform/live obligations remain separate owners. No scope reduction is proposed.
