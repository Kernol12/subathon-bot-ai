# STORAGECONTRACT-001 — Internal application transaction bundle

Module: Operations. Kind: contract. Specification v0.45 §3.4/§16, DB-01, NUM-01–04.
Status/owner/dependencies: progress/state.json. Documentation-only future task.

After STORAGE-015, define versioned internal row/port examples for the atomic
application effect/state/job/operation-result/audit bundle, using retained receipt,
contribution and captured-plan references. Depend on CONTRACT-002, STORAGE-015,
ACCT-003 and CORE-005, never ACCT-004 or STORAGE-002 (which consume this contract).

Specify exact signed64 effects and lossless original/rational data, full-range IDs,
resolvable provenance and operation/reversal uniqueness. Distinguish already-durable
intake from a failed application: B08 must leave the receipt pending and no prepared
effect/state/job/application status or success result visible. Define concrete
examples/error cases before STORAGETEST-002 is authored and independently reviewed.

This is not CONTRACT-003 wire schema work, recovery checkpoint semantics, delivery
policy implementation or permission/rule evaluation. Do not finalize unknown fields
from a conceptual record table. Identify remaining questions explicitly, preserve
all release scope, obtain separate actual review and record appropriate documentation
CI checks. No code, schema implementation or feature-red claim belongs to this task.
