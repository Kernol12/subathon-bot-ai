# SESSIONCONTRACT-004 — Trustworthy current session-clock producer contract

Parent: [EPIC-007](../EPIC-007.md). Kind: contract, document only.
Specification v0.45 §10/§16, AUTH-05/09/12, DB-01, SERVER-01, TEST-04/05.
Dependencies/status: progress/state.json. Import supplier v1 §2/3, transport v1
publication-time requirements, lifecycle SESSIONCONTRACT-003 and physical
STORAGECONTRACT-002 unchanged after their independent acceptance.

Own a focused clock producer contract/examples/author evidence only. Define how
the actual host establishes a current UTC sample inside the caller's authoritative
execution/publication scope and refuses unresolved rollback/discontinuity/recovery
uncertainty. A supplied timestamp, fake clock, recovered checkpoint or caller
`trusted` flag is insufficient evidence. Do not choose a continuity heuristic in
this planning task or claim that a monotonic counter alone establishes UTC trust.

The later contract must name concrete runtime/OS observation and continuity-state
ownership, bounded sampling/failure behavior, startup/restart/recovery/restore fences,
when trust can be established or must be withheld, and serialized transitions with
installation/account state. State any durable facts/API/schema additions required
and their owner/review implications. Preserve full int64 UTC milliseconds, exact
expiry/freshness arithmetic and independent timer policies; never guess zero,
silently clamp/regress time, extend an existing maximum or make an outage a login
failure. Publication samples cannot bypass commit or revive an expired credential.

Supply independent examples for valid time, signed extremes, sampling failure,
wall-clock rollback/forward discontinuity, execution versus delayed publication,
restart with unavailable evidence, unchanged normal startup and restored state before
its installation-epoch fence. If a guarantee cannot be implemented from the selected
platform evidence, record the gap and explicit fail-closed behavior for review rather
than invent support. Actual ACCESS-005/STORAGE-007 fence protocols remain their own
baseline/implementation gates; no dependency on those downstream implementations.

A different reviewer must assess sources and actual trust/recovery/ownership
semantics. Completion supplies only SESSIONTEST-009's reviewed contract; no native
clock calls, tests, producer, countdown/recovery workflow or platform claim. Record
exact revision, literal/source audits, checks and durable handoff.
