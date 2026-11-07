# SESSION-008 — Bounded token preparation and current-session resolution

Parent: [EPIC-007](../EPIC-007.md). Paired baseline: SESSIONTEST-008.
Kinds: test_baseline / implementation. Specification v0.45 §10, AUTH-05/12,
TEST-04/05. Dependencies/status: progress/state.json; planned only by ADR-0010.
Contract: accepted [supplier v1](../../contracts/session-resolution-v1.md) §1–3/5,
unchanged; this brief establishes no new contract or reviewed tests.

## Scope and ownership

Implement only the accepted `prepare_account_session` and
`resolve_current_account_session` component boundary. The future header is already
named by the accepted contract; actual minimal declarations/stubs require review.
Own focused server component/test paths agreed in the claim; the integrator owns
shared build, SDK selection, CI and state. Read root/server/tests instructions.
Fakes implement explicitly simulated source/clock facts, never real authority.
Issuance RNG, SQL/current-source implementation, lifecycle mutations, login,
transport and host queue admission remain downstream tasks.

## Baseline acceptance before production

SESSIONTEST-008 derives literal digest/time/error-priority cases independently
from the specification and supplier examples. Cover exact canonical 64-lowerhex
whole-input validation and length-before-scan; selected native initialization/hash
failure and scratch erasure; request/fact/result preflight and per-value/peak bounds;
source call count/variant handling and malformed linked facts; digest/installation/
account bindings; revoked/disabled/epoch mismatch before clock; clock unavailability,
rollback, expiry exclusivity, exact five-minute boundary and full signed-domain age.
Exercise source exceptions/allocation failures as required by the reviewed contract
without manufacturing successful crypto or using the SUT to generate expectations.

Record native readiness/source/scratch evidence separately from feature red, using
existing selected dependency/harness conventions. Minimal interfaces/harness/fakes
and immediate unimplemented behavior are allowed before tests; no feature body is.
Obtain a different test review, then a compiling, actually executed intended red
from explicit missing behavior. Record exact revisions/commands/environment/output.

## Implementation acceptance and limits

SESSION-008 starts only after SESSIONTEST-008 and its other graph prerequisites
are complete. Preserve its reviewed assertions and receipts; pass the relevant
component/regression suite, obtain separate source/resource/configuration review
and promote stable selected CI through the integrator. Prove source/clock ordering,
bounded ownership and secret lifetime beyond the happy-path vectors.

A resolved DTO/digest is not a durable authority proof. This component neither
issues sessions nor provides a real current SQL snapshot/clock or permissions.
Its passing suite completes neither SESSION-002 nor SESSION-003. Leave actual
platform/security/recovery/provider/release evidence open. Use synthetic secrets,
disposable state, one current handoff and the normal latest-head CI/PR merge gates.
