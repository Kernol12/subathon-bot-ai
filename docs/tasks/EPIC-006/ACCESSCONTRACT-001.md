# ACCESSCONTRACT-001 — Actual accepted-local-login authority contract

Parent: [EPIC-006](../EPIC-006.md). Kind: contract, document only.
Specification v0.45 §10, AUTH-01/05/08/09/10/11/12/15, SERVER-01, TEST-04/05.
Dependencies/status: progress/state.json. Import password logical/API v1, supplier
v1 §4, accepted lifecycle SESSIONCONTRACT-003, physical STORAGECONTRACT-002 and
session-clock SESSIONCONTRACT-004 contracts; existing account setup scope unchanged.

Own [local-login authority v1](../../contracts/local-login-authority-v1.md)
(identity `subathon.local-login-authority.v1`) and this brief's contract link.
Recover valid draft/review context from unpublished `53524fc`; fresh author
reconciles current imports, with current evidence in the handoff/eventual PR.
Publication/review PR link: pending coordinator. Add no files under docs/evidence.
The integrator owns shared schemas/build/CI/state. No implementation, header,
proof/protocol format, fixture/test source, provider action or session route is
authorized by this planning brief.

Define the actual current local credential reader and accepted-login owner, including
stable account binding, admission/finite secret lifetime and off-writer expensive
password verification. On authoritative queue return, recheck exact credential,
revision, account access, installation/epochs and actual login-attempt conditions
before acceptance/issuance eligibility. Helper success, account_ref, attempt_ref,
snapshot or caller flag must not certify acceptance. The actual boundary must work
before any account session exists; preserve owner local fallback and no owner
elevation. Setup/invitation/recovery policy remains ACCESS-001/005.

Make retained accepted-attempt scope/lifetime/invalidation and the real
`PasswordLoginSource` successful-login checks concrete. Preserve source-bound
post-login rehash ordering and conditional save against current exact verifier/
revision/accepted attempt; reset/revocation/stale results cannot overwrite new
credentials or retroactively become a wrong-password outcome. Define non-enumerating
failure and truthful storage/unavailable/unknown handling without inventing HTTP
schemas or blanket success retries. Expensive crypto never runs on the writer.

Supply independent acceptance-denial/race/failure examples: no account/access,
invalid verifier/verification failure, reset/account/epoch change during verification
or rehash, revoked/stale attempt, concurrent successful logins, resource refusal and
unknown commit. Name the reviewed freshness/clock and session lifecycle handoff,
including its recheck; no public transferable proof is implied by an internal result.

A different reviewer checks actual authority/API/secret/thread/rehash semantics.
Completion provides ACCESSTEST-008's contract only. ACCESS-004 keeps full local plus
optional verified Twitch login/linking, stable provider IDs, local fallback and fresh
login-method changes; downstream provider proof is separately tested there. No
SESSION-002/ACCESS-004/Twitch runtime dependency may be introduced upstream here.
Record reviewed revision, audits/checks and handoff.
