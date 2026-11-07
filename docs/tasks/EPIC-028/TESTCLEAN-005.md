# TESTCLEAN-005 — Remove repeated implementation-detail assertions

Module: Test infrastructure. Kind: cleanup. Specification: v0.45; §§17–18.
Contract revision: current shipped migration, codec and repository contracts.
Parent: EPIC-028. Depends on: TESTCLEAN-004. Shared-file ownership: affected Qt behavior
suites, migration tests and their fixtures.

## Scope

Replace private implementation pins with observable behavior where the specification
does not make them public or persistent contracts. Review duplicate schema object counts,
per-object DDL comparisons, repeated migration hashes, exact struct-derived byte bounds,
private error wording and literal service call-order lists. Keep one deliberate migration
freeze test where immutability of shipped migrations is required; preserve exact bytes
where a codec is explicitly persisted/public format.

## Acceptance criteria

- Each removed pin has a written rationale tied to the specification and is independently
  reviewed; no required migration immutability or persisted encoding is weakened.
- Boundary checks use clear under-limit/over-limit behavior and verify rollback where
  required, rather than coupling to private layout.
- Behavior suites assert observable state/results rather than private SQL shape, counts,
  messages or orchestration order unless those are contractual.
- Focused affected tests pass; retain old receipts and describe any changed baseline.
- Separate approved review is recorded before task completion.
