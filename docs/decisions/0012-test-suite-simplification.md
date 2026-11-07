# ADR-0012 — Simplify test machinery without reducing product scope

Date: 7 November 2026. Status: owner-directed planning decision; each cleanup still
requires its own independent review before completion.
Authority: specification v0.45 §§17–18, baseline-integrity rules in `AGENTS.md`, and
the reviewed test-quality findings for the existing suites.

## Context

The checked-in tests are much larger than the current production surface and include
duplicated transaction scaffolding, environment/foundation programs, giant frozen
fixtures, and harness checks that do not exercise product behavior. At the same time,
most specification decisions remain untested. This imbalance adds agent and maintenance
work without establishing missing product capabilities.

The existing baseline-integrity rule protects reviewed assertions from silent removal.
The test-review findings are a proposal, not approval to delete tests; this record gives
future cleanup tasks a narrow, reviewable basis while preserving that rule.

## Decision

Keep the full specification and release scope. Prefer small, named, independently
derived behavior tests; keep one primary suite for shared mechanics; remove only test
machinery that a scoped task and a separate reviewer find redundant or unrelated to
specified behavior. Archive detailed historical receipts under git tag
`evidence-archive-2026-11`; use a concise, human-readable area summary as the current
record. Record any intentionally changed baseline. Never change expected behavior merely
to obtain a green suite.

Where schema checks are in scope, preserve the §18 purpose by validating every real
schema against valid and invalid examples with a selected standards-compliant validator.
Do not treat schema validation as authorization, provider support, or proof of the
runtime transport. A cleanup may supersede ADR-0009 or its native-process contract only
after a separate review establishes that the documented process boundary is not a
required capability; until then those accepted documents remain authoritative.

High-value missing behavior (crash/reopen, populated-database upgrades, corruption and
disk-full handling, authorization expiry/permissions, committed-reader visibility,
end-to-end application effects, and exact conservation properties) stays visible as
follow-up scope. Simplification does not claim that these behaviors are implemented.

## Alternatives

- Keep every existing assertion and harness: rejected because redundant machinery
  obscures behavioral gaps and increases maintenance without preserving unique product
  evidence.
- Delete broad test areas immediately: rejected because assertion deletion needs a
  task-specific scope, archived receipts, and an independent reviewer.
- Reduce the product/test requirements to match current coverage: rejected; the
  specification and all seven integrations and three FX providers remain in scope.

## Consequences and gates

EPIC-028 divides test and evidence cleanup into ten bounded tasks. Every completed
cleanup needs a separate approved review, evidence in its PR, and one state note of at
most 500 characters. Implementation tasks still require the appropriate test-first and
acceptance gates. This decision authorizes planning only; it is not an independent review
of any task or test suite.
