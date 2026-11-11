# APICONTRACT-002 — Bounded host authentication continuations

Parent: [EPIC-008](../EPIC-008.md). Kind: contract, document only.
Specification v0.45 §10, AUTH-01/08/10/11/12, SERVER-01, TEST-04/05.
Dependencies/status: progress/state.json. This is a planned prerequisite identified
by [local-login authority v1](../../contracts/local-login-authority-v1.md), not
an authorization to implement or extend the frozen API-001/APITEST-001 surface.

Own a separate host-extension contract and independent examples. Import existing
host lifecycle/consumer, account-ingress, writer, password and local-login contracts
without weakening them. Coordinator owns shared schema/build/CI/state. No code,
header, test body, provider call, public proof or route is part of this task.

Define one bounded original request association held across off-writer password
work, with finite response lease, cancellation/disconnect/stop and once-only final
publication. Define new per-stage work identities/admission/cumulative capacity,
queue deadlines and fresh execution/completion checks; an execution facade must
never survive a yield or worker wait. Preserve every actual writer outcome,
precommit current-installation check and current-source invalidation on uncertainty.
Do not reinterpret API-001's synchronous dispatcher as a continuation API.

Specify guarded named local-credential/conditional-upgrade operations and a narrow
directory port on the same writer/finalization path, never raw tickets or generic
callbacks. ACCESS-001 owns actual indexed directory facts and any separately
reviewed logical/physical extension; a missing producer refuses. Define a bounded
consumer read of the same host elapsed source and one installation-wide password
work permit for login/setup/reset, independently of any login owner or account
session. No reverse dependency on ACCESS-001/005/008 or SESSION-002/003 is allowed.

Bind SESSION-010's actual policy-generation producer and RequestIngressScope.
Define the exact pre-queue admission point after capture/guards and before any
additional stage copies. Audit every earlier ingress capture/host plaintext copy:
owner, count, bound, explicit erasure on transfer, refusal, cancellation and cleanup.
A release-only upstream contract is insufficient; propose any needed adapter
extension explicitly for separate review, preserving upstream baselines. No claim
that two login/helper buffers bound the entire ingress/host process is permitted.

Examples must cover late crypto, full/closed queues, elapsed failures and equality,
policy changes, disconnect/stop during stages, guard revocation, late completions,
unknown commit, teardown/drain, permit exclusion including setup/reset and each
secret-copy cleanup path. Reconcile future host hard-shutdown policy with native
crypto's non-cooperative lifetime; no hard deadline or safe forced-free claim.
Obtain the complete independent review cycle before APITEST-013. Record exact
reviewed revision and unresolved boundaries; no runtime capability is established.
