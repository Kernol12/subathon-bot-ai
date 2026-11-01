# ADR-0002 — Split storage repositories behind an explicit writer transaction contract

Date/status: 2026-11-01; accepted for contract/planning integration after independent review27ac83b.
Specification: v0.45 §2/§3.4/§16; DB-01, SERVER-01, OPS-02, NUM-01–04.

## Context

STORAGE-002 combined migrations and every repository family while several layouts
remain conceptual. More immediately, existing ReceiptRepository/ContributionRepository
ports synchronously begin/read/decide/write/commit and ReceiptIntake returns after
actual atomic commit. The current SqliteWriter exposes asynchronous prebuilt batches
and a private single connection. Separate batches split the decision transaction;
a second writer violates DB-01; an outer deferred commit cannot masquerade as the
service's synchronous commit success. This gap is an engineering prerequisite, not
a new provider capability or a reason to reduce release scope.

## Decision and alternatives

Define the documentation-only writer-owned work/session contract in CONTRACT-004,
then add real baseline/implementation pairs010–015: transaction integration,
migration catalog, retained configuration/context versions, receipts, contributions,
plan histories. Each retains its own contract/examples/review/red/code/pass gates.
Retained context precedes intake so durable arrival references have actual meaning.

After those slices, STORAGECONTRACT-001 defines only the internal application bundle
contract. Original STORAGETEST-002 then authors a real missing-bundle baseline and
STORAGE-002 implements it plus cross-record integration. They keep their original
kinds and downstream consumers. Do not introduce an aggregate-red shortcut or a
child dependency on STORAGE-002/ACCT-004 that creates a cycle.

Rejected alternatives: assign all repositories in one PR; expose/duplicate the SQL
writer; pump nested loops or fragment atomic decisions across async batches; claim
planned child scope or passing old tests as a new red baseline. No narrower product
scope or final unreviewed schema replaces the original package.

## Consequences and required preservation

Full uint64 receipt/contribution IDs and plan revisions need an explicit lossless
SQLite representation, never implicit signed64/REAL binding. Original monetary/rate
spellings, optional unknown facts and arbitrary-precision rational components must
round-trip. These are future reviewed codec/schema decisions, not a schema finalized
by this planning record. CONTRACT-003 still owns wire schemas; checkpoints, delivery
policies, provider verification and all release tasks retain their responsibilities.

No existing test, public service port or implemented behavior changes here. Original
writer25 and service simulation baselines remain valid and explicitly limited. New
suites must add real disposable-SQL integration and stable CI as they become executable.

## Evidence and review

Read-only investigation by /root/storage_decomposition inspected actual port headers,
service commit behavior, current private writer and dependency graph. Independent [CONTRACT-004 review](../evidence/contract-004-review.md) examined the
actual split, briefs/state edges and writer contract at8acbc0a. This accepts the
engineering sequence, not any implementation/test/observed-red capability.
