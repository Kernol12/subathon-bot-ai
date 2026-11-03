# STORAGE-017 — Typed early application-claim projection

Parent: [EPIC-009](../EPIC-009.md). Baseline STORAGETEST-017 → implementation
STORAGE-017. Specificationv0.45 §§3.4,5; DB-01, NUM-04. Status/owners/dependencies:
progress/state.json. Logical semantics: ACCTCONTRACT-001 after actual review.

## Actual gap and bounded scope

The existing bundle store exposes lookup by operation key; stage checks other
claims only AFTER a complete valid proposal's pure preflight. The application
service must detect existing stable subject/source claims before computing today's
new numeric proposal (which may now overflow/change) without raw SQL or dummy
proposals. Add a narrowly reviewed typed borrowed-session claim read over existing
operation/claim records, retaining exact full selected-result closure and existing
error/limit/provisional semantics. This finalizes no signature or codec.

Define/review its complete pure input, claim precedence and typed result shape,
including absence, same-key intent/command distinctions, original tagged subject,
same-kind source identity and any scoped reversal identity supported by the
reviewed logical contract. Resolve existing records with the same full closure as
lookup, never a fabricated successful result. An actual unknown/read failure is
not a duplicate success. No own writes/cleanup on healthy absence/refusal; known
storage failures must prove real whole-unit rollback and actual uncertainty stays
original WorkFailure. All normal data remains provisional until the actual caller
commits/completes. This storage port does not authorize historical-result access or
interpret opaque command documents; the service owns those separate obligations.

Do not change existing stage Intent equality, claim behavior, physical schema or
prior tests to accommodate the new reader. No migration is expected; root controls
shared CMake/CI/state and explicitly coordinates any genuinely necessary change.

## Gates and verification

STORAGETEST-017 authors the concrete additive contract/API and immediate missing-
behavior stub, independently derived real-SQL tests/frozen literals, different test
review, successful compilation/exact discovery and actual intended red. Existing
lookup/stage or old209 passing tests do not supply this reader's red. Preserve all
original assertions/fixtures and capability/protocol/unknown behavior.

STORAGE-017 then implements only this query extension, preserves reviewed tests,
runs relevant new/full regressions, obtains separate source/config review and
actual ordinary selected-Qt CI before normal integration. ACCTTEST-004 starts from
both integrated hold016 and claim017 prerequisites, retaining its own service
baseline/red. No current policy/authority/provider/UI/full-release proof follows.

Primary paths: server/include and server/src bundle extension, scoped tests/qt.
Read root/server/tests AGENTS. Synthetic disposable state only; record actual
artifact revisions/commands/environment/results and remaining release gates.
