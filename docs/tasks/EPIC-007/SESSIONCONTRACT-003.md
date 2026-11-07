# SESSIONCONTRACT-003 — Concrete session lifecycle mutation and inspection contract

Parent: [EPIC-007](../EPIC-007.md). Kind: contract, document only.
Specification v0.45 §10/§16, AUTH-01/05/09/10/11/12/15, DB-01, TEST-04/05.
Dependencies/status: progress/state.json. Inputs: accepted supplier v1 §4,
transport v1 §3/4, password logical/API v1 and writer transaction v1 unchanged.

Define and independently review the missing internal lifecycle C++/port proposal,
literal examples and finite ownership/outcomes for SESSIONTEST-002. This task owns
new contract/example/author evidence documents only. The integrator owns shared
schemas/build/CI/state; no header, stub, tests, physical schema or implementation.

The contract must make issuance, token-only rotation, accepted-fresh-sign-in
reauthentication, logout/individual revocation and authorized self/admin inspection
concrete. Preserve absolute configurable positive U64 lifetimes and exact signed
expiry arithmetic, defaults, fresh-auth age, no sliding renewal, no overlap of old
and replacement digest, collision refusal, per-session independence, source-bound
revisions/epochs and current execution permission/disclosure. Include checked finite
inspection/page semantics without silently imposing a product session-count limit.

Define actual accepted-authentication and queue/transaction ownership requirements,
not a client boolean or password-helper success certificate. Separate upstream
local acceptance from downstream actual verified Twitch evidence under ACCESS-004;
both must bind current account/credential/installation state. No owner elevation
through issuance. Specify the synchronous writer work/session composition and
conditional updates, known rollback versus unknown/cleanup/post-commit failure,
normal-completion-only raw-token publication and reconciliation/orphan obligations.
Inspection never exports token/digest. Preserve transport errors/secret publication;
an unresolved HTTP route/body schema remains its owning HTTP task.

Supply independently derived race/failure examples: stale login/reset, simultaneous
rotation/revocation, original-digest queued use after commit, full-domain expiry,
collision, failed/unknown commit, committed response loss, unauthorized selected
session, changed permissions and account/installation epoch invalidation. Identify
the physical storage, real login/clock/permission producers still required; do not
declare them implemented by accepting a document.

Review actual API/limits/authority/publication examples separately from authorship.
Any new requirement conflict or substantial prerequisite must be recorded and split
before implementation. Completion provides only the contract for later reviewed
tests/red/service implementation; SESSION-002 retains all original scope and gates.
Record exact reviewed revision, literal audit, project checks and durable handoff.
