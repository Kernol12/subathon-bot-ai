# STORAGE-020 — Physical account/session store and current fact producers

Parent: [EPIC-009](../EPIC-009.md). Paired baseline: STORAGETEST-020.
Kinds: test_baseline / implementation. Specification v0.45 §10/§16,
AUTH-01/05/11/12, DB-01, SERVER-01, OPS-02, TEST-04/05.
Dependencies/status: progress/state.json. Contract: actual separately accepted
STORAGECONTRACT-002; this plan finalizes no API or schema.

## Scope and ownership

Implement only that physical account/local-credential/session/epoch store, additive
migration and actual bounded current readers/snapshot producer/conditional primitives.
Bind the existing current-session and password-source contracts on one caller-owned
actual Qt writer scope. Own focused server SQL/codec/tests paths agreed in the claim;
read root/server/tests instructions. Integrator owns schema/migration prefix, root
build/CI and state. Login decisions, accepted-attempt authority, token generation/
publication, permission evaluation and setup/recovery/provider policy remain owners.

## Baseline acceptance before production

STORAGETEST-020 builds real disposable Qt SQL tests against the reviewed physical
contract with independently specified rows and results. Cover exact types/digests/
times/epochs/verifier revisions, full selected linked closure and honest indexed
missing versus corruption/unavailable/resource refusal; complete per-value bounds
before materialization; coherent current snapshot and capability expiry; conditional
update losers, collision/unique constraints and atomic state transitions. Exercise
caller commit/whole-unit rollback, injected reached SQL errors and honest unknown
outcomes, restart/reopen and unchanged migration prefix/catalog. Prove lookup/query
bounds rather than passing only small datasets. No success-only fake SQL adapter
substitutes for this real-store baseline.

Obtain separate contract/interface/fixture/test reviews as applicable and record
an executed intended behavioral red with working dependencies/compilation. Immediate
unimplemented stubs/harness are allowed; production waits for the completed baseline.

## Implementation acceptance and limits

STORAGE-020 preserves all reviewed assertions and original receipts, passes real
selected Qt SQL acceptance and prior applicable writer/migration regressions, then
obtains separate source/resource/schema/configuration review and actual stable CI.
All returned writes remain provisional until actual caller commit and normal work
completion. No SQL handle or connection crosses thread/capability boundaries.

Passing the store does not accept a login, issue/revoke a service-authorized session,
implement setup/invitation/recovery/restore, or establish platform/live support.
Record exact commands/environment/results, reviewed revisions and next owner in one
handoff; use synthetic/redacted credentials and normal latest-head CI/PR merge gates.
