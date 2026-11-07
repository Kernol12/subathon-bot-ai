# STORAGECONTRACT-002 — Account, credential and current-session storage contract

Parent: [EPIC-009](EPIC-009.md). Kind: contract, document only.
Specification v0.45 §10/§16, AUTH-01/05/11/12, DB-01, SERVER-01, OPS-02.
Dependencies/status: progress/state.json. Import accepted supplier v1 §2/3,
password logical/API v1, writer/sqlite-writer v1 and migration catalog v1 unchanged;
use the separately reviewed SESSIONCONTRACT-003 lifecycle contract.

Own new physical/internal API proposal, literal fixtures/examples and author evidence
documents. The integrator owns migration ordering, shared schemas/build/CI/pins/state.
No production header, SQL migration, fixture source, stub or behavior is delivered.

Define only durable account/application-access/current-local-credential facts,
installation/account session epochs and hash-only session facts/revisions needed by
those contracts. Define actual consistent bounded current-session/password readers
and conditional writer primitives for the lifecycle/local-login owners. Preserve
existing setup/invitation, roles/permissions, recovery, provider-link and operational
owners; specify shared fact creation/update handoffs without absorbing their policy.
No raw session token or password column/diagnostic; verifier records are their
separate protected, source-bound credential facts.

Make exact SQL types/codecs, constraints, installation+digest index, unique session
identity, strict links/format validation, immutable fields, revisions/epoch updates
and coherent snapshot-reference production concrete through separate review. Define
current scope/lifetime on the existing caller-owned Qt writer session; no additional
writer/connection, nested transaction, history scan or cached authority. Keep full
signed-time/U64 semantics, bounded query/metadata/payload preflight before copies,
complete selected-fact projections, finite inspection and honest missing/invalid/
unavailable/resource/unknown classifications. Existing finite readers do not gain
new tables or authority implicitly.

Include additive migration/catalog compatibility with the then-current prefix and
explicit updates, no automatic startup migration. Preserve caller commit/RAII
rollback and unknown outcomes. Independently derive disposable-SQL fixtures and
examples for dangling/corrupt links, disabled/revoked/epoch-mismatched records,
duplicates/collisions, stale conditional updates, same-unit consistency, reopen,
rollback/commit failure and length-before-materialization/resource refusal.

The different review must verify the actual physical/API proposal against lifecycle,
password and writer obligations. Completion supplies STORAGETEST-020's contract,
not a working source, accepted login, permission decision or restore mechanism.
Record exact reviewed revision, compatibility implications/checks and handoff.
