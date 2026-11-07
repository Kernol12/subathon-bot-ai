# ACCESS-008 — Accepted local login and current password-source integration

Parent: [EPIC-006](../EPIC-006.md). Paired baseline: ACCESSTEST-008.
Kinds: test_baseline / implementation. Specification v0.45 §10,
AUTH-01/05/08/11/12/15, SERVER-01, TEST-04/05.
Dependencies/status: progress/state.json. Contract: independently accepted
ACCESSCONTRACT-001; this brief finalizes no acceptance API/proof format.

Own the actual local-login acceptance owner, source-bound accepted-attempt lifetime,
real password credential/source integration and conditional upgrade save under the
reviewed contract. Use ACCESS-001-established accounts, actual STORAGE-020 facts,
SESSION-001 primitives and SESSION-009 clock. Read root/server/tests instructions;
agree focused source/test ownership before editing. Integrator owns shared schema/
build/CI/state. No session issuance, HTTP route or provider linking is included.

ACCESSTEST-008 authors independent literal outcome/race tests with disposable real
account/credential storage and controllable admitted verification completion. Cover
actual local acceptance without an existing session, current disabled/missing/
credential/revision/epoch changes before queue acceptance, wrong password, malformed
source, finite secret/admission failure and stale/revoked attempts. Exercise both
real password-source accepted-login checks and conditional rehash persistence,
including concurrent reset and unknown storage outcomes. Distinguish this actual
owner/store integration from password-primitive fake-source acceptance.

Use selected native crypto only after appropriate reviewed foundation/harness
readiness; reuse valid existing readiness receipts within their actual scope.
Obtain separate test approval and a compiling, executed intended red before feature
implementation. Minimal explicit stubs/harness/fakes do not establish acceptance.

ACCESS-008 preserves reviewed assertions, passes the relevant real integration and
affected password/store regressions, obtains separate authority/secret/thread/source/
configuration review and actual stable CI. Prove expensive crypto runs away from
the writer and current eligibility is rechecked on return; raw secret or helper
success never becomes an external authority certificate.

ACCESS-004 retains complete local/Twitch account login/linking and consumes this
upstream local component; no reverse dependency or claimed Twitch support. Session
generation/lifetimes/publication remain SESSION-002/003. Record exact commands,
environment/revisions/results, platform/live limits and next owners in one handoff;
normal latest-head CI/PR merge gates and all release requirements remain.
